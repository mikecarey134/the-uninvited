#include "doctest.h"
#include "Notebook.h"
#include "RecordingCanvas.h"
#include <algorithm>
#include <string>

using namespace irr;
using namespace core;

namespace
{
	const character::Profession medic{ "Medic", { { character::MEDICINE, 30 }, { character::SPEECH, 5 } }, {} };

	character::CharacterSheet ada(bool bloodline)
	{
		character::Aegis aegis;
		for (int i = 0; i < character::ATTRIBUTE_COUNT; ++i)
			aegis.rolled[i] = 11 + i * 7;
		aegis.placed[character::GUILE] = 2;
		aegis.hasBloodline = bloodline;
		aegis.bloodlineAttribute = character::STRENGTH;
		aegis.bloodline = "Ironblood";
		return character::CharacterSheet::create("Ada", character::Gender::Female, medic, aegis);
	}

	struct Book
	{
		Book() : console(false), opened(0), notebook([this] { return console; }, [this] { ++opened; }) { canvas.screenDimension = dimension2d<u32>(1280, 720); }

		bool key(EKEY_CODE code, bool down = true)
		{
			SEvent event;
			event.EventType = EET_KEY_INPUT_EVENT;
			event.KeyInput.Key = code;
			event.KeyInput.PressedDown = down;
			event.KeyInput.Char = 0;
			event.KeyInput.Shift = false;
			event.KeyInput.Control = false;
			return notebook.onEvent(event);
		}

		bool click(position2d<s32> at)
		{
			SEvent event;
			event.EventType = EET_MOUSE_INPUT_EVENT;
			event.MouseInput.Event = EMIE_LMOUSE_PRESSED_DOWN;
			event.MouseInput.X = at.X;
			event.MouseInput.Y = at.Y;
			event.MouseInput.Wheel = 0.0f;
			event.MouseInput.ButtonStates = 0;
			event.MouseInput.Shift = false;
			event.MouseInput.Control = false;
			return notebook.onEvent(event);
		}

		void draw(const character::CharacterSheet& sheet)
		{
			canvas.calls.clear();
			notebook.draw(canvas, sheet);
		}

		const RecordingCanvas::Call* image(const std::string& file, int nth = 0)
		{
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::IMAGE && canvas.pathOf(call.texture) == "assets/hud/" + file && nth-- == 0)
					return &call;
			return nullptr;
		}

		const RecordingCanvas::Call* text(const std::wstring& line)
		{
			for (const RecordingCanvas::Call& call : canvas.calls)
				if (call.kind == RecordingCanvas::TEXT && call.text == line)
					return &call;
			return nullptr;
		}

		bool console;
		int opened;
		Notebook notebook;
		RecordingCanvas canvas;
	};

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}
}

TEST_CASE("C and I open the notebook to their page, the same key again closes it, the other key switches, and Escape closes")
{
	Book book;
	CHECK_FALSE(book.notebook.isOpen());
	CHECK_FALSE(book.key(KEY_ESCAPE));
	CHECK(book.key(KEY_KEY_C));
	CHECK(book.notebook.isOpen());
	CHECK(book.notebook.page() == Notebook::CHARACTER);
	CHECK(book.key(KEY_KEY_I));
	CHECK(book.notebook.isOpen());
	CHECK(book.notebook.page() == Notebook::INVENTORY);
	CHECK(book.key(KEY_KEY_I));
	CHECK_FALSE(book.notebook.isOpen());
	CHECK(book.key(KEY_KEY_I));
	CHECK(book.notebook.page() == Notebook::INVENTORY);
	CHECK(book.key(KEY_KEY_C));
	CHECK(book.notebook.page() == Notebook::CHARACTER);
	CHECK(book.key(KEY_KEY_C));
	CHECK_FALSE(book.notebook.isOpen());
	CHECK(book.key(KEY_KEY_C));
	CHECK(book.key(KEY_ESCAPE));
	CHECK_FALSE(book.notebook.isOpen());
	CHECK(book.opened == 3);
}

TEST_CASE("while the console is up the notebook keys do nothing, so typing into the console is safe")
{
	Book book;
	book.console = true;
	CHECK_FALSE(book.key(KEY_KEY_C));
	CHECK_FALSE(book.key(KEY_KEY_I));
	CHECK_FALSE(book.notebook.isOpen());
	CHECK(book.opened == 0);
}

