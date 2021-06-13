
#pragma once

#include <wrl/client.h>
#include <d3d11_2.h>

class Device {

public:
	Device(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM, UINT backBufferCount = 2, D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_10_0, bool vSync = true);
	~Device() = default;

	void SetWindow(HWND, int, int);
	HRESULT CreateDevice();
	void Present();
	
	D3D_DRIVER_TYPE GetD3DDriverType() { return m_DriverType; }
	ID3D11Device* GetD3DDevice() const { return m_D3DDevice.Get(); }
	ID3D11DeviceContext* GetD3DDeviceContext() const { return m_D3DDeviceContext.Get(); }
	ID3D11RenderTargetView* GetRenderTargetView() const { return m_RenderTargetView.Get(); }
	D3D11_VIEWPORT GetScreenViewport() const { return m_ScreenViewport; }
	

private:
	D3D_DRIVER_TYPE									m_DriverType;
	D3D_FEATURE_LEVEL								m_FeatureLevel;
	Microsoft::WRL::ComPtr<ID3D11Device>			m_D3DDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		m_D3DDeviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_RenderTargetView;
	D3D11_VIEWPORT									m_ScreenViewport;

	DXGI_FORMAT										m_BackBufferFormat;
	UINT											m_BackBufferCount;
	D3D_FEATURE_LEVEL								m_MinFeatureLevel;

	HWND											m_hWnd;
	bool											m_VSync;
};