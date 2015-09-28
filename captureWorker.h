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
    void negociate_format(void);
    void request_buffers(void);
    void init_device(void);
    void uninit_device(void);
    void start_streaming(void);
    void stop_streaming(void);
    void print_video_formats(void);
    void disable_camera_autoexposure(void);
    void set_fix_framerate(uint framerate);

    void toggle_preview();
    void stop_capturing();

    public slots:
    void process();
    void set_center(int x, int y);


    signals :
    void finished();
    void imageCaptured(IplImage image);
    void qimageCaptured(QImage image);
    void message(QString msg);
    void gotFrame(qint64 id);


    private:
    struct video_capability         capability;
    //v4l2 variables and structs
    struct v4l2_capability          cap;
    struct v4l2_format              fmt;
    struct v4l2_buffer              buf;
    struct v4l2_requestbuffers      req;
    enum   v4l2_buf_type            type;
    struct v4l2_streamparm          sparams;
    fd_set                          fds;
    struct timeval                  tv;
    int                             r, fd;
    unsigned int                    i, n_buffers;
    struct buffer                   *buffers;

    //Opencv image
    IplImage frame;
    QImage captFrame;
    QAtomicInt stop;
    bool preview;
    double timestamp;
    CvPoint eyeCenter;

    qint64 timestamp2;

};

#endif // CAPTUREWORKER_H
