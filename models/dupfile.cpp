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

quint64 DupFile::calculatePHash() {
    // Charger l'image
    QImage image(fileInfo.filePath());
    if (image.isNull()) {
        qWarning() << "Impossible de charger l'image :" << fileInfo.filePath();
        return 0;  // Retourner 0 si l'image ne peut pas être chargée
    }

    // Convertir l'image en niveaux de gris pour simplifier la comparaison
    image = image.convertToFormat(QImage::Format_Grayscale8);

    // Redimensionner l'image pour une comparaison plus rapide et fiable
    image = image.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Convertir l'image en une série de bits
    QByteArray byteArray;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            // Obtenir l'intensité de gris du pixel
            int pixelValue = qGray(image.pixel(x, y));
            byteArray.append(static_cast<char>(pixelValue > 128 ? 1 : 0)); // 1 pour pixel clair, 0 pour pixel sombre
        }
    }

    // Calculer le perceptual hash comme un hash de ces bits
    quint64 pHash = 0;
    for (int i = 0; i < byteArray.size(); ++i) {
        pHash |= (static_cast<quint64>(byteArray[i]) << (i * 8));
    }

    return pHash;  // Retourner le pHash sous forme d'un entier (64 bits)
}
