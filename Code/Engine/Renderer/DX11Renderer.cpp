#include "Engine/Renderer/DX11Renderer.hpp"

#ifdef ENGINE_RENDER_D3D11

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_win32.h"
#include "ThirdParty/imgui/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

//-----------------------------------------------------------------------------------------------
#if defined(_DEBUG)
#define ENGINE_DEBUG_RENDER
#endif

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#if defined(OPAQUE)
#undef OPAQUE
#endif


#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}


//-----------------------------------------------------------------------------------------------
DX11Renderer::DX11Renderer(RendererConfig const& config)
	: Renderer(config)
{
}

void DX11Renderer::Startup()
{
	InitializeDebugModule();
	CreateDeviceAndSwapChain();
	CreateRenderTargetView();
	CreateDefaultShaderAndTexture();
	CreateBuffers();
	CreateBlendStates();
	CreateSamplerStates();
	CreateRasterizerStates();
	CreateDepthStencilTextureAndView();
	CreateDepthStencilStates();

	//-----------------------------------------------------------------------------------------------
	// Annotations for RenderDoc
	//-----------------------------------------------------------------------------------------------
	HRESULT hr;

	hr = m_deviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<void**>(&m_userDefinedAnnotations));
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create user defined annotations interface!");
	}

	ImGuiStartup();
}

void DX11Renderer::BeginFrame()
{
	// Set render target
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilDSV);
	ImGuiBeginFrame();
}

void DX11Renderer::EndFrame()
{
	ImGuiEndFrame();
	// Present
	HRESULT hr;
	hr = m_swapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		ERROR_AND_DIE("Device has been lost, application will now terminate.")
	}
}

void DX11Renderer::Shutdown()
{
	m_swapChain->SetFullscreenState(FALSE, nullptr);
	ImGuiShutdown();

	for (int textureIndex = 0; textureIndex < (int)m_loadedTextures.size(); ++textureIndex)
	{
		delete m_loadedTextures[textureIndex];
		m_loadedTextures[textureIndex] = nullptr;
	}
	m_defaultTexture = nullptr;
	m_defaultNormalTexture = nullptr;
	m_defaultSpecGlossEmitTexture = nullptr;

	for (int fontIndex = 0; fontIndex < (int)m_loadedFonts.size(); ++fontIndex)
	{
		delete m_loadedFonts[fontIndex];
		m_loadedFonts[fontIndex] = nullptr;
	}

	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		delete m_loadedShaders[shaderIndex];
		m_loadedShaders[shaderIndex] = nullptr;
	}
	//m_currentShader = nullptr;
	m_defaultShader = nullptr;

	delete m_immediateVBO;
	m_immediateVBO = nullptr;
	delete m_immediateIBO;
	m_immediateIBO = nullptr;
	delete m_cameraCBO;
	m_cameraCBO = nullptr;
	delete m_modelCBO;
	m_modelCBO = nullptr;
	delete m_lightCBO;
	m_lightCBO = nullptr;
	delete m_engineCBO;
	m_engineCBO = nullptr;
	delete m_perFrameCBO;
	m_perFrameCBO = nullptr;

	for (int blendIndex = 0; blendIndex < (int)(BlendMode::COUNT); ++blendIndex)
	{
		DX_SAFE_RELEASE(m_blendStates[blendIndex]);
	}
	m_blendState = nullptr;

	for (int samplerIndex = 0; samplerIndex < (int)(SamplerMode::COUNT); ++samplerIndex)
	{
		DX_SAFE_RELEASE(m_samplerStates[samplerIndex]);
	}
	for (int i = 0; i < NUM_SAMPLER_SLOT; ++i)
	{
		m_samplerStateBySlot[i] = nullptr;
	}

	for (int rasterizerIndex = 0; rasterizerIndex < (int)(RasterizerMode::COUNT); ++rasterizerIndex)
	{
		DX_SAFE_RELEASE(m_rasterizerStates[rasterizerIndex]);
	}
	m_rasterizerState = nullptr;

	for (int depthStencilIndex = 0; depthStencilIndex < (int)(DepthMode::COUNT); ++depthStencilIndex)
	{
		DX_SAFE_RELEASE(m_depthStencilStates[depthStencilIndex]);
	}
	m_depthStencilState = nullptr;

	//DX_SAFE_RELEASE(m_rasterizerState);
	DX_SAFE_RELEASE(m_renderTargetView);
	DX_SAFE_RELEASE(m_swapChain);
	DX_SAFE_RELEASE(m_deviceContext);
	DX_SAFE_RELEASE(m_device);
	DX_SAFE_RELEASE(m_depthStencilTexture);
	DX_SAFE_RELEASE(m_depthStencilDSV);
	DX_SAFE_RELEASE(m_userDefinedAnnotations);

	// Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
	((IDXGIDebug*)m_dxgiDebug)->ReportLiveObjects(
		DXGI_DEBUG_ALL,
		(DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
	);

	((IDXGIDebug*)m_dxgiDebug)->Release();
	m_dxgiDebug = nullptr;

	::FreeLibrary((HMODULE)m_dxgiDebugModule);
	m_dxgiDebugModule = nullptr;
#endif
}

