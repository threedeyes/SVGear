/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ScrollBar.h>
#include <Window.h>
#include <ControlLook.h>
#include <Font.h>
#include <Catalog.h>
#include <File.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <IconUtils.h>
#include <OS.h>

#include <cmath>
#include <cstring>
#include <cstdio>

#include "IconGridView.h"
#include "IconInfoView.h"
#include "HvifStoreDefs.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "HVIFStoreDialog"

const float IconGridView::kBaseIconSize = 64.0f;
const float IconGridView::kBaseCellWidth = 110.0f;
const float IconGridView::kBaseCellHeight = 105.0f;
const float IconGridView::kBasePadding = 8.0f;
const float IconGridView::kBaseFontSize = 12.0f;
const float IconGridView::kAnimationSpeed = 0.000005f;


IconGridView::IconGridView()
	:
	BView("IconGrid", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE 
		| B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED),
	fItems(20),
	fSelection(-1),
	fHoveredItem(-1),
	fLoadMoreHovered(false),
	fGeneration(0),
	fLoading(false),
	fHasMore(false),
	fInfoView(NULL),
	fColumns(1),
	fTotalHeight(0),
	fIconSize(kBaseIconSize),
	fCellWidth(kBaseCellWidth),
	fCellHeight(kBaseCellHeight),
	fPadding(kBasePadding),
	fDragButton(0),
	fClickPoint(0, 0),
	fDragStarted(false),
	fDragItemIndex(-1)
{
	SetViewUIColor(B_LIST_BACKGROUND_COLOR);
	SetLowUIColor(B_LIST_BACKGROUND_COLOR);
	SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);

	SetExplicitMinSize(BSize(200, 150));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	_CleanupOldTempFiles();
}


IconGridView::~IconGridView()
{
}


void
IconGridView::_CalculateSizes()
{
	float fontSize = be_plain_font->Size();
	float scale = fontSize / kBaseFontSize;

	if (scale < 1.0f)
		scale = 1.0f;

	fIconSize = floorf(kBaseIconSize * scale);
	fCellWidth = floorf(kBaseCellWidth * scale);
	fCellHeight = floorf(kBaseCellHeight * scale);
	fPadding = floorf(kBasePadding * scale);
}


void
IconGridView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewUIColor(B_LIST_BACKGROUND_COLOR);
	SetLowUIColor(B_LIST_BACKGROUND_COLOR);

	if (Window() != NULL)
		Window()->SetPulseRate(50000);

	_CalculateSizes();
	_RecalculateLayout();
}


void
IconGridView::Pulse()
{
	if (!fLoading)
		return;

	if (fItems.IsEmpty()) {
		Invalidate(_LoadingIndicatorRect());
	} else if (fHasMore || fLoading) {
		Invalidate(_LoadMoreIconRect());
	}
}


float
IconGridView::_AnimationAngle() const
{
	return fmod(system_time() * kAnimationSpeed, 2 * M_PI);
}


BRect
IconGridView::_LoadingIndicatorRect() const
{
	BRect bounds = Bounds();
	float centerX = bounds.Width() / 2;
	float centerY = bounds.Height() / 2;
	float radius = 30;

	font_height fh;
	GetFontHeight(&fh);
	float textHeight = fh.ascent + fh.descent;

	return BRect(
		centerX - radius - 10,
		centerY - radius - 10,
		centerX + radius + 10,
		centerY + radius + textHeight + 20
	);
}


BRect
IconGridView::_LoadMoreIconRect() const
{
	BRect frame = _LoadMoreFrame();
	if (!frame.IsValid())
		return BRect();

	float iconLeft = frame.left + (fCellWidth - fIconSize) / 2;
	float iconTop = frame.top + fPadding;

	return BRect(iconLeft - 2, iconTop - 2, iconLeft + fIconSize + 2, iconTop + fIconSize + 2);
}


