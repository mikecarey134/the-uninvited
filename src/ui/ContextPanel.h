#ifndef CONTEXT_PANEL_H
#define CONTEXT_PANEL_H

#include "Canvas.h"
#include <string>
#include <vector>

struct ContextRow
{
	int number;           // the 1-4 key that picks this row
	std::string iconPath; // relative to assets/ui/icons/
	std::string label;
};

// The PC list of a target's approaches, floating just below the crosshair. Only available approaches get a row.
class ContextPanel
{
public:
	void open(const std::vector<ContextRow>& rows);
	void close();
	bool isOpen() const { return open_; }
	const std::vector<ContextRow>& rows() const { return rows_; }

	void hover(irr::core::position2d<irr::s32> at);
	int hovered() const { return hovered_; }
	// The index of the row under a screen point, or -1.
	int rowAt(irr::core::position2d<irr::s32> at) const;
	irr::core::rect<irr::s32> rowArea(int index) const;
	void render(Canvas& canvas);

private:
	std::vector<ContextRow> rows_;
	bool open_ = false;
	int hovered_ = -1;
	irr::core::dimension2d<irr::u32> screen_ = irr::core::dimension2d<irr::u32>(1280, 720);
};

#endif
