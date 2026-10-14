#include "MenuScreens.h"
#include "SoundEngine.h"
#include "UiAnchor.h"

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;
using menu::Screen;

namespace
{
	const char* const gameTitle = "UNTITLED SURVIVAL";
	const char* const bigFont = "assets/fonts/bigfont.png";
	const char* const smallFont = "assets/fonts/console.bmp";
	const SColor white(255,255,255,255);
	const SColor grey(255,170,170,170);
	const SColor warning(255,255,120,100);

	std::wstring wide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}
}

MenuScreens::MenuScreens(IrrlichtDevice* device, Canvas& canvas, menu::MenuFlow& flow, gamecore::SoundEngine* sound,
	const std::deque<std::string>& status)
	: device_(device), canvas_(canvas), flow_(flow), sound_(sound), status_(status),
	panel_(device->getGUIEnvironment(), canvas.screenSize()), built_(Screen::Title), hasBuilt_(false),
	random_(std::random_device()())
{
}

void MenuScreens::hide()
{
	panel_.clear();
	hasBuilt_ = false;
}

void MenuScreens::build()
{
	hide();
	built_ = flow_.screen();
	hasBuilt_ = true;

	switch (built_)
	{
	case Screen::Title:
		panel_.button(rect<s32>(340,250,460,290), GUI_ID_PLAY, "Play");
		panel_.button(rect<s32>(340,305,460,345), GUI_ID_SETTINGS, "Settings");
		panel_.button(rect<s32>(340,360,460,400), GUI_ID_QUIT, "Quit");
		break;

	case Screen::Settings:
		panel_.button(rect<s32>(340,480,460,520), GUI_ID_BACK, "Back");
		break;

	case Screen::CharacterSelect:
	{
		std::vector<std::string> names;
		for (const character::CharacterSheet& sheet : flow_.characters())
			names.push_back(sheet.name() + "  -  " + sheet.profession().name);
		panel_.list(rect<s32>(250,140,550,400), GUI_ID_CHARACTERS, names, flow_.highlighted());
		panel_.button(rect<s32>(250,420,395,460), GUI_ID_NEW_CHARACTER, "New Character");
		panel_.button(rect<s32>(405,420,550,460), GUI_ID_SELECT, "Select");
		panel_.button(rect<s32>(340,480,460,520), GUI_ID_BACK, "Back");
		break;
	}

	case Screen::CharacterCreation:
	{
		panel_.textBox(rect<s32>(250,172,550,200), GUI_ID_NAME, "", menu::MAX_NAME_LENGTH, true);
		panel_.toggle(rect<s32>(250,237,550,267), GUI_ID_GENDER, { "Male", "Female" });
		panel_.button(rect<s32>(250,480,395,520), GUI_ID_CREATE, "Create");
		panel_.button(rect<s32>(405,480,550,520), GUI_ID_BACK, "Back");
		std::vector<std::string> professions;
		for (const character::Profession& profession : flow_.professions())
			professions.push_back(profession.name);
		panel_.dropdown(rect<s32>(250,304,550,330), GUI_ID_PROFESSION, professions, 7);
		break;
	}

	case Screen::WorldSelect:
		panel_.button(rect<s32>(325,250,475,290), GUI_ID_HOST, "Host Game");
		panel_.button(rect<s32>(325,305,475,345), GUI_ID_JOIN, "Join Game");
		panel_.button(rect<s32>(340,480,460,520), GUI_ID_BACK, "Back");
		break;

	case Screen::JoinGame:
		panel_.textBox(rect<s32>(250,222,550,250), GUI_ID_ADDRESS, "", 253, true);
		panel_.textBox(rect<s32>(250,287,400,315), GUI_ID_PORT, std::to_string(menu::DEFAULT_PORT), 5, false);
		panel_.button(rect<s32>(250,420,395,460), GUI_ID_CONNECT, "Connect");
		panel_.button(rect<s32>(405,420,550,460), GUI_ID_BACK, "Back");
		break;

	case Screen::Playing:
		panel_.button(rect<s32>(340,480,460,520), GUI_ID_LEAVE, "Leave");
		break;

	default:
		break;
	}
}

void MenuScreens::label(FontHandle font, const std::string& text, s32 x1, s32 y1, s32 x2, s32 y2, SColor colour, bool centred)
{
	canvas_.drawText(font, wide(text).c_str(), anchored(rect<s32>(x1, y1, x2, y2), 0.5f, 0.5f, canvas_.screenSize()),
		colour, centred, true);
}

