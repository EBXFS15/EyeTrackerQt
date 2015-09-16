#include "eyetrackerWorker.h"

using namespace cv;

EyeTrackerWorker::EyeTrackerWorker()
{
    close = 0;
    processing = 1;
    cascade = NULL;
    cascade = (CvHaarClassifierCascade*)cvLoad("/usr/local/bin/haarcascade_eye_tree_eyeglasses.xml");
    cvInitImageHeader(&grayImg,cvSize(320,240),IPL_DEPTH_8U, 1, IPL_ORIGIN_TL, 4 );
    /* Allocate space for RGBA data */
    grayImg.imageData = (char *)cvAlloc(grayImg.imageSize);
}

EyeTrackerWorker::~EyeTrackerWorker()
{
    cvFree(&grayImg.imageData);
}

void EyeTrackerWorker::onImageCaptured(IplImage image)
{
    if((0 == close) && (cascade!=NULL) && (true == processing))
    {
        cvCvtColor(&image,&grayImg,CV_RGB2GRAY);
        CvMemStorage *storage = cvCreateMemStorage(0);
        CvSeq* faces;
        CvSize size= cvSize(80,80);
        faces = cvHaarDetectObjects(&image, cascade,storage,1.1,3,CV_HAAR_FIND_BIGGEST_OBJECT,size);
        for(int i=0;i< faces->total; i++)
        {
           CvRect eye_rect = *(CvRect*) cvGetSeqElem(faces,i);
           CvPoint center= cvPoint(eye_rect.x+eye_rect.width/2,eye_rect.y+eye_rect.height/2);
           emit eyeFound(center.x, center.y);
        }
    }
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    if(1==close)
    {
        emit finished();
    }
}

void EyeTrackerWorker::abortThread()
{
    close = 1;
}

void EyeTrackerWorker::toggleProcessing()
{
    processing = !processing;
}
