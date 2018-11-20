/*
 * TODO:
 *  - ReadAll -> read()
 *  - Fix resize column
 *  - Add write to file
 *  - Change hash-function
 *  - Add enter-pressed support
 *  - Add navigation
 *  - Add multithread
 *  - Add progress-bar
 * */


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

main_window::main_window(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->treeWidget->setUniformRowHeights(true);

    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    ui->treeWidget->setSortingEnabled(true);

    QCommonStyle style;
    ui->actionShow_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogResetButton));
    ui->actionHome->setIcon(style.standardIcon(QCommonStyle::SP_DirHomeIcon));
    ui->actionClear_All->setIcon(style.standardIcon(QCommonStyle::SP_TrashIcon));
    ui->actionReturn->setIcon(style.standardIcon(QCommonStyle::SP_FileDialogBack));


    connect(ui->actionShow_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::scan_directory);
    connect(ui->actionClear_All, &QAction::triggered, this, &main_window::clear_all_duplicates);
    connect(ui->actionHome, &QAction::triggered, this, &main_window::show_home);
    connect(ui->actionReturn, &QAction::triggered, this, &main_window::return_to_folder);
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem * , int)), this,
            SLOT(change_directory(QTreeWidgetItem * )));

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

void main_window::scan_directory() {

    if ((_last_scanned_directory == QDir::currentPath() && accept_form("Are you really want to scan dir again?")) ||
         _last_scanned_directory != QDir::currentPath()) {

        _last_scanned_directory = QDir::currentPath();

        ui->treeWidget->clear();
        main_window::_duplicates.clear();
        main_window::_files.clear();

        setWindowTitle(QString("Result of scanning - %1").arg(QDir::currentPath()));

        auto dir = QDir::current();
        dir.setFilter(QDir::Hidden | QDir::NoDotAndDotDot | QDir::AllEntries | QDir::NoSymLinks);
        find_suspects(dir.path());
        find_duplicates();



        qint64 count_duplicates = 0;
        for (auto paths : _duplicates) {
            if (paths.size() > 1) {
                ++count_duplicates;
                QTreeWidgetItem *parent = new QTreeWidgetItem(ui->treeWidget);

                set_data(parent, *paths.begin());

                for (auto path: paths) {
                    QTreeWidgetItem *child = new QTreeWidgetItem();
                    set_data(child, path);
                    parent->addChild(child);
                }
                parent->sortChildren(3, Qt::SortOrder::AscendingOrder);
                ui->treeWidget->addTopLevelItem(parent);
            }
        }
        ui->treeWidget->sortItems(2, Qt::SortOrder::DescendingOrder);

        if (count_duplicates == 0) {
            information_form("There're no duplicates in the folder");
            show_directory(QDir::currentPath());
        } else {
            information_form(QString("%1 duplicates found").arg(count_duplicates));
        }

   }
}

void main_window::clear_all_duplicates() {
    bool permission = accept_form("Are you really want to delete all the duplicates?");
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
    _files.clear();
    _duplicates.clear();

    QDirIterator iter(dir, QDir::NoDot | QDir::AllEntries);
    QDir::setCurrent(dir);
    setWindowTitle(QString("Directory content - %1").arg(QDir::currentPath()));

    while (iter.hasNext()) {
        iter.next();
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
        set_data(item, iter.filePath());
    }
    ui->treeWidget->sortItems(0, Qt::SortOrder::AscendingOrder);
}

void main_window::find_duplicates() {
    for (auto paths: _files) {
        if (paths.size() > 1) {
            for (auto path : paths) {
                QCryptographicHash crypto(QCryptographicHash::Sha1);
                QFile file(path);
                file.open(QFile::ReadOnly);
                while(!file.atEnd()) {
                    crypto.addData(file.read(8192));
                }
                QByteArray hash = crypto.result();
                QString string_hash = hash.toHex().data();
                _duplicates[string_hash].push_back(path);
            }
        }
    }
}

void main_window::set_data(QTreeWidgetItem *item, const QString &path) {
    QFileInfo file(path);

    item->setTextColor(0, Qt::black);
    item->setTextColor(1, Qt::gray);
    item->setTextColor(2, Qt::red);
    item->setTextColor(3, Qt::blue);


    item->setText(0, file.fileName());
    item->setText(1, file.filePath());
    item->setData(2, Qt::DisplayRole, file.size());
    item->setData(3, Qt::DisplayRole, file.lastModified());
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

