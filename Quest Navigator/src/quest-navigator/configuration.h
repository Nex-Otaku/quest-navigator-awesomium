#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <string>

using namespace std;

namespace QuestNavigator {

	// В этом классе хранятся всяческие параметры плеера
	class Configuration {
	private:
		static string _contentPath;

	public:

		static string getContentPath();
		static void setContentPath(string path);


	};
}

#endif
