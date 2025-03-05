#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>

#include <models/dupfile.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private:
    Ui::MainWindow *ui;

    int maxSizeProgressBar;

    void addUserLog(const QString &log);

    std::vector<DupFile> filterFiles(const std::vector<DupFile> &files);
    void findDuplicateFileName(std::vector<DupFile> &files);
    void findDuplicateSHA256(std::vector<DupFile> &files);
    void findDuplicateSizeAndDate(std::vector<DupFile> &files);
    void findDuplicatePHash(std::vector<DupFile> &files, int similarityThreshold);

    int hammingDistance(quint64 hash1, quint64 hash2);


private slots:
    void selectFolderPushButton();

};
#endif // MAINWINDOW_H
