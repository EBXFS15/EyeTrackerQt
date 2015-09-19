#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <QObject>
#include <QStringList>
#include <QStandardItem>

class Timestamp : public QObject
{
    Q_OBJECT

private:
    qint64 id;
    qint64 alternativeId;
    qint64 registrationTime;
    int position;

    QList<QStandardItem *> rowItems;

    int setPosition(QString line);
    qint64 setTimestamp(QString line);
    qint64 setId(QString line);

public:
    /**
     * @brief Timestamp object to manage ebx_monitoring data.
     * @param parent to allow easier clean up.
     * @param line the measurement line raw data. Expected format: <Identifier>us, <RegistrationTime>us, <measurementPosition>
     */
    explicit Timestamp(QObject *parent = 0, QString line = "");

    bool operator<(const Timestamp & other) const {
        return (registrationTime < other.registrationTime);
    }
    bool operator==(const Timestamp& other) const {
        return ((id == other.id) || (id == other.alternativeId));
    }    

    bool isRelated(Timestamp * reference);


    int getPosition();
    qint64 getTimestamp();
    qint64 getId();
    qint64 getAlternativeId();
    void setAlternativeId(qint64 newId);

    QString toString(Timestamp * previousTimestamp = 0);

    double getDelayInMs(Timestamp * timestampToCompare);
    double getDelayInMs();

    ~Timestamp();

signals:

public slots:
};

#endif // TIMESTAMP_H
