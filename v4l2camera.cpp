///* This is the contributed code:

//File:             v4l2camera.cpp

//Original Version: 2003-03-12  Magnus Lundin lundin@mlu.mine.nu
//Original Comments:

//*/

////#include "precomp.hpp"
//#include <opencv/cv.h>
//#include <opencv/highgui.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>

//#define CLEAR(x) memset (&(x), 0, sizeof (x))

//#include <stdio.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <errno.h>
//#include <sys/types.h>
//#include <sys/mman.h>
//#include <string.h>
//#include <stdlib.h>
//#include <asm/types.h>          /* for videodev2.h */
//#include <assert.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>

//#include <linux/videodev2.h>

//#include <libv4l1.h>
//#include <libv4l2.h>

///* Defaults - If your board can do better, set it here.  Set for the most common type inputs. */
//#define DEFAULT_V4L_WIDTH  640
//#define DEFAULT_V4L_HEIGHT 480

//#define CHANNEL_NUMBER 1
//#define MAX_CAMERAS 8


//// default and maximum number of V4L buffers, not including last, 'special' buffer
//#define MAX_V4L_BUFFERS 10
//#define DEFAULT_V4L_BUFFERS 4

//// if enabled, copies data from the buffer. this uses a bit more memory,
////  but much more reliable for some UVC cameras
//#define USE_TEMP_BUFFER

//#define MAX_DEVICE_DRIVER_NAME 80

///* Device Capture Objects */
///* V4L2 structure */
//struct buffer
//{
//  void *  start;
//  size_t  length;
//};
//static unsigned int n_buffers = 0;

///* TODO: Dilemas: */
///* TODO: Consider drop the use of this data structure and perform ioctl to obtain needed values */
///* TODO: Consider at program exit return controls to the initial values - See v4l2_free_ranges function */
///* TODO: Consider at program exit reset the device to default values - See v4l2_free_ranges function */
//typedef struct v4l2_ctrl_range {
//  __u32 ctrl_id;
//  __s32 initial_value;
//  __s32 current_value;
//  __s32 minimum;
//  __s32 maximum;
//  __s32 default_value;
//} v4l2_ctrl_range;

//typedef struct CvCaptureCAM_V4L
//{
//    char* deviceName;
//    int deviceHandle;
//    int bufferIndex;
//    int FirstCapture;

//    int width; int height;
//    int mode;

//    struct video_capability capability;
//    struct video_window     captureWindow;
//    struct video_picture    imageProperties;
//    struct video_mbuf       memoryBuffer;
//    struct video_mmap       *mmaps;
//    char *memoryMap;
//    IplImage frame;

//   /* V4L2 variables */
//   buffer buffers[MAX_V4L_BUFFERS + 1];
//   struct v4l2_capability cap;
//   struct v4l2_input inp;
//   struct v4l2_format form;
//   struct v4l2_crop crop;
//   struct v4l2_cropcap cropcap;
//   struct v4l2_requestbuffers req;
//   struct v4l2_jpegcompression compr;
//   struct v4l2_control control;
//   enum v4l2_buf_type type;
//   struct v4l2_queryctrl queryctrl;

//   struct timeval timestamp;

//   /** value set the buffer of V4L*/
//   int sequence;

//   /* V4L2 control variables */
//   v4l2_ctrl_range** v4l2_ctrl_ranges;
//   int v4l2_ctrl_count;

//   int is_v4l2_device;
//}
//CvCaptureCAM_V4L;

//static void icvCloseCAM_V4L( CvCaptureCAM_V4L* capture );

//static int icvGrabFrameCAM_V4L( CvCaptureCAM_V4L* capture );
//static IplImage* icvRetrieveFrameCAM_V4L( CvCaptureCAM_V4L* capture, int );
//CvCapture* cvCreateCameraCapture_V4L( int index );

//static int    icvSetPropertyCAM_V4L( CvCaptureCAM_V4L* capture, int property_id, double value );

//static int icvSetVideoSize( CvCaptureCAM_V4L* capture, int w, int h);

///***********************   Implementations  ***************************************/

//static int numCameras = 0;
//static int indexList = 0;

//// IOCTL handling for V4L2
//#ifdef HAVE_IOCTL_ULONG
//static int xioctl( int fd, unsigned long request, void *arg)
//#else
//static int xioctl( int fd, int request, void *arg)
//#endif
//{

//  int r;


