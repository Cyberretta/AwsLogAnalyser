#include "AwsLogAnalyser.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AwsLogAnalyser w;
    w.show();
    return a.exec();
}
