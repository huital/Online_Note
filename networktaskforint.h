#ifndef NETWORKTASKFORINT_H
#define NETWORKTASKFORINT_H

#include "NetWorkBase.h"
#include <functional>
#include <QDebug>

class NetworkTaskForInt : public NetworkTaskBase {
    Q_OBJECT
public:
    using TaskFunction = std::function<int()>;

    NetworkTaskForInt(TaskFunction func, QObject* parent = nullptr)
        : NetworkTaskBase(parent), taskFunction(func) {}

    void run() override {
        int result = taskFunction();
//        qDebug() << "操作返回值:" << result;
        emit resultReady(result);
        emit taskFinished(); // 任务完成信号
    }

signals:
    void resultReady(int result);

private:
    TaskFunction taskFunction;
};

#endif // NETWORKTASKFORINT_H
