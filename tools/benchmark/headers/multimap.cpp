/**
 * @file: multimap.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Стенд сравнения контейнера HTTP-заголовков — мультикарта стандартной библиотеки
 *
 * @details Хранение полей мультикартой с регистронезависимым сравнением - самая
 *          распространённая самодельная реализация: так устроены заголовки в
 *          cpp-httplib и в доброй половине служб, написанных без готовой
 *          библиотеки HTTP. Стенд отвечает на вопрос, что вообще даёт отдельный
 *          контейнер по сравнению с этим - и стоит ли он того
 *
 * @note Порядок полей мультикарта не сохраняет, а он несёт смысл (RFC 9110 §5.3):
 *       собрать по ней сообщение обратно нельзя. Сценарий сборки поэтому измеряет
 *       у неё меньшую работу, чем у остальных, и доля по нему сравнением
 *       равного с равным не является - это записано и в COMPARISON.md
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы стендов
 */
#include "driver.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>

/**
 * @brief Инкапсулируем сценарии стенда в пространство имён
 *
 */
namespace {
	/**
	 * @brief Структура регистронезависимого сравнения названий полей
	 *
	 */
	typedef struct Insensitive {
		/**
		 * @brief Оператор сравнения названий полей
		 *
		 * @param first  первое название поля
		 * @param second второе название поля
		 * @return       результат сравнения без учёта регистра
		 *
		 */
		bool operator()(const std::string & first, const std::string & second) const noexcept {
			// Выводим результат регистронезависимого сравнения названий
			return (::strcasecmp(first.c_str(), second.c_str()) < 0);
		}
	} insensitive_t;

	/**
	 * @brief Тип контейнера полей сравниваемой реализации
	 *
	 */
	using fields_t = std::multimap <std::string, std::string, insensitive_t>;

	/**
	 * @brief Функция наполнения контейнера полями образца
	 *
	 * @param fields наполняемый контейнер полей
	 *
	 */
	static void fill(fields_t & fields) noexcept {
		/**
		 * Проходим по всем полям образца
		 */
		for(size_t i = 0; i < scenarios::FIELDS; i++)
			// Добавляем очередное поле образца, сохраняя одноимённые
			fields.emplace(scenarios::NAMES[i], scenarios::VALUES[i]);
	}
};

/**
 * @brief Функция входа в стенд
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Получаем фильтр названий выполняемых сценариев
	const char * mask = rival::filter(argc, argv);
	// Выполняем сценарий наполнения контейнера полями запроса
	driver::execute("headers/build/request", "наборов/с", scenarios::ROUNDS, mask, []([[maybe_unused]] const size_t index) noexcept -> uint64_t {
		// Создаём контейнер полей
		fields_t fields;
		// Наполняем контейнер полями образца
		::fill(fields);
		// Выводим итог наполнения контейнера
		return static_cast <uint64_t> (fields.size());
	});
	/**
	 * Выполняем сценарий поиска поля по названию без учёта регистра
	 */
	{
		// Создаём контейнер полей сценария
		static fields_t fields;
		// Наполняем контейнер полями образца
		::fill(fields);
		// Выполняем сценарий поиска поля по названию
		driver::execute("headers/lookup/hit", "поисков/с", scenarios::LIGHT_ROUNDS, mask, [](const size_t index) noexcept -> uint64_t {
			// Выполняем поиск поля по названию
			const auto i = fields.find(scenarios::LOOKUPS[index & 1]);
			// Выводим длину найденного значения поля
			return static_cast <uint64_t> ((i != fields.end()) ? i->second.size() : 0);
		});
	}
	/**
	 * Выполняем сценарий сборки сообщения в виде, пригодном для передачи
	 */
	{
		// Создаём контейнер полей сценария
		static fields_t fields;
		// Наполняем контейнер полями образца
		::fill(fields);
		// Выполняем сценарий сборки сообщения
		driver::execute("headers/serialize/request", "сборок/с", scenarios::ROUNDS, mask, []([[maybe_unused]] const size_t index) noexcept -> uint64_t {
			// Создаём строку собираемого сообщения
			std::string result;
			// Резервируем память под собираемое сообщение
			result.reserve(1024);
			// Дописываем стартовую строку запроса
			result.append("GET /index.html HTTP/1.1\r\n");
			/**
			 * Проходим по всем полям контейнера
			 */
			for(const auto & field : fields){
				// Дописываем название поля
				result.append(field.first);
				// Дописываем разделитель названия и значения
				result.append(": ");
				// Дописываем значение поля
				result.append(field.second);
				// Дописываем завершающий перевод строки
				result.append("\r\n");
			}
			// Дописываем завершающую пустую строку
			result.append("\r\n");
			// Выводим длину собранного сообщения
			return static_cast <uint64_t> (result.size());
		});
	}
	/**
	 * Выполняем сценарий замены значения существующего поля
	 */
	{
		// Создаём контейнер полей сценария
		static fields_t fields;
		// Наполняем контейнер полями образца
		::fill(fields);
		// Выполняем сценарий замены значения поля
		driver::execute("headers/replace/existing", "замен/с", scenarios::LIGHT_ROUNDS, mask, [](const size_t index) noexcept -> uint64_t {
			// Снимаем прежние вхождения поля
			fields.erase(scenarios::REPLACE);
			// Добавляем поле с очередным значением
			fields.emplace(scenarios::REPLACE, scenarios::REPLACEMENTS[index & 1]);
			// Выводим итог замены значения поля
			return static_cast <uint64_t> (fields.size());
		});
	}
	// Выводим накопленные итоги работы сценариев
	driver::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}
