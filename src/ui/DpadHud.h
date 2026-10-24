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

	TextureHandle texture(Canvas& canvas, const std::string& iconPath, TextureHandle& cached);

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
};

#endif
