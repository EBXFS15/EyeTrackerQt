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
    void addTimestamp(double timestamp);
    QImage captFrame;
    QPixmap pixmap;
    CaptureWorker captureWorker;
    EyeTrackerWorker eyetrackerWorker;    
    QThread captureThread;
    QThread eyetrackerThread;    

    EbxMonitorWorker  * ebxMonitorWorker;
    QThread ebxMonitorThread;
    QStandardItemModel *ebxMonitorModel;

signals:
    void gotNewFrame(qint64 frameId);
    void stopEbxMonitor();
    void stopEbxCaptureWorker();
    void stopEbxEyeTracker();

public slots:
    void onCaptured(QImage frame, double timestamp);
    void onClosed();
    void onCaptureMessage(QString msg);
    void onTrackerMessage(QString msg);
    void onEyeFound(int x, int y);
    void onGotFrame(qint64 id);
    void togglePreview();
    void toggleProcessing();

private slots:
    void on_pushButton_pressed();

private:
    Ui::EyeTrackerWindow *ui;
    double timestamp;
};

#endif // EYETRACKERWINDOW_H
