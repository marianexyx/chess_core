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

//zmienne ogolne
QString QS_chenardAnswer;                   // odpowiedź z chenard
QString QS_chenardQuestion;                 // zmienna do chenard
QString QS_cmdForSite;                      // zmienna typu temp, do wysłania na stronę

bool b_promotion_confirmed = false;         //informacja o tym, że chenard potwierdził poprawność wykonania rucu typu promocja

//zamienniki mysqla
QString QS_nameWhite = "Biały";             // nazwa białego- startowo "Biały
QString QS_nameBlack = "Czarny";            // nazwa czarnego- startowo "Czarny"
QString QS_whoseTurn;                       // czyja tura aktualnie - no_turn, white_turn, black_turn

bool b_blokada_zapytan_tcp = false;         // nie przyjmuj zapytań, dopóki poprzednie nie jest zrealizowane

bool a_b_board[8][8] =                      // plansza jako tablica. jedynki to zajęte pola
{{1, 1, 0, 0, 0, 0, 1, 1} ,
{1, 1, 0, 0, 0, 0, 1, 1} ,
{1, 1, 0, 0, 0, 0, 1, 1} ,
{1, 1, 0, 0, 0, 0, 1, 1} ,
{1, 1, 0, 0, 0, 0, 1, 1} ,
{1, 1, 0, 0, 0, 0, 1, 1} ,
{1, 1, 0, 0, 0, 0, 1, 1} ,
{1, 1, 0, 0, 0, 0, 1, 1}};




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
    
    connect(usb_port,SIGNAL(readyRead()),this,SLOT(readUsbData())); //??? czy to się nei gryzie z readyRead w tcp
    
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
//globalne zmienna
int n_cyfraPolaFrom;
int n_literaPolaFrom;
QString QS_literaPolaFrom;
QString QS_pieceFrom;

int n_cyfraPolaTo;
int n_literaPolaTo;
QString QS_literaPolaTo;
QString QS_pieceTo;

QString QS_piecieFromTo;

void MainWindow::findBoardPos(QString QS_piecePositions)
{
    QS_piecieFromTo = QS_piecePositions.mid(5,4); // [xx]
    QS_pieceFrom = "[" + QS_piecePositions.mid(5,2) + "]f";
    QS_pieceTo   = "[" + QS_piecePositions.mid(7,2) + "]t";
    
    QS_literaPolaFrom = QS_piecePositions.mid(5,1);
    QS_literaPolaTo = QS_piecePositions.mid(7,1);
    
    if (QS_literaPolaFrom == "a" || QS_literaPolaFrom == "A") {n_literaPolaFrom = 0; QS_literaPolaFrom = "a";}
    else if (QS_literaPolaFrom == "b" || QS_literaPolaFrom == "B") {n_literaPolaFrom = 1; QS_literaPolaFrom = "b";}
    else if (QS_literaPolaFrom == "c" || QS_literaPolaFrom == "C") {n_literaPolaFrom = 2; QS_literaPolaFrom = "c";}
    else if (QS_literaPolaFrom == "d" || QS_literaPolaFrom == "D") {n_literaPolaFrom = 3; QS_literaPolaFrom = "d";}
    else if (QS_literaPolaFrom == "e" || QS_literaPolaFrom == "E") {n_literaPolaFrom = 4; QS_literaPolaFrom = "e";}
    else if (QS_literaPolaFrom == "f" || QS_literaPolaFrom == "F") {n_literaPolaFrom = 5; QS_literaPolaFrom = "f";}
    else if (QS_literaPolaFrom == "g" || QS_literaPolaFrom == "G") {n_literaPolaFrom = 6; QS_literaPolaFrom = "g";}
    else if (QS_literaPolaFrom == "h" || QS_literaPolaFrom == "H") {n_literaPolaFrom = 7; QS_literaPolaFrom = "h";}
    
    if (QS_literaPolaTo== "a" || QS_literaPolaTo== "A") {n_literaPolaTo = 0; QS_literaPolaTo= "a";}
    else if (QS_literaPolaTo== "b" || QS_literaPolaTo== "B") {n_literaPolaTo = 1; QS_literaPolaTo= "b";}
    else if (QS_literaPolaTo== "c" || QS_literaPolaTo== "C") {n_literaPolaTo = 2; QS_literaPolaTo= "c";}
    else if (QS_literaPolaTo== "d" || QS_literaPolaTo== "D") {n_literaPolaTo = 3; QS_literaPolaTo= "d";}
    else if (QS_literaPolaTo== "e" || QS_literaPolaTo== "E") {n_literaPolaTo = 4; QS_literaPolaTo= "e";}
    else if (QS_literaPolaTo== "f" || QS_literaPolaTo== "F") {n_literaPolaTo = 5; QS_literaPolaTo= "f";}
    else if (QS_literaPolaTo== "g" || QS_literaPolaTo== "G") {n_literaPolaTo = 6; QS_literaPolaTo= "g";}
    else if (QS_literaPolaTo== "h" || QS_literaPolaTo== "H") {n_literaPolaTo = 7; QS_literaPolaTo= "h";}
    
    n_cyfraPolaFrom = QS_piecePositions.mid(6,1).toInt() - 1;
    n_cyfraPolaTo = QS_piecePositions.mid(8,1).toInt() - 1;
}

