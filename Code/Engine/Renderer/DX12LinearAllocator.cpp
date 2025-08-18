#include "Engine/Renderer/DX12LinearAllocator.hpp"

#ifdef ENGINE_RENDER_D3D12

#include "Engine/Core/EngineCommon.hpp"
#include "ThirdParty/directx/d3dx12.h"

// Some Alignment math
namespace DX12Math
{
	inline size_t AlignUpWithMask(size_t value, size_t mask)
	{
		return ((value + mask) & ~mask);
	}

	inline size_t AlignDownWithMask(size_t value, size_t mask)
	{
		return (value & ~mask);
	}

	inline size_t AlignUp(size_t value, size_t alignment)
	{
		return AlignUpWithMask(value, alignment - 1);
	}

	inline size_t AlignDown(size_t value, size_t alignment)
	{
		return AlignDownWithMask(value, alignment - 1);
	}

	inline bool IsAligned(size_t value, size_t alignment)
	{
		return 0 == (value & (alignment - 1));
	}


	inline size_t AlignUpDiv(size_t value, size_t alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}
}


DX12LinearAllocator::DX12LinearAllocator(ID3D12Device* device, size_t totalSize)
	: m_totalSize(totalSize), m_curOffset(0)
{
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(totalSize);

	HRESULT hr = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_uploadBuffer));

	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create upload heap for DX12LinearAllocator");
	}

	m_uploadBuffer->Map(0, nullptr, &m_mappedPtr);
}

DX12LinearAllocator::~DX12LinearAllocator()
{
	if (m_uploadBuffer)
	{
		m_uploadBuffer->Unmap(0, nullptr);
		m_uploadBuffer->Release();
	}
}

DynAlloc DX12LinearAllocator::Allocate(size_t sizeInBytes, size_t alignment /*= D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT*/)
{
	size_t const alignmentMask = alignment - 1;
	GUARANTEE_OR_DIE((alignmentMask & alignment) == 0, "alignment is not a power of two.");

	size_t const alignedSize = DX12Math::AlignUpWithMask(sizeInBytes, alignmentMask);

	m_curOffset = DX12Math::AlignUpWithMask(m_curOffset, alignmentMask);

	if (m_curOffset + alignedSize > m_totalSize)
	{
		ERROR_AND_DIE("Linear Allocator is out of memory!");
	}

	DynAlloc alloc;
	alloc.m_buffer = m_uploadBuffer; // Base resource
	alloc.m_dataPtr = static_cast<uint8_t*>(m_mappedPtr) + m_curOffset;
	alloc.m_gpuAddress = m_uploadBuffer->GetGPUVirtualAddress() + m_curOffset;
	alloc.m_size = alignedSize;
	alloc.m_offset = m_curOffset;

	m_curOffset += alignedSize;

	return alloc;
}

DynAlloc DX12LinearAllocator::AllocateAnyAlign(size_t sizeInBytes, size_t alignment /*= D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT*/)
{
	GUARANTEE_OR_DIE(alignment > 0, "alignment must be greater than zero.");

	size_t const alignedSize = DX12Math::AlignUpDiv(sizeInBytes, alignment);

	m_curOffset = DX12Math::AlignUpDiv(m_curOffset, alignment);

	if (m_curOffset + alignedSize > m_totalSize)
	{
		ERROR_AND_DIE("Linear Allocator is out of memory!");
	}

	DynAlloc alloc;
	alloc.m_buffer = m_uploadBuffer; // Base resource
	alloc.m_dataPtr = static_cast<uint8_t*>(m_mappedPtr) + m_curOffset;
	alloc.m_gpuAddress = m_uploadBuffer->GetGPUVirtualAddress() + m_curOffset;
	alloc.m_size = alignedSize;
	alloc.m_offset = m_curOffset;

	m_curOffset += alignedSize;

	return alloc;
}

void DX12LinearAllocator::Reset()
{
	m_curOffset = 0;
}

#endif // ENGINE_RENDER_D3D12
