#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include <QMessageBox>
#include <QTcpSocket>

QT_USE_NAMESPACE

// globalne zmienne
QList <QSerialPortInfo> available_port;     // lista portów pod którymi są urządzenia
const QSerialPortInfo *info;                // obecnie wybrany serial port
QSerialPort port;                           // obecnie otwarty port
QString QStr_chenardAnswer;                 // odpowiedź z chenard
QString QStr_chenardQuestion;               // zmienna do chenard
QString QStr_cmdForSite;                    // zmienna typu temp, do wysłania na stronę
QString QStr_movementMade;                  // zmienna pamiętająca ruch wykonany ze strony
QString QStr_pieceFrom;                     // z jakiego pola pionek będzie podnoszony
QString QStr_pieceTo;                       // na jakie pole pionek będzie przenoszony
QString QStr_msgFromSerial;                 // wiadomość która przyszła z serial portu

//zamienniki mysqla
QString QStr_nameWhite = "Biały";           // nazwa białego- startowo "Biały
QString QStr_nameBlack = "Czarny";          // nazwa czarnego- startowo "Czarny"
QString QStr_whoseTurn;                     // czyja tura aktualnie - no_turn, white_turn, black_turn

bool b_blokada_zapytan = false;             // nie przyjmuj zapytań, dopóki poprzednie nie jest zrealizowane
bool a_b_board[8][8] =                      // plansza jako macierz. jedynki to zajęte pola
{{1, 1, 0, 0, 0, 0, 1, 1} ,
 {1, 1, 0, 0, 0, 0, 1, 1} ,
 {1, 1, 0, 0, 0, 0, 1, 1} ,
 {1, 1, 0, 0, 0, 0, 1, 1} ,
 {1, 1, 0, 0, 0, 0, 1, 1} ,
 {1, 1, 0, 0, 0, 0, 1, 1} ,
 {1, 1, 0, 0, 0, 0, 1, 1} ,
 {1, 1, 0, 0, 0, 0, 1, 1}};
int n_literaPola;
int n_cyfraPola;


MainWindow::MainWindow(quint16 port, QWidget *parent) :
    QMainWindow(parent),
    m_pWebSocketServer(Q_NULLPTR),
    m_clients(),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    info = NULL;
    searchDevices();
    connect(ui->action_refreshPorts,SIGNAL(triggered()),this,SLOT(refresh()));

    ///////websockets
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("Chat Server"),
                                              QWebSocketServer::NonSecureMode,
                                              this);//nowy serwer websocket
    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) //nasłuchuj na porcie
    {
        addTextToWebsocketConsole("Server listening on port " + QString::number(port) + "\n");
        //połącz slot nowego połączenia wbudowanej metody websocket z moją metodą onNewConnection
        //czyli jeżeli będzie nowe połączenie na websocketach, to odpali się funckja onNewConnection
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &MainWindow::onNewConnection);
    }///////
}

MainWindow::~MainWindow()
{
    if(port.isOpen())
        port.close();

    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());

    delete ui;
}

////////////////////////serial port
void MainWindow::findBoardPos(QString QStr_pieceRejecting)
{
    QString QStr_literaPola = QStr_pieceRejecting.mid(1,1);
    if (QStr_literaPola == "a" || QStr_literaPola == "A") {n_literaPola = 0; QStr_literaPola = "a";}
    else if (QStr_literaPola == "b" || QStr_literaPola == "B") {n_literaPola = 1; QStr_literaPola = "b";}
    else if (QStr_literaPola == "c" || QStr_literaPola == "C") {n_literaPola = 2; QStr_literaPola = "c";}
    else if (QStr_literaPola == "d" || QStr_literaPola == "D") {n_literaPola = 3; QStr_literaPola = "d";}
    else if (QStr_literaPola == "e" || QStr_literaPola == "E") {n_literaPola = 4; QStr_literaPola = "e";}
    else if (QStr_literaPola == "f" || QStr_literaPola == "F") {n_literaPola = 5; QStr_literaPola = "f";}
    else if (QStr_literaPola == "g" || QStr_literaPola == "G") {n_literaPola = 6; QStr_literaPola = "g";}
    else if (QStr_literaPola == "h" || QStr_literaPola == "H") {n_literaPola = 7; QStr_literaPola = "h";}

    n_cyfraPola = QStr_pieceRejecting.mid(2,1).toInt() - 1;

    addTextToWebsocketConsole("n_literaPola = " + QString::number(n_literaPola) + "\n");
    addTextToWebsocketConsole("n_cyfraPola = " + QString::number(n_cyfraPola) + "\n");
    addTextToWebsocketConsole("a_b_board[m][n] = " + QString::number(a_b_board[n_literaPola][n_cyfraPola]) + "\n");
}

