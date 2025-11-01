QT += core gui widgets concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11

# 添加 libarchive 库
# 根据你的 vcpkg 安装路径调整
VCPKG_ROOT = $$(VCPKG_ROOT)
isEmpty(VCPKG_ROOT) {
    VCPKG_ROOT = j:/vcpkg  # 修改为你的 vcpkg 路径
}

INCLUDEPATH += $$VCPKG_ROOT/installed/x64-windows/include
LIBS += -L$$VCPKG_ROOT/installed/x64-windows/lib -larchive

# 如果是 Linux/macOS，使用：
# INCLUDEPATH += $$VCPKG_ROOT/installed/x64-linux/include
# LIBS += -L$$VCPKG_ROOT/installed/x64-linux/lib -larchive

SOURCES += main.cpp \
    archivehandler.cpp \
    canvascontrolpanel.cpp \
    configmanager.cpp \
    imagewidget_archive.cpp \
    imagewidget_canvas.cpp \
    imagewidget_config.cpp \
    imagewidget_core.cpp \
    imagewidget_file.cpp \
    imagewidget_fileops.cpp \
    imagewidget_help.cpp \
    imagewidget_keyboard.cpp \
    imagewidget_menu.cpp \
    imagewidget_mouse.cpp \
    imagewidget_shortcuts.cpp \
    imagewidget_slideshow.cpp \
    imagewidget_transform.cpp \
    imagewidget_view.cpp \
    imagewidget_viewmode.cpp \
    thumbnailwidget.cpp

HEADERS += \
    archivehandler.h \
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
VERSION = 1.4.0.0
QMAKE_TARGET_COMPANY = "berylok"
QMAKE_TARGET_DESCRIPTION = "berylok app"
QMAKE_TARGET_COPYRIGHT = "Copyright @ 2025 berylok"
QMAKE_TARGET_PRODUCT = "PictureView"

DISTFILES +=
