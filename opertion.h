#ifndef OPERTION_H
#define OPERTION_H
#include <alibabacloud/oss/OssClient.h>
#include <QVector>
#include <QString>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QBrush>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QFileInfo>
#include <QDir>
#include "PathGroup.h"

using namespace AlibabaCloud::OSS;

// 数据转换API
int model_to_path(QStandardItem* item, QString& relateive_path, int model_selected=3);
bool is_dir(QString path, int model_selected=3);

// 数据库API
QSqlDatabase create_database(QString name);
void database_init(QSqlDatabase db);        // 初始化数据库


// 服务器端API
QVector<QString> get_files_inside(OssClient client, QString bucket_name, QString keyPrefix, bool with_prefix = false, bool is_recurved=false);        // 返回指定目录下的文件以及子目录，第二个参数控制是否显示子目录内容
QString get_basename(std::string file_path, QString prefix, bool is_prefix=true);    // 剔除前缀，返回值可能为空,通过第三个参数决定是前缀还是后缀
QString uniqueFilePath(const QString& path);        // 确保本地端文件名称唯一性
int upload_file(OssClient client, std::string bucket_name, QString remote_path, QString absolute_path);
int download_file(OssClient client, std::string bucket_name, QString remote_path, QString absolute_path);
int buildFileTree_socket(OssClient client, QString bucket_name, QString rootPath, QStandardItemModel* model, int depth);
void populateDirectory(OssClient client, QString bucket_name, QStandardItem *parentItem, const QString &rootPath, int currentDepth, int depth);



bool same_name_check(OssClient client, QString bucket_name, QString keyPrefix, QString name);           // 重名检查

#endif // OPERTION_H
