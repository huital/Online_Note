#include "socketsystem.h"
#include <memory>
#include <fstream>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QVariantList>
#include <QMap>
#include <functional>
#include <QElapsedTimer>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QTextCodec>
#include <QEventLoop>
#include <QTimer>

SocketSystem::SocketSystem()
{
    this->absolute_root_path = "";
    this->remote_root_path = "";
    this->temp_root_path = "";
    this->database_root_path = "";
    this->BucketName = "";
    this->Endpoint = "";
    this->AccessId = "";
    this->AccessSecret = "";
    this->ObjectName = "";
    this->temp_files = new PathGroup(MAX_FILE_NUMBER);
}

SocketSystem::SocketSystem(QString absolute_path, QString remote_path)
{
    this->absolute_root_path = absolute_path;
    this->remote_root_path = remote_path;
    this->temp_root_path = absolute_path + "tmp/";
    this->database_root_path = absolute_path;
    this->BucketName = "";
    this->Endpoint = "";
    this->AccessId = "";
    this->AccessSecret = "";
    this->ObjectName = "";
    this->temp_files = new PathGroup(MAX_FILE_NUMBER);
    this->attributes = {"序号", "文件名称", "文件路径"};
}

SocketSystem::~SocketSystem()
{

}

void SocketSystem::registerFunction(const QString &functionName, std::function<void (QString, QStandardItem*)> func)
{
    functionRegistry[functionName] = func;
}

void SocketSystem::initializeFunctions()
{
    registerFunction("create_bank", std::bind(&SocketSystem::create_bank_thread, this, std::placeholders::_1, std::placeholders::_2));
    registerFunction("create_group", std::bind(&SocketSystem::create_group_thread, this, std::placeholders::_1, std::placeholders::_2));
    registerFunction("create_note", std::bind(&SocketSystem::create_note_thread, this, std::placeholders::_1, std::placeholders::_2));
    registerFunction("create_label", std::bind(&SocketSystem::create_label_thread, this, std::placeholders::_1, std::placeholders::_2));
    registerFunction("open_bank", std::bind(&SocketSystem::open_bank_thread, this, std::placeholders::_1, std::placeholders::_2));
}

void SocketSystem::init_root_path(QString absolute_path, QString remote_path)
{
    this->absolute_root_path = absolute_path;
    this->remote_root_path = remote_path;
    this->temp_root_path = absolute_path + "tmp/";
    this->database_root_path = absolute_path;
}

int SocketSystem::create_bank(QString name, QStandardItem *item)
{
    if (name.endsWith('/')) {
        return NAME_INVALID;
    }
    if (exists(item, name, true)) {
        clear_path();
        clear_root_path();
        return NAME_ERROR;
    }
    this->remote_root_path = name + "/";
    this->current_root_path =  this->absolute_root_path + name + "/";
    // 更新路径
    this->remote_path = this->remote_root_path;
    this->absolute_path = this->current_root_path;
    this->temp_root_path = this->current_root_path + "tmp/";
    // 本地端创建目录项
    QString dirPath = this->absolute_path;

    dirPath.chop(1);
    QDir dir;
    if (!dir.mkpath(dirPath)) { // 尝试创建目录
        clear_path();
        clear_root_path();
        return DIR_ERROR;
    }
    dirPath.append("/tmp");
    if (!dir.mkpath(dirPath)) { // 尝试创建目录
        clear_path();
        clear_root_path();
        return DIR_ERROR;
    }
    int ret = create_database(this->database);
    if (ret != SUCCESS) {
        clear_path();
        clear_root_path();
        return ret;
    }
    ret = upload_file(remote_path, dirPath);
    if (ret != SUCCESS) {
        clear_path();
        clear_root_path();
        return ret;
    }

    QString remote_data_path = absolute_to_remote(this->database_root_path);
    ret = upload_file(remote_data_path, this->database_root_path);
    return ret;
}

void SocketSystem::create_bank_thread(QString name, QStandardItem *item)
{
    qDebug() << "正在执行多线程";

    emit dataProcessed(create_bank(name, item));
}

int SocketSystem::create_group(QString name, QStandardItem *item)
{
    if (name.endsWith('/')) {
        return NAME_INVALID;
    }
    if (exists(item, name, true)) {
        return NAME_ERROR;
    }
    int ret = upload_file(current_path, "");

    return ret;
}

void SocketSystem::create_group_thread(QString name, QStandardItem *item)
{
    emit dataProcessed(create_group(name, item));
}

int SocketSystem::create_note(QString name, QStandardItem *item)
{
    if (name.endsWith('.txt')) {
        return NAME_INVALID;
    }
    if (exists(item, name, false)) {
        return NAME_ERROR;
    }
    int ret = 0;
    // 解析远程路径以及实际物理路径
    remote_path = current_path;
    absolute_path = uniqueFilePath(this->temp_root_path + name +".txt");
    qDebug() << "实际文件地址:" << remote_path;
    ret = createFile(absolute_path);
    if (ret == SUCCESS) {
        ret = upload_file(remote_path, absolute_path);

//        QStandardItem *nameItem = new QStandardItem(name);
//        QStandardItem *labelItem = new QStandardItem("");
//        item->appendRow(QList<QStandardItem*>() << nameItem << labelItem);
//        nameItem->setForeground(QBrush(Qt::blue));

    }

    qDebug() << "笔记创建完成";
    return ret;
}

void SocketSystem::create_note_thread(QString name, QStandardItem *item)
{
    int result = create_note(name, item);
    qDebug() << "发射信号" << result;
    emit dataProcessed(result);
}

int SocketSystem::create_label(QString name, QStandardItem *item)
{
    int ret = create_label_table(name);
    return ret;
}

void SocketSystem::create_label_thread(QString name, QStandardItem *item)
{
    emit dataProcessed(create_label(name, item));
}

int SocketSystem::open_bank(QString name, QStandardItemModel* model)
{
    int ret = buildFileTree_socket(model, name, 3);
    this->remote_root_path = name ;
    current_root_path =  this->absolute_root_path + name;
    // 更新路径
    this->remote_path = this->remote_root_path;
    name.chop(1);
    remote_path.append(name + ".db");
    this->absolute_path = this->current_root_path;
    this->temp_root_path = this->current_root_path + "tmp/";


    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(this->database_root_path);
    database.open();
    qDebug() << database.databaseName();
    ret = download_file(remote_path, database_root_path);
//    ret = buildLabel(model);
    qDebug() << "笔记库初始化完毕";
    return ret;
}

void SocketSystem::open_bank_thread(QString name, QStandardItem *item)
{
    QStandardItemModel *model = item->model();
    emit dataProcessed(open_bank(name, model));
}

