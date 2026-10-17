#ifndef INVENTORYSCREEN_H
#define INVENTORYSCREEN_H

#include <functional>
#include "Backpack.h"
#include "Canvas.h"
#include "InputRouter.h"

class InventoryScreen : public InputLayer
{
public:
	InventoryScreen(std::function<bool()> blocked, std::function<void()> opened);

	void attach(backpack::Page* page) { page_ = page; }
	bool onEvent(const irr::SEvent& event) override;
	bool isOpen() const { return open_; }
	void open();
	void close();
	void draw(Canvas& canvas);
	irr::core::rect<irr::s32> panel() const;
	backpack::Layout layout() const;

private:
	std::function<bool()> blocked_;
	std::function<void()> opened_;
	backpack::Page* page_ = nullptr;
	bool open_ = false;
	irr::core::dimension2d<irr::u32> screen_{ 1280, 720 };
	irr::core::position2d<irr::s32> mouse_{ 0, 0 };
	backpack::Cursor cursor_;
	bool keyboard_ = true;
};

#endif
