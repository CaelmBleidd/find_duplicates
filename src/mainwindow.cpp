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


main_window::main_window(QWidget *parent) : QMainWindow(parent), thread(nullptr), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    ui->progressBar->setValue(0);
    ui->progressBar->setMinimum(0);

    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    ui->actionCancel->setEnabled(false);

    ui->treeWidget->setSortingEnabled(true);

    QCommonStyle style;

    connect(ui->actionShow_directory, &QPushButton::clicked, this, &main_window::select_directory);
    connect(ui->actionScan_directory, &QPushButton::clicked, this, &main_window::scan_directory);
    connect(ui->actionClear_all, &QPushButton::clicked, this, &main_window::clear_all_duplicates);
    connect(ui->actionGo_home, &QPushButton::clicked, this, &main_window::show_home);
    connect(ui->actionChoose_directory, &QPushButton::clicked, this, &main_window::select_directory);
    connect(ui->actionGo_back, &QPushButton::clicked, this, &main_window::return_to_folder);
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem * , int)), this, SLOT(change_directory(QTreeWidgetItem * )));
    connect(ui->actionCancel, &QPushButton::clicked, this, &main_window::cancel);

    show_directory(QDir::homePath());
}

main_window::~main_window() {}

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
}


void main_window::change_tree(qint64 size, QMap<QString, QVector<QString>> hashes, const QDir & directory) {
    for (auto hash: hashes.keys()) {
        if (hashes[hash].size() > 1) {
            ++duplicates_number;
            QTreeWidgetItem *parent = new QTreeWidgetItem(ui->treeWidget);
            set_data(parent, *hash.begin(), size, directory.path());
            for (auto path: hashes[hash]) {
                QTreeWidgetItem *child = new QTreeWidgetItem();
                set_data(child, path, size, directory.path());
                parent->addChild(child);
            }
//            parent->sortChildren(3, Qt::SortOrder::AscendingOrder);
            ui->treeWidget->addTopLevelItem(parent);
        }
//    ui->treeWidget->sortItems(2, Qt::SortOrder::DescendingOrder);
    }
}

//надо вынести
void main_window::scan_directory() {

    if ((_last_scanned_directory == QDir::currentPath() && accept_form("Do you really want to scan dir again?")) ||
         _last_scanned_directory != QDir::currentPath()) {

        _last_scanned_directory = QDir::currentPath();
        ui->treeWidget->clear();
        duplicates_number = 0;

        HashThread* hash_thread = new HashThread(_last_scanned_directory);
        thread = new QThread();
        hash_thread->moveToThread(thread);

        ui->actionGo_home->setDisabled(true);

        qRegisterMetaType<QMap<QString, QVector<QString>>>("QMap<QString, QVector<QString>>");
        qRegisterMetaType<QDir>("QDir");

        connect(thread, SIGNAL(started()), hash_thread, SLOT(process()));
        connect(hash_thread, SIGNAL(update_timer(double)), this, SLOT(show_number_of_duplicates(double)));

        connect(hash_thread, SIGNAL(add_to_tree(qint64, QMap<QString, QVector<QString>> const&, QDir const&)),
                this, SLOT(change_tree(qint64, QMap<QString, QVector<QString>> const&, QDir const&)));

        connect(hash_thread, SIGNAL(finished()), thread, SLOT(quit()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        connect(hash_thread, SIGNAL(finished()), hash_thread, SLOT(deleteLater()));


        thread->start();
        ui->actionCancel->setEnabled(true);
        ui->actionScan_directory->setDisabled(true);

   }
}

//надо вынести
void main_window::clear_all_duplicates() {
    bool permission = accept_form("Do you really want to delete all the duplicates?");
    if (permission) {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem *parent = ui->treeWidget->topLevelItem(i);
            for (int j = 1; j < parent->childCount(); ++j) {
                QFile(parent->child(j)->text(1)).remove();
            }
        }
        information_form("All duplicates has been removed");
        show_directory(QDir::currentPath());
    } else {
        //do nothing
    }
}

void main_window::show_directory(QString const &dir) {
    ui->treeWidget->clear();

    QDirIterator iter(dir, QDir::NoDot | QDir::AllEntries);
    QDir::setCurrent(dir);
    setWindowTitle(QString("Directory content - %1").arg(QDir::currentPath()));

    while (iter.hasNext()) {
        iter.next();
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
        set_data(item, iter.filePath(), iter.fileInfo().size(), iter.fileName());
    }
    ui->treeWidget->sortItems(0, Qt::SortOrder::AscendingOrder);
}



//что-то с этим делать нужно, это не ок
void main_window::set_data(QTreeWidgetItem *item, const QString &path, qint64 size, QString const& directory) {
    QFileInfo file(path);

    item->setTextColor(0, Qt::black);
    item->setTextColor(1, Qt::gray);
    item->setTextColor(2, Qt::red);
    item->setTextColor(3, Qt::blue);


    item->setText(0, file.fileName());
    item->setText(1, directory);
    item->setData(2, Qt::DisplayRole, size);
//    item->setData(3, Qt::DisplayRole, file.lastModified());
}

void main_window::change_directory(QTreeWidgetItem *item) {
    QString line = QDir::currentPath() + '/' + item->text(0);
    if (QFileInfo(line).isDir()) {
        main_window::show_directory(line);
    }
}

void main_window::show_home() {
    show_directory(QDir::homePath());
}

void main_window::return_to_folder() {
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
    if (thread != nullptr)
        thread->requestInterruption();
}
