#include "gamestock.h"
#include "utils.h"
#include "../deps/sqlite/sqlite3.h"

namespace QuestNavigator {
	vector<GamestockEntry> Gamestock::vecLocalGames;
	map<string, GamestockEntry> Gamestock::mapLocalGames;
	
	int Gamestock::cbSelectLocalGames(void *data, int argc, char **argv, char **azColName)
	{
		// Сохраняем все полученные из базы строки в статические контейнеры.
		GamestockEntry game;
		game.id = stoi(argv[0]);
		game.web = stoi(argv[1]) == 1;
		game.local_file = argv[2];
		game.title = argv[3];
		game.hash = argv[4];
		game.cache = argv[5];
		game.saves = argv[6];
		game.last_run = stoi(argv[7]);
		vecLocalGames.push_back(game);
		mapLocalGames[game.hash] = game;
		return 0;
	}

	bool Gamestock::readLocalGames()
	{
		// Хэндл соединения с БД.
		sqlite3 *db = 0; 
		char *err = 0;

		// Открываем соединение.
		if (sqlite3_open("D:\\qn.db", &db)) {
			string sqliteErr = sqlite3_errmsg(db);
			showError("Ошибка открытия/создания БД: " + sqliteErr);
			return false;
		} else {
			// Выполняем SQL.

			// Если таблицы нет, создаём её.
			string sql = "CREATE TABLE IF NOT EXISTS games ("; 
			sql += "\"id\" INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,";
			sql += "\"web\" INTEGER NOT NULL DEFAULT (0),";
			sql += "\"local_file\" TEXT,";
			sql += "\"title\" TEXT,";
			sql += "\"hash\" TEXT,";
			sql += "\"cache\" TEXT,";
			sql += "\"saves\" TEXT,";
			sql += "\"last_run\" INTEGER);";

			// Читаем список локальных игр.
			sql += "SELECT * FROM games WHERE web = '0' ORDER BY last_run DESC;";
			// Обработка результатов SELECT идёт в cbSelectLocalGames.
			if (sqlite3_exec(db, sql.c_str(), cbSelectLocalGames, 0, &err))
			{
				string sqliteErr = err;
				showError("Ошибка SQL: " + sqliteErr);
				sqlite3_free(err);
				return false;
			}
			return true;
		}
		
		//STUB
		// Сделать удаление из списка всех несуществующих игр.

		// Закрываем соединение.
		sqlite3_close(db);
	}

	bool Gamestock::getLocalGames(vector<GamestockEntry> &vec)
	{
		if (!readLocalGames())
			return false;
		vec = vecLocalGames;
		return true;
	}

	bool Gamestock::getLocalGames(map<string, GamestockEntry> &map)
	{
		if (!readLocalGames())
			return false;
		map = mapLocalGames;
		return true;
	}
}