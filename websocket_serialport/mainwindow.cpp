#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include <QMessageBox>
#include <QTcpSocket>

QT_USE_NAMESPACE

/// globalne zmienne
//serial port
QList <QSerialPortInfo> available_port;     // lista portów pod którymi są urządzenia
const QSerialPortInfo *info;                // obecnie wybrany serial port
QSerialPort usb_port;                       // obecnie otwarty port
QByteArray QByteA_data;                     // macierz niezorganizowanych danych przypływających z usb

//zmienne ogolne
QString QStr_chenardAnswer;                 // odpowiedź z chenard
QString QStr_chenardQuestion;               // zmienna do chenard
QString QStr_cmdForSite;                    // zmienna typu temp, do wysłania na stronę
QString QStr_movementMade;                  // zmienna pamiętająca ruch wykonany ze strony
QString QStr_pieceFrom;                     // z jakiego pola pionek będzie podnoszony
QString QStr_pieceTo;                       // na jakie pole pionek będzie przenoszony
QString QStr_msgFromSerial = "";            // wiadomość która przyszła z serial portu

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


MainWindow::MainWindow(quint16 port, QWidget *parent) : //konstruktor
    QMainWindow(parent),
    m_pWebSocketServer(Q_NULLPTR),
    m_clients(),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    usb_port = new QSerialPort(this);
    info = NULL; //wartośc wskażnika obecnie wybranego portu ustgawiamy na pustą wartość
    searchDevices(); //wyszukujemy obecnie podłączone urządzenia usb

    //łączymy sygnał wciśnięcia przycisku w menu odpowiadającego za żądanie
    //dodatkowego zaktualizowania listy portów z odpowiednim slotem
    connect(ui->action_refreshPorts,SIGNAL(triggered()),this,SLOT(refresh()));

    connect(usb_port,SIGNAL(readyRead()),this,SLOT(readData())); //??? czy to się nei gryzie z readyRead w tcp

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

MainWindow::~MainWindow() //destruktor
{
    if(usb_port->isOpen())
        usb_port->close();

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

    /* addTextToWebsocketConsole("n_literaPola = " + QString::number(n_literaPola) + "\n");
    addTextToWebsocketConsole("n_cyfraPola = " + QString::number(n_cyfraPola) + "\n");
    addTextToWebsocketConsole("a_b_board[m][n] = " + QString::number(a_b_board[n_literaPola][n_cyfraPola]) + "\n"); */
}

void MainWindow::simplyPieceMoving(QString QStr_msgFromSerial) // !! to nie tylko są wiadomość z usb
{
    if (QStr_msgFromSerial.left(4) == "move" && QStr_msgFromSerial.mid(4,1) != "d")
    { // !! zmienić jakoś te stringi by nie robić takiego dziwnego warunku dla pierwszego przejścia
        QStr_pieceFrom = "[" + QStr_msgFromSerial.mid(5,2) + "]f";
        QStr_pieceTo   = "[" + QStr_msgFromSerial.mid(7,2) + "]t";

        findBoardPos(QStr_pieceTo); //znajdź pozycje planszy dla funkcji zbijającej

        if (a_b_board[n_literaPola][n_cyfraPola] == true) //sprawdzanie czy na pole, gdzie pionek idzie nie jest zajęte
        {
            QString QStr_pieceToReject = "[" + QStr_msgFromSerial.mid(7,2) + "]r";
            sendDataToUsb(QStr_pieceToReject, true); //zbijanie pionka
        }
        else if (a_b_board[n_literaPola][n_cyfraPola] == false)
        {
           sendDataToUsb(QStr_pieceFrom, true);
        }
    }

    ///pojedyńcze ruchy przy zbijaniu bierki
    else if (QStr_msgFromSerial.left(8) == "movedToR") sendDataToUsb("openR1", true);
    else if (QStr_msgFromSerial.left(8) == "openedR1") sendDataToUsb("downR", true);
    else if (QStr_msgFromSerial.left(5) == "downR") sendDataToUsb("closeR", true);
    else if (QStr_msgFromSerial.left(7) == "closedR") sendDataToUsb("upR", true);
    else if (QStr_msgFromSerial.left(6) == "armUpR") sendDataToUsb("trashR", true);
    else if (QStr_msgFromSerial.left(8) == "trashedR")
    {
        sendDataToUsb("openR2", true);
        //brak przypadku/reakcji/funkcji dla komunikatu: openedR2. zbędne. tylko dostaję info
        a_b_board[n_literaPola][n_cyfraPola] = false; //miejsce już nie jest zajęte
    }

    ///standardowe ruchy przy przenoszeniu bierki
    else if (QStr_msgFromSerial.left(9) == "movedFrom" && QStr_msgFromSerial.mid(10,2) == QStr_pieceFrom.mid(1,2))
        sendDataToUsb("open1", true);
    else if (QStr_msgFromSerial.left(7) == "opened1") sendDataToUsb("down1", true);
    else if (QStr_msgFromSerial.left(8)  == "armDown1") sendDataToUsb("close1", true);
    else if (QStr_msgFromSerial.left(7)  == "closed1") sendDataToUsb("up1", true);
    else if (QStr_msgFromSerial.left(6)  == "armUp1")
    {
        findBoardPos(QStr_pieceFrom); //znajdź pozycję z której pionka zabrano
        a_b_board[n_literaPola][n_cyfraPola] = false; //miejsce ruszanego pionka jest już puste
        sendDataToUsb(QStr_pieceTo, true);
    }
    else if (QStr_msgFromSerial.left(7) == "movedTo" && QStr_msgFromSerial.mid(8,2) == QStr_pieceTo.mid(1,2))
        sendDataToUsb("down2", true);
    else if (QStr_msgFromSerial.left(8)  == "armDown2") sendDataToUsb("open2", true);
    else if (QStr_msgFromSerial.left(7)  == "opened2")
    {
        findBoardPos(QStr_pieceTo); //znajdź pozycję na której znalazł się przeniesiony pionek
        a_b_board[n_literaPola][n_cyfraPola] = true; //nowe miejsce ruszpnego pionka jest już teraz zajęte
        sendDataToUsb("up2", true);
    }
    else if (QStr_msgFromSerial.left(6)  == "armUp2")
    {
        QStr_msgFromSerial = ""; //czyścimy na wszelki wypadek. raczej to zbędne i inaczej wypadałoby to wykonać
        addTextToUsbConsole("-End of move sequence- \n",false);
        emit processMessage("OK 1\n");
    }
    else
    {
        addTextToUsbConsole("error:", true);

        // dodawanie tekstu do konsoli
        QString line = QStr_msgFromSerial + "\n";
        ui->output->setPlainText(ui->output->toPlainText() + line);

        // auto scroll
        QScrollBar *scroll = ui->output->verticalScrollBar();
        scroll->setValue(scroll->maximum());
    }
    QStr_msgFromSerial = ""; // !! spróbować zmienić sprawdzanie na clear() / =empty
}

