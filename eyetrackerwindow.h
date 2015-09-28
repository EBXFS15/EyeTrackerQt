#ifndef EYETRACKERWINDOW_H
#define EYETRACKERWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QMetaType>
#include <QStandardItemModel>
#include <QPainter>
#include <QMutex>
#include <QGraphicsScene>
#include "captureWorker.h"
#include "eyetrackerWorker.h"
#include "ebxMonitorWorker.h"


#define INTERCEPTION_FRAME_COUNT 8
#define STYLE_DISABLED "color: black;background-color: rgb(242, 241, 240);"
#define STYLE_ENABLED "color: green;background-color: rgb(218, 218, 218);"

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
    bool toggleButton(QPushButton * button, QString enabled, QString disabled);

    QImage captFrame;
    QAtomicInt searching;
    QPixmap pixmap;
    CaptureWorker captureWorker;
    EyeTrackerWorker eyetrackerWorker;    
    QThread captureThread;
    QThread eyetrackerThread;    
    QThread ebxMonitorThread;

    QGraphicsScene scene;

    EbxMonitorWorker  * ebxMonitorWorker;
    QMutex protectMonitorTree;
    QStandardItemModel *ebxMonitorModel;
    QList<QStandardItem * > createdStandardItems;




signals:
    void gotNewFrame(qint64 frameId, int position);
    void gotNewFrame(qint64 frameId, qint64 registrationTime,  int position);
    void sampleEbxMonitor();
    void stopEbxMonitor();
    void stopEbxCaptureWorker();
    void stopEbxEyeTracker();
    void setCenter(int x, int y);
    void setNewFrameNumberOffset(unsigned int delay);
    void cleanUpDone();

public slots:
    void onCaptured(QImage frame);
    void onClosed();
    void onCaptureMessage(QString msg);
    void onTrackerMessage(QString msg);    
    void onGotFrame(qint64 id);
    void enableInterception();
    void reportInitialTimestamp(QString csvData);
    void reportMeasurementPoint(QString csvData);
    void reportEnd(int count);
    void matchStatus(qint64 ebxTStart, qint64 ebxTStop, qint64 currentTimestamp , bool match);
    void progressBarIncrement();
    void progressBarStart();


private slots:
    void on_btn_intercept_pressed();
    void on_ommitFrames_valueChanged(int arg1);
    void on_btn_disable_processing_clicked();
    void on_btn_disable_preview_clicked();

private:
    Ui::EyeTrackerWindow *ui;

};

#endif // EYETRACKERWINDOW_H
