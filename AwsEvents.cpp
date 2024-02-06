#include "AwsEvents.h"

QStringList AwsEvents::findJsonLogs(const QString& logPath){
    QDirIterator it(logPath, QDir::NoFilter, QDirIterator::Subdirectories);
    QStringList jsonFiles;

    while (it.hasNext()) {
        it.next();
        if (QFileInfo(it.filePath()).isFile()){
            if (QFileInfo(it.filePath()).suffix() == "json"){
                jsonFiles << it.filePath();
            }
        }
    }

    return jsonFiles;
}

QJsonObject AwsEvents::findEvent(const QString& eventID, const QString& filePath){
    QJsonObject result;

    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly)){
        QByteArray data = file.readAll();
        QJsonParseError errorPtr;
        QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
        if (doc.isNull()) {
            //Error parsing file
        }else{
            QJsonObject rootObj = doc.object();
            QJsonArray values = rootObj.value("Records").toArray();
            foreach(const QJsonValue &val, values){
                if(val.toObject().value("eventID") == eventID){
                    file.close();
                    result = val.toObject();
                    break;
                }
            }
        }
        file.close();
    }
    else{
        //Error opening file
    }

    return result;
}

QString AwsEvents::loadDictionnary(AwsEvents::Dictionary *dictionnary, const QStringList& jsonFiles){
    //Load keys from each log files
    foreach(QString filePath, jsonFiles){
        QString error = AwsEvents::loadDictionnaryFromFile(dictionnary, filePath);
        if(!error.isEmpty()){
            return error;
        }
    }

    //Remove duplicates
    dictionnary->knownKeys.removeDuplicates();
    foreach(const QString& key, dictionnary->knownValues.keys()){
        QStringList* stringList = dictionnary->knownValues.value(key);
        stringList->removeDuplicates();
    }

    return "";
}

QString AwsEvents::loadDictionnaryFromFile(AwsEvents::Dictionary* dictionnary, const QString& filePath){
    QStringList keysList;

    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly)){
        QByteArray data = file.readAll();
        QJsonParseError errorPtr;
        QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
        if (doc.isNull()) {
            //Error parse failed
        }else{
            QJsonObject rootObj = doc.object();
            QJsonArray events = rootObj.value("Records").toArray();
            foreach(const QJsonValue &event, events){
                keysList += AwsEvents::loadDictionnaryFromEvent(dictionnary, event);
            }
        }
        file.close();
    }else{
        return "Error, unable to open file";
    }

    dictionnary->knownKeys.removeDuplicates();

    return "";
}

QString AwsEvents::loadDictionnaryFromEvent(AwsEvents::Dictionary* dictionnary, const QJsonValue& obj, const QString& keyPath){
    if(dictionnary == nullptr){
        return "No dictionnary pointer provided";
    }else{
        if(obj.isObject()){
            //If it's an object, process each sub-keys of it.
            foreach(const QString& subKey,  obj.toObject().keys()){
                QJsonValue subObj = obj.toObject().value(subKey);
                QString path = subKey;
                if(!keyPath.isEmpty()){
                    path = keyPath + "." + path;
                }
                (dictionnary->knownKeys) += AwsEvents::loadDictionnaryFromEvent(dictionnary, subObj, path);
            }
        }else if(obj.isArray()){
            //If it's an array, process each sub-keys of it.
            foreach(const QJsonValue& subObj,  obj.toArray()){
                dictionnary->knownKeys += AwsEvents::loadDictionnaryFromEvent(dictionnary, subObj, keyPath);
            }
        }else{
            //Else, it does not contain subkeys, so append the key to result array
            dictionnary->knownKeys << keyPath;
            if(!dictionnary->knownValues.value(keyPath)){
                dictionnary->knownValues.insert(keyPath, new QStringList());
            }
            dictionnary->knownValues[keyPath]->append(AwsEvents::jsonValueToQString(obj));
        }
    }

    return "";
}

QJsonValue AwsEvents::getValue(const QString& keyPath, const QJsonObject& parentObject){
    if(keyPath.contains(".")){
        QStringList keysList = keyPath.split(".");
        QString parentKey = keysList.at(0);
        QString subKey = "";
        for(unsigned int i = 1; i < keysList.size(); i++){
            subKey += keysList.at(i);
            if(i < keysList.size() - 1){
                subKey += ".";
            }
        }
        if(parentObject[parentKey].isArray()){
            QJsonArray array = parentObject[parentKey].toArray();
            //TO DO : implement array handling
        }else if(parentObject[parentKey].isObject()){
            QJsonObject childObject = parentObject[parentKey].toObject();
            return getValue(subKey, childObject);
        }
    }else{
        return parentObject.value(keyPath);
    }

    return "";
}

