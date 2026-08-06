/**
 * @file: packing.cpp
 * @date: 2026-08-04
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проба сжатия записи хранилища собранных выражений — замер размера
 *        записи и расхода восстановления по каждому методу сжатия модуля
 *        «compressor» и сличение поведения восстановленных выражений
 *
 * @details Хранилище собранных выражений сжатия не выполняет: оно вызывает
 *          обработчики, потребителем установленные. Проба эта и служит
 *          образцом такой установки, а заодно даёт числа для выбора метода.
 *
 *          Проба собирается отдельно от библиотеки и запускается вручную:
 *          @code
 *          c++ -std=c++17 -O2 -Iinclude -o packing tools/regex/packing.cpp -L build -lawh
 *          ./packing
 *          @endcode
 *
 * @warning Числа снимать надлежит со сборки в режиме выпуска: сборка отладочная
 *          со сбором покрытия занижает показатели восьмикратно и для выбора
 *          метода непригодна.
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <string>
#include <vector>
#include <random>
#include <chrono>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <regex/grok/table.hpp>
#include <regex/regex.hpp>
#include <regex/storage.hpp>
#include <compressor/block.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Количество порождаемых выражений пробы
 *
 */
static constexpr size_t SAMPLES = 3000;

/**
 * @brief Количество прогонов замера
 *
 */
static constexpr size_t PASSES = 5;

/**
 * @brief Описание проверяемого метода сжатия записи
 *
 */
typedef struct Method {
	// Название метода сжатия записи
	const char * name;
	// Метод сжатия записи хранилища
	compressor::method_t method;
} method_t;

/**
 * @brief Набор проверяемых методов сжатия записи
 *
 */
static const method_t METHODS[] = {
	{"без сжатия", compressor::method_t::NONE},
	{"SNAPPY",     compressor::method_t::SNAPPY},
	{"DENSITY",    compressor::method_t::DENSITY},
	{"LZ4",        compressor::method_t::LZ4},
	{"LIZARD",     compressor::method_t::LIZARD},
	{"ZSTD",       compressor::method_t::ZSTD},
	{"BROTLI",     compressor::method_t::BROTLI},
	{"DEFLATE",    compressor::method_t::DEFLATE},
	{"ZLIB",       compressor::method_t::ZLIB},
	{"GZIP",       compressor::method_t::GZIP},
	{"BZIP2",      compressor::method_t::BZIP2},
	{"LZMA",       compressor::method_t::LZMA}
};

/**
 * @brief Функция извлечения показания часов в миллисекундах
 *
 * @return показание часов в миллисекундах
 *
 */
static double clocking() noexcept {
	// Выводим показание часов в миллисекундах
	return (static_cast <double> (chrono::duration_cast <chrono::nanoseconds> (
	 chrono::steady_clock::now().time_since_epoch()).count()) / 1000000.0);
}
/**
 * @brief Функция сборки набора выражений пробы
 *
 * @param regexp объект работы с регулярными выражениями
 * @return       набор собранных выражений пробы
 *
 * @details Набор складывается из шаблонов Grok, каковы суть выражения
 *          настоящие и немалые, и из выражений порождённых.
 *
 */
