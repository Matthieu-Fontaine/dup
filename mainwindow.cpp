#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDirIterator>

#include <QtConcurrent/QtConcurrentMap>
#include <QFuture>
#include <QMutex>

#include <vector>

#include <models/dupfile.h>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("dup");
    setFixedSize(240, 641);

    connect(ui->addFolderPushButton, &QPushButton::released, this, &MainWindow::selectFolderPushButton);
    connect(ui->findDuplicatePushButton, &QPushButton::released, this, &MainWindow::findDuplicatePushButton);
    connect(ui->deleteDuplicatePushButton, &QPushButton::released, this, &MainWindow::deleteDuplicatePushButton);
    connect(ui->deleteSelectedPushButton, &QPushButton::released, this, &MainWindow::deleteSelectedPushButton);

    connect(ui->optionsImageCheckBox, &QCheckBox::toggled, this, &MainWindow::onOptionsCheckBoxStateChanged);
    connect(ui->optionsVideoCheckBox, &QCheckBox::toggled, this, &MainWindow::onOptionsCheckBoxStateChanged);
    connect(ui->optionsDocumentCheckBox, &QCheckBox::toggled, this, &MainWindow::onOptionsCheckBoxStateChanged);
    connect(ui->optionsAudioCheckBox, &QCheckBox::toggled, this, &MainWindow::onOptionsCheckBoxStateChanged);
    connect(ui->optionsArchiveCheckBox, &QCheckBox::toggled, this, &MainWindow::onOptionsCheckBoxStateChanged);
    connect(ui->optionsOtherCheckBox, &QCheckBox::toggled, this, &MainWindow::onOptionsCheckBoxStateChanged);

    ui->filterSHA256MD5CheckBox->setToolTip("Plus long, très précis");

    // Taille des fichiers qui vont etre traiter -> temp estimer
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::selectFolderPushButton() {
    addUserLog("Selection d'un dossier");

    QString folder_path = QFileDialog::getExistingDirectory(nullptr, "Sélectionner un dossier");

    if (folder_path.isEmpty()) {
        addUserLog("Annulation");
        return; // Si l'utilisateur annule, ne rien faire
    }

    // Création d'un itérateur pour parcourir le dossier récursivement
    QDirIterator it(folder_path, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        dup_files.push_back(DupFile(QFileInfo(it.next())));
    }

    int imagesCount = 0;
    int videosCount = 0;
    int documentsCount = 0;
    int audiosCount = 0;
    int archivesCount = 0;
    int othersCount = 0;

    for(const auto& file : dup_files) {
        switch (file.fileType) {
        case DupFile::FileType::Image:
            imagesCount++;
            break;
        case DupFile::FileType::Video:
            videosCount++;
            break;
        case DupFile::FileType::Document:
            documentsCount++;
            break;
        case DupFile::FileType::Audio:
            audiosCount++;
            break;
        case DupFile::FileType::Archive:
            archivesCount++;
            break;
        case DupFile::FileType::Unknow:
            othersCount++;
            break;
        }

    }

    ui->numberOfImageFilesValueLabel->setText(QString::number(imagesCount));
    ui->numberOfVideoFilesValueLabel->setText(QString::number(videosCount));
    ui->numberOfDocumentFilesValueLabel->setText(QString::number(documentsCount));
    ui->numberOfAudioFilesValueLabel->setText(QString::number(audiosCount));
    ui->numberOfArchiveFilesValueLabel->setText(QString::number(archivesCount));
    ui->numberOfOtherFilesValueLabel->setText(QString::number(othersCount));

}

