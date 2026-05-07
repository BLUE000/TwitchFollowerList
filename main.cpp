/**
 * @file main.cpp
 * @brief アプリケーションのエントリーポイント。
 */

#include "ui/mainwindow.h"
#include "core/app_controller.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>

/**
 * @brief メイン関数。
 * @param iArgc 引数カウント。
 * @param ppArgv 引数配列。
 * @return 終了ステータス。
 */
int main(int iArgc, char *ppArgv[]) {
    QApplication oApp(iArgc, ppArgv);

    // 翻訳のセットアップ
    QTranslator oTrnsltr;
    const QStringList lstUiLangs = QLocale::system().uiLanguages();
    for (const QString& szLang : lstUiLangs) {
        const QString szBaseNm = "FollowerList_" + QLocale(szLang).name();
        if (oTrnsltr.load(":/i18n/" + szBaseNm)) {
            oApp.installTranslator(&oTrnsltr);
            break;
        } else { /* skip */ }
    }

    MainWindow oW;
    AppController oCntrlr(&oW);
    oCntrlr.initialize();
    
    oW.show();
    return oApp.exec();
}