// aktualizowanie listy z urządzeniami
//Tutaj wyszukujemy obecnie podłączone urządzenia usb i „wrzucamy” je do wcześniej przygotowanej listy
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
        ui->port->addItem(available_port.at(i).portName()); //wypełnianie combo boxa portami
    }
}

//Slot jest aktywowany po zmianie wartości przez użytkownika w combo box’ie. Ustawia on wskaźnik
//na nowy obiekt (lub nic) i wyświetla odpowiednią informację w pasku statusu
void MainWindow::on_port_currentIndexChanged(int index) //zmiana/wybór portu
{ //bodajze jest ustawianie połączenie z portem automatycznie jak tylko zostanie on wybrany
    if(usb_port->isOpen()) usb_port->close();
    QString txt = "NULL";
    if (index > 0){
        info = &available_port.at(index-1);
        txt = info->portName();

        //funkcja setPort() dziedziczy wszystkie atrybuty portu typu BaudRate, DataBits, Parity itd.
        usb_port->setPort(available_port.at(index-1)); // ???czy tu ju jest port otwarty/polaczony???
        if(!usb_port->open(QIODevice::ReadWrite)) //jezeli port nie jest otwarty
            QMessageBox::warning(this,"Device error","Unable to open port."); //wyrzuć error
        /*else //a jeżeli jest port otwarty, to łap sygnay z portu i wrzucaj do slotu
            connect(port,SIGNAL(readyRead()),this,SLOT(readData())); //??? czy to się nei gryzie z readyRead w tcp
    */
    }
    else
        //wskaźnik czyszczony, by nie wskazywał wcześniejszych informacji ze wskaźnika z pamięci
        info = NULL;

    ui->statusBar->showMessage("Połączono z portem: " + txt,2000);
}

void MainWindow::refresh()
{
    searchDevices();
}

void MainWindow::on_commandLine_returnPressed() //wciśnięcie przycisku entera
{
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    sendDataToUsb(ui->commandLine->text(),true); //zczytaj wiadomośc z pola textowe i wyślij na usb
    ui->commandLine->clear(); // wyczyść pole textowe
}


void MainWindow::on_enterButton_clicked() //wciśnięcie entera
{
    on_commandLine_returnPressed(); //odpala tą samą funkcję co przy wciśnięciu przycisku entera
}