void DX11Renderer::ClearScreen(Rgba8 const& clearColor)
{
	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);
	m_deviceContext->ClearDepthStencilView(m_depthStencilDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void DX11Renderer::BeginCamera(Camera const& camera)
{
	SetModelConstants();

	CameraConstants cameraConstants;
	cameraConstants.WorldToCameraTransform = camera.GetWorldToCameraTransform();
	cameraConstants.CameraToRenderTransform = camera.GetCameraToRenderTransform();
	cameraConstants.RenderToClipTransform = camera.GetRenderToClipTransform();
	cameraConstants.CameraWorldPosition = camera.GetPosition();
	CopyCPUToGPU(&cameraConstants, sizeof(CameraConstants), m_cameraCBO);
	BindConstantBuffer(k_cameraConstantsSlot, m_cameraCBO);

	// Set viewport
	D3D11_VIEWPORT viewport = {};
	camera.GetDirectXViewport(Vec2(m_config.m_window->GetClientDimensions()),
		viewport.TopLeftX,
		viewport.TopLeftY,
		viewport.Width,
		viewport.Height);

	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	m_deviceContext->RSSetViewports(1, &viewport);
}

void DX11Renderer::EndCamera(Camera const& camera)
{
	// DO nothing for now
	UNUSED(camera);

}

void DX11Renderer::DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes)
{
	m_immediateVBO->m_stride = sizeof(Vertex_PCU);
	CopyCPUToGPU(vertexes, numVertexes * m_immediateVBO->GetStride(), m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, numVertexes);
}

void DX11Renderer::DrawVertexArray(std::vector<Vertex_PCU> const& verts)
{
	m_immediateVBO->m_stride = sizeof(Vertex_PCU);
	CopyCPUToGPU(verts.data(), static_cast<unsigned int>(verts.size()) * m_immediateVBO->GetStride(), m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, static_cast<unsigned int>(verts.size()));
	//DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void DX11Renderer::DrawVertexArray(std::vector<Vertex_PCUTBN> const& verts)
{
	m_immediateVBO->m_stride = sizeof(Vertex_PCUTBN);
	CopyCPUToGPU(verts.data(), static_cast<unsigned int>(verts.size()) * m_immediateVBO->GetStride(), m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, static_cast<unsigned int>(verts.size()));
}

void DX11Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCU> const& verts, std::vector<unsigned int> const& indexes)
{
	m_immediateVBO->m_stride = sizeof(Vertex_PCU);
	CopyCPUToGPU(verts.data(), static_cast<unsigned int>(verts.size()) * m_immediateVBO->GetStride(), m_immediateVBO);
	CopyCPUToGPU(indexes.data(), static_cast<unsigned int>(indexes.size()) * m_immediateIBO->GetStride(), m_immediateIBO);
	DrawIndexedVertexBuffer(m_immediateVBO, m_immediateIBO, static_cast<unsigned int>(indexes.size()));
}

void DX11Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> const& verts, std::vector<unsigned int> const& indexes)
{
	m_immediateVBO->m_stride = sizeof(Vertex_PCUTBN);
	CopyCPUToGPU(verts.data(), static_cast<unsigned int>(verts.size()) * m_immediateVBO->GetStride(), m_immediateVBO);
	CopyCPUToGPU(indexes.data(), static_cast<unsigned int>(indexes.size()) * m_immediateIBO->GetStride(), m_immediateIBO);
	DrawIndexedVertexBuffer(m_immediateVBO, m_immediateIBO, static_cast<unsigned int>(indexes.size()));
}

Texture* DX11Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}

	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}

Texture* DX11Renderer::CreateOrGetTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(config.m_name.c_str());
	if (existingTexture)
	{
		return existingTexture;
	}
	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureCubeFromSixFaces(config);
	return newTexture;
}

void DX11Renderer::BindTexture(Texture const* texture, int slot /*= 0*/)
{
	if (texture == nullptr)
	{
		if (slot == 1)
		{
			texture = m_defaultNormalTexture;
		}
		if (slot == 2)
		{
			texture = m_defaultSpecGlossEmitTexture;
		}
		else
		{
			texture = m_defaultTexture;
		}
	}

	m_deviceContext->PSSetShaderResources(slot, 1, &texture->m_shaderResourceView);
}

