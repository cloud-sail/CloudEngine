#pragma once
#include "Game/EngineBuildPreferences.hpp"

#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Core/Rgba8.hpp"

#include <string>
#include <vector>
#include <dxgiformat.h>

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
class Buffer;

struct IntVec2;
struct Rgba8;
struct Vertex_PCU;
struct Vertex_PCUTBN;
struct ShaderConfig;


constexpr uint32_t INVALID_INDEX_U32 = static_cast<uint32_t>(-1);

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
	POINT_WARP,
	POINT_CLAMP,
	BILINEAR_WRAP,
	BILINEAR_CLAMP,
	ANISO_WARP,
	ANISO_CLAMP,
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
	VERTEX_NONE,
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
	Mat44 ClipToWorldTransform;
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
	SHADER_STAGE_CS = 1 << 5,
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
	std::string m_computeEntryPoint = "ComputeMain";

	ShaderStage m_stages = (ShaderStage)(SHADER_STAGE_VS | SHADER_STAGE_PS);
	ShaderCompilerType m_compilerType = SHADER_COMPILER_DXC;

	std::string m_shaderModelVS = "vs_6_6";
	std::string m_shaderModelPS = "ps_6_6";
	std::string m_shaderModelGS = "gs_6_6";
	std::string m_shaderModelHS = "hs_6_6";
	std::string m_shaderModelDS = "ds_6_6";
	std::string m_shaderModelCS = "cs_6_6";
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
constexpr unsigned int FRAMES_IN_FLIGHT = 3;
constexpr size_t MAX_LINEAR_ALLOCATOR_SIZE = 64 * 1024 * 1024; // 64MB
constexpr size_t MAX_MAIN_LINEAR_ALLOCATOR_SIZE = 64 * 1024 * 1024; // 64MB
constexpr unsigned int IMGUI_SRV_HEAP_SIZE = 64;


//-----------------------------------------------------------------------------------------------
// Math
//-----------------------------------------------------------------------------------------------
namespace RendererMath
{
	// Bitmask Alignment (alignment must be power of 2)
	inline size_t AlignUpWithMask(size_t value, size_t mask)
	{
		return ((value + mask) & ~mask);
	}

	inline size_t AlignDownWithMask(size_t value, size_t mask)
	{
		return (value & ~mask);
	}

	inline size_t AlignUp(size_t value, size_t alignment)
	{
		return AlignUpWithMask(value, alignment - 1);
	}

	inline size_t AlignDown(size_t value, size_t alignment)
	{
		return AlignDownWithMask(value, alignment - 1);
	}

	inline bool IsAligned(size_t value, size_t alignment)
	{
		return 0 == (value & (alignment - 1));
	}

	inline size_t AlignUpDiv(size_t value, size_t alignment)
	{
		// alignment must not be zero!
		return ((value + alignment - 1) / alignment) * alignment;
	}

	inline size_t AlignDownDiv(size_t value, size_t alignment)
	{
		return (value / alignment) * alignment;
	}

	inline bool IsAlignedDiv(size_t value, size_t alignment)
	{
		return (value % alignment) == 0;
	}

	inline bool IsPowerOfTwo(unsigned int n)
	{
		// n is positive
		return (n & (n - 1)) == 0;
	}

	inline unsigned int CeilDivide(unsigned int value, unsigned int alignment)
	{
		return (value + alignment - 1) / alignment;
	}
}

//-----------------------------------------------------------------------------------------------
// Buffer
//-----------------------------------------------------------------------------------------------
struct BufferInit
{
	uint64_t m_size = 0;
	bool m_allowUnorderedAccess = false; 
	bool m_isConstantBuffer = false; // will AlignUp the bufferSize to 256
	const wchar_t* m_name = L"Debug Buffer";
};


//-----------------------------------------------------------------------------------------------
// Texture
//-----------------------------------------------------------------------------------------------
enum class DXResourceDimension
{
	UNKNOWN,
	BUFFER,
	TEXTURE1D,
	TEXTURE2D,
	TEXTURE3D,
};

//enum DXResourceFlags : unsigned int
//{
//	DXRF_NONE = 0,
//	DXRF_RENDER_TARGET			= 1 << 0,
//	DXRF_DEPTH_STENCIL			= 1 << 1,
//	DXRF_UNORDERED_ACCESS		= 1 << 2,
//	DXRF_DENY_SHADER_RESOURCE	= 1 << 3,
//};

struct TextureInit
{
	DXResourceDimension m_resourceDimension = DXResourceDimension::TEXTURE2D;
	unsigned int m_width = 0;
	unsigned int m_height = 0;
	unsigned short m_depthOrArraySize = 1;
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
	unsigned short m_mipLevels = 1; // 1: no mipmap 0: auto maximize mip
	// SampleDesc: no msaa {1, 0}
	unsigned int m_sampleCount = 1;
	unsigned int m_sampleQuality = 0; 
	// Layout: texture D3D12_TEXTURE_LAYOUT_UNKNOWN
	
