/*****************************************************************************

  Вспомогательные функции

*****************************************************************************/

#include "utils.h"

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
 * Парсинг ip-адреса и порта из строки
 *
 * @param
 *  [in] a_configStr - строка в формате (квадратными скобками отмечены не обязательные поля):
 *                     ip_address1:port1 [ip_address2:port2] [mode=[non]blocking]
 *
 * @return
 *  enpoints - вектор I_Socket::endpoint из адреса, порта и режима сокета
 */
std::vector<I_Socket::endpoint> parseEndpointStr( std::string a_configStr )
{
    // Парсинг режима сокета (блокирующий/неблокирующий)
    E_SocketMode blockingMode = E_SocketMode::Blocking;
    std::size_t foundPos = a_configStr.find( "nonblocking" );
    if ( foundPos != std::string::npos ) {
        blockingMode = E_SocketMode::NonBlocking;
    }

    // Создание строки, содержащей только адреса и порты без "mode"
    std::string modeRemovedStr = a_configStr;
    foundPos = a_configStr.find( "mode" );
    if ( foundPos != std::string::npos ) {
        modeRemovedStr = a_configStr.substr( 0, foundPos );
    }

    // Разбитие строки по пробелам на вектор строк
    std::istringstream iss( modeRemovedStr );
    std::vector<std::string> spaceSplitted( std::istream_iterator<std::string>{ iss },
                                            std::istream_iterator<std::string>() );

    // Выяснение отдельных адресов и портов каждого endpoint
    std::vector<I_Socket::endpoint> enpoints;
    for( auto& item : spaceSplitted ) {
        std::string addr;
        uint16_t port;
        foundPos = item.find_first_of(":");
        if ( foundPos != std::string::npos ) {
            addr = item.substr( 0, foundPos );
            port = static_cast<uint16_t>( std::stoi( item.substr( ++foundPos ) ) );

        }
        auto endpoint = std::make_tuple( addr, port, blockingMode );
        enpoints.push_back(endpoint);
    }
    return enpoints;
}

/*****************************************************************************
 * Вывод заголовка на экран
 *
 * @param
 *  [in] a_header - указатель на заголовок
 */
void print( const T_PacketFileHeader *a_header ) {
    using namespace std;
    if( a_header ) {
        g_log << "--------PACKET FILE HEADER--------" << endl;
        g_log << "Type: ";
        for( uint32_t i = 0; i < 4; i++ ) {
            g_log <<  a_header->Type.TypeStr[i];
        }
        g_log << endl;
        //----------------------------------------------------------------------------
        g_log << "Stream quan: "     << a_header->StreamQuan    << endl;
        g_log << "Records in file: " << a_header->RecordsInFile << endl;
        g_log << "Record time: "     << a_header->RecordTime    << endl;
        //----------------------------------------------------------------------------
        g_log << "RecordName: ";
        for( uint32_t i = 0; i < 64; i++ ) {
            g_log << a_header->RecordName[i];
        }
        g_log << endl;

        g_log << "Last Change Time: " << a_header->LastChangeTime   << endl;

        g_log << "Info: ";
        for( uint32_t i = 0; i < 60; i++ ) {
            g_log << a_header->Info[i];
        }
        g_log << endl;
        g_log << "----------------------------------" << endl;
    }
}

/*****************************************************************************
 * Вывод пакета на экран
 *
 * @param
 *  [in] a_packet - указатель на пакет
 */
void print( const T_Packet *a_packet ) {
    using namespace std;
    if( a_packet ) {
        g_log << "Time: "          << a_packet->Time      << " ms" << endl;
        g_log << "Data size: "     << a_packet->DataSize  << " bytes" <<endl;
      //  g_log << "Stream number: " << a_packet->StreamNum << endl;
      //  g_log << "Info: "          << a_packet->Info      << endl;
        g_log << "----------------------------------" << endl;
    }
}

/*****************************************************************************
 * Вывод данных пакета на экран
 *
 * @param
 *  [in] a_packet - указатель на пакет
 */
void printData( const T_Packet *a_packet ) {
    if( a_packet ) {
        g_log << "Packet Data: ";
        for( uint32_t i = 0; i < a_packet->DataSize; i++ ) {
            g_log << a_packet->Data[i];
        }
        g_log << std::endl << "----------------------------------" << std::endl;
    }
}

/*****************************************************************************
 * Расчет размера пакета
 *
 * @param
 *  [in] a_packet - указатель на пакет
 *
 * @return
 *  - размер пакета в байтах
 */
unsigned int getPacketSize( T_Packet *a_packet ) {
    return sizeof(T_Packet) + a_packet->DataSize;
}

/*****************************************************************************
 * Расчет размера заголовка
 *
 * @return
 *  - размер заголовка в байтах
 */
unsigned int getHeaderSize() {
    return sizeof(T_PacketFileHeader);
}

/*****************************************************************************
 * Сериализация сетевого пакета в байтовый поток
 *
 * @param
 *  [in] a_packet - пакет, подлежащий сериализации
 *
 * @return
 *  - байтовый буфер
 */
std::vector<char> serialize( const T_NetPacket &a_packet )
{
    uint16_t value = static_cast<uint16_t>(a_packet.Head);
    char lo = value & 0xFF;
    char hi = value >> 8;

    std::vector<char> outputBuf;
    outputBuf.resize( sizeof(uint16_t) + a_packet.Data.size() );
    outputBuf[0] = hi;
    outputBuf[1] = lo;

    memcpy( &outputBuf[2], a_packet.Data.data(), a_packet.Data.size() );
    return outputBuf;
}

/*****************************************************************************
 * Десериализация сетевого пакета из байтового потока
 *
 * @param
 *  [in] a_buffer - входящий байтовый поток
 *
 * @return
 *  - десериализованный из байтового потока пакет
 */
T_NetPacket deserialize( const std::vector<char> &a_buffer )
{
    unsigned char hi = a_buffer[0];
    unsigned char lo = a_buffer[1];

    T_NetPacket packet;
    packet.Head = (Header)( uint16_t(hi) << 8 | lo  );
    packet.Data.assign( &a_buffer[2], &a_buffer[2] + a_buffer.size() - 2 );

    return packet;
}

} // namespace network