void MenuScreens::draw()
{
	if (!hasBuilt_ || flow_.screen() != built_)
		build();

	IGUIEnvironment* guienv = device_->getGUIEnvironment();
	if (IGUIFont* skinFont = guienv->getFont(smallFont))
		guienv->getSkin()->setFont(skinFont);

	dimension2d<u32> screen = canvas_.screenSize();
	canvas_.drawGradientRect(rect<s32>(0, 0, (s32)screen.Width, (s32)screen.Height), SColor(255,34,36,40), SColor(255,6,6,8));
	FontHandle big = canvas_.loadFont(bigFont);
	FontHandle small = canvas_.loadFont(smallFont);
	std::string playing = flow_.chosen() ? flow_.chosen()->name() : std::string();

	switch (built_)
	{
	case Screen::Title:
		label(big, gameTitle, 0,120,800,180, white, true);
		break;
	case Screen::Settings:
		label(big, "Settings", 0,60,800,110, white, true);
		label(small, "Nothing to set yet.", 0,250,800,280, grey, true);
		break;
	case Screen::CharacterSelect:
		label(big, "Characters", 0,60,800,110, white, true);
		if (flow_.characters().empty())
			label(small, "No characters yet. Make one with New Character.", 0,400,800,415, grey, true);
		break;
	case Screen::CharacterCreation:
		label(big, "New Character", 0,60,800,110, white, true);
		label(small, "Name", 250,150,550,170, white, false);
		label(small, "Gender", 250,215,550,235, white, false);
		label(small, "Profession", 250,282,550,302, white, false);
		break;
	case Screen::WorldSelect:
		label(big, "Playing as " + playing, 0,60,800,110, white, true);
		break;
	case Screen::JoinGame:
		label(big, "Join Game", 0,60,800,110, white, true);
		label(small, "Server address", 250,200,550,220, white, false);
		label(small, "Port", 250,265,550,285, white, false);
		break;
	case Screen::Playing:
	{
		label(big, playing, 0,60,800,110, white, true);
		std::string where = flow_.session() == menu::Session::Host
			? "Hosting on port " + std::to_string(flow_.port())
			: "Joining " + flow_.address() + ":" + std::to_string(flow_.port());
		label(small, where, 0,150,800,170, grey, true);
		s32 y = 200;
		for (const std::string& line : status_)
		{
			label(small, line, 0,y,800,y + 20, white, true);
			y += 24;
		}
		break;
	}
	default:
		break;
	}

	if (!flow_.message().empty())
		label(small, flow_.message(), 100,530,700,560, warning, true);
	canvas_.drawText(small, L"v0.1 (early development)", anchored(rect<s32>(20,570,20,590), 0.0f, 1.0f, screen), white);

	guienv->drawAll();
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
	case GUI_ID_BACK: flow_.back(); break;
	case GUI_ID_NEW_CHARACTER: flow_.newCharacter(); break;
	case GUI_ID_SELECT:
		flow_.highlight(panel_.selected(GUI_ID_CHARACTERS));
		flow_.select();
		break;
	case GUI_ID_NAME:
	case GUI_ID_CREATE:
		flow_.create(panel_.text(GUI_ID_NAME), panel_.selected(GUI_ID_GENDER) == 1 ? character::Gender::Female : character::Gender::Male,
			panel_.selected(GUI_ID_PROFESSION), random_);
		break;
	case GUI_ID_HOST: flow_.host(); break;
	case GUI_ID_JOIN: flow_.join(); break;
	case GUI_ID_ADDRESS:
	case GUI_ID_PORT:
	case GUI_ID_CONNECT:
		flow_.connect(panel_.text(GUI_ID_ADDRESS), panel_.text(GUI_ID_PORT));
		break;
	case GUI_ID_LEAVE: flow_.returnToTitle(); break;
	default:
		return false;
	}
	click();
	return true;
}

void MenuScreens::back()
{
	if (flow_.screen() != Screen::Title && flow_.screen() != Screen::Playing)
		click();
	flow_.back();
}

bool MenuScreens::onGuiEvent(const SEvent& event)
{
	if (panel_.handle(event))
	{
		click();
		return true;
	}
	switch (event.GUIEvent.EventType)
	{
	case EGET_BUTTON_CLICKED:
	case EGET_EDITBOX_ENTER:
		return clicked(event.GUIEvent.Caller ? event.GUIEvent.Caller->getID() : -1);
	case EGET_LISTBOX_CHANGED:
		flow_.highlight(panel_.selected(GUI_ID_CHARACTERS));
		return true;
	case EGET_LISTBOX_SELECTED_AGAIN:
		return clicked(GUI_ID_SELECT);
	default:
		return false;
	}
}
