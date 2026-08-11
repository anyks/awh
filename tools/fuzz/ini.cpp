/**
 * @file: ini.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Инструмент фаззинга кодека текста настроек INI — построение полуструктурированного
 *        текста настроек всех признаваемых наречий с точечной порчей, подача его чтению
 *        целиком и кусками произвольного размера, сборка дерева и его перезапись для поиска
 *        аварийных завершений, выходов за границы буфера и расхождений разбора
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include <codec/ini/ini.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён контейнера настроек
 */
using namespace awh::codec;

/**
 * @brief Внутренние вспомогательные средства генератора (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Учёт проделанной работы
	 *
	 */
	struct Statistic {
		// Количество построенных текстов настроек
		uint64_t texts;
		// Количество испорченных текстов настроек
		uint64_t corrupted;
		// Количество текстов, разобранных чтением до конца
		uint64_t survived;
		// Количество выданных чтением событий
		uint64_t events;
		// Количество собранных деревьев настроек
		uint64_t trees;
		// Количество перезаписей дерева настроек
		uint64_t rewrites;
		/**
		 * @brief Конструктор
		 *
		 */
		Statistic() noexcept :
		 texts(0), corrupted(0), survived(0),
		 events(0), trees(0), rewrites(0) {}
	/**
	 * Учёт проделанной работы
	 *
	 * @note Имя «stat» тут не годится: у MinGW заголовки объявляют «struct stat»
	 *       в области видимости, и обращение к учёту становится двусмысленным
	 */
	} totals;

	/**
	 * @brief Событие разбора, запомненное для сличения
	 *
	 */
	struct Event {
		// Разновидность события
		uint8_t event;
		// Имя раздела и имя подраздела
		string section, subsection;
		// Имя свойства и его значение
		string key, value;
		// Содержимое примечания
		string comment;
		// Признаки свойства
		bool quoted, valueless, append;
		/**
		 * @brief Оператор сравнения
		 *
		 * @param other событие для сравнения
		 * @return      результат сравнения
		 *
		 */
		bool operator != (const Event & other) const noexcept {
			// Выполняем сличение всех признаков события
			return (
				(this->event != other.event) ||
				(this->section != other.section) ||
				(this->subsection != other.subsection) ||
				(this->key != other.key) ||
				(this->value != other.value) ||
				(this->comment != other.comment) ||
				(this->quoted != other.quoted) ||
				(this->valueless != other.valueless) ||
				(this->append != other.append)
			);
		}
	};

	/**
	 * @brief Метод выборки наречия записи по его порядковому номеру
	 *
	 * @param index порядковый номер наречия
	 * @return      настройки разбора выбранного наречия
	 *
	 */
	ini::reader_t::settings_t dialect(const uint32_t index) noexcept {
		/**
		 * Выполняем выборку наречия записи
		 */
		switch(index % 5){
			// Выполняем выборку наречия MS Windows
			case 0: return ini::reader_t::settings_t::windows();
			// Выполняем выборку наречия языка Python
			case 1: return ini::reader_t::settings_t::python();
			// Выполняем выборку наречия системы инициализации systemd
			case 2: return ini::reader_t::settings_t::systemd();
			// Выполняем выборку наречия системы хранения версий Git
			case 3: return ini::reader_t::settings_t::git();
		}
		// Выполняем выборку строгого наречия
		return ini::reader_t::settings_t::strict();
	}

	/**
	 * @brief Метод построения полуструктурированного текста настроек
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       построенный текст настроек
	 *
	 */
	string generate(mt19937 & engine) noexcept {
		// Результат работы функции - построенный текст настроек
		string result;
		// Количество записей в тексте настроек
		const uint32_t count = (engine() % 24);
		/**
		 * Выполняем построение записей текста настроек
		 */
		for(uint32_t i = 0; i < count; i++){
			/**
			 * Выполняем выборку разновидности записи
			 */
			switch(engine() % 10){
				// Выполняем построение объявления раздела
				case 0:
				case 1: {
					// Дописываем отступ перед объявлением раздела
					result.append(engine() % 4, ' ');
					// Дописываем знак начала объявления раздела
					result.push_back('[');
					// Дописываем имя раздела
					result.append(1, static_cast <char> ('a' + (engine() % 4)));
					/**
					 * Если требуется дописать имя подраздела
					 */
					if((engine() % 3) == 0){
						/**
						 * Выполняем выборку записи имени подраздела
						 */
						switch(engine() % 3){
							// Дописываем имя подраздела через разделитель
							case 0: result.append(".sub"); break;
							// Дописываем имя подраздела в кавычках
							case 1: result.append(" \"sub\""); break;
							// Дописываем имя подраздела с превышением глубины
							case 2: result.append(".a.b.c.d"); break;
						}
					}
					// Дописываем знак завершения объявления раздела
					result.push_back(']');
					/**
					 * Если требуется дописать примечание конца строки
					 */
					if((engine() % 4) == 0)
						// Дописываем примечание конца строки
						result.append(" ; хвост [не раздел]");
				} break;
				// Выполняем построение свойства со значением
				case 2:
				case 3:
				case 4:
				case 5: {
					// Дописываем отступ перед именем свойства
					result.append(engine() % 3, ' ');
					// Дописываем имя свойства
					result.append(1, static_cast <char> ('k' + (engine() % 4)));
					/**
					 * Если требуется дописать признак добавления к перечню
					 */
					if((engine() % 8) == 0)
						// Дописываем признак добавления к перечню
						result.append("[]");
					// Дописываем знак разделителя имени и значения
					result.push_back((engine() % 4) == 0 ? ':' : '=');
					// Дописываем отступ перед значением свойства
					result.append(engine() % 3, ' ');
					/**
					 * Выполняем выборку записи значения свойства
					 */
					switch(engine() % 10){
						// Дописываем простое значение
						case 0: result.append("value"); break;
						// Дописываем значение в кавычках
						case 1: result.append("\" quoted ; value \""); break;
						// Дописываем значение с управляющими последовательностями
						case 2: result.append("a\\n\\t\\x41\\u0416b"); break;
						// Дописываем значение с незакрытой кавычкой
						case 3: result.append("\"unbalanced"); break;
						// Дописываем значение с обращением к другому значению
						case 4: result.append("${a:kk} ${self} $$"); break;
						// Дописываем значение с примечанием конца строки
						case 5: result.append("value ; примечание"); break;
						// Дописываем значение со знаками не из набора ASCII
						case 6: result.append("значение"); break;
						// Дописываем значение с недопустимой последовательностью UTF-8
						case 7: result.append("bad\xC3\x28 tail"); break;
						// Дописываем длинное значение
						case 8: result.append(200, 'x'); break;
						// Дописываем пустое значение
						case 9: break;
					}
					/**
					 * Если требуется дописать строки продолжения значения
					 */
					if((engine() % 6) == 0){
						// Количество строк продолжения значения
						const uint32_t lines = (1 + (engine() % 3));
						/**
						 * Выполняем дозапись строк продолжения значения
						 */
						for(uint32_t j = 0; j < lines; j++){
							/**
							 * Выполняем выборку записи строки продолжения
							 */
							switch(engine() % 2){
								// Дописываем продолжение обратной косой чертой
								case 0: result.append("\\\n more"); break;
								// Дописываем продолжение отступом
								case 1: result.append("\n\t more"); break;
							}
						}
					}
				} break;
				// Выполняем построение свойства без значения
				case 6: {
					// Дописываем имя свойства без разделителя и значения
					result.append(1, static_cast <char> ('k' + (engine() % 4)));
				} break;
				// Выполняем построение примечания
				case 7: {
					// Дописываем отступ перед примечанием
					result.append(engine() % 3, ' ');
					// Дописываем знак начала примечания
					result.push_back((engine() % 2) == 0 ? ';' : '#');
					// Дописываем содержимое примечания
					result.append(" примечание [a] k = v");
				} break;
				// Выполняем построение пустой строки
				case 8: break;
				// Выполняем построение произвольного набора знаков
				case 9: {
					// Количество знаков произвольного набора
					const uint32_t length = (engine() % 12);
					/**
					 * Выполняем дозапись знаков произвольного набора
					 */
					for(uint32_t j = 0; j < length; j++)
						// Дописываем произвольный знак
						result.push_back(static_cast <char> (engine() % 256));
				} break;
			}
			// Дописываем знак завершения строки
			result.append(((engine() % 5) == 0) ? "\r\n" : "\n");
		}
		/**
		 * Если требуется дописать метку порядка байтов
		 */
		if((engine() % 12) == 0)
			// Дописываем метку порядка байтов в начало текста настроек
			result.insert(0, "\xEF\xBB\xBF");
		// Выводим построенный текст настроек
		return result;
	}

	/**
	 * @brief Метод порчи построенного текста настроек
	 *
	 * @param text   текст настроек для порчи
	 * @param engine источник псевдослучайных чисел
	 *
	 */
	void corrupt(string & text, mt19937 & engine) noexcept {
		// Если текст настроек пустой, то порчу не выполняем
		if(text.empty())
			// Выходим из функции
			return;
		// Количество вносимых искажений
		const uint32_t count = (1 + (engine() % 4));
		/**
		 * Выполняем внесение искажений в текст настроек
		 */
		for(uint32_t i = 0; i < count; i++){
			// Разновидность вносимого искажения
			const uint32_t kind = (engine() % 4);
			/**
			 * Положение вносимого искажения
			 *
			 * @note Обращения к источнику разнесены по отдельным предложениям
			 *       намеренно: порядок вычисления доводов одного выражения языком
			 *       не определён, и два обращения в одном выражении давали бы разную
			 *       последовательность у разных построителей. Прогон обязан
			 *       воспроизводиться не только на своей машине
			 */
			const size_t place = (engine() % text.length());
			// Знак, вносимый искажением
			const char letter = static_cast <char> (engine() % 256);
			/**
			 * Выполняем выборку разновидности искажения
			 */
			switch(kind){
				// Выполняем замену произвольного знака
				case 0: text.at(place) = letter; break;
				// Выполняем обрезание текста настроек
				case 1: text.resize(place); break;
				// Выполняем вставку произвольного знака
				case 2: text.insert(place, 1, letter); break;
				// Выполняем удаление произвольного знака
				case 3: text.erase(place, 1); break;
			}
			// Если текст настроек опустел, то порчу прекращаем
			if(text.empty())
				// Выходим из цикла внесения искажений
				break;
		}
	}

	/**
	 * @brief Метод разбора текста настроек с запоминанием выданных событий
	 *
	 * @param text     разбираемый текст настроек
	 * @param settings настройки разбора текста настроек
	 * @param chunk    размер куска подачи, ноль - подача текста целиком
	 * @param events   собираемый перечень выданных событий
	 * @return         состояние чтения по завершении разбора
	 *
	 */
	ini::state_t consume(const string & text, const ini::reader_t::settings_t & settings, const size_t chunk, vector <Event> & events) noexcept {
		// Создаём объект чтения текста настроек
		ini::reader_t reader(settings);
		// Размер куска подачи текста настроек
		const size_t size = (chunk > 0 ? chunk : text.length());
		// Смещение начала очередного куска подачи
		size_t offset = 0;
		/**
		 * Выполняем подачу текста настроек кусками
		 */
		do {
			// Размер подаваемого куска текста настроек
			const size_t length = ((text.length() - offset) < size ? (text.length() - offset) : size);
			// Признак того, что подаётся последний кусок текста настроек
			const bool end = ((offset + length) >= text.length());
			// Если подача куска текста настроек не удалась
			if(!reader.feed(text.data() + offset, length, end))
				// Выходим из цикла подачи текста настроек
				break;
			// Выполняем смещение начала очередного куска подачи
			offset += length;
			/**
			 * Выполняем чтение выданных разбором событий
			 */
			while(reader.next()){
				// Создаём запоминаемое событие разбора
				Event event;
				// Запоминаем разновидность события
				event.event = static_cast <uint8_t> (reader.event());
				// Запоминаем имя раздела
				event.section.assign(reader.section().section);
				// Запоминаем имя подраздела
				event.subsection.assign(reader.section().subsection);
				// Запоминаем имя свойства
				event.key.assign(reader.property().key);
				// Запоминаем значение свойства
				event.value.assign(reader.property().value);
				// Запоминаем содержимое примечания
				event.comment.assign(reader.comment().text);
				// Запоминаем признак заключения значения в кавычки
				event.quoted = reader.property().quoted;
				// Запоминаем признак записи свойства без значения
				event.valueless = reader.property().valueless;
				// Запоминаем признак добавления значения к перечню
				event.append = reader.property().append;
				// Выполняем сохранение события в перечне
				events.push_back(::move(event));
				// Выполняем учёт выданного разбором события
				totals.events++;
			}
			// Если разбор прекращён ошибкой
			if(reader.state() == ini::state_t::FAILED)
				// Выходим из цикла подачи текста настроек
				break;
		// Выполняем подачу до исчерпания текста настроек
		} while(offset < text.length());
		// Выводим состояние чтения по завершении разбора
		return reader.state();
	}

	/**
	 * @brief Метод вывода разбираемого текста настроек в шестнадцатеричном виде
	 *
	 * @param text выводимый текст настроек
	 *
	 */
	void dump(const string & text) noexcept {
		// Выводим начало записи разбираемого текста настроек
		::fprintf(stderr, "ini fuzz: text=\"");
		/**
		 * Выполняем перебор всех знаков разбираемого текста настроек
		 */
		for(size_t i = 0; i < text.length(); i++)
			// Выводим очередной знак разбираемого текста настроек
			::fprintf(stderr, "\\x%02X", static_cast <uint8_t> (text.at(i)));
		// Выводим завершение записи разбираемого текста настроек
		::fprintf(stderr, "\"\n");
	}

	/**
	 * @brief Метод сличения перечней выданных разбором событий
	 *
	 * @param first  перечень событий подачи текста целиком
	 * @param second перечень событий подачи текста кусками
	 * @param chunk  размер куска подачи текста настроек
	 * @param text   разбираемый текст настроек
	 * @return       результат сличения перечней
	 *
	 */
	bool compare(const vector <Event> & first, const vector <Event> & second, const size_t chunk, const string & text) noexcept {

		// Если количество выданных событий разошлось
		if(first.size() != second.size()){
			// Выводим сообщение о расхождении количества выданных событий
			::fprintf(stderr, "ini fuzz: chunk=%zu events %zu != %zu\n", chunk, first.size(), second.size());
			// Выводим разбираемый текст настроек
			dump(text);
			// Выводим результат сличения перечней
			return false;
		}
		/**
		 * Выполняем перебор всех выданных разбором событий
		 */
		for(size_t i = 0; i < first.size(); i++){
			// Если очередное событие разошлось
			if(first.at(i) != second.at(i)){
				// Выводим сообщение о расхождении выданного события
				::fprintf(stderr, "ini fuzz: chunk=%zu event %zu differs\n", chunk, i);
				// Выводим разбираемый текст настроек
				dump(text);
				// Выводим результат сличения перечней
				return false;
			}
		}
		// Выводим результат сличения перечней
		return true;
	}

	/**
	 * @brief Метод проверки дерева настроек на построенном тексте
	 *
	 * @param text     разбираемый текст настроек
	 * @param settings настройки разбора текста настроек
	 * @param engine   источник псевдослучайных чисел
	 * @return         результат проверки дерева настроек
	 *
	 */
	bool tree(const string & text, const ini::reader_t::settings_t & reading, mt19937 & engine) noexcept {
		// Собираемые настройки дерева настроек
		ini::document_t::settings_t settings;
		// Устанавливаем настройки разбора текста настроек
		settings.reader = reading;
		// Создаём дерево настроек
		ini::document_t document(settings);
		// Выполняем учёт собранного дерева настроек
		totals.trees++;
		// Если разбор текста настроек не удался, то проверку прекращаем
		if(!document.parse(text))
			// Выводим результат проверки дерева настроек
			return true;
		/**
		 * Выполняем перебор всех разделов дерева настроек
		 */
		for(auto & section : document.sections()){
			/**
			 * Выполняем перебор всех свойств раздела
			 */
			for(auto & key : document.keys(section.section, section.subsection)){
				// Выполняем обращение к значению свойства
				const string value(document.get(key, section.section, section.subsection));
				// Выполняем обращение к числовому виду значения свойства
				int64_t number = 0;
				// Выполняем чтение числового вида значения свойства
				document.value(number, key, section.section, section.subsection);
				// Выполняем обращение к признаку наличия свойства
				document.has(key, section.section, section.subsection);
			}
		}
		// Выполняем перезапись дерева настроек
		const string first = document.text();
		// Выполняем учёт перезаписи дерева настроек
		totals.rewrites++;
		// Если перезапись дерева настроек не удалась, то проверку прекращаем
		if(first.empty())
			// Выводим результат проверки дерева настроек
			return true;
		// Создаём дерево настроек для повторного разбора
		ini::document_t repeat(settings);
		// Если повторный разбор перезаписанного текста не удался
		if(!repeat.parse(first)){
			// Выводим сообщение об отказе повторного разбора перезаписанного текста
			::fprintf(stderr, "ini fuzz: reparse failed, text=[%s], written=[%s]\n", text.c_str(), first.c_str());
			// Выводим результат проверки дерева настроек
			return false;
		}
		// Выполняем повторную перезапись дерева настроек
		const string second = repeat.text();
		// Если повторная перезапись разошлась с первой
		if(second != first){
			// Выводим сообщение о расхождении повторной перезаписи
			::fprintf(stderr, "ini fuzz: rewrite unstable\n");
			// Выводим первую перезапись дерева настроек
			dump(first);
			// Выводим повторную перезапись дерева настроек
			dump(second);
			// Выводим результат проверки дерева настроек
			return false;
		}
		// Количество вносимых в дерево настроек правок
		const uint32_t count = (engine() % 8);
		/**
		 * Выполняем внесение правок в дерево настроек
		 */
		for(uint32_t i = 0; i < count; i++){
			// Имя правимого раздела
			const string section(1, static_cast <char> ('a' + (engine() % 4)));
			// Имя правимого подраздела
			const string subsection(((engine() % 3) == 0) ? "sub" : "");
			// Имя правимого свойства
			const string key(1, static_cast <char> ('k' + (engine() % 4)));
			/**
			 * Выполняем выборку разновидности правки
			 */
			switch(engine() % 5){
				// Выполняем создание раздела
				case 0: document.create(section, subsection); break;
				// Выполняем установку значения свойства
				case 1: document.set(key, "value", section, subsection); break;
				// Выполняем установку значения свойства со знаками записи
				case 2: document.set(key, " a ; b \" c \\ d ", section, subsection); break;
				// Выполняем удаление свойства
				case 3: document.erase(key, section, subsection); break;
				// Выполняем удаление раздела
				case 4: document.remove(section, subsection); break;
			}
		}
		// Выполняем перезапись правленого дерева настроек
		const string edited = document.text();
		// Выполняем учёт перезаписи дерева настроек
		totals.rewrites++;
		// Если перезапись правленого дерева настроек не удалась, то проверку прекращаем
		if(edited.empty())
			// Выводим результат проверки дерева настроек
			return true;
		// Создаём дерево настроек для разбора правленого текста
		ini::document_t rebuilt(settings);
		// Если разбор правленого текста настроек не удался
		if(!rebuilt.parse(edited)){
			// Выводим сообщение об отказе разбора правленого текста настроек
			::fprintf(stderr, "ini fuzz: edited reparse failed, written=[%s]\n", edited.c_str());
			// Выводим результат проверки дерева настроек
			return false;
		}
		// Если перезапись правленого дерева настроек оказалась неустойчивой
		if(rebuilt.text() != edited){
			// Выводим сообщение о расхождении перезаписи правленого дерева настроек
			::fprintf(stderr, "ini fuzz: edited rewrite unstable\n");
			// Выводим перезапись правленого дерева настроек
			dump(edited);
			// Выводим повторную перезапись правленого дерева настроек
			dump(rebuilt.text());
			// Выводим результат проверки дерева настроек
			return false;
		}
		// Выводим результат проверки дерева настроек
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
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Количество выполняемых проходов генератора
	uint64_t count = 3000;
	// Если количество проходов задано параметром командной строки
	if(argc > 1)
		// Выполняем чтение количества проходов из параметра командной строки
		count = static_cast <uint64_t> (::strtoull(argv[1], nullptr, 10));
	// Создаём источник псевдослучайных чисел с закреплённым зерном
	mt19937 engine(0x5A1CE);
	/**
	 * Выполняем проходы генератора
	 */
	for(uint64_t i = 0; i < count; i++){
		// Выполняем выборку наречия записи текста настроек
		const ini::reader_t::settings_t settings = dialect(static_cast <uint32_t> (engine()));
		// Выполняем построение текста настроек
		string text = generate(engine);
		// Выполняем учёт построенного текста настроек
		totals.texts++;
		// Если требуется испортить построенный текст настроек
		if((engine() % 3) == 0){
			// Выполняем порчу построенного текста настроек
			corrupt(text, engine);
			// Выполняем учёт испорченного текста настроек
			totals.corrupted++;
		}
		// Перечень событий подачи текста настроек целиком
		vector <Event> whole;
		// Выполняем подачу текста настроек целиком
		const ini::state_t state = consume(text, settings, 0, whole);
		// Если текст настроек разобран до конца
		if(state == ini::state_t::FINISHED)
			// Выполняем учёт разобранного до конца текста настроек
			totals.survived++;
		// Размер куска подачи текста настроек
		const size_t chunk = (1 + (engine() % 16));
		// Перечень событий подачи текста настроек кусками
		vector <Event> chunked;
		// Выполняем подачу текста настроек кусками
		consume(text, settings, chunk, chunked);
		// Если перечни выданных разбором событий разошлись
		if(!compare(whole, chunked, chunk, text))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		// Если проверка дерева настроек не удалась
		if(!tree(text, settings, engine))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
	}
	// Выводим статистику работы генератора
	::fprintf(
		stdout,
		"ini fuzz: %llu texts (%llu corrupted), %llu events, %llu parsed to the end, %llu trees, %llu rewrites\n",
		static_cast <unsigned long long> (totals.texts),
		static_cast <unsigned long long> (totals.corrupted),
		static_cast <unsigned long long> (totals.events),
		static_cast <unsigned long long> (totals.survived),
		static_cast <unsigned long long> (totals.trees),
		static_cast <unsigned long long> (totals.rewrites)
	);
	// Выходим из приложения
	return EXIT_SUCCESS;
}
