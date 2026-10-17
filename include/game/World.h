#ifndef WORLD_H
#define WORLD_H

#include <functional>
#include <map>
#include <vector>
#include <irrlicht.h>
#include "CharacterSheet.h"
#include "Hud.h"
#include "InputRouter.h"
#include "net/Interpolator.h"
#include "Loot.h"
#include "protocol/GameMessages.h"

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
	void addGround();
	void addPlayer(const char* model, const char* skin);
	void addLights();
	void addProps();
	void addProp(const char* name, const char* file, irr::f32 x, irr::f32 z, const irr::core::vector3df& rotation, irr::f32 length);
	bool blocked(const irr::core::vector3df& at) const;
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
	irr::f32 feet_;
	std::vector<irr::core::aabbox3df> obstacles_;
	character::CharacterSheet sheet_;
	irr::f32 unticked_;
	irr::f32 compression_;
	irr::f32 timeOfDay_;
	irr::f32 speed_;
	std::vector<loot::Container> containers_;
	bool debugShown_;
	irr::scene::ILightSceneNode* sun_;
	irr::video::SColor zenith_;
	irr::video::SColor horizon_;
	std::map<uint16_t, Remote> remotes_;
	irr::f32 clock_;
};

#endif
