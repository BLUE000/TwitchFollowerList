/**
 * @file app_controller.h
 * @brief アプリケーション全体の制御ロジック（Controller）の定義。
 */

#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include "../ui/mainwindow.h"
#include "../api/twitch_authenticator.h"
#include "../api/twitch_api_client.h"
#include "../api/file_manager.h"
#include "data_models.h"

class QTimer;

/**
 * @class AppController
 * @brief UI（MainWindow）と各 API サービスを仲介し、ビジネスロジックを制御するクラス。
 */
class AppController : public QObject {
    Q_OBJECT
public:
    static const int LOGIN_TIMEOUT_MS = 30000; ///< ログインタイムアウト（30秒）
    static const int SMART_REFRESH_WAIT_MS = 500; ///< 更新演出用のウェイト（0.5秒）
    /**
     * @brief コンストラクタ。
     * @param pMainWindow メインウィンドウへのポインタ。
     * @param pParent 親オブジェクト。
     */
    explicit AppController(MainWindow *pMainWindow, QObject *pParent = nullptr);

    /**
     * @brief コントローラーを初期化し、シグナル・スロットを接続する。
     */
    void initialize();

public slots:
    /**
     * @brief ログイン要求を処理する。
     */
    void handleLoginRequest();

    /**
     * @brief 認証完了時の処理。
     * @param szTkn 取得したアクセストークン。
     */
    void handleAuthCompleted(const QString& szTkn);

    /**
     * @brief ログインユーザー情報の取得完了時の処理。
     * @param szUsrId Twitch の Numeric User ID。
     */
    void handleCurrentUserFetched(const QString& szUsrId);

    /**
     * @brief フォロワーリスト取得完了時の処理。
     * @param lstFllwrs 取得されたフォロワーリスト。
     */
    void handleFollowersFetched(const QList<TwitchFollower>& lstFllwrs);
    
    /**
     * @brief フォロワーのグループ割り当て処理。
     * @param szUsrId ユーザー ID。
     * @param iGrpId グループ ID。
     */
    void handleFollowerAssigned(const QString& szUsrId, int iGrpId);

    /**
     * @brief フォロワーのグループ解除処理。
     * @param szUsrId ユーザー ID。
     * @param iGrpId グループ ID。
     */
    void handleFollowerUnassigned(const QString& szUsrId, int iGrpId);

    /**
     * @brief 新規グループ作成処理。
     * @param szGrpNm グループ名。
     */
    void handleGroupCreated(const QString& szGrpNm);

    /**
     * @brief グループ削除処理。
     * @param iGrpId 削除対象のグループ ID。
     */
    void handleGroupDeleted(int iGrpId);

    /**
     * @brief Undo（元に戻す）処理。
     */
    void handleUndoRequested();

    /**
     * @brief Redo（やり直し）処理。
     */
    void handleRedoRequested();

    /**
     * @brief 通信タイムアウト時の処理。
     */
    void handleTimeout();

    /**
     * @brief グループ選択変更時の処理。
     * @param iGrpId 選択されたグループ ID。
     */
    void handleGroupSelected(int iGrpId);

    /**
     * @brief ファイル出力要求時の処理。
     */
    void handleOutputRequested();

private:
    MainWindow *pView;                    ///< メインウィンドウへのポインタ
    TwitchAuthenticator *pAuthntctr;      ///< 認証ハンドラ
    TwitchApiClient *pApiClient;          ///< Twitch API クライアント
    FileManager *pFilMngr;                ///< ファイル管理
    QTimer *pTmoutTmr;                    ///< タイムアウト監視タイマー

    bool bIsBsy;                          ///< 処理中フラグ
    QList<TwitchFollower> lstCrntFllwrs;  ///< 現在のフォロワーリスト
    QList<TwitchFollower> lstCrntDltdUsrs; ///< 削除済み（フォロー解除）リスト
    QMap<int, QString> mapCrntGrps;       ///< 現在のグループマップ
    QList<ActionRecord> lstActnHstry;     ///< 操作履歴
    int iHstryCrsr;                       ///< 履歴カーソル位置
    int iNxtGrpId;                        ///< 次に発行するグループ ID
    int iSlctdGrpId;                      ///< 現在 UI で選択されているグループ ID

    /**
     * @brief アクションを履歴に追加し、適用する。
     * @param oActn 記録するアクションレコード。
     */
    void pushAction(const ActionRecord& oActn);

    /**
     * @brief アクションを現在の状態に適用する。
     * @param oActn 適用するアクション。
     */
    void applyAction(const ActionRecord& oActn);

    /**
     * @brief アクションを逆転させて状態を戻す。
     * @param oActn 逆転させるアクション。
     */
    void revertAction(const ActionRecord& oActn);
    
    /**
     * @brief UI の表示内容を現在のデータで更新する。
     */
    void updateView();

    /**
     * @brief 操作履歴をファイルへ保存する。
     */
    void saveHistory();

    /**
     * @brief クラス固有のログメッセージを ID 指定で出力する。
     * @param id メッセージ ID。
     */
    void log(int id);

    /// @brief ログメッセージ ID の定義
    enum LogId {
        INF_APP_INIT    = 101,
        INF_LOGIN_START = 102,
        INF_AUTH_OK     = 103,
        INF_USR_FETCH_OK = 104,
        INF_FLW_FETCH_OK = 105,
        INF_EXPORT_OK   = 106,
        WRN_TIMEOUT     = 201,
        ERR_USR_FETCH   = 301,
        ERR_FLW_FETCH   = 302
    };

    QMap<int, QString> m_infoTable;  ///< INFO 用メッセージテーブル
    QMap<int, QString> m_warnTable;  ///< WARN 用メッセージテーブル
    QMap<int, QString> m_errorTable; ///< ERROR 用メッセージテーブル

    /**
     * @brief 現在のすべての状態をファイルに保存する。
     */
    void saveAllState();

    /**
     * @brief ビジー状態（UIロック）を切り替える。
     * @param bBsy true ならロック、false なら解除。
     */
    void setBusy(bool bBsy);
};

#endif // APP_CONTROLLER_H
