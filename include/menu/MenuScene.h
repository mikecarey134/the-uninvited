#ifndef MENUSCENE_H
#define MENUSCENE_H

#include <irrlicht.h>

class MenuScene
{
public:
	explicit MenuScene(irr::IrrlichtDevice* device);
	~MenuScene();

	void draw();
	irr::scene::ISceneManager* scene() const { return scene_; }
	irr::scene::ICameraSceneNode* camera() const { return camera_; }

private:
	irr::scene::ISceneNode* box(const char* name, const irr::core::vector3df& position, const irr::core::vector3df& size,
		irr::video::SColor colour, const irr::core::vector3df& rotation = irr::core::vector3df());
	void addSky();
	void addGround();
	void addCity();
	void addProps();
	void addLights();

	irr::IrrlichtDevice* device_;
	irr::scene::ISceneManager* scene_;
	irr::scene::ICameraSceneNode* camera_;
};

#endif
