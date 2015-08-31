#include "v4l2camera.h"

v4l2Camera::v4l2Camera()
{

}

v4l2Camera::~v4l2Camera()
{

}

//static CvCaptureCAM_V4L * v4l2Camera::captureFromCAM_V4L2 (int index)
//{
//   static int autoindex;
//   autoindex = 0;

//   char deviceName[MAX_DEVICE_DRIVER_NAME];

//   if (!numCameras)
//      icvInitCapture_V4L(); /* Havent called icvInitCapture yet - do it now! */
//   if (!numCameras)
//     return NULL; /* Are there any /dev/video input sources? */

//   //search index in indexList
//   if ( (index>-1) && ! ((1 << index) & indexList) )
//   {
//     fprintf( stderr, "VIDEOIO ERROR: V4L: index %d is not correct!\n",index);
//     return NULL; /* Did someone ask for not correct video source number? */
//   }
//   /* Allocate memory for this humongus CvCaptureCAM_V4L structure that contains ALL
//      the handles for V4L processing */
//   CvCaptureCAM_V4L * capture = (CvCaptureCAM_V4L*)cvAlloc(sizeof(CvCaptureCAM_V4L));
//   if (!capture) {
//      fprintf( stderr, "VIDEOIO ERROR: V4L: Could not allocate memory for capture process.\n");
//      return NULL;
//   }
//   /* Select camera, or rather, V4L video source */
//   if (index<0) { // Asking for the first device available
//     for (; autoindex<MAX_CAMERAS;autoindex++)
//    if (indexList & (1<<autoindex))
//        break;
//     if (autoindex==MAX_CAMERAS)
//    return NULL;
//     index=autoindex;
//     autoindex++;// i can recall icvOpenCAM_V4l with index=-1 for next camera
//   }
//   /* Print the CameraNumber at the end of the string with a width of one character */
//   sprintf(deviceName, "/dev/video%1d", index);

//   /* w/o memset some parts  arent initialized - AKA: Fill it with zeros so it is clean */
//   memset(capture,0,sizeof(CvCaptureCAM_V4L));
//   /* Present the routines needed for V4L funtionality.  They are inserted as part of
//      the standard set of cv calls promoting transparency.  "Vector Table" insertion. */
//   capture->FirstCapture = 1;

//   if (_capture_V4L2 (capture, deviceName) == -1)
//   {
//       icvCloseCAM_V4L(capture);
//       V4L2_SUPPORT = 0;
//   }
//   else
//   {
//       V4L2_SUPPORT = 1;
//   }

//   return capture;
//}

//static IplImage* v4l2Camera::retrieveFrameCAM_V4L2( CvCaptureCAM_V4L* capture, int)
//{
//    /* Now get what has already been captured as a IplImage return */
//    /* First, reallocate imageData if the frame size changed */
//    if (V4L2_SUPPORT == 1)
//    {

//      if(((unsigned long)capture->frame.width != capture->form.fmt.pix.width)
//         || ((unsigned long)capture->frame.height != capture->form.fmt.pix.height)) {
//          cvFree(&capture->frame.imageData);
//          cvInitImageHeader( &capture->frame,
//                cvSize( capture->form.fmt.pix.width,
//                    capture->form.fmt.pix.height ),
//                IPL_DEPTH_8U, 3, IPL_ORIGIN_TL, 4 );
//         capture->frame.imageData = (char *)cvAlloc(capture->frame.imageSize);
//      }
//    }
//    if (V4L2_SUPPORT == 1)
//    {
//      switch (capture->palette)
//      {
//      case PALETTE_BGR24:
//          memcpy((char *)capture->frame.imageData,
//                 (char *)capture->buffers[capture->bufferIndex].start,
//                 capture->frame.imageSize);
//          break;

//      case PALETTE_YVU420:
//          yuv420p_to_rgb24(capture->form.fmt.pix.width,
//                   capture->form.fmt.pix.height,
//                   (unsigned char*)(capture->buffers[capture->bufferIndex].start),
//                   (unsigned char*)capture->frame.imageData);
//          break;

//      case PALETTE_YUV411P:
//          yuv411p_to_rgb24(capture->form.fmt.pix.width,
//                   capture->form.fmt.pix.height,
//                   (unsigned char*)(capture->buffers[capture->bufferIndex].start),
//                   (unsigned char*)capture->frame.imageData);
//          break;
//  #ifdef HAVE_JPEG
//      case PALETTE_MJPEG:
//          if (!mjpeg_to_rgb24(capture->form.fmt.pix.width,
//                      capture->form.fmt.pix.height,
//                      (unsigned char*)(capture->buffers[capture->bufferIndex]
//                               .start),
//                      capture->buffers[capture->bufferIndex].length,
//                      (unsigned char*)capture->frame.imageData))
//            return 0;
//          break;
//  #endif

//      case PALETTE_YUYV:
//          yuyv_to_rgb24(capture->form.fmt.pix.width,
//                    capture->form.fmt.pix.height,
//                    (unsigned char*)(capture->buffers[capture->bufferIndex].start),
//                    (unsigned char*)capture->frame.imageData);
//          break;
//      case PALETTE_UYVY:
//          uyvy_to_rgb24(capture->form.fmt.pix.width,
//                    capture->form.fmt.pix.height,
//                    (unsigned char*)(capture->buffers[capture->bufferIndex].start),
//                    (unsigned char*)capture->frame.imageData);
//          break;
//      case PALETTE_SBGGR8:
//          bayer2rgb24(capture->form.fmt.pix.width,
//                  capture->form.fmt.pix.height,
//                  (unsigned char*)capture->buffers[capture->bufferIndex].start,
//                  (unsigned char*)capture->frame.imageData);
//          break;

