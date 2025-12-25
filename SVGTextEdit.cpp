/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <Clipboard.h>

#include "SVGConstants.h"
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

HighlightWorker::HighlightWorker()
	: BLooper("highlight_worker"),
	  fShutdown(false),
	  fLastRequestTime(0),
	  fWorkerThread(-1)
{
	Run();
	fWorkerThread = Thread();
}

HighlightWorker::~HighlightWorker()
{
	fShutdown = true;
}

bool
HighlightWorker::QuitRequested()
{
	fShutdown = true;
	return true;
}

void
HighlightWorker::Shutdown()
{
	if (!fShutdown) {
		fShutdown = true;
		PostMessage(MSG_WORKER_QUIT);
	}
}

void
HighlightWorker::RequestHighlighting(const char* text, int32 length,
									syntax_type type, bigtime_t timestamp,
									BMessenger target)
{
	if (fShutdown)
		return;

	BMessage request(MSG_HIGHLIGHT_REQUEST);
	request.AddString("text", text);
	request.AddInt32("length", length);
	request.AddInt32("syntax_type", (int32)type);
	request.AddInt64("timestamp", timestamp);
	request.AddMessenger("target", target);

	PostMessage(&request);
}

void
HighlightWorker::CancelRequests(bigtime_t beforeTime)
{
	if (fShutdown)
		return;

	BMessage cancel(MSG_HIGHLIGHT_CANCEL);
	cancel.AddInt64("before_time", beforeTime);
	PostMessage(&cancel);
}

void
HighlightWorker::MessageReceived(BMessage* message)
{
	if (fShutdown)
		return;

	switch (message->what) {
		case MSG_HIGHLIGHT_REQUEST:
			_ProcessHighlighting(message);
			break;

		case MSG_HIGHLIGHT_CANCEL:
			{
				bigtime_t beforeTime;
				if (message->FindInt64("before_time", &beforeTime) == B_OK) {
					if (beforeTime > fLastRequestTime)
						fLastRequestTime = beforeTime;
				}
			}
			break;

		case MSG_WORKER_QUIT:
			fShutdown = true;
			Quit();
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}

void
HighlightWorker::_ProcessHighlighting(BMessage* request)
{
	if (fShutdown)
		return;

	bigtime_t timestamp;
	if (request->FindInt64("timestamp", &timestamp) != B_OK)
		return;

	if (timestamp < fLastRequestTime)
		return;

	fLastRequestTime = timestamp;

	const char* text;
	int32 length;
	int32 syntaxType;
	BMessenger target;

	if (request->FindString("text", &text) != B_OK ||
	    request->FindInt32("length", &length) != B_OK ||
	    request->FindInt32("syntax_type", &syntaxType) != B_OK ||
	    request->FindMessenger("target", &target) != B_OK)
		return;

	BMessage* result = _CreateHighlightResult(text, length, (syntax_type)syntaxType);
	if (result && !fShutdown) {
		result->AddInt64("timestamp", timestamp);
		target.SendMessage(result);
		delete result;
	}
}

BMessage*
HighlightWorker::_CreateHighlightResult(const char* text, int32 length, syntax_type type)
{
	if (fShutdown)
		return NULL;

	BMessage* result = new BMessage(MSG_HIGHLIGHT_RESULT);
	if (!result)
		return NULL;

	BList ranges;

	switch (type) {
		case SYNTAX_CPP:
			_AnalyzeCppSyntax(text, length, &ranges);
			break;
		case SYNTAX_SVG_XML:
			_AnalyzeSVGSyntax(text, length, &ranges);
			break;
		case SYNTAX_RDEF:
			_AnalyzeRdefSyntax(text, length, &ranges);
			break;
		default:
			break;
	}

	for (int32 i = 0; i < ranges.CountItems(); i++) {
		if (fShutdown) {
			for (int32 j = i; j < ranges.CountItems(); j++)
				delete (HighlightRange*)ranges.ItemAt(j);
			delete result;
			return NULL;
		}

		HighlightRange* range = (HighlightRange*)ranges.ItemAt(i);
		if (range) {
			result->AddInt32("start", range->start);
			result->AddInt32("end", range->end);
			result->AddInt32("type", (int32)range->type);
		}
	}

	for (int32 i = 0; i < ranges.CountItems(); i++)
		delete (HighlightRange*)ranges.ItemAt(i);

	return result;
}

void
HighlightWorker::_AddRange(BList* ranges, int32 start, int32 end, highlight_type type)
{
	if (!ranges || fShutdown)
		return;

	HighlightRange* range = new HighlightRange(start, end, type);
	if (range)
		ranges->AddItem(range);
}

SVGTextEdit::SVGTextEdit(const char* name)
	: BTextView(name),
	fMaxUndoLevels(DEFAULT_MAX_UNDO_LEVELS),
	fInUndoRedo(false),
	fLastOperationTime(0),
	fMergeTimeLimit(MERGE_TIME_LIMIT_MICROSECONDS),
	fLastWasTyping(false),
	fSyntaxType(SYNTAX_NONE),
	fHighlightWorker(NULL),
	fLastHighlightRequest(0),
	fHighlightDelayRunner(NULL),
	fForceHighlightUpdate(false)
{
	SetWordWrap(false);
	MakeEditable(true);
	SetStylable(true);

	SetExplicitMinSize(BSize(32, 32));

	BFont sourceFont(be_fixed_font);
	const ColorScheme& colors = GetColorScheme(this);
	SetFontAndColor(&sourceFont, B_FONT_ALL, &colors.text);

	SetDoesUndo(false);

	fHighlightWorker = new HighlightWorker();
}

SVGTextEdit::~SVGTextEdit()
{
	delete fHighlightDelayRunner;

	if (fHighlightWorker) {
		fHighlightWorker->Shutdown();
		snooze(100000);
	}

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

		case MSG_DELAYED_HIGHLIGHTING:
			{
				bigtime_t msgTime;
				if (message->FindInt64("time", &msgTime) == B_OK) {
					if (msgTime == fLastHighlightRequest) {
						_SendHighlightRequest();
					}
				}
			}
			break;

		case MSG_HIGHLIGHT_RESULT:
			_ApplyHighlightResult(message);
			break;

		default:
			break;
	}
	BTextView::MessageReceived(message);
}

