#ifndef __QUTE_NOTE__CLIENT__TYPES__QEVERCLOUD_OPTIONAL_QSTRING_H
#define __QUTE_NOTE__CLIENT__TYPES__QEVERCLOUD_OPTIONAL_QSTRING_H

#include <QEverCloud.h>

namespace qevercloud {

template<> inline Optional<QString> & Optional<QString>::operator=(const QString & value) {
    value_ = value;
    isSet_ = !value.isEmpty();
    return *this;
}

/*
bool operator==(const UserAttributes & lhs, const UserAttributes & rhs);
bool operator!=(const UserAttributes & lhs, const UserAttributes & rhs);

bool operator==(const Accounting & lhs, const Accounting & rhs);
bool operator!=(const Accounting & lhs, const Accounting & rhs);

bool operator==(const BusinessUserInfo & lhs, const BusinessUserInfo & rhs);
bool operator!=(const BusinessUserInfo & lhs, const BusinessUserInfo & rhs);

bool operator==(const PremiumInfo & lhs, const PremiumInfo & rhs);
bool operator!=(const PremiumInfo & lhs, const PremiumInfo & rhs);

bool operator==(const Data & lhs, const Data & rhs);
bool operator!=(const Data & lhs, const Data & rhs);

bool operator==(const LazyMap & lhs, const LazyMap & rhs);
bool operator!=(const LazyMap & lhs, const LazyMap & rhs);

bool operator==(const ResourceAttributes & lhs, const ResourceAttributes & rhs);
bool operator!=(const ResourceAttributes & lhs, const ResourceAttributes & rhs);

bool operator==(const Resource & lhs, const Resource & rhs);
bool operator!=(const Resource & lhs, const Resource & rhs);

bool operator==(const NoteAttributes & lhs, const NoteAttributes & rhs);
bool operator!=(const NoteAttributes & lhs, const NoteAttributes & rhs);

bool operator==(const Note & lhs, const Note & rhs);
bool operator!=(const Note & lhs, const Note & rhs);
*/
}

#endif // __QUTE_NOTE__CLIENT__TYPES__QEVERCLOUD_OPTIONAL_QSTRING_H
