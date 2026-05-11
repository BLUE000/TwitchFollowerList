/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *createFolderButton;
    QPushButton *deleteFolderButton;
    QPushButton *undoButton;
    QPushButton *redoButton;
    QSpacerItem *horizontalSpacer;
    QPushButton *twitchLoginButton;
    QSplitter *splitter;
    QTreeView *outDirTreeView;
    QWidget *rightContainer;
    QVBoxLayout *rightVerticalLayout;
    QLineEdit *searchLineEdit;
    QTableView *followerListView;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        createFolderButton = new QPushButton(centralwidget);
        createFolderButton->setObjectName("createFolderButton");

        horizontalLayout->addWidget(createFolderButton);

        deleteFolderButton = new QPushButton(centralwidget);
        deleteFolderButton->setObjectName("deleteFolderButton");

        horizontalLayout->addWidget(deleteFolderButton);

        undoButton = new QPushButton(centralwidget);
        undoButton->setObjectName("undoButton");
        undoButton->setEnabled(false);

        horizontalLayout->addWidget(undoButton);

        redoButton = new QPushButton(centralwidget);
        redoButton->setObjectName("redoButton");
        redoButton->setEnabled(false);

        horizontalLayout->addWidget(redoButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        twitchLoginButton = new QPushButton(centralwidget);
        twitchLoginButton->setObjectName("twitchLoginButton");

        horizontalLayout->addWidget(twitchLoginButton);


        verticalLayout->addLayout(horizontalLayout);

        splitter = new QSplitter(centralwidget);
        splitter->setObjectName("splitter");
        splitter->setOrientation(Qt::Horizontal);
        outDirTreeView = new QTreeView(splitter);
        outDirTreeView->setObjectName("outDirTreeView");
        outDirTreeView->setAcceptDrops(true);
        outDirTreeView->setDragDropMode(QAbstractItemView::DropOnly);
        splitter->addWidget(outDirTreeView);
        rightContainer = new QWidget(splitter);
        rightContainer->setObjectName("rightContainer");
        rightVerticalLayout = new QVBoxLayout(rightContainer);
        rightVerticalLayout->setObjectName("rightVerticalLayout");
        rightVerticalLayout->setContentsMargins(0, 0, 0, 0);
        searchLineEdit = new QLineEdit(rightContainer);
        searchLineEdit->setObjectName("searchLineEdit");
        searchLineEdit->setClearButtonEnabled(true);

        rightVerticalLayout->addWidget(searchLineEdit);

        followerListView = new QTableView(rightContainer);
        followerListView->setObjectName("followerListView");
        followerListView->setDragEnabled(true);
        followerListView->setDragDropMode(QAbstractItemView::DragOnly);
        followerListView->setSelectionBehavior(QAbstractItemView::SelectRows);

        rightVerticalLayout->addWidget(followerListView);

        splitter->addWidget(rightContainer);

        verticalLayout->addWidget(splitter);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 21));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Twitch Follower List", nullptr));
        createFolderButton->setText(QCoreApplication::translate("MainWindow", "\343\203\225\343\202\251\343\203\253\343\203\200\344\275\234\346\210\220", nullptr));
        deleteFolderButton->setText(QCoreApplication::translate("MainWindow", "\343\203\225\343\202\251\343\203\253\343\203\200\345\211\212\351\231\244", nullptr));
        undoButton->setText(QCoreApplication::translate("MainWindow", "Undo", nullptr));
        redoButton->setText(QCoreApplication::translate("MainWindow", "Redo", nullptr));
        twitchLoginButton->setText(QCoreApplication::translate("MainWindow", "Twitch Login", nullptr));
        searchLineEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\346\244\234\347\264\242 (\350\241\250\347\244\272\345\220\215 \343\201\276\343\201\237\343\201\257 \343\203\246\343\203\274\343\202\266\343\203\274\345\220\215)...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
