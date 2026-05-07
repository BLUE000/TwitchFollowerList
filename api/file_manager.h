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

    // 暗号化設定
    void setTwitchUserId(const QString& userId);

    // エンコード・デコード処理
    QString encodeData(const QString& csvData);
    QString decodeData(const QString& encodedData);

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
    QString m_twitchUserId;

    QString getExecutablePath();
    QString getDynamicKey();
    bool writeEncodedFile(const QString& filePath, const QString& csvData);
    QString readEncodedFile(const QString& filePath);
};

#endif // FILE_MANAGER_H
