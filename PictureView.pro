QT += core gui widgets concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11
SOURCES += main.cpp \
    imagewidget.cpp \
    thumbnailwidget.cpp

HEADERS += \
    imagewidget.h \
    thumbnailwidget.h
