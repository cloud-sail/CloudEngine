#pragma once
#include "Engine/Renderer/RendererCommon.hpp"
#include <vector>
#include <string>

extern const std::string WINDOW_RESIZE_EVENT;

class Renderer
{
public:
    Renderer(RendererConfig const& config) : m_config(config) { s_mainRenderer = this; }
    virtual ~Renderer() = default;
    virtual void Startup() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Shutdown() = 0;

	virtual void ClearScreen(Rgba8 const& clearColor) = 0;
	virtual void BeginCamera(Camera const& camera) = 0;
	virtual void EndCamera(Camera const& camera) = 0;

public:
	virtual VertexBuffer* CreateVertexBuffer(const unsigned int size, unsigned int stride) = 0;
	virtual IndexBuffer* CreateIndexBuffer(const unsigned int size) = 0;
	virtual ConstantBuffer* CreateConstantBuffer(const unsigned int size) = 0;

	virtual void CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo) = 0;
	virtual void CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo) = 0;
	virtual void CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo) = 0;

    // Draw Calls
	virtual void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes) = 0;
	virtual void DrawVertexArray(std::vector<Vertex_PCU> const& verts) = 0;
    virtual void DrawVertexArray(std::vector<Vertex_PCUTBN> const& verts) = 0;
	virtual void DrawIndexedVertexArray(std::vector<Vertex_PCU> const& verts, std::vector<unsigned int> const& indexes) = 0;
	virtual void DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> const& verts, std::vector<unsigned int> const& indexes) = 0;

	virtual void DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount) = 0;
	virtual void DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount) = 0;


#ifdef ENGINE_RENDER_D3D12
	virtual void DrawProcedural(unsigned int vertexCount) = 0;

	// Dispatch Calls
	virtual void Dispatch1D(unsigned int threadCountX, unsigned int groupSizeX = 64) = 0;
	virtual void Dispatch2D(unsigned int threadCountX, unsigned int threadCountY, unsigned int groupSizeX = 8, unsigned int groupSizeY = 8) = 0;
	virtual void Dispatch3D(unsigned int threadCountX, unsigned int threadCountY, unsigned int threadCountZ, unsigned int groupSizeX, unsigned int groupSizeY, unsigned int groupSizeZ) = 0;
	virtual void DispatchRaw(unsigned int groupCountX, unsigned int groupCountY, unsigned int groupCountZ) = 0;
#endif // ENGINE_RENDER_D3D12


public:
    //-----------------------------------------------------------------------------------------------
    // Set Renderer States
    //-----------------------------------------------------------------------------------------------
    // Graphics Pipeline States
    virtual void BindShader(Shader* shader) = 0;
    virtual void SetBlendMode(BlendMode blendMode) = 0;
	virtual void SetRasterizerMode(RasterizerMode rasterizerMode) = 0;
	virtual void SetDepthMode(DepthMode depthMode) = 0;

	virtual void SetRenderTargetFormats(const std::vector<DXGI_FORMAT>& rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
		unsigned int msaaCount = 1,
		unsigned int msaaQuality = 0) = 0;

#ifdef ENGINE_RENDER_D3D12
    // Compute Pipeline States
    virtual void BindComputeShader(Shader* csShader) = 0;
#endif // ENGINE_RENDER_D3D12

    // Slot-based binding
    virtual void BindTexture(Texture const* texture, int slot = 0) = 0;
	virtual void SetSamplerMode(SamplerMode samplerMode, int slot = 0) = 0;

public:
    virtual void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::OPAQUE_WHITE) = 0;
	
    virtual void SetEngineConstants(int debugInt = 0, float debugFloat = 0.f) = 0;
    virtual void SetLightConstants(Vec3 const& sunDirection = Vec3(2.f, -1.f, -1.f), float sunIntensity = 0.85f, float ambientIntensity = 0.35f) = 0;
    virtual void SetLightConstants(LightConstants const& lightConstants) = 0;
    virtual void SetPerFrameConstants(PerFrameConstants const& perframeConstants) = 0;

#ifdef ENGINE_RENDER_D3D12
	virtual uint32_t GetCurrentEngineConstantsIndex() const = 0;
	virtual uint32_t GetCurrentPerFrameConstantsIndex() const = 0;
	virtual uint32_t GetCurrentLightConstantsIndex() const = 0;
	virtual uint32_t GetCurrentCameraConstantsIndex() const = 0;
	virtual uint32_t GetCurrentModelConstantsIndex() const = 0;
#endif // ENGINE_RENDER_D3D12

public:
    //-----------------------------------------------------------------------------------------------
    // Manage Resources - Public
    //-----------------------------------------------------------------------------------------------
    virtual Texture* CreateOrGetTextureFromFile(char const* imageFilePath) = 0;
    virtual Texture* CreateOrGetTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config) = 0;

#ifdef ENGINE_RENDER_D3D12
	virtual uint32_t GetSrvIndexFromLoadedTexture(Texture const* texture, DefaultTexture type = DefaultTexture::WhiteOpaque2D) = 0;
	virtual uint32_t GetDefaultSamplerIndex(SamplerMode mode) const = 0;
#endif // ENGINE_RENDER_D3D12

    virtual BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension) = 0;

    // Only Support VS+PS
    virtual Shader* CreateOrGetShader(char const* shaderName, VertexType type = VertexType::VERTEX_PCU) = 0;
    virtual void AttachGeometryShader(Shader* shader, char const* shaderName) = 0;

    // Advanced Shader Compiling, DX11 will fall to PS+VS
    virtual Shader* CreateOrGetShader(ShaderConfig const& config, VertexType type = VertexType::VERTEX_PCU) = 0;

