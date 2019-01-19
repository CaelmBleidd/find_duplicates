#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QFileSystemModel>
#include <QCryptographicHash>
#include <QIODevice>
#include <QThread>
#include <QObject>


main_window::main_window(QWidget *parent) : QMainWindow(parent), thread(nullptr), ui(new Ui::MainWindow),
                                            searching_in_process(false), directory(true), search_has_ended(false)
{

    ui->setupUi(this);

    ui->progressBar->setValue(0);
    ui->progressBar->setMinimum(0);


    ui->listWidget->setStyleSheet("QListWidget { color: black }");

    ui->actionCancel->setEnabled(false);
    ui->actionClear_all->setDisabled(true);


    QCommonStyle style;

    connect(ui->actionShow_directory, &QPushButton::clicked, this, &main_window::select_directory);
    connect(ui->actionScan_directory, &QPushButton::clicked, this, &main_window::scan_directory);
    connect(ui->actionClear_all, &QPushButton::clicked, this, &main_window::clear_all_duplicates);
    connect(ui->actionGo_home, &QPushButton::clicked, this, &main_window::show_home);
    connect(ui->actionGo_back, &QPushButton::clicked, this, &main_window::return_to_folder);
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &main_window::on_double_clicked);
    connect(ui->actionCancel, &QPushButton::clicked, this, &main_window::cancel);

    show_directory(QDir::homePath());
}

main_window::~main_window() {}

void main_window::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return && ui->listWidget->selectedItems().size() > 0) {
        on_double_clicked();
        return;
    }

    if (event->key() == Qt::Key_Backspace) {
        return_to_folder();
        return;
    }
}

void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select directory for scanning", QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        main_window::show_directory(dir);
    }
}

void main_window::show_number_of_duplicates(double seconds) {
    QMessageBox box;
    box.setText(QString("%1 duplicates found in %2 seconds").arg(duplicates_number).arg(seconds));
    box.addButton(QMessageBox::Ok);
    box.exec();
    ui->actionCancel->setDisabled(true);
    ui->actionScan_directory->setDisabled(true);
    ui->actionGo_back->setEnabled(true);
    ui->actionGo_home->setEnabled(true);

    search_has_ended = true;
    searching_in_process = false;
    directory = false;
    if (duplicates_number == 0) {
        show_directory(QDir::currentPath());
        ui->actionScan_directory->setEnabled(true);
    }
}


void main_window::change_tree(qint64 size, QMap<QString, QVector<QString>> const &hashes, const QDir & directory) {
    for (auto hash: hashes.keys()) {
        if (hashes[hash].size() > 1) {
            ++duplicates_number;
            ui->listWidget->addItem(hash);
            _duplicates[hash] = hashes[hash];
        }
    }
}

void main_window::scanning_was_stopped() {
    ui->progressBar->setValue(0);
    ui->actionCancel->setDisabled(true);

    searching_in_process = false;
    search_has_ended = false;
    directory = true;

    show_directory(QDir::currentPath());
    _last_scanned_directory = "";
    ui->actionScan_directory->setEnabled(true);
    ui->actionGo_home->setEnabled(true);
    ui->actionGo_back->setEnabled(true);
}

