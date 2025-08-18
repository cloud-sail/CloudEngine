#pragma once
#include "Game/EngineBuildPreferences.hpp"

// For Math Unit Tests (No Renderer Subsystem)
#if !defined(ENGINE_RENDER_D3D11) && !defined(ENGINE_RENDER_D3D12)
#include "Engine/Math/IntVec2.hpp"
#include <string>

//------------------------------------------------------------------------------------------------
class Texture
{
private:
	Texture() = default; // can't instantiate directly; must ask Renderer to do it for you
	Texture(Texture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~Texture() = default;

public:
	IntVec2				GetDimensions() const { return m_dimensions; }
	std::string const& GetImageFilePath() const { return m_name; }

protected:
	std::string			m_name;
	IntVec2				m_dimensions;
};
#endif



#ifdef ENGINE_RENDER_D3D11
#include "Engine/Math/IntVec2.hpp"
#include <string>
//-----------------------------------------------------------------------------------------------
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

//------------------------------------------------------------------------------------------------
class Texture
{
	friend class Renderer; // Only the Renderer can create new Texture objects!
	friend class DX11Renderer;

private:
	Texture(); // can't instantiate directly; must ask Renderer to do it for you
	Texture(Texture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~Texture();

public:
	IntVec2				GetDimensions() const { return m_dimensions; }
	std::string const& GetImageFilePath() const { return m_name; }

protected:
	std::string			m_name;
	IntVec2				m_dimensions;

protected:
	ID3D11Texture2D* m_texture = nullptr;
	ID3D11ShaderResourceView* m_shaderResourceView = nullptr;
};
#endif // ENGINE_RENDER_D3D11


#ifdef ENGINE_RENDER_D3D12
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/RendererCommon.hpp"
#include <string>
#include <dxgiformat.h>
//-----------------------------------------------------------------------------------------------
struct ID3D12Resource;
class DX12Renderer;

//------------------------------------------------------------------------------------------------
class Texture
{
	friend class Renderer; // Only the Renderer can create new Texture objects!
	friend class DX12Renderer; 

private:
	Texture(DX12Renderer* renderer); // can't instantiate directly; must ask Renderer to do it for you
	Texture(Texture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~Texture();

public:
	IntVec2				GetDimensions() const { return m_dimensions; }
	std::string const& GetImageFilePath() const { return m_name; }
	unsigned int		GetWidth() const { return static_cast<unsigned int>(m_dimensions.x); }
	unsigned int		GetHeight() const { return static_cast<unsigned int>(m_dimensions.y); }
	unsigned int		GetDepth() const { return m_depth; }
	float				GetClearDepth() const { return m_dsvClearDepth; }
	uint8_t				GetClearStencil() const { return m_dsvClearStencil; }

protected:
	std::string			m_name;
	IntVec2				m_dimensions; // x: width y: height
	unsigned int		m_depth = 1; // depth or arraySize

	uint32_t			m_mipLevels = 1;
	uint32_t			m_sampleCount = 1;
	uint32_t			m_sampleQuality = 0;

	DXGI_FORMAT			m_format = DXGI_FORMAT_UNKNOWN;

	bool m_supportSRV = true;
	bool m_supportRTV = false;
	bool m_supportDSV = false;
	bool m_supportUAV = false;

	float       m_dsvClearDepth = 1.0f;
	uint8_t     m_dsvClearStencil = 0;

	unsigned int m_currentResourceState = 0;

	ID3D12Resource* m_resource = nullptr;
	DXResourceDimension m_resourceDimension = DXResourceDimension::TEXTURE2D;
protected:
	DX12Renderer* m_renderer = nullptr;
	// only for loaded textures, persistent, destroy on shutdown, only have one srv, createfrom file BOOL? 
	// GetIndexFor this kind of texture, check the bool first
	uint32_t m_srvHeapIndex = INVALID_INDEX_U32;
};
#endif // ENGINE_RENDER_D3D12
