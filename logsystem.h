#ifndef LOGSYSTEM_H
#define LOGSYSTEM_H

#define LOGFILENAME QCoreApplication::applicationName() + "LogFileConfig%1.txt"
#define MB 1024*1024
#include <QObject>
#include<QFile>
#include<QDateTime>
#include<QTimer>
#include<QFileInfo>
#include<QCoreApplication>
class LogSystem : public QObject
{
    Q_OBJECT
public:
    explicit LogSystem(QObject *parent = nullptr);
    ~LogSystem();

    //写入日志
    void writeLog(QString text);

    //通过静态函数获取唯一实例
    static std::shared_ptr<LogSystem> getLogSystemObject();

private:
    LogSystem(const LogSystem&) = delete;//将拷贝构造删了，防止利用拷贝构造创建出不同的实例

    //通过静态指针来去确保创建的是唯一实例
    static std::shared_ptr<LogSystem> logSystemPtr;

    QFile logFile;//用于写入日志的file对象
    QFile logFileConfigFile;//用户保存日志文件配置的file对象

    QTimer flushTimer;

    //日志名
    QString logFileName;

    //日志大小轮换机制
    void checkSize();

    //读出日志文件配置
    void readLogFileConfig();

    //保存日志文件配置
    void saveLogFileConfig();

    //日志文件存在检测机制
    QString logFileExistsMechanism(char index);
signals:
};

#endif // LOGSYSTEM_H
