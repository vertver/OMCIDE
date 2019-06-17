/*********************************************************
* Copyright (C) VERTVER, 2019. All rights reserved.
* OMCIDE - Open Micro-controller IDE
* License: GPLv3
**********************************************************
* Module Name: GUI Render
*********************************************************/
#define COBJMACROS 1
#include "GUI_Render.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#ifdef UI_PART
#include "nuklear.h"
#include "demo/d3d11/nuklear_d3d11.h"
#endif

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_INDEX_BUFFER 128 * 1024

PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN         pD3D11CreateDevice = NULL;

int RenType = -1;		// not initialized
struct nk_context *ctx;
struct nk_colorf bg;
static IDXGISwapChain *pSwapChain;
static ID3D11Device *pDevice;
static ID3D11DeviceContext *pContext;
static ID3D11RenderTargetView* pRTView;
extern struct nk_context *ctx;
extern struct nk_colorf bg;

boolean
OMCRenderInit(int RenderType)
{
	boolean bVal = false;

	switch (RenderType)
	{
	case 0:
		bVal = OMCRenderInitD3D11();
		if (bVal)
		{ 
			RenType = 0; 	
			break;
		}
		OMCRenderDestroyD3D11();

	case 1:
		bVal = OMCRenderInitOGL();
		if (bVal)
		{
			RenType = 1;
			break;
		}
		OMCRenderDestroyOGL();

	case 2:
		bVal = OMCRenderInitGDI();
		if (bVal) { RenType = 2; }
	default:
		break;
	}

	return bVal;
}

void
OMCRenderRestart(int RenderType) 
{
	OMCRenderDestroy();

}

void
OMCRenderDestroy() 
{
	switch (RenType)
	{
	case 0:
		OMCRenderDestroyD3D11();
		break;
	case 1:
		OMCRenderDestroyOGL();
		break;
	case 2:
		OMCRenderDestroyGDI();
		break;
	default:
		break;
	}
}

void
OMCRenderDraw() 
{
	switch (RenType)
	{
	case 0:
		OMCRenderDrawD3D11();
		break;
	case 1:
		OMCRenderDrawOGL();
		break;
	case 2:
		OMCRenderDrawGDI();
		break;
	default:
		break;
	}
}

void
OMCRenderResize(
	int Width,
	int Height
)
{
	switch (RenType)
	{
	case 0:
		OMCRenderResizeD3D11(Width, Height);
		break;
	case 1:
		OMCRenderResizeOGL(Width, Height);
		break;
	case 2:
		OMCRenderResizeGDI(Width, Height);
		break;
	default:
		break;
	}
}

int
OMCRenderGetType() 
{
	return RenType;
}

void
OMCRenderNuklear()
{
	/* GUI */
	if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
		NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		enum { EASY, HARD };
		static int op = EASY;
		static int property = 20;

		nk_layout_row_static(ctx, 30, 80, 1);
		if (nk_button_label(ctx, "button"))
			fprintf(stdout, "button pressed\n");
		nk_layout_row_dynamic(ctx, 30, 2);
		if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
		if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
		nk_layout_row_dynamic(ctx, 22, 1);
		nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

		nk_layout_row_dynamic(ctx, 20, 1);
		nk_label(ctx, "background:", NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 25, 1);
		if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400))) {
			nk_layout_row_dynamic(ctx, 120, 1);
			bg = nk_color_picker(ctx, bg, NK_RGBA);
			nk_layout_row_dynamic(ctx, 25, 1);
			bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
			bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
			bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
			bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
			nk_combo_end(ctx);
		}
	}
	nk_end(ctx);
}

