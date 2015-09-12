#include "eyetrackerWorker.h"

using namespace cv;

EyeTrackerWorker::EyeTrackerWorker()
{
    close = false;
}

EyeTrackerWorker::~EyeTrackerWorker()
{
}

void EyeTrackerWorker::onImageCaptured(IplImage image)
{
    while(!close)
    {
        cvResize(&image,&image);
    }
    emit finished();
}

void EyeTrackerWorker::abortThread()
{
    close = true;
}
