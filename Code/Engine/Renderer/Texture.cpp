#include "Engine/Renderer/Texture.hpp"

#ifdef ENGINE_RENDER_D3D11
#include <d3d11.h>
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
#include <d3d12.h>
#endif // ENGINE_RENDER_D3D12


#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}


Texture::Texture()
{
}

Texture::~Texture()
{
#ifdef ENGINE_RENDER_D3D11
	DX_SAFE_RELEASE(m_texture);
	DX_SAFE_RELEASE(m_shaderResourceView);
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
	DX_SAFE_RELEASE(m_texture);
#endif // ENGINE_RENDER_D3D12
}
