#include "uploadmanager.h"
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QJsonArray>
#include <QDateTime>

UploadManager::UploadManager(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void UploadManager::setBaseUrl(const QString &url)
{
    m_baseUrl = QUrl(url);
    qDebug() << "UploadManager base URL set to:" << m_baseUrl.toString();
}

void UploadManager::uploadImage(const OrderItem &item)
{
    if (item.tempImagePath.isEmpty() || item.tempThumbPath.isEmpty()) {
        emit imageUploadFinished(false, "Image paths not set", item.uid);
        return;
    }
    
    QUrl uploadUrl = m_baseUrl;
    uploadUrl.setPath("/twf/jdesign/v1/img/upload");
    
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    QFile *imageFile = new QFile(item.tempImagePath);
    if (!imageFile->open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open image file:" << item.tempImagePath;
        emit imageUploadFinished(false, "Cannot open image file", item.uid);
        delete imageFile;
        delete multiPart;
        return;
    }
    
    QFileInfo imageFi(item.tempImagePath);
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                        QVariant(QString("form-data; name=\"image\"; filename=\"%1\"").arg(imageFi.fileName())));
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setBodyDevice(imageFile);
    imageFile->setParent(multiPart);
    multiPart->append(imagePart);
    
    QFile *thumbFile = new QFile(item.tempThumbPath);
    if (!thumbFile->open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open thumbnail file:" << item.tempThumbPath;
        emit imageUploadFinished(false, "Cannot open thumbnail file", item.uid);
        delete thumbFile;
        delete multiPart;
        return;
    }
    
    QFileInfo thumbFi(item.tempThumbPath);
    QHttpPart thumbPart;
    thumbPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                        QVariant(QString("form-data; name=\"thumbnail\"; filename=\"%1\"").arg(thumbFi.fileName())));
    thumbPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    thumbPart.setBodyDevice(thumbFile);
    thumbFile->setParent(multiPart);
    multiPart->append(thumbPart);
    
    QHttpPart uidPart;
    uidPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                      QVariant("form-data; name=\"uid\""));
    uidPart.setBody(item.uid.toUtf8());
    multiPart->append(uidPart);
    
    QNetworkRequest request(uploadUrl);
    QNetworkReply *reply = m_nam->post(request, multiPart);
    multiPart->setParent(reply);
    
    reply->setProperty("uid", item.uid);
    
    connect(reply, &QNetworkReply::finished, this, &UploadManager::onImageUploadFinished);
    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &UploadManager::onNetworkError);
    
    qDebug() << "Uploading image for UID:" << item.uid;
    qDebug() << "  URL:" << uploadUrl.toString();
    qDebug() << "  Image:" << item.tempImagePath;
    qDebug() << "  Thumbnail:" << item.tempThumbPath;
}

void UploadManager::onImageUploadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString uid = reply->property("uid").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        qCritical() << "Image upload failed for UID" << uid << ":" << error;
        emit imageUploadFinished(false, error, uid);
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    qDebug() << "Image upload response for UID" << uid << ":" << responseData;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("JSON parse error: %1").arg(parseError.errorString());
        qCritical() << error;
        emit imageUploadFinished(false, error, uid);
        reply->deleteLater();
        return;
    }
    
    QJsonObject obj = doc.object();
    int code = obj["code"].toInt(-1);
    
    if (code == 200) {
        qDebug() << "Image upload SUCCESS for UID:" << uid;
        emit imageUploadFinished(true, QString(), uid);
    } else {
        QString message = obj["message"].toString("Unknown error");
        QString error = QString("Server error: %1 - %2").arg(code).arg(message);
        qCritical() << "Image upload FAILED for UID" << uid << ":" << error;
        emit imageUploadFinished(false, error, uid);
    }
    
    reply->deleteLater();
}

void UploadManager::uploadOrder(const OrderItem &item)
{
    QUrl uploadUrl = m_baseUrl;
    uploadUrl.setPath("/twf/jdesign/v1/orders/add");
    
    QJsonObject jsonData = createOrderJson(item);
    QJsonDocument doc(jsonData);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "Uploading order for UID:" << item.uid;
    qDebug() << "  URL:" << uploadUrl.toString();
    qDebug() << "  JSON:" << doc.toJson(QJsonDocument::Indented);
    
    QNetworkRequest request(uploadUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply *reply = m_nam->post(request, jsonBytes);
    reply->setProperty("uid", item.uid);
    
    connect(reply, &QNetworkReply::finished, this, &UploadManager::onOrderUploadFinished);
    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &UploadManager::onNetworkError);
}

void UploadManager::onOrderUploadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString uid = reply->property("uid").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        qCritical() << "Order upload failed for UID" << uid << ":" << error;
        emit orderUploadFinished(false, error, uid);
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    qDebug() << "Order upload response for UID" << uid << ":" << responseData;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("JSON parse error: %1").arg(parseError.errorString());
        qCritical() << error;
        emit orderUploadFinished(false, error, uid);
        reply->deleteLater();
        return;
    }
    
    QJsonObject obj = doc.object();
    int code = obj["code"].toInt(-1);
    
    if (code == 200) {
        qDebug() << "Order upload SUCCESS for UID:" << uid;
        emit orderUploadFinished(true, QString(), uid);
    } else {
        QString message = obj["message"].toString("Unknown error");
        QString error = QString("Server error: %1 - %2").arg(code).arg(message);
        qCritical() << "Order upload FAILED for UID" << uid << ":" << error;
        emit orderUploadFinished(false, error, uid);
    }
    
    reply->deleteLater();
}

void UploadManager::onNetworkError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        qCritical() << "Network error:" << reply->errorString();
    }
}

QJsonObject UploadManager::createOrderJson(const OrderItem &item)
{
    QJsonObject data;
    
    data["id"] = 0;
    data["order_no"] = "";
    data["comp_id"] = 0;
    data["cont_id"] = item.cont_id;
    data["dser_id"] = item.dser_id;
    data["order_subject"] = item.product_name;
    data["order_description"] = "";
    data["order_status"] = 0;
    data["order_type"] = item.is_reorder ? 1 : 0;
    data["order_design_image"] = item.uid;
    data["order_pieces_amount"] = item.pieces;
    data["order_piece_weight"] = item.wax_weight;
    
    if (item.is_reorder) {
        data["order_wax_weight"] = 0.0;
        data["order_wax_supp_weight"] = item.wax_total;
    } else {
        data["order_wax_weight"] = item.wax_total;
        data["order_wax_supp_weight"] = 0.0;
    }
    
    data["order_design_cost"] = item.design_fee;
    data["order_wax_cost"] = item.wax_total;
    data["order_cost_estimate"] = item.estimated_total;
    data["order_created_at"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    data["order_updated_at"] = "";
    
    QJsonObject root;
    root["data"] = data;
    
    return root;
}
