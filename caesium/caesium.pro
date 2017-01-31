QT       += core gui concurrent network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Caesium
TEMPLATE = app

QMAKE_MAC_SDK = macosx10.11

macx {
    QMAKE_CXXFLAGS_CXX11 = -std=gnu++1y
    CONFIG *= c++11
    QMAKE_CXXFLAGS += -stdlib=libc++
    LIBS += -L/usr/local/lib -lexiv2 -lcaesium
    INCLUDEPATH += /usr/local/include
    ICON = icons/icons/icon.icns
}

win32 {
    LIBS +=  -LC:\\exiv2\\src\\.libs -lexiv2 -lcaesium
    INCLUDEPATH += C:\\exiv2\\include
    RC_ICONS = icons/main/icon.ico
}

unix {
    LIBS += -lcaesium -lexiv2
}

CONFIG += warn_off c++11

SOURCES += src/main.cpp\
    src/aboutdialog.cpp \
    src/utils.cpp \
    src/exif.cpp \
    src/preferencedialog.cpp \
    src/networkoperations.cpp \
    src/qdroptreewidget.cpp \
    src/clist.cpp \
    src/cimage.cpp \
    src/ctreewidgetitem.cpp \
    src/caesium.cpp

HEADERS  += \
    src/aboutdialog.h \
    src/utils.h \
    src/exif.h \
    src/preferencedialog.h \
    src/networkoperations.h \
    src/qdroptreewidget.h \
    src/ctreewidgetitem.h \
    src/clist.h \
    src/cimage.h \
    src/caesium.h

FORMS    += \
    src/aboutdialog.ui \
    src/preferencedialog.ui \
    src/caesium.ui

RESOURCES += \
    icons.qrc \
    style.qrc

DISTFILES += \
    qss/style_osx.qss \
    qss/style_win.qss \
    qss/style_linux.qss

TRANSLATIONS = translations/caesium_en_US.ts \
    translations/caesium_it_IT.ts
