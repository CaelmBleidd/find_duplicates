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

//private signals:


private:
    void find_suspects(QDir const& dir);
    void find_duplicates();

    std::unique_ptr<Ui::MainWindow> ui;

    QMap<QString, QVector<QString>>  duplicates;
    QMap<qlonglong, QVector<QString>> files;

};

#endif // MAINWINDOW_H