#ifndef AWSEVENTS_H
#define AWSEVENTS_H

#include <QDir>
#include <QDirIterator>
#include <QJsonValue>
#include <QList>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>

namespace AwsEvents{
struct Dictionary{
    QStringList knownKeys;
    QMap<QString, QStringList*> knownValues;
};

struct Filter{
    QString operation;
    QString key;
    QString value;
};

struct Scope{
    QDateTime fromDateTime;
    QDateTime toDateTime;
    bool fromEnabled;
    bool toEnabled;
};

struct Event{
    QString filePath;
    QString id;
    QString dateTime;
    QString region;
};

struct Error{
    Event event;
    QString source;
    QString message;
    QString code;
};

struct Statistic{
    QString key;
    QString value;
    int occurences;
    int totalOccurences;
    float percentage;
};

struct UserPolicy{
    QString name;
    QString document;
    bool operator ==(const AwsEvents::UserPolicy& a) const{
        return (a.document == this->document) && (a.name == this->name);
    }
};

struct SuspiciousActivity{
    QStringList userAccountsAccessed;
    QList<AwsEvents::UserPolicy> userPoliciesCreated;
    unsigned int accessDeniedCount;
};

QStringList findJsonLogs(const QString &logPath);
QJsonObject findEvent(const QString& eventID, const QString& filePath);
QJsonValue getValue(const QString& keyPath, const QJsonObject& parentObject);

bool matchFilters(const QJsonValue& event, const std::vector<AwsEvents::Filter>& filters);
bool matchScope(const QJsonValue& event, const Scope& scope);
QString getStatisticsFromScope(const QString& key, const std::vector<Event>& events, std::vector<AwsEvents::Statistic>* statistics);
QString getStatisticsFromFiles(const QString& key, const QStringList& jsonFiles, std::vector<AwsEvents::Statistic>* statistics);

QString loadDictionnary(AwsEvents::Dictionary* dictionnary, const QStringList& jsonFiles);
QString loadDictionnaryFromFile(AwsEvents::Dictionary* dictionnary, const QString& filePath);
QString loadDictionnaryFromEvent(AwsEvents::Dictionary* dictionnary, const QJsonValue& obj, const QString& keyPath = "");
void clearDictionary(AwsEvents::Dictionary* dictionnary);

QString jsonValueToQString(const QJsonValue& jsonValue);

//Threat hunting related methods
const QMap<QString, SuspiciousActivity> findSuspiciousActivity(const QStringList& jsonFiles);
}

#endif // AWSEVENTS_H