void
IconGridView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgDeleteTempFile: {
			BString filePath;
			if (message->FindString("path", &filePath) == B_OK) {
				BEntry entry(filePath.String());
				if (entry.Exists())
					entry.Remove();
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
IconGridView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL) {
		float currentWidth = Bounds().Width();
		*width = (currentWidth > 10) ? currentWidth : 200;
	}

	if (height != NULL) {
		float currentHeight = Bounds().Height();
		*height = (currentHeight > 10) ? currentHeight : 150;
	}
}


void
IconGridView::SetLoading(bool loading)
{
	if (fLoading != loading) {
		fLoading = loading;
		if (!fLoading)
			_CheckAutoLoad();
		Invalidate();
	}
}


void
IconGridView::SetHasMore(bool hasMore)
{
	if (fHasMore != hasMore) {
		fHasMore = hasMore;
		_RecalculateLayout();
		_CheckAutoLoad();
		Invalidate();
	}
}


void
IconGridView::_DrawLoadingIndicator(BRect bounds)
{
	float angle = _AnimationAngle();

	float centerX = bounds.Width() / 2;
	float centerY = bounds.Height() / 2;
	float radius = 20;

	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	
	const char* text = B_TRANSLATE("Loading...");
	float textWidth = StringWidth(text);
	font_height fh;
	GetFontHeight(&fh);
	DrawString(text, BPoint(centerX - textWidth / 2, centerY + radius + fh.ascent + 10));

	for (int i = 0; i < 8; i++) {
		float dotAngle = angle + i * (M_PI / 4);
		float x = centerX + cos(dotAngle) * radius;
		float y = centerY + sin(dotAngle) * radius;

		float alpha = 1.0f - (i / 8.0f);
		rgb_color color = tint_color(ViewColor(), B_DARKEN_3_TINT);
		color.alpha = (uint8)(alpha * 255);
		SetHighColor(color);

		FillEllipse(BPoint(x, y), 4, 4);
	}
}


void
IconGridView::_DrawLoadMoreItem(BRect frame)
{
	if (fLoadMoreHovered) {
		SetHighColor(tint_color(ui_color(B_LIST_BACKGROUND_COLOR), 
			B_DARKEN_1_TINT));
		BRect hoverRect = frame;
		hoverRect.InsetBy(2, 2);
		FillRoundRect(hoverRect, 4, 4);
	}

	float iconLeft = frame.left + (fCellWidth - fIconSize) / 2;
	float iconTop = frame.top + fPadding;
	BRect iconRect(iconLeft, iconTop, 
		iconLeft + fIconSize - 1, iconTop + fIconSize - 1);

	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRoundRect(iconRect, 4, 4);

	if (fLoading) {
		float angle = _AnimationAngle();

		float centerX = iconRect.left + iconRect.Width() / 2;
		float centerY = iconRect.top + iconRect.Height() / 2;
		float radius = fIconSize / 4;

		for (int i = 0; i < 8; i++) {
			float dotAngle = angle + i * (M_PI / 4);
			float x = centerX + cos(dotAngle) * radius;
			float y = centerY + sin(dotAngle) * radius;

			float alpha = 1.0f - (i / 8.0f);
			rgb_color color = tint_color(ViewColor(), B_DARKEN_3_TINT);
			color.alpha = (uint8)(alpha * 255);
			SetHighColor(color);

			FillEllipse(BPoint(x, y), 3, 3);
		}
	} else {
		SetHighColor(tint_color(ViewColor(), B_DARKEN_3_TINT));
		float dotY = iconRect.top + iconRect.Height() / 2;
		float dotSpacing = fIconSize / 5;
		float dotX = iconRect.left + iconRect.Width() / 2 - dotSpacing;
		float dotRadius = fIconSize / 16;
		if (dotRadius < 3) dotRadius = 3;

		for (int d = 0; d < 3; d++) {
			FillEllipse(BPoint(dotX + d * dotSpacing, dotY), dotRadius, dotRadius);
		}
	}

	font_height fh;
	GetFontHeight(&fh);

	SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	const char* text = fLoading ? B_TRANSLATE("Loading...") : B_TRANSLATE("Load more");
	float textWidth = StringWidth(text);
	float textX = frame.left + (fCellWidth - textWidth) / 2;
	float textY = frame.top + fPadding + fIconSize + 4 + fh.ascent;

	DrawString(text, BPoint(textX, textY));
}


