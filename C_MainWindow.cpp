/*****************************************************************************

  C_MainWindow

  Класс главного окна приложения

*****************************************************************************/

#include <QScrollBar>
#include <QComboBox>

#include "C_MainWindow.h"
#include "ui_mainwindow.h"

/*****************************************************************************
  Macro Definitions
*****************************************************************************/

/*****************************************************************************
  Forward Declarations
*****************************************************************************/

/*****************************************************************************
  Types and Classes Definitions
*****************************************************************************/

/*****************************************************************************
  Functions Prototypes
*****************************************************************************/

/*****************************************************************************
  Variables Definitions
*****************************************************************************/

/*****************************************************************************
  Functions Definitions
*****************************************************************************/

/*****************************************************************************
 * Конструктор
 */
C_MainWindow::C_MainWindow( QWidget *parent ) :
    QMainWindow(parent),
    ui( new Ui::MainWindow )
{
    using namespace network;

    ui->setupUi(this);
    // Установка заголовка окна и нередактируемости поля с логами
    this->setWindowTitle("UDP/TCP file sender");
    ui->logScreen->setReadOnly(true);

    // Связывание глобального логера с выводом на экран GUI
    connect( services::g_log.get(), &services::C_Logger::addedNewLogLine, this, &C_MainWindow::printNewLogLine, Qt::QueuedConnection );

    m_server = new C_Server( "server",
                             "127.0.0.1:8888 mode=nonblocking",
                             "data.mes",
                             E_Protocol::TCP );

    m_client = new C_Client( "client",
                             "127.7.7.7:7777 127.0.0.1:8888 mode=nonblocking",
                             E_Protocol::TCP );

    /*
     * Создание потока, в котором будет выполняться сервер
     * Привязка сигнала от кнопки интерфейса к слоту главного цикла обработчика сервера
     * Помещение сервера в созданный поток
     */
    m_serverThread = new QThread(this);
    m_server->moveToThread(m_serverThread);

    connect( m_serverThread, &QThread::started,     m_server,       &C_Server::work );
    connect( m_server,       &C_Server::finished,   m_serverThread, &QThread::quit  );

    // То же самое что и для сервера, см. выше
    m_clientThread = new QThread(this);
    m_client->moveToThread(m_clientThread);

    connect( m_clientThread, &QThread::started,     m_client,       &C_Client::work );
    connect( m_client,       &C_Client::finished,   m_clientThread, &QThread::quit  );

    // Привязка обработчиков к кнопкам "Server/Client Start"
    connect( ui->startServerBtn, &QPushButton::clicked, this, &C_MainWindow::onStartServerBtnClicked );
    connect( ui->startClientBtn, &QPushButton::clicked, this, &C_MainWindow::onStartClientBtnClicked );
    connect( ui->clearLogBtn,    &QPushButton::clicked, this, [this](){ ui->logScreen->clear(); }    );

    // Реакция кнопок на завершение работы клиента и сервера
    connect( m_server, &C_Server::finished, this, [this](){ ui->startServerBtn->setText("Start Server");
        isServerStartedBtnPressed = false;} );
    connect( m_client, &C_Client::finished, this, [this](){ ui->startClientBtn->setText("Start Client");
        isClientStartedBtnPressed = false;} );

    // Получение указателя на скролл-бар окна логов
    QScrollBar* scrollBar = ui->logScreen->verticalScrollBar();
    // Установка стиля для скролл-бара окна логов
    scrollBar->setStyleSheet( QString::fromUtf8( "QScrollBar:vertical {"
                    "    border: 1px solid #999999;"
                    "    background:white;"
                    "    width:10px;    "
                    "    margin: 0px 0px 0px 0px;"
                    "}"
                    "QScrollBar::handle:vertical {"
                    "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                    "    stop: 0 rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130), stop:1 rgb(32, 47, 130));"
                    "    min-height: 0px;"
                    "}"
                    "QScrollBar::add-line:vertical {"
                    "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                    "    stop: 0 rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
                    "    height: 0px;"
                    "    subcontrol-position: bottom;"
                    "    subcontrol-origin: margin;"
                    "}"
                    "QScrollBar::sub-line:vertical {"
                    "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                    "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
                    "    height: 0 px;"
                    "    subcontrol-position: top;"
                    "    subcontrol-origin: margin;"
                    "}" ) );

    connect( ui->protocolBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onProtocolSelected(int)) );
}

/*****************************************************************************
 * Деструктор
 */
C_MainWindow::~C_MainWindow()
{
    // Остановка клиента и сервера, если они работают
    m_server->stop();
    m_client->stop();

    // Ожидание завершения работы потоков клиента и сервера
    m_clientThread->quit();
    m_serverThread->quit();
    m_clientThread->wait(1000);
    m_serverThread->wait(1000);

    delete m_server;
    delete m_client;

    delete ui;
}

/*****************************************************************************
 * Обработчик сигнала, полученного от класа C_Logger
 *
 * @param
 *  [in] a_newLine- ссылка на обертку некоторого типа, выводимого в консоль
 */
void C_MainWindow::printNewLogLine( const QVariant &a_newLine )
{
    ui->logScreen->insertPlainText( a_newLine.toString() );
    // Установка автоскроллинга поля лога
    ui->logScreen->ensureCursorVisible();
}

/*****************************************************************************
 * Обработчик сигнала выбора типа протокола
 */
void C_MainWindow::onProtocolSelected(int a)
{
    qDebug() << "protocol type:" << ui->protocolBox->currentIndex();
}

/*****************************************************************************
 * Обработчик нажатия кнопки "startServerBtn"
 */
void C_MainWindow::onStartServerBtnClicked()
{
    if( !isServerStartedBtnPressed ) {
        // Запуск потока сервера
        m_serverThread->start();
        ui->startServerBtn->setText("Stop Server");
        isServerStartedBtnPressed = true;
    }
    else {
        // Остановка выполнения работы сервером
        m_server->stop();
        ui->startServerBtn->setText("Start Server");
        isServerStartedBtnPressed = false;
    }
}

/*****************************************************************************
 * Обработчик нажатия кнопки "startClientBtn"
 */
void C_MainWindow::onStartClientBtnClicked()
{
    if( !isClientStartedBtnPressed ) {
        // Запуск потока клиента
        m_clientThread->start();
        ui->startClientBtn->setText("Stop Client");
        isClientStartedBtnPressed = true;
    }
    else {
        // Остановка выполнения работы клиентом
        m_client->stop();
        ui->startClientBtn->setText("Start Client");
        isClientStartedBtnPressed = false;
   }
}


