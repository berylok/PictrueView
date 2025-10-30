// libarchivehandler.cpp
#include "libarchivehandler.h"

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

LibArchiveHandler::LibArchiveHandler(QObject *parent)
    : QObject(parent)
{
    // 支持的图片格式
    m_supportedFormats = {
        "jpg", "jpeg", "png", "bmp", "gif", "webp",
        "tiff", "tif", "ppm", "pgm", "pbm", "xbm", "xpm", "svg"
    };
}

LibArchiveHandler::~LibArchiveHandler()
{
    closeArchive();
}

bool LibArchiveHandler::isSupportedArchive(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    static QSet<QString> archiveFormats = {
        "zip", "rar", "7z", "tar", "gz", "bz2", "xz",
        "cab", "iso", "lzh", "lha", "ar", "cpio",
        "zipx", "alz", "egg", "jar", "war", "ear"
    };

    bool supported = archiveFormats.contains(suffix);
    qDebug() << "文件格式检查:" << filePath << "后缀:" << suffix << "是否支持:" << supported;

    if (suffix == "rar") {
        qDebug() << "注意: RAR 格式支持有限，加密的 RAR 文件无法读取";
    }

    return supported;
}

bool LibArchiveHandler::loadArchive(const QString &archivePath)
{
    QMutexLocker locker(&m_mutex);

    qDebug() << "=== 开始加载压缩包 ===";
    qDebug() << "压缩包路径:" << archivePath;

    if (!QFileInfo::exists(archivePath)) {
        qDebug() << "错误: 压缩包文件不存在";
        return false;
    }

    QFileInfo fileInfo(archivePath);
    qDebug() << "文件大小:" << fileInfo.size() << "字节";
    qDebug() << "文件后缀:" << fileInfo.suffix();

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
            qDebug() << "Found image in archive:" << fileName;
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

    return !m_imageEntries.isEmpty();
}

bool LibArchiveHandler::extractEntryData(const QString &entryName, QByteArray &data)
{
    if (m_currentArchive.isEmpty()) {
        return false;
    }

    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;
    bool found = false;

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    r = archive_read_open_filename(a, m_currentArchive.toLocal8Bit(), 10240);
    if (r != ARCHIVE_OK) {
        qDebug() << "Failed to open archive for extraction:" << m_currentArchive;
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

bool LibArchiveHandler::isImageFile(const QString &fileName) const
{
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix().toLower();
    return m_supportedFormats.contains(suffix);
}

QStringList LibArchiveHandler::getImageList() const
{
    QMutexLocker locker(&m_mutex);
    return m_imageEntries;
}

// 确保这个函数存在
QImage LibArchiveHandler::extractImage(int index)
{
    QMutexLocker locker(&m_mutex);

    qDebug() << "=== 开始提取图片 ===";
    qDebug() << "索引:" << index << "，总图片数:" << m_imageEntries.size();

    if (index < 0 || index >= m_imageEntries.size()) {
        qDebug() << "错误: 索引超出范围";
        return QImage();
    }

    if (m_currentArchive.isEmpty()) {
        qDebug() << "错误: 当前没有加载压缩包";
        return QImage();
    }

    QString imageName = m_imageEntries[index];
    qDebug() << "图片名称:" << imageName;

    return extractImage(imageName);
}

// 确保这个函数也存在
QImage LibArchiveHandler::extractImage(const QString &imageName)
{
    QMutexLocker locker(&m_mutex);

    qDebug() << "提取图片:" << imageName;

    if (m_currentArchive.isEmpty() || !m_imageEntries.contains(imageName)) {
        qDebug() << "错误: 图片不在列表中或压缩包未加载";
        return QImage();
    }

    QByteArray imageData;
    qDebug() << "开始提取数据...";

    if (extractEntryData(imageName, imageData)) {
        qDebug() << "数据提取成功，大小:" << imageData.size() << "字节";

        QImage image;
        if (image.loadFromData(imageData)) {
            qDebug() << "图片加载成功，尺寸:" << image.size();
            return image;
        } else {
            qDebug() << "Failed to load image data for:" << imageName;
        }
    } else {
        qDebug() << "Failed to extract image data for:" << imageName;
    }

    return QImage();
}

void LibArchiveHandler::closeArchive()
{
    QMutexLocker locker(&m_mutex);
    m_imageEntries.clear();
    m_currentArchive.clear();
}
