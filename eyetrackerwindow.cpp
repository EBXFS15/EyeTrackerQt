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
    //timestamp = -1;



    /** Signals to stop the workers. Some are less relevant but it seems to be better to do it this way. */

    connect(this, SIGNAL(stopEbxEyeTracker()),&eyetrackerWorker, SLOT(abortThread()));
    connect(this, SIGNAL(stopEbxCaptureWorker()),&captureWorker, SLOT(stopCapturing()));


    /** ebxMonitor setup
     * The ebxMonitor waits for a trigger to start parsing and matching timestamps.
     * During parsing all matching timestamps are reported through the respective signals
     * As soon one timestamp has been found the finisched signal is sent.
     * This signal is used to enable the intercept button on the gui.
     *
     * In addition to the parsing the ebxMonitorWorker collects the timestamps from the top level aplication.
     */
    ebxMonitorWorker = new EbxMonitorWorker();
    setupEbxMonitorTree();
    connect(&ebxMonitorThread, SIGNAL(started()),ebxMonitorWorker,SLOT(sarchMatch()));
    connect(this, SIGNAL(startIntercept()), ebxMonitorWorker, SLOT(sarchMatch()));

    connect(ebxMonitorWorker, SIGNAL(reportInitialTimestamp(QList<QString>)), this, SLOT(reportInitialTimestamp(QList<QString>)));
    connect(ebxMonitorWorker, SIGNAL(reportMeasurementPoint(QList<QString>)), this, SLOT(reportMeasurementPoint(QList<QString>)));

    connect(ebxMonitorWorker, SIGNAL(finished()),this, SLOT(enableInterception()));

    connect(this, SIGNAL(gotNewFrame(qint64,int)), ebxMonitorWorker,SLOT(gotNewFrame(qint64,int)));

    connect(ebxMonitorWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));
    //connect(ebxMonitorWorker, SIGNAL(finished()), ebxMonitorWorker, SLOT(deleteLater()));

    /** Signals to stop the workers. Some are less relevant but it seems to be better to do it this way. */
    connect(this, SIGNAL(stopEbxMonitor()),ebxMonitorWorker, SLOT(stop()));




    /** This is absolutely needed. Informs the Thread that the work is done. **/
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
    /*
    ui->label_timestamp->setText("Timestamp:"+ QString::number(timestamp));
    if(this->timestamp>-1)
    {
        ui->label_fps->setText("FPS:"+ QString::number(1/(timestamp-this->timestamp)));
    }
    this->timestamp = timestamp;
*/
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
    emit stopEbxMonitor();
    ebxMonitorThread.exit();
    ebxMonitorThread.wait();
    ebxMonitorModel->deleteLater();

    cleanEbxMonitorTree();

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
    static double avgLatency = -1;
    static double avgFps = -1;
    static qint64 rxTimeStamp;
    static qint64 previousTimestamp;
    static int count = INTERCEPTION_FRAME_COUNT;
    struct timespec gotTime;
    double fps = 0;
    double latency = 0;

    clock_gettime(CLOCK_MONOTONIC, &gotTime);
    rxTimeStamp = (((qint64)gotTime.tv_sec) * 1000000 + ((qint64)gotTime.tv_nsec)/1000);
    latency = (rxTimeStamp-id)/1000;
    fps = 1000000/((double)(rxTimeStamp - previousTimestamp));

    if (avgLatency < 0)
    {
        avgLatency = latency;
        avgFps = fps;
    }
    else
    {
        avgLatency = (avgLatency + latency)/2;
        avgFps = (avgFps + fps)/2;
        ui->label_latency_txt_avg->setText(QString("%1ms").arg(avgLatency));

        ui->label_fps_avg_txt->setText(QString("%1").arg(avgFps));
        ui->label_fps_txt->setText(QString("%1").arg(fps));
        ui->label_rxtime_txt->setText(QString("%1").arg(rxTimeStamp));        
        ui->label_latency_txt->setText(QString("%1ms").arg(latency));
    }
    previousTimestamp = rxTimeStamp;
    if (count++ > INTERCEPTION_FRAME_COUNT){
        emit gotNewFrame(id, 255);
        count = 0;
    }
}

void EyeTrackerWindow::on_pushButton_pressed()
{
    //ebxMonitorWorker.updateEbxMonitorData(ebxMonitorModel, ui->ebxMonitorTree);
}

void EyeTrackerWindow::enableInterception()
{
    ui->btn_intercept->setEnabled(true);
}

void EyeTrackerWindow::on_btn_intercept_pressed()
{
    ui->btn_intercept->setEnabled(false);
    emit startIntercept();
}

void EyeTrackerWindow::cleanEbxMonitorTree()
{
    if(ebxMonitorModel != 0)
    {
        delete ebxMonitorModel;
        ebxMonitorModel = 0;
    }
    while(!createdStandardItems.isEmpty())
    {
        delete createdStandardItems.at(0);
        createdStandardItems.removeAt(0);
    }
}

void EyeTrackerWindow::setupEbxMonitorTree()
{
    ebxMonitorModel = new QStandardItemModel(this);
    ui->ebxMonitorTree->setModel(ebxMonitorModel);
    ui->ebxMonitorTree->show();
    ui->ebxMonitorTree->setColumnWidth(0,180);
    ui->ebxMonitorTree->setColumnWidth(1,45);
    ui->ebxMonitorTree->setColumnWidth(2,180);
    ui->ebxMonitorTree->setColumnWidth(3,100);
    ui->ebxMonitorTree->setColumnWidth(4,100);
}

void EyeTrackerWindow::reportInitialTimestamp(QList<QString> data)
{
    cleanEbxMonitorTree();
    setupEbxMonitorTree();
    reportMeasurementPoint(data);
}

void EyeTrackerWindow::reportMeasurementPoint(QList<QString> data)
{
    QList<QStandardItem *> rowItems;
    foreach (QString str, data)
    {
        QStandardItem * tmpItem = new QStandardItem(str);
        createdStandardItems << tmpItem;
        rowItems << tmpItem;
    }
    ebxMonitorModel->invisibleRootItem()->appendRow(rowItems);
}

