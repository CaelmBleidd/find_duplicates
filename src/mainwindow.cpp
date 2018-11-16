#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QFileSystemModel>
#include <QWidgetItem>
#include <QCryptographicHash>
#include <qcryptographichash.h>
#include <QCryptographicHash>
#include <QTextStream>
#include <QIODevice>
#include <QDir>

main_window::main_window(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));



    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    ui->treeWidget->setSortingEnabled(true);

    QCommonStyle style;
    ui->actionShow_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogResetButton));
    ui->actionHome->setIcon(style.standardIcon(QCommonStyle::SP_DirHomeIcon));


    connect(ui->actionShow_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::scan_directory);
    connect(ui->actionHome, &QAction::triggered, this, &main_window::show_home);
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
            SLOT(change_directory(QTreeWidgetItem *)));

    show_directory(QDir::homePath());
}

main_window::~main_window() {
    _files.clear();
    _duplicates.clear();
}

void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select directory for scanning", QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        main_window::show_directory(dir);
    }
}


void main_window::show_directory(QString const &dir) {
    ui->treeWidget->clear();
    _files.clear();
    _duplicates.clear();

    setWindowTitle(QString("Directory content - %1").arg(dir));
    QDirIterator iter(dir, QDir::NoDot | QDir::AllEntries);
    QDir::setCurrent(dir);

    while (iter.hasNext()) {
        iter.next();
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        set_data(item, iter.filePath());
    }
    ui->treeWidget->sortItems(0, Qt::SortOrder::AscendingOrder);
}

void main_window::scan_directory() {
    if (last_scanned_directory != QDir::currentPath()) {
        last_scanned_directory = QDir::currentPath();

        ui->treeWidget->clear();
        main_window::_duplicates.clear();
        main_window::_files.clear();

        setWindowTitle(QString("Result of scanning - %1").arg(QDir::currentPath()));

        auto dir = QDir::current();
        dir.setFilter(QDir::Hidden | QDir::NoDotAndDotDot | QDir::AllEntries | QDir::NoSymLinks);
        find_suspects(dir.path());
        find_duplicates();

        bool are_there_duplicates = false;
        for (auto paths : _duplicates) {
            if (paths.size() > 1) {
                are_there_duplicates = true;
                QTreeWidgetItem* parent =  new QTreeWidgetItem(ui->treeWidget);

                set_data(parent, *paths.begin());

                for (auto path: paths) {
                    QTreeWidgetItem* child = new QTreeWidgetItem();
                    set_data(child, path);
                    parent->addChild(child);
//                    drow_data(path);
                }
                ui->treeWidget->addTopLevelItem(parent);
            }
        }
        ui->treeWidget->sortItems(2, Qt::SortOrder::DescendingOrder);

        if (!are_there_duplicates) {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            item->setText(0, "No duplicates found");
            item->setTextColor(0, Qt::black);
            ui->treeWidget->addTopLevelItem(item);
        }
    }
}

void main_window::find_duplicates() {
    for (auto paths: _files) {
        if (paths.size() > 1) {

            for (auto path : paths) {
                QCryptographicHash crypto(QCryptographicHash::Md5);
                QFile file(path);
                file.open(QFile::ReadOnly);
                while (!file.atEnd()) {
                    crypto.addData(file.read(8192));
                }
                QByteArray hash = crypto.result();
                QString string_hash = crypto.result().toHex().data();
                _duplicates[string_hash].push_back(path);
            }
        }
    }
}

void main_window::set_data(QTreeWidgetItem* item, const QString &path) {

    QFileInfo file(path);

    item->setTextColor(0, Qt::black);
    item->setTextColor(1, Qt::gray);
    item->setTextColor(2, Qt::red);
    item->setTextColor(3, Qt::blue);


    item->setText(0, file.fileName());
    item->setText(1, file.filePath());
    item->setData(2, Qt::DisplayRole, file.size());
    item->setData(3, Qt::DisplayRole, file.lastModified());

//    ui->treeWidget->addTopLevelItem(item);
}

void main_window::find_suspects(QString const &dir) {
    QDirIterator iter(dir, QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks | QDir::AllEntries,
                      QDirIterator::Subdirectories);

    while (iter.hasNext()) {
        iter.next();
        auto file_info = iter.fileInfo();
        if (file_info.isFile()) {
            _files[file_info.size()].push_back(file_info.absoluteFilePath());
        }
    }
}

void main_window::change_directory(QTreeWidgetItem *item) {
    QString line = QDir::currentPath() + '/' + item->text(0);
    if (QFileInfo(line).isDir()) {
        main_window::show_directory(line);
    }
}

void main_window::show_home()
{
    show_directory(QDir::homePath());
}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}

