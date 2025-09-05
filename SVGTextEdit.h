/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_TEXT_EDIT_H
#define SVG_TEXT_EDIT_H

#include <Application.h>
#include <SupportDefs.h>
#include <TextView.h>

const rgb_color kColorDefault = {0, 0, 0, 255};        // Black
const rgb_color kColorTag = {0, 0, 128, 255};          // Navy blue
const rgb_color kColorAttribute = {128, 0, 128, 255};  // Purple
const rgb_color kColorString = {0, 128, 0, 255};       // Green
const rgb_color kColorComment = {128, 128, 128, 255};  // Gray
const rgb_color kColorValue = {255, 69, 0, 255};       // Orange red

class SVGTextEdit : public BTextView {
public:
    SVGTextEdit(const char* name);

    virtual void InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs);
    virtual void DeleteText(int32 start, int32 finish);

    void ApplySyntaxHighlighting();

private:
    void _ApplySyntaxHighlighting();
    void _HighlightTag(int32 start, int32 end, const BFont& font);
};


#endif
