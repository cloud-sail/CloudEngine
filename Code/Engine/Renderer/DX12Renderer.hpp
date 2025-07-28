#pragma once
#include "Engine/Renderer/Renderer.hpp"

#ifdef ENGINE_RENDER_D3D12
#include <d3d12.h>

#include <map>
#include <string>

struct IDXGISwapChain4;
struct IDXGIFactory4;

class DX12DeferredReleaseQueue;
class DX12LinearAllocator;
class GraphicsPSO;
class VertexBuffer;
class PSO;

typedef void* HANDLE;




enum class RootSignatureType
{
	Graphics,
	Compute,
	NUM
};

enum class GeneralRootBinding
{
	MODEL_CONSTANTS,    // b3
	TEXTURE_SLOT_0,		// t0
	TEXTURE_SLOT_1,		// t1
	TEXTURE_SLOT_2,		// t2
	CAMERA_CONSTANTS,   // b2
	PERFRAME_CONSTANTS, // b1
	LIGHT_CONSTANTS,	// b4
	ENGINE_CONSTANTS,   // b0
	TEXTURE_CUBE,		// t9
	//MATERIAL_SRVS,      // t0-t2
	//MATERIAL_SAMPLERS,  // s0-s2
	//COMMON_SRVS,        // t10-t19
	NUM
};

enum class GraphicsRootSignatureType
{
	General,
	//Shadow,
	//PostProcess,
	//UI,
	Count
};

enum class ComputeRootSignatureType
{
	PostProcessCompute,
	ParticleCompute,
	Count
};

//-----------------------------------------------------------------------------------------------
class DX12Renderer : public Renderer
{
public:
	static ID3D12Device* s_device; // for other class to create something

public:
	DX12Renderer(RendererConfig const& config);
	virtual ~DX12Renderer() = default;
	void Startup() override;
	void BeginFrame() override;
	void EndFrame() override;
	void Shutdown() override;

public:
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
public:
	void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes) override;
	void DrawVertexArray(std::vector<Vertex_PCU> const& verts) override;
	void DrawVertexArray(std::vector<Vertex_PCUTBN> const& verts) override;
	void DrawIndexedVertexArray(std::vector<Vertex_PCU> const& verts, std::vector<unsigned int> const& indexes) override;
	void DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> const& verts, std::vector<unsigned int> const& indexes) override;

	void DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount) override;
	void DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount) override;

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

	void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::OPAQUE_WHITE) override;
	
	// if (m_currFrameResource == nullptr) return; // Before the first frame
	void SetEngineConstants(int debugInt = 0, float debugFloat = 0.f) override;
	void SetLightConstants(Vec3 const& sunDirection = Vec3(2.f, -1.f, -1.f), float sunIntensity = 0.85f, float ambientIntensity = 0.35f) override;
	void SetLightConstants(LightConstants const& lightConstants) override;
	void SetPerFrameConstants(PerFrameConstants const& perframeConstants) override;

protected:
	// Game code may change these every frame or not, or even before the first frame.
	// But in DX12 we need to set them every frame, since the command list is reset.
	EngineConstants m_engineConstants;
	PerFrameConstants m_perFrameConstants;
	LightConstants m_lightConstants;


public:
	//-----------------------------------------------------------------------------------------------
	// Manage Resources - Public
	//-----------------------------------------------------------------------------------------------
	Texture* CreateOrGetTextureFromFile(char const* imageFilePath) override;
	Texture* CreateOrGetTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config) override;

	BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension) override;

	Shader* CreateOrGetShader(char const* shaderName, VertexType type = VertexType::VERTEX_PCU) override;

	void AttachGeometryShader(Shader* shader, char const* shaderName) override;

	// More Advanced Shader Compiling
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
	Shader* CreateShader(ShaderConfig const& config, VertexType type = VertexType::VERTEX_PCU);
	Shader* CreateShader(ShaderConfig const& config, char const* shaderSource, VertexType type = VertexType::VERTEX_PCU);
	
	bool CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, const char* name,
		const char* source, const char* entryPoint, const char* target, ShaderCompilerType compilerType);
	
	bool CompileShaderToByteCode_D3DCompile(std::vector<unsigned char>& outByteCode, char const* name,
		char const* source, char const* entryPoint, char const* target);

	bool CompileShaderToByteCode_DXC(std::vector<unsigned char>& outByteCode, const char* name,
		const char* source, const char* entryPoint, const char* target);

