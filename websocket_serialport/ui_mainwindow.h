/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.4.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionNONE;
    QAction *action_refreshPorts;
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QPlainTextEdit *tcp_debug;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_3;
    QPlainTextEdit *websocket_debug;
    QLabel *label;
    QComboBox *port;
    QLabel *label_2;
    QLineEdit *commandLine;
    QPushButton *visual;
    QPushButton *enterButton;
    QPlainTextEdit *output;
    QLabel *czas_bialego;
    QLabel *czas_czarnego;
    QMenuBar *menuBar;
    QMenu *menuTools;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(527, 386);
        MainWindow->setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));
        actionNONE = new QAction(MainWindow);
        actionNONE->setObjectName(QStringLiteral("actionNONE"));
        actionNONE->setCheckable(true);
        action_refreshPorts = new QAction(MainWindow);
        action_refreshPorts->setObjectName(QStringLiteral("action_refreshPorts"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        centralWidget->setEnabled(true);
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        tcp_debug = new QPlainTextEdit(centralWidget);
        tcp_debug->setObjectName(QStringLiteral("tcp_debug"));

        gridLayout->addWidget(tcp_debug, 2, 3, 1, 1);

        label_4 = new QLabel(centralWidget);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 4, 2, 1, 1);

        label_5 = new QLabel(centralWidget);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout->addWidget(label_5, 6, 2, 1, 1);

        label_3 = new QLabel(centralWidget);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 1, 3, 1, 1);

        websocket_debug = new QPlainTextEdit(centralWidget);
        websocket_debug->setObjectName(QStringLiteral("websocket_debug"));

        gridLayout->addWidget(websocket_debug, 2, 2, 1, 1);

        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 1, 0, 1, 1);

        port = new QComboBox(centralWidget);
        port->setObjectName(QStringLiteral("port"));

        gridLayout->addWidget(port, 6, 0, 1, 1);

        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 1, 2, 1, 1);

        commandLine = new QLineEdit(centralWidget);
        commandLine->setObjectName(QStringLiteral("commandLine"));
        commandLine->setDragEnabled(false);

        gridLayout->addWidget(commandLine, 4, 0, 1, 1);

        visual = new QPushButton(centralWidget);
        visual->setObjectName(QStringLiteral("visual"));

        gridLayout->addWidget(visual, 6, 1, 1, 1);

        enterButton = new QPushButton(centralWidget);
        enterButton->setObjectName(QStringLiteral("enterButton"));

        gridLayout->addWidget(enterButton, 4, 1, 1, 1);

        output = new QPlainTextEdit(centralWidget);
        output->setObjectName(QStringLiteral("output"));
        output->setReadOnly(true);
        output->setOverwriteMode(false);

        gridLayout->addWidget(output, 2, 0, 1, 2);

        czas_bialego = new QLabel(centralWidget);
        czas_bialego->setObjectName(QStringLiteral("czas_bialego"));

        gridLayout->addWidget(czas_bialego, 4, 3, 1, 1);

        czas_czarnego = new QLabel(centralWidget);
        czas_czarnego->setObjectName(QStringLiteral("czas_czarnego"));

        gridLayout->addWidget(czas_czarnego, 6, 3, 1, 1);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 527, 18));
        menuTools = new QMenu(menuBar);
        menuTools->setObjectName(QStringLiteral("menuTools"));
        MainWindow->setMenuBar(menuBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);

        menuBar->addAction(menuTools->menuAction());
        menuTools->addAction(action_refreshPorts);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "Connection core", 0));
        actionNONE->setText(QApplication::translate("MainWindow", "NONE", 0));
        action_refreshPorts->setText(QApplication::translate("MainWindow", "Refresh Ports", 0));
        label_4->setText(QApplication::translate("MainWindow", "Czas Bia\305\202ego:", 0));
        label_5->setText(QApplication::translate("MainWindow", "Czas Czarnego:", 0));
        label_3->setText(QApplication::translate("MainWindow", "TCP socket", 0));
        label->setText(QApplication::translate("MainWindow", "Serial port", 0));
        label_2->setText(QApplication::translate("MainWindow", "WebSockets", 0));
        commandLine->setText(QString());
        commandLine->setPlaceholderText(QString());
        visual->setText(QApplication::translate("MainWindow", "Visualize", 0));
        enterButton->setText(QApplication::translate("MainWindow", "Enter", 0));
        czas_bialego->setText(QApplication::translate("MainWindow", "0", 0));
        czas_czarnego->setText(QApplication::translate("MainWindow", "0", 0));
        menuTools->setTitle(QApplication::translate("MainWindow", "Settings", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
