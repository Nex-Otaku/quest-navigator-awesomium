#include "application.h"
#include "view.h"
#include <Awesomium/WebCore.h>
#include <string>
#include "../quest-navigator/utils.h"
#include "../quest-navigator/configuration.h"
#include "../quest-navigator/listener.h"

using namespace Awesomium;
using namespace std;
using namespace QuestNavigator;

#define KEYSTATE_PRESSED 0x8000

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
		while (is_running_ && GetMessage(&msg, NULL, 0, 0)) {
			web_core_->Update();

			// Игнорируем нажатия Backspace вне текстовых полей.
			bool bBackSpace = ((msg.message == WM_KEYDOWN) || (msg.message == WM_KEYUP)) &&
				((msg.wParam == VK_BACK) || (msg.wParam == VK_BROWSER_BACK)) &&
				!listener()->textInputIsFocused();

			// Переключение в полноэкранный режим по Alt + Enter.
			bool bFullscreen = (msg.message == WM_SYSKEYDOWN) && (msg.wParam == VK_RETURN);
			if (bFullscreen) {
				listener()->toggleFullscreen();
			}

			// Закрываем окно по Alt + F4.
			if ((msg.message == WM_SYSKEYDOWN) && (msg.wParam == VK_F4)) {
				this->Quit();
			}

			// Если нажата комбинация Ctrl+F5, 
			// выполняем полную перезагрузку игры.
			if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_F5)) {
				SHORT keyState = GetKeyState(VK_CONTROL);
				if (keyState & KEYSTATE_PRESSED) {
					QnApplicationListener* pListener = QnApplicationListener::listener;
					// Перезапускаем, если игра запущена.
					if (pListener->isGameRunning()) {
						pListener->runNewGame(Configuration::getString(ecpGameFilePath));
					}
				}
			}

			if (!bBackSpace && !bFullscreen) {
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