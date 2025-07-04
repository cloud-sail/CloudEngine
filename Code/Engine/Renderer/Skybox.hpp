#pragma once
#include <vector>
#include <string>

//-----------------------------------------------------------------------------------------------
class Renderer;
class Texture;
class Camera;
struct Vertex_PCU;


//-----------------------------------------------------------------------------------------------
enum class SkyboxType
{
	CUBE_MAP,
};

struct SkyboxConfig
{
	Renderer* m_defaultRenderer = nullptr;
	SkyboxType m_type = SkyboxType::CUBE_MAP;
	std::string m_fileName;
};

class Skybox
{
public:
	Skybox(SkyboxConfig const& config);
	~Skybox() = default;

	void Render(Camera const& camera, Renderer* rendererOverride = nullptr) const;

private:
	SkyboxConfig m_config;

	std::vector<Vertex_PCU> m_unlitVertexs;
	Texture* m_texture = nullptr;
};

