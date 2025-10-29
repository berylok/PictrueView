QT += core gui widgets concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11

SOURCES += main.cpp \
    canvascontrolpanel.cpp \
    configmanager.cpp \
    imagewidget.cpp \
    thumbnailwidget.cpp

HEADERS += \
    canvascontrolpanel.h \
    configmanager.h \
    imagewidget.h \
    thumbnailwidget.h

# 资源文件
RESOURCES += \
    app.qrc

# 翻译支持
TRANSLATIONS += \
    # translations/PictureView_zh_CN.ts \
    translations/PictureView_en_US.ts

# 设置默认语言
DEFAULT_LANG = zh_CN

# .pro 文件 - 设置应用程序图标（必须用 ICO）
RC_ICONS = icons/PictureView.ico

# 基本应用信息
VERSION = 1.3.8.1
QMAKE_TARGET_COMPANY = "berylok"
QMAKE_TARGET_DESCRIPTION = "berylok app"
QMAKE_TARGET_COPYRIGHT = "Copyright @ 2025 berylok"
QMAKE_TARGET_PRODUCT = "PictureView"

DISTFILES +=
