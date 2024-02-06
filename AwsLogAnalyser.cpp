#include "AwsLogAnalyser.h"
#include "./ui_AwsLogAnalyser.h"

AwsLogAnalyser::AwsLogAnalyser(QWidget *parent) : QWidget(parent), ui(new Ui::AwsLogAnalyser){
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/images/AwsLogAnalyser_logo_small.png"));
    this->ui->tableWidget_filters->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->ui->tableWidget_matching_events->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->ui->tableWidget_statistics_results->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->ui->tableWidget_errors->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->ui->dateTimeEdit_time_from->setTimeSpec(Qt::UTC);
    this->ui->dateTimeEdit_time_to->setTimeSpec(Qt::UTC);
    this->ui->lineEdit_addfilter_value->setCompleter(new QCompleter(QStringList()));
    this->ui->lineEdit_addfilter_key->setCompleter(new QCompleter(QStringList()));
    this->ui->lineEdit_statistics_key->setCompleter(new QCompleter(QStringList()));
    this->ui->treeWidget_alerts->header()->setStretchLastSection(false);
    this->ui->treeWidget_alerts->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->showMaximized();
    this->newProject();
}

AwsLogAnalyser::~AwsLogAnalyser(){
    delete ui;
}

//-----------------------------Slots-----------------------------//
void AwsLogAnalyser::on_pushButton_search_clicked(){
    //Clear table first
    this->clearMatchingEvents();

    //Read each json file
    for(unsigned int i = 0; i < m_jsonFiles.size(); i++){
        QFile file(m_jsonFiles.at(i));
        if(file.open(QIODevice::ReadOnly)){
            QByteArray data = file.readAll();
            file.close();
            QJsonParseError errorPtr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
            if (doc.isNull()) {
                this->showErrorMessage("Parse failed");
            }
            else{
                //Process each events of the processing file
                QJsonObject rootObj = doc.object();
                QJsonArray events = rootObj.value("Records").toArray();
                AwsEvents::Scope scope;
                scope.fromEnabled = this->ui->checkBox_time_from->isChecked();
                scope.toEnabled = this->ui->checkBox_time_to->isChecked();
                scope.toDateTime = this->ui->dateTimeEdit_time_to->dateTime();
                scope.fromDateTime = this->ui->dateTimeEdit_time_from->dateTime();
                foreach(const QJsonValue &event, events){
                    if(AwsEvents::matchFilters(event, this->m_filters) && AwsEvents::matchScope(event, scope)){
                        AwsEvents::Event matchingEvent;
                        matchingEvent.filePath = m_jsonFiles.at(i);
                        const QDateTime& dateTime = QDateTime::fromString(AwsEvents::getValue("eventTime", event.toObject()).toString(), Qt::ISODate);
                        const QString& dateTimeString = dateTime.toString("dd/MM/yyyy hh:mm:ss");
                        matchingEvent.dateTime = dateTimeString;
                        matchingEvent.id = AwsEvents::getValue("eventID", event.toObject()).toString();
                        matchingEvent.region = AwsEvents::getValue("awsRegion", event.toObject()).toString();
                        this->m_matchingEventsResults.push_back(matchingEvent);
                    }
                }
            }
        }
    }
    //Update matching events table
    this->updateMatchingEventsTable();
}

void AwsLogAnalyser::on_pushButton_addfilter_clicked(){
    if(this->ui->lineEdit_addfilter_key->text() != "" && this->ui->lineEdit_addfilter_value->text() != ""){
        QTableWidgetItem* key = new QTableWidgetItem();
        QTableWidgetItem* operation = new QTableWidgetItem();
        QTableWidgetItem* value = new QTableWidgetItem();
        key->setText(this->ui->lineEdit_addfilter_key->text());
        operation->setText(this->ui->comboBox_filter_operation->currentText());
        value->setText(this->ui->lineEdit_addfilter_value->text());

        this->ui->tableWidget_filters->insertRow(this->ui->tableWidget_filters->rowCount());
        this->ui->tableWidget_filters->setItem(this->ui->tableWidget_filters->rowCount()-1, 0, key);
        this->ui->tableWidget_filters->setItem(this->ui->tableWidget_filters->rowCount()-1, 1, operation);
        this->ui->tableWidget_filters->setItem(this->ui->tableWidget_filters->rowCount()-1, 2, value);

        AwsEvents::Filter filter;
        filter.key = this->ui->lineEdit_addfilter_key->text();
        filter.operation = this->ui->comboBox_filter_operation->currentText();
        filter.value = this->ui->lineEdit_addfilter_value->text();
        this->m_filters.push_back(filter);

        this->ui->lineEdit_addfilter_key->setText("");
        this->ui->lineEdit_addfilter_value->setText("");
    }
}

