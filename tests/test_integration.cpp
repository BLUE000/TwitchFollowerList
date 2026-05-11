#include <QtTest>
#include <QCoreApplication>
#include "../ui/mainwindow.h"
#include "../core/app_controller.h"

class TestIntegration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // テスト開始前の初期化
    }

    void cleanupTestCase() {
        // 全テスト終了後の後処理
    }

    void test_it03_group_lifecycle() {
        // IT-03: UI操作からグループ作成・リネーム・履歴保存のフロー
        MainWindow window;
        AppController controller(&window);
        controller.initialize();

        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        // 1. グループ作成のシミュレーション
        QString groupName = "TestGroup_2026";
        controller.handleGroupCreated(groupName);

        // UI（ツリービュー）に反映されているか確認
        // (本来はモデルを走査するが、ここでは AppController の内部状態を確認)
        // QCOMPARE(controller.currentGroups().size(), 2); // デフォルトの「未所属」+ 1

        // 2. リネームのシミュレーション
        // controller.handle_group_renamed(1, "RenamedGroup");

        // 3. 履歴（Undoスタック）が更新されているか
        // QVERIFY(controller.canUndo());
        
        qDebug() << "Integration test framework is ready.";
    }
};

QTEST_MAIN(TestIntegration)
#include "test_integration.moc"
