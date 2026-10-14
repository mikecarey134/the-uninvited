#include "MenuSkin.h"
#include "Theme.h"
#include <IGUIElement.h>
#include <IGUIEnvironment.h>
#include <IVideoDriver.h>

using namespace irr;
using namespace core;
using namespace gui;
using video::SColor;

MenuSkin::MenuSkin(IGUIEnvironment* environment, IGUISkin* inner) : environment_(environment), inner_(inner)
{
	inner_->grab();
	for (s32 i = 0; i < EGDC_COUNT; ++i)
		colours_[i] = inner_->getColor((EGUI_DEFAULT_COLOR)i);
	colours_[EGDC_BUTTON_TEXT] = theme::dim;
	colours_[EGDC_GRAY_TEXT] = theme::dim;
	colours_[EGDC_HIGH_LIGHT] = theme::accentFill;
	colours_[EGDC_HIGH_LIGHT_TEXT] = theme::text;
	colours_[EGDC_WINDOW] = theme::input;
	colours_[EGDC_EDITABLE] = theme::input;
	colours_[EGDC_FOCUSED_EDITABLE] = theme::input;
	colours_[EGDC_SCROLLBAR] = theme::panel;
	colours_[EGDC_WINDOW_SYMBOL] = theme::muted;
	colours_[EGDC_GRAY_WINDOW_SYMBOL] = theme::dim;
	colours_[EGDC_3D_FACE] = theme::panel;
	colours_[EGDC_3D_HIGH_LIGHT] = theme::panel;
	for (s32 i = 0; i < EGDC_COUNT; ++i)
		inner_->setColor((EGUI_DEFAULT_COLOR)i, colours_[i]);
}

MenuSkin::~MenuSkin()
{
	inner_->drop();
}

bool MenuSkin::isDrawnByGame(const IGUIElement* element)
{
	return element && element->getType() == EGUIET_BUTTON &&
		!(element->getParent() && element->getParent()->getType() == EGUIET_SCROLL_BAR);
}

void MenuSkin::flat(const rect<s32>& area, SColor fill, SColor edge, const rect<s32>* clip)
{
	video::IVideoDriver* driver = environment_->getVideoDriver();
	if (fill.getAlpha())
		driver->draw2DRectangle(fill, area, clip);
	driver->draw2DRectangle(edge, rect<s32>(area.UpperLeftCorner.X, area.UpperLeftCorner.Y, area.LowerRightCorner.X, area.UpperLeftCorner.Y + 1), clip);
	driver->draw2DRectangle(edge, rect<s32>(area.UpperLeftCorner.X, area.LowerRightCorner.Y - 1, area.LowerRightCorner.X, area.LowerRightCorner.Y), clip);
	driver->draw2DRectangle(edge, rect<s32>(area.UpperLeftCorner.X, area.UpperLeftCorner.Y + 1, area.UpperLeftCorner.X + 1, area.LowerRightCorner.Y - 1), clip);
	driver->draw2DRectangle(edge, rect<s32>(area.LowerRightCorner.X - 1, area.UpperLeftCorner.Y + 1, area.LowerRightCorner.X, area.LowerRightCorner.Y - 1), clip);
}

SColor MenuSkin::getColor(EGUI_DEFAULT_COLOR color) const
{
	return color < EGDC_COUNT ? colours_[color] : SColor();
}

void MenuSkin::setColor(EGUI_DEFAULT_COLOR which, SColor newColor)
{
	if (which < EGDC_COUNT)
		colours_[which] = newColor;
}

s32 MenuSkin::getSize(EGUI_DEFAULT_SIZE size) const
{
	return size == EGDS_TEXT_DISTANCE_X ? 12 : inner_->getSize(size);
}

void MenuSkin::setSize(EGUI_DEFAULT_SIZE which, s32 size)
{
	inner_->setSize(which, size);
}

const wchar_t* MenuSkin::getDefaultText(EGUI_DEFAULT_TEXT text) const
{
	return inner_->getDefaultText(text);
}

