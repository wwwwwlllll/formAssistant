#include "widget.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>
#include <QDateTime>
#include <QScrollBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>
#include <QDialog>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QDesktopServices>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , m_dataLoader(new DataLoader(this))
    , m_uploadManager(new UploadManager(this))
    , m_currentUploadIndex(0)
    , m_uploadSuccessCount(0)
    , m_uploadFailCount(0)
    , m_isUploading(false)
{
    m_tempDir = QCoreApplication::applicationDirPath() + "/tmp";
    m_currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    
    loadDataFiles();
    
    QSettings settings;
    QString serverUrl = settings.value("serverUrl", "http://localhost:8080").toString();
    m_uploadManager->setBaseUrl(serverUrl);
    
    connect(m_uploadManager, &UploadManager::imageUploadFinished,
            this, &Widget::onImageUploadFinished);
    connect(m_uploadManager, &UploadManager::orderUploadFinished,
            this, &Widget::onOrderUploadFinished);
    
    setupUi();
    
    setMinimumSize(1200, 700);
    setWindowTitle(tr("订单图片批量上传助手"));
}

Widget::~Widget()
{
    cleanupTempFiles();
}

void Widget::loadDataFiles()
{
    QString appDir = QCoreApplication::applicationDirPath();
    
    QString contactsPath = appDir + "/contacts.txt";
    if (m_dataLoader->loadContacts(contactsPath)) {
        qDebug() << "Contacts loaded from:" << contactsPath;
        m_dataLoader->printContacts();
    } else {
        qWarning() << "Failed to load contacts from:" << contactsPath;
    }
    
    QString designersPath = appDir + "/designers.txt";
    if (m_dataLoader->loadDesigners(designersPath)) {
        qDebug() << "Designers loaded from:" << designersPath;
        m_dataLoader->printDesigners();
    } else {
        qWarning() << "Failed to load designers from:" << designersPath;
    }
}

void Widget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *urlLabel = new QLabel(tr("服务器地址:"), this);
    m_serverUrlEdit = new QLineEdit(this);
    m_serverUrlEdit->setText(m_uploadManager->baseUrl());
    m_serverUrlEdit->setPlaceholderText(tr("例如: http://localhost:8080"));
    connect(m_serverUrlEdit, &QLineEdit::editingFinished,
            this, &Widget::onServerUrlChanged);
    
    topLayout->addWidget(urlLabel);
    topLayout->addWidget(m_serverUrlEdit, 1);
    mainLayout->addLayout(topLayout);
    
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget, 1);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: #666; padding: 5px; }");
    mainLayout->addWidget(m_statusLabel);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_btnBack = new QPushButton(tr("上一步"), this);
    m_btnBack->setEnabled(false);
    connect(m_btnBack, &QPushButton::clicked, this, &Widget::onBackClicked);
    
    m_btnNext = new QPushButton(tr("下一步"), this);
    m_btnNext->setEnabled(false);
    connect(m_btnNext, &QPushButton::clicked, this, &Widget::onNextClicked);
    
    m_btnStart = new QPushButton(tr("开始批量上传"), this);
    m_btnStart->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; padding: 10px 20px; }");
    connect(m_btnStart, &QPushButton::clicked, this, &Widget::onStartClicked);
    
    m_btnUpload = new QPushButton(tr("批量上传"), this);
    m_btnUpload->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; padding: 10px 20px; background-color: #4CAF50; color: white; }");
    m_btnUpload->setEnabled(false);
    connect(m_btnUpload, &QPushButton::clicked, this, &Widget::onUploadClicked);
    
    m_btnRestart = new QPushButton(tr("重新开始"), this);
    connect(m_btnRestart, &QPushButton::clicked, this, &Widget::onRestartClicked);
    m_btnRestart->setVisible(false);
    
    buttonLayout->addWidget(m_btnBack);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnStart);
    buttonLayout->addWidget(m_btnNext);
    buttonLayout->addWidget(m_btnUpload);
    buttonLayout->addWidget(m_btnRestart);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    createStartPage();
    createDragDropPage();
    createFileListPage();
    createUploadingPage();
    createResultPage();
    
    m_stackedWidget->setCurrentIndex(Page_Start);
    m_statusLabel->setText(tr("就绪"));
}

