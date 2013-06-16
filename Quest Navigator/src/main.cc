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

		initLib();

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
		//QSPInit();
		//std::wstring message_str = QSPGetVersion();
		//   MessageBox(0, message_str.c_str(), message_str.c_str(), NULL);
		//QSPDeInit();

		gameIsRunning = false;
		qspInited = false;
		
		//curGameFile = "www/standalone_content/game.qsp";
		//curSaveDir = mainActivity.getFilesDir().getPath().concat(File.separator);

  //      //Создаем список для всплывающего меню
  //      menuList = new Vector<ContainerMenuItem>();

  //      //Создаем объект для таймера
  //      timerHandler = new Handler(Looper.getMainLooper());

  //      //Запускаем поток библиотеки
  //      StartLibThread();
	}

	// ********************************************************************
	// ********************************************************************
	// ********************************************************************
	//                       Колбэки интерпретатора
	// ********************************************************************
	// ********************************************************************
	// ********************************************************************

    static void RefreshInt(int isRedraw) 
    {
    	//Контекст библиотеки
  //      bool needUpdate = skin.isSomethingChanged();
  //      skin.updateBaseVars();
  //      needUpdate = needUpdate || skin.isSomethingChanged();
  //      skin.updateMainScreen();
  //      needUpdate = needUpdate || skin.isSomethingChanged();
  //      
		//JSONObject jsSkin = null;
  //      if (needUpdate)
  //          jsSkin = skin.getJsSkin();

  //      //основное описание
  //      String mainDesc = null;
  //      if ((QSPIsMainDescChanged() == true) || skin.isHtmlModeChanged)
  //      {
  //          mainDesc = skin.applyHtmlFixes(QSPGetMainDesc());
  //      }
  //  	
  //      //список действий
  //      JSONArray acts = null;
  //      if ((QSPIsActionsChanged() == true) || skin.isHtmlModeChanged)
  //      {
		//	int nActsCount = QSPGetActionsCount();
		//	acts = new JSONArray();
		//	for (int i = 0; i < nActsCount; i++)
		//	{
		//      	ContainerJniResult actsResult = (ContainerJniResult) QSPGetActionData(i);
		//		JSONObject act = new JSONObject();
		//		try {
		//			act.put("image", actsResult.str2);
		//			act.put("desc", skin.applyHtmlFixes(actsResult.str1));
		//			acts.put(i, act);
		//		} catch (JSONException e) {
		//    		Utility.WriteLog("ERROR - act or acts[] in RefreshInt!");
		//			e.printStackTrace();
		//		}
		//	}
		//}
  //      
  //      //инвентарь
  //      JSONArray objs = null;
  //      if ((QSPIsObjectsChanged() == true) || skin.isHtmlModeChanged)
  //      {
  //          int nObjsCount = QSPGetObjectsCount();
  //          int nSelectedObject = QSPGetSelObjectIndex();
  //          objs = new JSONArray();
  //          for (int i = 0; i < nObjsCount; i++)
  //          {
	 //       	ContainerJniResult objsResult = (ContainerJniResult) QSPGetObjectData(i);
  //              
		//		JSONObject obj = new JSONObject();
		//		try {
		//			obj.put("image", objsResult.str2);
		//			obj.put("desc", skin.applyHtmlFixes(objsResult.str1));
		//			obj.put("selected", (i == nSelectedObject)? 1 : 0);
		//			objs.put(i, obj);
		//		} catch (JSONException e) {
		//    		Utility.WriteLog("ERROR - obj or objs[] in RefreshInt!");
		//			e.printStackTrace();
		//		}
  //          }
  //      }
  //      
  //      //доп. описание
  //      String varsDesc = null;
  //      if ((QSPIsVarsDescChanged() == true) || skin.isHtmlModeChanged)
  //      {
  //          varsDesc = skin.applyHtmlFixes(QSPGetVarsDesc());
  //      }
  //      
  //      // Яваскрипт, переданный из игры командой EXEC('JS:...')
  //      String jsCmd = null;
  //      if (jsExecBuffer.length() > 0)
  //      {
  //      	jsCmd = jsExecBuffer;
  //      	jsExecBuffer = "";
  //      }
  //      
  //      // Передаем собранные данные в яваскрипт
  //      if ((jsSkin != null) || (mainDesc != null) || (acts != null) || (objs != null) || (varsDesc != null) ||
  //      	(jsCmd != null))
  //      {
  //      	JSONObject groupedContent = new JSONObject();
  //      	try {
	 //           if (jsSkin != null)
	 //           {
	 //               groupedContent.put("skin", jsSkin);
	 //           }
	 //           if (mainDesc != null)
	 //           {
	 //               groupedContent.put("main", mainDesc);
	 //           }
	 //           if (acts != null)
	 //           {
	 //               groupedContent.put("acts", acts);
	 //           }
	 //           if (varsDesc != null)
	 //           {
	 //               groupedContent.put("vars", varsDesc);
	 //           }
	 //           if (objs != null)
	 //           {
	 //               groupedContent.put("objs", objs);
	 //           }
	 //           if (jsCmd != null)
	 //           {
	 //               groupedContent.put("js", jsCmd);
	 //           }
		//	} catch (JSONException e) {
	 //   		Utility.WriteLog("ERROR - groupedContent in RefreshInt!");
		//		e.printStackTrace();
		//	}
  //      	final JSONObject groupedContentObject = groupedContent;
		//	mainActivity.runOnUiThread(new Runnable() {
		//		public void run() {
		//			jsSetGroupedContent(groupedContentObject);
		//		}
		//	});
  //      }
  //      skin.resetUpdate();
    }
    
    static void SetTimer(int msecs)
    {
  //  	//Контекст библиотеки
  //  	final int timeMsecs = msecs;
		//mainActivity.runOnUiThread(new Runnable() {
		//	public void run() {
		//		timerInterval = timeMsecs;
		//	}
		//});
    }

	static void ShowMessage(QSP_CHAR* message)
    {
  //  	//Контекст библиотеки
		//if (libThread==null)
		//{
		//	Utility.WriteLog("ShowMessage: failed, libThread is null");
		//	return;
		//}

	 //   // Обновляем скин
  //      skin.updateBaseVars();
  //      skin.updateMsgDialog();
  //      skin.updateEffects();
	 //   // Если что-то изменилось, то передаем в яваскрипт
	 //   if (skin.isSomethingChanged() && (skin.disableAutoRef != 1))
	 //   {
	 //       Utility.WriteLog("Hey! Skin was changed! Updating it before showing MSG.");
	 //       RefreshInt(QSP_TRUE);
	 //   }
		//
		//String msgValue = "";
		//if (message != null)
		//	msgValue = message;
		//
		//dialogHasResult = false;

  //  	final String msg = skin.applyHtmlFixes(msgValue);
		//mainActivity.runOnUiThread(new Runnable() {
		//	public void run() {
		//		jsQspMsg(msg);
		//		Utility.WriteLog("ShowMessage(UI): dialog showed");
		//	}
		//});
  //  	
		//Utility.WriteLog("ShowMessage: parking library thread");
  //      while (!dialogHasResult) {
  //      	setThreadPark();
  //      }
  //      parkThread = null;
		//Utility.WriteLog("ShowMessage: library thread unparked, finishing");
    }
    
    static void PlayFile(QSP_CHAR* file, int volume)
    {
  //  	//Контекст библиотеки
  //  	file = Utility.QspPathTranslate(file);
  //  	
  //  	if (file == null || file.length() == 0)
  //  	{
  //  		Utility.WriteLog("ERROR - filename is " + (file == null ? "null" : "empty"));
  //  		return;
  //  	}
  //  	
  //  	//Проверяем, проигрывается ли уже этот файл.
  //  	//Если проигрывается, ничего не делаем.
  //  	if (CheckPlayingFileSetVolume(file, true, volume))
  //  		return;

  //  	// Добавляем к имени файла полный путь
  //  	String prefix = "";
  //  	if (curGameDir != null)
  //  		prefix = curGameDir;
  //  	String assetFilePath = prefix.concat(file);

  //  	//Проверяем, существует ли файл.
		////Если нет, ничего не делаем.
  //      if (!Utility.CheckAssetExists(uiContext, assetFilePath, "PlayFile"))
  //      	return;

  //  	AssetManager assetManager = uiContext.getAssets();
  //  	if (assetManager == null)
  //  	{
  //  		Utility.WriteLog("PlayFile: failed, assetManager is null");
  //  		return;
  //  	}
  //  	AssetFileDescriptor afd = null;
  //  	try {
		//	afd = assetManager.openFd(assetFilePath);
		//} catch (IOException e) {
		//	e.printStackTrace();
		//	return;
		//}
  //  	if (afd == null)
  //  		return;
  //      
  //  	MediaPlayer mediaPlayer = new MediaPlayer();
	 //   try {
		//	mediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
		//} catch (IllegalArgumentException e) {
		//	e.printStackTrace();
		//	return;
		//} catch (IllegalStateException e) {
		//	e.printStackTrace();
		//	return;
		//} catch (IOException e) {
		//	e.printStackTrace();
		//	return;
		//}
	 //   try {
		//	mediaPlayer.prepare();
		//} catch (IllegalStateException e) {
		//	e.printStackTrace();
		//	return;
		//} catch (IOException e) {
		//	e.printStackTrace();
		//	return;
		//}
		//final String fileName = file;
		//mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
		//	@Override
		//	public void onCompletion(MediaPlayer mp) {
		//        musicLock.lock();
		//        try {
		//	    	for (int i=0; i<mediaPlayersList.size(); i++)
		//	    	{
		//	    		ContainerMusic it = mediaPlayersList.elementAt(i);    		
		//	    		if (it.path.equals(fileName))
		//	    		{
		//	    			it.player.release();
		//	    			it.player = null;
		//	    			mediaPlayersList.remove(it);
		//	    			break;
		//	    		}
		//	    	}
		//        } finally {
		//        	musicLock.unlock();
		//        }
		//	}
		//});
		//
  //      musicLock.lock();
  //      try {
		//	float realVolume = GetRealVolume(volume);
		//	mediaPlayer.setVolume(realVolume, realVolume);
		//    mediaPlayer.start();
		//    ContainerMusic ContainerMusic = new ContainerMusic();
		//    ContainerMusic.path = file;
		//    ContainerMusic.volume = volume;
		//    ContainerMusic.player = mediaPlayer;
  //      	mediaPlayersList.add(ContainerMusic);
  //      } finally {
  //      	musicLock.unlock();
  //      }
    }
    
	static QSP_BOOL IsPlayingFile(QSP_CHAR* file)
    {
    	////Контекст библиотеки
    	//return CheckPlayingFileSetVolume(Utility.QspPathTranslate(file), false, 0);

		return QSP_FALSE;
    }

    static void CloseFile(QSP_CHAR* file)
    {
    	////Контекст библиотеки
    	//file = Utility.QspPathTranslate(file);
    	//
    	////Если вместо имени файла пришел null, значит закрываем все файлы(CLOSE ALL)
    	//bool bCloseAll = false;
    	//if (file == null)
    	//	bCloseAll = true;
    	//else if (file.length() == 0)
    	//	return;
     //   musicLock.lock();
     //   try {
	    //	for (int i=0; i<mediaPlayersList.size(); i++)
	    //	{
	    //		ContainerMusic it = mediaPlayersList.elementAt(i);    		
	    //		if (bCloseAll || it.path.equals(file))
	    //		{
	    //			if (it.player.isPlaying())
	    //				it.player.stop();
	    //			it.player.release();
	    //			it.player = null;
	    //			if (!bCloseAll)
	    //			{
	    //				mediaPlayersList.remove(it);
	    //				break;
	    //			}
	    //		}
	    //	}
    	//	if (bCloseAll)
    	//		mediaPlayersList.clear();
     //   } finally {
     //   	musicLock.unlock();
     //   }
    }
    
    static void ShowPicture(QSP_CHAR* file)
    {
  //  	//Контекст библиотеки
  //  	if (file == null)
  //  		file = "";
  //  	
  //  	final String fileName = Utility.QspPathTranslate(file);
  //  	
		//mainActivity.runOnUiThread(new Runnable() {
		//	public void run() {
		//		if (fileName.length() > 0)
		//		{
		//	    	String prefix = "";
		//	    	if (curGameDir != null)
		//	    		prefix = curGameDir;
		//	    	
		//	    	//Проверяем, существует ли файл.
		//	    	//Если нет - выходим
		//	        if (!Utility.CheckAssetExists(uiContext, prefix.concat(fileName), "ShowPicture"))
		//	        	return;
		//		}
		//        // "Пустое" имя файла тоже имеет значение - так мы скрываем картинку
		//        jsQspView(fileName);
		//	}
		//});    	    	
    }
    
    static void InputBox(const QSP_CHAR* prompt, QSP_CHAR* buffer, int maxLen)
    {
  //  	//Контекст библиотеки
		//if (libThread==null)
		//{
		//	Utility.WriteLog("InputBox: failed, libThread is null");
		//	return "";
		//}

	 //   // Обновляем скин
	 //   skin.updateBaseVars();
	 //   skin.updateInputDialog();
	 //   skin.updateEffects();
	 //   // Если что-то изменилось, то передаем в яваскрипт
	 //   if (skin.isSomethingChanged() && (skin.disableAutoRef != 1))
	 //   {
	 //       Utility.WriteLog("Hey! Skin was changed! Updating it before showing INPUT.");
	 //       RefreshInt(QSP_TRUE);
	 //   }
		//
		//String promptValue = "";
		//if (prompt != null)
		//	promptValue = prompt;
		//
		//dialogHasResult = false;
		//
  //  	final String inputboxTitle = skin.applyHtmlFixes(promptValue);
		//mainActivity.runOnUiThread(new Runnable() {
		//	public void run() {
		//		inputboxResult = "";
		//		jsQspInput(inputboxTitle);
		//		Utility.WriteLog("InputBox(UI): dialog showed");
		//	}
		//});
  //  	
		//Utility.WriteLog("InputBox: parking library thread");
  //      while (!dialogHasResult) {
  //      	setThreadPark();
  //      }
  //      parkThread = null;
		//Utility.WriteLog("InputBox: library thread unparked, finishing");
  //  	return inputboxResult;
		
		
		//wcsncpy(buffer, dialog.GetText().c_str(), maxLen);
    }
    
    /**
     * Функция запросов к плееру.
     * С помощью этой функции мы можем в игре узнать параметры окружения плеера.
     * Вызывается так: $platform = GETPLAYER('platform')
     * @param resource
     * @return
     */
    static void PlayerInfo(QSP_CHAR* resource, QSP_CHAR* buffer, int maxLen)
    {
    	////Контекст библиотеки
    	//resource = resource.toLowerCase();
    	//if (resource.equals("platform")) {
    	//	return "Android";
    	//} else if (resource.equals("player")) {
    	//	return "Quest Navigator";
    	//} else if (resource.equals("player.version")) {
    	//	return "1.0.0";
    	//}
    	//return "";


		//wcsncpy(buffer, dialog.GetText().c_str(), maxLen);
    }
    
    static int GetMSCount()
    {
    	////Контекст библиотеки
    	//return (int) (System.currentTimeMillis() - gameStartTime);
    }
    
    static void AddMenuItem(QSP_CHAR* name, QSP_CHAR* imgPath)
    {
    	////Контекст библиотеки
    	//ContainerMenuItem item = new ContainerMenuItem();
    	//item.imgPath = Utility.QspPathTranslate(imgPath);
    	//item.name = name;
    	//menuList.add(item);
    }
    
    static int ShowMenu()
    {
  //  	//Контекст библиотеки
		//if (libThread==null)
		//{
		//	Utility.WriteLog("ShowMenu: failed, libThread is null");
		//	return -1;
		//}

	 //   // Обновляем скин
	 //   skin.updateMenuDialog();
	 //   skin.updateEffects();
	 //   // Если что-то изменилось, то передаем в яваскрипт
	 //   if (skin.isSomethingChanged() && (skin.disableAutoRef != 1))
	 //   {
	 //       Utility.WriteLog("Hey! Skin was changed! Updating it before showing user menu.");
	 //       RefreshInt(QSP_TRUE);
	 //   }
		//
		//dialogHasResult = false;
		//menuResult = -1;

		//final Vector<ContainerMenuItem> uiMenuList = menuList; 
  //  	
		//mainActivity.runOnUiThread(new Runnable() {
		//	public void run() {
		//        JSONArray jsonMenuList = new JSONArray();
		//		for (int i = 0; i < uiMenuList.size(); i++)
		//		{
		//			JSONObject jsonMenuItem = new JSONObject();
		//			try {
		//				jsonMenuItem.put("image", uiMenuList.elementAt(i).imgPath);
		//				jsonMenuItem.put("desc", skin.applyHtmlFixes(uiMenuList.elementAt(i).name));
		//				jsonMenuList.put(i, jsonMenuItem);
		//			} catch (JSONException e) {
		//	    		Utility.WriteLog("ERROR - jsonMenuItem or jsonMenuList[] in ShowMenu!");
		//				e.printStackTrace();
		//			}
		//		}
		//		jsQspMenu(jsonMenuList);
		//	    Utility.WriteLog("ShowMenu(UI): dialog showed");
		//	}
		//});
  //  	
		//Utility.WriteLog("ShowMenu: parking library thread");
  //      while (!dialogHasResult) {
  //      	setThreadPark();
  //      }
  //      parkThread = null;
		//Utility.WriteLog("ShowMenu: library thread unparked, finishing");
  //  	
		//return menuResult;
    }
    
    static void DeleteMenu()
    {
    	////Контекст библиотеки
    	//menuList.clear();
    }
    
    static void Wait(int msecs)
    {
  //  	//Контекст библиотеки
  //  	try {
		//	Thread.sleep(msecs);
		//} catch (InterruptedException e) {
		//	Utility.WriteLog("WAIT in library thread was interrupted");
		//	e.printStackTrace();
		//}
    }
    
	static void ShowWindow(int type, QSP_BOOL isShow)
    {
    	//// Контекст библиотеки
    	//skin.showWindow(type, isShow);
    }
    
    static void System(QSP_CHAR* cmd)
    {
    	////Контекст библиотеки
    	//if (cmd == null)
    	//	cmd = "";
    	//
    	//if (cmd.toUpperCase().startsWith("JS:"))
    	//{
     //   	// Выполняем яваскрипт, переданный из игры командой EXEC('JS:...')
    	//	final String jsCommand = cmd.substring("JS:".length());
    	//	mainActivity.runOnUiThread(new Runnable() {
    	//		public void run() {
    	//			execJS(jsCommand);
    	//		}
    	//	});
    	//}
    }

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
