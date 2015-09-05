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

//----------
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <unistd.h>
#include <sys/stat.h>

#include <linux/videodev2.h>

//#include <libv4l2.h>

struct buffer {
        void   *start;
        size_t length;
};

#define CLEAR(x) memset(&(x), 0, sizeof(x))

class CaptureThread: public QThread
{
    Q_OBJECT

    public:
    CaptureThread();
    ~CaptureThread();
    void run();
    int read_frame(void);
    void stopCapturing();
    void getFrameV4l2(void);
    void open_device(void);
    void close_device(void);
    void init_device(void);
    void init_mmap(void);
    void init_read(unsigned int buffer_size);
    void uninit_device(void);
    void start_capturing(void);
    void stop_capturing(void);

    signals :
    void imageCaptured(QImage image, double timestamp);


    private:
    bool m_close;
    //v4l2 variables and structs
    int              fd;
    struct buffer    *buffers;
    unsigned int     n_buffers;
    bool force_format;
    //Opencv image
    IplImage frame;

};

#endif // CAPTURETHREAD_H
