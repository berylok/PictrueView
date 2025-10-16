#include "imagewidget.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("PictureView");
    app.setApplicationVersion("1.3.10");
    app.setOrganizationName("berylok");

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
    QCommandLineOption registerOption("register", QCoreApplication::translate("main", "Register file associations"));
    parser.addOption(registerOption);

    parser.process(app);

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

    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        qDebug() << "命令行打开文件:" << filePath;

        if (QFile::exists(filePath)) {
            QFileInfo fileInfo(filePath);
            if (fileInfo.isDir()) {
                // 文件夹：使用缩略图模式
                window.setCurrentDir(QDir(filePath));
                window.loadImageList();
                // 确保是缩略图模式
                window.switchToThumbnailView();
            } else {
                // 单张图片：使用单张视图模式
                // 重要：先设置目录和图片列表
                window.setCurrentDir(fileInfo.absoluteDir());
                window.loadImageList();

                // 找到图片在列表中的索引
                int index = window.getImageList().indexOf(fileInfo.fileName());
                qDebug() << "图片在列表中的索引:" << index;

                if (index >= 0) {
                    // 重要：使用带索引的切换方法，确保正确设置单张模式
                    window.switchToSingleView(index);
                } else {
                    // 如果不在列表中，直接加载图片并切换到单张模式
                    window.loadImage(filePath);
                    window.switchToSingleView();
                }
            }
        } else {
            qWarning() << "文件不存在:" << filePath;
            QMessageBox::warning(nullptr, "错误", "文件不存在:\n" + filePath);
        }
    }

    window.show();

    return app.exec();
}
