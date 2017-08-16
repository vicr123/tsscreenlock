#-------------------------------------------------
#
# Project created by QtCreator 2016-06-14T17:05:11
#
#-------------------------------------------------

QT       += core gui x11extras dbus multimedia thelib svg
CONFIG   += c++11
LIBS     += -lxcb -lX11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tsscreenlock
TEMPLATE = app
DBUS_ADAPTORS += org.thesuite.tsscreenlock.xml

SOURCES += main.cpp\
    coverframe.cpp \
    topborderframe.cpp \
    notificationdbus.cpp \
    newcall.cpp \
    timercomplete.cpp \
    lockscreen.cpp

HEADERS  += \
    coverframe.h \
    topborderframe.h \
    notificationdbus.h \
    newcall.h \
    timercomplete.h \
    lockscreen.h

FORMS    += \
    newcall.ui \
    timercomplete.ui \
    lockscreen.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    triangles.svg

unix {
    target.path = /usr/lib

    background.path = /usr/share/tsscreenlock/
    background.files = triangles.svg

    INSTALLS += target background
}