void MainWindow::normalPieceMoving(QString QS_normalMove) //sekwencja normalnego przemieszczanie bierki
{  
    //pierwszy ruch wykonywany z poziomu odpowiedzi z tcp
    if (QS_normalMove.left(11) == "n_movedFrom") // && QS_normalMove.mid(14,2) == QS_pieceFrom.mid(1,2) -ew. dodatkowe zabezpieczenie (może być błąd)
        sendDataToUsb("n_open1", true); // odpowiedź na pierwszy ruch
    else if (QS_normalMove.left(9) == "n_opened1") sendDataToUsb("n_down1", true);
    else if (QS_normalMove.left(10)  == "n_armDown1") sendDataToUsb("n_close1", true);
    else if (QS_normalMove.left(9)  == "n_closed1") sendDataToUsb("n_up1", true);
    else if (QS_normalMove.left(8)  == "n_armUp1")
    {
        a_b_board[n_literaPolaFrom][n_cyfraPolaFrom] = false; //miejsce ruszanego pionka jest już puste
        sendDataToUsb("n_" + QS_pieceTo, true);
    }
    else if (QS_normalMove.left(9) == "n_movedTo") //  && QS_normalMove.mid(8,2) == QS_pieceTo.mid(1,2) -ew. dodatkowe zabezpieczenie (może być błąd)
        sendDataToUsb("n_down2", true);
    else if (QS_normalMove.left(10)  == "n_armDown2") sendDataToUsb("n_open2", true);
    else if (QS_normalMove.left(9)  == "n_opened2")
    {
        a_b_board[n_literaPolaTo][n_cyfraPolaTo] = true; //nowe miejsce ruszpnego pionka jest już teraz zajęte
        sendDataToUsb("n_up2", true);
    }
    else if (QS_normalMove.left(8)  == "n_armUp2")
    {
        QS_normalMove = ""; //czyścimy na wszelki wypadek. raczej to zbędne i inaczej wypadałoby to wykonać
        addTextToUsbConsole("-End of move sequence- \n",false);
        emit processWebsocketMsg("OK 1\n"); // ! to jest bardzo mylący zapis
    }
    else
    {
        addTextToUsbConsole("error01: ", true);

        // dodawanie tekstu do konsoli
        QString line = QS_normalMove + "\n";
        ui->output->setPlainText(ui->output->toPlainText() + line);

        // auto scroll
        QScrollBar *scroll = ui->output->verticalScrollBar();
        scroll->setValue(scroll->maximum());
    }

    QS_normalMove = ""; //czyszcenie !!! spróbować zmienić sprawdzanie na clear() i =empty
}

QString QS_pieceToReject; //globalna zmienna: bierka która ma być zbita
bool MainWindow::removeStatements() // funkcje do sprawdzania czy bijemy bierkę
{
    if (a_b_board[n_literaPolaTo][n_cyfraPolaTo] == true) //sprawdzanie czy na pole, gdzie bierka idzie nie jest zajęte (nieprawdziwe dla enpassant)
    {
        QS_pieceToReject = "r_" + QS_pieceTo;
        return 1;
    }
    else return 0;
}

void MainWindow::removePieceSequence(QString QS_msgFromSerial) //sekwencja ruchów przy zbijaniu bierki
{
    //pierwszy ruch wykonywany z poziomu odpowiedzi z tcp
    if (QS_msgFromSerial.left(9) == "r_movedTo") sendDataToUsb("r_open1", true);
    else if (QS_msgFromSerial.left(9) == "r_opened1") sendDataToUsb("r_down", true);
    else if (QS_msgFromSerial.left(6) == "r_down") sendDataToUsb("r_close", true);
    else if (QS_msgFromSerial.left(8) == "r_closed") sendDataToUsb("r_up", true);
    else if (QS_msgFromSerial.left(7) == "r_armUp") sendDataToUsb("r_trash", true);
    else if (QS_msgFromSerial.left(9) == "r_trashed")
    {
        sendDataToUsb("r_open2", true);
        a_b_board[n_literaPolaTo][n_cyfraPolaTo] = false; //miejsce już nie jest zajęte
    }
    else if (QS_msgFromSerial.left(9) == "r_opened2") //zakończono usuwanie bierki...
    {
        sendDataToUsb("n_" + QS_pieceFrom, true); //...rozpocznij normalne przenoszenie
    }
    else addTextToWebsocketConsole("Error02!: Wrong command /n");
}


