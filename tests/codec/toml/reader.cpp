/**
 * @file: reader.cpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверки потокового чтения текста настроек TOML — разбор таблиц, пар, строк
 *        всех четырёх записей, чисел, отметок времени, перечней и встроенных таблиц,
 *        а также независимость выдачи от нарезки текста на куски
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/toml/toml.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Событие разбора, запомненное для сличения
 *
 */
struct Event {
	// Разновидность события и тип значения
	uint8_t event, type;
	// Собранное имя ключа события
	string path;
	// Содержимое строкового значения либо примечания
	string text;
	// Целое число значения
	int64_t integer;
	// Логическое значение
	bool boolean;
	// Место события в исходном тексте
	uint32_t line, column;
	/**
	 * @brief Конструктор
	 *
	 */
	Event() noexcept : event(0), type(0), integer(0), boolean(false), line(0), column(0) {}
};

/**
 * @brief Метод разбора текста настроек кусками заданного размера
 *
 * @param text     разбираемый текст настроек
 * @param chunk    размер куска подачи, ноль означает подачу целиком
 * @param settings настройки разбора текста настроек
 * @param events   собранные события разбора
 * @return         состояние разбора по его завершении
 *
 */
static toml::state_t consume(const string & text, const size_t chunk, const toml::reader_t::settings_t & settings, vector <Event> & events) noexcept {
	// Объект потокового чтения текста настроек
	toml::reader_t reader(settings);
	// Позиция чтения разбираемого текста
	size_t offset = 0;
	/**
	 * Выполняем подачу разбираемого текста кусками
	 */
	do {
		// Получаем размер очередного куска подачи
		const size_t length = ((chunk == 0) ? text.size() : min(chunk, (text.size() - offset)));
		// Получаем признак того, что кусок является последним
		const bool end = ((offset + length) >= text.size());
		/**
		 * Если подача очередного куска не удалась
		 */
		if(!reader.feed(text.data() + offset, length, end))
			// Выводим состояние разбора текста настроек
			return reader.state();
		// Выполняем переход к следующему куску подачи
		offset += length;
		/**
		 * Выполняем перебор выданных разбором событий
		 */
		while(reader.next()){
			// Собираемое событие разбора
			Event item;
			// Запоминаем разновидность события
			item.event = static_cast <uint8_t> (reader.event());
			// Запоминаем тип значения события
			item.type = static_cast <uint8_t> (reader.value().type);
			/**
			 * Выполняем перебор составных частей имени ключа события
			 */
			for(auto & part : reader.path()){
				/**
				 * Если часть имени не первая
				 */
				if(!item.path.empty())
					// Выполняем добавление разделителя составных частей имени
					item.path.push_back('.');
				// Выполняем добавление составной части имени
				item.path.append(part.name);
			}
			/**
			 * Если событием является примечание
			 */
			if(reader.event() == toml::event_t::COMMENT)
				// Запоминаем содержимое примечания
				item.text.assign(reader.comment().text);
			// Запоминаем содержимое строкового значения
			else item.text.assign(reader.value().text);
			// Запоминаем целое число значения
			item.integer = reader.value().integer;
			// Запоминаем логическое значение
			item.boolean = reader.value().boolean;
			// Запоминаем номер строки события
			item.line = reader.location().line;
			// Запоминаем положение события в строке
			item.column = reader.location().column;
			// Выполняем добавление собранного события
			events.push_back(item);
		}
		/**
		 * Если кусок оказался последним
		 */
		if(end)
			// Выходим из цикла подачи разбираемого текста
			break;
	} while(offset <= text.size());
	// Выводим состояние разбора текста настроек
	return reader.state();
}

/**
 * @brief Проверка разбора простейшего текста настроек
 *
 */