void MainWindow::simplyPieceMoving(QString QStr_msgToProcess)
{
    QStr_pieceFrom = "[" + QStr_msgToProcess.mid(5,2) + "]f";
    QStr_pieceTo   = "[" + QStr_msgToProcess.mid(7,2) + "]t";

    findBoardPos(QStr_pieceTo); //znajdź pozycje planszy dla funkcji zbijającej
    if (a_b_board[n_literaPola][n_cyfraPola] == true) //sprawdzanie czy na pole, gdzie pionek idzie nie jest zajęte
    {
        QString QStr_pieceToReject = "[" + QStr_msgToProcess.mid(7,2) + "]r";
        addTextToConsole(QStr_pieceToReject, true); //zbijanie pionka
        if (QStr_msgFromSerial.left(8) == "movedToR") addTextToConsole("openR1", true);
        if (QStr_msgFromSerial.left(8) == "openedR1") addTextToConsole("downR", true);
        if (QStr_msgFromSerial.left(5) == "downR") addTextToConsole("closeR", true);
        if (QStr_msgFromSerial.left(7) == "closedR") addTextToConsole("upR", true);
        if (QStr_msgFromSerial.left(6) == "armUpR") addTextToConsole("trashR", true);
        if (QStr_msgFromSerial.left(8) == "trashedR") addTextToConsole("openR2", true);
        //brak przypadku/reakcji/funkcji dla komunikatu: openedR2. zbędne. tylko dostaję info
        a_b_board[n_literaPola][n_cyfraPola] = false; //miejsce już nie jest zajęte
    }

    addTextToConsole(QStr_pieceFrom, true);
    if (QStr_msgFromSerial.left(9) == "movedFrom" && QStr_msgFromSerial.mid(10,2) == QStr_pieceFrom.mid(1,2))
        addTextToConsole("open1", true);
    if (QStr_msgFromSerial.left(7) == "opened1") addTextToConsole("down1", true);
    if (QStr_msgFromSerial.left(8)  == "armDown1") addTextToConsole("close1", true);
    if (QStr_msgFromSerial.left(7)  == "closed1") addTextToConsole("up1", true);
    if (QStr_msgFromSerial.left(6)  == "armUp1")
    {
        findBoardPos(QStr_pieceFrom); //znajdź pozycję z której pionka zabrano
        a_b_board[n_literaPola][n_cyfraPola] = false; //miejsce ruszanego pionka jest już puste
        addTextToConsole(QStr_pieceTo, true);
    }
    if (QStr_msgFromSerial.left(7) == "movedTo" && QStr_msgFromSerial.mid(8,2) == QStr_pieceTo.mid(1,2))
        addTextToConsole("down2", true);
    if (QStr_msgFromSerial.left(8)  == "armDown2") addTextToConsole("open2", true);
    if (QStr_msgFromSerial.left(7)  == "opened2")
    {
        findBoardPos(QStr_pieceTo); //znajdź pozycję na której znalazł się przeniesiony pionek
        a_b_board[n_literaPola][n_cyfraPola] = true; //nowe miejsce ruszpnego pionka jest już teraz zajęte
        addTextToConsole("up2", true);
    }
    if (QStr_msgFromSerial.left(6)  == "armUp2") emit processMessage("OK 1\n");
    /*else
    {
        addTextToConsole("error:", true);
        // dodawanie tekstu do konsoli
        QString line = QStr_msgFromSerial + "\n";
        ui->output->setPlainText(ui->output->toPlainText() + line);
        // auto scroll
        QScrollBar *scroll = ui->output->verticalScrollBar();
        scroll->setValue(scroll->maximum());
    }*/
}

