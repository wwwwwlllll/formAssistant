#ifndef FILENAMEPARSER_H
#define FILENAMEPARSER_H

#include "datamodels.h"
#include <QObject>
#include <QString>
#include <QRegularExpression>

class FileNameParser : public QObject
{
    Q_OBJECT
public:
    explicit FileNameParser(QObject *parent = nullptr);
    ~FileNameParser() = default;
    
    static ParsedFileName parse(const QString &fileName);
    static bool isImageFile(const QString &fileName);

private:
    static QStringList extractBracedContent(const QString &str);
    static int parseDesignFeeBase(const QString &str);
    static int parsePieces(const QString &str);
    static double parseWaxWeight(const QString &fileName);
};

#endif 
