#pragma once
#include "Engine/Renderer/RendererCommon.hpp"

#ifdef ENGINE_RENDER_D3D11
//-----------------------------------------------------------------------------------------------
struct ID3D11Device;
struct ID3D11Buffer;

//-----------------------------------------------------------------------------------------------
class VertexBuffer
{
	friend class Renderer;
	friend class DX11Renderer;

public:
	VertexBuffer(const VertexBuffer& copy) = delete;
	void operator=(VertexBuffer const& copy) = delete;
	virtual ~VertexBuffer();

	VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride);


	void Create();
	void Resize(unsigned int size);

	unsigned int GetSize();
	unsigned int GetStride();

private:
	ID3D11Device* m_device = nullptr;
	ID3D11Buffer* m_buffer = nullptr;
	unsigned int m_size = 0; // total Size of the buffer in bytes
	unsigned int m_stride = 0;

};
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct D3D12_VERTEX_BUFFER_VIEW;
class DX12Renderer;

class VertexBuffer
{
public:
	VertexBuffer(const VertexBuffer& copy) = delete;
	void operator=(VertexBuffer const& copy) = delete;
	virtual ~VertexBuffer();

	VertexBuffer(DX12Renderer* renderer, ID3D12Device* device, unsigned int size, unsigned int stride);

	void Create();
	void Resize(unsigned int size);
	void Upload(ID3D12GraphicsCommandList* cmdList, void const* data, unsigned int dataSize);

	unsigned int GetSize() const { return m_size; }
	unsigned int GetStride() const { return m_stride; }

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
private:
	void CreateDefaultBuffer();
	//void CreateUploadBuffer(unsigned int size);

private:
	DX12Renderer* m_renderer = nullptr;
	ID3D12Device* m_device = nullptr;
	ID3D12Resource* m_defaultBuffer = nullptr;
	//ID3D12Resource* m_uploadBuffer = nullptr;

	unsigned int m_size = 0;
	unsigned int m_stride = 0;
	//unsigned int m_uploadBufferSize = 0;
	unsigned int m_actualDataSize = 0;

	unsigned int m_currentResourceState = 0;
	//D3D12_RESOURCE_STATES m_currentResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
};
#endif // ENGINE_RENDER_D3D12



