#ifndef CIPHER_STEP_H
#define CIPHER_STEP_H

#include <QByteArray>

/**
 * @brief 暗号化/復号化の単一ステップを表す抽象基底クラス
 */
class CipherStep {
public:
    virtual ~CipherStep() = default;

    /**
     * @brief 正方向の変換（暗号化）
     */
    virtual QByteArray process(const QByteArray& data) = 0;

    /**
     * @brief 逆方向の変換（復号）
     */
    virtual QByteArray undo(const QByteArray& data) = 0;
};

#endif // CIPHER_STEP_H
