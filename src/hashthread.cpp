#include "hashthread.h"

HashThread::HashThread(QString const &dir_name): _dir_name(dir_name) {}

HashThread::~HashThread() {}

QMap<qint64, QVector<QString>> HashThread::find_suspects(QDir const &directory) {

    QMap<qint64, QVector<QString>> grouped_files;

    QDirIterator iter(directory.path(),
                      QDir::NoDotAndDotDot
                      | QDir::Hidden
                      | QDir::NoSymLinks
                      | QDir::AllEntries,
                      QDirIterator::Subdirectories);

    while (iter.hasNext()) {
        iter.next();
        auto file_info = iter.fileInfo();
        if (file_info.isFile()) {
            auto size = file_info.size();
            grouped_files[size].push_back(file_info.absoluteFilePath());
        }
        if (QThread::currentThread()->isInterruptionRequested())  break;
    }
    return grouped_files;
}

void add_to_result(QMap<QString, QVector<QString>> &hashes, QPair<QString, QString> const &info) {
    hashes[info.second].push_back(info.first);
}


QPair<QString, QString> calculateHash(const QString &path) {
    QCryptographicHash crypto(QCryptographicHash::Sha1);
    QFile file(path);
    file.open(QFile::ReadOnly);
    while(!file.atEnd()) {
        crypto.addData(file.read(4096));
    }
    QByteArray hash = crypto.result();
    QString string_hash = hash.toHex().data();
    return {path, string_hash};
}

void HashThread::process() {
    QDir directory(_dir_name);
    if (!directory.exists()) {
        QMessageBox error;
        error.setText("Choosen directory doesn't exist");
        error.show();
    } else {
        QTime timer;
        timer.start();

        auto grouped_files = find_suspects(directory);

        emit set_max_progress_value(grouped_files.keys().size());

        for (auto size: grouped_files.keys()) {


            if (QThread::currentThread()->isInterruptionRequested()) {
                emit scanning_was_stopped();
                emit finished();
                return;
            }

            auto suspects = grouped_files[size];
            if (suspects.size() == 1) {
                emit update_progress_bar();
                continue;
            }

            QFuture<QMap<QString, QVector<QString>>> future = QtConcurrent::mappedReduced(suspects, calculateHash, add_to_result);
            future.waitForFinished();

            auto hashes = future.result();

            emit update_progress_bar();
            emit add_to_tree(size, hashes, directory);

        }
        emit update_timer(timer.elapsed() / 1000.0);
    }

    emit finished();
}
