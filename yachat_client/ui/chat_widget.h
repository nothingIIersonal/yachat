#ifndef CHAT_WIDGET_H
#define CHAT_WIDGET_H

#include <QLabel>
#include <QListWidget>
#include <QTextEdit>
#include <QWidget>
#include <memory>

#include "client.hpp"
#include "login_widget.h"

namespace ui {

class MessageBlock final : public QWidget {
  Q_OBJECT

 public:
  explicit MessageBlock(const QString& username, const QString& msg);

 private:
  QLabel username_;
  QLabel msg_;
};

class ChatWidget final : public QWidget {
  Q_OBJECT

 public:
  explicit ChatWidget(Client& client);

 private:
  Client& client_;
  std::shared_ptr<LoginWidget> loginWidget_;
  std::shared_ptr<QListWidget> messagesList_;
  std::shared_ptr<QListWidget> usersList_;
  QHash<QString, QList<packet::packet_t::payload_t::target_t::message_t>>
      all_messages_;
  QString selected_user_;
  void updateMessages_(const QString& username);
  void addMessageToList_(std::shared_ptr<QListWidget> list,
                         MessageBlock* msgBlock);
};

}  // namespace ui

#endif  // CHAT_WIDGET_H
