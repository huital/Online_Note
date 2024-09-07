#ifndef SOCKETSYSTEM_H
#define SOCKETSYSTEM_H

#include "System.h"
#include "PathGroup.h"
#include <QSqlDatabase>
#include <QObject>
#include <QStandardItemModel>
#include <alibabacloud/oss/OssClient.h>
#include <qobject.h>
#include <QSerialPort>
using namespace AlibabaCloud::OSS;


class SocketSystem : public System
{
    Q_OBJECT

public:

    SocketSystem();
    SocketSystem(QString absolute_path, QString remote_path);
    ~SocketSystem();

    /*
    * 注册函数
    * @param functionName: 函数名称
    * @param func: 函数指针
    */
    void registerFunction(const QString &functionName, std::function<void(QString, QStandardItem*)> func);

    void initializeFunctions();

    void init_root_path(QString absolute_path, QString remote_path);

    /*
    // 创建笔记库，
    // @param name: 笔记库名称，输入不能以/结尾
    // @param item: 创建位置
    // returns: 根据错误信息成功会返回SUCCESS， 其余可能出错NAME_ERROR;
    */
    int create_bank(QString name, QStandardItem* item) override;

    void create_bank_thread(QString name, QStandardItem *item);

    /*
    * 创建分组，item表示创建位置
    * @param name: 创建名称
    * @param item: 创建位置
    */
    int create_group(QString name, QStandardItem* item) override;
    void create_group_thread(QString name, QStandardItem* item);



    /*
    * 创建笔记
    * @param name: 创建名称
    * @param item: 创建位置
    */
    int create_note(QString name, QStandardItem* item) override;
    void create_note_thread(QString name, QStandardItem* item);



    /*
    * 创建标签，在数据库中建表
    * @param name: 创建名称
    * @param item: 创建位置
    */
    int create_label(QString name, QStandardItem* item) override;
    void create_label_thread(QString name, QStandardItem* item);




    /*
    * 打开一个已有的笔记库，并在model中进行初始化
    * @param name:需要打开的笔记库名称
    * @param model:初始化的model
    */
    int open_bank(QString name, QStandardItemModel* model) override;

    void open_bank_thread(QString name, QStandardItem* item);


    /*
    * 在指定的item中添加标签，注意标签来源于经过创建后的label中，
    * 注意id属性将放置在labelitem中，而不是文件名称列
    * @param name:需要添加标签的文件名称
    * @param item:添加位置，名称列
    */
    int add_label(QString name, QStandardItem* item) override;





    int delete_bank(QString name, QStandardItem* item) override;
    int delete_group(QString name, QStandardItem* item) override;
    int delete_note(QString name, QStandardItem* item) override;
    int delete_label(QString name, QStandardItem* item) override;

    int rename_bank(QString name, QStandardItem* item) override;
    int rename_group(QString name, QStandardItem* item) override;
    int rename_note(QString name, QStandardItem* item) override;
    int rename_label(QString oldName, QString newName, QStandardItem* item) override;

    int init_socket(QString BucketName, QString EndPoint, QString AccessId, QString AccessSecret, QString ObjectName);

    /*获得指定目录下的内容（文件以及子目录）*/
    bool exists(QStandardItem* item, QString name, bool is_dir);
    /*
    * 在指定path中寻找其下的内容（文件以及子目录）
    * @param path:符合model路径
    * @param with_prefix:为false时将返回远程端绝对路径，否则只返回名称
    * @param with_suffix:为false时，将剔除返回值的后缀（.txt,.db,/)
    * @return:返回一个名称列表
    */
    QVector<QString> get_content_from_group(QString path, bool with_prefix=false, bool with_suffix=true);

    QVector<QVector<QString>> get_conetent_with_time(QString path, bool with_prefix=false, bool with_suffix=true);

    void processRemotePath(const QString& remotePath, const QString& localPrefix);
    void ensureLocalDirectoryExists(const QString& path);
    int download_in_local(QString local_prefix, bool is_covered = true);

    int download_in_loacal(bool is_covered);

    void get_content_from_group_thread(QString path, bool with_prefix=false, bool with_suffix=true);

    /*
    * 通过查找标签列表来找到系统现存的标签
    * @return:返回现存标签列表，如果没有任何标签，那么将返回空
    */
    QVector<QString> get_current_labels();

    void get_current_labels_thread();

    QVector<QString> get_record_by_id(QString tableName, QString id);


    /*
    * 通过查找指定标签所在的表，返回其中所有记录属性（id,名称，路径）
    * @return:返回现存标签列表，如果没有任何标签，那么将返回空
    */
    QVector<QVector<QString>> get_records(QString tableName);




    /*
    * 从服务器上获得笔记库存储结构，并在model中显示出来（不含标签显示）
    * @param model: 需要显示的model
    * @param rootPath:笔记库名称（含/为后缀）
    * @param depth:最大显示深度
    * @return:根据执行情况，返回标志码
    */
    int buildFileTree_socket(QStandardItemModel* model, QString rootPath, int depth);



    /*
    * 从本地数据库中（从服务器上更新过）获得标签系统，并在model中显示
    * @param model: 需要显示的model
    * @return:根据执行情况，返回标志码
    */
    int buildLabel(QStandardItemModel *model);



