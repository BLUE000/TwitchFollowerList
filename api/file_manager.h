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
};

#endif // FILE_MANAGER_H
