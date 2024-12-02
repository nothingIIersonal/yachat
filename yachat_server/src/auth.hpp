#pragma once

#include <QHash>
#include <cstdlib>
#include <ctime>

#include "common.hpp"
#include "db.hpp"

namespace auth {

using session_id_t = QString;
struct {
 public:
  common::result_t<session_id_t> get(const QString& username) const {
    if (!contains(username)) {
      return {};
    }

    common::result_t<session_id_t> session_id_monad;
    session_id_monad.error = false;
    session_id_monad.data = sessions_[username];

    return session_id_monad;
  }

  common::result_t<qintptr> getSocketDescriptor(const QString& username) const {
    if (!contains(username)) {
      return {};
    }

    common::result_t<qintptr> socket_descriptor_monad;
    socket_descriptor_monad.error = false;
    socket_descriptor_monad.data = socket_sessions_.key(username);

    return socket_descriptor_monad;
  }

  bool contains(const QString& username) const {
    return sessions_.contains(username);
  }

  common::result_t<session_id_t> add(const QString& username,
                                     const qintptr socket_descriptor) {
    if (contains(username)) {
      return {};
    }

    common::result_t<session_id_t> session_id_monad;
    session_id_monad.error = false;
    session_id_monad.data = genSessionId_();

    sessions_.insert(username, session_id_monad.data);
    socket_sessions_.insert(socket_descriptor, username);

    return session_id_monad;
  }

  bool remove(const QString& username) {
    if (!contains(username)) {
      return false;
    }

    const auto socket_descriptor = socket_sessions_.key(username);
    socket_sessions_.remove(socket_descriptor);

    return sessions_.remove(username);
  }

  void forcedRemove(const qintptr socket_descriptor) {
    if (!socketContains_(socket_descriptor)) {
      return;
    }

    const auto username = socket_sessions_[socket_descriptor];
    socket_sessions_.remove(socket_descriptor);

    if (!contains(username)) {
      return;
    }

    sessions_.remove(username);
  }

 private:
  QHash<QString, session_id_t> sessions_;    // <username, session_id>
                                             // only one session is allowed
  QHash<qintptr, QString> socket_sessions_;  // <socket_descriptor, username>
                                             // to reset the session after
                                             // disconnecting the client

  session_id_t genSessionId_() const {
    constexpr int diff = 'Z' - '0';
    srand(time(0));
    session_id_t session_id;
    for (int i = 0; i < 17; ++i) {  // imagine that hash len is 17
      session_id += QChar('A' + (rand() % diff));
    }

    return session_id;
  }

  bool socketContains_(const qintptr socket_descriptor) const {
    return socket_sessions_.contains(socket_descriptor);
  }
} sessions;

bool registerUser(const QString& username, const QString& password) {
  if (db::db.getUserExists(username)) {
    common::logAll(QtDebugMsg,
                   "[AUTH | REGISTER] User " + username + " already exists");
    return false;
  }

  if (!db::db.createUser(username, password)) {
    common::logAll(QtDebugMsg,
                   "[AUTH | REGISTER] Can't create user " + username);
    return false;
  }

  common::logAll(QtDebugMsg,
                 "[AUTH | REGISTER] User " + username + " registered");

  return true;
}

common::result_t<session_id_t> logInUser(const QString& username,
                                         const QString& password,
                                         const qintptr socket_descriptor) {
  if (!db::db.getUserExists(username)) {
    common::logAll(QtDebugMsg,
                   "[AUTH | LOG IN] User " + username + " doesn't exist");
    return {};
  }

  if (!db::db.checkUserPassword(username, password)) {
    common::logAll(QtDebugMsg,
                   "[AUTH | LOG IN] Invalid password for user " + username);
    return {};
  }

  const auto session_id_monad = sessions.add(username, socket_descriptor);
  if (session_id_monad.error) {
    common::logAll(QtDebugMsg, "[AUTH | LOG IN] User " + username +
                                   " is already logged in");
    return {};
  }

  common::logAll(QtDebugMsg, "[AUTH | LOG IN] User " + username + " logged in");

  return session_id_monad;
}

bool logOutUser(const session_id_t& session_id, const QString& username) {
  if (!db::db.getUserExists(username)) {
    common::logAll(QtDebugMsg,
                   "[AUTH | LOG OUT] User " + username + " doesn't exist");
    return false;
  }

  const auto session_id_monad = sessions.get(username);
  if (session_id_monad.error) {
    common::logAll(QtDebugMsg,
                   "[AUTH | LOG OUT] User " + username + " is not logged in");
    return false;
  }

  if (session_id_monad.data != session_id) {
    common::logAll(QtDebugMsg, "[AUTH | LOG OUT] Session ID " + session_id +
                                   " is incorrect");
    return false;
  }

  if (!sessions.remove(username)) {
    common::logAll(
        QtDebugMsg,
        "[AUTH | LOG OUT] An unknown error occured while erasing the session " +
            session_id + " of user " + username);
    return false;
  }

  common::logAll(QtDebugMsg, "[AUTH | LOG OUT] User " + username +
                                 " logged out successfully");

  return true;
}

void forcedLogOutUser(const qintptr socket_descriptor) {
  sessions.forcedRemove(socket_descriptor);
  common::logAll(QtDebugMsg,
                 "[AUTH | FORCED LOG OUT] User with socket descriptor " +
                     QString::number(socket_descriptor) +
                     " was successfully forcibly logged out");
}

common::result_t<qintptr> getSocketDescriptor(const QString& username) {
  if (!db::db.getUserExists(username)) {
    common::logAll(QtDebugMsg, "[AUTH | GET SOCKET DESCRIPTOR] User " +
                                   username + " doesn't exist");
    return {};
  }

  return sessions.getSocketDescriptor(username);
}

// for external usage
bool isAuthorized(const QString& session_id, const QString& username) {
  if (!db::db.getUserExists(username)) {
    common::logAll(QtDebugMsg, "[AUTH | IS AUTHORIZED] User " + username +
                                   " doesn't exist");
    return false;
  }

  const auto session_id_monad = sessions.get(username);
  if (session_id_monad.error) {
    common::logAll(QtDebugMsg, "[AUTH | IS AUTHORIZED] User " + username +
                                   " is not logged in");
    return false;
  }

  if (session_id_monad.data != session_id) {
    common::logAll(QtDebugMsg, "[AUTH | IS AUTHORIZED] Session ID " +
                                   session_id + " is incorrect");
    return false;
  }

  return true;
}

};  // namespace auth
