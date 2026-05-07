/**
 * @file twitch_authenticator.cpp
 * @brief TwitchAuthenticator クラスの実装。
 */

#include "twitch_authenticator.h"
#include <QDesktopServices>
#include <QUrl>
#include <QTcpSocket>
#include <QTextStream>

/**
 * @brief コンストラクタ。
 * @param pParent 親オブジェクト。
 */
TwitchAuthenticator::TwitchAuthenticator(QObject *pParent)
    : QObject(pParent), szClntId("gp762nuuoqcoxypju8c569th9wz7q5") // Example Client ID
{
    pTcpSrvr = new QTcpServer(this);
    connect(pTcpSrvr, &QTcpServer::newConnection, this, &TwitchAuthenticator::onNewConnection);
}

/**
 * @brief 認証ブラウザを起動し、ローカルサーバーで待機する。
 */
void TwitchAuthenticator::login() {
    if (!pTcpSrvr->isListening()) {
        pTcpSrvr->listen(QHostAddress::LocalHost, 8080);
    } else {
        // Already listening
    }

    QString szUrl = QString("https://id.twitch.tv/oauth2/authorize"
                          "?client_id=%1"
                          "&redirect_uri=http://localhost:8080"
                          "&response_type=token"
                          "&scope=user:read:follows").arg(szClntId);
    
    QDesktopServices::openUrl(QUrl(szUrl));
}

/**
 * @brief ブラウザからのリダイレクトをハンドルし、フラグメントからトークンを抽出する。
 * 注意：実際の実装では HTML ファイルでフラグメントをクエリに変換して送信する等の処理が必要。
 * ここではモック的な実装とする。
 */
void TwitchAuthenticator::onNewConnection() {
    QTcpSocket *pSckt = pTcpSrvr->nextPendingConnection();
    if (pSckt) {
        connect(pSckt, &QTcpSocket::readyRead, [this, pSckt]() {
            QString szReq = pSckt->readAll();
            // 実際はここで URL フラグメント (#access_token=...) を取得するロジックが必要
            // 簡易実装としてダミーのトークンを発行
            emit authCompleted("mock_access_token_12345");
            
            QTextStream oRes(pSckt);
            oRes << "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                 << "<html><body><h1>認証完了</h1><p>ブラウザを閉じてアプリに戻ってください。</p></body></html>";
            pSckt->disconnectFromHost();
        });
    } else {
        // No socket
    }
}
