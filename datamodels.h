#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <QPixmap>
#include <QDateTime>

struct Contact
{
    int id = 0;
    int comp_id = 0;
    QString cont_name;
    QString cont_position;
    QString cont_phone;
    QString cont_email;
    QString cont_wechat;
    double cont_wax_price = 0.0;
    QString cont_created_at;
    QString cont_remark;
};

struct Designer
{
    int id = 0;
    QString dser_name;
    QString dser_contact_phone;
    QString dser_email;
    int dser_status = 0;
    QString dser_created_at;
    QString dser_updated_at;
};

struct OrderItem
{
    QString sourceFileName;
    QString sourceFilePath;
    
    int cont_id = 0;
    QString cont_name;
    bool cont_found = false;
    
    QString product_name;
    
    int dser_id = 0;
    QString dser_name;
    bool dser_found = false;
    
    int design_fee_base = 0;
    double design_fee = 0.0;
    int pieces = 1;
    bool is_reorder = false;
    double wax_weight = 0.0;
    double wax_price = 0.0;
    double wax_total = 0.0;
    double estimated_total = 0.0;
    
    QString uid;
    QString tempImagePath;
    QString tempThumbPath;
    QPixmap thumbnail;
    
    bool imageUploaded = false;
    bool orderUploaded = false;
    bool uploadSuccess = false;
    QString uploadError;
};

struct ParsedFileName
{
    QString customerName;
    QString productName;
    QString designerName;
    int designFeeBase = 0;
    int pieces = 1;
    bool isReorder = false;
    double waxWeight = 0.0;
    bool valid = false;
};

#endif 
