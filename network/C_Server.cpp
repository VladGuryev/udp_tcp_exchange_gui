/******************************************************************************

  C_Server

  Класс UDP/TCP сервера

  ДЕТАЛИ РЕАЛИЗАЦИИ

  * Прием и передача данных осуществляются с помощью сокета, реализующего интерфейс
    I_Socket. Создание сокета производится в фабричном методе C_SocketFactory::createSocket()

  * Обработка запросов от клиента происходит в стейт-машине функции work()

  * В главном цикле обработчике сервера расположен КА, разные
    состояния которого описаны в enum States в классе C_Server. Цикл событий управляется
    атомарным счетчиком isRunning, посредством которого можно управлять работой цикла.
    Каждое состояние сервера обрабатывается отдельной функцией, реализующей
    необходимое поведение. Переходы между состояниями заданы требуемой в ТЗ
    логикой работы сервера. Завершение работы сервера производится в состоянии
    States::Finish, в котором флаг isStarted переводится в значение false.

  * Ответ на запрос клиента "отправь данные" формируется из  файла, который
    индексируется анализатором потока (C_StreamAnalyzer.h). Индексация файла заключается в нахождении
    байтовых границ каждого пакета, включая заголовок. Границы определяются парами итераторов на начало
    и конец пакета.

  * Для UDP протокола реализован отдельный протокол установления сеанса соединения для того, чтобы
    можно было удостовериться, что соединение с клиентом установлено.

  * Каждый принятый пакет данных от клиента парсится с помощью функции
    parseComand(packet) для того, чтобы распознать команду, отправленную клиентом.

******************************************************************************/

#include "C_Server.h"

#include <random>
#include <thread>
#include <functional>
#include <chrono>

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

const size_t C_Server::s_bufSize = 8 * 1024;   // Размер буфера приема/отправки 8 kilobytes

/*****************************************************************************
  Functions Definitions
*****************************************************************************/

/*****************************************************************************
 * Конструктор
 *
 * TODO: заменить на Builder
 */
C_Server::C_Server( std::string a_logLabel,
                    std::string a_authority,
                    std::string a_filePath,
                    E_Protocol a_protoType )
                  : m_buffer   ( s_bufSize         ),
                    m_name     ( a_logLabel + ": " ),
                    m_authority( a_authority       ),
                    m_protoType( a_protoType       ),
                    m_filePath ( a_filePath )
{
    std::string fdProto = ( m_protoType == E_Protocol::TCP ) ? "TCP " : "UDP ";
    g_log << "-------" << fdProto << "SERVER "
            << "CREATED" << "-------" << std::endl << std::endl;
}

/*****************************************************************************
 * Остановить работу сервера
 */
void C_Server::stop()
{
    isRunning = false;
}

/*****************************************************************************
 * Главный цикл-обработчик сервера
 */
void C_Server::work()
{
    using namespace std::chrono_literals;

    E_States state     = E_States::Setup;   // Текущее состояние стейт-машины
    E_States prevState = E_States::Setup;   // Предыдущее состояние стейт-машины

    unsigned long             packetIdx    = 0;
    std::chrono::milliseconds sleepTime    = 1000ms;    // Время ожидания между двумя неуспешными операциями, мсек
    std::chrono::milliseconds previousTime = 0ms;       // Предыдущее время ожидания между итерациями цикла событий машины состояний

    isRunning = true;
    bool headerIsSent = false;

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
                    state = E_States::RecvPacket;
                }
                break;

            case E_States::RecvPacket:
                sleepTime = 10ms;
                if ( recvPacket() ) {
                    state = E_States::ParsePacket;
                }
                break;

            case E_States::ParsePacket:
                if ( parseComand() == Comand::Data ) {
                    if ( !headerIsSent ) {
                        state = E_States::LoadFile;
                        break;
                    }
                    state = E_States::SendPacket;
                    break;
                }
                else {
                    state = E_States::RecvPacket;
                    break;
                }

            case E_States::LoadFile:
                loadFile(m_filePath);
                state = E_States::SendHeader;
                break;

            case E_States::SendHeader: {
                sleepTime = 10ms;
                auto headIters = m_packetProvider->headerRange();
                if ( sendPacket( Comand::Data, { headIters.first, headIters.second } ) ) {
                    headerIsSent = true;
                    state = E_States::RecvPacket;
                }
            } break;

            case E_States::SendPacket: {
                sleepTime = 10ms;
                if ( packetIdx < m_packetProvider->packetCount() ) {
                    if ( !processPacket( packetIdx, previousTime, sleepTime ) ) {
                        g_log << m_name << "packet at index " << packetIdx << " is not sent" << std::endl;
                        state = E_States::RecvPacket;
                        break;
                    }
                }
                else {
                   state = E_States::Finish;
                }
            } break;

            case E_States::Finish:
                if ( sendPacket( Comand::Finish ) ) {
                    sleep( 50ms );
                    g_log << m_name << m_filePath << " file is sent!\n";
                    g_log << m_name + m_handle->name() + " " << "stopped" << std::endl;
                    close();
                    return;
                }
        }
        if ( state == prevState ) {
            sleep( sleepTime );
        }
    }
    close();
    return;
}

