/**
 * @file twitch_api_client.h
 * @brief Twitch API との通信を管理するクラスの定義。
 */

#ifndef TWITCH_API_CLIENT_H
#define TWITCH_API_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include "../core/data_models.h"

/**
 * @class TwitchApiClient
 * @brief Twitch ヘリックス API を呼び出し、ユーザー情報やフォロワー情報を取得するクラス。
 */
class TwitchApiClient : public QObject {
    Q_OBJECT
public:
    /**
     * @brief コンストラクタ。
     * @param pParent 親オブジェクト。
     */
    explicit TwitchApiClient(QObject *pParent = nullptr);

    /**
     * @brief アクセストークンを設定する。
     * @param szTkn OAuth2 アクセストークン。
     */
    void setAccessToken(const QString& szTkn);

    /**
     * @brief ログイン中のユーザー情報（特に ID）を取得する。
     */
    void fetchCurrentUser();

    /**
     * @brief 指定したユーザーのフォロワーリストを取得する。
     */
    void fetchFollowers();

signals:
    /**
     * @brief ユーザー ID の取得が完了した際のシグナル。
     * @param szUsrId Twitch User ID。
     */
    void currentUserFetched(const QString& szUsrId);

    /**
     * @brief フォロワーリストの取得が完了した際のシグナル。
     * @param lstFllwrs 取得されたリスト。
     */
    void followersFetched(const QList<TwitchFollower>& lstFllwrs);

private slots:
    void onCurrentUserReply(QNetworkReply *pRply);

    /**
     * @brief フォロワー取得レスポンスのパース。
     * @param pRply ネットワークリプライ。
     */
    void onFollowersReply(QNetworkReply *pRply);

    /**
     * @brief 次のページのフォロワーを取得する。
     * @param szCursor カーソル。
     */
    void fetchNextFollowersPage(const QString& szCursor);

private:
    QNetworkAccessManager *pNtwrkMngr; ///< ネットワークアクセス管理
    QString szAccsTkn;                 ///< アクセストークン
    QString szClntId;                  ///< Twitch Client ID
    QString szCrntUsrId;               ///< 取得した自ユーザー ID
    QList<TwitchFollower> lstAllFllwrs; ///< 取得中の全フォロワーリスト

    // API エンドポイント URL
    static constexpr const char* szURL_USERS     = "https://api.twitch.tv/helix/users";
    static constexpr const char* szURL_FOLLOWERS = "https://api.twitch.tv/helix/channels/followers?broadcaster_id=%1&first=100";
    static constexpr const char* szURL_FOLLOWERS_NEXT = "https://api.twitch.tv/helix/channels/followers?broadcaster_id=%1&first=100&after=%2";

    // JSON キー
    static constexpr const char* szJS_DATA       = "data";
    static constexpr const char* szJS_ID         = "id";
    static constexpr const char* szJS_USR_ID     = "user_id";
    static constexpr const char* szJS_USR_LGN    = "user_login";
    static constexpr const char* szJS_USR_NM     = "user_name";
    static constexpr const char* szJS_PAGINATION = "pagination";
    static constexpr const char* szJS_CURSOR     = "cursor";

    // HTTP ヘッダー
    static constexpr const char* szHDR_AUTH      = "Authorization";
    static constexpr const char* szHDR_CLNT_ID   = "Client-Id";
    static constexpr const char* szHDR_BEARER    = "Bearer ";

    /**
     * @brief クラス固有のログメッセージを ID 指定で出力する。
     * @param id メッセージ ID。
     */
    void log(int id);

    /// @brief ログメッセージ ID の定義
    enum LogId {
        INF_REQ_SEND    = 101,
        INF_RES_RECV    = 102,
        ERR_NET_FAIL    = 301,
        ERR_JSON_PARSE  = 302,
        ERR_API_RESP    = 303
    };

    QMap<int, QString> m_infoTable;  ///< INFO 用メッセージテーブル
    QMap<int, QString> m_warnTable;  ///< WARN 用メッセージテーブル
    QMap<int, QString> m_errorTable; ///< ERROR 用メッセージテーブル
};

#endif // TWITCH_API_CLIENT_H
