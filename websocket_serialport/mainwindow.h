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
    explicit MainWindow(quint16 port, QWidget *parent = 0); //konstruktor. pobiera port.
    //dobrze by było nadać temu portowi jakiś przydomek typu "tcp_port" by się wyróżniał od portu usb...
    //...nie udało mi się to ostatnim razem

    virtual ~MainWindow();

    void doTcpConnect(); //tcp

    //nie mam deklaracji żadnego sygnału (powinien mieć?)

private slots:
    //serial port
    void on_port_currentIndexChanged(int index); //zmiana/wybór portu
    void refresh();
    void on_commandLine_returnPressed(); //reakcja na wciśnięcie entera na klawiaturze...
    void on_enterButton_clicked(); //... i przycisku w programie
    void readUsbData(); //sczytywanie danych lecących asynchronicznie z usb

    //tcp
    void connected(); //sygnał i slot muszą mieć takie same przyjmowane argumenty
    void disconnected();
    void bytesWritten(qint64 bytes);
    void readyRead();

private Q_SLOTS:
    //websockets
    void onNewConnection();
    void processWebsocketMsg(QString QS_WbstMsgToProcess);
    void socketDisconnected();

private:
    Ui::MainWindow *ui;

    //serial port
    void searchDevices();
    QSerialPort *usb_port;
    //addTextToUsbConsole jako argumenty przyjmuje wiadomość, którą ma wstawić na konsolę i info o tym czy
    //jest ona od użytkownika (a więc należy ją jeszcze wysłać do urządzenia), czy od urządzenia
    void addTextToUsbConsole(QString text, bool sender=false);
    void sendDataToUsb(QString QS_msg, bool sender=false); //wysyłanie wiadomośći na usb
    void receive(); //odbieranie wiadomości z usb

    void normalPieceMoving(QString QS_normalMove);
    bool removeStatements();
    void removePieceSequence(QString QS_msgFromSerial);
    void castlingMovingSequence(QString QS_msgFromSerial);
    bool castlingStatements(QString QS_possibleCastlingVal);
    void enpassantMovingSequence();
    bool testEnpassant();
    bool testPromotion();

    void findBoardPos(QString QS_piecePositions);

    //websockets
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    void addTextToWebsocketConsole(QString text);
    void sendWsMsgToSite(QWebSocket* client, QString QS_command, QString QS_prcessedMsg);

    //tcp
    QTcpSocket *socket;
    void addTextToTcpConsole(QString text);
};

#endif // MAINWINDOW_H
