//-----------------------------------------------------------------
// ���ܣ�Direct2D�ü����ܣ�������ϸ������ο���http://www.cnblogs.com/Ray1024/
// ���ߣ�Ray1024
// ��ַ��http://www.cnblogs.com/Ray1024/
//-----------------------------------------------------------------

#include "D2DClip.h"

//-----------------------------------------------------------------
// ����Դ�ļ�����D2Dλͼ
//-----------------------------------------------------------------
HRESULT LoadBitmapFromFile(
	ID2D1RenderTarget *pRenderTarget,
	IWICImagingFactory *pIWICFactory,
	LPCWSTR uri,
	UINT width,
	UINT height,
	ID2D1Bitmap **ppBitmap)
{
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pSource = NULL;
	IWICStream *pStream = NULL;
	IWICFormatConverter *pConverter = NULL;
	IWICBitmapScaler *pScaler = NULL;

	// ����λͼ-------------------------------------------------
	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
		uri,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
		);

	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pSource);
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateFormatConverter(&pConverter);
	}

	if (SUCCEEDED(hr))
	{
		if (width != 0 || height != 0)
		{
			UINT originalWidth, originalHeight;
			hr = pSource->GetSize(&originalWidth, &originalHeight);
			if (SUCCEEDED(hr))
			{
				if (width == 0)
				{
					FLOAT scalar = static_cast<FLOAT>(height) / static_cast<FLOAT>(originalHeight);
					width = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
				}
				else if (height == 0)
				{
					FLOAT scalar = static_cast<FLOAT>(width) / static_cast<FLOAT>(originalWidth);
					height = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
				}

				hr = pIWICFactory->CreateBitmapScaler(&pScaler);
				if (SUCCEEDED(hr))
				{
					hr = pScaler->Initialize(
						pSource,
						width,
						height,
						WICBitmapInterpolationModeCubic
						);
				}
				if (SUCCEEDED(hr))
				{
					hr = pConverter->Initialize(
						pScaler,
						GUID_WICPixelFormat32bppPBGRA,
						WICBitmapDitherTypeNone,
						NULL,
						0.f,
						WICBitmapPaletteTypeMedianCut
						);
				}
			}
		}
		else // Don't scale the image.
		{
			hr = pConverter->Initialize(
				pSource,
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				NULL,
				0.f,
				WICBitmapPaletteTypeMedianCut
				);
		}
	}
	if (SUCCEEDED(hr))
	{

		// Create a Direct2D bitmap from the WIC bitmap.
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			ppBitmap
			);
	}

	SafeRelease(&pDecoder);
	SafeRelease(&pSource);
	SafeRelease(&pStream);
	SafeRelease(&pConverter);
	SafeRelease(&pScaler);

	return hr;
}



/******************************************************************
�������
******************************************************************/
int WINAPI WinMain(
    HINSTANCE	/* hInstance */		,
    HINSTANCE	/* hPrevInstance */	,
    LPSTR		/* lpCmdLine */		,
    int			/* nCmdShow */		)
{
    // Ignoring the return value because we want to continue running even in the
    // unlikely event that HeapSetInformation fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            DemoApp app;

            if (SUCCEEDED(app.Initialize()))
            {
                app.RunMessageLoop();
            }
        }
        CoUninitialize();
    }

    return 0;
}

/******************************************************************
DemoAppʵ��
******************************************************************/

DemoApp::DemoApp() :
    m_hwnd(NULL),
    m_pD2DFactory(NULL),
	m_pDWriteFactory(NULL),
    m_pRT(NULL),
	m_pWICFactory(NULL),
	m_pBitmap(NULL),
	m_pPathGeometry(NULL),
	m_pBrush(NULL),
	m_pTextFormat(NULL)
{
}

