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

///
/// \brief CaptureWorker::CaptureWorker
///
CaptureWorker::CaptureWorker()
{
    stop = 0;
    preview=true;
    r       = -1;
    fd      = -1;
    i       =  0;
    /* Set up Image data */
    cvInitImageHeader(&frame,cvSize(DEF_IMG_WIDTH,DEF_IMG_HEIGHT),IPL_DEPTH_8U, 3, IPL_ORIGIN_TL, 4 );
    /* Allocate space for RGBA data */
    frame.imageData = (char *)cvAlloc(frame.imageSize);
    eyeCenter.x=-1;
    eyeCenter.y=-1;
    sequence = 0;
    memset (&sparams,0,sizeof(struct v4l2_streamparm));
}

///
/// \brief CaptureWorker::getFrameV4l2
/// \return 1 if successful, any other number otherwise
///
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
            QCoreApplication::processEvents();
        }
        while (((r == -1) && (errno = EINTR)) && (!stop));

        if (0 > r)
        {
            if (EINTR == errno)
            {
                return 0;
            }
            emit message(QString("select error"));
            perror("select error");
            return errno;
        }

        if (0 == r)
        {
            emit message(QString("select timeout"));
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
        if(((unsigned long)frame.width != fmt.fmt.pix.width) || ((unsigned long)frame.height != fmt.fmt.pix.height))
        {
            emit message(QString("Change width to:")+QString::number(fmt.fmt.pix.width));
            cvFree(&frame.imageData);
            cvInitImageHeader( &frame,cvSize(fmt.fmt.pix.width,fmt.fmt.pix.height ),
                                   IPL_DEPTH_8U, 3,IPL_ORIGIN_TL, 4 );
            frame.imageData = (char *)cvAlloc(frame.imageSize);
        }

        else if(buffers[buf.index].start)
        {
              memcpy((char *)frame.imageData,
              (char *)buffers[buf.index].start,
              frame.imageSize);
        }

        if (buf.sequence - sequence > 1)
        {
            //at least one frame was dropped
            emit message(QString("Lost frame(s): ")+ QString::number(buf.sequence - sequence));
        }
        sequence = buf.sequence;
        timestamp = buf.timestamp.tv_sec + ((double)buf.timestamp.tv_usec) / 1000000;
        emit gotFrame(((qint64)buf.timestamp.tv_sec) * 1000000 + ((qint64)buf.timestamp.tv_usec));

        xioctl(fd, VIDIOC_QBUF, &buf);

        return 1;
}

///
/// \brief CaptureWorker::stop_streaming
/// Stops streaming video
///
void CaptureWorker::stop_streaming(void)
{
    /* Why do we set this again? */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == v4l2_ioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        v4l2_close(fd);
        perror("cannot stop video stream");
    }
}

///
/// \brief CaptureWorker::start_streaming
/// Starts streaming video
///
void CaptureWorker::start_streaming(void)
{
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
    {
        emit message(QString("cannot start video stream:"));
    }
}

///
/// \brief CaptureWorker::uninit_device
/// Unmap all memory mapped video buffers
///
void CaptureWorker::uninit_device(void)
{
    for (i = 0; i < n_buffers; ++i)
    {
        v4l2_munmap(buffers[i].start, buffers[i].length);
    }
    /* Taken from v4l example... was missing here */
    //free(buffers);
}

///
/// \brief CaptureWorker::negociate_format
/// Negociates pixel formats and print all possible framerates for this format
///
void CaptureWorker::negociate_format(void)
{
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (fd, VIDIOC_G_FMT, &fmt)) {
        emit message(QString("Could not obtain camera format"));
        perror("Could not obtain camera format");
        exit(EXIT_FAILURE);
    }

    //list all possible fps for current format
    struct v4l2_frmivalenum frmival;
    memset(&frmival,0,sizeof(frmival));
    frmival.pixel_format = fmt.fmt.pix_mp.pixelformat;
    frmival.width = DEF_IMG_WIDTH;
    frmival.height = DEF_IMG_HEIGHT;
    emit message(QString("list of possible framerate:"));
    while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0)
    {
        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
        {
            emit message(QString("fps=")+QString::number(1.0*frmival.discrete.denominator/frmival.discrete.numerator));
        }
        else
        {
            //do nothing
        }
        frmival.index++;
    }

    emit message(QString("Current Format:\t %1; %2x%3")
                 .arg(fcc2s(fmt.fmt.pix.pixelformat).c_str())
                 .arg(fmt.fmt.pix_mp.width)
                 .arg(fmt.fmt.pix_mp.height));

    fmt.fmt.pix.width       = DEF_IMG_WIDTH;
    fmt.fmt.pix.height      = DEF_IMG_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    // if not supported by the device, libv4l2 will convert the fame data in RGB colorspace
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;
    if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        emit message(QString("Error setting pixel format"));
        perror("Error setting pixel format");
        exit(EXIT_FAILURE);
    }

    if (-1 == xioctl (fd, VIDIOC_G_FMT, &fmt)) {
            emit message(QString("Could not obtain camera format"));
            perror("Could not obtain camera format");
            exit(EXIT_FAILURE);
        }

   emit message(QString("New Format:\t %1; %2x%3")
                     .arg(fcc2s(fmt.fmt.pix.pixelformat).c_str())
                     .arg(fmt.fmt.pix_mp.width)
                     .arg(fmt.fmt.pix_mp.height));
}

