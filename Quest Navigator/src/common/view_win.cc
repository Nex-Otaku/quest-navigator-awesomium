#include "view.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <vector>
#include "../quest-navigator/utils.h"
#include "../quest-navigator/configuration.h"

class ViewWin;

static bool g_is_initialized = false;
static std::vector<ViewWin*> g_active_views_;
const wchar_t szWindowClass[] = L"ViewWinClass";
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

using namespace Awesomium;
using namespace QuestNavigator;

class ViewWin : public View,
	public WebViewListener::View {
public:
	ViewWin(int width, int height) {
		PlatformInit();

		web_view_ = WebCore::instance()->CreateWebView(width,
			height,
			0,
			Awesomium::kWebViewType_Window);

		web_view_->set_view_listener(this);

		wstring wWindowTitle = widen(Configuration::getString(ecpWindowTitle));
		LPCWSTR wszTitle = wWindowTitle.c_str();

		// Вычисляем толщину рамок окна
		bool result = this->calcBorders();
		if (!result)
			exit(-1);

		// Create our WinAPI Window
		DWORD dwWindowStyle = getWindowStyle();
		HINSTANCE hInstance = GetModuleHandle(0);
		hwnd_ = CreateWindow(szWindowClass,
			wszTitle,
			dwWindowStyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			this->getFullWindowWidth(width),
			this->getFullWindowHeight(height), 
			NULL,
			NULL,
			hInstance,
			NULL);

		if (!hwnd_)
			exit(-1);

		g_active_views_.push_back(this);

		web_view_->set_parent_window(hwnd_);

		ShowWindow(hwnd_, SW_SHOWNORMAL);
		UpdateWindow(hwnd_);

		SetTimer (hwnd_, 0, 15, NULL );
	}

	virtual ~ViewWin() {
		for (std::vector<ViewWin*>::iterator i = g_active_views_.begin();
			i != g_active_views_.end(); i++) {
				if (*i == this) {
					g_active_views_.erase(i);
					break;
				}
		}

		web_view_->Destroy();
	}

	HWND hwnd() { return hwnd_; }

	static void PlatformInit() {
		if (g_is_initialized)
			return;

		WNDCLASSEX wc;

		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style         = 0;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = GetModuleHandle(0);
		wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = szWindowClass;
		wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

		if(!RegisterClassEx(&wc)) {
			exit(-1);
		}

		g_is_initialized = true;
	}

	static ViewWin* GetFromHandle(HWND handle) {
		for (std::vector<ViewWin*>::iterator i = g_active_views_.begin();
			i != g_active_views_.end(); i++) {
				if ((*i)->hwnd() == handle) {
					return *i;
				}
		}

		return NULL;
	}

	void toggleFullscreen()
	{
		toggleFullscreenByHwnd(hwnd_);
	}

	// Following methods are inherited from WebViewListener::View

	virtual void OnChangeTitle(Awesomium::WebView* caller,
		const Awesomium::WebString& title) {
			std::string title_utf8(ToString(title));
			std::wstring title_wide(title_utf8.begin(), title_utf8.end());

			SetWindowText(hwnd_, title_wide.c_str());
	}

	virtual void OnChangeAddressBar(Awesomium::WebView* caller,
		const Awesomium::WebURL& url) { }

	virtual void OnChangeTooltip(Awesomium::WebView* caller,
		const Awesomium::WebString& tooltip) { }

	virtual void OnChangeTargetURL(Awesomium::WebView* caller,
		const Awesomium::WebURL& url) { }

	virtual void OnChangeCursor(Awesomium::WebView* caller,
		Awesomium::Cursor cursor) { }

	virtual void OnChangeFocus(Awesomium::WebView* caller,
		Awesomium::FocusedElementType focused_type) { }

	virtual void OnShowCreatedWebView(Awesomium::WebView* caller,
		Awesomium::WebView* new_view,
		const Awesomium::WebURL& opener_url,
		const Awesomium::WebURL& target_url,
		const Awesomium::Rect& initial_pos,
		bool is_popup) { }

	virtual void OnAddConsoleMessage(Awesomium::WebView* caller,
		const Awesomium::WebString& message,
		int line_number,
		const Awesomium::WebString& source) { }

	int getFullWindowWidth(int viewWidth)
	{
		int borderWidth = Configuration::getBool(ecpGameResizeable) ? this->sizeableBorderWidth : this->fixedBorderWidth;
		return 2*borderWidth + viewWidth;
	};

	int getFullWindowHeight(int viewHeight)
	{
		int borderHeight = Configuration::getBool(ecpGameResizeable) ? this->sizeableBorderHeight : this->fixedBorderHeight;
		return 2*borderHeight + this->captionHeight + viewHeight;
	};

protected:
	// Вычисляем толщину рамки окна.
	// Требуется для корректного расчёта ширины и высоты окна.
	// Мы должны задать размер для всего окна,
	// зная требуемый размер клиентской части.
	bool calcBorders()
	{
		// Высота заголовка окна.
		int res = GetSystemMetrics(SM_CYCAPTION);
		bool bError = res == 0;
		this->captionHeight = res;

		// Высота горизонтальной границы для растягиваемого окна.
		res = GetSystemMetrics(SM_CXSIZEFRAME);
		bError = bError || res == 0;
		this->sizeableBorderHeight = res;

		// Ширина вертикальной границы для растягиваемого окна.
		res = GetSystemMetrics(SM_CYSIZEFRAME);
		bError = bError || res == 0;
		this->sizeableBorderWidth = res;

		// Высота горизонтальной границы для нерастягиваемого окна.
		res = GetSystemMetrics(SM_CXFIXEDFRAME);
		bError = bError || res == 0;
		this->fixedBorderHeight = res;

		// Ширина вертикальной границы для нерастягиваемого окна
		res = GetSystemMetrics(SM_CYFIXEDFRAME);
		bError = bError || res == 0;
		this->fixedBorderWidth = res;

		if (bError) {
			showError("Ошибка при определении параметров отрисовки");
			return false;
		}
		return true;
	};

protected:
	HWND hwnd_;
	int captionHeight;
	int sizeableBorderWidth;
	int sizeableBorderHeight;
	int fixedBorderWidth;
	int fixedBorderHeight;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	ViewWin* view = ViewWin::GetFromHandle(hWnd);

	switch (message) {
	case WM_COMMAND:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_TIMER:
		break;
	case WM_GETMINMAXINFO:
		{
			// Ограничиваем минимальный и максимальный размер окна
			MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
			POINT ptMin = pInfo->ptMinTrackSize;
			POINT ptMax = pInfo->ptMaxTrackSize;
			int minWidth = Configuration::getInt(ecpGameMinWidth);
			if (minWidth != 0)
				ptMin.x = view->getFullWindowWidth(minWidth);
			int minHeight = Configuration::getInt(ecpGameMinHeight);
			if (minHeight != 0)
				ptMin.y = view->getFullWindowHeight(minHeight);
			int maxWidth = Configuration::getInt(ecpGameMaxWidth);
			if (maxWidth != 0)
				ptMax.x = view->getFullWindowWidth(maxWidth);
			int maxHeight = Configuration::getInt(ecpGameMaxHeight);
			if (maxHeight != 0)
				ptMax.y = view->getFullWindowHeight(maxHeight);
			pInfo->ptMinTrackSize = ptMin;
			pInfo->ptMaxTrackSize = ptMax;
			return 0;
		}
		break;
	case WM_SIZE:
		view->web_view()->Resize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_QUIT:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

View* View::Create(int width, int height) {
	return new ViewWin(width, height);
}