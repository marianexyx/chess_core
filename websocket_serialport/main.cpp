#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow server(1234); // na którym porcie websockety mają nasłuchiwać
    server.show();

    //server.doConnect(); //tcp - ręczne odpalenie komendy wrzucanej do chenard

    return a.exec();
}
