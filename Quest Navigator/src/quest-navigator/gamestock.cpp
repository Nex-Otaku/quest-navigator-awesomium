#include "gamestock.h"
#include "utils.h"

namespace QuestNavigator {
	sqlite3* Gamestock::pDb = 0;
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
		// Открываем соединение.
		if (!openDb())
			return false;

		// Читаем список локальных игр.
		string sql = "SELECT * FROM games WHERE web = '0' ORDER BY last_run DESC;";
		// Обработка результатов SELECT идёт в cbSelectLocalGames.
		if (!execSql(sql, cbSelectLocalGames))
			return false;

		// Закрываем соединение.
		closeDb();
		return true;
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
	
	bool Gamestock::addGame(GamestockEntry game)
	{
		// Открываем соединение.
		if (!openDb())
			return false;

		// Читаем список локальных игр.
		//string sql = "SELECT * FROM games WHERE web = '0' ORDER BY last_run DESC;";
		// Обработка результатов SELECT идёт в cbSelectLocalGames.
		//if (!execSql(sql, cbSelectLocalGames))
		//	return false;

		// Закрываем соединение.
		closeDb();
		return true;
	}

	bool Gamestock::openDb()
	{
		// Открываем соединение.
		if (sqlite3_open("D:\\qn.db", &pDb)) {
			string sqliteErr = sqlite3_errmsg(pDb);
			showError("Ошибка открытия/создания БД: " + sqliteErr);
			return false;
		}

		// Готовим табличку к чтению и записи.
		if (!prepareGamesTable())
			return false;

		return true;
	}

	void Gamestock::closeDb()
	{
		// Закрываем соединение.
		sqlite3_close(pDb);
		pDb = 0;
	}

	bool Gamestock::prepareGamesTable()
	{
		// Если таблицы нет, создаём её.
		string sql = "CREATE TABLE IF NOT EXISTS games ("; 
		sql += "\"id\" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,";
		sql += "\"web\" INTEGER NOT NULL DEFAULT (0),";
		sql += "\"local_file\" TEXT,";
		sql += "\"title\" TEXT,";
		sql += "\"hash\" TEXT,";
		sql += "\"cache\" TEXT,";
		sql += "\"saves\" TEXT,";
		sql += "\"last_run\" INTEGER);";
		if (!execSql(sql, 0))
			return false;
		return true;
	}

	bool Gamestock::execSql(string sql, sqlite3_callback callback)
	{
		// БД должна быть открыта.
		if (pDb == 0) {
			showError("Не установлено соединение с БД");
			return false;
		}
		// Выполняем SQL-запрос.
		char *err = 0;
		if (sqlite3_exec(pDb, sql.c_str(), callback, 0, &err))
		{
			string sqliteErr = err;
			sqlite3_free(err);
			showError("Ошибка SQL: " + sqliteErr);

			// Закрываем соединение.
			closeDb();
			return false;
		}
		return true;
	}
}