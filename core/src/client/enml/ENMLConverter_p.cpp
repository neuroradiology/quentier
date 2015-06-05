#include "ENMLConverter_p.h"
#include "HTMLCleaner.h"
#include <client/types/Note.h>
#include <logging/QuteNoteLogger.h>
#include <libxml/xmlreader.h>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QScopedPointer>
#include <QDomDocument>

namespace qute_note {

#define WRAP(x) \
    << x

static const QSet<QString> forbiddenXhtmlTags = QSet<QString>()
#include "forbiddenXhtmlTags.inl"
;

static const QSet<QString> forbiddenXhtmlAttributes = QSet<QString>()
#include "forbiddenXhtmlAttributes.inl"
;


static const QSet<QString> evernoteSpecificXhtmlTags = QSet<QString>()
#include "evernoteSpecificXhtmlTags.inl"
;


static const QSet<QString> allowedXhtmlTags = QSet<QString>()
#include "allowedXhtmlTags.inl"
;

#undef WRAP

ENMLConverterPrivate::ENMLConverterPrivate() :
    m_pHtmlCleaner(nullptr),
    m_cachedNoteContent(),
    m_cachedConvertedXml()
{}

ENMLConverterPrivate::~ENMLConverterPrivate()
{
    delete m_pHtmlCleaner;
}

bool ENMLConverterPrivate::htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::htmlToNoteContent: note local guid = " << note.localGuid());

    if (!m_pHtmlCleaner) {
        m_pHtmlCleaner = new HTMLCleaner;
    }

    m_cachedConvertedXml.resize(0);
    bool res = m_pHtmlCleaner->htmlToXml(html, m_cachedConvertedXml, errorDescription);
    if (!res) {
        errorDescription.prepend(QT_TR_NOOP("Could not clean up note's html: "));
        return false;
    }

    QXmlStreamReader reader(m_cachedConvertedXml);

    m_cachedNoteContent.resize(0);
    QXmlStreamWriter writer(&m_cachedNoteContent);
    writer.writeStartDocument();
    writer.writeDTD("<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">");

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isEndDocument()) {
            writer.writeEndDocument();
            break;
        }

        if (reader.isStartElement())
        {
            QString name = reader.name().toString();
            if (name == "form") {
                QNTRACE("Skipping <form> tag");
                continue;
            }
            else if (name == "html") {
                QNTRACE("Skipping <html> tag");
                continue;
            }
            else if (name == "body") {
                name = "en-note";
                QNTRACE("Found \"body\" HTML tag, will replace it with \"en-note\" tag for written ENML");
            }

            const QString namespaceUri = reader.namespaceUri().toString();

            auto tagIt = forbiddenXhtmlTags.find(name);
            if (tagIt != forbiddenXhtmlTags.end()) {
                QNTRACE("Skipping forbidden XHTML tag: " << name);
                continue;
            }

            tagIt = allowedXhtmlTags.find(name);
            if (tagIt == allowedXhtmlTags.end()) {
                QNTRACE("Haven't found tag " << name << " within the list of allowed XHTML tags, skipping it");
                continue;
            }

            QXmlStreamAttributes attributes = reader.attributes();

            if ((name == "div") && attributes.hasAttribute(namespaceUri, "en-tag"))
            {
                const QString enTag = attributes.value(namespaceUri, "en-tag").toString();
                if (enTag == "en-decrypted")
                {
                    QNTRACE("Found decrypted text area, need to convert it back to en-crypt form");
                    // TODO: convert to en-crypt and ignore the style specific for this div
                }
            }
            else if (name == "img")
            {
                if (attributes.hasAttribute(namespaceUri, "src"))
                {
                    const QString srcValue = attributes.value(namespaceUri, "src").toString();
                    if (srcValue == "qrc:/checkbox_icons/checkbox_no.png") {
                        writer.writeStartElement(namespaceUri, "en-todo");
                        writer.writeEndElement();
                        continue;
                    }
                    else if (srcValue == "qrc:/checkbox_icons/checkbox_yes.png") {
                        writer.writeStartElement(namespaceUri, "en-todo");
                        writer.writeAttribute("checked", "true");
                        writer.writeEndElement();
                        continue;
                    }
                }
                else
                {
                    errorDescription = QT_TR_NOOP("Can't convert note to ENML: found img html tag without src attribute");
                    QNWARNING(errorDescription << ", html = " << html << ", cleaned up xml = " << m_cachedConvertedXml);
                    return false;
                }
            }

            if ((name == "object") || (name == "img"))
            {
                bool res = writeObjectInfoToEnml(reader, namespaceUri, writer, errorDescription);
                if (!res) {
                    return false;
                }

                continue;
            }

            // Erasing the forbidden attributes
            for(QXmlStreamAttributes::iterator it = attributes.begin(); it != attributes.end(); )
            {
                const QStringRef attributeName = it->name();
                if (isForbiddenXhtmlAttribute(attributeName.toString())) {
                    QNTRACE("Erasing the forbidden attribute " << attributeName);
                    it = attributes.erase(it);
                    continue;
                }

                ++it;
            }

            writer.writeStartElement(namespaceUri, name);
            writer.writeAttributes(attributes);
            QNTRACE("Wrote element: namespaceUri = " << namespaceUri << ", name = " << name << " and its attributes");
        }

