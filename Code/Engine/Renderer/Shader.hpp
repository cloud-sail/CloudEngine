#pragma once
#include <string>

//-----------------------------------------------------------------------------------------------
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11InputLayout;


#include <d3dcommon.h>

//-----------------------------------------------------------------------------------------------
struct ShaderConfig
{
	std::string m_name;
	std::string m_vertexEntryPoint = "VertexMain";
	std::string m_pixelEntryPoint = "PixelMain";
	std::string m_geometryEntryPoint = "GeometryMain";
	// May be store shader target name here
};

class Shader
{
	friend class Renderer;
	friend class GraphicsPSO;

public:
	Shader(const ShaderConfig& config);
	Shader(const Shader& copy) = delete;
	~Shader();

	const std::string& GetName() const;

private:
	ShaderConfig m_config;
	
	// DX11
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11GeometryShader* m_geometryShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;

	// DX12
	ID3DBlob* m_VS = nullptr;
	ID3DBlob* m_PS = nullptr;

};