BitmapFont* DX11Renderer::CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	BitmapFont* existingBitmapFont = GetBitmapFont(bitmapFontFilePathWithNoExtension);
	if (existingBitmapFont)
	{
		return existingBitmapFont;
	}
	BitmapFont* newBitmapFont = CreateBitmapFont(bitmapFontFilePathWithNoExtension);
	return newBitmapFont;
}

Shader* DX11Renderer::CreateOrGetShader(char const* shaderName, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	Shader* existingShader = GetShader(shaderName);
	if (existingShader)
	{
		return existingShader;
	}
	Shader* newShader = CreateShader(shaderName, type);
	return newShader;
}

Shader* DX11Renderer::CreateOrGetShader(ShaderConfig const& config, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	return CreateOrGetShader(config.m_name.c_str(), type);
}

void DX11Renderer::AttachGeometryShader(Shader* shader, char const* shaderName)
{
	std::string shaderFilePath(shaderName);
	shaderFilePath = shaderFilePath + ".hlsl";
	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderFilePath);
	GUARANTEE_RECOVERABLE(result >= 0, "Could not read shader.");

	DX_SAFE_RELEASE(shader->m_geometryShader);
	std::vector<unsigned char> geometryShaderByteCode;
	bool isSucceed;
	isSucceed = CompileShaderToByteCode(geometryShaderByteCode, (shader->m_config.m_name).c_str(), shaderSource.c_str(), (shader->m_config.m_geometryEntryPoint).c_str(), "gs_5_0");
	GUARANTEE_RECOVERABLE(isSucceed, "Could not compile geometry shader.");
	shader->m_geometryShader = CreateGeometryShader(geometryShaderByteCode);
}

void DX11Renderer::SetBlendMode(BlendMode blendMode)
{
	m_desiredBlendMode = blendMode;
}

Texture* DX11Renderer::GetTextureForFileName(char const* imageFilePath)
{
	for (int i = 0; i < static_cast<int>(m_loadedTextures.size()); ++i)
	{
		if (m_loadedTextures[i]->m_name == imageFilePath)
		{
			return m_loadedTextures[i];
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------------------------
Texture* DX11Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	Image fileImage(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(fileImage);
	return newTexture;
}

Texture* DX11Renderer::CreateTextureFromImage(Image const& image)
{
	Texture* newTexture = new Texture();
	newTexture->m_name = image.GetImageFilePath();
	newTexture->m_dimensions = image.GetDimensions();

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = image.GetDimensions().x;
	textureDesc.Height = image.GetDimensions().y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureData;
	textureData.pSysMem = image.GetRawData();
	textureData.SysMemPitch = 4 * image.GetDimensions().x;

	HRESULT hr;
	hr = m_device->CreateTexture2D(&textureDesc, &textureData, &newTexture->m_texture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateTextureFromImage failed for \"%s\".", image.GetImageFilePath().c_str()));
	}

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for \"%s\".", image.GetImageFilePath().c_str()));
	}

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

Texture* DX11Renderer::CreateTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config)
{
	Image images[6] = {
	Image(config.m_rightImageFilePath.c_str(), false),
	Image(config.m_leftImageFilePath.c_str(), false),
	Image(config.m_upImageFilePath.c_str(), false),
	Image(config.m_downImageFilePath.c_str(), false),
	Image(config.m_forwardImageFilePath.c_str(), false),
	Image(config.m_backwardImageFilePath.c_str(), false)
	};

	GUARANTEE_OR_DIE(images[0].GetDimensions() == images[1].GetDimensions() &&
		images[1].GetDimensions() == images[2].GetDimensions() &&
		images[2].GetDimensions() == images[3].GetDimensions() &&
		images[3].GetDimensions() == images[4].GetDimensions() &&
		images[4].GetDimensions() == images[5].GetDimensions(),
		"Texture Cube Six Faces do not have same dimensions");

	int width = images[0].GetDimensions().x;
	int height = images[0].GetDimensions().y;
	GUARANTEE_OR_DIE(width == height, "Texture Cube face is not a square");

	Texture* newTexture = new Texture();
	newTexture->m_name = config.m_name;
	newTexture->m_dimensions = images[0].GetDimensions();

	D3D11_SUBRESOURCE_DATA initData[6];
	for (int i = 0; i < 6; ++i) 
	{
		initData[i].pSysMem = images[i].GetRawData();
		initData[i].SysMemPitch = width * 4;
		initData[i].SysMemSlicePitch = initData[i].SysMemPitch * height; // not sure maybe 0?
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	HRESULT hr;
	hr = m_device->CreateTexture2D(&texDesc, initData, &newTexture->m_texture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create texture cube");
	}

	//D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = texDesc.Format;
	//srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	//srvDesc.TextureCube.MipLevels = 1;
	//srvDesc.TextureCube.MostDetailedMip = 0;

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create shader resource view for texture cube");
	}

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

BitmapFont* DX11Renderer::GetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	for (int i = 0; i < static_cast<int>(m_loadedFonts.size()); ++i)
	{
		if (m_loadedFonts[i]->m_fontFilePathNameWithNoExtension == bitmapFontFilePathWithNoExtension)
		{
			return m_loadedFonts[i];
		}
	}
	return nullptr;
}

