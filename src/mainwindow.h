#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QDir>
#include <QWidgetItem>
#include <QMap>
#include <QTreeWidget>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include "hashthread.h"

namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();
    std::atomic_bool searching_in_process;

private slots:
    void select_directory();
    void scan_directory();
    void clear_all_duplicates();
    void show_directory(QString const &dir);
    void change_directory(QTreeWidgetItem *item);
    void return_to_folder();
    void show_home();
    void show_about_dialog();
    void show_number_of_duplicates(double);
    void cancel();

public slots:
    void change_tree(qint64, QMap<QString, QVector<QString>> const&, QDir const&);
    void change_max_progress_value(int);
    void update_progress_value();
    void scanning_was_stopped();


private:
    void set_data(QTreeWidgetItem *item, QString const &path);
    void information_form(QString const &text);
    bool accept_form(QString const &text);
    QThread *thread;


    std::unique_ptr<Ui::MainWindow> ui;
    QThread* hashThread;

    QString _last_scanned_directory;

    qint64 duplicates_number = 0;
};

#endif // MAINWINDOW_H
