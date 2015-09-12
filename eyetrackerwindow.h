#ifndef EYETRACKERWINDOW_H
#define EYETRACKERWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QStandardItem>
#include "capturethread.h"
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include "time.h"
#include <sys/time.h>

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
    void addTimestamp(qint64 timestamp);

    QList<QStandardItem *> prepareRow(const QString &line);
    long long getFromRowItem(QList<QStandardItem *> rowItems, int position);
    long long getTimestamp(QList<QStandardItem *> rowItems);
    long long getId(QList<QStandardItem *> rowItems);
    long long getDelta(QList<QStandardItem *> rowItems);
    long long getDelta(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);
    double getDeltaInMs(QList<QStandardItem *> rowItems);
    double getDeltaInMs(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);
    void readEbxMonitor();


    QPixmap pixmap;
    CaptureThread captureThread;
    QStandardItemModel *frameMonitor;

public slots:
    void onCaptured(QImage image,qint64 timestamp);
    void gotFrame(qint64 timestamp);
    void onClosed();

private:
    Ui::EyeTrackerWindow *ui;
};

#endif // EYETRACKERWINDOW_H
