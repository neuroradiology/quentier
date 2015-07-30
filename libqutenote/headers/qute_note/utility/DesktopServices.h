#ifndef __LIB_QUTE_NOTE__UTILITY__DESKTOP_SERVICES_H
#define __LIB_QUTE_NOTE__UTILITY__DESKTOP_SERVICES_H

#include <tools/Linkage.h>
#include <QString>
#include <QStyle>
#include <QFileDialog>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace qute_note {

// Convenience functions for some paths important for the application
const QString QUTE_NOTE_EXPORT applicationPersistentStoragePath();
const QString QUTE_NOTE_EXPORT applicationTemporaryStoragePath();
const QString QUTE_NOTE_EXPORT homePath();

QUTE_NOTE_EXPORT QStyle * applicationStyle();
const QString QUTE_NOTE_EXPORT humanReadableSize(const int bytes);

// Convenience functions for message boxes with proper modality on Mac OS X
void QUTE_NOTE_EXPORT genericMessageBox(QWidget * parent, const QString & title,
                                        const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT informationMessageBox(QWidget * parent, const QString & title,
                                            const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT warningMessageBox(QWidget * parent, const QString & title,
                                        const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT criticalMessageBox(QWidget * parent, const QString & title,
                                         const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT questionMessageBox(QWidget * parent, const QString & title,
                                         const QString & briefText, const QString & detailedText = QString());

// Convenience function for critical message box due to internal error, has built-in title
// ("Internal error") and brief text so the caller only needs to provide the detailed text
void QUTE_NOTE_EXPORT internalErrorMessageBox(QWidget * parent, QString detailedText = QString());

// Convenience functions for file dialogues with proper modality on Mac OS X
const QString QUTE_NOTE_EXPORT getExistingFolderDialog(QWidget * parent, const QString & title,
                                                       const QString & initialFolder,
                                                       QFileDialog::Options options = QFileDialog::ShowDirsOnly);

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__DESKTOP_SERVICES_H