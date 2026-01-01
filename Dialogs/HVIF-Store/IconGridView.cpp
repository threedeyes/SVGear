/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ScrollBar.h>
#include <Window.h>
#include <ControlLook.h>
#include <Font.h>
#include <Catalog.h>

#include <cmath>

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
	fPadding(kBasePadding)
{
	SetViewUIColor(B_LIST_BACKGROUND_COLOR);
	SetLowUIColor(B_LIST_BACKGROUND_COLOR);
	SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);

	SetExplicitMinSize(BSize(200, 150));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
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

	fIconSize = kBaseIconSize * scale;
	fCellWidth = kBaseCellWidth * scale;
	fCellHeight = kBaseCellHeight * scale;
	fPadding = kBasePadding * scale;
}


void
IconGridView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewUIColor(B_LIST_BACKGROUND_COLOR);
	SetLowUIColor(B_LIST_BACKGROUND_COLOR);

	if (Window() != NULL)
		Window()->SetPulseRate(100000);

	_CalculateSizes();
	_RecalculateLayout();
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
		Invalidate();
	}
}


void
IconGridView::SetHasMore(bool hasMore)
{
	if (fHasMore != hasMore) {
		fHasMore = hasMore;
		_RecalculateLayout();
		Invalidate();
	}
}


void
IconGridView::_DrawLoadingIndicator(BRect bounds)
{
	static float angle = 0;
	angle += 0.3f;
	if (angle > 2 * M_PI)
		angle -= 2 * M_PI;

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

	if (fLoading) {
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		FillRoundRect(iconRect, 4, 4);

		static float angle = 0;
		angle += 0.2f;
		if (angle > 2 * M_PI)
			angle -= 2 * M_PI;

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
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		FillRoundRect(iconRect, 4, 4);

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

		float iconLeft = frame.left + (fCellWidth - fIconSize) / 2;
		float iconTop = frame.top + fPadding;
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

		float textY = frame.top + fPadding + fIconSize + 4 + fh.ascent;

		TruncateString(&name, B_TRUNCATE_MIDDLE, maxWidth);
		float textWidth = StringWidth(name);
		float textX = frame.left + (fCellWidth - textWidth) / 2;
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
	_RecalculateLayout();
	Invalidate();
	BView::FrameResized(width, height);
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

	int32 clicks = 1;
	if (window != NULL && window->CurrentMessage() != NULL)
		window->CurrentMessage()->FindInt32("clicks", &clicks);
	if (clicks == 2 && fSelection >= 0 && window != NULL)
		window->PostMessage(kMsgOpenIcon);
}


void
IconGridView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
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

	if (fInfoView != NULL)
		fInfoView->Clear();

	ScrollTo(B_ORIGIN);
	_RecalculateLayout();
	Invalidate();
}


void
IconGridView::SetIcon(int32 id, BBitmap* bmp, int32 generation)
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
			Invalidate(_ItemFrame(i));

			if (i == fSelection && fInfoView != NULL)
				fInfoView->SetIcon(item);

			return;
		}
	}
	delete bmp;
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
