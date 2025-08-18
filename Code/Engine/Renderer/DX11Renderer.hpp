#pragma once
#include "Engine/Renderer/Renderer.hpp"

#ifdef ENGINE_RENDER_D3D11
//-----------------------------------------------------------------------------------------------
// Forward Declarations - DirectX
//-----------------------------------------------------------------------------------------------
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11InputLayout;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct ID3D11DepthStencilState;
struct ID3DUserDefinedAnnotation;


class DX11Renderer : public Renderer
{
public:
	DX11Renderer(RendererConfig const& config);
	virtual ~DX11Renderer() = default;
	void Startup() override;
	void BeginFrame() override;
	void EndFrame() override;
	void Shutdown() override;

	void ClearScreen(Rgba8 const& clearColor) override;
	void BeginCamera(Camera const& camera) override;
	void EndCamera(Camera const& camera) override;


public:
	VertexBuffer* CreateVertexBuffer(const unsigned int size, unsigned int stride) override;
	IndexBuffer* CreateIndexBuffer(const unsigned int size) override;
	ConstantBuffer* CreateConstantBuffer(const unsigned int size) override;


	void CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo) override;
	void CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo) override;
	void CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo) override;

	// Draw Calls
	void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes) override;
	void DrawVertexArray(std::vector<Vertex_PCU> const& verts) override;
	void DrawVertexArray(std::vector<Vertex_PCUTBN> const& verts) override;
	void DrawIndexedVertexArray(std::vector<Vertex_PCU> const& verts, std::vector<unsigned int> const& indexes) override;
	void DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> const& verts, std::vector<unsigned int> const& indexes) override;


	void DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount) override;
	void DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount) override;

protected:
	void BindVertexBuffer(VertexBuffer* vbo);
	void BindIndexBuffer(IndexBuffer* ibo);
	void BindPrimitiveTopology();



public:
	//-----------------------------------------------------------------------------------------------
	// Set Renderer States
	//-----------------------------------------------------------------------------------------------
	void BindTexture(Texture const* texture, int slot = 0) override;
	void BindShader(Shader* shader) override;
	void SetBlendMode(BlendMode blendMode) override;
	void SetSamplerMode(SamplerMode samplerMode, int slot = 0) override;
	void SetRasterizerMode(RasterizerMode rasterizerMode) override;
	void SetDepthMode(DepthMode depthMode) override;

	void SetRenderTargetFormats(const std::vector<DXGI_FORMAT>& rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		unsigned int msaaCount = 1,
		unsigned int msaaQuality = 0) override;

	void SetEngineConstants(int debugInt = 0, float debugFloat = 0.f) override;
	void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::OPAQUE_WHITE) override;
	void SetLightConstants(Vec3 const& sunDirection = Vec3(2.f, -1.f, -1.f), float sunIntensity = 0.85f, float ambientIntensity = 0.35f) override;
	void SetLightConstants(LightConstants const& lightConstants) override;
	void SetPerFrameConstants(PerFrameConstants const& perframeConstants) override;

protected:
	void SetStatesIfChanged();
	void BindConstantBuffer(int slot, ConstantBuffer* cbo);

public:
	//-----------------------------------------------------------------------------------------------
	// Manage Resources - Public
	//-----------------------------------------------------------------------------------------------
	Texture* CreateOrGetTextureFromFile(char const* imageFilePath) override;
	Texture* CreateOrGetTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config) override;

	BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension) override;

	Shader* CreateOrGetShader(char const* shaderName, VertexType type = VertexType::VERTEX_PCU) override;

	void AttachGeometryShader(Shader* shader, char const* shaderName) override;

	Shader* CreateOrGetShader(ShaderConfig const& config, VertexType type = VertexType::VERTEX_PCU) override;

protected:
	//-----------------------------------------------------------------------------------------------
	// Manage Resources - Protected
	//-----------------------------------------------------------------------------------------------
	Texture* GetTextureForFileName(char const* imageFilePath);
	Texture* CreateTextureFromFile(char const* imageFilePath);
	Texture* CreateTextureFromImage(Image const& image);
	Texture* CreateTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config);

	BitmapFont* GetBitmapFont(char const* bitmapFontFilePathWithNoExtension);
	BitmapFont* CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension);

	Shader* GetShader(char const* shaderName);
	Shader* CreateShader(char const* shaderName, VertexType type = VertexType::VERTEX_PCU);
	Shader* CreateShader(char const* shaderName, char const* shaderSource, VertexType type = VertexType::VERTEX_PCU);
	bool CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name,
		char const* source, char const* entryPoint, char const* target);
	ID3D11VertexShader* CreateVertexShader(std::vector<unsigned char> const& byteCode);
	ID3D11PixelShader* CreatePixelShader(std::vector<unsigned char> const& byteCode);
	ID3D11GeometryShader* CreateGeometryShader(std::vector<unsigned char> const& byteCode);
	ID3D11InputLayout* CreateInputLayout(std::vector<unsigned char> const& byteCode, VertexType type = VertexType::VERTEX_PCU);

