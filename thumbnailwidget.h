#ifndef THUMBNAILWIDGET_H
#define THUMBNAILWIDGET_H

// 在文件开头添加
#define THUMBNAIL_CACHE_MAX_SIZE 200

#include <QWidget>
#include <QPixmap>
#include <QDir>
#include <QMap>
#include <QStringList>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QFuture>
#include <QtConcurrent>
#include <QMenu>
#include <QAction>
#include <QQueue>

class ThumbnailWidget : public QWidget
{
    Q_OBJECT

public:
    ThumbnailWidget(QWidget *parent = nullptr);
    ~ThumbnailWidget();

    void setImageList(const QStringList &list, const QDir &dir);
    void setSelectedIndex(int index);
    int getSelectedIndex() const;
    void stopLoading();
    void clearThumbnailCache();
    void ensureVisible(int index);
    static void clearThumbnailCacheForImage(const QString &imagePath);

signals:
    void thumbnailClicked(int index);
    void ensureRectVisible(const QRect &rect);
    void loadingProgress(int loaded, int total);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateThumbnails();
    void onThumbnailLoaded(const QString &path, const QPixmap &thumbnail);
    void processPreloadQueue();

private:
    void selectThumbnailAtPosition(const QPoint &pos);
    QPixmap loadThumbnail(const QString &path);
    void schedulePreload(int centerIndex);
    void cleanupThumbnailCache();

    QSize thumbnailSize;
    int thumbnailSpacing;
    QStringList imageList;
    QDir currentDir;
    int selectedIndex;

    static QMap<QString, QPixmap> thumbnailCache;
    static QMutex cacheMutex;

    int loadedCount;
    int totalCount;
    QFutureWatcher<QPixmap> *futureWatcher;
    bool isLoading;

    // 预加载相关
    QQueue<QString> preloadQueue;
    const int preloadQueueCapacity = 50;
    QRect lastVisibleRect;



};

#endif // THUMBNAILWIDGET_H
