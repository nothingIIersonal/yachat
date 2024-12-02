QT = core
QT += sql
QT += network

CONFIG += c++17 cmdline network

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        src/main.cpp

HEADERS += \
        src/auth.hpp \
        src/common.hpp \
        src/db.hpp \
        src/msg.hpp \
        src/packet.hpp \
        src/server.hpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# vars
DB_PATH = $$PWD/db/db.sqlite
DEFINES += "DB_PATH='\"$$DB_PATH\"'"
