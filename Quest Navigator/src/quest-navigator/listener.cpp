#include "listener.h"

#include "../common/application.h"
#include "../common/view.h"
#include "../common/view_win.h"
#include "../common/method_dispatcher.h"
#include "../deps/qsp/qsp.h"
#include "../deps/qsp/bindings/default/qsp_default.h"
#include "../deps/md5/md5.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/DataPak.h>
#include <Awesomium/STLHelpers.h>
#include "utils.h"
#include "configuration.h"
#include "skin.h"
#include "sound.h"
#include "gamestock.h"
#include <process.h>
#include <algorithm>

using namespace Awesomium;

namespace QuestNavigator {

	// Глобальные переменные для работы с потоками

	// Структура для критических секций
	CRITICAL_SECTION g_csSharedData;

	// Разделяемые данные
	struct SharedData
	{
		string str;
		int num;
		JSValue jsValue;
		bool flag;
	};
	SharedData g_sharedData[evLast];

	// События для синхронизации потоков
	HANDLE g_eventList[evLast];

	QnApplicationListener::QnApplicationListener() 
		: app_(Application::Create()),
		view_(0),
		data_source_(0),
		resource_interceptor_(),

		gameIsRunning(false),
		programLoaded(false),
		jsLibObjectCreated(false),

		libThread(NULL),

		hInstanceMutex(NULL)
	{
		app_->set_listener(this);
	}

	QnApplicationListener::~QnApplicationListener() {
		if (view_)
			app_->DestroyView(view_);
		if (data_source_)
			delete data_source_;
		if (app_)
			delete app_;
	}

	void QnApplicationListener::Run() {
		app_->Run();
	}

	// Inherited from Application::Listener
	void QnApplicationListener::OnLoaded() {
		// Контекст UI

		if (!Configuration::init() || !initOptions("") || !prepareGameFiles() || !registerInstance()) {
			app_->Quit();
			return;
		}

		// Проверяем обновления
		#ifdef _WIN32
		if (!Configuration::getBool(ecpGameIsStandalone)) {
			checkUpdate();
		}
		#endif

		view_ = View::Create(Configuration::getInt(QuestNavigator::ecpGameWidth), Configuration::getInt(ecpGameHeight));

		// Перехватчик запросов, выполняющихся при нажатии на ссылку.
		resource_interceptor_.setApp(app_);
		app_->web_core()->set_resource_interceptor(&resource_interceptor_);

		initLib();

		// Привязываем обработку событий загрузки HTML-фреймов.
		// Интерфейс Awesomium::WebViewListener::Load.
		view_->web_view()->set_load_listener(this);

		// Загружаем страницу с HTML, CSS и JS. 
		// В обработчике onDocumentReady привяжем глобальный JS-объект для библиотеки.
		// По завершению загрузки будет вызван обработчик onFinishLoadingFrame.
		std::string url = QuestNavigator::getContentUrl();
		view_->web_view()->LoadURL(WebURL(ToWebString(url)));

		programLoaded = true;
	}

	// Inherited from Application::Listener
	void QnApplicationListener::OnUpdate() {
		if (gameIsRunning && checkForSingle(evJsCommitted)) {
			// Библиотека сообщила потоку UI,
			// что требуется выполнить JS-запрос.
			processLibJsCall();
		};
	}

	// Inherited from Application::Listener
	void QnApplicationListener::OnShutdown() {
		if (programLoaded) {
			FreeResources();

			// Завершаем работу апдейтера.
			#ifdef _WIN32
			if (!Configuration::getBool(ecpGameIsStandalone)) {
				finishUpdate();
			}
			#endif
		}

		// Завершаем работу класса конфигурации.
		Configuration::deinit();

		// Сбрасываем мьютекс.
		unregisterInstance();
	}

	/// This event occurs when the page begins loading a frame.
	void QnApplicationListener::OnBeginLoadingFrame(Awesomium::WebView* caller,
		int64 frame_id,
		bool is_main_frame,
		const Awesomium::WebURL& url,
		bool is_error_page) {
			// Ничего не делаем.
	};
	/// This event occurs when a frame fails to load. See error_desc
	/// for additional information.
	void QnApplicationListener::OnFailLoadingFrame(Awesomium::WebView* caller,
		int64 frame_id,
		bool is_main_frame,
		const Awesomium::WebURL& url,
		int error_code,
		const Awesomium::WebString& error_desc) {
			// Ничего не делаем.
			// Хотелось бы здесь выводить сообщение об ошибке и закрывать плеер,
			// но Awesomium выдаёт здесь ошибку при выполнении ссылок "EXEC:",
			// несмотря на то, 
			// что навигация отменяется в перехватчике запросов QnInterceptor::OnFilterNavigation.
	};
	/// This event occurs when the page finishes loading a frame.
	/// The main frame always finishes loading last for a given page load.
	void QnApplicationListener::OnFinishLoadingFrame(Awesomium::WebView* caller,
		int64 frame_id,
		bool is_main_frame,
		const Awesomium::WebURL& url) {
		// Инициализация Awesomium закончена.
		// Ещё раньше выполнены все обработчики "$(document).ready".
		// Страница полностью загружена, объект "QspLibAwesomium" загружен и к нему привязаны колбэки.
		// Сообщаем JS, что всё готово к запуску API.
		if (!jsLibObjectCreated) {
			showError("JS-объект \"QspLibAwesomium\" не был инициализирован к моменту завершения загрузки фрейма");
			app_->Quit();
			return;
		}
		if (!onWebDeviceReady()) {
			app_->Quit();
			return;
		}
	};
	/// This event occurs when the DOM has finished parsing and the
	/// window object is available for JavaScript execution.
	void QnApplicationListener::OnDocumentReady(Awesomium::WebView* caller,
		const Awesomium::WebURL& url) {
		// Привязываем глобальный JS-объект для вызова методов Навигатора из JS.
		// Делаем это только при первом вызове обработчика, 
		// т.к. после создания объект будет присутствовать на всех страницах.
		if (jsLibObjectCreated)
			return;
		if (!BindMethods(view_->web_view())) {
			app_->Quit();
			return;
		}
		jsLibObjectCreated = true;
	};