void MainWindow::findDuplicatePushButton() {
    onOptionsCheckBoxStateChanged(); // TODO A CHANGER

    int activeFilters = countActiveFilters();

    maxSizeProgressBar = activeFilters * static_cast<int>(filteredFiles.size());

    ui->filterProgressBar->setMaximum(maxSizeProgressBar);

    if(ui->filterNameCheckBox->isChecked()) {
        findDuplicateFileName(filteredFiles);
    }

    if(ui->filterSHA256MD5CheckBox->isChecked()) {
        findDuplicateSHA256(filteredFiles);
    }

    if(ui->filterSizeAndDateCheckBox->isChecked()) {
        findDuplicateSizeAndDate(filteredFiles);
    }

    for (const auto &file : filteredFiles) {
        int duplicateCount = 0;

        if (file.duplicateByFileName) duplicateCount++;
        if (file.duplicateBySHA256) duplicateCount++;
        if (file.duplicateBySizeAndDate) duplicateCount++;
        if (file.duplicateByPHash) duplicateCount++;

        if (duplicateCount == activeFilters) {
            qDebug() << "Image en double selon tous les filtres :" << file.fileInfo.filePath();
        }
    }

    ui->filterProgressBar->setValue(maxSizeProgressBar);
    QCoreApplication::processEvents();

    afficherDoublons(filteredFiles);
}

void MainWindow::deleteDuplicatePushButton() {
    qDebug() << "Delete";
}

std::vector<DupFile> MainWindow::filterFiles(const std::vector<DupFile> &files) {
    std::vector<DupFile> out_files;

    for (const auto &file : files) {
        // Vérifier les options cochées et les ajoutes dans un second tableau
        if (ui->optionsImageCheckBox->isChecked() && file.fileType == DupFile::FileType::Image) {
            out_files.push_back(file);
        } else if (ui->optionsVideoCheckBox->isChecked() && file.fileType == DupFile::FileType::Video) {
            out_files.push_back(file);
        } else if (ui->optionsDocumentCheckBox->isChecked() && file.fileType == DupFile::FileType::Document) {
            out_files.push_back(file);
        } else if (ui->optionsAudioCheckBox->isChecked() && file.fileType == DupFile::FileType::Audio) {
            out_files.push_back(file);
        } if (ui->optionsArchiveCheckBox->isChecked() && file.fileType == DupFile::FileType::Archive) {
            out_files.push_back(file);
        }
    }

    return out_files;
}

void MainWindow::findDuplicateFileName(std::vector<DupFile> &files) {
    addUserLog("Recherche des doublons par nom...");

    std::unordered_map<QString, DupFile*> uniqueFiles;  // Associe un nom de fichier à son premier fichier unique
    size_t numberOfDuplicate = 0;

    for (auto &file : files) {
        QString fileName = file.fileInfo.fileName().trimmed().toLower();

        if (uniqueFiles.find(fileName) == uniqueFiles.end()) {
            // Si le nom n'existe pas encore, c'est un fichier unique
            uniqueFiles[fileName] = &file;
        } else {
            // Doublon détecté, on l'associe au fichier original
            file.duplicateByFileName = true;
            uniqueFiles[fileName]->duplicates.push_back(&file);
            numberOfDuplicate++;
        }
        //     ui->filterProgressBar->setValue(0);

    }

    addUserLog(QString::number(numberOfDuplicate) + " doublons par nom.");
}

void MainWindow::findDuplicateSHA256(std::vector<DupFile> &files) {
    addUserLog("Recherche des doublons SHA-256...");

    // Structure thread-safe pour stocker les fichiers uniques par hachage SHA-256
    QHash<QString, DupFile*> uniqueFileHash;
    QMutex mutex; // Pour synchroniser l'accès à uniqueFileHash

    // Fonction pour traiter un fichier
    auto processFile = [&](DupFile &file) {
        QString fileHash = file.calculateSHA256();

        if (fileHash.isEmpty()) {
            return; // Si erreur, on ignore ce fichier
        }

        QMutexLocker locker(&mutex); // Verrouiller l'accès à uniqueFileHash

        auto it = uniqueFileHash.find(fileHash);
        if (it == uniqueFileHash.end()) {
            // SHA-256 unique → On l'ajoute comme référence
            uniqueFileHash[fileHash] = &file;
        } else {
            // Doublon trouvé → On l'ajoute à la liste des doublons
            file.duplicateBySHA256 = true;
            it.value()->duplicates.push_back(&file);
        }
    };

    // Lancer le traitement en parallèle avec QtConcurrent::map
    QFuture<void> future = QtConcurrent::map(files, processFile);

    // Attendre que tous les threads aient terminé
    future.waitForFinished();

    // Compter le nombre de doublons
    size_t numberOfDuplicate = 0;
    for (const auto &file : files) {
        if (file.duplicateBySHA256) {
            numberOfDuplicate++;
        }
    }

    addUserLog(QString::number(numberOfDuplicate) + " doublons détectés par SHA-256.");
}

