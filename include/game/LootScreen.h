#ifndef LOOTSCREEN_H
#define LOOTSCREEN_H

#include <functional>
#include <string>
#include "Backpack.h"
#include "Canvas.h"
#include "InputRouter.h"
#include "Loot.h"

class LootScreen : public InputLayer
{
public:
	static const irr::s32 ROW_HEIGHT = 44;

	LootScreen(std::function<bool()> blocked, std::function<void()> opened, std::function<loot::Container*()> nearby);

	void attach(inventory::Inventory* carried, const inventory::Catalog* catalog) { carried_ = carried; catalog_ = catalog; }
	bool onEvent(const irr::SEvent& event) override;
	bool isOpen() const { return open_; }
	void open();
	void close();
	void draw(Canvas& canvas);
	irr::core::rect<irr::s32> panel() const;
	irr::core::rect<irr::s32> list() const;
	irr::core::rect<irr::s32> row(int index) const;
	backpack::Layout layout() const;
	loot::Container* container() const { return container_; }
	const std::string& message() const { return message_; }

private:
	int visibleRows() const;
	void showRow();

	std::function<bool()> blocked_;
	std::function<void()> opened_;
	std::function<loot::Container*()> nearby_;
	inventory::Inventory* carried_ = nullptr;
	const inventory::Catalog* catalog_ = nullptr;
	loot::Container* container_ = nullptr;
	bool open_ = false;
	int scroll_ = 0;
	bool inList_ = true;
	int row_ = 0;
	inventory::Cell cell_{ 0, 0 };
	bool keyboard_ = true;
	std::string message_;
	irr::core::dimension2d<irr::u32> screen_{ 1280, 720 };
};

#endif