	bool m_allowSRV = true;
	bool m_allowRTV = false;
	bool m_allowDSV = false;
	bool m_allowUAV = false;

	const wchar_t* m_debugName = L"Debug Texture";

	std::string m_name = "UNKNOWN_TEXTURE";

	// Clear Value
	DXGI_FORMAT m_rtvClearFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT m_dsvClearFormat = DXGI_FORMAT_UNKNOWN; 
	float       m_rtvClearColor[4] = { 0, 0, 0, 0 };
	float       m_dsvClearDepth = 1.0f; 
	uint8_t     m_dsvClearStencil = 0;

	// Initial Resource State is D3D12_RESOURCE_STATE_COMMON
	// Texture Sample only:	D3D12_RESOURCE_STATE_COPY_DEST -> (Copy/Upload) 
	//		-> D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	// Render Target:		D3D12_RESOURCE_STATE_RENDER_TARGET
	// Depth Buffer:		D3D12_RESOURCE_STATE_DEPTH_WRITE
	// UAV, Read Write:		D3D12_RESOURCE_STATE_UNORDERED_ACCESS

	// TODO make a default static Make Function

	// Depth Buffer
	// m_format = DXGI_FORMAT_R24G8_TYPELESS
	// SRV format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	// DSV format = DXGI_FORMAT_D24_UNORM_S8_UINT
	// m_dsvClearFormat = DXGI_FORMAT_D24_UNORM_S8_UINT

	// Render Texture (swap chain back buffer do not manage it)
};




enum class DescriptorHeapType
{
	UNKNOWN,
	RTV,
	DSV,
	CBV_SRV_UAV,
	SAMPLER,
};

struct DescriptorHandle
{
	DescriptorHeapType m_type = DescriptorHeapType::UNKNOWN;
	uint32_t m_index = INVALID_INDEX_U32;

	void Reset()
	{
		m_type = DescriptorHeapType::UNKNOWN;
		m_index = INVALID_INDEX_U32;
	}
};


// enum Default Texture Type
enum class DefaultTexture
{
	Magenta2D,
	BlackOpaque2D,
	BlackTransparent2D,
	WhiteOpaque2D,
	DefaultNormalMap,
	DefaultSpecGlossEmitMap,
	DefaultOcclusionRoughnessMetalnessMap,
	CheckerboardMagentaBlack2D,
	NUM
};


//-----------------------------------------------------------------------------------------------
// Default Shader
struct UnlitRenderResources
{
	uint32_t diffuseTextureIndex = INVALID_INDEX_U32;
	uint32_t diffuseSamplerIndex = INVALID_INDEX_U32;

	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
};

struct DiffuseRenderResources
{
	uint32_t diffuseTextureIndex = INVALID_INDEX_U32;
	uint32_t diffuseSamplerIndex = INVALID_INDEX_U32;

	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
	uint32_t lightConstantsIndex = INVALID_INDEX_U32;
};

struct BlinnPhongRenderResources
{
	uint32_t diffuseTextureIndex = INVALID_INDEX_U32;
	uint32_t normalTextureIndex = INVALID_INDEX_U32;
	uint32_t specGlossEmitTextureIndex = INVALID_INDEX_U32;

	uint32_t engineConstantsIndex = INVALID_INDEX_U32;
	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
	uint32_t lightConstantsIndex = INVALID_INDEX_U32;
};

struct SkyboxRenderResources
{
	uint32_t cubeMapTextureIndex = INVALID_INDEX_U32;

	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
};

struct PBRRenderResources
{
	uint32_t albedoTextureIndex = INVALID_INDEX_U32;
	uint32_t metallicRoughnessTextureIndex = INVALID_INDEX_U32;
	uint32_t normalTextureIndex = INVALID_INDEX_U32;
	uint32_t occlusionTextureIndex = INVALID_INDEX_U32;
	uint32_t emissiveTextureIndex = INVALID_INDEX_U32;

	uint32_t samplerIndex = INVALID_INDEX_U32;

	uint32_t engineConstantsIndex = INVALID_INDEX_U32;
	uint32_t cameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t modelConstantsIndex = INVALID_INDEX_U32;
	uint32_t lightConstantsIndex = INVALID_INDEX_U32;
	/*
	uint32_t materialConstantsIndex; // #Todo
	struct MaterialConstants
	{
		float4 baseColorFactor;
		float metallicFactor;
		float roughnessFactor;
		float3 emissiveFactor;
		float normalScale;
		float occlusionStrength;
	};
	*/
};




