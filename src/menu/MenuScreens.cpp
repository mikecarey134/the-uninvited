#include "MenuScreens.h"
#include "SoundEngine.h"
#include "Theme.h"
#include "UiAnchor.h"
#include <algorithm>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;
using menu::Screen;
using namespace scene;

namespace
{
	const char* const gameTitle = "GAME NAME";
	const char* const previewModel = "assets/models/stick_mike.ms3d";
	const rect<s32> previewArea(864, 64, 1216, 656);
	const s32 aegisLeft = 480;
	const s32 aegisRight = 832;
	const s32 formRight = 448;
	const char* const tagline = "The dead don't rest. Neither can you.";
	const char* const titleFont = "assets/fonts/title.png";
	const char* const headingFont = "assets/fonts/heading.png";
	const char* const buttonFont = "assets/fonts/button.png";
	const char* const bodyFont = "assets/fonts/body.png";
	const char* const labelFont = "assets/fonts/label.png";
	const wchar_t check = L'\x7f';

	const s32 left = 64;
	const s32 right = 608;
	const s32 middle = (left + right) / 2;

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	std::string bonuses(const character::Profession& profession)
	{
		std::string text;
		for (const std::pair<character::Skill, int>& bonus : profession.skillBonuses)
			text += (text.empty() ? "" : ", ") + std::string(character::skillName(bonus.first)) + " +" + std::to_string(bonus.second);
		return text;
	}
}

MenuScreens::MenuScreens(IrrlichtDevice* device, Canvas& canvas, menu::MenuFlow& flow, gamecore::SoundEngine* sound,
	const std::deque<std::string>& status)
	: device_(device), canvas_(canvas), flow_(flow), sound_(sound), status_(status),
	panel_(device->getGUIEnvironment(), canvas.screenSize()), built_(Screen::Title), hasBuilt_(false),
	gender_(character::Gender::Male), model_(0), camera_(0), light_(0)
{
}

void MenuScreens::showPreview()
{
	scene::ISceneManager* smgr = device_->getSceneManager();
	scene::IAnimatedMesh* mesh = smgr->getMesh(previewModel);
	if (!mesh)
		return;
	model_ = smgr->addAnimatedMeshSceneNode(mesh);
	model_->setFrameLoop(206, 250);
	model_->setAnimationSpeed(12.0f);
	aabbox3df box = mesh->getBoundingBox();
	vector3df centre = box.getCenter();
	f32 height = box.getExtent().Y;
	vector3df target = centre + vector3df(0.0f, height * 0.24f, 0.0f);
	camera_ = smgr->addCameraSceneNode(0, target + vector3df(0.0f, height * 0.12f, -height * 1.3f), target);
	smgr->setActiveCamera(camera_);
	smgr->setAmbientLight(SColorf(0.35f, 0.3f, 0.26f));
	light_ = smgr->addLightSceneNode(0, centre + vector3df(-height, height, -height * 1.5f), SColorf(1.0f, 0.78f, 0.55f), height * 6.0f);
}

void MenuScreens::hidePreview()
{
	for (scene::ISceneNode* node : { (scene::ISceneNode*)model_, (scene::ISceneNode*)camera_, (scene::ISceneNode*)light_ })
		if (node)
			node->remove();
	model_ = 0;
	camera_ = 0;
	light_ = 0;
}