//  do r = v4l2_ioctl (fd, request, arg);
//  while (-1 == r && EINTR == errno);

//  return r;

//}


///* Simple test program: Find number of Video Sources available.
//   Start from 0 and go to MAX_CAMERAS while checking for the device with that name.
//   If it fails on the first attempt of /dev/video0, then check if /dev/video is valid.
//   Returns the global numCameras with the correct value (we hope) */

//static void icvInitCapture_V4L() {
//   int deviceHandle;
//   int CameraNumber;
//   char deviceName[MAX_DEVICE_DRIVER_NAME];

//   CameraNumber = 0;
//   while(CameraNumber < MAX_CAMERAS) {
//      /* Print the CameraNumber at the end of the string with a width of one character */
//      sprintf(deviceName, "/dev/video%1d", CameraNumber);
//      /* Test using an open to see if this new device name really does exists. */
//      deviceHandle = open(deviceName, O_RDONLY);
//      if (deviceHandle != -1) {
//         /* This device does indeed exist - add it to the total so far */
//    // add indexList
//    indexList|=(1 << CameraNumber);
//        numCameras++;
//    }
//    if (deviceHandle != -1)
//      close(deviceHandle);
//      /* Set up to test the next /dev/video source in line */
//      CameraNumber++;
//   } /* End while */

//}; /* End icvInitCapture_V4L */


//static int try_init_v4l2(CvCaptureCAM_V4L* capture, char *deviceName)
//{

//  // if detect = -1 then unable to open device
//  // if detect = 0 then detected nothing
//  // if detect = 1 then V4L2 device
//  int detect = 0;


//  // Test device for V4L2 compability

//  /* Open and test V4L2 device */
//  capture->deviceHandle = v4l2_open (deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);

//  if (capture->deviceHandle == 0)
//  {
//    detect = -1;

//    icvCloseCAM_V4L(capture);
//  }

//  if (detect == 0)
//  {
//    CLEAR (capture->cap);
//    if (-1 == xioctl (capture->deviceHandle, VIDIOC_QUERYCAP, &capture->cap))
//    {
//      detect = 0;

//      icvCloseCAM_V4L(capture);
//    }
//      else
//    {
//      CLEAR (capture->capability);
//      capture->capability.type = capture->cap.capabilities;

//      /* Query channels number */
//      if (-1 != xioctl (capture->deviceHandle, VIDIOC_G_INPUT, &capture->capability.channels))
//      {
//        detect = 1;
//      }
//    }
//  }

//  return detect;

//}


//static void v4l2_free_ranges(CvCaptureCAM_V4L* capture) {
//  int i;
//  if (capture->v4l2_ctrl_ranges != NULL) {
//    for (i = 0; i < capture->v4l2_ctrl_count; i++) {
//      /* Return device to initial values: */
//      /* double value = (capture->v4l2_ctrl_ranges[i]->initial_value == 0)?0.0:((float)capture->v4l2_ctrl_ranges[i]->initial_value - capture->v4l2_ctrl_ranges[i]->minimum) / (capture->v4l2_ctrl_ranges[i]->maximum - capture->v4l2_ctrl_ranges[i]->minimum); */
//      /* Return device to default values: */
//      /* double value = (capture->v4l2_ctrl_ranges[i]->default_value == 0)?0.0:((float)capture->v4l2_ctrl_ranges[i]->default_value - capture->v4l2_ctrl_ranges[i]->minimum + 1) / (capture->v4l2_ctrl_ranges[i]->maximum - capture->v4l2_ctrl_ranges[i]->minimum); */

//      /* icvSetPropertyCAM_V4L(capture, capture->v4l2_ctrl_ranges[i]->ctrl_id, value); */
//      free(capture->v4l2_ctrl_ranges[i]);
//    }
//  }
//  free(capture->v4l2_ctrl_ranges);
//  capture->v4l2_ctrl_count  = 0;
//  capture->v4l2_ctrl_ranges = NULL;
//}

//static void v4l2_add_ctrl_range(CvCaptureCAM_V4L* capture, v4l2_control* ctrl) {
//  v4l2_ctrl_range* range    = (v4l2_ctrl_range*)malloc(sizeof(v4l2_ctrl_range));
//  range->ctrl_id            = ctrl->id;
//  range->initial_value      = ctrl->value;
//  range->current_value      = ctrl->value;
//  range->minimum            = capture->queryctrl.minimum;
//  range->maximum            = capture->queryctrl.maximum;
//  range->default_value      = capture->queryctrl.default_value;
//  capture->v4l2_ctrl_ranges[capture->v4l2_ctrl_count] = range;
//  capture->v4l2_ctrl_count += 1;
//  capture->v4l2_ctrl_ranges = (v4l2_ctrl_range**)realloc((v4l2_ctrl_range**)capture->v4l2_ctrl_ranges, (capture->v4l2_ctrl_count + 1) * sizeof(v4l2_ctrl_range*));
//}

