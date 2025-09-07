/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <Clipboard.h>

#include "SVGTextEdit.h"
#include "SVGTextEdit_Highlighters.h"

UndoCommand::UndoCommand()
	: type(CMD_INSERT_TEXT),
	offset(0),
	length(0),
	text(NULL),
	runs(NULL),
	selectionStart(0),
	selectionEnd(0),
	timestamp(0),
	canMerge(true)
{
}

UndoCommand::~UndoCommand()
{
	free(text);
	free(runs);
}

SVGTextEdit::SVGTextEdit(const char* name)
	: BTextView(name),
	  fMaxUndoLevels(50),
	  fInUndoRedo(false),
	  fLastOperationTime(0),
	  fMergeTimeLimit(2000000),
	  fLastWasTyping(false),
	  fSyntaxType(SYNTAX_NONE)
{
	SetWordWrap(false);
	MakeEditable(true);
	SetStylable(true);

	SetExplicitMinSize(BSize(32, 32));

	BFont sourceFont(be_fixed_font);
	const ColorScheme& colors = GetColorScheme(this);
	SetFontAndColor(&sourceFont, B_FONT_ALL, &colors.text);

	SetDoesUndo(false);
}

SVGTextEdit::~SVGTextEdit()
{
	ClearUndoHistory();
}

void
SVGTextEdit::KeyDown(const char* bytes, int32 numBytes)
{
	fLastWasTyping = _IsTypingOperation(bytes, numBytes);
	BTextView::KeyDown(bytes, numBytes);
}

void
SVGTextEdit::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PASTE:
			fLastWasTyping = false;
			BreakUndoGroup();
			break;
		default:
			break;
	}
	BTextView::MessageReceived(message);
}

void
SVGTextEdit::InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs)
{
	if (!_IsInUndoRedoMode()) {
		bool canMerge = fLastWasTyping && (length == 1);
		if (length > 10) {
			canMerge = false;
		}

		_AddUndoCommand(CMD_INSERT_TEXT, offset, length, text, runs, canMerge);

		for (int32 i = 0; i < fRedoStack.CountItems(); i++) {
			_DeleteCommand(static_cast<UndoCommand*>(fRedoStack.ItemAt(i)));
		}
		fRedoStack.MakeEmpty();
	}

	BTextView::InsertText(text, length, offset, runs);

	if (!_IsInUndoRedoMode()) {
		_ApplySyntaxHighlighting();
	}
}

void
SVGTextEdit::DeleteText(int32 start, int32 finish)
{
	if (!_IsInUndoRedoMode()) {
		int32 deleteLength = finish - start;
		if (deleteLength > 0) {
			char* deletedText = (char*)malloc(deleteLength + 1);
			if (deletedText) {
				GetText(start, deleteLength, deletedText);

				text_run_array* deletedRuns = RunArray(start, finish);

				bool canMerge = fLastWasTyping && (deleteLength == 1);
				_AddUndoCommand(CMD_DELETE_TEXT, start, deleteLength, deletedText, deletedRuns, canMerge);

				free(deletedText);
				free(deletedRuns);
			}
		}

		for (int32 i = 0; i < fRedoStack.CountItems(); i++) {
			_DeleteCommand(static_cast<UndoCommand*>(fRedoStack.ItemAt(i)));
		}
		fRedoStack.MakeEmpty();
	}

	BTextView::DeleteText(start, finish);

	if (!_IsInUndoRedoMode()) {
		_ApplySyntaxHighlighting();
	}
}

void
SVGTextEdit::Undo(BClipboard* clipboard)
{
	if (!CanUndo())
		return;

	UndoCommand* cmd = static_cast<UndoCommand*>(fUndoStack.RemoveItem(fUndoStack.CountItems() - 1));
	if (!cmd)
		return;

	_SetUndoRedoMode(true);

	int32 currentStart, currentEnd;
	GetSelection(&currentStart, &currentEnd);

	_ExecuteCommand(cmd, true);

	fRedoStack.AddItem(cmd);

	_SetUndoRedoMode(false);
	_ApplySyntaxHighlighting();

	BreakUndoGroup();
}

