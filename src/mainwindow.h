#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QDir>
#include <QWidgetItem>
#include <QMap>
#include <QTreeWidget>

namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();

private slots:
    void select_directory();
    void scan_directory();
    void clear_all_duplicates();
    void show_directory(QString const &dir);
    void change_directory(QTreeWidgetItem *item);
    void return_to_folder();
    void show_home();
    void show_about_dialog();

private:
    void find_suspects(QString const &dir);
    void find_duplicates();
    void set_data(QTreeWidgetItem *item, QString const &path);
    void information_form(QString const &text);
    bool accept_form();

    std::unique_ptr<Ui::MainWindow> ui;

    QMap<QString, QVector<QString>>  _duplicates;
    QMap<qlonglong, QVector<QString>> _files;

    QString _last_scanned_directory;
};

#endif // MAINWINDOW_H
