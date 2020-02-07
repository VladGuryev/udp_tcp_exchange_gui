/*****************************************************************************

  C_StreamAnalyzer

  Класс для индексации пакетов внутри файла с данными в формате *.mes


  ДЕТАЛИ РЕАЛИЗАЦИИ

  * Нахождение указателей в буфере на каждый пакет осуществляется в закрытой
    функции doCalcIndex(), которая рекуррентно обходит буфер с данными и сохраняет
    рассчитанные диапазоны пакетов в буфер.

*****************************************************************************/

#include "C_StreamAnalyzer.h"

#include <limits>

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
 * Конструктор
 */
C_StreamAnalyzer::C_StreamAnalyzer( std::istream &a_stream,
                                    std::vector<char> &a_buffer )
    : m_stream(a_stream), m_buffer(a_buffer)
{
    m_stream.ignore( std::numeric_limits<std::streamsize>::max() );
    std::streamsize length = m_stream.gcount();
    m_stream.clear();   //  Since ignore will have set eof
    m_stream.seekg( 0, std::ios_base::beg );
    g_log << "file size: " << length << std::endl;

    m_buffer.clear();
    m_buffer.resize(length);
    m_stream.read( m_buffer.data(), length );
}

/*****************************************************************************
 * Границы заголовка внутри файла
 *
 * @return
 *  - пара итераторов на начало и конец заголовка в буфере
 */
C_StreamAnalyzer::range_t C_StreamAnalyzer::headerRange()
{
    return std::make_pair< iter_t, iter_t >
           ( m_buffer.begin(), std::next( m_buffer.begin(),
                                          sizeof(T_PacketFileHeader) ) );
}

/*****************************************************************************
 * Границы пакета внутри файла
 *
 * @param
 *  [in] a_packNo - номер пакета, границы которого необходимо получить
 *
 * @return
 *  - итераторы начала и конца пакета
 */
C_StreamAnalyzer::range_t C_StreamAnalyzer::packetRange( std::size_t a_packNo )
{
    if ( m_index.size() == 0 ) {
        g_log << "index is empty" << std::endl;
        return range_t{};
    }

    if ( a_packNo < m_index.size() ) {
        return m_index.at(a_packNo);
    }
    else {
        g_log << "required packet index is out of range" << std::endl;
        return range_t{};
    }
}

/*****************************************************************************
 * Получение количества пакетов внутри буфера
 *
 * @return
 *  - количество пакетов в буфере
 */
unsigned long C_StreamAnalyzer::packetCount()
{
    return reinterpret_cast<T_PacketFileHeader*>( m_buffer.data() )->RecordsInFile;
}

/*****************************************************************************
 * Получение индекса с пакетами в буфере
 *
 * @return
 *  - вектор с диапазонами пакетов
 */
const C_StreamAnalyzer::index_t & C_StreamAnalyzer::index()
{
    return m_index;
}

/*****************************************************************************
 * Получение индекса с пакетами в буфере (перегруженная по константности версия функции)
 *
 * @return
 *  - вектор с диапазонами пакетов
 */
const C_StreamAnalyzer::index_t & C_StreamAnalyzer::index() const
{
    return m_index;
}
/*****************************************************************************
 * Получение указателя на пакет
 *
 * @param
 *  [in] a_packNum - номер необходимого пакета
 *
 * @return
 *  - указатель на пакет
 */
const T_Packet * C_StreamAnalyzer::getPacketPtr( unsigned long a_packNo )
{
    return iterToPtr( packetRange( a_packNo ).first );
}

/*****************************************************************************
 * Расчет границ пакетов внутри буфера с данными
 *
 * @return
 * Результат успешности расчета индекса
 *  true  - расчет индекса произведен успешно
 *  false - при расчете индекса произошла ошибка
 */
bool C_StreamAnalyzer::calcIndex()
{
    return doCalcIndex();
}

/*****************************************************************************
 * Реализация расчета границ пакетов внутри буфера с данными
 *
 * @return
 * Результат успешности расчета индекса
 *  true - расчет индекса произведен успешно
 *  false - при расчете индекса произошла ошибка
 */
bool C_StreamAnalyzer::doCalcIndex()
{
    // Подсчет текущего количества проанализированных байт буфера
    unsigned long currentByteSum = 0;
    // Нахождение количества пакетов в буфере
    unsigned long allPacketCount = packetCount();
    m_index.resize(allPacketCount);

    // Получение итераторов начала и конца заголовка буфера
    range_t headIters = headerRange();
    auto headSize = std::distance( headIters.first, headIters.second );
    currentByteSum += headSize;
    /* Для нахождения размера первого пакета необходимо найти указатель на начало этого пакета.
     * Итератор на конец заголовка является итератором на начало первого пакета в буфере. Его
     * можно привести к указателю на первый пакет.
     */
    const T_Packet* firstPacketPtr = iterToPtr( headIters.second );
    auto firstPacketSize = sizeof(T_Packet) + firstPacketPtr->DataSize;
    currentByteSum += firstPacketSize;

    // Проверка на валидность данных первого пакета
    if ( currentByteSum > m_buffer.size() ) {
        g_log << "Error occured while calculating m_index[" << 0 << "]" << std::endl;
        return false;
    }

    // Получение диапазона первого пакета и запись его в индекс
    m_index[0] = { headIters.second, std::next( headIters.second, firstPacketSize ) };

    // Поиск диапазонов всех пакетов в буфере
    for( std::size_t i = 1; i < allPacketCount; i++ ) {
         // Получение указателя на первый пакет
         const T_Packet* curPacketPtr = iterToPtr( m_index[i - 1].second );
         auto curPacketSize = sizeof(T_Packet) + curPacketPtr->DataSize;
         currentByteSum += curPacketSize;

         // Проверка на валидность данных i-ого пакета
         if ( currentByteSum > m_buffer.size() ) {
             g_log << "Error occured while calculating m_index[" << i << "]" << std::endl;
             return false;
         }

         m_index[i] = { m_index[i - 1].second,
                        std::next( m_index[i - 1].second, curPacketSize ) };
    }
    return true;
}

/*****************************************************************************
 * Преобразование итератора на некоторый пакет в указатель на этот пакет
 *
 * @return
 *  - указатель на пакет
 */
const T_Packet * C_StreamAnalyzer::iterToPtr( C_StreamAnalyzer::iter_t a_iter )
{
    return reinterpret_cast<const T_Packet*>( &( *a_iter ) );
}


} // namespace network


