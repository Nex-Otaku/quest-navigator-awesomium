#include "skin.h"
#include "../deps/qsp/qsp.h"
#include "../deps/qsp/bindings/default/qsp_default.h"
#include "utils.h"
#include <Awesomium/STLHelpers.h>

namespace QuestNavigator {

	// Имена, под которыми параметры будут использоваться в JS
	const char *jsParamNames[] = {
		"hideScrollAny",
		"hideScrollArrows",
		"hideScrollMain",
		"hideScrollActs",
		"hideScrollVars",
		"hideScrollObjs",

		"useHtml",
		"noSave",
		"disableScroll",
		"viewAlwaysShow",
		"isStandalone",

		"showActs",
		"showVars",
		"showObjs",
		"showInput",

		"msgTextFormat",
		"inputTextFormat",
		"mainDescTextFormat",
		"varsDescTextFormat",
	};

	ConfigValue Skin::_paramList[espLast];
	ConfigValue Skin::_defaultList[espLast];

	string Skin::getString(eSkinParam param)
	{
		return _paramList[param].getString();
	}
	void Skin::setString(eSkinParam param, string value)
	{
		_paramList[param].setString(value);
	}
	int Skin::getInt(eSkinParam param)
	{
		return _paramList[param].getInt();
	}
	void Skin::setInt(eSkinParam param, int value)
	{
		_paramList[param].setInt(value);
	}
	string Skin::getDefaultString(eSkinParam param)
	{
		return _defaultList[param].getString();
	}
	void Skin::setDefaultString(eSkinParam param, string value)
	{
		_defaultList[param].setString(value);
	}
	int Skin::getDefaultInt(eSkinParam param)
	{
		return _defaultList[param].getInt();
	}
	void Skin::setDefaultInt(eSkinParam param, int value)
	{
		_defaultList[param].setInt(value);
	}
	eConfigValueType Skin::getType(eSkinParam param)
	{
		return _defaultList[param].getType();
	}


	bool Skin::firstUpdate;
	bool Skin::isChanged;
	bool Skin::isActionsStyleChanged;
	bool Skin::isHtmlModeChanged;
	bool Skin::isBaseVarsChanged;
	bool Skin::isSoftBaseVarsChanged;
	bool Skin::isMainDescChanged;
	bool Skin::isSoftMainDescChanged;
	bool Skin::isVarsDescChanged;
	bool Skin::isSoftVarsDescChanged;
	bool Skin::isActsListChanged;
	bool Skin::isSoftActsListChanged;
	bool Skin::isObjsListChanged;
	bool Skin::isSoftObjsListChanged;
	bool Skin::isViewChanged;
	bool Skin::isUserInputChanged;

	// Заполняем значения по умолчанию
	void Skin::initDefaults()
	{
		setDefaultInt(espHideScrollAny, 0);
		setDefaultInt(espHideScrollArrows, 0);
		setDefaultInt(espHideScrollMain, 0);
		setDefaultInt(espHideScrollActs, 0);
		setDefaultInt(espHideScrollVars, 0);
		setDefaultInt(espHideScrollObjs, 0);

		setDefaultInt(espUseHtml, 0);
		setDefaultInt(espNoSave, 0);
		setDefaultInt(espDisableScroll, 0);
		setDefaultInt(espViewAlwaysShow, 0);
		setDefaultInt(espIsStandalone, 0);

		setDefaultInt(espShowActs, 1);
		setDefaultInt(espShowVars, 1);
		setDefaultInt(espShowObjs, 1);
		setDefaultInt(espShowInput, 1);

		setDefaultString(espMsgTextFormat, "%TEXT%");
		setDefaultString(espInputTextFormat, "%TEXT%");
		setDefaultString(espMainDescTextFormat, "%TEXT%");
		setDefaultString(espVarsDescTextFormat, "%TEXT%");
	}

	// Устанавливаем значения по умолчанию
	void Skin::resetSettings()
	{
		for (int i = 0; i < espLast; i++) {
			eSkinParam eIterator = (eSkinParam)i;
			if (getType(eIterator) == ecvString) {
				setString(eIterator, getDefaultString(eIterator));
			} else if (getType(eIterator) == ecvInt) {
				setInt(eIterator, getDefaultInt(eIterator));
			} else {
				showError("Не инициализировано значение");
			}
		}

		firstUpdate = true;
	}

