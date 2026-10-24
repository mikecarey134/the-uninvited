#include "MenuScreens.h"
#include "Focus.h"
#include "Outfits.h"
#include "World.h"
#include "Hud.h"
#include "DebugOverlay.h"
#include "MenuScene.h"
#include "SoundEngine.h"
#include "Theme.h"
#include "UiAnchor.h"
#include <algorithm>
#include <cmath>
#include "IconArt.h"
#include "InteractPrompt.h"

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;
using menu::Screen;

namespace
{
	const char* const gameTitle = "GAME NAME";
	const s32 turnArea = 480;
	const f32 degreesPerPixel = 0.5f;
	const s32 formRight = 448;
	const s32 statTop = 428;
	const s32 statRow = 28;
	const s32 skillTop = 150;
	const s32 skillRow = 32;
	const s32 headingY = 40;
	const s32 contentY = 104;
	const s32 bottomTop = 632;
	const s32 bottomBottom = 672;
	const char* const tagline = "The dead don't rest. Neither can you.";
	const char* const titleFont = "assets/fonts/title.png";
	const char* const headingFont = "assets/fonts/heading.png";
	const char* const buttonFont = "assets/fonts/button.png";
	const char* const bodyFont = "assets/fonts/body.png";
	const char* const labelFont = "assets/fonts/label.png";
	const wchar_t check = L'\x7f';
	const rect<s32> outfitPrevious(744, 416, 784, 456);
	const rect<s32> outfitNext(1120, 416, 1160, 456);

	const s32 left = 64;
	const s32 right = 608;
	const s32 middle = (left + right) / 2;
	const s32 tabWidth = 160;
	const s32 settingsLeft = left + tabWidth + 24;

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	std::string wrapped(IGUIFont* font, const std::string& text, s32 room)
	{
		std::string result;
		std::string line;
		std::string::size_type at = 0;
		while (at < text.size())
		{
			std::string::size_type comma = text.find(", ", at);
			std::string::size_type end = comma == std::string::npos ? text.size() : comma + 2;
			std::string piece = text.substr(at, end - at);
			std::string longer = line + piece;
			while (!longer.empty() && longer.back() == ' ')
				longer.pop_back();
			if (!line.empty() && font && (s32)font->getDimension(wide(longer).c_str()).Width > room)
			{
				result += line + "\n";
				line = "      " + piece;
			}
			else
				line += piece;
			at = end;
		}
		return result + line;
	}

	std::string bonuses(const character::Profession& profession)
	{
		std::string text;
		for (const std::pair<int, int>& bonus : profession.skillBonuses)
			text += (text.empty() ? "" : ", ") + std::string(character::skillName(bonus.first)) + " +" + std::to_string(bonus.second);
		return text;
	}
}

MenuScreens::MenuScreens(IrrlichtDevice* device, Canvas& canvas, menu::MenuFlow& flow, gamecore::SoundEngine* sound,
	const std::deque<std::string>& status, MenuScene* backdrop)
	: device_(device), canvas_(canvas), flow_(flow), sound_(sound), status_(status),
	panel_(device->getGUIEnvironment(), canvas.screenSize()), built_(Screen::Title), hasBuilt_(false),
	gender_(character::Gender::Male), outfit_(0), timeMode_(menu::TimeMode::RealTime), backdrop_(backdrop), world_(nullptr), turning_(false), turnFromX_(0)
{
}

void MenuScreens::showCharacter()
{
	bool playing = built_ == Screen::WorldSelect || built_ == Screen::WorldSettings || built_ == Screen::JoinGame || built_ == Screen::Playing;
	bool wanted = built_ == Screen::CharacterCreation || built_ == Screen::SkillAllocation || (built_ == Screen::CharacterSelect && flow_.highlighted() >= 0) ||
		(playing && flow_.chosen() && !(world_ && built_ == Screen::Playing));
	if (!backdrop_)
		return;
	character::Gender gender = gender_;
	int outfit = outfit_;
	const character::CharacterSheet* shown = built_ == Screen::CharacterSelect && flow_.highlighted() >= 0
		? &flow_.characters()[flow_.highlighted()] : (playing ? flow_.chosen() : nullptr);
	if (shown)
	{
		gender = shown->gender();
		outfit = shown->outfit();
	}
	if (wanted)
		backdrop_->showCharacter(menu::characterModel(gender), menu::outfitSkin(gender, outfit).c_str());
	else
		backdrop_->hideCharacter();
}

void MenuScreens::drawNameplate()
{
	const character::CharacterSheet* chosen = flow_.chosen();
	position2d<s32> top;
	if (!chosen || !backdrop_ || !backdrop_->characterTop(top))
		return;
	FontHandle nameFont = canvas_.loadFont(buttonFont);
	FontHandle detailFont = canvas_.loadFont(bodyFont);
	canvas_.drawText(nameFont, wide(chosen->name()).c_str(), rect<s32>(top.X - 200, top.Y - 42, top.X + 200, top.Y - 18), theme::text, true, true);
	canvas_.drawText(detailFont, wide(chosen->profession().name).c_str(), rect<s32>(top.X - 200, top.Y - 18, top.X + 200, top.Y + 2),
		theme::muted, true, true);
}

bool MenuScreens::onScene(position2d<s32> point) const
{
	IGUIElement* root = device_->getGUIEnvironment()->getRootGUIElement();
	IGUIElement* under = root->getElementFromPoint(point);
	return !under || under == root || under->getType() == EGUIET_TAB;
}

bool MenuScreens::onEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT)
		return navigate(event.KeyInput);
	if (event.EventType == EET_MOUSE_INPUT_EVENT && event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN && built_ == Screen::Settings)
		for (const Button& button : buttons_)
			if (button.style == Style::Slider && button.area.isPointInside(position2d<s32>(event.MouseInput.X, event.MouseInput.Y)))
			{
				s32 width = button.area.getWidth();
				setVolume(button.id, ((event.MouseInput.X - button.area.UpperLeftCorner.X) * 100 + width / 2) / width);
				device_->getGUIEnvironment()->setFocus(device_->getGUIEnvironment()->getRootGUIElement()->getElementFromId(button.id, true));
				return true;
			}
	if (event.EventType != EET_MOUSE_INPUT_EVENT || !backdrop_ || !backdrop_->character())
	{
		turning_ = false;
		return false;
	}
	const SEvent::SMouseInput& mouse = event.MouseInput;
	switch (mouse.Event)
	{
	case EMIE_LMOUSE_PRESSED_DOWN:
		if (mouse.X < turnArea || !onScene(position2d<s32>(mouse.X, mouse.Y)))
			return false;
		turning_ = true;
		turnFromX_ = mouse.X;
		return true;
	case EMIE_MOUSE_MOVED:
		if (!turning_)
			return false;
		backdrop_->turnCharacter((f32)(turnFromX_ - mouse.X) * degreesPerPixel);
		turnFromX_ = mouse.X;
		return true;
	case EMIE_LMOUSE_LEFT_UP:
		if (!turning_)
			return false;
		turning_ = false;
		return true;
	default:
		return false;
	}
}

