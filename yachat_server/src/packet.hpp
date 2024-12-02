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

auto jsonExtractPacketHeader(const QJsonDocument& jsonObj)
    -> common::result_t<packet_t::header_t> {
  common::result_t<packet_t::header_t> header_monad;

  const auto header_data = jsonObj[request_json_tags::header];
  if (header_data.isUndefined()) {
    return {};
  }

  const auto header_command_data =
      header_data[request_json_tags::header_command];
  if (header_command_data.isUndefined()) {
    return {};
  }

  header_monad.error = false;
  header_monad.data.command = packet::packet_t::header_t::QStringToCommand(
      header_command_data.toString());

  return header_monad;
}

auto jsonExtractAuthData(const QJsonDocument& jsonObj)
    -> common::result_t<packet_t::payload_t::auth_data_t> {
  common::result_t<packet_t::payload_t::auth_data_t> auth_data_monad;

  const auto payload_data = jsonObj[request_json_tags::payload];
  if (payload_data.isUndefined()) {
    return {};
  }

  const auto payload_auth_data_data =
      payload_data[request_json_tags::payload_auth_data];
  if (payload_auth_data_data.isUndefined()) {
    return {};
  }

  const auto payload_auth_data_username_data =
      payload_auth_data_data[request_json_tags::payload_auth_data_username];
  const auto payload_auth_data_password_data =
      payload_auth_data_data[request_json_tags::payload_auth_data_password];
  const auto payload_auth_data_session_id_data =
      payload_auth_data_data[request_json_tags::payload_auth_data_session_id];

  if (!payload_auth_data_username_data.isUndefined()) {
    auth_data_monad.data.username = payload_auth_data_username_data.toString();
  }
  if (!payload_auth_data_password_data.isUndefined()) {
    auth_data_monad.data.password = payload_auth_data_password_data.toString();
  }
  if (!payload_auth_data_session_id_data.isUndefined()) {
    auth_data_monad.data.session_id =
        payload_auth_data_session_id_data.toString();
  }

  auth_data_monad.error = false;
  return auth_data_monad;
}

auto jsonExtractTargetData(const QJsonDocument& jsonObj)
    -> common::result_t<packet_t::payload_t::target_t> {
  common::result_t<packet_t::payload_t::target_t> target_data_monad;

  const auto payload_data = jsonObj[request_json_tags::payload];
  if (payload_data.isUndefined()) {
    return {};
  }

  const auto payload_target_data =
      payload_data[request_json_tags::payload_target];
  if (payload_target_data.isUndefined()) {
    return {};
  }

  const auto payload_target_username_data =
      payload_target_data[request_json_tags::payload_target_username];
  const auto payload_target_message_data =
      payload_target_data[request_json_tags::payload_target_message];

  if (!payload_target_username_data.isUndefined()) {
    target_data_monad.data.username = payload_target_username_data.toString();
  }
  if (!payload_target_message_data.isUndefined()) {
    target_data_monad.data.message = payload_target_message_data.toString();
  }

  target_data_monad.error = false;
  return target_data_monad;
}

struct Response {
  Response() = default;
  ~Response() = default;

  virtual QJsonDocument to_json() { return {}; };
};

struct StatusResponse : public Response {
  QString header_command;
  QString header_status;
  QString header_msg;

  StatusResponse(packet::packet_t::header_t::command_t header_command,
                 packet::packet_t::header_t::status_t header_status,
                 const QString& header_msg)
      : header_command(
            packet::packet_t::header_t::commandToQString(header_command)),
        header_status(
            packet::packet_t::header_t::statusToQString(header_status)),
        header_msg(header_msg) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;

    header_json.insert(packet::response_json_tags::header_command,
                       header_command);
    header_json.insert(packet::response_json_tags::header_status,
                       header_status);
    header_json.insert(packet::response_json_tags::header_msg, header_msg);

