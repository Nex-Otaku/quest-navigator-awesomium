#include "gamestock.h"
#include "configuration.h"
#include "utils.h"

namespace QuestNavigator {
	sqlite3* Gamestock::pDb = 0;
	vector<GamestockEntry> Gamestock::vecLocalGames;
	map<string, GamestockEntry> Gamestock::mapLocalGames;

	string GamestockEntry::idVal() 
	{
		return Gamestock::escape(id);
	}
	string GamestockEntry::webVal()
	{
		return Gamestock::escape(web);
	}
	string GamestockEntry::localFileVal()
	{
		return Gamestock::escape(local_file);
	}
	string GamestockEntry::titleVal()
	{
		return Gamestock::escape(title);
	}
	string GamestockEntry::hashVal()
	{
		return Gamestock::escape(hash);
	}
	string GamestockEntry::cacheVal()
	{
		return Gamestock::escape(cache);
	}
	string GamestockEntry::savesVal()
	{
		return Gamestock::escape(saves);
	}
	string GamestockEntry::lastRunVal()
	{
		return Gamestock::escape(last_run);
	}

	string GamestockEntry::sqlInsertValues()
	{
		string values = "(";
		values += webVal() + ", ";
		values += localFileVal() + ", ";
		values += titleVal() + ", ";
		values += hashVal() + ", ";
		values += cacheVal() + ", ";
		values += savesVal() + ", ";
		values += lastRunVal() + ")";
		return values;
	}

	string GamestockEntry::sqlUpdateValues()
	{
		string values = "SET web = " + webVal() + ", ";
		values += "local_file = " + localFileVal() + ", ";
		values += "title = " + titleVal() + ", ";
		values += "hash = " + hashVal() + ", ";
		values += "cache = " + cacheVal() + ", ";
		values += "saves = " + savesVal() + ", ";
		values += "last_run = " + lastRunVal();
		return values;
	}

	string Gamestock::escape(int num)
	{
		return escape(intToString(num));
	}

	string Gamestock::escape(bool flag)
	{
		return escape(intToString(flag ? 1 : 0));
	}

	string Gamestock::escape(string text)
	{
		replaceAll(text, "'", "''");
		return "'" + text + "'";
	}
	
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

		// Очищаем контейнеры.
		vecLocalGames.clear();
		mapLocalGames.clear();

		// Читаем список локальных игр.
		string sql = "SELECT id, web, local_file, title, hash, cache, saves, last_run";
		sql += " FROM games";
		sql += " WHERE web = " + escape(0) + " ORDER BY last_run DESC;";
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

	// Удаляем из списка все игры, которые были удалены с диска.
	bool Gamestock::deleteBrokenGames()
	{
		if (!readLocalGames())
			return false;

		// Список игр прочитан.
		// Проходим по списку и удаляем из списка и кэша все игры, 
		// которые загружены не из сети, а с локальных дисков,
		// и в данный момент недоступны.
		for (int i = 0; i < (int)vecLocalGames.size(); i++) {
			GamestockEntry game = vecLocalGames[i];
			if ((game.web == false) && !fileExists(game.local_file)) {
				if (!deleteGame(game)) {
					showError("Ошибка при удалении игры: " + game.hash);
					return false;
				}
			}
		}

		return true;
	}

	// Возвращаем список игр, позаботившись об удалении "потерянных" игр.
	bool Gamestock::getLocalGamesWithCheck(vector<GamestockEntry> &vec)
	{
		if (!deleteBrokenGames())
			return false;
		if (!getLocalGames(vec))
			return false;
		return true;
	}
	
	bool Gamestock::addGame(GamestockEntry game)
	{
		// Открываем соединение.
		if (!openDb())
			return false;

		// Добавляем игру в список.
		string sql = "INSERT INTO games (web, local_file, title, hash, cache, saves, last_run) ";
		sql += "VALUES " + game.sqlInsertValues() + ";";
		if (!execSql(sql, 0))
			return false;

		// Закрываем соединение.
		closeDb();
		return true;
	}

	bool Gamestock::updateGame(GamestockEntry game)
	{
		GamestockEntry test;
		// Если игра есть в списке, обновляем поля.
		// Если нет, добавляем в список.
		bool gameExists = getLocalGame(game.hash, test);
		if (gameExists) {
			// Открываем соединение.
			if (!openDb())
				return false;

			// Обновляем игру.
			string sql = "UPDATE games ";
			sql += game.sqlUpdateValues() + " WHERE id = " + test.idVal() + ";";
			if (!execSql(sql, 0))
				return false;

			// Закрываем соединение.
			closeDb();
			return true;
		} else {
			if (!addGame(game))
				return false;
		}
		return true;
	}

	bool Gamestock::deleteGame(GamestockEntry game)
	{
		GamestockEntry test;
		// Если игра есть в БД, удаляем её.
		// Также очищаем папку кэша игры.
		// При этом сейвы остаются на прежнем месте.
		// Если игра вернётся в своё расположение,
		// сохранения будут снова доступны.
		bool gameExists = getLocalGame(game.hash, test);
		if (gameExists) {
			// Открываем соединение.
			if (!openDb())
				return false;

			// Удаляем запись игры в БД.
			string sql = "DELETE FROM games WHERE id = " + test.idVal() + ";";
			if (!execSql(sql, 0)) {
				return false;
			}

			// Закрываем соединение.
			closeDb();

			// Очищаем папку кэша игры.
			if (!deleteDirectory(game.cache))
				return false;
		}
		return true;
	}

	bool Gamestock::openDb()
	{
		// Открываем соединение.
		string dbPath = Configuration::getString(ecpAppDataDir) + PATH_DELIMITER + "qn.db";
		if (sqlite3_open(dbPath.c_str(), &pDb)) {
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
		sql += "\"hash\" TEXT UNIQUE,";
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

	bool Gamestock::getLocalGame(string hash, GamestockEntry &game)
	{
		// Загружаем карту по играм.
		// Вывод ошибки выполняется внутри вызова, нам остаётся просто выйти.
		map<string, GamestockEntry> mapGames;
		if (!Gamestock::getLocalGames(mapGames))
			return false;

		// Ищем игру в списке по хэшу.
		map<string, GamestockEntry>::iterator it = mapGames.find(hash);
		if (it == mapGames.end())
			return false;

		// Игра найдена.
		game = it->second;
		return true;
	}
}