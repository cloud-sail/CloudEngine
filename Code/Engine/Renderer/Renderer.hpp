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

public:
    //-----------------------------------------------------------------------------------------------
    // Set Renderer States
    //-----------------------------------------------------------------------------------------------
    virtual void BindTexture(Texture const* texture, int slot = 0) = 0;
    virtual void BindShader(Shader* shader) = 0;
    virtual void SetBlendMode(BlendMode blendMode) = 0;
	virtual void SetSamplerMode(SamplerMode samplerMode, int slot = 0) = 0;
	virtual void SetRasterizerMode(RasterizerMode rasterizerMode) = 0;
	virtual void SetDepthMode(DepthMode depthMode) = 0;
	virtual void SetEngineConstants(int debugInt = 0, float debugFloat = 0.f) = 0;
    virtual void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::OPAQUE_WHITE) = 0;
    virtual void SetLightConstants(Vec3 const& sunDirection = Vec3(2.f, -1.f, -1.f), float sunIntensity = 0.85f, float ambientIntensity = 0.35f) = 0;
    virtual void SetLightConstants(LightConstants const& lightConstants) = 0;
    virtual void SetPerFrameConstants(PerFrameConstants const& perframeConstants) = 0;

public:
    //-----------------------------------------------------------------------------------------------
    // Manage Resources - Public
    //-----------------------------------------------------------------------------------------------
    virtual Texture* CreateOrGetTextureFromFile(char const* imageFilePath) = 0;
    virtual Texture* CreateOrGetTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config) = 0;

    virtual BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension) = 0;

    // Only Support VS+PS
    virtual Shader* CreateOrGetShader(char const* shaderName, VertexType type = VertexType::VERTEX_PCU) = 0;

    virtual void AttachGeometryShader(Shader* shader, char const* shaderName) = 0;

    // Advanced Shader Compiling, DX11 will fall to PS+VS
    virtual Shader* CreateOrGetShader(ShaderConfig const& config, VertexType type = VertexType::VERTEX_PCU) = 0;

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
