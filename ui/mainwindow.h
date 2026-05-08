/**
 * @file mainwindow.h
 * @brief メイン画面（View）の定義。
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QList>
#include "../core/data_models.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QStandardItemModel;
class QPushButton;

/**
 * @class MainWindow
 * @brief アプリケーションのメインウィンドウ。ユーザーインターフェース操作を管理する。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    static const int INVALID_ID = -999; ///< 無効な ID を示す定数

    /**
     * @brief コンストラクタ。
     * @param pParent 親オブジェクト。
     */
    explicit MainWindow(QWidget *pParent = nullptr);

    /**
     * @brief デストラクタ。
     */
    ~MainWindow();

    /**
     * @brief ログイン状態の表示を更新する。
     * @param bLgned ログイン済みなら true。
     */
    void setLoginStatus(bool bLgned);

    /**
     * @brief フォロワーリストを UI に反映する。
     * @param lstFllwrs 表示対象のフォロワーリスト。
     */
    void setFollowers(const QList<TwitchFollower>& lstFllwrs, const QMap<int, QString>& mapGrps);

    /**
     * @brief グループ一覧を UI に反映する。
     * @param mapGrps 表示対象のグループマップ。
     */
    void setGroups(const QMap<int, QString>& mapGrps);

    /**
     * @brief Undo/Redo ボタンの有効無効を切り替える。
     * @param bCanWnd Undo 可能なら true。
     * @param bCanRd Redo 可能なら true。
     */
    void setUndoRedoEnabled(bool bCanWnd, bool bCanRd);

    /**
     * @brief イベントフィルター。ドラッグ＆ドロップ等のイベントを捕捉する。
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    /**
     * @brief ログインボタンが押された際のシグナル。
     */
    void loginRequested();

    /**
     * @brief フォロワーをグループに割り当てる要求のシグナル。
     * @param szUsrId ユーザー ID。
     * @param iGrpId グループ ID。
     */
    void followerAssignedToGroup(const QString& szUsrId, int iGrpId);

    /**
     * @brief フォロワーのグループ解除要求のシグナル。
     * @param szUsrId ユーザー ID。
     * @param iGrpId グループ ID。
     */
    void followerUnassignedFromGroup(const QString& szUsrId, int iGrpId);

    /**
     * @brief グループ作成要求のシグナル。
     * @param szGrpNm グループ名。
     */
    void groupCreated(const QString& szGrpNm);

    /**
     * @brief グループ削除要求のシグナル。
     * @param iGrpId グループ ID。
     */
    void groupDeleted(int iGrpId);

    /**
     * @brief Undo 要求のシグナル。
     */
    void undoRequested();

    /**
     * @brief Redo 要求のシグナル。
     */
    void redoRequested();
 
    /**
     * @brief Output 要求のシグナル。
     */
    void outputRequested();

    /**
     * @brief グループ選択変更のシグナル。
     * @param iGrpId 選択されたグループ ID。
     */
    void groupSelected(int iGrpId);

private slots:
    void onLoginButtonClicked();
    void onAddGroupButtonClicked();
    void onDeleteGroupButtonClicked();
    void onUndoButtonClicked();
    void onRedoButtonClicked();
    void onGroupTreeClicked(const QModelIndex& index);
    void onFollowerListContextMenu(const QPoint& pos);
    void onOutputButtonClicked();

private:
    Ui::MainWindow *pUi;                    ///< UI デザインへのポインタ
    QStandardItemModel *pMdlFllwr;         ///< フォロワー表示用モデル
    QStandardItemModel *pMdlGrp;           ///< グループ表示用モデル
    QPushButton *pBtnOutput;               ///< Output ボタン

    /**
     * @brief UI 要素の初期設定を行う。
     */
    void setupUiExtra();
};

#endif // MAINWINDOW_H