// aktualizowanie listy z urządzeniami
void MainWindow::searchDevices()
{
    // dodanie ich do listy
    available_port = QSerialPortInfo::availablePorts();

    int porty = available_port.size();
    QString message = QString::number(porty) + (porty == 1 ? " port is ready to use" : " ports are ready to use");

    // wyświetlamy wiadomość z informacją ile znaleźliwśmy urządzeń gotowych do pracy
    ui->statusBar->showMessage(message,3000);

    // aktualizujemy
    info = NULL;
    ui->port->clear();
    ui->port->addItem("NULL");
    for(int i=0;i<porty;i++)
    {
        ui->port->addItem(available_port.at(i).portName());
    }
}

void MainWindow::on_port_currentIndexChanged(int index)
{
    if(port.isOpen()) port.close();
    QString txt = "NULL";
    if (index > 0){
        info = &available_port.at(index-1);
        txt = info->portName();

        port.setPort(available_port.at(index-1));
        if(!port.open(QIODevice::ReadWrite))
            QMessageBox::warning(this,"Device error","Unable to open port.");
    }
    else
        info = NULL;

    ui->statusBar->showMessage("Selected port: " + txt,2000);
}

void MainWindow::refresh()
{
    searchDevices();
}

void MainWindow::on_commandLine_returnPressed()
{
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    addTextToConsole(ui->commandLine->text(),true);
    ui->commandLine->clear();
}


void MainWindow::on_enterButton_clicked() //wciśnięcie entera
{
    on_commandLine_returnPressed();
}

void MainWindow::addTextToConsole(QString msg, bool sender)
{
    if(msg.isEmpty()) return;

    // komendy działające na form, ale nie na port
    if(msg == "/clear") { //czyszczenie okna serial portu w formie
        ui->output->clear();
        return;
    }

    // dodawanie tekstu do konsoli
    QString line = (sender ? "<user>: " : "<device>: ");
    line += msg;
    if (sender) line += "\n"; //stringi z arduino zawsze mają enter
    ui->output->setPlainText(ui->output->toPlainText() + line);

    // auto scroll
    QScrollBar *scroll = ui->output->verticalScrollBar();
    scroll->setValue(scroll->maximum());

    // wysyłanie wiadomości do urządzenia
    if(sender && msg!="error") send(msg+"$"); //każda wiadomość zakończy się dolarem
}

void MainWindow::send(QString msg) //wyslij wiadomość na serial port
{
    if(port.isOpen()) {
        port.write(msg.toStdString().c_str());
        port.waitForBytesWritten(-1); //??? czekaj w nieskonczonosć???
    }

    // spodziewamy się odpowiedzi więc odbieramy dane
    receive();
}


void MainWindow::receive() //odbierz wiadomość z serial portu
{
    // czekamy 5 sekund na odpowiedź
    if(port.waitForReadyRead(10000)) {
        QByteArray r_data = port.readAll();


        // sprawdzamy czy nie dojdą żadne nowe dane w 10ms
        while(port.waitForReadyRead(10)) {
            r_data += port.readAll();
        }

        QString str(r_data);
        str.remove("$");

        addTextToConsole(str);
        QStr_msgFromSerial = str;
    }
}

//dodawania wiadmomości do consoli websocketów
void MainWindow::addTextToWebsocketConsole(QString msg)
{
    ui->websocket_debug->setPlainText(ui->websocket_debug->toPlainText() + msg);

    // auto scroll
    QScrollBar *scroll_websct = ui->websocket_debug->verticalScrollBar();
    scroll_websct->setValue(scroll_websct->maximum());
}

///////////////////websockets
void MainWindow::onNewConnection() //nowe połączenia
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    //jeżeli socket dostanie wiadomość, to odpalamy metody przetwarzająca ją
    //sygnał textMessageReceived emituje QStringa do processMessage
    connect(pSocket, &QWebSocket::textMessageReceived, this, &MainWindow::processMessage);

    //a jak disconnect, to disconnect
    connect(pSocket, &QWebSocket::disconnected, this, &MainWindow::socketDisconnected);

    m_clients << pSocket;

    addTextToWebsocketConsole("New connection\n");
}

