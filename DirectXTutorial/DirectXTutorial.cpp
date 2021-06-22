
#include <windows.h>
#include <memory>

#include "Device.h"
#include "Shader.h"
#include "Model.h"
#include "Camera.h"


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE				g_hInst = nullptr;
HWND					g_hWnd = nullptr;

Microsoft::WRL::ComPtr<ID3D11Texture2D> g_DepthStencil = nullptr;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> g_DepthStencilView = nullptr;

std::unique_ptr<Device>	g_Device;
std::unique_ptr<Shader> g_Shader;
std::unique_ptr<Model> g_Model;
std::unique_ptr<Model> g_Model2;
std::unique_ptr<Camera> g_Camera;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE, int);
HRESULT InitDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Move();
void Render();
void Clear();
void CleanupDevice();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow))) {
		return 0;
	}

	g_Device = std::make_unique<Device>();
	
	if (FAILED(InitDevice())) {
		CleanupDevice();
		g_Device.reset();
		return 0;
	}

	MSG msg = {};
	while (msg.message != WM_QUIT) {

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			Move();
			Render();
		}
	}

	CleanupDevice();
	g_Device.reset();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow) {
	
	// Register class
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(hInstance, IDI_WINLOGO);

	if (!RegisterClassEx(&wcex)) {
		return E_FAIL;
	}

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0,0,720,480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	// For fullscreen
	//g_hWnd = CreateWindowEx(WS_EX_TOPMOST, L"TutorialWindowClass", L"DirectX Tutorial", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
	g_hWnd = CreateWindowEx(0, L"TutorialWindowClass", L"DirectX Tutorial", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

	if (!g_hWnd) {
		return E_FAIL;
	}

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice() {

	HRESULT hr = S_OK;

	RECT rc = {};
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	g_Device->SetWindow(g_hWnd, width, height);

	hr = g_Device->CreateDevice();
	if (FAILED(hr)) {
		return hr;
	}

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth = {};
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_Device->GetD3DDevice()->CreateTexture2D(&descDepth, nullptr, g_DepthStencil.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	// Create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_Device->GetD3DDevice()->CreateDepthStencilView(g_DepthStencil.Get(), &descDSV, g_DepthStencilView.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	auto renderTarget = g_Device->GetRenderTargetView();
	g_Device->GetD3DDeviceContext()->OMSetRenderTargets(1, &renderTarget, g_DepthStencilView.Get());

	g_Shader = std::make_unique<Shader>();
	hr = g_Shader->CreateShader(g_Device->GetD3DDevice());
	if (FAILED(hr)) {
		return hr;
	}

	g_Model = std::make_unique<Model>();
	hr = g_Model->CreateModel(g_Device->GetD3DDevice());
	if (FAILED(hr)) {
		return hr;
	}

	g_Model2 = std::make_unique<Model>();
	hr = g_Model2->CreateModel(g_Device->GetD3DDevice());
	if (FAILED(hr)) {
		return hr;
	}

	g_Camera = std::make_unique<Camera>();

	// Initialize the view matrix
	const auto Eye = DirectX::XMVectorSet(0.0f, 1.5f, -5.0f, 0.0f);
	const auto At = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_Camera->SetCamera(Eye, At);

	// Initialize the projection matrix
	const auto fov = DirectX::XM_PIDIV2;
	g_Camera->SetScreen(fov, width / (float)height, 0.01f, 100.0f);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	PAINTSTRUCT ps = {};
	HDC hdc = nullptr;

	switch (message) {

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Move the frame
//--------------------------------------------------------------------------------------
void Move() {

	// Update our time
	static float t = 0.0f;
	if (g_Device->GetD3DDriverType() == D3D_DRIVER_TYPE_REFERENCE) {
		t += (float)DirectX::XM_PI * 0.0125f;
	}
	else {
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount64();
		if (dwTimeStart == 0) {
			dwTimeStart = dwTimeCur;
		}
		t = (float)(dwTimeCur - dwTimeStart) / 1000.0f;
	}

	// Animate the cube
	g_Model->MoveModel([]()
	{
		return DirectX::XMMatrixRotationY(t);
	});

	g_Model2->MoveModel([]()
	{
		const auto mScale = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f);
		const auto mSpin = DirectX::XMMatrixRotationZ(-t);
		const auto mTranslate = DirectX::XMMatrixTranslation(-3.0f, 0.0f, 0.0f);
		const auto mOrbit = DirectX::XMMatrixRotationY(-t * 2.0f);
		return mScale * mSpin * mTranslate * mOrbit;
	});
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render() {

	Clear();

	auto world = g_Model->GetWorld();
	auto view = g_Camera->GetView();
	auto projection = g_Camera->GetProjection();

	g_Shader->SetRenderShader(g_Device->GetD3DDeviceContext(), world, view, projection);

	g_Model->RenderModel(g_Device->GetD3DDeviceContext());

	// Render second cube
	auto world2 = g_Model2->GetWorld();

	g_Shader->SetRenderShader(g_Device->GetD3DDeviceContext(), world2, view, projection);

	g_Model2->RenderModel(g_Device->GetD3DDeviceContext());

	g_Device->Present();
}


//--------------------------------------------------------------------------------------
// Just clear the backbuffer
//--------------------------------------------------------------------------------------
void Clear() {

	auto context = g_Device->GetD3DDeviceContext();
	auto renderTarget = g_Device->GetRenderTargetView();

	float ClearColor[4] = { 0.0f,0.125f,0.3f,1.0f };
	context->ClearRenderTargetView(renderTarget, ClearColor);

	// Clear the depth buffer to 1.0 (max depth)
	context->ClearDepthStencilView(g_DepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	context->OMSetRenderTargets(1, &renderTarget, g_DepthStencilView.Get());

	// Set the viewport
	auto viewport = g_Device->GetScreenViewport();
	context->RSSetViewports(1, &viewport);
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice() {

	auto context = g_Device->GetD3DDeviceContext();
	context->ClearState();

	g_DepthStencil.Reset();
	g_DepthStencilView.Reset();

	g_Camera.reset();
	g_Model2.reset();
	g_Model.reset();
	g_Shader.reset();
}