//static int v4l2_get_ctrl_default(CvCaptureCAM_V4L* capture, __u32 id) {
//  int i;
//  for (i = 0; i < capture->v4l2_ctrl_count; i++) {
//    if (id == capture->v4l2_ctrl_ranges[i]->ctrl_id) {
//      return capture->v4l2_ctrl_ranges[i]->default_value;
//    }
//  }
//  return -1;
//}

//static int v4l2_get_ctrl_min(CvCaptureCAM_V4L* capture, __u32 id) {
//  int i;
//  for (i = 0; i < capture->v4l2_ctrl_count; i++) {
//    if (id == capture->v4l2_ctrl_ranges[i]->ctrl_id) {
//      return capture->v4l2_ctrl_ranges[i]->minimum;
//    }
//  }
//  return -1;
//}

//static int v4l2_get_ctrl_max(CvCaptureCAM_V4L* capture, __u32 id) {
//  int i;
//  for (i = 0; i < capture->v4l2_ctrl_count; i++) {
//    if (id == capture->v4l2_ctrl_ranges[i]->ctrl_id) {
//      return capture->v4l2_ctrl_ranges[i]->maximum;
//    }
//  }
//  return -1;
//}


//static void v4l2_scan_controls(CvCaptureCAM_V4L* capture) {

//  __u32 ctrl_id;
//  struct v4l2_control c;
//  if (capture->v4l2_ctrl_ranges != NULL) {
//    v4l2_free_ranges(capture);
//  }
//  capture->v4l2_ctrl_ranges = (v4l2_ctrl_range**)malloc(sizeof(v4l2_ctrl_range*));
//#ifdef V4L2_CTRL_FLAG_NEXT_CTRL
//  /* Try the extended control API first */
//  capture->queryctrl.id      = V4L2_CTRL_FLAG_NEXT_CTRL;
//  if(0 == v4l2_ioctl (capture->deviceHandle, VIDIOC_QUERYCTRL, &capture->queryctrl)) {
//    do {
//      c.id = capture->queryctrl.id;
//      capture->queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
//      if(capture->queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
//        continue;
//      }
//      if(capture->queryctrl.type != V4L2_CTRL_TYPE_INTEGER &&
//         capture->queryctrl.type != V4L2_CTRL_TYPE_BOOLEAN &&
//         capture->queryctrl.type != V4L2_CTRL_TYPE_MENU) {
//        continue;
//      }
//      if(v4l2_ioctl(capture->deviceHandle, VIDIOC_G_CTRL, &c) == 0) {
//        v4l2_add_ctrl_range(capture, &c);
//      }

//    } while(0 == v4l2_ioctl (capture->deviceHandle, VIDIOC_QUERYCTRL, &capture->queryctrl));
//  } else
//#endif
//  {
//    /* Check all the standard controls */
//    for(ctrl_id=V4L2_CID_BASE; ctrl_id<V4L2_CID_LASTP1; ctrl_id++) {
//      capture->queryctrl.id = ctrl_id;
//      if(v4l2_ioctl(capture->deviceHandle, VIDIOC_QUERYCTRL, &capture->queryctrl) == 0) {
//        if(capture->queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
//          continue;
//        }
//        if(capture->queryctrl.type != V4L2_CTRL_TYPE_INTEGER &&
//           capture->queryctrl.type != V4L2_CTRL_TYPE_BOOLEAN &&
//           capture->queryctrl.type != V4L2_CTRL_TYPE_MENU) {
//          continue;
//        }
//        c.id = ctrl_id;

//        if(v4l2_ioctl(capture->deviceHandle, VIDIOC_G_CTRL, &c) == 0) {
//          v4l2_add_ctrl_range(capture, &c);
//        }
//      }
//    }

//    /* Check any custom controls */
//    for(ctrl_id=V4L2_CID_PRIVATE_BASE; ; ctrl_id++) {
//      capture->queryctrl.id = ctrl_id;
//      if(v4l2_ioctl(capture->deviceHandle, VIDIOC_QUERYCTRL, &capture->queryctrl) == 0) {
//        if(capture->queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
//          continue;
//        }


