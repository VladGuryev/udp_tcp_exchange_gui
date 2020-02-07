/*****************************************************************************

  C_MainWindow

  Класс главного окна приложения

*****************************************************************************/

#pragma once

#include <QMainWindow>
#include <QThread>

#include "network/C_Server.h"
#include "network/C_Client.h"
#include "C_Logger.h"

/*****************************************************************************
  Macro Definitions
*****************************************************************************/

/*****************************************************************************
  Forward Declarations
*****************************************************************************/

namespace Ui {
class MainWindow;
}

/*****************************************************************************
  Types and Classes Definitions
*****************************************************************************/

/*****************************************************************************
 * Класс главного окна приложения
 *
 * Содержит интерфейс для запуска клиента и сервера
 * - кнопки Start, Stop, Pause, Resume
 * - окно для вывода логов
 */
class C_MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit C_MainWindow( QWidget *parent = nullptr );
    ~C_MainWindow();

private slots:

    // Обработчик нажатия кнопки "startServerBtn"
    void onStartServerBtnClicked();
    // Обработчик нажатия кнопки "startClientBtn"
    void onStartClientBtnClicked();
    // Обработчик сигнала, полученного от класа C_Logger
    void printNewLogLine( const QVariant &a_newLine );
    // Обработчик сигнала выбора типа протокола
    void onProtocolSelected(int a);

private:
    Ui::MainWindow *ui;

    network::C_Client   *m_client       = nullptr;                              // Указатель на клиент
    network::C_Server   *m_server       = nullptr;                              // Указатель на сервер
    QThread             *m_clientThread = nullptr;                         // Указатель на поток клиента
    QThread             *m_serverThread = nullptr;                        // Указатель на поток сервера
    bool                 isServerStartedBtnPressed = false;     // Флаг нажатия кнопки "Start Server"
    bool                 isClientStartedBtnPressed = false;     // Флаг нажатия кнопки "Start Client"
};

/*****************************************************************************
  Functions Prototypes
*****************************************************************************/

/*****************************************************************************
  Variables Deсlarations
*****************************************************************************/

/*****************************************************************************
  Inline Functions Definitions
*****************************************************************************/