//globalne zmienne do roszady:
QString QS_kingPosF;                     // skąd najpierw idzie król
QString QS_kingPosT;                     // dokąd najpierw idzie król
QString QS_rookPosF;                     // skąd później idzie wieża
QString QS_rookPosT;                     // dokąd później idzie wieża
bool MainWindow::castlingStatements(QString QS_possibleCastlingVal) // sprawdzanie czy roszadę czas wykonać
{
    if (QS_possibleCastlingVal == "e1c1" || QS_possibleCastlingVal == "e1C1" || QS_possibleCastlingVal == "E1c1" || QS_possibleCastlingVal == "E1C1" ||
            QS_possibleCastlingVal == "e1g1" || QS_possibleCastlingVal == "e1G1" || QS_possibleCastlingVal == "E1g1" || QS_possibleCastlingVal == "E1G1" ||
            QS_possibleCastlingVal == "e8c8" || QS_possibleCastlingVal == "e8C8" || QS_possibleCastlingVal == "E8c8" || QS_possibleCastlingVal == "E8C8" ||
            QS_possibleCastlingVal == "e8g8" || QS_possibleCastlingVal == "e8G8" || QS_possibleCastlingVal == "E8g8" || QS_possibleCastlingVal == "E8G8" )
    {
        //ustawiane skąd-dokąd przenoszony będzie król podczas roszady
        QS_kingPosF = "c_" + QS_pieceFrom + "K"; //jest to info dla arduino, że mamy do czynienia z roszadą
        QS_kingPosT = "c_" + QS_pieceTo + "K";

        //ustawiane skąd-dokąd przenoszona będzie wieża podczas roszady
        if (QS_literaPolaTo == "c")
        {
            QS_rookPosF = "c_[a";
            QS_rookPosT = "c_[d";
        }
        else
        {
            QS_rookPosF = "c_[h";
            QS_rookPosT = "c_[f";
        }
        if(n_cyfraPolaTo == 1)
        {
            QS_rookPosF += "1]fR";
            QS_rookPosT += "1]tR";
        }
        else
        {
            QS_rookPosF += "8]fR";
            QS_rookPosT += "8]tR";
        }
        return 1;
    }
    else
        return 0;
}

void MainWindow::castlingMovingSequence(QString QS_msgFromSerial)
{
    if (QS_msgFromSerial.left(12) == "c_movedFromK") sendDataToUsb("c_open1", true);
    else if (QS_msgFromSerial.left(9) == "c_opened1") sendDataToUsb("c_down1", true);
    else if (QS_msgFromSerial.left(10) == "c_armDown1") sendDataToUsb("c_close1", true);
    else if (QS_msgFromSerial.left(9) == "c_closed1") sendDataToUsb("c_up1", true);
    else if (QS_msgFromSerial.left(8) == "c_armUp1") sendDataToUsb(QS_kingPosT, true);
    else if (QS_msgFromSerial.left(10) == "c_movedToK") sendDataToUsb("c_down2", true);
    else if (QS_msgFromSerial.left(10) == "c_armDown2") sendDataToUsb("c_open2", true);
    else if (QS_msgFromSerial.left(9) == "c_opened2") sendDataToUsb("c_up2", true);
    else if (QS_msgFromSerial.left(8) == "c_armUp2") sendDataToUsb(QS_rookPosF, true);
    else if (QS_msgFromSerial.left(12) == "c_movedFromR") sendDataToUsb("c_down3", true);
    else if (QS_msgFromSerial.left(10) == "c_armDown3") sendDataToUsb("c_close2", true);
    else if (QS_msgFromSerial.left(9) == "c_closed2") sendDataToUsb("c_up3", true);
    else if (QS_msgFromSerial.left(8) == "c_armUp3") sendDataToUsb(QS_rookPosT, true);
    else if (QS_msgFromSerial.left(10) == "c_movedToR") sendDataToUsb("c_down4", true);
    else if (QS_msgFromSerial.left(10) == "c_armDown4") sendDataToUsb("c_open3", true);
    else if (QS_msgFromSerial.left(9) == "c_opened3") sendDataToUsb("c_up4", true);
    else if (QS_msgFromSerial.left(8) == "c_armUp4")
    {
        // zmiana miejsc na tablicy pól zajętych
        a_b_board[n_literaPolaFrom][n_cyfraPolaFrom] = false;
        a_b_board[n_literaPolaTo][n_cyfraPolaTo] = true;

        //czyszczenie zmiennych, gdyby przez przypadek coś kiedyś chciało ich bez przypisywania używać
        ///!!! sprawdzić czy to tu tak ok
        QS_kingPosF.clear();
        QS_kingPosT.clear();
        QS_rookPosF.clear();
        QS_rookPosT.clear();

        addTextToUsbConsole("-End of move sequence- \n",true);
        emit processWebsocketMsg("OK 1\n");
    }
}