void Widget::createStartPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(30);
    
    QLabel *titleLabel = new QLabel(tr("订单图片批量上传助手"), page);
    titleLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #333; }");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    QLabel *descLabel = new QLabel(page);
    descLabel->setText(tr("使用步骤：\n"
                          "1. 点击【开始批量上传】按钮\n"
                          "2. 拖拽包含图片的文件夹到窗口\n"
                          "3. 确认文件列表无误后点击【批量上传】\n"
                          "4. 等待上传完成，查看结果"));
    descLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; padding: 20px; }");
    descLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(descLabel);
    
    QLabel *infoLabel = new QLabel(page);
    infoLabel->setText(tr("已加载数据：\n"
                          "  客户数据: %1 条\n"
                          "  设计师数据: %2 条")
                        .arg(m_dataLoader->contactCount())
                        .arg(m_dataLoader->designerCount()));
    infoLabel->setStyleSheet("QLabel { font-size: 12px; color: #888; padding: 10px; }");
    infoLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(infoLabel);
    
    layout->addStretch();
    
    m_stackedWidget->addWidget(page);
}

void Widget::createDragDropPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel *instLabel = new QLabel(tr("请拖拽包含图片的文件夹到下方区域："), page);
    instLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    layout->addWidget(instLabel);
    
    m_dragDropLabel = new DragDropArea(page);
    m_dragDropLabel->setMinimumHeight(400);
    m_dragDropLabel->setStyleSheet(
        "QLabel {"
        "  border: 4px dashed #aaa;"
        "  border-radius: 10px;"
        "  background-color: #fafafa;"
        "  font-size: 18px;"
        "  color: #999;"
        "}"
    );
    m_dragDropLabel->setAlignment(Qt::AlignCenter);
    m_dragDropLabel->setText(tr("拖拽文件夹到这里\n\n支持的图片格式: JPG, PNG, BMP, GIF"));
    m_dragDropLabel->setAcceptDrops(true);
    
    connect(static_cast<DragDropArea*>(m_dragDropLabel), &DragDropArea::folderDropped,
            this, &Widget::onFolderDropped);
    
    layout->addWidget(m_dragDropLabel, 1);
    
    QLabel *hintLabel = new QLabel(tr("提示：程序会自动解析图片文件名中的订单信息"), page);
    hintLabel->setStyleSheet("QLabel { color: #888; font-size: 12px; }");
    layout->addWidget(hintLabel);
    
    m_stackedWidget->addWidget(page);
}

void Widget::createFileListPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(10, 10, 10, 10);
    
    QLabel *titleLabel = new QLabel(tr("文件列表确认"), page);
    titleLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; }");
    layout->addWidget(titleLabel);
    
    m_fileTable = new QTableWidget(page);
    m_fileTable->setColumnCount(15);
    
    QStringList headers;
    headers << tr("缩略图") 
            << tr("源文件名")
            << tr("客户ID")
            << tr("客户名称")
            << tr("产品名称")
            << tr("设计师ID")
            << tr("设计师")
            << tr("设计费")
            << tr("件数")
            << tr("是否补单")
            << tr("蜡重(g)")
            << tr("蜡单价")
            << tr("蜡总额")
            << tr("预估总额")
            << tr("UID");
    
    m_fileTable->setHorizontalHeaderLabels(headers);
    m_fileTable->verticalHeader()->setDefaultSectionSize(80);
    m_fileTable->horizontalHeader()->setStretchLastSection(true);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTable->setAlternatingRowColors(true);
    
    connect(m_fileTable, &QTableWidget::cellClicked,
            this, &Widget::onTableCellClicked);
    
    layout->addWidget(m_fileTable, 1);
    
    QHBoxLayout *infoLayout = new QHBoxLayout();
    QLabel *countLabel = new QLabel(tr("共 0 张图片"), page);
    countLabel->setObjectName("countLabel");
    infoLayout->addWidget(countLabel);
    infoLayout->addStretch();
    
    QLabel *legendLabel = new QLabel(tr("<font color='red'>红色</font>表示客户或设计师未找到"), page);
    infoLayout->addWidget(legendLabel);
    layout->addLayout(infoLayout);
    
    m_stackedWidget->addWidget(page);
}

