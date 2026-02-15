#include "view_mainwindow.h"
#include "ui_view_mainwindow.h"

View_MainWindow::View_MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::View_MainWindow)
{
    ui->setupUi(this);

    this->setWindowIcon(QIcon(":/new/prefix1/image/文档管理系统.ico"));
    this->setWindowTitle("文档管理系统");

    //创建日志系统对象
    this->logSystemPtr = LogSystem::getLogSystemObject();

    //设置连接
    setConnect();

    //加载图片资源
    loadPixmap(":/new/prefix1/image/txt.png");
    loadPixmap(":/new/prefix1/image/word.png");
    loadPixmap(":/new/prefix1/image/excel.png");
    loadPixmap(":/new/prefix1/image/ppt.png");
    loadPixmap(":/new/prefix1/image/墨笺笔记图标.png");
    loadPixmap(":/new/prefix1/image/对check.png");

    //创建搜索中相关组件加入至窗口后，通过参数带回创建的widget和label控件
    //为什么这么做，就是因为查找某个功能逻辑和查找全部盘逻辑关于widget的代码都是相同的，只不过要获取到不同的对象
    //所以封装了这么个函数，通过参数将不同对象带回来
    this->showSearchOnePathWidget = nullptr;
    this->showSearchingTextLabel = nullptr;
    this->showSearchingMovieLabel = nullptr;
    this->initSearchingRelatedComponentsAddToWidget(this->showSearchOnePathWidget,this->showSearchingTextLabel,this->showSearchingMovieLabel,"showTextLable","showMovieLable");
    this->storeSearchingMovie = new QMovie(":/new/prefix1/image/动画搜索.gif",QByteArray(),this);//创建并初始化movie
    this->storeSearchingMovie->setScaledSize(QSize(32,32));
}
View_MainWindow::~View_MainWindow()
{
    //释放根节点
    deteleModel(ui->treeView->model());

    qDebug() << "View_MainWindow析构函数";

    delete ui;
}
//设置连接
void View_MainWindow::setConnect()
{
    //响应请求设置组节点图标信号
    connect(this,&View_MainWindow::requestSetGroupItemIcon,this,&View_MainWindow::setGroupItemIcon);

    //创建功能连接
    //响应创建信号
    connect(ui->createFileMenu,&QMenu::triggered,this,[=](QAction* clickAction){
        emit clickSuffixItem(clickAction->text());
    });

    //提示错误信息
    connect(this,&View_MainWindow::requestPrompError,this,&View_MainWindow::showQMessBoxErrorInfo);

    //响应设置treeModel信号
    connect(this,&View_MainWindow::setTreeMode,this,&View_MainWindow::setModel);

    //响应获取树模型信号
    connect(this,&View_MainWindow::getTreeModel,this,&View_MainWindow::getTreeMod);

    //双击功能连接
    //响应双击操作
    connect(ui->treeView,&QTreeView::doubleClicked,this,[=](const QModelIndex& indexItem){
        if(isFileItem(indexItem))
        {
            qDebug() << "openFilePath：" << indexItem.data(Qt::UserRole).toString();
            emit doubleClicked(indexItem.data(Qt::UserRole).toString());
        }
    });

    //响应是否选择文件信号
    connect(this,&View_MainWindow::isSelectFile,this,&View_MainWindow::isSelectNewFile);

    //响应提示信息信号
    connect(this,&View_MainWindow::showInfo,this,&View_MainWindow::showInfomation);

    //打开笔记功能连接
    //响应打开笔记信号
    connect(ui->openFileMenu,&QMenu::triggered,this,&View_MainWindow::openFileClicked);

    //查找文件按钮功能连接
    //响应查找文件按钮点击信号
    connect(ui->actionSerachFile,&QAction::triggered,this,[=](){
        selectDirectoryPath(emit requestGetUserName());
    });

    //响应提示搜索中信号
    connect(this,&View_MainWindow::requestPromptSearching,this,&View_MainWindow::promptSearching);

    //响应提示搜索完成信号
    connect(this,&View_MainWindow::requestPromptSearchFinish,this,&View_MainWindow::promptSearchFinish);

    //响应timeout信号
    connect(&this->controlWidgetHideTimer,&QTimer::timeout,this,[=](){
        this->showSearchOnePathWidget->hide();
        this->controlWidgetHideTimer.stop();
    });

    //查找全部盘下文件功能
    //响应查找全部系统文件被点击的信号
    connect(ui->actionFindAllSystemFile,&QAction::triggered,this,&View_MainWindow::requestSearchAllSystemFile);

    // //响应搜索中信号
    // connect(this,&View_MainWindow::requestPromatSearchingAll,this,&View_MainWindow::promatSearchingAll);

    //功能指引功能
    //请求功能指引窗口显示
    connect(ui->actionFunctionGuide,&QAction::triggered,this,&View_MainWindow::requestFunctionGuideWindowShow);
}
bool View_MainWindow::getnewFileName(QString suffix,QString& fileName)
{
    bool isOk;
    fileName = QInputDialog::getText(this,"获取名称","获取笔记名称，后缀：" + suffix,QLineEdit::Normal,"",&isOk);
    return isOk;
}
void View_MainWindow::showQMessBoxErrorInfo(QString errorText)
{
    QMessageBox::critical(this,"错误",errorText);
}
void View_MainWindow::setModel(QStandardItemModel* model)
{
    //设置treeModel
    ui->treeView->setModel(model);
}
QAbstractItemModel* View_MainWindow::getTreeMod()
{
    //返回treeModel
    return ui->treeView->model();
}
bool View_MainWindow::isFileItem(const QModelIndex& indexItem)
{
    //查看点击的节点是否是文件节点
    if(indexItem.parent().isValid())
    {
        return true;
    }
    return false;
}
QString View_MainWindow::isSelectNewFile(QString userName,QString filteredFile)
{
    QString newFilePath;
    //提示用户是否选择新路径
    if(askQuestion("是否选择新笔记并替换旧笔记\n是：重新指定路径\n否：将该文件从程序中移除，不再管理"))
    {
        //选择新路径时我希望过滤出旧路径软件的所有后缀的文件，比如过滤出doc和docx
        //通过文件路径找到软件名id，再根据软件名id找到软件名，再根据软件名找到该软件名所有的后缀
        newFilePath = selectNewFilePath(userName,filteredFile);
    }
    return newFilePath;
}
QString View_MainWindow::selectNewFilePath(QString userName,QString filteredFile)
{
    //获取文件路径
    return QFileDialog::getOpenFileName(this,"获取新路径","C:/Users/" + userName + "/Desktop",filteredFile);
}
void View_MainWindow::showInfomation(QString text)
{
    //提示信息
    QMessageBox::information(this,"信息",text);
}
void View_MainWindow::openFileClicked(QAction* clickAction)
{
    //通过触发信号获取用户名和文件过滤格式从而打开对话框获取文件路径，然后通过信号将文件路径传递给控制器
    emit requestCallOpenFileItemClicked(selectNewFilePath(emit requestGetUserName(),emit requestGenerateFileFilteringFormat(clickAction->text())));
}
void View_MainWindow::selectDirectoryPath(QString userName)
{
    //获取目录路径
    QString directoryPath = QFileDialog::getExistingDirectory(this,"获取路径","C:/Users/" + userName + "/Desktop");

    //触发信号将路径传递给模型层
    emit requestSearchFile(directoryPath);
}
void View_MainWindow::promptSearching(QString searchText)
{
    //停止计时器防止在搜索过程中被隐藏
    this->controlWidgetHideTimer.stop();

    //提示搜索中
    this->showSearchingTextLabel->setText(searchText);

    //显示搜索中动图
    this->showSearchingMovieLabel->setMovie(this->storeSearchingMovie);
    this->storeSearchingMovie->start();//开启动图播放

    //加入至状态栏中
    ui->statusbar->addWidget(this->showSearchOnePathWidget);

    //查找窗口显示
    this->showSearchOnePathWidget->show();
}
void View_MainWindow::promptSearchFinish(QString text)
{
    //提示搜索完成
    this->showSearchingTextLabel->setText(text);

    //停止搜索动图的播放
    //当查找全盘的过程中再次查找某个路径可能会因为信号发送finish的时机导致解引用到空指针，因此才有判断语句
    if(this->showSearchingMovieLabel->movie() != nullptr){
        this->showSearchingMovieLabel->movie()->stop();
    }
    this->showSearchingMovieLabel->setPixmap(loadPixmap(":/new/prefix1/image/对check.png").scaled(QSize(32,32)));//通过函数将搜索完成图片设置在label控件上

    //启用定时器，10秒后移除显示搜索中的组件
    this->controlWidgetHideTimer.start(10000);
}
void View_MainWindow::deteleModel(QAbstractItemModel* model)
{
    //先置空，再释放
    ui->treeView->setModel(nullptr);
    if(model != nullptr)
    {
        delete model;
    }
}
void View_MainWindow::initSearchingRelatedComponentsAddToWidget(QWidget*& widget,QLabel*& showTextLabel,QLabel*& showMovieLabel,QString textLableObjectName,QString movieLableObjectName)
{
    //将查找某个路径相关组件加入至widget用于显示
    //创建带有水平布局的widget
    //我以引用的方式传递一级指针，就可以为原来的指针赋值

    //创建存储搜索中动图的组件和显示搜索中字样的组件，将这两个组件加入至widget中，搜索时刻显示在状态栏中
    widget = new QWidget(this);//创建存储搜索中组件的窗口
    widget->setContentsMargins(0,0,0,0);//设置widget与父组件的边距
    QHBoxLayout* horizontalLayout = new QHBoxLayout();//创建水平布局
    horizontalLayout->setContentsMargins(0,0,0,0);//设置widget与布局之间的组件
    widget->setLayout(horizontalLayout);
    widget->hide();//防止作为顶层覆盖组件

    //将相关控件加入至widget中
    showTextLabel = new QLabel(this);
    showTextLabel->setObjectName(textLableObjectName);
    showMovieLabel = new QLabel(this);
    showMovieLabel->setObjectName(movieLableObjectName);

    //将动图label控件放左边，文字控件放右边
    horizontalLayout->addWidget(showMovieLabel);
    horizontalLayout->addWidget(showTextLabel);
}
QPixmap& View_MainWindow::loadPixmap(const QString& path)
{
    //如果容器中找不到对应的路径，证明还未预加载图片
    if(!pictureMap.contains(path))
    {
        //否则加载图片
        //创建QPixmap对象来存储加载后的图片
        QPixmap pixmap(path);
        //注意：pixmap.scaled是一个函数，它是返回一个新的对象，而不是在原对象修改
        //pixmap = pixmap.scaled(32,32,Qt::KeepAspectRatio);//并设置宽高像素都为32，并且保留原始尺寸，不被压缩
        pictureMap[path] = pixmap;//将QPixmap对象转换为QIcon放到容器中，以便下次直接拿来使用
    }
    return pictureMap[path];//如果找到对应的路径，则直接返回路径对应的图标即可
}
void View_MainWindow::setGroupItemIcon(QStandardItem* gruopItem,QString iconPath)
{
    //通过信号和槽的方式在视图层进行显示相关的操作
    gruopItem->setIcon(loadPixmap(iconPath));
}
bool View_MainWindow::askQuestion(QString text)
{
    if(QMessageBox::Yes == QMessageBox::question(this,"提问",text))
    {
        return true;
    }
    return false;
}
// void View_MainWindow::promatSearchingAll(QString directoryPath)
// {

