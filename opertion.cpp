#include "opertion.h"



QVector<QString> get_files_inside(OssClient client, QString bucket_name, QString keyPrefix, bool with_prefix, bool is_recurved)
{
    QVector<QString> result;
    std::string nextMarker = "";
    bool isTruncated = false;
    do {
        // 构建请求
       ListObjectsRequest request(bucket_name.toStdString());
       request.setPrefix(keyPrefix.toStdString());
       if (!is_recurved)
           request.setDelimiter("/");
       request.setMarker(nextMarker);

       // 执行请求指令
       auto outcome = client.ListObjects(request);
       if (!outcome.isSuccess()) {
           /* 异常处理。*/
           std::cout << "ListObjects fail" <<
           ",code:" << outcome.error().Code() <<
           ",message:" << outcome.error().Message() <<
           ",requestId:" << outcome.error().RequestId() << std::endl;
           break;
       }
        // 将去除前缀后的文件（夹）名称添加至组中
       for (const auto& object : outcome.result().ObjectSummarys()) {
           if (with_prefix) {
               if (keyPrefix.toStdString() != object.Key()) {
                   result.append(QString::fromStdString(object.Key()));
               }
           } else {
               result.append(get_basename(object.Key(), keyPrefix));
           }

       }
       for (const auto& commonPrefix : outcome.result().CommonPrefixes()) {
           if (with_prefix) {
               result.append(QString::fromStdString(commonPrefix));
           } else {
               result.append(get_basename(commonPrefix, keyPrefix));
           }

       }
       nextMarker = outcome.result().NextMarker();
       isTruncated = outcome.result().IsTruncated();
    } while(isTruncated);
    return result;
}

QString get_basename(std::string file_path, QString prefix, bool is_prefix)
{
    // 第一步：将 std::string 转换为 QString
    QString qFilePath = QString::fromStdString(file_path);

    // 第二步：判断是否以 prefix 开头
    if (is_prefix) {
        if (qFilePath.startsWith(prefix)) {
            // 如果以 prefix 为前缀，返回去除前缀后的子字符串
            return qFilePath.mid(prefix.length());
        } else {
            // 如果不以 prefix 为前缀，返回空字符串
            return "";
        }
    } else {
        if (qFilePath.endsWith(prefix)) {
            qFilePath.chop(4);
        }
        return qFilePath;
    }
}

int upload_file(OssClient client, std::string bucket_name, QString remote_path, QString absolute_path)
{
    PutObjectOutcome outcome;
    // 上传文件夹
    if (remote_path.endsWith('/')) {
        std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
        outcome = client.PutObject(bucket_name, remote_path.toStdString(), content, ObjectMetaData());
    } else {
        outcome = client.PutObject(bucket_name, remote_path.toStdString(), absolute_path.toStdString());
    }
    if (!outcome.isSuccess()) {
        /* 异常处理。*/
        std::cout << "PutObject fail" <<
        ",code:" << outcome.error().Code() <<
        ",message:" << outcome.error().Message() <<
        ",requestId:" << outcome.error().RequestId() << std::endl;
        return -1;
    }
    return 0;
}

QSqlDatabase create_database(QString name)
{
    QFile file(name);

    // 添加数据库驱动
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    // 设置数据库文件的名称
    db.setDatabaseName(name);

    // 尝试打开数据库连接
    if (!db.open()) {
        return QSqlDatabase();
    }
    return db;
}

int buildFileTree_socket(OssClient client, QString bucket_name, QString rootPath, QStandardItemModel *model, int depth)
{
    if (depth <= 0) {
        return -1; // 深度为0，无法继续遍历
    }


    // 使用递归函数填充模型
    populateDirectory(client, bucket_name, model->invisibleRootItem(), rootPath, 0, depth);
    return 0; // 成功
}

void populateDirectory(OssClient client, QString bucket_name, QStandardItem *parentItem, const QString &rootPath, int currentDepth, int depth)
{
    if (currentDepth > depth) return;
    QVector<QString> entries = get_files_inside(client, bucket_name, rootPath, true);
    if (entries.isEmpty()){
        return;
    }
    foreach (const QString &entry, entries){
        if (entry.endsWith(".db"))
            continue;
        QString fileName = get_basename(entry.toStdString(), rootPath);
        QStandardItem *nameItem = new QStandardItem(fileName);
        QStandardItem *labelItem = new QStandardItem("");
        if (!is_dir(entry)) {
            fileName = get_basename(fileName.toStdString(), ".txt", false);
            nameItem->setText(fileName);
            nameItem->setForeground(QBrush(Qt::blue));
           parentItem->appendRow(QList<QStandardItem*>() << nameItem << labelItem);
        } else {
            parentItem->appendRow(QList<QStandardItem*>() << nameItem << labelItem);
            QStandardItem *childItem = parentItem->child(parentItem->rowCount() - 1, 0);
            populateDirectory(client, bucket_name, childItem, entry, currentDepth + 1, depth);
        }
    }
}

void database_init(QSqlDatabase db)
{
    if (!db.isOpen()) {
        qDebug() << "Database is not open!";
        return ;
    }

    // 创建表的SQL语句
    QString createTableSQL = "CREATE TABLE IF NOT EXISTS 标签 ("
                             "标签名称 TEXT NOT NULL)";

    // 使用QSqlQuery执行SQL语句
    QSqlQuery query(db);
    if (!query.exec(createTableSQL)) {
        qDebug() << "Error creating table: " << query.lastError().text();
        return ;
    }
    return ;
}

int model_to_path(QStandardItem *item, QString& relateive_path, int model_selected)
{
    if(item == nullptr) {
        return -1;
    }

    if (relateive_path != "") {
        return -2;
    }


    if (model_selected == 3) {
        while (item) {
            // For files, only take the name and ignore any possible labels (second column)
            if (item->columnCount() > 1 && item->index().column() == 1) {
                // Skip the current item and move to its parent
                item = item->parent();
                continue;
            }

            // Prepend the text of the current item to the virtual path
            QString itemText = item->text();
            if (!itemText.endsWith('/'))
                itemText.append(".txt");
            relateive_path.prepend(itemText);

            // Move to the parent item
            item = item->parent();
        }
    }
    return 0;
}

bool is_dir(QString path, int model_selected)
{
    if (model_selected == 3) {
        if (path.endsWith('/')) {
            return true;
        }
    }
    return false;
}

bool same_name_check(OssClient client, QString bucket_name, QString keyPrefix, QString name)
{
    QVector<QString> result = get_files_inside(client, bucket_name, keyPrefix);
    if (result.contains(name)){
        return false;
    } else {
        return true;
    }

}

QString uniqueFilePath(const QString &path)
{
    QString uniquePath = path;
    QFileInfo originalInfo(path);
    QString originalDir = originalInfo.absolutePath();
    qDebug() << "原来的文件目录:" << originalDir;
    int count = 1;
    while (QFile::exists(uniquePath)) {
        QFileInfo fileInfo(path);
        QString baseName = fileInfo.baseName();
        QString suffix = fileInfo.completeSuffix();
        uniquePath = QString("%1_%2.%3").arg(baseName).arg(count).arg(suffix);
        uniquePath = QDir(originalDir).filePath(uniquePath);
        count++;
    }

    // Ensure the returned path has the same prefix as the input path
    return uniquePath;
}