void Widget::createUploadingPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(50, 50, 50, 50);
    layout->setSpacing(20);
    
    QLabel *titleLabel = new QLabel(tr("正在上传..."), page);
    titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; }");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    m_progressLabel = new QLabel(tr("准备上传 0/0"), page);
    m_progressLabel->setStyleSheet("QLabel { font-size: 14px; }");
    m_progressLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_progressLabel);
    
    m_progressBar = new QProgressBar(page);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setMinimumHeight(30);
    layout->addWidget(m_progressBar);
    
    QLabel *hintLabel = new QLabel(tr("请勿关闭程序，等待上传完成..."), page);
    hintLabel->setStyleSheet("QLabel { color: #888; }");
    hintLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(hintLabel);
    
    layout->addStretch();
    
    m_stackedWidget->addWidget(page);
}

void Widget::createResultPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel *titleLabel = new QLabel(tr("上传结果"), page);
    titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; }");
    layout->addWidget(titleLabel);
    
    QHBoxLayout *summaryLayout = new QHBoxLayout();
    QLabel *successLabel = new QLabel(tr("成功: 0"), page);
    successLabel->setObjectName("successLabel");
    successLabel->setStyleSheet("QLabel { font-size: 16px; color: green; font-weight: bold; }");
    
    QLabel *failLabel = new QLabel(tr("失败: 0"), page);
    failLabel->setObjectName("failLabel");
    failLabel->setStyleSheet("QLabel { font-size: 16px; color: red; font-weight: bold; }");
    
    summaryLayout->addWidget(successLabel);
    summaryLayout->addWidget(failLabel);
    summaryLayout->addStretch();
    layout->addLayout(summaryLayout);
    
    m_resultText = new QTextEdit(page);
    m_resultText->setReadOnly(true);
    m_resultText->setStyleSheet("QTextEdit { font-family: 'Consolas', 'Monaco', monospace; font-size: 12px; }");
    layout->addWidget(m_resultText, 1);
    
    m_stackedWidget->addWidget(page);
}

void Widget::onServerUrlChanged()
{
    QString url = m_serverUrlEdit->text().trimmed();
    if (!url.isEmpty()) {
        m_uploadManager->setBaseUrl(url);
        QSettings settings;
        settings.setValue("serverUrl", url);
        qDebug() << "Server URL updated to:" << url;
    }
}

void Widget::onStartClicked()
{
    m_stackedWidget->setCurrentIndex(Page_DragDrop);
    m_btnStart->setVisible(false);
    m_btnBack->setEnabled(true);
    m_statusLabel->setText(tr("请拖拽文件夹"));
}

void Widget::onBackClicked()
{
    int current = m_stackedWidget->currentIndex();
    if (current > Page_Start) {
        m_stackedWidget->setCurrentIndex(current - 1);
        
        if (current - 1 == Page_Start) {
            m_btnBack->setEnabled(false);
            m_btnStart->setVisible(true);
        }
        
        m_btnUpload->setVisible(false);
        m_btnNext->setVisible(true);
        m_btnNext->setEnabled(false);
        m_btnRestart->setVisible(false);
    }
}

void Widget::onNextClicked()
{
    int current = m_stackedWidget->currentIndex();
    if (current < m_stackedWidget->count() - 1) {
        m_stackedWidget->setCurrentIndex(current + 1);
        
        if (current + 1 == Page_FileList) {
            m_btnNext->setVisible(false);
            m_btnUpload->setVisible(true);
            m_btnUpload->setEnabled(validateAllItems());
        }
    }
}

