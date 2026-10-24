#ifndef WORLD_H
#define WORLD_H

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <irrlicht.h>
#include "CharacterSheet.h"
#include "DpadHud.h"
#include "Hud.h"
#include "InteractionDetector.h"
#include "ForceBreak.h"
#include "InteractionInput.h"
#include "TargetHud.h"
#include "InputRouter.h"
#include "net/Interpolator.h"
#include "Loot.h"
#include "protocol/GameMessages.h"

struct TestObject;
class ContainerInteractable;

class World : public InputLayer
{
public:
	static const uint8_t IDLE_STATE = 0;
	static const uint8_t WALKING_STATE = 3;
	static constexpr irr::f32 START_HOUR = 8.0f;
	static const int HURT_DAMAGE = 5;

	World(irr::IrrlichtDevice* device, const char* model, const char* skin, std::function<void()> leave,
		const character::CharacterSheet& sheet = character::CharacterSheet::create("", character::Gender::Male, character::Profession()));
	~World();

	bool onEvent(const irr::SEvent& event) override;
	void look(irr::f32 degrees);
	// Tilts the camera up (positive) or down, held within 80 degrees either way.
	void tilt(irr::f32 degrees);
	irr::f32 pitch() const { return pitch_; }
	void update(irr::f32 seconds);
	void draw();

	irr::scene::ISceneManager* scene() const { return scene_; }
	irr::scene::IAnimatedMeshSceneNode* player() const { return player_; }
	irr::scene::ICameraSceneNode* camera() const { return camera_; }
	irr::f32 cameraDistance() const;
	irr::f32 playerRadius() const;
	const character::CharacterSheet& sheet() const { return sheet_; }
	bool dead() const { return sheet_.isDead(); }
	irr::f32 timeCompression() const { return compression_; }
	void setTimeCompression(irr::f32 compression) { compression_ = compression; }
	irr::f32 timeOfDay() const { return timeOfDay_; }
	void setTimeOfDay(irr::f32 hours);
	void advanceClock(irr::f32 seconds);
	bool debugShown() const { return debugShown_; }
	void teleport(irr::f32 x, irr::f32 y, irr::f32 z);
	void wield(const char* model);
	const HudWeapon& weapon() const { return weapon_; }
	DpadHud& dpad() { return dpad_; }
	const InteractionDetector& detector() const { return detector_; }
	const ForceBreak& forceBreak() const { return *forceBreak_; }
	bool targetMarker(irr::core::position2d<irr::s32>& at) const;
	ContextPanel& contextPanel() { return contextPanel_; }
	bool contextPanelOpen() const { return contextPanel_.isOpen(); }
	void setInteractionLog(std::function<void(const std::string&)> log) { interaction_.setLog(log); }
	void setInteractionBlocked(std::function<bool()> blocked) { interaction_.setBlocked(blocked); }
	void setInteractionCue(std::function<void(Cue)> cue) { interaction_.setCue(cue); }
	// How the containers' approaches open the loot screen on one container.
	void setOpenLoot(std::function<void(loot::Container&)> open) { openLoot_ = open; }
	const std::string& interactionFeedback() const { return interaction_.feedback(); }
	// The interact prompt by the target: which key, and what it does now (empty with no target).
	std::string promptAction() const { return targetHud_.promptAction(); }
	const std::string& promptKey() const { return promptKey_; }
	void setPromptKey(const std::string& key) { promptKey_ = key; }
	bool interactionSucceeded() const { return interaction_.feedbackSucceeded(); }
	void arm(const HudWeapon& weapon) { weapon_ = weapon; }
	irr::f32 speed() const { return speed_; }
	void setSpeed(irr::f32 speed) { speed_ = speed; }
	void setHealth(int health) { sheet_.setHealth(health); }
	void hurt() { sheet_.setHealth(sheet_.health() - HURT_DAMAGE); }
	std::vector<loot::Container>& containers() { return containers_; }
	loot::Container* containerInReach();
	irr::scene::ILightSceneNode* sun() const { return sun_; }
	irr::video::SColor skyZenith() const { return zenith_; }
	irr::video::SColor skyHorizon() const { return horizon_; }
	protocol::State localState() const;

	void addRemote(uint16_t id, const char* model, const char* skin);
	void placeRemote(uint16_t id, uint16_t seq, const irr::f32 position[3], irr::f32 yaw, bool walking);
	void removeRemote(uint16_t id);
	void updateRemotes(irr::f32 seconds);
	irr::scene::IAnimatedMeshSceneNode* remote(uint16_t id) const;

private:
	void addTestInteractables();
	void addContainerInteractables();
	void detect(irr::f32 seconds);
	void addGround();
	void addPlayer(const char* model, const char* skin);
	void addLights();
	void addProps();
	void addProp(const char* name, const char* file, irr::f32 x, irr::f32 z, const irr::core::vector3df& rotation, irr::f32 length);
	bool blocked(const irr::core::vector3df& at) const;
	void unblock(irr::scene::ISceneNode* node);
	irr::f32 groundBelow(const irr::core::vector3df& at) const;
	void settle(irr::f32 seconds);
	void move(const irr::core::vector3df& step);
	void follow();
	void animate(bool walking);
	irr::scene::IAnimatedMeshSceneNode* addCharacter(const char* model, const char* skin, irr::f32& feet);

	struct Remote
	{
		irr::scene::IAnimatedMeshSceneNode* node;
		irr::f32 feet;
		Interpolator path;
		bool walking;
		bool animatedWalking;
	};

	irr::IrrlichtDevice* device_;
	irr::scene::ISceneManager* scene_;
	irr::scene::ICameraSceneNode* camera_;
	irr::scene::IAnimatedMeshSceneNode* player_;
	HudWeapon weapon_;
	std::function<void()> leave_;
	bool held_[4];
	bool walking_;
	irr::f32 yaw_;
	irr::f32 pitch_; // camera tilt in degrees: negative looks down
	irr::f32 fall_ = 0.0f; // vertical speed, negative while falling
	irr::f32 feet_;
	std::vector<irr::core::aabbox3df> obstacles_;
	character::CharacterSheet sheet_;
	irr::f32 unticked_;
	irr::f32 compression_;
	irr::f32 timeOfDay_;
	irr::f32 speed_;
	std::vector<loot::Container> containers_;
	bool debugShown_;
	DpadHud dpad_;
	irr::scene::ILightSceneNode* sun_;
	irr::video::SColor zenith_;
	irr::video::SColor horizon_;
	std::map<uint16_t, Remote> remotes_;
	irr::f32 clock_;
	InteractionDetector detector_;
	std::vector<std::unique_ptr<TestObject>> testInteractables_;
	std::vector<std::unique_ptr<ContainerInteractable>> containerInteractables_;
	std::function<void(loot::Container&)> openLoot_;
	std::unique_ptr<ForceBreak> forceBreak_;
	std::vector<irr::scene::ISceneNode*> props_;
	std::string promptKey_ = "E";
	Interactable* shownTarget_ = nullptr;
	TargetHud targetHud_ = TargetHud(defaultApproaches());
	ContextPanel contextPanel_;
	InteractionInput interaction_{ targetHud_, dpad_, contextPanel_ };
};

#endif