//        if(capture->queryctrl.type != V4L2_CTRL_TYPE_INTEGER &&
//           capture->queryctrl.type != V4L2_CTRL_TYPE_BOOLEAN &&
//           capture->queryctrl.type != V4L2_CTRL_TYPE_MENU) {
//           continue;
//        }

//        c.id = ctrl_id;

//        if(v4l2_ioctl(capture->deviceHandle, VIDIOC_G_CTRL, &c) == 0) {
//          v4l2_add_ctrl_range(capture, &c);
//        }
//      } else {
//        break;
//      }
//    }
//  }
//}

//static inline int channels_for_mode()
//{
////    switch(mode) {
////    case CV_CAP_MODE_GRAY:
////        return 1;
////    case CV_CAP_MODE_YUYV:
////        return 2;
////    default:
//        return 3;
//    //}
//}

//static int _capture_V4L2 (CvCaptureCAM_V4L *capture, char *deviceName)
//{
//   int detect_v4l2 = 0;

//   capture->deviceName = strdup(deviceName);

//   detect_v4l2 = try_init_v4l2(capture, deviceName);

//   if (detect_v4l2 != 1) {
//       /* init of the v4l2 device is not OK */
//       return -1;
//   }

//   /* starting from here, we assume we are in V4L2 mode */
//   capture->is_v4l2_device = 1;

//   capture->v4l2_ctrl_ranges = NULL;
//   capture->v4l2_ctrl_count = 0;

//   /* Scan V4L2 controls */
//   v4l2_scan_controls(capture);

//   if ((capture->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
//      /* Nope. */
//      fprintf( stderr, "VIDEOIO ERROR: V4L2: device %s is unable to capture video memory.\n",deviceName);
//      icvCloseCAM_V4L(capture);
//      return -1;
//   }

//   /* The following code sets the CHANNEL_NUMBER of the video input.  Some video sources
//   have sub "Channel Numbers".  For a typical V4L TV capture card, this is usually 1.
//   I myself am using a simple NTSC video input capture card that uses the value of 1.
//   If you are not in North America or have a different video standard, you WILL have to change
//   the following settings and recompile/reinstall.  This set of settings is based on
//   the most commonly encountered input video source types (like my bttv card) */

//   if(capture->inp.index > 0) {
//       CLEAR (capture->inp);
//       capture->inp.index = CHANNEL_NUMBER;
//       /* Set only channel number to CHANNEL_NUMBER */
//       /* V4L2 have a status field from selected video mode */
//       if (-1 == xioctl (capture->deviceHandle, VIDIOC_ENUMINPUT, &capture->inp))
//       {
//         fprintf (stderr, "VIDEOIO ERROR: V4L2: Aren't able to set channel number\n");
//         icvCloseCAM_V4L (capture);
//         return -1;
//       }
//   } /* End if */

//   /* Find Window info */
//   CLEAR (capture->form);
//   capture->form.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

//   if (-1 == xioctl (capture->deviceHandle, VIDIOC_G_FMT, &capture->form)) {
//       fprintf( stderr, "VIDEOIO ERROR: V4L2: Could not obtain specifics of capture window.\n\n");
//       icvCloseCAM_V4L(capture);
//       return -1;
//   }

//  /* libv4l will convert from any format to V4L2_PIX_FMT_BGR24,
//     V4L2_PIX_FMT_RGV24, or V4L2_PIX_FMT_YUV420 */
//  unsigned int requestedPixelFormat;
//  //switch (capture->mode) {
//  //case CV_CAP_MODE_RGB:
//    requestedPixelFormat = V4L2_PIX_FMT_RGB24;
//    //break;
////  case CV_CAP_MODE_GRAY:
////    requestedPixelFormat = V4L2_PIX_FMT_YUV420;
////    break;
////  case CV_CAP_MODE_YUYV:
////    requestedPixelFormat = V4L2_PIX_FMT_YUYV;
////    break;
////  default:
////    requestedPixelFormat = V4L2_PIX_FMT_BGR24;
////    break;
////  }
//  CLEAR (capture->form);
//  capture->form.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//  capture->form.fmt.pix.pixelformat = requestedPixelFormat;
//  capture->form.fmt.pix.field       = V4L2_FIELD_ANY;
//  capture->form.fmt.pix.width       = capture->width;
//  capture->form.fmt.pix.height      = capture->height;

