QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

CONFIG += c++17

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    ui/login_widget.cpp \
    ui/chat_widget.cpp

HEADERS += \
    common.hpp \
    config.hpp \
    packet.hpp \
    client.hpp \
    ui/login_widget.h \
    ui/chat_widget.h

CONFIG += file_copies
COPIES += json_files

json_files.files = $$PWD/config.json
json_files.path = $$OUT_PWD

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