void Widget::onFolderDropped(const QString &folderPath)
{
    qDebug() << "Folder dropped:" << folderPath;
    
    QDir dir(folderPath);
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("错误"), tr("文件夹不存在！"));
        return;
    }
    
    processFolder(folderPath);
    
    if (m_orderItems.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("该文件夹中没有找到图片文件。"));
        return;
    }
    
    populateTable();
    
    m_stackedWidget->setCurrentIndex(Page_FileList);
    m_btnNext->setEnabled(true);
    m_btnUpload->setEnabled(validateAllItems());
    m_statusLabel->setText(tr("已加载 %1 张图片").arg(m_orderItems.size()));
}

void Widget::processFolder(const QString &folderPath)
{
    m_orderItems.clear();
    cleanupTempFiles();
    
    QDir dir(folderPath);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif";
    dir.setNameFilters(filters);
    
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    
    qDebug() << "Found" << fileList.size() << "image files in" << folderPath;
    
    for (const QFileInfo &fi : fileList) {
        OrderItem item = createOrderItem(fi.absoluteFilePath());
        if (!item.sourceFileName.isEmpty()) {
            m_orderItems.append(item);
        }
    }
    
    qDebug() << "Successfully processed" << m_orderItems.size() << "items";
}

OrderItem Widget::createOrderItem(const QString &filePath)
{
    OrderItem item;
    QFileInfo fi(filePath);
    
    item.sourceFileName = fi.fileName();
    item.sourceFilePath = filePath;
    
    ParsedFileName parsed = FileNameParser::parse(fi.fileName());
    
    if (!parsed.valid) {
        qWarning() << "Failed to parse file name:" << fi.fileName();
        item.sourceFileName = "";
        return item;
    }
    
    item.cont_name = parsed.customerName;
    item.product_name = parsed.productName;
    item.dser_name = parsed.designerName;
    item.design_fee_base = parsed.designFeeBase;
    item.pieces = parsed.pieces;
    item.is_reorder = parsed.isReorder;
    item.wax_weight = parsed.waxWeight;
    
    if (m_dataLoader->hasContact(item.cont_name)) {
        Contact contact = m_dataLoader->getContactByName(item.cont_name);
        item.cont_id = contact.id;
        item.cont_found = true;
        item.wax_price = contact.cont_wax_price;
    } else {
        item.cont_found = false;
        qWarning() << "Contact not found:" << item.cont_name;
    }
    
    if (m_dataLoader->hasDesigner(item.dser_name)) {
        Designer designer = m_dataLoader->getDesignerByName(item.dser_name);
        item.dser_id = designer.id;
        item.dser_found = true;
    } else {
        item.dser_found = false;
        qWarning() << "Designer not found:" << item.dser_name;
    }
    
    if (item.is_reorder) {
        item.design_fee = 0.0;
        item.wax_total = 0.0;
    } else {
        item.design_fee = item.design_fee_base * 100.0;
        item.wax_total = (item.pieces * item.wax_weight) * item.wax_price;
    }
    
    item.estimated_total = item.design_fee + item.wax_total;
    
    qint64 nanoSecs = QDateTime::currentMSecsSinceEpoch() * 1000000LL + 
                       QTime::currentTime().msec() * 1000LL;
    item.uid = QString::number(nanoSecs);
    
    QString tempFileName = ImageProcessor::generateTempFileName(item.sourceFileName, m_currentDate);
    QString thumbFileName = ImageProcessor::generateThumbFileName(tempFileName);
    
    if (ImageProcessor::processImage(item.sourceFilePath, m_tempDir, 
                                       tempFileName, thumbFileName, item.thumbnail)) {
        item.tempImagePath = m_tempDir + "/" + tempFileName;
        item.tempThumbPath = m_tempDir + "/" + thumbFileName;
    } else {
        qCritical() << "Failed to process image:" << item.sourceFilePath;
    }
    
    return item;
}

