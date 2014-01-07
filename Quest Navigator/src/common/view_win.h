#ifndef VIEW_WIN_H_
#define VIEW_WIN_H_

#include "view.h"
#include <Awesomium/WebCore.h>

using namespace Awesomium;

class ViewWin : public View,
	public WebViewListener::View {
public:
	ViewWin(int width, int height);
	virtual ~ViewWin();

	HWND hwnd();

	static void PlatformInit();

	static ViewWin* GetFromHandle(HWND handle);

	// Переключение в полноэкранный режим и обратно.
	void toggleFullscreen();

	// Обновляем свойства окна согласно настройкам шаблона.
	void applySkinToWindow();

	// Following methods are inherited from WebViewListener::View

	virtual void OnChangeTitle(Awesomium::WebView* caller,
		const Awesomium::WebString& title);

	virtual void OnChangeAddressBar(Awesomium::WebView* caller,
		const Awesomium::WebURL& url);

	virtual void OnChangeTooltip(Awesomium::WebView* caller,
		const Awesomium::WebString& tooltip);

	virtual void OnChangeTargetURL(Awesomium::WebView* caller,
		const Awesomium::WebURL& url);

	virtual void OnChangeCursor(Awesomium::WebView* caller,
		Awesomium::Cursor cursor);

	virtual void OnChangeFocus(Awesomium::WebView* caller,
		Awesomium::FocusedElementType focused_type);

	virtual void OnShowCreatedWebView(Awesomium::WebView* caller,
		Awesomium::WebView* new_view,
		const Awesomium::WebURL& opener_url,
		const Awesomium::WebURL& target_url,
		const Awesomium::Rect& initial_pos,
		bool is_popup);

	virtual void OnAddConsoleMessage(Awesomium::WebView* caller,
		const Awesomium::WebString& message,
		int line_number,
		const Awesomium::WebString& source);

	int getFullWindowWidth(int viewWidth);

	int getFullWindowHeight(int viewHeight);

protected:
	// Вычисляем толщину рамки окна.
	// Требуется для корректного расчёта ширины и высоты окна.
	// Мы должны задать размер для всего окна,
	// зная требуемый размер клиентской части.
	bool calcBorders();

protected:
	HWND hwnd_;
	int captionHeight;
	int sizeableBorderWidth;
	int sizeableBorderHeight;
	int fixedBorderWidth;
	int fixedBorderHeight;
};



#endif