static vector <regex::storage_t::exp_t> collect(const regexp_t & regexp) noexcept {
	// Набор собранных выражений пробы
	vector <regex::storage_t::exp_t> result;
	/**
	 * @brief Набор составляющих порождаемого выражения
	 *
	 */
	const char * atoms[] = {
		"a", "[a-z]", "\\d", "\\w", ".", "x", "(?:ab|cd)", "[^q]", "\\s",
		"(?<name>z)", "(?=a)", "(?<!b)", "\\bq", "(?>a|b)", "[[:alpha:]]"
	};
	/**
	 * @brief Набор кванторов повторения порождаемого выражения
	 *
	 */
	const char * repeats[] = {"", "*", "+", "?", "{2,4}", "*?", "+?", "{1,3}?", "*+"};
	/**
	 * Выполняем перебор набора шаблонов Grok
	 */
	for(size_t i = 0; i < grok::PATTERNS_COUNT; i++) {
		// Выполняем сборку регулярного выражения шаблона
		const auto exp = regexp.build(grok::PATTERNS[i].body, {regexp_t::flag_t::DUPNAMES});
		/**
		 * Если сборка регулярного выражения выполнена
		 */
		if(exp)
			// Выполняем добавление собранного выражения в набор
			result.push_back(exp);
	}
	// Создаём порождатель случайных значений с постоянным зерном
	mt19937 generator(20260804);
	/**
	 * Выполняем порождение набора выражений
	 */
	for(size_t i = 0; i < SAMPLES; i++) {
		// Текст порождаемого выражения
		string pattern;
		// Получаем количество составляющих порождаемого выражения
		const size_t length = (1 + (generator() % 5));
		/**
		 * Выполняем порождение составляющих выражения
		 */
		for(size_t j = 0; j < length; j++) {
			// Выполняем добавление составляющей выражения
			pattern.append(atoms[generator() % 15]);
			// Выполняем добавление квантора повторения
			pattern.append(repeats[generator() % 9]);
		}
		/**
		 * Если выражение получает захватывающую группу со ссылкой
		 */
		if((generator() % 4) == 0)
			// Выполняем оборачивание выражения захватывающей группой
			pattern = ("(" + pattern + ")\\1?");
		// Выполняем сборку порождённого регулярного выражения
		const auto exp = regexp.build(pattern, {regexp_t::flag_t::DUPNAMES});
		/**
		 * Если сборка регулярного выражения выполнена
		 */
		if(exp)
			// Выполняем добавление собранного выражения в набор
			result.push_back(exp);
	}
	// Выводим набор собранных выражений пробы
	return result;
}
/**
 * @brief Функция входа в пробу сжатия записи хранилища
 *
 * @return результат исполнения пробы
 *
 */
