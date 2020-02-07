#-------------------------------------------------
#
# Project created by QtCreator 2020-01-28T10:32:41
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Issue4
TEMPLATE = app
CONFIG += c++14

SOURCES += main.cpp\
    network/C_Client.cpp \
    network/C_Server.cpp \
    network/C_Socket.cpp \
    network/C_SocketFactory.cpp \
    network/C_StreamAnalyzer.cpp \
    network/C_TcpSocket.cpp \
    network/C_UdpSocket.cpp \
    network/utils.cpp \
    C_MainWindow.cpp \
    C_Logger.cpp

HEADERS  += \
    network/C_Client.h \
    network/C_Server.h \
    network/C_Socket.h \
    network/C_SocketFactory.h \
    network/C_StreamAnalyzer.h \
    network/C_TcpSocket.h \
    network/C_UdpSocket.h \
    network/common_types.h \
    network/I_Socket.h \
    network/utils.h \
    C_Logger.h \
    C_MainWindow.h

FORMS    += mainwindow.ui


LIBS += -lws2_32

QMAKE_CXXFLAGS += -O3
