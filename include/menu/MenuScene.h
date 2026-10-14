#ifndef MENUSCENE_H
#define MENUSCENE_H

#include <irrlicht.h>

class MenuScene
{
public:
	explicit MenuScene(irr::IrrlichtDevice* device);
	~MenuScene();

	void draw();
	void showCharacter(const char* model);
	void hideCharacter();
	void turnCharacter(irr::f32 degrees);
	irr::scene::IAnimatedMeshSceneNode* character() const { return character_; }
	irr::f32 characterYaw() const { return yaw_; }
	irr::f32 characterTargetYaw() const { return targetYaw_; }
	bool characterTop(irr::core::position2d<irr::s32>& at) const;
	irr::scene::ISceneManager* scene() const { return scene_; }
	irr::scene::ICameraSceneNode* camera() const { return camera_; }

private:
	irr::scene::ISceneNode* box(const char* name, const irr::core::vector3df& position, const irr::core::vector3df& size,
		irr::video::SColor colour, const irr::core::vector3df& rotation = irr::core::vector3df(), irr::scene::ISceneNode* parent = 0);
	void finish(irr::scene::ISceneNode* node, irr::video::SColor colour);
	irr::scene::ISceneNode* car(const irr::core::vector3df& position, const irr::core::vector3df& rotation);
	void addSky();
	void addGround();
	void addCity();
	void addProps();
	void prop(const char* name, const char* file, irr::f32 x, irr::f32 z, const irr::core::vector3df& rotation, irr::f32 length);
	void addModels();
	void addLights();

	irr::IrrlichtDevice* device_;
	irr::scene::ISceneManager* scene_;
	irr::scene::ICameraSceneNode* camera_;
	irr::scene::IAnimatedMeshSceneNode* character_;
	irr::f32 yaw_;
	irr::f32 targetYaw_;
	irr::u32 lastTime_;
};

#endif
