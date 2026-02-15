#include "view_filesearchresultswindow.h"
#include "ui_view_filesearchresultswindow.h"

View_FileSearchResultsWindow::View_FileSearchResultsWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::View_FileSearchResultsWindow)
{
    ui->setupUi(this);

    //创建日志系统对象
    this->logSystemPtr = LogSystem::getLogSystemObject();

    this->setWindowIcon(QIcon(":/new/prefix1/image/文档管理系统.ico"));
    this->setWindowTitle("搜索窗口");

    //设置连接
    setConnect();
}

View_FileSearchResultsWindow::~View_FileSearchResultsWindow()
{
    delete ui;
}
//设置连接
void View_FileSearchResultsWindow::setConnect()
{
    //响应设置treeModel信号
    connect(this,&View_FileSearchResultsWindow::setTreeMode,this,&View_FileSearchResultsWindow::setModel);

    //响应请求显示查找文件窗口信号
    connect(this,&View_FileSearchResultsWindow::requestFileSearchWinodwShow,this,&View_FileSearchResultsWindow::show);

    //响应请求显示搜索到的路径信号
    connect(this,&View_FileSearchResultsWindow::requesetShowSeaechFilePath,this,&View_FileSearchResultsWindow::showSearchFilePath);

    //响应请求清空文本编辑器信号
    connect(this,&View_FileSearchResultsWindow::rquesetClearTextEdit,this,&View_FileSearchResultsWindow::clearTextEdit);

    //响应请求提示搜索中信号
    connect(this,&View_FileSearchResultsWindow::requestPromptSearching,this,&View_FileSearchResultsWindow::promptSearching);

    //响应请求提示搜索完成信号
    connect(this,&View_FileSearchResultsWindow::requestPromptSearchFinish,this,&View_FileSearchResultsWindow::promptSearchFinish);

    //响应查找完成的timeout的信号
    progressValue = 0;
    connect(&searchingTimer,&QTimer::timeout,this,&View_FileSearchResultsWindow::respondToSearchingTimeoutSignal);

    //响应searchingTimer的timeout信号，处理查找中的情况
    progressValue = 0;
    connect(&searchFinishTimer,&QTimer::timeout,this,&View_FileSearchResultsWindow::respondToSearchFinishTimeoutSignal);

    //响应提示取消搜索进程成功信号
    connect(this,&View_FileSearchResultsWindow::requestPromatCancelSuccess,this,&View_FileSearchResultsWindow::promatCancelSuccess);
}
void View_FileSearchResultsWindow::setModel(QStandardItemModel* model)
{
    qDebug() << "设置树模型并显示窗口";

    //先将上一次的模型销毁在设置这一次的，防止内存泄漏
    deteleModel(ui->treeView->model());
    ui->treeView->setModel(model);
}
void View_FileSearchResultsWindow::showSearchFilePath(QString filePath)
{
    ui->textEdit->append(filePath);//利用append在末尾添加路径且换到下一行
}
void View_FileSearchResultsWindow::clearTextEdit()
{
    qDebug() << "清空文本编辑器数据";
    ui->textEdit->clear();
}
void View_FileSearchResultsWindow::promptSearching()
{
    //每次查找都要清空文本编辑器的文件路径，防止新旧数据混合
    clearTextEdit();

    ui->treeView->hide();
    ui->textEdit->show();//textEdit显示查找到的路径
    ui->progressBar->show();
    ui->progressBar->setValue(0);//设置进度条初始值为0
    this->progressValue = 0;//进度条变量值归0
    ui->pushButtonWidget->show();//将带有pushbutton的widget显示

    searchingTimer.start(2000);//开始推进进度条

    this->show();
}
void View_FileSearchResultsWindow::promptSearchFinish()
{
    ui->progressBar->setValue(100);//设置进度条为100%
    this->progressValue = 0;//进度条变量值归0
    this->searchingTimer.stop();//暂停计时器
    ui->textEdit->append("正在分类文件......");
    this->searchFinishTimer.start(2000);//等2秒后才展示最终效果，避免突然切换而感到突兀
}
void View_FileSearchResultsWindow::respondToSearchingTimeoutSignal()
{
    //每2秒增加4%
    this->progressValue += 4;
    //当加起来的值大于等于99就说明还没查找完，就一直卡在99%，直到搜索完成
    if(this->progressValue >= 99)
    {
        ui->progressBar->setValue(99);
        return;
    }
    //如果小于99则正常推进
    ui->progressBar->setValue(this->progressValue);
}
void View_FileSearchResultsWindow::respondToSearchFinishTimeoutSignal()
{
    ui->textEdit->hide();
    ui->progressBar->hide();
    ui->pushButtonWidget->hide();//将带有pushbutton的widget隐藏
    ui->treeView->show();//treeView显示最终分类的成果
    this->show();
    this->searchFinishTimer.stop();
}
void View_FileSearchResultsWindow::deteleModel(QAbstractItemModel* model)
{
    //先置空后释放
    ui->treeView->setModel(nullptr);
    if(model != nullptr)
    {
        delete model;
    }
}
void View_FileSearchResultsWindow::on_pushButton_clicked()
{
    //定时器停止，不再推进进度条
    searchingTimer.stop();

    //提示用户取消中
    ui->textEdit->append("取消中......");

    //触发信号至模型层取消进程
    emit requestCancelSearchProcess();
}
void View_FileSearchResultsWindow::promatCancelSuccess()
{
    //提示用户取消成功
    ui->textEdit->append("取消成功！");
}
