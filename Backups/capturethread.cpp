#include "capturethread.h"

CaptureThread::CaptureThread()
{
    m_close = false;
    fd      = -1;
    force_format=true;
    /* Set up Image data */
    cvInitImageHeader(&frame,cvSize(640,480),IPL_DEPTH_8U, 2, IPL_ORIGIN_TL, 4 );
    /* Allocate space for RGBA data */
    frame.imageData = (char *)cvAlloc(frame.imageSize);
}

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

int CaptureThread::read_frame(void)
{
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
                case EAGAIN:
                return 0;

                case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
                default:
                errno_exit("VIDIOC_DQBUF");
            }
        }
        assert(buf.index < n_buffers);
        int s = frame.imageSize;
        if(buffers[buf.index].start)
        {
              memcpy((char *)frame.imageData,
                     (char *)buffers[buf.index].start,
                    frame.imageSize);
        }


        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        {
            errno_exit("VIDIOC_QBUF");
        }
        return 1;
}

void CaptureThread::getFrameV4l2(void)
{

    for (;;)
    {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r)
        {
            if (EINTR == errno)
                continue;
        errno_exit("select");
        }

        if (0 == r) {
            fprintf(stderr, "select timeout\n");
            exit(EXIT_FAILURE);
        }

        if (read_frame())
        break;
        /* EAGAIN - continue select loop. */
    }
}

void CaptureThread::stop_capturing(void)
{
        enum v4l2_buf_type type;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");
}

void CaptureThread::start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        for (i = 0; i < n_buffers; ++i)
        {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
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

void CaptureThread::init_read(unsigned int buffer_size)
{
        buffers = (buffer*) calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}

void CaptureThread::init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "/dev/video0 does not support memory mapping\n");
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on /dev/video0\n");
                exit(EXIT_FAILURE);
        }

        buffers = (buffer*) calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

void CaptureThread::init_device(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "/dev/video0 is no V4L2 device\n");
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "/dev/video0 is no video capture device\n");
                exit(EXIT_FAILURE);
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                        fprintf(stderr, "/dev/video0 does not support streaming i/o\n");
                        exit(EXIT_FAILURE);
        }



        /* Select video input, video standard and tune here. */


        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }


        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (force_format) {
                fmt.fmt.pix.width       = 640;
                fmt.fmt.pix.height      = 480;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_YUYV;
                fmt.fmt.pix.field       = V4L2_FIELD_ANY;//V4L2_FIELD_INTERLACED;

                if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                /* Note VIDIOC_S_FMT may change width and height. */
        } else {
                /* Preserve original settings as set by v4l2-ctl for example */
                if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                        errno_exit("VIDIOC_G_FMT");
        }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        init_mmap();
}

void CaptureThread::close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
}

void CaptureThread::open_device(void)
{
        struct stat st;

        if (-1 == stat("/dev/video0", &st)) {
                fprintf(stderr, "Cannot identify '/dev/video0': %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "/dev/video0 is no device\n");
                exit(EXIT_FAILURE);
        }

        fd = open("/dev/video0", O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '/dev/video0': %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

CaptureThread::~CaptureThread()
{
}

void CaptureThread::run()
{
    //CvCapture* capture = cvCreateCameraCapture( -1 );
    open_device();
    init_device();
    start_capturing();
    QImage captFrame;
    while( m_close==false)
    {
        getFrameV4l2();
        //cvCvtColor(&frame, &frame, CV_BGR2RGB);
        //double timestamp = cvGetCaptureProperty(capture,CV_CAP_PROP_POS_MSEC);
        captFrame = QImage((const uchar*)frame.imageData, 640, 480, QImage::Format_RGB888).copy();
        emit imageCaptured(captFrame,0);// timestamp);
    }
    stop_capturing();
    uninit_device();
    close_device();
}

void CaptureThread::stopCapturing()
{
    m_close = true;
}
