#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QTcpSocket>
#include <QAbstractSocket>
#include "QtWebSockets/QWebSocketServer"
#include "QtWebSockets/QWebSocket"

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(quint16 port, QWidget *parent = 0); //konstruktor. pobiera port
    virtual ~MainWindow();

    void doConnect(); //tcp

    //nie mam deklaracji żadnego sygnału (powinien mieć?)

private slots:
    //serial port
    void on_port_currentIndexChanged(int index);
    void refresh();
    void on_commandLine_returnPressed();
    void on_enterButton_clicked();

    //tcp
    void connected(); //sygnał i slot muszą mieć takie same przyjmowane argumenty
    void disconnected();
    void bytesWritten(qint64 bytes);
    void readyRead();

private Q_SLOTS:
    //websockets
    void onNewConnection();
    void processMessage(QString QStr_messageToProcess);
    void socketDisconnected();

private:
    Ui::MainWindow *ui;

    //serial port
    void searchDevices();
    void addTextToConsole(QString text, bool sender=false);
    void send(QString msg);
    void receive();
    void simplyPieceMoving(QString QStr_messageToProcess);
    void findBoardPos(QString QStr_pieceRejecting);

    //websockets
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    void addTextToWebsocketConsole(QString text);
    void sendWsMsgToSite(QWebSocket* client, QString QStr_command, QString QStr_prcessedMsg);

    //tcp
    QTcpSocket *socket;
    void addTextToTcpConsole(QString text);
};

#endif // MAINWINDOW_H
