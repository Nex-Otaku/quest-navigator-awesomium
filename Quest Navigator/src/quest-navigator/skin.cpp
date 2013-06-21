#include "skin.h"
#include "../deps/qsp/qsp.h"
#include "../deps/qsp/bindings/default/qsp_default.h"
#include "utils.h"

namespace QuestNavigator {

	ConfigValue Skin::_paramList[espLast];

	string Skin::getString(eSkinParam param)
	{
		return _paramList[param].getString();
	}
	void Skin::setString(eSkinParam param, string value)
	{
		_paramList[param].setString(value);
	}


	bool Skin::firstUpdate;
	bool Skin::isChanged;
	bool Skin::isMainBackChanged;
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

	// Устанавливаем значения по умолчанию
	void Skin::resetSettings()
	{

		//STUB

		firstUpdate = true;
	}
	
	// Сбрасываем флаги обновления
	void Skin::resetUpdate()
	{
	    firstUpdate = false;
	    isChanged = false;
	    isMainBackChanged = false;
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

}