void MainWindow::findDuplicateSizeAndDate(std::vector<DupFile> &files) {
    addUserLog("Recherche des doublons par taille et date...");

    std::unordered_map<QString, DupFile*> uniqueFiles; // Associe "taille_date" → premier fichier unique
    size_t numberOfDuplicate = 0;

    int total = static_cast<int>(files.size());

    for (size_t i = 0; i < files.size(); ++i) {
        // Clé unique basée sur la taille et la date de modification
        QString key = QString::number(files[i].fileInfo.size()) + "_" +
                      QString::number(files[i].fileInfo.lastModified().toSecsSinceEpoch());

        auto it = uniqueFiles.find(key);
        if (it == uniqueFiles.end()) {
            // Taille + date unique → On l'ajoute comme référence
            uniqueFiles[key] = &files[i];
        } else {
            // Doublon trouvé → On l'ajoute à la liste des doublons
            files[i].duplicateBySizeAndDate = true;
            it->second->duplicates.push_back(&files[i]);
            numberOfDuplicate++;
        }

        // Mise à jour de la ProgressBar
        ui->filterProgressBar->setValue(static_cast<int>(((i + 1) / static_cast<double>(total)) * 100));
        QCoreApplication::processEvents();
    }

    addUserLog(QString::number(numberOfDuplicate) + " doublons détectés par taille et date.");
}

void MainWindow::addUserLog(const QString &message) {
    ui->logUserListWidget->addItem(message);
    ui->logUserListWidget->scrollToBottom();
}

void MainWindow::afficherDoublons(const std::vector<DupFile> &files) {
    for (const auto &file : files) {
        if (!file.duplicates.empty()) { // S'il a des doublons
            qDebug() << "Original: " << file.fileInfo.filePath();
            for (const auto *dup : file.duplicates) {
                qDebug() << "   -> Doublon: " << dup->fileInfo.filePath();
            }
        }
    }
}

int MainWindow::countActiveFilters() {
    int count = 0;
    if (ui->filterNameCheckBox->isChecked()) count++;
    if (ui->filterSHA256MD5CheckBox->isChecked()) count++;
    if (ui->filterSizeAndDateCheckBox->isChecked()) count++;
    return count;
}

double MainWindow::getSizeToComputeInGo(const std::vector<DupFile> &dup_files) {
    qint64 size = 0;  // Changer de int à qint64

    // Calculer la taille totale en octets
    for (const auto &file : dup_files) {
        size += file.fileInfo.size();
    }

    // Convertir la taille en Go (1 Go = 1024^3 octets)
    double sizeInGo = static_cast<double>(size) / (1024 * 1024 * 1024);

    return sizeInGo;
}

void MainWindow::onOptionsCheckBoxStateChanged() {
    filteredFiles = filterFiles(dup_files);

    double sizeToCompute = getSizeToComputeInGo(filteredFiles);

    ui->selectSizeLabel->setText("Taille à traiter : " + QString::number(sizeToCompute, 'f', 2) + "Go");
}

void MainWindow::deleteSelectedPushButton() {
    // Vider les tableaux
    dup_files.clear();
    filteredFiles.clear();
    // Reset progressBar
    ui->filterProgressBar->setValue(0);
    // Clear UI
    ui->logUserListWidget->clear();
}
