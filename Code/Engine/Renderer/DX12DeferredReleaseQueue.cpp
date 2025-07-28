#include "Engine/Renderer/DX12DeferredReleaseQueue.hpp"

#ifdef ENGINE_RENDER_D3D12
#include <d3d12.h>

DX12DeferredReleaseQueue::~DX12DeferredReleaseQueue()
{
	for (DeferredReleaseItem& item : m_queue)
	{
		if (item.resource)
			item.resource->Release();
	}
	m_queue.clear();
}

void DX12DeferredReleaseQueue::Enqueue(uint64_t fenceValue, IUnknown* resource)
{
	if (resource)
	{
		m_queue.emplace_back(DeferredReleaseItem{ fenceValue, resource });
	}
}

void DX12DeferredReleaseQueue::Process(uint64_t completedFenceValue)
{
	auto it = m_queue.begin();
	while (it != m_queue.end())
	{
		if (it->fenceValue <= completedFenceValue)
		{
			if (it->resource)
			{
				it->resource->Release();
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