bool b_test_enpassant = false;	//globalna zmienna odpowiadająca za wykonywanie ruchu enpassant
bool MainWindow::testEnpassant() //sprawdzanie możliwości wystąpienia enpassant
{
    //sprawdzmay czy zapytanie/ruch może być biciem w przelocie
    if (abs(n_literaPolaFrom - n_literaPolaTo) == 1 && //jeżeli pionek nie idzie po prostej (tj. po ukosie o 1 pole)...
            ((QS_whoseTurn == "white_turn" && n_cyfraPolaFrom == 5 && n_cyfraPolaTo == 6) || //... i jeżeli bije biały...
             (QS_whoseTurn == "black_turn" && n_cyfraPolaFrom == 4 && n_cyfraPolaTo == 3)) && //... lub czarny...
            a_b_board[n_literaPolaTo][n_cyfraPolaTo] == false) //... i pole na które idzie nie jest zajete (inaczej byłoby zwykłe bicie)
    {
        QS_chenardQuestion = "test " + QS_piecieFromTo;
        b_test_enpassant = true;
        doTcpConnect();
        return 1;
    }
    else return 0;
}

QString QS_enpassantToReject;   //globalna zmienna: pozycja pionka bitego w enpassant
void MainWindow::enpassantMovingSequence()
{
    int n_enpassantCyfraPos;  //cyfra pionka bitego

    if (QS_whoseTurn == "white_turn") n_enpassantCyfraPos = n_cyfraPolaTo-1; //pozycja zbijanego czarnego pionka przez pionka białego w jego turze
    else if (QS_whoseTurn == "black_turn") n_enpassantCyfraPos = n_cyfraPolaTo+1; //pozycja zbijanego białego pionka przez pionka czarnego w jego turze
    else
    {
        addTextToWebsocketConsole("Error03!: Wrong turn in enpassant statement /n");
        return;
    }
    //wyjątkowo zbijany będzie gdzie indziej niż lądujący
    QS_enpassantToReject = "r_[" + QString::number(n_literaPolaTo) + QString::number(n_enpassantCyfraPos) + "]";
    b_test_enpassant = false; //wyłączenie, by nie zapętlać testów w odpowiedzi tcp
    sendDataToUsb(QS_enpassantToReject, true); //rozpocznij enpassant jeżeli test się powiódł
}

bool b_test_promotion = false;	//globalna zmienna odpowiadająca za wykonywanie ruchu promocji
QString QS_futurePromote = "";  //globalna zmienna pamiętająca jakie było zapytanie o ruch typu promocja
bool MainWindow::testPromotion() //sprawdzanie możliwości wystąpienia enpassant
{
    if (((QS_whoseTurn == "white_turn" &&  n_cyfraPolaFrom == 7 && n_cyfraPolaTo == 8) ||  //jeżelii bierka chce isc z pola 2 na 1 w turze bialego...
         (QS_whoseTurn == "black_turn" && n_cyfraPolaFrom == 2 && n_cyfraPolaTo == 1)) &&   //...lub z pola 7 na 8 w turze czarnego...
            abs(n_literaPolaFrom - n_literaPolaTo) <= 1)          //...i ruch odbywa się max o 1 pole odległości liter po ukosie
    {
        QS_futurePromote = QS_piecieFromTo; //póki nie wiadomo na co promujemy to zapamiętaj zapytanie o ten ruch
        QS_chenardQuestion = "test " + QS_piecieFromTo + "q"; //test który pójdzie na chenard
        b_test_promotion = true; //zaczynamy sprawdzanie czy dany ruch jest ruchem typu promocja
        doTcpConnect();
        return 1;
    }
    else return 0;
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
            QMessageBox::warning(this,"Device error04","Unable to open port."); //wyrzuć error
        /*else //a jeżeli jest port otwarty, to łap sygnay z portu i wrzucaj do slotu
            connect(port,SIGNAL(readyRead()),this,SLOT(readUsbData())); //??? czy to się nei gryzie z readyRead w tcp
        */
    }
    else
        //wskaźnik czyszczony, by nie wskazywał wcześniejszych informacji ze wskaźnika z pamięci
        info = NULL;

    ui->statusBar->showMessage("Połączono z portem: " + txt,5000);
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

