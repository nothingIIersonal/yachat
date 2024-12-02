#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>

#include "common.hpp"

namespace packet {

namespace request_json_tags {

constexpr auto header = "header";
constexpr auto header_command = "command";
constexpr auto payload = "payload";
constexpr auto payload_auth_data = "auth_data";
constexpr auto payload_auth_data_username = "username";
constexpr auto payload_auth_data_password = "password";
constexpr auto payload_auth_data_session_id = "session_id";
constexpr auto payload_target = "target";
constexpr auto payload_target_username = "username";
constexpr auto payload_target_message = "message";

};  // namespace request_json_tags

namespace response_json_tags {

constexpr auto header = "header";
constexpr auto header_command = "command";
constexpr auto header_status = "status";
constexpr auto header_msg = "msg";
constexpr auto payload = "payload";
constexpr auto payload_auth_data = "auth_data";
constexpr auto payload_auth_data_session_id = "session_id";
constexpr auto payload_target = "target";
constexpr auto payload_target_username = "username";
constexpr auto payload_target_messages = "messages";
constexpr auto payload_target_messages_y = "y";
constexpr auto payload_target_messages_t = "t";
constexpr auto payload_target_all_messages = "all_messages";
constexpr auto payload_target_all_messages_username = "username";
constexpr auto payload_target_all_messages_messages = "messages";
constexpr auto payload_target_all_messages_messages_t = "t";
constexpr auto payload_target_all_messages_messages_y = "y";

};  // namespace response_json_tags

struct packet_t {
  struct header_t {
    enum class command_t : quint16 {
      REGISTER = 0,  // req (client -> server)
      LOGIN,         // req (client -> server)
      SENDMSG,       // req (client -> server)
      GETMSGS,       // req (client -> server)
      NOTIFY,        // res (server -> client)
      LOGOUT,        // req (client -> server)
      STATUS,        // res (server -> client)
      AUTH,          // res (server -> client)
      MSGS,          // res (server -> client)
      GETALLMSGS,    // req (client -> server)
      ALLMSGS,       // res (server -> client)
    } command;

    [[maybe_unused]] static QString commandToQString(command_t c) noexcept {
      return QString::number(static_cast<quint16>(c));
    }
    [[maybe_unused]] static command_t QStringToCommand(
        const QString& s) noexcept {
      return static_cast<command_t>(s.toInt());
    }

    enum class status_t : quint16 {
      OK = 0,
      FAIL,
    } status;  // res (server -> client)

    [[maybe_unused]] static QString statusToQString(status_t s) noexcept {
      return QString::number(static_cast<quint16>(s));
    }
    [[maybe_unused]] static status_t QStringToStatus(
        const QString& s) noexcept {
      return static_cast<status_t>(s.toInt());
    }

    QString msg;  // res (server -> client)
  } header;

  struct __attribute_maybe_unused__ payload_t {
    struct __attribute_maybe_unused__ auth_data_t {
      QString username;    // req (client -> server)
      QString password;    // req (client -> server)
      QString session_id;  // req, res (client -> server, server -> client)
    } auth_data;