	bool QnApplicationListener::BindMethods(WebView* web_view) {
		// Создаём глобальный JS-объект "QspLib"
		JSValue result = web_view->CreateGlobalJavascriptObject(WSLit("QspLibAwesomium"));
		if (result.IsObject()) {
			JSObject& app_object = result.ToObject();

			// Привязываем колбэки
			method_dispatcher_.Bind(app_object,
				WSLit("restartGame"),
				JSDelegate(this, &QnApplicationListener::restartGame));

			method_dispatcher_.Bind(app_object,
				WSLit("executeAction"),
				JSDelegate(this, &QnApplicationListener::executeAction));

			method_dispatcher_.Bind(app_object,
				WSLit("selectObject"),
				JSDelegate(this, &QnApplicationListener::selectObject));

			method_dispatcher_.Bind(app_object,
				WSLit("loadGame"),
				JSDelegate(this, &QnApplicationListener::loadGame));

			method_dispatcher_.Bind(app_object,
				WSLit("saveGame"),
				JSDelegate(this, &QnApplicationListener::saveGame));

			method_dispatcher_.Bind(app_object,
				WSLit("saveSlotSelected"),
				JSDelegate(this, &QnApplicationListener::saveSlotSelected));

			method_dispatcher_.Bind(app_object,
				WSLit("msgResult"),
				JSDelegate(this, &QnApplicationListener::msgResult));

			method_dispatcher_.Bind(app_object,
				WSLit("errorResult"),
				JSDelegate(this, &QnApplicationListener::errorResult));

			method_dispatcher_.Bind(app_object,
				WSLit("userMenuResult"),
				JSDelegate(this, &QnApplicationListener::userMenuResult));

			method_dispatcher_.Bind(app_object,
				WSLit("inputResult"),
				JSDelegate(this, &QnApplicationListener::inputResult));

			method_dispatcher_.Bind(app_object,
				WSLit("setMute"),
				JSDelegate(this, &QnApplicationListener::setMute));

			method_dispatcher_.Bind(app_object,
				WSLit("setInputString"),
				JSDelegate(this, &QnApplicationListener::setInputString));

			method_dispatcher_.Bind(app_object,
				WSLit("runInputString"),
				JSDelegate(this, &QnApplicationListener::runInputString));

			method_dispatcher_.Bind(app_object,
				WSLit("openGameFile"),
				JSDelegate(this, &QnApplicationListener::openGameFile));

			method_dispatcher_.Bind(app_object,
				WSLit("runDefaultGame"),
				JSDelegate(this, &QnApplicationListener::runDefaultGame));

			method_dispatcher_.Bind(app_object,
				WSLit("listLocalGames"),
				JSDelegate(this, &QnApplicationListener::listLocalGames));

			method_dispatcher_.Bind(app_object,
				WSLit("selectLocalGameInGamestock"),
				JSDelegate(this, &QnApplicationListener::selectLocalGameInGamestock));

			// Привязываем колбэк для обработки вызовов alert
			method_dispatcher_.Bind(app_object,
				WSLit("alert"),
				JSDelegate(this, &QnApplicationListener::alert));
		} else {
			showError("Не удалось создать JS-объект QspLibAwesomium");
			return false;
		}

		// Привязываем обработчик колбэков к WebView
		web_view->set_js_method_handler(&method_dispatcher_);
		return true;
	}

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                   Основная логика
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************


	// Устанавливаем мьютекс для управления экземплярами плеера.
	bool QnApplicationListener::registerInstance()
	{
		// Идентификатором мьютекса является:
		// уникальный ключ приложения + строка пути к запускаемому файлу
		// Это позволит перезапускать только обновлённые игры.
		bool single = Configuration::getBool(ecpLimitSingleInstance);
		string mutexId = getInstanceMutexId();
		//Configuration::setString(ecpWindowTitle, mutexId);
		wstring wMutexId = widen(mutexId);
		hInstanceMutex = CreateMutex(NULL, FALSE, wMutexId.c_str());
		DWORD dwError = GetLastError();
		if (hInstanceMutex == NULL) {
			showError("Ошибка при создании мьютекса");
			return false;
		} else if (dwError == ERROR_ALREADY_EXISTS) {
			// Плеер для указанной игры уже запущен.
			if (single) {
				// Сообщаем ему, что игру нужно перезагрузить с диска, 
				// и молча выходим.
				string gameFilePath = Configuration::getString(ecpGameFilePath);
				COPYDATASTRUCT cds;
				cds.dwData = (ULONG_PTR)eidtRestart;
				cds.cbData = (DWORD)gameFilePath.length() + 1;
				cds.lpData = (PVOID)gameFilePath.c_str();
				HWND hwndSender = hwnd();
				HWND hwndReceiver = FindWindow(szWindowClass, NULL);
				if (hwndReceiver == NULL) {
					showError("Ошибка при поиске существующего окна плеера");
					return false;
				}
				LRESULT res = SendMessage(hwndReceiver, WM_COPYDATA, (WPARAM)hwndSender, (LPARAM)&cds);
				if (res != TRUE) {
					showError("Ошибка при отправке сообщения экземпляру плеера");
				}
				// Переключаемся в окно ранее запущенного плеера.
				SwitchToThisWindow(hwndReceiver, FALSE);
				return false;
			}
		}
		return true;
	}

	// Сбрасываем мьютекс для управления экземплярами плеера.
	void QnApplicationListener::unregisterInstance()
	{
		if (hInstanceMutex != NULL) {
			freeHandle(hInstanceMutex);
			hInstanceMutex = NULL;
		}
	}

	// Хэндл основного окна плеера.
	HWND QnApplicationListener::hwnd()
	{
		return view_ != NULL ? ((ViewWin*)(view_))->hwnd() : NULL;
	}

	void QnApplicationListener::initLib()
	{
		// Контекст UI
		gameIsRunning = false;

		//Запускаем поток библиотеки
		StartLibThread();
	}

	void QnApplicationListener::FreeResources()
	{
		// Контекст UI

		// Процедура "честного" высвобождения всех ресурсов - в т.ч. остановка потока библиотеки

		// Очищаем ВСЕ на выходе
		if (gameIsRunning)
		{
			StopGame(false);
		}
		// Останавливаем поток библиотеки
		StopLibThread();
	}

	void QnApplicationListener::runGame(string fileName)
	{
		// Контекст UI
		if (gameIsRunning) {
			if (!checkForSingle(evLibIsReady))
				return;
		} else {
			waitForSingle(evLibIsReady);
		}

		// Готовим данные для передачи в поток
		lockData();
		g_sharedData[evRunGame].str = fileName;
		// Передаём настройку из конфига в скин.
		g_sharedData[evRunGame].num = Configuration::getBool(ecpGameIsStandalone) ? 1 : 0;
		runSyncEvent(evRunGame);
		unlockData();

		gameIsRunning = true;

		// Если плеер не запущен в режиме "standalone",
		// и запущена игра не по умолчанию,
		// то записываем время последнего запуска в БД.
		if (!Configuration::getBool(ecpGameIsStandalone)
			&& !Configuration::getBool(ecpRunningDefaultGame)) {
			GamestockEntry game;
			// Пока что запускаем только локальные игры.
			game.web = false;
			game.local_file = Configuration::getString(ecpGameFilePath);
			game.title = Configuration::getString(ecpGameTitle);
			game.hash = Configuration::getString(ecpGameHash);
			game.cache = Configuration::getString(ecpCacheDir);
			game.saves = Configuration::getString(ecpSaveDir);
			game.last_run = (int)time(0);
			Gamestock::updateGame(game);
		}
	}

	void QnApplicationListener::StopGame(bool restart)
	{
		//Контекст UI
		if (gameIsRunning)
		{
			// Мы должны иметь возможность остановить игру в любой момент.
			runSyncEvent(evStopGame);
			waitForSingle(evGameStopped);
			gameIsRunning = false;
		}
	}

	void QnApplicationListener::runNewGame(string contentPath)
	{
		// Контекст UI

		// Запускаем игру из нового пути, 
		// либо полностью перечитываем с диска существующую игру.

		// Для начала, очищаем то, что уже запущено.
		FreeResources();

		// Заново загружаем конфигурацию, копируем файлы.
		if (!initOptions(contentPath) || !prepareGameFiles()) {
			app_->Quit();
			return;
		}

		// Обновляем свойства окна согласно настройкам шаблона.
		view_->applySkinToWindow();

		// Запускаем поток библиотеки.
		initLib();

		string url = QuestNavigator::getContentUrl();
		WebURL newUrl(ToWebString(url));
		WebView* webView = view_->web_view();
		if (newUrl == webView->url()) {
			// Очищаем кэш веб-содержимого.
			webView->session()->ClearCache();
			// Загружаем страницу заново, игнорируя закэшированные файлы.
			webView->Reload(true);
		} else {
			// Загружаем шаблон.
			webView->LoadURL(newUrl);
		}
	}