void
SVGTextEdit::Select(int32 startOffset, int32 endOffset)
{
	BTextView::Select(startOffset, endOffset);
	BWindow* window = Window();
	if (window) {
		BMessage msg(MSG_SELECTION_CHANGED);
		msg.AddInt32("from", startOffset);
		msg.AddInt32("to", endOffset);
		msg.AddPointer("source", this);
		window->PostMessage(&msg);
	}
}

void
SVGTextEdit::SetText(const char* text, const text_run_array* runs)
{
	fLastHighlightedText.SetTo("");
	fForceHighlightUpdate = true;
	_CancelPendingHighlighting();

	BTextView::SetText(text, runs);
	BTextView::ScrollToOffset(0);

	ForceHighlightRefresh();
}

void
SVGTextEdit::InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs)
{
	if (!_IsInUndoRedoMode()) {
		bool canMerge = fLastWasTyping && (length == 1);
		if (length > MAX_MERGEABLE_TEXT_LENGTH)
			canMerge = false;

		_AddUndoCommand(CMD_INSERT_TEXT, offset, length, text, runs, canMerge);

		for (int32 i = 0; i < fRedoStack.CountItems(); i++)
			_DeleteCommand((UndoCommand*)fRedoStack.ItemAt(i));

		fRedoStack.MakeEmpty();
	}

	BTextView::InsertText(text, length, offset, runs);

	if (!_IsInUndoRedoMode()) {
		_RequestAsyncHighlighting();

		BWindow* window = Window();
		if (window) {
			BMessage msg(MSG_TEXT_MODIFIED);
			window->PostMessage(&msg);
		}
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
				deletedText[deleteLength] = '\0';

				text_run_array* deletedRuns = RunArray(start, finish);

				bool canMerge = fLastWasTyping && (deleteLength == 1);
				_AddUndoCommand(CMD_DELETE_TEXT, start, deleteLength, deletedText, deletedRuns, canMerge);

				free(deletedText);
				free(deletedRuns);
			}
		}

		for (int32 i = 0; i < fRedoStack.CountItems(); i++)
			_DeleteCommand((UndoCommand*)fRedoStack.ItemAt(i));

		fRedoStack.MakeEmpty();
	}

	BTextView::DeleteText(start, finish);

	if (!_IsInUndoRedoMode()) {
		_RequestAsyncHighlighting();

		BWindow* window = Window();
		if (window) {
			BMessage msg(MSG_TEXT_MODIFIED);
			window->PostMessage(&msg);
		}
	}
}

