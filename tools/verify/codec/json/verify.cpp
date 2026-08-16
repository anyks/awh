/**
 * @file verify.cpp
 * @date 2026-08-16
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Стенд сличения разбора JSON с эталоном — разбор корпуса текстов с выдачей
 *        исхода по каждому и перезаписью принятого для сличения значений
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/json/json.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён контейнеров данных
 */
using namespace awh::codec;

/**
 * @brief Безымянное пространство имён вспомогательных объявлений стенда
 *
 */
namespace {
	/**
	 * Размеры кусков, какими подаётся текст сверх подачи целиком
	 *
	 * @note Размеры взяты нарочито неудобными: они рвут и записи чисел, и знаки
	 *       кодировки, и отменяющие последовательности
	 */
	static const vector <size_t> CHUNKS = {1, 2, 3, 7};
	/**
	 * @brief Функция разбора текста документа с заданным размером куска
	 *
	 * @param text   разбираемый текст документа
	 * @param chunk  размер подаваемого куска, ноль означает подачу целиком
	 * @param output собранная перезапись дерева документа
	 * @return       признак успешности разбора
	 *
	 */
	static bool digest(const string & text, const size_t chunk, string & output) noexcept {
		// Объект документа
		json::document_t doc;
		// Получаем настройки документа
		json::document_t::settings_t settings = doc.settings();
		/**
		 * Дозволяем повторяющиеся имена полей объекта
		 *
		 * @note Стандарт повтора не запрещает, а наше правило по умолчанию его отвергает.
		 *       Сличение идёт со стандартом, оттого правило здесь ослаблено намеренно
		 */
		settings.duplicates = json::duplicate_t::KEEP;
		// Выполняем установку настроек документа
		doc.settings(settings);
		/**
		 * Если текст подаётся целиком
		 */
		if(chunk == 0){
			/**
			 * Если разбор текста документа завершился отказом
			 */
			if(!doc.parse(text))
				// Выводим признак неудачного разбора
				return false;
			// Выполняем перезапись дерева документа
			output = doc.dump();
			// Выводим признак успешного разбора
			return true;
		}
		// Объект потокового чтения текста документа
		json::reader_t reader;
		// Получаем настройки потокового чтения текста
		json::reader_t::settings_t reading = reader.settings();
		// Выполняем установку настроек потокового чтения текста
		reader.settings(reading);
		/**
		 * Выполняем подачу текста документа кусками
		 */
		for(size_t offset = 0; offset <= text.size(); offset += chunk){
			// Получаем размер очередного подаваемого куска
			const size_t length = (((offset + chunk) < text.size()) ? chunk : (text.size() - offset));
			/**
			 * Если подача очередного куска текста документа завершилась отказом
			 */
			if(!reader.feed(text.data() + offset, length, ((offset + length) >= text.size())))
				// Выводим признак неудачного разбора
				return false;
			/**
			 * Выполняем изъятие всех собранных событий разбора
			 */
			while(reader.next())
				// Пропускаем очередное событие разбора
				(void) reader.event();
			/**
			 * Если текст документа исчерпан
			 */
			if((offset + length) >= text.size())
				// Прекращаем подачу текста документа
				break;
		}
		// Выводим признак успешного разбора
		return true;
	}
}

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	/**
	 * Если каталог корпуса не передан
	 */
	if(argc < 2){
		// Выводим порядок вызова стенда
		cerr << "Вызов: verify <каталог корпуса>" << endl;
		// Выходим из приложения с кодом ошибки
		return EXIT_FAILURE;
	}
	// Перечень разбираемых текстов корпуса
	vector <filesystem::path> corpus;
	/**
	 * Выполняем сбор всех текстов корпуса
	 */
	for(const auto & entry : filesystem::directory_iterator(argv[1])){
		/**
		 * Если запись каталога является обычным файлом
		 */
		if(entry.is_regular_file())
			// Добавляем текст в перечень разбираемых
			corpus.push_back(entry.path());
	}
	// Выполняем упорядочение текстов корпуса по имени
	sort(corpus.begin(), corpus.end());
	/**
	 * Выполняем перебор всех текстов корпуса
	 */
	for(const auto & path : corpus){
		// Открываем файл очередного текста корпуса
		ifstream file(path, ios::binary);
		/**
		 * Если файл открыть не удалось
		 */
		if(!file.is_open())
			// Выполняем переход к следующему тексту корпуса
			continue;
		// Читаем содержимое файла целиком
		const string text((istreambuf_iterator <char> (file)), istreambuf_iterator <char> ());
		// Собранная перезапись дерева документа
		string output;
		// Выполняем разбор текста, поданного целиком
		const bool whole = ::digest(text, 0, output);
		// Признак совпадения исхода при всякой нарезке текста
		bool stable = true;
		/**
		 * Выполняем перебор всех размеров подаваемого куска
		 */
		for(const size_t chunk : ::CHUNKS){
			// Отбрасываемая перезапись дерева документа
			string ignored;
			// Запоминаем расхождение исхода при подаче кусками
			stable = (stable && (::digest(text, chunk, ignored) == whole));
		}
		/**
		 * Выводим исход разбора очередного текста корпуса
		 *
		 * @note Перезапись выдаётся одной строкой: переводов строки в сжатой записи
		 *       не бывает, и разбор выдачи сличением остаётся построчным
		 */
		cout << path.filename().string() << '\t'
		     << (whole ? "accept" : "reject") << '\t'
		     << (stable ? "stable" : "unstable") << '\t'
		     << output << endl;
	}
	// Выходим из приложения
	return EXIT_SUCCESS;
}