int SocketSystem::add_label(QString name, QStandardItem *item)
{
    QStandardItem* labelitem = item->parent()->child(item->row(), 1);
    QString label_path = model_to_path(item, "");
    QString id = get_id(item);
    int ret = add_label_record(name, id, item->text(), label_path);
    if (ret == SUCCESS) {

    }
    qDebug() << "添加后数据记录:" << get_record_by_id(name, id);
    return ret;
}

int SocketSystem::delete_bank(QString name, QStandardItem *item)
{
    return 0;
}

int SocketSystem::delete_group(QString name, QStandardItem *item)
{
    if (item == nullptr) {
        return NO_ITEM; // 错误：item为空
    }

    // 从传入的item开始遍历和处理
    traverseAndProcess(item);
    remote_path = model_to_path(item, "");
    remote_path.prepend(this->remote_root_path);
    qDebug() << "待删除分组:" << remote_path;
    int ret = delete_group(remote_path);
    return ret;
}

int SocketSystem::delete_note(QString name, QStandardItem *item)
{
    remote_path = model_to_path(item, ".txt");
    remote_path.prepend(this->remote_root_path);
    int ret;
    if (name == "" || item == nullptr) return NO_ITEM;
    QStandardItem* labelItem = item->parent()->child(item->row(), 1);
    if (labelItem->text() != "") {
        ret = delete_label(labelItem->text(), item);
        if (ret != SUCCESS) return ret;
    }
    absolute_path = "";
    ret = delete_file(remote_path, absolute_path);
    return ret;
}

int SocketSystem::delete_label(QString name, QStandardItem *item)
{
    if (name == "") {
        return DELETE_ERROR;
    }

//    QStandardItem *labelItem = item->parent()->child(item->row(), 1);
    qDebug() << "待删除标签名称:" <<  name;
    QString id = get_id(item);
    qDebug() << "标签:" << id;
    int ret = delete_label_record(name, id);
//    if (ret == SUCCESS) {
//        labelItem->setText("");
//    }
    return ret;
}

int SocketSystem::rename_bank(QString name, QStandardItem *item)
{
    return 0;
}

int SocketSystem::rename_group(QString name, QStandardItem *item)
{
    QString old_path = model_to_path(item, "");
    QString new_path = rename_path(item->text(), name, old_path);
    traverseAndRename(item, old_path, new_path);
    return 0;
}

int SocketSystem::rename_note(QString name, QStandardItem *item)
{
    if (exists(item->parent(), name, false)) {
        return NAME_ERROR;
    }
    QString oldPath = model_to_path(item, "");
    QString newPath = rename_path(item->text(), name, oldPath);
    QString label = item->parent()->child(item->row(), 1)->text();
    int ret = SUCCESS;
    if (label != "") {
        ret = rename_label_record(label, get_id(item), name, newPath);
    }
    oldPath.prepend(this->remote_root_path);
    newPath.prepend(this->remote_root_path);
    if (!oldPath.endsWith('/')) {
        oldPath.append(".txt");
        newPath.append(".txt");
    }
    copy_file(oldPath, newPath);
    delete_file(oldPath, "");
    return ret;
}

int SocketSystem::rename_label(QString oldName, QString newName, QStandardItem *item)
{
    QString id = get_id(item);
    qDebug() << "数据记录:" << get_record_by_id(oldName, id);
    int ret = remove_label_record(oldName, newName, id);
    qDebug() << "旧label名称:" << oldName;
    qDebug() << "新label名称:" << newName;
    qDebug() << "id:" << id;

    return ret;
}


int SocketSystem::init_socket(QString BucketName, QString EndPoint, QString AccessId, QString AccessSecret, QString ObjectName)
{
    this->BucketName = BucketName.toStdString();
    this->Endpoint = EndPoint.toStdString();
    this->AccessId = AccessId.toStdString();
    this->AccessSecret = AccessSecret.toStdString();
    this->ObjectName = ObjectName.toStdString();
    InitializeSdk();
    ClientConfiguration conf;
    this->client = new OssClient(this->Endpoint, this->AccessId, this->AccessSecret, conf);
    ShutdownSdk();

    return SUCCESS;
}

bool SocketSystem::exists(QStandardItem *item, QString name, bool is_dir)
{

    QString check_file = name;
    if (!is_dir) {
        check_file.append(".txt");
    } else {
        check_file.append('/');
    }
    current_path = "";
    if (item != nullptr) {

        check_file = model_to_path(item, check_file);
    }
    check_file = this->remote_root_path + check_file;
    current_path = check_file;
    qDebug() << "带检查文件:" << check_file;
    InitializeSdk();
    auto outcome = client->DoesObjectExist(this->BucketName, check_file.toStdString());
    ShutdownSdk();
    if (outcome) {
        return true;
    } else {
        return false;
    }
}

QVector<QString> SocketSystem::get_content_from_group(QString path, bool with_prefix, bool with_suffix)
{
    InitializeSdk();
    QVector<QString> result;
    std::string nextMarker = "";
    bool isTruncated = false;
    do {
        // 构建请求
       ListObjectsRequest request(BucketName);
       request.setPrefix(path.toStdString());
       request.setDelimiter("/");   // 设置不选择递归显示
       request.setMarker(nextMarker);

       // 执行请求指令
       auto outcome = client->ListObjects(request);
       if (!outcome.isSuccess()) {

           break;
       }
        // 将去除前缀后的文件（夹）名称添加至组中
       for (const auto& object : outcome.result().ObjectSummarys()) {
           if (with_prefix) {
               if (path.toStdString() != object.Key()) {
                   QString temp = get_basename("", QString::fromStdString(object.Key()), with_suffix);
                   result.append(temp);
               }
           } else {
               QString temp = get_basename(path, QString::fromStdString(object.Key()), with_suffix);
               if (temp != "")
                    result.append(temp);
           }

       }
       for (const auto& commonPrefix : outcome.result().CommonPrefixes()) {
           if (with_prefix) {
               QString temp = get_basename("", QString::fromStdString(commonPrefix), with_suffix);
               result.append(temp);
           } else {
               QString temp = get_basename(path, QString::fromStdString(commonPrefix), with_suffix);
               if (temp != "")
                result.append(temp);
           }

       }
       nextMarker = outcome.result().NextMarker();
       isTruncated = outcome.result().IsTruncated();
    } while(isTruncated);
    ShutdownSdk();
    qDebug() << "完成查询";
    return result;
}

void SocketSystem::get_content_from_group_thread(QString path, bool with_prefix, bool with_suffix)
{
    qDebug() << "多线程：分组内容查询中";
    QVector<QString> result = get_content_from_group(path, with_prefix, with_suffix);
    emit groupTransfered(result);
    qDebug() << "结束";
}

