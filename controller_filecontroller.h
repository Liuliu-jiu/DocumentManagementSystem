#ifndef CONTROLLER_FILECONTROLLER_H
#define CONTROLLER_FILECONTROLLER_H

#include <QObject>
#include"view_mainwindow.h"
#include"model_filemodel.h"
#include"view_filesearchresultswindow.h"
#include"logsystem.h"
#include"view_functionguidewindow.h"
class Controller_FileController : public QObject
{
    Q_OBJECT
public:

    explicit Controller_FileController(QObject *parent = nullptr);

    ~Controller_FileController();

    Model_FileModel* m_fileModel;//使用qt对象树帮Model_FileModel回收内存
    std::shared_ptr<View_MainWindow> v_mainWindow;//主窗口
    std::shared_ptr<View_FileSearchResultsWindow> v_fileSearchResultsWindow;//查找窗口
    std::shared_ptr<View_FunctionGuideWindow> v_functionGuideWindow;//功能指引窗口

    //设置连接
    void setConnect();

    //创建文件
    void createFile(QString text);

    //双击打开功能
    void doubleClickOpenFile(QString filePath);

    //处理双击失败函数
    void handleDoubleClickFailure(QString oldFilePath,QString userName);

    //监听打开笔记信号
    void openFileItemClicked(QString filePath);
private:
    //日志系统对象
    std::shared_ptr<LogSystem> logSystemPtr;
signals:

};

#endif // CONTROLLER_FILECONTROLLER_H
