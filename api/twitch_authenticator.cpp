#include "twitch_authenticator.h"
#include <QDesktopServices>
#include <QUrl>
#include <QTcpSocket>
#include <QRegularExpression>

TwitchAuthenticator::TwitchAuthenticator(QObject *parent) : QObject(parent), tcpServer(new QTcpServer(this)) {
    clientId = "lfyil72vhg1s7baymtguh4g06h93qb"; // User's Client ID
    connect(tcpServer, &QTcpServer::newConnection, this, &TwitchAuthenticator::onNewConnection);
}

void TwitchAuthenticator::login() {
    if (!tcpServer->isListening()) {
        tcpServer->listen(QHostAddress::LocalHost, 3000);
    }

    // 認証URLの構築（Implicit Flow）
    QString authUrl = QString("https://id.twitch.tv/oauth2/authorize"
                              "?response_type=token"
                              "&client_id=%1"
                              "&redirect_uri=http://localhost:3000"
                              "&scope=moderator:read:followers")
                          .arg(clientId);

    // 標準ブラウザで開く
    QDesktopServices::openUrl(QUrl(authUrl));
}

void TwitchAuthenticator::onNewConnection() {
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
        QByteArray requestData = clientSocket->readAll();
        QString requestStr = QString::fromUtf8(requestData);

        if (requestStr.contains("GET /?access_token=")) {
            // JavaScriptからハッシュ部分が送信されてきた場合
            QRegularExpression re("access_token=([^&\\s]+)");
            QRegularExpressionMatch match = re.match(requestStr);
            if (match.hasMatch()) {
                QString token = match.captured(1);
                emit authCompleted(token);
                tcpServer->close(); // トークン取得後はリッスン終了
            }
            
            QString response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                               "<html><body><h2>Authentication successful! You can close this window.</h2></body></html>";
            clientSocket->write(response.toUtf8());
            clientSocket->disconnectFromHost();
        } else if (requestStr.contains("GET / ")) {
            // 初回のリダイレクトアクセス（URLハッシュはサーバーに送られないため、JSで再度送らせる）
            QString response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                               "<html><head><script>"
                               "window.onload = function() {"
                               "  if(window.location.hash) {"
                               "    fetch('/?' + window.location.hash.substring(1)).then(function() {"
                               "       document.body.innerHTML = '<h2>Authentication successful! You can close this window.</h2>';"
                               "    });"
                               "  } else {"
                               "    document.body.innerHTML = '<h2>Error: No token received.</h2>';"
                               "  }"
                               "}"
                               "</script></head><body><h2>Processing authentication...</h2></body></html>";
            clientSocket->write(response.toUtf8());
            clientSocket->disconnectFromHost();
        } else {
            // その他のリクエスト（favicon等）
            QString response = "HTTP/1.1 404 Not Found\r\n\r\n";
            clientSocket->write(response.toUtf8());
            clientSocket->disconnectFromHost();
        }
    });
}