void
IconGridView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();
	
	SetLowColor(ViewColor());
	FillRect(updateRect, B_SOLID_LOW);

	if (fLoading && fItems.IsEmpty()) {
		SetDrawingMode(B_OP_ALPHA);
		_DrawLoadingIndicator(bounds);
		SetDrawingMode(B_OP_COPY);
		return;
	}

	if (fItems.IsEmpty() && !fHasMore) {
		SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
		const char* text = B_TRANSLATE("No icons loaded");
		float width = StringWidth(text);
		font_height fh;
		GetFontHeight(&fh);
		DrawString(text, BPoint(
			(bounds.Width() - width) / 2,
			(bounds.Height() + fh.ascent) / 2
		));
		return;
	}

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	font_height fh;
	GetFontHeight(&fh);

	for (int32 i = 0; i < fItems.CountItems(); i++) {
		BRect frame = _ItemFrame(i);

		if (!updateRect.Intersects(frame))
			continue;

		IconItem* item = fItems.ItemAt(i);

		if (i == fHoveredItem && i != fSelection) {
			SetHighColor(tint_color(ui_color(B_LIST_BACKGROUND_COLOR), 
				B_DARKEN_1_TINT));
			BRect hoverRect = frame;
			hoverRect.InsetBy(2, 2);
			FillRoundRect(hoverRect, 4, 4);
		}

		if (i == fSelection) {
			SetHighColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
			BRect selRect = frame;
			selRect.InsetBy(2, 2);
			FillRoundRect(selRect, 4, 4);
		}

		float iconLeft = floorf(frame.left + (fCellWidth - fIconSize) / 2.0f);
		float iconTop = floorf(frame.top + fPadding);
		BRect iconRect(iconLeft, iconTop, 
			iconLeft + fIconSize - 1, iconTop + fIconSize - 1);

		if (item->bitmap != NULL) {
			DrawBitmap(item->bitmap, iconRect);
		} else {
			SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
			FillRoundRect(iconRect, 4, 4);

			SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
			float dotY = iconRect.top + iconRect.Height() / 2;
			float dotX = iconRect.left + iconRect.Width() / 2 - 12;
			for (int d = 0; d < 3; d++) {
				FillEllipse(BPoint(dotX + d * 12, dotY), 3, 3);
			}
		}

		rgb_color textColor = (i == fSelection) 
			? ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)
			: ui_color(B_LIST_ITEM_TEXT_COLOR);
		SetHighColor(textColor);

		BString name = item->title;
		float maxWidth = fCellWidth - fPadding * 2;

		float textY = floorf(frame.top + fPadding + fIconSize + 4 + fh.ascent);

		TruncateString(&name, B_TRUNCATE_MIDDLE, maxWidth);
		float textWidth = StringWidth(name);
		float textX = floorf(frame.left + (fCellWidth - textWidth) / 2.0f);
		DrawString(name, BPoint(textX, textY));
	}

	if (fHasMore || fLoading) {
		BRect loadMoreFrame = _LoadMoreFrame();
		if (updateRect.Intersects(loadMoreFrame)) {
			_DrawLoadMoreItem(loadMoreFrame);
		}
	}
}


