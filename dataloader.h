#ifndef DATALOADER_H
#define DATALOADER_H

#include "datamodels.h"
#include <QObject>
#include <QMap>
#include <QVector>

class DataLoader : public QObject
{
    Q_OBJECT
public:
    explicit DataLoader(QObject *parent = nullptr);
    ~DataLoader() = default;
    
    bool loadContacts(const QString &filePath);
    bool loadDesigners(const QString &filePath);
    
    Contact getContactByName(const QString &name) const;
    Designer getDesignerByName(const QString &name) const;
    
    bool hasContact(const QString &name) const;
    bool hasDesigner(const QString &name) const;
    
    int contactCount() const { return m_contacts.size(); }
    int designerCount() const { return m_designers.size(); }
    
    void printContacts() const;
    void printDesigners() const;

private:
    QMap<QString, Contact> m_contacts;
    QMap<QString, Designer> m_designers;
};

#endif 
