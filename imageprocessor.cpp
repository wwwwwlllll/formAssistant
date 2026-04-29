#include "imageprocessor.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QBuffer>

ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent)
{
}

QString ImageProcessor::getFileExtension(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();
    if (ext == "jpeg") ext = "jpg";
    return ext;
}

QString ImageProcessor::generateTempFileName(const QString &sourceFile, const QString &dateStr)
{
    QFileInfo fi(sourceFile);
    QString baseName = fi.completeBaseName();
    QString ext = getFileExtension(sourceFile);
    
    QString hashInput = baseName + dateStr;
    QByteArray hash = QCryptographicHash::hash(hashInput.toUtf8(), QCryptographicHash::Md5);
    QString hashHex = hash.toHex().left(8);
    
    return hashHex + "." + ext;
}

QString ImageProcessor::generateThumbFileName(const QString &tempBaseName)
{
    QFileInfo fi(tempBaseName);
    QString base = fi.completeBaseName();
    QString ext = fi.suffix();
    
    return base + "_thumb." + ext;
}

qint64 ImageProcessor::getFileSizeKB(const QString &filePath)
{
    QFileInfo fi(filePath);
    return fi.size() / 1024;
}

bool ImageProcessor::ensureDirExists(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return dir.mkpath(".");
    }
    return true;
}

bool ImageProcessor::copyFile(const QString &source, const QString &dest)
{
    QFile::remove(dest);
    return QFile::copy(source, dest);
}

QPixmap ImageProcessor::createThumbnail(const QString &sourceFile, int size)
{
    QImage image(sourceFile);
    if (image.isNull()) {
        qWarning() << "Failed to load image for thumbnail:" << sourceFile;
        return QPixmap();
    }
    return createThumbnail(image, size);
}

QPixmap ImageProcessor::createThumbnail(const QImage &sourceImage, int size)
{
    if (sourceImage.isNull()) {
        return QPixmap();
    }
    
    QImage scaledImage;
    if (sourceImage.width() > sourceImage.height()) {
        scaledImage = sourceImage.scaledToWidth(size, Qt::SmoothTransformation);
    } else {
        scaledImage = sourceImage.scaledToHeight(size, Qt::SmoothTransformation);
    }
    
    int x = (scaledImage.width() - size) / 2;
    int y = (scaledImage.height() - size) / 2;
    x = qMax(0, x);
    y = qMax(0, y);
    
    QImage croppedImage = scaledImage.copy(x, y, size, size);
    return QPixmap::fromImage(croppedImage);
}

bool ImageProcessor::compressImage(const QString &sourceFile, const QString &destFile, qint64 maxSizeKB)
{
    QImage image(sourceFile);
    if (image.isNull()) {
        qWarning() << "Failed to load image for compression:" << sourceFile;
        return false;
    }
    
    QFileInfo fi(destFile);
    QString format = fi.suffix().toLower();
    if (format.isEmpty()) format = "jpg";
    if (format == "jpeg") format = "jpg";
    
    int quality = 90;
    bool success = false;
    
    while (quality >= 10) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        
        if (format == "jpg" || format == "jpeg") {
            image.save(&buffer, "JPEG", quality);
        } else if (format == "png") {
            image.save(&buffer, "PNG", quality);
        } else {
            image.save(&buffer, format.toUtf8().constData(), quality);
        }
        buffer.close();
        
        qint64 sizeKB = ba.size() / 1024;
        
        if (sizeKB <= maxSizeKB) {
            QFile outFile(destFile);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(ba);
                outFile.close();
                success = true;
                qDebug() << "Compressed image to" << sizeKB << "KB with quality" << quality;
                break;
            }
        }
        
        quality -= 10;
    }
    
    if (!success) {
        qWarning() << "Could not compress image to target size:" << sourceFile;
    }
    
    return success;
}

bool ImageProcessor::processImage(const QString &sourceFile, 
                                    const QString &tempDir,
                                    const QString &tempFileName,
                                    const QString &thumbFileName,
                                    QPixmap &outThumbnail)
{
    if (!ensureDirExists(tempDir)) {
        qCritical() << "Failed to create temp directory:" << tempDir;
        return false;
    }
    
    QString destImagePath = tempDir + "/" + tempFileName;
    QString destThumbPath = tempDir + "/" + thumbFileName;
    
    qint64 fileSizeKB = getFileSizeKB(sourceFile);
    qDebug() << "Processing image:" << sourceFile << "size:" << fileSizeKB << "KB";
    
    bool imageProcessed = false;
    
    if (fileSizeKB > 500) {
        qDebug() << "Image exceeds 500KB, compressing...";
        imageProcessed = compressImage(sourceFile, destImagePath, 500);
    } else {
        qDebug() << "Copying image to temp...";
        imageProcessed = copyFile(sourceFile, destImagePath);
    }
    
    if (!imageProcessed) {
        qCritical() << "Failed to process main image:" << sourceFile;
        return false;
    }
    
    outThumbnail = createThumbnail(sourceFile, 100);
    if (outThumbnail.isNull()) {
        qCritical() << "Failed to create thumbnail for:" << sourceFile;
        return false;
    }
    
    QFileInfo fi(thumbFileName);
    QString format = fi.suffix().toLower();
    if (format.isEmpty()) format = "jpg";
    if (format == "jpeg") format = "jpg";
    
    bool thumbSaved = false;
    if (format == "jpg" || format == "jpeg") {
        thumbSaved = outThumbnail.save(destThumbPath, "JPEG", 85);
    } else if (format == "png") {
        thumbSaved = outThumbnail.save(destThumbPath, "PNG");
    } else {
        thumbSaved = outThumbnail.save(destThumbPath, format.toUtf8().constData());
    }
    
    if (!thumbSaved) {
        qCritical() << "Failed to save thumbnail to:" << destThumbPath;
        return false;
    }
    
    qDebug() << "Image processing complete:";
    qDebug() << "  Main image:" << destImagePath;
    qDebug() << "  Thumbnail:" << destThumbPath;
    
    return true;
}
