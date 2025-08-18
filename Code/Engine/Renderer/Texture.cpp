#include "Engine/Renderer/Texture.hpp"


#ifdef ENGINE_RENDER_D3D11
#include <d3d11.h>

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
	DX_SAFE_RELEASE(m_texture);
	DX_SAFE_RELEASE(m_shaderResourceView);
}

#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
#include "Engine/Renderer/DX12Renderer.hpp"
#include <d3d12.h>


#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}

Texture::Texture(DX12Renderer* renderer)
	: m_renderer(renderer)
{

}

Texture::~Texture()
{
	if (m_resource)
	{
		m_renderer->EnqueueDeferredRelease(m_resource);
	}
}
#endif // ENGINE_RENDER_D3D12