void Widget::populateTable()
{
    m_fileTable->setRowCount(m_orderItems.size());
    
    for (int i = 0; i < m_orderItems.size(); ++i) {
        updateTableItem(i, m_orderItems[i]);
    }
    
    m_fileTable->resizeColumnsToContents();
    
    for (int i = 0; i < m_fileTable->columnCount(); ++i) {
        int width = m_fileTable->columnWidth(i);
        if (width > 200) {
            m_fileTable->setColumnWidth(i, 200);
        }
    }
    
    QLabel *countLabel = m_fileTable->parentWidget()->findChild<QLabel*>("countLabel");
    if (countLabel) {
        countLabel->setText(tr("共 %1 张图片").arg(m_orderItems.size()));
    }
}

void Widget::updateTableItem(int row, const OrderItem &item)
{
    bool hasError = !item.cont_found || !item.dser_found;
    
    QColor textColor = hasError ? Qt::red : Qt::black;
    
    QTableWidgetItem *thumbItem = new QTableWidgetItem();
    if (!item.thumbnail.isNull()) {
        thumbItem->setData(Qt::DecorationRole, item.thumbnail.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    thumbItem->setTextAlignment(Qt::AlignCenter);
    m_fileTable->setItem(row, 0, thumbItem);
    
    auto createTextItem = [&](const QString &text) -> QTableWidgetItem* {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setForeground(QBrush(textColor));
        return item;
    };
    
    m_fileTable->setItem(row, 1, createTextItem(item.sourceFileName));
    m_fileTable->setItem(row, 2, createTextItem(QString::number(item.cont_id)));
    m_fileTable->setItem(row, 3, createTextItem(item.cont_name + (item.cont_found ? "" : " (未找到)")));
    m_fileTable->setItem(row, 4, createTextItem(item.product_name));
    m_fileTable->setItem(row, 5, createTextItem(QString::number(item.dser_id)));
    m_fileTable->setItem(row, 6, createTextItem(item.dser_name + (item.dser_found ? "" : " (未找到)")));
    m_fileTable->setItem(row, 7, createTextItem(QString::number(item.design_fee, 'f', 2)));
    m_fileTable->setItem(row, 8, createTextItem(QString::number(item.pieces)));
    m_fileTable->setItem(row, 9, createTextItem(item.is_reorder ? tr("是") : tr("否")));
    m_fileTable->setItem(row, 10, createTextItem(QString::number(item.wax_weight, 'f', 3)));
    m_fileTable->setItem(row, 11, createTextItem(QString::number(item.wax_price, 'f', 2)));
    m_fileTable->setItem(row, 12, createTextItem(QString::number(item.wax_total, 'f', 2)));
    m_fileTable->setItem(row, 13, createTextItem(QString::number(item.estimated_total, 'f', 2)));
    m_fileTable->setItem(row, 14, createTextItem(item.uid));
}

bool Widget::validateAllItems() const
{
    for (const OrderItem &item : m_orderItems) {
        if (!item.cont_found || !item.dser_found) {
            return false;
        }
    }
    return true;
}

void Widget::onTableCellClicked(int row, int column)
{
    if (column == 0 && row >= 0 && row < m_orderItems.size()) {
        const OrderItem &item = m_orderItems[row];
        if (!item.tempImagePath.isEmpty()) {
            showLargeImage(item.tempImagePath);
        }
    }
}

void Widget::showLargeImage(const QString &imagePath)
{
    ImagePreviewDialog *dialog = new ImagePreviewDialog(imagePath, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void Widget::onUploadClicked()
{
    if (!validateAllItems()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("确认"), 
            tr("部分客户或设计师未找到，是否继续上传（这些记录将被跳过）？"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) {
            return;
        }
    }
    
    if (m_orderItems.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("没有可上传的图片！"));
        return;
    }
    
    m_stackedWidget->setCurrentIndex(Page_Uploading);
    m_btnBack->setEnabled(false);
    m_btnUpload->setEnabled(false);
    m_statusLabel->setText(tr("正在上传..."));
    
    startUpload();
}

void Widget::startUpload()
{
    m_currentUploadIndex = 0;
    m_uploadSuccessCount = 0;
    m_uploadFailCount = 0;
    m_isUploading = true;
    
    m_progressBar->setMaximum(m_orderItems.size() * 2);
    m_progressBar->setValue(0);
    m_progressLabel->setText(tr("准备上传 0/%1").arg(m_orderItems.size()));
    
    uploadNextItem();
}

void Widget::uploadNextItem()
{
    while (m_currentUploadIndex < m_orderItems.size()) {
        OrderItem &item = m_orderItems[m_currentUploadIndex];
        
        if (!item.cont_found || !item.dser_found) {
            qWarning() << "Skipping item" << m_currentUploadIndex << "- contact or designer not found";
            item.uploadSuccess = false;
            item.uploadError = tr("客户或设计师未找到");
            m_uploadFailCount++;
            m_currentUploadIndex++;
            continue;
        }
        
        if (item.tempImagePath.isEmpty() || item.tempThumbPath.isEmpty()) {
            qWarning() << "Skipping item" << m_currentUploadIndex << "- image files not processed";
            item.uploadSuccess = false;
            item.uploadError = tr("图片处理失败");
            m_uploadFailCount++;
            m_currentUploadIndex++;
            continue;
        }
        
        if (!item.imageUploaded) {
            m_progressLabel->setText(tr("正在上传图片 %1/%2")
                                     .arg(m_currentUploadIndex + 1)
                                     .arg(m_orderItems.size()));
            m_uploadManager->uploadImage(item);
            return;
        }
        
        if (!item.orderUploaded) {
            m_progressLabel->setText(tr("正在上传订单 %1/%2")
                                     .arg(m_currentUploadIndex + 1)
                                     .arg(m_orderItems.size()));
            m_uploadManager->uploadOrder(item);
            return;
        }
        
        m_currentUploadIndex++;
    }
    
    onUploadComplete();
}

void Widget::onImageUploadFinished(bool success, const QString &error, const QString &uid)
{
    for (int i = 0; i < m_orderItems.size(); ++i) {
        if (m_orderItems[i].uid == uid) {
            OrderItem &item = m_orderItems[i];
            item.imageUploaded = success;
            
            if (!success) {
                item.uploadSuccess = false;
                item.uploadError = tr("图片上传失败: %1").arg(error);
                m_uploadFailCount++;
                m_currentUploadIndex++;
            }
            
            int progress = m_currentUploadIndex * 2 + (success ? 1 : 2);
            m_progressBar->setValue(progress);
            
            uploadNextItem();
            return;
        }
    }
}

void Widget::onOrderUploadFinished(bool success, const QString &error, const QString &uid)
{
    for (int i = 0; i < m_orderItems.size(); ++i) {
        if (m_orderItems[i].uid == uid) {
            OrderItem &item = m_orderItems[i];
            item.orderUploaded = success;
            
            if (success) {
                item.uploadSuccess = true;
                m_uploadSuccessCount++;
            } else {
                item.uploadSuccess = false;
                item.uploadError = tr("订单上传失败: %1").arg(error);
                m_uploadFailCount++;
            }
            
            int progress = m_currentUploadIndex * 2 + 2;
            m_progressBar->setValue(progress);
            m_currentUploadIndex++;
            
            uploadNextItem();
            return;
        }
    }
}

void Widget::onUploadComplete()
{
    m_isUploading = false;
    m_statusLabel->setText(tr("上传完成"));
    
    QString resultText;
    resultText += tr("上传结果汇总:\n");
    resultText += tr("==============================\n");
    resultText += tr("总计: %1 张\n").arg(m_orderItems.size());
    resultText += tr("成功: %1 张\n").arg(m_uploadSuccessCount);
    resultText += tr("失败: %1 张\n").arg(m_uploadFailCount);
    resultText += tr("==============================\n\n");
    
    for (int i = 0; i < m_orderItems.size(); ++i) {
        const OrderItem &item = m_orderItems[i];
        QString status = item.uploadSuccess ? tr("[成功]") : tr("[失败]");
        resultText += QString("%1 %2\n").arg(status).arg(item.sourceFileName);
        
        if (!item.uploadError.isEmpty()) {
            resultText += QString("  错误: %1\n").arg(item.uploadError);
        }
        resultText += "\n";
    }
    
    m_resultText->setText(resultText);
    
    QWidget *resultPage = m_stackedWidget->widget(Page_Result);
    QLabel *successLabel = resultPage->findChild<QLabel*>("successLabel");
    QLabel *failLabel = resultPage->findChild<QLabel*>("failLabel");
    
    if (successLabel) {
        successLabel->setText(tr("成功: %1").arg(m_uploadSuccessCount));
    }
    if (failLabel) {
        failLabel->setText(tr("失败: %1").arg(m_uploadFailCount));
    }
    
    m_stackedWidget->setCurrentIndex(Page_Result);
    m_btnRestart->setVisible(true);
    m_btnBack->setEnabled(false);
    m_btnUpload->setEnabled(false);
    
    cleanupTempFiles();
}

void Widget::onRestartClicked()
{
    m_orderItems.clear();
    m_currentUploadIndex = 0;
    m_uploadSuccessCount = 0;
    m_uploadFailCount = 0;
    m_isUploading = false;
    
    m_fileTable->setRowCount(0);
    
    m_stackedWidget->setCurrentIndex(Page_Start);
    
    m_btnBack->setEnabled(false);
    m_btnNext->setEnabled(false);
    m_btnNext->setVisible(true);
    m_btnStart->setVisible(true);
    m_btnUpload->setVisible(false);
    m_btnUpload->setEnabled(false);
    m_btnRestart->setVisible(false);
    
    m_statusLabel->setText(tr("就绪"));
}

void Widget::cleanupTempFiles()
{
    QDir dir(m_tempDir);
    if (dir.exists()) {
        QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        for (const QString &file : files) {
            dir.remove(file);
        }
        qDebug() << "Cleaned up" << files.size() << "files in temp directory:" << m_tempDir;
    }
}

void Widget::onUploadProgress(int current, int total)
{
    Q_UNUSED(current)
    Q_UNUSED(total)
}

DragDropArea::DragDropArea(QWidget *parent)
    : QLabel(parent)
{
    setAcceptDrops(true);
}

void DragDropArea::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        setStyleSheet(
            "QLabel {"
            "  border: 4px solid #4CAF50;"
            "  border-radius: 10px;"
            "  background-color: #E8F5E9;"
            "  font-size: 18px;"
            "  color: #4CAF50;"
            "}"
        );
    }
}

void DragDropArea::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DragDropArea::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    setStyleSheet(
        "QLabel {"
        "  border: 4px dashed #aaa;"
        "  border-radius: 10px;"
        "  background-color: #fafafa;"
        "  font-size: 18px;"
        "  color: #999;"
        "}"
    );
}