///
/// \brief CaptureWorker::request_buffers
/// Requests memory mapped buffers
///
void CaptureWorker::request_buffers(void)
{
    // request 4 memory mapped buffers for video capture
    CLEAR(req);
    req.count = 4;
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
                    perror("mmap error");
                    exit(EXIT_FAILURE);
            }
            if(stop)
            {
                break;
            }
    }

    for (i = 0; i < n_buffers; ++i)
    {
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            xioctl(fd, VIDIOC_QBUF, &buf);
            if(stop)
            {
                break;
            }            
            QCoreApplication::processEvents();
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

///
/// \brief CaptureWorker::init_device
/// Initialize video capture device
///
void CaptureWorker::init_device(void)
{
    negociate_format();
    request_buffers();
}

///
/// \brief CaptureWorker::close_device
/// Close video capture device
///
void CaptureWorker::close_device(void)
{
    if(v4l2_close(fd))
    {
        perror(strerror(errno));
    }
}

///
/// \brief CaptureWorker::open_device
/// Open video capture device
///
void CaptureWorker::open_device(void)
{
    fd = v4l2_open(DEV_NAME, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0)
    {
        perror("Cannot open video capture device");
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

///
/// \brief CaptureWorker::set_fix_framerate
/// \param framerate
/// Set framerate to the given value, send message to gui if not possible
///
void CaptureWorker::set_fix_framerate(uint framerate)
{

    // try to set framerate to given framerate
    sparams.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl (fd, VIDIOC_G_PARM, &sparams))
    {
        perror("Cannot get parameter from device.");
        exit(EXIT_FAILURE);
    }
    if (!sparams.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
    {
       perror("Device cannot change time per frame (FPS)");
    }
    else
    {
        sparams.parm.capture.timeperframe.numerator = 1;
        sparams.parm.capture.timeperframe.denominator = framerate;
        if (-1 == xioctl (fd, VIDIOC_S_PARM, &sparams))
        {
            perror("Cannot set fps");
            exit(EXIT_FAILURE);
        }
        CLEAR (sparams);
        sparams.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl (fd, VIDIOC_G_PARM, &sparams))
        {
            perror("Cannot get parameter from device.");
            exit(EXIT_FAILURE);
        }
        if (sparams.parm.capture.timeperframe.denominator != framerate)
        {
            emit message(QString("Cannot set fps to ")
                         + QString::number(framerate)
                         + QString(", current fps is ")
                         + QString::number(sparams.parm.capture.timeperframe.denominator
                                          /sparams.parm.capture.timeperframe.numerator));
        }
    }
}

///
/// \brief CaptureWorker::print_video_formats
/// Send each available video formats
///
void CaptureWorker::print_video_formats()
{
    struct v4l2_fmtdesc fmtdesc;

    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = type;
    QString msg;
    msg.append("Pixel formats supported: ");
    while ((v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0) && (!stop))
    {
        msg.append(QString("%1; ").arg((fcc2s(fmtdesc.pixelformat).c_str())));
        fmtdesc.index++;
    }
    emit message(msg);
}

///
/// \brief CaptureWorker::disable_camera_autoexposure
/// Disable auto exposure, send message if not successful
///
void CaptureWorker::disable_camera_autoexposure()
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

///
/// \brief CaptureWorker::~CaptureWorker
///
CaptureWorker::~CaptureWorker()
{

    // Nothing to clean up here, the required step are done before stopping thread
}

///
/// \brief CaptureWorker::process
/// Running loop to get frames and send them to connected slots
///
void CaptureWorker::process()
{
    open_device();
    init_device();
    set_fix_framerate(15);
    start_streaming();
    print_video_formats();
    disable_camera_autoexposure();

    while(!stop)
    {
        getFrameV4l2();
        emit imageCaptured(frame);
        if(preview==true)
        {
            cvDrawCircle(&frame,eyeCenter,20,CV_RGB(0,0,255 ),2);
            captFrame = QImage((const uchar*)frame.imageData, frame.width, frame.height, QImage::Format_RGB888);
            emit qimageCaptured(captFrame);
        }
        QCoreApplication::processEvents();//to catch all incoming signals
    }

    stop_streaming();
    uninit_device();
    close_device();
    this->thread()->quit();
    emit finished();
}

///
/// \brief CaptureWorker::stopCapturing
/// Tell thread to stop
///
void CaptureWorker::stop_capturing()
{
    stop = 1;
}

///
/// \brief CaptureWorker::setCenter
/// \param x
/// \param y
/// Change eye center
///
void CaptureWorker::set_center(int x, int y)
{
    eyeCenter.x=x;
    eyeCenter.y=y;
    emit message (QString("Eye found at x=%1 y=%2").arg(x).arg(y));
}


///
/// \brief CaptureWorker::setPreview
/// Set preview, that is to say sending or not copy of frames to connected slots
///
void CaptureWorker::set_preview(bool enable)

{
    if (enable)
    {
        preview = 1;
    }
    else
    {
        preview = 0;
    }
}
