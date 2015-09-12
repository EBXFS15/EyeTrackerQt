#include "eyetrackerwindow.h"
#include "ui_eyetrackerwindow.h"



using namespace cv;


EyeTrackerWindow::EyeTrackerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EyeTrackerWindow)
{
    ui->setupUi(this);
    frameMonitor = new QStandardItemModel;

    connect(&captureThread, SIGNAL(imageCaptured(QImage, qint64)), this, SLOT(onCaptured(QImage, qint64)));
    connect(&captureThread, SIGNAL(onGotFrame(qint64)), this,SLOT(gotframe(qint64)));
    connect(ui->quitBtn,SIGNAL(clicked()),this,SLOT(onClosed()));

    ui->frames->setModel(frameMonitor);
    ui->frames->show();
    captureThread.start();
}

EyeTrackerWindow::~EyeTrackerWindow()
{
    delete ui;
}

void EyeTrackerWindow::updateImage(QPixmap pixmap)
{
    ui->label->clear();
    ui->label->setPixmap(pixmap);
}

void EyeTrackerWindow::addTimestamp(qint64 timestamp)
{
    static int count = 0;
    ui->label_timestamp->setText("Timestamp:"+ QString::number(timestamp));
    if (count > 30)
    {
        gotFrame(timestamp);
        count = 0;
    }
    count++;
}

void EyeTrackerWindow::onCaptured(QImage captFrame, qint64 timestamp)
{
    updateImage(QPixmap::fromImage(captFrame));
    addTimestamp(timestamp);    
}

void EyeTrackerWindow::gotFrame(qint64 timestamp)
{
    struct timespec gotTime;
    clock_gettime(CLOCK_REALTIME_COARSE, &gotTime);
    qint64 rxTimeStamp = ((gotTime.tv_sec) * 1000000 + gotTime.tv_nsec/1000);
    readEbxMonitor();
    QStandardItem *item = frameMonitor->invisibleRootItem();
    if (item != 0)
    {
        QString line = QString("%1us, %2us, 255").arg(timestamp).arg(rxTimeStamp);
        QList<QStandardItem *>  rowItems = prepareRow(line);
        if (rowItems.count() > 0)
        {
            QString searchPattern = rowItems.first()->text();
            QModelIndexList match = item->model()->match(
                        item->model()->index(0, 0),
                        Qt::DisplayRole,
                        QVariant::fromValue(searchPattern),
                        1, // look *
                        Qt::MatchRecursive);
            if (match.count() > 0)
            {
                QStandardItem * mainNode = frameMonitor->itemFromIndex(match.first());
                if (mainNode->rowCount() > 0)
                {
                    double delta = getDeltaInMs(rowItems, mainNode->takeRow(mainNode->rowCount()-1));

                    rowItems << new QStandardItem(QString("%1ms").arg(delta));
                }

                frameMonitor->itemFromIndex(match.first())->appendRow(rowItems);
                for (int i = 0; i <rowItems.count(); i++)
                {
                    ui->frames->resizeColumnToContents(i);
                }
                ui->frames->expand(match.first());
            }
            else{
                item->appendRow(rowItems);
            }
        }
    }
}



void EyeTrackerWindow::readEbxMonitor()
{
    struct timespec gotTime;
    clock_gettime(CLOCK_MONOTONIC, &gotTime);
    qint64 rxTimeStamp = ((gotTime.tv_sec) * 1000000 + gotTime.tv_nsec/1000);

    QStandardItem *item = frameMonitor->invisibleRootItem();
    if (item != 0)
    {
        FILE* fd = fopen("/dev/ebx_monitor", "r"); // get a file descriptor somehow
        if (fd!=0)
        {
            QFile file;

            if (!file.open(fd, QIODevice::ReadOnly))
            {
                QMessageBox::warning(this, "Error", QString("Failed to open iPod device. Reason: ").append(file.errorString()));
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
                        QModelIndexList match = item->model()->match(
                                    item->model()->index(0, 0),
                                    Qt::DisplayRole,
                                    QVariant::fromValue(searchPattern),
                                    1, // look *
                                    Qt::MatchRecursive);
                        if (match.count() > 0)
                        {
                            QStandardItem * mainNode = frameMonitor->itemFromIndex(match.first());
                            if (mainNode->rowCount() > 0)
                            {
                                double delta = getDeltaInMs(rowItems, mainNode->takeRow(mainNode->rowCount()-1));

                                rowItems << new QStandardItem(QString("%1ms").arg(delta));

                                //long long previousTimestamp = frameMonitor->itemFromIndex(match.first())->takeRow(0)->at(1)->split("us").first().toLongLong();
                                //long long currentTimestamp = rowItems->at(1)->split("us")->first()->toLongLong();
                            }


                            frameMonitor->itemFromIndex(match.first())->appendRow(rowItems);
                            for (int i = 0; i <rowItems.count(); i++)
                            {
                                ui->frames->resizeColumnToContents(i);
                            }
                            ui->frames->expand(match.first());
                        }
                        else{
                            item->appendRow(rowItems);
                        }
                    }
                }
                file.close();
                fclose(fd);
            }
            if(item->rowCount() == 0){
                for (int i = 0; i < 4; i++)
                {
                    ui->frames->resizeColumnToContents(i);
                }
            }
            if(item->rowCount() >= 16){
                item->removeRows(0,item->rowCount()-16);
            }
            if (ui->chk_scrollToBottom->isChecked())
            {
                ui->frames->scrollToBottom();
            }
        }
    }
}

long long EyeTrackerWindow::getFromRowItem(QList<QStandardItem *> rowItems, int position){
    if (rowItems.count()>position)
    {
        return rowItems.at(position)->text().split("us").first().toLongLong();
    }
    else
    {
        return -1;
    }
}

long long EyeTrackerWindow::getTimestamp(QList<QStandardItem *> rowItems){
    return getFromRowItem(rowItems, 1);
}

long long EyeTrackerWindow::getId(QList<QStandardItem *> rowItems){
    return getFromRowItem(rowItems, 0);
}

long long EyeTrackerWindow::getDelta(QList<QStandardItem *> rowItems){
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

long long EyeTrackerWindow::getDelta(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder){
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

double EyeTrackerWindow::getDeltaInMs(QList<QStandardItem *> rowItems){
    if (rowItems.count()>=2)
    {
        return ((double)getDelta(rowItems))/1000;
    }
    else
    {
        return -1;
    }
}

double EyeTrackerWindow::getDeltaInMs(QList<QStandardItem *> rowItemsNewer,QList<QStandardItem *> rowItemsOlder){
    if (rowItemsNewer.count()>=2 && rowItemsOlder.count()>=2)
    {
        return ((double)getDelta(rowItemsNewer, rowItemsOlder))/1000;
    }
    else
    {
        return -1;
    }
}

QList<QStandardItem *> EyeTrackerWindow::prepareRow(const QString &line)
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

void EyeTrackerWindow::onClosed()
{
    captureThread.stopCapturing();
    captureThread.wait();
    this->close();
}
