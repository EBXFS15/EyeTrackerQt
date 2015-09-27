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
#define EBX_CMD_START "start"
#define EBX_READ_BUFFER (50 + 2)

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
    QAtomicInt          nbrOfFramesToIgnoreDefault;
    QAtomicInt          nbrOfFramesToIgnore;
    QTimer              mytimer;



    /**
     * @brief By definition all newly created Timestamps shall be added to listOfTimestamps. All other lists just use pointers to these objects. During clean up these references get invalidated and all objects deleted.
     */
    QList<Timestamp *>  listOfTimestamps;
    QList<Timestamp *>  matchingTableFromFInalId;
    QList<Timestamp *>  results;

    void storeMeasurementData(QList<QString> lines);
    void cleanupMemory();

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
    void sendCmdToEbxMonitor(QString cmd);
    void fetchAndParseMeasurementData();
    void searchMatch();
    void stopMonitoring();
    void setNewFrameNumberOffset(unsigned int nmrOfFrames);
    void findMatchingTimestamps(Timestamp * criteria);
    void activateTrigger();
};

#endif // EBXMONITORWORKER_H

