#include "eyetrackerwindow.h"
#include "ui_eyetrackerwindow.h"
#include <QtConcurrent/QtConcurrent>
#include <QThread>


using namespace cv;

EyeTrackerWindow::EyeTrackerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EyeTrackerWindow)
{

    ui->setupUi(this);
    pixmap = new QPixmap();
    capture = cvCreateCameraCapture(0);
//    QtConcurrent::run(this, &EyeTrackerWindow::getImage);
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
        //QImage captFrame(QSize(640,480),QImage::Format_RGB32);
        //captFrame.fill(Qt::red);
        updateImage(QPixmap::fromImage(captFrame));
//        update();
        QApplication::processEvents();
        //QThread::msleep(100);
    }
}

void EyeTrackerWindow::paintEvent(QPaintEvent *event)
{
    ui->label->update();
}

