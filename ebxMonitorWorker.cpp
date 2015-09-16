#include "ebxMonitorWorker.h"

EbxMonitorWorker::EbxMonitorWorker(QObject *parent, QStandardItemModel * model, QTreeView * treeView) : QObject(parent)
{
    this->model = model;
    this->treeView = treeView;
    treeViewMutex.unlock();
    this->model->setColumnCount(5);
    this->model->invisibleRootItem()->setEditable(false);
}

void EbxMonitorWorker::startGrabbing(){
    stopMonitor = false;
    connect(&timer, SIGNAL (timeout()), this, SLOT (grabfromdriver()));
    timer.start(TIMER_DELAY);
}


void EbxMonitorWorker::grabfromdriver()
{
    timer.stop();
    if(0 == stopMonitor)
    {
        updateEbxMonitorData(-1);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        grabfromdriver();
    }
    else
    {        
        emit finished();
    }
}

void EbxMonitorWorker::stop()
{
    stopMonitor = 1;
}

void EbxMonitorWorker::updateEbxMonitorData(qint64 id){
    if ((0 == model ) || 0 == treeView)
    {
        return;
    }

    QStandardItem *invisibleRootItem = model->invisibleRootItem();
    if (invisibleRootItem != 0)
    {
        FILE* fd = fopen("/dev/ebx_monitor", "r"); // get a file descriptor somehow
        if (fd!=0)
        {           
            QFile file;

            if (!file.open(fd, QIODevice::ReadOnly))
            {

            }
            else
            {
                QTextStream in(&file);
                //while(!in.atEnd()) {

                    while(!in.atEnd() && !stopMonitor)
                    {
                        //this->parent()->processEvents(QEventLoop::ExcludeUserInputEvents);
                        QString content = in.readAll();

                        QList<QString> lines = content.split(QString("\n"));
/*
                        foreach (QString line , lines)
                        {
                            // Only add items with the ID = 0 or
                            qint64 idOfLine = getId(line);
                            int position = getPosition(line);
                            if(idOfLine == id)
                            {
                                QList<QStandardItem *> * rowItems = new QList<QStandardItem *>();
                                if (prepareRow(rowItems, line)>0)
                                {
                                    appendRowItems(rowItems);
                                }
                                else
                                {
                                    delete rowItems;
                                    rowItems = 0;
                                }
                            }
                            else
                            {
                                QString searchPattern = QString("%1us").arg(idOfLine);
                                QModelIndexList match = invisibleRootItem->model()->match(
                                            invisibleRootItem->model()->index(0, 0),
                                            Qt::DisplayRole,
                                            QVariant::fromValue(searchPattern),
                                            1, // look *
                                            Qt::MatchRecursive);
                                if (match.count() > 0)
                                {
                                    model->blockSignals(true);
                                    QStandardItem * mainNode = model->itemFromIndex(match.first());
                                    QList<QStandardItem *> * rowItems = new QList<QStandardItem *>();
                                    if (prepareRow(rowItems, line)>0)
                                    {
                                        treeView->expand(mainNode->index());
                                        appendRowItems(rowItems);
                                    }
                                    else
                                    {
                                        delete rowItems;
                                        rowItems = 0;
                                    }
                                }
                            }

                        }*/
                    }
            file.close();
            fclose(fd);
        }
    }
    }
}


void EbxMonitorWorker::appendRowItems(QList<QStandardItem *> * rowItems){
    static QAtomicInt format = 1;
    if ((0 == model ) || 0 == treeView)
    {
        return;
    }
    QStandardItem *invisibleRootItem = model->invisibleRootItem();

    if ((invisibleRootItem != 0) && ((*rowItems).count() > 0))
    {
        // Limit size to 9 lines
        while(invisibleRootItem->rowCount() > 2)
        {
            //model->blockSignals(true);
            int count = model->item( 0 )->rowCount();
            while(count)
            {
                QList< QStandardItem* > items = model->item( 0 )->takeRow( 0 );
                qDeleteAll( items );
            }
            invisibleRootItem->removeRow(0);
            //model->blockSignals(false);
        }
        switch(getPosition(*rowItems))
        {
        /**
         * Add in any case as it is the last timestamp that is expected.
         **/
        case 255:

            invisibleRootItem->appendRow((*rowItems));

            break ;
        /**
         * Add only if a match is found
         **/
        default:
            QString searchPattern = rowItems->first()->text();
            QModelIndexList match = invisibleRootItem->model()->match(
                        invisibleRootItem->model()->index(0, 0),
                        Qt::DisplayRole,
                        QVariant::fromValue(searchPattern),
                        1, // look *
                        Qt::MatchRecursive);
            if (match.count() > 0)
            {
                model->blockSignals(true);
                QStandardItem * mainNode = model->itemFromIndex(match.first());

                /*if (mainNode->rowCount() > 0)
                {
                    model->blockSignals(true);
                    double delta = getDeltaInMs(rowItems, mainNode->takeRow(mainNode->rowCount()-1));
                    model->blockSignals(false);
                    rowItems << new QStandardItem(QString("%1ms").arg(delta));
                }*/

                model->blockSignals(true);
                mainNode->appendRow((*rowItems));
                treeView->expandAll();
                model->blockSignals(false);
            }
            else
            {
                /**
                  * OK no match found
                  **/
            }

            break;
        }
        /**
         * Do some formating if not already done.
         **/
        if (format)
        {
            model->blockSignals(true);
            this->treeView->setColumnWidth(0,180);
            this->treeView->setColumnWidth(1,180);
            this->treeView->setColumnWidth(2,45);
            this->treeView->setColumnWidth(3,100);
            model->blockSignals(false);
            format = 0;
        }
    }    
}

