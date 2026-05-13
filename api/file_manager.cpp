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
#include "logger.h"

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

    // ログテーブルの初期化
    m_infoTable[INF_FILE_SAVE]   = "ファイルの保存に成功しました。";
    m_infoTable[INF_FILE_LOAD]   = "ファイルの読み込みに成功しました。";
    m_infoTable[INF_CNFG_SAVE]   = "設定ファイルを保存しました。";

    m_errorTable[ERR_FILE_OPEN]   = "ファイルを開くことができませんでした。";
    m_errorTable[ERR_DIR_CREATE]  = "ディレクトリの作成に失敗しました。";
    m_errorTable[ERR_DECODE_FAIL] = "データの復号に失敗しました。";

    // 保存されているユーザー ID をロード
    QFile oFile(szCnfgDirPth + "/config.ini");
    if (oFile.open(QIODevice::ReadOnly)) {
        szTwtchUsrId = oFile.readAll().trimmed();
        oFile.close();
    }
}

/**
 * @brief Twitch ユーザー ID を保持する。
 * @param szUsrId ユーザー ID。
 */
void FileManager::setTwitchUserId(const QString& szUsrId) {
    szTwtchUsrId = szUsrId;
    
    // ID を設定ファイルに保存
    QFile oFile(szCnfgDirPth + "/config.ini");
    if (oFile.open(QIODevice::WriteOnly)) {
        oFile.write(szUsrId.toUtf8());
        oFile.close();
        log(INF_CNFG_SAVE);
    }
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
        log(ERR_DECODE_FAIL);
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
        log(ERR_DECODE_FAIL);
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
        log(ERR_FILE_OPEN);
        return false;
    } else {
        QTextStream oStrm(&oFil);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        oStrm.setCodec("UTF-8");
#endif
        oStrm << szEncd;
        oFil.close();
        log(INF_FILE_SAVE);
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
 * @brief カラム内の記号を内部保存用にエンコードする。
 */
QString FileManager::escapeInternal(const QString& szText) {
    QString szRes = szText;
    szRes.replace(",", "&comma;");
    szRes.replace("\"", "&quot;");
    szRes.replace("\n", "&nl;");
    szRes.replace("\r", ""); // Windows 改行対応（CR削除）
    return szRes;
}

/**
 * @brief 内部保存用エンコードを平文に戻す。
 */
QString FileManager::unescapeInternal(const QString& szEncoded) {
    QString szRes = szEncoded;
    szRes.replace("&comma;", ",");
    szRes.replace("&quot;", "\"");
    szRes.replace("&nl;", "\n");
    return szRes;
}

/**
 * @brief 全フォロワーリストをロードする。
 * @param lstFllwrs 格納先。
 * @return 成功なら true。
 */
bool FileManager::loadAllListDat(QList<TwitchFollower>& lstFllwrs) {
    QString szCsvDt = readEncodedFile(szOutDirPth + "/" + szFN_ALL_LST);
    if (szCsvDt.isEmpty()) {
        return false;
    } else {
        lstFllwrs.clear();
        QStringList lstLns = szCsvDt.split('\n', Qt::SkipEmptyParts);
        if (lstLns.isEmpty()) return false;

        // ヘッダー行を解析してカラムインデックスを特定
        QStringList lstHdr = lstLns[0].split(',');
        QMap<QString, int> mapHdr;
        for (int i = 0; i < lstHdr.size(); ++i) {
            mapHdr[lstHdr[i].trimmed()] = i;
        }

        // 必須カラムの存在確認 (IDがあれば最低限読み込み可能とする)
        if (!mapHdr.contains("ユーザID")) return false;

        for (int i = 1; i < lstLns.size(); ++i) {
            const QString& szLn = lstLns[i];
            QStringList lstPrts = szLn.split(',');
            
            TwitchFollower oFllwr;
            // ヘッダーマップに基づいてデータを取得
            if (mapHdr.contains("ユーザID")) oFllwr.userId = lstPrts.value(mapHdr["ユーザID"]);
            if (mapHdr.contains("ニックネーム")) oFllwr.nickname = unescapeInternal(lstPrts.value(mapHdr["ニックネーム"]));
            if (mapHdr.contains("表示名")) oFllwr.userName = lstPrts.value(mapHdr["表示名"]);
            if (mapHdr.contains("ユーザ名")) oFllwr.userLogin = lstPrts.value(mapHdr["ユーザ名"]);
            
            // グループID (複数保持)
            if (mapHdr.contains("グループID")) {
                QStringList lstGids = lstPrts.value(mapHdr["グループID"]).split('|', Qt::SkipEmptyParts);
                for (const QString& szGid : lstGids) {
                    oFllwr.groupIds.append(szGid.toInt());
                }
            }

            // 最新フォロー日時
            if (mapHdr.contains("最新フォロー日時")) {
                oFllwr.followedAt = QDateTime::fromString(lstPrts.value(mapHdr["最新フォロー日時"]), Qt::ISODate);
            }

            // フォロー履歴 (セミコロン区切り)
            if (mapHdr.contains("フォロー履歴")) {
                QStringList lstHist = lstPrts.value(mapHdr["フォロー履歴"]).split(';', Qt::SkipEmptyParts);
                for (const QString& szDt : lstHist) {
                    oFllwr.followHistory.append(QDateTime::fromString(szDt, Qt::ISODate));
                }
            }

            // フォロー削除履歴 (セミコロン区切り)
            if (mapHdr.contains("フォロー削除履歴")) {
                QStringList lstHist = lstPrts.value(mapHdr["フォロー削除履歴"]).split(';', Qt::SkipEmptyParts);
                for (const QString& szDt : lstHist) {
                    oFllwr.unfollowHistory.append(QDateTime::fromString(szDt, Qt::ISODate));
                }
            }

            // メモ (v2.0)
            if (mapHdr.contains("メモ")) {
                oFllwr.memo = unescapeInternal(lstPrts.value(mapHdr["メモ"]));
            }

            lstFllwrs.append(oFllwr);
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
    QString szCsvDt = readEncodedFile(szOutDirPth + "/" + szFN_GRP_LST);
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
    QString szCsvDt = readEncodedFile(szOutDirPth + "/" + szFN_DLTD_USR);
    if (szCsvDt.isEmpty()) {
        return false;
    } else {
        lstDltdUsrs.clear();
        QStringList lstLns = szCsvDt.split('\n', Qt::SkipEmptyParts);
        if (lstLns.isEmpty()) return false;

        // ヘッダー解析
        QStringList lstHdr = lstLns[0].split(',');
        QMap<QString, int> mapHdr;
        for (int i = 0; i < lstHdr.size(); ++i) mapHdr[lstHdr[i].trimmed()] = i;

        if (!mapHdr.contains("ユーザID")) return false;

        for (int i = 1; i < lstLns.size(); ++i) {
            const QString& szLn = lstLns[i];
            QStringList lstPrts = szLn.split(',');
            
            TwitchFollower oFllwr;
            if (mapHdr.contains("ユーザID")) oFllwr.userId = lstPrts.value(mapHdr["ユーザID"]);
            if (mapHdr.contains("ニックネーム")) oFllwr.nickname = unescapeInternal(lstPrts.value(mapHdr["ニックネーム"]));
            if (mapHdr.contains("表示名")) oFllwr.userName = lstPrts.value(mapHdr["表示名"]);
            if (mapHdr.contains("ユーザ名")) oFllwr.userLogin = lstPrts.value(mapHdr["ユーザ名"]);
            
            if (mapHdr.contains("グループID")) {
                QStringList lstGids = lstPrts.value(mapHdr["グループID"]).split('|', Qt::SkipEmptyParts);
                for (const QString& szGid : lstGids) oFllwr.groupIds.append(szGid.toInt());
            }

            if (mapHdr.contains("最新フォロー日時")) {
                oFllwr.followedAt = QDateTime::fromString(lstPrts.value(mapHdr["最新フォロー日時"]), Qt::ISODate);
            }

            if (mapHdr.contains("フォロー履歴")) {
                QStringList lstHist = lstPrts.value(mapHdr["フォロー履歴"]).split(';', Qt::SkipEmptyParts);
                for (const QString& szDt : lstHist) oFllwr.followHistory.append(QDateTime::fromString(szDt, Qt::ISODate));
            }

            // フォロー削除履歴 (セミコロン区切り)
            if (mapHdr.contains("フォロー削除履歴")) {
                QStringList lstHist = lstPrts.value(mapHdr["フォロー削除履歴"]).split(';', Qt::SkipEmptyParts);
                for (const QString& szDt : lstHist) oFllwr.unfollowHistory.append(QDateTime::fromString(szDt, Qt::ISODate));
            }

            // メモ (v2.0)
            if (mapHdr.contains("メモ")) {
                oFllwr.memo = unescapeInternal(lstPrts.value(mapHdr["メモ"]));
            }

            lstDltdUsrs.append(oFllwr);
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

        QStringList lstFlwHist;
        for (const auto& dt : oFllwr.followHistory) lstFlwHist << dt.toString(Qt::ISODate);
        
        QStringList lstUnflwHist;
        for (const auto& dt : oFllwr.unfollowHistory) lstUnflwHist << dt.toString(Qt::ISODate);

        szCsv += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10\n")
                 .arg(i + 1)
                 .arg(escapeInternal(oFllwr.nickname))
                 .arg(oFllwr.userName)
                 .arg(oFllwr.userLogin)
                 .arg(oFllwr.userId)
                 .arg(lstGids.join("|"))
                 .arg(oFllwr.followedAt.toString(Qt::ISODate))
                 .arg(lstFlwHist.join(";"))
                 .arg(lstUnflwHist.join(";"))
                 .arg(escapeInternal(oFllwr.memo));
    }
    return writeEncodedFile(szOutDirPth + "/" + szFN_DLTD_USR, szCsv);
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

        QStringList lstFlwHist;
        for (const auto& dt : oFllwr.followHistory) lstFlwHist << dt.toString(Qt::ISODate);
        
        QStringList lstUnflwHist;
        for (const auto& dt : oFllwr.unfollowHistory) lstUnflwHist << dt.toString(Qt::ISODate);
        
        szCsvDt += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10\n")
                   .arg(iNo++)
                   .arg(escapeInternal(oFllwr.nickname))
                   .arg(oFllwr.userName)
                   .arg(oFllwr.userLogin)
                   .arg(oFllwr.userId)
                   .arg(lstGidsStr.join("|"))
                   .arg(oFllwr.followedAt.toString(Qt::ISODate))
                   .arg(lstFlwHist.join(";"))
                   .arg(lstUnflwHist.join(";"))
                   .arg(escapeInternal(oFllwr.memo));
    }
    
    return writeEncodedFile(szOutDirPth + "/" + szFN_ALL_LST, szCsvDt);
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
    
    return writeEncodedFile(szOutDirPth + "/" + szFN_GRP_LST, szCsvDt);
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

        QStringList lstFlwHist;
        for (const auto& dt : oFllwr.followHistory) lstFlwHist << dt.toString(Qt::ISODate);
        
        QStringList lstUnflwHist;
        for (const auto& dt : oFllwr.unfollowHistory) lstUnflwHist << dt.toString(Qt::ISODate);
        
        szCsvDt += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10\n")
                   .arg(iNo++)
                   .arg(escapeInternal(oFllwr.nickname))
                   .arg(oFllwr.userName)
                   .arg(oFllwr.userLogin)
                   .arg(oFllwr.userId)
                   .arg(lstGidsStr.join("|"))
                   .arg(oFllwr.followedAt.toString(Qt::ISODate))
                   .arg(lstFlwHist.join(";"))
                   .arg(lstUnflwHist.join(";"))
                   .arg(escapeInternal(oFllwr.memo));
    }
    
    return writeEncodedFile(szTgtDir + "/" + szFN_LSTS, szCsvDt);
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
    
    return writeEncodedFile(szCnfgDirPth + "/" + szFN_ACTN_HSTRY, szCsvDt);
}
void FileManager::log(int id) {
    if (m_errorTable.contains(id)) {
        Logger::output("ERROR", m_errorTable[id]);
    } else if (m_warnTable.contains(id)) {
        Logger::output("WARN", m_warnTable[id]);
    } else if (m_infoTable.contains(id)) {
        Logger::output("INFO", m_infoTable[id]);
    }
}
