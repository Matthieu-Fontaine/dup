#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDirIterator>

#include <unordered_set>

#include <vector>

#include <models/dupfile.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("dup");
    setFixedSize(240, 557);

    connect(ui->selectFolderPushButton, &QPushButton::released, this, &MainWindow::selectFolderPushButton);


    ui->filterSHA256MD5CheckBox->setToolTip("");
    ui->filterPHashCheckBox->setToolTip("pHash (Perceptual Hash) compare les images en fonction de leur apparence visuelle.");
    ui->pHashThresholdSpinBox->setToolTip("Tolérance réglable (0 similaire - 15 différentes)");

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::selectFolderPushButton() {
    addUserLog("-------------------");
    addUserLog("Selection d'un dossier");

    QString folder_path = QFileDialog::getExistingDirectory(nullptr, "Sélectionner un dossier");

    if (folder_path.isEmpty()) {
        addUserLog("Annulation");
        return; // Si l'utilisateur annule, ne rien faire
    }

    // Création d'un itérateur pour parcourir le dossier récursivement
    QDirIterator it(folder_path, QDir::Files, QDirIterator::Subdirectories);

    std::vector<DupFile> dup_files;

    while (it.hasNext()) {
        dup_files.push_back(DupFile(QFileInfo(it.next())));
    }

    std::vector<DupFile> filteredFiles = filterFiles(dup_files);

    maxSizeProgressBar = (ui->filterNameCheckBox->isChecked() * static_cast<int>(filteredFiles.size())) +
                         (ui->filterSHA256MD5CheckBox->isChecked() * static_cast<int>(filteredFiles.size())) +
                         (ui->filterSizeAndDateCheckBox->isChecked() * static_cast<int>(filteredFiles.size())) +
                         (ui->filterPHashCheckBox->isChecked() * static_cast<int>(filteredFiles.size()));

    ui->filterProgressBar->setMaximum(maxSizeProgressBar);

    if(ui->filterNameCheckBox->isChecked()) {
        findDuplicateFileName(filteredFiles);
    } else {
        qDebug() << "filterNameCheckBox not checked";
    }

    if(ui->filterSHA256MD5CheckBox->isChecked()) {
        findDuplicateSHA256(filteredFiles);
    } else {
        qDebug() << "filterSHA256MD5CheckBox not checked";
    }

    if(ui->filterSizeAndDateCheckBox->isChecked()) {
        findDuplicateSizeAndDate(filteredFiles);
    } else {
        qDebug() << "findDuplicateSizeAndDate not checked";
    }

    if(ui->filterPHashCheckBox->isChecked()) {
        findDuplicatePHash(filteredFiles, ui->pHashThresholdSpinBox->value());
    } else {
        qDebug() << "findDuplicatePHash not checked";
    }

    ui->filterProgressBar->setValue(100);
    QCoreApplication::processEvents();

}

std::vector<DupFile> MainWindow::filterFiles(const std::vector<DupFile> &files) {
    std::vector<DupFile> out_files;

    for (const auto &file : files) {
        // Vérifier les options cochées
        if (ui->optionsImageCheckBox->isChecked()) {
            out_files.push_back(file);
        } else if (ui->optionsVideoCheckBox->isChecked()) {
            out_files.push_back(file);
        } else if (ui->optionsDocumentCheckBox->isChecked()) {
            out_files.push_back(file);
        } else if (ui->optionsAudioCheckBox->isChecked()) {
            out_files.push_back(file);
        } if (ui->optionsArchiveCheckBox->isChecked()) {
            out_files.push_back(file);
        }
    }

    return out_files;
}

void MainWindow::findDuplicateFileName(std::vector<DupFile> &files) {
    addUserLog("Recherche des doublons par nom...");

    std::unordered_set<QString> uniqueFileNames;
    size_t numberOfDuplicate = 0;

    for (size_t i = 0; i < files.size(); ++i) {
        QString fileName = files[i].fileInfo.fileName().trimmed().toLower();

        if (uniqueFileNames.insert(fileName).second) {
            // Unique
        } else {
            // Doublon
            files[i].duplicateByFileName = true;
            numberOfDuplicate++;
        }
        ui->filterProgressBar->setValue(ui->filterProgressBar->value() + static_cast<int>(i));
        QCoreApplication::processEvents();
    }

    addUserLog(QString::number(numberOfDuplicate) + " doublons par nom.");
}

