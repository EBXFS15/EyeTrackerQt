#ifndef EBXMONITORWORKER_H
#define EBXMONITORWORKER_H

#include <QObject>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QCoreApplication>
#include <QStandardItem>
#include <QTreeView>
#include <QThread>
#include <QTimer>
#include "time.h"
#include <sys/time.h>
#include <QMutex>
#include <QQueue>
#include <QProcess>
#include "timestamp.h"

#define EBX_DEVICE_PATH "/dev/ebx_monitor"

class EbxMonitorWorker : public QObject
{
    Q_OBJECT    
    QMutex accessData;

private:
    QAtomicInt          stop;
    QAtomicInt          triggerActive;
    QAtomicInt          matchingIsPending;
    QAtomicInt          enqueNewFrames;
    //QAtomicInt          matchingIsPending;
    QAtomicInt          delayAfterEbxMonitorReset;
    QAtomicInt          nmbrOfIgnoredFrames;
    QTimer              mytimer;



    /**
     * @brief listOfTimestamps is the main reference to the objects. This list is used to delete the objects.
     */
    QList<Timestamp *>  listOfTimestamps;
    QList<Timestamp *>  matchingTableFromFInalId;
    QList<Timestamp *>  results;
    void storeMeasurementData(QList<QString> lines);

public:
    explicit EbxMonitorWorker(QObject *parent = 0);
    ~EbxMonitorWorker();

signals:
    void finished();
    void reportInitialTimestamp(QString csvData);
    void reportMeasurementPoint(QString csvData);
    void reportEnd(int count);
    void message(QString msg);
    void continueToFetchAndParse();
    void searchMatchingTimestamps(Timestamp * criteria);
    void done();

public slots:    
    void gotNewFrame(qint64 timestamp, int measurementPosition);
    void flushOldMeasurementData();
    void fetchAndParseMeasurementData();
    void sarchMatch();
    void stopMonitoring();
    void setNewEnqueueingDelay(unsigned int delay);
    void findMatchingTimestamps(Timestamp * criteria);
    void activateTrigger();
};

#endif // EBXMONITORWORKER_H

