/*****************************************************************************

  C_Socket

  Базовый абстрактный класс сокета, частично реализующий интерфейс I_Socket,
  общий для UDP/TCP сокетов


  ОПИСАНИЕ

  * Класс С_Socket реализует общую функциональность UDP/TCP сокетов, в частности
    настройку сокета, инициализацию WinSock библиотеки,
    а также содержит общий для UDP/TCP поля данных.


  ИСПОЛЬЗОВАНИЕ

  * Использование объектов класса C_Socket равносильно работе с объектами,
    реализующими интерфейс I_Socket (см. I_Socket).

  * Класс содержит чистый абстрактный метод:
    createAppropriateSocket(), который следует переопределить для правильного создания
    сокета необходимого типа (UDP/TCP).

*****************************************************************************/

#pragma once

#include <winsock2.h>
#include <memory>
#include <functional>

#include "I_Socket.h"
#include "utils.h"
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
 * Базовый класс сокета, частично реализующий интерфейс I_Socket
 */
class C_Socket : public I_Socket
{

public:

    C_Socket( E_Protocol a_proto, std::string a_name );
    virtual ~C_Socket() override;

public:

    /**
     * Интерфейс I_Socket
     */

    // Инициализация параметров сокета, задание адреса и порта созданного сокета
    virtual bool setup(const std::string &a_configuration ) override;
    // Сброс настроек сокета к начальным значениям
    virtual bool flush() override;

    // Вызов системного API для создания сокета
    virtual bool open() override;
    // Деинициализация сокета, закрытие библиотеки WinSock
    virtual void close() override;

    // Лог-метка сокета
    virtual std::string name() const override;

protected:

    /**
     * Возможность добавить необходимые настроки сокету на этапе конфигурации сокета
     * Вызов этого метода производится в теле функции initialize()
     */
    virtual bool setSockOptions() { return true; }

    // Конфигурация параметров сокета
    virtual bool configSock ( const I_Socket::endpoint &a_endpoint );

    // Настройка параметров адресата (для клиента)
    virtual void setPeerAddr( const I_Socket::endpoint &a_endpoint );

    // Установка режима сокета (блокирующий/неблокирующий)
    bool setNonBlocking( E_SocketMode a_mode );

    // Инициализация библиотеки WinSock
    bool initLib();

protected: //types

    struct ProtoCredentials {                       // Параметры используемого протокола
        int domain;
        int type;
        int protocol;
    } m_protoCred;

protected:

    SOCKET          m_masterSock = INVALID_SOCKET;          // Файловый дескриптор сокета
    E_Protocol      m_protoType;                            // Тип протокола создаваемого сокета
    E_SocketType    m_socketType = E_SocketType::Invalid;   // Тип сокета: клиентский/серверный
    sockaddr_in     m_myService;                            // Структура сокета
    sockaddr_in     m_peerService;                          // Структура для хранения адреса получателя
    std::string     m_name;                                 // Метка сокета

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
