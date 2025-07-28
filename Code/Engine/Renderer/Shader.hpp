#pragma once
#include "Engine/Renderer/RendererCommon.hpp"
#include <string>
#include <vector>

//-----------------------------------------------------------------------------------------------
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11InputLayout;


/*
	Graphics,   // VS, PS, (GS, HS, DS)
	Compute,    // CS
	Mesh        // MS, AS, PS
*/



class Shader
{
	friend class Renderer;
	friend class DX11Renderer;
	friend class DX12Renderer;
	friend class GraphicsPSO;

public:
	Shader(const ShaderConfig& config);
	Shader(const Shader& copy) = delete;
	~Shader();

	const std::string& GetName() const;

private:
	ShaderConfig m_config;
	
#ifdef ENGINE_RENDER_D3D11
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11GeometryShader* m_geometryShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
	std::vector<unsigned char> m_vertexShaderByteCode;
	std::vector<unsigned char> m_pixelShaderByteCode;
	std::vector<unsigned char> m_geometryShaderByteCode;
	std::vector<unsigned char> m_hullShaderByteCode;
	std::vector<unsigned char> m_domainShaderByteCode;

	VertexType m_inputLayoutMode = VertexType::VERTEX_PCU;
#endif // ENGINE_RENDER_D3D12


};

