#ifndef QUTE_NOTE_MODELS_NOTEBOOK_STACK_ITEM_H
#define QUTE_NOTE_MODELS_NOTEBOOK_STACK_ITEM_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NotebookModelItem)

class NotebookStackItem: public Printable
{
public:
    NotebookStackItem(const QString & name = QString()) :
        m_name(name)
    {}

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString     m_name;
};

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_NOTEBOOK_STACK_ITEM_H
