#include "interceptor.h"
#include <string>
#include <Awesomium/STLHelpers.h>
#include "utils.h"
#include "listener.h"

using namespace std;
using namespace QuestNavigator;

///
/// Override this method to intercept requests for resources. You can use
/// this to modify requests before they are sent, respond to requests using
/// your own custom resource-loading back-end, or to monitor requests for
/// tracking purposes.
///
/// @param  request  The resource request.
///
/// @return  Return a new ResourceResponse (see ResourceResponse::Create)
///          to override the response, otherwise, return NULL to allow
///          normal behavior.
///
/// @note WARNING: This method is called on the IO Thread, you should not
///       make any calls to WebView or WebCore (they are not threadsafe).
///
ResourceResponse* QnInterceptor::OnRequest(ResourceRequest* request) 
{
	WebURL url = request->url();
	string sUrl = ToString(url.spec());
	string scheme = ToString(url.scheme());
	// Ссылки на сайты должны открываться
	// в браузере по умолчанию.
	if ((scheme == "http") || (scheme == "https")) {
		// Блокируем запросы, оканчивающиеся на "wpad.dat":
		// это системные запросы для автоопределения прокси.
		if (!endsWith(sUrl, "wpad.dat")) {
			app_->openUrlInSystemBrowser(sUrl);
		}
		request->Cancel();
	}
	return 0;
}

///
/// Override this method to intercept frame navigations. You can use this to
/// block or log navigations for each frame of a WebView.
///
/// @param  origin_process_id   The unique process id of the origin WebView.
///                             See WebView::process_id()
///
/// @param  origin_routing_id   The unique routing id of the origin WebView.
///                             See WebView::routing_id()
///
/// @param  method   The HTTP method (usually GET or POST) of the request.
///
/// @param  url    The URL that the frame wants to navigate to.
///
/// @param  is_main_frame   Whether or not this navigation involves the main
///                         frame (eg, the whole page is navigating away).
///
/// @return  Return True to block a navigation. Return False to let it continue.
///
/// @note WARNING: This method is called on the IO Thread, you should not
///       make any calls to WebView or WebCore (they are not threadsafe).
///
bool QnInterceptor::OnFilterNavigation(int origin_process_id,
									   int origin_routing_id,
									   const WebString& method,
									   const WebURL& url,
									   bool is_main_frame)
{
	string sUrl = ToString(url.spec());
	string sUrlUpper = toUpper(sUrl);
	if (startsWith(sUrlUpper, "EXEC:")) {
		// К сожалению, Awesomium вырезает символы перевода строки из запроса.
		// Без костылей с этим ничего сделать нельзя.
		// Поэтому многострочный код "EXEC:" нельзя использовать в Навигаторе.

		// Выполняем код EXEC
		string code = unescapeHtml(decodeUrl(sUrl.substr(5)));
		app_->listener()->executeCode(code);
		return true;
	}
	return false;
}

///
/// Override this method to intercept download events (usually triggered
/// when a server response indicates the content should be downloaded, eg:
/// Content-Disposition: attachment). You can use this to log these
/// type of responses.
///
/// @param  origin_process_id   The unique process id of the origin WebView.
///                             See WebView::process_id()
///
/// @param  origin_routing_id   The unique routing id of the origin WebView.
///                             See WebView::routing_id()
///
/// @param  url    The URL of the request that initiated the download.
///
/// @note WARNING: This method is called on the IO Thread, you should not
///       make any calls to WebView or WebCore (they are not threadsafe).
///
void QnInterceptor::OnWillDownload(int origin_process_id,
								   int origin_routing_id,
								   const WebURL& url)
{
}

QnInterceptor::QnInterceptor() : app_(NULL) {}

void QnInterceptor::setApp(Application* app)
{
	app_ = app;
}