TEST(CodecTomlReader, Simple) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Выполняем признание знаков Юникода в именах без кавычек
	settings.unicode = true;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_EQ(consume("[сервер]\nадрес = \"127.0.0.1\"\nпорт = 8080\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку количества выданных событий
	ASSERT_EQ(events.size(), 5u);
	// Выполняем проверку события объявления таблицы
	ASSERT_EQ(events.at(0).event, static_cast <uint8_t> (toml::event_t::TABLE));
	// Выполняем проверку имени объявленной таблицы
	ASSERT_EQ(events.at(0).path, "сервер");
	// Выполняем проверку события имени первого ключа
	ASSERT_EQ(events.at(1).event, static_cast <uint8_t> (toml::event_t::KEY));
	// Выполняем проверку имени первого ключа
	ASSERT_EQ(events.at(1).path, "адрес");
	// Выполняем проверку события значения первого ключа
	ASSERT_EQ(events.at(2).event, static_cast <uint8_t> (toml::event_t::VALUE));
	// Выполняем проверку типа значения первого ключа
	ASSERT_EQ(events.at(2).type, static_cast <uint8_t> (toml::type_t::STRING));
	// Выполняем проверку содержимого значения первого ключа
	ASSERT_EQ(events.at(2).text, "127.0.0.1");
	// Выполняем проверку имени второго ключа
	ASSERT_EQ(events.at(3).path, "порт");
	// Выполняем проверку типа значения второго ключа
	ASSERT_EQ(events.at(4).type, static_cast <uint8_t> (toml::type_t::INTEGER));
	// Выполняем проверку значения второго ключа
	ASSERT_EQ(events.at(4).integer, 8080);
}
/**
 * @brief Проверка разбора составного имени ключа
 *
 */
TEST(CodecTomlReader, DottedKeys) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_EQ(consume("a.b.\"c d\" = 1\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку количества выданных событий
	ASSERT_EQ(events.size(), 2u);
	// Выполняем проверку собранного имени ключа
	ASSERT_EQ(events.at(0).path, "a.b.c d");
	// Выполняем проверку значения ключа
	ASSERT_EQ(events.at(1).integer, 1);
}
/**
 * @brief Проверка разбора строк всех четырёх записей
 *
 */
TEST(CodecTomlReader, Strings) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Собранные события разбора
	vector <Event> events;
	// Разбираемый текст настроек со строками всех записей
	const string text =
		"basic = \"первая\\nвторая\\tтабуляция \\u0416\"\n"
		"literal = 'как \\n записано'\n"
		"multi = \"\"\"\nпервая\nвторая\"\"\"\n"
		"folded = \"\"\"один \\\n   два\"\"\"\n"
		"raw = '''\nдословно\\n'''\n";
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку количества выданных событий
	ASSERT_EQ(events.size(), 10u);
	// Выполняем проверку разбора управляющих последовательностей основной строки
	ASSERT_EQ(events.at(1).text, "первая\nвторая\tтабуляция Ж");
	// Выполняем проверку сохранности содержимого дословной строки
	ASSERT_EQ(events.at(3).text, "как \\n записано");
	// Выполняем проверку отбрасывания переноса за открывающими кавычками
	ASSERT_EQ(events.at(5).text, "первая\nвторая");
	// Выполняем проверку склейки строк обратной косой чертой в конце строки
	ASSERT_EQ(events.at(7).text, "один два");
	// Выполняем проверку сохранности содержимого многострочной дословной строки
	ASSERT_EQ(events.at(9).text, "дословно\\n");
}
/**
 * @brief Проверка разбора чисел
 *
 */
TEST(CodecTomlReader, Numbers) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Собранные события разбора
	vector <Event> events;
	// Разбираемый текст настроек с числами
	const string text = "a = 42\nb = -17\nc = 1_000_000\nd = 0xDEAD_beef\ne = 0o755\nf = 0b1010\n";
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку разбора десятичного числа
	ASSERT_EQ(events.at(1).integer, 42);
	// Выполняем проверку разбора отрицательного числа
	ASSERT_EQ(events.at(3).integer, -17);
	// Выполняем проверку разбора числа с разделителями разрядов
	ASSERT_EQ(events.at(5).integer, 1000000);
	// Выполняем проверку разбора шестнадцатеричного числа
	ASSERT_EQ(events.at(7).integer, 0xDEADBEEF);
	// Выполняем проверку разбора восьмеричного числа
	ASSERT_EQ(events.at(9).integer, 493);
	// Выполняем проверку разбора двоичного числа
	ASSERT_EQ(events.at(11).integer, 10);
	/**
	 * Выполняем проверку отказа разбора ошибочных записей чисел
	 */
	{
		// Ошибочные записи чисел
		const vector <string> refusals = {"a = 011\n", "a = 1__0\n", "a = _1\n", "a = 1_\n", "a = -0x1\n", "a = 0x\n"};
		/**
		 * Выполняем перебор ошибочных записей чисел
		 */
		for(auto & source : refusals){
			// Собранные события разбора
			vector <Event> refused;
			// Выполняем проверку отказа разбора ошибочной записи числа
			ASSERT_EQ(consume(source, 0, settings, refused), toml::state_t::FAILED) << source;
		}
	}
	/**
	 * Выполняем проверку отказа разбора числа за отрезком значений
	 */
	{
		// Собранные события разбора
		vector <Event> refused;
		// Объект потокового чтения текста настроек
		toml::reader_t reader;
		// Разбираемая запись числа за отрезком значений
		const string source = "a = 9223372036854775808\n";
		// Выполняем проверку отказа разбора записи числа
		ASSERT_FALSE(reader.feed(source.data(), source.size(), true));
		// Выполняем проверку выданного кода ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::NUMBER_OVERFLOW);
	}
}
/**
 * @brief Проверка разбора чисел с плавающей точкой и логических значений
 *
 */