BitmapFont* DX11Renderer::CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	std::string imageFilePath(bitmapFontFilePathWithNoExtension);
	imageFilePath += ".png";
	Texture* fontTexture = CreateOrGetTextureFromFile(imageFilePath.c_str());
	BitmapFont* newFont = new BitmapFont(bitmapFontFilePathWithNoExtension, *fontTexture);
	m_loadedFonts.push_back(newFont);
	return newFont;
}

Shader* DX11Renderer::GetShader(char const* shaderName)
{
	for (int i = 0; i < static_cast<int>(m_loadedShaders.size()); ++i)
	{
		if (m_loadedShaders[i]->m_config.m_name == shaderName)
		{
			return m_loadedShaders[i];
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------------------------
Shader* DX11Renderer::CreateShader(char const* shaderName, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	std::string shaderFilePath(shaderName);
	shaderFilePath = shaderFilePath + ".hlsl";
	//shaderFilePath = "Data/Shaders/" + shaderFilePath + ".hlsl";
	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderFilePath);
	GUARANTEE_RECOVERABLE(result >= 0, "Could not read shader.");
	return CreateShader(shaderName, shaderSource.c_str(), type);
}

Shader* DX11Renderer::CreateShader(char const* shaderName, char const* shaderSource, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	ShaderConfig shaderConfig;
	shaderConfig.m_name = shaderName;
	Shader* shader = new Shader(shaderConfig);

	std::vector<unsigned char> vertexShaderByteCode;
	bool isSucceed;
	isSucceed = CompileShaderToByteCode(vertexShaderByteCode, (shader->m_config.m_name).c_str(), shaderSource, (shader->m_config.m_vertexEntryPoint).c_str(), "vs_5_0");
	GUARANTEE_RECOVERABLE(isSucceed, "Could not compile vertex shader.");
	shader->m_vertexShader = CreateVertexShader(vertexShaderByteCode);

	shader->m_inputLayout = CreateInputLayout(vertexShaderByteCode, type);

	std::vector<unsigned char> pixelShaderByteCode;
	isSucceed = CompileShaderToByteCode(pixelShaderByteCode, (shader->m_config.m_name).c_str(), shaderSource, (shader->m_config.m_pixelEntryPoint).c_str(), "ps_5_0");
	GUARANTEE_RECOVERABLE(isSucceed, "Could not compile pixel shader.");
	shader->m_pixelShader = CreatePixelShader(pixelShaderByteCode);


	m_loadedShaders.push_back(shader);

	return shader;
}

bool DX11Renderer::CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name, char const* source, char const* entryPoint, char const* target)
{
	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
	ID3DBlob* shaderBlob = NULL;
	ID3DBlob* errorBlob = NULL;

	HRESULT hr;
	hr = D3DCompile(
		source, strlen(source),
		name, nullptr, nullptr,
		entryPoint, target, shaderFlags,
		0, &shaderBlob, &errorBlob);
	if (SUCCEEDED(hr))
	{
		outByteCode.resize(shaderBlob->GetBufferSize());
		memcpy(
			outByteCode.data(),
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize()
		);
	}
	else
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
		}
		ERROR_AND_DIE(Stringf("Could not compile shader."));
	}

	shaderBlob->Release();
	if (errorBlob != NULL)
	{
		errorBlob->Release();
	}

	return true;
}

ID3D11VertexShader* DX11Renderer::CreateVertexShader(std::vector<unsigned char> const& byteCode)
{
	ID3D11VertexShader* result;
	HRESULT hr;
	hr = m_device->CreateVertexShader(
		byteCode.data(),
		byteCode.size(),
		NULL, &result
	);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Could not create vertex shader."));
	}
	return result;
}

ID3D11PixelShader* DX11Renderer::CreatePixelShader(std::vector<unsigned char> const& byteCode)
{
	ID3D11PixelShader* result;
	HRESULT hr;
	hr = m_device->CreatePixelShader(
		byteCode.data(),
		byteCode.size(),
		NULL, &result
	);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Could not create pixel shader."));
	}
	return result;
}

ID3D11GeometryShader* DX11Renderer::CreateGeometryShader(std::vector<unsigned char> const& byteCode)
{
	ID3D11GeometryShader* result;
	HRESULT hr;
	hr = m_device->CreateGeometryShader(
		byteCode.data(),
		byteCode.size(),
		NULL, &result
	);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("Could not create geometry shader."));
	}
	return result;
}

