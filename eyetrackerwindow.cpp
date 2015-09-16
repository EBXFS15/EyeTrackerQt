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
    ebxMonitorWorker = new EbxMonitorWorker(0,ebxMonitorModel,ui->ebxMonitorTree);

    /** Signals to stop the workers. Some are less relevant but it seems to be better to do it this way. */
    connect(this, SIGNAL(stopEbxMonitor()),ebxMonitorWorker, SLOT(stop()));
    connect(this, SIGNAL(stopEbxEyeTracker()),&eyetrackerWorker, SLOT(abortThread()));
    connect(this, SIGNAL(stopEbxCaptureWorker()),&captureWorker, SLOT(stopCapturing()));


    /** This is absolutely needed. Informs the Thread that the work is done. **/
    connect(ebxMonitorWorker, SIGNAL(finished()),&ebxMonitorThread, SLOT(quit()));
    connect(&captureWorker, SIGNAL(finished()), &captureThread, SLOT(quit()));
    connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerThread, SLOT(quit()));


    connect(&captureThread, SIGNAL(started()), &captureWorker, SLOT(process()));
    connect(&captureWorker, SIGNAL(qimageCaptured(QImage, double)), this, SLOT(onCaptured(QImage, double)));

    /**
     * I don't know why we should delete something that is handled by QT anyway
     * Main reason behind is that if I understand it correctly these objects are part of the parent object variables.
     * The delete later concept is found in examples where the thread is created dynamically.
     * I think this is not needed.
     */
    //connect(&captureWorker, SIGNAL(finished()), &captureWorker, SLOT(deleteLater()));
    //connect(&captureThread, SIGNAL(finished()), &captureThread, SLOT(deleteLater()));
    //connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerWorker, SLOT(deleteLater()));
    //connect(&eyetrackerThread, SIGNAL(finished()), &eyetrackerThread, SLOT(deleteLater()));

    connect(&captureWorker, SIGNAL(imageCaptured(IplImage)), &eyetrackerWorker, SLOT(onImageCaptured(IplImage)));
    connect(&eyetrackerWorker, SIGNAL(eyeFound(int,int)), this, SLOT(onEyeFound(int,int)));


    connect(&captureWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));
    connect(&eyetrackerWorker, SIGNAL(message(QString)), this, SLOT(onTrackerMessage(QString)));
    connect(ui->quitBtn,SIGNAL(clicked()),this,SLOT(onClosed()));
    connect(ui->btn_disable_preview,SIGNAL(clicked()),this,SLOT(togglePreview()));
    connect(ui->btn_disable_processing,SIGNAL(clicked()),this,SLOT(toggleProcessing()));

    connect(&captureWorker, SIGNAL(gotFrame(qint64)), this, SLOT(onGotFrame(qint64)));
    connect(this, SIGNAL(gotNewFrame(qint64)), ebxMonitorWorker,SLOT(gotNewFrame(qint64)));
    connect(&ebxMonitorThread, SIGNAL(started()), ebxMonitorWorker,SLOT(startGrabbing()));
    connect(ebxMonitorWorker, SIGNAL(finished()), ebxMonitorWorker, SLOT(deleteLater()));

    ui->ebxMonitorTree->setModel(ebxMonitorModel);
    ui->ebxMonitorTree->show();

    captureWorker.moveToThread(&captureThread);
    eyetrackerWorker.moveToThread(&eyetrackerThread);
    ebxMonitorWorker->moveToThread(&ebxMonitorThread);


    captureThread.start();
    eyetrackerThread.start();
    ebxMonitorThread.start();


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

    int cropPosition = 0;
    while (ui->label_message->text().split("\n").count() > 15)
    {
        cropPosition = ui->label_message->text().indexOf("\n",cropPosition) + 1;
        if (cropPosition > 0)
        {
            ui->label_message->setText(ui->label_message->text().mid(cropPosition));
        }
    }
    ui->label_message->setText(ui->label_message->text().mid(cropPosition) + msg + "\n");
}

void EyeTrackerWindow::onTrackerMessage(QString msg)
{
    onCaptureMessage(msg);
}


void EyeTrackerWindow::onClosed()
{

    ui->ebxMonitorTree->setModel(new QStandardItemModel);    
    emit stopEbxMonitor();
    ebxMonitorThread.exit();
    ebxMonitorThread.wait();
    ebxMonitorModel->deleteLater();

    emit stopEbxEyeTracker();
    emit stopEbxCaptureWorker();

    captureThread.wait();
    eyetrackerThread.wait();

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
    static int count = 11;
    if (count++ > 20){
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
        emit gotNewFrame(id);
        count = 0;
    }
}

void EyeTrackerWindow::on_pushButton_pressed()
{
    //ebxMonitorWorker.updateEbxMonitorData(ebxMonitorModel, ui->ebxMonitorTree);
}
