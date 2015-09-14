#ifndef EBXMONITORWORKER_H
#define EBXMONITORWORKER_H

#include <QObject>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QStandardItem>
#include <QTreeView>
#include <QThread>
#include <QTimer>
#include "time.h"
#include <sys/time.h>


#define TIMER_DELAY 500

class EbxMonitorWorker : public QObject
{
    Q_OBJECT    

private:
    QStandardItemModel * model;
    QTreeView * treeView;
    QList<QStandardItem *> prepareRow(const QString &line);
    QAtomicInt stopMonitor;
    QTimer timer;

    long long getFromRowItem(QList<QStandardItem *> rowItems, int position);
    long long getTimestamp(QList<QStandardItem *> rowItems);
    long long getId(QList<QStandardItem *> rowItems);
    long long getDelta(QList<QStandardItem *> rowItems);
    long long getDelta(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);
    double getDeltaInMs(QList<QStandardItem *> rowItems);
    double getDeltaInMs(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);

    void appendRowItems( QList<QStandardItem *>  rowItems);

public:
    explicit EbxMonitorWorker(QObject *parent = 0, QStandardItemModel *model = 0, QTreeView *treeView = 0);
    ~EbxMonitorWorker();

signals:
    void finished();

public slots:
    void updateEbxMonitorData( qint64 id);
    void gotNewFrame(qint64 timestamp);
    void grabfromdriver();
    void stop();
    void start();
};

#endif // EBXMONITORWORKER_H

