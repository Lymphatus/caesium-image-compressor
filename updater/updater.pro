QT += core network
QT -= gui

CONFIG += c++11

TARGET = updater
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += src/main.cpp \
    src/updater.cpp

HEADERS += \
    src/updater.h
