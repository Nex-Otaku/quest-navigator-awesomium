#ifndef GAMESTOCK_H_
#define GAMESTOCK_H_

#include <string>
#include <vector>
#include <map>
#include "../deps/sqlite/sqlite3.h"

using namespace std;

namespace QuestNavigator {
	struct GamestockEntry {
		int id;
		bool web;
		string local_file;
		string title;
		string hash;
		string cache;
		string saves;
		int last_run;

		// Перевод значений в строки для SQL-запросов.
		string idVal();
		string webVal();
		string localFileVal();
		string titleVal();
		string hashVal();
		string cacheVal();
		string savesVal();
		string lastRunVal();

		// Представление полей в виде VALUES(...),
		// для вставки в БД.
		string sqlInsertValues();
		// Представление полей в виде SET field = value, ...
		// для вставки в БД.
		string sqlUpdateValues();
	};

	class Gamestock {
	private:
		static sqlite3* pDb;
		static vector<GamestockEntry> vecLocalGames;
		static map<string, GamestockEntry> mapLocalGames;
		
		static bool openDb();
		static void closeDb();
		static bool execSql(string sql, sqlite3_callback callback);

		static bool readLocalGames();
		static bool prepareGamesTable();
	public:
		static bool getLocalGames(vector<GamestockEntry> &vec);
		static bool getLocalGamesWithCheck(vector<GamestockEntry> &vec);
		static bool getLocalGames(map<string, GamestockEntry> &map);
		static bool getLocalGame(string hash, GamestockEntry &game);
		static bool addGame(GamestockEntry game);
		static bool updateGame(GamestockEntry game);
		static bool deleteGame(GamestockEntry game);
		static bool deleteBrokenGames();

		static string escape(int num);
		static string escape(bool flag);
		static string escape(string text);

		// Колбэки для SQLite.
		static int cbSelectLocalGames(void *data, int argc, char **argv, char **azColName);
	};
}

#endif
