#pragma once
//#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/RendererCommon.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <vector>

#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}

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

struct IntVec2;
struct Rgba8;
struct Vertex_PCU;
struct Vertex_PCUTBN;



//-----------------------------------------------------------------------------------------------
// Renderer
//-----------------------------------------------------------------------------------------------
class Renderer
{
public:
    Renderer(RendererConfig const& config);
    ~Renderer() = default;
    void Startup();
    void BeginFrame();
    void EndFrame();
    void Shutdown();

    void ClearScreen(Rgba8 const& clearColor);
    void BeginCamera(Camera const& camera);
    void EndCamera(Camera const& camera);


public:
	VertexBuffer* CreateVertexBuffer(const unsigned int size, unsigned int stride);
	IndexBuffer* CreateIndexBuffer(const unsigned int size);
	ConstantBuffer* CreateConstantBuffer(const unsigned int size);

	void BindVertexBuffer(VertexBuffer* vbo);
	void BindIndexBuffer(IndexBuffer* ibo);
	void BindPrimitiveTopology();

	void CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo);
	void CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo);
	void CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo);

    // Draw Calls
	void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes);
	void DrawVertexArray(std::vector<Vertex_PCU> const& verts);
    void DrawVertexArray(std::vector<Vertex_PCUTBN> const& verts);
	void DrawIndexedVertexArray(std::vector<Vertex_PCU> const& verts, std::vector<unsigned int> const& indexes);
	void DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> const& verts, std::vector<unsigned int> const& indexes);


	void DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount);
	void DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount);

public:
    //-----------------------------------------------------------------------------------------------
    // Set Renderer States
    //-----------------------------------------------------------------------------------------------
    //void SetViewPort(AABB2 const& viewport); // after Begin Camera, Direct X coordinates // keep the original code for old project
    // 0-diffuse 1-normal
    void BindTexture(Texture const* texture, int slot = 0);
    void BindShader(Shader* shader);
    void SetBlendMode(BlendMode blendMode);
	//void SetSamplerMode(SamplerMode samplerMode);
	void SetSamplerMode(SamplerMode samplerMode, int slot = 0);
	void SetRasterizerMode(RasterizerMode rasterizerMode);
	void SetDepthMode(DepthMode depthMode);
	void SetEngineConstants(int debugInt = 0, float debugFloat = 0.f);
    void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::OPAQUE_WHITE);
    void SetLightConstants(Vec3 const& sunDirection = Vec3(2.f, -1.f, -1.f), float sunIntensity = 0.85f, float ambientIntensity = 0.35f);
    void SetLightConstants(LightConstants const& lightConstants);
    void SetPerFrameConstants(PerFrameConstants const& perframeConstants);

protected:
    void SetStatesIfChanged();
	void BindConstantBuffer(int slot, ConstantBuffer* cbo);

public:
    //-----------------------------------------------------------------------------------------------
    // Manage Resources - Public
    //-----------------------------------------------------------------------------------------------
    Texture* CreateOrGetTextureFromFile(char const* imageFilePath);

    BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension);

    Shader* CreateOrGetShader(char const* shaderName, VertexType type = VertexType::VERTEX_PCU);

    void AttachGeometryShader(Shader* shader, char const* shaderName);

protected:
	//-----------------------------------------------------------------------------------------------
    // Manage Resources - Protected
    //-----------------------------------------------------------------------------------------------
    Texture* GetTextureForFileName(char const* imageFilePath);
    Texture* CreateTextureFromFile(char const* imageFilePath);
    Texture* CreateTextureFromImage(Image const& image);

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
    Shader* m_currentShader = nullptr; // No one use?
    Shader* m_defaultShader = nullptr;


    RendererConfig m_config;

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

	ID3D11Device*           m_device = nullptr;
	ID3D11DeviceContext*    m_deviceContext = nullptr;
	IDXGISwapChain*         m_swapChain = nullptr;
	ID3D11RenderTargetView* m_renderTargetView = nullptr;
    ID3D11Texture2D*        m_depthStencilTexture = nullptr;
    ID3D11DepthStencilView* m_depthStencilDSV = nullptr;

    VertexBuffer* m_immediateVBO = nullptr;
    IndexBuffer* m_immediateIBO = nullptr;

	ConstantBuffer* m_engineCBO = nullptr;
	ConstantBuffer* m_perFrameCBO = nullptr;
	ConstantBuffer* m_cameraCBO = nullptr;
    ConstantBuffer* m_modelCBO = nullptr;
    ConstantBuffer* m_lightCBO = nullptr;

    static constexpr int NUM_SAMPLER_SLOT = 2;

    ID3D11BlendState*           m_blendState = nullptr;
    ID3D11SamplerState*         m_samplerStateBySlot[NUM_SAMPLER_SLOT] = {};

	ID3D11RasterizerState*      m_rasterizerState = nullptr;
    ID3D11DepthStencilState*    m_depthStencilState = nullptr;

    BlendMode       m_desiredBlendMode = BlendMode::ALPHA;
    SamplerMode     m_desiredSamplerMode = SamplerMode::POINT_CLAMP;
    SamplerMode     m_desiredSamplerModeBySlot[NUM_SAMPLER_SLOT] = {};

    RasterizerMode  m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
    DepthMode       m_desiredDepthMode = DepthMode::READ_WRITE_LESS_EQUAL;

    ID3D11BlendState*           m_blendStates[(int)(BlendMode::COUNT)] = {};
    ID3D11SamplerState*         m_samplerStates[(int)(SamplerMode::COUNT)] = {};
    ID3D11RasterizerState*      m_rasterizerStates[(int)(RasterizerMode::COUNT)] = {};
    ID3D11DepthStencilState*    m_depthStencilStates[(int)(DepthMode::COUNT)] = {};

public:
    //-----------------------------------------------------------------------------------------------
    // For RenderDoc
    //-----------------------------------------------------------------------------------------------
    void BeginRenderEvent(char const* eventName);
    void EndRenderEvent(char const* optional_eventName);

protected:
    ID3DUserDefinedAnnotation* m_userDefinedAnnotations = nullptr;
};
