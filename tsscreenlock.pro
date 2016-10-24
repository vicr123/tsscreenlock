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
DBUS_ADAPTORS += org.thesuite.tsscreenlock.xml

SOURCES += main.cpp\
        mainwindow.cpp \
    coverframe.cpp \
    topborderframe.cpp \
    notificationdbus.cpp \
    newcall.cpp

HEADERS  += mainwindow.h \
    coverframe.h \
    topborderframe.h \
    notificationdbus.h \
    newcall.h

FORMS    += mainwindow.ui \
    newcall.ui

RESOURCES += \
    resources.qrc
