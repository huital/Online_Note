#ifndef NETWORKTASKFORMODEL_H
#define NETWORKTASKFORMODEL_H

#include "NetWorkBase.h"
#include <functional>
#include <QStandardItemModel>
#include <QDebug>

class NetworkTaskForModel : public NetworkTaskBase {
    Q_OBJECT
public:
    using TaskFunction = std::function<QStandardItemModel*()>;

    NetworkTaskForModel(TaskFunction func, QObject* parent = nullptr)
        : NetworkTaskBase(parent), taskFunction(func) {}

    void run() override {
        QStandardItemModel* result = taskFunction();
//        qDebug() << "操作返回值:" << result;
        emit resultReady(result);
        emit taskFinished(); // 任务完成信号
    }

signals:
    void resultReady(QStandardItemModel* result);

private:
    TaskFunction taskFunction;
};

#endif // NETWORKTASKFORMODEL_H
