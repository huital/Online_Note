#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QStringList>
#include <QMouseEvent>
#include <iostream>
#include <alibabacloud/oss/OssClient.h>
#include <OssClientImpl.h>
#include "opertion.h"
#include <QDir>
#include "socketsystem.h"
#include "workthread.h"
#include <qobject.h>
#include <QElapsedTimer>
#include <QWaitCondition>
#include <QMutex>
#include <QThreadPool>
#include <QSerialPortInfo>
#include <QSerialPort>
#include "networktask.h"
#include "networktaskforint.h"
#include <QTextCodec>

using namespace AlibabaCloud::OSS;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define COL_WIDTH           250

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int init_system();              // 初始化系统（恢复初始设置）
    void create_banks(int selected_model);
    void mousePressEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    QStandardItem* get_selected_item(bool flag=true);
    void error_show(int ret);
    void switch_item(QStandardItem* item);
    void set_font_menu();
    void set_size_menu();
    void set_lineHeight_menu();
    void adjustIndentation(QTextCursor &cursor, int adjust);
    void removeLeadingSpaces(QTextCursor &cursor);
    void initializeProcessing(const QString &functionName, QString name, QStandardItem* item);
    void runForDuration(qint64 duration);

    void open_bank_aggregator();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void handleTabKey();

signals:
    void download_files(QString path, int is_dir, QString content);

private slots:

    void showTreeContextMenu(const QPoint &pos);

    void on_actionModelSelect_triggered();

    void on_actionCreateBank_triggered();

    void on_actionTest_triggered();

    void on_actionCheck_triggered();

    void on_actionOpenBank_triggered();

    void on_action_triggered();

    void on_actionCreateNote_triggered();

    void on_actionCreateLabel_triggered();

    void on_actionSave_triggered();

    void on_actionAddLabel_triggered();

    void on_actionDeleteLabel_triggered();

    void on_actionDeleteNote_triggered();

    void on_actionDeleteGroup_triggered();

    void on_actionRenameLabel_triggered();

    void on_actionRenameNote_triggered();

    void on_actionRenameGroup_triggered();

    void on_actionOpenNote_triggered();

    void on_actionSaveNote_triggered();

    void onNoteTreeDoubleClicked(const QModelIndex &index);

    void handleSearchResult(int index);

    void performSearch();

    void on_tbnHeavy_clicked();

    void on_tbnUnderline_clicked();

    void on_tbnColor_clicked();

    void on_tbnList_clicked();

    void on_ptnList_clicked();

    void onBanksTaskFinished(QVector<QString> result);


    void onBanksOpenFinished(QVector<QString> result);
    void onLabelsOpenTaskFinished(QVector<QString> result);

    void onErrorShow(int result);
    void showLabel(int result);
    void onItemCreate(int result);
    void onDeleteLabelItem(int result);
    void onDeleteNoteItem(int result);


    void on_actionDownload_triggered();

    void on_download_files(QString path, int is_dir, QString content);

    void handleReadyRead() {

        QByteArray data = port->readAll();

        QTextCodec *tc = QTextCodec::codecForName("GBK");

        QString str = tc->toUnicode(data);//str如果是中文则是中文字符
        QString data_2 = QString::fromLocal8Bit(port->readAll());
        qDebug() << "Received data:" << str;
        // 根据实际的响应内容判断是否成功
        if (str.contains("OK")) { // 假设成功的响应包含 "OK"
            qDebug() << str;
            QMessageBox::information(this, "成功", "发送命令成功");
        } else {
            QMessageBox::critical(this, "失败", "发送命令失败");
        }
    }


private:
    Ui::MainWindow *ui;
    int modelSelected;      // 模式选择标记位，无效值0
    QSqlDatabase data_base;
    QStandardItemModel* model;
    QStandardItem* current_item;
    QString current_name;
    // 控制标记
    bool is_operated;           // 检测是否打开了一个笔记库
    SocketSystem *system;
    QVector<QString> current_banks;
    QVector<QString> current_labels;
    int current_type;
    WorkThread* worker_thread;

    //多线程资源
    QThreadPool *threadPool;
    QWaitCondition waitCondition;
    QMutex mutex;
    bool dataProcessed;
    bool banksTaskFinished;
    bool labelsTaskFinished;
    QSerialPort* port;

};
#endif // MAINWINDOW_H