protected:
	//-----------------------------------------------------------------------------------------------
	// Manage Resources - data
	//-----------------------------------------------------------------------------------------------
	ID3D12DescriptorHeap* m_textureHeap = nullptr;
	
	std::vector<Texture*> m_loadedTextures;
	std::vector<BitmapFont*> m_loadedFonts;
	std::vector<Shader*> m_loadedShaders;
	const Texture* m_defaultTexture = nullptr;
	const Texture* m_defaultNormalTexture = nullptr;
	const Texture* m_defaultSpecGlossEmitTexture = nullptr;

	Shader* m_defaultShader = nullptr;

protected:
	//-----------------------------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------------------------
	void CreateDevice();
	void CreateFenceAndHandleAndDescriptorSizes();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateDescriptorHeaps();
	void CreateRenderTargetViews();
	void CreateDepthStencilBufferAndView();
	void CreateGraphicsRootSignatures();
	void CreateTextureHeap();

	void InitializeDefaultShaderAndTexture();
	void InitializeFrameResources();
	void DestroyFrameResources();

	void SetDebugNameForBasicDXObjects();


protected:
	// command list can have 2 root sigs at same time graphics and compute
	// command list can have 1 pipeline state object at the same time

	// FUTURE CONSIDERATION: 
	// if we have multiple graphics root signature, consider using a enum class for different root signature, we may need to hash the root sig type
	// 1. if we hash the root sig, we can still use one m_desiredGraphicsPSO, when SetGraphicsRootSignature(ID3D12RootSignature* rootSig) we need to change root sig in m_desiredGraphicsPSO.
	// 2. if we do not hash the root sig, we will need multiple corresponding m_desiredGraphicsPSO. Bind Shader, SetBlendMode, SetRasterizerMode, SetDepthMode will also be changed
	// When we switch to another root signature, keep in mind the context is totally different.
	// pso may need to hash the root sig type. or not necessary, since shader for different root sig must be different.
	// 


	ID3D12RootSignature* m_graphicsRootSignatures[(int)GraphicsRootSignatureType::Count] = {}; // remember to release


	GraphicsPSO* m_desiredGraphicsPSO = nullptr; // currently only using m_graphicsRootSignature
	//ComputePSO* m_desiredComputePSO = nullptr;

	// IMPORTANT: reset these to nullptr in begin frame once, because every time you reset the command list, the command list does not remember rootsig/pso last frame
	ID3D12RootSignature* m_curGraphicsRootSignature = nullptr; // do not release, just a weak pointer or an observer
	ID3D12RootSignature* m_curComputeRootSignature = nullptr; // do not release, just a weak pointer or an observer
	ID3D12PipelineState* m_currentPipelineState = nullptr; // do not release, just a weak pointer or an observer


protected:
	//-----------------------------------------------------------------------------------------------
	// DirectX 12 fundamentals
	//-----------------------------------------------------------------------------------------------
	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

protected:
	IDXGIFactory4*		m_dxgiFactory = nullptr;
	IDXGISwapChain4*	m_swapChain	= nullptr;
	ID3D12Device*		m_device = nullptr;

	ID3D12Resource* m_swapChainBuffer[SWAP_CHAIN_BUFFER_COUNT] = {};
	unsigned int m_currBackBuffer = 0; 

	ID3D12Resource* m_depthStencilBuffer = nullptr;

	ID3D12DescriptorHeap* m_rtvHeap = nullptr;
	ID3D12DescriptorHeap* m_dsvHeap = nullptr;

	unsigned int m_rtvDescriptorSize = 0;
	unsigned int m_dsvDescriptorSize = 0;
	unsigned int m_cbvSrvUavDescriptorSize = 0;

public:
	//-----------------------------------------------------------------------------------------------
	// Frame Resources
	//-----------------------------------------------------------------------------------------------
	DX12LinearAllocator* GetCurrentLinearAllocator() const;
protected:
	ID3D12CommandAllocator* GetCurrentCommandAllocator() const { return m_currFrameResource->m_commandAllocator; }
	bool IsBeforeTheFirstFrame() const { return m_currFrameResource == nullptr; }

protected:
	struct FrameResource 
	{
		DX12LinearAllocator* m_linearAllocator = nullptr;
		ID3D12CommandAllocator* m_commandAllocator = nullptr;
		uint64_t m_fenceValue = 0;
	};

	FrameResource* m_frameResources[FRAMES_IN_FLIGHT] = {};
	FrameResource* m_currFrameResource = nullptr;
	unsigned int m_currFrameResourceIndex = 0;


