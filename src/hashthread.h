#ifndef HASHTHREAD_H
#define HASHTHREAD_H

#include <QObject>
#include <QMainWindow>
#include <QDir>
#include <QMessageBox>
#include <QTime>
#include <QDirIterator>
#include <QMetaType>
#include <QThread>
#include <QMap>
#include <QVector>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <math.h>

class HashThread
{
public:
    HashThread(QString const &dir): dir_name(dir) {}
    ~HashThread();

private:
    void find_suspects(QString const &dir);
    void find_duplicates();
    void scan_directory(QString const &dir);
    QString dir_name;

public slots:
    void process();

signals:

};

#endif // HASHTHREAD_H
