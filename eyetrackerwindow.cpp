#include "eyetrackerwindow.h"
#include "ui_eyetrackerwindow.h"


using namespace cv;


EyeTrackerWindow::EyeTrackerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EyeTrackerWindow)
{

    ui->setupUi(this);

    ui->label_message->setText(QString("Messages:\n"));    

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



    //register Iplimage to use slot/signal with Qt
    qRegisterMetaType<IplImage>("IplImage");

    //connect(&ebxMonitorThread, SIGNAL(started()),ebxMonitorWorker,SLOT(searchMatch()));

    connect(this, SIGNAL(sampleEbxMonitor()), ebxMonitorWorker, SLOT(searchMatch()));
    connect(this, SIGNAL(setNewFrameNumberOffset(uint)), ebxMonitorWorker, SLOT(setNewFrameNumberOffset(uint)));

    connect(ebxMonitorWorker, SIGNAL(reportInitialTimestamp(QString)), this, SLOT(reportInitialTimestamp(QString)));
    connect(ebxMonitorWorker, SIGNAL(reportMeasurementPoint(QString)), this, SLOT(reportMeasurementPoint(QString)));
    connect(ebxMonitorWorker, SIGNAL(reportEnd(int)), this, SLOT(reportEnd(int)));
    connect(ebxMonitorWorker, SIGNAL(done()),this, SLOT(enableInterception()));
    connect(ebxMonitorWorker, SIGNAL(finished()),this, SLOT(deleteLater()));
    connect(ebxMonitorWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));

    connect(this, SIGNAL(gotNewFrame(qint64,int)), ebxMonitorWorker,SLOT(gotNewFrame(qint64,int)));       

    /** Signals to stop the workers. The stop slot may be only hit when thread.wait() is called.*/
    connect(this, SIGNAL(stopEbxMonitor()),ebxMonitorWorker, SLOT(stopMonitoring()));

    /** This is absolutely needed. Informs the Thread that the work is done. **/
    connect(&captureWorker, SIGNAL(finished()), &captureThread, SLOT(quit()));
    connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerThread, SLOT(quit()));

    connect(&captureThread, SIGNAL(started()), &captureWorker, SLOT(process()));
    connect(&eyetrackerWorker, SIGNAL(eyeFound(int,int)), &captureWorker, SLOT(setCenter(int,int)));
    connect(&captureWorker, SIGNAL(qimageCaptured(QImage)), this, SLOT(onCaptured(QImage)));

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

    /**
     * Separation is not really needed but for the moment there is no reason to remove it.
     * ater this could be merged into one slot.
     */
    connect(&captureWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));
    connect(&eyetrackerWorker, SIGNAL(message(QString)), this, SLOT(onTrackerMessage(QString)));

    connect(ui->quitBtn,SIGNAL(pressed()),this,SLOT(onClosed()));
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



void EyeTrackerWindow::onCaptured(QImage captFrame)
{       
    updateImage(QPixmap::fromImage(captFrame));
}

void EyeTrackerWindow::onCaptureMessage(QString msg)
{   
//    if (!msg.endsWith("\n"))
//    {
//        msg.append("\n");
//    }
    ui->label_message->append(msg);

}

void EyeTrackerWindow::onTrackerMessage(QString msg)
{
    onCaptureMessage(msg);
}


void EyeTrackerWindow::onClosed()
{
    /**
      * Avoid pending interception loops
      */
    ui->chk_interception->setChecked(false);
    /**
     * Avoid delay by stopping preview
     * Would be nicer with signals but ok...
     * */
    togglePreview();
    /**
     * Avoid delay by stopping eyetracker
     * Would be nicer with signals but ok...
     * */
    toggleProcessing();

    /**
     * Cleanup EbxMonitorWorker
     */    
    emit stopEbxMonitor();
    ebxMonitorThread.wait();   
    cleanEbxMonitorTree();

    /**
     * Stop eyetracker and capture
     */
    // eyeTrackerthread must be stopped before captureThread to be sure
    // that it is aborted. Otherwise captureThread can be stopped without
    // sending the last onImageCaptured signal. Once the eyeTrackerThread
    // is lost, the signal/slot connection is lost between
    // capture and eyetracker threads and nothing happens.
    eyetrackerWorker.abortThread();
    eyetrackerThread.wait();
    captureWorker.stopCapturing();
    captureThread.wait();

    this->close();
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
    //static int count = INTERCEPTION_FRAME_COUNT;
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

    //if ((count++) > INTERCEPTION_FRAME_COUNT){
        emit gotNewFrame(id, 255);
    //    count = 0;
    //}
}



void EyeTrackerWindow::enableInterception()
{
    if(ui->chk_interception->isChecked())
    {
        emit sampleEbxMonitor();
    }
    else
    {
        ui->btn_intercept->setEnabled(true);
    }
}

void EyeTrackerWindow::on_btn_intercept_pressed()
{
    ui->btn_intercept->setEnabled(false);
    cleanEbxMonitorTree();
    emit sampleEbxMonitor();
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
        QStandardItem * first  = createdStandardItems.at(0);
        if((first != 0) && (first->rowCount() > 0))
        {
            first->removeRows(0,first->rowCount());
        }
        if((first != 0) && (first->columnCount() > 0))
        {
            first->removeRows(0,first->columnCount());
        }
        createdStandardItems.removeAt(0);
    }
}

void EyeTrackerWindow::setupEbxMonitorTree()
{
    ebxMonitorModel = new QStandardItemModel(this);
    ui->ebxMonitorTree->setModel(ebxMonitorModel);
    ui->ebxMonitorTree->show();
}

void EyeTrackerWindow::reportInitialTimestamp(QString csvData)
{
    cleanEbxMonitorTree();
    setupEbxMonitorTree();
    reportMeasurementPoint(csvData);
    for(int i = 0 ; i < 5 ;i++)
    {
        ui->ebxMonitorTree->resizeColumnToContents(i);
    }
}

void EyeTrackerWindow::reportMeasurementPoint(QString csvData)
{
    QList<QStandardItem *> rowItems;
    foreach (QString str, csvData.split(","))
    {
        QStandardItem * tmpItem = new QStandardItem(str);
        createdStandardItems << tmpItem;
        rowItems << tmpItem;
    }
    ebxMonitorModel->invisibleRootItem()->appendRow(rowItems);
}

void EyeTrackerWindow::reportEnd(int count)
{
    if((ui->chk_stop_interception->isChecked()) && (count >= ui->nbr_stop_interception->value()))
    {
        emit ui->chk_interception->setChecked(false);
    }
    for(int i = 0 ; i < 5 ;i++)
    {
        ui->ebxMonitorTree->resizeColumnToContents(i);
    }
}

void EyeTrackerWindow::on_ommitFrames_valueChanged(int value)
{
    emit setNewFrameNumberOffset(value);
}
