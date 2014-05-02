#ifndef __QUTE_NOTE__TOOLS_PRINTABLE_H
#define __QUTE_NOTE__TOOLS_PRINTABLE_H

#include "Linkage.h"
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <Types_types.h>
#include <QEverCloud.h>

namespace qute_note {

/**
 * @brief The Printable class is the interface for QuteNote's internal classes
 * which should be able to write themselves into QTextStream and/or convert to QString
 */
class QUTE_NOTE_EXPORT Printable
{
public:
    virtual QTextStream & Print(QTextStream & strm) const = 0;

    virtual const QString ToQString() const;

    friend QUTE_NOTE_EXPORT QTextStream & operator << (QTextStream & strm,
                                                       const Printable & printable);
    friend QUTE_NOTE_EXPORT QDebug & operator << (QDebug & debug,
                                                  const Printable & printable);
protected:
    Printable() = default;
    virtual ~Printable() = default;

private:
    Printable(const Printable & other) = delete;
    Printable & operator=(const Printable & other) = delete;
};

} // namespace qute_note

// printing operators for existing classes not inheriting from Printable

template<class T>
const QString ToQString(const T & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << object;
    return std::move(str);
}

#define __QUTE_NOTE_DECLARE_PRINTABLE(type) \
    QTextStream & operator << (QTextStream & strm, const type & obj); \
    inline QDebug & operator << (QDebug & debug, const type & obj) \
    { \
        debug << ToQString<type>(obj); \
        return debug; \
    }

__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::BusinessUserInfo)
__QUTE_NOTE_DECLARE_PRINTABLE(evernote::edam::Guid)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::PremiumInfo)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Accounting)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::UserAttributes)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::NoteAttributes)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::ResourceAttributes)
__QUTE_NOTE_DECLARE_PRINTABLE(evernote::edam::PrivilegeLevel::type)
__QUTE_NOTE_DECLARE_PRINTABLE(evernote::edam::QueryFormat::type)
__QUTE_NOTE_DECLARE_PRINTABLE(evernote::edam::SharedNotebookPrivilegeLevel::type)
__QUTE_NOTE_DECLARE_PRINTABLE(evernote::edam::NoteSortOrder::type)
__QUTE_NOTE_DECLARE_PRINTABLE(evernote::edam::NotebookRestrictions)
__QUTE_NOTE_DECLARE_PRINTABLE(evernote::edam::SharedNotebookInstanceRestrictions::type)

#endif // __QUTE_NOTE__TOOLS_PRINTABLE_H
