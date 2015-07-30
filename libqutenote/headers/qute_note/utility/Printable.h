#ifndef __LIB_QUTE_NOTE__UTILITY__PRINTABLE_H
#define __LIB_QUTE_NOTE__UTILITY__PRINTABLE_H

#include <tools/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QEverCloud.h>
#include <oauth.h>

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
    Printable();
    Printable(const Printable & other);
    virtual ~Printable();

private:
    Printable & operator=(const Printable & other) Q_DECL_DELETE;
};

} // namespace qute_note

// printing operators for existing classes not inheriting from Printable

template <class T>
const QString ToQString(const T & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << object;
    return str;
}

template <class TKey, class TValue>
const QString ToQString(const QHash<TKey, TValue> & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << "QHash: \n";

    typedef typename QHash<TKey,TValue>::const_iterator CIter;
    CIter hashEnd = object.end();
    for(CIter it = object.begin(); it != hashEnd; ++it) {
        strm << "[" << it.key() << "] = " << it.value() << ";\n";
    }
    return str;
}

template <class T>
const QString ToQString(const QSet<T> & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << "QSet: \n";

    typedef typename QSet<T>::const_iterator CIter;
    CIter setEnd = object.end();
    for(CIter it = object.begin(); it != setEnd; ++it) {
        strm << "[" << *it << "];\n";
    }
    return str;
}

#define __QUTE_NOTE_DECLARE_PRINTABLE(type, ...) \
    QTextStream & operator << (QTextStream & strm, const type & obj); \
    inline QDebug & operator << (QDebug & debug, const type & obj) \
    { \
        debug << ToQString<type, ##__VA_ARGS__>(obj); \
        return debug; \
    }

__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::BusinessUserInfo)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::PremiumInfo)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Accounting)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::UserAttributes)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::NoteAttributes)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::PrivilegeLevel::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::QueryFormat::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebookPrivilegeLevel::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::NoteSortOrder::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::NotebookRestrictions)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebookInstanceRestrictions::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::ResourceAttributes)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Resource)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SyncChunk)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Tag)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SavedSearch)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::LinkedNotebook)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Notebook)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Publishing)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebook)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::BusinessNotebook)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::User)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebookRecipientSettings)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::ReminderEmailConfig::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::PremiumOrderStatus::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::BusinessUserRole::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SponsoredGroupRole::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Note)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::EDAMErrorCode::type)
__QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::EvernoteOAuthWebView::OAuthResult)

#endif // __LIB_QUTE_NOTE__UTILITY__PRINTABLE_H