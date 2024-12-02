#include "chat_widget.h"

#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

namespace ui {

ChatWidget::ChatWidget(Client& client) : client_(client) {
  loginWidget_ = std::make_shared<LoginWidget>(client);

  connect(loginWidget_.get(), &LoginWidget::authDone, this, [this]() {
    // logout buttion
    auto logOutButton = new QPushButton("Log Out");

    // send buttion
    auto sendButton = new QPushButton("Send");

    // messages list
    messagesList_ = std::make_shared<QListWidget>();

    // users list
    usersList_ = std::make_shared<QListWidget>();

    // message text field
    auto messageField = new QTextEdit();

    // set positions
    auto main_layout = new QGridLayout();
    main_layout->addWidget(usersList_.get(), 0, 0, 2, 2);
    main_layout->addWidget(messagesList_.get(), 0, 1, 1, 2);
    main_layout->addWidget(messageField, 1, 1, 1, 2);
    main_layout->addWidget(logOutButton, 2, 1);
    main_layout->addWidget(sendButton, 2, 2);
    main_layout->setSpacing(0);

    setWindowTitle("Logged In as " + client_.getUsername());

    setLayout(main_layout);

    client_.SendGetAllMsgs();

    loginWidget_->close();

    show();

    // process log out
    connect(logOutButton, &QPushButton::clicked, this, [this]() {
      client_.SendLogOut();
      close();
      loginWidget_->show();
    });

    // process send
    connect(sendButton, &QPushButton::clicked, this, [this, messageField]() {
      const auto& msg = messageField->toPlainText();
      client_.SendSendMsg(selected_user_, msg);
      client_.SendGetMsgs(selected_user_);
    });

    // process get all msgs response
    connect(
        &client_, &Client::respAllMsgsReceived, this,
        [this](packet::packet_t::header_t::status_t status, const QString& msg,
               QHash<QString,
                     QList<packet::packet_t::payload_t::target_t::message_t>>
                   all_messages) {
          if (status == packet::packet_t::header_t::status_t::OK) {
            usersList_->clear();
            foreach (const auto& username, all_messages.keys()) {
              usersList_->addItem(username);
            }
            all_messages_ = all_messages;
            updateMessages_(selected_user_);
          } else {
            QMessageBox::warning(this, "Failure", msg, QMessageBox::Ok);
          }
        });

    // process notify response
    connect(&client_, &Client::respNotifyReceived, this,
            [this](packet::packet_t::header_t::status_t status,
                   const QString& msg, const QString& username) {
              if (status == packet::packet_t::header_t::status_t::OK) {
                client_.SendGetMsgs(username);
              } else {
                QMessageBox::warning(this, "Failure", msg, QMessageBox::Ok);
              }
            });

    // process get msgs response
    connect(
        &client_, &Client::respMsgsReceived, this,
        [this](packet::packet_t::header_t::status_t status, const QString& msg,
               const QString& username,
               const QList<packet::packet_t::payload_t::target_t::message_t>&
                   msgs) {
          if (status == packet::packet_t::header_t::status_t::OK) {
            all_messages_[username] = msgs;
            if (selected_user_ == username) {
              updateMessages_(selected_user_);
            }
          } else {
            QMessageBox::warning(this, "Failure", msg, QMessageBox::Ok);
          }
        });

    // process get messages for selected user
    connect(usersList_.get(), &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) {
              QString username = item->text();
              selected_user_ = username;
              updateMessages_(selected_user_);
            });
  });
}

void ChatWidget::updateMessages_(const QString& username) {
  messagesList_->clear();
  foreach (const auto& msg, all_messages_[username]) {
    if (msg.side == packet::response_json_tags::payload_target_messages_y) {
      addMessageToList_(messagesList_, new MessageBlock{"You", msg.message});
    } else if (msg.side ==
               packet::response_json_tags::payload_target_messages_t) {
      addMessageToList_(messagesList_, new MessageBlock{username, msg.message});
    }
  }
}

void ChatWidget::addMessageToList_(std::shared_ptr<QListWidget> list,
                                   MessageBlock* msgBlock) {
  auto item = new QListWidgetItem();
  item->setSizeHint(msgBlock->sizeHint());
  list->addItem(item);
  list->setItemWidget(item, msgBlock);
}

MessageBlock::MessageBlock(const QString& username, const QString& msg) {
  username_.setStyleSheet("font-weight: bold; font-size: 10pt");
  username_.setText(username);
  msg_.setText(msg);

  auto layout = new QVBoxLayout();
  layout->addWidget(&username_);
  layout->addWidget(&msg_);
  layout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

  setLayout(layout);
}

}  // namespace ui
