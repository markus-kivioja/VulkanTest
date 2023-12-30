#pragma once
#include "SceneObject.h"

#include "Mesh.h"

class EnvironmentCube : public SceneObject
{
public:
	EnvironmentCube(uint32_t id, Transforms* transforms, Mesh* mesh, Material* material);

	virtual void update(float dt, uint32_t bufferIdx) override;
private:
	inline static const std::vector<std::string> ALBEDO_FILENAMES =
	{
		"assets/posx.jpg",
		"assets/negx.jpg",
		"assets/posy.jpg",
		"assets/negy.jpg",
		"assets/posz.jpg",
		"assets/negz.jpg",
	};

	
};

