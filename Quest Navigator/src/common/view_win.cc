#include "view_win.h"
#include <Awesomium/STLHelpers.h>
#include <vector>
#include <Windows.h>
#include "../quest-navigator/utils.h"
#include "../quest-navigator/configuration.h"
#include "../quest-navigator/resource.h"
#include "../quest-navigator/listener.h"

class ViewWin;

static bool g_is_initialized = false;
static std::vector<ViewWin*> g_active_views_;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

using namespace Awesomium;
using namespace QuestNavigator;

ViewWin::ViewWin(int width, int height) {
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
		exit(eecFailToCalcBorders);

	// Create our WinAPI Window
	DWORD dwWindowStyle = getWindowStyle();
	HINSTANCE hInstance = GetModuleHandle(0);
	hwnd_ = CreateWindowEx(
		WS_EX_ACCEPTFILES,
		szWindowClass,
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
		exit(eecFailToCreateMainWindow);

	g_active_views_.push_back(this);

	web_view_->set_parent_window(hwnd_);

	ShowWindow(hwnd_, SW_SHOWNORMAL);
	if (Configuration::getBool(ecpIsFullscreen)) {
		setFullscreenByHwnd(hwnd_, Configuration::getBool(ecpIsFullscreen));
	}
	UpdateWindow(hwnd_);

	SetTimer (hwnd_, 0, 15, NULL );
}

ViewWin::~ViewWin() {
	for (std::vector<ViewWin*>::iterator i = g_active_views_.begin();
		i != g_active_views_.end(); i++) {
			if (*i == this) {
				g_active_views_.erase(i);
				break;
			}
	}

	web_view_->Destroy();
}

HWND ViewWin::hwnd() { return hwnd_; }

void ViewWin::PlatformInit() {
	if (g_is_initialized)
		return;

	HICON hMyIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_LOGO));

	WNDCLASSEX wc;

	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(0);
	wc.hIcon         = hMyIcon;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szWindowClass;
	wc.hIconSm       = hMyIcon;

	if(!RegisterClassEx(&wc)) {
		exit(ecpConfigFilePath);
	}

	g_is_initialized = true;
}

ViewWin* ViewWin::GetFromHandle(HWND handle) {
	for (std::vector<ViewWin*>::iterator i = g_active_views_.begin();
		i != g_active_views_.end(); i++) {
			if ((*i)->hwnd() == handle) {
				return *i;
			}
	}

	return NULL;
}

// Переключение в полноэкранный режим и обратно.
void ViewWin::toggleFullscreen()
{
	toggleFullscreenByHwnd(hwnd_);
}

// Обновляем свойства окна согласно настройкам шаблона.
void ViewWin::applySkinToWindow()
{
	// Обновляем стиль окна.
	LONG_PTR lpResult = SetWindowLongPtr(hwnd_, GWL_STYLE, getWindowStyle());
	if (lpResult == 0) {
		showError("Не удалось обновить стиль окна");
		return;
	}

	// Обновляем заголовок окна.
	wstring wTitle = widen(Configuration::getString(ecpWindowTitle));
	BOOL res = SetWindowText(hwnd_, wTitle.c_str());
	if (res == 0) {
		showError("Не удалось обновить заголовок окна");
		return;
	}

	// Вычисляем размер клиентской области окна.
	RECT r;
	res = GetClientRect(hwnd_, &r);
	if (res == 0) {
		showError("Не удалось запросить размер окна");
		return;
	}
	int currentWidth = r.right - r.left;
	int currentHeight = r.bottom - r.top;

	// Рассчитываем требуемые размеры.
	int gameWidth = Configuration::getInt(ecpGameWidth);
	int gameHeight = Configuration::getInt(ecpGameHeight);
	int minWidth = Configuration::getInt(ecpGameMinWidth);
	int minHeight = Configuration::getInt(ecpGameMinHeight);
	int maxWidth = Configuration::getInt(ecpGameMaxWidth);
	int maxHeight = Configuration::getInt(ecpGameMaxHeight);
	bool resizeable = Configuration::getBool(ecpGameResizeable);
	int width = 0;
	int height = 0;
	if (resizeable) {
		// Для растягиваемого шаблона, 
		// стараемся сохранить текущие размеры окна 
		// в рамках заданных шаблоном ограничений.
		width = currentWidth;
		height = currentHeight;
		if ((minWidth != 0) && (width < minWidth))
			width = minWidth;
		if ((minHeight != 0) && (height < minHeight))
			height = minHeight;
		if ((maxWidth != 0) && (width > maxWidth))
			width = maxWidth;
		if ((maxHeight != 0) && (height > maxHeight))
			height = maxHeight;
	} else {
		// Для нерастягиваемого шаблона,
		// размеры должны быть всегда равны заданным.
		width = gameWidth;
		height = gameHeight;
	}
	// Учитываем толщину рамок по краям окна.
	width = getFullWindowWidth(width);
	height = getFullWindowHeight(height);
	// Устанавливаем новый размер окна.
	// Делаем это, даже если размер не изменился, 
	// так как мог поменяться стиль окна,
	// а он не обновляется без вызова SetWindowPos().
	res = SetWindowPos(hwnd_, NULL, 0, 0, width, height, SWP_NOMOVE 
		| SWP_NOZORDER 
		| SWP_SHOWWINDOW
		| SWP_FRAMECHANGED
		| SWP_NOCOPYBITS);
	if (res == 0) {
		showError("Не удалось обновить размер окна");
	}
}

