#ifndef LOGIN_WIDGET_H
#define LOGIN_WIDGET_H

#include <QWidget>

#include "client.hpp"

namespace ui {

class LoginWidget final : public QWidget {
  Q_OBJECT

 public:
  explicit LoginWidget(Client &client);

 signals:
  void authDone();

 private:
  Client &client_;
};

}  // namespace ui

#endif  // LOGIN_WIDGET_H
