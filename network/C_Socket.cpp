/*****************************************************************************

  C_Socket

  Класс сокета, частично реализующий интерфейс I_Socket, общий для UDP/TCP сокетов


  ДЕТАЛИ РЕАЛИЗАЦИИ

  * Во время вызова функции setup() производится инициализация WINSOCK API с помощью
    вызова WSAStartup(), а затем создание файлового дескриптора сокета с помощью
    вызова функции socket() в зависимости от типа сокета (UDP/TCP).

  * При удалении объекта класса C_Socket в деструкторе класса производится вызов
    функции close() - деинициализация WINSOCK библиотеки и удаление файлового дескриптора
    сокета

  * Класс реализует стратегию RAII и заботится о создании и удалении файлового дескриптора
    низкоуровневого WinSock сокета наряду с инициализацией и деинициализацией WINSOCK библиотеки,
    тем самым обеспечивая сокрытие низкоуровневых функций для работы с сокетом для конечного пользователя

*****************************************************************************/

#include "C_Socket.h"

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
 * Конструктор
 */
C_Socket::C_Socket( E_Protocol  a_proto,
                    std::string a_name )
                  : m_protoType( a_proto ),
                    m_name     ( a_name  )
{
    switch ( m_protoType ) {
        case E_Protocol::TCP:
            m_protoCred = { AF_INET, SOCK_STREAM, IPPROTO_TCP };
            break;

        case E_Protocol::UDP:
            m_protoCred = { AF_INET, SOCK_DGRAM, IPPROTO_UDP };
            break;

        default:
            m_protoCred = { AF_INET, SOCK_DGRAM, IPPROTO_UDP };
            m_protoType = E_Protocol::UDP;
            break;
    }
}

/*****************************************************************************
 * Деструктор
 */
C_Socket::~C_Socket()
{
    close();
}

/*****************************************************************************
 * Инициализация параметров сокета, задание адреса и порта созданного сокета
 *
 * @param
 *  [in] configuration - строка с настройками для сокета
 *
 * @return
 *  true  - сокет успешно настроен
 *  false - во время настройки произошла ошибка
 */
bool C_Socket::setup( const std::string &a_configuration )
{
    std::vector<I_Socket::endpoint> endpoints = parseEndpointStr( a_configuration );
    m_socketType = endpoints.size() > 1 ? E_SocketType::Client : E_SocketType::Server;

    I_Socket::endpoint firstEndpoint = *endpoints.begin();
    bool setStat = configSock( firstEndpoint );

    if ( m_socketType == E_SocketType::Client ) {
        auto secondEndpoint = *endpoints.rbegin();
        setPeerAddr( secondEndpoint );
    }
    return setStat;
}

/*****************************************************************************
 * Сброс настроек сокета к начальным значениям
 *
 * @return
 *  true  - сброс произведен успешно
 *  false - во время сброса настроек произошла ошибка
 */
bool C_Socket::flush()
{
    // Allow socket descriptor to be reuseable
    u_long on = 1;
    int rc = setsockopt( m_masterSock, SOL_SOCKET,  SO_REUSEADDR,
                         reinterpret_cast<char*>(&on), sizeof(on) );

    if ( rc < 0 ) {
        g_log << name() << "setsockopt() failed: "
                << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        g_log << name() << "setsock success" << std::endl;
    }

    if ( !setNonBlocking( E_SocketMode::Blocking ) ) {
        g_log << name() << "problem with setting socket to blocking mode" <<
                   WSAGetLastError() << std::endl;
        return false;
    }

    memset( reinterpret_cast<char*>(&m_myService),
            0,
            sizeof(m_myService) );

    memset( reinterpret_cast<char*>(&m_peerService),
            0,
            sizeof(m_peerService) );

    m_name.clear();
    m_socketType = E_SocketType::Invalid;
    m_protoType  = E_Protocol::Invalid;

    return true;
}

/*****************************************************************************
 * Вызов системного API для создания сокета нужного типа
 *
 * Вызов этого метода производится в теле функции initialize()
 *
 * @return
 *  Статус успешности создания сокета нужного типа
 *  true  - создание сокета произошло успешно
 *  false - во время создания сокета произошла ошибка
 */
bool C_Socket::open()
{
    if ( !initLib() ) {
        return false;
    }

    if ( m_masterSock && (m_masterSock != INVALID_SOCKET) ) {
        g_log << "error: current socket descriptor has already been created!" << std::endl;
        return false;
    }

    m_masterSock = socket( m_protoCred.domain, m_protoCred.type, m_protoCred.protocol );

//    m_socketDeleter socketDeleter = [](SOCKET* handler) { closesocket(*handler); };
//    SOCKET socket = ::socket( m_protoCred.domain,
//                              m_protoCred.type,
//                              m_protoCred.protocol );
//    if ( socket != INVALID_SOCKET ) {
//        m_masterSock = { &socket, socketDeleter } ;
//    }

    if ( m_masterSock  == INVALID_SOCKET ) {
        g_log << name() << "socket() failed with error code:"
                << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        g_log << name() << "initialized" << std::endl;
        return true;
    }
}

