#include "Engine/Renderer/Shader.hpp"

#ifdef ENGINE_RENDER_D3D11
#include <d3d11.h>
#endif // ENGINE_RENDER_D3D11

#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}

Shader::Shader(const ShaderConfig& config)
	: m_config(config)
{

}

Shader::~Shader()
{
#ifdef ENGINE_RENDER_D3D11
	DX_SAFE_RELEASE(m_vertexShader);
	DX_SAFE_RELEASE(m_pixelShader);
	DX_SAFE_RELEASE(m_geometryShader);
	DX_SAFE_RELEASE(m_inputLayout);
#endif // ENGINE_RENDER_D3D11
}

const std::string& Shader::GetName() const
{
	return m_config.m_name;
}
