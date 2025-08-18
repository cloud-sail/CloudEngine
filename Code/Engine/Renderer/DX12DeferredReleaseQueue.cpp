#include "Engine/Renderer/DX12DeferredReleaseQueue.hpp"

#ifdef ENGINE_RENDER_D3D12
#include "Engine/Renderer/DX12DescriptorHeap.hpp"
#include <d3d12.h>

DX12DeferredReleaseQueue::~DX12DeferredReleaseQueue()
{
	for (DeferredReleaseItem& item : m_queue)
	{
		if (item.type == DeferredReleaseType::Resource)
		{
			if (item.resource)
			{
				item.resource->Release();
			}
		}
		else if (item.type == DeferredReleaseType::Descriptor)
		{
			if (item.heap && item.index != uint32_t(-1))
			{
				item.heap->FreePersistent(item.index);
			}
		}
	}
	m_queue.clear();
}

void DX12DeferredReleaseQueue::EnqueueResource(uint64_t fenceValue, IUnknown* resource)
{
	if (resource)
	{
		m_queue.emplace_back(fenceValue, resource);
	}
}

void DX12DeferredReleaseQueue::EnqueueDescriptor(uint64_t fenceValue, DX12DescriptorHeap* heap, uint32_t index)
{
	if (heap && index != uint32_t(-1))
	{
		m_queue.emplace_back(fenceValue, heap, index);
	}
}

void DX12DeferredReleaseQueue::Process(uint64_t completedFenceValue)
{
	auto it = m_queue.begin();
	while (it != m_queue.end())
	{
		if (it->fenceValue <= completedFenceValue)
		{
			if (it->type == DeferredReleaseType::Resource)
			{
				if (it->resource)
				{
					it->resource->Release();
				}
			}
			else if (it->type == DeferredReleaseType::Descriptor)
			{
				if (it->heap && it->index != uint32_t(-1))
				{
					it->heap->FreePersistent(it->index);
				}
			}
			it = m_queue.erase(it);
		}
		else
		{
			++it;
		}
	}
}
#endif // ENGINE_RENDER_D3D12