rect<s32> MenuScreens::place(const rect<s32>& area) const
{
	position2d<s32> offset = panel_.at(rect<s32>(0, 0, 0, 0)).UpperLeftCorner;
	return rect<s32>(area.UpperLeftCorner - offset, area.LowerRightCorner - offset);
}

void MenuScreens::addButton(s32 id, const rect<s32>& area, const std::string& label, Style style, const std::string& detail)
{
	panel_.button(place(area), id, "");
	buttons_.push_back(Button{ id, area, label, detail, style });
}

IGUIEditBox* MenuScreens::addTextBox(s32 id, const rect<s32>& area, const std::string& text, u32 maxLength, bool focus)
{
	IGUIEditBox* box = panel_.textBox(place(area), id, text, maxLength, focus);
	box->setOverrideFont(device_->getGUIEnvironment()->getFont(bodyFont));
	box->setOverrideColor(theme::text);
	box->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	return box;
}

void MenuScreens::header(const std::string& title)
{
	title_ = title;
	addButton(GUI_ID_BACK, rect<s32>(left, bottomTop, left + 120, bottomBottom), "Back", Style::Secondary);
}

void MenuScreens::hide()
{
	if (backdrop_)
		backdrop_->hideCharacter();
	turning_ = false;
	panel_.clear();
	buttons_.clear();
	hasBuilt_ = false;
}

