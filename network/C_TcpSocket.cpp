/*****************************************************************************

  C_TcpSocket

  Класс TCP-сокета, реализующий интерфейс I_Socket поверх системной библиотеки
  сокетов ОС Windows - WINSOCK

*****************************************************************************/

#include "C_TcpSocket.h"

namespace network {

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
 * Деструктор
 */
C_TcpSocket::~C_TcpSocket()
{
    close();
}

/*****************************************************************************
 * Закрытие сокета и деинициализация библиотеки Winsock
 *
 * Закрывается рабочий сокет (если класс представляет клиентский сокет)
 * Закрывается рабочий и слушающий сокет (если класс представляет серверный сокет)
 */
void C_TcpSocket::close()
{
    if ( m_acceptedSocket != INVALID_SOCKET ) {
        int ret = shutdown( m_acceptedSocket, SD_BOTH );
        if ( ret == SOCKET_ERROR ) {
            g_log << name() << "shutdown: failed with error: " << WSAGetLastError() << std::endl;
        }
        else {
             g_log << name() << "shutdown: successfull " << std::endl;
        }
    }

    if ( m_acceptedSocket != INVALID_SOCKET ) {
        closesocket( m_acceptedSocket );
        m_acceptedSocket = INVALID_SOCKET;
    }
}

/*****************************************************************************
 * Отправка данных через сокет
 *
 * @param
 *  [in] a_buff - ссылка на буфер, данные из которого отправляются по сокету
 *
 * @return
 *  Статус успешности отправки данных через сокет
 *  true  - отправка данных произошла успешно
 *  false - во время отправки данных произошла ошибка
 */
bool C_TcpSocket::send( const std::vector<char> &a_buff )
{
    auto numBytes = ::send( m_acceptedSocket,
                            a_buff.data(),
                            static_cast<int>( a_buff.size() ),
                            0 );
    if ( numBytes == SOCKET_ERROR ) {
//        g_log << name() << "send: failed with error: "
//                << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        if ( static_cast<size_t>(numBytes) == a_buff.size() ) {
//            g_log << name() << "send: the buffer with size=" << a_buff.size() <<  " is sent successfully\n";
            return true;
        }
        else {
//            g_log << name() << "send: not all buffer is sent but only " << numBytes << std::endl;
            return false;
        }
    }
}

/*****************************************************************************
 * Прием данных через сокет
 *
 * @param
 *  [out] a_buff- ссылка на буфер, в который пишутся данные из сокета
 *
 * @return
 *  Статус успешности приема данных через сокет
 *  true  - прием данных произошел успешно
 *  false - во время приема данных произошла ошибка
 */
bool C_TcpSocket::recv( std::vector<char> &a_buff )
{
    auto numBytes =  ::recv( m_acceptedSocket,
                             a_buff.data(),
                             static_cast<int>( a_buff.size() ),
                             0 );

    if ( numBytes == SOCKET_ERROR ) {
//        g_log << name() << "recv: failed with error: "
//                << WSAGetLastError() << std::endl;
        return false;
    }
    else if ( numBytes == 0 ) {
        g_log << name() << "recv: connection closed" << std::endl;
        return false;
    }
    else {
//        g_log <<  name() << "recv: received bytes in packet: " << numBytes << std::endl;
        return true;
    }
}

/*****************************************************************************
 * Вызов системного API для создания сокета нужного типа
 *
 * @return
 *  Статус успешности дополнительной настроки сокета
 *  true  - настройка сокета произошло успешно
 *  false - во время настройки сокета произошла ошибка
 */
bool C_TcpSocket::setSockOptions()
{
    if ( m_masterSock == INVALID_SOCKET ) {
        g_log << "error: socket is not initialized" <<std::endl;
        return false;
    }

    /*
     * Передача маленьких пакетов накладывает дополнительные расходы на время ожидания
     * подтверждающих пакетов ACK TCP протокола, поэтому для уменьшения накладных расходов
     * на время ожидания ответа (200ms) был отключен алгоритм Наггла (no_delay option), а также
     * отключен буфер отправки SO_SNDBUF
     * reference: https://support.microsoft.com/en-us/help/214397/design-issues-sending-small-data-segments-over-tcp-with-winsock
     */
    if( m_socketType == E_SocketType::Server ) {
        int sendWindowSize = 0;
        long rc1 = setsockopt( m_masterSock, SOL_SOCKET, SO_SNDBUF,
                               reinterpret_cast<char*>(&sendWindowSize), sizeof(sendWindowSize) );
        DWORD value = 1;
        long rc2 = setsockopt( m_masterSock, IPPROTO_TCP, TCP_NODELAY,
                               reinterpret_cast<char*>(&value), sizeof(value) );
        if ( rc1 < 0 || rc2 < 0 ) {
           g_log << name() << "setsockopt() failed while configuring no_delay or send buff option: "
                   << WSAGetLastError() << std::endl;
           return false;
        }
    }

    LINGER linger = { 1, 0 };
    long rc1 = setsockopt( m_masterSock, SOL_SOCKET, SO_LINGER,
                           reinterpret_cast<char*>(&linger), sizeof(linger) );
    if ( rc1 < 0 ) {
       g_log << name() << "setsockopt() failed: "
               << WSAGetLastError() << std::endl;
       return false;
    }
    else {
       g_log << name() << "setsock configured successfully" << std::endl;
       return true;
    }
}

