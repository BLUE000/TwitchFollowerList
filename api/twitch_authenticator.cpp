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
    : QObject(pParent), szClntId(TWITCH_CLIENT_ID), 
      szRdrctUri(QString("http://localhost:%1").arg(AUTH_PORT)), iExprsIn(0)
{
    pTcpSrvr = new QTcpServer(this);
    connect(pTcpSrvr, &QTcpServer::newConnection, this, &TwitchAuthenticator::onNewConnection);
}

/**
 * @brief 認証ブラウザを起動し、ローカルサーバーで待機する。
 */
void TwitchAuthenticator::login() {
    if (!pTcpSrvr->isListening()) {
        pTcpSrvr->listen(QHostAddress::LocalHost, AUTH_PORT);
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

/**
 * @brief トークンが現在有効かどうかを判定する。
 */
bool TwitchAuthenticator::isTokenValid() const {
    if (oDtGrntd.isNull() || iExprsIn <= 0) return false;
    // 現在時刻が (取得時刻 + 有効期間) より前であれば有効
    return oDtGrntd.addSecs(iExprsIn) > QDateTime::currentDateTime();
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
                    
                    // 有効期限 (expires_in) も抽出
                    int iExpIdx = szReq.indexOf("expires_in=");
                    if (iExpIdx != -1) {
                        iExprsIn = szReq.mid(iExpIdx + 11).split(' ').first().split('&').first().toInt();
                    } else {
                        // 取得できない場合はデフォルト値を使用
                        iExprsIn = DEFAULT_EXPIRES_IN;
                    }
                    oDtGrntd = QDateTime::currentDateTime();
                    
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