/*****************************************************************************
 * Завершение работы сервера
 */
void C_Server::close()
{
    if ( m_file.is_open() ) {
        m_file.close();
    }

    // Закрытие сокета и его удаление
    m_handle->close();
    m_handle.reset();
    // Очистка входного буфера
    m_buffer.clear();
    // Удаление парсера файлов
    m_packetProvider.reset();
    g_log << m_name << "deinitialized" << std::endl;
    // Запустить остановку потока сервера
    emit finished();
}

/*****************************************************************************
 * Создание и настройка сокета
 *
 * @return
 *  Cтатус успешности создания и настройки сокета
 *  true  - настройка сокета произведена успешно
 *  false - ошибка при настройке сокета
 */
bool C_Server::setup()
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
            g_log << m_name << "Waiting for acception..." << std::endl;
        }
        return true;
    }

    errno = enSockSetupError;
    return false;
}

/*****************************************************************************
 * Создание сессии с клиентом
 *
 * @return
 *  Cтатус успешности подключения
 *  true  - сервер успешно установил соединение
 *  false - ошибка при установке соединения
 */
bool C_Server::connect()
{
    using namespace std::chrono_literals;

    if ( !m_handle ) {
        g_log << m_name << "connect error: socket creation failed" << std::endl;
        return false;
    }
    if ( m_handle->connect() ) {
        g_log << m_name << "establishing connection with client..." << std::endl;

        if ( m_protoType == E_Protocol::UDP ) {
            udpConHandler();
            g_log << m_name << "connected to udp client" << std::endl;
        }
        return true;
    }

    g_log << m_name << "error with opening connection" << std::endl;
    return false;
}

/*****************************************************************************
 * Проведение процедуры "handshake" с клиентом по UDP протоколу
 *
 * Функция синхронная, не передаст управление, пока не будет завершена процедура
 */
void C_Server::udpConHandler()
{
    using namespace std::chrono_literals;
    bool isConnected = false;                                       // Статус флаг цикла
    E_ConnectionStates state = E_ConnectionStates::WaitReqt;        // Текущее состояние стейт-машины
    E_ConnectionStates prevState = E_ConnectionStates::WaitReqt;    // Предыдущее состояние стейт-машины
    std::chrono::milliseconds timeout = 100ms;                      // Время ожидания между запросами подтверждения, мсек
    unsigned char sendCounter  = 0;                                 // Счетчик отправленных пакетов с эхо-запросом
    unsigned char approveCount = 0;                                 // Счетчик полученных пакетов с эхо-ответом

    while( !isConnected ) {
        prevState = state;
        switch (state) {
            // Ожидание эхо-запроса
            case E_ConnectionStates::WaitReqt: {
                m_buffer.clear();
                m_buffer.resize(s_bufSize);
                if( m_handle->recv(m_buffer) ) {
                    // Десериализация пакета из массива принятых байтов
                    T_NetPacket packet = deserialize(m_buffer);
                    if( packet.Head == Header::EchoReqt ){
                        state = E_ConnectionStates::EchoResp;
                        approveCount = packet.Data.front();
                    }
                }
            } break;
            // Отправка эхо-ответа
            case E_ConnectionStates::EchoResp: {
                // Формирование пакета эхо-ответа
                T_NetPacket packet;
                packet.Head = Header::EchoResp;
                if ( m_handle->send( serialize(packet) ) ) {
                    sendCounter++;
                    state = E_ConnectionStates::VerifyStatus;
                }
            } break;
            // Проверка условия установления соединения
            case E_ConnectionStates::VerifyStatus:
                    state = sendCounter < approveCount ? E_ConnectionStates::WaitReqt
                                                       : E_ConnectionStates::Connected;
            break;
            // Соединение установлено
            case E_ConnectionStates::Connected:
                    isConnected = true;
            break;
        }

        if ( state == prevState ) {
            std::this_thread::sleep_for(timeout);
        }
    }
    return;
}


/*****************************************************************************
 * Прием данных от клиента
 *
 * @return
 *  Статус успешности приема сообщения
 *  true  - сервер успешно принял сообщение
 *  false - ошибка при приеме сообщения
 */
bool C_Server::recvPacket()
{
    m_buffer.clear();
    m_buffer.resize(s_bufSize);
    return m_handle->recv(m_buffer);
}

