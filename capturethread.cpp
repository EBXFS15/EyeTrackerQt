#include "capturethread.h"


static int xioctl(int fh, int request, void *arg)
{
        int r = -ENODEV;;

        do
        {
                r = v4l2_ioctl(fh, request, arg);
        }
        while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        return r;
}

CaptureThread::CaptureThread()
{
    m_close = false;
    r       = -1;
    fd      = -1;
    i       =  0;

    /* Set up Image data */
    cvInitImageHeader(&frame,cvSize(640,480),IPL_DEPTH_8U, 3, IPL_ORIGIN_TL, 4 );
    /* Allocate space for RGBA data */
    frame.imageData = (char *)cvAlloc(frame.imageSize);
}


int CaptureThread::getFrameV4l2(void)
{
    do
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        r = select(fd + 1, &fds, NULL, NULL, &tv);
        }
        while ((r == -1 && (errno = EINTR)));

        if (r == -1)
        {
            perror("select");
            return errno;
        }
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
        {
            printf("Warning: cannot dequeue video buffer");
        }

        //reallocate image data if size changed
        if(((unsigned long)frame.width != fmt.fmt.pix.width) || ((unsigned long)frame.height != fmt.fmt.pix.height))
        {
            cvFree(&frame.imageData);
            cvInitImageHeader( &frame,cvSize(fmt.fmt.pix.width,fmt.fmt.pix.height ),
                                   IPL_DEPTH_8U, 3,IPL_ORIGIN_TL, 4 );
            frame.imageData = (char *)cvAlloc(frame.imageSize);
        }
//        if(buffers[buf.index].start)
//        {
//              memcpy((char *)frame.imageData,
//              (char *)buffers[buf.index].start,
//              frame.imageSize);
//        }

        //timestamp = 1000 * buf.timestamp.tv_sec + ((double)buf.timestamp.tv_usec) / 1000;

        timestamp =  ((buf.timestamp.tv_sec) * 1000000 + buf.timestamp.tv_usec);
        //qint64 rxTimeStamp = ((gotTime.tv_sec) * 1000000 + gotTime.tv_usec);
        emit onGotFrame(timestamp);
        xioctl(fd, VIDIOC_QBUF, &buf);

        return 1;
}

void CaptureThread::stop_capturing(void)
{
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);
}

void CaptureThread::start_capturing(void)
{
    xioctl(fd, VIDIOC_STREAMON, &type);
}

void CaptureThread::uninit_device(void)
{
    for (i = 0; i < n_buffers; ++i)
    {
        v4l2_munmap(buffers[i].start, buffers[i].length);
    }
}

void CaptureThread::init_device(void)
{
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 640;
    fmt.fmt.pix.height      = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//V4L2_PIX_FMT_BGR24;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;
    xioctl(fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24) {
            printf("Libv4l didn't accept RGB24 format. Can't proceed.\n");
            exit(EXIT_FAILURE);
    }
    if ((fmt.fmt.pix.width != 640) || (fmt.fmt.pix.height != 480))
            printf("Warning: driver is sending image at %dx%d\n",
                    fmt.fmt.pix.width, fmt.fmt.pix.height);

    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    buffers = (buffer*) calloc(req.count, sizeof(*buffers));
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
            CLEAR(buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = n_buffers;

            xioctl(fd, VIDIOC_QUERYBUF, &buf);

            buffers[n_buffers].length = buf.length;
            buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
                          PROT_READ | PROT_WRITE, MAP_SHARED,
                          fd, buf.m.offset);

            if (MAP_FAILED == buffers[n_buffers].start) {
                    perror("mmap");
                    exit(EXIT_FAILURE);
            }
    }

    for (i = 0; i < n_buffers; ++i)
    {
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            xioctl(fd, VIDIOC_QBUF, &buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

void CaptureThread::close_device(void)
{
    v4l2_close(fd);
}

void CaptureThread::open_device(void)
{
    fd = v4l2_open(DEV_NAME, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0)
    {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }

    CLEAR (cap);
    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap))
    {
        perror("Cannot query capabilities of device");
        exit(EXIT_FAILURE);
    }
    else
    {
        CLEAR (capability);
        capability.type = cap.capabilities;
        /* Query channels number */
        if (-1 == xioctl (fd, VIDIOC_G_INPUT, &capability.channels))
        {
            perror("Not a v4l2 driver for this device");
            exit(EXIT_FAILURE);
        }
    }
}

CaptureThread::~CaptureThread()
{
}

void CaptureThread::run()
{
    #ifdef USE_DIRECT_V4L2
    //CvCapture* capture = cvCreateCameraCapture( -1 );
    open_device();
    init_device();
    start_capturing();
    QImage captFrame;
    while( m_close==false)
    {
        getFrameV4l2();
        cvCvtColor(&frame, &frame, CV_BGR2RGB);
        //double timestamp = cvGetCaptureProperty(capture,CV_CAP_PROP_POS_MSEC);
        captFrame = QImage((const uchar*)frame.imageData, frame.width, frame.height, QImage::Format_RGB888).copy();        
        emit imageCaptured(captFrame,timestamp);// timestamp);
    }
    stop_capturing();
    uninit_device();
    close_device();
    #else
    CvCapture* capture = cvCreateCameraCapture( -1 );
    IplImage *frame=NULL;
    QImage captFrame;
    while( m_close==false)
    {
        frame = cvQueryFrame(capture);
        if(frame)
        {
            cvCvtColor(frame, frame, CV_BGR2RGB);
            double timestamp = cvGetCaptureProperty(capture,CV_CAP_PROP_POS_MSEC);
            captFrame = QImage((const uchar*)frame->imageData, 640, 480, QImage::Format_RGB888).copy();
            emit imageCaptured(captFrame, timestamp);
        }
    }

    cvReleaseCapture( &capture );
    #endif
}

void CaptureThread::stopCapturing()
{
    m_close = true;
}
