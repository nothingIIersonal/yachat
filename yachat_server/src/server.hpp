#pragma once

#include <stdlib.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "auth.hpp"
#include "common.hpp"
#include "msg.hpp"
#include "packet.hpp"

namespace server {

class Server : public QTcpServer {
 public:
  explicit Server(quint16 port, QObject *parent = nullptr)
      : QTcpServer(parent) {
    connect(this, &QTcpServer::newConnection, this, &Server::onNewConnection_);

    if (!listen(QHostAddress::Any, port)) {
      common::logAll(QtFatalMsg,
                     "[SERVER | CONSTRUCTOR] Unable to start the server: " +
                         errorString());
      QCoreApplication::exit(EXIT_FAILURE);
    }
  }

 private slots:
  void onNewConnection_() {
    QTcpSocket *clientSocket = nextPendingConnection();

    common::logAll(QtDebugMsg, "[SERVER | ON NEW CONNECTION] " +
                                   clientSocket->localAddress().toString() +
                                   " connected");

    const auto socket_descriptor = clientSocket->socketDescriptor();
    sockets_.insert(socket_descriptor, clientSocket);

    connect(clientSocket, &QTcpSocket::readyRead, this,
            [=]() { processConnection_(clientSocket); });
    connect(clientSocket, &QTcpSocket::disconnected, this,
            [=]() { onDisconnection_(clientSocket, socket_descriptor); });
  }

