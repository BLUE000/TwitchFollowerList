#ifndef CIPHER_RESULT_H
#define CIPHER_RESULT_H

#include "trans_cipher_export.h"
#include "cipher_error.h"
#include <QByteArray>
#include <QString>

class TRANSCIPHER_EXPORT CipherResult {
public:
    CipherResult(CipherError error = CipherError::Success, 
                 const QByteArray& data = QByteArray(), 
                 const QString& message = "");

    bool isSuccess() const;
    CipherError error() const;
    QByteArray data() const;
    QString message() const;

private:
    CipherError m_error;
    QByteArray m_data;
    QString m_message;
};

#endif // CIPHER_RESULT_H
