#pragma once

//-----------------------------------------------------------------------------------------------
struct ID3D11Device;
struct ID3D11Buffer;

//-----------------------------------------------------------------------------------------------
class ConstantBuffer
{
	friend class Renderer;
	friend class DX11Renderer;

public:
	ConstantBuffer(ID3D11Device* device, size_t size);
	ConstantBuffer(const ConstantBuffer& copy) = delete;
	virtual ~ConstantBuffer();

private:
	void Create();

private:
	ID3D11Device* m_device = nullptr;
	ID3D11Buffer* m_buffer = nullptr;
	size_t m_size = 0;
};

