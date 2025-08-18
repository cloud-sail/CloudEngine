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
class DX12DescriptorHeap;
class GraphicsPSO;
class ComputePSO;
class VertexBuffer;
class PSO;
class Buffer;

typedef void* HANDLE;




//enum class RootSignatureType
//{
//	Graphics,
//	Compute,
//	NUM
//};
//
//enum class GeneralRootBinding
//{
//	MODEL_CONSTANTS,    // b3
//	TEXTURE_SLOT_0,		// t0
//	TEXTURE_SLOT_1,		// t1
//	TEXTURE_SLOT_2,		// t2
//	CAMERA_CONSTANTS,   // b2
//	PERFRAME_CONSTANTS, // b1
//	LIGHT_CONSTANTS,	// b4
//	ENGINE_CONSTANTS,   // b0
//	TEXTURE_CUBE,		// t9
//	//MATERIAL_SRVS,      // t0-t2
//	//MATERIAL_SAMPLERS,  // s0-s2
//	//COMMON_SRVS,        // t10-t19
//	NUM
//};
//
//enum class GraphicsRootSignatureType
//{
//	General,
//	//Shadow,
//	//PostProcess,
//	//UI,
//	Count
//};
//
//enum class ComputeRootSignatureType
//{
//	PostProcessCompute,
//	ParticleCompute,
//	Count
//};

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

	void DrawProcedural(unsigned int vertexCount) override;

public:
	void Dispatch1D(unsigned int threadCountX, unsigned int groupSizeX = 64) override;
	void Dispatch2D(unsigned int threadCountX, unsigned int threadCountY, unsigned int groupSizeX = 8, unsigned int groupSizeY = 8) override;
	void Dispatch3D(unsigned int threadCountX, unsigned int threadCountY, unsigned int threadCountZ, unsigned int groupSizeX, unsigned int groupSizeY, unsigned int groupSizeZ) override;
	void DispatchRaw(unsigned int groupCountX, unsigned int groupCountY, unsigned int groupCountZ) override;

public:
	//-----------------------------------------------------------------------------------------------
	// Set Renderer States
	//-----------------------------------------------------------------------------------------------
	// Graphics Pipeline States
	void BindShader(Shader* shader) override;
	void SetBlendMode(BlendMode blendMode) override;
	void SetRasterizerMode(RasterizerMode rasterizerMode) override;
	void SetDepthMode(DepthMode depthMode) override;
	void SetRenderTargetFormats(const std::vector<DXGI_FORMAT>& rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
								DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
								unsigned int msaaCount = 1,
								unsigned int msaaQuality = 0) override;

	// Compute Pipeline States
	void BindComputeShader(Shader* csShader) override;

	// Slot-based binding Not Used in DX12
	void BindTexture(Texture const* texture, int slot = 0) override; 
	void SetSamplerMode(SamplerMode samplerMode, int slot = 0) override;

public:
	void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::OPAQUE_WHITE) override;
	
	// Will save a copy in renderer
	// if (m_currFrameResource == nullptr) return; // Before the first frame
	void SetEngineConstants(int debugInt = 0, float debugFloat = 0.f) override;
	void SetLightConstants(Vec3 const& sunDirection = Vec3(2.f, -1.f, -1.f), float sunIntensity = 0.85f, float ambientIntensity = 0.35f) override;
	void SetLightConstants(LightConstants const& lightConstants) override;
	void SetPerFrameConstants(PerFrameConstants const& perframeConstants) override;
	
	// Get in-engine cbv index
	uint32_t GetCurrentEngineConstantsIndex() const override{ return m_tempEngineConstantsIndex; }
	uint32_t GetCurrentPerFrameConstantsIndex() const override { return m_tempPerFrameConstantsIndex; }
	uint32_t GetCurrentLightConstantsIndex() const override { return m_tempLightConstantsIndex; }
	uint32_t GetCurrentCameraConstantsIndex() const override { return m_tempCameraConstantsIndex; }
	uint32_t GetCurrentModelConstantsIndex() const override { return m_tempModelConstantsIndex; }


