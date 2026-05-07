/**
 * @file twitch_authenticator.h
 * @brief Twitch OAuth2 認証を管理するクラスの定義。
 */

#ifndef TWITCH_AUTHENTICATOR_H
#define TWITCH_AUTHENTICATOR_H

#include <QObject>
#include <QTcpServer>

/**
 * @class TwitchAuthenticator
 * @brief Twitch 認証フローを制御し、アクセストークンを取得するクラス。
 */
class TwitchAuthenticator : public QObject {
    Q_OBJECT
public:
    /**
     * @brief コンストラクタ。
     * @param pParent 親オブジェクト。
     */
    explicit TwitchAuthenticator(QObject *pParent = nullptr);

    /**
     * @brief ブラウザを開き、Twitch 認証を開始する。
     */
    void login();

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
};

#endif // TWITCH_AUTHENTICATOR_H
