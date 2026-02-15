#include "logsystem.h"
//静态变量要在类外进行初始化，来去分配空间
std::shared_ptr<LogSystem> LogSystem::logSystemPtr = nullptr;
LogSystem::LogSystem(QObject *parent)
    : QObject{parent}
{
    //设置日志配置文件路径(文件名格式：应用程序名+logFileConfig 组成 日志文件名)
    this->logFileConfigFile.setFileName("./"+QCoreApplication::applicationName() + "LogFileConfig"+".txt");

    //读取日志的不带后缀文件名
    readLogFileConfig();

    //设置日志文件路径
    this->logFile.setFileName("./"+this->logFileName+".log");//设置日志文件路径
    checkSize();//检查大小
    this->logFile.open(QIODevice::WriteOnly | QIODevice::Append);//以追加的方式进行写入文件
    connect(&this->flushTimer,&QTimer::timeout,this,[this](){
        //刷新缓冲区写入日志至磁盘中
        logFile.flush();
    });

    //定时保存，防止程序崩溃写入失败，每20秒写入一次
    this->flushTimer.start(10000);
}
LogSystem::~LogSystem()
{
    this->logFile.close();
    this->flushTimer.stop();
}
void LogSystem::writeLog(QString text)
{
    //日志格式：时间+日志信息
    QDateTime dateTime = QDateTime::currentDateTime();
    logFile.write(dateTime.toString("yyyy-MM-dd HH:mm:ss").toUtf8() + "   " + text.toUtf8() + "\n");
}
std::shared_ptr<LogSystem> LogSystem::getLogSystemObject()
{
    //如果未创建实例就创建实例，否则直接返回该实例，确保使用的是同一个实例
    if(logSystemPtr == nullptr)
    {
        logSystemPtr = std::make_shared<LogSystem>();
    }
    return logSystemPtr;
}
void LogSystem::checkSize()
{
    //如果大小超过了100mb就更换日志文件
    if(this->logFile.size() > 100*MB)
    {
        //通过日志文件存在检测机制获取新的日志文件名，从而防止获取到旧文件名覆盖旧日志文件
        this->logFileName = this->logFileExistsMechanism(this->logFileName.back().toLatin1() + 1);
        this->logFile.setFileName("./"+this->logFileName+".log");//重名命后重新设置文件路径
        qDebug() << "新日志文件：" <<this->logFileName;
    }
    saveLogFileConfig();
}
void LogSystem::readLogFileConfig()
{
    //以读的方式读出不带后缀的文件名
    this->logFileConfigFile.open(QIODevice::ReadOnly);

    if(!this->logFileConfigFile.isOpen())
    {
        //重新创建日志文件时防止旧日志文件被覆盖而写了日志文件检测机制
        this->logFileName = logFileExistsMechanism('1');

        //当日志配置文件不存在时，调用saveLogFileConfig重新创建文件
        saveLogFileConfig();
        qDebug() << "重新命名的日志文件：" << this->logFileName;
        return;
    }
    this->logFileName = this->logFileConfigFile.readAll();
    qDebug() << "打开的日志文件名：" <<this->logFileName;
    this->logFileConfigFile.close();
}
void LogSystem::saveLogFileConfig()
{
    this->logFileConfigFile.open(QIODevice::WriteOnly);
    //由于我只写入不带后缀的文件名，所以直接将不带后缀的文件名写入进去，不加任何说明)
    this->logFileConfigFile.write(this->logFileName.toUtf8());//保存不带后缀的文件名，方便读出
    this->logFileConfigFile.close();
}
QString LogSystem::logFileExistsMechanism(char index)
{
    //日志文件存在检测机制，利用do，while寻找最新一次的日志文件
    //通过index递增寻找不存在的文件，将不存在的文件名返回来当作新的日志文件名
    QString tempLogFileName;
    do
    {
        //this->logFileName记录的是不带后缀的文件名
        tempLogFileName = QCoreApplication::applicationName() + "LogFile" + index ;
        index += 1;
    }while(QFile(tempLogFileName + ".log").exists());
    return tempLogFileName;
}