//  if (-1 == xioctl (capture->deviceHandle, VIDIOC_S_FMT, &capture->form)) {
//      fprintf(stderr, "VIDEOIO ERROR: libv4l unable to ioctl S_FMT\n");
//      return -1;
//  }

//  if (requestedPixelFormat != capture->form.fmt.pix.pixelformat) {
//      fprintf( stderr, "VIDEOIO ERROR: libv4l unable convert to requested pixfmt\n");
//      return -1;
//  }

//   /* icvSetVideoSize(capture, DEFAULT_V4L_WIDTH, DEFAULT_V4L_HEIGHT); */

//   unsigned int min;

//   /* Buggy driver paranoia. */
//   min = capture->form.fmt.pix.width * 2;

//   if (capture->form.fmt.pix.bytesperline < min)
//       capture->form.fmt.pix.bytesperline = min;

//   min = capture->form.fmt.pix.bytesperline * capture->form.fmt.pix.height;

//   if (capture->form.fmt.pix.sizeimage < min)
//       capture->form.fmt.pix.sizeimage = min;

//   CLEAR (capture->req);

//   unsigned int buffer_number = DEFAULT_V4L_BUFFERS;

//   try_again:

//   capture->req.count = buffer_number;
//   capture->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//   capture->req.memory = V4L2_MEMORY_MMAP;

//   if (-1 == xioctl (capture->deviceHandle, VIDIOC_REQBUFS, &capture->req))
//   {
//       if (EINVAL == errno)
//       {
//         fprintf (stderr, "%s does not support memory mapping\n", deviceName);
//       } else {
//         perror ("VIDIOC_REQBUFS");
//       }
//       /* free capture, and returns an error code */
//       icvCloseCAM_V4L (capture);
//       return -1;
//   }

//   if (capture->req.count < buffer_number)
//   {
//       if (buffer_number == 1)
//       {
//           fprintf (stderr, "Insufficient buffer memory on %s\n", deviceName);

//           /* free capture, and returns an error code */
//           icvCloseCAM_V4L (capture);
//           return -1;
//       } else {
//         buffer_number--;
//   fprintf (stderr, "Insufficient buffer memory on %s -- decreaseing buffers\n", deviceName);

//   goto try_again;
//       }
//   }

//   for (n_buffers = 0; n_buffers < capture->req.count; ++n_buffers)
//   {
//       struct v4l2_buffer buf;

//       CLEAR (buf);

//       buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//       buf.memory = V4L2_MEMORY_MMAP;
//       buf.index = n_buffers;

//       if (-1 == xioctl (capture->deviceHandle, VIDIOC_QUERYBUF, &buf)) {
//           perror ("VIDIOC_QUERYBUF");

//           /* free capture, and returns an error code */
//           icvCloseCAM_V4L (capture);
//           return -1;
//       }

//       capture->buffers[n_buffers].length = buf.length;
//       capture->buffers[n_buffers].start =
//         v4l2_mmap (NULL /* start anywhere */,
//                    buf.length,
//                    PROT_READ | PROT_WRITE /* required */,
//                    MAP_SHARED /* recommended */,
//                    capture->deviceHandle, buf.m.offset);

//       if (MAP_FAILED == capture->buffers[n_buffers].start) {
//           perror ("mmap");

//           /* free capture, and returns an error code */
//           icvCloseCAM_V4L (capture);
//           return -1;
//       }

//#ifdef USE_TEMP_BUFFER
//       if (n_buffers == 0) {
//           if (capture->buffers[MAX_V4L_BUFFERS].start) {
//               free(capture->buffers[MAX_V4L_BUFFERS].start);
//               capture->buffers[MAX_V4L_BUFFERS].start = NULL;
//       }

//           capture->buffers[MAX_V4L_BUFFERS].start = malloc(buf.length);
//           capture->buffers[MAX_V4L_BUFFERS].length = buf.length;
//       };
//#endif
//   }

//   /* Set up Image data */
//   cvInitImageHeader( &capture->frame,
//                      cvSize( capture->captureWindow.width,
//                              capture->captureWindow.height ),
//                      IPL_DEPTH_8U, channels_for_mode(),
//                      IPL_ORIGIN_TL, 4 );
//   /* Allocate space for RGBA data */
//   capture->frame.imageData = (char *)cvAlloc(capture->frame.imageSize);

