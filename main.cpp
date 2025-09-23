#include "imagewidget.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Image Viewer");
    app.setApplicationVersion("1.2");

    QCommandLineParser parser;
    parser.setApplicationDescription("一个支持缩略图浏览的图片查看器");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption registerOption("register", "注册文件关联");
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

        qDebug() << "文件关联已注册";
        return 0;
    }

    ImageWidget window;

    // main.cpp
    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        qDebug() << "打开文件:" << filePath;

        if (QFile::exists(filePath)) {
            QFileInfo fileInfo(filePath);
            if (fileInfo.isDir()) {
                // 使用公共方法设置当前目录
                window.setCurrentDir(QDir(filePath));
                window.loadImageList();
            } else {
                window.loadImage(filePath);
                window.switchToSingleView();
            }
        } else {
            qWarning() << "文件不存在:" << filePath;
            QMessageBox::warning(nullptr, "错误", QString("文件不存在:\n%1").arg(filePath));
        }
    }

    window.show();

    return app.exec();
}
