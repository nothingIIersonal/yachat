#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QtSql>

#include "common.hpp"
#include "packet.hpp"

namespace db {

class DB {
 public:
  DB(const DB&) = delete;
  DB& operator=(const DB&) = delete;

  static DB& getInstance() {
    if (!instance_) {
      instance_ = new DB();
    }
    return *instance_;
  }

  bool getUserExists(const QString& username) {
    QSqlQuery query;
    query.prepare("SELECT user_id FROM users WHERE username = ?");
    query.addBindValue(username);
    query.exec();

    if (query.next()) {
      return true;
    }

    return false;
  }

  common::result_t<quint64> getUserId(const QString& username) {
    common::result_t<quint64> res;

    QSqlQuery query;
    query.prepare("SELECT user_id FROM users WHERE username = ?");
    query.addBindValue(username);
    query.exec();

    if (query.next()) {
      res.error = false;
      res.data = query.value(0).toUInt();
      return res;
    }

    return {};
  }

  common::result_t<QList<packet::packet_t::payload_t::target_t::message_t>>
  getMsgs(const quint64 from_user_id, const quint64 to_user_id) {
    common::result_t<QList<packet::packet_t::payload_t::target_t::message_t>>
        res;

    QSqlQuery query;
    query.prepare(
        "SELECT from_user_id, to_user_id, message FROM messages WHERE "
        "from_user_id IN (?, ?) AND to_user_id IN (?, ?)");
    query.addBindValue(from_user_id);
    query.addBindValue(to_user_id);
    query.addBindValue(from_user_id);
    query.addBindValue(to_user_id);
    query.exec();

    bool found = false;
    while (query.next()) {
      found = true;

      const auto sender_id = query.value(0).toUInt();
      const auto message = query.value(2).toString();

      if (sender_id == from_user_id) {
        res.data.push_back(
            {packet::response_json_tags::payload_target_messages_y, message});
      } else {
        res.data.push_back(
            {packet::response_json_tags::payload_target_messages_t, message});
      }
    }

    if (found) {
      res.error = false;
      return res;
    }

    return {};
  }

  common::result_t<
      QHash<QString, QList<packet::packet_t::payload_t::target_t::
                               message_t>>>  // {{<username>, <msgs>},
                                             // {<username>, <msgs>}}
  getAllMsgs(const quint64 user_id) {
    common::result_t<
        QHash<QString, QList<packet::packet_t::payload_t::target_t::message_t>>>
        res;

    QSqlQuery usernameQuery;
    usernameQuery.prepare("SELECT username FROM users WHERE user_id == ?");
    usernameQuery.addBindValue(user_id);
    usernameQuery.exec();

    if (!usernameQuery.next()) {
      return {};
    }

    const auto username = usernameQuery.value(0).toString();

    QSqlQuery query;
    query.prepare(
        "SELECT from_user_id, to_user_id, message FROM messages WHERE "
        "from_user_id == ? OR to_user_id == ?");
    query.addBindValue(user_id);
    query.addBindValue(user_id);
    query.exec();

    bool found = false;
    while (query.next()) {
      found = true;

      const auto sender_id = query.value(0).toUInt();
      const auto rec_id = query.value(1).toUInt();
      const auto message = query.value(2).toString();

      usernameQuery.prepare("SELECT username FROM users WHERE user_id == ?");

      QString targetUsername;
      if (sender_id != user_id) {
        usernameQuery.addBindValue(sender_id);
      } else {
        usernameQuery.addBindValue(rec_id);
      }

      usernameQuery.exec();
      usernameQuery.next();
      targetUsername = usernameQuery.value(0).toString();

      if (sender_id == user_id) {
        res.data[targetUsername].push_back(
            {packet::response_json_tags::payload_target_all_messages_messages_y,
             message});
      } else {
        res.data[targetUsername].push_back(
            {packet::response_json_tags::payload_target_all_messages_messages_t,
             message});
      }
    }

    if (found) {
      res.error = false;
      return res;
    }

    return {};
  }

  bool checkUserPassword(const QString& username, const QString& password) {
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(username);
    query.exec();

    if (query.next()) {
      if (query.value(0).toString() == password) {
        return true;
      }
    }

    return false;
  }

  bool createUser(const QString& username, const QString& password) {
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(password);
    return query.exec();
  }

  bool createMessage(const quint64 from_user_id, const quint64 to_user_id,
                     const QString& message) {
    QSqlQuery query;
    query.prepare(
        "INSERT INTO messages (from_user_id, to_user_id, message) VALUES (?, "
        "?, ?)");
    query.addBindValue(from_user_id);
    query.addBindValue(to_user_id);
    query.addBindValue(message);
    return query.exec();
  }

 private:
  QSqlDatabase sdb_;
  static DB* instance_;

  DB() {
    sdb_ = QSqlDatabase::addDatabase("QSQLITE");
    sdb_.setDatabaseName(DB_PATH);  // DB_PATH is a compile-time variable
    if (!sdb_.open()) {
      common::logAll(QtFatalMsg, "[DB] " + sdb_.lastError().text());
      exit(EXIT_FAILURE);
    }
  }

  ~DB() = default;
};

DB* DB::instance_ = nullptr;
DB& db = DB::getInstance();

};  // namespace db