//przetwarzanie wiadomośći otrzymanej przez websockety
void MainWindow::processMessage(QString QStr_messageToProcess)
{
    //QWebSocket *pSender = qobject_cast<QWebSocket *>(sender()); // !! unused variable !! wykomentować?
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    // if (pClient) //!!!zrozumieć te mechanizmy!!!
    //if (pClient != pSender) don't echo message back to sender

    //wiadomość pójdzie tylko do tego kto ją przysłał
    if (pClient && QStr_messageToProcess == "keepConnected")
        sendWsMsgToSite(pClient, "connectionOnline", "");
    else if (QStr_messageToProcess == "new" || QStr_messageToProcess.left(4) == "move")
    {
        if (b_blokada_zapytan == false) //jeżeli chenard nie przetwarza żadnego zapytania
        {  //!!! program gdzieś musi stackować zapytania których nie może jeszcze przetworzyć
            b_blokada_zapytan = true;
            if (QStr_messageToProcess.left(4) == "move") QStr_movementMade = QStr_messageToProcess.mid(5);
            QStr_chenardQuestion = QStr_messageToProcess; //string do tcp będzie przekazana przez globalną zmienną

            doConnect(); //łączy i rozłącza z tcp za każdym razem gdy jest wiadomość do przesłania
        }
        else addTextToWebsocketConsole("Error! Previous msg hasn't been processed.\n");
    }
    else if (QStr_messageToProcess == "OK 1\n") //jeżeli chenard wykonał ruch prawidłowo
    {
        QStr_chenardQuestion = "status"; //zapytaj się tcp o stan gry poprzez globalną zmienną
        addTextToWebsocketConsole("Send to tcp: status\n");
        doConnect(); //wykonaj drugie zapytanie tcp
    }
    else
    {
        Q_FOREACH (QWebSocket *pClient, m_clients) //dla każdego klienta wykonaj
        { //!!! dla każdego gracza mam 1 komunikat w formie. wywalić foreach dla wiadomości, albo zrobić liczniki
            //!!! sprawdzić czy gracze nie dostają komunikatów gracza przeciwnego
            if (QStr_messageToProcess.left(17) == "white_player_name")
            {
                QStr_nameWhite = QStr_messageToProcess.mid(18); //zapamiętaj imię białego gracza
                sendWsMsgToSite(pClient, "new_white ", QStr_nameWhite); //wyślij do websocketowców nową nazwę białego
            }
            else if (QStr_messageToProcess.left(17) == "black_player_name")
            {
                QStr_nameBlack = QStr_messageToProcess.mid(18); //zapamiętaj imię czarnego gracza
                sendWsMsgToSite(pClient, "new_black ", QStr_nameBlack); //wyślij do websocketowców nową nazwę czarnego
            }
            else if (QStr_messageToProcess.left(10) == "whose_turn")
            {
                QStr_whoseTurn = QStr_messageToProcess.mid(11); //zapamiętaj czyja jest tura
                sendWsMsgToSite(pClient, "whose_turn ", QStr_whoseTurn); //wyślij do websocketowców info o turze
            }
            else if (QStr_messageToProcess == "OK\n") //jeżeli chenard zaczął nową grę
            {
                sendWsMsgToSite(pClient, "new_game", "");
                //ui->czas_bialego->setText(); !!dorobić
                b_blokada_zapytan = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QStr_messageToProcess.left(1) == "*") //gra w toku
            {
                //podaj na stronę info o tym jaki ruch został wykonany
                sendWsMsgToSite(pClient, "game_in_progress ", QStr_movementMade);
                b_blokada_zapytan = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QStr_messageToProcess.left(3) == "1-0") //biały wygrał
            {
                sendWsMsgToSite(pClient, "white_won", "");
                b_blokada_zapytan = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QStr_messageToProcess.left(3) == "0-1") //czarny wygrał
            {
                sendWsMsgToSite(pClient, "black_won", "");
                b_blokada_zapytan = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QStr_messageToProcess.left(7) == "1/2-1/2") //remis
            {
                sendWsMsgToSite(pClient, "draw", "");
                b_blokada_zapytan = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QStr_messageToProcess.left(5) == "check") //czy ta wiadomośc ma ich do wszystkich?
            {
                if (QStr_messageToProcess.mid(6) == "white_player")
                    sendWsMsgToSite(pClient, "checked_wp_is ", QStr_nameWhite);
                else if (QStr_messageToProcess.mid(6) == "black_player")
                    sendWsMsgToSite(pClient, "checked_bp_is ", QStr_nameBlack);
                else if (QStr_messageToProcess.mid(6) == "whose_turn")
                    sendWsMsgToSite(pClient, "checked_wt_is ", QStr_whoseTurn);
            }
            else if (QStr_messageToProcess.left(8) == "BAD_MOVE")
            {
                QStr_movementMade = QStr_messageToProcess.mid(9);
                QStr_movementMade = QStr_movementMade.simplified(); //wywal białe znaki
                sendWsMsgToSite(pClient, "BAD_MOVE ", QStr_movementMade);
                b_blokada_zapytan = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else //jeżeli chenard da inną odpowiedź (nie powinien)
            {
                sendWsMsgToSite(pClient, "Error! Wrong msg from tcp: ", QStr_messageToProcess);
                b_blokada_zapytan = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
        }

        //send(message+"$"); //wyślij tą wiadmość na serial port

        /* if (pClient == pSender) //jeżeli wiadomośc wpadła od klienta (tj.z sieci)
        {
            addTextToWebsocketConsole("pClient == pSender\n");
            addTextToWebsocketConsole("Received from web: " + QStr_messageToProcess + "\n");

            if(QStr_messageToProcess == "new" || QStr_messageToProcess.left(4) == "move") //jeżeli są to wiadomości dla tcp
            {
                QStr_chenardQuestion = QStr_messageToProcess; //string do tcp będzie przekazana przez globalną zmienną
                doConnect(); //łączy i rozłącza z tcp za każdym razem gdy jest wiadomość do przesłania
            }
            else //wiadomości o stanie stołu
            {
                if (QStr_messageToProcess.left(17) == "white_player_name")
                {
                    QStr_nameWhite = QStr_messageToProcess.mid(18);
                    pClient->sendTextMessage("new_white " + QStr_nameWhite); //wyślij do websocketowców nową nazwę białego
                    addTextToWebsocketConsole("Send to web: new_white " + QStr_nameWhite + "\n");
                }
                else if (QStr_messageToProcess.left(17) == "black_player_name")
                {
                    QStr_nameBlack = QStr_messageToProcess.mid(18);
                    pClient->sendTextMessage("new_black " +QStr_nameBlack); //wyślij do websocketowców nową nazwę czarnego
                    addTextToWebsocketConsole("Send to web: new_black " + QStr_nameBlack + "\n");
                }
                else addTextToWebsocketConsole("ERROR UNKNOW MESSAGE!\n");
            }
        }

        if (pClient != pSender) //jeżeli wiadomość jest wygenerowana przez serwer i leci na stronę
        {
            addTextToWebsocketConsole("pClient != pSender\n");
            if (QStr_messageToProcess == "OK\n") //jeżeli chenard zaczął nową grę
            {
                pClient->sendTextMessage("new_game");// wyślij websocketem odpowiedź z tcp na stronę
                addTextToWebsocketConsole("Send to web: new_game\n");
            }
            else if (QStr_messageToProcess == "OK 1\n") //jeżeli chenard wykonał ruch prawidłowo
            {
                QStr_chenardQuestion = "status"; //zapytaj się tcp o stan gry
                addTextToWebsocketConsole("Send to tcp: status\n");
                doConnect(); //wykonaj drugie zapytanie tcp
            }
            else
            {
                if (QStr_messageToProcess.left(1) == "*") //gra w toku
                {
                    pClient->sendTextMessage("game_in_progress");
                    addTextToWebsocketConsole("Send to web: game_in_progress\n\n");
                }
                else if (QStr_messageToProcess.left(3) == "1-0") //biały wygrał
                {
                    pClient->sendTextMessage("white_won");
                    addTextToWebsocketConsole("Send to web: white_won\n\n");
                }
                else if (QStr_messageToProcess.left(3) == "0-1") //czarny wygrał
                {
                    pClient->sendTextMessage("black_won");
                    addTextToWebsocketConsole("Send to web: black_won\n\n");
                }
                else if (QStr_messageToProcess.left(7) == "1/2-1/2") //remis
                {
                    pClient->sendTextMessage("draw");
                    addTextToWebsocketConsole("Send to web: draw\n\n");
                }
                else //jeżeli chenard da inną odpowiedź
                {
                    pClient->sendTextMessage("error");
                    addTextToWebsocketConsole("Send to web: error\n\n");
                }// wszystko to powyżej da się ładnie zapakować w 2 funkcje
            }
        }
    }*/
    }
}

void MainWindow::socketDisconnected() //rozłączanie websocketa
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
        addTextToWebsocketConsole("Disconnected\n");
    }
}

