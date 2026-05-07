#ifndef CIPHER_ERROR_H
#define CIPHER_ERROR_H

enum class CipherError {
    Success = 0,            // 正常終了
    InvalidHeader = 1,      // 識別子不一致（鍵が違う、またはデータ破損）
    UnsupportedVersion = 2, // 未知のバージョン指定
    ProcessFailed = 3,      // 変換失敗（不正なBase64データ等）
    UnknownError = 9        // 予期せぬエラー
};

#endif // CIPHER_ERROR_H