QVector<QString> SocketSystem::get_current_labels()
{
    QVector<QString> labels;

    if (!database.isOpen()) {
        return labels;
    }
    qDebug() << "标签查询中";
    QString queryStr = "SELECT 标签名称 FROM 标签";
    QSqlQuery query(database);

    if (!query.exec(queryStr)) {
        return labels;
    }

    while (query.next()) {
        QString label = query.value(0).toString();
        labels.append(label);
    }

    return labels;
}

void SocketSystem::get_current_labels_thread()
{
    qDebug() << "多线程：标签查询中";
    emit labelTransfered(get_current_labels());
}

QVector<QString> SocketSystem::get_record_by_id(QString tableName, QString id)
{
    QVector<QString> records;
    // Check if the table exists
    QString checkTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(tableName);
    QSqlQuery checkQuery(database);
    if (!checkQuery.exec(checkTableSQL)) {
        return records;
    }
    if (!checkQuery.next()) {
        return records;
    }

    // Query to get the record with the specified ID from the specified table
    QString fetchRecordSQL = QString("SELECT 序号, 文件名称, 文件路径 FROM %1 WHERE 序号 = :id").arg(tableName);
    QSqlQuery query(database);
    query.prepare(fetchRecordSQL);
    query.bindValue(":id", id);
    if (!query.exec()) {
        return records;
    }
    // Fetch the record
    if (query.next()) {
        records.append(query.value(0).toString()); // 序号
        records.append(query.value(1).toString()); // 文件名称
        records.append(query.value(2).toString()); // 文件路径
    }

    return records;
}

QVector<QVector<QString>> SocketSystem::get_records(QString tableName)
{
    QVector<QVector<QString>> records;
    if (!database.isOpen()) {
        return records;
    }

    // Check if the table exists
    QString checkTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(tableName);
    QSqlQuery checkQuery(database);
    if (!checkQuery.exec(checkTableSQL)) {
        return records;
    }
    if (!checkQuery.next()) {
        return records;
    }

    // Query to get all records from the specified table
    QString selectSQL = QString("SELECT 序号, 文件名称, 文件路径 FROM %1").arg(tableName);
    QSqlQuery selectQuery(database);

    if (!selectQuery.exec(selectSQL)) {
        return records;
    }

    // Fetch all records and store them in the QVector
    while (selectQuery.next()) {
        QVector<QString> record;
        record.append(selectQuery.value(0).toString()); // 序号
        record.append(selectQuery.value(1).toString()); // 文件名称
        record.append(selectQuery.value(2).toString()); // 文件路径
        records.append(record);
    }
    return records;
}

QString SocketSystem::get_basename(QString prefix, QString path, bool with_suffix)
{
    // Step 1: Check if path starts with prefix and remove prefix
    if (!path.startsWith(prefix)) {
        return "";
    }
    path = path.mid(prefix.length());

    // Step 2: If with_suffix is true, remove specific suffixes
    if (!with_suffix) {
        qDebug() << "path: " << path;
        if (path.endsWith("/")) {
            path.chop(1);
        } else if (path.endsWith(".txt")) {
            path.chop(4);
        } else if (path.endsWith(".db")) {
            path.chop(3);
        }
    }

    // Step 3: Return the processed path
    return path;
}

int SocketSystem::buildFileTree_socket(QStandardItemModel *model, QString rootPath, int depth)
{
    if (depth <= 0) {
        return -1; // 深度为0，无法继续遍历
    }


    // 使用递归函数填充模型
    populateDirectory(model->invisibleRootItem(), rootPath, 0, depth);
    return 0; // 成功
}

int SocketSystem::buildLabel(QStandardItemModel *model)
{
    if (!database.isOpen()) {
        return SQL_OPEN_ERROR;
    }

    // 获取所有标签名称
    QSqlQuery query(database);
    if (!query.exec("SELECT 标签名称 FROM 标签")) {
        return SQL_EXEC_ERROR;  // 查询失败
    }

    // 遍历所有标签表
    while (query.next()) {
        QString labelTableName = query.value(0).toString();
        QSqlQuery subQuery(database);
        if (!subQuery.exec("SELECT * FROM " + labelTableName)) {
            continue;  // 查询表失败，继续下一个表
        }

        while (subQuery.next()) {
            QString absolutePath = subQuery.value(2).toString();
            QString relativePath = absolutePath;
            // 根据路径找到对应的模型项
            QStandardItem *item = path_to_model(model, relativePath);

            if (item) {
                QStandardItem *labelItem = item->parent()->child(item->row(), 1);
                if (labelItem) {
                    labelItem->setText(labelTableName);
                    item->setData(subQuery.value(0).toString(), Qt::UserRole + 1);
                }
             }

        }
    }

    return SUCCESS;  // 成功
}

void SocketSystem::populateDirectory(QStandardItem *parentItem, const QString &rootPath, int currentDepth, int depth)
{
    if (currentDepth > depth) return;
    QVector<QString> entries = get_content_from_group(rootPath, true, true);
    if (entries.isEmpty()){
        return;
    }
    foreach (const QString &entry, entries){
        if (entry.endsWith(".db"))
            continue;
        QString fileName = entry;
        if (entry.endsWith(".txt")) {
            fileName.chop(4);
        }
        fileName = get_basename(rootPath, fileName, true);
        QStandardItem *nameItem = new QStandardItem(fileName);
        QStandardItem *labelItem = new QStandardItem("");
        if (!entry.endsWith("/")) {
            nameItem->setText(fileName);
            nameItem->setForeground(QBrush(Qt::blue));
           parentItem->appendRow(QList<QStandardItem*>() << nameItem << labelItem);
        } else {
            parentItem->appendRow(QList<QStandardItem*>() << nameItem << labelItem);
            QStandardItem *childItem = parentItem->child(parentItem->rowCount() - 1, 0);
            populateDirectory(childItem, entry, currentDepth + 1, depth);
        }
    }
}

int SocketSystem::create_database(QSqlDatabase &databse)
{
    QString name = this->remote_root_path;
    name.chop(1);
    this->database_root_path = this->current_root_path + name + ".db";
    QFile file(database_root_path);

    // 添加数据库驱动
    databse = QSqlDatabase::addDatabase("QSQLITE");

    // 设置数据库文件的名称
    databse.setDatabaseName(database_root_path);
    qDebug() << "创建的数据库名称:" << databse.databaseName();
    // 尝试打开数据库连接
    if (!databse.open()) {
        return SQL_OPEN_ERROR;
    }

    // 创建表的SQL语句
    QString createTableSQL = "CREATE TABLE IF NOT EXISTS 标签 ("
                             "标签名称 TEXT NOT NULL)";

    // 使用QSqlQuery执行SQL语句
    QSqlQuery query(databse);
    if (!query.exec(createTableSQL)) {
        return SQL_EXEC_ERROR;
    }
    qDebug () << "数据库创建成功:" << database_root_path;
    return SUCCESS;
}

