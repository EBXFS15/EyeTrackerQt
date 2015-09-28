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
#include "timestamp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <QProcess>
#include <QDebug>

#define EBX_DEVICE_PATH "/dev/ebx_monitor"
#define EBX_SETUP_SCRIPT_PATH "./setup.sh"
#define EBX_SETUP_SCRIPT_MEASUREMENT_POINTS "150"
#define EBX_CMD_START "start"
#define EBX_READ_BUFFER (50 + 2)
/**
  * Minimum length is exepcted to be "0us, 0us, 0"
  */
#define EBX_READ_LINE_MINIMUM_LEN 11

class EbxMonitorWorker : public QObject
{
    Q_OBJECT    
    QMutex ebxDataMutex;

private:
    QAtomicInt          stop;
    QAtomicInt          triggerActive;    
    QAtomicInt          enqueNewFrames;    
    QAtomicInt          nbrOfFramesToIgnoreDefault;
    QAtomicInt          nbrOfFramesToIgnore;
    QTimer              watchdog;

    /**
     * @brief By definition all newly created Timestamps shall be added to listOfTimestamps. All other lists just use pointers to these objects. During clean up these references get invalidated and all objects deleted.
     */
    QList<Timestamp *>  listOfTimestamps;
    QList<Timestamp *>  matchingTableFromFinalId;
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
    void done(qint64 ebxTStart, qint64 ebxTStop, qint64 currentTimestamp , bool match);
    void readingFromEbxMonitor();
    void startReadingFromEbxMonitor();

public slots:    
    void gotNewFrame(qint64 timestamp, int measurementPosition);
    void gotNewFrame(qint64 frameId, qint64 registrationTime,  int position);
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

