#include "dataloader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QTextCodec>

DataLoader::DataLoader(QObject *parent)
    : QObject(parent)
{
}

bool DataLoader::loadContacts(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open contacts file:" << filePath << file.errorString();
        return false;
    }
    
    QTextStream in(&file);
    in.setCodec("UTF-8");
    
    m_contacts.clear();
    QString line;
    bool isFirstLine = true;
    
    while (in.readLineInto(&line)) {
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }
        
        if (line.trimmed().isEmpty())
            continue;
        
        QStringList fields = line.split("|");
        if (fields.size() < 3)
            continue;
        
        Contact contact;
        contact.id = fields[0].toInt();
        contact.comp_id = fields[1].toInt();
        contact.cont_name = fields[2].trimmed();
        
        if (fields.size() > 3) contact.cont_position = fields[3].trimmed();
        if (fields.size() > 4) contact.cont_phone = fields[4].trimmed();
        if (fields.size() > 5) contact.cont_email = fields[5].trimmed();
        if (fields.size() > 6) contact.cont_wechat = fields[6].trimmed();
        if (fields.size() > 7) contact.cont_wax_price = fields[7].toDouble();
        if (fields.size() > 8) contact.cont_created_at = fields[8].trimmed();
        if (fields.size() > 9) contact.cont_remark = fields[9].trimmed();
        
        if (!contact.cont_name.isEmpty()) {
            m_contacts[contact.cont_name] = contact;
        }
    }
    
    file.close();
    return true;
}

bool DataLoader::loadDesigners(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open designers file:" << filePath << file.errorString();
        return false;
    }
    
    QTextStream in(&file);
    in.setCodec("UTF-8");
    
    m_designers.clear();
    QString line;
    bool isFirstLine = true;
    
    while (in.readLineInto(&line)) {
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }
        
        if (line.trimmed().isEmpty())
            continue;
        
        QStringList fields = line.split("|");
        if (fields.size() < 2)
            continue;
        
        Designer designer;
        designer.id = fields[0].toInt();
        designer.dser_name = fields[1].trimmed();
        
        if (fields.size() > 2) designer.dser_contact_phone = fields[2].trimmed();
        if (fields.size() > 3) designer.dser_email = fields[3].trimmed();
        if (fields.size() > 4) designer.dser_status = fields[4].toInt();
        if (fields.size() > 5) designer.dser_created_at = fields[5].trimmed();
        if (fields.size() > 6) designer.dser_updated_at = fields[6].trimmed();
        
        if (!designer.dser_name.isEmpty()) {
            m_designers[designer.dser_name] = designer;
        }
    }
    
    file.close();
    return true;
}

Contact DataLoader::getContactByName(const QString &name) const
{
    return m_contacts.value(name.trimmed());
}

Designer DataLoader::getDesignerByName(const QString &name) const
{
    return m_designers.value(name.trimmed());
}

bool DataLoader::hasContact(const QString &name) const
{
    return m_contacts.contains(name.trimmed());
}

bool DataLoader::hasDesigner(const QString &name) const
{
    return m_designers.contains(name.trimmed());
}

void DataLoader::printContacts() const
{
    qDebug() << "========================================";
    qDebug() << "Loaded" << m_contacts.size() << "contacts:";
    qDebug() << "========================================";
    
    for (auto it = m_contacts.begin(); it != m_contacts.end(); ++it) {
        const Contact &c = it.value();
        qDebug().noquote() << "ID:" << c.id 
                 << "| Name:" << c.cont_name 
                 << "| Comp ID:" << c.comp_id
                 << "| Wax Price:" << c.cont_wax_price;
    }
    qDebug() << "========================================";
}

void DataLoader::printDesigners() const
{
    qDebug() << "========================================";
    qDebug() << "Loaded" << m_designers.size() << "designers:";
    qDebug() << "========================================";
    
    for (auto it = m_designers.begin(); it != m_designers.end(); ++it) {
        const Designer &d = it.value();
        qDebug().noquote() << "ID:" << d.id 
                 << "| Name:" << d.dser_name 
                 << "| Status:" << d.dser_status;
    }
    qDebug() << "========================================";
}