void
SVGTextEdit::Undo(BClipboard* clipboard)
{
	if (!CanUndo())
		return;

	UndoCommand* cmd = (UndoCommand*)fUndoStack.RemoveItem(fUndoStack.CountItems() - 1);
	if (!cmd)
		return;

	_SetUndoRedoMode(true);

	int32 currentStart, currentEnd;
	GetSelection(&currentStart, &currentEnd);

	_ExecuteCommand(cmd, true);

	fRedoStack.AddItem(cmd);

	_SetUndoRedoMode(false);

	_RequestAsyncHighlighting();

	// Уведомляем об изменении текста после undo
	BWindow* window = Window();
	if (window) {
		BMessage msg(MSG_TEXT_MODIFIED);
		window->PostMessage(&msg);
	}

	BreakUndoGroup();
}

void
SVGTextEdit::Redo()
{
	if (!CanRedo())
		return;

	UndoCommand* cmd = (UndoCommand*)fRedoStack.RemoveItem(fRedoStack.CountItems() - 1);
	if (!cmd)
		return;

	_SetUndoRedoMode(true);
	_ExecuteCommand(cmd, false);
	fUndoStack.AddItem(cmd);

	_SetUndoRedoMode(false);

	_RequestAsyncHighlighting();

	BWindow* window = Window();
	if (window) {
		BMessage msg(MSG_TEXT_MODIFIED);
		window->PostMessage(&msg);
	}

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
		_DeleteCommand((UndoCommand*)fUndoStack.ItemAt(i));

	fUndoStack.MakeEmpty();

	for (int32 i = 0; i < fRedoStack.CountItems(); i++)
		_DeleteCommand((UndoCommand*)fRedoStack.ItemAt(i));

	fRedoStack.MakeEmpty();

	fLastOperationTime = 0;

	BWindow* window = Window();
	if (window) {
		BMessage msg(MSG_TEXT_MODIFIED);
		window->PostMessage(&msg);
	}
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
	if (fSyntaxType != type) {
		fSyntaxType = type;
		ForceHighlightRefresh();
	}
}

void
SVGTextEdit::ApplySyntaxHighlighting()
{
	ForceHighlightRefresh();
}

void
SVGTextEdit::ForceHighlightRefresh()
{
	fLastHighlightedText.SetTo("");
	fForceHighlightUpdate = true;

	if (fSyntaxType == SYNTAX_NONE && TextLength() > 0)
		fSyntaxType = _DetectSyntaxFromContent();

	_RequestAsyncHighlighting();
}

bool
SVGTextEdit::Find(const char* text, bool forward, bool wrap)
{
	if (!text || strlen(text) == 0)
		return false;

	BString content(Text());
	int32 contentLength = content.Length();
	int32 searchLen = strlen(text);

	int32 selStart, selEnd;
	GetSelection(&selStart, &selEnd);

	int32 foundPos = B_ERROR;

	if (forward) {
		foundPos = content.FindFirst(text, selEnd);
		if (foundPos == B_ERROR && wrap) {
			foundPos = content.FindFirst(text, 0);
		}
	} else {
		if (selStart > 0) {
			BString sub = content;
			sub.Truncate(selStart);
			foundPos = sub.FindLast(text);
		}
		if (foundPos == B_ERROR && wrap) {
			foundPos = content.FindLast(text);
		}
	}

	if (foundPos != B_ERROR) {
		Select(foundPos, foundPos + searchLen);
		ScrollToSelection();
		return true;
	}

	return false;
}

void
SVGTextEdit::_RequestAsyncHighlighting()
{
	_CancelPendingHighlighting();

	fLastHighlightRequest = system_time();

	BMessage msg(MSG_DELAYED_HIGHLIGHTING);
	msg.AddInt64("time", fLastHighlightRequest);

	fHighlightDelayRunner = new BMessageRunner(BMessenger(this), &msg,
											HIGHLIGHT_DELAY_MICROSECONDS, 1);
}

void
SVGTextEdit::_SendHighlightRequest()
{
	if (!fHighlightWorker)
		return;

	syntax_type detectedType = _DetectSyntaxFromContent();
	if (detectedType != SYNTAX_NONE) {
		fSyntaxType = detectedType;
	}

	const char* text = Text();
	int32 length = TextLength();

	fHighlightWorker->RequestHighlighting(text, length, fSyntaxType,
										fLastHighlightRequest, BMessenger(this));
}