ID3D11InputLayout* DX11Renderer::CreateInputLayout(std::vector<unsigned char> const& byteCode, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	if (type == VertexType::VERTEX_PCU)
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
				0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,
				0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
				0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};


		ID3D11InputLayout* result;
		UINT numElements = ARRAYSIZE(inputElementDesc); // 3
		HRESULT hr;
		hr = m_device->CreateInputLayout(
			inputElementDesc, numElements,
			byteCode.data(), // will check the shader-input signature of the shader
			byteCode.size(),
			&result
		);
		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE("Could not create input layout.");
		}
		return result;
	}
	else if (type == VertexType::VERTEX_PCUTBN)
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		ID3D11InputLayout* result;
		UINT numElements = ARRAYSIZE(inputElementDesc);
		HRESULT hr;
		hr = m_device->CreateInputLayout(
			inputElementDesc, numElements,
			byteCode.data(),
			byteCode.size(),
			&result
		);
		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE("Could not create input layout.");
		}
		return result;
	}

	ERROR_AND_DIE("Could not create vertex layout. Not supported VertexType");
}

void DX11Renderer::BindShader(Shader* shader)
{
	if (shader == nullptr)
	{
		shader = m_defaultShader;
	}

	m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
	m_deviceContext->GSSetShader(shader->m_geometryShader, nullptr, 0);
	m_deviceContext->IASetInputLayout(shader->m_inputLayout);
	//m_currentShader = shader;
}


VertexBuffer* DX11Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
	return new VertexBuffer(m_device, size, stride);
}

IndexBuffer* DX11Renderer::CreateIndexBuffer(const unsigned int size)
{
	return new IndexBuffer(m_device, size);
}

//-----------------------------------------------------------------------------------------------
void DX11Renderer::CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo)
{
	if (vbo->GetSize() < size)
	{
		vbo->Resize(size);
	}

	D3D11_MAPPED_SUBRESOURCE resource;
	HRESULT hr;
	hr = m_deviceContext->Map(vbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not copy cpu to gpu.");
	}
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(vbo->m_buffer, 0);
}

void DX11Renderer::CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo)
{
	if (ibo->GetSize() < size)
	{
		ibo->Resize(size);
	}

	D3D11_MAPPED_SUBRESOURCE resource;
	HRESULT hr;
	hr = m_deviceContext->Map(ibo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not copy cpu to gpu.");
	}
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(ibo->m_buffer, 0);
}

ConstantBuffer* DX11Renderer::CreateConstantBuffer(const unsigned int size)
{
	return new ConstantBuffer(m_device, size);
}

void DX11Renderer::CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	HRESULT hr;
	hr = m_deviceContext->Map(cbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not copy cpu to gpu.");
	}
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(cbo->m_buffer, 0);
}

void DX11Renderer::BindConstantBuffer(int slot, ConstantBuffer* cbo)
{
	m_deviceContext->VSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->PSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->GSSetConstantBuffers(slot, 1, &cbo->m_buffer);
}

void DX11Renderer::BindPrimitiveTopology()
{
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // later will add more type
}

void DX11Renderer::SetStatesIfChanged()
{
	if (m_blendStates[(int)(m_desiredBlendMode)] != m_blendState)
	{
		m_blendState = m_blendStates[(int)(m_desiredBlendMode)];
		float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		UINT sampleMask = 0xffffffff;
		m_deviceContext->OMSetBlendState(m_blendState, blendFactor, sampleMask);
	}

	for (int i = 0; i < NUM_SAMPLER_SLOT; ++i)
	{
		if (m_samplerStates[(int)(m_desiredSamplerModeBySlot[i])] != m_samplerStateBySlot[i])
		{
			m_samplerStateBySlot[i] = m_samplerStates[(int)(m_desiredSamplerModeBySlot[i])];
			m_deviceContext->PSSetSamplers(i, 1, &m_samplerStateBySlot[i]);
		}
	}

	if (m_rasterizerStates[(int)(m_desiredRasterizerMode)] != m_rasterizerState)
	{
		m_rasterizerState = m_rasterizerStates[(int)(m_desiredRasterizerMode)];
		m_deviceContext->RSSetState(m_rasterizerState);
	}

	if (m_depthStencilStates[(int)(m_desiredDepthMode)] != m_depthStencilState)
	{
		m_depthStencilState = m_depthStencilStates[(int)(m_desiredDepthMode)];
		m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);
	}

}

void DX11Renderer::SetPerFrameConstants(PerFrameConstants const& perframeConstants)
{
	CopyCPUToGPU(&perframeConstants, sizeof(PerFrameConstants), m_perFrameCBO);
	BindConstantBuffer(k_perFrameConstantsSlot, m_perFrameCBO);
}

