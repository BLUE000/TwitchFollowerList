#include "twitch_api_client.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

/**
 * @brief コンストラクタ。
 * @param pParent 親オブジェクト。
 */
TwitchApiClient::TwitchApiClient(QObject *pParent)
    : QObject(pParent), szClntId("gp762nuuoqcoxypju8c569th9wz7q5")
{
    pNtwrkMngr = new QNetworkAccessManager(this);
}

/**
 * @brief 認証トークンを保持する。
 * @param szTkn アクセストークン。
 */
void TwitchApiClient::setAccessToken(const QString& szTkn) {
    szAccsTkn = szTkn;
}

/**
 * @brief 自身のユーザー情報を取得する。
 */
void TwitchApiClient::fetchCurrentUser() {
    QNetworkRequest oReq;
    oReq.setUrl(QUrl(szURL_USERS));
    oReq.setRawHeader(szHDR_AUTH, szHDR_BEARER + szAccsTkn.toUtf8());
    oReq.setRawHeader(szHDR_CLNT_ID, szClntId.toUtf8());

    QNetworkReply *pRply = pNtwrkMngr->get(oReq);
    connect(pRply, &QNetworkReply::finished, [this, pRply]() {
        onCurrentUserReply(pRply);
    });
}

/**
 * @brief 自身のフォロワーリストを取得する。
 */
void TwitchApiClient::fetchFollowers() {
    if (szCrntUsrId.isEmpty()) {
        return;
    } else {
        // Helix API: Get Channel Followers
        QString szUrl = QString(szURL_FOLLOWERS).arg(szCrntUsrId);
        QNetworkRequest oReq;
        oReq.setUrl(QUrl(szUrl));
        oReq.setRawHeader(szHDR_AUTH, szHDR_BEARER + szAccsTkn.toUtf8());
        oReq.setRawHeader(szHDR_CLNT_ID, szClntId.toUtf8());

        QNetworkReply *pRply = pNtwrkMngr->get(oReq);
        connect(pRply, &QNetworkReply::finished, [this, pRply]() {
            onFollowersReply(pRply);
        });
    }
}

/**
 * @brief ユーザー情報レスポンスのパース。
 * @param pRply ネットワークリプライ。
 */
void TwitchApiClient::onCurrentUserReply(QNetworkReply *pRply) {
    if (pRply->error() == QNetworkReply::NoError) {
        QByteArray oDt = pRply->readAll();
        QJsonDocument oDoc = QJsonDocument::fromJson(oDt);
        QJsonObject oObj = oDoc.object();
        QJsonArray oArr = oObj[szJS_DATA].toArray();
        if (!oArr.isEmpty()) {
            szCrntUsrId = oArr[0].toObject()[szJS_ID].toString();
            emit currentUserFetched(szCrntUsrId);
        } else { /* no data */ }
    } else {
        // Error handling
    }
    pRply->deleteLater();
}

/**
 * @brief フォロワーリストレスポンスのパース。
 * @param pRply ネットワークリプライ。
 */
void TwitchApiClient::onFollowersReply(QNetworkReply *pRply) {
    if (pRply->error() == QNetworkReply::NoError) {
        QList<TwitchFollower> lstFllwrs;
        QByteArray oDt = pRply->readAll();
        QJsonDocument oDoc = QJsonDocument::fromJson(oDt);
        QJsonObject oObj = oDoc.object();
        QJsonArray oArr = oObj[szJS_DATA].toArray();

        for (const auto& oV : oArr) {
            QJsonObject oF = oV.toObject();
            TwitchFollower oFllwr;
            oFllwr.userId = oF[szJS_USR_ID].toString();
            oFllwr.userLogin = oF[szJS_USR_LGN].toString();
            oFllwr.userName = oF[szJS_USR_NM].toString();
            lstFllwrs.append(oFllwr);
        }
        emit followersFetched(lstFllwrs);
    } else {
        // Error handling
    }
    pRply->deleteLater();
}
