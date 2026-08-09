/**
 * @file: ini-reader.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример потокового чтения текста настроек INI — разбор одного и того же текста
 *        разными наречиями, подача текста кусками и разбор значений числами
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
#include <codec/ini/reader.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция вывода потока событий разбора текста настроек
 *
 * @param title    название разбираемого наречия записи
 * @param text     разбираемый текст настроек
 * @param settings настройки разбора текста настроек
 * @param step     размер куска подаваемого текста, нулевой для подачи целиком
 *
 */
static void parse(const string & title, const string & text, const codec::ini::reader_t::settings_t & settings, const size_t step = 0) noexcept {
	// Создаём объект потокового чтения текста настроек
	codec::ini::reader_t reader(settings);
	// Выводим название разбираемого наречия записи
	cout << "== " << title << " ==" << endl;
	// Положение начала очередного куска подаваемого текста
	size_t offset = 0;
	/**
	 * Выполняем подачу текста настроек до его исчерпания
	 */
	while(offset <= text.size()){
		// Получаем размер очередного куска подаваемого текста
		const size_t size = ((step == 0) ? text.size() : ((offset + step) > text.size() ? (text.size() - offset) : step));
		/**
		 * Если передачу очередного куска выполнить не удалось
		 */
		if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size())))
			// Выполняем прекращение подачи текста настроек
			break;
		/**
		 * Выполняем перебор всех накопленных событий разбора
		 */
		while(reader.next()){
			/**
			 * Определяем вид текущего события разбора
			 */
			switch(static_cast <uint8_t> (reader.event())){
				// Если событием является объявление раздела
				case static_cast <uint8_t> (codec::ini::event_t::SECTION): {
					// Выводим имя объявленного раздела
					cout << "раздел: " << reader.section().section;
					/**
					 * Если раздел несёт имя подраздела
					 */
					if(!reader.section().subsection.empty())
						// Выводим имя объявленного подраздела
						cout << " / " << reader.section().subsection;
					// Выводим знак конца строки
					cout << endl;
				} break;
				// Если событием является свойство со значением
				case static_cast <uint8_t> (codec::ini::event_t::PROPERTY): {
					// Выводим имя и значение прочитанного свойства
					cout << "  " << reader.key() << " = «" << reader.text() << "»";
					/**
					 * Если свойство записано без разделителя и значения
					 */
					if(reader.property().valueless)
						// Выводим признак свойства без значения
						cout << " (без значения)";
					// Выводим знак конца строки
					cout << endl;
				} break;
				// Если событием является примечание
				case static_cast <uint8_t> (codec::ini::event_t::COMMENT):
					// Выводим содержимое прочитанного примечания
					cout << "  примечание: " << reader.text() << endl;
				break;
			}
		}
		/**
		 * Если исходный текст настроек исчерпан
		 */
		if((offset + size) >= text.size())
			// Выполняем прекращение подачи текста настроек
			break;
		// Выполняем переход к следующему куску подаваемого текста
		offset += size;
	}
	/**
	 * Если разбор прекращён ошибкой
	 */
	if(reader.state() == codec::ini::state_t::FAILED)
		// Выводим сведения об обнаруженной ошибке разбора
		cout << "ошибка: " << codec::ini::message(reader.error())
		     << " в строке " << reader.errorLocation().line
		     << ", знак " << reader.errorLocation().column << endl;
	// Выводим знак конца строки
	cout << endl;
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
	/**
	 * Разбираемый текст настроек
	 *
	 * @note Текст этот намеренно построен так, чтобы наречия прочли его по-разному:
	 *       точка с запятой внутри значения у одних начинает примечание, у других
	 *       остаётся частью пути
	 */
	const string text =
		"; настройки приложения\n"
		"[paths]\n"
		"search = /opt/bin;/usr/local/bin ; перечень путей\n"
		"\n"
		"[server]\n"
		"host = 127.0.0.1\n"
		"port = 8080\n";
	// Выполняем разбор текста настроек наречием по умолчанию
	::parse("наречие по умолчанию", text, codec::ini::reader_t::settings_t());
	/**
	 * Выполняем разбор текста настроек наречием настроек Git
	 *
	 * @note Наречие это примечание в конце строки признаёт, и значение свойства
	 *       обрывается на точке с запятой
	 */
	::parse("наречие настроек Git", text, codec::ini::reader_t::settings_t::git());
	/**
	 * Выполняем разбор текста настроек наречием MS Windows
	 *
	 * @note Наречие это примечания в конце строки не признаёт, и точка с запятой
	 *       остаётся разделителем путей внутри значения
	 */
	::parse("наречие MS Windows", text, codec::ini::reader_t::settings_t::windows());
	/**
	 * Выполняем разбор текста настроек с подачей его кусками по три байта
	 *
	 * @note Поток событий от нарезки исходного текста не зависит: разрыв куска
	 *       допустим в любом месте, в том числе посреди имени либо значения
	 */
	::parse("подача кусками по три байта", text, codec::ini::reader_t::settings_t(), 3);
	// Создаём объект потокового чтения текста настроек
	codec::ini::reader_t reader;
	/**
	 * Если передачу текста настроек выполнить удалось
	 */
	if(reader.feed("[server]\nport = 8080\nsecure = on\n")){
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если событием является свойство со значением
			 */
			if(reader.event() == codec::ini::event_t::PROPERTY){
				// Значение номера порта
				uint16_t port = 0;
				// Признак работы через защищённое соединение
				bool secure = false;
				/**
				 * Если значение свойства разобрано номером порта
				 */
				if(reader.value(port))
					// Выводим разобранный номер порта
					cout << "номер порта числом: " << port << endl;
				/**
				 * Если значение свойства разобрано логическим значением
				 */
				else if(reader.value(secure))
					// Выводим разобранный признак защищённого соединения
					cout << "защищённое соединение: " << (secure ? "да" : "нет") << endl;
			}
		}
	}
	// Выводим код успешного выхода из приложения
	return EXIT_SUCCESS;
}
