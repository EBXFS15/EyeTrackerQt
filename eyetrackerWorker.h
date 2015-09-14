#ifndef EYETRACKERWORKER_H
#define EYETRACKERWORKER_H

#include <QThread>
#include <QImage>
#include <QtCore>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


class EyeTrackerWorker: public QObject
{
    Q_OBJECT

    public:
    EyeTrackerWorker();
    ~EyeTrackerWorker();
    void abortThread();
    void toggleProcessing();

    public slots:
    void onImageCaptured(IplImage image);

    signals :
    void finished();
    void message(QString msg);
    void eyeFound(int x, int y);


    private:
    bool close;
    bool processing;
    CvHaarClassifierCascade* cascade;
    IplImage grayImg;

};

#endif // EYETRACKERWORKER_H
