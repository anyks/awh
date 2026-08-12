/**
 * @file: toml-reader.cpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример потокового чтения текста настроек TOML — выдача событий разбора по мере
 *        поступления текста, подача его кусками и разбор значений всех отводимых типов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/toml/reader.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Разбираемый текст настроек
 *
 */
static const char * TEXT =
 "# настройки службы\n"
 "title = \"пример\"\n"
 "\n"
 "[server]\n"
 "host = 'локальный' # хозяин\n"
 "port = 8080\n"
 "ratio = 0.25\n"
 "started = 1979-05-27T07:32:00Z\n"
 "flags = [true, false]\n"
 "point = { x = 1, y = 2 }\n"
 "\n"
 "[[products]]\n"
 "name = \"гвоздь\"\n";

/**
 * @brief Функция вывода составного имени ключа события
 *
 * @param path составные части имени ключа события
 *
 */
static void named(const vector <codec::toml::part_t> & path) noexcept {
	/**
	 * Выполняем перебор всех составных частей имени ключа
	 */
	for(size_t i = 0; i < path.size(); i++){
		/**
		 * Если составная часть имени не первая
		 */
		if(i > 0)
			// Выводим разделитель составных частей имени
			cout << ".";
		// Выводим очередную составную часть имени
		cout << path.at(i).name;
	}
}
/**
 * @brief Функция вывода значения события разбора
 *
 * @param value выводимое значение события разбора
 *
 */
static void valued(const codec::toml::value_t & value) noexcept {
	// Выводим название типа значения
	cout << codec::toml::name(value.type) << " ";
	/**
	 * Выполняем выбор типа выводимого значения
	 */
	switch(static_cast <uint8_t> (value.type)){
		// Если значением является последовательность знаков
		case static_cast <uint8_t> (codec::toml::type_t::STRING):
			// Выводим содержимое строкового значения
			cout << value.text;
		break;
		// Если значением является целое число
		case static_cast <uint8_t> (codec::toml::type_t::INTEGER):
			// Выводим целое число значения
			cout << value.integer;
		break;
		// Если значением является число с плавающей точкой
		case static_cast <uint8_t> (codec::toml::type_t::FLOAT):
			// Выводим число с плавающей точкой значения
			cout << value.real;
		break;
		// Если значением является логическое значение
		case static_cast <uint8_t> (codec::toml::type_t::BOOLEAN):
			// Выводим логическое значение
			cout << (value.boolean ? "true" : "false");
		break;
		// Если значением является отметка времени со смещением часового пояса
		case static_cast <uint8_t> (codec::toml::type_t::OFFSET_DATETIME):
			// Выводим год, месяц и день отметки времени
			cout << static_cast <uint32_t> (value.stamp.date.year) << "-"
			     << static_cast <uint32_t> (value.stamp.date.month) << "-"
			     << static_cast <uint32_t> (value.stamp.date.day);
		break;
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
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Блокируем неиспользуемую переменную
	(void) argc;
	// Блокируем неиспользуемую переменную
	(void) argv;
	// Разбираемый текст настроек
	const string text(TEXT);
	// Создаём объект потокового чтения текста настроек
	codec::toml::reader_t reader;
	/**
	 * Размер куска подачи разбираемого текста
	 *
	 * @note Текст подаётся кусками намеренно: разбор обещает одну и ту же выдачу
	 *       при всякой нарезке, и потребителю не нужно ждать всего текста целиком
	 */
	const size_t chunk = 16;
	// Смещение очередного подаваемого куска текста
	size_t offset = 0;
	/**
	 * Выполняем подачу разбираемого текста кусками
	 */
	do {
		// Получаем размер очередного подаваемого куска текста
		const size_t size = (((offset + chunk) > text.size()) ? (text.size() - offset) : chunk);
		/**
		 * Если подача очередного куска текста не удалась
		 */
		if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size())))
			// Выполняем прекращение подачи разбираемого текста
			break;
		// Выполняем смещение на размер поданного куска текста
		offset += size;
		/**
		 * Выполняем перебор всех событий, выданных разбором
		 */
		while(reader.next()){
			// Выводим место события в исходном тексте
			cout << reader.location().line << ":" << reader.location().column << " ";
			/**
			 * Выполняем выбор разновидности выданного события
			 */
			switch(static_cast <uint8_t> (reader.event())){
				// Если событием является объявление таблицы
				case static_cast <uint8_t> (codec::toml::event_t::TABLE): {
					// Выводим обозначение объявления таблицы
					cout << "таблица ";
					// Выводим имя объявленной таблицы
					::named(reader.path());
				} break;
				// Если событием является объявление очередной таблицы набора таблиц
				case static_cast <uint8_t> (codec::toml::event_t::ARRAY_TABLE): {
					// Выводим обозначение объявления таблицы набора таблиц
					cout << "таблица набора ";
					// Выводим имя объявленного набора таблиц
					::named(reader.path());
				} break;
				// Если событием является имя ключа пары
				case static_cast <uint8_t> (codec::toml::event_t::KEY): {
					// Выводим обозначение имени ключа пары
					cout << "ключ ";
					// Выводим имя ключа пары
					::named(reader.path());
				} break;
				// Если событием является значение
				case static_cast <uint8_t> (codec::toml::event_t::VALUE): {
					// Выводим обозначение значения
					cout << "значение ";
					// Выводим содержимое значения
					::valued(reader.value());
				} break;
				// Если событием является начало перечня значений
				case static_cast <uint8_t> (codec::toml::event_t::ARRAY_OPEN): cout << "начало перечня"; break;
				// Если событием является конец перечня значений
				case static_cast <uint8_t> (codec::toml::event_t::ARRAY_CLOSE): cout << "конец перечня"; break;
				// Если событием является начало встроенной таблицы
				case static_cast <uint8_t> (codec::toml::event_t::INLINE_OPEN): cout << "начало встроенной таблицы"; break;
				// Если событием является конец встроенной таблицы
				case static_cast <uint8_t> (codec::toml::event_t::INLINE_CLOSE): cout << "конец встроенной таблицы"; break;
				// Если событием является примечание
				case static_cast <uint8_t> (codec::toml::event_t::COMMENT): {
					// Выводим обозначение примечания
					cout << (reader.comment().trailing ? "примечание в конце строки " : "примечание ");
					// Выводим содержимое примечания
					cout << reader.comment().text;
				} break;
				// Если событием является пустая строка
				case static_cast <uint8_t> (codec::toml::event_t::BLANK): cout << "пустая строка"; break;
			}
			// Выводим знак конца строки
			cout << endl;
		}
	// Выполняем подачу до исчерпания разбираемого текста
	} while(offset < text.size());
	/**
	 * Если разбор текста настроек завершился ошибкой
	 */
	if(reader.error() != codec::toml::error_t::NONE)
		// Выводим сведения об обнаруженной ошибке разбора
		cout << "ошибка: " << codec::toml::message(reader.error())
		     << " в " << reader.errorLocation().line << ":" << reader.errorLocation().column << endl;
	// Выводим название определённой кодировки исходного текста
	else cout << "кодировка: " << codec::toml::name(reader.encoding()) << endl;
	// Выводим код успешного выхода из приложения
	return EXIT_SUCCESS;
}
