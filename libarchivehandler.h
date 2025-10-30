// libarchivehandler.h
#ifndef LIBARCHIVEHANDLER_H
#define LIBARCHIVEHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QMutex>
#include <QSet>

class LibArchiveHandler : public QObject
{
    Q_OBJECT

public:
    explicit LibArchiveHandler(QObject *parent = nullptr);
    ~LibArchiveHandler();

    // 检查文件是否为支持的压缩格式
    static bool isSupportedArchive(const QString &filePath);

    // 加载压缩包并获取图片列表
    bool loadArchive(const QString &archivePath);

    // 获取压缩包内的图片列表
    QStringList getImageList() const;

    // 从压缩包中提取图片
    QImage extractImage(const QString &imageName);
    QImage extractImage(int index);  // 确保这行存在

    // 关闭当前压缩包
    void closeArchive();

    // 获取当前压缩包路径
    QString getCurrentArchive() const { return m_currentArchive; }

    // 获取图片数量
    int imageCount() const { return m_imageEntries.size(); }

signals:
    void archiveLoaded(bool success);

private:
    bool extractEntryData(const QString &entryName, QByteArray &data);
    bool isImageFile(const QString &fileName) const;

    QString m_currentArchive;
    QStringList m_imageEntries;
    QSet<QString> m_supportedFormats;
    mutable QMutex m_mutex;
};

#endif // LIBARCHIVEHANDLER_H