	bool QnApplicationListener::isGameRunning()
	{
		return gameIsRunning;
	}

	JSObject QnApplicationListener::getSaveSlots(bool open)
	{
		//Контекст UI
		JSArray jsSlots;
		int maxSlots = Configuration::getInt(ecpSaveSlotMax);
		for (int i = 0; i < maxSlots; i++)
		{
			string title;
			string slotname = to_string(i + 1) + ".sav";
			string slotpath = getRightPath(Configuration::getString(ecpSaveDir) + PATH_DELIMITER + slotname);
			if (fileExists(slotpath))
				title = to_string(i + 1);
			else
				title = "-empty-";
			jsSlots.Push(ToWebString(title));
		}

		JSObject jsSlotsContainer;
		jsSlotsContainer.SetProperty(WSLit("open"), JSValue(open ? 1 : 0));
		jsSlotsContainer.SetProperty(WSLit("slots"), jsSlots);
		return jsSlotsContainer;
	}

	// Выполнение строки кода
	void QnApplicationListener::executeCode(string qspCode)
	{
		//Контекст UI
		if (gameIsRunning)
		{
			if (!checkForSingle(evLibIsReady))
				return;

			lockData();
			g_sharedData[evExecuteCode].str = qspCode;
			runSyncEvent(evExecuteCode);
			unlockData();
		}
	}

	// Курсор находится в текстовом поле
	bool QnApplicationListener::textInputIsFocused()
	{
		return view_->web_view()->focused_element_type() == kFocusedElementType_TextInput;
	}

	// Переключение полноэкранного режима
	void QnApplicationListener::toggleFullscreen()
	{
		view_->toggleFullscreen();
	}

	// Обработка команды из другого экземпляра плеера.
	bool QnApplicationListener::processIpcData(COPYDATASTRUCT* pCds)
	{
		eIpcDataType type = (eIpcDataType)pCds->dwData;
		switch (type) {
			// Другой экземпляр плеера сообщил,
			// что нам требуется перечитать игру с диска заново.
		case eidtRestart:
			{
				// Получаем путь к игре.
				// Мы его и так знаем, 
				// но это на случай, 
				// если в будущем захочется запустить другую игру вместо уже запущенной.
				string message = (char*)pCds->lpData;
				// Перезапускаем игру.
				runNewGame(message);
			}
			break;
		default:
			return false;
		}
		return true;
	}

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                       Колбэки интерпретатора
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void QnApplicationListener::RefreshInt(int isRedraw) 
	{
		//Контекст библиотеки
		bool needUpdate = Skin::isSomethingChanged();
		Skin::updateBaseVars();
		needUpdate = needUpdate || Skin::isSomethingChanged();
		Skin::updateMainScreen();
		needUpdate = needUpdate || Skin::isSomethingChanged();

		JSObject jsSkin;
		bool bSkinPrepared = false;
		if (needUpdate) {
			jsSkin = Skin::getJsSkin();
			bSkinPrepared = true;
		}

		//основное описание
		string mainDesc = "";
		bool bMainDescPrepared = false;
		bool bMainDescNeedScroll = false;
		if ((QSPIsMainDescChanged() == QSP_TRUE) || Skin::isHtmlModeChanged)
		{
			mainDesc = Skin::applyHtmlFixes(fromQsp(QSPGetMainDesc()));
			bMainDescNeedScroll = startsWith(mainDesc, lastMainDesc);
			lastMainDesc = mainDesc;
			bMainDescPrepared = true;
		}

		//список действий
		JSArray acts;
		bool bActsPrepared = false;
		if ((QSPIsActionsChanged() == QSP_TRUE) || Skin::isHtmlModeChanged)
		{
			int nActsCount = QSPGetActionsCount();
			for (int i = 0; i < nActsCount; i++)
			{
				QSP_CHAR* pImgPath;
				QSP_CHAR* pDesc;
				QSPGetActionData(i, &pImgPath, &pDesc);
				JSObject act;
				string imgPath = getRightPath(fromQsp(pImgPath));
				string desc = Skin::applyHtmlFixes(fromQsp(pDesc));
				act.SetProperty(WSLit("image"), ToWebString(imgPath));
				act.SetProperty(WSLit("desc"), ToWebString(desc));
				acts.Push(act);
			}
			bActsPrepared = true;
		}

		//инвентарь
		JSArray objs;
		bool bObjsPrepared = false;
		int nSelectedObject = QSPGetSelObjectIndex();
		if ((QSPIsObjectsChanged() == QSP_TRUE) || (nSelectedObject != objectSelectionIndex) || Skin::isHtmlModeChanged)
		{
			objectSelectionIndex = nSelectedObject;
			int nObjsCount = QSPGetObjectsCount();
			for (int i = 0; i < nObjsCount; i++)
			{
				QSP_CHAR* pImgPath;
				QSP_CHAR* pDesc;
				QSPGetObjectData(i, &pImgPath, &pDesc);
				JSObject obj;
				string imgPath = getRightPath(fromQsp(pImgPath));
				string desc = Skin::applyHtmlFixes(fromQsp(pDesc));
				int selected = (i == nSelectedObject) ? 1 : 0;
				obj.SetProperty(WSLit("image"), ToWebString(imgPath));
				obj.SetProperty(WSLit("desc"), ToWebString(desc));
				obj.SetProperty(WSLit("selected"), JSValue(selected));
				objs.Push(obj);
			}
			bObjsPrepared = true;
		}

		//доп. описание
		string varsDesc = "";
		bool bVarsDescPrepared = false;
		if ((QSPIsVarsDescChanged() == QSP_TRUE) || Skin::isHtmlModeChanged)
		{
			varsDesc = Skin::applyHtmlFixes(fromQsp(QSPGetVarsDesc()));
			bVarsDescPrepared = true;
		}

		// Яваскрипт, переданный из игры командой EXEC('JS:...')
		string jsCmd = "";
		bool bJsCmdPrepared = false;
		if (jsExecBuffer.length() > 0)
		{
			jsCmd = jsExecBuffer;
			jsExecBuffer = "";
			bJsCmdPrepared = true;
		}

		// Передаем собранные данные в яваскрипт
		if (bSkinPrepared || bMainDescPrepared || bActsPrepared || bObjsPrepared || bVarsDescPrepared ||
			bJsCmdPrepared)
		{
			JSObject groupedContent;
			if (bSkinPrepared)
				groupedContent.SetProperty(WSLit("skin"), jsSkin);
			if (bMainDescPrepared) {
				groupedContent.SetProperty(WSLit("main"), ToWebString(mainDesc));
				groupedContent.SetProperty(WSLit("scrollmain"), JSValue(bMainDescNeedScroll ? 1 : 0));
			}
			if (bActsPrepared)
				groupedContent.SetProperty(WSLit("acts"), acts);
			if (bVarsDescPrepared)
				groupedContent.SetProperty(WSLit("vars"), ToWebString(varsDesc));
			if (bObjsPrepared)
				groupedContent.SetProperty(WSLit("objs"), objs);
			if (bJsCmdPrepared)
				groupedContent.SetProperty(WSLit("js"), ToWebString(jsCmd));
			qspSetGroupedContent(groupedContent);
		}
		Skin::resetUpdate();
	}

	void QnApplicationListener::SetTimer(int msecs)
	{
		//Контекст библиотеки
		timerInterval = msecs;
		startTimer();
	}

