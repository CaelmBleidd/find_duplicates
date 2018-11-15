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
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    QCommonStyle style;
    ui->actionShow_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogResetButton));

    connect(ui->actionShow_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::scan_directory);
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(change_directory(QTreeWidgetItem*)));

    show_directory(QDir::homePath());
}

main_window::~main_window() {
    files.clear();
    duplicates.clear();
}

void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select directory for scanning", QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        main_window::show_directory(dir);
    }
}



void main_window::show_directory(QString const& dir) {
    ui->treeWidget->clear();
    setWindowTitle(QString("Directory content - %1").arg(dir));
    QDir d(dir);
    QDir::setCurrent(dir);
    QFileInfoList list = d.entryInfoList(QDir::NoDot | QDir::AllEntries);
    for (QFileInfo file_info: list) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);

        item->setTextColor(0, Qt::black);
        item->setTextColor(1, Qt::gray);
        item->setTextColor(2, Qt::red);
        item->setTextColor(3, Qt::blue);

        item->setText(0, file_info.fileName());
        item->setText(1, file_info.filePath());
        item->setText(2, QString::number(file_info.size()));
        item->setText(3, file_info.lastModified().toString("hh:mm:ss dd.MM.yyyy"));

        ui->treeWidget->addTopLevelItem(item);
    }
}

void main_window::scan_directory()
{
    ui->treeWidget->clear();
    main_window::duplicates.clear();
    main_window::files.clear();

    setWindowTitle(QString("Result of scanning - %1").arg(QDir::currentPath()));

    auto dir = QDir::current();
    dir.setFilter(QDir::Hidden | QDir::NoDotAndDotDot | QDir::AllEntries | QDir::NoSymLinks);
    find_suspects(dir);
    find_duplicates();
    int i = 0;

    QFile text("/Users/CaelmBleidd/Programming/duplicate_utility/duplicate_utility/output.txt");
        text.open(QIODevice::ReadWrite);
        QTextStream stream( &text );

    for (auto paths : duplicates) {
        if (paths.size() > 1) {
            i += paths.size();
        for (auto path: paths) {
              QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);

              QFileInfo file(path);

              item->setTextColor(0, Qt::black);
              item->setTextColor(1, Qt::gray);
              item->setTextColor(2, Qt::red);
              item->setTextColor(3, Qt::blue);


              item->setText(0, file.fileName());
              item->setText(1, file.filePath());
              item->setText(2, QString::number(file.size()));
              item->setText(3, file.lastModified().toString("hh:mm:ss dd.MM.yyyy"));



              stream << file.absoluteFilePath()  << endl;
              stream.flush();


              ui->treeWidget->addTopLevelItem(item);


         }
        }
    }
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);

    item->setText(2, QString::number(i));

    ui->treeWidget->addTopLevelItem(item);
    text.close();
}

void main_window::find_duplicates() {
    for (auto paths: files) {
        if (paths.size() > 1) {

            for (auto path : paths) {
                QCryptographicHash crypto(QCryptographicHash::Sha1);
                QFile file(path);
                file.open(QFile::ReadOnly);
                while(!file.atEnd()){
                  crypto.addData(file.read(8192));
                }
                QByteArray hash = crypto.result();
                QString string_hash = crypto.result().toHex().data();
                duplicates[string_hash].push_back(path);
            }
        }
    }
}

void main_window::find_suspects(QDir const &dir)
{
    QFileInfoList list =  dir.entryInfoList();
    for (QFileInfo file_info: list) {
        if (file_info.isFile()) {
            files[file_info.size()].push_back(file_info.absoluteFilePath());
        } else {
            auto dire = QDir(file_info.filePath());
            dire.setFilter(QDir::Hidden | QDir::NoDotAndDotDot | QDir::AllEntries | QDir::NoSymLinks);
            find_suspects(dire);
        }
    }
}

void main_window::change_directory(QTreeWidgetItem* item) {
    QString line = QDir::currentPath() + '/' + item->text(0);
    if (QFileInfo(line).isDir()) {
        main_window::show_directory(line);
    }
}


void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