void
IconGridView::FrameResized(float width, float height)
{
	int32 anchorIndex = -1;

	if (fSelection < 0 && fColumns > 0 && !fItems.IsEmpty()) {
		float rowHeight = fCellHeight + fPadding;
		int32 row = (int32)(Bounds().top / rowHeight);
		anchorIndex = row * fColumns;
		if (anchorIndex < 0) anchorIndex = 0;
		if (anchorIndex >= fItems.CountItems())
			anchorIndex = fItems.CountItems() - 1;
	}

	_RecalculateLayout();

	float targetY = -1.0f;

	if (fSelection >= 0) {
		BRect frame = _ItemFrame(fSelection);
		targetY = frame.top + (frame.Height() / 2) - (height / 2);
	} else if (anchorIndex >= 0) {
		BRect frame = _ItemFrame(anchorIndex);
		targetY = frame.top - fPadding;
	}

	if (targetY != -1.0f) {
		float maxScroll = fTotalHeight - height;
		if (maxScroll < 0) maxScroll = 0;

		if (targetY < 0) targetY = 0;
		if (targetY > maxScroll) targetY = maxScroll;

		ScrollTo(BPoint(0, targetY));
	}

	_CheckAutoLoad();
	Invalidate();
	BView::FrameResized(width, height);
}


void
IconGridView::ScrollTo(BPoint where)
{
	BView::ScrollTo(where);
	_CheckAutoLoad();
}


void
IconGridView::MouseDown(BPoint where)
{
	MakeFocus(true);

	if (_IsLoadMoreAtPoint(where)) {
		if (!fLoading && fHasMore) {
			BWindow* window = Window();
			if (window != NULL)
				window->PostMessage(kMsgLoadMore);
		}
		return;
	}

	int32 newSelection = _ItemAtPoint(where);

	BMessage* currentMessage = Window()->CurrentMessage();
	uint32 buttons = 0;
	int32 clicks = 1;

	if (currentMessage != NULL) {
		currentMessage->FindInt32("buttons", (int32*)&buttons);
		currentMessage->FindInt32("clicks", &clicks);
	}

	if (newSelection >= 0) {
		IconItem* item = fItems.ItemAt(newSelection);
		if (item != NULL && item->hvifData != NULL && item->hvifSize > 0) {
			fDragButton = buttons;
			fClickPoint = where;
			fDragStarted = false;
			fDragItemIndex = newSelection;
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		}
	}

	if (newSelection != fSelection) {
		int32 oldSelection = fSelection;
		fSelection = newSelection;

		if (oldSelection >= 0)
			Invalidate(_ItemFrame(oldSelection));
		if (fSelection >= 0)
			Invalidate(_ItemFrame(fSelection));

		_UpdateInfoView();
	}

	BWindow* window = Window();
	if (window != NULL)
		window->PostMessage(kMsgSelectIcon);

	if (clicks == 2 && fSelection >= 0 && window != NULL)
		window->PostMessage(kMsgOpenIcon);
}


void
IconGridView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fDragButton != 0 && !fDragStarted && fDragItemIndex >= 0) {
		if (abs((int32)(where.x - fClickPoint.x)) > kDragThreshold ||
			abs((int32)(where.y - fClickPoint.y)) > kDragThreshold) {

			fDragStarted = true;
			IconItem* item = fItems.ItemAt(fDragItemIndex);
			if (item != NULL && item->hvifData != NULL)
				_StartDrag(fClickPoint, item);
		}
	}

	int32 newHovered = -1;
	bool newLoadMoreHovered = false;

	if (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW) {
		if (_IsLoadMoreAtPoint(where)) {
			newLoadMoreHovered = true;
		} else {
			newHovered = _ItemAtPoint(where);
		}
	}

	if (newHovered != fHoveredItem) {
		int32 oldHovered = fHoveredItem;
		fHoveredItem = newHovered;

		if (oldHovered >= 0)
			Invalidate(_ItemFrame(oldHovered));
		if (fHoveredItem >= 0)
			Invalidate(_ItemFrame(fHoveredItem));
	}

	if (newLoadMoreHovered != fLoadMoreHovered) {
		fLoadMoreHovered = newLoadMoreHovered;
		if (fHasMore || fLoading)
			Invalidate(_LoadMoreFrame());
	}

	BView::MouseMoved(where, transit, dragMessage);
}


