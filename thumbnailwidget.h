// thumbnailwidget.h
#ifndef THUMBNAILWIDGET_H
#define THUMBNAILWIDGET_H

#include "qfuturewatcher.h"
#include <QWidget>
#include <QPixmap>
#include <QDir>
#include <QStringList>
#include <QMap>
#include <QMutex>

class ImageWidget;  // 前向声明

class ThumbnailWidget : public QWidget
{
    Q_OBJECT

public:
    // 只有一个构造函数声明
    ThumbnailWidget(ImageWidget *imageWidget = nullptr, QWidget *parent = nullptr);
    ~ThumbnailWidget();

    void setImageList(const QStringList &list, const QDir &dir);
    void setSelectedIndex(int index);
    int getSelectedIndex() const;
    void ensureVisible(int index);
    void clearThumbnailCache();
    static void clearThumbnailCacheForImage(const QString &imagePath);
    void stopLoading();

    // 信号
signals:
    void thumbnailClicked(int index);
    void ensureRectVisible(const QRect &rect);
    void loadingProgress(int loaded, int total);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    bool isArchiveFile(const QString &fileName) const;
    QPixmap createArchiveIcon() const;
    QPixmap loadThumbnail(const QString &path);
    void updateThumbnails();
    void selectThumbnailAtPosition(const QPoint &pos);

    ImageWidget *imageWidget;
    QSize thumbnailSize;
    int thumbnailSpacing;
    QStringList imageList;
    QDir currentDir;
    int selectedIndex;

    // 加载相关
    int loadedCount;
    int totalCount;
    QFutureWatcher<QPixmap> *futureWatcher;
    bool isLoading;

    // 静态缓存
    static QMap<QString, QPixmap> thumbnailCache;
    static QMutex cacheMutex;
};

#endif // THUMBNAILWIDGET_H
