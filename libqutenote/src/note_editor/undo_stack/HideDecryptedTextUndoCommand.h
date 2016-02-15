#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__HIDE_DECRYPTED_TEXT_UNDO_COMMAND
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__HIDE_DECRYPTED_TEXT_UNDO_COMMAND

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace qute_note {

class HideDecryptedTextUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    HideDecryptedTextUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, QUndoCommand * parent = Q_NULLPTR);
    HideDecryptedTextUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~HideDecryptedTextUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    Callback    m_callback;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__HIDE_DECRYPTED_TEXT_UNDO_COMMAND