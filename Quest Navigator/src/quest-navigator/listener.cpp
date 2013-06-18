#include "listener.h"

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

QnApplicationListener::QnApplicationListener() 
	: app_(Application::Create()),
	view_(0),
	data_source_(0),

	gameIsRunning(false),
	qspInited(false),

	libThread(NULL)
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
void QnApplicationListener::OnUpdate() {
}

// Inherited from Application::Listener
void QnApplicationListener::OnShutdown() {
	FreeResources();
}

void QnApplicationListener::BindMethods(WebView* web_view) {
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
void QnApplicationListener::OnSayHello(WebView* caller,
									   const JSArray& args) {
										   app_->ShowMessage("Hello!");
}

// Bound to App.Exit() in JavaScript
void QnApplicationListener::OnExit(WebView* caller,
								   const JSArray& args) {
									   app_->Quit();
}

// Bound to App.GetSecretMessage() in JavaScript
JSValue QnApplicationListener::OnGetSecretMessage(WebView* caller,
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

void QnApplicationListener::initLib()
{
	// Привязываем колбэки
	QSPSetCallBack(QSP_CALL_REFRESHINT, (QSP_CALLBACK)&RefreshInt);
	//QSPSetCallBack(QSP_CALL_SETTIMER, (QSP_CALLBACK)&SetTimer);
	//QSPSetCallBack(QSP_CALL_SETINPUTSTRTEXT, (QSP_CALLBACK)&SetInputStrText);
	//QSPSetCallBack(QSP_CALL_ISPLAYINGFILE, (QSP_CALLBACK)&IsPlay);
	//QSPSetCallBack(QSP_CALL_PLAYFILE, (QSP_CALLBACK)&PlayFile);
	//QSPSetCallBack(QSP_CALL_CLOSEFILE, (QSP_CALLBACK)&CloseFile);
	//QSPSetCallBack(QSP_CALL_SHOWMSGSTR, (QSP_CALLBACK)&Msg);
	//QSPSetCallBack(QSP_CALL_SLEEP, (QSP_CALLBACK)&Sleep);
	//QSPSetCallBack(QSP_CALL_GETMSCOUNT, (QSP_CALLBACK)&GetMSCount);
	//QSPSetCallBack(QSP_CALL_DELETEMENU, (QSP_CALLBACK)&DeleteMenu);
	//QSPSetCallBack(QSP_CALL_ADDMENUITEM, (QSP_CALLBACK)&AddMenuItem);
	//QSPSetCallBack(QSP_CALL_SHOWMENU, (QSP_CALLBACK)&ShowMenu);
	//QSPSetCallBack(QSP_CALL_INPUTBOX, (QSP_CALLBACK)&Input);
	//QSPSetCallBack(QSP_CALL_SHOWIMAGE, (QSP_CALLBACK)&ShowImage);
	//QSPSetCallBack(QSP_CALL_SHOWWINDOW, (QSP_CALLBACK)&ShowPane);
	//QSPSetCallBack(QSP_CALL_OPENGAMESTATUS, (QSP_CALLBACK)&OpenGameStatus);
	//QSPSetCallBack(QSP_CALL_SAVEGAMESTATUS, (QSP_CALLBACK)&SaveGameStatus);


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

	//Запускаем поток библиотеки
	StartLibThread();
}

void QnApplicationListener::FreeResources()
{
	//Контекст UI

	//Процедура "честного" высвобождения всех ресурсов - в т.ч. остановка потока библиотеки

	//Очищаем ВСЕ на выходе
	if (qspInited)
	{
//		Utility.WriteLog("onDestroy: stopping game");
		StopGame(false);
	}
	//Останавливаем поток библиотеки
	StopLibThread();
}

void QnApplicationListener::runGame(string fileName)
{
	////Контекст UI
	//if (!Utility.CheckAssetExists(uiContext, fileName, "runGame"))
	//	return;

	//if (libThreadHandler == null)
	//{
	//	Utility.WriteLog("runGame: failed, libThreadHandler is null");
	//	return;
	//}

	//if (libraryThreadIsRunning)
	//{
	//	Utility.WriteLog("runGame: failed, library thread is already running");
	//	return;
	//}

	//final bool inited = qspInited;
	//qspInited = true;
	//jsExecBuffer = "";
	//final String gameFileName = fileName;
	//curGameFile = gameFileName;
	//curGameDir = gameFileName.substring(0, gameFileName.lastIndexOf(File.separator, gameFileName.length() - 1) + 1);

	//libThreadHandler.post(new Runnable() {
	//	public void run() {
	//		InputStream fIn = null;
	//		int size = 0;

	//		AssetManager assetManager = uiContext.getAssets();
	//		if (assetManager == null)
	//		{
	//			Utility.WriteLog("runGame(in library thread): failed, assetManager is null");
	//			return;
	//		}
	//		try {
	//			fIn = assetManager.open(gameFileName);
	//			size = fIn.available();
	//		} catch (Exception e) {
	//			e.printStackTrace();
	//			Utility.ShowError(uiContext, "Не удалось получить доступ к файлу");
	//			try {
	//				fIn.close();
	//			} catch (IOException e1) {
	//				Utility.ShowError(uiContext, "Не удалось освободить дескриптор файла");
	//				e1.printStackTrace();
	//			}
	//			return;
	//		}

	//		byte[] inputBuffer = new byte[size];
	//		try {
	//			// Fill the Buffer with data from the file
	//			fIn.read(inputBuffer);
	//			fIn.close();
	//		} catch (IOException e) {
	//			e.printStackTrace();
	//			Utility.ShowError(uiContext, "Не удалось прочесть файл");
	//			return;
	//		}

	//		if (!inited)
	//			QSPInit();
	//		final bool gameLoaded = QSPLoadGameWorldFromData(inputBuffer, size, gameFileName);
	//		CheckQspResult(gameLoaded, "runGame: QSPLoadGameWorldFromData");

	//		if (gameLoaded)
	//		{
	//			mainActivity.runOnUiThread(new Runnable() {
	//				public void run() {
	//					//Запускаем таймер
	//					timerInterval = 500;
	//					timerStartTime = System.currentTimeMillis();
	//					timerHandler.removeCallbacks(timerUpdateTask);
	//					timerHandler.postDelayed(timerUpdateTask, timerInterval);

	//					//Запускаем счетчик миллисекунд
	//					gameStartTime = System.currentTimeMillis();

	//					//Все готово, запускаем игру
	//					libThreadHandler.post(new Runnable() {
	//						public void run() {
	//							libraryThreadIsRunning = true;
	//							bool result = QSPRestartGame(true);
	//							CheckQspResult(result, "runGame: QSPRestartGame");
	//							libraryThreadIsRunning = false;
	//						}
	//					});

	//					gameIsRunning = true;
	//				}
	//			});
	//		}
	//	}
	//});
}

void QnApplicationListener::StopGame(bool restart)
{
	////Контекст UI
	//if (gameIsRunning)
	//{
	//	//останавливаем таймер
	//	timerHandler.removeCallbacks(timerUpdateTask);

	//	//останавливаем музыку
	//	libThreadHandler.post(new Runnable() {
	//		public void run() {
	//			CloseFile(null);
	//		}
	//	});

	//	gameIsRunning = false;
	//}
	//curGameDir = "";
	//curGameFile = "";
	//jsExecBuffer = "";

	////Очищаем библиотеку
	//if (restart || libraryThreadIsRunning)
	//	return;

	//qspInited = false;
	//libThreadHandler.post(new Runnable() {
	//	public void run() {
	//		libraryThreadIsRunning = true;
	//		QSPDeInit();
	//		libraryThreadIsRunning = false;
	//	}
	//});
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

void QnApplicationListener::SetTimer(int msecs)
{
	//  	//Контекст библиотеки
	//  	final int timeMsecs = msecs;
	//mainActivity.runOnUiThread(new Runnable() {
	//	public void run() {
	//		timerInterval = timeMsecs;
	//	}
	//});
}

void QnApplicationListener::ShowMessage(QSP_CHAR* message)
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

void QnApplicationListener::PlayFile(QSP_CHAR* file, int volume)
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

QSP_BOOL QnApplicationListener::IsPlayingFile(QSP_CHAR* file)
{
	////Контекст библиотеки
	//return CheckPlayingFileSetVolume(Utility.QspPathTranslate(file), false, 0);

	return QSP_FALSE;
}

void QnApplicationListener::CloseFile(QSP_CHAR* file)
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

void QnApplicationListener::ShowPicture(QSP_CHAR* file)
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

void QnApplicationListener::InputBox(const QSP_CHAR* prompt, QSP_CHAR* buffer, int maxLen)
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


// Функция запросов к плееру.
// С помощью этой функции мы можем в игре узнать параметры окружения плеера.
// Вызывается так: $platform = GETPLAYER('platform')
// @param resource
// @return

void QnApplicationListener::PlayerInfo(QSP_CHAR* resource, QSP_CHAR* buffer, int maxLen)
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

int QnApplicationListener::GetMSCount()
{
	////Контекст библиотеки
	//return (int) (System.currentTimeMillis() - gameStartTime);

	return 0;
}

void QnApplicationListener::AddMenuItem(QSP_CHAR* name, QSP_CHAR* imgPath)
{
	////Контекст библиотеки
	//ContainerMenuItem item = new ContainerMenuItem();
	//item.imgPath = Utility.QspPathTranslate(imgPath);
	//item.name = name;
	//menuList.add(item);
}

int QnApplicationListener::ShowMenu()
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

	return 0;
}

void QnApplicationListener::DeleteMenu()
{
	////Контекст библиотеки
	//menuList.clear();
}

void QnApplicationListener::Wait(int msecs)
{
	//  	//Контекст библиотеки
	//  	try {
	//	Thread.sleep(msecs);
	//} catch (InterruptedException e) {
	//	Utility.WriteLog("WAIT in library thread was interrupted");
	//	e.printStackTrace();
	//}
}

void QnApplicationListener::ShowWindow(int type, QSP_BOOL isShow)
{
	//// Контекст библиотеки
	//skin.showWindow(type, isShow);
}

void QnApplicationListener::System(QSP_CHAR* cmd)
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

void QnApplicationListener::jsCallApi(string name, JSValue arg)
{
	JSValue window = view_->web_view()->ExecuteJavascriptWithResult(
		WSLit("window"), WSLit(""));
	if (window.IsObject()) {
		JSArray args;
		args.Push(arg);
		window.ToObject().Invoke(ToWebString(name), args);
	}
}
void QnApplicationListener::qspSetGroupedContent(JSObject content)
{
	jsCallApi("qspSetGroupedContent", content);
}
void QnApplicationListener::qspShowSaveSlotsDialog(JSObject content)
{
	jsCallApi("qspShowSaveSlotsDialog", content);
}
void QnApplicationListener::qspMsg(WebString text)
{
	jsCallApi("qspMsg", text);
}
void QnApplicationListener::qspError(WebString error)
{
	jsCallApi("qspError", error);
}
void QnApplicationListener::qspMenu(JSObject menu)
{
	jsCallApi("qspMenu", menu);
}
void QnApplicationListener::qspInput(WebString text)
{
	jsCallApi("qspInput", text);
}
void QnApplicationListener::qspView(WebString path)
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

void QnApplicationListener::restartGame(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("restarted!");
}

void QnApplicationListener::executeAction(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("action pressed!");
}

void QnApplicationListener::selectObject(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("object selected!");
}

void QnApplicationListener::loadGame(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("game load dialog need to be opened!");
}

void QnApplicationListener::saveGame(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("game save dialog need to be opened!");
}

void QnApplicationListener::saveSlotSelected(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("slot selected!");
}

void QnApplicationListener::msgResult(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("msg dialog closed!");
}

void QnApplicationListener::errorResult(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("error dialog closed!");
}

void QnApplicationListener::userMenuResult(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("user menu dialog closed!");
}

void QnApplicationListener::inputResult(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("input dialog closed!!");
}

void QnApplicationListener::setMute(WebView* caller, const JSArray& args)
{
	app_->ShowMessage("mute called!");
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

//******************************************************************************
//******************************************************************************
//****** / THREADS \ ***********************************************************
//******************************************************************************
//******************************************************************************
// паркует-останавливает указанный тред, и сохраняет на него указатель в parkThread
void QnApplicationListener::setThreadPark()
{

	//Utility.WriteLog("setThreadPark: enter ");    	
	////Контекст библиотеки
	//if (libThread == null)
	//{
	//	Utility.WriteLog("setThreadPark: failed, libthread is null");
	//	return;
	//}
	//parkThread = libThread;
	//LockSupport.park();
	//Utility.WriteLog("setThreadPark: success ");    	
}

// возобновляет работу треда сохраненного в указателе parkThread
bool QnApplicationListener::setThreadUnpark()
{
	//Utility.WriteLog("setThreadUnPark: enter ");
	////Контекст UI
	//if (parkThread!=null && parkThread.isAlive()) {
	//	LockSupport.unpark(parkThread);
	//	Utility.WriteLog("setThreadUnPark: success ");    	
	//	return true;
	//}
	//Utility.WriteLog("setThreadUnPark: failed, ");
	//if (parkThread==null)
	//	Utility.WriteLog("parkThread is null ");
	//else
	//	Utility.WriteLog("parkThread is dead ");
	//return false;

	return false;
}

// Запуск потока библиотеки
void QnApplicationListener::StartLibThread()
{
	//Utility.WriteLog("StartLibThread: enter ");    	
	//Контекст UI
	if (libThread != NULL)
	{
//		Utility.WriteLog("StartLibThread: failed, libThread is not null");    	
		return;
	}

//HANDLE WINAPI CreateThread(
//  _In_opt_   LPSECURITY_ATTRIBUTES lpThreadAttributes,
//  _In_       SIZE_T dwStackSize,
//  _In_       LPTHREAD_START_ROUTINE lpStartAddress,
//  _In_opt_   LPVOID lpParameter,
//  _In_       DWORD dwCreationFlags,
//  _Out_opt_  LPDWORD lpThreadId
//);

	libThread = CreateThread(NULL, 0, &QnApplicationListener::libThreadFunc, this, 0, NULL);
	if (libThread == NULL) {
		showError("Не получилось создать поток интерпретатора.");
		return;
	}


	//final QspLib pluginObject = this; 

	////Запускаем поток библиотеки
	//Thread t = new Thread() {
	//	public void run() {
	//		Looper.prepare();
	//		libThreadHandler = new Handler();
	//		Utility.WriteLog("LibThread runnable: libThreadHandler is set");    	

	//		libThreadHandler.post(new Runnable() {
	//			public void run() {
	//				// Создаем скин
	//				skin = new QspSkin(pluginObject);

	//				//Создаем список для звуков и музыки
	//				musicLock.lock();
	//				try {
	//					mediaPlayersList = new Vector<ContainerMusic>();
	//					muted = false;
	//				} finally {
	//					musicLock.unlock();
	//				}

	//				// Сообщаем яваскрипту, что библиотека запущена
	//				// и можно продолжить инициализацию
	//				mainActivity.runOnUiThread(new Runnable() {
	//					public void run() {
	//						jsInitNext();
	//					}
	//				});
	//			}
	//		});

	//		Looper.loop();
	//		Utility.WriteLog("LibThread runnable: library thread exited");
	//	}
	//};
	//libThread = t;
	//t.start();
	//Utility.WriteLog("StartLibThread: success");
}

// Остановка потока библиотеки
void QnApplicationListener::StopLibThread()
{
	//Utility.WriteLog("StopLibThread: enter");    	
	////Контекст UI
	////Останавливаем поток библиотеки
	//libThreadHandler.getLooper().quit();

	//STUB
	// Сделать правильное завершение потока

	CloseHandle(libThread);
	libThread = NULL;
	//Utility.WriteLog("StopLibThread: success");    	
}

// Основная функция потока библиотеки
DWORD WINAPI QnApplicationListener::libThreadFunc(LPVOID pvParam)
{
	DWORD dwResult = 0;
	// STUB
	return dwResult;
}
//******************************************************************************
//******************************************************************************
//****** \ THREADS / ***********************************************************
//******************************************************************************
//******************************************************************************
