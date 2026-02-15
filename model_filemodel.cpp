#include "model_filemodel.h"

Model_FileModel::Model_FileModel(QObject *parent)
    : QObject{parent}
{
    //初始化futureWathcer对象
    this->searchingFileFuctureWatcher = std::make_shared<QFutureWatcher<void>>();

    //创建日志系统对象
    this->logSystemPtr = LogSystem::getLogSystemObject();

    //确定数据库驱动类型及服务器地址相关信息
    this->queryPtr = initMysqlConnect("mainConnection",this->db);

    //初始化查找全盘监视者
    this->searchAllFileFutureWatcherPtr = std::make_shared<QFutureWatcher<void>>();

    this->searchAllFileIsCancel = false;

    //通过map容器来去永久映射后缀与软件名id之间的关系
    suffixToIdMap = {
        {"doc",1},{"docx",1},
        {"xls",2},{"xlsx",2},
        {"ppt",3},{"pptx",3},
        {"txt",4}
    };

    //为每个盘符创建对应的表，方便存储对应盘的文件
    createTableByDirectoryPath();

    //建立连接
    setConnect();
}
Model_FileModel::~Model_FileModel()
{
    qDebug() << "Model_FileModel析构函数";

    //关闭窗口时后台搜索进程还在运行，就将后台进程取消
    cancelAllBackgroundProcesses();

    db.close();
}
//初始化连接
std::shared_ptr<QSqlQuery> Model_FileModel::initMysqlConnect(QString connectName, QSqlDatabase& qsqlDB)
{
    //确定数据库驱动类型
    qsqlDB = QSqlDatabase::addDatabase("QSQLITE",connectName);
    qsqlDB.setDatabaseName("ink_note_database.db");//数据库文件路径

    //连接数据库
    if(!qsqlDB.open())
    {
        this->logSystemPtr->writeLog("数据库连接失败！错误信息：" + qsqlDB.lastError().text());
        qDebug() << "数据库连接失败！错误信息：" << qsqlDB.lastError().text();
        return nullptr;
    }
    else
    {
        qDebug() << "数据库连接成功！";
    }

    //使用数据库
    std::shared_ptr<QSqlQuery> query = std::make_shared<QSqlQuery>(qsqlDB);
    if(query != nullptr)
    {
        qDebug() << "数据库使用成功！";
        //外键支持启用
        if(!query->exec("pragma foreign_key = on"))
        {
            this->logSystemPtr->writeLog("外键支持启用失败！错误信息：" + qsqlDB.lastError().text());
            qDebug() << "外键支持启用失败";
        }
        else
        {

            qDebug() << "外键支持启用成功";
        }
    }
    else
    {
        this->logSystemPtr->writeLog("数据库使用失败！错误信息：" + qsqlDB.lastError().text());
        qDebug() << "数据库使用失败！错误信息：" << query->lastError().text();
        qsqlDB.close();
        return nullptr;
    }
    return query;
}
void Model_FileModel::setConnect()
{
    //响应请求生成文件过滤格式信号
    connect(this,&Model_FileModel::requestGenerateFileFilteringFormat,this,&Model_FileModel::generateFileFilteringFormatBySoftName);

    //响应请求获取用户名信号
    connect(this,&Model_FileModel::requestGetUserName,this,&Model_FileModel::getSystemUserName);

    //响应后台任务完成信号
    connect(this->searchingFileFuctureWatcher.get(),&QFutureWatcher<void>::finished,this,[=](){
        finishBackGroudTask(QStringList() << "fileSearchInfoTable");
    });

    //响应请求取消搜索进程信号
    connect(this,&Model_FileModel::requestCancelSearchProcess,this,&Model_FileModel::cancelSearchProcess);

    //响应请求查找全部系统文件信号
    connect(this,&Model_FileModel::requestSearchAllSystemFile,this,[this](){
        //将searchAllSystemFile函数放到后台进程执行
        this->logSystemPtr->writeLog("用户点击了查找全部盘文件按钮");

        if(this->searchingFileFuctureWatcher->isRunning() || this->searchAllFileFutureWatcherPtr->isRunning())
        {
            cancelSearchProcess();
        }

        QFuture searchAllFileFuture = QtConcurrent::run([=](){
            return searchAllSystemFile();
        });
        this->searchAllFileFutureWatcherPtr->setFuture(searchAllFileFuture);
    });

    //响应查找全盘完成信号
    connect(this->searchAllFileFutureWatcherPtr.get(),&QFutureWatcher<void>::finished,this,[=](){
        finishBackGroudTask(getTableNameByAllDiretoryPath());
    });
}
bool Model_FileModel::addFile(QString fileName,QString suffix)
{
    //拼接完整路径
    QString filePath = QDir::currentPath() + "/testRecordFile/" + fileName + "." +suffix;

    //检查路径是否存在和文件名有效性
    if (!isValidText(fileName) || isExist(filePath))
    {
        this->logSystemPtr->writeLog("(Model_FileModel::addFile)文件名无效或路径已存在");
        qDebug() << "(Model_FileModel::addFile)文件名无效或路径已存在";
        return false;
    }

    //创建文件
    QFile createFile(filePath);
    createFile.open(QIODevice::WriteOnly);
    if(!createFile.isOpen())
    {
        this->logSystemPtr->writeLog("(Model_FileModel::addFile)Create fail，error text：" + createFile.errorString());
        qDebug() << "(Model_FileModel::addFile)Create fail，error text：" + createFile.errorString();
        return false;
    }
    createFile.close();

    //将路径加入至数据库中
    if(!insertFilePathToDataBase(filePath,suffix))
    {
        qDebug() << "(Model_FileModel::addFile)文件路径插入失败！文件路径：" << filePath;
        return false;
    }

    //添加文件节点
    addFileItem(qobject_cast<const QStandardItemModel*>(emit getTreeModel()),filePath);
    this->logSystemPtr->writeLog(filePath+" 创建成功！");
    return true;
}
bool Model_FileModel::isValidText(QString text)
{
    //正斜杠和反斜杠有时候和我指定的路径的正斜杠形成一个正斜杠，就会导致文件名变为空白并且逃过名称检测机制，所以要手动检测正斜杠和反斜杠，其它符号(如<>)由系统检测
    if(text.isEmpty() || text.contains("/") || text.contains("\\"))
    {
        return false;
    }
    return true;
}
bool Model_FileModel::isExist(QString filePath)
{
    queryPtr->prepare("select* from fileInfoTable where filePath = ?");
    queryPtr->addBindValue(filePath);
    if(queryPtr->exec())
    {
        if(queryPtr->next())
        {
            queryPtr->finish();//释放查询结果集
            //查找到返回true
            return true;
        }
    }
    else
    {
        //返回true防止继续往下执行
        this->logSystemPtr->writeLog( "(Model_FileModel::isExist)sql语句执行失败！错误信息：" + queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::isExist)sql语句执行失败！错误信息：" << queryPtr->lastError().text();
        return true;
    }
    //查找不到返回false
    return false;
}
bool Model_FileModel::insertFilePathToDataBase(QString filePath,QString suffix)
{
    //根据后缀分配对应的软件名id
    int softNameId = suffixToIdMap.value(suffix);

    //通过预定义语句插入不同的路径及软件名id
    queryPtr->prepare("insert into fileInfoTable(filePath,softNameId) values(?,?)");
    queryPtr->addBindValue(filePath);
    queryPtr->addBindValue(softNameId);
    if(!queryPtr->exec())
    {
        this->logSystemPtr->writeLog("Model_FileModel::insertFilePathToDataBase)插入失败！错误信息："+ queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::insertFilePathToDataBase)插入失败！错误信息：" << queryPtr->lastError().text();
        return false;
    }
    return true;
}
void Model_FileModel::initTreeMode()
{
    //查询softNameTable表,遍历软件名，为每个软件名生成一个组节点分别加入至根节点下
    //遍历完软件名时，查看软件名对应的文件，然后将这些文件加入至对应的软件名下

    this->logSystemPtr->writeLog("程序运行，初步初始化树模型");

    QSqlQuery selectFileItemQuery(db);//用来查询文件节点的sql对象

    QStandardItemModel* treeModel = new QStandardItemModel();
    treeModel->setHorizontalHeaderLabels(QStringList() << "笔记列表");
    //查询软件名表
    if(queryPtr->exec("select* from softNameTable"))
    {
        while(queryPtr->next())
        {
            //根据软件名名称封装成一个个组节点
            QString softName = queryPtr->value("softName").toString();
            QStandardItem* groupItem = new QStandardItem(softName);

            //通过信号设置组节点图标
            emit requestSetGroupItemIcon(groupItem,":/new/prefix1/image/"+softName.toLower()+".png");

            //通过select语句查看软件名对应的文件路径，然后将文件路径分别添加至该组节点下
            selectFileItemQuery.prepare("select* from fileInfoTable where softNameId = ?");
            selectFileItemQuery.addBindValue(queryPtr->value("id").toInt());

            this->logSystemPtr->writeLog("初始化的组节点名：" + softName);
            if(!selectFileItemQuery.exec())
            {
                this->logSystemPtr->writeLog("(Model_FileModel::initTreeMode)添加文件节点失败！错误信息：" + selectFileItemQuery.lastError().text());
                qDebug() << "(Model_FileModel::initTreeMode)添加文件节点失败！错误信息：" << selectFileItemQuery.lastError().text();
                return;
            }

            //遍历表，将数据封装成文件节点加入至组节点下
            while(selectFileItemQuery.next())
            {
                QString filePath = selectFileItemQuery.value("filePath").toString();
                QStandardItem* fileItem = new QStandardItem(QFileInfo(filePath).fileName());
                fileItem->setData(filePath,Qt::UserRole);
                groupItem->appendRow(fileItem);
            }
            treeModel->appendRow(groupItem);
        }
        queryPtr->finish();//释放查询结果集
        emit setTreeMode(treeModel);
    }
    else
    {
        this->logSystemPtr->writeLog("(Model_FileModel::initTreeMode)sql语句执行失败！错误信息： "+ queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::initTreeMode)sql语句执行失败！错误信息： "+ queryPtr->lastError().text();
    }
}
void Model_FileModel::initSearchWindowTreeMode(QStringList tableNameList)
{
    //查询softNameTable表,遍历软件名，为每个软件名生成一个组节点分别加入至根节点下
    //遍历完软件名时，查看软件名对应的文件，然后将这些文件加入至对应的软件名下

    QSqlQuery selectFileItemQuery(db);//用来查询文件节点的sql对象

    QStandardItemModel* treeModel = new QStandardItemModel();
    treeModel->setHorizontalHeaderLabels(QStringList() << "笔记列表");

    //查询软件名表
    if(queryPtr->exec("select* from softNameTable"))
    {
        while(queryPtr->next())
        {
            //根据软件名名称封装成一个个组节点
            QString softName = queryPtr->value("softName").toString();
            QStandardItem* groupItem = new QStandardItem(softName);

            //通过信号设置组节点图标
            emit requestSetGroupItemIcon(groupItem,":/new/prefix1/image/"+softName.toLower()+".png");

            //遍历表名，将单张或多张表里面的数据设置在对应的组节点上
            foreach(QString tableName ,tableNameList)
            {
                //通过select语句查看软件名对应的文件路径，然后将文件路径分别添加至该组节点下
                selectFileItemQuery.prepare("select* from "+tableName+" where softNameId = ?");
                selectFileItemQuery.addBindValue(queryPtr->value("id").toInt());
                if(!selectFileItemQuery.exec())
                {
                    qDebug() << "(Model_FileModel::initTreeMode)添加文件节点失败！错误信息：" << selectFileItemQuery.lastError().text();
                    return;
                }

                //遍历表，将数据封装成文件节点加入至组节点下
                while(selectFileItemQuery.next())
                {
                    QString filePath = selectFileItemQuery.value("filePath").toString();
                    QStandardItem* fileItem = new QStandardItem(filePath);
                    fileItem->setData(filePath,Qt::UserRole);
                    groupItem->appendRow(fileItem);
                }
            }
            //根节点和组节点关联
            treeModel->appendRow(groupItem);
        }
        queryPtr->finish();//释放查询结果集
        //触发信号更新树模型
        emit setSearchWinodwTreeMode(treeModel);
    }
    else
    {
        qDebug() << "(Model_FileModel::initTreeMode)sql语句执行失败！错误信息： "+ queryPtr->lastError().text();
    }
}
void Model_FileModel::addFileItem(const QStandardItemModel* model,QString filePath)
{
    QString softName;

    //根据文件名称找到软件名id，再根据软件名id找到软件名
    queryPtr->prepare("select softName from softNameTable where id = (select softNameId from fileInfoTable where filePath = ?)");
    queryPtr->addBindValue(filePath);
    if(queryPtr->exec())
    {
        while(queryPtr->next())
        {
            softName = queryPtr->value("softName").toString();

            //找到对应的组节点，将文件路径封装成文件节点加入至组节点下
            QStandardItem* groupItem = findGroupItem(model,softName);

            QStandardItem* fileItem = new QStandardItem(QFileInfo(filePath).fileName());
            fileItem->setData(filePath,Qt::UserRole);
            groupItem->appendRow(fileItem);
        }
    }
    else
    {
        this->logSystemPtr->writeLog("(Model_FileModel::addFileItem)sql语句执行失败！错误信息：" + queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::addFileItem)sql语句执行失败！错误信息：" << queryPtr->lastError().text();
    }
    queryPtr->finish();//释放查询结果集
}
QStandardItem* Model_FileModel::findGroupItem(const QStandardItemModel* model,QString text)
{
    //遍历根节点中的组节点，查看哪个项跟传过来的文本一样
    for(int i = 0;i < model->rowCount();i++)
    {
        QStandardItem* item = model->item(i);
        if(item->text() == text)
        {
            return item;
        }
    }
    return nullptr;
}
bool Model_FileModel::openFile(QString filePath)
{
    this->logSystemPtr->writeLog("用户双击了 "+filePath + " 路径文件");
    if(QDesktopServices::openUrl(QUrl::fromLocalFile(filePath)))
    {
        return true;
    }
    return false;
}
QString Model_FileModel::getSystemUserName()
{
    //检查用户名是否已经被获取，如果没被获取，就利用QProcessEnvironment获取
    if(this->userName.isEmpty())
    {
        //QProcessEnvironment是包含系统变量的一个类
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment(); //systemEnvironment获取系统所有的环境变量，包含用户名
        if (QSysInfo::productType()== "windows") //判断操作系统是否是windows，如果不是windows，那就是linux/macos
        {
            this->userName = env.value("USERNAME");
        }
        else
        {
            //linux分为主用户和备用户，所以当主用户获取不到那就获取备用户名
            this->userName = env.value("USER");
            if (this->userName.isEmpty())
            {
                this->userName = env.value("LOGUSER");
            }
        }
        //当都获取失败时，利用外部进程进行获取（相当于利用cmd获取）
        if (this->userName.isEmpty())
        {
            QProcess process;
            process.start("whoami", {}, QProcess::ReadOnly);
            process.waitForStarted(1000);

            //当进程的状态是正常退出时，代表获取到了用户名，那就读出结果将其转换为QString类型放到userName中
            if (process.exitStatus() == QProcess::NormalExit)
            {
                this->userName = QString::fromUtf8((process.readAllStandardOutput())).trimmed();
            }
        }
    }
    return this->userName;
}
QString Model_FileModel::generateFileFilteringFormatByFilePath(QString filePath)
{
    QString filteredFile;
    //通过文件路径找到软件名id，再根据软件名id找到软件名所有后缀
    queryPtr->prepare("select *from suffixTable where softNameId = (select softNameId from fileInfoTable where filePath = ?)");
    queryPtr->addBindValue(filePath);
    if(queryPtr->exec())
    {
        //遍历软件名下所有后缀，组成一个过滤格式
        while(queryPtr->next())
        {
            filteredFile += "*." + (queryPtr->value("softNameSuffix").toString() + " ");
        }
    }
    else
    {
        qDebug() << "(Model_FileModel::generateFileFilteringFormatByFilePath)sql语句执行错误！错误信息：" << queryPtr->lastError().text();
    }
    queryPtr->finish();//释放查询结果集
    qDebug() << "filteredFile：" << filteredFile;
    return filteredFile;
}
QString Model_FileModel::generateFileFilteringFormatBySoftName(QString softName)
{
    QString filteredFile;
    //通过文件路径找到软件名id，再根据软件名id找到软件名所有后缀
    queryPtr->prepare("select *from suffixTable where softNameId = (select id from softNameTable where softName = ?)");
    queryPtr->addBindValue(softName);
    if(queryPtr->exec())
    {
        //遍历软件名下所有后缀，组成一个过滤格式
        while(queryPtr->next())
        {
            filteredFile += "*." + (queryPtr->value("softNameSuffix").toString() + " ");
        }
    }
    else
    {
        qDebug() << "(Model_FileModel::generateFileFilteringFormatByFilePath)sql语句执行错误！错误信息：" << queryPtr->lastError().text();
    }
    queryPtr->finish();//释放查询结果集
    qDebug() << "filteredFile：" << filteredFile;
    return filteredFile;
}
bool Model_FileModel::replaceFilePath(QString oldFilePath,QString newFilePath)
{
    //利用update更新某个数据的路径值
    queryPtr->prepare("update fileInfoTable set filePath = ? where filePath = ?");
    queryPtr->addBindValue(newFilePath);
    queryPtr->addBindValue(oldFilePath);
    if(!queryPtr->exec())
    {
        this->logSystemPtr->writeLog("(Model_FileModel::replaceFilePath)sql语句执行失败！错误信息：" + queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::replaceFilePath)sql语句执行失败！错误信息：" << queryPtr->lastError().text();
        return false;
    }
    return true;
}
void Model_FileModel::updateFileItemFilePath(const QStandardItemModel* model,QString oldFilePath,QString newFilePath)
{
    queryPtr->prepare("select * from softNameTable where id = (select softNameId from fileInfoTable where filePath = ?)");
    queryPtr->addBindValue(newFilePath);
    if(queryPtr->exec())
    {
        //调用next使得指针停留至第一行数据上
        if(queryPtr->next())
        {
            QStandardItem* fileItem = findFileItem(findGroupItem(model,queryPtr->value("softName").toString()),oldFilePath);
            fileItem->setData(newFilePath,Qt::UserRole);
            fileItem->setText(QFileInfo(newFilePath).fileName());
        }
        queryPtr->finish();//释放查询结果集
    }
    else
    {
        this->logSystemPtr->writeLog("(Model_FileModel::updateFileItemFilePath)sql语句执行失败！错误信息：" + queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::updateFileItemFilePath)sql语句执行失败！错误信息：" << queryPtr->lastError().text();
    }
}
QStandardItem* Model_FileModel::findFileItem(const QStandardItem* groupItem,QString filePath)
{
    //根据组节点和路径找到对应文件节点
    for(int i = 0;i < groupItem->rowCount();i++)
    {
        if(groupItem->child(i)->data(Qt::UserRole).toString() == filePath)
        {
            qDebug() << "child：" + groupItem->child(i)->data(Qt::UserRole).toString(); + "   oldFilePath：" + filePath;
            return groupItem->child(i);
        }
    }
    return nullptr;
}
QStandardItem* Model_FileModel::findFileItem(const QStandardItemModel* model,QString filePath)
{
    QStandardItem* fileItem = nullptr;
    queryPtr->prepare("select * from softNameTable where id = (select softNameId from fileInfoTable where filePath = ?)");
    queryPtr->addBindValue(filePath);
    if(queryPtr->exec())
    {
        if(queryPtr->next())
        {
            //利用重载版本找文件节点
            fileItem = findFileItem(findGroupItem(model,queryPtr->value("softName").toString()),filePath);
        }
    }
    else
    {
        qDebug() << "( Model_FileModel::findFileItem)sql语句执行失败！错误信息：" << queryPtr->lastError().text();
    }
    queryPtr->finish();//释放查询结果集
    return fileItem;
}
bool Model_FileModel::removeFileItem(QStandardItemModel* model,QString removeFilePath)
{
    //从组节点中移除文件节点
    QStandardItem* fileItem = findFileItem(model,removeFilePath);

    //获取文件节点索引相关信息
    QModelIndex index = fileItem->index();

    //根据提供的索引相关信息删除该行
    model->removeRow(index.row(),index.parent());

    //从数据库中移除路径
    queryPtr->prepare("delete from fileInfoTable where filePath = ?");
    queryPtr->addBindValue(removeFilePath);
    if(!queryPtr->exec())
    {
        this->logSystemPtr->writeLog("(Model_FileModel::removeFileItem)sql语句执行失败！错误信息：" + queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::removeFileItem)sql语句执行失败！错误信息：" << queryPtr->lastError().text();
        return false;
    }
    return true;
}
void Model_FileModel::searchDirectoryFile(QString directoryPath)
{
    //查找文件逻辑过程在txt文档有
    //查看目录路径是否为空
    this->logSystemPtr->writeLog("用户点击了查找某个文件按钮，查找路径：" + directoryPath);
    if(!directoryPath.isEmpty())
    {
        //查看上一次进程有没有在进行，如果在进行那就取消
        if(this->searchingFileFuctureWatcher->isRunning() || this->searchAllFileFutureWatcherPtr->isRunning())
        {   
            // //设置取消标志
            // this->searchingFileFuctureWatcher->cancel();

            // //等待取消成功
            // this->searchingFileFuctureWatcher->waitForFinished();

            // this->searchAllFileFutureWatcherPtr->cancel();
            // this->searchAllFileFutureWatcherPtr->waitForFinished();

            // this->logSystemPtr->writeLog("取消上一次搜索进程");
            // qDebug() << "取消上一次搜索进程";
            cancelAllBackgroundProcesses();
        }

        qDebug() << "1";
        //当查找的路径是与上次路径相同并没有取消进程的，则直接显示窗口，无需重复查找
        if(prevSearchiFilePath == directoryPath && !this->searchingFileFuctureWatcher->isCanceled())
        {
            this->logSystemPtr->writeLog("本次和上次查找的路径一致，因此直接打开窗口");
            qDebug() << "prevSearchiFilePath：" << prevSearchiFilePath << "\n directoryPath：" << directoryPath;
            emit requestFileSearchWinodwShow();
            return;
        }
        prevSearchiFilePath = directoryPath;//记录这一次路径，跟下次路径进行对比
        qDebug() << "1.5";
        //不管是否有上一次进程，都将fileSearchInfoTable表里面的数据删除，因为每一次搜索都是一次新的数据，不能混合旧数据
        queryPtr->exec("delete from fileSearchInfoTable;");//清除表数据
        qDebug() << "1.6";
        queryPtr->exec("delete from sqlite_sequence where name = 'fileSearchInfoTable';");//清除表的最大计数器值，使其从0开始
        qDebug() << "2";
        //获取所有后缀
        QStringList filteredFileList = this->getAllSuffixFilterFormats();
        //如果list容器为空，说明获取后缀失败，则直接退出
        if(filteredFileList.isEmpty())
        {
            emit requestPromptError("获取文件过滤格式失败，请联系作者！");
            return;
        }
        qDebug() << "searchiFileFilteredFile："<< filteredFileList;

        //将目录路径放到后台进程进行搜索
        int i = 1;
        QFuture<void> searchFileFuture = QtConcurrent::run([=](){
            return findSystemFile(directoryPath,filteredFileList,i,this->searchingFileFuctureWatcher,"fileSearchInfoTable");
        });

        //开始计时
        this->calculateExecutionTimeElapsedTimer.start();

        //设置监视对象
        this->searchingFileFuctureWatcher->setFuture(searchFileFuture);
        qDebug() << "3";
        //提示用户在搜索中
        qDebug() << "开始搜索" << directoryPath << " 目录路径";
        this->logSystemPtr->writeLog( "开始搜索" + directoryPath + " 目录路径");
        emit requestPromptSearching("   搜索中   ");
    }
}
void Model_FileModel::findSystemFile(QString directoryPath,QStringList filteredFile,int i,std::shared_ptr<QFutureWatcher<void>> futureWatcherPtr,QString tableName)
{
    qDebug() << "(findSystemFile)i:" << i;
    //创建新连接，不同线程不能使用同一个连接
    QSqlDatabase searchFileDB;
    int fileCount = 0;
    //通过i来确定不同的连接名，防止同时多次调用只有最后一次连接有效
    QString connectName = QString("searchFileConnecttion %1").arg(i);
    std::shared_ptr<QSqlQuery> searchFileQuery = initMysqlConnect(connectName,searchFileDB);
    qDebug() << directoryPath << "  connectName：" << connectName;
    qDebug() << "directoryPath:" << directoryPath;
    qDebug() << "filteredFile:" << filteredFile;
    //QDirIterator是专门遍历目录和子目录的迭代器，QDirIterator it(路径, 过滤规则, 文件类型, 遍历选项);
    //| QDir::Hidden
    //首次启动时开启事务
    if(searchFileDB.transaction())
    { qDebug() << "首次开启临时提交成功！"; }
    else
    {
        this->logSystemPtr->writeLog("首次开启临时提交失败！错误信息：" + searchFileDB.lastError().text());
        qDebug() << "首次开启临时提交失败！";
    }
    QDirIterator it(directoryPath,filteredFile,QDir::Files | QDir::Readable | QDir::NoDotAndDotDot | QDir::NoSymLinks,QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while(it.hasNext())
    {
        //检查是否设置了取消标志
        if(futureWatcherPtr->isCanceled())
        {
            break;//跳出循环关闭连接
        }
        //利用sql语句插入至数据库中
        searchFileQuery->prepare("insert into "+tableName+"(filePath,softNameId) values(?,?)");
        QString filePath = it.next();
        searchFileQuery->addBindValue(filePath);
        searchFileQuery->addBindValue(suffixToIdMap.value(QFileInfo(filePath).suffix().toLower()));

        if(!searchFileQuery->exec())
        {
            //保存了上一次的值，也就是begin transaction，导致开启事务后再次执行exec时就会重复开启
            this->logSystemPtr->writeLog("(Model_FileModel::findSystemFile)sql语句执行失败！文件路径："+ filePath +"  错误信息：" + searchFileQuery->lastError().text());
            emit requestPromptError("查询失败！错误路径："+ filePath + "\n请联系作者！");
            qDebug() << "filePath:" << filePath;
            qDebug() << "(Model_FileModel::findSystemFile)sql语句执行失败！错误信息：" << searchFileQuery->lastError().text();
            break;
        }

        //每200个文件提交一次事务
        if((++fileCount) % 200 == 0)
        {
            qDebug() << "提交文件的总数：" <<fileCount;
            //提交事务
            if(searchFileDB.commit())
            { qDebug() << "临时提交成功！"; }
            else
            {
                this->logSystemPtr->writeLog("最后临时提交失败！错误信息：" + searchFileDB.lastError().text());
                qDebug() << "临时提交失败！";
            }
            //提交完重新开启事务
            if(searchFileDB.transaction())
            { qDebug() << "开启临时提交成功！"; }
            else
            {
                this->logSystemPtr->writeLog("开启临时提交失败！错误信息：" + searchFileDB.lastError().text());
                qDebug() << "开启临时提交失败！";
            }

        }
        //触发信号将文件路径打印至屏幕中
        emit requesetShowSeaechFilePath(filePath);

        //检查是否设置了取消标志
        if(futureWatcherPtr->isCanceled())
        {
            break;
        }
    }
    //当不足一百个文件时再次提交事务
    if(searchFileDB.commit())
    { qDebug() << "最后临时提交成功！"; }
    else
    {
        this->logSystemPtr->writeLog("最后临时提交失败！错误信息：" + searchFileDB.lastError().text());
        qDebug() << "最后临时提交失败！";
    }
    //关闭连接
    searchFileDB.close();

    this->spendTimeList.push_back(this->calculateExecutionTimeElapsedTimer.elapsed()/1000.0);//获取查找时间并插入至容器中
    this->logSystemPtr->writeLog(directoryPath + " 查找完成");
    qDebug() << "搜索完成：" + directoryPath;
    qDebug() << "后台进程结束";
}
void Model_FileModel::finishBackGroudTask(QStringList tableNameList)
{
    qDebug() << "搜索完成，开始初始化树模型";

    //初始化树模型并将模型设置到查找文件窗口的treeview中
    initSearchWindowTreeMode(tableNameList);

    //获取文件总数
    int fileCount = 0;
    foreach(QString tabelName,tableNameList)
    {
        if(!queryPtr->exec("select count(id) from "+tabelName))
            this->logSystemPtr->writeLog("获取文件数失败！错误信息："+queryPtr->lastError().text());
        queryPtr->next();//将指针移到至第一行
        fileCount += queryPtr->value("count(id)").toInt();
        queryPtr->finish();//释放查询结果集，查询完成后告诉数据库释放资源，取消对应的锁状态
    }
    qDebug() << "文件总数：" << fileCount;
    //获取经过时间，并通过arg插入至字符串中显示
    QString finishText = QString("   搜索完成，耗时 %1 秒，获取到 %2 个文件   ").arg(this->calculateExecutionTimeElapsedTimer.elapsed()/1000.0).arg(fileCount);

    //触发信号让两个视图知道搜索完成了，进行相关操作，执行完成将后台进程和设置树模型所花费的时间通过信号传到视图显示
    emit requestPromptSearchFinish(finishText);
}
void Model_FileModel::cancelSearchProcess()
{
    cancelAllBackgroundProcesses();
}
void Model_FileModel::searchAllSystemFile()
{
    int MainI = 1;//代表这是第几个搜索文件连接

    //获取所有盘符路径
    QFileInfoList fileInfoList = QDir::drives();

    //获取所有后缀格式
    QStringList filteredFileList = this->getAllSuffixFilterFormats();
    if(filteredFileList.isEmpty())
    {
        emit requestPromptError("获取文件过滤格式失败，请联系作者！");
        return;
    }

    //开启计时器
    this->calculateExecutionTimeElapsedTimer.start();

    //遍历盘符，为每个盘符封装成一个后台进程
    for(QFileInfo& fileInfo : fileInfoList)
    {
        qDebug() << "fileInfo：" << fileInfo.filePath();
        // //如果标志位为true，那么无需进行下次搜索
        // if(this->searchAllFileIsCancel){
        //     return;
        // }
        //获取盘符
        QString drivePath = fileInfo.path();

        this->logSystemPtr->writeLog("查找的盘符：" + drivePath);//记录查找盘符

        //获取表名，将每个表名传到耗时函数让进程插入至该表中
        QString tableName = QString("%1").arg(drivePath.front())+"_tabel";

        //搜索前清空表数据
        queryPtr->exec("delete from "+tableName);
        queryPtr->prepare("delete from sqlite_sequence where name = ?");
        queryPtr->addBindValue(tableName);
        if(!queryPtr->exec())
        {
            this->logSystemPtr->writeLog(drivePath + "盘符线程执行错误，错误信息：" +queryPtr->lastError().text());
            qDebug() << drivePath << "盘符线程执行错误，错误信息：" << queryPtr->lastError().text();
            return;
        }

        //通知UI组件显示搜索中
        emit requestPromptSearching(drivePath+"搜索中......");
        qDebug() << "(searchAllSystemFile::for)i:" <<  MainI;

        //执行耗时函数
        findSystemFile(drivePath,filteredFileList,MainI,this->searchAllFileFutureWatcherPtr,tableName);

        MainI++;
    }
}
QStringList Model_FileModel::getAllSuffixFilterFormats()
{
    //获取所有后缀过滤格式
    QStringList filteredFile;
    if(!queryPtr->exec("select *from suffixTable"))
    {
        this->logSystemPtr->writeLog("(Model_FileModel::getAllSuffixFilterFormats)sql语句执行失败！错误信息：" + queryPtr->lastError().text());
        qDebug() << "(Model_FileModel::getAllSuffixFilterFormats)sql语句执行失败！错误信息：" << queryPtr->lastError().text();
        return filteredFile;
    }
    while(queryPtr->next())
    {
        filteredFile << ("*." + queryPtr->value("softNameSuffix").toString());
    }
    queryPtr->finish();//释放查询结果集
    return filteredFile;
}
void Model_FileModel::createTableByDirectoryPath()
{
    this->logSystemPtr->writeLog("程序运行，即将确定盘符和对应的表名");
    for(QFileInfo& fileInfo : QDir::drives())
    {
        //根据盘符路径创建表
        QString drivePath = fileInfo.path();
        QString tableName = QString("%1").arg(drivePath.front())+"_tabel";
        queryPtr->prepare("create table if not exists "+tableName+"("
                          "id integer primary key autoincrement,"
                          "filePath text not null unique,"
                          "softNameId integer,"
                          "constraint f_foreign_key_"+tableName+" foreign key(softNameId) references softNameTable(id)"
                          ")");
        this->logSystemPtr->writeLog("盘符：" + drivePath + " 表名："+ tableName);
        if(!queryPtr->exec())
        {
            this->logSystemPtr->writeLog("(Model_FileModel::createTableByDirectoryPath)sql语句执行失败，错误信息：" + queryPtr->lastError().text());
            qDebug() << "(Model_FileModel::createTableByDirectoryPath)sql语句执行失败！错误信息：" + queryPtr->lastError().text();
        }
    }
}
QStringList Model_FileModel::getTableNameByAllDiretoryPath()
{
    QStringList tableNameList;
    for(QFileInfo& fileInfo : QDir::drives())
    {
        //获取所有盘符的第一个字母与"_table"字符串组成一个表名
        tableNameList += QString("%1").arg(fileInfo.path().front()) + "_tabel";
    }
    qDebug() << "tableNameList：" <<tableNameList;
    return tableNameList;
}
void Model_FileModel::cancelAllBackgroundProcesses()
{
    this->logSystemPtr->writeLog("检查查找文件进程，如果进行则取消");
    //当搜索进程在运行并且没有设置取消标志的时候就取消
    if(this->searchingFileFuctureWatcher->isRunning() && !this->searchingFileFuctureWatcher->isCanceled())
    {
        //取消后仅展示查找到的文件，没查找到的不显示
        this->searchingFileFuctureWatcher->cancel();

        qDebug() << "取消后台进程";

        this->searchingFileFuctureWatcher->waitForFinished();

        //触发请求提示完成信号
        emit requestPromatCancelSuccess();
    }

    //检查是否运行且未设置取消标志
    if(this->searchAllFileFutureWatcherPtr->isRunning() && !this->searchAllFileFutureWatcherPtr->isCanceled())
    {
        this->searchAllFileIsCancel = true;

        this->searchAllFileFutureWatcherPtr->cancel();

        qDebug() << "取消后台进程";

        this->searchAllFileFutureWatcherPtr->waitForFinished();
    }
}
void Model_FileModel::loadFile(QString diretoryPath)
{

}
void Model_FileModel::testFindTime(QString testPath,int testCount)
{
    for(int i = 0;i< testCount;i++)
    {
        qDebug() << "进入第" << i + 1 <<  "次查找";
        searchDirectoryFile(testPath);
        this->searchingFileFuctureWatcher->waitForFinished();
    }
    double sumTime = 0;
    for(int i = 0;i< this->spendTimeList.length();i++)
    {
        qDebug() << "第" << i + 1 << "次查找时间：" << this->spendTimeList[i];
        sumTime += this->spendTimeList[i];
    }
    qDebug() << testCount << "次总共花费时间：" << sumTime;
    qDebug() << testCount << "次平均花费时间：" << sumTime/testCount;
}