void
IconGridView::MouseUp(BPoint where)
{
	fDragButton = 0;
	fDragStarted = false;
	fDragItemIndex = -1;
	BView::MouseUp(where);
}


void
IconGridView::KeyDown(const char* bytes, int32 numBytes)
{
	if (fItems.IsEmpty()) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	int32 newSelection = fSelection;

	switch (bytes[0]) {
		case B_LEFT_ARROW:
			if (fSelection > 0) newSelection--;
			break;
		case B_RIGHT_ARROW:
			if (fSelection < fItems.CountItems() - 1) newSelection++;
			break;
		case B_UP_ARROW:
			if (fSelection >= fColumns) newSelection -= fColumns;
			break;
		case B_DOWN_ARROW:
			if (fSelection + fColumns < fItems.CountItems())
				newSelection += fColumns;
			else if (fSelection < 0 && fItems.CountItems() > 0)
				newSelection = 0;
			break;
		case B_ENTER: {
			BWindow* window = Window();
			if (fSelection >= 0 && window != NULL)
				window->PostMessage(kMsgOpenIcon);
			return;
		}
		case B_HOME:
			newSelection = 0;
			break;
		case B_END:
			newSelection = fItems.CountItems() - 1;
			break;
		default:
			BView::KeyDown(bytes, numBytes);
			return;
	}

	if (newSelection != fSelection && newSelection >= 0) {
		int32 oldSelection = fSelection;
		fSelection = newSelection;

		if (oldSelection >= 0)
			Invalidate(_ItemFrame(oldSelection));

		Invalidate(_ItemFrame(fSelection));

		_ScrollToSelection();
		_UpdateInfoView();

		BWindow* window = Window();
		if (window != NULL)
			window->PostMessage(kMsgSelectIcon);
	}
}


void
IconGridView::SetInfoView(IconInfoView* infoView)
{
	fInfoView = infoView;
}


void
IconGridView::AddItem(IconItem* item)
{
	item->generation = fGeneration;
	fItems.AddItem(item);
	_RecalculateLayout();
	_CheckAutoLoad();
	Invalidate(_ItemFrame(fItems.CountItems() - 1));
}


void
IconGridView::Clear()
{
	fGeneration++;
	fItems.MakeEmpty();
	fSelection = -1;
	fHoveredItem = -1;
	fLoadMoreHovered = false;
	fTotalHeight = 0;
	fHasMore = false;
	fDragButton = 0;
	fDragStarted = false;
	fDragItemIndex = -1;

	if (fInfoView != NULL)
		fInfoView->Clear();

	ScrollTo(B_ORIGIN);
	_RecalculateLayout();
	Invalidate();
}


void
IconGridView::SetIcon(int32 id, BBitmap* bmp, int32 generation,
	const uint8* hvifData, size_t hvifDataSize)
{
	if (generation != fGeneration) {
		delete bmp;
		return;
	}

	for (int32 i = 0; i < fItems.CountItems(); i++) {
		IconItem* item = fItems.ItemAt(i);
		if (item->id == id) {
			delete item->bitmap;
			item->bitmap = bmp;

			delete[] item->hvifData;
			if (hvifData != NULL && hvifDataSize > 0) {
				item->hvifData = new uint8[hvifDataSize];
				memcpy(item->hvifData, hvifData, hvifDataSize);
				item->hvifSize = (int32)hvifDataSize;
			} else {
				item->hvifData = NULL;
			}

			Invalidate(_ItemFrame(i));

			if (i == fSelection && fInfoView != NULL)
				fInfoView->SetIcon(item);

			return;
		}
	}
	delete bmp;
}


bool
IconGridView::SelectIcon(int32 id)
{
	for (int32 i = 0; i < fItems.CountItems(); i++) {
		IconItem* item = fItems.ItemAt(i);
		if (item->id == id) {
			int32 oldSelection = fSelection;
			fSelection = i;

			if (oldSelection >= 0)
				Invalidate(_ItemFrame(oldSelection));

			Invalidate(_ItemFrame(fSelection));

			_ScrollToSelection();
			_UpdateInfoView();

			return true;
		}
	}
	return false;
}


