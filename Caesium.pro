#-------------------------------------------------
#
# Project created by QtCreator 2016-01-04T10:23:20
#
#-------------------------------------------------

QT       += core gui concurrent network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Caesium
TEMPLATE = app

QMAKE_MAC_SDK = macosx10.11

macx {
    QMAKE_CXXFLAGS_CXX11 = -std=gnu++1y
    CONFIG *= c++11
    QMAKE_CXXFLAGS += -stdlib=libc++
    LIBS += -L/usr/local/lib -lexiv2.14 -L/opt/mozjpeg/lib -ljpeg.62 -lturbojpeg -lzopflipng -stdlib=libc++
    INCLUDEPATH += /opt/mozjpeg/include /usr/local/include
    ICON = icons/icons/icon.icns
}

win32 {
    LIBS += -LC:\\mozjpeg\\lib -ljpeg -LC:\\exiv2\\src\\.libs -lexiv2 -lturbojpeg -lzopflipng
    INCLUDEPATH += C:\\mozjpeg\\include C:\\exiv2\\include
    RC_ICONS = icons/main/icon.ico
}

unix {
    LIBS += -ljpeg -lexiv2 -lturbojpeg -lzopflipng
}

CONFIG += warn_off c++11

SOURCES += main.cpp\
        caesium.cpp \
    aboutdialog.cpp \
    cimageinfo.cpp \
    utils.cpp \
    exif.cpp \
    preferencedialog.cpp \
    usageinfo.cpp \
    networkoperations.cpp \
    qdroptreewidget.cpp \
    ctreewidgetitem.cpp \
    clist.cpp \
    jpeg.cpp \
    lodepng.cpp \
    png.cpp

HEADERS  += caesium.h \
    aboutdialog.h \
    cimageinfo.h \
    utils.h \
    exif.h \
    preferencedialog.h \
    usageinfo.h \
    networkoperations.h \
    qdroptreewidget.h \
    ctreewidgetitem.h \
    clist.h \
    jpeg.h \
    lodepng.h \
    png.h

FORMS    += caesium.ui \
    aboutdialog.ui \
    preferencedialog.ui

RESOURCES += \
    icons.qrc \
    style.qrc

DISTFILES += \
    qss/style_osx.qss \
    qss/style_win.qss \
    qss/style_linux.qss \
    README.md \
    CHANGELOG.md \
    LICENSE \
    TODO.md