//надо вынести
void main_window::scan_directory() {

    if ((_last_scanned_directory == QDir::currentPath() && accept_form("Do you really want to scan dir again?")) ||
         _last_scanned_directory != QDir::currentPath()) {

        _last_scanned_directory = QDir::currentPath();
        ui->listWidget->clear();
        duplicates_number = 0;

        HashThread* hash_thread = new HashThread(_last_scanned_directory);
        thread = new QThread();
        hash_thread->moveToThread(thread);

        ui->actionGo_home->setDisabled(true);
        ui->actionGo_back->setDisabled(true);

        qRegisterMetaType<QMap<QString, QVector<QString>>>("QMap<QString, QVector<QString>>");
        qRegisterMetaType<QDir>("QDir");

        connect(thread, SIGNAL(started()), hash_thread, SLOT(process()));
        connect(hash_thread, SIGNAL(update_timer(double)), this, SLOT(show_number_of_duplicates(double)));


        connect(hash_thread, SIGNAL(scanning_was_stopped()), this, SLOT(scanning_was_stopped()));
        connect(hash_thread, SIGNAL(add_to_tree(qint64, QMap<QString, QVector<QString>> const&, QDir const&)),
                this, SLOT(change_tree(qint64, QMap<QString, QVector<QString>> const&, QDir const&)));

        connect(hash_thread, SIGNAL(update_progress_bar()), this, SLOT(update_progress_value()));
        connect(hash_thread, SIGNAL(set_max_progress_value(int)), this, SLOT(change_max_progress_value(int)));

        connect(hash_thread, SIGNAL(finished()), thread, SLOT(quit()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        connect(hash_thread, SIGNAL(finished()), hash_thread, SLOT(deleteLater()));

        ui->progressBar->setValue(0);

        searching_in_process = true;
        search_has_ended = false;
        directory = false;

        ui->actionScan_directory->setEnabled(false);
        thread->start(); 
        ui->actionCancel->setEnabled(true);
   }
}



//надо вынести
void main_window::clear_all_duplicates() {
//    bool permission = accept_form("Do you really want to delete all the duplicates?");
//    if (permission) {
//        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
//            QTreeWidgetItem *parent = ui->treeWidget->topLevelItem(i);
//            for (int j = 1; j < parent->childCount(); ++j) {
//                QFile(parent->child(j)->text(0)).remove();
//            }
//        }
//        information_form("All duplicates has been removed");
//        show_directory(QDir::currentPath());
//    } else {
//        //do nothing
//    }
}

void main_window::show_directory(QString const &dir) {
    ui->listWidget->clear();

    ui->progressBar->setValue(0);
    ui->actionCancel->setDisabled(true);
    ui->actionGo_home->setEnabled(true);
    ui->actionGo_back->setEnabled(true);
    ui->actionScan_directory->setEnabled(true);

    directory = true;
    search_has_ended = false;
    searching_in_process = false;

    QDirIterator iter(dir, QDir::NoDot | QDir::AllEntries);
    QDir::setCurrent(dir);
    setWindowTitle(QString("Directory content - %1").arg(QDir::currentPath()));

    while (iter.hasNext()) {
        iter.next();
        ui->listWidget->addItem(QDir(QDir::currentPath()).relativeFilePath(iter.filePath()));
    }
}



void main_window::set_data(QTreeWidgetItem *item, const QString &path) {
    QDir file(QDir::currentPath());

    item->setText(0, file.relativeFilePath(path));
}

void main_window::change_max_progress_value(int value) {
    ui->progressBar->setMaximum(value);
}

void main_window::update_progress_value() {
    ui->progressBar->setValue(ui->progressBar->value() + 1);
}

void main_window::on_double_clicked() {
    if (search_has_ended && !directory) {
        QString hash = ui->listWidget->currentItem()->text();
        ui->listWidget->clear();
        for (auto &file: _duplicates[hash]) {
            ui->listWidget->addItem(file);
        }

        return;
    }
    QString line = QDir::currentPath() + QDir::separator() + ui->listWidget->currentItem()->text();
    if (QFileInfo(line).isDir()) {
        main_window::show_directory(line);
    }
}

void main_window::show_home() {
    show_directory(QDir::homePath());
}

void main_window::return_to_folder() {
    if (!directory) {
        ui->listWidget->clear();
        for (auto &hash: _duplicates.keys()) {
              ui->listWidget->addItem(hash);
        }
        return;
    }
    QDir::setCurrent("..");
    show_directory(QDir::currentPath());
}

bool main_window::accept_form(QString const& text) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Attention", text, QMessageBox::Yes | QMessageBox::No);
    return (reply == QMessageBox::Yes);
}

void main_window::information_form(QString const &text) {
    QMessageBox::information(this, QString::fromUtf8("Notice"), text);
}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}

void main_window::cancel() {
    if (thread != nullptr && searching_in_process)
        thread->requestInterruption();
}
