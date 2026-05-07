#include "file_manager.h"
#include "cipher_engine.h" // TransCipher API
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QByteArray>
#include <windows.h>

// 内部用固定シークレット（32文字以上のランダム文字列）
const QString PROJECT_SECRET = "FL_Sec_9x7v2B_m4K_L0q9_zX1wP_R3n8_T5j6_vG4h";

FileManager::FileManager(QObject *parent) : QObject(parent) {
    QString exePath = getExecutablePath();
    QDir exeDir(exePath);
    
    outDirPath = exeDir.absoluteFilePath("out");
    configDirPath = exeDir.absoluteFilePath("Config");
    
    QDir().mkpath(outDirPath);
    QDir().mkpath(configDirPath);
}

void FileManager::setTwitchUserId(const QString& userId) {
    m_twitchUserId = userId;
}

QString FileManager::getExecutablePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    QString fullPath = QString::fromLocal8Bit(path);
    return QFileInfo(fullPath).absolutePath();
}

QString FileManager::getDynamicKey() {
    // [プロジェクト固定キー] + [Twitch ID]
    return PROJECT_SECRET + m_twitchUserId;
}

QString FileManager::encodeData(const QString& csvData) {
    // TransCipher を使用して暗号化（動的鍵を使用）
    CipherResult result = CipherEngine::encrypt(csvData.toUtf8(), getDynamicKey());
    if (result.isSuccess()) {
        // 保存用に Base64 形式に変換
        return result.data().toBase64();
    }
    return QString();
}

QString FileManager::decodeData(const QString& encodedData) {
    // Base64 デコードしてバイナリに戻す
    QByteArray cipherData = QByteArray::fromBase64(encodedData.toUtf8());
    
    // TransCipher を使用して復号（動的鍵を使用）
    CipherResult result = CipherEngine::decrypt(cipherData, getDynamicKey());
    if (result.isSuccess()) {
        return QString::fromUtf8(result.data());
    }
    return QString();
}

bool FileManager::writeEncodedFile(const QString& filePath, const QString& csvData) {
    QString encoded = encodeData(csvData);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);
    // Base64結果はASCIIのみなので文字コード指定は不要だが明示的においておく
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << encoded;
    file.close();
    return true;
}

QString FileManager::readEncodedFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QString content = file.readAll();
    file.close();
    return decodeData(content);
}

bool FileManager::loadAllListDat(QList<TwitchFollower>& followers) {
    QString csvData = readEncodedFile(outDirPath + "/AllList.dat");
    if (csvData.isEmpty()) return false;
    
    followers.clear();
    QStringList lines = csvData.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QStringList parts = line.split(',');
        if (parts.size() >= 4) {
            TwitchFollower f;
            f.userName = parts[1];
            f.userLogin = parts[2];
            f.userId = parts[3];
            if (parts.size() >= 5) {
                QStringList gids = parts[4].split('|', Qt::SkipEmptyParts);
                for (const QString& gid : gids) {
                    f.groupIds.append(gid.toInt());
                }
            }
            followers.append(f);
        }
    }
    return true;
}

bool FileManager::loadGroupsListDat(QMap<int, QString>& groups) {
    QString csvData = readEncodedFile(outDirPath + "/GroupsList.dat");
    if (csvData.isEmpty()) return false;
    
    groups.clear();
    QStringList lines = csvData.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QStringList parts = line.split(',');
        if (parts.size() >= 2) {
            groups.insert(parts[0].toInt(), parts[1]);
        }
    }
    return true;
}

bool FileManager::loadDeletedUserDat(QList<TwitchFollower>& deletedUsers) {
    QString csvData = readEncodedFile(outDirPath + "/DeletedUser.dat");
    if (csvData.isEmpty()) return false;
    
    deletedUsers.clear();
    QStringList lines = csvData.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QStringList parts = line.split(',');
        if (parts.size() >= 4) {
            TwitchFollower f;
            f.userName = parts[1];
            f.userLogin = parts[2];
            f.userId = parts[3];
            if (parts.size() >= 5) {
                QStringList gids = parts[4].split('|', Qt::SkipEmptyParts);
                for (const QString& gid : gids) {
                    f.groupIds.append(gid.toInt());
                }
            }
            deletedUsers.append(f);
        }
    }
    return true;
}

bool FileManager::saveDeletedUserDat(const QList<TwitchFollower>& deletedUsers) {
    QString csv;
    for (int i = 0; i < deletedUsers.size(); ++i) {
        const auto& f = deletedUsers[i];
        QStringList gids;
        for (int gid : f.groupIds) gids << QString::number(gid);
        csv += QString("%1,%2,%3,%4,%5\n")
               .arg(i + 1)
               .arg(f.userName)
               .arg(f.userLogin)
               .arg(f.userId)
               .arg(gids.join("|"));
    }
    return writeEncodedFile(outDirPath + "/DeletedUser.dat", csv);
}

bool FileManager::saveAllListDat(const QList<TwitchFollower>& followers) {
    QString csvData;
    QTextStream stream(&csvData);
    stream << "No,表示名,ユーザ名,ユーザID,グループID\n";
    
    int no = 1;
    for (const auto& follower : followers) {
        QStringList groupsStrList;
        for (int gid : follower.groupIds) {
            groupsStrList << QString::number(gid);
        }
        QString groupsCol = groupsStrList.join("|");
        
        stream << no++ << ","
               << follower.userName << ","
               << follower.userLogin << ","
               << follower.userId << ","
               << groupsCol << "\n";
    }
    
    return writeEncodedFile(outDirPath + "/AllList.dat", csvData);
}

bool FileManager::saveGroupsListDat(const QMap<int, QString>& groups) {
    QString csvData;
    QTextStream stream(&csvData);
    stream << "グループID,グループ名\n";
    
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        stream << it.key() << "," << it.value() << "\n";
    }
    
    return writeEncodedFile(outDirPath + "/GroupsList.dat", csvData);
}

bool FileManager::saveGroupListsDat(const QString& groupName, const QList<TwitchFollower>& groupFollowers) {
    QString targetDir = outDirPath + "/" + groupName;
    QDir().mkpath(targetDir);
    
    QString csvData;
    QTextStream stream(&csvData);
    stream << "No,表示名,ユーザ名,ユーザID,グループID\n";
    
    int no = 1;
    for (const auto& follower : groupFollowers) {
        QStringList groupsStrList;
        for (int gid : follower.groupIds) {
            groupsStrList << QString::number(gid);
        }
        QString groupsCol = groupsStrList.join("|");
        
        stream << no++ << ","
               << follower.userName << ","
               << follower.userLogin << ","
               << follower.userId << ","
               << groupsCol << "\n";
    }
    
    return writeEncodedFile(targetDir + "/Lists.dat", csvData);
}

bool FileManager::saveActionHistory(const QList<ActionRecord>& history) {
    QString csvData;
    QTextStream stream(&csvData);
    stream << "Type,UserId,GroupId,GroupName\n";
    
    for (const auto& record : history) {
        stream << record.type << ","
               << record.targetUserId << ","
               << record.targetGroupId << ","
               << record.targetGroupName << "\n";
    }
    
    return writeEncodedFile(configDirPath + "/ActionHistory.dat", csvData);
}
