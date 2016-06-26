#-------------------------------------------------
#
# Project created by QtCreator 2016-06-14T17:05:11
#
#-------------------------------------------------

QT       += core gui x11extras dbus multimedia
CONFIG   += c++11
LIBS     += -lxcb -lX11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tsscreenlock
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    coverframe.cpp \
    topborderframe.cpp

HEADERS  += mainwindow.h \
    coverframe.h \
    topborderframe.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
