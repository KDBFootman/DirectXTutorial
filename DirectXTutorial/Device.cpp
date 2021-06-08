
#include "Device.h"

#define SAFE_RELEASE(p) {	if(p) {	(p)->Release();	(p) = nullptr;	}	}


//--------------------------------------------------------------------------------------
Device::Device(DXGI_FORMAT backBufferFormat,UINT backBufferCount,D3D_FEATURE_LEVEL minFeatureLevel, bool vSync)
	:m_DriverType(D3D_DRIVER_TYPE_NULL)
	,m_FeatureLevel(D3D_FEATURE_LEVEL_9_1)
	,m_D3DDevice(nullptr)
	,m_D3DDeviceContext(nullptr)
	,m_SwapChain(nullptr)
	,m_RenderTargetView(nullptr)
	,m_ScreenViewport{0.0f,0.0f,1.0f,1.0f}
	,m_BackBufferFormat(backBufferFormat)
	,m_BackBufferCount(backBufferCount)
	,m_MinFeatureLevel(minFeatureLevel)
	,m_hWnd(nullptr)
	,m_VSync(vSync)
{
}


//--------------------------------------------------------------------------------------
void Device::SetWindow(HWND window, int width, int height) {

	m_hWnd = window;
	m_ScreenViewport = CD3D11_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
}


//--------------------------------------------------------------------------------------
HRESULT Device::CreateDevice() {

	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT featureLevelCount = 0;
	for (; featureLevelCount < _countof(featureLevels); ++featureLevelCount) {
		if (featureLevels[featureLevelCount] < m_MinFeatureLevel) {
			break;
		}
	}

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = static_cast<UINT>(m_ScreenViewport.Width);
	sd.BufferDesc.Height = static_cast<UINT>(m_ScreenViewport.Height);
	sd.BufferDesc.Format = m_BackBufferFormat;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = m_BackBufferCount;
	sd.OutputWindow = m_hWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	if (m_VSync) {
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	}
	else {
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	for (auto& driverType : driverTypes) {
		m_DriverType = driverType;
		hr = D3D11CreateDeviceAndSwapChain(nullptr, m_DriverType, nullptr, createDeviceFlags, featureLevels, featureLevelCount, D3D11_SDK_VERSION, &sd, m_SwapChain.ReleaseAndGetAddressOf(), m_D3DDevice.ReleaseAndGetAddressOf(), &m_FeatureLevel, m_D3DDeviceContext.ReleaseAndGetAddressOf());
		if (SUCCEEDED(hr)) {
			break;
		}
	}
	if (FAILED(hr)) {
		return hr;
	}

	// Create a render target view
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
	hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)pBackBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_D3DDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, m_RenderTargetView.ReleaseAndGetAddressOf());
	pBackBuffer.Reset();
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}


//--------------------------------------------------------------------------------------
void Device::Present() {

	if (m_VSync) {
		m_SwapChain->Present(1, 0);
	}
	else {
		m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}
}
