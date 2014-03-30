#ifndef GAMESTOCK_H_
#define GAMESTOCK_H_

#include <string>
#include <vector>
#include <map>

using namespace std;

namespace QuestNavigator {
	struct GamestockEntry {
		int id;
		bool web;
		string title;
		string hash;
		string cache;
		string saves;
		int last_run;
	};

	class Gamestock {
	private:
		static vector<GamestockEntry> vecLocalGames;
		static map<string, GamestockEntry> mapLocalGames;
		
		static bool readLocalGames();
	public:
		static bool getLocalGames(vector<GamestockEntry> &vec);
		static bool getLocalGames(map<string, GamestockEntry> &map);

		// Колбэки для SQLite.
		static int cbSelectLocalGames(void *data, int argc, char **argv, char **azColName);
	};
}

#endif