void MenuScreens::build()
{
	bool wasSettings = built_ == Screen::Settings;
	hide();
	built_ = flow_.screen();
	hasBuilt_ = true;
	title_.clear();
	std::string playing = flow_.chosen() ? flow_.chosen()->name() : std::string();

	switch (built_)
	{
	case Screen::Title:
		addButton(GUI_ID_PLAY, rect<s32>(left, 456, left + 320, 504), "Play", Style::Primary);
		addButton(GUI_ID_SETTINGS, rect<s32>(left, 512, left + 320, 552), "Settings", Style::Secondary);
		addButton(GUI_ID_QUIT, rect<s32>(left, 560, left + 320, 600), "Quit", Style::Secondary);
		break;

	case Screen::Settings:
	{
		header("Settings");
		if (!wasSettings)
			tab_ = 0;
		const char* const tabs[4] = { "Audio", "Display", "Gameplay", "Controls" };
		for (s32 i = 0; i < 4; ++i)
			addButton(GUI_ID_TAB_FIRST + i, rect<s32>(left, contentY + i * 48, left + tabWidth, contentY + i * 48 + 40), tabs[i], Style::Choice);
		if (tab_ == 0)
			for (s32 i = 0; i < 3; ++i)
				addButton(GUI_ID_MASTER_VOLUME + i, rect<s32>(settingsLeft, contentY + 24 + i * 80, right, contentY + 52 + i * 80), "", Style::Slider);
		else if (tab_ == 1)
		{
			s32 half = (settingsLeft + right) / 2;
			addButton(GUI_ID_WINDOWED, rect<s32>(settingsLeft, contentY + 24, half - 4, contentY + 56), "Windowed", Style::Choice);
			addButton(GUI_ID_FULLSCREEN, rect<s32>(half + 4, contentY + 24, right, contentY + 56), "Fullscreen", Style::Choice);
			addButton(GUI_ID_RESOLUTION, rect<s32>(settingsLeft, contentY + 104, right, contentY + 136), "", Style::Choice);
		}
		else if (tab_ == 2)
		{
			const char* const modes[] = { "Real Time (1:1)", "Half Speed", "Fast" };
			for (s32 i = 0; i < 3; ++i)
				addButton(GUI_ID_DEFAULT_TIME_FIRST + i, rect<s32>(settingsLeft, contentY + 24 + i * 40, right, contentY + 56 + i * 40), modes[i], Style::Choice);
		}
		break;
	}

	case Screen::CharacterSelect:
	{
		header("Characters");
		std::vector<std::string> names;
		for (const character::CharacterSheet& sheet : flow_.characters())
			names.push_back(sheet.name() + "   -   " + sheet.profession().name);
		panel_.list(place(rect<s32>(left, contentY, right, 584)), GUI_ID_CHARACTERS, names, flow_.highlighted());
		addButton(GUI_ID_DELETE, rect<s32>(right - 88, 592, right, 616), "DELETE", Style::Link);
		markCharacters();
		addButton(GUI_ID_NEW_CHARACTER, rect<s32>(left + 128, bottomTop, left + 328, bottomBottom), "New Character", Style::Secondary);
		addButton(GUI_ID_SELECT, rect<s32>(left + 336, bottomTop, right, bottomBottom), "Select & Play", Style::Primary);
		break;
	}

	case Screen::CharacterCreation:
	{
		header("New Character");
		const menu::PendingCharacter* pending = flow_.pending();
		gender_ = pending ? pending->gender : character::Gender::Male;
		outfit_ = pending ? pending->outfit : 0;
		addTextBox(GUI_ID_NAME, rect<s32>(left, 118, formRight, 154), pending ? pending->name : "", menu::MAX_NAME_LENGTH, true);
		addButton(GUI_ID_MALE, rect<s32>(left, 184, (left + formRight) / 2 - 4, 216), "Male", Style::Choice);
		addButton(GUI_ID_FEMALE, rect<s32>((left + formRight) / 2 + 4, 184, formRight, 216), "Female", Style::Choice);
		std::vector<std::string> professions;
		for (const character::Profession& profession : flow_.professions())
			professions.push_back("  " + profession.name + "   -   " + bonuses(profession));
		IGUIListBox* list = panel_.list(place(rect<s32>(left, 244, formRight, 418)), GUI_ID_PROFESSION, professions, pending ? pending->profession : 0);
		IGUISkin* skin = device_->getGUIEnvironment()->getSkin();
		s32 room = list->getAbsolutePosition().getWidth() - skin->getSize(EGDS_SCROLLBAR_SIZE) - 2 * skin->getSize(EGDS_TEXT_DISTANCE_X);
		IGUIFont* font = device_->getGUIEnvironment()->getFont(bodyFont);
		s32 rowHeight = 30;
		for (u32 i = 0; i < list->getItemCount(); ++i)
		{
			std::string row = wrapped(font, professions[i], room);
			list->setItem(i, wide(row).c_str(), -1);
			if (font)
				rowHeight = std::max(rowHeight, (s32)font->getDimension(wide(row).c_str()).Height + 8);
		}
		list->setItemHeight(rowHeight);
		for (s32 i = 0; i < character::ATTRIBUTE_COUNT; ++i)
		{
			s32 y = statTop + i * statRow;
			addButton(GUI_ID_MINUS_FIRST + i, rect<s32>(left + 268, y + 2, left + 292, y + 26), "-", Style::Step);
			addButton(GUI_ID_PLUS_FIRST + i, rect<s32>(left + 300, y + 2, left + 324, y + 26), "+", Style::Step);
		}
		addButton(GUI_ID_REROLL, rect<s32>(left + 236, statTop + 5 * statRow + 2, left + 324, statTop + 5 * statRow + 26), "RE-ROLL", Style::Link);
		addButton(GUI_ID_OUTFIT_PREVIOUS, outfitPrevious, "<", Style::Choice);
		addButton(GUI_ID_OUTFIT_NEXT, outfitNext, ">", Style::Choice);
		addButton(GUI_ID_CREATE, rect<s32>(left + 128, bottomTop, formRight, bottomBottom), "Create Character", Style::Primary);
		break;
	}

	case Screen::SkillAllocation:
	{
		header("Skills");
		for (s32 i = 0; i < character::SKILL_COUNT; ++i)
		{
			s32 y = skillTop + i * skillRow;
			addButton(GUI_ID_SKILL_MINUS_FIRST + i, rect<s32>(left + 268, y + 4, left + 292, y + 28), "-", Style::Step);
			addButton(GUI_ID_SKILL_PLUS_FIRST + i, rect<s32>(left + 300, y + 4, left + 324, y + 28), "+", Style::Step);
		}
		addButton(GUI_ID_CONFIRM, rect<s32>(left + 128, bottomTop, formRight, bottomBottom), "Confirm", Style::Primary);
		break;
	}

	case Screen::WorldSelect:
	case Screen::WorldSettings:
	case Screen::JoinGame:
	{
		header("Host or Join");
		addButton(GUI_ID_HOST, rect<s32>(left, contentY, middle - 4, contentY + 64), "Host Game", Style::Card);
		addButton(GUI_ID_JOIN, rect<s32>(middle + 4, contentY, right, contentY + 64), "Join Game", Style::Card);
		bool joining = built_ == Screen::JoinGame;
		if (joining)
		{
			addTextBox(GUI_ID_ADDRESS, rect<s32>(left, 210, middle + 96, 246), "", 253, true);
			addTextBox(GUI_ID_PORT, rect<s32>(middle + 104, 210, right, 246), std::to_string(menu::DEFAULT_PORT), 5, false);
		}
		if (built_ == Screen::WorldSettings)
		{
			timeMode_ = flow_.preferences().time;
			addTextBox(GUI_ID_WORLD_NAME, rect<s32>(left, 210, right, 246), menu::DEFAULT_WORLD_NAME, 32, true);
			const char* const modes[] = { "Real Time (1:1)", "Half Speed", "Fast" };
			s32 width = (right - left - 16) / 3;
			for (s32 i = 0; i < 3; ++i)
				addButton(GUI_ID_TIME_FIRST + i, rect<s32>(left + i * (width + 8), 282, left + i * (width + 8) + width, 314), modes[i], Style::Choice);
			addTextBox(GUI_ID_MAX_PLAYERS, rect<s32>(left, 376, left + 120, 412), std::to_string(menu::DEFAULT_MAX_PLAYERS), 2, false);
		}
		if (built_ != Screen::WorldSelect)
			addButton(GUI_ID_START, rect<s32>(left + 128, bottomTop, right, bottomBottom), joining ? "Connect" : "Start Server", Style::Primary);
		break;
	}

	case Screen::Playing:
		title_ = playing;
		addButton(GUI_ID_LEAVE, rect<s32>(left, bottomTop, left + 200, bottomBottom), "Leave", Style::Secondary);
		break;

	default:
		break;
	}
	std::vector<IGUIElement*> elements = focusable();
	IGUIEnvironment* guienv = device_->getGUIEnvironment();
	if (keepFocus_ >= 0)
		if (IGUIElement* kept = guienv->getRootGUIElement()->getElementFromId(keepFocus_, true))
			guienv->setFocus(kept);
	keepFocus_ = -1;
	if (!elements.empty() && std::find(elements.begin(), elements.end(), guienv->getFocus()) == elements.end())
	{
		std::vector<rect<s32> > areas;
		for (IGUIElement* element : elements)
			areas.push_back(element->getAbsolutePosition());
		guienv->setFocus(elements[focus::first(areas)]);
	}
}

bool MenuScreens::isEnabled(s32 id) const
{
	const character::Aegis& draft = flow_.draft();
	if (id >= GUI_ID_PLUS_FIRST && id < GUI_ID_PLUS_FIRST + character::ATTRIBUTE_COUNT)
		return draft.unplaced() > 0;
	if (id >= GUI_ID_MINUS_FIRST && id < GUI_ID_MINUS_FIRST + character::ATTRIBUTE_COUNT)
		return draft.placed[id - GUI_ID_MINUS_FIRST] > 0;
	if (id >= GUI_ID_SKILL_PLUS_FIRST && id < GUI_ID_SKILL_PLUS_FIRST + character::SKILL_COUNT)
		return flow_.skillsUnplaced() > 0;
	if (id >= GUI_ID_SKILL_MINUS_FIRST && id < GUI_ID_SKILL_MINUS_FIRST + character::SKILL_COUNT)
		return flow_.skillPlaced(id - GUI_ID_SKILL_MINUS_FIRST) > 0;
	if (id == GUI_ID_OUTFIT_PREVIOUS || id == GUI_ID_OUTFIT_NEXT)
		return menu::outfitSkins(gender_).size() > 1;
	return true;
}

