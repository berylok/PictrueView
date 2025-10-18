#ifndef THUMBNAILWIDGET_H
#define THUMBNAILWIDGET_H

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

signals:
    void thumbnailClicked(int index);
    void ensureRectVisible(const QRect &rect);
    void loadingProgress(int loaded, int total);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override; // 只有声明，没有实现
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateThumbnails();

private:
    void selectThumbnailAtPosition(const QPoint &pos);

    QPixmap loadThumbnail(const QString &path);

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

public:
    static void clearThumbnailCacheForImage(const QString &imagePath);
    void ensureVisible(int index);
};

#endif // THUMBNAILWIDGET_H