	// Сбрасываем флаги обновления
	void Skin::resetUpdate()
	{
		firstUpdate = false;
		isChanged = false;
		isActionsStyleChanged = false;
		isHtmlModeChanged = false;
		isBaseVarsChanged = false;
		isSoftBaseVarsChanged = false;
		isMainDescChanged = false;
		isSoftMainDescChanged = false;
		isVarsDescChanged = false;
		isSoftVarsDescChanged = false;
		isActsListChanged = false;
		isSoftActsListChanged = false;
		isObjsListChanged = false;
		isSoftObjsListChanged = false;
		isViewChanged = false;
		isUserInputChanged = false;
	}

	string Skin::getNewStrValue(string curVal, string varName, string defValue)
	{
		string val = "";
		int numVal;
		QSP_CHAR* pStrVal;
		QSP_BOOL res = QSPGetVarValues(widen(varName).c_str(), 0, &numVal, &pStrVal);
		if (res == QSP_FALSE)
		{
			//Utility.WriteLog("ERROR: getNewStrValue failed for: " + varName);
			return defValue;
		}
		val = narrow(wstring(pStrVal));
		if (val != curVal)
		{
			isChanged = true;
			return val;
		}
		return curVal;
	}

	int Skin::getNewNumValue(int curVal, string varName, int defValue)
	{
		int val = 0;
		int numVal;
		QSP_CHAR* pStrVal;
		QSP_BOOL res = QSPGetVarValues(widen(varName).c_str(), 0, &numVal, &pStrVal);
		if (res == QSP_FALSE)
		{
			//Utility.WriteLog("ERROR: getNewStrValue failed for: " + varName);
			return defValue;
		}
		val = numVal;
		if (val != curVal)
		{
			isChanged = true;
			return val;
		}
		return curVal;
	}

	// Загружаем переменную из библиотеки
	void Skin::loadValue(eSkinParam param, string name)
	{
		if (getType(param) == ecvString) {
			setString(param, getNewStrValue(getString(param), name, getDefaultString(param)));
		} else if (getType(param) == ecvInt) {
			setInt(param, getNewNumValue(getInt(param), name, getDefaultInt(param)));
		} else {
			showError("Не инициализировано значение");
		}
	}

	void Skin::updateBaseVars()
	{
		// Forced
		isChanged = false;
		loadValue(espHideScrollMain, "HIDE_MAIN_SCROLL");
		loadValue(espHideScrollActs, "HIDE_ACTS_SCROLL");
		loadValue(espHideScrollVars, "HIDE_STAT_SCROLL");
		loadValue(espHideScrollObjs, "HIDE_OBJS_SCROLL");
		loadValue(espHideScrollAny, "HIDE_ALL_SCROLL");
		loadValue(espHideScrollArrows, "HIDE_SCROLL_ARROWS");
		loadValue(espNoSave, "NOSAVE");
		if (isChanged) isBaseVarsChanged = true;
		// -----------
		isChanged = false;
		loadValue(espUseHtml, "USEHTML");
		if (isChanged) isHtmlModeChanged = true;
		// Others
		loadValue(espDisableScroll, "DISABLESCROLL");
	}

	void Skin::updateMainScreen()
	{
		// Forced
		isChanged = false;
		loadValue(espMainDescTextFormat, "MAIN_FORMAT");
		if (isChanged) isMainDescChanged = true;
		// ----------------------
		// Forced
		isChanged = false;
		loadValue(espVarsDescTextFormat, "STAT_FORMAT");
		if (isChanged) isVarsDescChanged = true;
		// Others
		loadValue(espViewAlwaysShow, "ALWAYS_SHOW_VIEW");
	}

	void Skin::updateInputDialog()
	{
		loadValue(espInputTextFormat, "INPUT_FORMAT");
	}

	void Skin::updateMsgDialog()
	{
		loadValue(espMsgTextFormat, "MSG_FORMAT");
	}