int SocketSystem::upload_file(QString remote_path, QString absolute_path)
{
    InitializeSdk();
    PutObjectOutcome outcome;
    // 上传文件夹
    if (remote_path.endsWith('/')) {
        std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
        outcome = client->PutObject(BucketName, remote_path.toStdString(), content, ObjectMetaData());
    } else {
        outcome = client->PutObject(BucketName, remote_path.toStdString(), absolute_path.toStdString());
    }
    if (!outcome.isSuccess()) {
        return UPLOAD_ERROR;
    }
    ShutdownSdk();
    return SUCCESS;
}

int SocketSystem::download_file(QString remote_path, QString absolute_path)
{
    InitializeSdk();
    qDebug() << "需要下载的文件：" << remote_path;
    qDebug() << "下载的实际地址：" << absolute_path;
    GetObjectRequest request(BucketName, remote_path.toStdString());
    request.setResponseStreamFactory([=]()
    {return std::make_shared<std::fstream>(absolute_path.toStdString(), std::ios_base::out | std::ios_base::in | std::ios_base::trunc| std::ios_base::binary);
    });

    auto outcome = client->GetObject(request);

    if (!outcome.isSuccess()) {
        return DOWNLOAD_ERROR;
    }
    ShutdownSdk();
    return SUCCESS;
}

int SocketSystem::copy_file(QString old_path, QString new_path)
{
    InitializeSdk();
    ClientConfiguration conf;



    CopyObjectRequest request(BucketName, new_path.toStdString());
    request.setCopySource(BucketName, old_path.toStdString());

    /* 拷贝文件。*/
    auto outcome = client->CopyObject(request);

    if (!outcome.isSuccess()) {
       return COPY_ERROR;
    }
    /* 释放网络等资源。*/
    ShutdownSdk();
    return 0;

}

int SocketSystem::delete_file(QString remote_path, QString absolute_path)
{
    InitializeSdk();
    DeleteObjectRequest request(BucketName, remote_path.toStdString());
    auto outcome = client->DeleteObject(request);
    if (!outcome.isSuccess()) {
        return DELETE_ERROR;
    }
    if (absolute_path != "") {
        // 使用QFile对象
        QFile file(absolute_path);
        // 尝试删除文件
        if (!file.remove()) {
            return DELETE_ERROR; // 删除失败，返回错误代码
        }
    }
    ShutdownSdk();
    return SUCCESS;
}

int SocketSystem::delete_group(QString remote_path)
{
    InitializeSdk();
    std::string nextMarker = "";
    bool isTruncated = false;
    do {
            /* 列举文件。*/
            ListObjectsRequest request(BucketName);
            request.setPrefix(remote_path.toStdString());
            request.setMarker(nextMarker);
            auto outcome = client->ListObjects(request);

            if (!outcome.isSuccess()) {
                break;
            }
            for (const auto& object : outcome.result().ObjectSummarys()) {
                DeleteObjectRequest request(BucketName, object.Key());
                /* 删除文件。*/
                auto delResult = client->DeleteObject(request);
            }
            nextMarker = outcome.result().NextMarker();
            isTruncated = outcome.result().IsTruncated();
    } while (isTruncated);
    /* 释放网络等资源。*/
    ShutdownSdk();
    return SUCCESS;
}

int SocketSystem::download_in_loacal(bool is_covered)
{
    // 从当前打开的笔记库开始访问
    /* 初始化网络等资源。*/
    InitializeSdk();
    /* 列举文件。*/
    ListObjectsRequest request(BucketName);
    auto outcome = client->ListObjects(request);

    std::string keyPrefix = "test1/";
    std::string nextMarker = "";
    bool isTruncated = false;
    do {
        /* 列举文件。*/
        ListObjectsRequest request(BucketName);
         /* 设置正斜线（/）为文件夹的分隔符 */
        request.setDelimiter("/");
        request.setPrefix(keyPrefix);
        request.setMarker(nextMarker);
        auto outcome = client->ListObjects(request);

        if (!outcome.isSuccess ()) {
            /* 异常处理。*/
            std::cout << "ListObjects fail" <<
            ",code:" << outcome.error().Code() <<
            ",message:" << outcome.error().Message() <<
            ",requestId:" << outcome.error().RequestId() << std::endl;
            break;
        }
        for (const auto& object : outcome.result().ObjectSummarys()) {
            std::cout << "object"<<
            ",name:" << object.Key() <<
            ",size:" << object.Size() <<
            ",lastmodify time:" << object.LastModified() << std::endl;
        }
        for (const auto& commonPrefix : outcome.result().CommonPrefixes()) {
            std::cout << "commonPrefix" << ",name:" << commonPrefix << std::endl;
        }
        nextMarker = outcome.result().NextMarker();
        isTruncated = outcome.result().IsTruncated();
    } while (isTruncated);

    /* 释放网络等资源。*/
    ShutdownSdk();
    return 0;
    // 依次在本地端重构远程端层级结构
    // 如果参数为真表示强制覆盖，否则按照更新时间进行。
}

int SocketSystem::createFile(const QString &absolute_path)
{
    // 创建QFile对象
    QFile file(absolute_path);

    // 打开文件，如果文件不存在则创建它
    if (!file.open(QIODevice::WriteOnly)) {
        return FILE_ERROR;
    }

    // 这里可以写入一些初始内容，如果需要的话
    QTextStream out(&file);
    // 关闭文件
    file.close();
    return SUCCESS;
}

int SocketSystem::create_label_table(QString name)
{
    QString name2 = database.databaseName();

    if (!database.isOpen()) {
        return SQL_OPEN_ERROR;
    }

    // 检查表是否存在
    QString checkTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(name);
    QSqlQuery checkQuery(database);
    if (checkQuery.exec(checkTableSQL)) {
        if (checkQuery.next()) {
            return LABEL_NAME_ERROR; // 表已经存在，返回错误码
        }
    } else {
        return LABEL_CREATE_ERROR;
    }

    // 创建新表的SQL语句
    QVector<QString> attributes = {"序号", "文件名称", "文件路径"};
    QString createTableSQL = QString("CREATE TABLE %1 (").arg(name);

    for (int i = 0; i < attributes.size(); ++i) {
        createTableSQL += attributes[i] + " TEXT";
        if (i < attributes.size() - 1) {
            createTableSQL += ", ";
        }
    }
    createTableSQL += ")";

    QSqlQuery query(database);
    if (!query.exec(createTableSQL)) {
        return LABEL_CREATE_ERROR;
    }
    // 在已经存在的表 '标签' 中添加一条记录
    QString insertSQL = QString("INSERT INTO 标签 (标签名称) VALUES (:name)");
    query.prepare(insertSQL);
    query.bindValue(":name", name);

    if (!query.exec()) {
        return LABEL_CREATE_ERROR;
    }

    return SUCCESS;
}