void MainWindow::sendWsMsgToSite(QWebSocket* pClient, QString QStr_command, QString QStr_prcessedMsg)
{
    pClient->sendTextMessage(QStr_command + QStr_prcessedMsg);
    addTextToWebsocketConsole("Send to web: " + QStr_command + QStr_prcessedMsg+ "\n");
}

////////////////////tcp
void MainWindow::addTextToTcpConsole(QString msg)
{//dodawanie wiadomości do konsoli tcp w formie
    ui->tcp_debug->setPlainText(ui->tcp_debug->toPlainText() + msg);

    // auto scroll
    QScrollBar *scroll_tcp = ui->tcp_debug->verticalScrollBar();
    scroll_tcp->setValue(scroll_tcp->maximum());
}

//główna funkcja tcp, czyli rozmowa z socketem. wpada tutaj argument do sygnału connected() poprzez
//wyemitowanie go w websocketowej funckji (sygnale) processMessage() (?na pewno? czy to jest stary opis?)
void MainWindow::doConnect()
{
    socket = new QTcpSocket(this);

    //definicją sygnałów (tj. funkcji) zajmuję się Qt

    //sygnał i slot musi mieć te same parametry (sygnał przekazuje go automatycznie do slotu)
    connect(socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64))); //to mi raczej zbędne
    connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

    addTextToTcpConsole("connecting...\n");

    // this is not blocking call
    socket->connectToHost("localhost", 22222);

    // we need to wait...
    if(!socket->waitForConnected(5000))
    {
        addTextToTcpConsole("Error:" + socket->errorString() + "\n");
    }
}

