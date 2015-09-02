#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H


# include <QThread>
# include <QImage>
# include <opencv/cv.h>
# include <opencv/highgui.h>


class CaptureThread: public QThread
{
    Q_OBJECT

    public:
    CaptureThread();
    ~CaptureThread();
    void run();
    void stopCapturing();


    signals :
    void imageCaptured(QImage image, double timestamp);


    private:
    bool m_close;

};

#endif // CAPTURETHREAD_H
