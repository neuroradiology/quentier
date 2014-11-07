#include "FullSynchronizationManager.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <algorithm>

namespace qute_note {

FullSynchronizationManager::FullSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                       QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                       QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult> pOAuthResult,
                                                       QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_pNoteStore(pNoteStore),
    m_pOAuthResult(pOAuthResult),
    m_maxSyncChunkEntries(50),
    m_syncChunks(),
    m_tags()
{
    createConnections();
}

void FullSynchronizationManager::start()
{
    QNDEBUG("FullSynchronizationManager::start");

    QUTE_NOTE_CHECK_PTR(m_pNoteStore.data());
    QUTE_NOTE_CHECK_PTR(m_pOAuthResult.data());

    qint32 afterUsn = 0;
    m_syncChunks.clear();
    qevercloud::SyncChunk * pSyncChunk = nullptr;

    QNDEBUG("Downloading sync chunks:");

    while(!pSyncChunk || (pSyncChunk->chunkHighUSN < pSyncChunk->updateCount))
    {
        if (pSyncChunk) {
            afterUsn = pSyncChunk->chunkHighUSN;
        }

        m_syncChunks.push_back(qevercloud::SyncChunk());
        pSyncChunk = &(m_syncChunks.back());
        *pSyncChunk = m_pNoteStore->getSyncChunk(afterUsn, m_maxSyncChunkEntries, true,
                                                 m_pOAuthResult->authenticationToken);
        QNDEBUG("Received sync chunk: " << *pSyncChunk);
    }

    QNDEBUG("Done. Processing tags from buffered sync chunks");


    // TODO: continue from here
}

void FullSynchronizationManager::onFindUserCompleted(UserWrapper user)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindUserFailed(UserWrapper user)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNotebookCompleted(Notebook notebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNotebookFailed(Notebook notebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNoteCompleted(Note note)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNoteFailed(Note note)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindTagCompleted(Tag tag)
{
    QNDEBUG("FullSynchronizationManager::onFindTagCompleted: tag = " << tag);

    // Attempt to find this tag by name within the list of tags waiting for processing;
    // first simply try the front tag from the list to avoid the costly lookup
    if (!tag.hasName()) {
        return;
    }

    TagsList::iterator it = findTagInList(tag.name());
    if (it == m_tags.end()) {
        QNDEBUG("Unable to find tag within the list of tags waiting for processing");
        return;
    }

    // The tag exists both in the client and in the server
    const qevercloud::Tag & remoteTag = *it;
    if (!remoteTag.updateSequenceNum.isSet()) {
        QString errorDescription = QT_TR_NOOP("Found tag from sync chunk without the update sequence number");
        QNWARNING(errorDescription << ": " << remoteTag);
        emit error(errorDescription);
    }

    if (!tag.hasUpdateSequenceNumber() || (remoteTag.updateSequenceNum.ref() > tag.updateSequenceNumber()))
    {
        if (!tag.isDirty())
        {
            // Remote tag is more recent, need to update the tag existing in local storage
            Tag updatedTag(remoteTag);
            updatedTag.unsetLocalGuid();
            emit updateTag(updatedTag);
        }
        else
        {
            // Remote tag is more recent but the local one has been modified;
            // Evernote's synchronization protocol description suggests trying
            // to do field-by-field merge but it's overcomplicated and error-prone;
            // it's much easier to rename the existing local tag to make it clear
            // it has a conflict with its remote counterpart and mark dirty so that
            // it would be sent to the server along with other local changes
            QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

            tag.setGuid("");
            tag.setName(QObject::tr("Conflicted tag ") + tag.name() + "(" + currentDateTime + ")");
            tag.setLocal(true);

            // Using event loop should ensure the update of local conflicting tag would be finished properly
            // before we add the remote tag with the same guid
            QEventLoop loop;
            QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag)), &loop, SLOT(quit()));
            QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString)), &loop, SLOT(quit()));
            emit updateTag(tag);
            loop.exec();

            Tag newTag(remoteTag);
            emit addTag(newTag);
        }
    }

    m_tags.erase(it);
    if (m_tags.empty()) {
        // TODO: done with tags, launch the sync of saved searches
    }
}

void FullSynchronizationManager::onFindTagFailed(Tag tag)
{
    QNDEBUG("FullSynchronizationManager::onFindTagFailed: tag = " << tag);

    // Attempt to find this tag by name within the list of tags waiting for processing;
    // first simply try the front tag from the list to avoid the costly lookup
    if (!tag.hasName()) {
        return;
    }

    // Ok, this tag wasn't found in the local storage, need to add it there
    // also removing the tag from the list of tags waiting for processing
    TagsList::iterator it = findTagInList(tag.name());
    if (it == m_tags.end()) {
        QNDEBUG("Unable to find tag within the list of tags waiting for processing");
        return;
    }

    Tag newTag;
    newTag.setName(tag.name());
    emit addTag(newTag);

    m_tags.erase(it);
    if (m_tags.empty()) {
        // TODO: done with tags, launch the sync of saved searches
    }
}

void FullSynchronizationManager::onFindResourceCompleted(ResourceWrapper resource)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindResourceFailed(ResourceWrapper resource)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindSavedSearchCompleted(SavedSearch savedSearch)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindSavedSearchFailed(SavedSearch savedSearch)
{
    // TODO: implement
}