void MenuScreens::drawAegis()
{
	const character::Aegis& draft = flow_.draft();
	FontHandle bodyType = canvas_.loadFont(bodyFont);
	for (s32 i = 0; i < character::ATTRIBUTE_COUNT; ++i)
	{
		int attribute = i;
		s32 y = statTop + i * statRow;
		canvas_.drawText(bodyType, wide(character::attributeName(attribute)).c_str(), rect<s32>(left, y, left + 200, y + statRow),
			theme::muted, false, true);
		canvas_.drawText(bodyType, wide(std::to_string(draft.value(attribute))).c_str(), rect<s32>(left + 200, y, left + 256, y + statRow),
			theme::text, false, true);
	}
	s32 y = statTop + 5 * statRow;
	canvas_.drawText(bodyType, wide("Points: " + std::to_string(draft.unplaced())).c_str(), rect<s32>(left, y, left + 200, y + statRow),
		draft.unplaced() > 0 ? theme::accent : theme::muted, false, true);
	if (draft.hasBloodline)
		canvas_.drawText(bodyType, wide("Bloodline: " + draft.bloodline).c_str(), rect<s32>(left, y + statRow, formRight, y + 2 * statRow),
			theme::text, false, true);
}

void MenuScreens::drawSkills()
{
	FontHandle bodyType = canvas_.loadFont(bodyFont);
	for (s32 i = 0; i < character::SKILL_COUNT; ++i)
	{
		s32 y = skillTop + i * skillRow;
		canvas_.drawText(bodyType, wide(character::skillName(i)).c_str(), rect<s32>(left, y, left + 200, y + skillRow), theme::muted, false, true);
		canvas_.drawText(bodyType, wide(std::to_string(flow_.skillValue(i))).c_str(), rect<s32>(left + 200, y, left + 256, y + skillRow),
			theme::text, false, true);
	}
	s32 y = skillTop + character::SKILL_COUNT * skillRow + 8;
	canvas_.drawText(bodyType, wide("Points remaining: " + std::to_string(flow_.skillsUnplaced())).c_str(), rect<s32>(left, y, formRight, y + statRow),
		flow_.skillsUnplaced() > 0 ? theme::accent : theme::muted, false, true);
	std::string why = "Intelligence " + std::to_string(flow_.draft().value(character::INTELLIGENCE)) + " gives " +
		std::to_string(flow_.skillPool()) + " points";
	canvas_.drawText(bodyType, wide(why).c_str(), rect<s32>(left, y + statRow, formRight, y + 2 * statRow), theme::muted, false, true);
}

bool MenuScreens::isChosen(s32 id) const
{
	switch (id)
	{
	case GUI_ID_MALE: return gender_ == character::Gender::Male;
	case GUI_ID_FEMALE: return gender_ == character::Gender::Female;
	case GUI_ID_TIME_FIRST: return timeMode_ == menu::TimeMode::RealTime;
	case GUI_ID_TIME_FIRST + 1: return timeMode_ == menu::TimeMode::HalfSpeed;
	case GUI_ID_TIME_FIRST + 2: return timeMode_ == menu::TimeMode::Fast;
	case GUI_ID_HOST: return built_ == Screen::WorldSettings;
	case GUI_ID_WINDOWED: return !flow_.preferences().fullscreen;
	case GUI_ID_FULLSCREEN: return flow_.preferences().fullscreen;
	case GUI_ID_JOIN: return built_ == Screen::JoinGame;
	default:
		if (id >= GUI_ID_TAB_FIRST && id < GUI_ID_TAB_FIRST + 4)
			return tab_ == id - GUI_ID_TAB_FIRST;
		if (id >= GUI_ID_DEFAULT_TIME_FIRST && id < GUI_ID_DEFAULT_TIME_FIRST + 3)
			return (int)flow_.preferences().time == id - GUI_ID_DEFAULT_TIME_FIRST;
		return false;
	}
}

void MenuScreens::markCharacters()
{
	IGUIElement* element = device_->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUI_ID_CHARACTERS, true);
	if (!element)
		return;
	IGUIListBox* list = static_cast<IGUIListBox*>(element);
	IGUISkin* skin = device_->getGUIEnvironment()->getSkin();
	s32 room = list->getAbsolutePosition().getWidth() - skin->getSize(EGDS_SCROLLBAR_SIZE) - 2 * skin->getSize(EGDS_TEXT_DISTANCE_X);
	IGUIFont* font = device_->getGUIEnvironment()->getFont(bodyFont);
	s32 rowHeight = 36;
	for (u32 i = 0; i < list->getItemCount() && i < flow_.characters().size(); ++i)
	{
		const character::CharacterSheet& sheet = flow_.characters()[i];
		std::wstring line = (s32)i == flow_.highlighted() ? L"  " + std::wstring(1, check) + L"   " : std::wstring(L"        ");
		line += wide(sheet.name() + "   -   " + sheet.profession().name);
		if (font && (s32)font->getDimension(line.c_str()).Width > room)
			line = line.substr(0, line.size() - sheet.profession().name.size() - 3) + L"\n            " + wide(sheet.profession().name);
		list->setItem(i, line.c_str(), -1);
		if (font)
			rowHeight = std::max(rowHeight, (s32)font->getDimension(line.c_str()).Height + 8);
	}
	list->setItemHeight(rowHeight);
}

void MenuScreens::text(FontHandle font, const std::string& line, s32 x, s32 y, SColor colour)
{
	dimension2d<u32> size = canvas_.textSize(font, wide(line).c_str());
	canvas_.drawText(font, wide(line).c_str(), rect<s32>(x, y, x + (s32)size.Width, y + (s32)size.Height), colour);
}

void MenuScreens::label(const std::string& line, s32 x, s32 y)
{
	text(canvas_.loadFont(labelFont), line, x, y, theme::muted);
}

void MenuScreens::drawIcon(s32 id, position2d<s32> at, SColor colour)
{
	if (id == GUI_ID_HOST)
		for (s32 row = 0; row < 3; ++row)
		{
			rect<s32> unit(at.X, at.Y + row * 10, at.X + 28, at.Y + row * 10 + 7);
			canvas_.drawRectOutline(unit, colour);
			canvas_.drawRect(rect<s32>(unit.LowerRightCorner.X - 6, unit.UpperLeftCorner.Y + 3, unit.LowerRightCorner.X - 4, unit.UpperLeftCorner.Y + 5), colour);
		}
	else
	{
		canvas_.drawRectOutline(rect<s32>(at.X + 12, at.Y, at.X + 28, at.Y + 28), colour);
		canvas_.drawLine(position2d<s32>(at.X, at.Y + 14), position2d<s32>(at.X + 18, at.Y + 14), colour);
		canvas_.drawLine(position2d<s32>(at.X + 12, at.Y + 8), position2d<s32>(at.X + 18, at.Y + 14), colour);
		canvas_.drawLine(position2d<s32>(at.X + 12, at.Y + 20), position2d<s32>(at.X + 18, at.Y + 14), colour);
	}
}

