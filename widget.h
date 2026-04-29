#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QProgressBar>
#include <QTextEdit>
#include <QScrollArea>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QLineEdit>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "datamodels.h"
#include "dataloader.h"
#include "filenameparser.h"
#include "imageprocessor.h"
#include "uploadmanager.h"

class WorkerThread;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    enum WizardPage {
        Page_Start = 0,
        Page_DragDrop = 1,
        Page_FileList = 2,
        Page_Uploading = 3,
        Page_Result = 4
    };

private slots:
    void onStartClicked();
    void onFolderDropped(const QString &folderPath);
    void onUploadClicked();
    void onImageUploadFinished(bool success, const QString &error, const QString &uid);
    void onOrderUploadFinished(bool success, const QString &error, const QString &uid);
    void onUploadProgress(int current, int total);
    void onUploadComplete();
    void onBackClicked();
    void onNextClicked();
    void onRestartClicked();
    void onTableCellClicked(int row, int column);
    void onServerUrlChanged();

private:
    void setupUi();
    void createStartPage();
    void createDragDropPage();
    void createFileListPage();
    void createUploadingPage();
    void createResultPage();
    
    void loadDataFiles();
    void processFolder(const QString &folderPath);
    OrderItem createOrderItem(const QString &filePath);
    void populateTable();
    void updateTableItem(int row, const OrderItem &item);
    void startUpload();
    void uploadNextItem();
    void cleanupTempFiles();
    void showLargeImage(const QString &imagePath);
    void updateUploadProgress();
    
    bool validateAllItems() const;

private:
    QStackedWidget *m_stackedWidget;
    
    QPushButton *m_btnBack;
    QPushButton *m_btnNext;
    QPushButton *m_btnStart;
    QPushButton *m_btnUpload;
    QPushButton *m_btnRestart;
    
    DataLoader *m_dataLoader;
    UploadManager *m_uploadManager;
    
    QVector<OrderItem> m_orderItems;
    QString m_tempDir;
    QString m_currentDate;
    
    int m_currentUploadIndex;
    int m_uploadSuccessCount;
    int m_uploadFailCount;
    
    QTableWidget *m_fileTable;
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;
    QTextEdit *m_resultText;
    QLabel *m_dragDropLabel;
    QLabel *m_statusLabel;
    QLineEdit *m_serverUrlEdit;
    
    bool m_isUploading;
    
    friend class DragDropArea;
};

class DragDropArea : public QLabel
{
    Q_OBJECT
public:
    explicit DragDropArea(QWidget *parent = nullptr);
    
signals:
    void folderDropped(const QString &folderPath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

class ImagePreviewDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImagePreviewDialog(const QString &imagePath, QWidget *parent = nullptr);
    ~ImagePreviewDialog() = default;
};

#endif 
