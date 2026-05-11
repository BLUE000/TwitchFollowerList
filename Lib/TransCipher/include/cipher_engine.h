#ifndef CIPHER_ENGINE_H
#define CIPHER_ENGINE_H

#include "trans_cipher_export.h"
#include "cipher_result.h"
#include <QString>
#include <QByteArray>

/**
 * @brief AES暗号化の適用モード
 */
enum class AesMode : uint8_t {
    Disabled = 0,   // AES不使用（互換性・軽量用）
    Mandatory = 1,  // 強制適用（先頭にAESを配置）
    Randomized = 2  // ランダム候補の一つとしてAESを追加
};

class TRANSCIPHER_EXPORT CipherEngine {
public:
    /**
     * @brief データを暗号化する
     * @param data 生データ
     * @param key 秘密鍵
     * @param mode AESの適用モード (デフォルトは Mandatory)
     * @return 処理結果（成功すれば 32byteヘッダー付与済みのバイナリ）
     */
    static CipherResult encrypt(const QByteArray& data, const QString& key, AesMode mode = AesMode::Mandatory);

    /**
     * @brief データを復号する
     * @param data 暗号化データ（ヘッダー付き）
     * @param key 秘密鍵
     * @return 処理結果（成功すれば生データ）
     */
    static CipherResult decrypt(const QByteArray& data, const QString& key);

private:
    static const int HEADER_BLOCK_SIZE = 32;
    static const char* MAGIC_NUMBER; // "TCF"
    static const char* VERSION_INFO; // "1.0"
};

#endif // CIPHER_ENGINE_H
