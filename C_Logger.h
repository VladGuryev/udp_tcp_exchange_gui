/******************************************************************************

  C_Logger

  Класс - лог (журнал)


  ОПИСАНИЕ

  * C_Logger - класс журнала. Вывод сообщений в лог реализуется через оператор вывода <<

  * Сам класс не учавствует в выводе сообщений в консоль, на экран или в файл, а лишь
    сигнализирует о вновь появившемся сообщении, которое получит тот, кто подпишется на сигналы
    глобального объекта g_log.

  * Многопоточный доступ к набору перегруженных по входному параметру операторов << осуществляется
    с помощью синхронизации мьютексом.


  ИСПОЛЬЗОВАНИЕ

  * Создание журнала может производиться следующим образом:

    В файле реализации создается объект класса (глобальный логгер)
    std::shared_ptr<services::C_Logger> g_log = std::make_shared<services::C_Logger>()

    В заголовочном файле объявляется глобальный логгер:

    extern std::shared_ptr<services::C_Logger> g_log, который виден в глобальной области видимости.

*******************************************************************************/

#pragma once

#include <QObject>
#include <QVariant>
#include <QString>

#include <mutex>
#include <memory>

namespace services {

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
 * Класс журнала сообщений, поддерживающий многопоточное обращение
 */
class C_Logger : public QObject
{

    Q_OBJECT

public:

    C_Logger()  = default;
    ~C_Logger() = default;

    /*****************************************************************************
     * Для поддержки строковых литералов
     *
     * @param
     *  [in] ptr   - ссылка на указатель логгера, завернутый в shared_ptr
     *  [in] input - входной аргумент для вывода в журнал (строковые литералы)
     *
     * @return
     *  - ссылка на указатель логгера, для поддержания конвеерного вызова функции
     */
    template<typename T>
    friend const std::shared_ptr<C_Logger> & operator<<( const std::shared_ptr<C_Logger>& ptr, const T& input )
    {
        std::lock_guard<std::mutex> lock( ptr->m_mutexCout );
        emit ptr->addedNewLogLine( QVariant(input) );
        return ptr;
    }

    /*****************************************************************************
     * Для поддержки long unsigned int
     *
     * @param
     *  [in] ptr   - ссылка на указатель логгера, завернутый в shared_ptr
     *  [in] input - входной аргумент типа long unsigned int для вывода в журнал
     *
     * @return
     *  - ссылка на указатель логгера, для поддержания конвеерного вызова функции
     */
    friend const std::shared_ptr<C_Logger> & operator<<( const std::shared_ptr<C_Logger>& ptr, const long unsigned int& input )
    {
        std::lock_guard<std::mutex> lock( ptr->m_mutexCout );
        emit ptr->addedNewLogLine( QString::number(input) );
        return ptr;
    }

    /*****************************************************************************
     * Для поддержки long int
     *
     * @param
     *  [in] ptr   - ссылка на указатель логгера, завернутый в shared_ptr
     *  [in] input - входной аргумент типа long int для вывода в журнал
     *
     * @return
     *  - ссылка на указатель логгера, для поддержания конвеерного вызова функции
     */
    friend const std::shared_ptr<C_Logger> & operator<<( const std::shared_ptr<C_Logger>& ptr, const long int& input )
    {
        std::lock_guard<std::mutex> lock( ptr->m_mutexCout );
        emit ptr->addedNewLogLine( QString::number(input) );
        return ptr;
    }

    /*****************************************************************************
     * Для поддержки std::string
     *
     * @param
     *  [in] ptr   - ссылка на указатель логгера, завернутый в shared_ptr
     *  [in] input - входной аргумент типа std::string для вывода в журнал
     *
     * @return
     *  - ссылка на указатель логгера, для поддержания конвеерного вызова функции
     */
    friend const std::shared_ptr<C_Logger> & operator<<( const std::shared_ptr<C_Logger>& ptr, const std::string& input )
    {
        std::lock_guard<std::mutex> lock( ptr->m_mutexCout );
        QString str = QString::fromStdString(input);
        emit ptr->addedNewLogLine( QVariant(str) );
        return ptr;
    }

    /*****************************************************************************
     * Для поддержки std::endl
     *
     * @param
     *  [in] ptr   - ссылка на указатель логгера, завернутый в shared_ptr
     *  [in] input - входной аргумент типа указателя на манипулятор выходного
     *               потока (указатель на функцию) для вывода в журнал
     *
     * @return
     *  - ссылка на указатель логгера, для поддержания конвеерного вызова функции
     */
    friend const std::shared_ptr<C_Logger> &operator<<( const std::shared_ptr<C_Logger>& ptr, std::ostream& (*os)(std::ostream&) )
    {
        Q_UNUSED(os)
        std::lock_guard<std::mutex> lock( ptr->m_mutexCout );
        emit ptr->addedNewLogLine( QVariant("\n") );
        return ptr;
    }

signals:

    // Сигнал помещения в журнал новой строки
    void addedNewLogLine( const QVariant& );

private:

    std::mutex m_mutexCout; // Мьютекс для защиты разделяемого между потоками строкового вывода
};

/*****************************************************************************
  Functions Prototypes
*****************************************************************************/

/*****************************************************************************
  Variables Deсlarations
*****************************************************************************/

// Глобальный логгер
extern std::shared_ptr<services::C_Logger> g_log;

/*****************************************************************************
  Inline Functions Definitions
*****************************************************************************/

} // namespace services
