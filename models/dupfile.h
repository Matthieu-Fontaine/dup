#ifndef DUPFILE_H
#define DUPFILE_H

#include <QFileInfo>

class DupFile
{
public:
    DupFile(QFileInfo _file);

    const QStringList image_formats = {"jpg", "jpeg", "png", "bmp", "gif", "tiff", "webp", "raw", "heif"};
    const QStringList video_formats = {"mp4", "avi", "mkv", "mov", "flv", "wmv", "webm"};
    const QStringList document_formats = {"pdf", "doc", "docx", "txt", "rtf", "xls", "xlsx", "ppt", "pptx", "odt", "csv"};
    const QStringList audio_formats = {"mp3", "wav", "flac", "ogg", "aac", "m4a"};
    const QStringList archive_formats = {"zip", "rar", "7z", "tar", "gz"};

    enum class FileType {
        Image,
        Video,
        Document,
        Audio,
        Archive,
        Unknow
    };

    QFileInfo fileInfo;
    FileType fileType = FileType::Unknow;

    bool duplicateByFileName = false;
    bool duplicateBySizeAndDate = false;
    bool duplicateBySHA256 = false;

    std::vector<DupFile*> duplicates; // Liste des fichiers similaires à ce fichier

    QString calculateSHA256();

private:


};

#endif // DUPFILE_H
