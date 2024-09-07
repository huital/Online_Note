#ifndef CONFIG_H
#define CONFIG_H

#define MAX_DEPTH           3
#define MAX_FILE_NUMBER            10

#define SUCCESS                 0       // 操作成功
#define NAME_ERROR              -1      // 重名错误
#define NAME_INVALID            -2      // 名称无效
#define DIR_ERROR               -3      // 创建目录失败
#define SQL_OPEN_ERROR          -4      // 数据库打开失败
#define SQL_EXEC_ERROR          -5      // 数据库指令执行失败
#define UPLOAD_ERROR            -6      // 服务器上传文件失败
#define DOWNLOAD_ERROR          -7      // 服务器下载文件失败
#define FILE_ERROR              -8      // 创建文件失败
#define LABEL_CREATE_ERROR      -9      // 数据库中标签创建失败
#define NO_ITEM                 -10     // 未选中项
#define LABEL_NAME_ERROR        -11     // 标签重名错误
#define DELETE_ERROR            -12     // 删除错误
#define COPY_ERROR              -13     // 拷贝错误
#define FILE_OPEN_ERROR         -14     // 文件打开失败
#define PATH_GROUP_ERROR        -15     // 文件缓存管理错误
#define PARAM_INVALID           -16     // 参数不匹配错误
#define FUNCTION_EXISTS_ERROR   -17     // 函数名称不存在错误

#endif // CONFIG_H
