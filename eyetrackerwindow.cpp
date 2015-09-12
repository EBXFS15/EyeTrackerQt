#include "eyetrackerwindow.h"
#include "ui_eyetrackerwindow.h"

using namespace cv;


EyeTrackerWindow::EyeTrackerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EyeTrackerWindow)
{
    ui->setupUi(this);

    //register Iplimage to use slot/signal with Qt
    qRegisterMetaType<IplImage>("IplImage");

    ui->label_message->setText(QString("Messages:\n"));
    timestamp = -1;

    captureWorker.moveToThread(&captureThread);
    eyetrackerWorker.moveToThread(&eyetrackerThread);
    connect(&captureThread, SIGNAL(started()), &captureWorker, SLOT(process()));
    connect(&captureWorker, SIGNAL(qimageCaptured(QImage, double)), this, SLOT(onCaptured(QImage, double)));
    connect(&captureWorker, SIGNAL(finished()), &captureThread, SLOT(quit()));
    connect(&captureWorker, SIGNAL(finished()), &captureWorker, SLOT(deleteLater()));
    connect(&captureThread, SIGNAL(finished()), &captureThread, SLOT(deleteLater()));

    connect(&captureWorker, SIGNAL(imageCaptured(IplImage)), &eyetrackerWorker, SLOT(onImageCaptured(IplImage)));
    connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerThread, SLOT(quit()));
    connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerWorker, SLOT(deleteLater()));
    connect(&eyetrackerThread, SIGNAL(finished()), &eyetrackerThread, SLOT(deleteLater()));

    connect(&captureWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));
    connect(&eyetrackerWorker, SIGNAL(message(QString)), this, SLOT(onTrackerMessage(QString)));

    connect(ui->quitBtn,SIGNAL(clicked()),this,SLOT(onClosed()));

    captureThread.start();
    eyetrackerThread.start();
}

EyeTrackerWindow::~EyeTrackerWindow()
{
    delete ui;
}

void EyeTrackerWindow::updateImage(QPixmap pixmap)
{
    ui->label->clear();
    ui->label->setPixmap(pixmap);
    ui->label->adjustSize();
}

void EyeTrackerWindow::addTimestamp(double timestamp)
{
    ui->label_timestamp->setText("Timestamp:"+ QString::number(timestamp));
    if(this->timestamp>-1)
    {
        ui->label_fps->setText("FPS:"+ QString::number(1/(timestamp-this->timestamp)));
    }
    this->timestamp = timestamp;

}

void EyeTrackerWindow::onCaptured(QImage captFrame, double timestamp)
{
    updateImage(QPixmap::fromImage(captFrame));
    addTimestamp(timestamp);
}

void EyeTrackerWindow::onCaptureMessage(QString msg)
{
    ui->label_message->setText(ui->label_message->text() + msg + "\n");
}

void EyeTrackerWindow::onTrackerMessage(QString msg)
{
    ui->label_message->setText(ui->label_message->text() + msg + "\n");
}


void EyeTrackerWindow::onClosed()
{
    eyetrackerWorker.abortThread();
    eyetrackerThread.wait();
    captureWorker.stopCapturing();
    captureThread.wait();
    this->close();
}
