// libarchiveimageprovider.h
#ifndef LIBARCHIVEIMAGEPROVIDER_H
#define LIBARCHIVEIMAGEPROVIDER_H

#include <QObject>
#include <QImage>
#include <QStringList>
#include <QMutex>
#include <QSet>

class LibArchiveImageProvider : public QObject
{
    Q_OBJECT

public:
    explicit LibArchiveImageProvider(QObject *parent = nullptr);
    ~LibArchiveImageProvider();

    // 加载压缩包
    bool loadArchive(const QString &archivePath);

    // 获取图片列表
    QStringList getImageList() const;

    // 通过索引获取图片
    QImage getImage(int index);

    // 通过文件名获取图片
    QImage getImage(const QString &imageName);

    // 关闭当前压缩包
    void closeArchive();

    // 获取当前压缩包路径
    QString getCurrentArchive() const { return m_currentArchive; }

    // 获取图片数量
    int imageCount() const { return m_imageEntries.size(); }

signals:
    void archiveLoaded(bool success);
    void imageListUpdated();

private:
    bool extractEntryData(const QString &archivePath, const QString &entryName, QByteArray &data);
    bool isImageFile(const QString &fileName) const;

    QString m_currentArchive;
    QStringList m_imageEntries; // 图片文件名列表
    QSet<QString> m_supportedFormats;
    mutable QMutex m_mutex;  // 添加 mutable 关键字
};

#endif // LIBARCHIVEIMAGEPROVIDER_H
