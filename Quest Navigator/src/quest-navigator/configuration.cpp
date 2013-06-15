#include "configuration.h"

namespace QuestNavigator {

	string Configuration::_contentPath = "";
	string Configuration::getContentPath()
	{
		return _contentPath;
	}
	void Configuration::setContentPath(string path)
	{
		_contentPath = path;
	}

}