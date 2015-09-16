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
#include <QMutex>
#include <QCoreApplication>



#define TIMER_DELAY 500

class EbxMonitorWorker : public QObject
{
    Q_OBJECT    

private:
    QStandardItemModel * model;
    QTreeView * treeView;    
    QAtomicInt stopMonitor;
    QTimer timer;
    QMutex treeViewMutex;
    QList<QStandardItem *> createdItems;


    int prepareRow(QList<QStandardItem *> *rowItems, const QString &line);
    QStandardItem * createRowItem(const QString data);

    long long getFromRowItem(QList<QStandardItem *> rowItems, int position);

    int getPosition(QList<QStandardItem *> rowItems);
    int getPosition(QString line);

    long long getTimestamp(QList<QStandardItem *> rowItems);
    long long getTimestamp(QString line);
    long long getId(QList<QStandardItem *> rowItems);
    long long getId(QString line);
    long long getDelta(QList<QStandardItem *> rowItems);
    long long getDelta(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);
    double getDeltaInMs(QList<QStandardItem *> rowItems);
    double getDeltaInMs(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder);

    void appendRowItems(QList<QStandardItem *>  * rowItems);

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
    void startGrabbing();
};

#endif // EBXMONITORWORKER_H

