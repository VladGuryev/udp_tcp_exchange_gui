/******************************************************************************

  C_Client

  Класс UDP/TCP клиента


  ДЕТАЛИ РЕАЛИЗАЦИИ

  * Прием и передача данных осуществляются с помощью сокета, реализующего интерфейс
    I_Socket. Создание сокета производится в фабричном методе C_SocketFactory::createSocket()

  * Работа клиента выполняется в машине состояний в функции work(). Цикл событий управляется атомарным счетчиком
    isRunning, посредством которого можно управлять работой цикла.

  * В главном цикле обработчике клиента расположена машина состояний, разные
    состояния которой описаны в enum States в клиента C_Client.
    Каждое состояние клиента обрабатывается отдельной функцией, реализующей
    необходимое поведение. Переходы между состояниями заданы требуемой в ТЗ
    логикой работы склиента. Завершение работы производится в состоянии
    States::Finish

  * После каждого запроса клиент ждет ответа от сервера со следующей посылкой данных.

  * Завершение работы клиента происходит после получения пакета FileSent от сервера,
    обозначающего, что весь файл передан (см. common_types.h).

******************************************************************************/

#include "C_Client.h"

#include <thread>
#include <cstring>

#include "C_SocketFactory.h"

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

const size_t        C_Client::s_bufSize      = 8 * 1024;    // Размер буфера приема/отправки

const unsigned char C_Client::s_approveCount = 3;           // Количество подтверждений от сервера для установления соединения

/*****************************************************************************
  Functions Definitions
*****************************************************************************/

/*****************************************************************************
 * Конструктор
 */
C_Client::C_Client( std::string a_logLabel,
                    std::string a_authority,
                    E_Protocol  a_protoType )
                  : m_buffer   ( s_bufSize         ),
                    m_name     ( a_logLabel + ": " ),
                    m_authority( a_authority       ),
                    m_protoType( a_protoType       )

{
    std::string fdProto = ( m_protoType == E_Protocol::TCP ) ? "TCP " : "UDP ";
    g_log << "-------" << fdProto << "CLIENT "
            << "CREATED" << "-------"  << std::endl << std::endl;
}

/*****************************************************************************
 * Остановить работу клиента
 */
void C_Client::stop()
{
    isRunning = false;
}

/*****************************************************************************
 * Главный цикл-обработчик клиента
 */
void C_Client::work()
{
    using namespace std::chrono_literals;

    E_States state     = E_States::Setup;   // Текущее состояние стейт-машины
    E_States prevState = E_States::Setup;   // Предыдущее состояние стейт-машины

    std::chrono::milliseconds sleepTime = 1000ms;   // Время ожидания между двумя неуспешными операциями, мсек
    unsigned long long recvCounter = 0;             // Счетчик пакетов

    isRunning = true;                               // Установить флаг работы для вхождения в цикл событий

    while (isRunning) {
        prevState = state;
        switch ( state ) {

            case E_States::Setup:
                sleepTime = 1000ms;
                if ( setup() ) {
                    state = E_States::Connect;
                }
                else {
                    g_log << m_name + "socket setup error: " << errno << std::endl;
                }
                break;

            case E_States::Connect:
                sleepTime = 1000ms;
                if ( connect() ) {
                    state =  E_States::SendPacket;
                }
                break;

            case E_States::SendPacket:
                sleepTime = 10ms;
                if ( sendPacket( Comand::Data ) ) {
                    state = E_States::RecvPacket;
                }
                break;

            case E_States::RecvPacket:
                sleepTime = 10ms;
                if ( recvPacket() ) {
                    recvCounter++;
                    state = E_States::ParseComand;
                }
                break;

            case E_States::ParseComand:
                if ( parseComand() != Comand::Finish ) {
                    state = ( recvCounter == 1 )
                          ? E_States::WriteHeader
                          : E_States::WritePacket;
                    break;
                }
                else {
                    g_log << "client: finish packet was received" << std::endl;
                    state = E_States::Finish;
                    break;
                }

            case E_States::WriteHeader:
                g_log << m_name << "file open status: " << openFile("received.mes") << std::endl;
                writeHeader();
                state = E_States::SendPacket;
                break;

            case E_States::WritePacket:
                writePacket();
                state = E_States::SendPacket;
                break;

            case E_States::Finish:
                g_log << m_name + m_handle->name() << " stopped" << std::endl;
                close();
                return;
        }
        if ( state == prevState ) {
            sleep( sleepTime );
        }
    }
    close();
    return;
}

