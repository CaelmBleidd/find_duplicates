#include "mainwindow.h"
#include <QApplication>
#include <cstdio>

#define QT_NO_DEBUG_OUTPUT

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    main_window w;
    w.show();

    return a.exec();
}
