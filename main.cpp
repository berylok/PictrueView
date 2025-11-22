#include "imagewidget.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#include <QImageReader>  // 添加这个头文件

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("PictureView");
    app.setApplicationVersion("1.4.0.0");
    app.setOrganizationName("berylok");

    // === 关键修复：提高 Qt 图像内存限制 ===
    // 获取当前限制
    int currentLimit = QImageReader::allocationLimit();
    qDebug() << "Current image allocation limit:" << currentLimit << "MB";

    // 设置新的限制为 1GB (1024 MB) 或完全禁用 (0)
    QImageReader::setAllocationLimit(1024);  // 1GB
    qDebug() << "New image allocation limit:" << QImageReader::allocationLimit() << "MB";

    // 检查支持的图像格式（用于调试）
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    qDebug() << "Supported image formats:" << supportedFormats;

    // 检查关键格式支持
    if (!supportedFormats.contains("webp")) {
        qWarning() << "WebP format not supported - consider installing additional plugins";
    }
    if (!supportedFormats.contains("jpeg")) {
        qWarning() << "JPEG format not supported";
    }

    // 创建翻译器
    QTranslator appTranslator;
    QTranslator qtTranslator;

    // 设置默认语言（中文）
    QString locale = "zh_CN";

    // 检查命令行参数是否指定语言
    QCommandLineParser parser;
    QCommandLineOption langOption("lang", "Set language (zh_CN, en_US)", "language");
    parser.addOption(langOption);
    parser.process(app);

    if (parser.isSet(langOption)) {
        locale = parser.value(langOption);
    }

    // 加载Qt自带的标准对话框翻译
    QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (qtTranslator.load("qt_" + locale, qtTranslationsPath)) {
        app.installTranslator(&qtTranslator);
    }

    // 加载应用程序翻译
    QString appTranslationsPath = QApplication::applicationDirPath() + "/translations";
    if (appTranslator.load("PictureView_" + locale, appTranslationsPath)) {
        app.installTranslator(&appTranslator);
        qDebug() << "Loaded translation for locale:" << locale;
    } else {
        qDebug() << "Failed to load translation for locale:" << locale;
        // 尝试从资源文件加载
        if (appTranslator.load(":/translations/PictureView_" + locale)) {
            app.installTranslator(&appTranslator);
            qDebug() << "Loaded translation from resources for locale:" << locale;
        }
    }

    // 注册文件关联选项
    QCommandLineOption registerOption(
        "register",
        QCoreApplication::translate("main", "Register file associations"));
    parser.addOption(registerOption);

    // 添加调试选项
    QCommandLineOption debugOption("debug", "Enable debug output");
    parser.addOption(debugOption);

    // 添加内存限制选项
    QCommandLineOption memoryOption("memory-limit",
                                    "Set image memory limit in MB (0=unlimited)",
                                    "mb", "1024");
    parser.addOption(memoryOption);

    parser.process(app);

    // 处理内存限制选项
    if (parser.isSet(memoryOption)) {
        bool ok;
        int memoryLimit = parser.value(memoryOption).toInt(&ok);
        if (ok && memoryLimit >= 0) {
            QImageReader::setAllocationLimit(memoryLimit);
            qDebug() << "Custom image memory limit set to:" << memoryLimit << "MB";
        }
    }

    if (parser.isSet(registerOption)) {
        QString exePath = QCoreApplication::applicationFilePath();
        QString shortExePath = ImageWidget().getShortPathName(exePath);
        QString openCommand = QString("\"%1\" \"%2\"").arg(shortExePath).arg("%1");

        ImageWidget().registerFileAssociation("png", "pngfile", openCommand);
        ImageWidget().registerFileAssociation("jpg", "jpgfile", openCommand);
        ImageWidget().registerFileAssociation("bmp", "bmpfile", openCommand);
        ImageWidget().registerFileAssociation("jpeg", "jpegfile", openCommand);
        ImageWidget().registerFileAssociation("webp", "webpfile", openCommand);
        ImageWidget().registerFileAssociation("gif", "giffile", openCommand);
        ImageWidget().registerFileAssociation("tiff", "tifffile", openCommand);
        ImageWidget().registerFileAssociation("tif", "tiffile", openCommand);

        qDebug() << QCoreApplication::translate("main", "File associations registered");
        return 0;
    }

    ImageWidget window;

    // 启用调试输出（如果指定了调试选项）
    if (parser.isSet(debugOption)) {
        qDebug() << "Debug mode enabled";
        // 你可以在这里添加更多的调试初始化
    }

    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        qDebug() << QCoreApplication::translate("main", "Opening file:") << filePath;

        if (QFile::exists(filePath)) {
            QFileInfo fileInfo(filePath);

            // 检查文件大小，避免加载过大的文件
            qint64 fileSize = fileInfo.size();
            if (fileSize > 500 * 1024 * 1024) { // 500MB
                qWarning() << "File too large:" << filePath << "Size:" << fileSize / (1024 * 1024) << "MB";
                QMessageBox::warning(
                    nullptr,
                    QCoreApplication::translate("main", "文件过大"),
                    QCoreApplication::translate("main", "文件过大 (%1 MB)，可能无法加载:\n%2")
                        .arg(fileSize / (1024 * 1024))
                        .arg(filePath)
                    );
            } else {
                if (fileInfo.isDir()) {
                    window.setCurrentDir(QDir(filePath));
                    window.loadImageList();
                } else {
                    window.loadImage(filePath);
                    window.switchToSingleView();
                }
            }
        } else {
            qWarning() << QCoreApplication::translate("main",
                                                      "File does not exist:")
                       << filePath;
            QMessageBox::warning(
                nullptr, QCoreApplication::translate("main", "错误"),
                QCoreApplication::translate("main", "文件不存在:\n%1")
                    .arg(filePath));
        }
    }

    window.show();

    return app.exec();
}