//      case PALETTE_SN9C10X:
//          sonix_decompress_init();
//          sonix_decompress(capture->form.fmt.pix.width,
//                   capture->form.fmt.pix.height,
//                   (unsigned char*)capture->buffers[capture->bufferIndex].start,
//                   (unsigned char*)capture->buffers[(capture->bufferIndex+1) % capture->req.count].start);

//          bayer2rgb24(capture->form.fmt.pix.width,
//                  capture->form.fmt.pix.height,
//                  (unsigned char*)capture->buffers[(capture->bufferIndex+1) % capture->req.count].start,
//                  (unsigned char*)capture->frame.imageData);
//          break;

//      case PALETTE_SGBRG:
//          sgbrg2rgb24(capture->form.fmt.pix.width,
//                  capture->form.fmt.pix.height,
//                  (unsigned char*)capture->buffers[(capture->bufferIndex+1) % capture->req.count].start,
//                  (unsigned char*)capture->frame.imageData);
//          break;
//      case PALETTE_RGB24:
//          rgb24_to_rgb24(capture->form.fmt.pix.width,
//                  capture->form.fmt.pix.height,
//                  (unsigned char*)capture->buffers[(capture->bufferIndex+1) % capture->req.count].start,
//                  (unsigned char*)capture->frame.imageData);
//          break;
//      }
//    }

//}

//static int v4l2Camera::grabFrameCAM_V4L2(CvCaptureCAM_V4L* capture) {

//   if (capture->FirstCapture)
//   {
//      /* Some general initialization must take place the first time through */

//      /* This is just a technicality, but all buffers must be filled up before any
//         staggered SYNC is applied.  SO, filler up. (see V4L HowTo) */

//    if (V4L2_SUPPORT == 1)
//      {

//        for (capture->bufferIndex = 0;
//             capture->bufferIndex < ((int)capture->req.count);
//             ++capture->bufferIndex)
//        {

//          struct v4l2_buffer buf;

//          CLEAR (buf);

//          buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//          buf.memory      = V4L2_MEMORY_MMAP;
//          buf.index       = (unsigned long)capture->bufferIndex;

//          if (-1 == ioctl (capture->deviceHandle, VIDIOC_QBUF, &buf)) {
//              perror ("VIDIOC_QBUF");
//              return 0;
//          }
//        }

//        /* enable the streaming */
//        capture->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//        if (-1 == ioctl (capture->deviceHandle, VIDIOC_STREAMON,
//                          &capture->type)) {
//            /* error enabling the stream */
//            perror ("VIDIOC_STREAMON");
//            return 0;
//        }
//      }


//#if defined(V4L_ABORT_BADJPEG) && defined(HAVE_CAMV4L2)
//     if (V4L2_SUPPORT == 1)
//     {
//        // skip first frame. it is often bad -- this is unnotied in traditional apps,
//        //  but could be fatal if bad jpeg is enabled
//        mainloop_v4l2(capture);
//     }
//#endif

//      /* preparation is ok */
//      capture->FirstCapture = 0;
//   }

//if (V4L2_SUPPORT == 1)
//   {

//     mainloop_v4l2(capture);

//   }
//   return(1);
//}

//static void v4l2Camera::mainloop_v4l2(CvCaptureCAM_V4L* capture) {
//    unsigned int count;

//    count = 1;

//    while (count-- > 0) {
//        for (;;) {
//            fd_set fds;
//            struct timeval tv;
//            int r;

//            FD_ZERO (&fds);
//            FD_SET (capture->deviceHandle, &fds);

//            /* Timeout. */
//            tv.tv_sec = 10;
//            tv.tv_usec = 0;

//            r = select (capture->deviceHandle+1, &fds, NULL, NULL, &tv);

//            if (-1 == r) {
//                if (EINTR == errno)
//                    continue;

//                perror ("select");
//            }

//            if (0 == r) {
//                fprintf (stderr, "select timeout\n");

//                /* end the infinite loop */
//                break;
//            }

//            if (read_frame_v4l2 (capture))
//                break;
//        }
//    }
//}

//static int v4l2Camera::read_frame_v4l2(CvCaptureCAM_V4L* capture) {
//    struct v4l2_buffer buf;

//    CLEAR (buf);

//    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    buf.memory = V4L2_MEMORY_MMAP;

//    if (-1 == ioctl (capture->deviceHandle, VIDIOC_DQBUF, &buf)) {
//        switch (errno) {
//        case EAGAIN:
//            return 0;

//        case EIO:
//        if (!(buf.flags & (V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_DONE)))
//        {
//          if (ioctl(capture->deviceHandle, VIDIOC_QBUF, &buf) == -1)
//          {
//            return 0;
//          }
//        }
//        return 0;

//        default:
//            /* display the error and stop processing */
//            perror ("VIDIOC_DQBUF");
//            return 1;
//        }
//   }

//   assert(buf.index < capture->req.count);

//   memcpy(capture->buffers[MAX_V4L_BUFFERS].start,
//      capture->buffers[buf.index].start,
//      capture->buffers[MAX_V4L_BUFFERS].length );
//   capture->bufferIndex = MAX_V4L_BUFFERS;
//   //printf("got data in buff %d, len=%d, flags=0x%X, seq=%d, used=%d)\n",
//   //	  buf.index, buf.length, buf.flags, buf.sequence, buf.bytesused);

//   if (-1 == ioctl (capture->deviceHandle, VIDIOC_QBUF, &buf))
//       perror ("VIDIOC_QBUF");

//   //set timestamp in capture struct to be timestamp of most recent frame
//   capture->timestamp = buf.timestamp;

//   return 1;
//}
