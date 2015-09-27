#ifndef EYETRACKERWINDOW_H
#define EYETRACKERWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QMetaType>
#include <QStandardItemModel>
#include <QPainter>
#include "captureWorker.h"
#include "eyetrackerWorker.h"
#include "ebxMonitorWorker.h"


#define INTERCEPTION_FRAME_COUNT 0

namespace Ui {
class EyeTrackerWindow;
}

class EyeTrackerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EyeTrackerWindow(QWidget *parent = 0);
    ~EyeTrackerWindow();
    void updateImage(QPixmap pixmap);
    void getImage();    
    void cleanEbxMonitorTree();
    void setupEbxMonitorTree();

    QImage captFrame;
    QPixmap pixmap;
    CaptureWorker captureWorker;
    EyeTrackerWorker eyetrackerWorker;    
    QThread captureThread;
    QThread eyetrackerThread;    
    QThread ebxMonitorThread;

    EbxMonitorWorker  * ebxMonitorWorker;

    QStandardItemModel *ebxMonitorModel;
    QList<QStandardItem * > createdStandardItems;



signals:
    void gotNewFrame(qint64 frameId, int position);
    void sampleEbxMonitor();
    void stopEbxMonitor();
    void stopEbxCaptureWorker();
    void stopEbxEyeTracker();
    void setCenter(int x, int y);
    void setNewFrameNumberOffset(unsigned int delay);

public slots:
    void onCaptured(QImage frame);
    void onClosed();
    void onCaptureMessage(QString msg);
    void onTrackerMessage(QString msg);    
    void onGotFrame(qint64 id);
    void togglePreview();
    void toggleProcessing();
    void enableInterception();
    void reportInitialTimestamp(QString csvData);
    void reportMeasurementPoint(QString csvData);
    void reportEnd(int count);


private slots:


    void on_btn_intercept_pressed();

    void on_scr_delay_enqueuing_valueChanged(int value);

private:
    Ui::EyeTrackerWindow *ui;

};

#endif // EYETRACKERWINDOW_H