DemoApp::~DemoApp()
{
	SafeRelease(&m_pWICFactory);
    SafeRelease(&m_pD2DFactory);
	SafeRelease(&m_pDWriteFactory);
    SafeRelease(&m_pRT);
    SafeRelease(&m_pBitmap);
	SafeRelease(&m_pPathGeometry);
	SafeRelease(&m_pBrush);
	SafeRelease(&m_pTextFormat);
}

HRESULT DemoApp::Initialize()
{
    HRESULT hr;

    //register window class
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = DemoApp::WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = sizeof(LONG_PTR);
    wcex.hInstance     = HINST_THISCOMPONENT;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = L"D2DDemoApp";

    RegisterClassEx(&wcex);

    hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr))
    {
        // Create the application window.
        //
        // Because the CreateWindow function takes its size in pixels, we
        // obtain the system DPI and use it to scale the window size.
        FLOAT dpiX, dpiY;
        m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);

        m_hwnd = CreateWindow(
            L"D2DDemoApp",
            L"D2DClip",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            static_cast<UINT>(ceil(720.f * dpiX / 96.f)),
            static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            this
            );
        hr = m_hwnd ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
			ShowWindow(m_hwnd, SW_SHOWNORMAL);

			UpdateWindow(m_hwnd);
        }
    }

    return hr;
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{
    HRESULT hr;
    ID2D1GeometrySink *pSink = NULL;

    // ����D2D����
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

	if (m_pDWriteFactory == NULL && SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
			reinterpret_cast<IUnknown **>(&m_pDWriteFactory));
	}

	if (m_pWICFactory == NULL && SUCCEEDED(hr))
	{
		if (!SUCCEEDED(
			CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_pWICFactory)
			)
			))
			return FALSE;
	}

    return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_pRT)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
            );

        // ����render target
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRT
            );

		// ����λͼ
		if (SUCCEEDED(hr))
		{
			LoadBitmapFromFile(m_pRT,m_pWICFactory, L"bitmap.png",0,0, &m_pBitmap);
		}

		// ����·������ͼ��

		ID2D1GeometrySink* pSink = NULL;
		if (SUCCEEDED(hr))
		{
			hr = m_pD2DFactory->CreatePathGeometry(&m_pPathGeometry);
		}

		// Write to the path geometry using the geometry sink to create a star.
		if (SUCCEEDED(hr))
		{
			hr = m_pPathGeometry->Open(&pSink);
		}
		if (SUCCEEDED(hr))
		{
			pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
			pSink->BeginFigure(D2D1::Point2F(20+ 300, 50 + 100), D2D1_FIGURE_BEGIN_FILLED);
			pSink->AddLine(D2D1::Point2F(130+ 300, 50 + 100));
			pSink->AddLine(D2D1::Point2F(20+ 300, 130 + 100));
			pSink->AddLine(D2D1::Point2F(80+ 300, 0 + 100));
			pSink->AddLine(D2D1::Point2F(130+ 300, 130 + 100));
			pSink->EndFigure(D2D1_FIGURE_END_CLOSED);

			hr = pSink->Close();
		}

		SafeRelease(&pSink);

		// ������ˢ
		if (SUCCEEDED(hr))
		{
			hr = m_pRT->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Red),
				&m_pBrush
				);
		}

		// ���������ʽ
		if (SUCCEEDED(hr))
		{
			hr = m_pDWriteFactory->CreateTextFormat( 
				L"Edwardian Script ITC",
				NULL, 
				DWRITE_FONT_WEIGHT_BOLD, 
				DWRITE_FONT_STYLE_NORMAL, 
				DWRITE_FONT_STRETCH_NORMAL, 
				20.0, 
				L"en-us",
				&m_pTextFormat);
		}
    }

    return hr;
}

void DemoApp::DiscardDeviceResources()
{
    SafeRelease(&m_pRT);
    SafeRelease(&m_pBitmap);
	SafeRelease(&m_pPathGeometry);
	SafeRelease(&m_pBrush);
	SafeRelease(&m_pTextFormat);
}

void DemoApp::RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

float g_rotation = 0;

