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
#include <MessageRunner.h>
#include <Looper.h>
#include <Messenger.h>
#include <String.h>
#include <Window.h>

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

enum highlight_type {
	HIGHLIGHT_TEXT,
	HIGHLIGHT_KEYWORD,
	HIGHLIGHT_STRING,
	HIGHLIGHT_COMMENT,
	HIGHLIGHT_NUMBER,
	HIGHLIGHT_OPERATOR,
	HIGHLIGHT_TAG,
	HIGHLIGHT_ATTRIBUTE,
	HIGHLIGHT_PREPROCESSOR
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

struct HighlightRange {
	int32 start;
	int32 end;
	highlight_type type;

	HighlightRange(int32 s, int32 e, highlight_type t)
		: start(s), end(e), type(t) {}
};

enum {
	MSG_DELAYED_HIGHLIGHTING = 'dlhl',
	MSG_HIGHLIGHT_REQUEST = 'hlrq',
	MSG_HIGHLIGHT_RESULT = 'hlrs',
	MSG_HIGHLIGHT_CANCEL = 'hlcn',
	MSG_WORKER_QUIT = 'wqut'
};

enum {
	DEFAULT_MAX_UNDO_LEVELS = 50,
	MERGE_TIME_LIMIT_MICROSECONDS = 2000000,
	HIGHLIGHT_DELAY_MICROSECONDS = 15000,
	MAX_MERGEABLE_TEXT_LENGTH = 10
};

class HighlightWorker : public BLooper {
public:
	HighlightWorker();
	virtual ~HighlightWorker();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

	void RequestHighlighting(const char* text, int32 length,
							syntax_type type, bigtime_t timestamp,
							BMessenger target);
	void CancelRequests(bigtime_t beforeTime);
	void Shutdown();

private:
	void _ProcessHighlighting(BMessage* request);
	BMessage* _CreateHighlightResult(const char* text, int32 length, syntax_type type);
	void _AnalyzeCppSyntax(const char* text, int32 length, BList* ranges);
	void _AnalyzeSVGSyntax(const char* text, int32 length, BList* ranges);
	void _AnalyzeRdefSyntax(const char* text, int32 length, BList* ranges);
	void _AddRange(BList* ranges, int32 start, int32 end, highlight_type type);

	volatile bool fShutdown;
	bigtime_t fLastRequestTime;
	thread_id fWorkerThread;
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
	virtual void Select(int32 startOffset, int32 endOffset);

	void SetText(const char* text, const text_run_array* runs = NULL);

	void Redo();
	bool CanUndo() const;
	bool CanRedo() const;
	void ClearUndoHistory();
	void BreakUndoGroup();

	void ApplySyntaxHighlighting();
	void SetSyntaxType(syntax_type type);
	syntax_type GetSyntaxType() const { return fSyntaxType; }
	void ForceHighlightRefresh();

private:
	void _RequestAsyncHighlighting();
	void _SendHighlightRequest();
	void _ApplyHighlightResult(BMessage* result);
	void _CancelPendingHighlighting();
	syntax_type _DetectSyntaxType(const char* filename = NULL);
	syntax_type _DetectSyntaxFromContent();

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

	HighlightWorker* fHighlightWorker;
	bigtime_t fLastHighlightRequest;
	BString fLastHighlightedText;
	BMessageRunner* fHighlightDelayRunner;
	bool fForceHighlightUpdate;
};

#endif