/*****************************************************************************
 * Инициализация соединения
 * Поведение функции разное в зависимости от типа сокета
 *
 * @param
 *  [in] backlog - количество возможных входящий соединений (для серверного сокета)
 *
 * @return
 *  Статус успешности инициализации соединения
 *  true - соединение установлено
 *  false - во время установки соединения произошла ошибка
 */
bool C_TcpSocket::connect()
{
    switch ( m_socketType ) {

        case E_SocketType::Client:
            return this->connectToServer();

        case E_SocketType::Server:
            return this->connectToClient();

        default:
            return false;
    }
}

/*****************************************************************************
 * Установка соединения с сервером (если класс представляет клиентский сокет)
 *
 * @return
 *  Статус успешности инициализации соединения
 *  true  - соединение клиента с сервером установлено
 *  false - во время установки соединения клиента с сервером произошла ошибка
 */
bool C_TcpSocket::connectToServer()
{
    g_log << name() << "start connecting..." << std::endl;

    // Устанавливаем соединение с сервером
    int retVal = ::connect( m_masterSock,
                            reinterpret_cast<sockaddr*>(&m_peerService),
                            sizeof (m_peerService) );

    if ( retVal == 0 || WSAGetLastError() == WSAEISCONN ) {
        /**
         * Серверный сокет при вызове accept возвращает m_socket,
         * посредством которого ведется прием-передача. Чтобы клиентский
         * сокет также мог работать, необходимо приравнять эти сокеты друг
         * другу, иначе клиентский сокет не сможет вести прием передачу.
         */
        m_acceptedSocket = m_masterSock;

        g_log << name() << "connection success" << std::endl;
        return true;
    }
    else {
//        g_log << name() << "connect() failed with error: "
//                << WSAGetLastError() << std::endl;
        return false;
    }
}

/*****************************************************************************
 * Установка соединения с клиентом (для серверного сокета)
 *
 * @return
 *  Статус успешности инициализации соединения
 *  true  - соединение сервера с клиентом установлено
 *  false - во время установки соединения сервера с клиентом произошла ошибка
 */
bool C_TcpSocket::connectToClient()
{
    if ( !listen( m_backlog ) ) {
        return false;
    }
    if ( !accept() ) {
        return false;
    }
    return true;
}

/*****************************************************************************
 * Установка прослушивания входящих соединений
 * (если класс представляет серверный сокет)
 *
 * @param
 *  [in] backlog - количество возможных входящий соединений
 *
 * @return
 *  int - статус успешности начала прослушивания
 */
bool C_TcpSocket::listen(int a_backlog)
{
    g_log << name() << "start listening" << std::endl;

    // Set the listen back log
    int retVal = ::listen( m_masterSock, a_backlog );
    if ( retVal == SOCKET_ERROR ) {
        g_log << name() << "listen() failed with error: "
                << WSAGetLastError() << std::endl;
        return false;
    }
    else if ( retVal == 0 ) {
        g_log << name() << "listening mode on" << std::endl;
        return true;
    }
    return false;
}

/*****************************************************************************
 * Принятие соединения (если класс представляет серверный сокет)
 *
 * @return
 *  - статус успешности принятия соединения, либо дескриптор принятого соединения
 */
bool C_TcpSocket::accept()
{
    g_log << name() << "start accepting" << std::endl;

    m_acceptedSocket = ::accept( m_masterSock, nullptr, nullptr );

    if ( m_acceptedSocket == INVALID_SOCKET ) {
        if ( WSAGetLastError() == WSAEWOULDBLOCK ) {
            g_log << name() << "No pending connections" << std::endl;
        }
        else {
            g_log << name() << "accept failed with error: "
                    << WSAGetLastError() << std::endl;
        }
        return false;
    }
    else {
        g_log << name()  << "client accepted" << std::endl;
        return true;
    }
}

} // namespace network

