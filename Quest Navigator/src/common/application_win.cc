#include "application.h"
#include "view.h"
#include <Awesomium/WebCore.h>
#include <string>
#include "../quest-navigator/utils.h"

using namespace Awesomium;
using namespace std;
using namespace QuestNavigator;

class ApplicationWin : public Application {
	bool is_running_;
public:
	ApplicationWin() {
		is_running_ = true;
		listener_ = NULL;
		web_core_ = NULL;
	}

	virtual ~ApplicationWin() {
		if (listener())
			listener()->OnShutdown();

		if (web_core_)
			web_core_->Shutdown();
	}

	virtual void Run() {
		Load();

		// Main message loop:
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) && is_running_) {
			web_core_->Update();
			bool bBackSpace = ((msg.message == WM_KEYDOWN) || (msg.message == WM_KEYUP)) &&
				((msg.wParam == VK_BACK) || (msg.wParam == VK_BROWSER_BACK)) &&
				!listener()->textInputIsFocused();
			if (!bBackSpace) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (listener())
				listener()->OnUpdate();
		}
	}

	virtual void Quit() {
		is_running_ = false;
	}

	virtual void Load() {
		web_core_ = WebCore::Initialize(WebConfig());

		if (listener())
			listener()->OnLoaded();
	}

	virtual View* CreateView(int width, int height) {
		return View::Create(width, height);
	}

	virtual void DestroyView(View* view) {
		delete view;
	}

	virtual void ShowMessage(const char* message) {
		wstring message_str(message, message + strlen(message));
		MessageBox(0, message_str.c_str(), message_str.c_str(), NULL);
	}

	// Открытие ссылки в новом окне, используя системный браузер
	virtual void openUrlInSystemBrowser(string url)
	{
		HINSTANCE r = ShellExecute(NULL, 
			widen("open").c_str(), 
			widen(url).c_str(), 
			NULL, 
			NULL, 
			SW_SHOWNORMAL);
		if ((int)r <= 32) {
			showError("Не удалось открыть URL: " + url);
		}
	}

};

Application* Application::Create() {
	return new ApplicationWin();
}