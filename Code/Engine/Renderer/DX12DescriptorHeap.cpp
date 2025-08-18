#include "Engine/Renderer/DX12DescriptorHeap.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Core/EngineCommon.hpp"


#ifdef ENGINE_RENDER_D3D12

#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}

void DX12DescriptorHeap::BeginFrame()
{
	m_temporaryAllocated = 0;
	m_heapIndex = (m_heapIndex + 1) % m_numHeaps;
}

DX12DescriptorHeap::DX12DescriptorHeap(uint32_t numPersistent, uint32_t numTemporary, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool isShaderVisible)
	: m_numPersistent(numPersistent)
	, m_numTemporary(numTemporary)
	, m_heapType(heapType)
	, m_isShaderVisible(isShaderVisible)
{
	// Notes: rtv/dsv is not shader visible, they are used in output merger
	// shader invisible resource is not using the GPU handle
	
	m_numHeaps = m_isShaderVisible ? FRAMES_IN_FLIGHT : 1;

	m_deadList.resize(m_numPersistent);
	for (uint32_t i = 0; i < m_numPersistent; ++i)
	{
		m_deadList[i] = i;
	}
	m_persistentInUse.resize(m_numPersistent, false);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_numPersistent + m_numTemporary;
	heapDesc.Type = m_heapType;
	heapDesc.Flags = m_isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	for (uint32_t i = 0; i < m_numHeaps; ++i)
	{
		HRESULT hr = DX12Renderer::s_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heaps[i]));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create descriptor heap");
		m_cpuStart[i] = m_heaps[i]->GetCPUDescriptorHandleForHeapStart();
		if (m_isShaderVisible)
		{
			m_gpuStart[i] = m_heaps[i]->GetGPUDescriptorHandleForHeapStart();
		}
	}

	m_descriptorSize = DX12Renderer::s_device->GetDescriptorHandleIncrementSize(m_heapType);
}

DX12DescriptorHeap::~DX12DescriptorHeap()
{
	// m_persistentAllocated should be 0, all persistent descriptor should be freed
	// Release all resources
	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		DX_SAFE_RELEASE(m_heaps[i]);
	}
}

PersistentDescriptorAlloc DX12DescriptorHeap::AllocatePersistent()
{
	GUARANTEE_OR_DIE(m_persistentAllocated < m_numPersistent, "Could not allocate any more persistent decriptors.");

	uint32_t idx = m_deadList[m_persistentAllocated];
	++m_persistentAllocated;
	GUARANTEE_OR_DIE(!m_persistentInUse[idx], "The index is being used, cannot allocate.");
	m_persistentInUse[idx] = true;

	PersistentDescriptorAlloc alloc;
	alloc.m_index = idx;

	for (uint32_t i = 0; i < m_numHeaps; ++i)
	{
		alloc.m_handles[i] = m_cpuStart[i];
		alloc.m_handles[i].ptr += idx * m_descriptorSize;
	}

	return alloc;
}

void DX12DescriptorHeap::FreePersistent(uint32_t& idx)
{
	if (idx == uint32_t(-1))
	{
		return;
	}

	GUARANTEE_OR_DIE(idx < m_numPersistent, "Out of the persistant descriptor range");
	GUARANTEE_OR_DIE(m_persistentInUse[idx], "The index is not being used, cannot free.");
	m_persistentInUse[idx] = false;

	GUARANTEE_OR_DIE(m_persistentAllocated > 0, "No persistent slot to free");
	m_deadList[m_persistentAllocated - 1] = idx;
	--m_persistentAllocated;
	idx = uint32_t(-1);
}

TempDescriptorAlloc DX12DescriptorHeap::AllocateTemporary(uint32_t count)
{
	GUARANTEE_OR_DIE(count > 0, "Cannot allocate 0 temporary descriptor")
	GUARANTEE_OR_DIE(m_temporaryAllocated < m_numTemporary, "Could not allocate any more temporary decriptors.");
	
	uint32_t startIdx = m_temporaryAllocated + m_numPersistent;
	m_temporaryAllocated += count;


	TempDescriptorAlloc alloc;
	alloc.m_startCPUHandle = m_cpuStart[m_heapIndex];
	alloc.m_startCPUHandle.ptr += startIdx * m_descriptorSize;
	alloc.m_startGPUHandle = m_gpuStart[m_heapIndex];
	alloc.m_startGPUHandle.ptr += startIdx * m_descriptorSize;
	alloc.m_startIndex = startIdx;

	return alloc;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::CPUHandleFromIndex(uint32_t descriptorIdx) const
{
	return CPUHandleFromIndex(descriptorIdx, m_heapIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::CPUHandleFromIndex(uint32_t descriptorIdx, uint32_t heapIdx) const
{
	GUARANTEE_OR_DIE(heapIdx < m_numHeaps, "Invalid heapIdx");
	GUARANTEE_OR_DIE(descriptorIdx < GetTotalNumDescriptors(), "Invalid descriptorIdx");
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cpuStart[heapIdx];
	handle.ptr += descriptorIdx * m_descriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GPUHandleFromIndex(uint32_t descriptorIdx) const
{
	return GPUHandleFromIndex(descriptorIdx, m_heapIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GPUHandleFromIndex(uint32_t descriptorIdx, uint32_t heapIdx) const
{
	GUARANTEE_OR_DIE(heapIdx < m_numHeaps, "Invalid heapIdx");
	GUARANTEE_OR_DIE(descriptorIdx < GetTotalNumDescriptors(), "Invalid descriptorIdx");
	GUARANTEE_OR_DIE(m_isShaderVisible, "Should not get gpu handle, if the heap is not visible to shader");
	D3D12_GPU_DESCRIPTOR_HANDLE  handle = m_gpuStart[heapIdx];
	handle.ptr += descriptorIdx * m_descriptorSize;
	return handle;
}

ID3D12DescriptorHeap* DX12DescriptorHeap::GetCurrentHeap() const
{
	return m_heaps[m_heapIndex];
}

#endif // ENGINE_RENDER_D3D12

