#ifndef EBXMONITORWORKER_H
#define EBXMONITORWORKER_H

#include <QObject>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QStandardItem>
#include <QTreeView>
#include "time.h"
#include <sys/time.h>

class EbxMonitorWorker : public QObject
{
    Q_OBJECT
private:
    QList<QStandardItem *> prepareRow(const QString &line);
    long long getFromRowItem(QList<QStandardItem *> rowItems, int position);
    long long getTimestamp(QList<QStandardItem *> rowItems);
    long long getId(QList<QStandardItem *> rowItems);
    long long getDelta(QList<QStandardItem *> rowItems);
    long long getDelta(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);
    double getDeltaInMs(QList<QStandardItem *> rowItems);
    double getDeltaInMs(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);

public:
    explicit EbxMonitorWorker(QObject *parent = 0);
    ~EbxMonitorWorker();

signals:    

public slots:
    void updateEbxMonitorData(QStandardItemModel * model, QTreeView * treeView);
    void gotNewFrame( QStandardItemModel * model, QTreeView * treeView, qint64 timestamp);
};

#endif // EBXMONITORWORKER_H
