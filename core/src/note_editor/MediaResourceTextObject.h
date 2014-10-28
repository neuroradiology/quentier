#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__MEDIA_RESOURCE_TEXT_OBJECT_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__MEDIA_RESOURCE_TEXT_OBJECT_H

#include <tools/Linkage.h>
#include <tools/qt4helper.h>
#include <QTextObjectInterface>

class QUTE_NOTE_EXPORT MediaResourceTextObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit MediaResourceTextObject() {}
    virtual ~MediaResourceTextObject() Q_DECL_OVERRIDE {}
    
public:
    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) = 0;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) = 0;
};

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__MEDIA_RESOURCE_TEXT_OBJECT_H
