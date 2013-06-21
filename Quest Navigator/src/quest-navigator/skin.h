#ifndef SKIN_H_
#define SKIN_H_

#include <string>
#include "configuration.h"

using namespace std;

namespace QuestNavigator {

	// Скин используется только в контексте библиотеки.

	// В этом классе хранятся настройки оформления
	enum eSkinParam
	{
		// Вопросами помечны кандидаты на выпиливание.
		// Сейчас они будут передаваться в JS с дефолтными значениями,
		// в дальнейшем от них нужно будет избавиться.

		//hideScrollAny ? 
		//hideScrollArrows ?
		//hideScrollMain ?
		//hideScrollActs ? 
		//hideScrollVars ?
		//hideScrollObjs ?

		//useHtml
		//showActs
		//showVars
		//showObjs

		//msgTextFormat
		//disableScroll
		//menuListItemFormat
		//inputTextFormat
		//viewAlwaysShow
		//mainDescTextFormat
		//varsDescTextFormat
		//actsListItemFormat
		//objsListSelItemFormat
		//objsListItemFormat

		//backColor ? 
		//menuBorder ? 
		//menuBorderColor ? 
		//linkColor ? 
		//fontColor ?
		//fontName ?
		//fontSize ?

		espHideScrollAny = 0,
		espHideScrollArrows,
		espHideScrollMain,
		espHideScrollActs,
		espHideScrollVars,
		espHideScrollObjs,

		espUseHtml,
		espShowActs,
		espShowVars,
		espShowObjs,

		espMsgTextFormat,
		espDisableScroll,
		espMenuListItemFormat,
		espInputTextFormat,
		espViewAlwaysShow,
		espMainDescTextFormat,
		espVarsDescTextFormat,
		espActsListItemFormat,
		espObjsListSelItemFormat,
		espObjsListItemFormat,

		espBackColor,
		espMenuBorder,
		espMenuBorderColor,

		espLinkColor,
		espFontColor,
		espFontName,
		espFontSize,

		espLast
	};

	class Skin {
	private:
		// Хранилище параметров
		static ConfigValue _paramList[espLast];

		static bool firstUpdate;
	public:
		static bool isChanged;
		static bool isMainBackChanged;
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
		static void resetSettings();
		static string getNewStrValue(string curVal, string varName, string defValue);
		static int getNewNumValue(int curVal, string varName, int defValue);

	public:
		static string getString(eSkinParam param);
		static void setString(eSkinParam param, string value);
		static void resetUpdate();
	};


}





#endif
