#ifndef NETWORKBASE_H
#define NETWORKBASE_H


#include <QObject>
#include <QRunnable>

class NetworkTaskBase : public QObject, public QRunnable {
    Q_OBJECT

signals:
    void taskFinished();

public:
    NetworkTaskBase(QObject* parent = nullptr) : QObject(parent) {}
    virtual void run() override = 0;
};

#endif // NETWORKBASE_H