	void QnApplicationListener::SetInputStrText(QSP_CHAR* text)
	{
		//Контекст библиотеки
		qspSetInputString(ToWebString(fromQsp(text)));
	}

	void QnApplicationListener::ShowMessage(QSP_CHAR* message)
	{
		//Контекст библиотеки

		// Обновляем скин
		Skin::updateBaseVars();
		Skin::updateMsgDialog();
		// Если что-то изменилось, то передаем в яваскрипт
		if (Skin::isSomethingChanged())
		{
			RefreshInt(QSP_TRUE);
		}

		string msgValue = Skin::applyHtmlFixes(fromQsp(message));

		// Передаём данные в поток UI
		qspMsg(ToWebString(msgValue));

		// Ждём закрытия диалога
		waitForSingleLib(evMsgClosed);
	}

	void QnApplicationListener::PlayFile(QSP_CHAR* file, int volume)
	{
		//Контекст библиотеки
		string fileName = fromQsp(file);
		SoundManager::play(fileName, volume);
	}

	QSP_BOOL QnApplicationListener::IsPlayingFile(QSP_CHAR* file)
	{
		//Контекст библиотеки
		bool isPlaying = SoundManager::isPlaying(fromQsp(file));
		return isPlaying ? QSP_TRUE : QSP_FALSE;
	}

	void QnApplicationListener::CloseFile(QSP_CHAR* file)
	{
		//Контекст библиотеки
		bool closeAll = file == NULL;
		SoundManager::close(closeAll, fromQsp(file));
	}

	void QnApplicationListener::ShowPicture(QSP_CHAR* file)
	{
		//Контекст библиотеки
		string fileName = getRightPath(fromQsp(file));

		// Проверяем читаемость файла.
		// Если файл не существует или не читается, выходим.
		if (fileName.length() > 0) {
			if (!fileExists(fileName)) {
				showError("Оператор VIEW. Не найден файл: " + fileName);
				return;
			}
		}

		// "Пустое" имя файла тоже имеет значение - так мы скрываем картинку
		qspView(ToWebString(fileName));
	}

	void QnApplicationListener::InputBox(const QSP_CHAR* prompt, QSP_CHAR* buffer, int maxLen)
	{
		//Контекст библиотеки

		// Обновляем скин
		Skin::updateBaseVars();
		Skin::updateInputDialog();
		// Если что-то изменилось, то передаем в яваскрипт
		if (Skin::isSomethingChanged())
		{
			RefreshInt(QSP_TRUE);
		}

		string promptValue = fromQsp(prompt);

		// Передаём данные в поток UI
		qspInput(ToWebString(promptValue));

		// Ждём закрытия диалога
		waitForSingleLib(evInputClosed);

		// Возвращаем результат в библиотеку
		string result = "";
		lockData();
		result = g_sharedData[evInputClosed].str;
		unlockData();
		wstring wResult = widen(result);
		wcsncpy(buffer, wResult.c_str(), maxLen);
	}


	// Функция запросов к плееру.
	// С помощью этой функции мы можем в игре узнать параметры окружения плеера.
	// Вызывается так: $platform = GETPLAYER('platform')
	// @param resource
	// @return

	void QnApplicationListener::PlayerInfo(QSP_CHAR* resource, QSP_CHAR* buffer, int maxLen)
	{
		//Контекст библиотеки
		string resourceName = fromQsp(resource);
		resourceName = toLower(resourceName);
		string result = "";
		if (resourceName == "platform") {
			result = "Windows";
		} else if (resourceName == "player") {
			result = QN_APP_NAME;
		} else if (resourceName == "player.version") {
			result = QN_VERSION;
		}

		// Возвращаем результат в библиотеку
		wstring wResult = widen(result);
		wcsncpy(buffer, wResult.c_str(), maxLen);
	}

	int QnApplicationListener::GetMSCount()
	{
		//Контекст библиотеки
		clock_t now = clock();
		int elapsed = (int) (((now - gameStartTime) * 1000) / CLOCKS_PER_SEC);
		gameStartTime = now;
		return elapsed;
	}

	void QnApplicationListener::AddMenuItem(QSP_CHAR* name, QSP_CHAR* imgPath)
	{
		//Контекст библиотеки
		ContainerMenuItem item;
		item.imgPath = getRightPath(fromQsp(imgPath));
		item.name = fromQsp(name);
		menuList.push_back(item);
	}

	int QnApplicationListener::ShowMenu()
	{
		//Контекст библиотеки

		JSArray jsMenuList;
		for (int i = 0; i < (int)menuList.size(); i++) {
			JSObject jsMenuItem;
			jsMenuItem.SetProperty(WSLit("image"), ToWebString(menuList[i].imgPath));
			jsMenuItem.SetProperty(WSLit("desc"), ToWebString(Skin::applyHtmlFixes(menuList[i].name)));
			jsMenuList.Push(jsMenuItem);
		}

		// Передаём данные в поток UI
		qspMenu(jsMenuList);

		// Ждём закрытия диалога
		waitForSingleLib(evMenuClosed);

		// Возвращаем результат
		int result = -1;
		lockData();
		result = g_sharedData[evMenuClosed].num;
		unlockData();

		return result;
	}

	void QnApplicationListener::DeleteMenu()
	{
		//Контекст библиотеки
		menuList.clear();
	}

	void QnApplicationListener::Wait(int msecs)
	{
		//Контекст библиотеки
		Sleep((DWORD)msecs);
	}

	void QnApplicationListener::ShowWindow(int type, QSP_BOOL isShow)
	{
		// Контекст библиотеки
		Skin::showWindow(type, isShow);
	}

	void QnApplicationListener::System(QSP_CHAR* cmd)
	{
		//Контекст библиотеки
		string jsCmd = fromQsp(cmd);
		string jsCmdUpper = toUpper(jsCmd);
		if (startsWith(jsCmdUpper, "JS:"))
		{
			jsCmd = jsCmd.substr(string("JS:").length());
			// Сохраняем яваскрипт, переданный из игры командой EXEC('JS:...')
			// На выполнение отдаём при обновлении интерфейса
			jsExecBuffer = jsExecBuffer + jsCmd;
		}
	}

	void QnApplicationListener::OpenGameStatus(QSP_CHAR* file)
	{
		//Контекст библиотеки
		if (file != 0) {
			// Библиотека возвращает абсолютный путь к файлу сохранения,
			// вычисляемый по пути к файлу игры.
			// Таким образом, если игра запущена из пути "D:\CoolGame\game.qsp",
			// то при выполнении команды 
			// OPENGAME 'saves\save1.sav'
			// сейв будет загружаться из папки игры:
			// "D:\CoolGame\saves\save1.sav"
			// Нас это не устраивает, 
			// нам нужно, чтобы сейвы хранились в отдельном безопасном месте.
			// Поэтому мы меняем путь, заданный библиотекой, на свой.
			string saveFile = getRealSaveFile(fromQsp(file));
			if (fileExists(saveFile)) {
				QSP_BOOL res = QSPOpenSavedGame(widen(saveFile).c_str(), QSP_FALSE);
				CheckQspResult(res, "QSPOpenSavedGame");
			}
		} else {
			jsExecBuffer = jsExecBuffer + ";qspLoadGame();";
		}
	}

