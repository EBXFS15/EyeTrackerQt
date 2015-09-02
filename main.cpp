#include "eyetrackerwindow.h"
#include <QApplication>

#include <signal.h>
#include <stdio.h>
#include <QImage>
#include <QPixmap>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    EyeTrackerWindow w;
    w.show();
    return a.exec();
}
