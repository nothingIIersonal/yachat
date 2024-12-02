#pragma once

#include <stdlib.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>

namespace common {

void logAll(QtMsgType type, const QString& msg) {
  switch (type) {
    case QtDebugMsg: {
      qDebug().noquote() << msg;
      break;
    };
    case QtWarningMsg: {
      qWarning().noquote() << msg;
      break;
    };
    case QtCriticalMsg: {
      qCritical().noquote() << msg;
      break;
    };
    case QtFatalMsg: {
      qFatal("%s", msg.toStdString().c_str());
      break;
    };
    case QtInfoMsg: {
      qInfo().noquote() << msg;
      break;
    };
    default: {
      qDebug().noquote() << msg;
      break;
    };
  }
#ifdef JOURNAL
  QFile file(JOURNAL);
  file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
  QTextStream out(&file);
  out << QDateTime::currentDateTime().toString("(yyyy-MM-dd hh:mm:ss.zzz) ")
      << msg << "\n";
  out.flush();
#endif
}

template <typename T>
struct result_t {
  bool error = true;
  T data;

  T&& unwrap() {
    if (error) {
      logAll(QtFatalMsg,
             "[COMMON | RESULT_T | UNWRAP] Error unwrapping monad data");
      QCoreApplication::exit(EXIT_FAILURE);
    }
    return std::move(data);
  }
};

};  // namespace common