void MenuSkin::setDefaultText(EGUI_DEFAULT_TEXT which, const wchar_t* newText)
{
	inner_->setDefaultText(which, newText);
}

IGUIFont* MenuSkin::getFont(EGUI_DEFAULT_FONT which) const
{
	return inner_->getFont(which);
}

void MenuSkin::setFont(IGUIFont* font, EGUI_DEFAULT_FONT which)
{
	inner_->setFont(font, which);
}

IGUISpriteBank* MenuSkin::getSpriteBank() const
{
	return inner_->getSpriteBank();
}

void MenuSkin::setSpriteBank(IGUISpriteBank* bank)
{
	inner_->setSpriteBank(bank);
}

u32 MenuSkin::getIcon(EGUI_DEFAULT_ICON icon) const
{
	return inner_->getIcon(icon);
}

void MenuSkin::setIcon(EGUI_DEFAULT_ICON icon, u32 index)
{
	inner_->setIcon(icon, index);
}

void MenuSkin::draw3DButtonPaneStandard(IGUIElement* element, const rect<s32>& area, const rect<s32>* clip)
{
	if (!isDrawnByGame(element))
		flat(area, theme::scrollThumb, environment_->getHovered() == element ? theme::borderHover : theme::border, clip);
}

void MenuSkin::draw3DButtonPanePressed(IGUIElement* element, const rect<s32>& area, const rect<s32>* clip)
{
	if (!isDrawnByGame(element))
		flat(area, theme::accentFill, theme::accentBorder, clip);
}

void MenuSkin::draw3DSunkenPane(IGUIElement* element, SColor, bool, bool fillBackGround, const rect<s32>& area, const rect<s32>* clip)
{
	bool editing = element && element->getType() == EGUIET_EDIT_BOX;
	bool focused = editing && environment_->hasFocus(element);
	SColor fill = fillBackGround ? (editing ? theme::input : theme::panel) : SColor(0, 0, 0, 0);
	flat(area, fill, focused ? theme::accentBorderHover : theme::border, clip);
}

rect<s32> MenuSkin::draw3DWindowBackground(IGUIElement*, bool, SColor, const rect<s32>& area, const rect<s32>* clip, rect<s32>* checkClientArea)
{
	if (checkClientArea)
		*checkClientArea = area;
	else
		flat(area, theme::panel, theme::border, clip);
	return area;
}

void MenuSkin::draw3DMenuPane(IGUIElement*, const rect<s32>& area, const rect<s32>* clip)
{
	flat(area, theme::panel, theme::border, clip);
}

void MenuSkin::draw3DToolBar(IGUIElement*, const rect<s32>& area, const rect<s32>* clip)
{
	flat(area, theme::panel, theme::border, clip);
}

void MenuSkin::draw3DTabButton(IGUIElement*, bool active, const rect<s32>& area, const rect<s32>* clip, EGUI_ALIGNMENT)
{
	flat(area, active ? theme::accentFill : theme::panel, active ? theme::accentBorder : theme::border, clip);
}

void MenuSkin::draw3DTabBody(IGUIElement*, bool border, bool background, const rect<s32>& area, const rect<s32>* clip, s32, EGUI_ALIGNMENT)
{
	if (border || background)
		flat(area, background ? theme::panel : SColor(0, 0, 0, 0), border ? theme::border : SColor(0, 0, 0, 0), clip);
}

void MenuSkin::drawIcon(IGUIElement* element, EGUI_DEFAULT_ICON icon, const position2di position, u32 starttime, u32 currenttime,
	bool loop, const rect<s32>* clip)
{
	inner_->drawIcon(element, icon, position, starttime, currenttime, loop, clip);
}

void MenuSkin::draw2DRectangle(IGUIElement*, const SColor& color, const rect<s32>& pos, const rect<s32>* clip)
{
	environment_->getVideoDriver()->draw2DRectangle(color, pos, clip);
}
