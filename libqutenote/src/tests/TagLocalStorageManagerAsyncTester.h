#ifndef __LIB_QUTE_NOTE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __LIB_QUTE_NOTE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/local_storage/LocalStorageManager.h>
#include <qute_note/types/Tag.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class TagLocalStorageManagerAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit TagLocalStorageManagerAsyncTester(QObject * parent = Q_NULLPTR);
    ~TagLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void getTagCountRequest(QUuid requestId = QUuid());
    void addTagRequest(Tag tag, QUuid requestId = QUuid());
    void updateTagRequest(Tag tag, QUuid requestId = QUuid());
    void findTagRequest(Tag tag, QUuid requestId = QUuid());
    void listAllTagsRequest(size_t limit, size_t offset, LocalStorageManager::ListTagsOrder::type order,
                            LocalStorageManager::OrderDirection::type orderDirection, QString linkedNotebookGuid,
                            QUuid requestId = QUuid());
    void expungeTagRequest(Tag tag, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetTagCountCompleted(int count, QUuid requestId);
    void onGetTagCountFailed(QString errorDescription, QUuid requestId);
    void onAddTagCompleted(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onUpdateTagCompleted(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onFindTagCompleted(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onListAllTagsCompleted(size_t limit, size_t offset, LocalStorageManager::ListTagsOrder::type order,
                                LocalStorageManager::OrderDirection::type orderDirection, QString linkedNotebookGuid,
                                QList<Tag> tags, QUuid requestId);
    void onListAllTagsFailed(size_t limit, size_t offset, LocalStorageManager::ListTagsOrder::type order,
                             LocalStorageManager::OrderDirection::type orderDirection, QString linkedNotebookGuid,
                             QString errorDescription, QUuid requestId);
    void onExpungeTagCompleted(Tag tag, QUuid requestId);
    void onExpungeTagFailed(Tag tag, QString errorDescription, QUuid requestId);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
        STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST,
        STATE_SENT_UPDATE_REQUEST,
        STATE_SENT_FIND_AFTER_UPDATE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST,
        STATE_SENT_DELETE_REQUEST,
        STATE_SENT_EXPUNGE_REQUEST,
        STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_ADD_EXTRA_TAG_ONE_REQUEST,
        STATE_SENT_ADD_EXTRA_TAG_TWO_REQUEST,
        STATE_SENT_LIST_TAGS_REQUEST
    };

    State m_state;

    LocalStorageManagerThreadWorker * m_pLocalStorageManagerThreadWorker;
    QThread *   m_pLocalStorageManagerThread;

    Tag         m_initialTag;
    Tag         m_foundTag;
    Tag         m_modifiedTag;
    QList<Tag>  m_initialTags;
};

} // namespace test
} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
