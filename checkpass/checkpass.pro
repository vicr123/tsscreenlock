QT += core
QT -= gui
LIBS += -lcrypt

CONFIG += c++11

TARGET = tscheckpass
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

unix {
    target.path = /usr/bin

    suid.path = /usr/bin
    suid.extra = chmod u+s tscheckpass

    INSTALLS += target suid
}