TEST_CASE("an open notebook swallows key presses and the mouse, but lets key releases and the console key through")
{
	Book book;
	CHECK_FALSE(book.key(KEY_KEY_W));
	book.key(KEY_KEY_C);
	CHECK(book.key(KEY_KEY_W));
	CHECK(book.key(KEY_KEY_D));
	CHECK_FALSE(book.key(KEY_KEY_W, false));
	CHECK_FALSE(book.key(KEY_OEM_3));
	CHECK(book.click(position2d<s32>(5, 5)));
	book.key(KEY_ESCAPE);
	CHECK_FALSE(book.click(position2d<s32>(5, 5)));
}

TEST_CASE("clicking a bookmark turns to its page")
{
	Book book;
	book.key(KEY_KEY_C);
	book.draw(ada(false));
	dimension2d<u32> screen(1280, 720);
	for (int tab : { Notebook::MAP, Notebook::NOTES, Notebook::INVENTORY, Notebook::CHARACTER })
	{
		CHECK(book.click(book.notebook.tabArea(tab, screen).getCenter()));
		CHECK(book.notebook.page() == tab);
		CHECK(book.notebook.isOpen());
	}
}

TEST_CASE("the open notebook dims the screen, centres the journal spread, and pokes four bookmarks out of its right edge, the active one pushed in and brighter")
{
	Book book;
	book.draw(ada(false));
	CHECK(book.canvas.calls.empty());
	book.key(KEY_KEY_I);
	book.draw(ada(false));
	REQUIRE_FALSE(book.canvas.calls.empty());
	const RecordingCanvas::Call& dim = book.canvas.calls.front();
	CHECK(dim.kind == RecordingCanvas::RECT);
	CHECK(dim.area == rect<s32>(0, 0, 1280, 720));
	CHECK(dim.colour.getAlpha() > 80);
	CHECK(dim.colour.getAlpha() < 230);
	CHECK(dim.colour.getRed() < 40);

	const RecordingCanvas::Call* spread = book.image("notebook_spread.png");
	REQUIRE(spread != nullptr);
	CHECK(spread->area.getHeight() >= 720 * 3 / 4);
	CHECK(spread->area.UpperLeftCorner.X >= 16);
	CHECK(spread->area.UpperLeftCorner.Y >= 16);
	CHECK(spread->area.LowerRightCorner.Y <= 720 - 16);
	CHECK(std::abs(spread->area.getCenter().Y - 360) <= 2);

	std::vector<const RecordingCanvas::Call*> tabs;
	for (int i = 0; i < 4; ++i)
	{
		const RecordingCanvas::Call* tab = book.image("bookmark_tabs.png", i);
		REQUIRE(tab != nullptr);
		tabs.push_back(tab);
		CHECK(tab->area.LowerRightCorner.X > spread->area.LowerRightCorner.X + 30);
		CHECK(tab->area.UpperLeftCorner.X < spread->area.LowerRightCorner.X);
		CHECK(tab->area.LowerRightCorner.X <= 1280);
	}
	std::sort(tabs.begin(), tabs.end(), [](const RecordingCanvas::Call* a, const RecordingCanvas::Call* b) { return a->area.UpperLeftCorner.Y < b->area.UpperLeftCorner.Y; });
	for (int i = 1; i < 4; ++i)
	{
		CHECK(tabs[i]->source.UpperLeftCorner.Y > tabs[i - 1]->source.UpperLeftCorner.Y);
		CHECK(tabs[i]->area.UpperLeftCorner.Y >= tabs[i - 1]->area.LowerRightCorner.Y);
	}
	const RecordingCanvas::Call* active = tabs[Notebook::INVENTORY];
	for (int i = 0; i < 4; ++i)
	{
		if (i == Notebook::INVENTORY)
			continue;
		CHECK(active->area.LowerRightCorner.X < tabs[i]->area.LowerRightCorner.X);
		CHECK(active->colour.getRed() > tabs[i]->colour.getRed());
		CHECK(tabs[i] < spread);
	}
	CHECK(active > spread);
	CHECK(book.notebook.tabArea(Notebook::INVENTORY, dimension2d<u32>(1280, 720)) == active->area);
}