QString AwsEvents::jsonValueToQString(const QJsonValue& jsonValue){
    QString result;
    if(jsonValue.isString()){
        result = jsonValue.toString();
    }else if(jsonValue.isDouble()){
        result = QString::number(jsonValue.toBool());
    }else if(jsonValue.isBool()){
        if(jsonValue.toBool() == true){
            result ="true";
        }else{
            result = "false";
        }
    }else{
        result = "";
    }
    return result;
}

bool AwsEvents::matchFilters(const QJsonValue& event, const std::vector<Filter>& filters){
    foreach(const Filter& filter, filters){
        if(AwsEvents::getValue(filter.key, event.toObject()).isUndefined()){
            return false;
        }
        else if(filter.operation == "equal"){
            if(AwsEvents::getValue(filter.key, event.toObject()) != filter.value){
                return false;
            }
        }
        else if(filter.operation == "not equal"){
            if(AwsEvents::getValue(filter.key, event.toObject()) == filter.value){
                return false;
            }
        }
        else if(filter.operation == "contains"){
            if(!AwsEvents::getValue(filter.key, event.toObject()).toString().contains(filter.value)){
                return false;
            }
        }
    }

    return true;
}

bool AwsEvents::matchScope(const QJsonValue& event, const Scope &scope){
    QDateTime eventDateTime = QDateTime::fromString(AwsEvents::getValue("eventTime", event.toObject()).toString(), Qt::ISODate);
    if(scope.fromEnabled){
        if(eventDateTime < scope.fromDateTime){
            return false;
        }
    }
    if(scope.toEnabled){
        if(eventDateTime > scope.toDateTime){
            return false;
        }
    }

    return true;
}

QString AwsEvents::getStatisticsFromScope(const QString& key, const std::vector<AwsEvents::Event>& events, std::vector<AwsEvents::Statistic>* statistics){
    QMap<QString, int> occurences;
    int totalOccurrences = 0;

    foreach(const AwsEvents::Event& event, events){
        const QJsonObject& eventData = AwsEvents::findEvent(event.id, event.filePath);
        const QString& jsonValue = AwsEvents::jsonValueToQString(AwsEvents::getValue(key, eventData));
        if(!occurences.value(jsonValue)){
            occurences.insert(jsonValue, 1);
        }else{
            occurences[jsonValue] += 1;
        }
        totalOccurrences += 1;
    }

    //Calculate percentages
    foreach(const QString& key, occurences.keys()){
        int occurance = occurences.value(key);
        float ratio = (double)occurance / (double)totalOccurrences;
        float percentage = ratio * 100.f;

        AwsEvents::Statistic statistic;
        statistic.value = key;
        statistic.percentage = percentage;
        statistic.occurences = occurance;
        statistic.totalOccurences = totalOccurrences;
        statistics->push_back(statistic);
    }

    return "";
}

QString AwsEvents::getStatisticsFromFiles(const QString& key, const QStringList& jsonFiles, std::vector<Statistic>* statistics){
    QMap<QString, int> occurences;
    int totalOccurrences = 0;

    foreach(const QString& filePath, jsonFiles){
        QFile file(filePath);
        if(file.open(QIODevice::ReadOnly)){
            QByteArray data = file.readAll();
            QJsonParseError errorPtr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
            if (doc.isNull()) {
                return "Parse failed";
            }else{
                QJsonObject rootObj = doc.object();
                QJsonArray events = rootObj.value("Records").toArray();
                foreach(const QJsonValue &event, events){
                    const QString& jsonValue = AwsEvents::jsonValueToQString(AwsEvents::getValue(key, event.toObject()));
                    if(!occurences.value(jsonValue)){
                        occurences.insert(jsonValue, 1);
                    }else{
                        occurences[jsonValue] += 1;
                    }
                    totalOccurrences += 1;
                }
            }
            file.close();
        }
        else{
            return "Error opening log file";
        }
    }

    //Calculate percentages
    foreach(const QString& key, occurences.keys()){
        int occurance = occurences.value(key);
        float ratio = (float)occurance / (float)totalOccurrences;
        float percentage = ratio * 100.f;

        AwsEvents::Statistic statistic;
        statistic.value = key;
        statistic.percentage = percentage;
        statistic.occurences = occurance;
        statistic.totalOccurences = totalOccurrences;
        statistics->push_back(statistic);
    }

    return "";
}