	JSObject Skin::getJsSkin()
	{
		JSObject skin;
		if (sizeof(jsParamNames) / sizeof(char*) != espLast) {
			showError("Список имён не совпадает со списком параметров");
			return skin;
		}
		for (int i = 0; i < espLast; i++) {
			eSkinParam eIterator = (eSkinParam)i;
			int numValue = 0;
			string strValue = "";
			string name = "";
			bool bResultIsString = getType(eIterator) == ecvString;
			if (bResultIsString) {
				strValue = getString(eIterator);
			} else {
				numValue = getInt(eIterator);
			}
			bool bError = false;
			name = jsParamNames[i];
			switch (eIterator)
			{
				// Целочисленные параметры
			case espHideScrollAny:
			case espHideScrollArrows:
			case espHideScrollMain:
			case espHideScrollActs:
			case espHideScrollVars:
			case espHideScrollObjs:
			case espUseHtml:
			case espNoSave:
			case espDisableScroll:
			case espViewAlwaysShow:
			case espIsStandalone:
			case espShowActs:
			case espShowVars:
			case espShowObjs:
			case espShowInput:
				break;
				// Строки с HTML-содержимым
			case espMsgTextFormat:
			case espInputTextFormat:
			case espMainDescTextFormat:
			case espVarsDescTextFormat:
				{
					strValue = applyHtmlFixes(strValue, true);
				}
				break;
			default:
				{
					showError("Неизвестный параметр оформления.");
					bError = true;
				}
				break;
			}
			if (bError)
				break;
			if (bResultIsString) {
				skin.SetProperty(ToWebString(name), ToWebString(strValue));
			} else {
				skin.SetProperty(ToWebString(name), JSValue(numValue));
			}
		}
		return skin;
	}

	bool Skin::isSomethingChanged()
	{
		return firstUpdate ||
			isChanged ||
			isActionsStyleChanged ||
			isHtmlModeChanged ||
			isBaseVarsChanged ||
			isSoftBaseVarsChanged ||
			isMainDescChanged ||
			isSoftMainDescChanged ||	
			isVarsDescChanged ||
			isSoftVarsDescChanged ||
			isActsListChanged ||
			isSoftActsListChanged ||
			isObjsListChanged ||
			isSoftObjsListChanged ||
			isViewChanged ||
			isUserInputChanged;
	}


	void Skin::showWindow(int type, int isShow)
	{
		//Контекст библиотеки

		// Одно из окон требуется скрыть либо показать
		switch (type)
		{
		case QSP_WIN_ACTS:
			if (getInt(espShowActs) != isShow)
			{
				setInt(espShowActs, isShow);
				isActsListChanged = true;
			}
			break;
		case QSP_WIN_OBJS:
			if (getInt(espShowObjs) != isShow)
			{
				setInt(espShowObjs, isShow);
				isObjsListChanged = true;
			}
			break;
		case QSP_WIN_VARS:
			if (getInt(espShowVars) != isShow)
			{
				setInt(espShowVars, isShow);
				isVarsDescChanged = true;
			}
			break;
		case QSP_WIN_INPUT:
			if (getInt(espShowInput) != isShow)
			{
				setInt(espShowInput, isShow);
				isUserInputChanged = true;
			}
			break;
		}
	}

	string Skin::applyHtmlFixes(string text, bool forceHtml)
	{
		// Контекст библиотеки
		if (!forceHtml && (getInt(espUseHtml) == 0)) {
			// Если USEHTML = 0, выводим текст "как есть".
			replaceAll(text, "&", "&amp;");
			replaceAll(text, "<", "&lt;");
			replaceAll(text, ">", "&gt;");
			// Заменяем переводы строк.
			replaceAll(text, "\r\n", "<br />");
			replaceAll(text, "\r", "<br />");
			replaceAll(text, "\n", "<br />");
		} else {
			// THIS IS ONLY FOR LEGACY
			// В Байтовском AeroQSP неправильно эскейпились кавычки и апострофы, и не эскейпились амперсанды(&).
			// Для совместимости с играми, написанными под Байтовский AeroQSP, делаем замену.
			// Специальная функция для правильной замены "&" на "&amp;"
			replaceAmp(text);
			replaceAll(text, "\\\"", "&quot;");
			replaceAll(text, "\\'", "&#39;");
			// Заменяем переводы строк, но только вне HTML-тегов.
			replaceNewlines(text);
		}
		return text;
	}
}