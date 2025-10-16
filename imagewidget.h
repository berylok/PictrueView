// imagewidget.h
#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#define ENABLE_DEBUG_LOGS 0

#if ENABLE_DEBUG_LOGS
#define DEBUG_LOG qDebug()
#else
#define DEBUG_LOG if(false) qDebug()
#endif

#include "qscrollarea.h"
#include "thumbnailwidget.h"
#include <QWidget>
#include <QPixmap>
#include <QDir>
#include <QTimer>
#include <QMap>
#include <QtConcurrent>
#include <QMutex>

#include "ConfigManager.h"  // 添加配置管理器头文件
#include "canvascontrolpanel.h"  // 添加控制面板头文件

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    enum ViewMode {
        SingleView,
        ThumbnailView
    };

    ImageWidget(QWidget *parent = nullptr);
    ~ImageWidget();

    bool loadImage(const QString &filePath, bool fromCache = false);
    void loadImageList();
    bool loadImageByIndex(int index, bool fromCache = true);
    void loadNextImage();
    void loadPreviousImage();
    void preloadAllImages();
    void clearImageCache();
    void startSlideshow();
    void stopSlideshow();
    void toggleSlideshow();
    void setSlideshowInterval(int interval); // 添加这行声明
    void slideshowNext();
    void updateWindowTitle();
    QString getShortPathName(const QString &longPath);

    void registerFileAssociation(const QString &fileExtension, const QString &fileTypeName, const QString &openCommand);
    void switchToSingleView(int index = -1);
    void switchToThumbnailView();
    void openFolder();

    // 清空缩略图缓存
    void clearThumbnailCache() {
        thumbnailWidget->clearThumbnailCache();
    }

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void closeEvent(QCloseEvent *event) override;  // 添加关闭事件处理
protected:
    void changeEvent(QEvent *event) override;


private slots:
    void onThumbnailClicked(int index);
    void onEnsureRectVisible(const QRect &rect);

private:
    void navigateThumbnails(int key);
    void toggleTitleBar();
    void toggleAlwaysOnTop();
    void toggleTransparentBackground();
    void copyImageToClipboard();
    void pasteImageFromClipboard();
    void saveImage();
    void showContextMenu(const QPoint &globalPos);
    void openImage();

    QPixmap pixmap;
    double scaleFactor;
    QPointF panOffset;
    bool isDraggingWindow;
    QPoint dragStartPosition;
    bool isPanningImage;
    QPointF panStartPosition;
    QString currentImagePath;

    QStringList imageList;
    int currentImageIndex;

    bool isSlideshowActive;
    int slideshowInterval;
    QTimer *slideshowTimer;

    QMap<QString, QPixmap> imageCache;

    ViewMode currentViewMode;
    QSize thumbnailSize;
    int thumbnailSpacing;
    QScrollArea *scrollArea;
    ThumbnailWidget *thumbnailWidget;
    QDir currentDir;

    // ini配置管理相关
    void loadConfiguration();  // 加载配置
    void saveConfiguration();  // 保存配置
    void applyConfiguration(const ConfigManager::Config &config);  // 应用配置
    // 添加当前配置对象
    ConfigManager::Config currentConfig;
    // 配置管理器
    ConfigManager *configManager;

    //多线程互斥体
private:
    QMutex cacheMutex; // 用于保护 imageCache 的访问

    //
public:
    void setCurrentDir(const QDir &dir);
public:
    void fitToWindow();
    void actualSize();
private:
    enum ViewStateType {
        ManualAdjustment,  // 手动调整
        FitToWindow,       // 合适大小
        ActualSize         // 实际大小
    };

    ViewStateType currentViewStateType;

    // 箭头指示相关
private:
    bool mouseInLeftQuarter; // 跟踪鼠标是否在左四分之一区域
    bool mouseInRightQuarter; // 跟踪鼠标是否在右四分之一区域

    // 导航提示相关
    bool mouseInImage;
    bool showNavigationHints;
    QTimer *hideHintsTimer;  // 隐藏提示的定时器

    // 鼠标位置跟踪
    QPoint lastMousePos;

    // 新增方法
    void updateNavigationHintsVisibility(const QPoint& mousePos);
    bool isMouseInImageArea(const QPoint& mousePos) const;
    void startHideHintsTimer();
    void stopHideHintsTimer();

    //键盘按下测试相关
    void testKeyboard();

public slots:
    //图片删除相关
    void deleteCurrentImage();
    void deleteSelectedThumbnail();
    void permanentlyDeleteCurrentImage();
    void permanentlyDeleteSelectedThumbnail(); // 需要实现这个方法的缩略图版本
private:
    bool moveFileToRecycleBin(const QString &filePath);


private:
    //窗口状态相关
    bool isAlwaysOnTop() const;
    bool hasTitleBar() const;
    bool hasTransparentBackground() const;

    //弹出菜单相关
    QAction *toggleTitleBarAction;
    QAction *toggleAlwaysOnTopAction;
    QAction *toggleTransparentBackgroundAction;





    void drawNavigationArrows(QPainter &painter, const QPointF &offset, const QSize &scaledSize);
    bool shouldShowNavigationArrows(const QSize &scaledSize);


    // 窗口透明度控制
    double m_windowOpacity;
    void setWindowOpacityValue(double opacity);

    // 画布模式
    bool canvasMode;

    // 鼠标穿透相关
    bool mousePassthrough;

    // 画布模式控制方法
    void toggleCanvasMode();
    void enableCanvasMode();
    void disableCanvasMode();
    bool isCanvasModeEnabled();

    // 鼠标穿透控制
    void enableMousePassthrough();
    void disableMousePassthrough();
    void ensureWindowVisible();
    void ensureFocus();


    // 画布控制面板
    CanvasControlPanel *controlPanel;

    // 控制面板方法
    void createControlPanel();
    void destroyControlPanel();
    void positionControlPanel();

    QIcon createMultiResolutionIcon();
    void createFallbackIcon();
private slots:
    // 控制面板信号槽
    void onExitCanvasMode();
    void showShortcutHelp();  // 新增：显示快捷键帮助


private:
    void createShortcutActions(); // 创建快捷键动作

    // 快捷键动作
    QAction *openFolderAction;
    QAction *openImageAction;
    QAction *saveImageAction;    // 新增：保存图片
    QAction *copyImageAction;    // 新增：拷贝图片
    QAction *pasteImageAction;   // 新增：粘贴图片

    // 关于窗口相关
    void showAboutDialog();  // 新增：显示关于对话框
    QAction *aboutAction;    // 新增：关于动作


    void switchToSingleViewDirectly(int index);
public:
    // 获取图片列表
    QStringList getImageList() const { return imageList; }

    // 设置视图状态为合适大小
    void setViewStateToFitToWindow() { currentViewStateType = FitToWindow; }

};

#endif // IMAGEWIDGET_H
