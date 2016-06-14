#-------------------------------------------------
#
# Project created by QtCreator 2016-06-14T17:05:11
#
#-------------------------------------------------

QT       += core gui x11extras
CONFIG   += c++11
LIBS     += -lxcb -lX11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tsscreenlock
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