    /*
    * 创建数据库文件，不能单独调用，必须在获得正确的remote_root_path后才能创建
    * 创建的数据库文件位置将在本地以及服务器中都存在，并且返回
    * @param databse: 给系统使用的数据库变量
    * @return:根据执行情况，返回标志码
    */
    int create_database(QSqlDatabase &databse);



    /*
    * 将本地文件上传到服务器中，目前测得若服务器端存在对应文件时，将采取覆盖更新（还是说根据最后修改时间？）
    * @param remote_path: 服务器端位置（不含bucket前缀，但是需要包含后缀）
    * @param absolute_path:本地位置，在temp中通过unique_filepath获得实际有效路径
    * @return:根据执行情况，返回标志码
    */
    int upload_file(QString remote_path, QString absolute_path);



    /*
    * 将服务器上的文件下载到本地端
    * @param remote_path: 服务器端位置（不含bucket前缀，但是需要包含后缀）
    * @param absolute_path:本地位置，在temp中通过unique_filepath获得实际有效路径
    * @return:根据执行情况，返回标志码
    */
    int download_file(QString remote_path, QString absolute_path);

    int copy_file(QString old_path, QString new_path);


    int delete_file(QString remote_path, QString absolute_path);

    int delete_group(QString remote_path);


    int download_in_loacal(QString local_predix, bool is_covered=true);
    /*
    * 在本地端指定路径（需要包含.txt后缀）创建文件
    * @param absolute_path:本地位置，在temp中通过unique_filepath获得实际有效路径
    * @return:根据执行情况，返回标志码
    */
    int createFile(const QString &absolute_path);



    /*
    * 在数据库中创建指定名称的label表，并且初始化表的属性，并在总表中添加记录
    * @param name:需要创建的label名称
    * @return:根据执行情况，返回标志码
    */
    int create_label_table(QString name);



    /*
    * 在指定label中添加一条记录
    * @param name:需要添加记录的label名称
    * @param id:标签列的id号
    * @param fileName:文件名称
    * @param filePath:文件所在路径（不包含.txt后缀，前缀不包含，即起点与model起点对应）
    * @return:根据执行情况，返回标志码
    */
    int add_label_record(QString tableName, QString id, QString fileName, QString filePath);

    int add_top_label_record(QString name);

    int delete_label_record(QString tableName, QString id);

    int remove_label_record(QString oldTable, QString newTable, QString id);

    int rename_label_record(QString tableName, QString id, QString name, QString path);


    int clear_label_table(QString tableName);


    /*
    * 将本地修改后的数据库同步更新到服务其中
    * @return:根据执行情况，返回标志码
    */
    int save_system();

    int open_file(QStandardItem *item, QString base, QString &content);
    int save_file(QStandardItem *item, QString &content);

    QString get_basename(QString prefix, QString path, bool with_suffix=true);
    QString model_to_path(QStandardItem* item, QString name);
    QString get_current_path();
    QString uniqueFilePath(const QString& path);
    QString absolute_to_remote(QString absolute_path);
    QString get_id(QStandardItem *item);
    QString rename_path(QString oldName, QString newName, QString oldPath);
    QString rename_path_by_prefix(QString oldName, QString newName, QString oldPath);

    QStandardItem* path_to_model(QStandardItemModel* model, QString path);

    QStringList* search_model(QString search_path, QStandardItemModel* model);

    void populateDirectory(QStandardItem *parentItem, const QString &rootPath, int currentDepth, int depth);

    // 上位机端口API
    QVector<QString> get_port();        // 获得当前的端口号
    QSerialPort* initializeSerialPort(const QString &portName); // 根据端口号初始化一个串口
    int sendCommand(QSerialPort* serialPort, const QString &command); // 发送命令
    bool sendCommandAndWaitForResponse(QSerialPort* serialPort, const QString &command);
    void processRemotePath2(const QString& remotePath, const QString& localPrefix, QSerialPort* serialPort);
    int test(QString remote_path, QString absolute_path);
    void clear_path();
    void set_path(QString path);
    void update_absolute_path();
    void update_remote_path();
    void clear_root_path();
    void show_path();
    void traverseAndProcess(QStandardItem *item, bool delete_labels=true);
    void traverseAndRename(QStandardItem *item, QString old_prefix, QString new_prefix);
    void clear_system();
    void set_database(QString name);

signals:
    void dataProcessed(int result);
    void groupTransfered(QVector<QString> result);
    void labelTransfered(QVector<QString> result);

public slots:
    void processData(const QString &functionName,  QString name, QStandardItem *item);

private:
    QString current_root_path;
    QString temp_root_path;
    QString database_root_path; // 数据库所在根路径
    QString absolute_path;      // 本地绝对路径
    QString remote_path;        // 远程绝对路径
    PathGroup *temp_files;      // 缓存管理系统
    OssClient *client;
    QSqlDatabase database;      // 数据库
    QVector<QString> attributes;    // 标签表属性
    // 访问服务器时需要配置的项
    std::string BucketName;     // 存储库名称
    std::string Endpoint;       // 入口点
    std::string AccessId;       // RAM访问ID
    std::string AccessSecret;   // RAM访问密码
    std::string ObjectName;     // 访问对象地址（绝对）
    // 多线程中需要使用的注册函数列表
    QMap<QString, std::function<void(QString, QStandardItem*)>> functionRegistry;

};

#endif // SOCKETSYSTEM_H
