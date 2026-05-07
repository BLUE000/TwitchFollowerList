/**
 * @file file_manager.cpp
 * @brief FileManager クラスの実装。
 */

#include "file_manager.h"
#include "cipher_engine.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QByteArray>
#include <QFileInfo>
#include <windows.h>

// 内部用固定シークレット（32文字以上のランダム文字列）
const QString szPRJCT_SCRT = "FL_Sec_9x7v2B_m4K_L0q9_zX1wP_R3n8_T5j6_vG4h";

/**
 * @brief コンストラクタ。実行環境からパスを特定し、必要なディレクトリを作成する。
 * @param pParent 親オブジェクト。
 */
FileManager::FileManager(QObject *pParent) : QObject(pParent) {
    QString szExePth = getExecutablePath();
    QDir oExeDir(szExePth);
    
    szOutDirPth = oExeDir.absoluteFilePath("out");
    szCnfgDirPth = oExeDir.absoluteFilePath("Config");
    
    QDir().mkpath(szOutDirPth);
    QDir().mkpath(szCnfgDirPth);
}

/**
 * @brief Twitch ユーザー ID を保持する。
 * @param szUsrId ユーザー ID。
 */
void FileManager::setTwitchUserId(const QString& szUsrId) {
    szTwtchUsrId = szUsrId;
}

/**
 * @brief Windows API を使用して実行ファイルのパスを取得する。
 * @return 実行ファイルが存在するディレクトリのパス。
 */
QString FileManager::getExecutablePath() {
    char szPth[MAX_PATH];
    GetModuleFileNameA(NULL, szPth, MAX_PATH);
    QString szFullPth = QString::fromLocal8Bit(szPth);
    return QFileInfo(szFullPth).absolutePath();
}

/**
 * @brief プロジェクトシークレットとユーザー ID を結合して鍵を生成する。
 * @return 生成された暗号化キー。
 */
QString FileManager::getDynamicKey() {
    return szPRJCT_SCRT + szTwtchUsrId;
}

/**
 * @brief データを暗号化する。
 * @param szCsvDt 元の CSV データ。
 * @return 暗号化後の文字列（Base64）。
 */
QString FileManager::encodeData(const QString& szCsvDt) {
    CipherResult oRes = CipherEngine::encrypt(szCsvDt.toUtf8(), getDynamicKey());
    if (oRes.isSuccess()) {
        return oRes.data().toBase64();
    } else {
        // 失敗時は空を返す
        return QString();
    }
}

/**
 * @brief 暗号化されたデータを復号する。
 * @param szEncdDt 暗号化されたデータ（Base64）。
 * @return 復号された CSV データ。
 */
QString FileManager::decodeData(const QString& szEncdDt) {
    QByteArray oCphrDt = QByteArray::fromBase64(szEncdDt.toUtf8());
    CipherResult oRes = CipherEngine::decrypt(oCphrDt, getDynamicKey());
    if (oRes.isSuccess()) {
        return QString::fromUtf8(oRes.data());
    } else {
        return QString();
    }
}

/**
 * @brief エンコードしてファイルへ書き込む。
 * @param szFilePth ファイルパス。
 * @param szCsvDt 保存データ。
 * @return 成功なら true。
 */
bool FileManager::writeEncodedFile(const QString& szFilePth, const QString& szCsvDt) {
    QString szEncd = encodeData(szCsvDt);
    QFile oFil(szFilePth);
    if (!oFil.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    } else {
        QTextStream oStrm(&oFil);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        oStrm.setCodec("UTF-8");
#endif
        oStrm << szEncd;
        oFil.close();
        return true;
    }
}

/**
 * @brief ファイルから読み込んで復号する。
 * @param szFilePth ファイルパス。
 * @return 復号されたデータ。
 */
QString FileManager::readEncodedFile(const QString& szFilePth) {
    QFile oFil(szFilePth);
    if (!oFil.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    } else {
        QString szCntnt = oFil.readAll();
        oFil.close();
        return decodeData(szCntnt);
    }
}

/**
 * @brief 全フォロワーリストをロードする。
 * @param lstFllwrs 格納先。
 * @return 成功なら true。
 */