TEST(CodecTomlReader, Floats) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Объект потокового чтения текста настроек
	toml::reader_t reader(settings);
	// Разбираемый текст настроек
	const string text = "a = 3.14\nb = -0.5e3\nc = inf\nd = -inf\ne = nan\nf = true\ng = false\n";
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
	// Собранные значения текста настроек
	vector <toml::value_t> values;
	/**
	 * Выполняем перебор выданных разбором событий
	 */
	while(reader.next()){
		/**
		 * Если событием является значение
		 */
		if(reader.event() == toml::event_t::VALUE)
			// Выполняем добавление собранного значения
			values.push_back(reader.value());
	}
	// Выполняем проверку количества собранных значений
	ASSERT_EQ(values.size(), 7u);
	// Выполняем проверку разбора числа с плавающей точкой
	ASSERT_DOUBLE_EQ(values.at(0).real, 3.14);
	// Выполняем проверку разбора числа с порядком
	ASSERT_DOUBLE_EQ(values.at(1).real, -500.0);
	// Выполняем проверку разбора бесконечности
	ASSERT_TRUE(::isinf(values.at(2).real) && (values.at(2).real > 0));
	// Выполняем проверку разбора отрицательной бесконечности
	ASSERT_TRUE(::isinf(values.at(3).real) && (values.at(3).real < 0));
	// Выполняем проверку разбора нечисла
	ASSERT_TRUE(::isnan(values.at(4).real));
	// Выполняем проверку разбора истины
	ASSERT_TRUE(values.at(5).boolean);
	// Выполняем проверку типа логического значения
	ASSERT_EQ(values.at(5).type, toml::type_t::BOOLEAN);
	// Выполняем проверку разбора лжи
	ASSERT_FALSE(values.at(6).boolean);
}
/**
 * @brief Проверка разбора отметок времени
 *
 */
