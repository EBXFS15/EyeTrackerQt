#ifndef CAPTUREWORKER_H
#define CAPTUREWORKER_H


#include <QThread>
#include <QImage>
#include <QtCore>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l1.h>
#include <libv4l2.h>

struct buffer {
        void   *start;
        size_t length;
};

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define DEV_NAME "/dev/video0"

#define DEF_IMG_WIDTH  320
#define DEF_IMG_HEIGHT 240

#define USE_DIRECT_V4L2

class CaptureWorker: public QObject
{
    Q_OBJECT

    public:
    CaptureWorker();
    ~CaptureWorker();
    void run();

    int getFrameV4l2(void);
    void open_device(void);
    void close_device(void);
    void init_device(void);
    void uninit_device(void);
    void start_capturing(void);
    void stop_capturing(void);
    void print_video_formats(void);
    void disable_camera_optimisation(void);
    void setCenter(int x, int y);
    void togglePreview();

    public slots:
    void process();
    void stopCapturing();

    signals :
    void finished();
    void imageCaptured(IplImage image);
    void qimageCaptured(QImage image, double timestamp);
    void message(QString msg);
    void gotFrame(qint64 id);


    private:
    struct video_capability         capability;
    //v4l2 variables and structs
    struct v4l2_capability          cap;
    struct v4l2_format              fmt;
    struct v4l2_buffer              buf;
    struct v4l2_requestbuffers      req;
    enum v4l2_buf_type              type;
    fd_set                          fds;
    struct timeval                  tv;
    int                             r, fd;
    unsigned int                    i, n_buffers;
    struct buffer                   *buffers;

    //Opencv image
    IplImage frame;
    QImage captFrame;
    bool close;
    bool preview;
    double timestamp;
    CvPoint eyeCenter;

    qint64 timestamp2;

};

#endif // CAPTUREWORKER_H