int SocketSystem::add_label_record(QString tableName, QString id, QString fileName, QString filePath)
{
    if (!database.isOpen()) {
        return SQL_OPEN_ERROR;
    }

    // Check if the table exists
    QString checkTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(tableName);
    QSqlQuery checkQuery(database);
    if (!checkQuery.exec(checkTableSQL)) {
        return SQL_EXEC_ERROR;
    }
    if (!checkQuery.next()) {
        return SQL_EXEC_ERROR;
    }

    // Insert the record into the specified table
    QString insertSQL = QString("INSERT INTO %1 (序号, 文件名称, 文件路径) VALUES (:id, :fileName, :filePath)").arg(tableName);
    QSqlQuery insertQuery(database);
    insertQuery.prepare(insertSQL);
    insertQuery.bindValue(":id", id);
    insertQuery.bindValue(":fileName", fileName);
    insertQuery.bindValue(":filePath", filePath);
    if (!insertQuery.exec()) {
        return SQL_EXEC_ERROR;
    }
    return SUCCESS;
}

int SocketSystem::delete_label_record(QString tableName, QString id)
{
    if (!database.isOpen()) {
        return SQL_OPEN_ERROR;
    }

    // Check if the table exists
    QString checkTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(tableName);
    QSqlQuery checkQuery(database);
    if (!checkQuery.exec(checkTableSQL)) {
        return SQL_EXEC_ERROR;
    }
    if (!checkQuery.next()) {
        return SQL_EXEC_ERROR;
    }

    // Delete the record from the specified table
    QString deleteSQL = QString("DELETE FROM %1 WHERE 序号 = :id").arg(tableName);
    QSqlQuery deleteQuery(database);
    deleteQuery.prepare(deleteSQL);
    deleteQuery.bindValue(":id", id);

    if (!deleteQuery.exec()) {
        return SQL_EXEC_ERROR;
    }

    return SUCCESS;
}

int SocketSystem::remove_label_record(QString oldTable, QString newTable, QString id)
{
    QString checkOldTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(oldTable);
    QSqlQuery checkOldTableQuery(database);
    if (!checkOldTableQuery.exec(checkOldTableSQL) || !checkOldTableQuery.next()) {
        return SQL_EXEC_ERROR;
    }

    QString checkNewTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(newTable);
    QSqlQuery checkNewTableQuery(database);
    if (!checkNewTableQuery.exec(checkNewTableSQL) || !checkNewTableQuery.next()) {
        return SQL_EXEC_ERROR;
    }

    QString fetchRecordSQL = QString("SELECT 序号, 文件名称, 文件路径 FROM %1 WHERE 序号 = :id").arg(oldTable);
    QSqlQuery fetchRecordQuery(database);
    fetchRecordQuery.prepare(fetchRecordSQL);
    fetchRecordQuery.bindValue(":id", id);
    if (!fetchRecordQuery.exec() || !fetchRecordQuery.next()) {
        qDebug() << fetchRecordQuery.lastError().text();
        return SQL_EXEC_ERROR;
    }

    QString sequenceNumber = fetchRecordQuery.value(0).toString();
    QString fileName = fetchRecordQuery.value(1).toString();
    QString filePath = fetchRecordQuery.value(2).toString();

    QString insertRecordSQL = QString("INSERT INTO %1 (序号, 文件名称, 文件路径) VALUES (?, ?, ?)").arg(newTable);
    QSqlQuery insertRecordQuery(database);
    insertRecordQuery.prepare(insertRecordSQL);
    insertRecordQuery.addBindValue(sequenceNumber);
    insertRecordQuery.addBindValue(fileName);
    insertRecordQuery.addBindValue(filePath);

    if (!insertRecordQuery.exec()) {
        qDebug() << "插入失败";
        return SQL_EXEC_ERROR;
    }

    QString deleteSQL = QString("DELETE FROM %1 WHERE 序号 = :id").arg(oldTable);
    QSqlQuery deleteQuery(database);
    deleteQuery.prepare(deleteSQL);
    deleteQuery.bindValue(":id", id);

    if (!deleteQuery.exec()) {
        return SQL_EXEC_ERROR;
    }

    return SUCCESS; // Success
}

int SocketSystem::rename_label_record(QString tableName, QString id, QString name, QString path)
{

    QString checkTableSQL = QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(tableName);
    QSqlQuery checkTableQuery(database);
    if (!checkTableQuery.exec(checkTableSQL) || !checkTableQuery.next()) {
        return SQL_EXEC_ERROR; // Table does not exist
    }

    QString fetchRecordSQL = QString("SELECT 序号 FROM %1 WHERE 序号 = :id").arg(tableName);
    QSqlQuery fetchRecordQuery(database);
    fetchRecordQuery.prepare(fetchRecordSQL);
    fetchRecordQuery.bindValue(":id", id);

    if (!fetchRecordQuery.exec()) {
        return SQL_EXEC_ERROR;
    }

    if (!fetchRecordQuery.next()) {
        return SQL_EXEC_ERROR;
    }
    QString updateRecordSQL = QString("UPDATE %1 SET ").arg(tableName);
    bool first = true;

    if (!name.isEmpty()) {
        updateRecordSQL += "文件名称 = :name";
        first = false;
    }
    if (!path.isEmpty()) {
        if (!first) {
            updateRecordSQL += ", ";
        }
        updateRecordSQL += "文件路径 = :path";
    }

    updateRecordSQL += " WHERE 序号 = :id";
    QSqlQuery updateRecordQuery(database);
    updateRecordQuery.prepare(updateRecordSQL);

    if (!name.isEmpty()) {
        updateRecordQuery.bindValue(":name", name);
    }
    if (!path.isEmpty()) {
        updateRecordQuery.bindValue(":path", path);
    }
    updateRecordQuery.bindValue(":id", id);
    if (!updateRecordQuery.exec()) {
        return SQL_EXEC_ERROR;
    }
    return SUCCESS;
}