TEST(CodecTomlReader, Stamps) {
	// Объект потокового чтения текста настроек
	toml::reader_t reader;
	// Разбираемый текст настроек с отметками времени
	const string text =
		"a = 1979-05-27T07:32:00Z\n"
		"b = 1979-05-27T00:32:00-07:00\n"
		"c = 1979-05-27 07:32:00.999999\n"
		"d = 1979-05-27\n"
		"e = 07:32:00\n";
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
	// Собранные значения текста настроек
	vector <toml::value_t> values;
	/**
	 * Выполняем перебор выданных разбором событий
	 */
	while(reader.next()){
		/**
		 * Если событием является значение
		 */
		if(reader.event() == toml::event_t::VALUE)
			// Выполняем добавление собранного значения
			values.push_back(reader.value());
	}
	// Выполняем проверку количества собранных значений
	ASSERT_EQ(values.size(), 5u);
	// Выполняем проверку типа отметки времени со смещением
	ASSERT_EQ(values.at(0).type, toml::type_t::OFFSET_DATETIME);
	// Выполняем проверку года отметки времени
	ASSERT_EQ(values.at(0).stamp.date.year, 1979);
	// Выполняем проверку признака записи часового пояса знаком «Z»
	ASSERT_TRUE(values.at(0).stamp.zulu);
	// Выполняем проверку смещения часового пояса отметки
	ASSERT_EQ(values.at(0).stamp.offset, 0);
	// Выполняем проверку смещения часового пояса второй отметки
	ASSERT_EQ(values.at(1).stamp.offset, -(7 * 60));
	// Выполняем проверку типа отметки времени без смещения
	ASSERT_EQ(values.at(2).type, toml::type_t::LOCAL_DATETIME);
	// Выполняем проверку признака разделения даты и времени пробелом
	ASSERT_TRUE(values.at(2).stamp.spaced);
	// Выполняем проверку доли секунды отметки времени
	ASSERT_EQ(values.at(2).stamp.time.nanosecond, 999999000u);
	// Выполняем проверку типа местной даты
	ASSERT_EQ(values.at(3).type, toml::type_t::LOCAL_DATE);
	// Выполняем проверку отсутствия смещения у местной даты
	ASSERT_EQ(values.at(3).stamp.offset, toml::NO_TIMEZONE);
	// Выполняем проверку типа местного времени
	ASSERT_EQ(values.at(4).type, toml::type_t::LOCAL_TIME);
	// Выполняем проверку часа местного времени
	ASSERT_EQ(values.at(4).stamp.time.hour, 7);
	/**
	 * Выполняем проверку отказа разбора несуществующих дат
	 */
	{
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Ошибочные записи отметок времени
		const vector <string> refusals = {"a = 2026-02-31\n", "a = 2026-13-01\n", "a = 25:00:00\n", "a = 2025-02-29\n"};
		/**
		 * Выполняем перебор ошибочных записей отметок времени
		 */
		for(auto & source : refusals){
			// Собранные события разбора
			vector <Event> refused;
			// Выполняем проверку отказа разбора ошибочной записи отметки
			ASSERT_EQ(consume(source, 0, settings, refused), toml::state_t::FAILED) << source;
		}
	}
	/**
	 * Выполняем проверку разбора двадцать девятого февраля года високосного
	 */
	{
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_EQ(consume("a = 2024-02-29\n", 0, settings, events), toml::state_t::FINISHED);
	}
}
/**
 * @brief Проверка разбора перечней и встроенных таблиц
 *
 */
TEST(CodecTomlReader, Collections) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Выполняем признание знаков Юникода в именах без кавычек
	settings.unicode = true;
	// Собранные события разбора
	vector <Event> events;
	// Разбираемый текст настроек с перечнями и встроенными таблицами
	const string text =
		"числа = [1, 2, 3]\n"
		"вложенный = [ [1], [\"два\"] ]\n"
		"многострочный = [\n  1, # примечание внутри\n  2,\n]\n"
		"точка = { x = 1, y = 2 }\n";
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	// Количество событий открытия перечня
	size_t opened = 0;
	// Количество событий закрытия перечня
	size_t closed = 0;
	/**
	 * Выполняем перебор собранных событий разбора
	 */
	for(auto & item : events){
		/**
		 * Если событием является открытие перечня
		 */
		if(item.event == static_cast <uint8_t> (toml::event_t::ARRAY_OPEN))
			// Выполняем увеличение количества открытий перечня
			opened++;
		/**
		 * Если событием является закрытие перечня
		 */
		if(item.event == static_cast <uint8_t> (toml::event_t::ARRAY_CLOSE))
			// Выполняем увеличение количества закрытий перечня
			closed++;
	}
	// Выполняем проверку количества открытий перечня
	ASSERT_EQ(opened, 5u);
	// Выполняем проверку совпадения количества открытий и закрытий
	ASSERT_EQ(opened, closed);
	/**
	 * Выполняем проверку отказа разбора запятой в конце встроенной таблицы
	 *
	 * @note Описание её запрещает: «{a = 1,}» записью встроенной таблицы не является
	 */
	{
		// Собранные события разбора
		vector <Event> refused;
		// Выполняем проверку отказа разбора встроенной таблицы
		ASSERT_EQ(consume("a = { b = 1, }\n", 0, settings, refused), toml::state_t::FAILED);
	}
	/**
	 * Выполняем проверку отказа разбора переноса внутри встроенной таблицы
	 */
	{
		// Собранные события разбора
		vector <Event> refused;
		// Выполняем проверку отказа разбора встроенной таблицы
		ASSERT_EQ(consume("a = { b = 1,\n c = 2 }\n", 0, settings, refused), toml::state_t::FAILED);
	}
}
/**
 * @brief Проверка обнаружения повторных объявлений
 *
 */
