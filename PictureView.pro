# PictureView.pro - 优化版本
QT += core gui widgets concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11

# 发布版优化设置
CONFIG(release, debug|release) {
    # 使用O2优化而不是O3
    QMAKE_CXXFLAGS_RELEASE -= -O3
    QMAKE_CXXFLAGS_RELEASE += -O2

    # 禁用可能产生问题的优化
    QMAKE_CXXFLAGS_RELEASE -= -ffast-math

    # 启用SSE优化（如果目标平台支持）
    QMAKE_CXXFLAGS_RELEASE += -msse2
}

# 调试版设置
CONFIG(debug, debug|release) {
    # 确保调试版有足够的调试信息但不过度优化
    QMAKE_CXXFLAGS_DEBUG += -O1
}

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

RESOURCES += \
    app.qrc

TRANSLATIONS += \
    translations/PictureView_zh_CN.ts \
    translations/PictureView_en_US.ts

DEFAULT_LANG = zh_CN
RC_ICONS = icons/PictureView.ico

VERSION = 1.3.11.0
QMAKE_TARGET_COMPANY = "berylok"
QMAKE_TARGET_DESCRIPTION = "PictureView"
QMAKE_TARGET_COPYRIGHT = "Copyright @ 2025 berylok"
QMAKE_TARGET_PRODUCT = "PictureView"