        if (reader.isEndElement()) {
            writer.writeEndElement();
        }
    }

    res = validateEnml(m_cachedNoteContent, errorDescription);
    if (!res)
    {
        if (!errorDescription.isEmpty()) {
            errorDescription = QT_TR_NOOP("Can't validate ENML with DTD: ") + errorDescription;
        }
        else {
            errorDescription = QT_TR_NOOP("Failed to convert, produced ENML is invalid according to dtd");
        }

        QNWARNING(errorDescription << ": " << m_cachedNoteContent << "\n\nSource html: " << html
                  << "\n\nCleaned up & converted xml: " << m_cachedConvertedXml);
        return false;
    }

    note.setContent(m_cachedNoteContent);
    return true;
}

bool ENMLConverterPrivate::noteContentToHtml(const Note & note, QString & html,
                                             QString & errorDescription,
                                             DecryptedTextCachePtr decryptedTextCache) const
{
    QNDEBUG("ENMLConverterPrivate::noteContentToHtml: note local guid = " << note.localGuid());

    html.resize(0);
    errorDescription.resize(0);

    if (!note.hasContent()) {
        return true;
    }

    QXmlStreamReader reader(note.content());

    QXmlStreamWriter writer(&html);
    writer.writeStartDocument();

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isEndDocument()) {
            writer.writeEndDocument();
            break;
        }

        if (reader.isStartElement())
        {
            QString name = reader.name().toString();
            const QString namespaceUri = reader.namespaceUri().toString();

            QXmlStreamAttributes attributes = reader.attributes();

            if (name == "en-note") {
                QNTRACE("Replacing en-note with \"body\" tag");
                name = "body";
            }
            else if (name == "en-media") {
                // TODO: process resource
                continue;
            }
            else if (name == "en-crypt")
            {
                bool res = writeEnCryptTagToHtml(reader, namespaceUri, writer, errorDescription, decryptedTextCache);
                if (!res) {
                    return res;
                }

                continue;
            }

            // NOTE: do not attempt to process en-todo tags here, it would be done below

            writer.writeStartElement(namespaceUri, name);
            writer.writeAttributes(attributes);

            if (reader.isCDATA()) {
                writer.writeCDATA(reader.text().toString());
            }
            else {
                writer.writeCharacters(reader.text().toString());
            }

            QNTRACE("Wrote element: namespaceUri = " << namespaceUri << ", name = "
                    << name << " and its attributes");
        }

        if (reader.isEndElement()) {
            writer.writeEndElement();
        }
    }

    bool res = convertEnToDoTagsToHtml(html, errorDescription);
    if (!res) {
        return false;
    }

    return true;
}

