#ifndef EYETRACKERWINDOW_H
#define EYETRACKERWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "capturethread.h"

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
    CaptureThread captureThread;


public slots:
    void onCaptured(QImage image,double timestamp);
    void onClosed();

private:
    Ui::EyeTrackerWindow *ui;
};

#endif // EYETRACKERWINDOW_H