protected:
	// Game code may change these every frame or not, or even before the first frame.
	// But in DX12 we need to set them every frame, since the command list is reset.
	EngineConstants m_engineConstants;
	PerFrameConstants m_perFrameConstants;
	LightConstants m_lightConstants;

	/*
	- EngineConstants		: Remember Last Frame Data
	- PerFrameConstants		: Remember Last Frame Data
	- LightConstants		: Remember Last Frame Data
	- CameraConstants		: Clear each frame
	- ModelConstants		: Clear each frame
	*/
	// Remember to reset to -1 in Begin Frame
	// descriptorHeap->AllocateTemporary(1)
	uint32_t m_tempEngineConstantsIndex = INVALID_INDEX_U32;
	uint32_t m_tempPerFrameConstantsIndex = INVALID_INDEX_U32;
	uint32_t m_tempLightConstantsIndex = INVALID_INDEX_U32;
	uint32_t m_tempCameraConstantsIndex = INVALID_INDEX_U32;
	uint32_t m_tempModelConstantsIndex = INVALID_INDEX_U32;

public:
	//-----------------------------------------------------------------------------------------------
	// Manage Resources - Public
	//-----------------------------------------------------------------------------------------------
	Texture* CreateOrGetTextureFromFile(char const* imageFilePath) override;
	Texture* CreateOrGetTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config) override;

	// If not loaded, create by yourself, please use the allocated index
	uint32_t GetSrvIndexFromLoadedTexture(Texture const* texture, DefaultTexture type = DefaultTexture::WhiteOpaque2D) override;
	uint32_t GetDefaultSamplerIndex(SamplerMode mode) const override { return m_defaultSamplers[(int)mode]; }

	BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension) override;

	// D3D Compile, not used
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
	
	uint32_t GetDefaultTextureSrvIndex(DefaultTexture type);

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

	Texture* m_defaultTextures[(int)DefaultTexture::NUM];
	uint32_t m_defaultSamplers[(int)SamplerMode::COUNT];

	Shader* m_defaultShader = nullptr;

public:
	// Default RenderTarget: Swapchain back buffer, no resource state tracking
	// Default defaultDepthBuffer: try to keep its states in depth write
	void SetDefaultRenderTargets() override; // e.g. change to other render target/depth buffer, then want to change back to default
	// ClearDefaultRenderTargets == ClearScreen

	// IMPORTANT: Before Setting and Clearing, please set the Texture to correct resource state
	void SetRenderTargetsByIndex(std::vector<uint32_t> rtvIndexs, uint32_t dsvIndex) override;
	void ClearRenderTargetByIndex(uint32_t rtvIndex, Rgba8 const& clearColor) override;
	void ClearDepthAndStencilByIndex(uint32_t dsvIndex, float clearDepth = 1.f, uint8_t clearStencil = 0) override;

	// Use it with care
	Texture* GetDefaultDepthBuffer() const override { return m_defaultDepthBuffer; }

	void SetGraphicsBindlessResources(size_t resourceSizeInBytes, const void* pResource) override;
	void SetComputeBindlessResources(size_t resourceSizeInBytes, const void* pResource) override;

