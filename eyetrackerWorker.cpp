#include "eyetrackerWorker.h"

using namespace cv;

EyeTrackerWorker::EyeTrackerWorker()
{
    close = 0;
    processing = 1;
    cascade = NULL;
    storage = NULL;
    faces = NULL;
    cascade = (CvHaarClassifierCascade*)cvLoad("./haarcascade_eye_tree_eyeglasses.xml");
    cvInitImageHeader(&grayImg,cvSize(320,240),IPL_DEPTH_8U, 1, IPL_ORIGIN_TL, 4 );
    /* Allocate space for RGBA data */
    grayImg.imageData = (char *)cvAlloc(grayImg.imageSize);
}

EyeTrackerWorker::~EyeTrackerWorker()
{
    cvFree(&grayImg.imageData);
    cvClearMemStorage( storage );
    cvReleaseMemStorage(&storage);
}

void EyeTrackerWorker::onImageCaptured(IplImage image)
{
    if((!close) && (cascade!=NULL) && (true == processing))
    {
        cvCvtColor(&image,&grayImg,CV_RGB2GRAY);
        storage = cvCreateMemStorage(0);
        CvSize size= cvSize(80,80);
        faces = cvHaarDetectObjects(&image, cascade,storage,1.1,3,CV_HAAR_FIND_BIGGEST_OBJECT,size);
        for(int i=0;i< faces->total; i++)
        {
           CvRect eye_rect = *(CvRect*) cvGetSeqElem(faces,i);
           CvPoint center= cvPoint(eye_rect.x+eye_rect.width/2,eye_rect.y+eye_rect.height/2);           
           emit eyeFound(center.x, center.y);
           if(close)
           {
               break;
           }
        }
    }
    if(close)
    {
        this->thread()->quit();
        emit finished();
    }
    QCoreApplication::processEvents();
}

void EyeTrackerWorker::abortThread()
{
    close = 1;
}

void EyeTrackerWorker::set_processing(bool enable)
{
    if(enable)
    {
        processing = 1;
    }
    else
    {
        processing = 0;
    }
}

