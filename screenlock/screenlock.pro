#-------------------------------------------------
#
# Project created by QtCreator 2016-06-14T17:05:11
#
#-------------------------------------------------

QT       += core gui x11extras dbus multimedia thelib svg tdesktopenvironment
CONFIG   += c++14
LIBS     += -lcrypt
SHARE_APP_NAME=tsscreenlock

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
    translations/nl_NL.ts \
    translations/sv_SE.ts

unix {
    # Include the-libs build tools
    equals(THELIBS_BUILDTOOLS_PATH, "") {
        THELIBS_BUILDTOOLS_PATH = $$[QT_INSTALL_PREFIX]/share/the-libs/pri
    }
    include($$THELIBS_BUILDTOOLS_PATH/buildmaster.pri)

    QMAKE_STRIP = echo

    target.path = $$THELIBS_INSTALL_LIB

    background.path = $$THELIBS_INSTALL_PREFIX/share/tsscreenlock/
    background.files = triangles.svg

    INSTALLS += target background
}
