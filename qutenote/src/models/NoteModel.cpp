#include "NoteModel.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/UidGenerator.h>

// Limit for the queries to the local storage
#define NOTE_LIST_LIMIT (100)

#define NOTE_CACHE_LIMIT (20)

#define NUM_NOTE_MODEL_COLUMNS (10)

namespace qute_note {

NoteModel::NoteModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                     QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_listNotesOffset(0),
    m_listNotesRequestId(),
    m_noteItemsNotYetInLocalStorageUids(),
    m_cache(NOTE_CACHE_LIMIT),
    m_addNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_deleteNoteRequestIds(),
    m_expungeNoteRequestIds(),
    m_findNoteToRestoreFailedUpdateRequestIds(),
    m_findNoteToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::ModificationTimestamp),
    m_sortOrder(Qt::AscendingOrder)
{
    createConnections(localStorageManagerThreadWorker);
    requestNoteList();
}

NoteModel::~NoteModel()
{}

QModelIndex NoteModel::indexForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG("Can't find the note item by local uid");
        return QModelIndex();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't find the indexed reference to the note item: " << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, Columns::Title);
}

Qt::ItemFlags NoteModel::flags(const QModelIndex & modelIndex) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(modelIndex);
    if (!modelIndex.isValid()) {
        return indexFlags;
    }

    int row = modelIndex.row();
    int column = modelIndex.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if ((column == Columns::Dirty) ||
        (column == Columns::Size) ||
        (column == Columns::Synchronizable))

    {
        return indexFlags;
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    const NoteModelItem & item = index.at(static_cast<size_t>(row));
    if (!canUpdateNoteItem(item)) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsEditable;
    return indexFlags;
}

QVariant NoteModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int rowIndex = index.row();
    int columnIndex = index.column();

    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size())) ||
        (columnIndex < 0) || (columnIndex >= NUM_NOTE_MODEL_COLUMNS))
    {
        return QVariant();
    }

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::CreationTimestamp:
    case Columns::ModificationTimestamp:
    case Columns::Title:
    case Columns::PreviewText:
    case Columns::ThumbnailImageFilePath:
    case Columns::NotebookName:
    case Columns::TagNameList:
    case Columns::Size:
    case Columns::Synchronizable:
    case Columns::Dirty:
        column = static_cast<Columns::type>(columnIndex);
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataText(rowIndex, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(rowIndex, column);
    default:
        return QVariant();
    }
}

QVariant NoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::CreationTimestamp:
        // TRANSLATOR: note's creation timestamp
        return QVariant(tr("Created"));
    case Columns::ModificationTimestamp:
        // TRANSLATOR: note's modification timestamp
        return QVariant(tr("Modified"));
    case Columns::Title:
        return QVariant(tr("Title"));
    case Columns::PreviewText:
        // TRANSLATOR: a short excerpt of note's text
        return QVariant(tr("Preview"));
    case Columns::NotebookName:
        return QVariant(tr("Notebook"));
    case Columns::TagNameList:
        // TRANSLATOR: the list of note's tags
        return QVariant(tr("Tags"));
    case Columns::Size:
        // TRANSLATOR: size of note in bytes
        return QVariant(tr("Size"));
    case Columns::Synchronizable:
        return QVariant(tr("Synchronizable"));
    case Columns::Dirty:
        return QVariant(tr("Dirty"));
    // NOTE: intentional fall-through
    case Columns::ThumbnailImageFilePath:
    default:
        return QVariant();
    }
}

int NoteModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_data.size());
}

int NoteModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return NUM_NOTE_MODEL_COLUMNS;
}

