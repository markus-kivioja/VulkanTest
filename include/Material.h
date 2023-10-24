#pragma once

#include "Texture.h"

#include <memory>

class Material
{
public:
private:
	std::unique_ptr<Texture> m_albedoMap;
	std::unique_ptr<Texture> m_normalMap;
	std::unique_ptr<Texture> m_displacementMap;
	std::unique_ptr<Texture> m_specularMap;
};