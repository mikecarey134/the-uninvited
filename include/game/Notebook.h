#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include <functional>
#include "Canvas.h"
#include "CharacterSheet.h"
#include "InputRouter.h"

class Notebook : public InputLayer
{
public:
	static const int CHARACTER = 0;
	static const int INVENTORY = 1;
	static const int MAP = 2;
	static const int NOTES = 3;
	static const int TABS = 4;

	Notebook(std::function<bool()> blocked, std::function<void()> opened);

	bool onEvent(const irr::SEvent& event) override;
	bool isOpen() const { return open_; }
	int page() const { return page_; }
	void open(int page);
	void close() { open_ = false; }
	void draw(Canvas& canvas, const character::CharacterSheet& sheet);
	irr::core::rect<irr::s32> tabArea(int tab, irr::core::dimension2d<irr::u32> screen) const;

private:
	void toggle(int page);

	std::function<bool()> blocked_;
	std::function<void()> opened_;
	bool open_;
	int page_;
	irr::core::dimension2d<irr::u32> screen_;
};

#endif