int SocketSystem::clear_label_table(QString tableName)
{
    if (!database.isOpen()) {
        return SQL_OPEN_ERROR; // 数据库未打开
    }

    // 构建删除所有记录的SQL语句
    QString queryStr = QString("DELETE FROM %1").arg(tableName);
    QSqlQuery query;
    if (!query.exec(queryStr)) {
        return SQL_EXEC_ERROR; // 执行失败
    }

    return SUCCESS; // 成功
}

int SocketSystem::add_top_label_record(QString name)
{

     QSqlQuery query(database);
    // 在已经存在的表 '标签' 中添加一条记录
    QString insertSQL = QString("INSERT INTO 标签 (标签名称) VALUES (:name)");
    query.prepare(insertSQL);
    query.bindValue(":name", name);
    return SUCCESS;
}

int SocketSystem::save_system()
{
    remote_path = absolute_to_remote(this->database_root_path);
    int ret = upload_file(remote_path, this->database_root_path);
    return ret;
}

QString SocketSystem::model_to_path(QStandardItem *item, QString name)
{
    QString result;
    if (name != "") {
        result = name;
    }

    while (item) {

        if (item->columnCount() > 1 && item->index().column() == 1) {

            item = item->parent();
            continue;
        }


        QString itemText = item->text();
        result.prepend(itemText);


        item = item->parent();
    }
    return result;
}

QString SocketSystem::get_current_path()
{
    return this->current_path;
}

void SocketSystem::clear_path()
{
    current_path = "";
}

void SocketSystem::set_path(QString path)
{
    this->current_path = path;
}

void SocketSystem::update_absolute_path()
{
    this->absolute_path = this->current_root_path + current_path;
}

void SocketSystem::update_remote_path()
{
    this->remote_path = this->remote_root_path + current_path;
}

void SocketSystem::clear_root_path()
{
    temp_root_path = "";
    database_root_path = ""; // 数据库所在根路径
    absolute_path = "";      // 本地绝对路径
    remote_path = "";
}

void SocketSystem::show_path()
{
    qDebug() << "当前系统中的根路径:" << current_path;
    qDebug() << "远程端根路径:" << remote_root_path;
    qDebug() << "本地端根路径:" << current_root_path;
    qDebug() << "数据端根路径:" << database_root_path;

}

void SocketSystem::traverseAndProcess(QStandardItem *item, bool delete_labels)
{
    if (item == nullptr) {
        return;
    }
    // 遍历当前item的所有子项
    for (int row = 0; row < item->rowCount(); ++row) {
        QStandardItem *childItem1 = item->child(row, 0); // 第一列item
        QStandardItem *childItem2 = item->child(row, 1); // 第二列item
        if (childItem1 != nullptr && childItem2 != nullptr) {
            // 如果第二列的text不为空，递归调用delete_group
            if (!childItem2->text().isEmpty() && delete_labels) {
                delete_label(childItem2->text(), childItem1);
            }
            // 递归遍历第一列item的孩子
            traverseAndProcess(childItem1);
            if (!childItem1->text().endsWith('/')) {
                delete_note(childItem1->text(), childItem1);
            }
        }
    }
}

void SocketSystem::traverseAndRename(QStandardItem *item, QString old_prefix, QString new_prefix)
{
    if (item == nullptr) {
        return;
    }

    QString oldPath = model_to_path(item, "");
    QString newPath = rename_path_by_prefix(old_prefix, new_prefix, oldPath);
    oldPath.prepend(this->remote_root_path);
    newPath.prepend(this->remote_root_path);
    if (!oldPath.endsWith('/')) {
        oldPath.append(".txt");
        newPath.append(".txt");
    }
    copy_file(oldPath, newPath);
    QStandardItem* parent = item->parent();
    if (parent != nullptr) {
        QString labelName = parent->child(item->row(), 1)->text();
        if (labelName != "") {
           rename_label_record(labelName, get_id(item), "", newPath);
        }
    }
    //copy_file(oldPath, newPath);
    // 如果第二列的text不为空，递归调用delete_group
    // 遍历当前item的所有子项
    for (int row = 0; row < item->rowCount(); ++row) {
        QStandardItem *childItem1 = item->child(row, 0); // 第一列item
        QStandardItem *childItem2 = item->child(row, 1); // 第二列item
        if (childItem1 != nullptr && childItem2 != nullptr) {
            // 递归遍历第一列item的孩子
            traverseAndRename(childItem1, old_prefix, new_prefix);
        }
    }
    delete_file(oldPath, "");
}

void SocketSystem::clear_system()
{
    this->temp_root_path.chop(1);
    QDir dir(this->temp_root_path);
    qDebug() << "删除地址:" << this->temp_root_path;
    if (dir.exists()) {
        // 获取所有文件和文件夹的列表
        QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

        // 遍历文件列表
        for (const QFileInfo &fileInfo : files) {
            if (fileInfo.isFile()) {
                // 删除文件
                QFile::remove(fileInfo.absoluteFilePath());
            } else if (fileInfo.isDir()) {
                // 递归删除子文件夹
                QDir subDir(fileInfo.absoluteFilePath());
                subDir.removeRecursively();
            }
        }
    }
}

void SocketSystem::set_database(QString name)
{
    QString names = name;
    names.chop(1);
    this->database_root_path = this->absolute_root_path + name + names + ".db";
}

void SocketSystem::processData(const QString &functionName, QString name, QStandardItem *item)
{
    if (functionRegistry.contains(functionName)) {
            functionRegistry[functionName](name, item);
    } else {
        emit dataProcessed(FUNCTION_EXISTS_ERROR);
    }
}

int SocketSystem::open_file(QStandardItem *item, QString base, QString &content)
{
    QString path_remote = this->remote_root_path + model_to_path(item, ".txt");

    QString absolute = temp_files->getAbsolutePath(path_remote);
    if (absolute == "")
        absolute = uniqueFilePath(this->temp_root_path + base + ".txt");

    int ret = 0;
    QFile file(absolute);
    if (!file.exists()) {
        ret = download_file(path_remote, absolute);
        if (ret != SUCCESS) return ret;

    }
    temp_files->addPath(path_remote, absolute);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return FILE_OPEN_ERROR; // 文件无法打开
    }
    // 读取文件内容
    QTextStream in(&file);
    in.setCodec("UTF-8");
    content = in.readAll();
    file.close();
    return ret;
}

int SocketSystem::save_file(QStandardItem *item, QString &content)
{
    // 确保路径有效性
    QString path_remote = this->remote_root_path + model_to_path(item, ".txt");
    QString absolute = temp_files->getAbsolutePath(path_remote);
    if (absolute == "") {
        return PATH_GROUP_ERROR;
    }
    qDebug() << "服务器绝对路径:" << path_remote;
    qDebug() << "本地端绝对路径:" << absolute;
    QFile file(absolute);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return FILE_OPEN_ERROR; // 文件无法打开
    }
    // 使用 QTextStream 并设置编码为 UTF-8
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << content;
    file.close();
    int ret = delete_file(path_remote, "");
    if (ret == SUCCESS)
        qDebug() << "删除成功";
    return upload_file(path_remote, absolute);
}