void DX11Renderer::SetModelConstants(Mat44 const& modelToWorldTransform /*= Mat44()*/, Rgba8 const& modelColor /*= Rgba8::OPAQUE_WHITE*/)
{
	ModelConstants modelConstants;

	modelConstants.ModelToWorldTransform = modelToWorldTransform;
	modelColor.GetAsFloats(modelConstants.ModelColor);

	CopyCPUToGPU(&modelConstants, sizeof(ModelConstants), m_modelCBO);
	BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
}

void DX11Renderer::SetLightConstants(Vec3 const& sunDirection /*= Vec3(2.f, -1.f, -1.f)*/, float sunIntensity /*= 0.85f*/, float ambientIntensity /*= 0.35f*/)
{
	// Deprecated: It only keeps sun light
	UNUSED(ambientIntensity);
	LightConstants lightConstants;

	Rgba8 color = Rgba8::OPAQUE_WHITE;
	color.ScaleAlpha(sunIntensity);
	color.GetAsFloats(lightConstants.m_sunColor);
	lightConstants.m_sunNormal = sunDirection.GetNormalized();

	CopyCPUToGPU(&lightConstants, sizeof(LightConstants), m_lightCBO);
	BindConstantBuffer(k_lightConstantsSlot, m_lightCBO);
}


void DX11Renderer::SetLightConstants(LightConstants const& lightConstants)
{
	CopyCPUToGPU(&lightConstants, sizeof(LightConstants), m_lightCBO);
	BindConstantBuffer(k_lightConstantsSlot, m_lightCBO);
}

//void DX11Renderer::SetSamplerMode(SamplerMode samplerMode)
//{
//	m_desiredSamplerMode = samplerMode;
//}

void DX11Renderer::SetSamplerMode(SamplerMode samplerMode, int slot /*= 0*/)
{
	m_desiredSamplerModeBySlot[slot] = samplerMode;
}

void DX11Renderer::SetRasterizerMode(RasterizerMode rasterizerMode)
{
	m_desiredRasterizerMode = rasterizerMode;
}

void DX11Renderer::SetDepthMode(DepthMode depthMode)
{
	m_desiredDepthMode = depthMode;
}

void DX11Renderer::SetRenderTargetFormats(const std::vector<DXGI_FORMAT>& rtvFormats /*= { DXGI_FORMAT_R8G8B8A8_UNORM }*/, DXGI_FORMAT dsvFormat /*= DXGI_FORMAT_D24_UNORM_S8_UINT*/, unsigned int msaaCount /*= 1*/, unsigned int msaaQuality /*= 0*/)
{
	UNUSED(rtvFormats);
	UNUSED(dsvFormat);
	UNUSED(msaaCount);
	UNUSED(msaaQuality);
}

void DX11Renderer::SetEngineConstants(int debugInt /*= 0*/, float debugFloat /*= 0.f*/)
{
	EngineConstants engineConstants;

	engineConstants.m_debugInt = debugInt;
	engineConstants.m_debugFloat = debugFloat;

	CopyCPUToGPU(&engineConstants, sizeof(EngineConstants), m_engineCBO);
	BindConstantBuffer(k_engineConstantsSlot, m_engineCBO);
}

void DX11Renderer::BindVertexBuffer(VertexBuffer* vbo)
{
	UINT stride = vbo->GetStride();
	UINT startOffset = 0;
	m_deviceContext->IASetVertexBuffers(0, 1, &vbo->m_buffer, &stride, &startOffset);
}

void DX11Renderer::BindIndexBuffer(IndexBuffer* ibo)
{
	m_deviceContext->IASetIndexBuffer(ibo->m_buffer, DXGI_FORMAT_R32_UINT, 0); // unsigned int 4 byte
}

void DX11Renderer::DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount)
{
	BindVertexBuffer(vbo);
	BindPrimitiveTopology();

	SetStatesIfChanged();
	m_deviceContext->Draw(vertexCount, 0);
}

void DX11Renderer::DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount)
{
	BindVertexBuffer(vbo);
	BindIndexBuffer(ibo);
	BindPrimitiveTopology();

	SetStatesIfChanged();
	m_deviceContext->DrawIndexed(indexCount, 0, 0);
}

//-----------------------------------------------------------------------------------------------
// Renderer Startup
//-----------------------------------------------------------------------------------------------
void DX11Renderer::InitializeDebugModule()
{
	//-----------------------------------------------------------------------------------------------
	// Create debug module 
	// before creating device and swap chain
	//-----------------------------------------------------------------------------------------------
#if defined(ENGINE_DEBUG_RENDER)
	m_dxgiDebugModule = (void*)::LoadLibraryA("dxgidebug.dll");
	if (m_dxgiDebugModule == nullptr)
	{
		ERROR_AND_DIE("Could not load dxgidebug.dll.");
	}

	typedef HRESULT(WINAPI* GetDebugModuleCB)(REFIID, void**);
	((GetDebugModuleCB)::GetProcAddress((HMODULE)m_dxgiDebugModule, "DXGIGetDebugInterface"))
		(__uuidof(IDXGIDebug), &m_dxgiDebug);

	if (m_dxgiDebug == nullptr)
	{
		ERROR_AND_DIE("Could not load debug module.");
	}
#endif
}

