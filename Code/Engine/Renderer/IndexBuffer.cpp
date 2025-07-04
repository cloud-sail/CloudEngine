#include "Engine/Renderer/IndexBuffer.hpp"
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

IndexBuffer::IndexBuffer(ID3D11Device* device, unsigned int size)
	: m_device(device)
	, m_size(size)
{
	Create();
}

IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

void IndexBuffer::Create()
{
	DX_SAFE_RELEASE(m_buffer);

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = m_size;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create index buffer.");
	}
}

void IndexBuffer::Resize(unsigned int size)
{
	m_size = size;
	Create();
}

unsigned int IndexBuffer::GetSize()
{
	return m_size;
}

unsigned int IndexBuffer::GetStride()
{
	return sizeof( unsigned int );
}

unsigned int IndexBuffer::GetCount()
{
	return GetSize() / GetStride();
}
