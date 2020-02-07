/*****************************************************************************

  C_UdpSocket

  Класс UDP-сокета, реализующий интерфейс I_Socket поверх системной библиотеки
  сокетов ОС Windows - WINSOCK


  ДЕТАЛИ РЕАЛИЗАЦИИ

  * C_UdpSocket наследует C_Socket, в котором распологается основной функционал
    по работе с сокетами на уровне WinAPI (см. C_Socket.h).

*****************************************************************************/

#include "C_UdpSocket.h"

#include <thread>

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
 * Отправка данных через сокет
 *
 * @param
 *  [in] a_buff - ссылка на буфер, данные из которого отправляются по сокету
 *
 * @return
 *  sendStat - статус успешности отправки
 *  sendStat != SOCKET_ERROR - отправка произошла успешно
 */
bool C_UdpSocket::send( const std::vector<char> &a_buff )
{
    int socketAddrSize = sizeof( m_peerService );

    auto numBytes = sendto( m_masterSock,
                            a_buff.data(),
                            a_buff.size(),
                            0,
                            reinterpret_cast<struct sockaddr*>(&m_peerService),
                            socketAddrSize );

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
 *  [out] a_buff - ссылка на буфер, в который помещаются принятые данные из сокета
 *
 * @return
 *  recvStat - статус успешности приема
 *  recvStat != SOCKET_ERROR - прием произошел успешно
 */
bool C_UdpSocket::recv( std::vector<char> &a_buff )
{
    int socketAddrSize = sizeof( m_peerService );

    memset( a_buff.data(), 0, a_buff.size() );

    auto numBytes = ::recvfrom( m_masterSock,
                                a_buff.data(),
                                static_cast<int>( a_buff.size() ),
                                0,
                                reinterpret_cast<struct sockaddr*>(&m_peerService),
                                &socketAddrSize );

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

} // namespace network
