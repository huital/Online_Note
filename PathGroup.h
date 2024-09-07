#ifndef PATHGROUP_H
#define PATHGROUP_H

#include <QStringList>
#include <QDateTime>
#include <QHash>
#include <QFile>
#include <QDebug>

class PathGroup {
public:
    PathGroup(int maxSize) : maxSize(maxSize) {}

    void addPath(const QString& remotePath, const QString& absolutePath) {
        // Check if the path already exists, if yes, remove it first
        paths.removeAll(remotePath);

        // Add the new path to the front of the list
        paths.prepend(remotePath);

        // Update the operation time for the added path
        operationTimes[remotePath] = QDateTime::currentDateTime();

        // Update remote_to_absolute mapping
        remote_to_absolute[remotePath] = absolutePath;

        // If the group size exceeds the maximum size, remove the least recently used path
        if (paths.size() > maxSize) {
            QString lruPath = findLRUPath();
            paths.removeOne(lruPath);
            operationTimes.remove(lruPath);
            remote_to_absolute.remove(lruPath);
        }
    }

    void removePath(const QString& remotePath) {
        // Remove the specified path from the group
        QString absolute_path = remote_to_absolute[remotePath];
        paths.removeAll(remotePath);
        operationTimes.remove(remotePath);
        remote_to_absolute.remove(remotePath);
        QFile file(absolute_path);
        if (file.exists()) {
            file.remove();
        }
    }

    void updateRemotePath(const QString &old_path, const QString &new_path) {
        // 更新paths中的路径
        for (int i = 0; i < paths.size(); ++i) {
            if (paths[i].startsWith(old_path)) {
                paths[i].replace(0, old_path.length(), new_path);
            }
        }

        // 更新operationTimes中的键值对
        QStringList oldKeys = operationTimes.keys();
        for (const QString &key : oldKeys) {
            if (key.startsWith(old_path)) {
                QDateTime operationTime = operationTimes.take(key);
                QString newKey = key;
                newKey.replace(0, old_path.length(), new_path);
                operationTimes.insert(newKey, operationTime);
            }
        }

        // 更新remote_to_absolute中的键值对
        QStringList oldRemotePaths = remote_to_absolute.keys();
        for (const QString &remotePath : oldRemotePaths) {
            if (remotePath.startsWith(old_path)) {
                QString absolutePath = remote_to_absolute.take(remotePath);
                QString newRemotePath = remotePath;
                newRemotePath.replace(0, old_path.length(), new_path);
                remote_to_absolute.insert(newRemotePath, absolutePath);
            }
        }

        // 删除旧的键值对
        operationTimes.remove(old_path);
        remote_to_absolute.remove(old_path);
    }

    void printInfo() const {
       qDebug() << "PathGroup Info:";
       for (const QString& remotePath : paths) {
           qDebug().noquote() << "Remote Path:" << remotePath << ", Absolute Path:" << remote_to_absolute.value(remotePath) << ", Operation Time:" << operationTimes.value(remotePath).toString();
       }
   }

    void updateOperationTime(const QString& remotePath) {
        // Update the operation time for the specified path
        operationTimes[remotePath] = QDateTime::currentDateTime();
    }

    void clear() {
        // Clear all paths and operation times
        paths.clear();
        operationTimes.clear();
        remote_to_absolute.clear();
    }

    QStringList getPaths() const {
        return paths;
    }

    QString getAbsolutePath(const QString& remotePath) const {
        return remote_to_absolute.value(remotePath);
    }

private:
    int maxSize;
    QStringList paths;
    QHash<QString, QDateTime> operationTimes; // Store the operation time for each path
    QHash<QString, QString> remote_to_absolute; // Mapping from remote path to absolute path

    QString findLRUPath() {
        // Find the path with the least recent operation time
        QDateTime lruTime = QDateTime::currentDateTime();
        QString lruPath;
        for (const QString& remotePath : paths) {
            if (operationTimes.value(remotePath) < lruTime) {
                lruTime = operationTimes.value(remotePath);
                lruPath = remotePath;
            }
        }
        return lruPath;
    }

};


#endif // PATHGROUP_H