IconItem*
IconGridView::SelectedItem() const
{
	if (fSelection < 0 || fSelection >= fItems.CountItems())
		return NULL;
	return fItems.ItemAt(fSelection);
}


int32
IconGridView::CountItems() const
{
	return fItems.CountItems();
}


void
IconGridView::_RecalculateLayout()
{
	float width = Bounds().Width();

	if (width < 50)
		return;

	fColumns = (int32)((width - fPadding) / (fCellWidth + fPadding));
	if (fColumns < 1)
		fColumns = 1;

	int32 count = fItems.CountItems();
	if (fHasMore || fLoading)
		count++;

	int32 rows = (count > 0) ? ((count + fColumns - 1) / fColumns) : 1;

	fTotalHeight = rows * (fCellHeight + fPadding) + fPadding;

	_UpdateScrollBar();
}


void
IconGridView::_UpdateScrollBar()
{
	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar == NULL)
		return;

	float visibleHeight = Bounds().Height();
	
	if (fTotalHeight <= visibleHeight) {
		scrollBar->SetRange(0, 0);
		scrollBar->SetProportion(1.0);
	} else {
		float maxScroll = fTotalHeight - visibleHeight;
		scrollBar->SetRange(0, maxScroll);
		scrollBar->SetProportion(visibleHeight / fTotalHeight);
	}

	scrollBar->SetSteps(fCellHeight / 3, visibleHeight - fCellHeight);
}


BRect
IconGridView::_ItemFrame(int32 index) const
{
	if (index < 0 || fColumns < 1)
		return BRect();

	int32 row = index / fColumns;
	int32 col = index % fColumns;

	float x = fPadding + col * (fCellWidth + fPadding);
	float y = fPadding + row * (fCellHeight + fPadding);

	return BRect(x, y, x + fCellWidth - 1, y + fCellHeight - 1);
}


BRect
IconGridView::_LoadMoreFrame() const
{
	return _ItemFrame(fItems.CountItems());
}


int32
IconGridView::_ItemAtPoint(BPoint point) const
{
	if (fItems.IsEmpty() || fColumns < 1)
		return -1;

	int32 col = (int32)((point.x - fPadding) / (fCellWidth + fPadding));
	int32 row = (int32)((point.y - fPadding) / (fCellHeight + fPadding));

	if (col < 0 || col >= fColumns || row < 0)
		return -1;

	int32 index = row * fColumns + col;

	if (index >= fItems.CountItems())
		return -1;

	if (_ItemFrame(index).Contains(point))
		return index;

	return -1;
}


bool
IconGridView::_IsLoadMoreAtPoint(BPoint point) const
{
	if (!fHasMore && !fLoading)
		return false;

	return _LoadMoreFrame().Contains(point);
}


void
IconGridView::_ScrollToSelection()
{
	if (fSelection < 0)
		return;

	BRect frame = _ItemFrame(fSelection);
	BRect bounds = Bounds();

	float scrollY = bounds.top;

	if (frame.top < bounds.top) {
		scrollY = frame.top - fPadding;
	} else if (frame.bottom > bounds.bottom) {
		scrollY = frame.bottom - bounds.Height() + fPadding;
	} else {
		return;
	}

	if (scrollY < 0)
		scrollY = 0;

	float maxScroll = fTotalHeight - bounds.Height();
	if (scrollY > maxScroll)
		scrollY = maxScroll;

	ScrollTo(BPoint(0, scrollY));
}


void
IconGridView::_UpdateInfoView()
{
	if (fInfoView == NULL)
		return;

	IconItem* item = SelectedItem();
	if (item != NULL)
		fInfoView->SetIcon(item);
	else
		fInfoView->Clear();
}