void MainWindow::connected()
{
    addTextToTcpConsole("connected...\n");

    QByteArray msg_arrayed;
    addTextToTcpConsole("msg from websocket: " + QStr_chenardQuestion + "\n");

    msg_arrayed.append(QStr_chenardQuestion); //przetworzenie parametru dla funkcji write()
    // send msg to tcp from web. chenard rozumie koniec wiadomości poprzez "\n"
    socket->write(msg_arrayed + "\n"); //write wysyła wiadomość (w bajtach)

    addTextToTcpConsole("wrote to TCP: " + msg_arrayed + "\n");
}

void MainWindow::disconnected()
{
    addTextToTcpConsole("disconnected...\n\n");
}

void MainWindow::bytesWritten(qint64 bytes) //mówi nam ile bajtów wysłaliśmy do tcp
{
    addTextToTcpConsole(QString::number(bytes) + " bytes written...\n"); //! odebrałem "tes written" !
}

void MainWindow::readyRead() //funckja odbierająca odpowiedź z tcp z wcześniej wysłanej wiadmoności
{
    addTextToTcpConsole("reading...\n");

    // read the data from the socket
    QStr_chenardAnswer = socket->readAll(); //w zmiennej zapisz odpowiedź z chenard
    addTextToTcpConsole("tcp answer: " + QStr_chenardAnswer); //pokaż ją w consoli tcp. \n dodaje się sam
    if (QStr_chenardAnswer == "OK 1\n") simplyPieceMoving(QStr_chenardQuestion);
    else emit processMessage(QStr_chenardAnswer);
}
