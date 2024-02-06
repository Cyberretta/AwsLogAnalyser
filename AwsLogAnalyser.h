#ifndef AWSLOGANALYSER_H
#define AWSLOGANALYSER_H

#include <QWidget>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTableWidgetItem>
#include <QCompleter>
#include <QFileDialog>
#include <QJsonArray>
#include <QMessageBox>
#include <QStringListModel>
#include <QMap>
#include <QTreeWidget>
#include <QHeaderView>
#include "progressbar.h"
#include "AwsEvents.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AwsLogAnalyser; }
QT_END_NAMESPACE

class AwsLogAnalyser : public QWidget
{
    Q_OBJECT

public:
    AwsLogAnalyser(QWidget *parent = nullptr);
    ~AwsLogAnalyser();

private slots:
    void on_pushButton_search_clicked();
    void on_pushButton_addfilter_clicked();
    void on_pushButton_removefilters_clicked();
    void on_tableWidget_matching_events_cellDoubleClicked(int row, int column);
    void on_pushButton_get_statistics_clicked();
    void on_checkBox_time_from_stateChanged(int arg1);
    void on_checkBox_time_to_stateChanged(int arg1);
    void on_lineEdit_addfilter_key_editingFinished();
    void on_comboBox_filter_operation_currentIndexChanged(int index);
    void on_radioButton_errors_scope_matching_clicked();
    void on_radioButton_errors_scope_all_clicked();
    void on_tableWidget_errors_cellDoubleClicked(int row, int column);
    void on_pushButton_browse_clicked();

private:
    Ui::AwsLogAnalyser *ui;

    QString m_logsPath;
    QStringList m_jsonFiles;
    AwsEvents::Dictionary m_dictionnary;

    //Members used to store results
    std::vector<AwsEvents::Event> m_matchingEventsResults;
    std::vector<AwsEvents::Error> m_errorsResults;
    std::vector<AwsEvents::Statistic> m_statisticsResults;
    std::vector<AwsEvents::Filter> m_filters;
    QMap<QString, AwsEvents::SuspiciousActivity> m_suspiciousActivity;

    //Private methods
    void newProject();
    void showErrorMessage(QString errorMessage);
    void updateValuesCompleter();

    //Clearing related methods
    void clearErrorsResults();
    void clearStatisticsResults();
    void clearMatchingEvents();

    //Update related methods
    void updateErrorsTable();
    void updateStatisticsTable();
    void updateMatchingEventsTable();
    void updateAlerts();
};
#endif // AWSLOGANALYSER_H