void AwsLogAnalyser::on_pushButton_removefilters_clicked(){
    QModelIndexList selection = this->ui->tableWidget_filters->selectionModel()->selectedRows();
    for(int i = selection.count() - 1; i >= 0; i--){
        QModelIndex index = selection.at(i);
        this->ui->tableWidget_filters->removeRow(index.row());
        this->m_filters.erase(this->m_filters.begin() + i);
    }
}

void AwsLogAnalyser::on_tableWidget_matching_events_cellDoubleClicked(int row, int column){
    QString eventID = this->ui->tableWidget_matching_events->item(row, 0)->text();

    //Find event id in m_matchingEvents array and retrieve the filename
    QString filename = "";
    foreach(const AwsEvents::Event& event, this->m_matchingEventsResults){
        if(event.id == eventID){
            filename = event.filePath;
            break;
        }
    }

    QJsonDocument doc(AwsEvents::findEvent(eventID, filename));
    QString eventData(doc.toJson());
    this->ui->textEdit_event_details->setText(eventData);
}

void AwsLogAnalyser::on_pushButton_get_statistics_clicked(){
    this->clearStatisticsResults();

    if(this->ui->radioButton_statistics_scope_all->isChecked()){
        AwsEvents::getStatisticsFromFiles(this->ui->lineEdit_statistics_key->text(), this->m_jsonFiles, &this->m_statisticsResults);
    }else if(this->ui->radioButton_statistics_scope_matching->isChecked()){
        AwsEvents::getStatisticsFromScope(this->ui->lineEdit_statistics_key->text(), this->m_matchingEventsResults, &this->m_statisticsResults);
    }

    this->updateStatisticsTable();
}

void AwsLogAnalyser::on_checkBox_time_from_stateChanged(int arg1){
    this->ui->dateTimeEdit_time_from->setEnabled(arg1);
}

void AwsLogAnalyser::on_checkBox_time_to_stateChanged(int arg1){
    this->ui->dateTimeEdit_time_to->setEnabled(arg1);
}

void AwsLogAnalyser::on_lineEdit_addfilter_key_editingFinished(){
    this->updateValuesCompleter();
}

void AwsLogAnalyser::on_comboBox_filter_operation_currentIndexChanged(int index){
    this->updateValuesCompleter();
}

void AwsLogAnalyser::on_tableWidget_errors_cellDoubleClicked(int row, int column){
    QString eventID = this->ui->tableWidget_errors->item(row, 0)->text();

    //Find Error object from selected event ID
    QString filePath = "";
    foreach(const AwsEvents::Error& error, this->m_errorsResults){
        if(error.event.id == eventID){
            filePath = error.event.filePath;
            break;
        }
    }

    QJsonDocument doc(AwsEvents::findEvent(eventID, filePath));
    QString eventData(doc.toJson());
    this->ui->textEdit_error_details->setText(eventData);
}

void AwsLogAnalyser::on_pushButton_browse_clicked(){
    newProject();
}

