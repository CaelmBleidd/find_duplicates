#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QDir>
#include <QWidgetItem>
#include <QMap>
#include <qtreewidget>

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
    void show_about_dialog();
    void show_directory(QString const& dir);
    void change_directory(QTreeWidgetItem* item);
    void show_home();

private:
    void find_suspects(QString const& dir);
    void find_duplicates();
    void set_data(QTreeWidgetItem* item, QString const& path);

    std::unique_ptr<Ui::MainWindow> ui;

    QMap<QString, QVector<QString>>  _duplicates;
    QMap<qlonglong, QVector<QString>> _files;

    QString last_scanned_directory;
};

#endif // MAINWINDOW_H