void FullSynchronizationManager::createConnections()
{
    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this, SIGNAL(addUser(UserWrapper)), &m_localStorageManagerThreadWorker, SLOT(onAddUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(updateUser(UserWrapper)), &m_localStorageManagerThreadWorker, SLOT(onUpdateUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(findUser(UserWrapper)), &m_localStorageManagerThreadWorker, SLOT(onFindUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(deleteUser(UserWrapper)), &m_localStorageManagerThreadWorker, SLOT(onDeleteUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(expungeUser(UserWrapper)), &m_localStorageManagerThreadWorker, SLOT(onExpungeUserRequest(UserWrapper)));

    QObject::connect(this, SIGNAL(addNotebook(Notebook)), &m_localStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(updateNotebook(Notebook)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findNotebook(Notebook)), &m_localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(expungeNotebook(Notebook)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNotebookRequest(Notebook)));

    QObject::connect(this, SIGNAL(addNote(Note,Notebook)), &m_localStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(updateNote(Note,Notebook)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(findNote(Note,bool)), &m_localStorageManagerThreadWorker, SLOT(onFindNoteRequest(Note,bool)));
    QObject::connect(this, SIGNAL(deleteNote(Note)), &m_localStorageManagerThreadWorker, SLOT(onDeleteNoteRequest(Note)));
    QObject::connect(this, SIGNAL(expungeNote(Note)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNoteRequest(Note)));

    QObject::connect(this, SIGNAL(addTag(Tag)), &m_localStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag)));
    QObject::connect(this, SIGNAL(updateTag(Tag)), &m_localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag)));
    QObject::connect(this, SIGNAL(findTag(Tag)), &m_localStorageManagerThreadWorker, SLOT(onFindTagRequest(Tag)));
    QObject::connect(this, SIGNAL(deleteTag(Tag)), &m_localStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag)));
    QObject::connect(this, SIGNAL(expungeTag(Tag)), &m_localStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag)));

    QObject::connect(this, SIGNAL(addResource(ResourceWrapper,Note)), &m_localStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(updateResource(ResourceWrapper,Note)), &m_localStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(findResource(ResourceWrapper,bool)), &m_localStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool)));
    QObject::connect(this, SIGNAL(expungeResource(ResourceWrapper)), &m_localStorageManagerThreadWorker, SLOT(onExpungeResourceRequest(ResourceWrapper)));

    QObject::connect(this, SIGNAL(addLinkedNotebook(LinkedNotebook)), &m_localStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook)), &m_localStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(findLinkedNotebook(LinkedNotebook)), &m_localStorageManagerThreadWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(expungeLinkedNotebook(LinkedNotebook)), &m_localStorageManagerThreadWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook)));

    QObject::connect(this, SIGNAL(addSavedSearch(SavedSearch)), &m_localStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch)), &m_localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(findSavedSearch(SavedSearch)), &m_localStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(expungeSavedSearch(SavedSearch)), &m_localStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findUserComplete(UserWrapper)), this, SLOT(onFindUserCompleted(UserWrapper)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findUserFailed(UserWrapper,QString)), this, SLOT(onFindUserFailed(UserWrapper)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook)), this, SLOT(onFindNotebookCompleted(Notebook)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString)), this, SLOT(onFindNotebookFailed(Notebook)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteComplete(Note,bool)), this, SLOT(onFindNoteCompleted(Note)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteFailed(Note,bool,QString)), this, SLOT(onFindNoteFailed(Note)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag)), this, SLOT(onFindTagCompleted(Tag)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString)), this, SLOT(onFindTagFailed(Tag)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool)), this, SLOT(onFindResourceCompleted(ResourceWrapper)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString)), this, SLOT(onFindResourceFailed(ResourceWrapper)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook)), this, SLOT(onFindLinkedNotebookCompleted(LinkedNotebook)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString)), this, SLOT(onFindLinkedNotebookFailed(LinkedNotebook)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch)), this, SLOT(onFindSavedSearchCompleted(SavedSearch)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString)), this, SLOT(onFindSavedSearchFailed(SavedSearch)));
}

void FullSynchronizationManager::launchTagsSync()
{
    QNDEBUG("FullSynchronizationManager::launchTagsSync");

    m_tags.clear();
    foreach(const qevercloud::SyncChunk & syncChunk, m_syncChunks)
    {
        if (syncChunk.tags.isSet()) {
            m_tags.append(syncChunk.tags.ref());
        }
    }

    if (m_tags.empty()) {
        // TODO: nothing to synchronize, launch the sync of saved searches
        return;
    }

    Tag tagToFind;
    tagToFind.unsetLocalGuid();

    const qevercloud::Tag & frontTag = m_tags.front();
    if (frontTag.name.isSet() && !frontTag.name->isEmpty()) {
        tagToFind.setName(frontTag.name.ref());
    }
    else if (frontTag.guid.isSet() && !frontTag.guid->isEmpty()) {
        tagToFind.setGuid(frontTag.guid.ref());
    }
    else {
        QString errorDescription = QT_TR_NOOP("Can't synchronize remote tag: no guid and no name");
        QNWARNING(errorDescription << ": " << frontTag);
        emit error(errorDescription);
        return;
    }

    emit findTag(tagToFind);
}

FullSynchronizationManager::TagsList::iterator FullSynchronizationManager::findTagInList(const QString & name)
{
    // Try the front tag first, in most cases it should be it

    const qevercloud::Tag & frontTag = m_tags.front();
    TagsList::iterator it = m_tags.begin();

    if (!frontTag.name.isSet() || (frontTag.name.ref() != name)) {
        it = std::find_if(m_tags.begin(), m_tags.end(), CompareTagByName(name));
    }

    return it;
}

bool FullSynchronizationManager::CompareTagByName::operator()(const qevercloud::Tag & tag) const
{
    if (tag.name.isSet()) {
        return (m_name == tag.name.ref());
    }
    else {
        return false;
    }
}

} // namespace qute_note