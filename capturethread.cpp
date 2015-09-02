#include "capturethread.h"

CaptureThread::CaptureThread()
{
    m_close = false;
}

CaptureThread::~CaptureThread()
{
}

void CaptureThread::run()
{
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
}

void CaptureThread::stopCapturing()
{
    m_close = true;
}