void MenuScreens::drawButton(const Button& button)
{
	IGUIEnvironment* guienv = device_->getGUIEnvironment();
	IGUIElement* element = guienv->getRootGUIElement()->getElementFromId(button.id, true);
	bool hovered = element && guienv->getHovered() == element;
	bool pressed = element && static_cast<IGUIButton*>(element)->isPressed();
	bool chosen = isChosen(button.id);
	const rect<s32>& area = button.area;
	FontHandle font = canvas_.loadFont(buttonFont);
	dimension2d<u32> size = canvas_.textSize(font, wide(button.label).c_str());
	s32 textY = (area.UpperLeftCorner.Y + area.LowerRightCorner.Y - (s32)size.Height) / 2;

	switch (button.style)
	{
	case Style::Primary:
		canvas_.drawRect(area, pressed || hovered ? theme::accentFillHover : theme::accentFill);
		canvas_.drawRectOutline(area, hovered ? theme::accentBorderHover : theme::accentBorder);
		canvas_.drawRect(rect<s32>(area.UpperLeftCorner.X, area.UpperLeftCorner.Y, area.UpperLeftCorner.X + 3, area.LowerRightCorner.Y), theme::accent);
		text(font, button.label, area.UpperLeftCorner.X + 24, textY, theme::text);
		break;
	case Style::Secondary:
		canvas_.drawRect(area, theme::panel);
		canvas_.drawRectOutline(area, hovered ? theme::borderHover : theme::border);
		text(font, button.label, area.UpperLeftCorner.X + 24, textY, hovered ? theme::text : theme::muted);
		break;
	case Style::Link:
		canvas_.drawText(canvas_.loadFont(labelFont), wide(button.label).c_str(), area, hovered ? theme::text : theme::muted, false, true);
		break;
	case Style::Choice:
	{
		std::string shown = button.id == GUI_ID_RESOLUTION ? "<   " + std::to_string(flow_.preferences().width) + " x " +
			std::to_string(flow_.preferences().height) + "   >" : button.label;
		canvas_.drawRect(area, chosen ? theme::accentFill : theme::panel);
		canvas_.drawRectOutline(area, chosen ? theme::accentBorder : (hovered ? theme::borderHover : theme::border));
		canvas_.drawText(font, wide(shown).c_str(), area, chosen ? theme::text : (hovered ? theme::muted : theme::dim), true, true);
		break;
	}
	case Style::Slider:
	{
		s32 share = area.getWidth() * volume(button.id) / 100;
		canvas_.drawRect(area, theme::panel);
		if (share > 0)
			canvas_.drawRect(rect<s32>(area.UpperLeftCorner.X, area.UpperLeftCorner.Y, area.UpperLeftCorner.X + share, area.LowerRightCorner.Y),
				SColor(110, theme::accent.getRed(), theme::accent.getGreen(), theme::accent.getBlue()));
		canvas_.drawRectOutline(area, hovered ? theme::borderHover : theme::border);
		std::string value = std::to_string(volume(button.id));
		FontHandle small = canvas_.loadFont(labelFont);
		s32 width = (s32)canvas_.textSize(small, wide(value).c_str()).Width;
		text(small, value, area.LowerRightCorner.X - width, area.UpperLeftCorner.Y - 24, theme::text);
		break;
	}
	case Style::Step:
	{
		bool enabled = isEnabled(button.id);
		canvas_.drawText(font, wide(button.label).c_str(), area, enabled ? (hovered ? theme::accent : theme::muted) : theme::dim, true, true);
		break;
	}
	case Style::Card:
	{
		canvas_.drawRect(area, chosen ? theme::accentFill : theme::panel);
		canvas_.drawRectOutline(area, chosen ? theme::accentBorder : (hovered ? theme::borderHover : theme::border));
		SColor strong = chosen ? theme::text : (hovered ? theme::muted : theme::dim);
		drawIcon(button.id, position2d<s32>(area.UpperLeftCorner.X + 20, (area.UpperLeftCorner.Y + area.LowerRightCorner.Y) / 2 - 14),
			chosen ? theme::accent : strong);
		canvas_.drawText(font, wide(button.label).c_str(), rect<s32>(area.UpperLeftCorner.X + 68, area.UpperLeftCorner.Y,
			area.LowerRightCorner.X, area.LowerRightCorner.Y), strong, false, true);
		break;
	}
	}
}