	void QnApplicationListener::SaveGameStatus(QSP_CHAR* file)
	{
		//Контекст библиотеки
		if (file != 0) {
			string saveDir = Configuration::getString(ecpSaveDir);
			if (!dirExists(saveDir) && !buildDirectoryPath(saveDir)) {
				showError("Не удалось создать папку для сохранения: " + saveDir);
				return;
			}
			// Библиотека возвращает абсолютный путь к файлу сохранения,
			// вычисляемый по пути к файлу игры.
			// Таким образом, если игра запущена из пути "D:\CoolGame\game.qsp",
			// то при выполнении команды 
			// SAVEGAME 'saves\save1.sav'
			// сейв будет сохраняться в папке игры:
			// "D:\CoolGame\saves\save1.sav"
			// Нас это не устраивает, 
			// нам нужно, чтобы сейвы хранились в отдельном безопасном месте.
			// Поэтому мы меняем путь, заданный библиотекой, на свой.
			string saveFile = getRealSaveFile(fromQsp(file));
			QSP_BOOL res = QSPSaveGame(widen(saveFile).c_str(), QSP_FALSE);
			CheckQspResult(res, "QSPSaveGame");
		} else {
			jsExecBuffer = jsExecBuffer + ";qspSaveGame();";
		}
	}

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                        Вызовы JS-функций
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

	void QnApplicationListener::processLibJsCall()
	{
		// Контекст UI
		string name = "";
		JSValue arg;
		lockData();
		name = g_sharedData[evJsCommitted].str;
		arg = g_sharedData[evJsCommitted].jsValue;
		unlockData();
		jsCallApiFromUi(name, arg);
		runSyncEvent(evJsExecuted);
	}

	bool QnApplicationListener::jsCallApiFromUi(string name, JSValue arg)
	{
		// Контекст UI
		// Получение ссылки на объект окна выполняется за 0.005 секунд, 
		// поэтому можно не кэшировать.
		JSValue window = view_->web_view()->ExecuteJavascriptWithResult(
			WSLit("window"), WSLit(""));
		if (window.IsObject()) {
			JSArray args;
			args.Push(arg);
			JSObject windowObject = window.ToObject();
			windowObject.Invoke(ToWebString(name), args);
			Error err = windowObject.last_error();
			if (err != Error::kError_None) {
				showError("Ошибка при выполнении JS-вызова.");
				return false;
			}
		} else {
			showError("Не удалось получить доступ к объекту окна.");
			return false;
		}
		return true;
	}
	void QnApplicationListener::jsCallApiFromLib(string name, JSValue arg)
	{
		// Контекст библиотеки
		// Вызвать функции Awesomium можно только из UI-потока.

		// Отправляем данные в UI-поток.
		lockData();
		g_sharedData[evJsCommitted].str = name;
		g_sharedData[evJsCommitted].jsValue = arg;
		runSyncEvent(evJsCommitted);
		unlockData();

		// Дожидаемся ответа, что JS-запрос выполнен.
		waitForSingleLib(evJsExecuted);
	}
	bool QnApplicationListener::onWebDeviceReady()
	{
		// Контекст UI
		JSValue v;
		return jsCallApiFromUi("onWebDeviceReady", v);
	}
	void QnApplicationListener::qspShowSaveSlotsDialog(JSObject content)
	{
		// Контекст UI
		jsCallApiFromUi("qspShowSaveSlotsDialog", content);
	}
	void QnApplicationListener::qspFillLocalGamesList(JSArray games)
	{
		// Контекст UI
		jsCallApiFromUi("qspFillLocalGamesList", games);
	}
	void QnApplicationListener::qspSetGroupedContent(JSObject content)
	{
		// Контекст библиотеки
		jsCallApiFromLib("qspSetGroupedContent", content);
	}
	void QnApplicationListener::qspMsg(WebString text)
	{
		// Контекст библиотеки
		jsCallApiFromLib("qspMsg", text);
	}
	void QnApplicationListener::qspError(JSObject error)
	{
		// Контекст библиотеки
		jsCallApiFromLib("qspError", error);
	}
	void QnApplicationListener::qspMenu(JSArray menu)
	{
		// Контекст библиотеки
		jsCallApiFromLib("qspMenu", menu);
	}
	void QnApplicationListener::qspInput(WebString text)
	{
		// Контекст библиотеки
		jsCallApiFromLib("qspInput", text);
	}
	void QnApplicationListener::qspView(WebString path)
	{
		// Контекст библиотеки
		jsCallApiFromLib("qspView", path);
	}
	void QnApplicationListener::qspSetInputString(WebString text)
	{
		// Контекст библиотеки
		jsCallApiFromLib("qspSetInputString", text);
	}





	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                   Обработчики вызовов из JS
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	void QnApplicationListener::restartGame(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		string gameFile = Configuration::getString(ecpGameFilePath);
		StopGame(true);
		runGame(gameFile);
	}

	void QnApplicationListener::executeAction(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		if (args.size() < 1) {
			showError("Не указан параметр для executeAction!");
			return;
		}

		if (!checkForSingle(evLibIsReady))
			return;

		JSValue jsPos = args[0];
		int pos = jsPos.ToInteger();
		lockData();
		g_sharedData[evExecuteAction].num = pos;
		runSyncEvent(evExecuteAction);
		unlockData();
	}

	void QnApplicationListener::selectObject(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		if (args.size() < 1) {
			showError("Не указан параметр для selectObject!");
			return;
		}
		JSValue jsPos = args[0];
		int pos = jsPos.ToInteger();

		if (!checkForSingle(evLibIsReady))
			return;

		lockData();
		g_sharedData[evSelectObject].num = pos;
		runSyncEvent(evSelectObject);
		unlockData();
	}

	void QnApplicationListener::loadGame(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		// Останавливаем таймер
		stopTimer();
		// Загружаем список файлов и отдаем в яваскрипт
		JSObject slots = getSaveSlots(true);
		qspShowSaveSlotsDialog(slots);
	}

	void QnApplicationListener::saveGame(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		// Останавливаем таймер
		stopTimer();
		// Загружаем список файлов и отдаем в яваскрипт
		JSObject slots = getSaveSlots(false);
		qspShowSaveSlotsDialog(slots);
	}

	void QnApplicationListener::saveSlotSelected(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		if (args.size() < 2) {
			showError("Не указаны параметры для saveSlotSelected!");
			return;
		}
		// Номер слота
		JSValue jsIndex = args[0];
		int index = jsIndex.ToInteger();
		// 1 - загрузка, 0 - сохранение
		JSValue jsMode = args[1];
		int mode = jsMode.ToInteger();

		if (index == -1)
		{
			// Запускаем таймер
			startTimer();
			return;
		}

		if (!checkForSingle(evLibIsReady))
			return;

		if (mode == 1) {
			lockData();
			g_sharedData[evLoadSlotSelected].num = index;
			runSyncEvent(evLoadSlotSelected);
			unlockData();
		} else {
			lockData();
			g_sharedData[evSaveSlotSelected].num = index;
			runSyncEvent(evSaveSlotSelected);
			unlockData();
		}
	}

	void QnApplicationListener::msgResult(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		runSyncEvent(evMsgClosed);
	}

	void QnApplicationListener::errorResult(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		runSyncEvent(evErrorClosed);
	}

	void QnApplicationListener::userMenuResult(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		if (args.size() < 1) {
			showError("Не указан параметр для userMenuResult!");
			return;
		}
		JSValue jsPos = args[0];
		int pos = jsPos.ToInteger();

		lockData();
		g_sharedData[evMenuClosed].num = pos;
		runSyncEvent(evMenuClosed);
		unlockData();
	}

