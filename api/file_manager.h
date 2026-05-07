#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include "../core/data_models.h"

class FileManager : public QObject {
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = nullptr);

    // エンコード・デコード処理
    static QString encodeData(const QString& csvData);
    static QString decodeData(const QString& encodedData);

    // ファイル保存処理
    bool saveAllListDat(const QList<TwitchFollower>& followers);
    bool saveGroupsListDat(const QMap<int, QString>& groups);
    bool saveGroupListsDat(const QString& groupName, const QList<TwitchFollower>& groupFollowers);
    bool saveActionHistory(const QList<ActionRecord>& history);
    bool saveDeletedUserDat(const QList<TwitchFollower>& deletedUsers);

    // ファイルロード処理
    bool loadAllListDat(QList<TwitchFollower>& followers);
    bool loadGroupsListDat(QMap<int, QString>& groups);
    bool loadDeletedUserDat(QList<TwitchFollower>& deletedUsers);

private:
    QString outDirPath;
    QString configDirPath;

    bool writeEncodedFile(const QString& filePath, const QString& csvData);
    QString readEncodedFile(const QString& filePath);
};

#endif // FILE_MANAGER_H
