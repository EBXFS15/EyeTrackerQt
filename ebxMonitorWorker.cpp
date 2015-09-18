#include "ebxMonitorWorker.h"


EbxMonitorWorker::EbxMonitorWorker(QObject *parent) : QObject(parent)
{
    delayUntilEngueueing = 10;


    connect(&mytimer,SIGNAL(timeout()),this,SLOT(fetchAndParseMeasurementData()));

}

void EbxMonitorWorker::sarchMatch(){    
    flushOldMeasurementData();    
}

void EbxMonitorWorker::stop()
{
    enqueNewFrames = 0;
}

void EbxMonitorWorker::setNewEnqueueingDelay(unsigned int delay)
{
    delayUntilEngueueing = delay;
}

void EbxMonitorWorker::flushOldMeasurementData()
{
    enqueNewFrames = 0;

    foreach (Timestamp * item , listOfTimestamps)
    {
        delete item;
    }
    listOfTimestamps.clear();
    enqueuedFrameTimestamps.clear();
    matchingTableFromFInalId.clear();
    while(!enqueuedFrameTimestamps.isEmpty())
    {
        delete enqueuedFrameTimestamps.dequeue();
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
            while(!in.atEnd())
            {
                in.readAll().split(QString("\n"));
            }
            file.close();
        }
        fclose(fd);
    }
    //QThread::usleep(delayUntilEngueuing);
    enqueNewFrames = 1;
    mytimer.start(delayUntilEngueueing);
}

void EbxMonitorWorker::fetchAndParseMeasurementData(){
    mytimer.stop();
    enqueNewFrames = 0;
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
            QList<QString> lines;
            while(!in.atEnd())
            {
                lines = in.readAll().split(QString("\n"));
                storeMeasurementData(lines);
            }
            file.close();
            findMatchingTimestamps();
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


void EbxMonitorWorker::storeMeasurementData(QList<QString> lines)
{
    foreach (QString line , lines)
    {
        QString data = line;
        Timestamp * newTimestamp = new Timestamp(this, data);
        /**
          * Detect if the timestamp has been updated in uvc_video.c	@ uvc_video_clock_update.
          * Decision is based on Measurepoint 10 and 11 (see Timestamp class)
          * if yes then add the new timestamp to the matching table.
          **/
        if (listOfTimestamps.count() > 0)
        {
            if (newTimestamp->isRelated(listOfTimestamps.last()))
            {
                /**
                  * Timestamp 10 is before modification
                  * Timestamp 11 is after modification
                  *
                  * By setting in the Timestamp 11 the alternative ID it will be possible to use Timestamp 11 to match.
                  */
                newTimestamp->setAlternativeId(listOfTimestamps.last()->getId());
                listOfTimestamps.last()->setAlternativeId(listOfTimestamps.last()->getId());
                matchingTableFromFInalId << newTimestamp;
            }
        }
        listOfTimestamps << newTimestamp;
    }
    qSort(listOfTimestamps);
}

/**
 * @brief EbxMonitorWorker::findMatchingTimestamps
 * Takes one of the recently enqued frames and checkes if a measurment point can be found in the matching elements.
 * attention: dequed elements have to be deleted therefore they are temporarely stored in the dequeuedElements list.
 */
void EbxMonitorWorker::findMatchingTimestamps()
{
    QList<Timestamp *> dequeuedElements;
    bool foundAtLeastOneMatchingElement = false;    
    results.clear();
    /**
      * If many a huge ammount of samples have to be processed then it may be better to add a stop variable.
      */
    while((!enqueuedFrameTimestamps.isEmpty()) && (!foundAtLeastOneMatchingElement))
    {
        Timestamp * oldestEnqueuedFrame = enqueuedFrameTimestamps.dequeue();
        if(oldestEnqueuedFrame != 0)
        {
            dequeuedElements << oldestEnqueuedFrame;
            emit message(QString("EBX-FIRST\t[%1]\t#[%2]\nQUE\t\t[%3]\t#[%4]\tdT[%5ms]\nEBX-LAST\t[%6]")
                         .arg(listOfTimestamps.first()->getTimestamp())
                         .arg(listOfTimestamps.count())
                         .arg(oldestEnqueuedFrame->getTimestamp())
                         .arg(enqueuedFrameTimestamps.count())
                         .arg(oldestEnqueuedFrame->getDelayInMs(listOfTimestamps.first()))
                         .arg(listOfTimestamps.last()->getTimestamp())
                         );

            results <<  oldestEnqueuedFrame;

            foreach (Timestamp * match, matchingTableFromFInalId) {
                if ((*match)  == (*oldestEnqueuedFrame))
                {
                    oldestEnqueuedFrame->setAlternativeId(match->getAlternativeId());
                    results <<  match;
                    foundAtLeastOneMatchingElement = true;
                }
            }

            if (foundAtLeastOneMatchingElement)
            {
                foreach (Timestamp * matchingCandidate, listOfTimestamps) {
                    if ((*matchingCandidate) == (*oldestEnqueuedFrame))
                    {
                        results <<  matchingCandidate;
                    }
                }

            }
        }
    }

    if (!foundAtLeastOneMatchingElement)
    {
        emit message(QString("No match found."));
    }    
    else
    {
        qSort(results);
        Timestamp * previousTimestamp = 0;
        int count = 0;
        foreach(Timestamp * tmpTimestamp, results)
        {
            if(0 == count++)
            {
                emit reportInitialTimestamp(tmpTimestamp->toString(previousTimestamp));
            }
            else
            {
                emit reportMeasurementPoint(tmpTimestamp->toString(previousTimestamp));
            }

            previousTimestamp = tmpTimestamp;
        }

        emit reportEnd(count);
        foreach(Timestamp * tmp, dequeuedElements)
        {
            delete tmp;
        }
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

    if(enqueNewFrames > 0)
    {
        enqueuedFrameTimestamps.enqueue(new Timestamp(this, QString("%1us, %2us, %3").arg(frameId)
                                                                            .arg(rxTimeStamp)
                                                                            .arg(measurementPosition)));
    }
}

EbxMonitorWorker::~EbxMonitorWorker()
{
    flushOldMeasurementData();
}

