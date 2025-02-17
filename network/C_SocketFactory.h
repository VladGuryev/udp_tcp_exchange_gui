/******************************************************************************

  C_SocketFactory

  Класс-фабрика для создания сокетов, реализующих интерфейс I_Socket


  ОПИСАНИЕ

  * Класс имеет единственный статический метод

    std::shared_ptr<I_Socket>
    createSocket( E_Protocol a_protocol, E_SocketType a_sockType )

  * Метод конструирует сокет согласно переданным параметрам:

    1. Тип протокола: UDP/TCP (см. common_types.h)
    2. Тип сокета: клиентский, серверный (см. common_types.h)


  ИСПОЛЬЗОВАНИЕ

  * Для того, чтобы создать сокет, необходимо вызвать метод, например так:

    std::shared_ptr<I_Socket> socket =
           C_SocketFactory::createSocket( E_Protocol::UDP, E_SocketType::Client );

    Тип сокета в рантайме будет C_UdpClientSocket

*******************************************************************************/

#pragma once

#include <memory>

#include "I_Socket.h"
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
 * Класс-фабрика для создания сокетов, реализующих интерфейс I_Socket
 */
class C_SocketFactory
{

public: //static

    // Фабричная функция для создания cокетов
    static std::shared_ptr<I_Socket> createSocket( E_Protocol a_protocol );
};

/*****************************************************************************
  Functions Prototypes
*****************************************************************************/

/*****************************************************************************
  Variables Definitions
*****************************************************************************/

/*****************************************************************************
  Inline Functions Definitions
*****************************************************************************/

} // namespace network