TEST_CASE("the character sheet page shows the title, profession and AEGIS on the left and every skill on the right, in dark ink, read through the getters")
{
	Book book;
	book.key(KEY_KEY_C);
	character::CharacterSheet sheet = ada(false);
	book.draw(sheet);
	const RecordingCanvas::Call* spread = book.image("notebook_spread.png");
	REQUIRE(spread != nullptr);
	s32 middle = spread->area.getCenter().X;
	const RecordingCanvas::Call* title = book.text(L"CHARACTER SHEET");
	REQUIRE(title != nullptr);
	CHECK(title->area.LowerRightCorner.X <= middle);
	REQUIRE(book.text(L"Medic") != nullptr);
	CHECK(book.text(L"Medic")->area.LowerRightCorner.X <= middle);
	const int values[] = { sheet.getAgility(), sheet.getEndurance(), sheet.getGuile(), sheet.getIntelligence(), sheet.getStrength() };
	for (int i = 0; i < character::ATTRIBUTE_COUNT; ++i)
	{
		INFO(character::attributeName(i));
		const RecordingCanvas::Call* name = book.text(wide(character::attributeName(i)));
		REQUIRE(name != nullptr);
		CHECK(name->area.LowerRightCorner.X <= middle);
		const RecordingCanvas::Call* value = book.text(std::to_wstring(values[i]));
		REQUIRE(value != nullptr);
		CHECK(value->area.LowerRightCorner.X <= middle);
		CHECK(std::abs(value->area.getCenter().Y - name->area.getCenter().Y) <= 2);
	}
	const int skills[] = { sheet.getSmithing(), sheet.getChemistry(), sheet.getMedicine(), sheet.getCooking(), sheet.getSurvivalism(),
		sheet.getLockpicking(), sheet.getElectronics(), sheet.getEngineering(), sheet.getSpeech(), sheet.getConstruction() };
	for (int i = 0; i < character::SKILL_COUNT; ++i)
	{
		INFO(character::skillName(i));
		const RecordingCanvas::Call* name = book.text(wide(character::skillName(i)));
		REQUIRE(name != nullptr);
		CHECK(name->area.UpperLeftCorner.X >= middle);
		bool valued = false;
		for (const RecordingCanvas::Call& call : book.canvas.calls)
			if (call.kind == RecordingCanvas::TEXT && call.text == std::to_wstring(skills[i]) && call.area.UpperLeftCorner.X >= middle &&
				std::abs(call.area.getCenter().Y - name->area.getCenter().Y) <= 2)
				valued = true;
		CHECK(valued);
	}
	for (const RecordingCanvas::Call& call : book.canvas.calls)
		if (call.kind == RecordingCanvas::TEXT)
		{
			CHECK(call.colour.getRed() >= 30);
			CHECK(call.colour.getRed() <= 120);
			CHECK(call.colour.getBlue() < call.colour.getRed());
		}
	for (const RecordingCanvas::Call& call : book.canvas.calls)
		CHECK(call.text.find(L"Ironblood") == std::wstring::npos);
}

TEST_CASE("a character with a bloodline has it named with its attribute on the right page")
{
	Book book;
	book.key(KEY_KEY_C);
	book.draw(ada(true));
	const RecordingCanvas::Call* spread = book.image("notebook_spread.png");
	REQUIRE(spread != nullptr);
	const RecordingCanvas::Call* line = nullptr;
	for (const RecordingCanvas::Call& call : book.canvas.calls)
		if (call.kind == RecordingCanvas::TEXT && call.text.find(L"Ironblood") != std::wstring::npos)
			line = &call;
	REQUIRE(line != nullptr);
	CHECK(line->text.find(L"Strength") != std::wstring::npos);
	CHECK(line->area.UpperLeftCorner.X >= spread->area.getCenter().X);
}

TEST_CASE("the inventory page says Empty under its title on the left, and the map and notes pages promise what is coming")
{
	Book book;
	book.key(KEY_KEY_I);
	book.draw(ada(false));
	const RecordingCanvas::Call* spread = book.image("notebook_spread.png");
	REQUIRE(spread != nullptr);
	s32 middle = spread->area.getCenter().X;
	REQUIRE(book.text(L"INVENTORY") != nullptr);
	const RecordingCanvas::Call* empty = book.text(L"Empty");
	REQUIRE(empty != nullptr);
	CHECK(empty->hcenter);
	CHECK(empty->area.LowerRightCorner.X <= middle);
	CHECK(empty->area.UpperLeftCorner.Y > book.text(L"INVENTORY")->area.UpperLeftCorner.Y);
	CHECK(book.text(L"Medic") == nullptr);
	CHECK(book.text(L"CHARACTER SHEET") == nullptr);

	book.click(book.notebook.tabArea(Notebook::MAP, dimension2d<u32>(1280, 720)).getCenter());
	book.draw(ada(false));
	CHECK(book.text(L"MAP - Coming Soon") != nullptr);
	CHECK(book.text(L"INVENTORY") == nullptr);
	book.click(book.notebook.tabArea(Notebook::NOTES, dimension2d<u32>(1280, 720)).getCenter());
	book.draw(ada(false));
	CHECK(book.text(L"NOTES - Coming Soon") != nullptr);
}
