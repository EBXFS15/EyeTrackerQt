#include "captureWorker.h"

using namespace cv;

static int xioctl(int fh, int request, void *arg)
{
        int r = -ENODEV;;

        do
        {
                r = v4l2_ioctl(fh, request, arg);
        }
        while (r == -1 && errno == EINTR);

        return r;
}

static std::string fcc2s(unsigned int val)
{
    std::string s;

    s += val & 0xff;
    s += (val >> 8) & 0xff;
    s += (val >> 16) & 0xff;
    s += (val >> 24) & 0xff;
    return s;
}

CaptureWorker::CaptureWorker()
{
    close = false;
    r       = -1;
    fd      = -1;
    i       =  0;
    /* Set up Image data */
    cvInitImageHeader(&frame,cvSize(DEF_IMG_WIDTH,DEF_IMG_HEIGHT),IPL_DEPTH_8U, 3, IPL_ORIGIN_TL, 4 );
    /* Allocate space for RGBA data */
    frame.imageData = (char *)cvAlloc(frame.imageSize);
    convertData=NULL;
}


int CaptureWorker::getFrameV4l2(void)
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

        if (-1 == r)
        {
            if (EINTR == errno)
            {
                return 0;
            }
            emit message(QString("select error"));
            //perror("select");
            return errno;
        }

        if (0 == r)
        {
            emit message(QString("select timeout"));
            //fprintf(stderr, "select timeout\n");
            //exit(EXIT_FAILURE);
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
                case EAGAIN:
                    emit message(QString("Dequeue buffer: EAGAIN"));
                    return 0;
                case EIO:
                    /* Could ignore EIO, see spec. */
                    /* fall through */
                default:
                    emit message(QString("Error dequeuing buffer"));
                    //perror("VIDIOC_DQBUF");
                    return errno;
            }
        }

        //reallocate image data if size changed
        if(((unsigned long)frame.width != destfmt.fmt.pix.width) || ((unsigned long)frame.height != destfmt.fmt.pix.height))
        {
            emit message(QString("Change width to:")+QString::number(destfmt.fmt.pix.width));
            cvFree(&frame.imageData);
            cvInitImageHeader( &frame,cvSize(destfmt.fmt.pix.width,destfmt.fmt.pix.height ),
                                   IPL_DEPTH_8U, 3,IPL_ORIGIN_TL, 4 );
            frame.imageData = (char *)cvAlloc(frame.imageSize);
        }

        if(destfmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24)
        {
            destfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
            v4lconvert_convert(convertData, &fmt, &destfmt,
                                    (unsigned char *)buffers[buf.index].start, buf.bytesused,
                                    (unsigned char *)frame.imageData, fmt.fmt.pix.sizeimage);
        }
        else if(buffers[buf.index].start)
        {
              memcpy((char *)frame.imageData,
              (char *)buffers[buf.index].start,
              frame.imageSize);
        }

        //timestamp = 1000 * buf.timestamp.tv_sec + ((double)buf.timestamp.tv_usec) / 1000;
        timestamp = buf.timestamp.tv_sec + ((double)buf.timestamp.tv_usec) / 1000000;

        xioctl(fd, VIDIOC_QBUF, &buf);

        return 1;
}

void CaptureWorker::stop_capturing(void)
{
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        emit message(QString("cannot stop video stream"));
    }
}

void CaptureWorker::start_capturing(void)
{
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
    {
        emit message(QString("cannot start video stream:"));
    }
}

void CaptureWorker::uninit_device(void)
{
    for (i = 0; i < n_buffers; ++i)
    {
        v4l2_munmap(buffers[i].start, buffers[i].length);
    }
}

void CaptureWorker::init_device(void)
{

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (fd, VIDIOC_G_FMT, &fmt)) {
        emit message(QString("Could not obtain camera format"));
        exit(EXIT_FAILURE);
    }
    emit message("Current Format: ");
    emit message(QString(fcc2s(fmt.fmt.pix.pixelformat).c_str()));
    emit message(QString::number((fmt.fmt.pix_mp.width))+"x"+QString::number((fmt.fmt.pix_mp.height)));
    destfmt = fmt;
    destfmt.fmt.pix.width       = DEF_IMG_WIDTH;
    destfmt.fmt.pix.height      = DEF_IMG_HEIGHT;
    destfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;//
    //destfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//
    destfmt.fmt.pix.field       = V4L2_FIELD_ANY;
    if(-1 == xioctl(fd, VIDIOC_S_FMT, &destfmt))
    {
        emit message(QString("Error setting pixel format"));
        exit(EXIT_FAILURE);
    }

    if (-1 == xioctl (fd, VIDIOC_G_FMT, &destfmt)) {
            emit message(QString("Could not obtain camera format"));
            exit(EXIT_FAILURE);
        }
        emit message("New Format: ");
        emit message(QString(fcc2s(destfmt.fmt.pix.pixelformat).c_str()));
        emit message(QString::number((fmt.fmt.pix_mp.width))+"x"+QString::number((fmt.fmt.pix_mp.height)));

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
                    emit message(QString("Memory mapping error"));
                    //perror("mmap");
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

void CaptureWorker::close_device(void)
{
    if(convertData)
    {
        v4lconvert_destroy(convertData);
    }
    v4l2_close(fd);
}

void CaptureWorker::open_device(void)
{
    fd = v4l2_open(DEV_NAME, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0)
    {
        emit message(QString("Cannot open video capture device"));
        //perror("Cannot open video capture device");
        exit(EXIT_FAILURE);
    }

    convertData = v4lconvert_create(fd);

    CLEAR (cap);
    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap))
    {
        emit message(QString("Cannot query capabilities of device"));
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

void CaptureWorker::print_video_formats()
{
    struct v4l2_fmtdesc fmtdesc;

    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = type;
    emit message("Pixel formats supported:");
    while (v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0)
    {
        emit message(QString(fcc2s(fmtdesc.pixelformat).c_str()));
        fmtdesc.index++;
    }
}

void CaptureWorker::disable_camera_optimisation()
{
    struct v4l2_control ctl;
    ctl.id = V4L2_CID_EXPOSURE_AUTO;
    ctl.value = V4L2_EXPOSURE_MANUAL;
    if(v4l2_ioctl(fd,VIDIOC_S_CTRL,&ctl))
    {
        emit message(QString("Auto Exposure disable."));
    }
    else
    {
        emit message(QString("Cannot disable Auto Exposure:"));
        emit message(QString(errno));
    }
}

CaptureWorker::~CaptureWorker()
{
}

void CaptureWorker::process()
{
    #ifdef USE_DIRECT_V4L2
    open_device();
    init_device();
    start_capturing();
    print_video_formats();
    disable_camera_optimisation();

    while( close==false)
    {
        getFrameV4l2();
        cvCvtColor(&frame, &frame, CV_BGR2RGB);
        emit imageCaptured(frame);
        captFrame = QImage((const uchar*)frame.imageData, frame.width, frame.height, QImage::Format_RGB888).scaled(
                        QSize(640,480));
        emit qimageCaptured(captFrame,timestamp);
    }
    stop_capturing();
    uninit_device();
    close_device();
    emit finished();
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
    emit finished();
    #endif
}

void CaptureWorker::stopCapturing()
{
    close = true;
}
