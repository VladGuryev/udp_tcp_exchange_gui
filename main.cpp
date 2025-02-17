/*****************************************************************************

  MAIN FUNCTION

  Запуск графического окна приложения

*****************************************************************************/

#include <QApplication>

#include "C_MainWindow.h"

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
 * Основная программа
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    C_MainWindow w;
    w.show();

    return a.exec();
}