/*****************************************************************************
 * Преобразование строки в вектор символов
 *
 * @param
 *  [in] a_str - rvalue-ссылка на строку, которую требуется преобразовать в вектор символов
 *
 * @return
 *  - результат преобразования
 */
std::vector<char> C_Server::convertStrToVec( std::string &&a_str )
{
    std::vector<char> vec;
    vec.resize( a_str.size() );
    std::copy( a_str.begin(), a_str.end(), vec.begin() );
    return vec;
}

/*****************************************************************************
 * Отправка данных из файла клиенту
 *
 * @param
 *  [in] a_comand -  команда, которую необходимо отправить клиенту
 *  [in] a_payload - ссылка на буфер, который необходимо передать
 *
 * @return
 *  Статус успешности отправки
 *  true  - сервер успешно отправил данные
 *  false - ошибка при отправке
 */
bool C_Server::sendPacket( Comand a_comand, std::vector<char> a_payload )
{
    T_NetPacket packet;
    if( a_comand == Comand::Data ) {
        packet.Head = Header::DataResp;
        packet.Data = a_payload;
    }
    else {
        packet.Head = Header::FileSent;
    }

    return m_handle->send( serialize(packet) );
}

/*****************************************************************************
 * Ожидание между неуспешными итерациями цикла-обработчика, мсек
 */
void C_Server::sleep( std::chrono::milliseconds a_sleepTime )
{
    std::this_thread::sleep_for( a_sleepTime );
}

/*****************************************************************************
 * Загрузка файла в память
 *
 * @param
 *  [in] a_filePath - путь, по которому находится файл с данными
 */
void C_Server::loadFile( std::string a_filePath )
{
    if ( a_filePath.empty() ) {
        return;
    }
    m_file.open( a_filePath, std::ios::binary | std::ios::in );
    g_log << m_name << "file open status: " << m_file.is_open() << std::endl;

    m_packetProvider = std::make_unique<C_StreamAnalyzer>( m_file, m_data );
    if ( m_packetProvider->calcIndex() ) {
        g_log << m_name << "file indexed" << std::endl;
    }
    else {
        g_log << m_name << "problem with indexing file" << std::endl;
    }
}

/*****************************************************************************
 * Отправка пакета клиенту
 *
 * @param
 *  [in] a_idx       - текущий номер пакета, подлежащего отправке
 *  [in] a_prevTime  - временная метка предыдущего пакета
 *  [in] a_sleepTime - время задержки между пакетами
 *
 * @return
 *  Статус успешности отправки пакета
 *  true  - сервер успешно отправил данные
 *  false - ошибка при отправке
 */
bool C_Server::processPacket( unsigned long& a_idx,
                              std::chrono::milliseconds& a_prevTime,
                              std::chrono::milliseconds& a_sleepTime )
{
    using namespace std::chrono_literals;
    // Получение указателя на пакет под номером a_idx
    const T_Packet* packetPtr = m_packetProvider->getPacketPtr(a_idx);
    auto packetTime = std::chrono::milliseconds(packetPtr->Time);

    // Расчет времени задержки между пакетами
    auto nonNullDelay = 10ms;
    auto diffPacketTime = packetTime - a_prevTime;

    if ( diffPacketTime < 10ms ) {
        a_sleepTime = diffPacketTime + nonNullDelay;
    }
    else {
        a_sleepTime = diffPacketTime;
        //a_sleepTime = nonNullDelay; //time boost
    }

    a_prevTime = packetTime;
    // Получение границ пакета под номером a_idx
    auto iters = m_packetProvider->packetRange(a_idx);
    // Отправка пакета клиенту с заголовком Header::DataResp
    if ( sendPacket( Comand::Data, { iters.first, iters.second } ) ) {
        g_log << m_name << "send packet #" << a_idx << std::endl;
        a_idx++;
        // Вывод мета-информации пакета на экран
        print( packetPtr );
        return true;
    }
    else {
        g_log << m_name << "problem with sending packet: " << a_idx << std::endl;
        return false;
    }
}

/*****************************************************************************
 * Извлечение команды из пакета данных
 *
 * @param
 *  [in] a_packet - ссылка на пакет, который требуется проанализировать
 *
 * @return
 *  E_Comands::FinishWork || E_Comands::ReqtNumber - вызвращает команду из пакета
 *  E_Comands::Invalid  - в принятом пакете нет никакой команды
 */
Comand C_Server::parseComand() const
{
    T_NetPacket recvPacket = deserialize(m_buffer);

    if ( recvPacket.Head == Header::DataReqt ) {
        return Comand::Data;
    }
    else {
        return Comand::Invalid;
    }
}

} // namespace network
