#include "logger.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>

QMutex Logger::m_mutex;

void Logger::output(const QString& szLevel, const QString& szMessage) {
    QMutexLocker locker(&m_mutex);

    // 1. タイムスタンプの生成 (ISO 8601: YYYY-MM-DDTHH:mm:ss)
    QString szTime = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");
    
    // 2. ログメッセージの整形
    QString szLogEntry = QString("[%1][%2] %3\n").arg(szTime).arg(szLevel).arg(szMessage);

    // 3. コンソール（qDebug）へのミラーリング
    qDebug().noquote() << szLogEntry.trimmed();

    // 4. ファイルへの書き出し
    QString szLogDir = QCoreApplication::applicationDirPath() + "/logs";
    QDir().mkpath(szLogDir);

    QString szFileName = QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
    QString szFilePath = szLogDir + "/" + szFileName;

    QFile file(szFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        stream << szLogEntry;
        file.close();
    }
}
