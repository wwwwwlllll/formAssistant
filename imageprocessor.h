#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QFileInfo>

class ImageProcessor : public QObject
{
    Q_OBJECT
public:
    explicit ImageProcessor(QObject *parent = nullptr);
    ~ImageProcessor() = default;
    
    static QString generateTempFileName(const QString &sourceFile, const QString &dateStr);
    static QString generateThumbFileName(const QString &tempBaseName);
    static QString getFileExtension(const QString &fileName);
    
    static bool processImage(const QString &sourceFile, 
                             const QString &tempDir,
                             const QString &tempFileName,
                             const QString &thumbFileName,
                             QPixmap &outThumbnail);
    
    static bool compressImage(const QString &sourceFile, 
                              const QString &destFile,
                              qint64 maxSizeKB = 500);
    
    static QPixmap createThumbnail(const QString &sourceFile, int size = 100);
    static QPixmap createThumbnail(const QImage &sourceImage, int size = 100);
    
    static qint64 getFileSizeKB(const QString &filePath);
    static bool ensureDirExists(const QString &dirPath);
    static bool copyFile(const QString &source, const QString &dest);
};

#endif 