void AwsLogAnalyser::on_radioButton_errors_scope_matching_clicked(){
    this->clearErrorsResults();

    foreach(const AwsEvents::Event& event, this->m_matchingEventsResults){
        const QJsonObject& eventData = AwsEvents::findEvent(event.id, event.filePath);
        const QString& errorCode = AwsEvents::jsonValueToQString(AwsEvents::getValue("errorCode", eventData));
        if(!errorCode.isEmpty()){
            AwsEvents::Error error;
            error.code = errorCode;
            error.event = event;
            error.message = AwsEvents::jsonValueToQString(AwsEvents::getValue("errorMessage", eventData));
            error.source = AwsEvents::jsonValueToQString(AwsEvents::getValue("sourceIPAddress", eventData));
            this->m_errorsResults.push_back(error);
        }
    }

    this->updateErrorsTable();
}

void AwsLogAnalyser::on_radioButton_errors_scope_all_clicked(){
    this->clearErrorsResults();

    foreach(const QString& filePath, this->m_jsonFiles){
        QFile file(filePath);
        if(file.open(QIODevice::ReadOnly)){
            QByteArray data = file.readAll();
            QJsonParseError errorPtr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
            if (doc.isNull()) {
                this->showErrorMessage("Parse failed");
            }else{
                QJsonObject rootObj = doc.object();
                QJsonArray events = rootObj.value("Records").toArray();
                foreach(const QJsonValue& jsonEvent, events){
                    const QJsonObject& objectEvent = jsonEvent.toObject();
                    const QString& errorCode = AwsEvents::jsonValueToQString(AwsEvents::getValue("errorCode", objectEvent));
                    if(!errorCode.isEmpty()){
                        AwsEvents::Error error;
                        error.code = errorCode;
                        error.event.id = AwsEvents::jsonValueToQString(AwsEvents::getValue("eventID", objectEvent));
                        error.event.dateTime = QDateTime::fromString(AwsEvents::jsonValueToQString(AwsEvents::getValue("eventTime", objectEvent)), Qt::ISODate).toString("dd/MM/yyyy hh:mm:ss");
                        error.message = AwsEvents::jsonValueToQString(AwsEvents::getValue("errorMessage", objectEvent));
                        error.source = AwsEvents::jsonValueToQString(AwsEvents::getValue("sourceIPAddress", objectEvent));
                        this->m_errorsResults.push_back(error);
                    }
                }
            }
            file.close();
        }
        else{
            this->showErrorMessage("Error opening log file");
        }
    }

    this->updateErrorsTable();
}
//---------------------------------------------------------------//


//-----------------------Private methods-------------------------//
void AwsLogAnalyser::newProject(){
    this->clearErrorsResults();
    this->clearMatchingEvents();
    this->clearStatisticsResults();
    AwsEvents::clearDictionary(&this->m_dictionnary);
    this->m_jsonFiles.clear();

    QString documentsPath = QString::fromStdString(getenv("USERPROFILE")) + "/Documents";
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open logs directory"), documentsPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    this->m_logsPath = dir;
    this->ui->label_logs_path->setText("Logs path : " + dir);

    this->m_jsonFiles = AwsEvents::findJsonLogs(this->m_logsPath);
    AwsEvents::loadDictionnary(&this->m_dictionnary, this->m_jsonFiles);

    //Find suspicious behaviour and update alerts
    this->updateAlerts();

    //Update completers models with new string lists
    QStringListModel* filterModel = (QStringListModel*)this->ui->lineEdit_addfilter_key->completer()->model();
    QStringListModel* statsModel = (QStringListModel*)this->ui->lineEdit_statistics_key->completer()->model();
    filterModel->setStringList(this->m_dictionnary.knownKeys);
    statsModel->setStringList(this->m_dictionnary.knownKeys);
}

void AwsLogAnalyser::showErrorMessage(QString errorMessage){
    QMessageBox msgBox(QMessageBox::Warning, "Error", errorMessage);
}

void AwsLogAnalyser::updateValuesCompleter(){
    //Retrieve selected operation string
    QString operation = this->ui->comboBox_filter_operation->currentText();

    //Retrieve value line edit completer model
    QStringListModel* completerModel = (QStringListModel*)this->ui->lineEdit_addfilter_value->completer()->model();

    if(operation == "equal" || operation == "not equal"){
        //Retrieve current key
        QString selectedKey = this->ui->lineEdit_addfilter_key->text();

        //Update completer's string list
        QStringList* values = this->m_dictionnary.knownValues.value(selectedKey);
        if(values){
            completerModel->setStringList(*values);
        }else{
            completerModel->setStringList(QStringList());
        }
    }else{
        //Remove completer from value line edit
        completerModel->setStringList(QStringList());
    }
}

