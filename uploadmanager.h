#ifndef UPLOADMANAGER_H
#define UPLOADMANAGER_H

#include "datamodels.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

class UploadManager : public QObject
{
    Q_OBJECT
public:
    explicit UploadManager(QObject *parent = nullptr);
    ~UploadManager() = default;
    
    void setBaseUrl(const QString &url);
    QString baseUrl() const { return m_baseUrl.toString(); }
    
    void uploadImage(const OrderItem &item);
    void uploadOrder(const OrderItem &item);

signals:
    void imageUploadFinished(bool success, const QString &error, const QString &uid);
    void orderUploadFinished(bool success, const QString &error, const QString &uid);

private slots:
    void onImageUploadFinished();
    void onOrderUploadFinished();
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *m_nam;
    QUrl m_baseUrl;
    
    QJsonObject createOrderJson(const OrderItem &item);
};

#endif 