public:
	// When having glitch please check the alignment, and struct in C++ and HLSL
	//-----------------------------------------------------------------------------------------------
	// Texture and Buffer
	// Notes: BackBuffer of swap chain should not use these functions
	// The Initial State of Both Buffer and Texture is COMMON
	// How Unity manage the resource?
	//-----------------------------------------------------------------------------------------------
	Texture* CreateTexture(TextureInit const& init) override;
	void UpdateTexture(Texture& texture, const void* data) override;
	void CopyTexture(Texture& dst, Texture& src) override;
	void DestroyTexture(Texture*& texture) override;

	// If format is unknown it will use the format that the texture is created with
	// D3D12_RESOURCE_DIMENSION cannot represent D3D12_SRV_DIMENSION_TEXTURECUBE
	// CPUHandleFromIndex for RTV DSV
	DescriptorHandle AllocateSRV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, bool isCubeMap = false) override;
	DescriptorHandle AllocateUAV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) override;
	DescriptorHandle AllocateRTV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) override;
	DescriptorHandle AllocateDSV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) override;

	void AddUAVBarrier(Texture& texture) override;
	void TransitionToCopyDest(Texture& texture) override;
	void TransitionToCopySource(Texture& texture) override;
	void TransitionToRenderTarget(Texture& texture) override;
	void TransitionToUnorderedAccess(Texture& texture) override;
	void TransitionToDepthWrite(Texture& texture) override;
	void TransitionToPixelShaderResource(Texture& texture) override;
	void TransitionToAllShaderResource(Texture& texture) override;

	//-----------------------------------------------------------------------------------------------
	// Dynamic/Temporary only exist one frame, Cannot be UAV, since it is allocated on Upload Heap
	// Not resource transition, the resource and view will be released after this frame
	uint32_t AllocateTempConstantBuffer(size_t bufferSize, const void* bufferData) override;
	uint32_t AllocateTempStructuredBuffer(size_t bufferSize, const void* bufferData, unsigned int elementSize, unsigned int numElements) override;
	uint32_t AllocateTempFormattedBuffer(size_t bufferSize, const void* bufferData, DXGI_FORMAT format, unsigned int numElements) override;
	uint32_t AllocateTempRawBuffer(size_t bufferSize, const void* bufferData, unsigned int numElements) override;

	//-----------------------------------------------------------------------------------------------
	Buffer* CreateBuffer(BufferInit const& init) override;
	void UpdateBuffer(Buffer& buffer, size_t dataSize, void const* bufferData, size_t dstOffset = 0) override;
	void CopyBuffer(Buffer& dst, size_t dstOffset, Buffer& src, size_t srcOffset, size_t numBytes) override;
	void DestroyBuffer(Buffer*& buffer) override;

	DescriptorHandle AllocateConstantBufferView(Buffer const& buffer) override; // only support one cbv one entire constant buffer
	DescriptorHandle AllocateStructuredBufferSRV(Buffer const& buffer, unsigned int elementSize, unsigned int numElements, uint64_t firstElement = 0) override;
	DescriptorHandle AllocateStructuredBufferUAV(Buffer const& buffer, unsigned int elementSize, unsigned int numElements, uint64_t firstElement = 0) override;
	DescriptorHandle AllocateRawBufferSRV(Buffer const& buffer, unsigned int numElements, uint64_t firstElement = 0) override;
	DescriptorHandle AllocateRawBufferUAV(Buffer const& buffer, unsigned int numElements, uint64_t firstElement = 0) override;
	DescriptorHandle AllocateFormattedBufferSRV(Buffer const& buffer, DXGI_FORMAT format, unsigned int numElements, uint64_t firstElement = 0) override;
	DescriptorHandle AllocateFormattedBufferUAV(Buffer const& buffer, DXGI_FORMAT format, unsigned int numElements, uint64_t firstElement = 0) override;

	void TransitionToGenericRead(Buffer& buffer) override; // SRV, ReadOnly
	void TransitionToUnorderedAccess(Buffer& buffer) override; // UAV, ReadWrite
	void TransitionToCopyDest(Buffer& buffer) override;
	void TransitionToCopySource(Buffer& buffer) override;
	// For example, if we write to a buffer or a texture as a UAV in one compute shader, then write or read from it from the next shader, 
	// we need to issue a UAV barrier between them (D3D12_RESOURCE_BARRIER_TYPE_UAV) to avoid a data hazard.
	void AddUAVBarrier(Buffer& buffer) override;


	// TODO: ReadBackBuffer, Read data from texture
	// May stall the rendering, flush rendering, copy back, start recording again, context stop start
	// TODO: Allocate Sampler with any sampler settings (e.g. sampler requirement from tinyGLTF)

protected:
	void TransitionResource(Buffer& buffer, D3D12_RESOURCE_STATES newState);
	void TransitionResource(Texture& texture, D3D12_RESOURCE_STATES newState);




protected:
	//-----------------------------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------------------------
	void CreateDevice();
	void CreateFenceAndHandle();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateDescriptorHeaps();
	void CreateRenderTargetViews();
	void CreateDepthStencilBufferAndView();
	//void CreateGraphicsRootSignatures();
	void CreateBindlessRootSignature();
	void CreateTextureHeap();

	void InitializeDefaultShaderAndTexture();
	void InitializeDefaultSamplers();
	void InitializeFrameResources();
	void DestroyFrameResources();

	void SetDebugNameForBasicDXObjects();