int main() {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект журнала работы
	const log_t log(&fmk);
	// Создаём объект блочной компрессии
	const compressor::Block block(&log);
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Получаем набор собранных выражений пробы
	const auto fresh = collect(regexp);
	/**
	 * Если сборка набора выражений не выполнена
	 */
	if(fresh.empty()) {
		// Выводим сообщение об отказе сборки набора выражений
		::printf("сборка набора выражений не выполнена\n");
		// Выводим результат исполнения пробы
		return 1;
	}
	// Выводим количество собранных выражений пробы
	::printf("выражений собрано: %zu\n\n", fresh.size());
	/**
	 * @brief Набор текстов сличения поведения выражений
	 *
	 */
	const vector <string> subjects = {
		"", "a", "abc", "aaa 123 zzz", "GET /index.html HTTP/1.1", "forman@anyks.com",
		"192.168.5.150:8080", "Слово слово", "AZaz09", "-42.5e3", "2026-08-04T12:30:00Z"
	};
	// Размер записи хранилища без сжатия содержимого
	size_t reference = 0;
	// Выводим заголовок таблицы замера
	::printf("%-12s %12s %8s %10s %14s %10s\n", "метод", "размер Кб", "доля", "запись мс", "восстановл. мс", "расхожд.");
	/**
	 * Выполняем перебор набора проверяемых методов сжатия
	 */
	for(const auto & item : METHODS) {
		// Создаём объект хранилища собранных выражений
		regex::storage_t storage;
		/**
		 * Если метод сжатия записи установлен
		 */
		if(item.method != compressor::method_t::NONE) {
			// Получаем метод сжатия записи хранилища
			const compressor::method_t method = item.method;
			/**
			 * Выполняем установку обработчиков сжатия записи хранилища
			 *
			 * @details Хранилище знает метод сжатия лишь по имени и записывает
			 *          его в заголовок, а само сжатие выполняют обработчики.
			 *          Устройство это оставляет модуль регулярных выражений
			 *          самостоятельным: посторонних библиотек он не подключает.
			 */
			storage.packer(method,
			 /**
			  * @brief Обработчик сжатия записи хранилища
			  *
			  * @param source исходное содержимое записи
			  * @param result сжатое содержимое записи
			  * @return       результат сжатия содержимого записи
			  *
			  */
			 [&block, method](string_view source, string & result) noexcept -> bool {
				// Выполняем сжатие содержимого записи хранилища
				block.compress(source.data(), source.size(), method, result);
				// Выводим результат сжатия содержимого записи
				return !result.empty();
			 },
			 /**
			  * @brief Обработчик разжатия записи хранилища
			  *
			  * @param source сжатое содержимое записи
			  * @param result разжатое содержимое записи
			  * @return       результат разжатия содержимого записи
			  *
			  */
			 [&block, method](string_view source, string & result) noexcept -> bool {
				// Выполняем разжатие содержимого записи хранилища
				block.decompress(source.data(), source.size(), method, result);
				// Выводим результат разжатия содержимого записи
				return !result.empty();
			 });
		}
		// Запись хранилища собранных выражений
		string record;
		// Расход записи собранных выражений
		double writing = 0.0;
		/**
		 * Выполняем перебор прогонов замера записи выражений
		 */
		for(size_t pass = 0; pass < 3; pass++) {
			// Записываемая запись хранилища очередного прогона
			string current;
			// Получаем показание часов на входе в запись
			const double started = clocking();
			/**
			 * Если запись собранных выражений не выполнена
			 */
			if(!storage.save(fresh, current)) {
				// Выводим сообщение об отказе записи собранных выражений
				::printf("%-12s ОТКАЗ записи, код %u\n", item.name, static_cast <uint32_t> (storage.error()));
				// Выполняем очистку записи хранилища
				record.clear();
				// Прекращаем перебор прогонов замера записи
				break;
			}
			// Получаем расход записи набора выражений
			const double spent = (clocking() - started);
			/**
			 * Если расход записи прогона наименьший
			 */
			if((pass == 0) || (spent < writing))
				// Выполняем установку расхода записи выражений
				writing = spent;
			// Выполняем сохранение записи хранилища
			record = ::move(current);
		}
		/**
		 * Если запись собранных выражений не выполнена
		 */
		if(record.empty())
			// Переходим к следующему методу сжатия записи
			continue;
		// Набор восстановленных выражений
		vector <regex::storage_t::exp_t> restored;
		// Расход восстановления собранных выражений
		double reading = 0.0;
		/**
		 * Выполняем перебор прогонов замера восстановления выражений
		 */
		for(size_t pass = 0; pass < PASSES; pass++) {
			// Набор восстановленных выражений очередного прогона
			vector <regex::storage_t::exp_t> records;
			// Получаем копию записи хранилища, передаваемую во владение
			string current = record;
			// Получаем показание часов на входе в восстановление
			const double started = clocking();
			/**
			 * Если восстановление собранных выражений не выполнено
			 */
			if(!storage.adopt(::move(current), records)) {
				// Выводим сообщение об отказе восстановления выражений
				::printf("%-12s ОТКАЗ восстановления, код %u\n", item.name, static_cast <uint32_t> (storage.error()));
				// Выполняем очистку набора восстановленных выражений
				restored.clear();
				// Прекращаем перебор прогонов замера восстановления
				break;
			}
			// Получаем расход восстановления набора выражений
			const double spent = (clocking() - started);
			/**
			 * Если расход восстановления прогона наименьший
			 */
			if((pass == 0) || (spent < reading))
				// Выполняем установку расхода восстановления выражений
				reading = spent;
			// Выполняем сохранение набора восстановленных выражений
			restored = ::move(records);
		}
		/**
		 * Если количество восстановленных выражений набору не отвечает
		 */
		if(restored.size() != fresh.size()) {
			// Выводим сообщение о расхождении количества выражений
			::printf("%-12s восстановлено %zu выражений вместо %zu\n", item.name, restored.size(), fresh.size());
			// Переходим к следующему методу сжатия записи
			continue;
		}
		// Количество обнаруженных расхождений поведения
		size_t divergences = 0;
		/**
		 * Выполняем перебор набора восстановленных выражений
		 */
		for(size_t i = 0; i < fresh.size(); i++) {
			/**
			 * Выполняем перебор набора текстов сличения
			 */
			for(const auto & subject : subjects) {
				/**
				 * Если границы совпадения выражений расходятся
				 */
				if(regexp.match(subject, fresh.at(i)) != regexp.match(subject, restored.at(i)))
					// Выполняем увеличение количества обнаруженных расхождений
					divergences++;
			}
		}
		/**
		 * Если размер записи без сжатия ещё не установлен
		 */
		if(reference == 0)
			// Выполняем установку размера записи без сжатия
			reference = record.size();
		// Выводим итог замера метода сжатия записи
		::printf("%-12s %12zu %7.1f%% %10.1f %14.2f %10zu\n", item.name, (record.size() / 1024),
		 (100.0 * static_cast <double> (record.size()) / static_cast <double> (reference > 0 ? reference : 1)),
		 writing, reading, divergences);
	}
	// Выводим результат исполнения пробы
	return 0;
}
