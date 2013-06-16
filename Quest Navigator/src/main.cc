#include "common/application.h"
#include "common/view.h"
#include "common/method_dispatcher.h"
#include "deps/qsp/qsp.h"
#include "deps/qsp/bindings/default/qsp_default.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/DataPak.h>
#include <Awesomium/STLHelpers.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#include "quest-navigator/utils.h"

using namespace Awesomium;
using namespace QuestNavigator;

class QnApplicationListener : public Application::Listener {
	Application* app_;
	View* view_;
	DataSource* data_source_;
	MethodDispatcher method_dispatcher_;

	bool gameIsRunning;
	bool qspInited;

public:
	QnApplicationListener() 
		: app_(Application::Create()),
		view_(0),
		data_source_(0),

		gameIsRunning(false),
		qspInited(false)
	{
			app_->set_listener(this);
	}

	virtual ~QnApplicationListener() {
		if (view_)
			app_->DestroyView(view_);
		if (data_source_)
			delete data_source_;
		if (app_)
			delete app_;
	}

	void Run() {
		app_->Run();
	}

	// Inherited from Application::Listener
	virtual void OnLoaded() {
		view_ = View::Create(512, 512);

		BindMethods(view_->web_view());


		//QSPInit();
		//std::wstring message_str = QSPGetVersion();
		//   MessageBox(0, message_str.c_str(), message_str.c_str(), NULL);
		//QSPDeInit();

		// Подключаем пак с данными по умолчанию.
		// Существование пака не проверяется.
		// Если нам в параметрах указали путь к файлу - тогда просто не используем пак.
		data_source_ = new DataPakSource(ToWebString("assets.pak"));
		view_->web_view()->session()->AddDataSource(WSLit("webui"), data_source_);

		QuestNavigator::initOptions();

		std::string url = QuestNavigator::getContentUrl();
		view_->web_view()->LoadURL(WebURL(ToWebString(url)));
	}

	// Inherited from Application::Listener
	virtual void OnUpdate() {
	}

	// Inherited from Application::Listener
	virtual void OnShutdown() {
	}

	void BindMethods(WebView* web_view) {
		// Создаём глобальный JS-объект "QspLib"
		JSValue result = web_view->CreateGlobalJavascriptObject(WSLit("QspLibAwesomium"));
		if (result.IsObject()) {
			JSObject& app_object = result.ToObject();

			// Привязываем тестовые колбэки
			method_dispatcher_.Bind(app_object,
				WSLit("SayHello"),
				JSDelegate(this, &QnApplicationListener::OnSayHello));
			method_dispatcher_.Bind(app_object,
				WSLit("Exit"),
				JSDelegate(this, &QnApplicationListener::OnExit));
			method_dispatcher_.BindWithRetval(app_object,
				WSLit("GetSecretMessage"),
				JSDelegateWithRetval(this, &QnApplicationListener::OnGetSecretMessage));

			// Привязываем настоящие колбэки
			method_dispatcher_.Bind(app_object,
				WSLit("restartGame"),
				JSDelegate(this, &QnApplicationListener::restartGame));

			// Привязываем колбэк для обработки вызовов alert
			method_dispatcher_.Bind(app_object,
				WSLit("alert"),
				JSDelegate(this, &QnApplicationListener::alert));
		}

		// Привязываем обработчик колбэков к WebView
		web_view->set_js_method_handler(&method_dispatcher_);
	}

	// Bound to App.SayHello() in JavaScript
	void OnSayHello(WebView* caller,
		const JSArray& args) {
			app_->ShowMessage("Hello!");
	}

	// Bound to App.Exit() in JavaScript
	void OnExit(WebView* caller,
		const JSArray& args) {
			app_->Quit();
	}

	// Bound to App.GetSecretMessage() in JavaScript
	JSValue OnGetSecretMessage(WebView* caller,
		const JSArray& args) {
			return JSValue(WSLit(
				"<img src='asset://webui/key.png'/> You have unlocked the secret message!"));
	}

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                   Основная логика
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void initLib()
	{
		/*
		gameIsRunning = false;
		qspInited = false;
		
		curGameFile = "www/standalone_content/game.qsp";
		curSaveDir = mainActivity.getFilesDir().getPath().concat(File.separator);

        //Создаем список для всплывающего меню
        menuList = new Vector<ContainerMenuItem>();

        //Создаем объект для таймера
        timerHandler = new Handler(Looper.getMainLooper());

        //Запускаем поток библиотеки
        StartLibThread();
		*/
	}

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                 Колбэки библиотеки интерпретатора
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                        Вызовы JS-функций
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void jsCallApi(string name, JSValue arg)
	{
		JSValue window = view_->web_view()->ExecuteJavascriptWithResult(
			WSLit("window"), WSLit(""));
		if (window.IsObject()) {
			JSArray args;
			args.Push(arg);
			window.ToObject().Invoke(ToWebString(name), args);
		}
	}
	void qspSetGroupedContent(JSObject content)
	{
		jsCallApi("qspSetGroupedContent", content);
	}
	void qspShowSaveSlotsDialog(JSObject content)
	{
		jsCallApi("qspShowSaveSlotsDialog", content);
	}
	void qspMsg(WebString text)
	{
		jsCallApi("qspMsg", text);
	}
	void qspError(WebString error)
	{
		jsCallApi("qspError", error);
	}
	void qspMenu(JSObject menu)
	{
		jsCallApi("qspMenu", menu);
	}
	void qspInput(WebString text)
	{
		jsCallApi("qspInput", text);
	}
	void qspView(WebString path)
	{
		jsCallApi("qspView", path);
	}





	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                   Обработчики вызовов из JS
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void restartGame(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("restarted!");
	}

	void executeAction(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("action pressed!");
	}

	void selectObject(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("object selected!");
	}

	void loadGame(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("game load dialog need to be opened!");
	}

	void saveGame(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("game save dialog need to be opened!");
	}

	void saveSlotSelected(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("slot selected!");
	}

	void msgResult(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("msg dialog closed!");
	}

	void errorResult(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("error dialog closed!");
	}

	void userMenuResult(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("user menu dialog closed!");
	}

	void inputResult(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("input dialog closed!!");
	}

	void setMute(WebView* caller, const JSArray& args)
	{
		app_->ShowMessage("mute called!");
	}

	// ********************************************************************
	// Вспомогательные обработчики для отладки
	// ********************************************************************
	void alert(WebView* caller, const JSArray& args)
	{
		string msg = "";
		if (args.size() > 0) {
			msg = ToString(args[0].ToString());
		}
		showMessage(msg, "");
	}

};

#ifdef _WIN32
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t*, 
					  int nCmdShow) {
#else
int main() {
#endif

	QnApplicationListener listener;
	listener.Run();

	return 0;
}