protected:
	// command list can have 2 root sigs at same time graphics and compute
	// command list can have 1 pipeline state object at the same time

	ID3D12RootSignature* m_bindlessRootSignature = nullptr; // 64 root constants (index), compute/graphics both can use it
	//ID3D12RootSignature* m_graphicsRootSignatures[(int)GraphicsRootSignatureType::Count] = {}; // remember to release

	GraphicsPSO* m_desiredGraphicsPSO = nullptr;
	ComputePSO* m_desiredComputePSO = nullptr;

	// These means current rootsigs and pso in the commandList now. these need to be reset to nullptr when the command list is reset
	// IMPORTANT: reset these to nullptr in begin frame once, because every time you reset the command list, the command list does not remember rootsig/pso last frame
	ID3D12RootSignature* m_curGraphicsRootSignature = nullptr; // do not release, just a weak pointer or an observer
	ID3D12RootSignature* m_curComputeRootSignature = nullptr; // do not release, just a weak pointer or an observer
	ID3D12PipelineState* m_currentPipelineState = nullptr; // do not release, just a weak pointer or an observer

protected:
	//-----------------------------------------------------------------------------------------------
	// DirectX 12 fundamentals
	//-----------------------------------------------------------------------------------------------
	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const; // default RTV
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const; // default DSV
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleFromDescriptorHandle(DescriptorHandle const& handle) const;

protected:
	IDXGIFactory4*		m_dxgiFactory = nullptr;
	IDXGISwapChain4*	m_swapChain	= nullptr;
	ID3D12Device*		m_device = nullptr;

	ID3D12Resource* m_swapChainBuffer[SWAP_CHAIN_BUFFER_COUNT] = {};
	DescriptorHandle m_swapChainBufferHandles[SWAP_CHAIN_BUFFER_COUNT] = {}; // RTV
	//uint32_t m_swapChainBufferRtvIndex[SWAP_CHAIN_BUFFER_COUNT] = {}; // remember to release?
	unsigned int m_currBackBuffer = 0; 

	//ID3D12Resource* m_depthStencilBuffer = nullptr;

	Texture* m_defaultDepthBuffer = nullptr;
	DescriptorHandle m_defaultDepthBufferHandle; // DSV
	//uint32_t m_defaultDsvIndex = INVALID_INDEX_U32;

	DX12DescriptorHeap* m_rtvDescriptorHeap = nullptr;
	DX12DescriptorHeap* m_dsvDescriptorHeap = nullptr;
	DX12DescriptorHeap* m_cbvSrvUavDescriptorHeap = nullptr;
	DX12DescriptorHeap* m_samplerDescriptorHeap = nullptr; // no one use it now
	// Begin Frame Set Heap, Heap->BeginFrame(Cycle and reset temp): IMPORTANT!!!!

	unsigned int m_rtvDescriptorSize = 0;
	unsigned int m_dsvDescriptorSize = 0;
	unsigned int m_cbvSrvUavDescriptorSize = 0;
	unsigned int m_samplerDescriptorSize = 0;

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
	bool							m_isCommandListOpened = false; // is opened when not between Renderer::BeginFrame and Renderer::EndFrame



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

	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, float color[4], D3D12_RECT* rect = nullptr);
	void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float clearDepth = 1.f, uint8_t clearStencil = 0);
	void ClearStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float clearDepth = 1.f, uint8_t clearStencil = 0);
	void ClearDepthAndStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float clearDepth = 1.f, uint8_t clearStencil = 0);

	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV) { SetRenderTargets(1, &RTV); }
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(1, &RTV, DSV); }
	void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

	// Graphics
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

	// Compute
	void SetComputeConstantArray(unsigned int rootParameterIndex, unsigned int numConstants, void const* pConstants);
	// Before Dispatch, remember to finalize the pso, and set it
	void Dispatch(unsigned int groupCountX = 1, unsigned int groupCountY = 1, unsigned int groupCountZ = 1);

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
	void EnqueueDeferredRelease(DX12DescriptorHeap* heap, uint32_t index);

	// Expose to Game Code
	void EnqueueDeferredRelease(DescriptorHeapType heapType, uint32_t index) override;
	void EnqueueDeferredRelease(DescriptorHandle& handle) override; // DescriptorHandle may need to be set mutable.

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
