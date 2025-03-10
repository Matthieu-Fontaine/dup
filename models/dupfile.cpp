#include "dupfile.h"

#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

DupFile::DupFile(QFileInfo _file) {
    fileInfo = _file;
    QString extension = fileInfo.suffix().toLower();

    if (image_formats.contains(extension, Qt::CaseInsensitive)) {
        fileType = FileType::Image;
    } else if (video_formats.contains(extension, Qt::CaseInsensitive)) {
        fileType = FileType::Video;
    } else if (document_formats.contains(extension, Qt::CaseInsensitive)) {
        fileType = FileType::Document;
    } else if (audio_formats.contains(extension, Qt::CaseInsensitive)) {
        fileType = FileType::Audio;
    } else if (archive_formats.contains(extension, Qt::CaseInsensitive)) {
        fileType = FileType::Archive;
    } else {
        fileType = FileType::Unknow;
    }
}

QString DupFile::calculateSHA256() {
    QFile file(fileInfo.filePath());
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Impossible d'ouvrir le fichier :" << fileInfo.filePath();
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        qWarning() << "Erreur lors de la lecture du fichier :" << fileInfo.filePath();
        return QString();
    }

    return hash.result().toHex();  // Retourne le hash sous forme hexadécimale
}
