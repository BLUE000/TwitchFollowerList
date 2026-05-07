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
    : QObject(pParent), szClntId("lfyil72vhg1s7baymtguh4g06h93qb")
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
    }
    
    lstAllFllwrs.clear();
    fetchNextFollowersPage("");
}

/**
 * @brief 指定されたカーソルを使用して次のページのフォロワーを取得する内部関数。
 * @param szCursor ページネーション用カーソル（空なら初回）。
 */
void TwitchApiClient::fetchNextFollowersPage(const QString& szCursor) {
    QString szUrl;
    if (szCursor.isEmpty()) {
        szUrl = QString(szURL_FOLLOWERS).arg(szCrntUsrId);
    } else {
        szUrl = QString(szURL_FOLLOWERS_NEXT).arg(szCrntUsrId).arg(szCursor);
    }
    
    QNetworkRequest oReq;
    oReq.setUrl(QUrl(szUrl));
    oReq.setRawHeader(szHDR_AUTH, szHDR_BEARER + szAccsTkn.toUtf8());
    oReq.setRawHeader(szHDR_CLNT_ID, szClntId.toUtf8());

    QNetworkReply *pRply = pNtwrkMngr->get(oReq);
    connect(pRply, &QNetworkReply::finished, [this, pRply]() {
        onFollowersReply(pRply);
    });
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
            lstAllFllwrs.append(oFllwr);
        }

        // 次のページがあるか確認
        QString szCursor = oObj[szJS_PAGINATION].toObject()[szJS_CURSOR].toString();
        if (!szCursor.isEmpty() && !oArr.isEmpty()) {
            fetchNextFollowersPage(szCursor);
        } else {
            emit followersFetched(lstAllFllwrs);
        }
    } else {
        // Error
        emit followersFetched(lstAllFllwrs);
    }
    pRply->deleteLater();
}
