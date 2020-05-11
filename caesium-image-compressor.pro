QT       += core gui concurrent svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
unix: {
    QMAKE_CFLAGS += -Wno-nullability-completeness
    QMAKE_CXXFLAGS += -Wno-nullability-completeness
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = "Caesium Image Compressor"
macx: {
    ICON = resources/icons/icon.icns
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
    QMAKE_RPATHDIR += @executable_path/../Frameworks

}
win32: {
    RC_ICONS = resources/icons/icon.ico
}

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/models/CImage.cpp \
    src/models/CImageTreeItem.cpp \
    src/models/CImageTreeModel.cpp \
    src/utils/Utils.cpp \
    src/dialogs/AboutDialog.cpp \
    src/widgets/QDropTreeView.cpp \
    src/widgets/QZoomGraphicsView.cpp

HEADERS += \
    src/vendor/caesium.h \
    src/MainWindow.h \
    src/models/CImage.h \
    src/models/CImageTreeItem.h \
    src/models/CImageTreeModel.h \
    src/utils/Utils.h \
    src/dialogs/AboutDialog.h \
    src/widgets/QDropTreeView.h \
    src/widgets/QZoomGraphicsView.h

FORMS += \
    views/MainWindow.ui \
    views/AboutDialog.ui

TRANSLATIONS += \
    locale/en_US.ts

RESOURCES += \
    resources/resources.qrc

unix: {
    LIBS += -L"/usr/local/lib/caesium" -lcaesium
}

win32: {
    LIBS += -L"C:\\libcaesium\\lib" -lcaesium \
            -L"C:\\zopfli\\lib" -lzopflipng \
            -L"C:\\mozjpeg\\lib" -ljpeg \
            -L"C:\\mozjpeg\\lib" -lturbojpeg \
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Info.plist \
    README.md \
    CHANGELOG.md
