#include "MenuScreens.h"
#include "Outfits.h"
#include "World.h"
#include "Hud.h"
#include "MenuScene.h"
#include "SoundEngine.h"
#include "Theme.h"
#include "UiAnchor.h"
#include <algorithm>

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

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	std::string fitted(IGUIFont* font, const std::string& text, s32 room)
	{
		if (!font || (s32)font->getDimension(wide(text).c_str()).Width <= room)
			return text;
		std::string cut = text;
		while (!cut.empty() && (s32)font->getDimension(wide(cut + "...").c_str()).Width > room)
			cut.pop_back();
		while (!cut.empty() && (cut.back() == ' ' || cut.back() == ','))
			cut.pop_back();
		return cut + "...";
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
	gender_(character::Gender::Male), outfit_(0), backdrop_(backdrop), world_(nullptr), turning_(false), turnFromX_(0)
{
}

void MenuScreens::showCharacter()
{
	bool playing = built_ == Screen::WorldSelect || built_ == Screen::JoinGame || built_ == Screen::Playing;
	bool wanted = built_ == Screen::CharacterCreation || (built_ == Screen::CharacterSelect && flow_.highlighted() >= 0) ||
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
		header("Settings");
		break;

	case Screen::CharacterSelect:
	{
		header("Characters");
		std::vector<std::string> names;
		for (const character::CharacterSheet& sheet : flow_.characters())
			names.push_back(sheet.name() + "   -   " + sheet.profession().name);
		IGUIListBox* list = panel_.list(place(rect<s32>(left, contentY, right, 612)), GUI_ID_CHARACTERS, names, flow_.highlighted());
		list->setItemHeight(36);
		markCharacters();
		addButton(GUI_ID_NEW_CHARACTER, rect<s32>(left + 128, bottomTop, left + 328, bottomBottom), "New Character", Style::Secondary);
		addButton(GUI_ID_SELECT, rect<s32>(left + 336, bottomTop, right, bottomBottom), "Select & Play", Style::Primary);
		break;
	}

	case Screen::CharacterCreation:
	{
		header("New Character");
		gender_ = character::Gender::Male;
		outfit_ = 0;
		addTextBox(GUI_ID_NAME, rect<s32>(left, 118, formRight, 154), "", menu::MAX_NAME_LENGTH, true);
		addButton(GUI_ID_MALE, rect<s32>(left, 184, (left + formRight) / 2 - 4, 216), "Male", Style::Choice);
		addButton(GUI_ID_FEMALE, rect<s32>((left + formRight) / 2 + 4, 184, formRight, 216), "Female", Style::Choice);
		std::vector<std::string> professions;
		for (const character::Profession& profession : flow_.professions())
			professions.push_back("  " + profession.name + "   -   " + bonuses(profession));
		IGUIListBox* list = panel_.list(place(rect<s32>(left, 244, formRight, 418)), GUI_ID_PROFESSION, professions, 0);
		list->setItemHeight(30);
		IGUISkin* skin = device_->getGUIEnvironment()->getSkin();
		s32 room = list->getAbsolutePosition().getWidth() - skin->getSize(EGDS_SCROLLBAR_SIZE) - 2 * skin->getSize(EGDS_TEXT_DISTANCE_X);
		IGUIFont* font = device_->getGUIEnvironment()->getFont(bodyFont);
		for (u32 i = 0; i < list->getItemCount(); ++i)
			list->setItem(i, wide(fitted(font, professions[i], room)).c_str(), -1);
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

	case Screen::WorldSelect:
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
}

bool MenuScreens::isEnabled(s32 id) const
{
	const character::Aegis& draft = flow_.draft();
	if (id >= GUI_ID_PLUS_FIRST && id < GUI_ID_PLUS_FIRST + character::ATTRIBUTE_COUNT)
		return draft.unplaced() > 0;
	if (id >= GUI_ID_MINUS_FIRST && id < GUI_ID_MINUS_FIRST + character::ATTRIBUTE_COUNT)
		return draft.placed[id - GUI_ID_MINUS_FIRST] > 0;
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

bool MenuScreens::isChosen(s32 id) const
{
	switch (id)
	{
	case GUI_ID_MALE: return gender_ == character::Gender::Male;
	case GUI_ID_FEMALE: return gender_ == character::Gender::Female;
	case GUI_ID_HOST: return built_ == Screen::WorldSelect;
	case GUI_ID_JOIN: return built_ == Screen::JoinGame;
	default: return false;
	}
}

void MenuScreens::markCharacters()
{
	IGUIElement* element = device_->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUI_ID_CHARACTERS, true);
	if (!element)
		return;
	IGUIListBox* list = static_cast<IGUIListBox*>(element);
	for (u32 i = 0; i < list->getItemCount() && i < flow_.characters().size(); ++i)
	{
		const character::CharacterSheet& sheet = flow_.characters()[i];
		std::wstring line = (s32)i == flow_.highlighted() ? L"  " + std::wstring(1, check) + L"   " : std::wstring(L"        ");
		line += wide(sheet.name() + "   -   " + sheet.profession().name);
		list->setItem(i, line.c_str(), -1);
	}
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
		canvas_.drawRect(area, chosen ? theme::accentFill : theme::panel);
		canvas_.drawRectOutline(area, chosen ? theme::accentBorder : (hovered ? theme::borderHover : theme::border));
		canvas_.drawText(font, wide(button.label).c_str(), area, chosen ? theme::text : (hovered ? theme::muted : theme::dim), true, true);
		break;
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
	if (built_ == Screen::WorldSelect || built_ == Screen::JoinGame || built_ == Screen::Playing)
		drawNameplate();

	if (built_ == Screen::Title)
	{
		text(canvas_.loadFont(titleFont), gameTitle, left - 4, 150, theme::text);
		canvas_.drawRect(rect<s32>(left, 232, left + 96, 236), theme::accent);
		text(canvas_.loadFont(bodyFont), tagline, left, 254, theme::muted);
	}
	else if (world_ && built_ == Screen::Playing)
	{
		drawVitals(canvas_, world_->sheet());
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
	case Screen::JoinGame:
		label("SERVER ADDRESS", left, 192);
		label("PORT", middle + 104, 192);
		break;
	case Screen::Playing:
	{
		std::string where = flow_.session() == menu::Session::Host
			? "Hosting on port " + std::to_string(flow_.port())
			: "Joining " + flow_.address() + ":" + std::to_string(flow_.port());
		text(canvas_.loadFont(bodyFont), where, left, contentY, theme::muted);
		s32 y = contentY + 32;
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

	s32 below = 0;
	for (const Button& button : buttons_)
		below = std::max(below, button.area.LowerRightCorner.Y);
	if (!flow_.message().empty())
		text(canvas_.loadFont(bodyFont), flow_.message(), left, below + 10, theme::accent);
	if (built_ == Screen::Title)
		text(canvas_.loadFont(labelFont), "V0.1   EARLY DEVELOPMENT", left, (s32)screen.Height - 40, theme::dim);
}

void MenuScreens::click()
{
	if (sound_)
		sound_->play2D("assets/sounds/button-25.wav", false);
}

bool MenuScreens::clicked(s32 id)
{
	switch (id)
	{
	case GUI_ID_PLAY: flow_.play(); break;
	case GUI_ID_SETTINGS: flow_.settings(); break;
	case GUI_ID_QUIT: flow_.quit(); break;
	case GUI_ID_BACK:
		if (flow_.screen() == Screen::JoinGame)
			flow_.back();
		flow_.back();
		break;
	case GUI_ID_NEW_CHARACTER: flow_.newCharacter(); break;
	case GUI_ID_SELECT:
		flow_.highlight(panel_.selected(GUI_ID_CHARACTERS));
		flow_.select();
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
		flow_.create(panel_.text(GUI_ID_NAME), gender_, panel_.selected(GUI_ID_PROFESSION), outfit_);
		break;
	case GUI_ID_REROLL: flow_.reroll(); break;
	case GUI_ID_HOST:
		if (flow_.screen() == Screen::JoinGame)
			flow_.back();
		break;
	case GUI_ID_JOIN: flow_.join(); break;
	case GUI_ID_ADDRESS:
	case GUI_ID_PORT:
	case GUI_ID_START:
		if (flow_.screen() == Screen::WorldSelect)
			flow_.host();
		else
			flow_.connect(panel_.text(GUI_ID_ADDRESS), panel_.text(GUI_ID_PORT));
		break;
	case GUI_ID_LEAVE: flow_.returnToTitle(); break;
	default:
		if (id >= GUI_ID_MINUS_FIRST && id < GUI_ID_MINUS_FIRST + character::ATTRIBUTE_COUNT)
			flow_.adjust(id - GUI_ID_MINUS_FIRST, -1);
		else if (id >= GUI_ID_PLUS_FIRST && id < GUI_ID_PLUS_FIRST + character::ATTRIBUTE_COUNT)
			flow_.adjust(id - GUI_ID_PLUS_FIRST, 1);
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
