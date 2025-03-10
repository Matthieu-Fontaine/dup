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
    std::vector<DupFile> dup_files;
    std::vector<DupFile> filteredFiles;

    void addUserLog(const QString &log);

    std::vector<DupFile> filterFiles(const std::vector<DupFile> &files);
    void findDuplicateFileName(std::vector<DupFile> &files);
    void findDuplicateSHA256(std::vector<DupFile> &files);
    void findDuplicateSizeAndDate(std::vector<DupFile> &files);

    void afficherDoublons(const std::vector<DupFile> &files);
    int countActiveFilters();
    double getSizeToComputeInGo(const std::vector<DupFile> &dup_files);

private slots:
    void selectFolderPushButton();
    void findDuplicatePushButton();
    void deleteDuplicatePushButton();
    void deleteSelectedPushButton();
    void onOptionsCheckBoxStateChanged();


};
#endif // MAINWINDOW_H
