#ifndef MENUSKIN_H
#define MENUSKIN_H

#include <IGUISkin.h>

namespace irr { namespace gui { class IGUIEnvironment; } }

class MenuSkin : public irr::gui::IGUISkin
{
public:
	MenuSkin(irr::gui::IGUIEnvironment* environment, irr::gui::IGUISkin* inner);
	~MenuSkin() override;

	irr::video::SColor getColor(irr::gui::EGUI_DEFAULT_COLOR color) const override;
	void setColor(irr::gui::EGUI_DEFAULT_COLOR which, irr::video::SColor newColor) override;
	irr::s32 getSize(irr::gui::EGUI_DEFAULT_SIZE size) const override;
	void setSize(irr::gui::EGUI_DEFAULT_SIZE which, irr::s32 size) override;
	const wchar_t* getDefaultText(irr::gui::EGUI_DEFAULT_TEXT text) const override;
	void setDefaultText(irr::gui::EGUI_DEFAULT_TEXT which, const wchar_t* newText) override;
	irr::gui::IGUIFont* getFont(irr::gui::EGUI_DEFAULT_FONT which = irr::gui::EGDF_DEFAULT) const override;
	void setFont(irr::gui::IGUIFont* font, irr::gui::EGUI_DEFAULT_FONT which = irr::gui::EGDF_DEFAULT) override;
	irr::gui::IGUISpriteBank* getSpriteBank() const override;
	void setSpriteBank(irr::gui::IGUISpriteBank* bank) override;
	irr::u32 getIcon(irr::gui::EGUI_DEFAULT_ICON icon) const override;
	void setIcon(irr::gui::EGUI_DEFAULT_ICON icon, irr::u32 index) override;

	void draw3DButtonPaneStandard(irr::gui::IGUIElement* element, const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip = 0) override;
	void draw3DButtonPanePressed(irr::gui::IGUIElement* element, const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip = 0) override;
	void draw3DSunkenPane(irr::gui::IGUIElement* element, irr::video::SColor bgcolor, bool flat, bool fillBackGround,
		const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip = 0) override;
	irr::core::rect<irr::s32> draw3DWindowBackground(irr::gui::IGUIElement* element, bool drawTitleBar,
		irr::video::SColor titleBarColor, const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip = 0,
		irr::core::rect<irr::s32>* checkClientArea = 0) override;
	void draw3DMenuPane(irr::gui::IGUIElement* element, const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip = 0) override;
	void draw3DToolBar(irr::gui::IGUIElement* element, const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip = 0) override;
	void draw3DTabButton(irr::gui::IGUIElement* element, bool active, const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip = 0, irr::gui::EGUI_ALIGNMENT alignment = irr::gui::EGUIA_UPPERLEFT) override;
	void draw3DTabBody(irr::gui::IGUIElement* element, bool border, bool background, const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip = 0, irr::s32 tabHeight = -1,
		irr::gui::EGUI_ALIGNMENT alignment = irr::gui::EGUIA_UPPERLEFT) override;
	void drawIcon(irr::gui::IGUIElement* element, irr::gui::EGUI_DEFAULT_ICON icon, const irr::core::position2di position,
		irr::u32 starttime = 0, irr::u32 currenttime = 0, bool loop = false, const irr::core::rect<irr::s32>* clip = 0) override;
	void draw2DRectangle(irr::gui::IGUIElement* element, const irr::video::SColor& color, const irr::core::rect<irr::s32>& pos,
		const irr::core::rect<irr::s32>* clip = 0) override;
	irr::gui::EGUI_SKIN_TYPE getType() const override { return irr::gui::EGST_UNKNOWN; }

	static bool isDrawnByGame(const irr::gui::IGUIElement* element);

private:
	void flat(const irr::core::rect<irr::s32>& area, irr::video::SColor fill, irr::video::SColor edge,
		const irr::core::rect<irr::s32>* clip);

	irr::gui::IGUIEnvironment* environment_;
	irr::gui::IGUISkin* inner_;
	irr::video::SColor colours_[irr::gui::EGDC_COUNT];
};

#endif