void DX11Renderer::CreateDeviceAndSwapChain()
{
	//-----------------------------------------------------------------------------------------------
	// Create device and swap chain
	//-----------------------------------------------------------------------------------------------
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = m_config.m_window->GetClientDimensions().x;
	swapChainDesc.BufferDesc.Height = m_config.m_window->GetClientDimensions().y;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = (HWND)m_config.m_window->GetHwnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Call D3D11 to create device and swap chain
	unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDER)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags,
		nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc,
		&m_swapChain, &m_device, nullptr, &m_deviceContext);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create D3D 11 device and swap chain.");
	}
}

void DX11Renderer::CreateRenderTargetView()
{
	//-----------------------------------------------------------------------------------------------
	// SAVE BACK BUFFER VIEW
	// Do this just after creating the device and swap chain
	// Bind back buffer to the render target
	//-----------------------------------------------------------------------------------------------
	// Get back buffer texture
	HRESULT hr;

	ID3D11Texture2D* backBuffer;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not get swap chain buffer.");
	}

	hr = m_device->CreateRenderTargetView(backBuffer, NULL, &m_renderTargetView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could create render target view for swap chain buffer.");
	}

	backBuffer->Release();
}

void DX11Renderer::CreateDefaultShaderAndTexture()
{
	//-----------------------------------------------------------------------------------------------
	// Create and Bind shader
	//-----------------------------------------------------------------------------------------------
	m_defaultShader = CreateShader("Default", g_defaultShaderSource);
	BindShader(m_defaultShader);

	//-----------------------------------------------------------------------------------------------
	m_defaultTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8::OPAQUE_WHITE, "DefaultDiffuse"));
	BindTexture(m_defaultTexture, 0);
	//-----------------------------------------------------------------------------------------------
	m_defaultNormalTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 255, 255), "DefaultNormal"));
	BindTexture(m_defaultNormalTexture, 1);

	m_defaultSpecGlossEmitTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 0, 255), "DefaultSpecGlossEmit"));
	BindTexture(m_defaultSpecGlossEmitTexture, 2);

}

void DX11Renderer::CreateBuffers()
{
	m_immediateVBO = CreateVertexBuffer(1 * sizeof(Vertex_PCU), sizeof(Vertex_PCU));
	m_immediateIBO = CreateIndexBuffer(1 * sizeof(unsigned int));
	m_engineCBO = CreateConstantBuffer(sizeof(EngineConstants));
	m_perFrameCBO = CreateConstantBuffer(sizeof(PerFrameConstants));
	m_lightCBO = CreateConstantBuffer(sizeof(LightConstants));
	m_cameraCBO = CreateConstantBuffer(sizeof(CameraConstants));
	m_modelCBO = CreateConstantBuffer(sizeof(ModelConstants));
}

void DX11Renderer::CreateBlendStates()
{
	//-----------------------------------------------------------------------------------------------
	// Create Blend States
	//-----------------------------------------------------------------------------------------------
	HRESULT hr;

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::OPAQUE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::OPAQUE failed");
	}

	blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ALPHA)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ALPHA failed");
	}

	blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ADDITIVE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ADDITIVE failed");
	}
}

void DX11Renderer::CreateSamplerStates()
{
	//-----------------------------------------------------------------------------------------------
	// Create Sampler States
	//-----------------------------------------------------------------------------------------------
	HRESULT hr;

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_device->CreateSamplerState(&samplerDesc,
		&m_samplerStates[(int)(SamplerMode::POINT_CLAMP)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for SamplerMode::POINT_CLAMP failed.");
	}

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hr = m_device->CreateSamplerState(&samplerDesc,
		&m_samplerStates[(int)(SamplerMode::BILINEAR_WRAP)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for SamplerMode::BILINEAR_WRAP failed.");
	}
}

void DX11Renderer::CreateRasterizerStates()
{
	//-----------------------------------------------------------------------------------------------
	// Create Rasterizer States
	//-----------------------------------------------------------------------------------------------
	HRESULT hr;

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.AntialiasedLineEnable = true;
	//rasterizerDesc.FrontCounterClockwise = false;
	//rasterizerDesc.DepthBias = 0;
	//rasterizerDesc.DepthBiasClamp = 0.0f;
	//rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	//rasterizerDesc.DepthClipEnable = true;
	//rasterizerDesc.ScissorEnable = false;
	//rasterizerDesc.MultisampleEnable = false;
	//rasterizerDesc.AntialiasedLineEnable = true;

	hr = m_device->CreateRasterizerState(&rasterizerDesc,
		&m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_NONE]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::SOLID_CULL_NONE failed.");
	}

	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;

	hr = m_device->CreateRasterizerState(&rasterizerDesc,
		&m_rasterizerStates[(int)RasterizerMode::SOLID_CULL_BACK]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::SOLID_CULL_BACK failed.");
	}

	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;

	hr = m_device->CreateRasterizerState(&rasterizerDesc,
		&m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_NONE]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::WIREFRAME_CULL_NONE failed.");
	}

	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;

	hr = m_device->CreateRasterizerState(&rasterizerDesc,
		&m_rasterizerStates[(int)RasterizerMode::WIREFRAME_CULL_BACK]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::WIREFRAME_CULL_BACK failed.");
	}

}

