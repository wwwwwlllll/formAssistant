#include "filenameparser.h"
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpressionMatch>

FileNameParser::FileNameParser(QObject *parent)
    : QObject(parent)
{
}

QStringList FileNameParser::extractBracedContent(const QString &str)
{
    QStringList result;
    QRegularExpression re("\\{([^}]+)\\}");
    QRegularExpressionMatchIterator i = re.globalMatch(str);
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            result.append(match.captured(1).trimmed());
        }
    }
    return result;
}

int FileNameParser::parseDesignFeeBase(const QString &str)
{
    QString s = str.trimmed();
    if (s.compare("L", Qt::CaseInsensitive) == 0 || 
        s.compare("0L", Qt::CaseInsensitive) == 0) {
        return 0;
    }
    
    QRegularExpression re("^(\\d+)L$");
    QRegularExpressionMatch match = re.match(s);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    return 0;
}

int FileNameParser::parsePieces(const QString &str)
{
    QString s = str.trimmed();
    QRegularExpression re("^(\\d+)件$");
    QRegularExpressionMatch match = re.match(s);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    return 1;
}

double FileNameParser::parseWaxWeight(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();
    
    int lastSpace = baseName.lastIndexOf(' ');
    if (lastSpace == -1) {
        return 0.0;
    }
    
    QString waxStr = baseName.mid(lastSpace + 1).trimmed();
    bool ok = false;
    double weight = waxStr.toDouble(&ok);
    return ok ? weight : 0.0;
}

ParsedFileName FileNameParser::parse(const QString &fileName)
{
    ParsedFileName result;
    result.valid = false;
    
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();
    
    QStringList braced = extractBracedContent(baseName);
    
    if (braced.size() < 5) {
        qDebug() << "File name has insufficient fields, got" << braced.size() << "fields";
        return result;
    }
    
    if (braced.size() >= 2) {
        result.customerName = braced[1].trimmed();
    }
    
    if (braced.size() >= 3) {
        result.productName = braced[2].trimmed();
    }
    
    if (braced.size() >= 4) {
        result.designerName = braced[3].trimmed();
    }
    
    if (braced.size() >= 5) {
        result.designFeeBase = parseDesignFeeBase(braced[4]);
    }
    
    if (braced.size() >= 6) {
        result.pieces = parsePieces(braced[5]);
    }
    
    if (braced.size() >= 7) {
        QString reorder = braced[6].trimmed();
        if (reorder == "补") {
            result.isReorder = true;
        } else if (reorder == "0") {
            result.isReorder = false;
        }
    }
    
    result.waxWeight = parseWaxWeight(baseName);
    
    if (!result.customerName.isEmpty() && !result.designerName.isEmpty()) {
        result.valid = true;
    }
    
    qDebug() << "Parsed file:" << fileName;
    qDebug() << "  Customer:" << result.customerName;
    qDebug() << "  Product:" << result.productName;
    qDebug() << "  Designer:" << result.designerName;
    qDebug() << "  DesignFeeBase:" << result.designFeeBase;
    qDebug() << "  Pieces:" << result.pieces;
    qDebug() << "  IsReorder:" << result.isReorder;
    qDebug() << "  WaxWeight:" << result.waxWeight;
    qDebug() << "  Valid:" << result.valid;
    
    return result;
}

bool FileNameParser::isImageFile(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp" || ext == "gif");
}
