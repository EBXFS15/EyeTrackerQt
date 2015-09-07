#include "capturethread.h"

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r = -ENODEV;
        do
        {
                r = ioctl(fh, request, arg);
        }
        while (-1 == r && EINTR == errno);

        return r;
}

CaptureThread::CaptureThread()
{
    m_close = false;
    r       = -1;
    fd      = -1;
    force_format = true;
    /* Set up Image data */
    cvInitImageHeader(&frame,cvSize(640,480),IPL_DEPTH_8U, 3, IPL_ORIGIN_TL, 4 );
    /* Allocate space for RGBA data */
    frame.imageData = (char *)cvAlloc(frame.imageSize);
}


int CaptureThread::getFrameV4l2(void)
{
    for(;;)
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r)
        {
            if (EINTR == errno)
            {
                continue;
            }
            errno_exit("select");
        }

        if (0 == r)
        {
            fprintf(stderr, "select timeout\n");
            exit(EXIT_FAILURE);
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
                case EAGAIN:
                    continue;
                case EIO:
                    /* Could ignore EIO, see spec. */
                    /* fall through */
                default:
                    errno_exit("VIDIOC_DQBUF");
            }
        }


        assert(buf.index < n_buffers);

//        if(buffers[buf.index].start)
//        {
//              memcpy((char *)frame.imageData,
//              (char *)buffers[buf.index].start,
//              frame.imageSize);
//        }

        //timestamp = 1000 * buf.timestamp.tv_sec + ((double)buf.timestamp.tv_usec) / 1000;
        timestamp = buf.timestamp.tv_sec + ((double)buf.timestamp.tv_usec) / 1000000;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        {
                errno_exit("VIDIOC_QBUF");
        }

        break;
    }
    return 1;
}

void CaptureThread::stop_capturing(void)
{
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        errno_exit("VIDIOC_STREAMOFF");
    }
}

void CaptureThread::start_capturing(void)
{
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
    {
        errno_exit("VIDIOC_STREAMON");
    }
}

void CaptureThread::uninit_device(void)
{
    unsigned int i;
    for (i = 0; i < n_buffers; ++i)
    {
        if (-1 == munmap(buffers[i].start, buffers[i].length))
        {
            errno_exit("munmap");
        }
    }
    free(buffers);
}

void CaptureThread::init_device(void)
{
    unsigned int min;
    unsigned int i;

    CLEAR (cap);
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s is no V4L2 device\n",DEV_NAME);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n",DEV_NAME);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        fprintf(stderr, "%s does not support streaming i/o\n",DEV_NAME);
        exit(EXIT_FAILURE);
    }

    /* Select video input, video standard and tune here. */
    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
                case EINVAL:
                /* Cropping not supported. */
                    break;
                default:
                /* Errors ignored. */
                    break;
            }
        }
    }
    else
    {
        /* Errors ignored. */
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (force_format)
    {
        fmt.fmt.pix.width       = 640;
        fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix.field       = V4L2_FIELD_ANY;
//        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
//        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        {
            errno_exit("VIDIOC_S_FMT");
        }
        /* Note VIDIOC_S_FMT may change width and height. */
    }
    else
    {
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
        {
            errno_exit("VIDIOC_G_FMT");
        }
    }
    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
    {
        fmt.fmt.pix.bytesperline = min;
    }
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
    {
        fmt.fmt.pix.sizeimage = min;
    }

    //---------------------
    CLEAR(req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support memory mapping\n", DEV_NAME);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n",DEV_NAME);
        exit(EXIT_FAILURE);
    }

    buffers = (buffer*) calloc(req.count, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
        {
            errno_exit("VIDIOC_QUERYBUF");
        }
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
        mmap(NULL /* start anywhere */,buf.length,
             PROT_READ | PROT_WRITE /* required */,
             MAP_SHARED /* recommended */,
             fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start)
        {
            errno_exit("mmap");
        }
    }

    for (i = 0; i < n_buffers; ++i)
    {
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            {
                errno_exit("VIDIOC_QBUF");
            }
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

void CaptureThread::close_device(void)
{

    if (-1 == close(fd))
    {
        errno_exit("close");
    }
    fd = -1;

}

void CaptureThread::open_device(void)
{
    struct stat st;

    if (-1 == stat(DEV_NAME, &st))
    {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",DEV_NAME, errno, strerror(errno));
                    exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no device\n", DEV_NAME);
        exit(EXIT_FAILURE);
    }

    fd = open(DEV_NAME, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (-1 == fd)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",DEV_NAME, errno, strerror(errno));
        exit(EXIT_FAILURE);
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
