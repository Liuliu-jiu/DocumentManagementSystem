#ifndef MODEL_FILEMODEL_H
#define MODEL_FILEMODEL_H

#include <QObject>
#include<QDir>
#include<QSqlDatabase>
#include<QSqlQuery>
#include<QSqlError>
#include<QStandardItemModel>
#include<QFileInfo>
#include<QUrl>
#include<QDesktopServices>
#include<QProcessEnvironment>
#include<QStandardItem>
#include<QFutureWatcher>
#include<QDirIterator>
#include<QtConcurrent/QtConcurrentRun>
#include<QFuture>
#include<QElapsedTimer>
#include"logsystem.h"

//__FILE__表示当前源文件路径的宏定义，可能是相对路径也有可能是绝对路径，取决于编译器，我需要获取到绝对路径，因此通过QFileInfo来进行路径转换
#define INK_NOTE_DATABASE_PATH QFileInfo(__FILE__).absoluteDir().absolutePath() + "/ink_note_database.db"
class Model_FileModel : public QObject
{
    Q_OBJECT
public:
    explicit Model_FileModel(QObject *parent = nullptr);

    ~Model_FileModel();

    QSqlDatabase db;

    //定义表名的枚举，决定将查找到的文件插入至哪张表当中
    //class代表使用枚举的时候必须带有作用域，保证了类型安全
    // enum class TableName
    // {
    //     FileInfoTable,//fileinfotable表名
    //     FileSearchInfoTable//filesearchinfotable表名
    // };
    // Q_ENUM(TableName);//将TableName枚举注册到元对象系统中才能使用

    // //该枚举决定触发哪个窗口的setTreeModel信号
    // enum class TreeModelSignal
    // {
    //     RequestSetMainWindowTreeModel,
    //     RequestSetSearchWindowTreeModel
    // };
    // Q_ENUM(TreeModelSignal);

    //在头文件声明指针，源文件就可以利用构造函数明确绑定数据库服务器，并且可以让源文件的所有位置知道这个指针
    std::shared_ptr<QSqlQuery> queryPtr;

    //存储后缀和软件名id之间的关系的map容器
    QMap<QString,int> suffixToIdMap;

    QString userName;

    //查找某个路径文件的监视者
    std::shared_ptr<QFutureWatcher<void>> searchingFileFuctureWatcher;

    //上一次查找的路径
    QString prevSearchiFilePath;

    //计算执行时间的计时器
    QElapsedTimer calculateExecutionTimeElapsedTimer;

    //初始化连接
    std::shared_ptr<QSqlQuery> initMysqlConnect(QString connectName, QSqlDatabase& qsqlDB);

    //查找全部文件的监视者
    std::shared_ptr<QFutureWatcher<void>> searchAllFileFutureWatcherPtr;

    bool searchAllFileIsCancel;

    //设置连接
    void setConnect();

    //添加文件
    bool addFile(QString fileName,QString suffix);

    //检查文本有效性
    bool isValidText(QString text);

    //判断路径是否存在
    bool isExist(QString filePath);

    //将文件路径插入至数据库中
    bool insertFilePathToDataBase(QString filePath,QString suffix);

    //初始化树模型
    void initTreeMode();
    void initSearchWindowTreeMode(QStringList tableNameList);

    //添加文件节点
    void addFileItem(const QStandardItemModel* model,QString filePath);

    //查找组节点
    QStandardItem* findGroupItem(const QStandardItemModel* model,QString text);

    //打开文件
    bool openFile(QString filePath);

    //获取用户名
    QString getSystemUserName();

    //通过文件路径或软件名生成过滤格式
    QString generateFileFilteringFormatByFilePath(QString filePath);
    QString generateFileFilteringFormatBySoftName(QString softName);

    //将新路径封装并替换旧路径
    bool replaceFilePath(QString oldFilePath,QString newFilePath);

    //更新文件节点
    void updateFileItemFilePath(const QStandardItemModel* model,QString oldFilePath,QString newFilePath);

    //查询文件节点
    QStandardItem* findFileItem(const QStandardItem* groupItem,QString filePath);
    QStandardItem* findFileItem(const QStandardItemModel* model,QString filePath);

    //删除文件节点
    bool removeFileItem(QStandardItemModel* model,QString removeFilePath);

    //查找目录路径文件
    void searchDirectoryFile(QString directoryPath);

    //查找系统文件
    void findSystemFile(QString directoryPath,QStringList filteredFile,int i,std::shared_ptr<QFutureWatcher<void>> futureWatcherPtr,QString tableName);

    //查找完成后的处理
    void finishBackGroudTask(QStringList tableNameList);

    //取消搜素进程
    void cancelSearchProcess();

    //查找全部盘下系统文件
    void searchAllSystemFile();

    //获取所有后缀过滤格式
    QStringList getAllSuffixFilterFormats();

    //根据盘符路径创建表
    void createTableByDirectoryPath();

    //获取所有关于盘符的表名
    QStringList getTableNameByAllDiretoryPath();

    //取消所有后台进程
    void cancelAllBackgroundProcesses();

    //加载文件
    void loadFile(QString diretoryPath);

    //测试查找时间
    //记录测试查找时间容器
    QList<double> spendTimeList;
    void testFindTime(QString testPath,int testCount);

private:
    //日志系统对象
    std::shared_ptr<LogSystem> logSystemPtr;

signals:
    void setTreeMode(QStandardItemModel* treeModel);
    QAbstractItemModel* getTreeModel();
    QString requestGenerateFileFilteringFormat(QString filePath);//请求生成文件过滤格式
    QString requestGetUserName();//请求获取用户名
    void requestPromptError(QString errorText);//请求提示错误信息
    void requestPromptSearching(QString searchingText);//请求提示搜索中
    void setSearchWinodwTreeMode(QStandardItemModel* treeModel);
    void requestFileSearchWinodwShow();//请求查找文件窗口显示
    void requesetShowSeaechFilePath(QString filePath);//请求显示搜索到的文件路径
    void rquesetClearTextEdit();
    void requestPromptSearchFinish(QString text);//请求提示搜索完成
    void requestCancelSearchProcess();//请求取消搜索进程
    void requestPromatCancelSuccess();//请求提示取消成功
    void requestSearchAllSystemFile();//请求搜索全部系统文件
    void requestSetGroupItemIcon(QStandardItem* gruopItem,QString iconPath);//请求设置组节点图标
    // void requestPromatSearchingAll(QString directoryPath);//请求提示搜索全部系统文件中
    // void requestPromatSearchAllFinish(QString directoryPath);//请求提示搜索完全部系统文件
};

#endif // MODEL_FILEMODEL_H