HRESULT DemoApp::OnRender()
{
    HRESULT hr;

	g_rotation++;

    hr = CreateDeviceResources();
    if (SUCCEEDED(hr) && !(m_pRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        // ��ʼ����
        m_pRT->BeginDraw();

        m_pRT->Clear();

		//////////////////////////////////////////////////////////////////////////
		// ʹ����ԳƵľ��ν��м���
		//////////////////////////////////////////////////////////////////////////

		m_pRT->PushAxisAlignedClip(	D2D1::RectF(50, 100, 250, 200), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

		m_pRT->SetTransform(D2D1::Matrix3x2F::Rotation(g_rotation, D2D1::Point2F(m_pBitmap->GetSize().width/2, m_pBitmap->GetSize().height/2)));

		m_pRT->DrawBitmap(
			m_pBitmap,
			D2D1::RectF(0,0,
			m_pBitmap->GetSize().width,
			m_pBitmap->GetSize().height));

		m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());

		m_pRT->PopAxisAlignedClip();

		m_pRT->DrawText(
			L"���μ���",
			wcslen(L"���μ���"),
			m_pTextFormat,
			D2D1::RectF(50,250,250,300),
			m_pBrush);

		//////////////////////////////////////////////////////////////////////////
		// ʹ�ü���ͼ���ɰ���м���
		//////////////////////////////////////////////////////////////////////////

		ID2D1Layer *pLayer = NULL;
		hr = m_pRT->CreateLayer(NULL, &pLayer);

		if (SUCCEEDED(hr))
		{
			// Push the layer with the geometric mask.
			m_pRT->PushLayer(
				D2D1::LayerParameters(D2D1::InfiniteRect(), m_pPathGeometry),
				pLayer
				);

			m_pRT->SetTransform(D2D1::Matrix3x2F::Rotation(g_rotation, D2D1::Point2F(m_pBitmap->GetSize().width/2, m_pBitmap->GetSize().height/2)));

			m_pRT->DrawBitmap(
				m_pBitmap,
				D2D1::RectF(0,0,
				m_pBitmap->GetSize().width,
				m_pBitmap->GetSize().height));

			m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());

			m_pRT->PopLayer();
		}

		SafeRelease(&pLayer);

		m_pRT->DrawText(
			L"����ͼ���ɰ����",
			wcslen(L"����ͼ���ɰ����"),
			m_pTextFormat,
			D2D1::RectF(300,250,500,300),
			m_pBrush);

        // ��������
        hr = m_pRT->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            hr = S_OK;
            DiscardDeviceResources();
        }
    }

    InvalidateRect(m_hwnd, NULL, FALSE);

    return hr;
}

void DemoApp::OnResize(UINT width, UINT height)
{
    if (m_pRT)
    {
        D2D1_SIZE_U size;
        size.width = width;
        size.height = height;

        // Note: This method can fail, but it's okay to ignore the
        // error here -- it will be repeated on the next call to
        // EndDraw.
        m_pRT->Resize(size);
    }
}

LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        DemoApp *pDemoApp = (DemoApp *)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            PtrToUlong(pDemoApp)
            );

        result = 1;
    }
    else
    {
        DemoApp *pDemoApp = reinterpret_cast<DemoApp *>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
                )));

        bool wasHandled = false;

        if (pDemoApp)
        {
            switch(message)
            {
            case WM_SIZE:
                {
                    UINT width = LOWORD(lParam);
                    UINT height = HIWORD(lParam);
                    pDemoApp->OnResize(width, height);
                }
                result = 0;
                wasHandled = true;
                break;

            case WM_PAINT:
            case WM_DISPLAYCHANGE:
                {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);

                    pDemoApp->OnRender();
                    EndPaint(hwnd, &ps);
                }
                result = 0;
                wasHandled = true;
                break;

            case WM_DESTROY:
                {
                    PostQuitMessage(0);
                }
                result = 1;
                wasHandled = true;
                break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}