/*****************************************************************************
 * Завершение работы клиента
 */
void C_Client::close()
{
    if ( m_file.is_open() ) {
        m_file.close();
    }
    // Закрытие сокета и его удаление
    m_handle->close();
    m_handle.reset();
    // Очистка входного буфера
    m_buffer.clear();
    // Сброс счетчика входящих пакетов
    m_counter = 0;
    g_log << m_name << "deinitialized" << std::endl;
    // Запустить остановку потока клиента
    emit finished();
}

/*****************************************************************************
 * Отправка данных на сервер
 *
 * Команда из enum E_Comands преобразуется в байт, записываемый в буфер отправки
 * сообщения после чего терминируется символом '\0'.
 *
 * @param
 *  [in] a_comand - команда, отправляемая на сервер
 *
 * @return
 *  true  - данные успешно отправлены на сервер
 *  false - ошибка при отправке данных на сервер
 */
bool C_Client::sendPacket( Comand a_comand )
{
    T_NetPacket packet;

    if ( a_comand == Comand::Data ) {
        packet.Head = Header::DataReqt;
    } else {
        return false;
    }

    return m_handle->send( serialize(packet) );
}

/*****************************************************************************
 * Прием данных с сервера
 *
 * @return
 *  true  - данные успешно приняты с сервера
 *  false - ошибка при приеме данных с сервера
 */
bool C_Client::recvPacket()
{
    m_buffer.clear();
    m_buffer.resize( s_bufSize );
    return m_handle->recv( m_buffer );
}

/*****************************************************************************
 * Запись принятого заголовка в файл
 */
void C_Client::writeHeader()
{
    g_log << m_name << "received header with size: "
            << getHeaderSize() << std::endl;

    T_NetPacket netPacket = deserialize(m_buffer);
    print( toPacketHeaderPtr(  netPacket.Data.data() ) );

    if ( !m_file.is_open() ) {
        g_log << m_name <<  "Error while opening file!\n";
        return;
    }

    m_file.write( netPacket.Data.data(), getHeaderSize() );
    return;
}

/*****************************************************************************
 * Запись принятого пакета в файл
 */
void C_Client::writePacket()
{
    T_NetPacket netPacket = deserialize(m_buffer);

    T_Packet *packetPtr  = toPacketPtr( netPacket.Data.data() );
    unsigned int packetSize = getPacketSize( packetPtr );
    g_log << m_name << "received packet #" << m_counter++ << " with size: "
            << packetSize << std::endl;

    print( packetPtr );
    m_file.write( netPacket.Data.data(), packetSize );
    return;
}

/*****************************************************************************
 * Создание и настройка сокета
 *
 * @return
 *  Статус успешности настройки сокета
 *  true  - открытие и настройка сокета произведена успешно
 *  false - ошибка при открытии и настройке сокета
 */
bool C_Client::setup()
{
    if ( !m_handle ) {
        m_handle = C_SocketFactory::createSocket( m_protoType );
    }
    else {
        errno = enSockAlreadyCreated;
        return false;
    }

    if ( !m_handle ) {
        errno = enSockCreateError;
        return false;
    }

    if ( !m_handle->open() ) {
        errno = enSockOpenError;
        return false;
    }

    if ( m_handle->setup( m_authority ) ) {
        if ( m_protoType == E_Protocol::TCP ) {
            g_log << m_name << "Waiting for connection..." << std::endl;
        }
        return true;
    }

    errno = enSockSetupError;
    return false;
}

/*****************************************************************************
 * Подключение к серверу
 *
 * @return
 *  Cтатус успешности подключения к серверу
 *  true  - подключение произведено успешно
 *  false - ошибка подключения
 */