protected:
	//-----------------------------------------------------------------------------------------------
	// CPU/GPU Synchronization
	//-----------------------------------------------------------------------------------------------
	uint64_t Signal();
	void WaitForFenceValue(uint64_t fenceValue);
	// Signal and Wait (if no commands, just want to make sure all tasks have been done)
	void FlushCommandQueue(); // e.g. before shutdown, before resize
	// close the command list, execute it, and signal (execute all commands, but no waiting)
	uint64_t ExecuteCommandList(); 
	// close the command list, execute it, signal, and wait (execute all commands, and wait until all commands are done)
	void ExecuteCommandListAndWait(); // e.g. after resize

protected:
	HANDLE m_fenceEvent = nullptr;
	ID3D12Fence* m_fence = nullptr;
	uint64_t m_nextFenceValueForSignal = 1; // Since fence initial complete value is 0, it should be larger than 0

	ID3D12CommandQueue*				m_commandQueue = nullptr;
	ID3D12GraphicsCommandList6*		m_commandList = nullptr;
	
	// It is not used between Renderer::BeginFrame and Renderer::EndFrame. 
	// e.g. before the first frame begin, resize the window(recreate rtvs dsv)
	// Between Renderer::BeginFrame and Renderer::EndFrame, we are using command allocators in frame resources
	// command list can be reset immediately to record new commands, after being submitted
	// However, a command allocator must only be reset after the GPU has finished executing all commands recording with it.
	// if you never reset the command allocator, the app will render normally. but the memory will be used up very soon.
	ID3D12CommandAllocator*			m_mainCommandAllocator = nullptr;
	DX12LinearAllocator*			m_mainLinearAllocator = nullptr;

protected:
	//-----------------------------------------------------------------------------------------------
	// commandList helper function
	//-----------------------------------------------------------------------------------------------
	// IMPORTANT: root signature must be set before SetGraphicsRootXXX
	void SetGraphicsRootSignature(ID3D12RootSignature* rootSig);
	void SetComputeRootSignature(ID3D12RootSignature* rootSig);

	// before every draw call, first finalize the PSO(Graphics/Compute), then use this function
	// compare to current pso, if different set this pipeline object
	void SetPipelineState(PSO const& pso); 
	void SetViewport(D3D12_VIEWPORT const& viewport);
	void SetScissor(D3D12_RECT const& rect);

	void SetConstantArray(unsigned int rootParameterIndex, unsigned int numConstants, void const* pConstants);
	void SetConstantBuffer(unsigned int rootParamterIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv);
	void SetDynamicConstantBuffer(unsigned int rootParamterIndex, size_t bufferSize, void const* bufferData);
	void SetDynamicVertexBuffer(unsigned int slot, size_t numVertices, size_t vertexStride, void const* vertexData);
	void SetDynamicIndexBuffer(size_t numIndexes, const void* indexData);
	void SetDynmaicSRV(unsigned int rootParameterIndex, size_t bufferSize, void const* bufferData);
	void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibView);
	void SetVertexBuffer(unsigned int slot, const D3D12_VERTEX_BUFFER_VIEW& vbView);
	void SetVertexBuffers(unsigned int startSlot, unsigned int Count, const D3D12_VERTEX_BUFFER_VIEW vbViews[]);

	void Draw(unsigned int vertexCount, unsigned int startVertexLocation = 0);
	void DrawIndexed(unsigned int indexCount, unsigned int startIndexLocation = 0, unsigned int baseVertexLocation = 0);
	void DrawInstanced(unsigned int vertexCountPerInstance, unsigned int instanceCount,
		unsigned int startVertexLocation = 0, unsigned int startInstanceLocation = 0);
	void DrawIndexedInstanced(unsigned int indexCountPerInstance, unsigned int instanceCount, unsigned int startIndexLocation,
		unsigned int baseVertexLocation, unsigned int startInstanceLocation);
	//void DrawIndirect();


public:
	//-----------------------------------------------------------------------------------------------
	// For RenderDoc
	//-----------------------------------------------------------------------------------------------
	void BeginRenderEvent(char const* eventName) override;
	void EndRenderEvent(char const* optional_eventName) override;

public:
	//-----------------------------------------------------------------------------------------------
	// Deferred Release resource (resource will be released after current frame is processed by GPU)
	//-----------------------------------------------------------------------------------------------
	void EnqueueDeferredRelease(IUnknown* resource);

protected:
	DX12DeferredReleaseQueue* m_deferredReleaseQueue = nullptr;
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

protected:
	ID3D12DescriptorHeap* m_ImGuiSrvDescHeap = nullptr;

};

#endif // ENGINE_RENDER_D3D12


/*
* Memory Leak Testing
auto a = (*IUnknown)->AddRef();
DebuggerPrintf("%d\n", a);
auto b = (*IUnknown)->Release();
DebuggerPrintf("%d\n", b);
*/
