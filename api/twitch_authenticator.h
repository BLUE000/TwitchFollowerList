#ifndef TWITCH_AUTHENTICATOR_H
#define TWITCH_AUTHENTICATOR_H

#include <QObject>
#include <QTcpServer>

class TwitchAuthenticator : public QObject {
    Q_OBJECT
public:
    explicit TwitchAuthenticator(QObject *parent = nullptr);
    void login();

signals:
    void authCompleted(const QString& token);

private slots:
    void onNewConnection();

private:
    QTcpServer *tcpServer;
    QString clientId;
};

#endif // TWITCH_AUTHENTICATOR_H
