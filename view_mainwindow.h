#ifndef VIEW_MAINWINDOW_H
#define VIEW_MAINWINDOW_H

#include <QMainWindow>
#include<QInputDialog>
#include<QLineEdit>
#include<QMessageBox>
#include<QStandardItemModel>
#include<QFileDialog>
#include<QLabel>
#include<QProgressBar>
#include<QTimer>
#include<QMovie>
#include<QIcon>
#include"logsystem.h"
namespace Ui {
class View_MainWindow;
}

class View_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit View_MainWindow(QWidget *parent = nullptr);
    ~View_MainWindow();

    //显示搜索某个路径的widget
    QWidget* showSearchOnePathWidget;
    QLabel* showSearchingTextLabel;//显示搜索中文本的label控件
    QLabel* showSearchingMovieLabel;//显示搜索动图的label控件
    QMovie* storeSearchingMovie;//存放搜索动图的movie
    QTimer controlWidgetHideTimer;//用于控制显示搜索中widget隐藏的timer

    //专门存储图片路径的容器
    QMap<QString,QPixmap> pictureMap;

    //关联盘符与QWidget之间的关系
    QMap<QString,QWidget*> direvPathToQWidget;

    //设置连接
    void setConnect();

    //获取笔记名称
    bool getnewFileName(QString suffix,QString& fileName);

    //展示错误信息
    void showQMessBoxErrorInfo(QString errorText);

    //设置Model
    void setModel(QStandardItemModel* model);

    //获取树模型
    QAbstractItemModel* getTreeMod();

    //检查是否是文件节点
    bool isFileItem(const QModelIndex& indexItem);

    //是否选择新路径
    QString isSelectNewFile(QString userName,QString filteredFile);

    //选择新路径
    QString selectNewFilePath(QString userNam,QString filteredFilee);

    //提示信息
    void showInfomation(QString text);

    //响应打开笔记信号
    void openFileClicked(QAction* clickAction);

    //选择目录路径
    void selectDirectoryPath(QString userName);

    //提示搜索中函数
    void promptSearching(QString searchText);//搜索中视图相关操作

    //提示搜索完成函数
    void promptSearchFinish(QString searchText);

    //销毁模型
    void deteleModel(QAbstractItemModel* model);

    //初始化关于提示搜索中的相关组件并添加至widget中
    void initSearchingRelatedComponentsAddToWidget(QWidget*& widget,QLabel*& showTextLabel,QLabel*& showMovieLabel,QString textLableObjectName,QString movieLableObjectName);

    //使用缓存机制加载图片资源
    QPixmap& loadPixmap(const QString& path);

    //设置组节点图标
    void setGroupItemIcon(QStandardItem* gruopItem,QString iconPath);

    //是否选择目录路径
    bool askQuestion(QString text);

    // //提示搜索全部系统文件中
    // void promatSearchingAll(QString directoryPath);

    // //提示查找全部系统文件进程完成
    // void promatSearchAllFinish(QString directoryPath);

private:
    //日志系统对象
    std::shared_ptr<LogSystem> logSystemPtr;
signals:
    void clickSuffixItem(QString text);//点击后缀项信号
    void requestPrompError(QString errorText);//展示错误信号
    void setTreeMode(QStandardItemModel* treeModel);//设置treeModel信号
    QAbstractItemModel* getTreeModel();//获取treeModel信号
    void doubleClicked(QString filePath);//双击信号
    QString isSelectFile(QString userName,QString filteredFile);//是否选择文件信号
    void showInfo(QString text);//展示信息信号
    QString requestGenerateFileFilteringFormat(QString filePath);//请求生成文件过滤格式
    QString requestGetUserName();//请求获取用户名
    void requestCallOpenFileItemClicked(QString filePath);//请求调用打开笔记函数
    void requestSearchFile(QString directoryPath);//请求搜索文件
    void requestPromptSearching(QString searchText);//请求提示搜索中
    void requestPromptSearchFinish(QString text);//请求提示搜索完成
    void requestSearchAllSystemFile();//请求搜索全部系统文件
    void requestSetGroupItemIcon(QStandardItem* gruopItem,QString iconPath);//请求设置组节点图标
    void requestFunctionGuideWindowShow();//请求功能指引窗口显示
    // void requestPromatSearchingAll(QString directoryPath);//请求提示搜索全部系统文件中
    // void requestPromatSearchAllFinish(QString directoryPath);//请求提示搜索完全部系统文件
private:
    Ui::View_MainWindow *ui;
};

#endif // VIEW_MAINWINDOW_H
