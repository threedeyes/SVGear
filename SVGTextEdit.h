/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_TEXT_EDIT_H
#define SVG_TEXT_EDIT_H

#include <Application.h>
#include <SupportDefs.h>
#include <TextView.h>
#include <List.h>
#include <OS.h>

const rgb_color kColorDefault = {0, 0, 0, 255};        // Black
const rgb_color kColorTag = {0, 0, 128, 255};          // Navy blue
const rgb_color kColorAttribute = {128, 0, 128, 255};  // Purple
const rgb_color kColorString = {0, 128, 0, 255};       // Green
const rgb_color kColorComment = {128, 128, 128, 255};  // Gray
const rgb_color kColorValue = {255, 69, 0, 255};       // Orange red

enum command_type {
    CMD_INSERT_TEXT,
    CMD_DELETE_TEXT,
    CMD_REPLACE_TEXT
};

struct UndoCommand {
    command_type type;
    int32 offset;
    int32 length;
    char* text;
    text_run_array* runs;
    int32 selectionStart;
    int32 selectionEnd;
    bigtime_t timestamp;
    bool canMerge;

    UndoCommand();
    ~UndoCommand();
};

class SVGTextEdit : public BTextView {
public:
    SVGTextEdit(const char* name);
    virtual ~SVGTextEdit();

    virtual void InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs);
    virtual void DeleteText(int32 start, int32 finish);
    virtual void Undo(BClipboard* clipboard);
    virtual void KeyDown(const char* bytes, int32 numBytes);
    virtual void MessageReceived(BMessage* message);

    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void ClearUndoHistory();
    void BreakUndoGroup();

    void ApplySyntaxHighlighting();

private:
    void _ApplySyntaxHighlighting();
    void _HighlightTag(int32 start, int32 end, const BFont& font);

    void _AddUndoCommand(command_type type, int32 offset, int32 length,
						const char* text, const text_run_array* runs, bool canMerge = true);
    UndoCommand* _CreateCommand(command_type type, int32 offset, int32 length,
						const char* text, const text_run_array* runs, bool canMerge = true);
    void _ExecuteCommand(UndoCommand* cmd, bool isUndo);
    text_run_array* _CopyRunArray(const text_run_array* runs);
    void _DeleteCommand(UndoCommand* cmd);
    bool _ShouldMergeCommands(UndoCommand* last, UndoCommand* current);
    void _MergeCommands(UndoCommand* target, UndoCommand* source);
    bool _IsTypingOperation(const char* bytes, int32 numBytes);

    void _SetUndoRedoMode(bool mode) { fInUndoRedo = mode; }
    bool _IsInUndoRedoMode() const { return fInUndoRedo; }

private:
    BList fUndoStack;
    BList fRedoStack;
    int32 fMaxUndoLevels;
    bool fInUndoRedo;
    bigtime_t fLastOperationTime;
    bigtime_t fMergeTimeLimit;
    bool fLastWasTyping;
};

#endif
