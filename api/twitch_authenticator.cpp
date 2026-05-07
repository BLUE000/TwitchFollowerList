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
    : QObject(pParent), szClntId("lfyil72vhg1s7baymtguh4g06h93qb"), szRdrctUri("http://localhost:8080")
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
                            "&redirect_uri=%2"
                            "&response_type=token"
                            "&scope=moderator:read:followers")
                    .arg(szClntId)
                    .arg(szRdrctUri);
    
    QDesktopServices::openUrl(QUrl(szUrl));
}

void TwitchAuthenticator::onNewConnection() {
    QTcpSocket *pSckt = pTcpSrvr->nextPendingConnection();
    if (pSckt) {
        connect(pSckt, &QTcpSocket::readyRead, [this, pSckt]() {
            QByteArray oRawReq = pSckt->readAll();
            QString szReq = QString::fromUtf8(oRawReq);
            
            QTextStream oRes(pSckt);
            oRes.setEncoding(QStringConverter::Utf8);

            if (szReq.contains("GET /token")) {
                // 2回目：JS から送られてきたトークンを抽出
                int iIdx = szReq.indexOf("access_token=");
                if (iIdx != -1) {
                    QString szTkn = szReq.mid(iIdx + 13).split(' ').first().split('&').first();
                    emit authCompleted(szTkn);
                }

                oRes << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: text/html; charset=UTF-8\r\n"
                     << "Connection: close\r\n\r\n"
                     << "<html><body style='text-align:center;padding-top:50px;font-family:sans-serif;'>"
                     << "<h1>認証完了！</h1><p>アプリに戻って確認してください。</p>"
                     << "</body></html>";
            } else {
                // 1回目：フラグメント（#）をクエリ（?）に変換して再送させる JS を返す
                oRes << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: text/html; charset=UTF-8\r\n"
                     << "Connection: close\r\n\r\n"
                     << "<html><script>"
                     << "if(location.hash){ location.href='/token?'+location.hash.substring(1); }"
                     << "</script><body>認証中...</body></html>";
            }
            oRes.flush();
            pSckt->disconnectFromHost();
        });
        connect(pSckt, &QTcpSocket::disconnected, pSckt, &QTcpSocket::deleteLater);
    }
}
