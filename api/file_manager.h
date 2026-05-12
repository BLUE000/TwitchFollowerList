/**
 * @file file_manager.h
 * @brief ファイルの永続化および暗号化を担当するクラスの定義。
 */

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include "../core/data_models.h"

/**
 * @class FileManager
 * @brief ローカルファイルへのデータ保存、読み込み、および暗号化を管理するクラス。
 */
class FileManager : public QObject {
    Q_OBJECT
public:
    // システム予約グループ ID
    static constexpr int iGRP_ID_ALL        = -1;
    static constexpr int iGRP_ID_UNASSIGNED = -2;
    static constexpr int iGRP_ID_DELETED    = -3;

    /**
     * @brief コンストラクタ。
     * @param pParent 親オブジェクト。
     */
    explicit FileManager(QObject *pParent = nullptr);

    /**
     * @brief 暗号化に使用する Twitch ユーザー ID を設定する。
     * @param szUsrId Twitch の Numeric User ID。
     */
    void setTwitchUserId(const QString& szUsrId);

    /**
     * @brief データを暗号化し Base64 文字列にする。
     * @param szCsvDt 暗号化対象の CSV 文字列。
     * @return 暗号化後の Base64 文字列。
     */
    QString encodeData(const QString& szCsvDt);

    /**
     * @brief 暗号化されたデータを復号する。
     * @param szEncdDt 暗号化された Base64 文字列。
     * @return 復号後の平文文字列。
     */
    QString decodeData(const QString& szEncdDt);

    /**
     * @brief 全フォロワーリストを保存する。
     * @param lstFllwrs 保存対象のフォロワーリスト。
     * @return 成功なら true。
     */
    bool saveAllListDat(const QList<TwitchFollower>& lstFllwrs);

    /**
     * @brief グループ一覧情報を保存する。
     * @param mapGrps 保存対象のグループマップ (ID -> 名前)。
     * @return 成功なら true。
     */
    bool saveGroupsListDat(const QMap<int, QString>& mapGrps);

    /**
     * @brief 特定グループの個別リストを保存する。
     * @param szGrpNm グループ名。
     * @param lstGrpFllwrs グループに所属するフォロワーリスト。
     * @return 成功なら true。
     */
    bool saveGroupListsDat(const QString& szGrpNm, const QList<TwitchFollower>& lstGrpFllwrs);

    /**
     * @brief 操作履歴を保存する。
     * @param lstHstry 保存対象の操作履歴リスト。
     * @return 成功なら true。
     */
    bool saveActionHistory(const QList<ActionRecord>& lstHstry);

    /**
     * @brief 削除済みユーザー（フォロー解除者）リストを保存する。
     * @param lstDltdUsrs 削除済みユーザーリスト。
     * @return 成功なら true。
     */
    bool saveDeletedUserDat(const QList<TwitchFollower>& lstDltdUsrs);

    /**
     * @brief 全フォロワーリストを読み込む。
     * @param lstFllwrs 読み込み先リスト（参照渡し）。
     * @return 成功なら true。
     */
    bool loadAllListDat(QList<TwitchFollower>& lstFllwrs);

    /**
     * @brief グループ一覧情報を読み込む。
     * @param mapGrps 読み込み先マップ（参照渡し）。
     * @return 成功なら true。
     */
    bool loadGroupsListDat(QMap<int, QString>& mapGrps);

    /**
     * @brief 削除済みユーザーリストを読み込む。
     * @param lstDltdUsrs 読み込み先リスト（参照渡し）。
     * @return 成功なら true。
     */
    bool loadDeletedUserDat(QList<TwitchFollower>& lstDltdUsrs);

private:
    QString szOutDirPth;   ///< 出力先ディレクトリパス
    QString szCnfgDirPth;  ///< 設定ファイルディレクトリパス
    QString szTwtchUsrId;  ///< 現在の Twitch ユーザー ID (Numeric)

    /**
     * @brief 実行ファイルのディレクトリパスを取得する。
     * @return 実行ファイルが存在するディレクトリの絶対パス。
     */
    QString getExecutablePath();

    /**
     * @brief 動的な暗号化キーを生成する。
     * @return プロジェクト固定シークレット + Twitch ID。
     */
    QString getDynamicKey();

