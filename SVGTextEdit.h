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

const rgb_color kColorDefault = {0, 0, 0, 255};
const rgb_color kColorTag = {0, 0, 128, 255};
const rgb_color kColorAttribute = {128, 0, 128, 255};
const rgb_color kColorString = {0, 128, 0, 255};
const rgb_color kColorComment = {128, 128, 128, 255};
const rgb_color kColorValue = {255, 69, 0, 255};
const rgb_color kColorKeyword = {0, 0, 255, 255};
const rgb_color kColorNumber = {255, 0, 0, 255};
const rgb_color kColorPreprocessor = {128, 0, 255, 255};
const rgb_color kColorOperator = {255, 140, 0, 255};

enum command_type {
	CMD_INSERT_TEXT,
	CMD_DELETE_TEXT,
	CMD_REPLACE_TEXT
};

enum syntax_type {
	SYNTAX_NONE,
	SYNTAX_SVG_XML,
	SYNTAX_CPP,
	SYNTAX_RDEF
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
	void SetSyntaxType(syntax_type type);
	syntax_type GetSyntaxType() const { return fSyntaxType; }

private:
	void _ApplySyntaxHighlighting();
	void _ApplySVGHighlighting();
	void _ApplyCppHighlighting();
	void _ApplyRdefHighlighting();
	void _HighlightTag(int32 start, int32 end, const BFont& font);

	syntax_type _DetectSyntaxType(const char* filename = NULL);
	syntax_type _DetectSyntaxFromContent();
	bool _IsCppKeyword(const char* word, int32 length);
	bool _IsRdefKeyword(const char* word, int32 length);
	int32 _SkipWhitespace(const char* text, int32 pos, int32 length);
	int32 _FindWordEnd(const char* text, int32 pos, int32 length);

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
	syntax_type fSyntaxType;
};

#endif