	void QnApplicationListener::inputResult(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		if (args.size() < 1) {
			showError("Не указан параметр для inputResult!");
			return;
		}
		JSValue jsText = args[0];
		string text = ToString(jsText.ToString());

		lockData();
		g_sharedData[evInputClosed].str = text;
		runSyncEvent(evInputClosed);
		unlockData();
	}

	void QnApplicationListener::setMute(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		if (args.size() < 1) {
			showError("Не указан параметр для setMute!");
			return;
		}
		JSValue jsFlag = args[0];
		bool flag = jsFlag.ToBoolean();

		lockData();
		g_sharedData[evMute].flag = flag;
		runSyncEvent(evMute);
		unlockData();
	}

	void QnApplicationListener::setInputString(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		// Изменился текст в строке ввода
		if (args.size() < 1) {
			showError("Не указан параметр для setInputString!");
			return;
		}
		JSValue jsText = args[0];
		string text = ToString(jsText.ToString());

		lockData();
		g_sharedData[evInputStringChanged].str = text;
		runSyncEvent(evInputStringChanged);
		unlockData();
	}

	void QnApplicationListener::runInputString(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		// Нажали Enter в строке ввода
		runSyncEvent(evInputStringEntered);
	}

	void QnApplicationListener::openGameFile(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		// Вызываем диалог открытия файла с диска.
		string filePath = openGameFileDialog(hwnd());
		if (filePath == "")
			return;

		// Запускаем выбранную игру.
		runNewGame(filePath);
	}

	void QnApplicationListener::runDefaultGame(WebView* caller, const JSArray& args)
	{
		// Контекст UI

		// Запускаем игру по умолчанию.
		runNewGame("");
	}

	void QnApplicationListener::listLocalGames(WebView* caller, const JSArray& args)
	{
		// Контекст UI

		// Возвращаем список игр локальной полки.
		vector<GamestockEntry> vecLocalGames;

		// Загружаем список игр.
		// Вывод ошибки выполняется внутри вызова, нам остаётся просто выйти.
		if (!Gamestock::getLocalGamesWithCheck(vecLocalGames))
			return;

		// Список загружен, формируем контейнер для передачи в JS.
		JSArray jsLocalGames;
		for (int i = 0; i < (int)vecLocalGames.size(); i++)
		{
			GamestockEntry game = vecLocalGames[i];
			JSObject jsLocalGame;
			jsLocalGame.SetProperty(ToWebString("hash"), ToWebString(game.hash));
			jsLocalGame.SetProperty(ToWebString("title"), ToWebString(game.title));
			jsLocalGame.SetProperty(ToWebString("local_file"), ToWebString(game.local_file));
			jsLocalGames.Push(jsLocalGame);
		}

		// Передаём список игр в JS.
		qspFillLocalGamesList(jsLocalGames);
	}

	void QnApplicationListener::selectLocalGameInGamestock(WebView* caller, const JSArray& args)
	{
		// Контекст UI
		// Выбор игры для запуска.
		if (args.size() < 1) {
			showError("Не указан параметр для selectLocalGameInGamestock!");
			return;
		}
		JSValue jsHash = args[0];
		string hash = ToString(jsHash.ToString());

		// Ищем игру по указанному хэшу.
		GamestockEntry game;
		if (!Gamestock::getLocalGame(hash, game)) {
			showError("Не найдена игра с указанным хэшем");
			return;
		}

		//STUB
		// Сделать обновление времени последнего запуска.

		// Запускаем выбранную игру.
		runNewGame(game.local_file);
	}

	// ********************************************************************
	// Вспомогательные обработчики для отладки
	// ********************************************************************
	void QnApplicationListener::alert(WebView* caller, const JSArray& args)
	{
		string msg = "";
		if (args.size() > 0) {
			msg = ToString(args[0].ToString());
		}
		showMessage(msg, "");
	}

	// ********************************************************************
	// Переменные библиотеки
	// ********************************************************************

	string QnApplicationListener::jsExecBuffer = "";
	string QnApplicationListener::lastMainDesc = "";
	QnApplicationListener* QnApplicationListener::listener = NULL;
	vector<ContainerMenuItem> QnApplicationListener::menuList;
	clock_t QnApplicationListener::gameStartTime = 0;
	int QnApplicationListener::timerInterval = 0;

	int QnApplicationListener::objectSelectionIndex = -2;

	//******************************************************************************
	//******************************************************************************
	//****** / THREADS \ ***********************************************************
	//******************************************************************************
	//******************************************************************************

	// Все функции библиотеки QSP (QSPInit и т.д.) 
	// вызываются только внутри потока библиотеки.

	// Создаём объект ядра для синхронизации потоков,
	// событие с автосбросом, инициализированное в занятом состоянии.
	HANDLE CreateSyncEvent()
	{
		HANDLE eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (eventHandle == NULL) {
			showError("Не получилось создать объект ядра \"событие\" для синхронизации потоков.");
			exit(eecFailToCreateEvent);
		}
		return eventHandle;
	}

	// Создаём таймер.
	HANDLE CreateTimer()
	{
		HANDLE timerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
		if (timerHandle == NULL) {
			showError("Не получилось создать таймер.");
			exit(eecFailToCreateTimer);
		}
		return timerHandle;
	}

	// Получаем HANDLE события по его индексу
	HANDLE getEventHandle(eSyncEvent ev)
	{
		return g_eventList[ev];
	}

	// Запускаем событие
	void runSyncEvent(eSyncEvent ev)
	{
		BOOL res = SetEvent(getEventHandle(ev));
		if (res == 0) {
			showError("Не удалось запустить событие синхронизации потоков.");
			exit(eecFailToSetEvent);
		}
	}

	// Высвобождаем описатель и ругаемся если что не так.
	void freeHandle(HANDLE handle)
	{
		BOOL res = CloseHandle(handle);
		if (res == 0) {
			showError("Не удалось высвободить описатель объекта ядра.");
			exit(eecFailToCloseHandle);
		}
	}

	// Входим в критическую секцию
	void lockData()
	{
		try {
			EnterCriticalSection(&g_csSharedData);
		} catch (...) {
			showError("Не удалось войти в критическую секцию.");
			exit(eecUnableEnterCs2);
		}
	}

	// Выходим из критической секции
	void unlockData()
	{
		LeaveCriticalSection(&g_csSharedData);
	}

	// Ожидаем события
	bool waitForSingle(HANDLE handle)
	{
		DWORD res = WaitForSingleObject(handle, INFINITE);
		if (res != WAIT_OBJECT_0) {
			showError("Не удалось дождаться события синхронизации");
			return false;
		}
		return true;
	}
	bool waitForSingle(eSyncEvent ev)
	{
		return waitForSingle(getEventHandle(ev));
	}
	bool waitForSingleLib(eSyncEvent ev)
	{
		// Контекст библиотеки.
		// Мы должны иметь возможность в любой момент остановить игру либо поток библиотеки.
		// Поэтому при ожидании в библиотеке единичного события синхронизации,
		// такого как например закрытие диалога,
		// мы вместо "waitForSingle" вызываем "waitForSingleLib".
		// Эта функция дополнительно проверяет на наличие события, 
		// указывающего, что мы должны остановить игру либо поток библиотеки.

		// События для синхронизации потоков
		HANDLE eventList[3];
		eSyncEvent syncEvents[3];
		syncEvents[0] = evShutdown;
		syncEvents[1] = evStopGame;
		syncEvents[2] = ev;
		for (int i = 0; i < 3; i++) {
			eventList[i] = getEventHandle(syncEvents[i]);
		}

		DWORD res = WaitForMultipleObjects((DWORD)3, eventList, FALSE, INFINITE);
		if ((res < WAIT_OBJECT_0) || (res > (WAIT_OBJECT_0 + 3 - 1))) {
			showError("Не удалось дождаться единичного события синхронизации библиотеки.");
		} else {
			// Если событие было "evShutdown" или "evStopGame",
			// вызываем их заново.
			// Это необходимо, так как вызов "WaitForMultipleObjects" их уже сбросил.
			for (int i = 0; i < 3; i++) {
				if (eventList[i])
				break;
			}
			eSyncEvent breakingEvent = syncEvents[res];
			if (breakingEvent != ev) {
				runSyncEvent(breakingEvent);
			}
		}
		return true;
	}
	bool checkForSingle(eSyncEvent ev)
	{
		// Проверяем, доступен ли объект синхронизации.
		// Если недоступен, сразу возвращаем "false".
		// Для ожидания объекта следует использовать "waitForSingle".
		HANDLE handle = getEventHandle(ev);
		DWORD res = WaitForSingleObject(handle, 0);
		if ((res == WAIT_ABANDONED) || (res == WAIT_FAILED)) {
			showError("Сбой синхронизации");
			return false;
		}
		return res == WAIT_OBJECT_0;
	}