    /**
     * @brief 暗号化してファイルへ書き込む内部関数。
     * @param szFilePth 保存先ファイルパス。
     * @param szCsvDt 保存する CSV データ。
     * @return 成功なら true。
     */
    bool writeEncodedFile(const QString& szFilePth, const QString& szCsvDt);

    /**
     * @brief ファイルから読み込んで復号する内部関数。
     * @param szFilePth 読み込み先ファイルパス。
     * @return 復号後の平文データ（失敗時は空文字列）。
     */
    QString readEncodedFile(const QString& szFilePth);

    /**
     * @brief カラム内の記号を内部保存用にエンコードする。
     * @param szText 平文。
     * @return エンコード後の文字列。
     */
    QString escapeInternal(const QString& szText);

    /**
     * @brief 内部保存用エンコードを平文に戻す。
     * @param szEncoded エンコード済み文字列。
     * @return 復元後の平文。
     */
    QString unescapeInternal(const QString& szEncoded);

    // CSV フォーマット規定（フォロワー情報）
    static constexpr int iCOL_FLW_MIN      = 4; ///< フォロワー情報の必須列数 (No, 表示名, ユーザ名, ID)
    static constexpr int iCOL_FLW_FULL     = 9; ///< フォロワー情報の最大列数 (グループ, 最新フォロー, 履歴x2, メモ)
    static constexpr int iIDX_FLW_NAME     = 1; ///< 列インデックス: 表示名
    static constexpr int iIDX_FLW_LOGIN    = 2; ///< 列インデックス: ユーザ名
    static constexpr int iIDX_FLW_ID       = 3; ///< 列インデックス: ユーザID
    static constexpr int iIDX_FLW_GRP_IDS  = 4; ///< 列インデックス: グループID
    static constexpr int iIDX_FLW_FOLLOWED_AT = 5; ///< 列インデックス: 最新フォロー日時
    static constexpr int iIDX_FLW_FOLLOW_HSTRY = 6; ///< 列インデックス: フォロー履歴 (セミコロン区切り)
    static constexpr int iIDX_FLW_UNFOLLOW_HSTRY = 7; ///< 列インデックス: 解除履歴 (セミコロン区切り)
    static constexpr int iIDX_FLW_MEMO     = 8; ///< 列インデックス: メモ (v2.0)

    // CSV フォーマット規定（グループ定義）
    static constexpr int iCOL_GRP_MIN      = 2; ///< グループ情報の必須列数 (ID, 名前)
    static constexpr int iIDX_GRP_ID       = 0; ///< 列インデックス: グループID
    static constexpr int iIDX_GRP_NAME     = 1; ///< 列インデックス: グループ名


    // CSV 見出し文字列
    static constexpr const char* szHDR_FLW = "No,表示名,ユーザ名,ユーザID,グループID,最新フォロー日時,フォロー履歴,解除履歴,メモ\n";
    static constexpr const char* szHDR_GRP = "グループID,グループ名\n";

    // ファイル名規定
    static constexpr const char* szFN_ALL_LST   = "AllList.dat";
    static constexpr const char* szFN_GRP_LST   = "GroupsList.dat";
    static constexpr const char* szFN_DLTD_USR  = "DeletedUser.dat";
    static constexpr const char* szFN_ACTN_HSTRY = "ActionHistory.dat";
    static constexpr const char* szFN_LSTS      = "Lists.dat"; // 各グループ配下のファイル

    /**
     * @brief クラス固有のログメッセージを ID 指定で出力する。
     * @param id メッセージ ID。
     */
    void log(int id);

    /// @brief ログメッセージ ID の定義
    enum LogId {
        INF_FILE_SAVE   = 101,
        INF_FILE_LOAD   = 102,
        INF_CNFG_SAVE   = 103,
        ERR_FILE_OPEN   = 301,
        ERR_DIR_CREATE  = 302,
        ERR_DECODE_FAIL = 303
    };

    QMap<int, QString> m_infoTable;  ///< INFO 用メッセージテーブル
    QMap<int, QString> m_warnTable;  ///< WARN 用メッセージテーブル
    QMap<int, QString> m_errorTable; ///< ERROR 用メッセージテーブル
};

#endif // FILE_MANAGER_H
