#include "eyetrackerwindow.h"
#include <QApplication>

#include <signal.h>
#include <stdio.h>
#include <QImage>
#include <QPixmap>

static int quit_signal=0;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    EyeTrackerWindow w;
    w.show();
    w.getImage();
    return a.exec();
}
