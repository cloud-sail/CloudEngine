#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <string>

//-----------------------------------------------------------------------------------------------
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

struct ID3D12Resource;

//------------------------------------------------------------------------------------------------
class Texture
{
	friend class Renderer; // Only the Renderer can create new Texture objects!
	friend class DX11Renderer; 
	friend class DX12Renderer; 

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
#ifdef ENGINE_RENDER_D3D11
	ID3D11Texture2D* m_texture = nullptr;
	ID3D11ShaderResourceView* m_shaderResourceView = nullptr;
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
	ID3D12Resource* m_texture = nullptr;
	uint32_t m_srvHeapIndex = 0xFFFFFFFF; // offset in srv Heap
#endif // ENGINE_RENDER_D3D12
};
