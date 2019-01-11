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

class HashThread : public QObject {
    Q_OBJECT
public:
    HashThread(QString const&);
    ~HashThread();

private:
    QMap<qint64, QVector<QString>> find_suspects(QDir const&);
    QString _dir_name;

public slots:
    void process();

signals:
    void finished();
    void add_to_tree(qint64, QMap<QString, QVector<QString>> const&, QDir const&);
    void update_timer(double);
};

#endif // HASHTHREAD_H
