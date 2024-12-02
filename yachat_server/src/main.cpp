// TODO: fix all users visibility
// TODO: traffic encryption
// TODO: add password hashing

#include <QCoreApplication>
#include <QString>

#define JOURNAL "journal.txt"

#include "server.hpp"

int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);

  server::Server server(1234, &a);
  common::logAll(QtDebugMsg, "[MAIN] Server started on port " +
                                 QString::number(server.serverPort()));

  return a.exec();
}
