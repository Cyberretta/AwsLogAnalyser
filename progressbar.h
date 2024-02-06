#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QProgressBar>
#include <QObject>

class ProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    ProgressBar(QWidget* parent = nullptr);

    virtual QString text() const Q_DECL_OVERRIDE;
};

#endif // PROGRESSBAR_H