//   return 1;
//}; /* End _capture_V4L2 */


//static CvCaptureCAM_V4L * icvCaptureFromCAM_V4L (int index)
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

//#ifdef USE_TEMP_BUFFER
//   capture->buffers[MAX_V4L_BUFFERS].start = NULL;
//#endif

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

//   /* set the default size */
//   capture->width  = DEFAULT_V4L_WIDTH;
//   capture->height = DEFAULT_V4L_HEIGHT;

//   if (_capture_V4L2 (capture, deviceName) == -1) {
//       icvCloseCAM_V4L(capture);
//       return NULL;
//   }
//   else
//   {
//       capture->is_v4l2_device = 1;
//   }

//   return capture;
//}; /* End icvOpenCAM_V4L */


//static int read_frame_v4l2(CvCaptureCAM_V4L* capture) {
//    struct v4l2_buffer buf;

//    CLEAR (buf);

//    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    buf.memory = V4L2_MEMORY_MMAP;

//    if (-1 == xioctl (capture->deviceHandle, VIDIOC_DQBUF, &buf)) {
//        switch (errno) {
//        case EAGAIN:
//            return 0;

//        case EIO:
//            /* Could ignore EIO, see spec. */

//            /* fall through */

//        default:
//            /* display the error and stop processing */
//            perror ("VIDIOC_DQBUF");
//            return 1;
//        }
//   }

//   assert(buf.index < capture->req.count);

//#ifdef USE_TEMP_BUFFER
//   memcpy(capture->buffers[MAX_V4L_BUFFERS].start,
//    capture->buffers[buf.index].start,
//    capture->buffers[MAX_V4L_BUFFERS].length );
//   capture->bufferIndex = MAX_V4L_BUFFERS;
//   //printf("got data in buff %d, len=%d, flags=0x%X, seq=%d, used=%d)\n",
//   //   buf.index, buf.length, buf.flags, buf.sequence, buf.bytesused);
//#else
//   capture->bufferIndex = buf.index;
//#endif

//   if (-1 == xioctl (capture->deviceHandle, VIDIOC_QBUF, &buf))
//       perror ("VIDIOC_QBUF");

//   //set timestamp in capture struct to be timestamp of most recent frame
//   /** where timestamps refer to the instant the field or frame was received by the driver, not the capture time*/
//   capture->timestamp = buf.timestamp;   //printf( "timestamp update done \n");
//   capture->sequence = buf.sequence;

//   return 1;
//}

//static void mainloop_v4l2(CvCaptureCAM_V4L* capture) {
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

//static int icvGrabFrameCAM_V4L(CvCaptureCAM_V4L* capture) {

//   if (capture->FirstCapture) {
//      /* Some general initialization must take place the first time through */

//      /* This is just a technicality, but all buffers must be filled up before any
//         staggered SYNC is applied.  SO, filler up. (see V4L HowTo) */

//        for (capture->bufferIndex = 0;
//             capture->bufferIndex < ((int)capture->req.count);
//             ++capture->bufferIndex)
//        {

//          struct v4l2_buffer buf;

//          CLEAR (buf);

//          buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//          buf.memory      = V4L2_MEMORY_MMAP;
//          buf.index       = (unsigned long)capture->bufferIndex;

//          if (-1 == xioctl (capture->deviceHandle, VIDIOC_QBUF, &buf)) {
//              perror ("VIDIOC_QBUF");
//              return 0;
//          }
//        }

//        /* enable the streaming */
//        capture->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//        if (-1 == xioctl (capture->deviceHandle, VIDIOC_STREAMON,
//                          &capture->type)) {
//            /* error enabling the stream */
//            perror ("VIDIOC_STREAMON");
//            return 0;
//        }

//      /* preparation is ok */
//      capture->FirstCapture = 0;
//   }

//   mainloop_v4l2(capture);

//   return(1);
//}

//static IplImage* icvRetrieveFrameCAM_V4L( CvCaptureCAM_V4L* capture, int) {

//   /* Now get what has already been captured as a IplImage return */

//   /* First, reallocate imageData if the frame size changed */

//    if(((unsigned long)capture->frame.width != capture->form.fmt.pix.width)
//       || ((unsigned long)capture->frame.height != capture->form.fmt.pix.height)) {
//        cvFree(&capture->frame.imageData);
//        cvInitImageHeader( &capture->frame,
//                           cvSize( capture->form.fmt.pix.width,
//                                   capture->form.fmt.pix.height ),
//                           IPL_DEPTH_8U, channels_for_mode(),
//                           IPL_ORIGIN_TL, 4 );
//       capture->frame.imageData = (char *)cvAlloc(capture->frame.imageSize);
//    }

