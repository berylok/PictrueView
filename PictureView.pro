QT += core gui widgets concurrent quick
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11

SOURCES += main.cpp \
    canvascontrolpanel.cpp \
    configmanager.cpp \
    imagewidget.cpp \
    libarchivehandler.cpp \
    libarchiveimageprovider.cpp \
    thumbnailwidget.cpp

HEADERS += \
    canvascontrolpanel.h \
    configmanager.h \
    imagewidget.h \
    libarchivehandler.h \
    libarchiveimageprovider.h \
    thumbnailwidget.h

# 读取压缩包功能所需依赖，使用vcpkg安装的libarchive
# 设置vcpkg根目录 - 根据您的实际路径调整
# 读取压缩包功能所需依赖
VCPKG_ROOT = $$(VCPKG_ROOT)
isEmpty(VCPKG_ROOT) {
    VCPKG_ROOT = J:/vcpkg
}

VCPKG_TARGET_TRIPLET = x64-windows

INCLUDEPATH += $${VCPKG_ROOT}/installed/$${VCPKG_TARGET_TRIPLET}/include
LIBS += -L$${VCPKG_ROOT}/installed/$${VCPKG_TARGET_TRIPLET}/lib

# 使用动态链接，链接 archive.lib
LIBS += -larchive

# Windows 依赖库
win32 {
    LIBS += -lbcrypt
    LIBS += -luser32
    LIBS += -lole32
    LIBS += -lws2_32
    LIBS += -ladvapi32
    LIBS += -lshlwapi
}

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
