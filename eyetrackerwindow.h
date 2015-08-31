#ifndef EYETRACKERWINDOW_H
#define EYETRACKERWINDOW_H

#include <QMainWindow>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

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

    CvCapture* capture;
    QPixmap* pixmap;


private:
    Ui::EyeTrackerWindow *ui;
};

#endif // EYETRACKERWINDOW_H