QModelIndex NoteModel::index(int row, int column, const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
    {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex NoteModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool NoteModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NoteModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
{
    if (!modelIndex.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int row= modelIndex.row();
    int column= modelIndex.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
    {
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();
    NoteModelItem item = index.at(static_cast<size_t>(row));

    if (!canUpdateNoteItem(item)) {
        return false;
    }

    bool dirty = item.isDirty();
    switch(column)
    {
    case Columns::Title:
        {
            QString title = value.toString();
            dirty |= (title != item.title());
            item.setTitle(title);
            break;
        }
    case Columns::Synchronizable:
        {
            if (item.isSynchronizable()) {
                QString error = QT_TR_NOOP("Can't make already synchronizable note not synchronizable");
                QNINFO(error << ", already synchronizable note item: " << item);
                emit notifyError(error);
                return false;
            }

            dirty |= (value.toBool() != item.isSynchronizable());
            item.setSynchronizable(value.toBool());
            break;
        }
    default:
        return false;
    }

    item.setDirty(dirty);
    item.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    index.replace(index.begin() + row, item);
    emit dataChanged(modelIndex, modelIndex);

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(item);
    emit layoutChanged();

    updateNoteInLocalStorage(item);
    return true;
}

bool NoteModel::insertRows(int row, int count, const QModelIndex & parent)
{
    // NOTE: NoteModel's own API is used to add rows
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool NoteModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        QNDEBUG("Ignoring the attempt to remove rows from note model for valid parent model index");
        return false;
    }

    if (Q_UNLIKELY((row + count) >= static_cast<int>(m_data.size())))
    {
        QString error = QT_TR_NOOP("Detected attempt to remove more rows than the note model contains: row") +
                        QStringLiteral(" = ") + QString::number(row) + QStringLiteral(", ") + QT_TR_NOOP("count") +
                        QStringLiteral(" = ") + QString::number(count) + QStringLiteral(", ") + QT_TR_NOOP("number of note model items") +
                        QStringLiteral(" = ") + QString::number(static_cast<int>(m_data.size()));
        QNINFO(error);
        emit notifyError(error);
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    for(int i = 0; i < count; ++i)
    {
        auto it = index.begin() + row;
        if (it->isSynchronizable()) {
            QString error = QT_TR_NOOP("Can't remove synchronizable note");
            QNINFO(error << ", synchronizable note item: " << *it);
            emit notifyError(error);
            return false;
        }
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        auto it = index.begin() + row + i;

        Note note;
        note.setLocalUid(it->localUid());

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeNoteRequestIds.insert(requestId))
        QNTRACE("Emitting the request to expunge the note from the local storage: request id = "
                << requestId << ", note local uid: " << it->localUid());
        emit expungeNote(note, requestId);
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))
    emit endRemoveRows();

    return true;
}

void NoteModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG("NoteModel::sort: column = " << column << ", order = " << order
            << " (" << (order == Qt::AscendingOrder ? "ascending" : "descending") << ")");

    if ( (column == Columns::ThumbnailImageFilePath) ||
         (column == Columns::TagNameList) )
    {
        // Should not sort by these columns
        return;
    }

    if (Q_UNLIKELY((column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))) {
        return;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    if (column == m_sortedColumn)
    {
        if (order == m_sortOrder) {
            QNDEBUG("Neither sorted column nor sort order have changed, nothing to do");
            return;
        }

        m_sortOrder = order;

        QNDEBUG("Only the sort order has changed, reversing the index");

        emit layoutAboutToBeChanged();
        index.reverse();
        emit layoutChanged();

        return;
    }

    m_sortedColumn = static_cast<Columns::type>(column);
    m_sortOrder = order;

    emit layoutAboutToBeChanged();

    QModelIndexList persistentIndices = persistentIndexList();
    QVector<std::pair<QString, int> > localUidsToUpdateWithColumns;
    localUidsToUpdateWithColumns.reserve(persistentIndices.size());

    QStringList localUidsToUpdate;
    for(auto it = persistentIndices.begin(), end = persistentIndices.end(); it != end; ++it)
    {
        const QModelIndex & modelIndex = *it;
        int column = modelIndex.column();

        if (!modelIndex.isValid()) {
            localUidsToUpdateWithColumns << std::pair<QString, int>(QString(), column);
            continue;
        }

        int row = modelIndex.row();

        if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
            (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
        {
            localUidsToUpdateWithColumns << std::pair<QString, int>(QString(), column);
        }

        auto itemIt = index.begin() + row;
        const NoteModelItem & item = *itemIt;
        localUidsToUpdateWithColumns << std::pair<QString, int>(item.localUid(), column);
    }

    std::vector<boost::reference_wrapper<const NoteModelItem> > items(index.begin(), index.end());
    std::sort(items.begin(), items.end(), NoteComparator(m_sortedColumn, m_sortOrder));
    index.rearrange(items.begin());

    QModelIndexList replacementIndices;
    for(auto it = localUidsToUpdateWithColumns.begin(), end = localUidsToUpdateWithColumns.end(); it != end; ++it)
    {
        const QString & localUid = it->first;
        const int column = it->second;

        if (localUid.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }

        QModelIndex newIndex = indexForLocalUid(localUid);
        if (!newIndex.isValid()) {
            replacementIndices << QModelIndex();
            continue;
        }

        QModelIndex newIndexWithColumn = createIndex(newIndex.row(), column);
        replacementIndices << newIndexWithColumn;
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    emit layoutChanged();
}

void NoteModel::onAddNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteModel::onAddNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteModel::onUpdateNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteModel::onUpdateNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteModel::onFindNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteModel::onFindNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteModel::onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                    size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QList<Note> foundNotes, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(foundNotes)
    Q_UNUSED(requestId)
}

void NoteModel::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                  size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteModel::onDeleteNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteModel::onDeleteNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteModel::onExpungeNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NoteModel::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    // TODO: implement
    Q_UNUSED(localStorageManagerThreadWorker)
}

void NoteModel::requestNoteList()
{
    // TODO: implement
}

QVariant NoteModel::dataText(const int row, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    return QVariant();
}

QVariant NoteModel::dataAccessibleText(const int row, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    return QVariant();
}

void NoteModel::removeItemByLocalUid(const QString & localUid)
{
    // TODO: implement
    Q_UNUSED(localUid)
}

void NoteModel::updateItemRowWithRespectToSorting(const NoteModelItem & item)
{
    // TODO: implement
    Q_UNUSED(item)
}

void NoteModel::updatePersistentModelIndices()
{
    // TODO: implement
}

void NoteModel::updateNoteInLocalStorage(const NoteModelItem & item)
{
    // TODO: implement
    Q_UNUSED(item)
}

bool NoteModel::canUpdateNoteItem(const NoteModelItem & item) const
{
    // TODO: implement
    Q_UNUSED(item)
    return true;
}

bool NoteModel::canCreateNoteItem(const QString & notebookLocalUid) const
{
    // TODO: implement
    Q_UNUSED(notebookLocalUid)
    return true;
}

bool NoteModel::NoteComparator::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    bool less = false;
    bool greater = false;

    switch(m_sortedColumn)
    {
    case Columns::CreationTimestamp:
        less = (lhs.creationTimestamp() < rhs.creationTimestamp());
        greater = (lhs.creationTimestamp() > rhs.creationTimestamp());
        break;
    case Columns::ModificationTimestamp:
        less = (lhs.modificationTimestamp() < rhs.modificationTimestamp());
        greater = (lhs.modificationTimestamp() > rhs.modificationTimestamp());
        break;
    case Columns::Title:
        {
            int compareResult = lhs.title().localeAwareCompare(rhs.title());
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::PreviewText:
        {
            int compareResult = lhs.previewText().localeAwareCompare(rhs.previewText());
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::NotebookName:
        {
            int compareResult = lhs.notebookName().localeAwareCompare(rhs.notebookName());
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::Size:
        less = (lhs.sizeInBytes() < rhs.sizeInBytes());
        greater = (lhs.sizeInBytes() > rhs.sizeInBytes());
        break;
    case Columns::Synchronizable:
        less = (!lhs.isSynchronizable() && rhs.isSynchronizable());
        greater = (lhs.isSynchronizable() && !rhs.isSynchronizable());
        break;
    case Columns::Dirty:
        less = (!lhs.isDirty() && rhs.isDirty());
        greater = (lhs.isDirty() && !rhs.isDirty());
        break;
    default:
        less = false;
        greater = false;
        break;
    }

    if (m_sortOrder == Qt::AscendingOrder) {
        return less;
    }
    else {
        return greater;
    }
}

} // namespace qute_note