  void processConnection_(QTcpSocket *clientSocket) {
    QByteArray requestData = clientSocket->readAll();
    QString requestString = QString::fromUtf8(requestData);
    QJsonDocument requestJson = QJsonDocument::fromJson(requestString.toUtf8());

    auto header_monad = packet::jsonExtractPacketHeader(requestJson);
    if (header_monad.error) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | PROCESS CONNECTION] Error parsing received JSON "
          "[header section]");
      return;
    }

    const auto header = header_monad.unwrap();
    const auto command = header.command;

    const auto response = processCommand_(command, std::move(requestJson),
                                          clientSocket->socketDescriptor());

    clientSocket->write(response.toJson(QJsonDocument::Indented));
    clientSocket->flush();
  }

  void onDisconnection_(QTcpSocket *clientSocket, qint16 socket_descriptor) {
    common::logAll(QtDebugMsg, "[SERVER | ON DISCONNECTION] " +
                                   clientSocket->localAddress().toString() +
                                   " disconnected");
    sockets_.remove(socket_descriptor);
    auth::forcedLogOutUser(socket_descriptor);
    clientSocket->disconnectFromHost();
    clientSocket->deleteLater();
  }

 private:
  QHash<qintptr, QTcpSocket *> sockets_;  // <socket descriptor, socket>

  QJsonDocument processCommand_(packet::packet_t::header_t::command_t command,
                                QJsonDocument &&packetData,
                                qintptr socketDescriptor) {
    QJsonDocument response;

    switch (command) {
      case packet::packet_t::header_t::command_t::REGISTER: {
        const auto auth_data = packet::jsonExtractAuthData(packetData);
        if (auth_data.error) {
          response =
              packet::StatusResponse(
                  packet::packet_t::header_t::command_t::STATUS,
                  packet::packet_t::header_t::status_t::FAIL,
                  "Error parsing received JSON on register [auth data section]")
                  .to_json();
          break;
        }

        if (commandRegister_(auth_data.data)) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::OK,
                         "Command 'register' completed")
                         .to_json();
        } else {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Command 'register' failed")
                         .to_json();
        }

        break;
      };
      case packet::packet_t::header_t::command_t::LOGIN: {
        const auto auth_data = packet::jsonExtractAuthData(packetData);
        if (auth_data.error) {
          response =
              packet::StatusResponse(
                  packet::packet_t::header_t::command_t::STATUS,
                  packet::packet_t::header_t::status_t::FAIL,
                  "Error parsing received JSON on login [auth data section]")
                  .to_json();
          break;
        }

        const auto session_id_monad =
            commandLogIn_(auth_data.data, socketDescriptor);
        if (!session_id_monad.error) {
          response = packet::AuthResponse(
                         packet::packet_t::header_t::command_t::AUTH,
                         packet::packet_t::header_t::status_t::OK,
                         "Command 'login' completed", session_id_monad.data)
                         .to_json();
        } else {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Command 'login' failed")
                         .to_json();
        }

        break;
      };
      case packet::packet_t::header_t::command_t::LOGOUT: {
        const auto auth_data = packet::jsonExtractAuthData(packetData);
        if (auth_data.error) {
          response =
              packet::StatusResponse(
                  packet::packet_t::header_t::command_t::STATUS,
                  packet::packet_t::header_t::status_t::FAIL,
                  "Error parsing received JSON on logout [auth data section]")
                  .to_json();
          break;
        }

        if (commandLogOut_(auth_data.data)) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::OK,
                         "Command 'logout' completed")
                         .to_json();
        } else {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Command 'logout' failed")
                         .to_json();
        }

        break;
      };
      case packet::packet_t::header_t::command_t::SENDMSG: {
        const auto auth_data = packet::jsonExtractAuthData(packetData);
        if (auth_data.error) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Error parsing received JSON on "
                         "send message [auth data section]")
                         .to_json();
          break;
        }

        const auto target_data = packet::jsonExtractTargetData(packetData);
        if (target_data.error) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Error parsing received JSON on send "
                         "message [target data section]")
                         .to_json();
          break;
        }

        if (commandSendMsg_(auth_data.data, target_data.data)) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::OK,
                         "Command 'sendmsg' completed")
                         .to_json();
          // send a "notify" packet to the target user
          sendNotify_(auth_data.data, target_data.data);
        } else {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Command 'sendmsg' failed")
                         .to_json();
        }

        break;
      };
      case packet::packet_t::header_t::command_t::GETMSGS: {
        const auto auth_data = packet::jsonExtractAuthData(packetData);
        if (auth_data.error) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Error parsing received JSON on "
                         "get messages [auth data section]")
                         .to_json();
          break;
        }

        const auto target_data = packet::jsonExtractTargetData(packetData);
        if (target_data.error) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Error parsing received JSON on get "
                         "messages [target data section]")
                         .to_json();
          break;
        }

        const auto target_monad =
            commandGetMsgs_(auth_data.data, target_data.data);
        if (!target_monad.error) {
          response = packet::MsgsResponse(
                         packet::packet_t::header_t::command_t::MSGS,
                         packet::packet_t::header_t::status_t::OK,
                         "Command 'getmsgs' completed",
                         target_monad.data.username, target_monad.data.messages)
                         .to_json();
        } else {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Command 'getmsgs' failed")
                         .to_json();
        }

        break;
      };
      case packet::packet_t::header_t::command_t::GETALLMSGS: {
        const auto auth_data = packet::jsonExtractAuthData(packetData);
        if (auth_data.error) {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Error parsing received JSON on "
                         "get messages [auth data section]")
                         .to_json();
          break;
        }

        const auto target_monad = commandGetAllMsgs_(auth_data.data);
        if (!target_monad.error) {
          response = packet::AllMsgsResponse(
                         packet::packet_t::header_t::command_t::ALLMSGS,
                         packet::packet_t::header_t::status_t::OK,
                         "Command 'getallmsgs' completed",
                         target_monad.data.all_messages)
                         .to_json();
        } else {
          response = packet::StatusResponse(
                         packet::packet_t::header_t::command_t::STATUS,
                         packet::packet_t::header_t::status_t::FAIL,
                         "Command 'getallmsgs' failed")
                         .to_json();
        }

        break;
      };
      case packet::packet_t::header_t::command_t::NOTIFY: {
        response = packet::StatusResponse(
                       packet::packet_t::header_t::command_t::STATUS,
                       packet::packet_t::header_t::status_t::OK,
                       "Command 'notify' not implemented")
                       .to_json();
        break;
      };
      default: {
        response =
            packet::StatusResponse(
                packet::packet_t::header_t::command_t::STATUS,
                packet::packet_t::header_t::status_t::FAIL, "Unknown command")
                .to_json();
        break;
      };
    }

    return response;
  }

  // returns success or failure
  bool commandRegister_(packet::packet_t::payload_t::auth_data_t auth_data) {
    const auto username = auth_data.username;
    if (username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | REGISTER] Can't parse required field [auth data "
          "section -> "
          "username]");
      return false;
    }

    const auto password = auth_data.password;
    if (password.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | REGISTER] Can't parse required field [auth data "
          "section -> "
          "password]");
      return false;
    }

    if (!auth::registerUser(username, password)) {
      common::logAll(QtDebugMsg, "[SERVER | REGISTER] Can't register");
      return false;
    }

    return true;
  }

  // returns session_id monad
  common::result_t<auth::session_id_t> commandLogIn_(
      packet::packet_t::payload_t::auth_data_t auth_data,
      qintptr socket_descriptor) {
    const auto username = auth_data.username;
    if (username.isEmpty()) {
      common::logAll(QtDebugMsg,
                     "[SERVER | LOG IN] Can't parse required field [auth data "
                     "section -> "
                     "username]");
      return {};
    }

    const auto password = auth_data.password;
    if (password.isEmpty()) {
      common::logAll(QtDebugMsg,
                     "[SERVER | LOG IN] Can't parse required field [auth data "
                     "section -> "
                     "password]");
      return {};
    }

    const auto session_id_monad =
        auth::logInUser(username, password, socket_descriptor);
    if (session_id_monad.error) {
      common::logAll(QtDebugMsg, "[SERVER | LOG IN] Can't log in");
      return {};
    }

    return session_id_monad;
  }

  // returns success or failure
  bool commandLogOut_(packet::packet_t::payload_t::auth_data_t auth_data) {
    const auto session_id = auth_data.session_id;
    if (session_id.isEmpty()) {
      common::logAll(QtDebugMsg,
                     "[SERVER | LOG OUT] Can't parse required field [auth data "
                     "section -> "
                     "session_id]");
      return false;
    }

    const auto username = auth_data.username;
    if (username.isEmpty()) {
      common::logAll(QtDebugMsg,
                     "[SERVER | LOG OUT] Can't parse required field [auth data "
                     "section -> "
                     "username]");
      return false;
    }

    if (!auth::logOutUser(session_id, username)) {
      common::logAll(QtDebugMsg, "[SERVER | LOG OUT] Can't log out");
      return false;
    }

    return true;
  }

  // returns success or failure
  bool commandSendMsg_(packet::packet_t::payload_t::auth_data_t auth_data,
                       packet::packet_t::payload_t::target_t target_data) {
    const auto session_id = auth_data.session_id;
    if (session_id.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | SEND MESSAGE] Can't parse required field [auth data "
          "section -> "
          "session_id]");
      return false;
    }

    const auto sender_username = auth_data.username;
    if (sender_username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | SEND MESSAGE] Can't parse required field [auth data "
          "section -> "
          "username]");
      return false;
    }

    const auto target_username = target_data.username;
    if (target_username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | SEND MESSAGE] Can't parse required field [target data "
          "section -> "
          "username]");
      return false;
    }

    const auto message = target_data.message;
    if (message.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | SEND MESSAGE] Can't parse required field [target data "
          "section -> "
          "message]");
      return false;
    }

    if (!msg::sendMsg(session_id, sender_username, target_username, message)) {
      common::logAll(QtDebugMsg, "[SERVER | SEND MESSAGE] Can't send message");
      return false;
    }

    return true;
  }

  // returns target_t monad
  common::result_t<packet::packet_t::payload_t::target_t> commandGetMsgs_(
      packet::packet_t::payload_t::auth_data_t auth_data,
      packet::packet_t::payload_t::target_t target_data) {
    const auto session_id = auth_data.session_id;
    if (session_id.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | GET MESSAGES] Can't parse required field [auth data "
          "section -> "
          "session_id]");
      return {};
    }

    const auto username = auth_data.username;
    if (username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | GET MESSAGES] Can't parse required field [auth data "
          "section -> "
          "username]");
      return {};
    }

    const auto target_username = target_data.username;
    if (target_username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | GET MESSAGES] Can't parse required field [target data "
          "section -> "
          "username]");
      return {};
    }

    const auto target_monad =
        msg::getMsgs(session_id, username, target_username);
    if (target_monad.error) {
      common::logAll(QtDebugMsg, "[SERVER | GET MESSAGES] Can't get messages");
      return {};
    }

    return target_monad;
  }

  // returns target_t monad
  common::result_t<packet::packet_t::payload_t::target_t> commandGetAllMsgs_(
      packet::packet_t::payload_t::auth_data_t auth_data) {
    const auto session_id = auth_data.session_id;
    if (session_id.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | GET ALL MESSAGES] Can't parse required field [auth data "
          "section -> "
          "session_id]");
      return {};
    }

    const auto username = auth_data.username;
    if (username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | GET ALL MESSAGES] Can't parse required field [auth data "
          "section -> "
          "username]");
      return {};
    }

    const auto target_monad = msg::getAllMsgs(session_id, username);
    if (target_monad.error) {
      common::logAll(QtDebugMsg,
                     "[SERVER | GET ALL MESSAGES] Can't get all messages");
      return {};
    }

    return target_monad;
  }

  // background dispatch
  void sendNotify_(packet::packet_t::payload_t::auth_data_t auth_data,
                   packet::packet_t::payload_t::target_t target_data) {
    const auto username = auth_data.username;
    if (username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | SEND NOTIFY] Can't parse required field [auth data "
          "section -> "
          "username]");
      return;
    }

    const auto target_username = target_data.username;
    if (target_username.isEmpty()) {
      common::logAll(
          QtDebugMsg,
          "[SERVER | SEND NOTIFY] Can't parse required field [target data "
          "section -> "
          "username]");
      return;
    }

    const auto socket_descriptor_monad =
        auth::getSocketDescriptor(target_username);
    if (socket_descriptor_monad.error) {
      common::logAll(QtDebugMsg,
                     "[SERVER | SEND NOTIFY] Can't get the socket descriptor "
                     "of the target user " +
                         target_username);
      return;
    }

    const auto socket = sockets_.value(socket_descriptor_monad.data);

    const auto response =
        packet::NotifyResponse(packet::packet_t::header_t::command_t::NOTIFY,
                               packet::packet_t::header_t::status_t::OK,
                               "Notify", username)
            .to_json();

    socket->write(response.toJson(QJsonDocument::Indented));
    socket->flush();

    common::logAll(QtDebugMsg,
                   "[SERVER | SEND NOTIFY] Sent a notification to user " +
                       target_username);
  }
};

};  // namespace server