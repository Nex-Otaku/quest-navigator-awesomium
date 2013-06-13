#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <string>

using namespace std;

namespace QuestNavigator {

	// ¬ этом классе хран€тс€ вс€ческие параметры плеера
	class Configuration {
	private:
		static string _contentPath;

	public:

		static string getContentPath();
		static void setContentPath(string path);


	};
}

#endif
