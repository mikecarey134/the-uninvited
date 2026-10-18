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
	LootScreen(std::function<bool()> blocked, std::function<void()> opened, std::function<loot::Container*()> nearby);

	void attach(inventory::Inventory* carried, const inventory::Catalog* catalog) { carried_ = carried; catalog_ = catalog; }
	bool onEvent(const irr::SEvent& event) override;
	bool isOpen() const { return open_; }
	void open();
	void close();
	void draw(Canvas& canvas);
	irr::core::rect<irr::s32> containerPanel() const;
	irr::core::rect<irr::s32> playerPanel() const;
	int containerColumns() const;
	int containerRows() const;
	irr::core::rect<irr::s32> containerCell(int index) const;
	backpack::Layout layout() const;
	loot::Container* container() const { return container_; }
	const std::string& message() const { return message_; }

private:
	irr::s32 left() const;

	std::function<bool()> blocked_;
	std::function<void()> opened_;
	std::function<loot::Container*()> nearby_;
	inventory::Inventory* carried_ = nullptr;
	const inventory::Catalog* catalog_ = nullptr;
	loot::Container* container_ = nullptr;
	bool open_ = false;
	bool inContainer_ = true;
	int slot_ = 0;
	inventory::Cell cell_{ 0, 0 };
	bool keyboard_ = true;
	std::string message_;
	irr::core::dimension2d<irr::u32> screen_{ 1280, 720 };
};

#endif
