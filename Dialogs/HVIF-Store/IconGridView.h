/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICON_GRID_VIEW_H
#define ICON_GRID_VIEW_H

#include <View.h>
#include <ScrollView.h>
#include <String.h>
#include <Bitmap.h>
#include <ObjectList.h>

struct IconItem {
	int32       id;
	BString     title;
	BString     author;
	BString     license;
	BString     mimeType;
	BString     tags;
	
	BString     hvifUrl;
	BString     svgUrl;
	BString     iomUrl;
	
	int32       hvifSize;
	int32       svgSize;
	int32       iomSize;
	
	BBitmap*    bitmap;
	int32       generation;
	
	IconItem() 
		: id(0), hvifSize(0), svgSize(0), iomSize(0), 
		  bitmap(NULL), generation(0) {}
	~IconItem() { 
		delete bitmap;
	}
};

class IconInfoView;

class IconGridView : public BView {
public:
							IconGridView();
	virtual                 ~IconGridView();

	virtual void            AttachedToWindow();
	virtual void            Draw(BRect updateRect);
	virtual void            FrameResized(float width, float height);
	virtual void            MouseDown(BPoint where);
	virtual void            MouseMoved(BPoint where, uint32 transit,
								const BMessage* dragMessage);
	virtual void            KeyDown(const char* bytes, int32 numBytes);
	virtual void            GetPreferredSize(float* width, float* height);

			void            AddItem(IconItem* item);
			void            Clear();
			void            SetIcon(int32 id, BBitmap* bmp, int32 generation);
			IconItem*       SelectedItem() const;
			int32           CountItems() const;
			int32           CurrentGeneration() const { return fGeneration; }
			int32           IconSize() const { return (int32)fIconSize; }
			
			void            SetInfoView(IconInfoView* infoView);
			void            SetLoading(bool loading);
			bool            IsLoading() const { return fLoading; }
			void            SetHasMore(bool hasMore);
			bool            HasMore() const { return fHasMore; }

private:
			void            _CalculateSizes();
			void            _RecalculateLayout();
			void            _UpdateScrollBar();
			BRect           _ItemFrame(int32 index) const;
			BRect           _LoadMoreFrame() const;
			int32           _ItemAtPoint(BPoint point) const;
			bool            _IsLoadMoreAtPoint(BPoint point) const;
			void            _ScrollToSelection();
			void            _UpdateInfoView();
			void            _DrawLoadingIndicator(BRect bounds);
			void            _DrawLoadMoreItem(BRect frame);

#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
			BObjectList<IconItem, true> fItems;
#else
			BObjectList<IconItem> fItems;
#endif
			int32           fSelection;
			int32           fHoveredItem;
			bool            fLoadMoreHovered;
			int32           fGeneration;
			bool            fLoading;
			bool            fHasMore;
			
			IconInfoView*   fInfoView;
			
			int32           fColumns;
			float           fTotalHeight;
			
			float           fIconSize;
			float           fCellWidth;
			float           fCellHeight;
			float           fPadding;
			
	static const float      kBaseIconSize;
	static const float      kBaseCellWidth;
	static const float      kBaseCellHeight;
	static const float      kBasePadding;
	static const float      kBaseFontSize;
};

#endif
