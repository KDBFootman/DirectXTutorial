
#include <DirectXMath.h>
#include "Model.h"

using namespace DirectX;

namespace
{
	struct SimpleVertex
	{
		XMFLOAT3 Pos;
	};

	SimpleVertex vertices[] =
	{
		XMFLOAT3(0.0f,0.5f,0.5f),
		XMFLOAT3(0.5f,-0.5f,0.5f),
		XMFLOAT3(-0.5f,-0.5f,0.5f),
	};
}


//--------------------------------------------------------------------------------------
Model::Model()
	:m_VertexBuffer(nullptr)
{
}


//--------------------------------------------------------------------------------------
HRESULT Model::CreateBuffer(ID3D11Device* const device) {

	HRESULT hr = S_OK;

	// Create the vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;

	hr = device->CreateBuffer(&bd, &InitData, m_VertexBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}
}


//--------------------------------------------------------------------------------------
void Model::RenderModel(ID3D11DeviceContext* const context) {

	// Set primitive topology
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &stride, &offset);

	// Render a triangle
	context->Draw(3, 0);
}