void MenuScreens::draw()
{
	if (!hasBuilt_ || flow_.screen() != built_)
		build();

	IGUIEnvironment* guienv = device_->getGUIEnvironment();
	if (IGUIFont* font = guienv->getFont(bodyFont))
		guienv->getSkin()->setFont(font);

	dimension2d<u32> screen = canvas_.screenSize();
	if (world_ && built_ == Screen::Playing)
		world_->draw();
	else if (backdrop_)
	{
		backdrop_->draw();
		for (s32 x = 0; x < 720; x += 4)
			canvas_.drawRect(rect<s32>(x, 0, x + 4, (s32)screen.Height), SColor((u32)(180 - x / 4), 8, 7, 5));
	}
	else
		canvas_.drawGradientRect(rect<s32>(0, 0, (s32)screen.Width, (s32)screen.Height), theme::backgroundTop, theme::backgroundBottom);

	showCharacter();
	if (built_ == Screen::CharacterCreation)
		drawAegis();
	if (built_ == Screen::SkillAllocation)
		drawSkills();
	if (built_ == Screen::WorldSelect || built_ == Screen::WorldSettings || built_ == Screen::JoinGame || built_ == Screen::Playing)
		drawNameplate();

	if (built_ == Screen::Title)
	{
		text(canvas_.loadFont(titleFont), gameTitle, left - 4, 150, theme::text);
		canvas_.drawRect(rect<s32>(left, 232, left + 96, 236), theme::accent);
		text(canvas_.loadFont(bodyFont), tagline, left, 254, theme::muted);
	}
	else if (world_ && built_ == Screen::Playing)
	{
		drawVitals(canvas_, world_->sheet(), world_->weapon());
		world_->dpad().render(canvas_);
		position2d<s32> marker;
		if (world_->targetMarker(marker))
		{
			// A 56px gold ring with a thick bright rim, so the target reads from across the room.
			const s32 radius = 28;
			canvas_.drawImage(canvas_.loadTexture("assets/ui/icons/rings/gold.png"), artRect("rings/gold.png", marker, radius * 2));
			for (s32 r = radius - 3; r <= radius; ++r)
				for (int i = 0; i < 40; ++i)
				{
					f32 a = i * 2.0f * PI / 40;
					f32 b = (i + 1) * 2.0f * PI / 40;
					canvas_.drawLine(position2d<s32>(marker.X + (s32)(std::cos(a) * r), marker.Y + (s32)(std::sin(a) * r)),
						position2d<s32>(marker.X + (s32)(std::cos(b) * r), marker.Y + (s32)(std::sin(b) * r)), SColor(255, 255, 204, 72));
				}
			std::string icon = world_->detector().getTarget() ? world_->detector().getTarget()->getInfo().iconPath : "";
			if (!icon.empty())
				canvas_.drawImage(canvas_.loadTexture(("assets/ui/icons/" + icon).c_str()), artRect(icon, marker, 34));
			drawInteractPrompt(canvas_, position2d<s32>(marker.X, marker.Y + radius + 12), world_->promptKey(), world_->promptAction());
		}
		world_->contextPanel().render(canvas_);
		if (!world_->interactionFeedback().empty())
		{
			std::wstring said(world_->interactionFeedback().begin(), world_->interactionFeedback().end());
			s32 width = (s32)canvas_.textSize(canvas_.loadFont(labelFont), said.c_str()).Width;
			canvas_.drawRect(rect<s32>((s32)screen.Width / 2 - width / 2 - 8, (s32)screen.Height / 2 + 63, (s32)screen.Width / 2 + width / 2 + 8,
				(s32)screen.Height / 2 + 87), SColor(165, 0, 0, 0));
			canvas_.drawText(canvas_.loadFont(labelFont), said.c_str(), rect<s32>(0, (s32)screen.Height / 2 + 60, (s32)screen.Width,
				(s32)screen.Height / 2 + 90), world_->interactionSucceeded() ? SColor(255, 235, 230, 220) : theme::accent, true, true, false);
		}
		if (world_->debugShown())
			DebugOverlay(device_, canvas_, nullptr).draw(world_->player() ? world_->player()->getPosition() : vector3df());
		if (world_->dead())
			canvas_.drawText(canvas_.loadFont(titleFont), L"YOU DIED", rect<s32>(0, 0, (s32)screen.Width, (s32)screen.Height), theme::accent,
				true, true, false);
	}
	else if (!title_.empty())
		text(canvas_.loadFont(headingFont), title_, left - 2, headingY, theme::text);

	switch (built_)
	{
	case Screen::CharacterCreation:
	{
		label("NAME", left, 100);
		label("GENDER", left, 166);
		label("PROFESSION", left, 228);
		position2d<s32> top;
		if (backdrop_ && backdrop_->characterTop(top))
		{
			std::string line = "OUTFIT   " + std::to_string(outfit_ + 1) + " / " + std::to_string(menu::outfitSkins(gender_).size());
			dimension2d<u32> size = canvas_.textSize(canvas_.loadFont(labelFont), wide(line).c_str());
			label(line, top.X - (s32)size.Width / 2, top.Y - 24);
		}
		break;
	}
	case Screen::WorldSettings:
	{
		label("WORLD NAME", left, 192);
		label("TIME", left, 264);
		const char* const days[] = { "24 real hours per game day", "12 real hours per game day", "20 real minutes per game day" };
		text(canvas_.loadFont(bodyFont), days[(int)timeMode_], left, 322, theme::muted);
		label("MAX PLAYERS", left, 358);
		break;
	}
	case Screen::Settings:
		drawSettings();
		break;
	case Screen::JoinGame:
		label("SERVER ADDRESS", left, 192);
		label("PORT", middle + 104, 192);
		break;
	case Screen::Playing:
	{
		std::string where = flow_.session() == menu::Session::Host
			? "Hosting " + flow_.worldName() + " on port " + std::to_string(flow_.port())
			: "Joining " + flow_.address() + ":" + std::to_string(flow_.port());
		s32 y = world_ ? 152 : contentY;
		text(canvas_.loadFont(bodyFont), where, left, y, theme::muted);
		y += 32;
		for (const std::string& line : status_)
		{
			text(canvas_.loadFont(bodyFont), line, left, y, theme::text);
			y += 24;
		}
		break;
	}
	default:
		break;
	}

	guienv->drawAll();
	for (const Button& button : buttons_)
		drawButton(button);
	drawFocus();

	s32 below = 0;
	for (const Button& button : buttons_)
		below = std::max(below, button.area.LowerRightCorner.Y);
	if (!flow_.message().empty())
		text(canvas_.loadFont(bodyFont), flow_.message(), left, below + 10, theme::accent);
	if (built_ == Screen::Title)
		text(canvas_.loadFont(labelFont), "V0.1   EARLY DEVELOPMENT", left, (s32)screen.Height - 40, theme::dim);
}

std::vector<IGUIElement*> MenuScreens::focusable() const
{
	std::vector<IGUIElement*> elements;
	if (built_ == Screen::Playing)
		return elements;
	std::vector<IGUIElement*> open(1, device_->getGUIEnvironment()->getRootGUIElement());
	while (!open.empty())
	{
		IGUIElement* parent = open.back();
		open.pop_back();
		for (IGUIElement* element : parent->getChildren())
		{
			EGUI_ELEMENT_TYPE type = element->getType();
			if (type == EGUIET_BUTTON || type == EGUIET_EDIT_BOX || type == EGUIET_LIST_BOX)
				elements.push_back(element);
			else
				open.push_back(element);
		}
	}
	return elements;
}

