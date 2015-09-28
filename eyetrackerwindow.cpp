#include "eyetrackerwindow.h"
#include "ui_eyetrackerwindow.h"


using namespace cv;


EyeTrackerWindow::EyeTrackerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EyeTrackerWindow)
{   
    ui->setupUi(this);
    searching = 0;

    ui->matchingStatus->setScene(&scene);

    ui->label_message->setText(QString("Messages:\n"));    
    on_btn_disable_processing_clicked();
    on_btn_disable_preview_clicked();

    /** ebxMonitor setup
     * The ebxMonitor waits for a trigger to start parsing and matching timestamps.
     * During parsing all matching timestamps are reported through the respective signals
     * As soon one timestamp has been found the finisched signal is sent.
     * This signal is used to enable the intercept button on the gui.
     *
     * In addition to the parsing the ebxMonitorWorker collects the timestamps from the top level aplication.
     */   
    ebxMonitorWorker = new EbxMonitorWorker();
    protectMonitorTree.unlock();

    setupEbxMonitorTree();
    ui->ommitFrames->setSingleStep(INTERCEPTION_FRAME_COUNT);
    ui->ommitFrames->setMaximum(INTERCEPTION_FRAME_COUNT * 10);


    //register Iplimage to use slot/signal with Qt
    qRegisterMetaType<IplImage>("IplImage");


    connect(this, SIGNAL(sampleEbxMonitor()), ebxMonitorWorker, SLOT(searchMatch()));
    connect(this, SIGNAL(setNewFrameNumberOffset(uint)), ebxMonitorWorker, SLOT(setNewFrameNumberOffset(uint)));

    connect(ebxMonitorWorker, SIGNAL(reportInitialTimestamp(QString)), this, SLOT(reportInitialTimestamp(QString)));
    connect(ebxMonitorWorker, SIGNAL(reportMeasurementPoint(QString)), this, SLOT(reportMeasurementPoint(QString)));
    connect(ebxMonitorWorker, SIGNAL(reportEnd(int)), this, SLOT(reportEnd(int)));

    connect(ebxMonitorWorker, SIGNAL(done(qint64,qint64,qint64,bool)),this, SLOT(matchStatus(qint64,qint64,qint64,bool)));

    connect(ebxMonitorWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));

    connect(ebxMonitorWorker, SIGNAL(startReadingFromEbxMonitor()),this,SLOT(progressBarStart()));
//    connect(ebxMonitorWorker, SIGNAL(readingFromEbxMonitor()),this,SLOT(progressBarIncrement()));

    connect(this, SIGNAL(gotNewFrame(qint64,int)), ebxMonitorWorker,SLOT(gotNewFrame(qint64,int)));       
    connect(this, SIGNAL(gotNewFrame(qint64,qint64, int)), ebxMonitorWorker,SLOT(gotNewFrame(qint64,qint64,int)));

    /** Signals to stop the workers. The stop slot may be only hit when thread.wait() is called.*/
    connect(this, SIGNAL(stopEbxMonitor()),ebxMonitorWorker, SLOT(stopMonitoring()));
    connect(this, SIGNAL(stopEbxCaptureWorker()),&captureWorker, SLOT(stop_capturing()));
    connect(this, SIGNAL(stopEbxEyeTracker()), &eyetrackerWorker, SLOT(abortThread()));

    /** This is absolutely needed. Informs the Thread that the work is done. **/
    connect(&captureWorker, SIGNAL(finished()), &captureThread, SLOT(quit()));
    connect(&eyetrackerWorker, SIGNAL(finished()), &eyetrackerThread, SLOT(quit()));

    connect(&captureThread, SIGNAL(started()), &captureWorker, SLOT(process()));
    connect(&eyetrackerWorker, SIGNAL(eyeFound(int,int)), &captureWorker, SLOT(set_center(int,int)));
    connect(&captureWorker, SIGNAL(qimageCaptured(QImage)), this, SLOT(onCaptured(QImage)));


    connect(&captureWorker, SIGNAL(imageCaptured(IplImage)), &eyetrackerWorker, SLOT(onImageCaptured(IplImage)));

    /**
     * Separation is not really needed but for the moment there is no reason to remove it.
     * ater this could be merged into one slot.
     */
    connect(&captureWorker, SIGNAL(message(QString)), this, SLOT(onCaptureMessage(QString)));
    connect(&eyetrackerWorker, SIGNAL(message(QString)), this, SLOT(onTrackerMessage(QString)));



    connect(ui->quitBtn,SIGNAL(pressed()),this,SLOT(onClosed()));

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
    emit captureWorker.set_preview(false);
    /**
     * Avoid delay by stopping eyetracker
     * Would be nicer with signals but ok...
     * */
    emit eyetrackerWorker.set_processing(false);

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

    emit eyetrackerWorker.abortThread();
    eyetrackerThread.wait();
    captureWorker.stop_capturing();
    captureThread.wait();

    this->close();
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

    if ((count++) > INTERCEPTION_FRAME_COUNT){
        emit gotNewFrame(id, rxTimeStamp, 255);
        count = 0;
        progressBarIncrement();
    }
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
    setupEbxMonitorTree();
    progressBarStart();
    emit sampleEbxMonitor();
}