bool ENMLConverterPrivate::validateEnml(const QString & enml, QString & errorDescription) const
{
    errorDescription.resize(0);

#if QT_VERSION >= 0x050101
    QScopedPointer<unsigned char, QScopedPointerArrayDeleter<unsigned char> > str(reinterpret_cast<unsigned char*>(qstrdup(enml.toUtf8().constData())));
#else
    QScopedPointer<unsigned char, QScopedPointerArrayDeleter<unsigned char> > str(reinterpret_cast<unsigned char*>(qstrdup(enml.toAscii().constData())));
#endif

    xmlDocPtr pDoc = xmlParseDoc(static_cast<unsigned char*>(str.data()));
    if (!pDoc) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't parse enml to xml doc");
        QNWARNING(errorDescription);
        return false;
    }

    QFile dtdFile(":/enml2.dtd");
    if (!dtdFile.open(QIODevice::ReadOnly)) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't open resource file with DTD");
        QNWARNING(errorDescription);
        xmlFreeDoc(pDoc);
        return false;
    }

    QByteArray dtdRawData = dtdFile.readAll();

    xmlParserInputBufferPtr pBuf = xmlParserInputBufferCreateMem(dtdRawData.constData(), dtdRawData.size(),
                                                                 XML_CHAR_ENCODING_NONE);
    if (!pBuf) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't allocate input buffer for dtd validation");
        QNWARNING(errorDescription);
        xmlFreeDoc(pDoc);
        return false;
    }

    xmlDtdPtr pDtd = xmlIOParseDTD(NULL, pBuf, XML_CHAR_ENCODING_NONE);
    if (!pDtd) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't parse dtd from buffer");
        QNWARNING(errorDescription);
        xmlFreeDoc(pDoc);
        xmlFreeParserInputBuffer(pBuf);
        return false;
    }

    xmlParserCtxtPtr pContext = xmlNewParserCtxt();
    if (!pContext) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't allocate parses context");
        QNWARNING(errorDescription);
        xmlFreeDoc(pDoc);
        xmlFreeParserInputBuffer(pBuf);
        xmlFreeDtd(pDtd);
    }

    bool res = static_cast<bool>(xmlValidateDtd(&pContext->vctxt, pDoc, pDtd));

    xmlFreeDoc(pDoc);
    xmlFreeParserInputBuffer(pBuf);
    xmlFreeDtd(pDtd);
    xmlFreeParserCtxt(pContext);

    return res;
}

bool ENMLConverterPrivate::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                                  QString & errorMessage)
{
    // FIXME: remake using QXmlStreamReader

    plainText.resize(0);

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    bool res = enXmlDomDoc.setContent(noteContent, &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage += QT_TR_NOOP(". Error happened at line ") + QString::number(errorLine) +
                        QT_TR_NOOP(", at column ") + QString::number(errorColumn) +
                        QT_TR_NOOP(", bad note content: ") + noteContent;
        return false;
    }

    QDomElement docElem = enXmlDomDoc.documentElement();
    QString rootTag = docElem.tagName();
    if (rootTag != QString("en-note")) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage = QT_TR_NOOP("Bad note content: wrong root tag, should be \"en-note\", instead: ");
        errorMessage += rootTag;
        return false;
    }

    QDomNode nextNode = docElem.firstChild();
    while(!nextNode.isNull())
    {
        QDomElement element = nextNode.toElement();
        if (!element.isNull())
        {
            QString tagName = element.tagName();
            if (isAllowedXhtmlTag(tagName)) {
                plainText += element.text();
            }
            else if (isForbiddenXhtmlTag(tagName)) {
                errorMessage = QT_TR_NOOP("Found forbidden XHTML tag in ENML: ");
                errorMessage += tagName;
                return false;
            }
            else if (!isEvernoteSpecificXhtmlTag(tagName)) {
                errorMessage = QT_TR_NOOP("Found XHTML tag not listed as either "
                                          "forbidden or allowed one: ");
                errorMessage += tagName;
                return false;
            }
        }
        else
        {
            errorMessage = QT_TR_NOOP("Found QDomNode not convertable to QDomElement");
            return false;
        }

        nextNode = nextNode.nextSibling();
    }

    return true;
}

bool ENMLConverterPrivate::noteContentToListOfWords(const QString & noteContent,
                                                    QStringList & listOfWords,
                                                    QString & errorMessage, QString * plainText)
{
    QString _plainText;
    bool res = noteContentToPlainText(noteContent, _plainText, errorMessage);
    if (!res) {
        listOfWords.clear();
        return false;
    }

    if (plainText) {
        *plainText = _plainText;
    }

    listOfWords = plainTextToListOfWords(_plainText);
    return true;
}

QStringList ENMLConverterPrivate::plainTextToListOfWords(const QString & plainText)
{
    // Simply remove all non-word characters from plain text
    return plainText.split(QRegExp("\\W+"), QString::SkipEmptyParts);
}

