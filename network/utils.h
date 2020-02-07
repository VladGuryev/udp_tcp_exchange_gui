/*****************************************************************************

  Вспомогательные функции

*****************************************************************************/

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <iterator>
#include <sstream>
#include <chrono>

#include <QDebug>

#include "I_Socket.h"
#include "C_Logger.h"

namespace network {

using namespace services;

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
 * Парсинг ip-адреса и порта из строки
 */
std::vector<I_Socket::endpoint> parseEndpointStr( std::string a_configStr );

/*****************************************************************************
  Variables Definitions
*****************************************************************************/

/*****************************************************************************
  Inline Functions Definitions
*****************************************************************************/

/*****************************************************************************
 * Преобразование long long в мсек вместо неработающего литерала operator ""_s
 */
constexpr std::chrono::milliseconds to_ms( long long a_msecs )
{
    return std::chrono::duration< int64_t, std::milli > { a_msecs };
}

/*****************************************************************************
 * Вывод заголовка на экран
 */
void print( const network::T_PacketFileHeader *a_header );

/*****************************************************************************
 * Вывод пакета на экран
 */
void print( const network::T_Packet *a_packet );

/*****************************************************************************
 * Вывод данных пакета на экран
 */
void printData( const network::T_Packet *a_packet );

/*****************************************************************************
 * Расчет размера пакета
 */
unsigned int getPacketSize( T_Packet* a_packet );

/*****************************************************************************
 * Расчет размера заголовка
 */
unsigned int getHeaderSize();

/*****************************************************************************
 * Приведение указателя T* к T_Packet*
 *
 * @param
 *  [in] a_rawPtr - некоторый указатель на область памяти в которой лежит T_Packet
 *
 * @return
 *  - указатель на T_Packet
 */
template <typename T>
T_Packet* toPacketPtr( const T* a_rawPtr )
{
   T* noConstPtr = const_cast<T*>( a_rawPtr );
   T_Packet* packetPtr = reinterpret_cast<T_Packet*>(noConstPtr);
   return packetPtr;
}

/*****************************************************************************
 * Приведение указателя T* к T_PacketFileHeader*
 *
 * @param
 *  [in] a_rawPtr - некоторый указатель на область памяти в которой лежит T_PacketFileHeader
 *
 * @return
 *  - указатель на T_PacketFileHeader
 */
template <typename T>
T_PacketFileHeader* toPacketHeaderPtr( const T* a_rawPtr )
{
   T* noConstPtr = const_cast<T*>( a_rawPtr );
   T_PacketFileHeader* packetPtr = reinterpret_cast<T_PacketFileHeader*>(noConstPtr);
   return packetPtr;
}

/*****************************************************************************
 * Сериализация сетевого пакета в байтовый поток
 */
std::vector<char> serialize( const T_NetPacket &a_packet );

/*****************************************************************************
 * Десериализация сетевого пакета из байтового потока
 */
T_NetPacket deserialize( const std::vector<char> &a_buffer );

} // namespace network
