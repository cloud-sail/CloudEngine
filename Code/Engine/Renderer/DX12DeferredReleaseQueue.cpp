#include "Engine/Renderer/DX12DeferredReleaseQueue.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

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

DX12DeferredReleaseQueue::DX12DeferredReleaseQueue(uint32_t maxResourceReleasesPerFrame)
	: m_maxResourceReleasesPerFrame(maxResourceReleasesPerFrame)
{

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
		if (it->fenceValue <= completedFenceValue &&
			it->type == DeferredReleaseType::Descriptor)
		{
			if (it->heap && it->index != uint32_t(-1))
			{
				it->heap->FreePersistent(it->index);
			}
			it = m_queue.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Second Pass
	uint32_t resourceReleasedCount = 0;
	it = m_queue.begin();
	while (it != m_queue.end())
	{
		if (it->fenceValue <= completedFenceValue &&
			it->type == DeferredReleaseType::Resource)
		{
			if (resourceReleasedCount >= m_maxResourceReleasesPerFrame)
			{
				break;
			}

			if (it->resource)
			{
				it->resource->Release();
			}
			it = m_queue.erase(it);
			++resourceReleasedCount;
		}
		else
		{
			++it;
		}
	}

	//uint32_t processedCount = 0;

	//auto it = m_queue.begin();
	//while (it != m_queue.end() && processedCount < m_maxResourceReleasesPerFrame)
	//{
	//	if (it->fenceValue <= completedFenceValue)
	//	{
	//		if (it->type == DeferredReleaseType::Resource)
	//		{
	//			if (it->resource)
	//			{
	//				it->resource->Release();
	//			}
	//			++processedCount;
	//		}
	//		else if (it->type == DeferredReleaseType::Descriptor)
	//		{
	//			if (it->heap && it->index != uint32_t(-1))
	//			{
	//				it->heap->FreePersistent(it->index);
	//			}
	//		}
	//		it = m_queue.erase(it);
	//	}
	//	else
	//	{
	//		++it;
	//	}
	//}
}
#endif // ENGINE_RENDER_D3D12

