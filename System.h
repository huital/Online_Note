#ifndef SYSTEM_H
#define SYSTEM_H

#include "config.h"

#include <QString>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QObject>
#include <qobject.h>

class System : public QObject{
    Q_OBJECT
public:

    virtual int create_bank(QString name, QStandardItem* item) = 0;
    virtual int create_group(QString name, QStandardItem* item) = 0;
    virtual int create_note(QString name, QStandardItem* item) = 0;
    virtual int create_label(QString name, QStandardItem* item) = 0;

    virtual int open_bank(QString name, QStandardItemModel* model) = 0;
    virtual int add_label(QString name, QStandardItem* item) = 0;

    virtual int delete_bank(QString name, QStandardItem* item) = 0;
    virtual int delete_group(QString name, QStandardItem* item) = 0;
    virtual int delete_note(QString name,  QStandardItem* item) = 0;
    virtual int delete_label(QString name, QStandardItem* item) = 0;

    virtual int rename_bank(QString name, QStandardItem* item) = 0;
    virtual int rename_group(QString name, QStandardItem* item) = 0;
    virtual int rename_note(QString name, QStandardItem* item) = 0;
    virtual int rename_label(QString oldName, QString newName, QStandardItem* item) = 0;


protected:
    QString absolute_root_path;     // 实际存放的根路径（/）
    QString remote_root_path;       // 远程存放的根路径（/）
    QString bank_name;              // 打开的笔记库名称
    QString current_path;           // 当前正在操作的路径
};

#endif // SYSTEM_H