void EyeTrackerWindow::cleanEbxMonitorTree()
{    
    protectMonitorTree.lock();
    if(ebxMonitorModel != 0)
    {
        delete ebxMonitorModel;
        ebxMonitorModel = 0;
    }

    while(!createdStandardItems.isEmpty())
    {
        createdStandardItems.removeAt(0);
    }
    protectMonitorTree.unlock();
}

void EyeTrackerWindow::setupEbxMonitorTree()
{
    protectMonitorTree.lock();
        ebxMonitorModel = new QStandardItemModel(this);
    protectMonitorTree.unlock();
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
    protectMonitorTree.lock();
    ebxMonitorModel->invisibleRootItem()->appendRow(rowItems);
    protectMonitorTree.unlock();
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




void EyeTrackerWindow::matchStatus(qint64 ebxTStart, qint64 ebxTStop, qint64 currentTimestamp , bool match)
{
    static qint64 xorigin = 0;
    QBrush ebxBrush = QBrush(QColor("yellow"));
    QBrush brush = QBrush(QColor("red"));
    if(match)
    {
        brush = QBrush(QColor("green"));
    }

    if (xorigin < ebxTStart)
    {
        xorigin = ebxTStart -10;
    }

    ui->matchingStatus->scene()->clear();

    qint64 ebx= (ebxTStop-ebxTStart)/10000;
    qint64 criteria = (currentTimestamp - xorigin)/10000;

    ui->matchingStatus->scene()->addRect(0,3,ebx,12,QPen(Qt::NoPen),ebxBrush );
    //ui->matchingStatus->scene()->addRect(criteria,14,10,14,QPen(Qt::NoPen), brush);
    ui->matchingStatus->scene()->addEllipse(criteria,16,8,8,QPen(Qt::NoPen), brush);

    ui->matchingStatus->repaint();
    this->enableInterception();

}

void EyeTrackerWindow::on_btn_disable_preview_clicked()
{
    captureWorker.set_preview(toggleButton(ui->btn_disable_preview,
                                          "Enable\npreview",
                                          "Disable\npreview"));
}
void EyeTrackerWindow::on_btn_disable_processing_clicked()
{
    eyetrackerWorker.set_processing(toggleButton(ui->btn_disable_processing,
                                                "Enable\neye tracking",
                                                "Disable\neye tracking"));
}
bool EyeTrackerWindow::toggleButton(QPushButton * button, QString enabled, QString disabled)
{
    if(button->isFlat())
    {
        button->setFlat(false);
        button->setText(enabled);
        button->setStyleSheet(STYLE_DISABLED);
        return false;
    }
    else
    {
        button->setFlat(true);
        button->setText(disabled);
        button->setStyleSheet(STYLE_ENABLED);
        button->setAutoFillBackground(true);
        return true;
    }
}

void EyeTrackerWindow::progressBarStart()
{
    ui->pbar_reading->setValue(0);
}

void EyeTrackerWindow::progressBarIncrement()
{
    if((ui->pbar_reading->value() +1) > ui->pbar_reading->maximum())
    {
        ui->pbar_reading->setValue(0);
    }
    else
    {
        ui->pbar_reading->setValue(ui->pbar_reading->value()+1);
    }

}