TEST(CodecTomlReader, Duplicates) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку отказа разбора повторного объявления ключа
	ASSERT_EQ(consume("a = 1\na = 2\n", 0, settings, events), toml::state_t::FAILED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку отказа разбора повторного объявления таблицы
	ASSERT_EQ(consume("[a]\n[a]\n", 0, settings, events), toml::state_t::FAILED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	/**
	 * Выполняем проверку того, что одинаковые ключи разных таблиц повтором не считаются
	 */
	ASSERT_EQ(consume("[a]\nk = 1\n[b]\nk = 2\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	/**
	 * Выполняем проверку того, что таблицы набора повтором не считаются
	 */
	ASSERT_EQ(consume("[[a]]\nk = 1\n[[a]]\nk = 2\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку отказа дополнения обычной таблицы набором таблиц
	ASSERT_EQ(consume("[a]\n[[a]]\n", 0, settings, events), toml::state_t::FAILED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем установку отмены проверки повторных объявлений
	settings.duplicates = false;
	// Выполняем проверку того, что повтор при отмене проверки разбор проходит
	ASSERT_EQ(consume("a = 1\na = 2\n", 0, settings, events), toml::state_t::FINISHED);
}
/**
 * @brief Проверка разбора примечаний и пустых строк
 *
 */
TEST(CodecTomlReader, Comments) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Выполняем установку выдачи событий пустых строк
	settings.emitBlanks = true;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_EQ(consume("# начало\n\na = 1 # хвост\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку количества выданных событий
	ASSERT_EQ(events.size(), 5u);
	// Выполняем проверку события примечания
	ASSERT_EQ(events.at(0).event, static_cast <uint8_t> (toml::event_t::COMMENT));
	// Выполняем проверку содержимого примечания
	ASSERT_EQ(events.at(0).text, "начало");
	// Выполняем проверку события пустой строки
	ASSERT_EQ(events.at(1).event, static_cast <uint8_t> (toml::event_t::BLANK));
	// Выполняем проверку события примечания в конце строки
	ASSERT_EQ(events.at(4).event, static_cast <uint8_t> (toml::event_t::COMMENT));
	// Выполняем проверку содержимого примечания в конце строки
	ASSERT_EQ(events.at(4).text, "хвост");
}
/**
 * @brief Проверка независимости выдачи разбора от нарезки текста на куски
 *
 * @details Договор этот у всякого потокового разбора AWH один, и нарушается он легче,
 * чем кажется: сличается вся выдача целиком - вид события, имя ключа, содержимое и
 * место каждого события
 *
 */
TEST(CodecTomlReader, ChunkIndependence) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Выполняем признание знаков Юникода в именах без кавычек
	settings.unicode = true;
	// Выполняем установку выдачи событий пустых строк
	settings.emitBlanks = true;
	// Проверяемые тексты настроек
	const vector <string> texts = {
		"[сервер]\nадрес = \"127.0.0.1\"\nпорт = 8080\n",
		"multi = \"\"\"\nпервая\nвторая\"\"\"\n# примечание\n",
		"числа = [\n  1,\n  2, # внутри\n]\nточка = { x = 1, y = 2 }\n",
		"a = 1979-05-27T07:32:00Z\nb = 1_000\nc = 'дословно'\n",
		"[[набор]]\nk = 1\n\n[[набор]]\nk = 2\n",
		"folded = \"\"\"один \\\n   два\"\"\"\n"
	};
	/**
	 * Выполняем перебор проверяемых текстов настроек
	 */
	for(auto & text : texts){
		// Собранные подачей целиком события разбора
		vector <Event> whole;
		// Выполняем разбор текста настроек подачей целиком
		const toml::state_t state = consume(text, 0, settings, whole);
		// Выполняем проверку того, что разбор текста настроек удался
		ASSERT_EQ(state, toml::state_t::FINISHED) << text;
		/**
		 * Выполняем перебор размеров куска подачи
		 */
		for(size_t chunk = 1; chunk <= 7; chunk++){
			// Собранные подачей кусками события разбора
			vector <Event> chunked;
			// Выполняем разбор текста настроек подачей кусками
			ASSERT_EQ(consume(text, chunk, settings, chunked), state) << text << " " << chunk;
			// Выполняем проверку совпадения количества выданных событий
			ASSERT_EQ(chunked.size(), whole.size()) << text << " " << chunk;
			/**
			 * Выполняем перебор выданных разбором событий
			 */
			for(size_t i = 0; i < whole.size(); i++){
				// Выполняем проверку совпадения разновидности события
				ASSERT_EQ(chunked.at(i).event, whole.at(i).event) << text << " " << chunk << " " << i;
				// Выполняем проверку совпадения имени ключа события
				ASSERT_EQ(chunked.at(i).path, whole.at(i).path) << text << " " << chunk << " " << i;
				// Выполняем проверку совпадения содержимого события
				ASSERT_EQ(chunked.at(i).text, whole.at(i).text) << text << " " << chunk << " " << i;
				// Выполняем проверку совпадения целого числа значения
				ASSERT_EQ(chunked.at(i).integer, whole.at(i).integer) << text << " " << chunk << " " << i;
				// Выполняем проверку совпадения номера строки события
				ASSERT_EQ(chunked.at(i).line, whole.at(i).line) << text << " " << chunk << " " << i;
				// Выполняем проверку совпадения положения события в строке
				ASSERT_EQ(chunked.at(i).column, whole.at(i).column) << text << " " << chunk << " " << i;
			}
		}
	}
}
/**
 * @brief Проверка пределов разбора текста настроек
 *
 */
TEST(CodecTomlReader, Limits) {
	/**
	 * Выполняем проверку предела глубины вложенности значений
	 */
	{
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Выполняем установку предела глубины вложенности значений
		settings.maxDepth = 2;
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку того, что разбор в пределах глубины удался
		ASSERT_EQ(consume("a = [[1]]\n", 0, settings, events), toml::state_t::FINISHED);
		// Выполняем очистку собранных событий разбора
		events.clear();
		// Выполняем проверку отказа разбора при превышении глубины
		ASSERT_EQ(consume("a = [[[1]]]\n", 0, settings, events), toml::state_t::FAILED);
	}
	/**
	 * Выполняем проверку предела длины имени ключа
	 */
	{
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Выполняем установку предела длины имени ключа
		settings.maxKey = 3;
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку того, что разбор в пределах длины имени удался
		ASSERT_EQ(consume("abc = 1\n", 0, settings, events), toml::state_t::FINISHED);
		// Выполняем очистку собранных событий разбора
		events.clear();
		// Выполняем проверку отказа разбора при превышении длины имени
		ASSERT_EQ(consume("abcd = 1\n", 0, settings, events), toml::state_t::FAILED);
	}
	/**
	 * Выполняем проверку того, что ноль в пределе означает отсутствие предела
	 */
	{
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Выполняем снятие предела длины логической строки
		settings.maxLine = 0;
		// Выполняем снятие предела длины имени ключа
		settings.maxKey = 0;
		// Выполняем снятие предела количества составных частей имени
		settings.maxParts = 0;
		// Выполняем признание знаков Юникода в именах без кавычек
		settings.unicode = true;
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку того, что разбор со снятыми пределами удался
		ASSERT_EQ(consume("оченьдлинноеимяключа = \"значение\"\n", 0, settings, events), toml::state_t::FINISHED);
	}
}
/**
 * @brief Проверка обращения со знаками Юникода в именах без кавычек
 *
 * @details Описание версии 1.0.0 отводит именам без кавычек лишь знаки US-ASCII:
 * умолчанием берётся оно, а признание знаков Юникода включается настройкой
 *
 */
TEST(CodecTomlReader, UnicodeKeys) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку отказа разбора имени со знаками Юникода
	ASSERT_EQ(consume("ключ = 1\n", 0, settings, events), toml::state_t::FAILED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	/**
	 * Выполняем проверку того, что имя в кавычках знаки Юникода принимает
	 */
	ASSERT_EQ(consume("\"ключ\" = 1\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку собранного имени ключа
	ASSERT_EQ(events.at(0).path, "ключ");
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем признание знаков Юникода в именах без кавычек
	settings.unicode = true;
	// Выполняем проверку разбора имени со знаками Юникода
	ASSERT_EQ(consume("ключ = 1\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку собранного имени ключа
	ASSERT_EQ(events.at(0).path, "ключ");
}
/**
 * @brief Проверка неделимости отвергнутой записи
 *
 * @details События записи выдаются потребителю все разом либо не выдаются вовсе:
 * иначе выдача зависела бы от нарезки текста на куски - подача целиком успевала бы
 * выдать события записи, отвергнутой ниже по тексту, а подача кусками откатывала бы
 * их вместе с самой записью
 *
 */
TEST(CodecTomlReader, RejectedRecord) {
	// Разбираемый текст настроек
	const string text = "first = 1\nsecond = { a = 1, длинноеимяключа = 2 }\n";
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Устанавливаем наибольшую допустимую длину имени ключа
	settings.maxKey = 8;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку отказа разбора текста настроек
	ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	/**
	 * Выполняем проверку количества выданных разбором событий
	 *
	 * @note Выданы события лишь первой записи: вторая отвергнута, и её события
	 *       потребителю не достаются
	 */
	ASSERT_EQ(events.size(), 2u);
	// Выполняем проверку разновидности первого выданного события
	ASSERT_EQ(events.at(0).event, static_cast <uint8_t> (toml::event_t::KEY));
	// Выполняем проверку разновидности второго выданного события
	ASSERT_EQ(events.at(1).event, static_cast <uint8_t> (toml::event_t::VALUE));
}
/**
 * @brief Проверка выдачи отложенного отказа приведения к кодировке UTF-8
 *
 * @details Отказ приведения откладывается до исчерпания уже приведённого начала
 * текста, и выдать его обязаны оба пути разбора: приведённое начало разбирается тем
 * из них, которому досталось, и отказ, выдаваемый лишь подачей куска, при подаче
 * текста целиком пропадал бы вовсе
 *
 */
TEST(CodecTomlReader, DeferredEncodingFailure) {
	// Разбираемый текст настроек с недопустимой последовательностью UTF-8
	const string text = string("value = 1\n") + "\xC3\x28" + "\n";
	// Собранные события разбора
	vector <Event> events;
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	// Выполняем проверку отказа разбора текста, поданного целиком
	ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	/**
	 * Выполняем проверку количества выданных разбором событий
	 *
	 * @note События приведённого начала текста потребитель получает прежде отказа
	 */
	ASSERT_EQ(events.size(), 2u);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку отказа разбора текста, поданного кусками
	ASSERT_EQ(consume(text, 1, settings, events), toml::state_t::FAILED);
	// Выполняем проверку совпадения количества событий при подаче кусками
	ASSERT_EQ(events.size(), 2u);
}
/**
 * @brief Проверка отличения пустого имени таблицы от верхнего уровня
 *
 * @details Описание дозволяет имя ключа пустым, и учёт объявленных имён обязан
 * отличать таблицу «[""]» от верхнего уровня текста настроек: иначе ключ её считался
 * бы повтором одноимённого ключа верхнего уровня
 *
 */
TEST(CodecTomlReader, EmptyTableName) {
	// Разбираемый текст настроек
	const string text = "host = 1\n[\"\"]\nhost = 2\n";
	// Собранные события разбора
	vector <Event> events;
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	// Выполняем проверку разбора текста настроек до конца
	ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку количества выданных разбором событий
	ASSERT_EQ(events.size(), 5u);
}
