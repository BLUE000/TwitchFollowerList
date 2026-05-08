/**
 * @file twitch_authenticator.h
 * @brief Twitch OAuth2 認証を管理するクラスの定義。
 */

#ifndef TWITCH_AUTHENTICATOR_H
#define TWITCH_AUTHENTICATOR_H

#include <QObject>
#include <QTcpServer>
#include <QDateTime>

/**
 * @class TwitchAuthenticator
 * @brief Twitch 認証フローを制御し、アクセストークンを取得するクラス。
 */
class TwitchAuthenticator : public QObject {
    Q_OBJECT
public:
    static const int DEFAULT_EXPIRES_IN = 3600; ///< デフォルト有効期間（1時間）
    static const int AUTH_PORT = 8080;         ///< 認証受信用ポート番号
    /**
     * @brief コンストラクタ。
     * @param pParent 親オブジェクト。
     */
    explicit TwitchAuthenticator(QObject *pParent = nullptr);

    /**
     * @brief ブラウザを開き、Twitch 認証を開始する。
     */
    void login();

    /**
     * @brief トークンが現在有効かどうかを判定する。
     * @return 有効なら true。
     */
    bool isTokenValid() const;

signals:
    /**
     * @brief 認証が完了し、トークンを取得した際のシグナル。
     * @param szTkn 取得したアクセストークン。
     */
    void authCompleted(const QString& szTkn);

private slots:
    /**
     * @brief ローカルサーバーへのコールバック接続をハンドルする内部スロット。
     */
    void onNewConnection();

private:
    QTcpServer *pTcpSrvr; ///< コールバック受信用サーバー
    QString szClntId;    ///< Twitch Client ID
    QString szRdrctUri;  ///< Redirect URI
    QDateTime oDtGrntd;  ///< トークン取得時刻
    int iExprsIn;        ///< 有効期間（秒）
};

#endif // TWITCH_AUTHENTICATOR_H
