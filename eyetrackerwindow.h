#ifndef EYETRACKERWINDOW_H
#define EYETRACKERWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "captureWorker.h"

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
    QPixmap pixmap;
    CaptureWorker captureWorker;
    QThread captureThread;


public slots:
    void onCaptured(QImage image,double timestamp);
    void onClosed();
    void onMessage(QString msg);

private:
    Ui::EyeTrackerWindow *ui;
};

#endif // EYETRACKERWINDOW_H
