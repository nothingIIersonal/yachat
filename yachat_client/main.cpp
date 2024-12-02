// TODO: refactor ui code
// TODO: fix all users visibility
// TODO: fix logout -> login bugs

#include <QApplication>

#define JOURNAL "journal_client.txt"

#include "client.hpp"
#include "config.hpp"
#include "ui/chat_widget.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  Config config{"config.json"};
  Client client{config};
  ui::ChatWidget chatWidget{client};
  return a.exec();
}