void EbxMonitorWorker::gotNewFrame(qint64 frameId){
    struct timespec gotTime;    
    clock_gettime(CLOCK_MONOTONIC, &gotTime);
    qint64 rxTimeStamp = (((qint64)gotTime.tv_sec) * 1000000 + ((qint64)gotTime.tv_nsec)/1000);
    QStandardItem *invisibleRootItem = model->invisibleRootItem();
    if (invisibleRootItem != 0)
    {
        QString line = QString("%1us, %2us, 255").arg(frameId).arg(rxTimeStamp);

        QList<QStandardItem *>  * rowItems = new QList<QStandardItem *>();
        if (prepareRow(rowItems,  line)>0)
        {

            appendRowItems(rowItems);
        }
        else{
            delete rowItems;
        }
    }
}

long long EbxMonitorWorker::getFromRowItem(QList<QStandardItem *> rowItems, int position){
    if (rowItems.count()>position)
    {

        long long tmpValue = rowItems.at(position)->text().split("us").first().toLongLong();
        return tmpValue;
    }
    else
    {
        return -1;
    }
}

int EbxMonitorWorker::getPosition(QList<QStandardItem *> rowItems){
    return getFromRowItem(rowItems, 2);
}

int EbxMonitorWorker::getPosition(QString line){
    const QStringList fields = line.split(",");
    if (fields.count()>2)
    {
        int tmpValue = fields.at(2).toInt();
        return tmpValue;
    }
    else
    {
        return -1;
    }
}

long long EbxMonitorWorker::getTimestamp(QList<QStandardItem *> rowItems){
    return getFromRowItem(rowItems, 1);
}

long long EbxMonitorWorker::getTimestamp(QString line)
{
    const QStringList fields = line.split(",");
    if (fields.count()>1)
    {
        long long tmpValue = fields.at(1).split("us").first().toLongLong();
        return tmpValue;
    }
    else
    {
        return -1;
    }
}


long long EbxMonitorWorker::getId(QList<QStandardItem *> rowItems){
    return getFromRowItem(rowItems, 0);
}

long long EbxMonitorWorker::getId(QString line)
{
    const QStringList fields = line.split(",");
    if (fields.count()>0)
    {
        QString str = fields.at(0).split("us").first();
        str = str.replace(QChar('\0'), QString(""));
        long long tmpValue = str.toLongLong();
        return tmpValue;
    }
    else
    {
        return -1;
    }
}


long long EbxMonitorWorker::getDelta(QList<QStandardItem *> rowItems){
    if (rowItems.count()>=2)
    {
        long long id = getId(rowItems);
        long long timestamp = getTimestamp(rowItems);
        return (timestamp - id);
    }
    else
    {
        return -1;
    }
}

long long EbxMonitorWorker::getDelta(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder){
    if (rowItemsNewer.count()>=2 && rowItemsOlder.count()>=2)
    {
        long long newerTimestamp = getTimestamp(rowItemsNewer);
        long long olderTimestamp = getTimestamp(rowItemsOlder);
        return (newerTimestamp -  olderTimestamp);
    }
    else
    {
        return -1;
    }
}

double EbxMonitorWorker::getDeltaInMs(QList<QStandardItem *> rowItems){
    if (rowItems.count()>=2)
    {
        return ((double)getDelta(rowItems))/1000;
    }
    else
    {
        return -1;
    }
}

double EbxMonitorWorker::getDeltaInMs(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder){
    if (rowItemsNewer.count()>=2 && rowItemsOlder.count()>=2)
    {
        return ((double)getDelta(rowItemsNewer, rowItemsOlder))/1000;
    }
    else
    {
        return -1;
    }
}

QStandardItem * EbxMonitorWorker::createRowItem(const QString data){
    QStandardItem * ret = new QStandardItem(data);
    createdItems << ret;
    return ret;
}

int EbxMonitorWorker::prepareRow(QList<QStandardItem *> * rowItems, const QString &line)
{  
    int ret = 0;
    const QStringList fields = line.split(",");
    if (fields.count() >=3){
        // Add ID
        if (fields.at(0).length()>2){
            QString str = fields.at(0);
            str = str.replace(QChar('\0'), QString(""));
            ret++;
            (*rowItems) << new QStandardItem(str);
        }
        else{
            ret++;
            (*rowItems)  << new QStandardItem(QString("?"));
        }
        // Add Timestamp
        if (fields.at(1).length()>2){
            ret++;
            (*rowItems)  << new QStandardItem(fields.at(1));
        }
        else{
            ret++;
            (*rowItems)  << new QStandardItem(QString("?"));
        }
        // Add Position TAG
        ret++;
        (*rowItems)  << new QStandardItem(fields.at(2));
        // Add delta in ms
        ret++;
        (*rowItems)  << new QStandardItem(QString("%1ms").arg(getDeltaInMs((*rowItems) )));
    }
    return ret;
}

EbxMonitorWorker::~EbxMonitorWorker()
{
    if (0 != this->model)
    {
        QStandardItem *invisibleRootItem = model->invisibleRootItem();

        if ((invisibleRootItem != 0))
        {
            // Limit size to 9 lines
            while(invisibleRootItem->rowCount())
            {
                //model->blockSignals(true);
                int count = model->item( 0 )->rowCount();
                while(count)
                {
                    QList< QStandardItem* > items = model->item( 0 )->takeRow( 0 );
                    qDeleteAll( items );
                }
                invisibleRootItem->removeRow(0);
                //model->blockSignals(false);
            }
        }
    }
    this->model = 0;
    this->treeView = 0;
    foreach (QStandardItem * tmpItem, createdItems)
    {
        delete tmpItem;
    }
}