void MainWindow::addTextToUsbConsole(QString QS_msg, bool sender) //dodawanie komunikatu do konsoli i wysyłanie na usb
{
    if(QS_msg.isEmpty()) return; //blokada możliwości wysyłania pustej wiadomości na serial port

    // komendy działające na form, ale nie na port
    if(QS_msg == "/clear") { //czyszczenie okna serial portu w formie
        ui->output->clear();
        return;
    }

    // dodawanie tekstu do konsoli
    QString line = (sender ? "< user > : " : "<device>: ");
    line += QS_msg;
    if (sender) line += "\n"; //stringi z arduino zawsze mają enter
    ui->output->setPlainText(ui->output->toPlainText() + line);

    // auto scroll
    QScrollBar *scroll = ui->output->verticalScrollBar();
    scroll->setValue(scroll->maximum());
}

void MainWindow::sendDataToUsb(QString QS_msg, bool sender) //wyslij wiadomość na serial port
{
    if(usb_port->isOpen() && QS_msg != "error05")
    {
        addTextToUsbConsole(QS_msg, sender);

        usb_port->write(QS_msg.toStdString().c_str()); // +'$'   -nie działa z tym
        usb_port->waitForBytesWritten(-1); //??? czekaj w nieskonczonosć??? co to jest
    }
    else addTextToUsbConsole("error06", true);
}

void MainWindow::readUsbData()
{
    QByteArray QByteA_data; // tablica niezorganizowanych danych przypływających z usb
    QByteA_data = usb_port->readAll(); //zapisz w tablicy wszystko co przyszło z usb
    while(usb_port->waitForReadyRead(100)) // ?? może dać mniej czasu?? będzie to wtedy działało szybciej??
        QByteA_data += usb_port->readAll(); //składamy tutaj wszystkie dane które przyszły w 1 zmienną

    QString QS_fullSerialMsg(QByteA_data); //tablicę znaków zamienamy na Qstring

    if(QS_fullSerialMsg.at(0) == '@' && //jeżeli pierwszy znak wiadomosci to @...
            QS_fullSerialMsg.at(QS_fullSerialMsg.size()-1) == '$') //...a ostatni to $...
    {
        QS_fullSerialMsg.remove('$'); //...to jest to cała wiadomość...
        QS_fullSerialMsg.remove('@'); //... i pousuwaj te znaki.

        addTextToUsbConsole(QS_fullSerialMsg + "\n", false);

        if (QS_fullSerialMsg.left(2) == "n_") normalPieceMoving(QS_fullSerialMsg);
        else if (QS_fullSerialMsg.left(2) == "r_") removePieceSequence(QS_fullSerialMsg);
        else if (QS_fullSerialMsg.left(2) == "c_") castlingMovingSequence(QS_fullSerialMsg);

        QByteA_data.clear();
    }
    else addTextToUsbConsole("error07: " + QS_fullSerialMsg, false);
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
    //!!! zacięcie czasami przy nowymn połączeniu !!!
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    //jeżeli socket dostanie wiadomość, to odpalamy metody przetwarzająca ją
    //sygnał textMessageReceived emituje QStringa do processWebsocketMsg
    connect(pSocket, &QWebSocket::textMessageReceived, this, &MainWindow::processWebsocketMsg);

    //a jak disconnect, to disconnect
    connect(pSocket, &QWebSocket::disconnected, this, &MainWindow::socketDisconnected);

    m_clients << pSocket;

    addTextToWebsocketConsole("New connection \n");
}