/*****************************************************************************
 * Деинициализация сокета, закрытие библиотеки WinSock
 */
void C_Socket::close()
{
    if ( m_masterSock ) {
        closesocket( m_masterSock );
    }
    WSACleanup();
}

/*****************************************************************************
 * Установка режима сокета (блокирующий/неблокирующий)
 *
 * @param
 *  [in] nonBlock - E_Mode::Blocking     - блокирующий режим
 *                - E_Mode::NonBlocking  - неблокирующий режим
 *
 * @return
 *  true  - режим успешно установлен
 *  false - во время установки режима произошла ошибка
 */
bool C_Socket::setNonBlocking( E_SocketMode a_mode )
{
    if ( m_masterSock == INVALID_SOCKET ) {
        g_log << "error: socket is not initialized" <<std::endl;
        return false;
    }

    if ( a_mode == E_SocketMode::Blocking ) {
        g_log << name() << "socket is blocking" << std::endl;
        return true;
    }
    u_long iMode = 1;
    int rc = ioctlsocket( m_masterSock, FIONBIO, &iMode );
    if ( rc < 0 ) {
       g_log << name() << "ioctlsocket() failed" << std::endl;
       return false;
    }
    else {
        g_log << name() << "socket non blocking mode set" << std::endl;
        return true;
    }
}

/*****************************************************************************
 * Инициализация библиотеки WinSock
 */
bool C_Socket::initLib()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    int initStat  = WSAStartup( wVersionRequested, &wsaData );

    if ( initStat != 0 ) {
        g_log << name() << "WSAStartup failed with error:" << initStat << std::endl;
        return false;
    }
    else {
        g_log << "WINSOCK DLL is initialized" << std::endl;
        return true;
    }
}

/*****************************************************************************
 * Установка адреса и порта назначения (для клиента)
 *
 * Функция не производит проверку входных параметров
 *
 * @param
 *  [in] a_endpoint - кортеж из адреса и порта получателя и режима работы сокета
 */
void C_Socket::setPeerAddr( const I_Socket::endpoint &a_endpoint )
{
    std::string addr;
    uint16_t    port;
    E_SocketMode      mode;
    std::tie( addr, port, mode ) = a_endpoint;

    memset( reinterpret_cast<char*>(&m_peerService),
            0,
            sizeof(m_peerService) );

    // Настройка IPv4
    m_peerService.sin_family      = AF_INET;
    // Задание порта
    m_peerService.sin_port        = htons    ( port        );
    // Установка адреса
    m_peerService.sin_addr.s_addr = inet_addr( addr.data() );

    g_log << name() << "destination is configured: "
            << "[" + addr + ":" + std::to_string(port) + "]" << std::endl;
    return;
}

/*****************************************************************************
 * Лог-метка сокета
 *
 * @return
 *  std::string - возвращает строковую метку сокета в формате:
 *                 "logLabel [_tag_ socket addr:port]"
 */
std::string C_Socket::name() const
{
    char   *addr = inet_ntoa( m_myService.sin_addr );
    u_short port = ntohs    ( m_myService.sin_port );

    return "[" + m_name + std::string(" ")
            + std::string(addr) + ":" + std::to_string(port) + "] ";
}

/*****************************************************************************
 * Инициализация библиотеки, конфигурация параметров сокета
 *
 * Задание адреса и порта структуры
 * Связывание (биндинг) файлового дескриптора сокета со структурой
 *
 * Функция не производит проверку входных параметров
 *
 * @param
 *  [in] a_endpoint - кортеж из адреса и порта получателя и режима работы сокета
 *
 * @return
 *   true  - сокет успешно инициализирован
 *   false - во время инициализации сокета произошла ошибка
 */
bool C_Socket::configSock( const I_Socket::endpoint &a_endpoint )
{
    std::string  addr;
    uint16_t     port;
    E_SocketMode blockingMode;
    std::tie( addr, port, blockingMode ) = a_endpoint;

    if ( !setSockOptions() ) {
        return false;
    }

    if ( !setNonBlocking( blockingMode ) ) {
        return false;
    }

    memset( reinterpret_cast<char*>(&m_myService),
            0,
            sizeof(m_myService) );

    m_myService.sin_family      = AF_INET;
    m_myService.sin_addr.s_addr = inet_addr( addr.data() );
    m_myService.sin_port        = htons    ( port        );

    int bindStat = bind( m_masterSock,
                         reinterpret_cast<sockaddr*>(&m_myService),
                         sizeof(m_myService) );
    if ( bindStat == SOCKET_ERROR ) {
       g_log << name() << "binding failed with error code: "
               << WSAGetLastError() << std::endl;
       return false;
    }
    else {
        g_log << name() << "binding done" << std::endl;
    }
    return true;
}

} // namespace network