QString SocketSystem::uniqueFilePath(const QString &path)
{
    QString uniquePath = path;
    QFileInfo originalInfo(path);
    QString originalDir = originalInfo.absolutePath();
    int count = 1;
    while (QFile::exists(uniquePath)) {
        QFileInfo fileInfo(path);
        QString baseName = fileInfo.baseName();
        QString suffix = fileInfo.completeSuffix();
        uniquePath = QString("%1_%2.%3").arg(baseName).arg(count).arg(suffix);
        uniquePath = QDir(originalDir).filePath(uniquePath);
        count++;
    }
    qDebug() << "检查后地址：" << uniquePath;
    return uniquePath;
}

QString SocketSystem::absolute_to_remote(QString absolute_path)
{
    QString result;
    // 检查是否以path1开头
    if (absolute_path.startsWith(this->current_root_path)) {
        // 将path1替换为path2，仅替换开头部分
        result =  absolute_path.replace(0, this->current_root_path.length(), this->remote_root_path);
    }

    // 如果不匹配path1，返回原始路径
    return result;
}

QString SocketSystem::get_id(QStandardItem *item)
{
    if (!item) return 0;  // 如果 item 为 null，返回空字符串表示错误
    QVariant uuid = item->data(Qt::UserRole + 1);  // 尝试获取存储在 UserRole + 1 中的 UUID
    if (!uuid.isValid()) {
        // 如果没有有效的 UUID，生成一个新的 UUID
        uuid = QUuid::createUuid().toString();
        item->setData(uuid, Qt::UserRole + 1);  // 将新生成的 UUID 存储在 item 中
        return uuid.toString();
    }
    return uuid.toString();  // 返回 UUID 字符串作为唯一标识符
}

QString SocketSystem::rename_path(QString oldName, QString newName, QString oldPath)
{
    // 检查 oldPath 是否以 oldName 结尾
    if (oldPath.endsWith(oldName)) {
        // 如果匹配，替换 oldName 为 newName
        int pos = oldPath.lastIndexOf(oldName);
        QString newPath = oldPath.left(pos) + newName;
        return newPath;
    } else {
        // 如果不匹配，返回空字符串
        return "";
    }
}

QString SocketSystem::rename_path_by_prefix(QString oldName, QString newName, QString oldPath)
{
    // 检查 oldPath 是否以 oldName 开头
    if (oldPath.startsWith(oldName)) {
        // 如果匹配，替换 oldName 为 newName
        QString newPath = oldPath;
        newPath.replace(0, oldName.length(), newName);
        return newPath;
    } else {
        // 如果不匹配，返回空字符串
        return "";
    }
}

QStandardItem *SocketSystem::path_to_model(QStandardItemModel* model, QString path)
{
    QStringList parts = path.split('/', QString::SkipEmptyParts);
    QStandardItem *currentItem = model->invisibleRootItem();
    foreach (const QString &part, parts) {
        bool found = false;
        for (int i = 0; i < currentItem->rowCount(); i++) {
            QStandardItem *child = currentItem->child(i);
            QString name2 = part + '/';
            // 在服务器端下目录以/为后缀
            if (child && (child->text() == part || child->text() == name2)) {
                currentItem = child;
                found = true;
                break;
            }
        }
        if (!found) return nullptr; // 如果找不到匹配项，返回nullptr
    }
    return currentItem;
}

QStringList *SocketSystem::search_model(QString search_path, QStandardItemModel *model)
{
    QStringList* result_paths = new QStringList(); // 创建新的结果字符串列表

    // 创建一个队列用于迭代遍历树形结构模型
    QList<QStandardItem*> queue;
    for (int row = 0; row < model->rowCount(); ++row) {
        queue.append(model->item(row));
    }

    while (!queue.isEmpty()) {
        QStandardItem* currentItem = queue.takeFirst();

        // 获取当前项的第一列文本
        QString item_text = currentItem->text();
        QStringList item_texts = item_text.split('\t'); // 假设第一列和第二列以制表符分隔
        QString first_column_text = item_texts.value(0);

        // 判断第一列文本是否包含搜索路径
        if (first_column_text.contains(search_path, Qt::CaseInsensitive)) {
            // 调用 model_to_path 函数获取完整路径，并添加到结果列表中
            QString path = item_text + "    " + model_to_path(currentItem, "");
            result_paths->append(path);
        }

        // 如果当前项是文件夹，则将其子项添加到队列中继续迭代
        if (currentItem->hasChildren()) {
            for (int row = 0; row < currentItem->rowCount(); ++row) {
                queue.append(currentItem->child(row));
            }
        }
    }

    return result_paths;
}

QVector<QVector<QString>> SocketSystem::get_conetent_with_time(QString path, bool with_prefix, bool with_suffix)
{
    InitializeSdk();
    QVector<QVector<QString>> result;
    std::string nextMarker = "";
    bool isTruncated = false;
    do {
        // 构建请求
       ListObjectsRequest request(BucketName);
       request.setPrefix(path.toStdString());
       request.setDelimiter("/");   // 设置不选择递归显示
       request.setMarker(nextMarker);

       // 执行请求指令
       auto outcome = client->ListObjects(request);
       if (!outcome.isSuccess()) {

           break;
       }
        // 将去除前缀后的文件（夹）名称添加至组中
       for (const auto& object : outcome.result().ObjectSummarys()) {
           QVector<QString> temp2;  // 暂存文件信息
           if (with_prefix) {
               if (path.toStdString() != object.Key()) {
                   QString temp = get_basename("", QString::fromStdString(object.Key()), with_suffix);
                   temp2.append(temp);
                   temp2.append(QString::fromStdString(object.LastModified()));
                   result.append(temp2);
               }
           } else {
               QString temp = get_basename(path, QString::fromStdString(object.Key()), with_suffix);
               if (temp != "") {
                   temp2.append(temp);
                   temp2.append(QString::fromStdString(object.LastModified()));
                   result.append(temp2);
               }

           }

       }
       // 文件夹
       for (const auto& commonPrefix : outcome.result().CommonPrefixes()) {
           QVector<QString> temp2;
           if (with_prefix) {
               QString temp = get_basename("", QString::fromStdString(commonPrefix), with_suffix);
               temp2.append(temp);
               temp2.append("");
               result.append(temp2);
           } else {
               QString temp = get_basename(path, QString::fromStdString(commonPrefix), with_suffix);
               if (temp != "") {
                   temp2.append(temp);
                   temp2.append("");
                   result.append(temp2);
               }

           }

       }
       nextMarker = outcome.result().NextMarker();
       isTruncated = outcome.result().IsTruncated();
    } while(isTruncated);
    ShutdownSdk();
    qDebug() << "完成查询";
    return result;
}

