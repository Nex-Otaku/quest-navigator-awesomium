#ifndef SKIN_H_
#define SKIN_H_

#include <string>
#include "configuration.h"
#include <Awesomium/WebCore.h>

using namespace std;
using namespace Awesomium;

namespace QuestNavigator {

	// Скин используется только в контексте библиотеки.

	// В этом классе хранятся настройки оформления
	enum eSkinParam
	{
		// Вопросами помечены кандидаты на выпиливание.
		// Сейчас они будут передаваться в JS с дефолтными значениями,
		// в дальнейшем от них нужно будет избавиться.

		//hideScrollAny ? 
		//hideScrollArrows ?
		//hideScrollMain ?
		//hideScrollActs ? 
		//hideScrollVars ?
		//hideScrollObjs ?

		//useHtml
		//disableScroll
		//viewAlwaysShow
		//isStandalone

		//showActs
		//showVars
		//showObjs
		//showInput

		//msgTextFormat
		//inputTextFormat
		//mainDescTextFormat
		//varsDescTextFormat

		espHideScrollAny = 0,
		espHideScrollArrows,
		espHideScrollMain,
		espHideScrollActs,
		espHideScrollVars,
		espHideScrollObjs,

		espUseHtml,
		espNoSave,
		espDisableScroll,
		espViewAlwaysShow,
		espIsStandalone,

		espShowActs,
		espShowVars,
		espShowObjs,
		espShowInput,

		espMsgTextFormat,
		espInputTextFormat,
		espMainDescTextFormat,
		espVarsDescTextFormat,

		espLast
	};

	class Skin {
	private:
		// Хранилище параметров
		static ConfigValue _paramList[espLast];
		// Значения параметров по умолчанию
		static ConfigValue _defaultList[espLast];

		static bool firstUpdate;
	public:
		static bool isChanged;
		static bool isActionsStyleChanged;
		static bool isHtmlModeChanged;
		static bool isBaseVarsChanged;
		static bool isSoftBaseVarsChanged;
		static bool isMainDescChanged;
		static bool isSoftMainDescChanged;
		static bool isVarsDescChanged;
		static bool isSoftVarsDescChanged;
		static bool isActsListChanged;
		static bool isSoftActsListChanged;
		static bool isObjsListChanged;
		static bool isSoftObjsListChanged;
		static bool isViewChanged;
		static bool isUserInputChanged;


	private:
		static string getNewStrValue(string curVal, string varName, string defValue);
		static int getNewNumValue(int curVal, string varName, int defValue);
		static string getDefaultString(eSkinParam param);
		static void setDefaultString(eSkinParam param, string value);
		static int getDefaultInt(eSkinParam param);
		static void setDefaultInt(eSkinParam param, int value);
		static void loadValue(eSkinParam param, string name);
	public:
		static string getString(eSkinParam param);
		static void setString(eSkinParam param, string value);
		static int getInt(eSkinParam param);
		static void setInt(eSkinParam param, int value);
		static eConfigValueType getType(eSkinParam param);
		static void initDefaults();
		static void resetSettings();
		static void resetUpdate();
		static void updateBaseVars();
		static void updateMainScreen();
		static void updateInputDialog();
		static void updateMsgDialog();
		static JSObject getJsSkin();
		static bool isSomethingChanged();
		static void showWindow(int type, int isShow);
		static string applyHtmlFixes(string text, bool forceHtml = false);
	};


}





#endif