protected:
	//-----------------------------------------------------------------------------------------------
	// Manage Resources - data
	//-----------------------------------------------------------------------------------------------
	std::vector<Texture*> m_loadedTextures;
	std::vector<BitmapFont*> m_loadedFonts;
	std::vector<Shader*> m_loadedShaders;
	const Texture* m_defaultTexture = nullptr;
	const Texture* m_defaultNormalTexture = nullptr;
	const Texture* m_defaultSpecGlossEmitTexture = nullptr;
	//Shader* m_currentShader = nullptr; // No one use?
	Shader* m_defaultShader = nullptr;


protected:
	//-----------------------------------------------------------------------------------------------
	// Render Startup - Break down
	//-----------------------------------------------------------------------------------------------
	void InitializeDebugModule();
	void CreateDeviceAndSwapChain();
	void CreateRenderTargetView();
	void CreateDefaultShaderAndTexture();
	void CreateBuffers();
	void CreateBlendStates();
	void CreateSamplerStates();
	void CreateRasterizerStates();
	void CreateDepthStencilTextureAndView();
	void CreateDepthStencilStates();




protected:
#if defined(ENGINE_DEBUG_RENDER)
	void* m_dxgiDebug = nullptr;
	void* m_dxgiDebugModule = nullptr;
#endif

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;
	IDXGISwapChain* m_swapChain = nullptr;
	ID3D11RenderTargetView* m_renderTargetView = nullptr;
	ID3D11Texture2D* m_depthStencilTexture = nullptr;
	ID3D11DepthStencilView* m_depthStencilDSV = nullptr;

	VertexBuffer* m_immediateVBO = nullptr;
	IndexBuffer* m_immediateIBO = nullptr;

	ConstantBuffer* m_engineCBO = nullptr;
	ConstantBuffer* m_perFrameCBO = nullptr;
	ConstantBuffer* m_cameraCBO = nullptr;
	ConstantBuffer* m_modelCBO = nullptr;
	ConstantBuffer* m_lightCBO = nullptr;

	static constexpr int NUM_SAMPLER_SLOT = 3;

	ID3D11BlendState* m_blendState = nullptr;
	ID3D11SamplerState* m_samplerStateBySlot[NUM_SAMPLER_SLOT] = {};

	ID3D11RasterizerState* m_rasterizerState = nullptr;
	ID3D11DepthStencilState* m_depthStencilState = nullptr;

	BlendMode       m_desiredBlendMode = BlendMode::ALPHA;
	SamplerMode     m_desiredSamplerMode = SamplerMode::POINT_CLAMP;
	SamplerMode     m_desiredSamplerModeBySlot[NUM_SAMPLER_SLOT] = {};

	RasterizerMode  m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	DepthMode       m_desiredDepthMode = DepthMode::READ_WRITE_LESS_EQUAL;

	ID3D11BlendState* m_blendStates[(int)(BlendMode::COUNT)] = {};
	ID3D11SamplerState* m_samplerStates[(int)(SamplerMode::COUNT)] = {};
	ID3D11RasterizerState* m_rasterizerStates[(int)(RasterizerMode::COUNT)] = {};
	ID3D11DepthStencilState* m_depthStencilStates[(int)(DepthMode::COUNT)] = {};

public:
	//-----------------------------------------------------------------------------------------------
	// For RenderDoc
	//-----------------------------------------------------------------------------------------------
	void BeginRenderEvent(char const* eventName) override;
	void EndRenderEvent(char const* optional_eventName) override;

protected:
	ID3DUserDefinedAnnotation* m_userDefinedAnnotations = nullptr;

public:
	void OnResize() override;

protected:
	//-----------------------------------------------------------------------------------------------
	// ImGui
	//-----------------------------------------------------------------------------------------------
	void ImGuiStartup();
	void ImGuiBeginFrame();
	void ImGuiEndFrame();
	void ImGuiShutdown();

};

#endif // ENGINE_RENDER_D3D11
