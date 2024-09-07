#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include <QObject>
#include <QThread>
#include "socketsystem.h"
class WorkThread : public QThread
{
    Q_OBJECT
public:
    SocketSystem* system;
    explicit WorkThread(QString absolute_path, QString remote_path, QObject *parent = nullptr) : QThread(parent), absolute_path(absolute_path), remote_path(remote_path){
        system = new SocketSystem(absolute_path, remote_path);
        system->moveToThread(this); // 数据处理，移动至子线程中
        system->initializeFunctions();
        connect(this, &WorkThread::process, system, &SocketSystem::processData);
        connect(system, &SocketSystem::dataProcessed, this, &WorkThread::dataProcessed);

        connect(this, &WorkThread::groupTransfer, system, &SocketSystem::get_content_from_group_thread);
        connect(system, &SocketSystem::groupTransfered, this, &WorkThread::groupTransfered);

        connect(this, &WorkThread::labelTransfer, system, &SocketSystem::get_current_labels_thread);
        connect(system, &SocketSystem::labelTransfered, this, &WorkThread::labelTransfered);
    }

    ~WorkThread()
     {
       quit();
       wait();
       delete system;
    }

signals:
    void process(const QString &functionName,  QString name, QStandardItem *item);
    void dataProcessed(int result);

    void groupTransfer(QString path, bool with_prefix, bool with_suffix);
    void groupTransfered(QVector<QString> result);

    void labelTransfer();
    void labelTransfered(QVector<QString> result);

protected:
    void run() override
    {
        exec();
    }

private:

    QString absolute_path;
    QString remote_path;

};

#endif // WORKTHREAD_H
