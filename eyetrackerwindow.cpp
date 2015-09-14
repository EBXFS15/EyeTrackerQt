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

    ebxMonitorModel = new QStandardItemModel;

    captureWorker.moveToThread(&captureThread);
    eyetrackerWorker.moveToThread(&eyetrackerThread);

    connect(&captureThread, SIGNAL(started()), &captureWorker, SLOT(process()));
    connect(&captureWorker, SIGNAL(qimageCaptured(QImage, double)), this, SLOT(onCaptured(QImage, double)));
    connect(&captureWorker, SIGNAL(finished()), &captureThread, SLOT(quit()));
    connect(&captureWorker, SIGNAL(finished()), &captureWorker, SLOT(deleteLater()));
    connect(&captureThread, SIGNAL(finished()), &captureThread, SLOT(deleteLater()));

    connect(&captureWorker, SIGNAL(imageCaptured(IplImage)), &eyetrackerWorker, SLOT(onImageCaptured(IplImage)));
    connect(&eyetrackerWorker, SIGNAL(eyeFound(int,int)), this, SLOT(onEyeFound(int,int)));

    connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerThread, SLOT(quit()));
    connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerWorker, SLOT(deleteLater()));
    connect(&eyetrackerThread, SIGNAL(finished()), &eyetrackerThread, SLOT(deleteLater()));

    connect(&captureWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));
    connect(&eyetrackerWorker, SIGNAL(message(QString)), this, SLOT(onTrackerMessage(QString)));
    connect(ui->quitBtn,SIGNAL(clicked()),this,SLOT(onClosed()));
    connect(ui->btn_disable_preview,SIGNAL(clicked()),this,SLOT(togglePreview()));
    connect(ui->btn_disable_processing,SIGNAL(clicked()),this,SLOT(toggleProcessing()));

    connect(&captureWorker, SIGNAL(gotFrame(qint64)), this, SLOT(onGotFrame(qint64)));
    connect(this, SIGNAL(gotNewFrame(QStandardItemModel*,QTreeView*,qint64)), &ebxMonitorWorker,SLOT(gotNewFrame(QStandardItemModel*,QTreeView*,qint64)));

    ui->ebxMonitorTree->setModel(ebxMonitorModel);    
    ui->ebxMonitorTree->show();

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

    ui->ebxMonitorTree->setModel(new QStandardItemModel);
    delete ebxMonitorModel;
    captureWorker.stopCapturing();
    eyetrackerWorker.abortThread();        
    eyetrackerThread.wait();
    captureThread.wait();
    this->close();
}

void EyeTrackerWindow::onEyeFound(int x, int y)
{
    captureWorker.setCenter(x,y);
}

void EyeTrackerWindow::togglePreview()
{
    captureWorker.togglePreview();
}

void EyeTrackerWindow::toggleProcessing()
{
    eyetrackerWorker.toggleProcessing();
}

void EyeTrackerWindow::onGotFrame(qint64 id)
{
    static double avg = -1;
    static int count = 31;
    if (count++ > 30){
        double latency;
        struct timespec gotTime;
        clock_gettime(CLOCK_MONOTONIC, &gotTime);
        qint64 rxTimeStamp = (((qint64)gotTime.tv_sec) * 1000000 + ((qint64)gotTime.tv_nsec)/1000);
        ui->label_rxtime_txt->setText(QString("%1").arg(rxTimeStamp));
        latency = (rxTimeStamp-id)/1000;
        ui->label_latency_txt->setText(QString("%1ms").arg(latency));

        if (avg < 0)
        {
            avg = latency;
        }
        else
        {
            avg = (avg + latency)/2;
            ui->label_latency_txt_avg->setText(QString("%1ms").arg(avg));
        }
        emit gotNewFrame(ebxMonitorModel, ui->ebxMonitorTree, id);
        count = 0;
    }
}

void EyeTrackerWindow::on_pushButton_pressed()
{
    //ebxMonitorWorker.updateEbxMonitorData(ebxMonitorModel, ui->ebxMonitorTree);
}
