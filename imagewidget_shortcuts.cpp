// imagewidget_shortcuts.cpp
#include "imagewidget.h"
#include <QAction>

void ImageWidget::createShortcutActions()
{
    // 打开文件夹快捷键：Ctrl+O
    openFolderAction = new QAction(tr("打开文件夹"), this);
    openFolderAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    openFolderAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(openFolderAction, &QAction::triggered, this, &ImageWidget::openFolder);
    this->addAction(openFolderAction);

    // 打开图片快捷键：Ctrl+Shift+O
    openImageAction = new QAction(tr("打开图片"), this);
    openImageAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    openImageAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(openImageAction, &QAction::triggered, this, &ImageWidget::openImage);
    this->addAction(openImageAction);

    // 保存图片快捷键：Ctrl+S
    saveImageAction = new QAction(tr("保存图片"), this);
    saveImageAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    saveImageAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveImageAction, &QAction::triggered, this, &ImageWidget::saveImage);
    this->addAction(saveImageAction);

    // 拷贝图片快捷键：Ctrl+C
    copyImageAction = new QAction(tr("拷贝图片"), this);
    copyImageAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    copyImageAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(copyImageAction, &QAction::triggered, this,
            &ImageWidget::copyImageToClipboard);
    this->addAction(copyImageAction);

    // 粘贴图片快捷键：Ctrl+V
    pasteImageAction = new QAction(tr("粘贴图片"), this);
    pasteImageAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
    pasteImageAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(pasteImageAction, &QAction::triggered, this,
            &ImageWidget::pasteImageFromClipboard);
    this->addAction(pasteImageAction);

    // 关于窗口快捷键：F1
    aboutAction = new QAction(tr("关于"), this);
    aboutAction->setShortcut(QKeySequence(Qt::Key_F1));
    aboutAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(aboutAction, &QAction::triggered, this, &ImageWidget::showAboutDialog);
    this->addAction(aboutAction);

    qDebug() << "快捷键已创建: "
             << "Ctrl+O (打开文件夹), "
             << "Ctrl+Shift+O (打开图片), "
             << "Ctrl+S (保存图片), "
             << "Ctrl+C (拷贝图片), "
             << "Ctrl+V (粘贴图片), "
             << "F1 (关于)";

    // 新增编辑快捷键
    // 旋转快捷键：Ctrl+R 顺时针旋转90度
    QAction *rotateCwAction = new QAction(tr("顺时针旋转"), this);
    rotateCwAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    rotateCwAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(rotateCwAction, &QAction::triggered, this,
            &ImageWidget::rotate90CW);
    this->addAction(rotateCwAction);

    // 旋转快捷键：Ctrl+Shift+R 逆时针旋转90度
    QAction *rotateCcwAction = new QAction(tr("逆时针旋转"), this);
    rotateCcwAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    rotateCcwAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(rotateCcwAction, &QAction::triggered, this,
            &ImageWidget::rotate90CCW);
    this->addAction(rotateCcwAction);

    // 水平镜像快捷键：Ctrl+H
    QAction *mirrorHAction = new QAction(tr("水平镜像"), this);
    mirrorHAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    mirrorHAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(mirrorHAction, &QAction::triggered, this,
            &ImageWidget::mirrorHorizontal);
    this->addAction(mirrorHAction);

    // 垂直镜像快捷键：Ctrl+Shift+V
    QAction *mirrorVAction = new QAction(tr("垂直镜像"), this);
    mirrorVAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    mirrorVAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(mirrorVAction, &QAction::triggered, this,
            &ImageWidget::mirrorVertical);
    this->addAction(mirrorVAction);

    // 重置变换快捷键：Ctrl+0
    QAction *resetTransformAction = new QAction(tr("重置变换"), this);
    resetTransformAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    resetTransformAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(resetTransformAction, &QAction::triggered, this,
            &ImageWidget::resetTransform);
    this->addAction(resetTransformAction);

    qDebug() << "编辑快捷键已创建: "
             << "Ctrl+R (顺时针旋转), "
             << "Ctrl+Shift+R (逆时针旋转), "
             << "Ctrl+H (水平镜像), "
             << "Ctrl+Shift+V (垂直镜像), "
             << "Ctrl+0 (重置变换)";
}
