#include "Engine/Renderer/VertexBuffer.hpp"

#ifdef ENGINE_RENDER_D3D11
#include "Engine/Core/EngineCommon.hpp"
#include <d3d11.h>

#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}

VertexBuffer::VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride)
	: m_device(device)
	, m_size(size)
	, m_stride(stride)
{
	Create();
}

VertexBuffer::~VertexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

void VertexBuffer::Create()
{
	DX_SAFE_RELEASE(m_buffer);

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = m_size;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer.");
	}
}

void VertexBuffer::Resize(unsigned int size)
{
	m_size = size;
	Create();
}

unsigned int VertexBuffer::GetSize()
{
	return m_size;
}

unsigned int VertexBuffer::GetStride()
{
	return m_stride;
}
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Renderer/DX12LinearAllocator.hpp"

#include <d3d12.h>
#include "ThirdParty/directx/d3dx12.h"


VertexBuffer::~VertexBuffer()
{
	if (m_defaultBuffer)
	{
		m_renderer->EnqueueDeferredRelease(m_defaultBuffer);
		m_defaultBuffer = nullptr;
	}

	//if (m_uploadBuffer)
	//{
	//	m_renderer->EnqueueDeferredRelease(m_uploadBuffer);
	//	m_uploadBuffer = nullptr;
	//}
}

VertexBuffer::VertexBuffer(DX12Renderer* renderer, ID3D12Device* device, unsigned int size, unsigned int stride)
	: m_renderer(renderer)
	, m_device(device)
	, m_size(size)
	, m_stride(stride)
{
	Create();
}

void VertexBuffer::Create()
{
	CreateDefaultBuffer();
}

void VertexBuffer::Resize(unsigned int size)
{
	m_size = size;
	CreateDefaultBuffer();
}

void VertexBuffer::Upload(ID3D12GraphicsCommandList* cmdList, void const* data, unsigned int dataSize)
{
	if (dataSize > m_size)
	{
		Resize(dataSize);
	}

	// if some one upload a smaller data, the view will change, although no effect. 
	// because draw call will require a vertex/index count to draw.
	// it restricts the maximum amount you can use
	m_actualDataSize = dataSize;

	DX12LinearAllocator* allocator = m_renderer->GetCurrentLinearAllocator();
	size_t alignment = 16;
	DynAlloc uploadAlloc = allocator->Allocate(dataSize, alignment);

	memcpy(uploadAlloc.m_dataPtr, data, dataSize);

	//if (m_uploadBuffer == nullptr || m_uploadBufferSize < dataSize)
	//{
	//	CreateUploadBuffer(dataSize);
	//}

	//D3D12_SUBRESOURCE_DATA subresourceData = {};
	//subresourceData.pData = data;
	//subresourceData.RowPitch = dataSize;
	//subresourceData.SlicePitch = dataSize;

	if (static_cast<D3D12_RESOURCE_STATES>(m_currentResourceState) != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		CD3DX12_RESOURCE_BARRIER barrierBefore = CD3DX12_RESOURCE_BARRIER::Transition(
			m_defaultBuffer, static_cast<D3D12_RESOURCE_STATES>(m_currentResourceState),
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		cmdList->ResourceBarrier(1, &barrierBefore);
		m_currentResourceState = static_cast<unsigned int>(D3D12_RESOURCE_STATE_COPY_DEST);
	}

	cmdList->CopyBufferRegion(
		m_defaultBuffer, 0,
		uploadAlloc.m_buffer, uploadAlloc.m_offset,
		dataSize
	);

	//UpdateSubresources<1>(cmdList, m_defaultBuffer, m_uploadBuffer, 0, 0, 1, &subresourceData);

	CD3DX12_RESOURCE_BARRIER barrierAfter = CD3DX12_RESOURCE_BARRIER::Transition(
		m_defaultBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
	);
	cmdList->ResourceBarrier(1, &barrierAfter);
	m_currentResourceState = static_cast<unsigned int>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	//-----------------------------------------------------------------------------------------------
	// deplete the upload buffer after uploading, every upload we create a brand new upload heap
	// #ToDo a large upload heap with ring buffer
	//if (m_uploadBuffer)
	//{
	//	m_renderer->EnqueueDeferredRelease(m_uploadBuffer);
	//	m_uploadBuffer = nullptr;
	//}
	//m_uploadBufferSize = 0;
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW view = {};
	view.BufferLocation = m_defaultBuffer->GetGPUVirtualAddress();
	view.StrideInBytes = m_stride;
	view.SizeInBytes = m_actualDataSize;
	return view;
}

void VertexBuffer::CreateDefaultBuffer()
{
	if (m_defaultBuffer)
	{
		m_renderer->EnqueueDeferredRelease(m_defaultBuffer);
		m_defaultBuffer = nullptr;
	}

	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(m_size);

	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_defaultBuffer)
	);
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer on default heap");
	}

#ifdef _DEBUG
	m_defaultBuffer->SetName(L"Default heap of vertex buffer");
#endif // _DEBUG

	m_currentResourceState = static_cast<unsigned int>(D3D12_RESOURCE_STATE_COMMON);
}

//void VertexBuffer::CreateUploadBuffer(unsigned int size)
//{
//	if (m_uploadBuffer)
//	{
//		m_renderer->EnqueueDeferredRelease(m_uploadBuffer);
//		m_uploadBuffer = nullptr;
//	}
//	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(m_size);
//
//	HRESULT hr = m_device->CreateCommittedResource(
//		&heapProps,
//		D3D12_HEAP_FLAG_NONE,
//		&resDesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(&m_uploadBuffer)
//	);
//
//	if (FAILED(hr))
//	{
//		ERROR_AND_DIE("Failed to create vertex buffer on upload heap");
//	}
//
//#ifdef _DEBUG
//	m_uploadBuffer->SetName(L"Upload heap of vertex buffer");
//#endif // _DEBUG
//
//	m_uploadBufferSize = size;
//}
#endif // ENGINE_RENDER_D3D12
