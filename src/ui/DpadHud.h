#ifndef DPAD_HUD_H
#define DPAD_HUD_H

#include "Canvas.h"
#include <string>

enum class DpadState { IDLE, ACTIVE, MULTI_OBJECT };
enum class DpadDirection { UP, RIGHT, DOWN, LEFT, NONE };

// The bottom-left four-way action widget. Icon paths are relative to assets/ui/icons/ (for example "skills/lockpicking.png").
class DpadHud
{
public:
	void setState(DpadState state);
	void setTarget(const std::string& name, const std::string& iconPath);
	void setSlot(DpadDirection dir, const std::string& label, const std::string& iconPath);
	void clearSlot(DpadDirection dir);
	void setSelectedDirection(DpadDirection dir);
	void clearSelection();
	void setActionPrompt(const std::string& text);
	void update(irr::f32 deltaTime);
	void render(Canvas& canvas);

	DpadState state() const { return state_; }
	irr::f32 opacity() const { return opacity_; }

private:
	struct Slot
	{
		bool available = false;
		std::string label;
		std::string iconPath;
		TextureHandle icon = nullptr;
	};

	struct Layout
	{
		irr::core::dimension2d<irr::u32> screen;
		irr::core::rect<irr::s32> box;
		irr::core::rect<irr::s32> wedges[4];
		irr::core::rect<irr::s32> centre;
		irr::s32 cell = 0;
		irr::s32 line = 0;
		irr::s32 ringSize = 0;
		irr::s32 iconSize = 0;
		FontHandle font = nullptr;
	};

	TextureHandle texture(Canvas& canvas, const std::string& iconPath, TextureHandle& cached);
	// How far a slot's content has popped in (0-1, eased): order 0 is UP and the centre, then RIGHT, DOWN, LEFT.
	irr::f32 reveal(int order) const;
	void drawContent(Canvas& canvas, const Layout& at, Slot (&slots)[4], const std::string& name, const std::string& iconPath,
		TextureHandle& icon, irr::f32 fade, bool current);

	DpadState state_ = DpadState::IDLE;
	Slot slots_[4];
	std::string targetName_;
	std::string targetIconPath_;
	TextureHandle targetIcon_ = nullptr;
	DpadDirection selected_ = DpadDirection::NONE;
	std::string prompt_;
	irr::f32 opacity_ = 0.35f; // the idle opacity, so the D-pad shows faded from the first frame
	irr::f32 pulseClock_ = 0.0f;
	TextureHandle ringTextures_[4] = {};
	// Animation: the entrance timeline (0 idle .. full), the pick pop, the target-switch crossfade and the lone-slot glow.
	irr::f32 timeline_ = 0.0f;
	irr::f32 popClock_ = 1.0f;
	irr::f32 switchClock_ = 1.0f;
	irr::f32 attentionClock_ = 10.0f;
	Slot oldSlots_[4];
	std::string oldName_;
	std::string oldIconPath_;
	TextureHandle oldIcon_ = nullptr;
	TextureHandle oldRingTextures_[4] = {};
};

#endif