void SocketSystem::ensureLocalDirectoryExists(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(path);
    }
}

// 递归处理远程路径的内容
void SocketSystem::processRemotePath(const QString& remotePath, const QString& localPrefix) {
    QVector<QVector<QString>> contents = get_conetent_with_time(remotePath);

    for (const QVector<QString>& entry : contents) {
        QString remoteEntry = entry[0];
        QString localEntry = localPrefix + remotePath + remoteEntry;

        if (remoteEntry.endsWith('/')) { // 目录
            ensureLocalDirectoryExists(localEntry);
            processRemotePath(remotePath + remoteEntry, localPrefix);
        } else { // 文件
            if (!remoteEntry.endsWith(".db")) {
                int result = download_file(remotePath + remoteEntry, localEntry);
                if (result != 0) {
                    qDebug() << "Failed to download file from" << (remotePath + remoteEntry) << "to" << localEntry;
                } else {
                    qDebug() << "Downloaded file from" << (remotePath + remoteEntry) << "to" << localEntry;
                }
            } else {
                qDebug() << "Skipping .db file:" << remoteEntry;
            }
        }
    }
}

int SocketSystem::download_in_local(QString local_prefix, bool is_covered) {
    // 保证本地目录存在
    ensureLocalDirectoryExists(local_prefix);

    // 默认远程文件路径
    QString remotePath = this->remote_root_path;

    // 处理远程路径
    processRemotePath(remotePath, local_prefix);

    return 0; // 返回值根据需求定义，这里假设0表示成功
}

QVector<QString> SocketSystem::get_port() {
    QVector<QString> portList;
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        portList.append(serialPortInfo.portName());
    }
    return portList;
}

QSerialPort* SocketSystem::initializeSerialPort(const QString &portName) {
    QSerialPort *serialPort = new QSerialPort();

    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud115200, QSerialPort::AllDirections);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open port" << portName << ", error:" << serialPort->errorString();
        delete serialPort;
        return nullptr;
    }

    qDebug() << "Serial port" << portName << "initialized and opened successfully.";
    return serialPort;
}


int SocketSystem::test(QString remote_path, QString absolute_path) {
    /* 如果未指定本地路径，则下载后的文件默认保存到示例程序所属项目对应本地路径中。*/
    std::string FileNametoSave = absolute_path.toStdString();

    /* 初始化网络等资源。*/
    InitializeSdk();

    ClientConfiguration conf;

    OssClient client(this->Endpoint, this->AccessId, this->AccessSecret, conf);
    std::string ObjectName2 = remote_path.toStdString();
    /* 下载Object到本地文件。*/
    GetObjectRequest request(BucketName, ObjectName2);
    request.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>(FileNametoSave, std::ios_base::out | std::ios_base::in | std::ios_base::trunc| std::ios_base::binary); });

    auto outcome = client.GetObject(request);

    if (outcome.isSuccess()) {
        std::cout << "GetObjectToFile success" << outcome.result().Metadata().ContentLength() << std::endl;
    }
    else {
        /* 异常处理。*/
        std::cout << "GetObjectToFile fail" <<
        ",code:" << outcome.error().Code() <<
        ",message:" << outcome.error().Message() <<
        ",requestId:" << outcome.error().RequestId() << std::endl;
        return -1;
    }

    /* 释放网络等资源。*/
    ShutdownSdk();
    return 0;
}

int SocketSystem::sendCommand(QSerialPort* serialPort, const QString &command) {
    if (!serialPort || !serialPort->isOpen()) {
        qDebug() << "Serial port is not open.";
        return 0;
    }

    // 将命令转换为字节数组并发送
    QTextCodec *tc = QTextCodec::codecForName("GBK");
    qint64 bytesWritten =  serialPort->write(tc->fromUnicode(command));
    
    if (bytesWritten == -1) {
        qDebug() << "Failed to write to port" << serialPort->portName() << ", error:" << serialPort->errorString();
        return 0;
    }

    // 等待所有数据发送完毕
    if (!serialPort->waitForBytesWritten(5000)) { // 5秒超时
        qDebug() << "Timed out waiting for bytes to be written";
        return 0;
    }
    return 1;
}


bool SocketSystem::sendCommandAndWaitForResponse(QSerialPort* serialPort, const QString &command) {
    if (sendCommand(serialPort, command) == 0) {
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(serialPort, &QSerialPort::readyRead, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start(5000); // 5秒超时
    loop.exec();

    if (timer.isActive()) {
        // 读取串口返回值
        QByteArray responseData = serialPort->readAll();
        QString response = QString::fromUtf8(responseData).trimmed();

        if (response == "OK") {
            return true;
        } else if (response == "FAILED") {
            return false;
        } else {
            qDebug() << "Unexpected response:" << response;
            return false;
        }
    } else {
        qDebug() << "Timeout waiting for response";
        return false;
    }
}

void SocketSystem::processRemotePath2(const QString& remotePath, const QString& localPrefix, QSerialPort* serialPort) {
    QVector<QVector<QString>> contents = get_conetent_with_time(remotePath);

    for (const QVector<QString>& entry : contents) {
        QString remoteEntry = entry[0];
        QString localEntry = localPrefix + remotePath + remoteEntry;

        if (remoteEntry.endsWith('/')) { // 目录
            ensureLocalDirectoryExists(localEntry);
            processRemotePath2(remotePath + remoteEntry, localPrefix, serialPort);
        } else { // 文件
            if (!remoteEntry.endsWith(".db")) {
                int result = download_file(remotePath + remoteEntry, localEntry);
                if (result == 0) {

                    QString command = QString("%1#%2#%3").arg(remotePath + remoteEntry).arg(1).arg(localEntry); // Assuming on_download_files is properly defined

                    // 发送命令并等待回复
                    if (sendCommandAndWaitForResponse(serialPort, command)) {
                        qDebug() << "Downloaded file from" << (remotePath + remoteEntry) << "to" << localEntry << "and command sent successfully";
                    } else {
                        qDebug() << "Failed to send command after downloading file from" << (remotePath + remoteEntry) << "to" << localEntry;
                    }
                } else {
                    qDebug() << "Failed to download file from" << (remotePath + remoteEntry) << "to" << localEntry;
                }
            } else {
                qDebug() << "Skipping .db file:" << remoteEntry;
            }
        }
    }
}
