#include "controller_filecontroller.h"

Controller_FileController::Controller_FileController(QObject *parent)
    : QObject{parent}
{
    //创建各类指针
    m_fileModel = new Model_FileModel(this);
    v_mainWindow = std::make_shared<View_MainWindow>();
    v_fileSearchResultsWindow = std::make_shared<View_FileSearchResultsWindow>();
    v_functionGuideWindow = std::make_shared<View_FunctionGuideWindow>();
    logSystemPtr = LogSystem::getLogSystemObject();

    //设置连接
    setConnect();

    //先建立信号后初始化，否则会因为信号触发的时候还未建立信号和槽导致无法将信号传递到视图层
    m_fileModel->initTreeMode();

    // //询问用户是否指定目录路径加载文件
    // if(v_mainWindow->askQuestion(
    //         "\t\t欢迎使用墨笺笔记！\t\t\t\n"
    //         "第一次启动可选择将对应的目录路径下的文件加载至程序中管理\n"
    //         "\t\t是否选择目录路径？"))
    // {
    //     //加载文件
    //     v_mainWindow->selectDirectoryPath(m_fileModel->getSystemUserName());
    // }
    //主窗口显示
    v_mainWindow->show();

    //m_fileModel->testFindTime("D:/",10);
}
Controller_FileController::~Controller_FileController()
{
    qDebug() << "Controller_FileController析构函数";
}
void Controller_FileController::setConnect()
{
    //响应请求设置组节点图标信号
    connect(m_fileModel,&Model_FileModel::requestSetGroupItemIcon,v_mainWindow.get(),&View_MainWindow::requestSetGroupItemIcon);

    //创建功能连接
    //响应创建信号
    connect(v_mainWindow.get(),&View_MainWindow::clickSuffixItem,this,&Controller_FileController::createFile);

    //响应设置treeModel信号
    connect(m_fileModel,&Model_FileModel::setTreeMode,v_mainWindow.get(),&View_MainWindow::setTreeMode);

    //响应获取树模型信号
    connect(m_fileModel,&Model_FileModel::getTreeModel,v_mainWindow.get(),&View_MainWindow::getTreeModel);

    //双击功能连接
    //响应双击信号
    connect(v_mainWindow.get(),&View_MainWindow::doubleClicked,this,&Controller_FileController::doubleClickOpenFile);

    //响应请求生成文件过滤格式信号
    connect(v_mainWindow.get(),&View_MainWindow::requestGenerateFileFilteringFormat,m_fileModel,&Model_FileModel::requestGenerateFileFilteringFormat);

    //响应请求获取用户名信号
    connect(v_mainWindow.get(),&View_MainWindow::requestGetUserName,m_fileModel,&Model_FileModel::requestGetUserName);

    //打开笔记功能连接
    //响应打开笔记信号
    connect(v_mainWindow.get(),&View_MainWindow::requestCallOpenFileItemClicked,this,&Controller_FileController::openFileItemClicked);

    //查找文件功能连接
    //响应设置查找窗口树模型信号
    connect(m_fileModel,&Model_FileModel::setSearchWinodwTreeMode,v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::setTreeMode);

    //响应查找文件信号
    connect(v_mainWindow.get(),&View_MainWindow::requestSearchFile,m_fileModel,&Model_FileModel::searchDirectoryFile);

    //响应请求显示查找文件窗口信号
    connect(m_fileModel,&Model_FileModel::requestFileSearchWinodwShow,v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::requestFileSearchWinodwShow);

    //响应提示搜索中信号
    connect(m_fileModel,&Model_FileModel::requestPromptSearching,v_mainWindow.get(),&View_MainWindow::requestPromptSearching);

    //响应请求显示搜索到的路径信号
    connect(m_fileModel,&Model_FileModel::requesetShowSeaechFilePath,v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::requesetShowSeaechFilePath);

    //响应请求清空文本编辑器信号
    connect(m_fileModel,&Model_FileModel::rquesetClearTextEdit,v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::rquesetClearTextEdit);

    //响应请求提示搜索中信号
    connect(m_fileModel,&Model_FileModel::requestPromptSearching,v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::requestPromptSearching);

    //响应请求提示搜索完成信号
    connect(m_fileModel,&Model_FileModel::requestPromptSearchFinish,v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::requestPromptSearchFinish);

    //响应请求取消搜索进程信号
    connect(v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::requestCancelSearchProcess,m_fileModel,&Model_FileModel::requestCancelSearchProcess);

    //响应请求提示取消搜索进程成功信号
    connect(m_fileModel,&Model_FileModel::requestPromatCancelSuccess,v_fileSearchResultsWindow.get(),&View_FileSearchResultsWindow::requestPromatCancelSuccess);

    //响应提示搜索完成信号
    connect(m_fileModel,&Model_FileModel::requestPromptSearchFinish,v_mainWindow.get(),&View_MainWindow::requestPromptSearchFinish);

    //响应请求提示错误信号
    connect(m_fileModel,&Model_FileModel::requestPromptError,v_mainWindow.get(),&View_MainWindow::requestPrompError);

    //查找全部盘下文件功能
    //响应请求查找全部系统文件信号
    connect(v_mainWindow.get(),&View_MainWindow::requestSearchAllSystemFile,m_fileModel,&Model_FileModel::requestSearchAllSystemFile);

    //响应搜索中信号
    //connect(m_fileModel,&Model_FileModel::requestPromatSearchingAll,v_mainWindow.get(),&View_MainWindow::requestPromatSearchingAll);

    //功能指引
    //响应请求功能指引窗口显示信号
    connect(v_mainWindow.get(),&View_MainWindow::requestFunctionGuideWindowShow,v_functionGuideWindow.get(),&View_FunctionGuideWindow::show);

}
void Controller_FileController::createFile(QString text)
{
    this->logSystemPtr->writeLog("用户点击了创建按钮");
    QString suffix = text.toLower();
    QString fileName;

    //获取笔记名称
    bool isOk = v_mainWindow->getnewFileName(suffix,fileName);
    if(isOk)
    {
        //创建文件并添加至程序中管理
        if(m_fileModel->addFile(fileName,suffix))
        {
            qDebug() << "Create success！";
        }
        else
        {
            this->logSystemPtr->writeLog("创建失败！错误原因：1.文件名为空或语法不正确(不能包含/ \\ | : * ? <>)2.文件已存在3.磁盘空间不足4.逻辑错误，请联系作者");
            qDebug() << "Create fail！";
            emit v_mainWindow->requestPrompError("创建失败！错误原因：\n1.文件名为空或语法不正确(不能包含/ \\ | : * ? <>)\n2.文件已存在\n3.磁盘空间不足\n4.逻辑错误，请联系作者");
        }
    }
}
void Controller_FileController::doubleClickOpenFile(QString filePath)
{
    if(m_fileModel->openFile(filePath))
    {
        qDebug() << filePath << " 打开成功！";
    }
    else
    {
        handleDoubleClickFailure(filePath,m_fileModel->getSystemUserName());
    }
}
void Controller_FileController::handleDoubleClickFailure(QString oldFilePath,QString userName)
{
    this->logSystemPtr->writeLog(oldFilePath + " 打开失败！");
    qDebug() << oldFilePath << " 打开失败！";
    emit v_mainWindow->requestPrompError(oldFilePath + " 打开失败！\n可能原因：\n1. 文件不存在 \n2. 没有关联的应用程序\n3. 系统权限限制");
    //提示用户是否选择新路径并替换旧路径
    QString newFilePath = emit v_mainWindow->isSelectFile(userName,m_fileModel->generateFileFilteringFormatByFilePath(oldFilePath));
    if(!newFilePath.isEmpty())
    {
        //当新老路径一致，说明用户可能在原来的目录下新建了一个跟之前一模一样的文件或将之前的文件移动回原来的目录，此时不需要任何变更
        if(oldFilePath != newFilePath)
        {
            //验证新路径是否已加入至程序中管理
            if(m_fileModel->isExist(newFilePath))
            {
                emit v_mainWindow->requestPrompError("该文件已加入至程序中管理，请重新选择！");
                return;
            }

            //将旧路径相关信息删除，并且替换为新路径
            //更新数据库路径
            if(m_fileModel->replaceFilePath(oldFilePath,newFilePath))
            {
                //更新文件节点
                m_fileModel->updateFileItemFilePath(qobject_cast<const QStandardItemModel*>(emit v_mainWindow->getTreeModel()),oldFilePath,newFilePath);

                //提示用户操作成功
                emit v_mainWindow->showInfo("替换成功！");

                //打开文件
                doubleClickOpenFile(newFilePath);
            }
        }
    }
    else
    {
        //移除文件路径相关信息
        if(!m_fileModel->removeFileItem(qobject_cast<QStandardItemModel*>(emit v_mainWindow->getTreeModel()),oldFilePath))
        {
            qDebug() << "(Controller_FileController::handleDoubleClickFailure) "+ oldFilePath + " 删除失败！";
        }
    }
}
void Controller_FileController::openFileItemClicked(QString filePath)
{
    //判断路径是否为空
    this->logSystemPtr->writeLog("用户点击了打开笔记按钮");
    if(!filePath.isEmpty())
    {
        //检查文件是否已添加至程序中管理，是则直接打开
        if(!m_fileModel->isExist(filePath))
        {
            //不存在则将文件路径添加至数据库中
            if(m_fileModel->insertFilePathToDataBase(filePath,QFileInfo(filePath).suffix()))
            {
                //添加成功后把文件路径添加至对应的组节点下
                m_fileModel->addFileItem(qobject_cast<const QStandardItemModel*>(emit v_mainWindow->getTreeModel()),filePath);
            }
            else
            {
                //添加失败则报错
                emit v_mainWindow->requestPrompError(filePath + " 添加失败！");
                return;
            }
        }
        //不论存不存在，最后都要打开文件
        doubleClickOpenFile(filePath);
    }
}