void AwsEvents::clearDictionary(AwsEvents::Dictionary* dictionnary){
    dictionnary->knownKeys.clear();

    foreach(const QString& key, dictionnary->knownValues.keys()){
        QStringList* stringList = dictionnary->knownValues.value(key);
        delete stringList;
    }

    dictionnary->knownValues.clear();
}

const QMap<QString, AwsEvents::SuspiciousActivity> AwsEvents::findSuspiciousActivity(const QStringList& jsonFiles){
    QMap<QString, SuspiciousActivity> suspiciousActivities;
    QStringList blacklist = {"AWS Internal"};

    //Find IP addresses with access denied errors
    foreach(const QString& jsonFile, jsonFiles){
        QFile file(jsonFile);
        if(file.open(QIODevice::ReadOnly)){
            QByteArray data = file.readAll();
            file.close();
            QJsonParseError errorPtr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
            if(!doc.isNull()){
                //Process each events in the current file
                QJsonObject rootObj = doc.object();
                QJsonArray events = rootObj.value("Records").toArray();
                foreach(const QJsonValue& event, events){
                    //If event has AccessDenied error
                    if(AwsEvents::getValue("errorCode", event.toObject()) == "AccessDenied"){
                        //Retrieve the source IP address
                        QString srcAddress = AwsEvents::getValue("sourceIPAddress", event.toObject()).toString();
                        //If this source IP address is blacklisted
                        if(!blacklist.contains(srcAddress)){
                            //If the suspicious source IP address is not yet registered
                            if(!suspiciousActivities.contains(srcAddress)){
                                //Register the suspicious IP address
                                SuspiciousActivity suspiciousActivity;
                                suspiciousActivity.accessDeniedCount = 1;
                                suspiciousActivities.insert(srcAddress, suspiciousActivity);
                            }
                            //Increment the AccessDenied errors count
                            suspiciousActivities[srcAddress].accessDeniedCount += 1;
                        }
                    }
                }
            }
        }
    }

    //Find user accounts accessed by suspicious addresses
    foreach(const QString& jsonFile, jsonFiles){
        QFile file(jsonFile);
        if(file.open(QIODevice::ReadOnly)){
            QByteArray data = file.readAll();
            file.close();
            QJsonParseError errorPtr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
            if(!doc.isNull()){
                //Process each events of the current file
                QJsonObject rootObj = doc.object();
                QJsonArray events = rootObj.value("Records").toArray();
                foreach(const QJsonValue& event, events){
                    QJsonObject eventObject = event.toObject();
                    QString srcAddress = AwsEvents::getValue("sourceIPAddress", eventObject).toString();
                    //If the current event was created by a suspicious source IP address
                    if(suspiciousActivities.contains(srcAddress)){
                        //Retrieve the user account accessed by the suspicious source IP address
                        QString userAccount = AwsEvents::getValue("userIdentity.userName", eventObject).toString();
                        if(!userAccount.isEmpty()){
                            //If user account was not registered yet
                            if(!suspiciousActivities[srcAddress].userAccountsAccessed.contains(userAccount)){
                                //Register it
                                suspiciousActivities[srcAddress].userAccountsAccessed.append(userAccount);
                            }
                        }

                        //Check if suspicious address created user policy
                        if(AwsEvents::getValue("eventName", eventObject).toString() == "PutUserPolicy"){
                            //Check if the action was successful
                            if(AwsEvents::getValue("errorCode", eventObject).toString().isEmpty()){
                                //Retrieve the new policy added by the suspicious address
                                AwsEvents::UserPolicy userPolicy;
                                userPolicy.name = AwsEvents::getValue("requestParameters.policyName", eventObject).toString();
                                userPolicy.document = AwsEvents::getValue("requestParameters.policyDocument", eventObject).toString();
                                //If the suspicious user policy is not registered yet
                                if(!suspiciousActivities[srcAddress].userPoliciesCreated.contains(userPolicy)){
                                    //Register it
                                    suspiciousActivities[srcAddress].userPoliciesCreated.append(userPolicy);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return suspiciousActivities;
}
