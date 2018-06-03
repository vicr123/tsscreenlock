#-------------------------------------------------
#
# Project created by QtCreator 2016-06-14T17:05:11
#
#-------------------------------------------------

QT       += core gui x11extras dbus multimedia thelib svg
CONFIG   += c++14
LIBS     += -lcrypt

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += x11 xcb xi
}

TARGET = tsscreenlock
TEMPLATE = app
DBUS_ADAPTORS += org.thesuite.tsscreenlock.xml

DEFINES += "SHAREDIR=\\\"/usr/share/tsscreenlock/\\\""

SOURCES += main.cpp\
    coverframe.cpp \
    topborderframe.cpp \
    notificationdbus.cpp \
    newcall.cpp \
    timercomplete.cpp \
    lockscreen.cpp \
    switchuserlistdelegate.cpp \
    underlineanimation.cpp

HEADERS  += \
    coverframe.h \
    topborderframe.h \
    notificationdbus.h \
    newcall.h \
    timercomplete.h \
    lockscreen.h \
    switchuserlistdelegate.h \
    underlineanimation.h

FORMS    += \
    newcall.ui \
    timercomplete.ui \
    lockscreen.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    triangles.svg

TRANSLATIONS += translations/vi_VN.ts \
    translations/au_AU.ts \
    translations/de_DE.ts \
    translations/da_DK.ts \
    translations/cy_GB.ts \
    translations/nl_NL.ts

unix {
    QMAKE_STRIP = echo

    target.path = /usr/lib

    background.path = /usr/share/tsscreenlock/
    background.files = triangles.svg

    translations.path = /usr/share/tsscreenlock/translations
    translations.files = translations/*

    INSTALLS += target background translations
}
