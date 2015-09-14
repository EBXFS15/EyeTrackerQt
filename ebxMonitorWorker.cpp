#include "ebxMonitorWorker.h"

EbxMonitorWorker::EbxMonitorWorker(QObject *parent, QStandardItemModel * model, QTreeView * treeView) : QObject(parent)
{
    this->model = model;
    this->treeView = treeView;
    treeViewMutex.unlock();
}

void EbxMonitorWorker::start(){
    stopMonitor = false;
    connect(&timer, SIGNAL (timeout()), this, SLOT (grabfromdriver()));
    timer.start(TIMER_DELAY);
}


void EbxMonitorWorker::grabfromdriver()
{
    timer.stop();
    if(!stopMonitor)
    {
        updateEbxMonitorData(-1);
        timer.start(TIMER_DELAY);
    }
    else
    {        
        emit finished();
    }
}

void EbxMonitorWorker::stop()
{
    stopMonitor = true;
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

                while(!in.atEnd()) {

                    QString content = in.readAll();
                    QList<QString> lines = content.split(QString("\n"));
                    foreach (QString line , lines)
                    {

                        QList<QStandardItem *>  rowItems = prepareRow(line);
                        qint64 idOfLine = getId(rowItems);
                        if(idOfLine == id || id == -1)
                        {
                            appendRowItems(rowItems);
                        }
                    }
                }
                file.close();
                fclose(fd);
            }
            if(invisibleRootItem->rowCount() >= 16){
                treeViewMutex.lock();
                invisibleRootItem->removeRows(0,invisibleRootItem->rowCount()-16);
                treeViewMutex.unlock();
            }

        }
    }


}


void EbxMonitorWorker::appendRowItems(QList<QStandardItem *> rowItems){
    if ((0 == model ) || 0 == treeView)
    {
        return;
    }

    QStandardItem *invisibleRootItem = model->invisibleRootItem();
    if (invisibleRootItem != 0)
    {
        if (rowItems.count() > 0)
        {
            //treeViewMutex.lock();
            QString searchPattern = rowItems.first()->text();
            QModelIndexList match = invisibleRootItem->model()->match(
                        invisibleRootItem->model()->index(0, 0),
                        Qt::DisplayRole,
                        QVariant::fromValue(searchPattern),
                        1, // look *
                        Qt::MatchRecursive);
            if (match.count() > 0)
            {
                QStandardItem * mainNode = model->itemFromIndex(match.first());

                if (mainNode->rowCount() > 0)
                {
                    double delta = getDeltaInMs(rowItems, mainNode->takeRow(mainNode->rowCount()-1));
                    rowItems << new QStandardItem(QString("%1ms").arg(delta));
                }
                treeViewMutex.lock();
                mainNode->appendRow(rowItems);
                treeViewMutex.unlock();
                /* treeView->expand(mainNode->index());

                for (int i = 0; i < model->columnCount(); i++)
                {
                    treeView->resizeColumnToContents(1);
                }*/
            }
            else
            {
                treeViewMutex.lock();
                invisibleRootItem->appendRow(rowItems);
                treeViewMutex.unlock();
            }
/*            while (invisibleRootItem->rowCount() >= 30)
            {
                QStandardItem *tmpMainNode = invisibleRootItem->child(0,0);
                while (tmpMainNode->rowCount() > 0)
                {
                    QStandardItem * tmpItem = invisibleRootItem->child(0,0)->child(0,0);
                    invisibleRootItem->removeRow(0);
                    delete tmpItem;
                }
                invisibleRootItem->removeRow(0);
                delete tmpMainNode;
            }*/
            //
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
        QList<QStandardItem *>  rowItems = prepareRow(line);
        appendRowItems(rowItems);
    }
}

long long EbxMonitorWorker::getFromRowItem(QList<QStandardItem *> rowItems, int position){
    if (rowItems.count()>position)
    {
        return rowItems.at(position)->text().split("us").first().toLongLong();
    }
    else
    {
        return -1;
    }
}

long long EbxMonitorWorker::getTimestamp(QList<QStandardItem *> rowItems){
    return getFromRowItem(rowItems, 1);
}

long long EbxMonitorWorker::getId(QList<QStandardItem *> rowItems){
    return getFromRowItem(rowItems, 0);
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

QList<QStandardItem *> EbxMonitorWorker::prepareRow(const QString &line)
{
    QList<QStandardItem *> rowItems;

    const QStringList fields = line.split(",");
    if (fields.count() >=3){
        // Add ID
        if (fields.at(0).length()>2){
            QString str = fields.at(0);
            str = str.replace(QChar('\0'), QString(""));
            rowItems << new QStandardItem(str);
        }
        else{
            rowItems << new QStandardItem(QString("?"));
        }
        // Add Timestamp
        if (fields.at(1).length()>2){
            rowItems << new QStandardItem(fields.at(1));
        }
        else{
            rowItems << new QStandardItem(QString("?"));
        }
        // Add Position TAG
        rowItems << new QStandardItem(fields.at(2));
        // Add delta in ms
        rowItems << new QStandardItem(QString("%1ms").arg(getDeltaInMs(rowItems)));
    }
    return rowItems;
}

EbxMonitorWorker::~EbxMonitorWorker()
{
    this->model = 0;
    this->treeView = 0;
}