bool FileManager::loadAllListDat(QList<TwitchFollower>& lstFllwrs) {
    QString szCsvDt = readEncodedFile(szOutDirPth + "/AllList.dat");
    if (szCsvDt.isEmpty()) {
        return false;
    } else {
        lstFllwrs.clear();
        QStringList lstLns = szCsvDt.split('\n', Qt::SkipEmptyParts);
        for (int i = 1; i < lstLns.size(); ++i) {
            const QString& szLn = lstLns[i];
            QStringList lstPrts = szLn.split(',');
            if (lstPrts.size() >= iCOL_FLW_MIN) {
                TwitchFollower oFllwr;
                oFllwr.userName = lstPrts[iIDX_FLW_NAME];
                oFllwr.userLogin = lstPrts[iIDX_FLW_LOGIN];
                oFllwr.userId = lstPrts[iIDX_FLW_ID];
                if (lstPrts.size() >= iCOL_FLW_FULL) {
                    QStringList lstGids = lstPrts[iIDX_FLW_GRP_IDS].split('|', Qt::SkipEmptyParts);
                    for (const QString& szGid : lstGids) {
                        oFllwr.groupIds.append(szGid.toInt());
                    }
                } else {
                    // 追加属性なし
                }
                lstFllwrs.append(oFllwr);
            } else {
                // 不正な行
            }
        }
        return true;
    }
}

/**
 * @brief グループ一覧をロードする。
 * @param mapGrps 格納先。
 * @return 成功なら true。
 */
bool FileManager::loadGroupsListDat(QMap<int, QString>& mapGrps) {
    QString szCsvDt = readEncodedFile(szOutDirPth + "/GroupsList.dat");
    if (szCsvDt.isEmpty()) {
        return false;
    } else {
        mapGrps.clear();
        QStringList lstLns = szCsvDt.split('\n', Qt::SkipEmptyParts);
        for (int i = 1; i < lstLns.size(); ++i) {
            const QString& szLn = lstLns[i];
            QStringList lstPrts = szLn.split(',');
            if (lstPrts.size() >= iCOL_GRP_MIN) {
                mapGrps.insert(lstPrts[iIDX_GRP_ID].toInt(), lstPrts[iIDX_GRP_NAME]);
            } else {
                // 不正な行
            }
        }
        return true;
    }
}

/**
 * @brief 削除済みユーザー（フォロー解除者）リストをロードする。
 * @param lstDltdUsrs 格納先。
 * @return 成功なら true。
 */
bool FileManager::loadDeletedUserDat(QList<TwitchFollower>& lstDltdUsrs) {
    QString szCsvDt = readEncodedFile(szOutDirPth + "/DeletedUser.dat");
    if (szCsvDt.isEmpty()) {
        return false;
    } else {
        lstDltdUsrs.clear();
        QStringList lstLns = szCsvDt.split('\n', Qt::SkipEmptyParts);
        for (int i = 1; i < lstLns.size(); ++i) {
            const QString& szLn = lstLns[i];
            QStringList lstPrts = szLn.split(',');
            if (lstPrts.size() >= iCOL_FLW_MIN) {
                TwitchFollower oFllwr;
                oFllwr.userName = lstPrts[iIDX_FLW_NAME];
                oFllwr.userLogin = lstPrts[iIDX_FLW_LOGIN];
                oFllwr.userId = lstPrts[iIDX_FLW_ID];
                if (lstPrts.size() >= iCOL_FLW_FULL) {
                    QStringList lstGids = lstPrts[iIDX_FLW_GRP_IDS].split('|', Qt::SkipEmptyParts);
                    for (const QString& szGid : lstGids) {
                        oFllwr.groupIds.append(szGid.toInt());
                    }
                } else {
                    // 追加属性なし
                }
                lstDltdUsrs.append(oFllwr);
            } else {
                // 不正な行
            }
        }
        return true;
    }
}

/**
 * @brief 削除済みユーザーリストを保存する。
 * @param lstDltdUsrs 保存対象。
 * @return 成功なら true。
 */
