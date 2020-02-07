/*****************************************************************************

  C_StreamAnalyzer

  Класс для индексации пакетов внутри файла с данными в формате *.mes


  ОПИСАНИЕ

  * Парсер позволяет анализировать буфер с данными и находить указатели и итераторы
    на каждый пакет в буфере


  ИСПОЛЬЗОВАНИЕ


    std::fstream 		m_file;             // Хендлер файла с данными
    std::vector<char> 	m_data;             // Буфер с данными из файла

  * Откроем файл:

    m_file.open( a_filePath, std::ios::binary | std::ios::in );

  * Создание парсера файла *.mes расположенного по пути a_filePath и передача ему файлового потока и буфера,
    в который будет производится выгрузка данных из файла:

    std::unique_ptr<C_StreamAnalyzer> m_packetProvider = std::make_unique<C_StreamAnalyzer>( m_file, m_data );

  * Попытка расчитать индекс файла через вызов функции calcIndex():

    if ( m_packetProvider->calcIndex() ) {
        cout <<  "file indexed" << std::endl;
    }
    else {
        cout << "problem with indexing file" << std::endl;
    }

  * Получение пары итераторов на заголовок файла:

    auto headIters = m_packetProvider->headerRange();

  * Конструирование буффера с данными из заголовка по итераторам:

    const std::vector<char> & header_payload =  { headIters.first, headIters.second };

  * Получение количества пакетов в файле:

    long long packetCount = m_packetProvider->packetCount();

  * Получение пакета под номером 100 (надо учесть чтобы индекс запрашиваемого пакета был меньше величины packetCount)
    int packetIndex = 100;

  * Получение пары итераторов на 100ый пакет:

    auto iters = m_packetProvider->packetRange(packetIndex);

  * Конструирование буффера с данными из 100ого пакета:

    const std::vector<char> & some_packet_payload = { iters.first, iters.second }

*****************************************************************************/

#pragma once

#include <istream>
#include <vector>
#include <utility>

#include "C_Logger.h"
#include "common_types.h"

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

class C_StreamAnalyzer
{

public: //types

    using iter_t  = typename std::vector<char>::const_iterator;     // Итератор на начало/конец пакета
    using range_t = std::pair< iter_t, iter_t >;                    // Тип диапазона пакета в буфере
    using index_t = std::vector<range_t>;                           // Тип диапазонов всех пакетов в буфере

public:

    C_StreamAnalyzer()                           = delete;
    C_StreamAnalyzer( const C_StreamAnalyzer&  ) = delete;
    C_StreamAnalyzer(       C_StreamAnalyzer&& ) = delete;
    C_StreamAnalyzer & operator = ( const C_StreamAnalyzer&  ) = delete;
    C_StreamAnalyzer & operator = (       C_StreamAnalyzer&& ) = delete;

    C_StreamAnalyzer( std::istream &a_stream, std::vector<char> &a_buffer );
    virtual ~C_StreamAnalyzer() = default;

    // Границы заголовка внутри буфера
    range_t headerRange();
    // Границы пакета внутри буфера
    range_t packetRange( std::size_t a_packNo );
    // Получение индекса с пакетами в буфере
    const index_t & index();
    // Получение индекса с пакетами в буфере (константная версия)
    const index_t & index() const;
    // Получение указателя на пакет
    const T_Packet * getPacketPtr( unsigned long a_packNo );
    // Получение количества пакетов внутри буфера
    unsigned long packetCount();
    // Расчет границ пакетов внутри буфера с данными
    bool calcIndex();

private:

    // Реализация расчета границ пакетов внутри буфера с данными
    virtual bool doCalcIndex();
    // Преобразование итератора на некоторый пакет в указатель на этот пакет
    const T_Packet * iterToPtr( iter_t a_iter );

    std::istream       &m_stream;       // Поток из которого считываются данные для индексации
    std::vector<char>  &m_buffer;       // Ссылка на буфер с данными, который необходимо разметить
    index_t             m_index;        // Диапазоны всех пакетов в буфере

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
