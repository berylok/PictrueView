// libarchiveimageprovider.cpp
#include "libarchiveimageprovider.h"

// 包含 libarchive 头文件
#ifdef _WIN32
#include <archive.h>
#include <archive_entry.h>
#else
#include <archive.h>
#include <archive_entry.h>
#endif

#include <QFileInfo>
#include <QDebug>
#include <QImage>

LibArchiveImageProvider::LibArchiveImageProvider(QObject *parent)
    : QObject(parent)
{
    // 支持的图片格式
    m_supportedFormats = {
        "jpg", "jpeg", "png", "bmp", "gif", "webp",
        "tiff", "tif", "ppm", "pgm", "pbm", "xbm", "xpm", "svg"
    };
}

LibArchiveImageProvider::~LibArchiveImageProvider()
{
    closeArchive();
}

bool LibArchiveImageProvider::loadArchive(const QString &archivePath)
{
    QMutexLocker locker(&m_mutex);

    if (!QFileInfo::exists(archivePath)) {
        qDebug() << "Archive file does not exist:" << archivePath;
        return false;
    }

    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;

    // 设置支持的格式
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    // 打开压缩包
    r = archive_read_open_filename(a, archivePath.toLocal8Bit(), 10240);
    if (r != ARCHIVE_OK) {
        qDebug() << "Failed to open archive:" << archivePath << archive_error_string(a);
        archive_read_free(a);
        return false;
    }

    m_imageEntries.clear();

    // 遍历压缩包内的文件
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *currentFileName = archive_entry_pathname(entry);
        QString fileName = QString::fromUtf8(currentFileName);

        // 跳过目录
        if (archive_entry_filetype(entry) == AE_IFDIR) {
            archive_read_data_skip(a);
            continue;
        }

        // 检查是否为图片文件
        if (isImageFile(fileName)) {
            m_imageEntries.append(fileName);
            qDebug() << "Found image:" << fileName;
        }

        archive_read_data_skip(a);
    }

    r = archive_read_close(a);
    if (r != ARCHIVE_OK) {
        qDebug() << "Error closing archive:" << archive_error_string(a);
    }
    archive_read_free(a);

    m_currentArchive = archivePath;

    qDebug() << "Loaded archive with" << m_imageEntries.size() << "images";

    // 发送信号
    emit archiveLoaded(!m_imageEntries.isEmpty());
    emit imageListUpdated();

    return !m_imageEntries.isEmpty();
}

bool LibArchiveImageProvider::extractEntryData(const QString &archivePath, const QString &entryName, QByteArray &data)
{
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;
    bool found = false;

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    r = archive_read_open_filename(a, archivePath.toLocal8Bit(), 10240);
    if (r != ARCHIVE_OK) {
        qDebug() << "Failed to open archive for extraction:" << archivePath;
        archive_read_free(a);
        return false;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *currentFileName = archive_entry_pathname(entry);
        QString fileName = QString::fromUtf8(currentFileName);

        if (fileName == entryName) {
            // 找到目标文件，读取数据
            const void *buff;
            size_t size;
            la_int64_t offset;

            data.clear();
            while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
                data.append(static_cast<const char*>(buff), static_cast<int>(size));
            }
            found = true;
            break;
        }

        archive_read_data_skip(a);
    }

    archive_read_close(a);
    archive_read_free(a);

    return found;
}

bool LibArchiveImageProvider::isImageFile(const QString &fileName) const
{
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix().toLower();
    return m_supportedFormats.contains(suffix);
}

QStringList LibArchiveImageProvider::getImageList() const
{
    QMutexLocker locker(&m_mutex);  // 现在可以正常锁定了
    return m_imageEntries;
}

QImage LibArchiveImageProvider::getImage(int index)
{
    QMutexLocker locker(&m_mutex);
    if (index < 0 || index >= m_imageEntries.size()) {
        return QImage();
    }
    return getImage(m_imageEntries[index]);
}

QImage LibArchiveImageProvider::getImage(const QString &imageName)
{
    QMutexLocker locker(&m_mutex);

    if (m_currentArchive.isEmpty() || !m_imageEntries.contains(imageName)) {
        return QImage();
    }

    QByteArray imageData;
    if (extractEntryData(m_currentArchive, imageName, imageData)) {
        QImage image;
        if (image.loadFromData(imageData)) {
            return image;
        } else {
            qDebug() << "Failed to load image data for:" << imageName;
        }
    } else {
        qDebug() << "Failed to extract image data for:" << imageName;
    }

    return QImage();
}

void LibArchiveImageProvider::closeArchive()
{
    QMutexLocker locker(&m_mutex);
    m_imageEntries.clear();
    m_currentArchive.clear();
}