QString ENMLConverterPrivate::getToDoCheckboxHtml(const bool checked)
{
    QString html = "<img src=\"qrc:/checkbox_icons/checkbox_";
    if (checked) {
        html += "yes.png\" class=\"checkbox_checked\" ";
    }
    else {
        html += "no.png\" class=\"checkbox_unchecked\" ";
    }

    html += "style=\"margin:0px 4px\" "
            "onmouseover=\"style.cursor=\\'default\\'\" "
            "onclick=\"if (this.className == \\'checkbox_unchecked\\') { "
            "this.src=\\'qrc:/checkbox_icons/checkbox_yes.png\\'; this.className = \\'checkbox_checked\\'; } "
            "else { this.src=\\'qrc:/checkbox_icons/checkbox_no.png\\'; this.className = \\'checkbox_unchecked\\' }\" />";
    return html;
}

bool ENMLConverterPrivate::isForbiddenXhtmlTag(const QString & tagName)
{
    auto it = forbiddenXhtmlTags.find(tagName);
    if (it == forbiddenXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isForbiddenXhtmlAttribute(const QString & attributeName)
{
    auto it = forbiddenXhtmlAttributes.find(attributeName);
    if (it == forbiddenXhtmlAttributes.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isEvernoteSpecificXhtmlTag(const QString & tagName)
{
    auto it = evernoteSpecificXhtmlTags.find(tagName);
    if (it == evernoteSpecificXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isAllowedXhtmlTag(const QString & tagName)
{
    auto it = allowedXhtmlTags.find(tagName);
    if (it == allowedXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::writeObjectInfoToEnml(QXmlStreamReader & reader, const QString & namespaceUri,
                                                 QXmlStreamWriter & writer, QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::writeObjectInfoToEnml");

    QXmlStreamAttributes attributes = reader.attributes();
    if (attributes.hasAttribute(namespaceUri, "en-tag"))
    {
        QString enTagValue = attributes.value(namespaceUri, "en-tag").toString();
        if (enTagValue == "en-crypt") {
            return writeEnCryptTagToEnml(reader, namespaceUri, writer, errorDescription);
        }
    }

    // TODO: implement
    return true;
}

bool ENMLConverterPrivate::writeEnCryptTagToEnml(QXmlStreamReader & reader,
                                                 const QString & namespaceUri,
                                                 QXmlStreamWriter & writer,
                                                 QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::writeEnCryptTagToEnml");

    QString hint;
    QString cipher;
    QString length;
    QString encryptedText;

    while(reader.readNextStartElement())
    {
        QString elementName = reader.name().toString();
        if (elementName != "param")  {
            break;
        }

        QXmlStreamAttributes paramAttributes;
        if (!paramAttributes.hasAttribute(namespaceUri, "name")) {
            errorDescription = QT_TR_NOOP("Detected invalid HTML: object tag with en-tag = en-crypt "
                                          "has child param tag without name attribute");
            return false;
        }

        if (!paramAttributes.hasAttribute(namespaceUri, "value")) {
            errorDescription = QT_TR_NOOP("Detected invalid HTML: object tag with en-tag = en-crypt "
                                          "has child param tag without value attribute");
            return false;
        }

        QString paramName = paramAttributes.value(namespaceUri, "name").toString();
        QString paramValue = paramAttributes.value(namespaceUri, "value").toString();

        if (paramName == "hint") {
            hint = paramValue;
        }
        else if (paramName == "cipher") {
            cipher = paramValue;
        }
        else if (paramName == "length") {
            length = paramValue;
        }
        else if (paramName == "encryptedText") {
            encryptedText = paramValue;
        }
        else {
            errorDescription = QT_TR_NOOP("Detected invalid HTML: object tag with en-tag = en-crypt "
                                          "has child param tag with unidentified name attribute: ") + paramName;
            return false;
        }
    }

    if (cipher.isEmpty()) {
        errorDescription = QT_TR_NOOP("Couldn't find cipher for en-crypt ENML tag");
        return false;
    }

    if (length.isEmpty()) {
        errorDescription = QT_TR_NOOP("Couldn't find (key) length for en-crypt ENML tag");
        return false;
    }

    if (encryptedText.isEmpty()) {
        errorDescription = QT_TR_NOOP("Couldn't find encrypted text for en-crypt ENML tag");
        return false;
    }

    writer.writeStartElement(namespaceUri, "en-crypt");
    writer.writeAttribute(namespaceUri, "cipher", cipher);
    writer.writeAttribute(namespaceUri, "length", length);
    if (!hint.isEmpty()) {
        writer.writeAttribute(namespaceUri, "hint", hint);
    }

    writer.writeCDATA(encryptedText);
    QNTRACE("Wrote en-crypt ENML element: cipher = " << cipher << ", length = " << length
            << ", encryptedText = " << encryptedText << ", hint = " << hint);
    return true;
}

bool ENMLConverterPrivate::convertEnToDoTagsToHtml(QString & html, QString & errorDescription) const
{
    QString toDoCheckboxUnchecked, toDoCheckboxChecked;

    // Replace en-todo tags with their html counterparts
    int toDoCheckboxIndex = html.indexOf("<en-todo");
    while(toDoCheckboxIndex >= 0)
    {
        if (Q_UNLIKELY(html.size() <= toDoCheckboxIndex + 10)) {
            errorDescription = QT_TR_NOOP("Detected incorrect html: ends with <en-todo");
            return false;
        }

        int tagEndIndex = html.indexOf("/>", toDoCheckboxIndex + 8);
        if (Q_UNLIKELY(tagEndIndex < 0)) {
            errorDescription = QT_TR_NOOP("Detected incorrect html: can't find the end of \"en-todo\" tag");
            return false;
        }

        // Need to check whether this en-todo tag is shortened i.e. if there are either nothing or only spaces between html[toDoCheckboxIndex+8] and html[tagEndIndex]
        int enToDoTagMiddleLength = tagEndIndex - toDoCheckboxIndex - 8;
        QString enToDoTagMiddle = html.mid(toDoCheckboxIndex + 8, enToDoTagMiddleLength);
        if (enToDoTagMiddle.simplified().isEmpty())
        {
            QNTRACE("Encountered shortened en-todo tag, will replace it with corresponding html");
            if (toDoCheckboxUnchecked.isEmpty()) {
                toDoCheckboxUnchecked = getToDoCheckboxHtml(/* checked = */ false);
            }

            html.replace(toDoCheckboxIndex, tagEndIndex + 2 - toDoCheckboxIndex, toDoCheckboxUnchecked);

            toDoCheckboxIndex = html.indexOf("<en-todo", toDoCheckboxIndex);
            continue;
        }

        QNTRACE("Encountered non-shortened en-todo tag");

        // NOTE: use heave protection from arbitrary number of spaces occuring between en-todo tag name, "checked" attribute name keyword, "=" sign and "true/false" attribute value
        int checkedAttributeIndex = html.indexOf("checked", toDoCheckboxIndex);
        if (Q_UNLIKELY(checkedAttributeIndex < 0)) {
            // NOTE: it can't be <en-todo/> as all of those were replaces with img tags above this loop
            errorDescription = QT_TR_NOOP("Detected incorrect html: can't find \"checked\" attribute within en-todo tag");
            return false;
        }

        int equalSignIndex = html.indexOf("=", checkedAttributeIndex);
        if (Q_UNLIKELY(equalSignIndex < 0)) {
            errorDescription = QT_TR_NOOP("Detected incorrect html: can't find \"=\" sign after \"checked\" attribute name within en-todo tag");
            return false;
        }

        tagEndIndex = html.indexOf("/>", equalSignIndex);
        if (Q_UNLIKELY(tagEndIndex < 0)) {
            errorDescription = QT_TR_NOOP("Detected incorrect html: can't find the end of \"en-todo\" tag");
            return false;
        }

        int trueValueIndex = html.indexOf("true", equalSignIndex);
        if ((trueValueIndex >= 0) && (trueValueIndex < tagEndIndex))
        {
            QNTRACE("found \"<en-todo checked=true/>\" tag, will replace it with html equivalent");

            if (toDoCheckboxChecked.isEmpty()) {
                toDoCheckboxChecked = getToDoCheckboxHtml(/* checked = */ true);
            }

            int replacedLength = tagEndIndex + 2 - toDoCheckboxIndex;
            html.replace(toDoCheckboxIndex, replacedLength, toDoCheckboxChecked);

            toDoCheckboxIndex = html.indexOf("<en-todo", toDoCheckboxIndex);
            continue;
        }

        int falseValueIndex = html.indexOf("false", equalSignIndex);
        if ((falseValueIndex >= 0) && (falseValueIndex < tagEndIndex))
        {
            QNTRACE("found \"<en-todo checked=false/>\" tag, will replace it with html equivalent");

            if (toDoCheckboxUnchecked.isEmpty()) {
                toDoCheckboxUnchecked = getToDoCheckboxHtml(/* checked = */ false);
            }

            int replacedLength = tagEndIndex + 2 - toDoCheckboxIndex;
            html.replace(toDoCheckboxIndex, replacedLength, toDoCheckboxUnchecked);

            toDoCheckboxIndex = html.indexOf("<en-todo", toDoCheckboxIndex);
            continue;
        }

        toDoCheckboxIndex = html.indexOf("<en-todo", toDoCheckboxIndex);
    }

    return true;
}

bool ENMLConverterPrivate::writeEnCryptTagToHtml(const QXmlStreamReader & reader,
                                                 const QString & namespaceUri,
                                                 QXmlStreamWriter & writer,
                                                 QString & errorDescription,
                                                 DecryptedTextCachePtr decryptedTextCache) const
{
    QNDEBUG("ENMLConverterPrivate::writeEnCryptTagToHtml");

    QString encryptedText = reader.text().toString();
    if (encryptedText.isEmpty()) {
        errorDescription = QT_TR_NOOP("Missing encrypted text data within en-crypt ENML tag");
        return false;
    }

    auto it = decryptedTextCache->find(encryptedText);
    if ((it != decryptedTextCache->end()) && it.value().second)
    {
        QNTRACE("Found encrypted text which has already been decrypted, "
                "cached and remembered for the whole session. Encrypted text = "
                << encryptedText);

        writer.writeStartElement(namespaceUri, "div");
        writer.writeAttribute(namespaceUri, "en-tag", "en-decrypted");
        writer.writeAttribute(namespaceUri, "style", "border: 2px solid; "
                                                     "border-color: rgb(195, 195, 195); "
                                                     "border-radius: 8px; "
                                                     "margin: 2px; "
                                                     "padding: 2px;");
        // TODO: add cipher, length and encryptedText attributes
        writer.writeCDATA(it.value().first);
        writer.writeEndElement();
        return true;
    }

    writer.writeStartElement(namespaceUri, "object");
    writer.writeAttribute(namespaceUri, "en-tag", "en-crypt");

    QXmlStreamAttributes attributes = reader.attributes();
    if (attributes.hasAttribute(namespaceUri, "hint")) {
        QString hint = attributes.value(namespaceUri, "hint").toString();
        writer.writeStartElement(namespaceUri, "param");
        writer.writeAttribute(namespaceUri, "name", "hint");
        writer.writeAttribute(namespaceUri, "value", hint);
        writer.writeEndElement();
    }

    if (attributes.hasAttribute(namespaceUri, "cipher")) {
        QString cipher = attributes.value(namespaceUri, "cipher").toString();
        writer.writeStartElement(namespaceUri, "param");
        writer.writeAttribute(namespaceUri, "name", "cipher");
        writer.writeAttribute(namespaceUri, "value", cipher);
        writer.writeEndElement();
    }
    else {
        errorDescription = QT_TR_NOOP("Missing cipher attribute within en-crypt ENML tag");
        return false;
    }

    if (attributes.hasAttribute(namespaceUri, "length")) {
        QString length = attributes.value(namespaceUri, "length").toString();
        writer.writeStartElement(namespaceUri, "param");
        writer.writeAttribute(namespaceUri, "name", "length");
        writer.writeAttribute(namespaceUri, "value", length);
        writer.writeEndElement();
    }
    else {
        errorDescription = QT_TR_NOOP("Missing length attribute within en-crypt ENML tag");
        return false;
    }

    writer.writeStartElement(namespaceUri, "param");
    writer.writeAttribute(namespaceUri, "name", "encryptedText");
    writer.writeAttribute(namespaceUri, "value", encryptedText);
    writer.writeEndElement();

    QNTRACE("Wrote custom \"object\" element corresponding to en-crypt ENML tag");

    return true;
}

} // namespace qute_note

