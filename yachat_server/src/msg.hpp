#pragma once

#include <QHash>
#include <cstdlib>
#include <ctime>

#include "auth.hpp"
#include "common.hpp"
#include "db.hpp"
#include "packet.hpp"

namespace msg {

bool sendMsg(const QString& session_id, const QString& sender_username,
             const QString& target_username, const QString& message) {
  if (!auth::isAuthorized(session_id, sender_username)) {
    common::logAll(QtDebugMsg, "[MSG | SEND MESSAGE] Unathorized");
    return false;
  }

  if (!db::db.getUserExists(target_username)) {
    common::logAll(QtDebugMsg, "[MSG | SEND MESSAGE] User " + target_username +
                                   " doesn't exist");
    return false;
  }

  const auto sender_user_id = db::db.getUserId(sender_username).unwrap();
  const auto target_user_id = db::db.getUserId(target_username).unwrap();

  if (!db::db.createMessage(sender_user_id, target_user_id, message)) {
    common::logAll(
        QtDebugMsg,
        "[MSG | SEND MESSAGE] Can't send message to user " + target_username);
    return false;
  }

  common::logAll(QtDebugMsg, "[MSG | SEND MESSAGE] Message to user " +
                                 target_username + " sent");

  return true;
}

common::result_t<packet::packet_t::payload_t::target_t> getMsgs(
    const QString& session_id, const QString& username,
    const QString& target_username) {
  if (!auth::isAuthorized(session_id, username)) {
    common::logAll(QtDebugMsg, "[MSG | GET MESSAGES] Unathorized");
    return {};
  }

  if (!db::db.getUserExists(target_username)) {
    common::logAll(QtDebugMsg, "[MSG | GET MESSAGES] User " + target_username +
                                   " doesn't exist");
    return {};
  }

  QList<packet::packet_t::payload_t::target_t::message_t> messages;

  const auto user_id = db::db.getUserId(username).unwrap();
  const auto target_user_id = db::db.getUserId(target_username).unwrap();

  const auto msgs_monad = db::db.getMsgs(user_id, target_user_id);
  if (msgs_monad.error) {
    common::logAll(QtDebugMsg,
                   "[MSG | GET MESSAGES] Can't get messages from users " +
                       username + " and " + target_username);
    return {};
  }

  common::logAll(QtDebugMsg,
                 "[MSG | GET MESSAGES] Messages received from users " +
                     username + " and " + target_username);

  common::result_t<packet::packet_t::payload_t::target_t> ret;
  ret.data.username = target_username;
  ret.data.messages = msgs_monad.data;
  ret.error = false;

  return ret;
}

common::result_t<packet::packet_t::payload_t::target_t> getAllMsgs(
    const QString& session_id, const QString& username) {
  if (!auth::isAuthorized(session_id, username)) {
    common::logAll(QtDebugMsg, "[MSG | GET ALL MESSAGES] Unathorized");
    return {};
  }

  QHash<QString, QList<packet::packet_t::payload_t::target_t::message_t>>
      all_messages;

  const auto user_id = db::db.getUserId(username).unwrap();

  const auto msgs_monad = db::db.getAllMsgs(user_id);
  if (msgs_monad.error) {
    common::logAll(
        QtDebugMsg,
        "[MSG | GET ALL MESSAGES] Can't get all messages with user " +
            username);
    return {};
  }

  common::logAll(
      QtDebugMsg,
      "[MSG | GET ALL MESSAGES] All messages received with user " + username);

  common::result_t<packet::packet_t::payload_t::target_t> ret;
  ret.data.all_messages = msgs_monad.data;
  ret.error = false;

  return ret;
}

};  // namespace msg