void
SVGTextEdit::Redo()
{
	if (!CanRedo())
		return;

	UndoCommand* cmd = static_cast<UndoCommand*>(fRedoStack.RemoveItem(fRedoStack.CountItems() - 1));
	if (!cmd)
		return;

	_SetUndoRedoMode(true);
	_ExecuteCommand(cmd, false);
	fUndoStack.AddItem(cmd);

	_SetUndoRedoMode(false);
	_ApplySyntaxHighlighting();

	BreakUndoGroup();
}

bool
SVGTextEdit::CanUndo() const
{
	return fUndoStack.CountItems() > 0;
}

bool
SVGTextEdit::CanRedo() const
{
	return fRedoStack.CountItems() > 0;
}

void
SVGTextEdit::ClearUndoHistory()
{
	for (int32 i = 0; i < fUndoStack.CountItems(); i++)
		_DeleteCommand(static_cast<UndoCommand*>(fUndoStack.ItemAt(i)));

	fUndoStack.MakeEmpty();

	for (int32 i = 0; i < fRedoStack.CountItems(); i++)
		_DeleteCommand(static_cast<UndoCommand*>(fRedoStack.ItemAt(i)));

	fRedoStack.MakeEmpty();

	fLastOperationTime = 0;
}

void
SVGTextEdit::BreakUndoGroup()
{
	fLastOperationTime = 0;
	fLastWasTyping = false;
}

void
SVGTextEdit::SetSyntaxType(syntax_type type)
{
	fSyntaxType = type;
	_ApplySyntaxHighlighting();
}

void
SVGTextEdit::ApplySyntaxHighlighting()
{
	_ApplySyntaxHighlighting();
}

void
SVGTextEdit::_ApplySyntaxHighlighting()
{
	if (fSyntaxType == SYNTAX_NONE) {
		fSyntaxType = _DetectSyntaxFromContent();
	}

	switch (fSyntaxType) {
		REGISTER_HIGHLIGHTER(SYNTAX_SVG_XML, SVG)
		REGISTER_HIGHLIGHTER(SYNTAX_CPP, Cpp)
		REGISTER_HIGHLIGHTER(SYNTAX_RDEF, Rdef)
		default:
			{
				BFont font(be_fixed_font);
				const ColorScheme& colors = GetColorScheme(this);
				SetFontAndColor(0, TextLength(), &font, B_FONT_ALL, &colors.text);
			}
			break;
	}
}

syntax_type
SVGTextEdit::_DetectSyntaxType(const char* filename)
{
	if (filename) {
		REGISTER_FILENAME_DETECTOR(SVG)
		REGISTER_FILENAME_DETECTOR(Cpp)
		REGISTER_FILENAME_DETECTOR(Rdef)
	}

	return _DetectSyntaxFromContent();
}

syntax_type
SVGTextEdit::_DetectSyntaxFromContent()
{
	const char* text = Text();
	int32 length = TextLength();

	if (length == 0)
		return SYNTAX_NONE;

	REGISTER_CONTENT_DETECTOR(SVG)
	REGISTER_CONTENT_DETECTOR(Cpp)
	REGISTER_CONTENT_DETECTOR(Rdef)

	return SYNTAX_NONE;
}

void
SVGTextEdit::_AddUndoCommand(command_type type, int32 offset, int32 length,
						const char* text, const text_run_array* runs, bool canMerge)
{
	bigtime_t currentTime = system_time();

	UndoCommand* newCmd = _CreateCommand(type, offset, length, text, runs, canMerge);
	if (!newCmd)
		return;

	newCmd->timestamp = currentTime;
	GetSelection(&newCmd->selectionStart, &newCmd->selectionEnd);

	if (canMerge && fUndoStack.CountItems() > 0) {
		UndoCommand* lastCmd = static_cast<UndoCommand*>(fUndoStack.ItemAt(fUndoStack.CountItems() - 1));

		if (lastCmd && _ShouldMergeCommands(lastCmd, newCmd)) {
			_MergeCommands(lastCmd, newCmd);
			_DeleteCommand(newCmd);
			fLastOperationTime = currentTime;
			return;
		}
	}

	fUndoStack.AddItem(newCmd);
	fLastOperationTime = currentTime;

	while (fUndoStack.CountItems() > fMaxUndoLevels) {
		UndoCommand* oldCmd = static_cast<UndoCommand*>(fUndoStack.RemoveItem((int32)0));
		_DeleteCommand(oldCmd);
	}
}