	// Запуск потока библиотеки. Вызывается только раз при старте программы.
	void QnApplicationListener::StartLibThread()
	{
		//Контекст UI
		if (libThread != NULL)
		{
			showError("StartLibThread: failed, libThread is not null");
			exit(eecLibThreadAlreadyStarted);
			return;
		}

		// Инициализируем объекты синхронизации.
		// Используемые объекты - события с автосбросом, 
		// инициализированные в занятом состоянии, и один таймер.
		for (int i = 0; i < (int)evLast; i++) {
			HANDLE eventHandle = (i == (int)evTimer) ? CreateTimer() : CreateSyncEvent();
			if (eventHandle == NULL)
				return;
			g_eventList[i] = eventHandle;
		}
		// Инициализируем структуру критической секции
		try {
			InitializeCriticalSection(&g_csSharedData);
		} catch (...) {
			showError("Не удалось проинициализировать структуру критической секции.");
			exit(eecFailToInitCs);
			return;
		}

		libThread = (HANDLE)_beginthreadex(NULL, 0, &QnApplicationListener::libThreadFunc, this, 0, NULL);
		if (libThread == NULL) {
			showError("Не получилось создать поток интерпретатора.");
			exit(eecFailToBeginLibThread);
			return;
		}
	}

	// Остановка потока библиотеки. Вызывается только раз при завершении программы.
	void QnApplicationListener::StopLibThread()
	{
		if (libThread == NULL)
			return;

		// Сообщаем потоку библиотеки, что нужно завершить работу
		runSyncEvent(evShutdown);
		// Ждём завершения библиотечного потока
		waitForSingle(libThread);
		// Закрываем хэндл библиотечного потока
		freeHandle(libThread);
		libThread = NULL;
		// Закрываем хэндлы событий
		for (int i = 0; i < (int)evLast; i++) {
			freeHandle(g_eventList[i]);
			g_eventList[i] = NULL;
		}
		// Высвобождаем структуру критической секции
		DeleteCriticalSection(&g_csSharedData);
	}