bool MenuScreens::navigate(const SEvent::SKeyInput& key)
{
	std::vector<IGUIElement*> elements = focusable();
	if (!key.PressedDown || elements.empty())
		return false;
	IGUIEnvironment* guienv = device_->getGUIEnvironment();
	std::vector<rect<s32> > areas;
	int current = -1;
	for (size_t i = 0; i < elements.size(); ++i)
	{
		areas.push_back(elements[i]->getAbsolutePosition());
		if (elements[i] == guienv->getFocus())
			current = (int)i;
	}
	EGUI_ELEMENT_TYPE type = current >= 0 ? elements[current]->getType() : EGUIET_ELEMENT;
	s32 id = current >= 0 ? elements[current]->getID() : -1;
	if (key.Key == KEY_LEFT || key.Key == KEY_RIGHT)
	{
		int step = key.Key == KEY_LEFT ? -1 : 1;
		if (id >= GUI_ID_MASTER_VOLUME && id <= GUI_ID_SFX_VOLUME)
		{
			setVolume(id, volume(id) + 5 * step);
			return true;
		}
		if (id == GUI_ID_RESOLUTION)
		{
			cycleResolution(step);
			return true;
		}
	}
	int next = current;
	switch (key.Key)
	{
	case KEY_TAB:
		next = focus::step(areas, current, key.Shift);
		break;
	case KEY_LEFT:
	case KEY_RIGHT:
		if (type == EGUIET_EDIT_BOX)
			return false;
		next = focus::toward(areas, current, key.Key);
		break;
	case KEY_UP:
	case KEY_DOWN:
		if (type == EGUIET_LIST_BOX)
			return false;
		next = focus::toward(areas, current, key.Key);
		break;
	default:
		return false;
	}
	if (next >= 0)
		guienv->setFocus(elements[next]);
	return true;
}

int MenuScreens::volume(s32 id) const
{
	const menu::Preferences& preferences = flow_.preferences();
	return id == GUI_ID_MASTER_VOLUME ? preferences.masterVolume : id == GUI_ID_MUSIC_VOLUME ? preferences.musicVolume : preferences.sfxVolume;
}

void MenuScreens::setVolume(s32 id, int value)
{
	menu::Preferences preferences = flow_.preferences();
	value = std::max(0, std::min(100, value));
	(id == GUI_ID_MASTER_VOLUME ? preferences.masterVolume : id == GUI_ID_MUSIC_VOLUME ? preferences.musicVolume : preferences.sfxVolume) = value;
	flow_.setPreferences(preferences);
}

std::vector<dimension2d<u32> > MenuScreens::resolutions(const std::vector<dimension2d<u32> >& modes, dimension2d<u32> current)
{
	std::vector<dimension2d<u32> > choices(1, current);
	std::vector<dimension2d<u32> > offered = modes;
	offered.push_back(dimension2d<u32>(menu::Preferences::SMALLEST_WIDTH, menu::Preferences::SMALLEST_HEIGHT));
	for (const dimension2d<u32>& mode : offered)
		if ((s32)mode.Width >= menu::Preferences::SMALLEST_WIDTH && (s32)mode.Height >= menu::Preferences::SMALLEST_HEIGHT && std::find(choices.begin(), choices.end(), mode) == choices.end())
			choices.push_back(mode);
	std::sort(choices.begin(), choices.end(), [](const dimension2d<u32>& a, const dimension2d<u32>& b) {
		return a.Width != b.Width ? a.Width < b.Width : a.Height < b.Height;
	});
	return choices;
}

void MenuScreens::cycleResolution(int step)
{
	menu::Preferences preferences = flow_.preferences();
	std::vector<dimension2d<u32> > modes;
	if (IVideoModeList* list = device_->getVideoModeList())
		for (s32 i = 0; i < list->getVideoModeCount(); ++i)
			modes.push_back(list->getVideoModeResolution(i));
	dimension2d<u32> current((u32)preferences.width, (u32)preferences.height);
	std::vector<dimension2d<u32> > choices = resolutions(modes, current);
	int at = (int)(std::find(choices.begin(), choices.end(), current) - choices.begin());
	const dimension2d<u32>& next = choices[(at + step + (int)choices.size()) % (int)choices.size()];
	preferences.width = (int)next.Width;
	preferences.height = (int)next.Height;
	flow_.setPreferences(preferences);
}

void MenuScreens::drawSettings()
{
	if (tab_ == 0)
	{
		const char* const names[3] = { "MASTER VOLUME", "MUSIC VOLUME", "SFX VOLUME" };
		for (s32 i = 0; i < 3; ++i)
			label(names[i], settingsLeft, contentY + i * 80);
	}
	else if (tab_ == 1)
	{
		label("WINDOW", settingsLeft, contentY);
		label("RESOLUTION", settingsLeft, contentY + 80);
		text(canvas_.loadFont(bodyFont), "Applies the next time the game starts.", settingsLeft, contentY + 148, theme::muted);
	}
	else if (tab_ == 2)
		label("DEFAULT TIME FOR NEW WORLDS", settingsLeft, contentY);
	else
	{
		static const char* const keys[][2] = { { "WASD", "Move" }, { "Mouse", "Look" }, { "I", "Inventory" },
			{ "C", "Notebook / Character Sheet" }, { "E", "Search Container" }, { "R", "Rotate Item" }, { "F3", "Debug Overlay" },
			{ "Backtick", "Console" }, { "Escape", "Back / Close" }, { "Tab / Shift+Tab", "Navigate UI" }, { "Arrow Keys", "Navigate UI" },
			{ "Enter / Space", "Confirm" }, { "Gamepad A", "Confirm" }, { "Gamepad B", "Back" }, { "Gamepad X", "Rotate Item" },
			{ "Gamepad Start", "Inventory" }, { "Gamepad Back", "Notebook" }, { "Stick / D-pad", "Navigate" } };
		s32 y = contentY;
		for (const auto& row : keys)
		{
			text(canvas_.loadFont(labelFont), row[0], settingsLeft, y + 4, theme::muted);
			text(canvas_.loadFont(bodyFont), row[1], settingsLeft + 150, y, theme::text);
			y += 26;
		}
	}
}

void MenuScreens::drawFocus()
{
	IGUIElement* focused = device_->getGUIEnvironment()->getFocus();
	for (IGUIElement* element : focusable())
		if (element == focused)
		{
			rect<s32> area = element->getAbsolutePosition();
			canvas_.drawRectOutline(rect<s32>(area.UpperLeftCorner.X - 2, area.UpperLeftCorner.Y - 2, area.LowerRightCorner.X + 2,
				area.LowerRightCorner.Y + 2), theme::accent);
		}
}

void MenuScreens::click()
{
	if (!sound_)
		return;
	if (gamecore::Sound* played = sound_->play2D("assets/sounds/button-25.wav", false, false, true))
	{
		played->setVolume(flow_.preferences().sfxVolume / 100.0f);
		played->drop();
	}
}

