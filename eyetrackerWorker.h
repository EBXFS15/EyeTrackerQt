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

    void setProcessing(bool enable);

    public slots:
    void onImageCaptured(IplImage image);
    void abortThread();

    signals :
    void finished();
    void cleanUpDone();
    void message(QString msg);
    void eyeFound(int x, int y);

    private:
    /* Changed to atomic */
    QAtomicInt close;
    QAtomicInt processing;
    CvHaarClassifierCascade* cascade;
    IplImage grayImg;

};

#endif // EYETRACKERWORKER_H
