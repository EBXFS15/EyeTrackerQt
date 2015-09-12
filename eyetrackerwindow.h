#ifndef EYETRACKERWINDOW_H
#define EYETRACKERWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QMetaType>
#include <QStandardItemModel>
#include "captureWorker.h"
#include "eyetrackerWorker.h"

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


public slots:
    void onCaptured(QImage frame, double timestamp);
    void onClosed();
    void onCaptureMessage(QString msg);
    void onTrackerMessage(QString msg);

private:
    Ui::EyeTrackerWindow *ui;
    double timestamp;
};

#endif // EYETRACKERWINDOW_H