//przetwarzanie wiadomośći otrzymanej przez websockety
void MainWindow::processWebsocketMsg(QString QS_WbstMsgToProcess)
{
    //QWebSocket *pSender = qobject_cast<QWebSocket *>(sender()); // !! unused variable !! wykomentować?
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    // if (pClient) //!!! zrozumieć te mechanizmy
    //if (pClient != pSender) don't echo message back to sender

    //wiadomość pójdzie tylko do tego kto ją przysłał
    if (pClient && QS_WbstMsgToProcess == "keepConnected")
        sendWsMsgToSite(pClient, "connectionOnline", "");

    else if (QS_WbstMsgToProcess.left(4) == "move") //jeżeli mamy doczynienia z żądaniem ruchu
    {
        if (b_blokada_zapytan_tcp == false) //jeżeli chenard nie przetwarza żadnego zapytania
            //!!! program gdzieś musi stackować zapytania których nie może jeszcze przetworzyć
        {
            findBoardPos(QS_WbstMsgToProcess); //oblicz wszystkie pozycje bierek, także dla ruchów specjalnych

            if(testPromotion()) return; //jeżeli możemy mieć doczynienia z promocją, to sprawdź tą opcję i przerwij
            else if (testEnpassant()) return; //jeżeli możemy mieć doczynienia z enpassant, to sprawdź tą opcję i przerwij
            else //a jeżeli było to każde inne rządzanie ruchu, to wykonuj przemieszczenie bierki...
                //...(do tych ruchów zaliczają się: zwykły ruch, bicie, roszada)
            {
                QS_chenardQuestion = QS_WbstMsgToProcess; //string do tcp będzie przekazana przez globalną zmienną
                addTextToWebsocketConsole("Send normal move to tcp: " + QS_chenardQuestion + "\n");
                doTcpConnect(); //łączy i rozłącza z tcp za każdym razem gdy jest wiadomość do przesłania
            }
        }
        else addTextToWebsocketConsole("Error08! Previous msg hasn't been processed.\n");
    }

    //rozpoczęcie nowej gry
    else if (QS_WbstMsgToProcess == "new")
    {
        if (b_blokada_zapytan_tcp == false) //jeżeli chenard nie przetwarza żadnego zapytania
        {  //!!! program gdzieś musi stackować zapytania których nie może jeszcze przetworzyć
            b_blokada_zapytan_tcp = true;
            if (QS_nameWhite != "Biały" && QS_nameWhite != "Czarny" && QS_nameWhite != "" && QS_nameWhite!= "")
            {
                QS_chenardQuestion = QS_WbstMsgToProcess; //string do tcp będzie przekazana przez globalną zmienną
                doTcpConnect(); //łączy i rozłącza z tcp za każdym razem gdy jest wiadomość do przesłania
            }
            else addTextToWebsocketConsole("Error09! Wrong players names.\n");
        }
        else addTextToWebsocketConsole("Error10! Previous msg hasn't been processed.\n");
    }

    //jeżeli chenard wykonał ruch prawidłowo
    else if (QS_WbstMsgToProcess == "OK 1\n")
    {
        QS_chenardQuestion = "status"; //zapytaj się tcp o stan gry poprzez globalną zmienną
        addTextToWebsocketConsole("Send to tcp: status\n");
        doTcpConnect(); //wykonaj drugie zapytanie tcp
    }

    //promocja pionka
    else if (QS_WbstMsgToProcess.left(10) == "promote_to")
    {
        QS_chenardQuestion = QS_futurePromote + QS_WbstMsgToProcess.mid(11,1); //scal żądanie o ruch z typem promocji
        b_test_promotion = false; //nie testujemy już, bo znamy wynik promocji, plus nie chcemy by testy się zapętliły
        b_promotion_confirmed = true; //w odpowiedzi na chenard ma się wykonać ruch typu promocja, by sprawdzić czy nie ma dodatkowo bicia
        doTcpConnect();
        //dopóki fizycznie nie podmieniam pionków na nowe figury w promocji, to ruch może się odbywać jako normalne przemieszczenie
    }
    else if (QS_WbstMsgToProcess.left(7) == "ILLEGAL") //jeżeli któryś test się nie powiódł
    {
        //warunki ruchów specjalnych się znoszą, dlatego nie trzeba ich implementować. jedynym wyjątkiem jest bicie
        //podczas promocji zawarte w warunku poniżej
        if(b_test_promotion && removeStatements()) //jeżeli dla testu promocji (który się nie powiódł) występuje inny special move (bicie bierki)..
            sendDataToUsb(QS_pieceToReject, true); //...to wykonaj normalne bicie, które potem samo przejdzie w normalny ruch
        else sendDataToUsb("n_" + QS_pieceFrom); //jako że innych przypadków nie trzeba rozpatrywać to każdy innych ruch jest zwykły
    }
    else
    {
        Q_FOREACH (QWebSocket *pClient, m_clients) //dla każdego klienta wykonaj
        //!!! dla każdego gracza mam 1 komunikat w formie. wywalić foreach dla wiadomości, albo zrobić liczniki
        //!!! sprawdzić czy gracze nie dostają komunikatów gracza przeciwnego
        {
            if (QS_WbstMsgToProcess.left(17) == "white_player_name")
            {
                QS_nameWhite = QS_WbstMsgToProcess.mid(18); //zapamiętaj imię białego gracza
                sendWsMsgToSite(pClient, "new_white ", QS_nameWhite); //wyślij do websocketowców nową nazwę białego
            }
            else if (QS_WbstMsgToProcess.left(17) == "black_player_name")
            {
                QS_nameBlack = QS_WbstMsgToProcess.mid(18); //zapamiętaj imię czarnego gracza
                sendWsMsgToSite(pClient, "new_black ", QS_nameBlack); //wyślij do websocketowców nową nazwę czarnego
            }
            else if (QS_WbstMsgToProcess.left(10) == "whose_turn")
            {
                QS_whoseTurn = QS_WbstMsgToProcess.mid(11); //zapamiętaj czyja jest tura
                sendWsMsgToSite(pClient, "whose_turn ", QS_whoseTurn); //wyślij do websocketowców info o turze
            }
            else if (QS_WbstMsgToProcess == "OK\n") //jeżeli chenard zaczął nową grę, albo wykonał poprawny test na promocję
            {
                //jeżeli chenard zaczął nową grę
                sendWsMsgToSite(pClient, "new_game", "");
                //ui->czas_bialego->setText(); !!dorobić
                b_blokada_zapytan_tcp = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QS_WbstMsgToProcess.left(1) == "*") //gra w toku
            {
                //podaj na stronę info o tym jaki ruch został wykonany
                sendWsMsgToSite(pClient, "game_in_progress ", QS_piecieFromTo);
                b_blokada_zapytan_tcp = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QS_WbstMsgToProcess.left(3) == "1-0") //biały wygrał
            {
                sendWsMsgToSite(pClient, "white_won", "");
                b_blokada_zapytan_tcp = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QS_WbstMsgToProcess.left(3) == "0-1") //czarny wygrał
            {
                sendWsMsgToSite(pClient, "black_won", "");
                b_blokada_zapytan_tcp = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QS_WbstMsgToProcess.left(7) == "1/2-1/2") //remis
            {
                sendWsMsgToSite(pClient, "draw", "");
                b_blokada_zapytan_tcp = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QS_WbstMsgToProcess.left(5) == "check") //czy ta wiadomośc ma ich do wszystkich?
            {
                if (QS_WbstMsgToProcess.mid(6) == "white_player")
                    sendWsMsgToSite(pClient, "checked_wp_is ", QS_nameWhite);
                else if (QS_WbstMsgToProcess.mid(6) == "black_player")
                    sendWsMsgToSite(pClient, "checked_bp_is ", QS_nameBlack);
                else if (QS_WbstMsgToProcess.mid(6) == "whose_turn")
                    sendWsMsgToSite(pClient, "checked_wt_is ", QS_whoseTurn);
            }
            else if (QS_WbstMsgToProcess.left(8) == "BAD_MOVE")
            {
                QS_piecieFromTo = QS_WbstMsgToProcess.mid(9);
                QS_piecieFromTo = QS_piecieFromTo.simplified(); //wywal białe znaki
                sendWsMsgToSite(pClient, "BAD_MOVE ", QS_piecieFromTo);
                b_blokada_zapytan_tcp = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
            else if (QS_WbstMsgToProcess.left(7) == "promote_to_what")
            {
                sendWsMsgToSite(pClient, "promote_to_what", "");
            }
            else //jeżeli chenard da inną odpowiedź (nie powinien)
            {
                sendWsMsgToSite(pClient, "Error11! Wrong msg from tcp: ", QS_WbstMsgToProcess);
                b_blokada_zapytan_tcp = false; //chenard przetworzył właśnie wiadomość. uwolnienie blokady zapytań
            }
        }

        //sendDataToUsb(message+"$"); //wyślij tą wiadmość na serial port

        /* if (pClient == pSender) //jeżeli wiadomośc wpadła od klienta (tj.z sieci)
            {
            addTextToWebsocketConsole("pClient == pSender\n");
            addTextToWebsocketConsole("Received from web: " + QS_WbstMsgToProcess + "\n");
            
            if(QS_WbstMsgToProcess == "new" || QS_WbstMsgToProcess.left(4) == "move") //jeżeli są to wiadomości dla tcp
            {
            QS_chenardQuestion = QS_WbstMsgToProcess; //string do tcp będzie przekazana przez globalną zmienną
            doTcpConnect(); //łączy i rozłącza z tcp za każdym razem gdy jest wiadomość do przesłania
            }
            else //wiadomości o stanie stołu
            {
            if (QS_WbstMsgToProcess.left(17) == "white_player_name")
            {
            QS_nameWhite = QS_WbstMsgToProcess.mid(18);
            pClient->sendTextMessage("new_white " + QS_nameWhite); //wyślij do websocketowców nową nazwę białego
            addTextToWebsocketConsole("Send to web: new_white " + QS_nameWhite + "\n");
            }
            else if (QS_WbstMsgToProcess.left(17) == "black_player_name")
            {
            QS_nameBlack = QS_WbstMsgToProcess.mid(18);
            pClient->sendTextMessage("new_black " +QS_nameBlack); //wyślij do websocketowców nową nazwę czarnego
            addTextToWebsocketConsole("Send to web: new_black " + QS_nameBlack + "\n");
            }
            else addTextToWebsocketConsole("ERROR UNKNOW MESSAGE!\n");
            }
            }
            
            if (pClient != pSender) //jeżeli wiadomość jest wygenerowana przez serwer i leci na stronę
            {
            addTextToWebsocketConsole("pClient != pSender\n");
            if (QS_WbstMsgToProcess == "OK\n") //jeżeli chenard zaczął nową grę
            {
            pClient->sendTextMessage("new_game");// wyślij websocketem odpowiedź z tcp na stronę
            addTextToWebsocketConsole("Send to web: new_game\n");
            }
            else if (QS_WbstMsgToProcess == "OK 1\n") //jeżeli chenard wykonał ruch prawidłowo
            {
            QS_chenardQuestion = "status"; //zapytaj się tcp o stan gry
            addTextToWebsocketConsole("Send to tcp: status\n");
            doTcpConnect(); //wykonaj drugie zapytanie tcp
            }
            else
            {
            if (QS_WbstMsgToProcess.left(1) == "*") //gra w toku
            {
            pClient->sendTextMessage("game_in_progress");
            addTextToWebsocketConsole("Send to web: game_in_progress\n\n");
            }
            else if (QS_WbstMsgToProcess.left(3) == "1-0") //biały wygrał
            {
            pClient->sendTextMessage("white_won");
            addTextToWebsocketConsole("Send to web: white_won\n\n");
            }
            else if (QS_WbstMsgToProcess.left(3) == "0-1") //czarny wygrał
            {
            pClient->sendTextMessage("black_won");
            addTextToWebsocketConsole("Send to web: black_won\n\n");
            }
            else if (QS_WbstMsgToProcess.left(7) == "1/2-1/2") //remis
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

// ta funkcja działa tylko z funkcją processWebsocketMsg, bo ma w sobie zmienną pClient
void MainWindow::sendWsMsgToSite(QWebSocket* pClient, QString QS_command, QString QS_processedMsg)
{
    pClient->sendTextMessage(QS_command + QS_processedMsg);
    addTextToWebsocketConsole("Send to web: " + QS_command + QS_processedMsg+ "\n");
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
//wyemitowanie go w websocketowej funckji (sygnale) processWebsocketMsg() (?na pewno? czy to jest stary opis?)
void MainWindow::doTcpConnect()
{
    socket = new QTcpSocket(this);
    
    //definicją sygnałów (tj. funkcji) zajmuje się Qt
    
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
        addTextToTcpConsole("Error12:" + socket->errorString() + "\n");
    }
}

void MainWindow::connected()
{
    addTextToTcpConsole("connected...\n");
    
    QByteArray msg_arrayed;
    addTextToTcpConsole("msg from websocket: " + QS_chenardQuestion + "\n");
    
    msg_arrayed.append(QS_chenardQuestion); //przetworzenie parametru dla funkcji write()
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
    QS_chenardAnswer = socket->readAll(); //w zmiennej zapisz odpowiedź z chenard
    addTextToTcpConsole("tcp answer: " + QS_chenardAnswer); //pokaż ją w consoli tcp. \n dodaje się sam
    
    //jeżeli odpowiedź z chenard mówi nam o tym, że właśnie udało się przemieścić w pamięci bierkę, lub udał się na nią test
    if (QS_chenardAnswer == "OK 1\n")
    {
        addTextToUsbConsole("-Begin move sequence-", true);
        
        if (b_test_promotion) processWebsocketMsg("promote_to_what"); //zapytaj stronę o typ promocji jeżeli test się powiódł
        else if (b_test_enpassant) enpassantMovingSequence(); //jeżeli test na enpassant się powiódł, to rozpocznij ten ruch
        else if (castlingStatements(QS_piecieFromTo)) sendDataToUsb(QS_kingPosF, true); //rozpocznij roszadę jeżeli spełnione są warunki
        else if (removeStatements()) sendDataToUsb(QS_pieceToReject, true); //rozpocznij zbijanie jeżeli spełnione są warunki
        else if (b_promotion_confirmed)
        {
            if (removeStatements()) sendDataToUsb(QS_pieceToReject, true);
            b_promotion_confirmed = false;
        }
        else sendDataToUsb("n_" + QS_pieceFrom, true); //rozpocznij normalny ruch
    }
    //...a jak nie, to niech websockety zadecydują co z tym dalej robić.
    else emit processWebsocketMsg(QS_chenardAnswer); // !! a raczej powinno coś innego! chociażby częściowo
}
