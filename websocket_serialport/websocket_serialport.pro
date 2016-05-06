QT       += core gui
QT       += serialport
QT       += core websockets
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RobotArmControl
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