void AwsLogAnalyser::updateMatchingEventsTable(){
    //Disable the sorting temporarily to simplify the usage of indexes when adding content to the table
    this->ui->tableWidget_matching_events->setSortingEnabled(false);

    //For each maching events, create a row in the matching events table
    foreach(const AwsEvents::Event& event, this->m_matchingEventsResults){
        QTableWidgetItem* eventID = new QTableWidgetItem();
        QTableWidgetItem* eventRegion = new QTableWidgetItem();
        QTableWidgetItem* eventTime = new QTableWidgetItem();
        eventID->setText(event.id);
        eventRegion->setText(event.region);
        eventTime->setText(event.dateTime);

        //Insert new row
        this->ui->tableWidget_matching_events->insertRow(this->ui->tableWidget_matching_events->rowCount());

        //Insert items in the new row
        this->ui->tableWidget_matching_events->setItem(this->ui->tableWidget_matching_events->rowCount()-1, 0, eventID);
        this->ui->tableWidget_matching_events->setItem(this->ui->tableWidget_matching_events->rowCount()-1, 1, eventTime);
        this->ui->tableWidget_matching_events->setItem(this->ui->tableWidget_matching_events->rowCount()-1, 2, eventRegion);
    }

    //Re-enable the sorting
    this->ui->tableWidget_matching_events->setSortingEnabled(true);

    //Update the matching events count
    QString countText = "Found " + QString::number(this->ui->tableWidget_matching_events->rowCount()) + " matching events";
    this->ui->label_matching_events_count->setText(countText);
}

void AwsLogAnalyser::updateStatisticsTable(){
    foreach(const AwsEvents::Statistic& statistic, this->m_statisticsResults){
        QTableWidgetItem* valueItem = new QTableWidgetItem();
        QTableWidgetItem* occurencesItem = new QTableWidgetItem();
        QProgressBar* percentageItem = new ProgressBar();
        valueItem->setText(statistic.value);
        occurencesItem->setText(QString::number(statistic.occurences));
        percentageItem->setMaximum(statistic.totalOccurences);
        percentageItem->setValue(statistic.occurences);
        percentageItem->setStyleSheet("QProgressBar { border: 2px solid grey; border-radius: 0px; text-align: center; } QProgressBar::chunk {background-color: #3add36; width: 1px;}");

        this->ui->tableWidget_statistics_results->insertRow(this->ui->tableWidget_statistics_results->rowCount());
        this->ui->tableWidget_statistics_results->setItem(this->ui->tableWidget_statistics_results->rowCount()-1, 0, valueItem);
        this->ui->tableWidget_statistics_results->setItem(this->ui->tableWidget_statistics_results->rowCount()-1, 1, occurencesItem);
        this->ui->tableWidget_statistics_results->setIndexWidget(this->ui->tableWidget_statistics_results->model()->index(this->ui->tableWidget_statistics_results->rowCount()-1, 2), percentageItem);
    }
}

void AwsLogAnalyser::updateErrorsTable(){
    this->ui->tableWidget_errors->setSortingEnabled(false);

    foreach(const AwsEvents::Error& error, this->m_errorsResults){
        QTableWidgetItem* eventIDItem = new QTableWidgetItem();
        QTableWidgetItem* errorCodeItem = new QTableWidgetItem();
        QTableWidgetItem* eventTimeItem = new QTableWidgetItem();
        QTableWidgetItem* eventSourceItem = new QTableWidgetItem();
        eventIDItem->setText(error.event.id);
        errorCodeItem->setText(error.code);
        eventTimeItem->setText(error.event.dateTime);
        eventSourceItem->setText(error.source);

        //Insert new row
        this->ui->tableWidget_errors->insertRow(this->ui->tableWidget_errors->rowCount());

        //Insert items in the new row
        this->ui->tableWidget_errors->setItem(this->ui->tableWidget_errors->rowCount()-1, 0, eventIDItem);
        this->ui->tableWidget_errors->setItem(this->ui->tableWidget_errors->rowCount()-1, 1, errorCodeItem);
        this->ui->tableWidget_errors->setItem(this->ui->tableWidget_errors->rowCount()-1, 2, eventTimeItem);
        this->ui->tableWidget_errors->setItem(this->ui->tableWidget_errors->rowCount()-1, 3, eventSourceItem);
    }

    this->ui->tableWidget_errors->setSortingEnabled(true);
}

