#ifndef LISTENER_H_
#define LISTENER_H_

#include "../common/application.h"
#include "../common/view.h"
#include "../common/method_dispatcher.h"
#include "../deps/qsp/qsp.h"
#include "../deps/qsp/bindings/default/qsp_default.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/DataPak.h>
#include <Awesomium/STLHelpers.h>
#include "utils.h"

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
	QnApplicationListener();
	virtual ~QnApplicationListener();
	
	void Run();

	// Inherited from Application::Listener
	virtual void OnLoaded();
	virtual void OnUpdate();
	virtual void OnShutdown();

	void BindMethods(WebView* web_view);

	// Bound to App.SayHello() in JavaScript
	void OnSayHello(WebView* caller, const JSArray& args);

	// Bound to App.Exit() in JavaScript
	void OnExit(WebView* caller, const JSArray& args);

	// Bound to App.GetSecretMessage() in JavaScript
	JSValue OnGetSecretMessage(WebView* caller, const JSArray& args);

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                   Основная логика
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void initLib();

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                       Колбэки интерпретатора
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

    static void RefreshInt(int isRedraw);
    static void SetTimer(int msecs);
	static void ShowMessage(QSP_CHAR* message);
    static void PlayFile(QSP_CHAR* file, int volume);
	static QSP_BOOL IsPlayingFile(QSP_CHAR* file);
    static void CloseFile(QSP_CHAR* file);
    static void ShowPicture(QSP_CHAR* file);
    static void InputBox(const QSP_CHAR* prompt, QSP_CHAR* buffer, int maxLen);
    static void PlayerInfo(QSP_CHAR* resource, QSP_CHAR* buffer, int maxLen);
    static int GetMSCount();
    static void AddMenuItem(QSP_CHAR* name, QSP_CHAR* imgPath);
    static int ShowMenu();
    static void DeleteMenu();
    static void Wait(int msecs);
	static void ShowWindow(int type, QSP_BOOL isShow);
    static void System(QSP_CHAR* cmd);

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                        Вызовы JS-функций
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void jsCallApi(string name, JSValue arg);
	void qspSetGroupedContent(JSObject content);
	void qspShowSaveSlotsDialog(JSObject content);
	void qspMsg(WebString text);
	void qspError(WebString error);
	void qspMenu(JSObject menu);
	void qspInput(WebString text);
	void qspView(WebString path);

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                   Обработчики вызовов из JS
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void restartGame(WebView* caller, const JSArray& args);
	void executeAction(WebView* caller, const JSArray& args);
	void selectObject(WebView* caller, const JSArray& args);
	void loadGame(WebView* caller, const JSArray& args);
	void saveGame(WebView* caller, const JSArray& args);
	void saveSlotSelected(WebView* caller, const JSArray& args);
	void msgResult(WebView* caller, const JSArray& args);
	void errorResult(WebView* caller, const JSArray& args);
	void userMenuResult(WebView* caller, const JSArray& args);
	void inputResult(WebView* caller, const JSArray& args);
	void setMute(WebView* caller, const JSArray& args);

	// ********************************************************************
	// Вспомогательные обработчики для отладки
	// ********************************************************************
	void alert(WebView* caller, const JSArray& args);

};





#endif