    struct __attribute_maybe_unused__ target_t {
      struct message_t {
        QString side;
        QString message;
      };
      QString username;  // req, res (client -> server, server -> client)
      QString message;   // req (client -> server)
      QList<message_t> messages;                      // res (server -> client)
      QHash<QString, QList<message_t>> all_messages;  // res (server -> client)
    } target;
  } payload;
};

static auto jsonExtractPacketHeader(const QJsonDocument& jsonObj)
    -> common::result_t<packet_t::header_t> {
  common::result_t<packet_t::header_t> header_monad;

  const auto header_data = jsonObj[request_json_tags::header];
  if (header_data.isUndefined()) {
    return {};
  }

  const auto header_command_data =
      header_data[response_json_tags::header_command];
  if (header_command_data.isUndefined()) {
    return {};
  }

  const auto header_status_data =
      header_data[response_json_tags::header_status];
  if (header_status_data.isUndefined()) {
    return {};
  }

  const auto header_msg_data = header_data[response_json_tags::header_msg];
  if (header_msg_data.isUndefined()) {
    return {};
  }

  header_monad.error = false;
  header_monad.data.command = packet::packet_t::header_t::QStringToCommand(
      header_command_data.toString());
  header_monad.data.status = packet::packet_t::header_t::QStringToStatus(
      header_status_data.toString());
  header_monad.data.msg = header_msg_data.toString();

  return header_monad;
}

static auto jsonExtractAuthData(const QJsonDocument& jsonObj)
    -> common::result_t<packet_t::payload_t::auth_data_t> {
  common::result_t<packet_t::payload_t::auth_data_t> auth_data_monad;

  const auto payload_data = jsonObj[response_json_tags::payload];
  if (payload_data.isUndefined()) {
    return {};
  }

  const auto payload_auth_data_data =
      payload_data[response_json_tags::payload_auth_data];
  if (payload_auth_data_data.isUndefined()) {
    return {};
  }

  const auto payload_auth_data_session_id_data =
      payload_auth_data_data[response_json_tags::payload_auth_data_session_id];

  if (!payload_auth_data_session_id_data.isUndefined()) {
    auth_data_monad.data.session_id =
        payload_auth_data_session_id_data.toString();
  }

  auth_data_monad.error = false;
  return auth_data_monad;
}

static auto jsonExtractTargetData(const QJsonDocument& jsonObj)
    -> common::result_t<packet_t::payload_t::target_t> {
  common::result_t<packet_t::payload_t::target_t> target_data_monad;

  const auto payload_data = jsonObj[response_json_tags::payload];
  if (payload_data.isUndefined()) {
    return {};
  }

  const auto payload_target_data =
      payload_data[response_json_tags::payload_target];
  if (payload_target_data.isUndefined()) {
    return {};
  }

  const auto payload_target_username_data =
      payload_target_data[response_json_tags::payload_target_username];
  const auto payload_target_messages_data =
      payload_target_data[response_json_tags::payload_target_messages];
  const auto payload_target_all_messages_data =
      payload_target_data[response_json_tags::payload_target_all_messages];

  if (!payload_target_username_data.isUndefined()) {
    target_data_monad.data.username = payload_target_username_data.toString();
  }
  if (!payload_target_messages_data.isUndefined()) {
    for (const auto& message : payload_target_messages_data.toArray()) {
      const auto& msg = message.toObject();
      const auto yval =
          msg[response_json_tags::payload_target_all_messages_messages_y];
      const auto tval =
          msg[response_json_tags::payload_target_all_messages_messages_t];
      if (!yval.isUndefined()) {
        target_data_monad.data.messages.push_back(
            {response_json_tags::payload_target_messages_y, yval.toString()});
      } else if (!tval.isUndefined()) {
        target_data_monad.data.messages.push_back(
            {response_json_tags::payload_target_messages_t, tval.toString()});
      } else {
        return {};
      }
    }
  }
  if (!payload_target_all_messages_data.isUndefined()) {
    for (const auto& one_user_msgs :
         payload_target_all_messages_data.toArray()) {
      const auto& one_user_msgs_obj = one_user_msgs.toObject();
      const auto& username_json_val = one_user_msgs_obj
          [response_json_tags::payload_target_all_messages_username];
      if (username_json_val.isUndefined()) {
        return {};
      }
      const auto username = username_json_val.toString();
      for (const auto& message :
           one_user_msgs_obj
               [response_json_tags::payload_target_all_messages_messages]
                   .toArray()) {
        const auto& msg = message.toObject();
        const auto yval =
            msg[response_json_tags::payload_target_all_messages_messages_y];
        const auto tval =
            msg[response_json_tags::payload_target_all_messages_messages_t];
        if (!yval.isUndefined()) {
          target_data_monad.data.all_messages[username].push_back(
              {response_json_tags::payload_target_all_messages_messages_y,
               yval.toString()});
        } else if (!tval.isUndefined()) {
          target_data_monad.data.all_messages[username].push_back(
              {response_json_tags::payload_target_all_messages_messages_t,
               tval.toString()});
        } else {
          return {};
        }
      }
    }
  }

  target_data_monad.error = false;
  return target_data_monad;
}

struct Request {
  QString header_command;

  Request(packet::packet_t::header_t::command_t header_command)
      : header_command(
            packet::packet_t::header_t::commandToQString(header_command)){};
  ~Request() = default;

  virtual QJsonDocument to_json() { return {}; };
};

struct AuthenRequest : public Request {
  QString payload_auth_data_username;
  QString payload_auth_data_password;

  AuthenRequest(packet::packet_t::header_t::command_t header_command,
                const QString& payload_auth_data_username,
                const QString& payload_auth_data_password)
      : Request(header_command),
        payload_auth_data_username(payload_auth_data_username),
        payload_auth_data_password(payload_auth_data_password) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject auth_data_json;

    auth_data_json.insert(packet::request_json_tags::payload_auth_data_username,
                          payload_auth_data_username);
    auth_data_json.insert(packet::request_json_tags::payload_auth_data_password,
                          payload_auth_data_password);

    payload_json.insert(packet::request_json_tags::payload_auth_data,
                        auth_data_json);

    header_json.insert(packet::request_json_tags::header_command,
                       header_command);

