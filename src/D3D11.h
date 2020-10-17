#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include "Memory.h"
#include <DirectXMath.h>
#include "Math.h"


#include "imgui\imgui_impl_dx11.h"
#include "imgui\imgui_impl_win32.h"

float fWidth = 0;
float fHeight = 0;
HWND g_HWND = NULL;

static bool showImGui = false;


#include <iostream>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace Microsoft::WRL;

enum class IDXGISwapChainVMT {
	QueryInterface,
	AddRef,
	Release,
	SetPrivateData,
	SetPrivateDataInterface,
	GetPrivateData,
	GetParent,
	GetDevice,
	Present,
	GetBuffer,
	SetFullscreenState,
	GetFullscreenState,
	GetDesc,
	ResizeBuffers,
	ResizeTarget,
	GetContainingOutput,
	GetFrameStatistics,
	GetLastPresentCount,
};

using fnWinProc = LRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM);
fnWinProc WndProc;
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProcHook(const HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (showImGui)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		return true;
	}

	return CallWindowProc(WndProc, hWnd, msg, wParam, lParam);
}




using fnPresent = HRESULT(__stdcall*)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);

class D3D11
{
public:
	D3D11()
	{}

	~D3D11()
	{}

	void Line(Vec2 start, Vec2 end)
	{
		// Data for Vertex Buffer
		struct Vertex {
			float x, y, z;
			//char r, g, b, a;
		};
		const Vertex vertices[] = {
			{ view_x(start.x, fWidth), view_y(start.y, fHeight), 1.0f },
			{ view_x(end.x, fWidth), view_y(end.y, fHeight), 1.0f }
		};
		// Vertex Buffer
		ComPtr<ID3D11Buffer> pVertexBuffer = nullptr;
		D3D11_BUFFER_DESC vbd = { 0 };
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.Usage = D3D11_USAGE_DEFAULT;
		vbd.CPUAccessFlags = 0;
		vbd.ByteWidth = sizeof(vertices);
		vbd.StructureByteStride = sizeof(Vertex);
		D3D11_SUBRESOURCE_DATA vsd = { 0 };
		vsd.pSysMem = vertices;
		pDevice->CreateBuffer(&vbd, &vsd, pVertexBuffer.GetAddressOf());
		const UINT stride = sizeof(Vertex);
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);