boolean
OMCRenderInitD3D11()
{
	D3D_FEATURE_LEVEL feature_level;
	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	ID3D11Texture2D* back_buffer = NULL;
	D3D11_RENDER_TARGET_VIEW_DESC desc;
	HRESULT hr = 0;

	if (!pD3D11CreateDevice)
	{
		void* hLib = OMCLoadLibrary("d3d11.dll");
		pD3D11CreateDevice = OMCGetProc(hLib, "D3D11CreateDeviceAndSwapChain");
	}

	void* hWnd = OMCMainWindowCreate();

	/*
		Init D3D11 device
	*/
	memset(&swap_chain_desc, 0, sizeof(swap_chain_desc));
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = 1;
	swap_chain_desc.OutputWindow = hWnd;
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_chain_desc.Flags = 0;

	if (FAILED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &swap_chain_desc, &pSwapChain, &pDevice, &feature_level, &pContext)))
	{
		/*
			If hardware device fails, then try WARP high-performance
			software rasterizer, this is useful for RDP sessions
		*/
		hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &swap_chain_desc, &pSwapChain, &pDevice, &feature_level, &pContext);
	}

	if (pRTView) ID3D11RenderTargetView_Release(pRTView);

	ID3D11DeviceContext_OMSetRenderTargets(pContext, 0, NULL, NULL);

	hr = IDXGISwapChain_ResizeBuffers(pSwapChain, 0, 640, 480, DXGI_FORMAT_UNKNOWN, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
	{
		return;
	}

	memset(&desc, 0, sizeof(desc));
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	hr = IDXGISwapChain_GetBuffer(pSwapChain, 0, &IID_ID3D11Texture2D, (void **)&back_buffer);
	hr = ID3D11Device_CreateRenderTargetView(pDevice, (ID3D11Resource *)back_buffer, &desc, &pRTView);

	ID3D11Texture2D_Release(back_buffer);

	/* GUI */
	ctx = nk_d3d11_init(pDevice, 640, 480, MAX_VERTEX_BUFFER, MAX_INDEX_BUFFER);

	struct nk_font_atlas *atlas;
	nk_d3d11_font_stash_begin(&atlas);
	nk_d3d11_font_stash_end();

	return true;
}

void
OMCRenderDestroyD3D11()
{
	if (pContext) ID3D11DeviceContext_ClearState(pContext);

	nk_d3d11_shutdown();

	if (pRTView) ID3D11RenderTargetView_Release(pRTView);
	if (pContext) ID3D11DeviceContext_Release(pContext);
	if (pDevice) ID3D11Device_Release(pDevice);
	if (pSwapChain) IDXGISwapChain_Release(pSwapChain);
	UnregisterClassW(L"OMCIDE_WINDOW", GetModuleHandle(0));
}

void
OMCRenderDrawD3D11()
{
	/* Draw */
	ID3D11DeviceContext_ClearRenderTargetView(pContext, pRTView, &bg.r);
	ID3D11DeviceContext_OMSetRenderTargets(pContext, 1, &pRTView, NULL);

	nk_d3d11_render(pContext, NK_ANTI_ALIASING_ON);

	HRESULT hr = IDXGISwapChain_Present(pSwapChain, 1, 0);
	if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		return;
	}
	else if (hr == DXGI_STATUS_OCCLUDED)
	{
		/*
			Window is not visible, so VSync won't work. Let's sleep a bit to reduce CPU usage
		*/
		Sleep(10);
	}

	OMCRenderNuklear();
}

int
NuklearHandleEvent(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return nk_d3d11_handle_event(wnd, msg, wparam, lparam);
}

int
NuklearInputBegin()
{
	nk_input_begin(ctx);
}

int
NuklearInputEnd()
{
	nk_input_end(ctx);
}

void
OMCRenderResizeD3D11(
	int Width,
	int Height
)
{
	ID3D11Texture2D* back_buffer = NULL;
	D3D11_RENDER_TARGET_VIEW_DESC desc;
	HRESULT hr = 0;

	if (pRTView) ID3D11RenderTargetView_Release(pRTView);

	ID3D11DeviceContext_OMSetRenderTargets(pContext, 0, NULL, NULL);

	hr = IDXGISwapChain_ResizeBuffers(pSwapChain, 0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
	{
		return;
	}

	memset(&desc, 0, sizeof(desc));
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	hr = IDXGISwapChain_GetBuffer(pSwapChain, 0, &IID_ID3D11Texture2D, (void **)&back_buffer);
	hr = ID3D11Device_CreateRenderTargetView(pDevice, (ID3D11Resource *)back_buffer, &desc, &pRTView);

	ID3D11Texture2D_Release(back_buffer);

	nk_d3d11_resize(pContext, Width, Height);
}

boolean
OMCRenderInitOGL()
{

}

boolean
OMCRenderInitGDI()
{

}

void
OMCRenderDestroyOGL()
{

}

void
OMCRenderDestroyGDI()
{

}

void
OMCRenderDrawOGL()
{

}

void
OMCRenderDrawGDI()
{

}

void
OMCRenderResizeOGL(
	int Width,
	int Height
)
{

}

void
OMCRenderResizeGDI(
	int Width,
	int Height
)
{

}