    response.insert(packet::request_json_tags::payload, payload_json);
    response.insert(packet::request_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct RegisterRequest : public AuthenRequest {
  RegisterRequest(const QString& payload_auth_data_username,
                  const QString& payload_auth_data_password)
      : AuthenRequest(packet::packet_t::header_t::command_t::REGISTER,
                      payload_auth_data_username, payload_auth_data_password) {}
};

struct LoginRequest : public AuthenRequest {
  LoginRequest(const QString& payload_auth_data_username,
               const QString& payload_auth_data_password)
      : AuthenRequest(packet::packet_t::header_t::command_t::LOGIN,
                      payload_auth_data_username, payload_auth_data_password) {}
};

struct AuthRequest : public Request {
  QString payload_auth_data_username;
  QString payload_auth_data_session_id;

  AuthRequest(packet::packet_t::header_t::command_t header_command,
              const QString& payload_auth_data_username,
              const QString& payload_auth_data_session_id)
      : Request(header_command),
        payload_auth_data_username(payload_auth_data_username),
        payload_auth_data_session_id(payload_auth_data_session_id) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject auth_data_json;

    auth_data_json.insert(packet::request_json_tags::payload_auth_data_username,
                          payload_auth_data_username);
    auth_data_json.insert(
        packet::request_json_tags::payload_auth_data_session_id,
        payload_auth_data_session_id);

    payload_json.insert(packet::request_json_tags::payload_auth_data,
                        auth_data_json);

    header_json.insert(packet::request_json_tags::header_command,
                       header_command);

    response.insert(packet::request_json_tags::payload, payload_json);
    response.insert(packet::request_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct LogoutRequest : public AuthRequest {
  LogoutRequest(const QString& payload_auth_data_username,
                const QString& payload_auth_data_session_id)
      : AuthRequest(packet::packet_t::header_t::command_t::LOGOUT,
                    payload_auth_data_username, payload_auth_data_session_id) {}
};

struct SendMsgRequest : public AuthRequest {
  QString payload_target_username;
  QString payload_target_message;

  SendMsgRequest(const QString& payload_auth_data_username,
                 const QString& payload_auth_data_session_id,
                 const QString& payload_target_username,
                 const QString& payload_target_message)
      : AuthRequest(packet::packet_t::header_t::command_t::SENDMSG,
                    payload_auth_data_username, payload_auth_data_session_id),
        payload_target_username(payload_target_username),
        payload_target_message(payload_target_message) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject auth_data_json;
    QJsonObject target_json;

    target_json.insert(packet::request_json_tags::payload_target_username,
                       payload_target_username);
    target_json.insert(packet::request_json_tags::payload_target_message,
                       payload_target_message);

    auth_data_json.insert(packet::request_json_tags::payload_auth_data_username,
                          payload_auth_data_username);
    auth_data_json.insert(
        packet::request_json_tags::payload_auth_data_session_id,
        payload_auth_data_session_id);

    payload_json.insert(packet::request_json_tags::payload_target, target_json);
    payload_json.insert(packet::request_json_tags::payload_auth_data,
                        auth_data_json);

    header_json.insert(packet::request_json_tags::header_command,
                       header_command);

    response.insert(packet::request_json_tags::payload, payload_json);
    response.insert(packet::request_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct GetMsgsRequest : public AuthRequest {
  QString payload_target_username;

  GetMsgsRequest(const QString& payload_auth_data_username,
                 const QString& payload_auth_data_session_id,
                 const QString& payload_target_username)
      : AuthRequest(packet::packet_t::header_t::command_t::GETMSGS,
                    payload_auth_data_username, payload_auth_data_session_id),
        payload_target_username(payload_target_username) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject auth_data_json;
    QJsonObject target_json;

    target_json.insert(packet::request_json_tags::payload_target_username,
                       payload_target_username);

    auth_data_json.insert(packet::request_json_tags::payload_auth_data_username,
                          payload_auth_data_username);
    auth_data_json.insert(
        packet::request_json_tags::payload_auth_data_session_id,
        payload_auth_data_session_id);

    payload_json.insert(packet::request_json_tags::payload_target, target_json);
    payload_json.insert(packet::request_json_tags::payload_auth_data,
                        auth_data_json);

    header_json.insert(packet::request_json_tags::header_command,
                       header_command);

    response.insert(packet::request_json_tags::payload, payload_json);
    response.insert(packet::request_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct GetAllMsgsRequest : public AuthRequest {
  GetAllMsgsRequest(const QString& payload_auth_data_username,
                    const QString& payload_auth_data_session_id)
      : AuthRequest(packet::packet_t::header_t::command_t::GETALLMSGS,
                    payload_auth_data_username, payload_auth_data_session_id) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject auth_data_json;

    auth_data_json.insert(packet::request_json_tags::payload_auth_data_username,
                          payload_auth_data_username);
    auth_data_json.insert(
        packet::request_json_tags::payload_auth_data_session_id,
        payload_auth_data_session_id);

    payload_json.insert(packet::request_json_tags::payload_auth_data,
                        auth_data_json);

    header_json.insert(packet::request_json_tags::header_command,
                       header_command);

    response.insert(packet::request_json_tags::payload, payload_json);
    response.insert(packet::request_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

};  // namespace packet