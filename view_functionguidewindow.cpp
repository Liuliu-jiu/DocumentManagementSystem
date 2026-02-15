#include "view_functionguidewindow.h"
#include "ui_view_functionguidewindow.h"

View_FunctionGuideWindow::View_FunctionGuideWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::View_FunctionGuideWindow)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/new/prefix1/image/文档管理系统.ico"));
    this->setWindowTitle("功能指引窗口");
    //利用按钮进行上下页切换图片和文字
    //实现效果：
    //1.图片和控件初始大小跟scroeArea一样
    //2.缩放窗口时，图片和控件的大小要和scroeArea窗口一致
    //问题：
    //1.当我初始化功能指引窗口时，为什么图片的宽度会距离窗口右边界有一段距离呢，当我缩放大小时又恢复正常
    //2.当我缩放窗口时，窗口y轴不断自动增加1
    // QTimer::singleShot(10, this, [=]() {
    //     //ui->label->setGeometry(ui->create->geometry());
    //     // qDebug() << "ui->label->size()" <<ui->label->size();
    //     // qDebug() << "ui->label->geometry()" <<ui->label->geometry();
    //     // qDebug() << "ui->create->geometry()" <<ui->create->geometry();
    //     // qDebug() << "ui->label->geometry().size()" <<ui->label->geometry().size();
    //     ui->label->setGeometry(ui->create->rect());
    //     ui->label->setPixmap(QPixmap(":/new/createGuide/image/CreationSteps/creationSteps0.png").scaled(ui->label->geometry().size()));
    //     //qDebug() << "ui->label->pixmap().size()" << ui->label->pixmap().size();
    // });
    //初始化创建标签页控件和图片
    // this->currentPixampPath = ":/new/createGuide/image/CreationSteps/creationSteps0.png";
    ui->showCreatePixmapLabel->setGeometry(ui->createWidget->rect());
    // ui->showCreatePixmapLabel->setPixmap(QPixmap(":/new/createGuide/image/CreationSteps/creationSteps0.png").scaled(ui->showCreatePixmapLabel->size()));


    // //初始化打开系统文件标签页控件和图片
    ui->showOpenSystemFileLabel->setGeometry(ui->openSystemFileWidget->geometry());
    // ui->showOpenSystemFileLabel->setPixmap(QPixmap(":/new/openSystemFileGuide/image/OpenSystemFileSteps/openSystemFileSteps0.png").scaled(ui->showOpenSystemFileLabel->size()));

    // //初始化查找单个路径标签页控件和图片
    ui->showFindOnePathLabel->setGeometry(ui->findOnePathWidget->geometry());
    // ui->showFindOnePathLabel->setPixmap(QPixmap(":/new/findOnePathGuide/image/FindOnePathSteps/findOnePathSteps0.png").scaled(ui->showFindOnePathLabel->size()));

    // //初始化查找全部路径标签页控件和图片
     ui->showFindAllPathLabel->setGeometry(ui->findOnePathWidget->geometry());
    // ui->showFindAllPathLabel->setPixmap(QPixmap(":/new/findAllSystemFile/image/FindAllSystemFileSteps/findAllSystemFileSteps0.png").scaled(ui->showFindAllPathLabel->size()));
    on_nextPagePushButton_clicked();
    ui->showFunctionGuideTabWidget->setCurrentIndex(0);
}

View_FunctionGuideWindow::~View_FunctionGuideWindow()
{
    qDebug() << ui->showCreatePixmapLabel->size();
    delete ui;
}
void View_FunctionGuideWindow::resizeEvent(QResizeEvent* event)
{
    // qDebug() << "ui->create->rect()：" <<ui->create->rect();
    // qDebug() << "ui->label->size()：" <<ui->label->size();
    ui->showCreatePixmapLabel->setGeometry(ui->createWidget->rect());
    ui->showCreatePixmapLabel->setPixmap(QPixmap(this->currentPixampPath).scaled(ui->showCreatePixmapLabel->size()));
    //QWidget::resizeEvent(event);
}
void View_FunctionGuideWindow::on_nextPagePushButton_clicked()
{
    static int currentCreateIndex = -1;
    static int currentOpenSystemFilePixmapIndex = -1;
    static int currentFindOnePathPixmapIndex = -1;
    static int currentFineAllPathPixmapIndex = -1;
    QString pixmapPath;
    int index = ui->showFunctionGuideTabWidget->currentIndex();
    //判断是哪个标签页，当处于创建标签页时，就在当前页切换创建步骤的图片
    if(index == 0)
    {
        currentCreateIndex++;
        if(currentCreateIndex >= CREATE_PIXMAP_COUNT)
        {
            currentCreateIndex = 0;
        }
        pixmapPath = QString(":/new/createGuide/image/CreationSteps/creationSteps%1.png").arg(currentCreateIndex);
        ui->showCreatePixmapLabel->setPixmap(QPixmap(pixmapPath).scaled(ui->showCreatePixmapLabel->size()));
    }
    //当处于打开笔记标签页时，就在当前页切换打开笔记步骤图片
    else if(index == 1)
    {
        currentOpenSystemFilePixmapIndex++;
        if(currentOpenSystemFilePixmapIndex >= OPEN_SYSTEN_FILE_COUNT)
        {
            currentOpenSystemFilePixmapIndex = 0;
        }
        pixmapPath = QString(":/new/openSystemFileGuide/image/OpenSystemFileSteps/openSystemFileSteps%1.png").arg(currentOpenSystemFilePixmapIndex);
        ui->showOpenSystemFileLabel->setPixmap(QPixmap(pixmapPath).scaled(ui->showOpenSystemFileLabel->size()));
    }
    //当处于查找单个路径标签页时，就在当前页切换查找单个路径步骤图片
    else if(index == 2)
    {
        currentFindOnePathPixmapIndex++;
        if(currentFindOnePathPixmapIndex >= FIND_ONE_PATH_FILE_COUNT)
        {
            currentFindOnePathPixmapIndex = 0;
        }
        pixmapPath = QString(":/new/findOnePathGuide/image/FindOnePathSteps/findOnePathSteps%1.png").arg(currentFindOnePathPixmapIndex);
        ui->showFindOnePathLabel->setPixmap(QPixmap(pixmapPath).scaled(ui->showFindOnePathLabel->size()));
    }
    //当处于查找全部路径标签页时，就在当前页切换查找全部路径步骤图片
    else if(index == 3)
    {
        currentFineAllPathPixmapIndex++;
        if(currentFineAllPathPixmapIndex >= FIND_ALL_PATH_FILE_COUNT)
        {
            currentFineAllPathPixmapIndex = 0;
        }
        pixmapPath = QString(":/new/findAllSystemFile/image/FindAllSystemFileSteps/findAllSystemFileSteps%1.png").arg(currentFineAllPathPixmapIndex);
        ui->showFindAllPathLabel->setPixmap(QPixmap(pixmapPath).scaled(ui->showFindAllPathLabel->size()));
    }
    this->currentPixampPath = pixmapPath;//将切换图片的路径记录下来，以便于缩放窗口时所用
}

void View_FunctionGuideWindow::on_showFunctionGuideTabWidget_currentChanged(int index)
{
    on_nextPagePushButton_clicked();
}

