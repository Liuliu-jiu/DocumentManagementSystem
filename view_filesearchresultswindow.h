#ifndef VIEW_FILESEARCHRESULTSWINDOW_H
#define VIEW_FILESEARCHRESULTSWINDOW_H

#include <QMainWindow>
#include<QStandardItemModel>
#include<QTimer>
#include"logsystem.h"
namespace Ui {
class View_FileSearchResultsWindow;
}

class View_FileSearchResultsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit View_FileSearchResultsWindow(QWidget *parent = nullptr);
    ~View_FileSearchResultsWindow();

    QTimer searchingTimer;//处理查找中的timer
    QTimer searchFinishTimer;//处理查找完成的timer
    int progressValue;

    //设置连接
    void setConnect();

    //设置树模型
    void setModel(QStandardItemModel* model);

    //显示搜索到的文件路径
    void showSearchFilePath(QString filePath);

    //清空文本编辑的数据
    void clearTextEdit();

    //提示搜索中
    void promptSearching();

    //提示搜索完成
    void promptSearchFinish();

    //响应查找中的timeout信号
    void respondToSearchingTimeoutSignal();

    //响应查找完成的timeout的信号
    void respondToSearchFinishTimeoutSignal();

    //销毁模型
    void deteleModel(QAbstractItemModel* model);

    //提示取消成功
    void promatCancelSuccess();
private:
    //日志系统对象
    std::shared_ptr<LogSystem> logSystemPtr;
signals:
    void setTreeMode(QStandardItemModel* treeModel);//设置树模型
    void requestFileSearchWinodwShow();//请求显示查找文件窗口
    void requesetShowSeaechFilePath(QString filePath);//请求显示搜索到的文件路径
    void rquesetClearTextEdit();//请求清除文本编辑器的数据
    void requestPromptSearching();//请求提示搜索中
    void requestPromptSearchFinish();//请求提示搜索完成
    void requestCancelSearchProcess();//请求取消搜索进程
    void requestPromatCancelSuccess();//请求提示取消成功
private slots:
    void on_pushButton_clicked();

private:
    Ui::View_FileSearchResultsWindow *ui;
};

#endif // VIEW_FILESEARCHRESULTSWINDOW_H