//   if(capture->buffers[capture->bufferIndex].start){
//      memcpy((char *)capture->frame.imageData,
//         (char *)capture->buffers[capture->bufferIndex].start,
//         capture->frame.imageSize);
//    }

//   return(&capture->frame);
//}

//static double GetTimestampPropertyCAM_V4L (CvCaptureCAM_V4L* capture) {
//      /* initialize the control structure */
//    if (capture->FirstCapture) {
//          return 0;
//      } else {
//          //would be maximally numerically stable to cast to convert as bits, but would also be counterintuitive to decode
//          return 1000 * capture->timestamp.tv_sec + ((double) capture->timestamp.tv_usec) / 1000;
//      }
//}


//static void icvCloseCAM_V4L( CvCaptureCAM_V4L* capture ){
//   /* Deallocate space - Hopefully, no leaks */
//   if (capture) {
//     v4l2_free_ranges(capture);

//       capture->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//       if (xioctl(capture->deviceHandle, VIDIOC_STREAMOFF, &capture->type) < 0) {
//         perror ("Unable to stop the stream.");
//       }
//       for (unsigned int n_buffers2 = 0; n_buffers2 < capture->req.count; ++n_buffers2) {
//         if (-1 == v4l2_munmap (capture->buffers[n_buffers2].start, capture->buffers[n_buffers2].length)) {
//           perror ("munmap");
//         }
//       }

//       if (capture->deviceHandle != -1) {
//         v4l2_close(capture->deviceHandle);
//       }

//     if (capture->frame.imageData)
//       cvFree(&capture->frame.imageData);

//#ifdef USE_TEMP_BUFFER
//     if (capture->buffers[MAX_V4L_BUFFERS].start) {
//       free(capture->buffers[MAX_V4L_BUFFERS].start);
//       capture->buffers[MAX_V4L_BUFFERS].start = NULL;
//     }
//#endif

//     free(capture->deviceName);
//     capture->deviceName = NULL;
//     //v4l2_free_ranges(capture);
//     //cvFree((void **)capture);
//   }
//};


//class CvCaptureCAM_V4L_CPP : CvCapture
//{
//public:
//    CvCaptureCAM_V4L_CPP() { captureV4L = 0; }
//    virtual ~CvCaptureCAM_V4L_CPP() { close(); }

//    virtual bool open( int index );
//    virtual void close();

//    virtual double getProperty(int) const;
//    virtual bool setProperty(int, double);
//    virtual bool grabFrame();
//    virtual IplImage* retrieveFrame(int);
//protected:

//    CvCaptureCAM_V4L* captureV4L;
//};

//bool CvCaptureCAM_V4L_CPP::open( int index )
//{
//    close();
//    captureV4L = icvCaptureFromCAM_V4L(index);
//    return captureV4L != 0;
//}

//void CvCaptureCAM_V4L_CPP::close()
//{
//    if( captureV4L )
//    {
//        icvCloseCAM_V4L( captureV4L );
//        cvFree( &captureV4L );
//    }
//}

//bool CvCaptureCAM_V4L_CPP::grabFrame()
//{
//    return captureV4L ? icvGrabFrameCAM_V4L( captureV4L ) != 0 : false;
//}

//IplImage* CvCaptureCAM_V4L_CPP::retrieveFrame(int)
//{
//    return captureV4L ? icvRetrieveFrameCAM_V4L( captureV4L, 0 ) : 0;
//}

//double CvCaptureCAM_V4L_CPP::getProperty( int propId ) const
//{
//    return captureV4L ? icvGetPropertyCAM_V4L( captureV4L, propId ) : 0.0;
//}

//bool CvCaptureCAM_V4L_CPP::setProperty( int propId, double value )
//{
//    return captureV4L ? icvSetPropertyCAM_V4L( captureV4L, propId, value ) != 0 : false;
//}

//CvCapture* cvCreateCameraCapture_V4L( int index )
//{
//    CvCaptureCAM_V4L_CPP* capture = new CvCaptureCAM_V4L_CPP;

//    if( capture->open( index ))
//        return (CvCapture*)capture;

//    delete capture;
//    return 0;
//}
