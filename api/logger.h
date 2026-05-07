#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QMutex>

/**
 * @brief ISO 8601 準拠のロガークラス。
 * 各クラスから渡されたメッセージにタイムスタンプを付与し、ファイルおよびコンソールに出力する。
 */
class Logger {
public:
    /**
     * @brief ログを出力する。
     * @param szLevel ログレベル ("INFO", "WARN", "ERROR")。
     * @param szMessage 出力するメッセージ本文。
     */
    static void output(const QString& szLevel, const QString& szMessage);

private:
    static QMutex m_mutex; ///< スレッドセーフのためのミューテックス
};

#endif // LOGGER_H
