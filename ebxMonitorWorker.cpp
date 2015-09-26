#include "ebxMonitorWorker.h"
#include <stdio.h>
#include <stdlib.h>
#include <QDebug>


EbxMonitorWorker::EbxMonitorWorker(QObject *parent) : QObject(parent)
{
    triggerActive = 0;
    delayAfterEbxMonitorReset = 0;
    //connect(this,SIGNAL(searchMatchingTimestamps(Timestamp)), this, SLOT(findMatchingTimestamps(Timestamp)));
    connect(&mytimer,SIGNAL(timeout()),this,SLOT(activateTrigger()));
}

void EbxMonitorWorker::searchMatch(){
    //int tmpDelay = delayAfterEbxMonitorReset;
    triggerActive = 0;
    cleanupMemory();
    flushOldMeasurementData();
    mytimer.start(500);
    fetchAndParseMeasurementData();

}

void EbxMonitorWorker::stopMonitoring()
{
    stop = 1;
}

void EbxMonitorWorker::setNewEnqueueingDelay(unsigned int delay)
{
    delayAfterEbxMonitorReset = delay;
}

void EbxMonitorWorker::flushOldMeasurementData()
{  
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
            while(!in.atEnd() && (!stop))
            {
                QString temp = in.readAll();
                lines.append(temp.split(QString("\n")));
            }
            file.close();
        }
        fclose(fd);        
    }
}

void EbxMonitorWorker::cleanupMemory()
{
    if(!listOfTimestamps.isEmpty())
    {
        foreach (Timestamp * item , listOfTimestamps)
        {
            delete item;
        }
        listOfTimestamps.clear();
    }
    if(!matchingTableFromFInalId.isEmpty())
    {
        matchingTableFromFInalId.clear();
    }
}

void EbxMonitorWorker::fetchAndParseMeasurementData()
{
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
            QString temp;
            while(!in.atEnd() && (!stop))
            {
                temp.append(in.readAll());
            }
            file.close();
            QList<QString> lines = temp.split(QString("\n"));
            storeMeasurementData(lines);
        }
        fclose(fd);
    }
    else
    {
        emit message("The filehandel for the ebx_monitor file could not be gathered.");        
    }    
}

bool toAscending( Timestamp * t1 , Timestamp * t2 )
{
    return t1->getPosition() < t2->getPosition();
}

void EbxMonitorWorker::storeMeasurementData(QList<QString> lines)
{
    foreach (QString line , lines)
    {
        QString data = line;
        /**
          * Bugfix to avoid empty timestamp if data is empty
          */
        if (line.count()> 6)
        {


            Timestamp * newTimestamp = new Timestamp(this, data);
            /**
              * Detect if the timestamp has been updated in uvc_video.c	@ uvc_video_clock_update.
              * Decision is based on Measurepoint 10 and 11 (see Timestamp class)
              * if yes then add the new timestamp to the matching table.
              **/
            if (!listOfTimestamps.isEmpty())
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
                /** Avoid long waiting if appication shall stop **/
                if(stop)
                {
                    break;
                }
            }
            listOfTimestamps << newTimestamp;

        }
    }
}

void EbxMonitorWorker::activateTrigger()
{
    mytimer.stop();
    triggerActive = 1;
    nmbrOfIgnoredFrames  = delayAfterEbxMonitorReset;
}

void EbxMonitorWorker::findMatchingTimestamps(Timestamp * criteria)
{
    if ((0 != criteria) && (!listOfTimestamps.isEmpty()) && ((*listOfTimestamps.first()) < (*criteria)))
    {
        bool foundAtLeastOneMatchingElement = false;
        results.clear();
        results <<  criteria;

        foreach (Timestamp * match, matchingTableFromFInalId) {
            if ((*match)  == (*criteria))
            {
                criteria->setAlternativeId(match->getAlternativeId());
                foundAtLeastOneMatchingElement = true;
            }
        }

        if (foundAtLeastOneMatchingElement)
        {
            emit message(QString("\nMatch found for %1!").arg(criteria->getId()));
            qDebug() << "Comparision for timestamp: " << criteria->getId() << " Alternative id: " << criteria->getAlternativeId();
            qDebug() << "Not matching elements:";
            qDebug() << "ID, Timestamp, Position, Alternative ID";

            foreach (Timestamp * matchingCandidate, listOfTimestamps) {
                if (((*matchingCandidate) == (*criteria)) || ((*criteria) == (*matchingCandidate)))
                {
                    results <<  matchingCandidate;
                }
                else
                {
                    qDebug() << matchingCandidate->getId() << ", "
                             << matchingCandidate->getTimestamp() << ", "
                             << matchingCandidate->getPosition() << ", "
                             << matchingCandidate->getAlternativeId();
                }
            }
            qSort(results.begin(),results.end(),toAscending);
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
            emit done();

        }
        else
        {
            emit message(QString("\nNo match found\nEBX-FIRST\t[%1]\t#[%2]\nQUE\t\t[%3]\tdT[%4ms]\nEBX-LAST\t\t[%5]")
                         .arg(listOfTimestamps.first()->getTimestamp())
                         .arg(listOfTimestamps.count())
                         .arg(criteria->getTimestamp())
                         .arg(criteria->getDelayInMs(listOfTimestamps.first()))
                         .arg(listOfTimestamps.last()->getTimestamp())
                         );
            if((*criteria) < (*listOfTimestamps.last()))
            {
                //triggerActive.store(1);
            }
        }
        delete criteria;
    }
}


void EbxMonitorWorker::gotNewFrame(qint64 frameId,int measurementPosition){
    if(nmbrOfIgnoredFrames <= 0)
    {
        if (triggerActive.testAndSetAcquire(1,0))
        {
            struct timespec gotTime;
            clock_gettime(CLOCK_MONOTONIC, &gotTime);
            qint64 rxTimeStamp = (((qint64)gotTime.tv_sec) * 1000000 + ((qint64)gotTime.tv_nsec)/1000);
            findMatchingTimestamps(new Timestamp(this,QString("%1us, %2us, %3").arg(frameId)
                                                    .arg(rxTimeStamp)
                                                    .arg(measurementPosition)));
        }

    }
    else
    {
        nmbrOfIgnoredFrames--;
    }
}

EbxMonitorWorker::~EbxMonitorWorker()
{
    cleanupMemory();
}

