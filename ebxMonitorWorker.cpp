#include "ebxMonitorWorker.h"

EbxMonitorWorker::EbxMonitorWorker(QObject *parent) : QObject(parent)
{

}

void
EbxMonitorWorker::updateEbxMonitorData( QStandardItemModel * model, QTreeView * treeView){
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

                    QString line = in.readLine();
                    QList<QStandardItem *>  rowItems = prepareRow(line);
                    if (rowItems.count() > 0)
                    {
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

                                //long long previousTimestamp = frameMonitor->itemFromIndex(match.first())->takeRow(0)->at(1)->split("us").first().toLongLong();
                                //long long currentTimestamp = rowItems->at(1)->split("us")->first()->toLongLong();
                            }


                            mainNode->appendRow(rowItems);
                            for (int i = 0; i <rowItems.count(); i++)
                            {
                                treeView->resizeColumnToContents(i);
                            }
                            treeView->expand(match.first());
                        }
                        else{
                            invisibleRootItem->appendRow(rowItems);
                        }
                    }
                }
                file.close();
                fclose(fd);
            }
            if(invisibleRootItem->rowCount() == 0){
                for (int i = 0; i < 4; i++)
                {
                    treeView->resizeColumnToContents(i);
                }
            }
            if(invisibleRootItem->rowCount() >= 16){
                invisibleRootItem->removeRows(0,invisibleRootItem->rowCount()-16);
            }
        }
    }
}


void EbxMonitorWorker::gotNewFrame(QStandardItemModel *model, QTreeView *treeView, qint64 frameId){
    struct timespec gotTime;
    clock_gettime(CLOCK_REALTIME_COARSE, &gotTime);
    qint64 rxTimeStamp = ((gotTime.tv_sec) * 1000000 + gotTime.tv_nsec/1000);
    updateEbxMonitorData(model, treeView);
    QStandardItem *invisibleRootItem = model->invisibleRootItem();
    if (invisibleRootItem != 0)
    {
        QString line = QString("%1us, %2us, 255").arg(frameId).arg(rxTimeStamp);
        QList<QStandardItem *>  rowItems = prepareRow(line);
        if (rowItems.count() > 0)
        {
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

                model->itemFromIndex(match.first())->appendRow(rowItems);
                for (int i = 0; i <rowItems.count(); i++)
                {
                    treeView->resizeColumnToContents(i);
                }
                treeView->expand(match.first());
            }
            else{
                invisibleRootItem->appendRow(rowItems);
            }
        }
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

}