void MainWindow::findDuplicateSHA256(std::vector<DupFile> &files) {
    addUserLog("Recherche des doublons SHA-256...");

    std::unordered_set<QString> uniqueFileHash;
    size_t numberOfDuplicate = 0;

    for (size_t i = 0; i < files.size(); ++i) {
        QString fileHash = files[i].calculateSHA256();

        if (fileHash.isEmpty()) continue; // Si erreur, on ignore ce fichier

        if (uniqueFileHash.insert(fileHash).second) {
            // Unique
        } else {
            // Doublon
            files[i].duplicateBySHA256 = true;
            numberOfDuplicate++;
        }
        ui->filterProgressBar->setValue(ui->filterProgressBar->value() + static_cast<int>(i));
        QCoreApplication::processEvents();
    }

    addUserLog(QString::number(numberOfDuplicate) + " doublons SHA-256.");
}

void MainWindow::findDuplicateSizeAndDate(std::vector<DupFile> &files) {
    addUserLog("Recherche des doublons par taille et date...");

    std::unordered_set<QString> uniqueFiles;
    size_t numberOfDuplicate = 0;

    int total = static_cast<int>(files.size());
    ui->filterProgressBar->setValue(0);

    for (size_t i = 0; i < files.size(); ++i) {
        // On crée une clé unique basée sur la taille et la date de modification
        QString key = QString::number(files[i].fileInfo.size()) + "_" +
                      QString::number(files[i].fileInfo.lastModified().toSecsSinceEpoch());

        if (uniqueFiles.insert(key).second) {
            // Unique
        } else {
            // Doublon
            files[i].duplicateBySizeAndDate = true;
            numberOfDuplicate++;
        }

        // Mise à jour de la ProgressBar
        ui->filterProgressBar->setValue(static_cast<int>(((i + 1) / static_cast<double>(total)) * 100));
        QCoreApplication::processEvents();
    }

    addUserLog(QString::number(numberOfDuplicate) + " doublons détectés par taille et date.");
}

void MainWindow::findDuplicatePHash(std::vector<DupFile> &files, int similarityThreshold) {
    addUserLog("Recherche des doublons par perceptual hash (pHash)...");

    std::vector<quint64> uniqueHashes; // Stocke les pHash des images de référence
    size_t numberOfDuplicate = 0;

    int total = static_cast<int>(files.size());
    ui->filterProgressBar->setValue(0);

    for (size_t i = 0; i < files.size(); ++i) {
        quint64 phash = files[i].calculatePHash(); // Calcul du pHash

        if (phash == 0) continue; // Ignorer si erreur

        bool isDuplicate = false;

        // Vérifier si cette image est similaire à une image déjà considérée comme unique
        for (const auto &storedPhash : uniqueHashes) {
            if (hammingDistance(storedPhash, phash) <= similarityThreshold) {
                isDuplicate = true;
                break; // Stop dès qu'un doublon est trouvé
            }
        }

        if (!isDuplicate) {
            uniqueHashes.push_back(phash); // Ajouter la première image du groupe
        } else {
            files[i].duplicateByPHash = true; // Marquer comme doublon
            numberOfDuplicate++;
        }

        // Mise à jour de la ProgressBar
        ui->filterProgressBar->setValue(static_cast<int>(((i + 1) / static_cast<double>(total)) * 100));
        QCoreApplication::processEvents();
    }

    addUserLog(QString::number(numberOfDuplicate) + " doublons détectés par pHash.");
}


int MainWindow::hammingDistance(quint64 hash1, quint64 hash2) {
    quint64 x = hash1 ^ hash2; // XOR pour voir les différences
    int distance = 0;
    while (x) {
        distance += x & 1;
        x >>= 1;
    }
    return distance;
}


void MainWindow::addUserLog(const QString &message) {
    ui->logUserListWidget->addItem(message);
    ui->logUserListWidget->scrollToBottom();
}
