#pragma once
#include "SceneObject.h"

class Mickey : public SceneObject
{
public:
	Mickey(uint32_t id, Transforms* transforms, Mesh* mesh, Material* material);

	virtual void update(float dt, uint32_t bufferIdx) override;
private:
};