void DX11Renderer::CreateDepthStencilTextureAndView()
{
	//-----------------------------------------------------------------------------------------------
	// Create depth stencil texture and view
	//-----------------------------------------------------------------------------------------------
	HRESULT hr;

	D3D11_TEXTURE2D_DESC depthTextureDesc = {};
	depthTextureDesc.Width = m_config.m_window->GetClientDimensions().x;
	depthTextureDesc.Height = m_config.m_window->GetClientDimensions().y;
	depthTextureDesc.MipLevels = 1;
	depthTextureDesc.ArraySize = 1;
	depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTextureDesc.SampleDesc.Count = 1;

	hr = m_device->CreateTexture2D(&depthTextureDesc, nullptr, &m_depthStencilTexture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create texture for depth stencil.");
	}

	hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilDSV);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create depth stencil view.");
	}

}

void DX11Renderer::CreateDepthStencilStates()
{
	//-----------------------------------------------------------------------------------------------
	// Create depth stencil states
	//-----------------------------------------------------------------------------------------------
	HRESULT hr;

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	hr = m_device->CreateDepthStencilState(&depthStencilDesc,
		&m_depthStencilStates[(int)DepthMode::DISABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::DISABLED failed.");
	}

	depthStencilDesc.DepthEnable = TRUE;

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc,
		&m_depthStencilStates[(int)DepthMode::READ_ONLY_ALWAYS]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_ONLY_ALWAYS failed.");
	}

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc,
		&m_depthStencilStates[(int)DepthMode::READ_ONLY_LESS_EQUAL]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_ONLY_LESS_EQUAL failed.");
	}

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc,
		&m_depthStencilStates[(int)DepthMode::READ_WRITE_LESS_EQUAL]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_WRITE_LESS_EQUAL failed.");
	}
}

//-----------------------------------------------------------------------------------------------
void DX11Renderer::BeginRenderEvent(char const* eventName)
{
	int eventNameLength = (int)strlen(eventName) + 1;
	int eventNameWideCharLength = MultiByteToWideChar(CP_UTF8, 0, eventName, eventNameLength, NULL, 0);

	wchar_t* eventNameWideCharStr = new wchar_t[eventNameWideCharLength];
	MultiByteToWideChar(CP_UTF8, 0, eventName, eventNameLength, eventNameWideCharStr, eventNameWideCharLength);

	m_userDefinedAnnotations->BeginEvent(eventNameWideCharStr);
}

void DX11Renderer::EndRenderEvent(char const* optional_eventName)
{
	UNUSED(optional_eventName);
	m_userDefinedAnnotations->EndEvent();
}


void DX11Renderer::OnResize()
{
	if (m_device == nullptr) return;
	DX_SAFE_RELEASE(m_renderTargetView);
	DX_SAFE_RELEASE(m_depthStencilTexture);
	DX_SAFE_RELEASE(m_depthStencilDSV);

	unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDER)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr;
	hr = m_swapChain->ResizeBuffers(2, 
		m_config.m_window->GetClientDimensions().x, m_config.m_window->GetClientDimensions().y,
		DXGI_FORMAT_R8G8B8A8_UNORM, deviceFlags);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not resize swapchain buffer");

	CreateRenderTargetView();
	CreateDepthStencilTextureAndView();

	FireEvent(WINDOW_RESIZE_EVENT);


}

void DX11Renderer::ImGuiStartup()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	ImGui_ImplWin32_Init((HWND)m_config.m_window->GetHwnd());
	ImGui_ImplDX11_Init(m_device, m_deviceContext);
}

void DX11Renderer::ImGuiBeginFrame()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	//bool show_demo_window = true;
	//if (show_demo_window)
	//	ImGui::ShowDemoWindow(&show_demo_window);
}

void DX11Renderer::ImGuiEndFrame()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void DX11Renderer::ImGuiShutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

#endif // ENGINE_RENDER_D3D11