void AwsLogAnalyser::clearMatchingEvents(){
    this->m_matchingEventsResults.clear();
    this->ui->textEdit_event_details->setText("");
    int rowCount = this->ui->tableWidget_matching_events->rowCount();
    if(rowCount > 0){
        for(unsigned int i = 0; i < rowCount; i++){
            this->ui->tableWidget_matching_events->removeRow(0);
        }
    }
}

void AwsLogAnalyser::clearErrorsResults(){
    this->m_errorsResults.clear();
    int rowCount = this->ui->tableWidget_errors->rowCount();
    if(rowCount > 0){
        for(unsigned int i = 0; i < rowCount; i++){
            this->ui->tableWidget_errors->removeRow(0);
        }
    }
}

void AwsLogAnalyser::clearStatisticsResults(){
    this->m_statisticsResults.clear();
    int rowCount = this->ui->tableWidget_statistics_results->rowCount();
    if(rowCount > 0){
        for(unsigned int i = 0; i < rowCount; i++){
            this->ui->tableWidget_statistics_results->removeRow(0);
        }
    }
}

void AwsLogAnalyser::updateAlerts(){
    this->m_suspiciousActivity = AwsEvents::findSuspiciousActivity(this->m_jsonFiles);

    //Present alerts on the GUI
    foreach(const QString& srcAddress, this->m_suspiciousActivity.keys()){
        QTreeWidget* tree = this->ui->treeWidget_alerts;

        QTreeWidgetItem* addressItem = new QTreeWidgetItem();
        addressItem->setText(0, srcAddress);
        tree->addTopLevelItem(addressItem);

        //Show Access Denied errors count
        QTreeWidgetItem* accessDeniedCountItem = new QTreeWidgetItem();
        accessDeniedCountItem->setText(0, "Access denied count");
        accessDeniedCountItem->setText(1, QString::number(this->m_suspiciousActivity[srcAddress].accessDeniedCount));
        addressItem->addChild(accessDeniedCountItem);

        //Show accessed user accounts
        if(this->m_suspiciousActivity[srcAddress].userAccountsAccessed.size() > 0){
            QTreeWidgetItem* accountsAccessedItem = new QTreeWidgetItem();
            accountsAccessedItem->setText(0,"User accounts accessed");
            addressItem->addChild(accountsAccessedItem);
            foreach(const QString& userAccount, this->m_suspiciousActivity[srcAddress].userAccountsAccessed){
                QTreeWidgetItem* accountAccessedItem = new QTreeWidgetItem();
                accountAccessedItem->setText(0, userAccount);
                accountsAccessedItem->addChild(accountAccessedItem);
            }
        }

        //Show user policy created
        if(this->m_suspiciousActivity[srcAddress].userPoliciesCreated.size() > 0){
            QTreeWidgetItem* userPolicyCreatedItem = new QTreeWidgetItem();
            userPolicyCreatedItem->setText(0,"User policy created");
            addressItem->addChild(userPolicyCreatedItem);
            foreach(const AwsEvents::UserPolicy& policy, this->m_suspiciousActivity[srcAddress].userPoliciesCreated){
                QTreeWidgetItem* userPolicyItem = new QTreeWidgetItem();
                userPolicyItem->setText(0, policy.name);
                userPolicyItem->setText(1, policy.document);
                userPolicyCreatedItem->addChild(userPolicyItem);
            }
        }
    }
}
//---------------------------------------------------------------//