bool MenuScreens::clicked(s32 id)
{
	switch (id)
	{
	case GUI_ID_PLAY: flow_.play(); break;
	case GUI_ID_SETTINGS: flow_.settings(); break;
	case GUI_ID_QUIT: flow_.quit(); break;
	case GUI_ID_BACK:
		if (flow_.screen() == Screen::JoinGame || flow_.screen() == Screen::WorldSettings)
			flow_.back();
		flow_.back();
		break;
	case GUI_ID_NEW_CHARACTER: flow_.newCharacter(); break;
	case GUI_ID_SELECT:
		flow_.highlight(panel_.selected(GUI_ID_CHARACTERS));
		flow_.select();
		break;
	case GUI_ID_DELETE:
		flow_.highlight(panel_.selected(GUI_ID_CHARACTERS));
		if (flow_.deleteHighlighted())
			hasBuilt_ = false;
		break;
	case GUI_ID_MALE: gender_ = character::Gender::Male; outfit_ = 0; break;
	case GUI_ID_FEMALE: gender_ = character::Gender::Female; outfit_ = 0; break;
	case GUI_ID_OUTFIT_PREVIOUS:
	case GUI_ID_OUTFIT_NEXT:
	{
		int count = (int)menu::outfitSkins(gender_).size();
		outfit_ = (outfit_ + (id == GUI_ID_OUTFIT_NEXT ? 1 : count - 1)) % count;
		break;
	}
	case GUI_ID_NAME:
	case GUI_ID_CREATE:
		flow_.toSkills(panel_.text(GUI_ID_NAME), gender_, panel_.selected(GUI_ID_PROFESSION), outfit_);
		break;
	case GUI_ID_CONFIRM: flow_.confirmSkills(); break;
	case GUI_ID_REROLL: flow_.reroll(); break;
	case GUI_ID_HOST:
		if (flow_.screen() == Screen::JoinGame)
			flow_.back();
		flow_.host();
		break;
	case GUI_ID_JOIN:
		if (flow_.screen() == Screen::WorldSettings)
			flow_.back();
		flow_.join();
		break;
	case GUI_ID_WORLD_NAME:
	case GUI_ID_MAX_PLAYERS:
		flow_.startServer(panel_.text(GUI_ID_WORLD_NAME), timeMode_, panel_.text(GUI_ID_MAX_PLAYERS));
		break;
	case GUI_ID_ADDRESS:
	case GUI_ID_PORT:
	case GUI_ID_START:
		if (flow_.screen() == Screen::WorldSettings)
			flow_.startServer(panel_.text(GUI_ID_WORLD_NAME), timeMode_, panel_.text(GUI_ID_MAX_PLAYERS));
		else
			flow_.connect(panel_.text(GUI_ID_ADDRESS), panel_.text(GUI_ID_PORT));
		break;
	case GUI_ID_LEAVE: flow_.returnToTitle(); break;
	case GUI_ID_MASTER_VOLUME:
	case GUI_ID_MUSIC_VOLUME:
	case GUI_ID_SFX_VOLUME:
		return true;
	case GUI_ID_WINDOWED:
	case GUI_ID_FULLSCREEN:
	{
		menu::Preferences preferences = flow_.preferences();
		preferences.fullscreen = id == GUI_ID_FULLSCREEN;
		flow_.setPreferences(preferences);
		break;
	}
	case GUI_ID_RESOLUTION: cycleResolution(1); break;
	default:
		if (id >= GUI_ID_MINUS_FIRST && id < GUI_ID_MINUS_FIRST + character::ATTRIBUTE_COUNT)
			flow_.adjust(id - GUI_ID_MINUS_FIRST, -1);
		else if (id >= GUI_ID_PLUS_FIRST && id < GUI_ID_PLUS_FIRST + character::ATTRIBUTE_COUNT)
			flow_.adjust(id - GUI_ID_PLUS_FIRST, 1);
		else if (id >= GUI_ID_SKILL_MINUS_FIRST && id < GUI_ID_SKILL_MINUS_FIRST + character::SKILL_COUNT)
			flow_.adjustSkill(id - GUI_ID_SKILL_MINUS_FIRST, -1);
		else if (id >= GUI_ID_SKILL_PLUS_FIRST && id < GUI_ID_SKILL_PLUS_FIRST + character::SKILL_COUNT)
			flow_.adjustSkill(id - GUI_ID_SKILL_PLUS_FIRST, 1);
		else if (id >= GUI_ID_TIME_FIRST && id < GUI_ID_TIME_FIRST + 3)
			timeMode_ = (menu::TimeMode)(id - GUI_ID_TIME_FIRST);
		else if (id >= GUI_ID_TAB_FIRST && id < GUI_ID_TAB_FIRST + 4)
		{
			tab_ = id - GUI_ID_TAB_FIRST;
			keepFocus_ = id;
			hasBuilt_ = false;
		}
		else if (id >= GUI_ID_DEFAULT_TIME_FIRST && id < GUI_ID_DEFAULT_TIME_FIRST + 3)
		{
			menu::Preferences preferences = flow_.preferences();
			preferences.time = (menu::TimeMode)(id - GUI_ID_DEFAULT_TIME_FIRST);
			flow_.setPreferences(preferences);
		}
		else
			return false;
	}
	click();
	return true;
}

void MenuScreens::back()
{
	if (flow_.screen() != Screen::Title && flow_.screen() != Screen::Playing)
		clicked(GUI_ID_BACK);
}

bool MenuScreens::onGuiEvent(const SEvent& event)
{
	switch (event.GUIEvent.EventType)
	{
	case EGET_BUTTON_CLICKED:
	case EGET_EDITBOX_ENTER:
		return clicked(event.GUIEvent.Caller ? event.GUIEvent.Caller->getID() : -1);
	case EGET_LISTBOX_CHANGED:
		if (event.GUIEvent.Caller && event.GUIEvent.Caller->getID() == GUI_ID_CHARACTERS)
		{
			flow_.highlight(panel_.selected(GUI_ID_CHARACTERS));
			markCharacters();
		}
		return true;
	case EGET_LISTBOX_SELECTED_AGAIN:
		return event.GUIEvent.Caller && event.GUIEvent.Caller->getID() == GUI_ID_CHARACTERS ? clicked(GUI_ID_SELECT) : true;
	default:
		return false;
	}
}
