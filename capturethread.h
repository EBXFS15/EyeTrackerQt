#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H


# include <QThread>
# include <QImage>
# include <opencv/cv.h>
# include <opencv/highgui.h>

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
#include <time.h>



struct buffer {
        void   *start;
        size_t length;
};

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define DEV_NAME "/dev/video0"

#define USE_DIRECT_V4L2

class CaptureThread: public QThread
{
    Q_OBJECT

    public:
    CaptureThread();
    ~CaptureThread();
    void run();
    void stopCapturing();
    int getFrameV4l2(void);
    void open_device(void);
    void close_device(void);
    void init_device(void);
    void uninit_device(void);
    void start_capturing(void);
    void stop_capturing(void);

    signals :
    void imageCaptured(QImage image, qint64 timestamp);
    void onGotFrame(qint64 id);


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
    int                             r, fd ;
    unsigned int                    i, n_buffers;
    struct buffer                   *buffers;

    //Opencv image
    IplImage frame;

    bool m_close;
    qint64 timestamp;
};

#endif // CAPTURETHREAD_H