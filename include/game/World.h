#ifndef WORLD_H
#define WORLD_H

#include <functional>
#include <vector>
#include <irrlicht.h>
#include "InputRouter.h"

class World : public InputLayer
{
public:
	World(irr::IrrlichtDevice* device, const char* model, const char* skin, std::function<void()> leave);
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

	irr::IrrlichtDevice* device_;
	irr::scene::ISceneManager* scene_;
	irr::scene::ICameraSceneNode* camera_;
	irr::scene::IAnimatedMeshSceneNode* player_;
	std::function<void()> leave_;
	bool held_[4];
	bool walking_;
	irr::f32 yaw_;
	irr::f32 feet_;
	std::vector<irr::core::aabbox3df> obstacles_;
};

#endif