//     //声明搜索中组件的指针
//     QWidget* widget = nullptr;
//     QLabel* showTextLable = nullptr;
//     QLabel* showMovieLable = nullptr;

//     //创建并初始化搜索中组件的指针
//     initSearchingRelatedComponentsAddToWidget(widget,showTextLable,showMovieLable,"showTextLable","showMovieLable");

//     //创建动图并设置到label控件
//     showMovieLable->setMovie(new QMovie(":/new/prefix1/image/动画搜索.gif"));
//     showMovieLable->movie()->setScaledSize(QSize(32,32));
//     showMovieLable->movie()->start();

//     //设置文字到label控件
//     showTextLable->setText(directoryPath + "搜索中......");

//     //关联盘符和widget的关系
//     direvPathToQWidget.insert(directoryPath,widget);

//     //加入至状态栏中
//     ui->statusbar->addWidget(widget);

//     //窗口显示
//     widget->show();

//     //搜索成功连接
// }
// void View_MainWindow::promatSearchAllFinish(QString directoryPath)
// {
//     //在map容器中通过盘符找到widget
//     QWidget* widget = direvPathToQWidget.value(directoryPath);

//     //通过项目名找到对应的label控件，修改对应信息
//     QLabel* showTextLabel = widget->findChild<QLabel*>("showTextLabel");
//     showTextLabel->setText(directoryPath+"搜索完成");

//     QLabel* showMovieLabel = widget->findChild<QLabel*>("showMovieLabel");
//     QMovie* movie = showMovieLabel->movie();
//     movie->stop();
//     showMovieLabel->setMovie(nullptr);
//     delete movie;

//     showMovieLabel->setPixmap(loadPixmap(":/new/prefix1/image/对check.png").scaled(QSize(32,32)));
// }
