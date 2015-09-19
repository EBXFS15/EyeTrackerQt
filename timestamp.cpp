#include "timestamp.h"

Timestamp::Timestamp(QObject *parent, QString line) : QObject(parent)
{
    this->position = -1;
    this->id =-1;
    this->registrationTime =-1;
    this->alternativeId = -1;
    setId(line);
    setTimestamp(line);
    setPosition(line);
}



int Timestamp::setPosition(QString line){
    const QStringList fields = line.split(",");
    if (fields.count()>2)
    {
        this->position= fields.at(2).toInt();
    }
    return getPosition();
}

qint64 Timestamp::setTimestamp(QString line)
{
    const QStringList fields = line.split(",");
    if (fields.count()>1)
    {
        this->registrationTime  = fields.at(1).split("us").first().toLongLong();
    }
    return getTimestamp();
}

qint64 Timestamp::setId(QString line)
{
    const QStringList fields = line.split(",");
    if (fields.count()>0)
    {
        QString str = fields.at(0).split("us").first();
        str = str.replace(QChar('\0'), QString(""));
        this->id = str.toLongLong();
    }
    return getId();
}


bool Timestamp::isRelated(Timestamp *previous)
{
    if (this->position != 11){
        return false;
    }
    if (previous->getPosition() != 10){
        return false;
    }
    qint64 delta = (this->getTimestamp() - previous->getTimestamp());
    if ((delta <= 2) && (delta >=0))
    {
        return true;
    }
    else
    {
        return false;
    }
}

int Timestamp::getPosition(){
    return position;
}

qint64 Timestamp::getTimestamp()
{
    return registrationTime;
}

qint64 Timestamp::getId()
{
    return id;
}

qint64 Timestamp::getAlternativeId()
{
    return alternativeId;
}

void Timestamp::setAlternativeId(qint64 newId)
{
    this->alternativeId = newId;
}
/**
 * @brief Timestamp::prepareRowinserts the data of the object to the expected form.
 * ID/ALTERNATIVE-ID, Measurement position, Registration Time, latency, delay to previous element or initial timestamp.
 * @param previousTimestamp
 * @return list that can be added directly to a treeview.
 */
QString Timestamp::toString(Timestamp * previousTimestamp)
{
    QString tempString;

    tempString.append(QString("%1(%2),").arg(this->id).arg(this->alternativeId));
    tempString.append(QString("%1,").arg(this->position));
    tempString.append(QString("%1us,").arg(this->registrationTime));
    tempString.append(QString("%1us,").arg(this->getDelayInMs()));

    if(0 != previousTimestamp)
    {
        tempString.append(QString("%1us").arg(this->getDelayInMs(previousTimestamp)));
    }
    else
    {
        tempString.append(QString("%1us").arg(this->getDelayInMs()));
    }

    return tempString;
}

double Timestamp::getDelayInMs(Timestamp *timestampToCompare)
{
    double tmp =  (((double)(timestampToCompare->getTimestamp() - this->getTimestamp()))/1000);
    if (tmp < 0)
    {
        return -tmp;
    }
    else
    {
        return tmp;
    }
}

double Timestamp::getDelayInMs()
{
    if(alternativeId != -1)
    {
        return (((double)(registrationTime - alternativeId))/1000);
    }
    else
    {
        return (((double)(registrationTime - id))/1000);
    }

}

Timestamp::~Timestamp()
{

}

