#include "eyetrackerwindow.h"
#include <QApplication>

//#include <opencv/cv.h>
//#include <opencv/cv.h>
//#include <opencv/highgui.h>
#include <signal.h>
#include <stdio.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
#include <QImage>
#include <QPixmap>

//using namespace cv;

volatile int quit_signal=0;

void quit_signal_handler(int signum) {
 if (quit_signal!=0) exit(0); // just exit already
 quit_signal=1;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    EyeTrackerWindow w;
    w.show();
    signal(SIGINT,quit_signal_handler); // listen for ctrl-C
        //CvCapture* capture =cvCreateCameraCapture(0);
        if (false)//!capture)
        {
            printf("Error. Cannot capture.");
        }
        else
        {
            printf("Start capture...");
//            //for(;;)
//            {
//                //cvSaveImage("test.png",frame);
//                if(quit_signal)
//                {
//                    //cvReleaseCapture(&capture);
//                    //cvDestroyWindow("Window");
//                    exit(0); // exit cleanly on interrupt
//                }
//            }
         }
        w.getImage();
    return a.exec();
}
