
#include <d3dcompiler.h>
#include "Shader.h"


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


//--------------------------------------------------------------------------------------
Shader::Shader()
	:m_VertexShader(nullptr)
	,m_PixelShader(nullptr)
	,m_VertexLayout(nullptr)
{
}


//--------------------------------------------------------------------------------------
HRESULT Shader::CreateShader(ID3D11Device* const device) {

	HRESULT hr = S_OK;

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

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = device->CreateInputLayout(layout, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_VertexLayout.ReleaseAndGetAddressOf());
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
}


//--------------------------------------------------------------------------------------
void Shader::SetRenderShader(ID3D11DeviceContext* const context) {

	// Set the input layout
	context->IASetInputLayout(m_VertexLayout.Get());

	// Set Vertex & Pixel Shader
	context->VSSetShader(m_VertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_PixelShader.Get(), nullptr, 0);

}