void MainWindow::addTextToUsbConsole(QString Qstr_msg, bool sender) //dodawanie komunikatu do konsoli i wysyłanie na usb
{
    if(Qstr_msg.isEmpty()) return; //blokada możliwości wysyłania pustej wiadomości na serial port

    // komendy działające na form, ale nie na port
    if(Qstr_msg == "/clear") { //czyszczenie okna serial portu w formie
        ui->output->clear();
        return;
    }

    // dodawanie tekstu do konsoli
    QString line = (sender ? "<user>: " : "<device>: ");
    line += Qstr_msg;
    if (sender) line += "\n"; //stringi z arduino zawsze mają enter
    ui->output->setPlainText(ui->output->toPlainText() + line);

    // auto scroll
    QScrollBar *scroll = ui->output->verticalScrollBar();
    scroll->setValue(scroll->maximum());

    /*// wysyłanie wiadomości do urządzenia
    // warunek prawdziwy tylko wtedy, gdy wysyłamy dane, a nie odbieramy
    if(sender) sendDataToUsb(Qstr_msg+"$"); //każda wiadomość zakończy się dolarem*/
}

void MainWindow::sendDataToUsb(QString QStr_msg, bool sender) //wyslij wiadomość na serial port
{
    if(usb_port->isOpen() && QStr_msg != "error")
    {
        addTextToUsbConsole(QStr_msg, sender);

        usb_port->write(QStr_msg.toStdString().c_str()); // +'$'   -nie działa z tym
        usb_port->waitForBytesWritten(-1); //??? czekaj w nieskonczonosć??? co to jest
    }
    else addTextToUsbConsole("error", true);
    // spodziewamy się odpowiedzi więc odbieramy dane:
    //receive(); -wykomentowana synchroniczna komunikacja
}

//wykomentowana synchroniczna komunikacja po usb:
/*void MainWindow::receive() //odbierz wiadomość z serial portu
{
    // czekamy 10 sekund na odpowiedź
    if(port.waitForReadyRead(10000)) {
        QByteA_data = port.readAll(); //zapisz w tablicy wszystko co przyszło z usb


        // sprawdzamy czy nie dojdą żadne nowe dane w 10ms
        while(port.waitForReadyRead(10)) {
            QByteA_data += port.readAll(); //składamy tutaj wszystkie dane które przyszły w 1 zmienną
        }

        QString QStr_fullSerialMsg(QByteA_data); //konwersja danych z serial portu na QString
        //usuwamy z odbieranych wiadomości znak końca linii. program nie ma mozliwości odbierania
        //kilka wiadomości naraz (i nie powinien potrzebować tego)
        QStr_fullSerialMsg.remove("$");

        sendDataToUsb(QStr_fullSerialMsg);
        QStr_msgFromSerial = QStr_fullSerialMsg;
    }
}*/

void MainWindow::readData()
{
    QByteA_data = usb_port->readAll(); //zapisz w tablicy wszystko co przyszło z usb
    while(usb_port->waitForReadyRead(100)) // ?? może dać mniej czasu?? będzie to wtedy działało szybciej??
        QByteA_data += usb_port->readAll(); //składamy tutaj wszystkie dane które przyszły w 1 zmienną

    QString QStr_fullSerialMsg(QByteA_data); //tablicę znaków zamienamy na Qstring

    if(QStr_fullSerialMsg.at(0) == '@' && //jeżeli pierwszy znak wiadomosci to @...
            QStr_fullSerialMsg.at(QStr_fullSerialMsg.size()-1) == '$') { //...a ostatni to $...
        QStr_fullSerialMsg.remove('$'); //...to jest to cała wiadomość...
        QStr_fullSerialMsg.remove('@'); //... i pousuwaj te znaki.

        addTextToUsbConsole(QStr_fullSerialMsg + "\n",false);
        simplyPieceMoving(QStr_fullSerialMsg); // !!! dosprawdzić czy to tu ok
        QStr_msgFromSerial = QStr_fullSerialMsg; // !!! dosprawdzić czy to tu ok. można
        //zrobić z tego po prostu globalną zmienna a nie przypisywać

        QByteA_data.clear();
    }
    else addTextToUsbConsole("error: " + QStr_fullSerialMsg,false);
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

        //sendDataToUsb(message+"$"); //wyślij tą wiadmość na serial port

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
    //jeżeli odpowiedź z chenard mówi nam o tym, że właśnie udało się przemieścić w pamięci bierkę...
    if (QStr_chenardAnswer == "OK 1\n")
    {
        addTextToUsbConsole("-Begin move sequence- \n",false);
        simplyPieceMoving(QStr_chenardQuestion); //...to rozpocznij jej przenoszenie...
    }
    else emit processMessage(QStr_chenardAnswer); //...a jak nie, to niech websockety zadecydują co z tym dalej robić.
}

void MainWindow::on_visual_clicked()
{

}
