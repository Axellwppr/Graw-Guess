#include <QApplication>

#include "mainwindow.h"
int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    MainWindow w; //主窗体入口
    w.show();
    return a.exec();
}
