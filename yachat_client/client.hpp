#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QTcpSocket>

#include "config.hpp"
#include "packet.hpp"

class Client final : public QObject {
  Q_OBJECT

 public:
  explicit Client(Config &config) : config_(config) {
    connect(&socket_, &QTcpSocket::connected, this, []() {
      qDebug() << "[CLIENT | ON CONNECTED] Connected to server!";
    });
    connect(&socket_, &QTcpSocket::disconnected, this, [=]() {
      qDebug() << "[CLIENT | ON DISCONNECTED] Disconnected";
      socket_.disconnectFromHost();
      socket_.deleteLater();
    });
    connect(&socket_, &QTcpSocket::errorOccurred, this, [=]() {
      qFatal("[CLIENT | ON ERROR OCCURED] Error: %s",
             socket_.errorString().toStdString().c_str());
      QApplication::exit(EXIT_FAILURE);
    });
    connect(&socket_, &QTcpSocket::readyRead, this, &Client::onReadyRead_);

    if (socket_.state() == QAbstractSocket::UnconnectedState) {
      socket_.connectToHost(config_.GetIP(), config_.GetPort());
    } else {
      qFatal("Can't initiate client");
      QApplication::exit(EXIT_FAILURE);
    }
  }

  // inline void setUsername(const QString &username) { username_ = username; }
  inline auto getUsername() const -> QString { return username_; }
  // inline void setPassword(const QString &password) { password_ = password; }
  inline auto getPassword() const -> QString { return password_; }
  inline auto getSessionID() const -> QString { return session_id_; }

  void SendRegister(const QString &username, const QString &password) {
    const auto req = packet::RegisterRequest(username, password).to_json();
    socket_.write(req.toJson(QJsonDocument::Indented));
    socket_.flush();
  }

  void SendLogIn(const QString &username, const QString &password) {
    username_ = username;
    password_ = password;
    const auto req = packet::LoginRequest(username_, password_).to_json();
    socket_.write(req.toJson(QJsonDocument::Indented));
    socket_.flush();
  }

  void SendLogOut() {
    const auto req = packet::LogoutRequest(username_, session_id_).to_json();
    socket_.write(req.toJson(QJsonDocument::Indented));
    socket_.flush();
  }

  void SendSendMsg(const QString &target_username, const QString &message) {
    const auto req =
        packet::SendMsgRequest(username_, session_id_, target_username, message)
            .to_json();
    socket_.write(req.toJson(QJsonDocument::Indented));
    socket_.flush();
  }

  void SendGetMsgs(const QString &target_username) {
    const auto req =
        packet::GetMsgsRequest(username_, session_id_, target_username)
            .to_json();
    socket_.write(req.toJson(QJsonDocument::Indented));
    socket_.flush();
  }

  void SendGetAllMsgs() {
    const auto req =
        packet::GetAllMsgsRequest(username_, session_id_).to_json();
    socket_.write(req.toJson(QJsonDocument::Indented));
    socket_.flush();
  }

 private slots:
  void onReadyRead_() {
    QByteArray requestData = socket_.readAll();
    QString requestString = QString::fromUtf8(requestData);
    QJsonDocument requestJson = QJsonDocument::fromJson(requestString.toUtf8());

    auto header_monad = packet::jsonExtractPacketHeader(requestJson);
    if (header_monad.error) {
      qDebug() << "[CLIENT | ON READY READ] Error parsing received JSON "
                  "[header section]";
      return;
    }

    const auto header = header_monad.unwrap();
    const auto command = header.command;

    process_(command, header, std::move(requestJson));
  }

 signals:
  void respNotifyReceived(packet::packet_t::header_t::status_t, const QString &,
                          const QString &);
  void respStatusReceived(packet::packet_t::header_t::status_t,
                          const QString &);
  void respAuthReceived(packet::packet_t::header_t::status_t, const QString &);
  void respMsgsReceived(
      packet::packet_t::header_t::status_t, const QString &, const QString &,
      const QList<packet::packet_t::payload_t::target_t::message_t> &);
  void respAllMsgsReceived(
      packet::packet_t::header_t::status_t, const QString &,
      const QHash<QString,
                  QList<packet::packet_t::payload_t::target_t::message_t>> &);

 private:
  Config &config_;
  QTcpSocket socket_;
  QString username_;
  QString password_;
  QString session_id_;

  void process_(packet::packet_t::header_t::command_t command,
                packet::packet_t::header_t header, QJsonDocument &&packetData) {
    switch (command) {
      case packet::packet_t::header_t::command_t::STATUS: {
        const auto status = header.status;
        const auto msg = header.msg;
        emit respStatusReceived(status, msg);
        break;
      };
      case packet::packet_t::header_t::command_t::AUTH: {
        auto auth_data_monad = packet::jsonExtractAuthData(packetData);
        if (auth_data_monad.error) {
          qDebug() << "[CLIENT | PROCESS AUTH] Error parsing received JSON "
                      "[auth data section]";
          return;
        }

        const auto status = header.status;
        const auto msg = header.msg;
        const auto session_id = auth_data_monad.unwrap().session_id;

        session_id_ = session_id;

        emit respAuthReceived(status, msg);

        break;
      };
      case packet::packet_t::header_t::command_t::ALLMSGS: {
        auto target_data_monad = packet::jsonExtractTargetData(packetData);
        if (target_data_monad.error) {
          qDebug() << "[CLIENT | PROCESS ALLMSGS] Error parsing received JSON "
                      "[target data section]";
          return;
        }

        const auto status = header.status;
        const auto msg = header.msg;

        const auto all_messages = target_data_monad.data.all_messages;

        emit respAllMsgsReceived(status, msg, all_messages);

        break;
      };
      case packet::packet_t::header_t::command_t::MSGS: {
        auto target_data_monad = packet::jsonExtractTargetData(packetData);
        if (target_data_monad.error) {
          qDebug() << "[CLIENT | PROCESS MSGS] Error parsing received JSON "
                      "[target data section]";
          return;
        }

        const auto status = header.status;
        const auto msg = header.msg;

        const auto username = target_data_monad.data.username;
        const auto messages = target_data_monad.data.messages;

        emit respMsgsReceived(status, msg, username, messages);

        break;
      };
      case packet::packet_t::header_t::command_t::NOTIFY: {
        auto target_data_monad = packet::jsonExtractTargetData(packetData);
        if (target_data_monad.error) {
          qDebug() << "[CLIENT | PROCESS NOTIFY] Error parsing received JSON "
                      "[target data section]";
          return;
        }

        const auto status = header.status;
        const auto msg = header.msg;

        const auto username = target_data_monad.data.username;

        emit respNotifyReceived(status, msg, username);

        break;
      };
      default: {
        break;
      };
    }
  }
};
