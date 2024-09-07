#ifndef NETWORKTASK_H
#define NETWORKTASK_H

#include "NetWorkBase.h"
#include <functional>
#include <QVector>
#include <QDebug>

class NetworkTaskForStringVector : public NetworkTaskBase {
    Q_OBJECT
public:
    using TaskFunction = std::function<QVector<QString>()>;

    NetworkTaskForStringVector(TaskFunction func, QObject* parent = nullptr)
        : NetworkTaskBase(parent), taskFunction(func) {}

    void run() override {
        QVector<QString> result = taskFunction();
        qDebug() << "任务完成";
        emit resultReady(result);
        emit taskFinished(); // 任务完成信号
    }

signals:
    void resultReady(QVector<QString> result);

private:
    TaskFunction taskFunction;
};

#endif // NETWORKTASK_H