void
IconGridView::_StartDrag(BPoint point, IconItem* item)
{
	if (item == NULL || item->bitmap == NULL ||
		item->hvifData == NULL || item->hvifSize <= 0)
		return;

	BPath tempPath;
	status_t status = _CreateTempFile(tempPath, item->title.String());
	if (status != B_OK)
		return;

	BFile tempFile(tempPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (tempFile.InitCheck() != B_OK)
		return;

	ssize_t written = tempFile.Write(item->hvifData, item->hvifSize);
	if (written != (ssize_t)item->hvifSize) {
		tempFile.Unset();
		return;
	}

	_SetupTempFile(tempPath, item->hvifData, item->hvifSize);
	tempFile.Unset();

	BMessage msg(B_SIMPLE_DATA);
	msg.AddData("icon", B_VECTOR_ICON_TYPE, item->hvifData, item->hvifSize);
	msg.AddPoint("click_pt", point);

	entry_ref ref;
	BEntry entry(tempPath.Path());
	if (entry.GetRef(&ref) == B_OK) {
		msg.AddRef("refs", &ref);
	}

	BPoint tmpLoc;
	uint32 button;
	GetMouse(&tmpLoc, &button);
	msg.AddInt32("buttons", (int32)button);
	msg.AddInt32("be:actions", B_COPY_TARGET);

	BBitmap* dragBitmap = new BBitmap(
		BRect(0, 0, fIconSize - 1, fIconSize - 1), B_RGBA32);
	if (BIconUtils::GetVectorIcon(item->hvifData, item->hvifSize, dragBitmap) != B_OK) {
		delete dragBitmap;
		dragBitmap = new BBitmap(item->bitmap);
	}

	BPoint dragOffset(fIconSize / 2, fIconSize / 2);
	DragMessage(&msg, dragBitmap, B_OP_ALPHA, dragOffset, this);

	fDragButton = 0;
	_DeleteFileDelayed(tempPath);
}


status_t
IconGridView::_CreateTempFile(BPath& tempPath, const char* title)
{
	BPath tempDir;
	status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempDir);
	if (status != B_OK)
		return status;

	BString safeName(title);
	safeName.ReplaceAll("/", "_");
	safeName.ReplaceAll(":", "_");
	safeName.ReplaceAll(" ", "_");

	char tempName[B_FILE_NAME_LENGTH];
	snprintf(tempName, sizeof(tempName), "hvif_%s_%lld.hvif", safeName.String(), system_time());

	status = tempPath.SetTo(tempDir.Path(), tempName);
	return status;
}


void
IconGridView::_SetupTempFile(const BPath& tempPath, const uint8* data, size_t size)
{
	BFile file(tempPath.Path(), B_READ_WRITE);
	if (file.InitCheck() != B_OK)
		return;

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		nodeInfo.SetType(MIME_HVIF_SIGNATURE);
		nodeInfo.SetIcon(data, size);
	}
}


void
IconGridView::_DeleteFileDelayed(const BPath& filePath)
{
	BMessage* deleteMsg = new BMessage(kMsgDeleteTempFile);
	deleteMsg->AddString("path", filePath.Path());

	BMessageRunner* runner = new BMessageRunner(this, deleteMsg, kTempFileDeleteDelay, 1);
	(void)runner;
}


void
IconGridView::_CleanupOldTempFiles()
{
	BPath tempDir;
	if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempDir) != B_OK)
		return;

	BDirectory dir(tempDir.Path());
	if (dir.InitCheck() != B_OK)
		return;

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_OK) {
			if (strncmp(name, "hvif_", 5) == 0) {
				time_t modTime;
				entry.GetModificationTime(&modTime);
				time_t now = time(NULL);
				if (now - modTime > 3600) {
					entry.Remove();
				}
			}
		}
	}
}


void
IconGridView::_CheckAutoLoad()
{
	if (fLoading || !fHasMore || Window() == NULL)
		return;

	float rowHeight = fCellHeight + fPadding;

	// Only auto-load if there is a gap of more than one row at the bottom
	if (Bounds().bottom > fTotalHeight + rowHeight)
		Window()->PostMessage(kMsgLoadMore);
}