		// Pixel Shader
		ComPtr<ID3D11PixelShader> pPixelShader = nullptr;
		ComPtr<ID3DBlob> pBlob = nullptr;
		D3DReadFileToBlob(L"PS.cso", &pBlob);
		pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader);
		pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);


		// Vertex Shader
		ComPtr<ID3D11VertexShader> pVertexShader = nullptr;
		//ComPtr<ID3DBlob> pBlob = nullptr;
		D3DReadFileToBlob(L"VS.cso", &pBlob);
		pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader);
		pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);


		// Topology
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);


		// Input Layout
		ComPtr<ID3D11InputLayout> pInputLayout = nullptr;
		const D3D11_INPUT_ELEMENT_DESC ied[]{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			//{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		pDevice->CreateInputLayout(ied, (UINT)std::size(ied), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pInputLayout);
		pContext->IASetInputLayout(pInputLayout.Get());

		// Draw Call
		pContext->Draw(2, 0);
	}


	bool Init(IDXGISwapChain* pSwapChain)
	{
		if (!pTarget)
		{
			pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
			pDevice->GetImmediateContext(&pContext);
			pContext->OMGetRenderTargets(1, &pTarget, nullptr);


			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			g_HWND = sd.OutputWindow;

			// Set Viewport
			D3D11_VIEWPORT pViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{ 0 };
			UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
			pContext->RSGetViewports(&numViewports, pViewports);
			fWidth = (float)pViewports[0].Width;
			fHeight = (float)pViewports[0].Height;

			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

			ImGuiStyle* style = &ImGui::GetStyle();

			style->Alpha = 0.9f;
			style->WindowPadding = ImVec2(15, 15);
			style->WindowRounding = 0.0f;
			style->WindowBorderSize = 1.0f;
			style->WindowTitleAlign = ImVec2(1.0f, 0.5f);
			//style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
			style->WindowMenuButtonPosition = ImGuiDir_None;
			style->FramePadding = ImVec2(1, 1);
			style->FrameRounding = 2.0f;
			style->FrameBorderSize = 1.0f;
			style->ItemSpacing = ImVec2(6, 9);
			style->ItemInnerSpacing = ImVec2(10, 10);

			style->ScrollbarSize = 6.0f;
			style->ScrollbarRounding = 0.0f;

			style->WindowMinSize = ImVec2(280.f, 300.f);


			style->Colors[ImGuiCol_Text] = ImVec4(0.756f, 0.749f, 0.866f, 1.0f);
			style->Colors[ImGuiCol_WindowBg] = ImVec4(0.141f, 0.121f, 0.274f, 1.0f);
			style->Colors[ImGuiCol_Border] = ImVec4(0.388f, 0.301f, 0.866f, 1.0f);

			//style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			//style->Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			//style->Colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			//style->Colors[ImGuiCol_BorderShadow] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_FrameBg] = ImVec4(0.117f, 0.101f, 0.227f, 1.0f);
			style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.117f, 0.101f, 0.227f, 1.0f);
			style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.388f, 0.301f, 0.866f, 1.0f);

			style->Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.388f, 0.301f, 0.866f, 1.0f);
			style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

			style->Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

			style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.388f, 0.301f, 0.866f, 1.0f);
			style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.756f, 0.749f, 0.866f, 1.0f);
			style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

			style->Colors[ImGuiCol_CheckMark] = ImVec4(0.388f, 0.301f, 0.866f, 1.0f);

			style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

			style->Colors[ImGuiCol_Button] = ImVec4(0.141f, 0.121f, 0.274f, 1.0f);
			style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.117f, 0.101f, 0.227f, 1.0f);
			style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.388f, 0.301f, 0.866f, 1.0f);

			style->Colors[ImGuiCol_Header] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_Separator] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_Tab] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_TabHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);


			WndProc = (WNDPROC)SetWindowLongPtr(sd.OutputWindow, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
			ImGui_ImplWin32_Init(sd.OutputWindow);
			ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
			ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;



		}

		return true;
	}

	uintptr_t GetPresentFunction()
	{
		if (ogPresent == 0)
		{
			DXGI_SWAP_CHAIN_DESC scd = { 0 };
			scd.BufferDesc.Width = 0;
			scd.BufferDesc.Height = 0;
			scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			scd.SampleDesc.Count = 1;
			scd.SampleDesc.Quality = 0;
			scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scd.BufferCount = 1;
			scd.OutputWindow = GetForegroundWindow();
			scd.Windowed = TRUE;
			scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			scd.Flags = 0;

			HRESULT hr = D3D11CreateDeviceAndSwapChain(
				nullptr, D3D_DRIVER_TYPE_HARDWARE,
				nullptr, 0, nullptr, 0,
				D3D11_SDK_VERSION,
				&scd, &pSwapChain, &pDevice, nullptr, &pContext
			);

			if (FAILED(hr))
				return 0;

			void** pVTM = *(void***)pSwapChain.Get();

			ogPresent = (uintptr_t)(fnPresent)(pVTM[(UINT)IDXGISwapChainVMT::Present]);
		}

		return ogPresent;
	}
private:
	ComPtr<ID3D11Device> pDevice = nullptr;
	ComPtr<IDXGISwapChain> pSwapChain = nullptr;
	ComPtr<ID3D11DeviceContext> pContext = nullptr;
	ComPtr<ID3D11RenderTargetView> pTarget = nullptr;
	uintptr_t ogPresent = 0;
};