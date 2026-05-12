#include <gtest/gtest.h>
#include "../api/file_manager.h"
#include <QString>

class FileManagerTest : public ::testing::Test {
protected:
    FileManager* fileManager;

    void SetUp() override {
        fileManager = new FileManager();
    }

    void TearDown() override {
        delete fileManager;
    }
};

// 単体テスト仕様書 2.1 準拠: encodeData / decodeData の整合性テスト
TEST_F(FileManagerTest, EncodeDecodeConsistency) {
    QString originalCsv = "1,TestUser,test_user,12345,";
    
    // エンコード
    QString encoded = fileManager->encodeData(originalCsv);
    EXPECT_FALSE(encoded.isEmpty());
    EXPECT_NE(originalCsv, encoded);

    // デコード
    QString decoded = fileManager->decodeData(encoded);
    EXPECT_EQ(originalCsv, decoded);
}

// 異常系: 空文字列の処理
TEST_F(FileManagerTest, EmptyStringHandling) {
    QString originalCsv = "";
    QString encoded = fileManager->encodeData(originalCsv);
    QString decoded = fileManager->decodeData(encoded);
    EXPECT_EQ(originalCsv, decoded);
}

// 内部エンコード（&comma; 等）のテスト (v2.0)
TEST_F(FileManagerTest, InternalEncodingTest) {
    QString memoWithSymbols = "Test, \"Memo\"\nNext Line";
    
    QList<TwitchFollower> followers;
    TwitchFollower f;
    f.userId = "123";
    f.userName = "TestUser";
    f.memo = memoWithSymbols;
    followers.append(f);

    // 保存して即読み込み
    fileManager->saveAllListDat(followers);
    
    QList<TwitchFollower> loaded;
    fileManager->loadAllListDat(loaded);

    ASSERT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].userId, f.userId);
    EXPECT_EQ(loaded[0].memo, memoWithSymbols);
}