	// Основная функция потока библиотеки. Вызывается только раз за весь жизненный цикл программы.
	unsigned int QnApplicationListener::libThreadFunc(void* pvParam)
	{
		// Сохраняем указатель на listener
		listener = (QnApplicationListener*) pvParam;

		// Инициализируем библиотеку
		QSPInit();

		// Привязываем колбэки
		QSPSetCallBack(QSP_CALL_REFRESHINT, (QSP_CALLBACK)&RefreshInt);
		QSPSetCallBack(QSP_CALL_SETTIMER, (QSP_CALLBACK)&SetTimer);
		QSPSetCallBack(QSP_CALL_SETINPUTSTRTEXT, (QSP_CALLBACK)&SetInputStrText);
		QSPSetCallBack(QSP_CALL_ISPLAYINGFILE, (QSP_CALLBACK)&IsPlayingFile);
		QSPSetCallBack(QSP_CALL_PLAYFILE, (QSP_CALLBACK)&PlayFile);
		QSPSetCallBack(QSP_CALL_CLOSEFILE, (QSP_CALLBACK)&CloseFile);
		QSPSetCallBack(QSP_CALL_SHOWMSGSTR, (QSP_CALLBACK)&ShowMessage);
		QSPSetCallBack(QSP_CALL_SLEEP, (QSP_CALLBACK)&Wait);
		QSPSetCallBack(QSP_CALL_GETMSCOUNT, (QSP_CALLBACK)&GetMSCount);
		QSPSetCallBack(QSP_CALL_DELETEMENU, (QSP_CALLBACK)&DeleteMenu);
		QSPSetCallBack(QSP_CALL_ADDMENUITEM, (QSP_CALLBACK)&AddMenuItem);
		QSPSetCallBack(QSP_CALL_SHOWMENU, (QSP_CALLBACK)&ShowMenu);
		QSPSetCallBack(QSP_CALL_INPUTBOX, (QSP_CALLBACK)&InputBox);
		QSPSetCallBack(QSP_CALL_PLAYERINFO, (QSP_CALLBACK)&PlayerInfo);
		QSPSetCallBack(QSP_CALL_SHOWIMAGE, (QSP_CALLBACK)&ShowPicture);
		QSPSetCallBack(QSP_CALL_SHOWWINDOW, (QSP_CALLBACK)&ShowWindow);
		QSPSetCallBack(QSP_CALL_SYSTEM, (QSP_CALLBACK)&System);
		QSPSetCallBack(QSP_CALL_OPENGAMESTATUS, (QSP_CALLBACK)&OpenGameStatus);
		QSPSetCallBack(QSP_CALL_SAVEGAMESTATUS, (QSP_CALLBACK)&SaveGameStatus);

		// Заполняем значения по умолчанию для скина
		Skin::initDefaults();

		// Флаг для завершения потока
		bool bShutdown = false;

		// Запускаем движок Audiere для проигрывания звуковых файлов
		if (!SoundManager::init()) {
			bShutdown = true;
		}

		// Обработка событий происходит в цикле
		while (!bShutdown) {
			// Сообщаем потоку UI, что библиотека готова к выполнению команд
			runSyncEvent(evLibIsReady);
			// Ожидаем любое из событий синхронизации
			DWORD res = WaitForMultipleObjects((DWORD)evLastUi, g_eventList, FALSE, INFINITE);
			if ((res < WAIT_OBJECT_0) || (res > (WAIT_OBJECT_0 + evLast - 1))) {
				showError("Не удалось дождаться множественного события синхронизации библиотеки.");
				bShutdown = true;
			} else {
				eSyncEvent ev = (eSyncEvent)res;
				switch (ev)
				{
				case evRunGame:
					{
						// Запуск игры
						string path = "";
						int isStandalone = 0;
						lockData();
						path = g_sharedData[evRunGame].str;
						isStandalone = g_sharedData[evRunGame].num;
						unlockData();
						QSP_BOOL res = QSPLoadGameWorld(widen(path).c_str());
						CheckQspResult(res, "QSPLoadGameWorld");
						// Очищаем скин
						Skin::resetUpdate();
						Skin::resetSettings();
						// Передаём настройку из конфига в скин.
						Skin::setInt(espIsStandalone, isStandalone);
						// Очищаем буфер JS-команд, передаваемых из игры
						jsExecBuffer = "";

						// Устанавливаем период выполнения и запускаем таймер
						SetTimer(500);

						//Запускаем счетчик миллисекунд
						gameStartTime = clock();

						res = QSPRestartGame(QSP_TRUE);
						CheckQspResult(res, "QSPRestartGame");
					}
					break;
				case evStopGame:
					{
						// Остановка игры

						// Останавливаем таймер.
						stopTimer();

						//останавливаем музыку
						CloseFile(NULL);

						// Очищаем буфер JS-команд, передаваемых из игры
						jsExecBuffer = "";

						runSyncEvent(evGameStopped);
					}
					break;
				case evShutdown:
					{
						// Завершение работы
						bShutdown = true;
					}
					break;
				case evExecuteCode:
					{
						// Выполнение строки кода
						string code = "";
						lockData();
						code = g_sharedData[evExecuteCode].str;
						unlockData();
						wstring wCode = widen(code);
						QSP_BOOL res = QSPExecString(wCode.c_str(), QSP_TRUE);
						CheckQspResult(res, "QSPExecString");
					}
					break;
				case evExecuteAction:
					{
						// Выполнение действия
						int pos = 0;
						lockData();
						pos = g_sharedData[evExecuteAction].num;
						unlockData();
						QSP_BOOL res = QSPSetSelActionIndex(pos, QSP_FALSE);
						CheckQspResult(res, "QSPSetSelActionIndex");
						res = QSPExecuteSelActionCode(QSP_TRUE);
						CheckQspResult(res, "QSPExecuteSelActionCode");
					}
					break;
				case evSelectObject:
					{
						// Выбор предмета
						int pos = 0;
						lockData();
						pos = g_sharedData[evSelectObject].num;
						unlockData();
						// Костыль - следим за номером выбранного предмета,
						// так как иначе невозможно будет обновить
						// окно предметов при вызове UNSEL в ONOBJSEL.
						// Нужно исправить это в библиотеке QSP.
						objectSelectionIndex = -2;
						QSP_BOOL res = QSPSetSelObjectIndex(pos, QSP_TRUE);
						CheckQspResult(res, "QSPSetSelObjectIndex");
					}
					break;
				case evTimer:
					{
						// Таймер
						QSP_BOOL res = QSPExecCounter(QSP_TRUE);
						CheckQspResult(res, "QSPExecCounter");
					}
					break;
				case evMute:
					{
						// Включение / выключение звука
						bool flag = false;
						lockData();
						flag = g_sharedData[evMute].flag;
						unlockData();
						SoundManager::mute(flag);
					}
					break;
				case evLoadSlotSelected:
					{
						int index = 0;
						lockData();
						index = g_sharedData[evLoadSlotSelected].num;
						unlockData();
						jsExecBuffer = "";

						string path = getRightPath(Configuration::getString(ecpSaveDir) + PATH_DELIMITER + to_string(index) + ".sav");
						if (!fileExists(path)) {
							showError("Не найден файл сохранения");
							break;
						}

						// Выключаем музыку
						CloseFile(NULL);

						// Загружаем сохранение
						QSP_BOOL res = QSPOpenSavedGame(widen(path).c_str(), QSP_TRUE);
						CheckQspResult(res, "QSPOpenSavedGame");

						// Запускаем таймер
						startTimer();
					}
					break;
				case evSaveSlotSelected:
					{
						int index = 0;
						lockData();
						index = g_sharedData[evSaveSlotSelected].num;
						unlockData();
						jsExecBuffer = "";

						string saveDir = Configuration::getString(ecpSaveDir);
						if (!dirExists(saveDir) && !buildDirectoryPath(saveDir)) {
							showError("Не удалось создать папку для сохранения: " + saveDir);
							break;
						}

						string path = getRightPath(saveDir + PATH_DELIMITER + to_string(index) + ".sav");

						QSP_BOOL res = QSPSaveGame(widen(path).c_str(), QSP_FALSE);
						CheckQspResult(res, "QSPSaveGame");

						startTimer();
					}
					break;
				case evInputStringChanged:
					{
						// Изменился текст в строке ввода
						string text = "";
						lockData();
						text = g_sharedData[evInputStringChanged].str;
						unlockData();
						QSPSetInputStrText(widen(text).c_str());
					}
					break;
				case evInputStringEntered:
					{
						QSP_BOOL res = QSPExecUserInput(QSP_TRUE);
						CheckQspResult(res, "QSPExecUserInput");
					}
					break;
				default:
					{
						showError("Необработанное событие синхронизации!");
					}
					break;
				}
			}
		}

		// Останавливаем звуковой движок
		SoundManager::close(true, "");
		SoundManager::deinit();

		// Завершаем работу библиотеки
		QSPDeInit();
		// Завершаем работу потока
		_endthreadex(0);
		return 0;
	}
	//******************************************************************************
	//******************************************************************************
	//****** \ THREADS / ***********************************************************
	//******************************************************************************
	//******************************************************************************

	// Проверяем статус выполнения операции и сообщаем об ошибке, если требуется.
	void QnApplicationListener::CheckQspResult(QSP_BOOL successfull, string failMsg)
	{
		//Контекст библиотеки
		if (successfull == QSP_FALSE)
		{
			//Контекст библиотеки
			int line = -1;
			int actIndex = -1;
			string desc = "";
			string loc = "";
			int errorNum = -1;
			QSP_CHAR* pErrorLoc = NULL;
			QSPGetLastErrorData(&errorNum, &pErrorLoc, &actIndex, &line);
			loc = Skin::applyHtmlFixes(fromQsp(pErrorLoc));
			desc = Skin::applyHtmlFixes(fromQsp(QSPGetErrorDesc(errorNum)));

			// Обновляем скин
			Skin::updateBaseVars();
			Skin::updateMsgDialog();
			// Если что-то изменилось, то передаем в яваскрипт
			if (Skin::isSomethingChanged())
			{
				RefreshInt(QSP_TRUE);
			}

			JSObject jsErrorContainer;
			jsErrorContainer.SetProperty(WSLit("desc"), ToWebString(desc));
			jsErrorContainer.SetProperty(WSLit("loc"), ToWebString(loc));
			jsErrorContainer.SetProperty(WSLit("actIndex"), JSValue(actIndex));
			jsErrorContainer.SetProperty(WSLit("line"), JSValue(line));

			// Передаём данные в поток UI
			qspError(jsErrorContainer);

			// Ждём закрытия диалога
			waitForSingleLib(evErrorClosed);
		}
	}

	// Установка и запуск таймера
	void QnApplicationListener::startTimer()
	{
		// Устанавливаем период выполнения
		__int64         qwDueTime;
		LARGE_INTEGER   liDueTime;
		// Отрицательное 64-битное число указывает, 
		// через сколько наносекунд должен сработать первый вызов таймера.
		qwDueTime = -(timerInterval * 10000);
		liDueTime.LowPart  = (DWORD) ( qwDueTime & 0xFFFFFFFF );
		liDueTime.HighPart = (LONG)  ( qwDueTime >> 32 );

		//Запускаем таймер
		BOOL res = SetWaitableTimer(getEventHandle(evTimer), &liDueTime, (LONG)timerInterval, NULL, NULL, FALSE);
		if (res == 0) {
			showError("Не удалось установить интервал таймера!");
		}
	}

	// Остановка таймера
	void QnApplicationListener::stopTimer()
	{
		BOOL res = CancelWaitableTimer(getEventHandle(evTimer));
		if (res == 0) {
			showError("Не удалось остановить таймер!");
		}
	}
}