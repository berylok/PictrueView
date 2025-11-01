#include "archivehandler.h"
#include <QFileInfo>
#include <QDebug>

ArchiveHandler::ArchiveHandler() : archive(nullptr)
{
}

ArchiveHandler::~ArchiveHandler()
{
    closeArchive();
}

bool ArchiveHandler::isSupportedArchive(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    // 支持的压缩格式
    return (suffix == "zip" || suffix == "rar" || suffix == "7z" ||
            suffix == "tar" || suffix == "gz" || suffix == "bz2");
}

bool ArchiveHandler::openArchive(const QString &filePath)
{
    closeArchive();

    archive = archive_read_new();
    archive_read_support_format_all(archive);
    archive_read_support_filter_all(archive);

    int r = archive_read_open_filename(
        archive, filePath.toLocal8Bit().constData(), 10240);
    if (r != ARCHIVE_OK) {
        qDebug() << "Failed to open archive:" << filePath
                 << archive_error_string(archive);
        archive_read_free(archive);
        archive = nullptr;
        return false;
    }

    archivePath = filePath;
    return true;
}

void ArchiveHandler::closeArchive()
{
    if (archive) {
        archive_read_close(archive);
        archive_read_free(archive);
        archive = nullptr;
    }
    archivePath.clear();
}

QStringList ArchiveHandler::getImageFiles()
{
    QStringList imageFiles;

    if (!archive) return imageFiles;

    // 重置读取位置
    archive_read_free(archive);
    archive = archive_read_new();
    archive_read_support_format_all(archive);
    archive_read_support_filter_all(archive);
    archive_read_open_filename(archive, archivePath.toLocal8Bit().constData(), 10240);

    struct archive_entry *entry;
    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        const char *filename = archive_entry_pathname(entry);
        if (filename && isImageFile(QString::fromUtf8(filename))) {
            imageFiles.append(QString::fromUtf8(filename));
        }
        archive_read_data_skip(archive);
    }

    // 重新打开以准备后续读取
    archive_read_free(archive);
    archive = archive_read_new();
    archive_read_support_format_all(archive);
    archive_read_support_filter_all(archive);
    archive_read_open_filename(archive, archivePath.toLocal8Bit().constData(), 10240);

    return imageFiles;
}

QByteArray ArchiveHandler::extractFile(const QString &filePath)
{
    QByteArray data;

    if (!archive) return data;

    struct archive_entry *entry;
    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        const char *filename = archive_entry_pathname(entry);
        if (filename && QString::fromUtf8(filename) == filePath) {
            // 找到文件，读取数据
            const void *buff;
            size_t size;
            la_int64_t offset;

            while (archive_read_data_block(archive, &buff, &size, &offset) ==
                   ARCHIVE_OK) {
                data.append(static_cast<const char *>(buff), size);
            }
            break;
        }
        archive_read_data_skip(archive);
    }

    // 重置读取位置以便下次查找
    archive_read_free(archive);
    archive = archive_read_new();
    archive_read_support_format_all(archive);
    archive_read_support_filter_all(archive);
    archive_read_open_filename(archive, archivePath.toLocal8Bit().constData(), 10240);

    return data;
}

bool ArchiveHandler::isImageFile(const QString &fileName)
{
    QString lowerName = fileName.toLower();
    return (lowerName.endsWith(".png") || lowerName.endsWith(".jpg") ||
            lowerName.endsWith(".jpeg") || lowerName.endsWith(".bmp") ||
            lowerName.endsWith(".webp") || lowerName.endsWith(".gif") ||
            lowerName.endsWith(".tiff") || lowerName.endsWith(".tif"));
}
