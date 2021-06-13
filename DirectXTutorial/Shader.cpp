
//#define CompileShader	// require d3dcompiler.lib

#include <d3dcompiler.h>
#include "Shader.h"

#ifndef CompileShader
#include "ReadData.h"
#endif // !CompileShader

#ifdef CompileShader

namespace
{
	//--------------------------------------------------------------------------------------
	// Helper for compiling shaders
	//--------------------------------------------------------------------------------------
	HRESULT CompileShaderFromFile(LPCWSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
	{
		HRESULT hr = S_OK;

		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined (DEBUG)||(_DEBUG)
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows
		// the shaders to be optimized and to run exactly the way they will run in
		// the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG;
		// Disable optimizations to further improve shader debugging
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
		hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, pErrorBlob.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			if (pErrorBlob) {
				OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
				pErrorBlob.Reset();
			}
			return hr;
		}
		pErrorBlob.Reset();

		return S_OK;
	}
}

#endif // CompileShader

using namespace DirectX;

namespace
{
	struct ConstantBuffer
	{
		XMMATRIX mWorld;
		XMMATRIX mView;
		XMMATRIX mProjection;
	};
	static_assert((sizeof(ConstantBuffer) % 16) == 0, "Constant buffer must always be 16-byte aligned");
}


//--------------------------------------------------------------------------------------
Shader::Shader()
	:m_VertexShader(nullptr)
	,m_PixelShader(nullptr)
	,m_VertexLayout(nullptr)
	,m_ConstantBuffer(nullptr)
{
}


//--------------------------------------------------------------------------------------
HRESULT Shader::CreateShader(ID3D11Device* const device) {

	HRESULT hr = S_OK;

#ifndef CompileShader

	// Load the vertex shader
	auto vertexShaderBlob = DX::ReadData(L"Data\\TutorialVS.cso");
	
	// Create the vertex shader
	hr = device->CreateVertexShader(vertexShaderBlob.data(), vertexShaderBlob.size(), nullptr, m_VertexShader.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		vertexShaderBlob.clear();
		return hr;
	}

#endif // !CompileShader

#ifdef CompileShader

	// Compile the vertex shader
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial.hlsli", "VS", "vs_4_0", vsBlob.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MessageBox(nullptr, L"The FX file cannot be compiled. Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_VertexShader.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		vsBlob.Reset();
		return hr;
	}

#endif // CompileShader

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

#ifndef CompileShader

	// Create the input layout
	hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBlob.data(), vertexShaderBlob.size(), m_VertexLayout.ReleaseAndGetAddressOf());
	vertexShaderBlob.clear();
	if (FAILED(hr)) {
		return hr;
	}

	// Load the pixel shader
	auto pixelShaderBlob = DX::ReadData(L"Data\\TutorialPS.cso");

	// Create the pixel shader
	hr = device->CreatePixelShader(pixelShaderBlob.data(), pixelShaderBlob.size(), nullptr, m_PixelShader.ReleaseAndGetAddressOf());
	pixelShaderBlob.clear();
	if (FAILED(hr)) {
		return hr;
	}

#endif // !CompileShader

#ifdef CompileShader

	// Create the input layout
	hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_VertexLayout.ReleaseAndGetAddressOf());
	vsBlob.Reset();
	if (FAILED(hr)) {
		return hr;
	}

	// Compile the pixel shader
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial.hlsli", "PS", "ps_4_0", psBlob.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MessageBox(nullptr, L"The FX file cannot be compiled. Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_PixelShader.ReleaseAndGetAddressOf());
	psBlob.Reset();
	if (FAILED(hr)) {
		return hr;
	}

#endif // CompileShader

	// Create the constant buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = device->CreateBuffer(&bd, nullptr, m_ConstantBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}


//--------------------------------------------------------------------------------------
void Shader::SetRenderShader(ID3D11DeviceContext* const context, XMMATRIX& world, XMMATRIX& view, XMMATRIX& projection) {

	// Update variables
	ConstantBuffer cb = {};
	cb.mWorld = DirectX::XMMatrixTranspose(world);
	cb.mView = DirectX::XMMatrixTranspose(view);
	cb.mProjection = DirectX::XMMatrixTranspose(projection);
	context->UpdateSubresource(m_ConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// Set the input layout
	context->IASetInputLayout(m_VertexLayout.Get());

	// Set Vertex & Pixel Shader
	context->VSSetShader(m_VertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
	context->PSSetShader(m_PixelShader.Get(), nullptr, 0);

}
