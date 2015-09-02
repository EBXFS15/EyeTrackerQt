#include "eyetrackerwindow.h"
#include "ui_eyetrackerwindow.h"

using namespace cv;


EyeTrackerWindow::EyeTrackerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EyeTrackerWindow)
{
    ui->setupUi(this);
    connect(&captureThread, SIGNAL(imageCaptured(QImage, double)), this, SLOT(onCaptured(QImage, double)));
    connect(ui->quitBtn,SIGNAL(clicked()),this,SLOT(onClosed()));
    captureThread.start();
}

EyeTrackerWindow::~EyeTrackerWindow()
{
    delete ui;
}

void EyeTrackerWindow::updateImage(QPixmap pixmap)
{
    ui->label->clear();
    ui->label->setPixmap(pixmap);
}

void EyeTrackerWindow::addTimestamp(double timestamp)
{
    ui->label_timestamp->setText("Timestamp:"+ QString::number(timestamp));
}

void EyeTrackerWindow::onCaptured(QImage captFrame, double timestamp)
{
    updateImage(QPixmap::fromImage(captFrame));
    addTimestamp(timestamp);
}

void EyeTrackerWindow::onClosed()
{
    captureThread.stopCapturing();
    captureThread.wait();
    this->close();
}
