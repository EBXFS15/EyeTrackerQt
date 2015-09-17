#ifndef EBXMONITORWORKER_H
#define EBXMONITORWORKER_H

#include <QObject>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QStandardItem>
#include <QTreeView>
#include <QThread>
#include <QTimer>
#include "time.h"
#include <sys/time.h>
#include <QMutex>
#include <QQueue>
#include "timestamp.h"

#define EBX_DEVICE_PATH "/dev/ebx_monitor"

class EbxMonitorWorker : public QObject
{
    Q_OBJECT    
    QMutex isQueueEmpty;

private:
    QAtomicInt          remainingLoops;
    QAtomicInt          matchingIsPending;
    QQueue<Timestamp *> receivedFrames;
    QList<Timestamp *>  listOfTimestamps;
    QList<Timestamp *>  matchingTable;
    void storeMeasurementData(QList<QString> lines);
    void findMatchingTimestamps();

public:
    explicit EbxMonitorWorker(QObject *parent = 0);
    ~EbxMonitorWorker();

signals:
    void finished();
    void reportInitialTimestamp(QList<QString> data);
    void reportMeasurementPoint(QList<QString> data);
    void message(QString msg);

public slots:    
    void gotNewFrame(qint64 timestamp, int measurementPosition);
    void flushOldMeasurementData();
    void fetchAndParseMeasurementData();
    void sarchMatch();
    void stop();
};

#endif // EBXMONITORWORKER_H

