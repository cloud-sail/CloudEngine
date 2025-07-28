#pragma once
#include "Game/EngineBuildPreferences.hpp"

#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Core/Rgba8.hpp"

#include <string>
#include <vector>

#if defined(ENGINE_RENDER_D3D11) && defined(ENGINE_RENDER_D3D12)
#error "Using Multiple RHI"
#endif

//-----------------------------------------------------------------------------------------------
// Forward Declarations - Engine
//-----------------------------------------------------------------------------------------------
class Window;
class Texture;
class BitmapFont;
class Shader;
class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class Image;
class Camera;

struct IntVec2;
struct Rgba8;
struct Vertex_PCU;
struct Vertex_PCUTBN;
struct ShaderConfig;




//-----------------------------------------------------------------------------------------------
// Renderer Config
//-----------------------------------------------------------------------------------------------
struct RendererConfig
{
	Window* m_window = nullptr;
};


//-----------------------------------------------------------------------------------------------
// Render States
//-----------------------------------------------------------------------------------------------
enum class BlendMode
{
	OPAQUE,
	ALPHA,
	ADDITIVE,
	COUNT,
};

enum class SamplerMode
{
	POINT_CLAMP,
	BILINEAR_WRAP,
	COUNT,
};

enum class RasterizerMode
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,
	COUNT,
};

enum class DepthMode
{
	DISABLED,
	READ_ONLY_ALWAYS,
	READ_ONLY_LESS_EQUAL,
	READ_WRITE_LESS_EQUAL,
	COUNT,
};

enum class VertexType
{
	VERTEX_PCU,
	VERTEX_PCUTBN,
	COUNT
};


// Constant Buffer has packing rules in HLSL
// Pack into 4 byte boundaries
// Constant Buffers MUST be 16B-aligned
// static_assert(sizeof(LightConstants) == 8 * sizeof(float));
// static_assert(alignof(LightConstants) == 16);
// float padding0[2];
// float padding1;
//-----------------------------------------------------------------------------------------------
struct EngineConstants
{
	int		m_debugInt = 0;
	float	m_debugFloat = 0.f;
	float	padding0[2];
};
constexpr int k_engineConstantsSlot = 0;

//-----------------------------------------------------------------------------------------------
struct PerFrameConstants
{
	Vec3	m_resolution;
	float	m_timeSeconds = 0.f;

	Vec4	m_mouse;
};


constexpr int k_perFrameConstantsSlot = 1;

//-----------------------------------------------------------------------------------------------
struct CameraConstants
{
	Mat44 WorldToCameraTransform;	// View Transform
	Mat44 CameraToRenderTransform;	// Non-standard transform from game to DirectX conventions
	Mat44 RenderToClipTransform;	// Projection transform
	Vec3  CameraWorldPosition;		// Camera World Position
	float padding;
};
constexpr int k_cameraConstantsSlot = 2;

//-----------------------------------------------------------------------------------------------
struct ModelConstants
{
	Mat44 ModelToWorldTransform; // Model Transform
	float ModelColor[4];
};
constexpr int k_modelConstantsSlot = 3;

//-----------------------------------------------------------------------------------------------
struct Light
{
	float m_color[4];
	Vec3 m_worldPosition;
	float padding;
	Vec3 m_spotForwardNormal;
	float m_ambience = 0.f;
	float m_innerRadius = 0.f;
	float m_outerRadius = 1.f;
	float m_innerDotThreshold = -1.f;
	float m_outerDotThreshold = -2.f;

	void SetColor(float r, float g, float b, float a)
	{
		m_color[0] = r;
		m_color[1] = g;
		m_color[2] = b;
		m_color[3] = a;
	}
};

constexpr int MAX_LIGHTS = 8;
struct LightConstants
{
	float   m_sunColor[4];
	Vec3    m_sunNormal;
	int     m_numLights = 0;
	Light   m_lights[MAX_LIGHTS];
};

constexpr int k_lightConstantsSlot = 4;

//-----------------------------------------------------------------------------------------------
// Shader
//-----------------------------------------------------------------------------------------------
enum ShaderStage : unsigned int
{
	SHADER_STAGE_NONE = 0,
	SHADER_STAGE_VS = 1 << 0,
	SHADER_STAGE_PS = 1 << 1,
	SHADER_STAGE_GS = 1 << 2,
	SHADER_STAGE_HS = 1 << 3,
	SHADER_STAGE_DS = 1 << 4,
};

enum ShaderCompilerType
{
	SHADER_COMPILER_D3DCOMPILE,
	SHADER_COMPILER_DXC,
};

struct ShaderConfig
{
	ShaderConfig() = default;
	ShaderConfig(char const* shaderName) : m_name(shaderName) {}

	std::string m_name;
	std::string m_vertexEntryPoint = "VertexMain";
	std::string m_pixelEntryPoint = "PixelMain";
	std::string m_geometryEntryPoint = "GeometryMain";
	std::string m_hullEntryPoint = "HullMain";
	std::string m_domainEntryPoint = "DomainMain";

	ShaderStage m_stages = (ShaderStage)(SHADER_STAGE_VS | SHADER_STAGE_PS);
	ShaderCompilerType m_compilerType = SHADER_COMPILER_DXC;

	std::string m_shaderModelVS = "vs_6_6";
	std::string m_shaderModelPS = "ps_6_6";
	std::string m_shaderModelGS = "gs_6_6";
	std::string m_shaderModelHS = "hs_6_6";
	std::string m_shaderModelDS = "ds_6_6";
};

//-----------------------------------------------------------------------------------------------
// Texture Cube
//-----------------------------------------------------------------------------------------------
struct TextureCubeSixFacesConfig
{
	std::string m_name;
	std::string m_rightImageFilePath;
	std::string m_leftImageFilePath;
	std::string m_upImageFilePath;
	std::string m_downImageFilePath;
	std::string m_forwardImageFilePath;
	std::string m_backwardImageFilePath;
};

//-----------------------------------------------------------------------------------------------
// DX12 Renderer Settings
//-----------------------------------------------------------------------------------------------
constexpr unsigned int SWAP_CHAIN_BUFFER_COUNT = 2;
constexpr unsigned int FRAMES_IN_FLIGHT = 2;
constexpr size_t MAX_LINEAR_ALLOCATOR_SIZE = 64 * 1024 * 1024; // 64MB
constexpr size_t MAX_MAIN_LINEAR_ALLOCATOR_SIZE = 64 * 1024 * 1024; // 64MB
constexpr unsigned int IMGUI_SRV_HEAP_SIZE = 64;