// Following methods are inherited from WebViewListener::View

void ViewWin::OnChangeTitle(Awesomium::WebView* caller,
	const Awesomium::WebString& title) {
		std::string title_utf8(ToString(title));
		std::wstring title_wide(title_utf8.begin(), title_utf8.end());

		SetWindowText(hwnd_, title_wide.c_str());
}

void ViewWin::OnChangeAddressBar(Awesomium::WebView* caller,
	const Awesomium::WebURL& url) { }

void ViewWin::OnChangeTooltip(Awesomium::WebView* caller,
	const Awesomium::WebString& tooltip) { }

void ViewWin::OnChangeTargetURL(Awesomium::WebView* caller,
	const Awesomium::WebURL& url) { }

void ViewWin::OnChangeCursor(Awesomium::WebView* caller,
	Awesomium::Cursor cursor) { }

void ViewWin::OnChangeFocus(Awesomium::WebView* caller,
	Awesomium::FocusedElementType focused_type) { }

void ViewWin::OnShowCreatedWebView(Awesomium::WebView* caller,
	Awesomium::WebView* new_view,
	const Awesomium::WebURL& opener_url,
	const Awesomium::WebURL& target_url,
	const Awesomium::Rect& initial_pos,
	bool is_popup) { }

void ViewWin::OnAddConsoleMessage(Awesomium::WebView* caller,
	const Awesomium::WebString& message,
	int line_number,
	const Awesomium::WebString& source) 
{
	// Возникла ошибка при выполнении яваскрипта,
	// либо просто вывод в консоль.
	string msg = ToString(message);
	string src = decodeUrl(ToString(source));
	writeConsole(hwnd(), "Файл: " + src + "\r\n"
						+ "Строка: " + intToString(line_number) + "\r\n"
						+ "Сообщение: " + msg + "\r\n\r\n");
}

int ViewWin::getFullWindowWidth(int viewWidth)
{
	int borderWidth = Configuration::getBool(ecpGameResizeable) ? this->sizeableBorderWidth : this->fixedBorderWidth;
	return 2*borderWidth + viewWidth;
};

int ViewWin::getFullWindowHeight(int viewHeight)
{
	int borderHeight = Configuration::getBool(ecpGameResizeable) ? this->sizeableBorderHeight : this->fixedBorderHeight;
	return 2*borderHeight + this->captionHeight + viewHeight;
};

// Вычисляем толщину рамки окна.
// Требуется для корректного расчёта ширины и высоты окна.
// Мы должны задать размер для всего окна,
// зная требуемый размер клиентской части.
bool ViewWin::calcBorders()
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
	case WM_COPYDATA:
		{
			// Обработка команды из другого экземпляра плеера.
			// Обрабатываем в цикле окна, чтобы быть уверенными, 
			// что сообщение не прерывает обработку модальных диалогов и пр.
			bool success = QnApplicationListener::listener->processIpcData((COPYDATASTRUCT*)lParam);
			return success ? TRUE : FALSE;
		}
		break;
	case WM_DROPFILES:
		{
			// Перетащили файл на окно плеера.
			HDROP hDropInfo = (HDROP)wParam;
			// Считаем, сколько всего файлов.
			UINT filesCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
			if (filesCount < 1) {
				showError("Не обнаружены перетаскиваемые файлы");
				DragFinish(hDropInfo);
				return 0;
			}
			if (filesCount > 1) {
				showError("Запуск более чем одного файла невозможен");
				DragFinish(hDropInfo);
				return 0;
			}
			// Сохраняем имя файла.
			wchar_t dropMsg[MAX_PATH];
			UINT res = DragQueryFile(hDropInfo, 0, (LPWSTR)dropMsg, sizeof(dropMsg));
			if (res == 0) {
				showError("Не удалось прочесть имя перетаскиваемого файла");
				DragFinish(hDropInfo);
				return 0;
			}
			// Завершаем перетаскивание.
			DragFinish(hDropInfo);
			// Преобразовываем имя файла в нормальный формат.
			wstring wFile = dropMsg;
			string file = narrow(wFile);
			// Запускаем файл.
			QnApplicationListener::listener->runNewGame(file);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_QUIT:
		break;
	case WM_ACTIVATE:
		{
			// Устанавливаем фокус ввода на окно браузера.
			// Это нужно, чтобы работал скролл мышкой.
			// К сожалению, при запуске плеера не срабатывает,
			// работает только при переключении в окно.
			view->web_view()->Focus();
			return 0;
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Определяем, что при создании экземпляра View всегда используется ViewWin.
View* View::Create(int width, int height) {
	return new ViewWin(width, height);
}