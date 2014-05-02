#ifndef COMMON_APPLICATION_H_
#define COMMON_APPLICATION_H_

#include <string>
#include <Windows.h>

using namespace std;

class View;

namespace Awesomium {
	class WebCore;
}

// Common class that sets up an application, creates the WebCore, handles
// the Run loop, and abstracts platform-specific details.
class Application {
public:
	// Listener interface to be used to handle various application events.
	class Listener {
	public:
		virtual ~Listener() {}

		// Event is fired when app (and WebCore) have been loaded.
		virtual void OnLoaded() = 0;

		// Event is fired for each iteration of the Run loop.
		virtual void OnUpdate() = 0;

		// Event is fired when the app is shutting down.
		virtual void OnShutdown() = 0;

		// Выполнение строки кода
		virtual void executeCode(string qspCode) = 0;

		// Курсор находится в текстовом поле
		virtual bool textInputIsFocused() = 0;

		// Переключение полноэкранного режима
		virtual void toggleFullscreen() = 0;

		// Обработка команды из другого экземпляра плеера.
		virtual bool processIpcData(COPYDATASTRUCT* pCds) = 0;
	};

	virtual ~Application() {}

	// Platform-specific factory constructor
	static Application* Create();

	// Begin the Run loop.
	virtual void Run() = 0;

	// Ends the Run loop.
	virtual void Quit() = 0;

	// Create a platform-specific, windowed View
	virtual View* CreateView(int width, int height) = 0;

	// Destroy a View
	virtual void DestroyView(View* view) = 0;

	// Show a modal message box
	virtual void ShowMessage(const char* message) = 0;

	// Get the WebCore
	virtual Awesomium::WebCore* web_core() { return web_core_; }

	// Get the Listener.
	Listener* listener() { return listener_; }

	// Set the Listener for various app events.
	void set_listener(Listener* listener) { listener_ = listener; }

	// Открытие ссылки в новом окне, используя системный браузер
	virtual void openUrlInSystemBrowser(string url) = 0;

protected:
	Application() { }

	virtual void Load() = 0;

	Listener* listener_;
	Awesomium::WebCore* web_core_;
};

#endif  // COMMON_APPLICATION_H_
