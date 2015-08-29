#include "eyetrackerwindow.h"
#include "ui_eyetrackerwindow.h"

using namespace cv;

EyeTrackerWindow::EyeTrackerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EyeTrackerWindow)
{

    ui->setupUi(this);
    pixmap = new QPixmap();
    capture = cvCreateCameraCapture(0);
}

EyeTrackerWindow::~EyeTrackerWindow()
{
    delete ui;
    cvReleaseCapture(&capture);
}

void EyeTrackerWindow::updateImage(QPixmap pixmap)
{
    ui->label->clear();
    ui->label->setPixmap(pixmap);
}

void EyeTrackerWindow::getImage()
{
    for (;;)
    {
        IplImage* frame = cvQueryFrame(capture);
        cvCvtColor(frame, frame, CV_BGR2RGB);
        QImage captFrame((const uchar*)frame->imageData, 640, 480, QImage::Format_RGB888);
        updateImage(QPixmap::fromImage(captFrame));
        QApplication::processEvents();
    }
}