bool FileManager::saveDeletedUserDat(const QList<TwitchFollower>& lstDltdUsrs) {
    QString szCsv = szHDR_FLW;
    for (int i = 0; i < lstDltdUsrs.size(); ++i) {
        const auto& oFllwr = lstDltdUsrs[i];
        QStringList lstGids;
        for (int iGid : oFllwr.groupIds) {
            lstGids << QString::number(iGid);
        }
        szCsv += QString("%1,%2,%3,%4,%5\n")
                 .arg(i + 1)
                 .arg(oFllwr.userName)
                 .arg(oFllwr.userLogin)
                 .arg(oFllwr.userId)
                 .arg(lstGids.join("|"));
    }
    return writeEncodedFile(szOutDirPth + "/DeletedUser.dat", szCsv);
}

/**
 * @brief 全フォロワーリストを保存する。
 * @param lstFllwrs 保存対象。
 * @return 成功なら true。
 */
bool FileManager::saveAllListDat(const QList<TwitchFollower>& lstFllwrs) {
    QString szCsvDt = szHDR_FLW;
    
    int iNo = 1;
    for (const auto& oFllwr : lstFllwrs) {
        QStringList lstGidsStr;
        for (int iGid : oFllwr.groupIds) {
            lstGidsStr << QString::number(iGid);
        }
        QString szGidsCol = lstGidsStr.join("|");
        
        szCsvDt += QString("%1,%2,%3,%4,%5\n")
                   .arg(iNo++)
                   .arg(oFllwr.userName)
                   .arg(oFllwr.userLogin)
                   .arg(oFllwr.userId)
                   .arg(szGidsCol);
    }
    
    return writeEncodedFile(szOutDirPth + "/AllList.dat", szCsvDt);
}

/**
 * @brief グループ一覧を保存する。
 * @param mapGrps 保存対象。
 * @return 成功なら true。
 */
bool FileManager::saveGroupsListDat(const QMap<int, QString>& mapGrps) {
    QString szCsvDt = szHDR_GRP;
    
    for (auto it = mapGrps.constBegin(); it != mapGrps.constEnd(); ++it) {
        szCsvDt += QString("%1,%2\n").arg(it.key()).arg(it.value());
    }
    
    return writeEncodedFile(szOutDirPth + "/GroupsList.dat", szCsvDt);
}

/**
 * @brief 各グループの個別フォロワーリストを保存する。
 * @param szGrpNm グループ名。
 * @param lstGrpFllwrs 対象フォロワーリスト。
 * @return 成功なら true。
 */
bool FileManager::saveGroupListsDat(const QString& szGrpNm, const QList<TwitchFollower>& lstGrpFllwrs) {
    QString szTgtDir = szOutDirPth + "/" + szGrpNm;
    QDir().mkpath(szTgtDir);
    
    QString szCsvDt = szHDR_FLW;
    
    int iNo = 1;
    for (const auto& oFllwr : lstGrpFllwrs) {
        QStringList lstGidsStr;
        for (int iGid : oFllwr.groupIds) {
            lstGidsStr << QString::number(iGid);
        }
        QString szGidsCol = lstGidsStr.join("|");
        
        szCsvDt += QString("%1,%2,%3,%4,%5\n")
                   .arg(iNo++)
                   .arg(oFllwr.userName)
                   .arg(oFllwr.userLogin)
                   .arg(oFllwr.userId)
                   .arg(szGidsCol);
    }
    
    return writeEncodedFile(szTgtDir + "/Lists.dat", szCsvDt);
}

/**
 * @brief 操作履歴を保存する。
 * @param lstHstry 保存対象。
 * @return 成功なら true。
 */
bool FileManager::saveActionHistory(const QList<ActionRecord>& lstHstry) {
    QString szCsvDt;
    szCsvDt += "Type,UserId,GroupId,GroupName\n";
    
    for (const auto& oRec : lstHstry) {
        szCsvDt += QString("%1,%2,%3,%4\n")
                   .arg(static_cast<int>(oRec.type))
                   .arg(oRec.targetUserId)
                   .arg(oRec.targetGroupId)
                   .arg(oRec.targetGroupName);
    }
    
    return writeEncodedFile(szCnfgDirPth + "/ActionHistory.dat", szCsvDt);
}
