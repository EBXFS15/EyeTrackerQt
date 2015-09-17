#include "ebxMonitorWorker.h"

EbxMonitorWorker::EbxMonitorWorker(QObject *parent) : QObject(parent)
{
}

void EbxMonitorWorker::sarchMatch(){
    remainingLoops = 2;
    flushOldMeasurementData();
    fetchAndParseMeasurementData();
}

void EbxMonitorWorker::stop()
{
    remainingLoops = 0;
}

void EbxMonitorWorker::storeMeasurementData(QList<QString> lines)
{
    foreach (QString line , lines)
    {
        QString data = line;
        Timestamp * tmpTimestamp = new Timestamp(this, data);
        /**
          * Detect if the timestamp has been updated in uvc_video.c	@ uvc_video_clock_update.
          * if yes then add the new timestamp to the matching table.
          **/
        if (listOfTimestamps.count() > 0)
        {
            if (tmpTimestamp->isRelated(listOfTimestamps.last()))
            {
                tmpTimestamp->setAlternativeId(listOfTimestamps.last()->getId());
                listOfTimestamps.last()->setAlternativeId(tmpTimestamp->getId());
                matchingTable << tmpTimestamp;
            }
        }
        listOfTimestamps << tmpTimestamp;
    }
    qSort(listOfTimestamps);
}

void EbxMonitorWorker::findMatchingTimestamps()
{
    bool foundAtLeastOneMatchingElement = false;
    while((!receivedFrames.isEmpty()) && (remainingLoops-- > 0) && (!foundAtLeastOneMatchingElement))
    {

        Timestamp * earliestReceivedFrame = receivedFrames.dequeue();
        if(earliestReceivedFrame != 0)
        {
            foreach (Timestamp * matchingCandidate, matchingTable) {
                if ((*matchingCandidate)  == (*earliestReceivedFrame))
                {
                    earliestReceivedFrame->setAlternativeId(matchingCandidate->getAlternativeId());
                    foundAtLeastOneMatchingElement = true;
                    break;
                }
            }

            if (foundAtLeastOneMatchingElement)
            {
                Timestamp * tmpTimestamp = 0;
                int count = 0;

                QList<QString> data = earliestReceivedFrame->prepareRow(tmpTimestamp);

                emit reportInitialTimestamp(data);

                foreach (Timestamp * matchingCandidate, listOfTimestamps) {
                    if ((*matchingCandidate) == (*earliestReceivedFrame))
                    {
                        QList<QString> data = matchingCandidate->prepareRow(tmpTimestamp);
                        emit reportMeasurementPoint(data);

                        tmpTimestamp = matchingCandidate;
                        emit message(QString("Timestamp [%1] matches with %2 measurement points.").arg(earliestReceivedFrame->getId())
                                                                                                  .arg(count++));
                    }
                }
            }
            else
            {
                emit message(QString("No matching timestamp found"));
            }
            delete earliestReceivedFrame;
        }
    }
    if (!foundAtLeastOneMatchingElement)
    {
        emit message(QString("No matching timestamp found. Probably no new frame got yet."));
    }
}

void EbxMonitorWorker::flushOldMeasurementData()
{
    foreach (Timestamp * item , listOfTimestamps)
    {
        delete item;
    }
    listOfTimestamps.clear();
    receivedFrames.clear();
    matchingTable.clear();

    while(!receivedFrames.isEmpty())
    {
        delete receivedFrames.dequeue();
    }
    FILE* fd = fopen(EBX_DEVICE_PATH, "r"); // get a file descriptor somehow
    if (fd!=0)
    {
        QFile file;

        if (!file.open(fd, QIODevice::ReadOnly))
        {
            emit message("The QFile could not open the ebx_monitor file.");
        }
        else
        {
            QTextStream in(&file);
            while(!in.atEnd() && (remainingLoops-- > 0))
            {
                in.readAll().split(QString("\n"));
            }
            file.close();

        }
        fclose(fd);
        emit finished();
    }
    else
    {
        emit message("The filehandel for the ebx_monitor file could not be gathered.");
        emit finished();
    }
}

void EbxMonitorWorker::fetchAndParseMeasurementData(){
    FILE* fd = fopen(EBX_DEVICE_PATH, "r"); // get a file descriptor somehow
    if (fd!=0)
    {
        QFile file;

        if (!file.open(fd, QIODevice::ReadOnly))
        {
            emit message("The QFile could not open the ebx_monitor file.");
        }
        else
        {
            QTextStream in(&file);
            while(!in.atEnd() && (remainingLoops-- > 0))
            {
                QList<QString> lines = in.readAll().split(QString("\n"));
                storeMeasurementData(lines);
                matchingIsPending = 1;
            }
            file.close();

        }
        fclose(fd);
        emit finished();
    }
    else
    {
        emit message("The filehandel for the ebx_monitor file could not be gathered.");
        emit finished();
    }
}

/**
 * @brief EbxMonitorWorker::gotNewFrame enques a timestamp in order to allow post processing.
 * @param frameId is the identifier of the frame. This identifier will be used to find matches within the ebx_monitor data.
 * @param measurementPosition is the measurement position within the application.
 */
void EbxMonitorWorker::gotNewFrame(qint64 frameId,int measurementPosition){
    struct timespec gotTime;    
    clock_gettime(CLOCK_MONOTONIC, &gotTime);
    qint64 rxTimeStamp = (((qint64)gotTime.tv_sec) * 1000000 + ((qint64)gotTime.tv_nsec)/1000);
    receivedFrames.enqueue(new Timestamp(this, QString("%1us, %2us, %3").arg(frameId)
                                                                        .arg(rxTimeStamp)
                                                                        .arg(measurementPosition)));
    if(matchingIsPending){
        findMatchingTimestamps();
        matchingIsPending = 0;
    }
}

EbxMonitorWorker::~EbxMonitorWorker()
{
    flushOldMeasurementData();
}