UndoCommand*
SVGTextEdit::_CreateCommand(command_type type, int32 offset, int32 length,
							const char* text, const text_run_array* runs, bool canMerge)
{
	UndoCommand* cmd = new UndoCommand;
	if (!cmd)
		return NULL;

	cmd->type = type;
	cmd->offset = offset;
	cmd->length = length;
	cmd->canMerge = canMerge;

	if (text && length > 0) {
		cmd->text = (char*)malloc(length + 1);
		if (cmd->text) {
			memcpy(cmd->text, text, length);
			cmd->text[length] = '\0';
		}
	}

	cmd->runs = _CopyRunArray(runs);
	return cmd;
}

bool
SVGTextEdit::_ShouldMergeCommands(UndoCommand* last, UndoCommand* current)
{
	if (!last || !current || !last->canMerge || !current->canMerge)
		return false;

	if (last->type != current->type)
		return false;

	if (current->timestamp - last->timestamp > fMergeTimeLimit)
		return false;

	if (last->type == CMD_INSERT_TEXT)
		return (current->offset == last->offset + last->length);
	else if (last->type == CMD_DELETE_TEXT)
		return (current->offset + current->length == last->offset) || (current->offset == last->offset);

	return false;
}

void
SVGTextEdit::_MergeCommands(UndoCommand* target, UndoCommand* source)
{
	if (!target || !source || target->type != source->type)
		return;

	if (target->type == CMD_INSERT_TEXT) {
		int32 newLength = target->length + source->length;
		char* newText = (char*)realloc(target->text, newLength + 1);
		if (newText) {
			target->text = newText;
			memcpy(target->text + target->length, source->text, source->length);
			target->text[newLength] = '\0';
			target->length = newLength;
		}
	} else if (target->type == CMD_DELETE_TEXT) {
		if (source->offset + source->length == target->offset) {
			int32 newLength = target->length + source->length;
			char* newText = (char*)malloc(newLength + 1);
			if (newText) {
				memcpy(newText, source->text, source->length);
				memcpy(newText + source->length, target->text, target->length);
				newText[newLength] = '\0';
				free(target->text);
				target->text = newText;
				target->length = newLength;
				target->offset = source->offset;
			}
		} else if (source->offset == target->offset) {
			int32 newLength = target->length + source->length;
			char* newText = (char*)realloc(target->text, newLength + 1);
			if (newText) {
				target->text = newText;
				memcpy(target->text + target->length, source->text, source->length);
				target->text[newLength] = '\0';
				target->length = newLength;
			}
		}
	}

	target->timestamp = source->timestamp;
}

bool
SVGTextEdit::_IsTypingOperation(const char* bytes, int32 numBytes)
{
	if (numBytes != 1)
		return false;

	unsigned char ch = bytes[0];

	if (ch == B_BACKSPACE || ch == B_DELETE)
		return true;

	if (ch >= 32 && ch < 127)
		return true;

	if (ch >= 128)
		return true;

	return false;
}

void
SVGTextEdit::_ExecuteCommand(UndoCommand* cmd, bool isUndo)
{
	if (!cmd)
		return;

	switch (cmd->type) {
		case CMD_INSERT_TEXT:
			if (isUndo) {
				BTextView::DeleteText(cmd->offset, cmd->offset + cmd->length);
				Select(cmd->selectionStart, cmd->selectionEnd);
			} else {
				BTextView::InsertText(cmd->text, cmd->length, cmd->offset, cmd->runs);
				Select(cmd->offset + cmd->length, cmd->offset + cmd->length);
			}
			break;

		case CMD_DELETE_TEXT:
			if (isUndo) {
				BTextView::InsertText(cmd->text, cmd->length, cmd->offset, cmd->runs);
				Select(cmd->selectionStart, cmd->selectionEnd);
			} else {
				BTextView::DeleteText(cmd->offset, cmd->offset + cmd->length);
				Select(cmd->offset, cmd->offset);
			}
			break;

		default:
			break;
	}
}

text_run_array*
SVGTextEdit::_CopyRunArray(const text_run_array* runs)
{
	if (!runs)
		return NULL;

	size_t size = sizeof(text_run_array) + (runs->count - 1) * sizeof(text_run);
	text_run_array* copy = static_cast<text_run_array*>(malloc(size));
	if (copy)
		memcpy(copy, runs, size);

	return copy;
}

void
SVGTextEdit::_DeleteCommand(UndoCommand* cmd)
{
	delete cmd;
}