void
SVGTextEdit::_ApplyHighlightResult(BMessage* result)
{
	bigtime_t timestamp;
	if (result->FindInt64("timestamp", &timestamp) != B_OK)
		return;

	if (timestamp != fLastHighlightRequest)
		return;

	if (!fForceHighlightUpdate && fLastHighlightedText == Text())
		return;

	fForceHighlightUpdate = false;

	BFont font(be_fixed_font);
	const ColorScheme& colors = GetColorScheme(this);

	SetFontAndColor(0, TextLength(), &font, B_FONT_ALL, &colors.text);

	BList sortedRanges;

	type_code messageType;
	int32 count;
	if (result->GetInfo("start", &messageType, &count) == B_OK) {
		for (int32 i = 0; i < count; i++) {
			int32 start, end, type;

			if (result->FindInt32("start", i, &start) == B_OK &&
			    result->FindInt32("end", i, &end) == B_OK &&
			    result->FindInt32("type", i, &type) == B_OK) {

				if (start >= 0 && end <= TextLength() && start < end) {
					HighlightRange* range = new HighlightRange(start, end, (highlight_type)type);
					if (range) {
						bool inserted = false;
						for (int32 j = 0; j < sortedRanges.CountItems(); j++) {
							HighlightRange* existing = (HighlightRange*)sortedRanges.ItemAt(j);
							if (existing && range->start < existing->start) {
								sortedRanges.AddItem(range, j);
								inserted = true;
								break;
							}
						}
						if (!inserted) {
							sortedRanges.AddItem(range);
						}
					}
				}
			}
		}
	}

	for (int32 i = 0; i < sortedRanges.CountItems(); i++) {
		HighlightRange* range = (HighlightRange*)sortedRanges.ItemAt(i);
		if (range) {
			rgb_color color;

			switch (range->type) {
				case HIGHLIGHT_KEYWORD:
					color = colors.keyword;
					break;
				case HIGHLIGHT_STRING:
					color = colors.string;
					break;
				case HIGHLIGHT_COMMENT:
					color = colors.comment;
					break;
				case HIGHLIGHT_NUMBER:
					color = colors.number;
					break;
				case HIGHLIGHT_OPERATOR:
					color = colors.operator_color;
					break;
				case HIGHLIGHT_TAG:
					color = colors.tag;
					break;
				case HIGHLIGHT_ATTRIBUTE:
					color = colors.attribute;
					break;
				case HIGHLIGHT_PREPROCESSOR:
					color = colors.preprocessor;
					break;
				default:
					color = colors.text;
					break;
			}

			SetFontAndColor(range->start, range->end, &font, B_FONT_ALL, &color);
		}
	}

	for (int32 i = 0; i < sortedRanges.CountItems(); i++)
		delete (HighlightRange*)sortedRanges.ItemAt(i);

	fLastHighlightedText.SetTo(Text());
}

void
SVGTextEdit::_CancelPendingHighlighting()
{
	if (fHighlightWorker && fLastHighlightRequest > 0) {
		fHighlightWorker->CancelRequests(system_time());
	}

	delete fHighlightDelayRunner;
	fHighlightDelayRunner = NULL;
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
		UndoCommand* lastCmd = (UndoCommand*)fUndoStack.ItemAt(fUndoStack.CountItems() - 1);

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
		UndoCommand* oldCmd = (UndoCommand*)fUndoStack.RemoveItem((int32)0);
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
	cmd->text = NULL;
	cmd->runs = NULL;

	if (text && length > 0) {
		cmd->text = (char*)malloc(length + 1);
		if (!cmd->text) {
			delete cmd;
			return NULL;
		}
		memcpy(cmd->text, text, length);
		cmd->text[length] = '\0';
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
		if (newText && source->text) {
			target->text = newText;
			memcpy(target->text + target->length, source->text, source->length);
			target->text[newLength] = '\0';
			target->length = newLength;
		}
	} else if (target->type == CMD_DELETE_TEXT) {
		if (source->offset + source->length == target->offset) {
			int32 newLength = target->length + source->length;
			char* newText = (char*)malloc(newLength + 1);
			if (newText && source->text && target->text) {
				memcpy(newText, source->text, source->length);
				memcpy(newText + source->length, target->text, target->length);
				newText[newLength] = '\0';
				free(target->text);
				target->text = newText;
				target->length = newLength;
				target->offset = source->offset;
			} else {
				free(newText);
			}
		} else if (source->offset == target->offset) {
			int32 newLength = target->length + source->length;
			char* newText = (char*)realloc(target->text, newLength + 1);
			if (newText && source->text) {
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
	if (!runs || runs->count <= 0)
		return NULL;

	size_t size = sizeof(text_run_array) + (runs->count - 1) * sizeof(text_run);
	text_run_array* copy = (text_run_array*)malloc(size);
	if (copy)
		memcpy(copy, runs, size);

	return copy;
}

void
SVGTextEdit::_DeleteCommand(UndoCommand* cmd)
{
	delete cmd;
}
