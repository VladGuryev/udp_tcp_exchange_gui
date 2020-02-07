/****************************************************************************

  I_Socket

  Класс интерфейса для создания сокетов


  ОПИСАНИЕ ИНТЕРФЕЙСА

  Интерфейс описывает набор функций для создания, настройки, использования и закрытия
  сокета. Класс I_Socket создан для унифицированной работы с разными типами сокетов:
  UDP,TCP.

  1. Общий интерфейс

     * Настройка созданного сокета:

       virtual bool setup( const std::vector<endpoint> &endpoints ) = 0;

       где endpoints - вектор из пар, каждая из которых описывает адрес и порт
       для сокета и пункта назначения, если такой задан (для клиента)

     * Закрытие сокета:

       virtual void close() = 0;

     * Отправка пакета данных a_buff, представляющего собой вектор символов:

       virtual int send( std::vector<char> &a_buff ) = 0;

     * Прием данных в буфер a_buff:

       virtual int recv( std::vector<char> &a_buff ) = 0;

     * Получение описания сокета:

       virtual std::string name() const = 0;

  2. TCP интерфейс

     * Подключение клиентского сокета к серверу (для клиентского сокета):

     virtual int connect() = 0;

     * Принятие запроса на установку соединения (для серверного сокета):

     virtual int accept()  = 0;

     * Установка слушающего сокета на backlog подключений (для серверного сокета):

     virtual int  listen( int  backlog ) = 0;

     * Установка режима сокета (блокирующий/неблокирующий):

     virtual bool setNonBlocking( bool nonBlock ) = 0;


  ОПИСАНИЕ ИСПОЛЬЗОВАНИЯ

     * Предположим, что мы имеет объект потомка класса I_Socket, созданный
       прямым вызовом конструктора или через вызов фабричного метода и реализующий
       логику TCP сокета.

       std::shared_ptr<I_Socket> sockObj = new MySocket();
       std::shared_ptr<I_Socket> sockObj = createSocket();

     * Тогда использование сокета будет иметь вид похожий на следующую цепочку
       вызовов:

       а) для клиентского сокета:

                 client socket {addr,port}     destination {addr, port}
       * sockObj->setup( { {"192.168.1.1", 5890},    {"254.117.0.8", 6060} } );

       * sockObj->connect();

       * Optionally: sockObj->setNonBlocking( true );

       * Прием - передача данных:

          sockObj->send(a_packet);
          sockObj->recv(m_buff);

       б) для серверного сокета:

                  server socket {addr,port}
       * sockObj->setup( { {"254.117.0.8", 6060} } );

       * int new_fs = sockObj->listen( numberOfAvailableClients );

       * sockObj->accept();

       * Прием - передача данных:

          sockObj->send(a_packet);
          sockObj->recv(m_buff);


*****************************************************************************/

#pragma once

#include <vector>
#include <string>
#include <tuple>

#include "common_types.h"

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
 * Класс интерфейса для создания TCP/UDP сокетов
 */
class I_Socket
{

public: // types

    using endpoint = std::tuple< std::string, uint16_t, E_SocketMode >;

public:

    // Открытие сокета
    virtual bool open() = 0;
    // Закрытие сокета
    virtual void close() = 0;

    // Настройка адреса и порта созданного сокета
    virtual bool setup( const std::string &a_configuration ) = 0;
    // Сброс настроек сокета
    virtual bool flush() = 0;

    // Инициализировать создание сессии
    virtual bool connect() = 0;

    // Отправка данных
    virtual bool send( const std::vector<char> &a_buff ) = 0;
    // Прием данных
    virtual bool recv(       std::vector<char> &a_buff ) = 0;

    // Метка сокета
    virtual std::string name() const = 0;

public:

    I_Socket()                   = default;
    I_Socket( const I_Socket&  ) = delete;
    I_Socket(       I_Socket&& ) = delete;
    I_Socket & operator = ( const I_Socket&  ) = delete;
    I_Socket & operator = (       I_Socket&& ) = delete;

    virtual ~I_Socket() noexcept = default;
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