bool C_Client::connect()
{
    using namespace std::chrono_literals;

    if( !m_handle ) {
        g_log << m_name << "connect error: socket is not created" << std::endl;
        return false;
    }

    if ( m_handle->connect() ) {
        g_log << m_name << "establishing connection with server..." << std::endl;

        if ( m_protoType == E_Protocol::UDP ) {
            udpConHandler();
            g_log << m_name << "connected to udp server" << std::endl;

        }
        return true;
    }

    g_log << m_name << "error with opening connection" << std::endl;
    return false;
}

/*****************************************************************************
 * Проведение процедуры "handshake" с сервером по UDP протоколу
 *
 * Функция синхронная, не передаст управление, пока не будет завершена процедура
 */
void C_Client::udpConHandler()
{
    using namespace std::chrono_literals;
    bool isConnected = false;                                       // Статус флаг цикла
    E_ConnectionStates state = E_ConnectionStates::EchoReqt;        // Текущее состояние стейт-машины
    E_ConnectionStates prevState = E_ConnectionStates::EchoReqt;    // Предыдущее состояние стейт-машины
    std::chrono::milliseconds timeout = 100ms;                      // Время ожидания между запросами подтверждения, мсек
    unsigned recvCounter = 0;

    while( !isConnected ) {
        prevState = state;
        switch (state) {
            // Отправка эхо-запроса
            case E_ConnectionStates::EchoReqt: {
                // Формирование пакета эхо-запроса
                T_NetPacket packet;
                packet.Head = Header::EchoReqt;
                packet.Data = { s_approveCount };
                if ( m_handle->send( serialize(packet) ) ) {
                    state = E_ConnectionStates::WaitResp;
                }
            } break;
            // Ожидание эхо-ответа
            case E_ConnectionStates::WaitResp: {
                m_buffer.clear();
                m_buffer.resize(s_bufSize);
                if ( m_handle->recv( m_buffer ) ) {
                    // Десериализация пакета из массива принятых байтов
                    T_NetPacket packet = deserialize(m_buffer);
                    if( packet.Head == Header::EchoResp ) {
                        recvCounter++;
                    }
                    state = E_ConnectionStates::VerifyStatus;
                }
            } break;
            // Проверка условия установления соединения
            case E_ConnectionStates::VerifyStatus:
                    state = recvCounter < s_approveCount ? E_ConnectionStates::EchoReqt
                                                         : E_ConnectionStates::Connected;
            break;
            // Соединение установлено
            case E_ConnectionStates::Connected:
                isConnected = true;
                break;
        }

        if ( state == prevState ) {
           std::this_thread::sleep_for( timeout );
        }
    }
    return;
}

/*****************************************************************************
 * Ожидание между неуспешными итерациями цикла-обработчика, мсек
 */
void C_Client::sleep( std::chrono::milliseconds a_sleepTime )
{
    std::this_thread::sleep_for( a_sleepTime );
}

/*****************************************************************************
 * Создание и открытие файла для сохранения входящих пакетов
 *
 * @param
 *  [in] a_filePath - путь, по которому должен располагаться создаваемый файл
 *
 * @return
 *  Статус операции открытия файла
 *  true  - файл успешно открыт
 *  false - ошибка при открытии файла
 */
bool C_Client::openFile( std::string a_filePath )
{
    if ( a_filePath.empty() ) {
        g_log << m_name << "filepath is empty!" << std::endl;
        return false;
    }
    // Открываем файл, очищаем содержимое, закрываем файл
    m_file.open( a_filePath, std::ios::trunc );
    m_file.close();
    // Открываем файл в режиме бинарный, выходной
    m_file.open( a_filePath,  std::ios::binary | std::ios::app | std::ios::out );
    // Перемещаем указатель в начало файла
    m_file.seekp( 0, std::ios_base::beg );

    return m_file.is_open();
}

/*****************************************************************************
 * Разбор принятой от сервера байтовой последовательности
 *
 * @param
 *  [in] a_packet - ссылка на буфер, содержащий принятый и готовый для разбора пакет
 *
 * @return
 *  - Команда, содержащаяся в пакете
 */
Comand C_Client::parseComand() const
{
    T_NetPacket recvPacket = deserialize(m_buffer);

    if ( recvPacket.Head == Header::FileSent ) {
        return Comand::Finish;
    }
    else {
        return Comand::Data;
    }
}

} // namespace network
