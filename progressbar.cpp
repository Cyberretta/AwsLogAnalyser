#include "progressbar.h"

ProgressBar::ProgressBar(QWidget *parent) : QProgressBar(parent){

}

QString ProgressBar::text() const{
    if (minimum() == maximum()) // divide by zero guard
        return QString();

    double percent = 100.0 * (value() - minimum()) / (maximum() - minimum());
    return QString("%1%").arg(percent, 0, 'f', 2);
}