void DragDropArea::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        if (!urlList.isEmpty()) {
            QString localPath = urlList.first().toLocalFile();
            QFileInfo fi(localPath);
            
            if (fi.isDir()) {
                emit folderDropped(localPath);
                event->acceptProposedAction();
            }
        }
    }
    
    setStyleSheet(
        "QLabel {"
        "  border: 4px dashed #aaa;"
        "  border-radius: 10px;"
        "  background-color: #fafafa;"
        "  font-size: 18px;"
        "  color: #999;"
        "}"
    );
}

ImagePreviewDialog::ImagePreviewDialog(const QString &imagePath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("图片预览"));
    setMinimumSize(800, 600);
    setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignCenter);
    
    QLabel *imageLabel = new QLabel(this);
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        QSize maxSize(1600, 1200);
        if (pixmap.width() > maxSize.width() || pixmap.height() > maxSize.height()) {
            pixmap = pixmap.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        imageLabel->setPixmap(pixmap);
    } else {
        imageLabel->setText(tr("无法加载图片: %1").arg(imagePath));
    }
    
    imageLabel->setAlignment(Qt::AlignCenter);
    scrollArea->setWidget(imageLabel);
    
    layout->addWidget(scrollArea);
    
    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    
    layout->addLayout(btnLayout);
}
