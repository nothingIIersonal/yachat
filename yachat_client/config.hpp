#pragma once

#include <stdlib.h>

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

class Config final {
 public:
  explicit Config(const QString& filename) {
    QFile config_file(filename);
    if (!config_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qFatal("Can't open configuration JSON file");
      QApplication::exit(EXIT_FAILURE);
    }

    const auto val = config_file.readAll();
    const auto dat = QJsonDocument::fromJson(val).object();

    server_ip_ = dat.value("serverip").toString();
    server_port_ = dat.value("serverport").toString().toUInt();
  }

  auto GetIP() const -> QString { return server_ip_; }
  auto GetPort() const -> quint16 { return server_port_; }

 private:
  QString server_ip_;
  quint16 server_port_;
};
