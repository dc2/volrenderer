#include <QApplication>
#include <QMainWindow>

#include "common.h"
#include "ui/MainWindow.h"
#include "Widgets/VolRenderer.h"

using namespace std;

int main(int argc, char* argv[])
{
    // enable threaded rendering on X11
    //QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
    QApplication a(argc, argv);
    
    MainWindow w;
    w.show();

    return a.exec();
}
