#include <qute_note/enml/ENMLConverter.h>
#include "ENMLConverter_p.h"

#ifndef USE_QT_WEB_ENGINE
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#endif

#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ENMLConverter::ENMLConverter() :
    d_ptr(new ENMLConverterPrivate)
{}

ENMLConverter::~ENMLConverter()
{
    delete d_ptr;
}

bool ENMLConverter::htmlToNoteContent(const QString & html, QString & noteContent,
                                      DecryptedTextManager & decryptedTextManager,
                                      QString & errorDescription) const
{
    QNDEBUG("ENMLConverter::htmlToNoteContent");

    Q_D(const ENMLConverter);
    return d->htmlToNoteContent(html, noteContent, decryptedTextManager, errorDescription);
}

bool ENMLConverter::noteContentToHtml(const QString & noteContent, QString & html,
                                      QString & errorDescription,
                                      DecryptedTextManager & decryptedTextManager,
                                      NoteContentToHtmlExtraData & extraData
#ifndef USE_QT_WEB_ENGINE
                                      , const NoteEditorPluginFactory * pluginFactory
#endif
                                      ) const
{
    QNDEBUG("ENMLConverter::noteContentToHtml");

    Q_D(const ENMLConverter);
    return d->noteContentToHtml(noteContent, html, errorDescription, decryptedTextManager, extraData
#ifndef USE_QT_WEB_ENGINE
                                , pluginFactory
#endif
                                );
}

bool ENMLConverter::validateEnml(const QString & enml, QString & errorDescription) const
{
    Q_D(const ENMLConverter);
    return d->validateEnml(enml, errorDescription);
}

bool ENMLConverter::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                           QString & errorMessage)
{
    return ENMLConverterPrivate::noteContentToPlainText(noteContent, plainText, errorMessage);
}

bool ENMLConverter::noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                             QString & errorMessage, QString * plainText)
{
    return ENMLConverterPrivate::noteContentToListOfWords(noteContent, listOfWords, errorMessage, plainText);
}

QStringList ENMLConverter::plainTextToListOfWords(const QString & plainText)
{
    return ENMLConverterPrivate::plainTextToListOfWords(plainText);
}

QString ENMLConverter::toDoCheckboxHtml(const bool checked, const quint64 idNumber)
{
    return ENMLConverterPrivate::toDoCheckboxHtml(checked, idNumber);
}

QString ENMLConverter::encryptedTextHtml(const QString & encryptedText, const QString & hint,
                                         const QString & cipher, const size_t keyLength,
                                         const quint64 enCryptIndex)
{
    return ENMLConverterPrivate::encryptedTextHtml(encryptedText, hint, cipher, keyLength, enCryptIndex);
}

QString ENMLConverter::decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                         const QString & hint, const QString & cipher,
                                         const size_t keyLength, const quint64 enDecryptedIndex)
{
    return ENMLConverterPrivate::decryptedTextHtml(decryptedText, encryptedText,
                                                   hint, cipher, keyLength, enDecryptedIndex);
}

void ENMLConverter::escapeString(QString & string)
{
    ENMLConverterPrivate::escapeString(string);
}

} // namespace qute_note

