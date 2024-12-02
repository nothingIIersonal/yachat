#include "login_widget.h"

#include <QGridLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include "packet.hpp"

namespace ui {

LoginWidget::LoginWidget(Client& client) : client_(client) {
  // login ui
  auto loginLabel = new QLabel("Login");
  auto loginLineEdit = new QLineEdit();

  // password ui
  auto passwordLabel = new QLabel("Password");
  auto passwordLineEdit = new QLineEdit();
  passwordLineEdit->setEchoMode(QLineEdit::EchoMode::Password);

  // login buttion
  auto logInButton = new QPushButton("Log In");

  // register button
  auto registerButton = new QPushButton("Register");

  // set positions
  auto main_layout = new QGridLayout();
  main_layout->addWidget(loginLabel, 0, 0);
  main_layout->addWidget(loginLineEdit, 0, 1);
  main_layout->addWidget(passwordLabel, 1, 0);
  main_layout->addWidget(passwordLineEdit, 1, 1);
  main_layout->addWidget(registerButton, 2, 0);
  main_layout->addWidget(logInButton, 2, 1);
  main_layout->setSpacing(0);

  setLayout(main_layout);

  setFixedSize(300, 150);

  show();

  // process log in
  connect(logInButton, &QPushButton::clicked, this,
          [this, loginLineEdit, passwordLineEdit]() {
            const auto username = loginLineEdit->text();
            const auto password = passwordLineEdit->text();
            client_.SendLogIn(username, password);
          });

  // process register
  connect(registerButton, &QPushButton::clicked, this,
          [this, loginLineEdit, passwordLineEdit]() {
            const auto username = loginLineEdit->text();
            const auto password = passwordLineEdit->text();
            client_.SendRegister(username, password);
          });

  // process status response
  connect(
      &client_, &Client::respStatusReceived, this,
      [this](packet::packet_t::header_t::status_t status, const QString& msg) {
        if (status != packet::packet_t::header_t::status_t::OK) {
          QMessageBox::warning(this, "Failure", msg, QMessageBox::Ok);
        }
      });

  // process auth response
  connect(
      &client_, &Client::respAuthReceived, this,
      [this](packet::packet_t::header_t::status_t status, const QString& msg) {
        if (status != packet::packet_t::header_t::status_t::OK) {
          QMessageBox::warning(this, "Failure", msg, QMessageBox::Ok);
        } else {
          emit authDone();
        }
      });
}

}  // namespace ui