    response.insert(packet::response_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct AuthResponse : public StatusResponse {
  QString payload_auth_data_session_id;

  AuthResponse(packet::packet_t::header_t::command_t header_command,
               packet::packet_t::header_t::status_t header_status,
               const QString& header_msg,
               const QString& payload_auth_data_session_id)
      : StatusResponse(header_command, header_status, header_msg),
        payload_auth_data_session_id(payload_auth_data_session_id) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject auth_data_json;

    auth_data_json.insert(
        packet::response_json_tags::payload_auth_data_session_id,
        payload_auth_data_session_id);

    payload_json.insert(packet::response_json_tags::payload_auth_data,
                        auth_data_json);

    header_json.insert(packet::response_json_tags::header_command,
                       header_command);
    header_json.insert(packet::response_json_tags::header_status,
                       header_status);
    header_json.insert(packet::response_json_tags::header_msg, header_msg);

    response.insert(packet::response_json_tags::payload, payload_json);
    response.insert(packet::response_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct NotifyResponse : public StatusResponse {
  QString payload_target_username;

  NotifyResponse(packet::packet_t::header_t::command_t header_command,
                 packet::packet_t::header_t::status_t header_status,
                 const QString& header_msg,
                 const QString& payload_target_username)
      : StatusResponse(header_command, header_status, header_msg),
        payload_target_username(payload_target_username) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject target_json;

    target_json.insert(packet::response_json_tags::payload_target_username,
                       payload_target_username);

    payload_json.insert(packet::response_json_tags::payload_target,
                        target_json);

    header_json.insert(packet::response_json_tags::header_command,
                       header_command);
    header_json.insert(packet::response_json_tags::header_status,
                       header_status);
    header_json.insert(packet::response_json_tags::header_msg, header_msg);

    response.insert(packet::response_json_tags::payload, payload_json);
    response.insert(packet::response_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct MsgsResponse : public NotifyResponse {
  QList<packet::packet_t::payload_t::target_t::message_t>
      payload_target_messages;

  MsgsResponse(packet::packet_t::header_t::command_t header_command,
               packet::packet_t::header_t::status_t header_status,
               const QString& header_msg,
               const QString& payload_target_username,
               const QList<packet::packet_t::payload_t::target_t::message_t>&
                   payload_target_messages)
      : NotifyResponse(header_command, header_status, header_msg,
                       payload_target_username),
        payload_target_messages(payload_target_messages) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject target_json;
    QJsonArray messages_json;

    for (const auto& msg : payload_target_messages) {
      QJsonObject message_json;
      message_json.insert(msg.side, msg.message);
      messages_json.append(message_json);
    }

    target_json.insert(packet::response_json_tags::payload_target_messages,
                       messages_json);
    target_json.insert(packet::response_json_tags::payload_target_username,
                       payload_target_username);

    payload_json.insert(packet::response_json_tags::payload_target,
                        target_json);

    header_json.insert(packet::response_json_tags::header_command,
                       header_command);
    header_json.insert(packet::response_json_tags::header_status,
                       header_status);
    header_json.insert(packet::response_json_tags::header_msg, header_msg);

    response.insert(packet::response_json_tags::payload, payload_json);
    response.insert(packet::response_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

struct AllMsgsResponse : public StatusResponse {
  QHash<QString, QList<packet::packet_t::payload_t::target_t::message_t>>
      payload_target_all_messages;

  AllMsgsResponse(
      packet::packet_t::header_t::command_t header_command,
      packet::packet_t::header_t::status_t header_status,
      const QString& header_msg,
      const QHash<QString,
                  QList<packet::packet_t::payload_t::target_t::message_t>>&
          payload_target_all_messages)
      : StatusResponse(header_command, header_status, header_msg),
        payload_target_all_messages(payload_target_all_messages) {}

  QJsonDocument to_json() {
    QJsonObject response;
    QJsonObject header_json;
    QJsonObject payload_json;
    QJsonObject target_json;
    QJsonArray all_messages_json;

    foreach (const auto& username, payload_target_all_messages.keys()) {
      QJsonObject one_user_messages;
      QJsonArray messages_json;

      foreach (const auto& msg, payload_target_all_messages[username]) {
        QJsonObject message_json;
        message_json.insert(msg.side, msg.message);
        messages_json.append(message_json);
      }

      one_user_messages.insert(
          packet::response_json_tags::payload_target_all_messages_username,
          username);
      one_user_messages.insert(
          packet::response_json_tags::payload_target_all_messages_messages,
          messages_json);

      all_messages_json.append(one_user_messages);
    }

    target_json.insert(packet::response_json_tags::payload_target_all_messages,
                       all_messages_json);

    payload_json.insert(packet::response_json_tags::payload_target,
                        target_json);

    header_json.insert(packet::response_json_tags::header_command,
                       header_command);
    header_json.insert(packet::response_json_tags::header_status,
                       header_status);
    header_json.insert(packet::response_json_tags::header_msg, header_msg);

    response.insert(packet::response_json_tags::payload, payload_json);
    response.insert(packet::response_json_tags::header, header_json);

    QJsonDocument doc(response);
    return doc;
  }
};

};  // namespace packet