#ifdef ENGINE_RENDER_D3D12
public:
    virtual void SetDefaultRenderTargets() = 0;
	
    virtual void SetRenderTargetsByIndex(std::vector<uint32_t> rtvIndexs, uint32_t dsvIndex) = 0;
    virtual void ClearRenderTargetByIndex(uint32_t rtvIndex, Rgba8 const& clearColor) = 0;
    virtual void ClearDepthAndStencilByIndex(uint32_t dsvIndex, float clearDepth = 1.f, uint8_t clearStencil = 0) = 0;

    virtual Texture* GetDefaultDepthBuffer() const = 0;

	virtual void SetGraphicsBindlessResources(size_t resourceSizeInBytes, const void* pResource) = 0;
	virtual void SetComputeBindlessResources(size_t resourceSizeInBytes, const void* pResource) = 0;

public:
	//-----------------------------------------------------------------------------------------------
	virtual Texture* CreateTexture(TextureInit const& init) = 0;
	virtual void UpdateTexture(Texture& texture, const void* data) = 0;
	virtual void CopyTexture(Texture& dst, Texture& src) = 0;
	virtual void DestroyTexture(Texture*& texture) = 0;

	virtual DescriptorHandle AllocateSRV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, bool isCubeMap = false) = 0;
	virtual DescriptorHandle AllocateUAV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) = 0;
	virtual DescriptorHandle AllocateRTV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) = 0;
	virtual DescriptorHandle AllocateDSV(Texture& texture, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) = 0;

	virtual void AddUAVBarrier(Texture& texture) = 0;
	virtual void TransitionToCopyDest(Texture& texture) = 0;
	virtual void TransitionToCopySource(Texture& texture) = 0;
	virtual void TransitionToRenderTarget(Texture& texture) = 0;
	virtual void TransitionToUnorderedAccess(Texture& texture) = 0;
	virtual void TransitionToDepthWrite(Texture& texture) = 0;
	virtual void TransitionToPixelShaderResource(Texture& texture) = 0;
	virtual void TransitionToAllShaderResource(Texture& texture) = 0;

	//-----------------------------------------------------------------------------------------------
	virtual uint32_t AllocateTempConstantBuffer(size_t bufferSize, const void* bufferData) = 0;
	virtual uint32_t AllocateTempStructuredBuffer(size_t bufferSize, const void* bufferData, unsigned int elementSize, unsigned int numElements) = 0;
	virtual uint32_t AllocateTempFormattedBuffer(size_t bufferSize, const void* bufferData, DXGI_FORMAT format, unsigned int numElements) = 0;
	virtual uint32_t AllocateTempRawBuffer(size_t bufferSize, const void* bufferData, unsigned int numElements) = 0;
	
	//-----------------------------------------------------------------------------------------------
	virtual Buffer* CreateBuffer(BufferInit const& init) = 0;
	virtual void UpdateBuffer(Buffer& buffer, size_t dataSize, void const* bufferData, size_t dstOffset = 0) = 0;
	virtual void CopyBuffer(Buffer& dst, size_t dstOffset, Buffer& src, size_t srcOffset, size_t numBytes) = 0;
	virtual void DestroyBuffer(Buffer*& buffer) = 0;

	virtual DescriptorHandle AllocateConstantBufferView(Buffer const& buffer) = 0;
	virtual DescriptorHandle AllocateStructuredBufferSRV(Buffer const& buffer, unsigned int elementSize, unsigned int numElements, uint64_t firstElement = 0) = 0;
	virtual DescriptorHandle AllocateStructuredBufferUAV(Buffer const& buffer, unsigned int elementSize, unsigned int numElements, uint64_t firstElement = 0) = 0;
	virtual DescriptorHandle AllocateRawBufferSRV(Buffer const& buffer, unsigned int numElements, uint64_t firstElement = 0) = 0;
	virtual DescriptorHandle AllocateRawBufferUAV(Buffer const& buffer, unsigned int numElements, uint64_t firstElement = 0) = 0;
	virtual DescriptorHandle AllocateFormattedBufferSRV(Buffer const& buffer, DXGI_FORMAT format, unsigned int numElements, uint64_t firstElement = 0) = 0;
	virtual DescriptorHandle AllocateFormattedBufferUAV(Buffer const& buffer, DXGI_FORMAT format, unsigned int numElements, uint64_t firstElement = 0) = 0;
	
	virtual void TransitionToGenericRead(Buffer& buffer) = 0;
	virtual void TransitionToUnorderedAccess(Buffer& buffer) = 0;
	virtual void TransitionToCopyDest(Buffer& buffer) = 0;
	virtual void TransitionToCopySource(Buffer& buffer) = 0;
	virtual void AddUAVBarrier(Buffer& buffer) = 0;

public:
	virtual void EnqueueDeferredRelease(DescriptorHeapType heapType, uint32_t index) = 0;
	virtual void EnqueueDeferredRelease(DescriptorHandle& handle) = 0;

#endif // ENGINE_RENDER_D3D12

public:
    //-----------------------------------------------------------------------------------------------
    // For RenderDoc
    //-----------------------------------------------------------------------------------------------
    virtual void BeginRenderEvent(char const* eventName) = 0;
    virtual void EndRenderEvent(char const* optional_eventName) = 0;

protected:
    RendererConfig m_config;

public:
    virtual void OnResize() = 0;

public:
    static Renderer* s_mainRenderer;
};