void MenuScreens::drawPreview()
{
	canvas_.drawRect(previewArea, theme::panel);
	canvas_.drawRectOutline(previewArea, theme::border);
	if (model_)
	{
		video::IVideoDriver* driver = device_->getVideoDriver();
		dimension2d<u32> screen = canvas_.screenSize();
		rect<s32> view(previewArea.UpperLeftCorner.X + 1, previewArea.UpperLeftCorner.Y + 1, previewArea.LowerRightCorner.X - 1,
			previewArea.LowerRightCorner.Y - 120);
		model_->setRotation(vector3df(0.0f, (f32)(device_->getTimer()->getTime() % 18000) * 0.02f, 0.0f));
		camera_->setAspectRatio((f32)view.getWidth() / (f32)view.getHeight());
		driver->setViewPort(view);
		device_->getSceneManager()->drawAll();
		driver->setViewPort(rect<s32>(0, 0, (s32)screen.Width, (s32)screen.Height));
	}
	s32 x = previewArea.UpperLeftCorner.X + 32;
	s32 floor = previewArea.LowerRightCorner.Y - 120;
	canvas_.drawRect(rect<s32>(x, floor, previewArea.LowerRightCorner.X - 32, floor + 1), theme::border);
	canvas_.drawRect(rect<s32>(x, floor, x + 48, floor + 2), theme::accent);

	std::string name = panel_.text(GUI_ID_NAME);
	std::string shown = name.empty() ? "Unnamed" : name;
	FontHandle nameFont = canvas_.loadFont(headingFont);
	if ((s32)canvas_.textSize(nameFont, wide(shown).c_str()).Width > previewArea.getWidth() - 64)
		nameFont = canvas_.loadFont(buttonFont);
	text(nameFont, shown, x - 2, floor + 18, name.empty() ? theme::dim : theme::text);
	s32 chosen = panel_.selected(GUI_ID_PROFESSION);
	if (chosen >= 0 && chosen < (s32)flow_.professions().size())
	{
		const character::Profession& profession = flow_.professions()[chosen];
		std::string who = std::string(gender_ == character::Gender::Female ? "Female" : "Male") + "   " + profession.name;
		text(canvas_.loadFont(bodyFont), who, x, floor + 66, theme::muted);
		text(canvas_.loadFont(bodyFont), bonuses(profession), x, floor + 90, theme::accent);
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

void MenuScreens::header(const std::string& title, const std::string& subtitle)
{
	addButton(GUI_ID_BACK, rect<s32>(left - 12, 40, left + 100, 72), "<  BACK", Style::Back);
	title_ = title;
	subtitle_ = subtitle;
}

void MenuScreens::hide()
{
	hidePreview();
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
	subtitle_.clear();
	std::string playing = flow_.chosen() ? flow_.chosen()->name() : std::string();

	switch (built_)
	{
	case Screen::Title:
		addButton(GUI_ID_PLAY, rect<s32>(left, 448, left + 320, 500), "Play", Style::Primary);
		addButton(GUI_ID_SETTINGS, rect<s32>(left, 512, left + 320, 556), "Settings", Style::Secondary);
		addButton(GUI_ID_QUIT, rect<s32>(left, 568, left + 320, 612), "Quit", Style::Secondary);
		break;

	case Screen::Settings:
		header("Settings", "Nothing to set yet.");
		break;

	case Screen::CharacterSelect:
	{
		header("Characters", "Choose who you will survive as.");
		std::vector<std::string> names;
		for (const character::CharacterSheet& sheet : flow_.characters())
			names.push_back(sheet.name() + "   -   " + sheet.profession().name);
		IGUIListBox* list = panel_.list(place(rect<s32>(left, 196, right, 560)), GUI_ID_CHARACTERS, names, flow_.highlighted());
		list->setItemHeight(48);
		markCharacters();
		addButton(GUI_ID_NEW_CHARACTER, rect<s32>(left, 584, middle - 8, 632), "New Character", Style::Secondary);
		addButton(GUI_ID_SELECT, rect<s32>(middle + 8, 584, right, 632), "Select & Play", Style::Primary);
		break;
	}

	case Screen::CharacterCreation:
	{
		header("New Character", "Your name, and the life you had before.");
		gender_ = character::Gender::Male;
		addTextBox(GUI_ID_NAME, rect<s32>(left, 204, formRight, 248), "", menu::MAX_NAME_LENGTH, true);
		addButton(GUI_ID_MALE, rect<s32>(left, 292, (left + formRight) / 2 - 8, 332), "Male", Style::Choice);
		addButton(GUI_ID_FEMALE, rect<s32>((left + formRight) / 2 + 8, 292, formRight, 332), "Female", Style::Choice);
		std::vector<std::string> professions;
		for (const character::Profession& profession : flow_.professions())
			professions.push_back("  " + profession.name + "   -   " + bonuses(profession));
		IGUIListBox* list = panel_.list(place(rect<s32>(left, 376, formRight, 600)), GUI_ID_PROFESSION, professions, 0);
		list->setItemHeight(34);
		addButton(GUI_ID_CREATE, rect<s32>(left, 620, formRight, 668), "Create Character", Style::Primary);
		for (s32 i = 0; i < character::ATTRIBUTE_COUNT; ++i)
		{
			s32 y = 204 + i * 50;
			addButton(GUI_ID_MINUS_FIRST + i, rect<s32>(aegisRight - 120, y + 6, aegisRight - 88, y + 38), "-", Style::Step);
			addButton(GUI_ID_PLUS_FIRST + i, rect<s32>(aegisRight - 48, y + 6, aegisRight - 16, y + 38), "+", Style::Step);
		}
		addButton(GUI_ID_REROLL, rect<s32>(aegisLeft, 532, aegisRight, 572), "Re-roll AEGIS", Style::Secondary);
		showPreview();
		break;
	}

	case Screen::WorldSelect:
	case Screen::JoinGame:
	{
		header("Host or Join", "Playing as " + playing);
		addButton(GUI_ID_HOST, rect<s32>(left, 184, middle - 8, 320), "Host Game", Style::Card, "Start a server on this machine");
		addButton(GUI_ID_JOIN, rect<s32>(middle + 8, 184, right, 320), "Join Game", Style::Card, "Connect to a friend's server");
		bool joining = built_ == Screen::JoinGame;
		if (joining)
		{
			addTextBox(GUI_ID_ADDRESS, rect<s32>(left, 368, middle + 96, 412), "", 253, true);
			addTextBox(GUI_ID_PORT, rect<s32>(middle + 112, 368, right, 412), std::to_string(menu::DEFAULT_PORT), 5, false);
		}
		addButton(GUI_ID_START, rect<s32>(left, 600, right, 648), joining ? "Connect" : "Start Server", Style::Primary);
		break;
	}

	case Screen::Playing:
		title_ = playing;
		addButton(GUI_ID_LEAVE, rect<s32>(left, 600, left + 260, 648), "Leave", Style::Secondary);
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
	return true;
}

void MenuScreens::drawAegis()
{
	const character::Aegis& draft = flow_.draft();
	label("AEGIS", aegisLeft, 186);
	FontHandle buttonType = canvas_.loadFont(buttonFont);
	FontHandle bodyType = canvas_.loadFont(bodyFont);
	for (s32 i = 0; i < character::ATTRIBUTE_COUNT; ++i)
	{
		character::Attribute attribute = (character::Attribute)i;
		bool blessed = draft.hasBloodline && draft.bloodlineAttribute == attribute;
		s32 y = 204 + i * 50;
		rect<s32> row(aegisLeft, y, aegisRight, y + 44);
		canvas_.drawRect(row, blessed ? theme::accentFill : theme::panel);
		canvas_.drawRectOutline(row, blessed ? theme::accentBorder : theme::border);
		text(buttonType, character::attributeName(attribute), aegisLeft + 16, y + 10, theme::text);
		std::string breakdown = std::to_string(draft.rolled[i]);
		if (draft.placed[i] > 0)
			breakdown += " + " + std::to_string(draft.placed[i]);
		if (blessed)
			breakdown += "  x" + std::to_string(character::Aegis::BLOODLINE_MULTIPLIER);
		text(bodyType, breakdown, aegisLeft + 150, y + 12, blessed ? theme::accent : theme::muted);
		canvas_.drawText(buttonType, wide(std::to_string(draft.value(attribute))).c_str(),
			rect<s32>(aegisRight - 88, y, aegisRight - 48, y + 44), draft.placed[i] > 0 || blessed ? theme::text : theme::muted, true, true);
	}

	label("POINTS TO PLACE", aegisLeft, 462);
	std::string left = std::to_string(draft.unplaced()) + " of " + std::to_string(draft.pool());
	text(buttonType, left, aegisLeft, 478, draft.unplaced() > 0 ? theme::accent : theme::text);
	s32 bonus = draft.pool() - character::Aegis::BASE_POOL;
	text(bodyType, std::to_string(character::Aegis::BASE_POOL) + " base + " + std::to_string(bonus) + " from Intelligence " +
		std::to_string(draft.rolled[(int)character::Attribute::Intelligence]), aegisLeft + 96, 482, theme::muted);

	rect<s32> slot(aegisLeft, 588, aegisRight, 668);
	if (draft.hasBloodline)
	{
		canvas_.drawRect(slot, theme::accentFill);
		canvas_.drawRectOutline(slot, theme::accentBorderHover);
		canvas_.drawRect(rect<s32>(slot.UpperLeftCorner.X, slot.UpperLeftCorner.Y, slot.UpperLeftCorner.X + 3, slot.LowerRightCorner.Y), theme::accent);
		text(canvas_.loadFont(labelFont), "BLOODLINE", aegisLeft + 20, 600, theme::accent);
		text(buttonType, draft.bloodline, aegisLeft + 20, 616, theme::text);
		text(bodyType, std::string(character::attributeName(draft.bloodlineAttribute)) + " x" +
			std::to_string(character::Aegis::BLOODLINE_MULTIPLIER), aegisLeft + 20, 642, theme::muted);
	}
	else
	{
		canvas_.drawRect(slot, theme::panel);
		canvas_.drawRectOutline(slot, theme::border);
		text(canvas_.loadFont(labelFont), "BLOODLINE", aegisLeft + 20, 600, theme::dim);
		text(bodyType, "None. About 1 in " + std::to_string(character::Aegis::BLOODLINE_ONE_IN) + " are born with one.",
			aegisLeft + 20, 624, theme::dim);
	}
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
	case Style::Back:
		font = canvas_.loadFont(labelFont);
		size = canvas_.textSize(font, wide(button.label).c_str());
		text(font, button.label, area.UpperLeftCorner.X + 12, (area.UpperLeftCorner.Y + area.LowerRightCorner.Y - (s32)size.Height) / 2,
			hovered ? theme::text : theme::muted);
		break;
	case Style::Choice:
		canvas_.drawRect(area, chosen ? theme::accentFill : theme::panel);
		canvas_.drawRectOutline(area, chosen ? theme::accentBorder : (hovered ? theme::borderHover : theme::border));
		canvas_.drawText(font, wide(button.label).c_str(), area, chosen ? theme::text : (hovered ? theme::muted : theme::dim), true, true);
		break;
	case Style::Step:
	{
		bool enabled = isEnabled(button.id);
		canvas_.drawRect(area, theme::panel);
		canvas_.drawRectOutline(area, enabled && hovered ? theme::accentBorderHover : theme::border);
		canvas_.drawText(font, wide(button.label).c_str(), area, enabled ? (hovered ? theme::text : theme::muted) : theme::dim, true, true);
		break;
	}
	case Style::Card:
	{
		canvas_.drawRect(area, chosen ? theme::accentFill : theme::panel);
		canvas_.drawRectOutline(area, chosen ? theme::accentBorder : (hovered ? theme::borderHover : theme::border));
		SColor strong = chosen ? theme::text : (hovered ? theme::muted : theme::dim);
		drawIcon(button.id, area.UpperLeftCorner + position2d<s32>(24, 24), chosen ? theme::accent : strong);
		text(font, button.label, area.UpperLeftCorner.X + 24, area.UpperLeftCorner.Y + 72, strong);
		text(canvas_.loadFont(bodyFont), button.detail, area.UpperLeftCorner.X + 24, area.UpperLeftCorner.Y + 100,
			chosen ? theme::muted : theme::dim);
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
	canvas_.drawGradientRect(rect<s32>(0, 0, (s32)screen.Width, (s32)screen.Height), theme::backgroundTop, theme::backgroundBottom);

	if (built_ == Screen::CharacterCreation)
	{
		drawPreview();
		drawAegis();
	}

	if (built_ == Screen::Title)
	{
		text(canvas_.loadFont(titleFont), gameTitle, left - 4, 150, theme::text);
		canvas_.drawRect(rect<s32>(left, 232, left + 96, 236), theme::accent);
		text(canvas_.loadFont(bodyFont), tagline, left, 254, theme::muted);
	}
	else if (!title_.empty())
	{
		text(canvas_.loadFont(headingFont), title_, left - 2, 92, theme::text);
		if (!subtitle_.empty())
			text(canvas_.loadFont(bodyFont), subtitle_, left, 140, theme::muted);
	}

	switch (built_)
	{
	case Screen::CharacterSelect:
		label("YOUR CHARACTERS", left, 176);
		break;
	case Screen::CharacterCreation:
		label("NAME", left, 186);
		label("GENDER", left, 274);
		label("PROFESSION", left, 358);
		break;
	case Screen::JoinGame:
		label("SERVER ADDRESS", left, 350);
		label("PORT", middle + 112, 350);
		break;
	case Screen::Playing:
	{
		std::string where = flow_.session() == menu::Session::Host
			? "Hosting on port " + std::to_string(flow_.port())
			: "Joining " + flow_.address() + ":" + std::to_string(flow_.port());
		text(canvas_.loadFont(bodyFont), where, left, 140, theme::muted);
		s32 y = 200;
		for (const std::string& line : status_)
		{
			text(canvas_.loadFont(bodyFont), line, left, y, theme::text);
			y += 26;
		}
		break;
	}
	default:
		break;
	}

	guienv->drawAll();
	for (const Button& button : buttons_)
		drawButton(button);

	if (built_ == Screen::CharacterSelect && flow_.characters().empty())
		text(canvas_.loadFont(bodyFont), "No one yet. Make a new character to begin.", left + 24, 216, theme::muted);
	s32 below = 0;
	for (const Button& button : buttons_)
		below = std::max(below, button.area.LowerRightCorner.Y);
	if (!flow_.message().empty())
		text(canvas_.loadFont(bodyFont), flow_.message(), left, below + 14, theme::accent);
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
	case GUI_ID_MALE: gender_ = character::Gender::Male; break;
	case GUI_ID_FEMALE: gender_ = character::Gender::Female; break;
	case GUI_ID_NAME:
	case GUI_ID_CREATE:
		flow_.create(panel_.text(GUI_ID_NAME), gender_, panel_.selected(GUI_ID_PROFESSION));
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
			flow_.adjust((character::Attribute)(id - GUI_ID_MINUS_FIRST), -1);
		else if (id >= GUI_ID_PLUS_FIRST && id < GUI_ID_PLUS_FIRST + character::ATTRIBUTE_COUNT)
			flow_.adjust((character::Attribute)(id - GUI_ID_PLUS_FIRST), 1);
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
