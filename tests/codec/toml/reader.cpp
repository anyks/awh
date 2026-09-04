/**
 * @file reader.cpp
 * @date 2026-08-12
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
 * @brief Проверки потокового чтения текста настроек TOML — разбор таблиц, пар, строк
 *        всех четырёх записей, чисел, отметок времени, перечней и встроенных таблиц,
 *        а также независимость выдачи от нарезки текста на куски
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 *
 * @note Заголовок cmath нужен ради std::isinf и std::isnan. Собиратели посвежее подтягивают
 *       его попутно другими заголовками, а gcc 12 с glibc 2.36 - нет, и сборка
 *       валится с "isinf is not a member of std". Проверено на стенде Debian 12
 */
#include <cmath>
#include <chrono>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/toml/toml.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

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
/**
 * @brief Безымянное пространство имён вспомогательных объявлений проверки
 *
 * @note Пространство это обязательно: объявления проверок всех кодеков сходятся в
 *       одну программу, и имя «Event» занято проверками не одного лишь TOML. Два
 *       разных строения под одним именем нарушают правило одного определения, а
 *       сказывается это порчей кучи вдали от места объявления
 *
 */
namespace {
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
	 * Смещение события от начала исходного текста
	 *
	 * @note Смещение это считается от основания, уплотнением накопителя переносимого, и
	 *       сличением строки со столбцом не поверяется: они считаются от начала строки
	 */
	uint64_t offset;
	/**
	 * @brief Конструктор
	 *
	 */
	Event() noexcept : event(0), type(0), integer(0), boolean(false), line(0), column(0), offset(0) {}
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
	toml::reader_t reader(::logger(), settings);
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
			// Запоминаем смещение события от начала исходного текста
			item.offset = reader.location().offset;
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
		toml::reader_t reader(::logger());
		// Разбираемая запись числа за отрезком значений
		const string source = "a = 9223372036854775808\n";
		// Выполняем проверку отказа разбора записи числа
		ASSERT_FALSE(reader.feed(source.data(), source.size(), true));
		// Выполняем проверку выданного кода ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::NUMBER_OVERFLOW);
	}
	/**
	 * Выполняем проверку отказа разбора дробного числа за отрезком значений
	 *
	 * @note Запись эта построена безупречно, и негодна в ней одна лишь величина:
	 *       ответ ошибкой построения посылал бы потребителя править запись, которая
	 *       правильна. Отвечать здесь обязан тот же код, что и у целого числа
	 */
	{
		/**
		 * Выполняем перебор записей дробных чисел за отрезком значений
		 */
		for(auto & source : {string("a = 1e1000\n"), string("a = 1.0e+400\n"), string("a = -1e400\n")}){
			// Объект потокового чтения текста настроек
			toml::reader_t reader(::logger());
			// Выполняем проверку отказа разбора записи числа
			ASSERT_FALSE(reader.feed(source.data(), source.size(), true)) << source;
			// Выполняем проверку выданного кода ошибки разбора
			ASSERT_EQ(reader.error(), toml::error_t::NUMBER_OVERFLOW) << source;
		}
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
	toml::reader_t reader(::logger(), settings);
	// Разбираемый текст настроек
	const string text = "a = 3.14\nb = -0.5e3\nc = inf\nd = -inf\ne = nan\nf = true\ng = false\n";
	// Выполняем проверку того, что разбор текста настроек удался
	ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
	// Собранные значения текста настроек
	vector <toml::content_t> values;
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
	ASSERT_TRUE(std::isinf(values.at(2).real) && (values.at(2).real > 0));
	// Выполняем проверку разбора отрицательной бесконечности
	ASSERT_TRUE(std::isinf(values.at(3).real) && (values.at(3).real < 0));
	// Выполняем проверку разбора нечисла
	ASSERT_TRUE(std::isnan(values.at(4).real));
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
	toml::reader_t reader(::logger());
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
	vector <toml::content_t> values;
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
/**
 * @brief Проверка меры предела длины записи
 *
 * @details Разбор записи оканчивается за знаком конца строки, а окончание это
 * содержимым записи не является: считая его, одна и та же строка предел то превышала
 * бы, то нет - смотря по тому, оканчивается ли ею текст, и каким из двух видов
 * окончания она отделена от следующей
 *
 */
TEST(CodecTomlReader, LineLimitTerminator) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Запоминаем предел длины записи, равный длине содержимого строки
	settings.maxLine = 5;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку разбора строки, знаком конца строки не оканчиваемой
	ASSERT_EQ(consume("a = 1", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку разбора той же строки с переводом строки
	ASSERT_EQ(consume("a = 1\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку разбора той же строки с возвратом каретки и переводом строки
	ASSERT_EQ(consume("a = 1\r\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку отказа разбора строки, предел превышающей
	ASSERT_EQ(consume("aa = 1\n", 0, settings, events), toml::state_t::FAILED);
}
/**
 * @brief Проверка места отказа превышения предела длины записи
 *
 * @details Предел длины меряется по разборе записи целиком, и разбор к этому времени
 * стоит уже на следующей строке: без отматывания счёта строк назад место отказа
 * указывало бы на строку, следующую за виновной
 *
 */
TEST(CodecTomlReader, LineLimitLocation) {
	/**
	 * Выполняем перебор мест виновной записи в разбираемом тексте
	 */
	for(const auto & item : {
		make_pair(string("bbbbbb = 1\nc = 2\n"), 1u),
		make_pair(string("c = 2\nbbbbbb = 1\nd = 3\n"), 2u),
		make_pair(string("c = 2\n\n\nbbbbbb = 1\n"), 4u),
		make_pair(string("a = \"\"\"\nx\ny\n\"\"\"\nb = 2\n"), 1u)
	}){
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Запоминаем предел длины записи
		settings.maxLine = 8;
		// Объект потокового чтения текста настроек
		toml::reader_t reader(::logger(), settings);
		// Выполняем подачу разбираемого текста настроек целиком
		static_cast <void> (reader.feed(item.first.data(), item.first.size(), true));
		/**
		 * Выполняем перебор выданных разбором событий
		 */
		while(reader.next()){}
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::LINE_TOO_LONG);
		// Выполняем проверку номера строки места отказа
		ASSERT_EQ(reader.errorLocation().line, item.second);
		// Выполняем проверку положения места отказа в строке
		ASSERT_EQ(reader.errorLocation().column, 1u);
	}
}
/**
 * @brief Проверка обрыва накопленного текста на пробеле отметки времени
 *
 * @details Описание дозволяет отделять время от даты пробелом наравне со знаком «T»,
 * и завершает ли пробел запись значения, видно лишь по знаку за ним: обрыв текста
 * ровно на этом пробеле обязан требовать продолжения, а не решать запись одной датой
 *
 */
TEST(CodecTomlReader, StampSpaceBoundary) {
	// Разбираемый текст настроек с отметкой времени, отделяемой пробелом
	const string text = "stamp = 1979-05-27 07:32:00\n";
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	/**
	 * Выполняем перебор размеров куска подачи разбираемого текста
	 */
	for(const size_t chunk : {static_cast <size_t> (0), static_cast <size_t> (1),
	                          static_cast <size_t> (3), static_cast <size_t> (18)}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку разбора текста настроек до конца
		ASSERT_EQ(consume(text, chunk, settings, events), toml::state_t::FINISHED);
		// Выполняем проверку количества выданных разбором событий
		ASSERT_EQ(events.size(), 2u);
		// Выполняем проверку типа значения отметки времени
		ASSERT_EQ(events.at(1).type, static_cast <uint8_t> (toml::type_t::LOCAL_DATETIME));
	}
}
/**
 * @brief Проверка запрета дополнения таблицы, объявленной исподволь
 *
 * @details Таблица объявляется исподволь всяким объемлющим именем: заголовок «[a.b]»
 * и составное имя ключа «a.b = 1» заводят таблицу «a», собственного объявления не
 * имеющую. Описание запрещает дополнять её набором таблиц наравне с объявленной:
 * значение по имени меняло бы при этом свой тип
 *
 */
TEST(CodecTomlReader, ImplicitTableAppend) {
	/**
	 * Выполняем перебор текстов настроек, дополняющих таблицу набором
	 */
	for(const string & text : {
		string("[value.0]\n[[value]]\n"),
		string("a.b = 1\n[[a]]\n"),
		string("[a.b.c]\n[[a.b]]\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Настройки разбора текста настроек
		const toml::reader_t::settings_t settings;
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	}
	/**
	 * Выполняем перебор текстов настроек, дополнением не являющихся
	 */
	for(const string & text : {
		string("[[fruit]]\n[fruit.apple]\n[[fruit]]\n[fruit.apple]\n"),
		string("[[a]]\n[[a.b]]\n[[a]]\n[[a.b]]\n"),
		string("[a.b]\n[a]\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Настройки разбора текста настроек
		const toml::reader_t::settings_t settings;
		// Выполняем проверку разбора текста настроек до конца
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	}
}
/**
 * @brief Проверка запрета управляющих знаков в примечании
 *
 * @details Описание дозволяет примечанию нести из управляющих знаков одну лишь
 * табуляцию. Возврат каретки к тому же неотличим в примечании от окончания строки:
 * примечание, его несущее, записью выдавалось бы окончанием, и перезапись текста
 * настроек устойчивости лишалась бы
 *
 */
TEST(CodecTomlReader, CommentControlCharacter) {
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	/**
	 * Выполняем перебор примечаний с управляющими знаками
	 */
	for(const string & text : {
		string("a = 1 # хвост\rb = 2\n"),
		string("# отдельное\rпримечание\n"),
		string("a = 1 # хвост\x01\n"),
		string("a = 1 # хвост\x7F\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	}
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку разбора примечания с табуляцией
	ASSERT_EQ(consume("a = 1 # хвост\tсквозь табуляцию\n", 0, settings, events), toml::state_t::FINISHED);
}
/**
 * @brief Проверка смещения часового пояса у местного времени
 *
 * @details Смещение часового пояса описание отводит лишь отметке с датой: у местного
 * времени часового пояса нет по существу. Отказ этот не задевал ни набор проверок, ни
 * корпус наречий, ни двадцать тысяч проходов ворошителя - нашло его снятие карты
 * слепых строк
 *
 */
TEST(CodecTomlReader, LocalTimeRejectsOffset) {
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	/**
	 * Выполняем перебор записей местного времени со смещением часового пояса
	 */
	for(const string & text : {
		string("a = 12:30:00+01:00\n"),
		string("a = 12:30:00-05:00\n"),
		string("a = 12:30:00Z\n"),
		string("a = 12:30:00.5+01:00\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	}
	/**
	 * Выполняем перебор записей, смещение часового пояса допускающих
	 */
	for(const string & text : {
		string("a = 12:30:00\n"),
		string("a = 1979-05-27T07:32:00+01:00\n"),
		string("a = 1979-05-27T07:32:00Z\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку успешного разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	}
}
/**
 * @brief Проверка возврата каретки одинокого в конце примечания
 *
 * @details Возврат каретки есть знак управляющий, и примечанию он негоден. Разбор
 * отбрасывал возврат хвостовой безусловно - и написание «# c\r», текст которым
 * оканчивается, принимал, тогда как то же примечание посреди текста отвергал.
 * Отбрасывать возврат дозволено лишь как половину пары «\r\n»
 *
 */
TEST(CodecTomlReader, CommentTrailingCarriage) {
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	/**
	 * Выполняем перебор примечаний, возвратом каретки одиноким оканчивающихся
	 */
	for(const string & text : {
		string("# c\r"),
		string("a = 1\n# c\r"),
		string("a = 1 # хвост\r"),
		string("a = 1\n# c\r\r\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	}
	/**
	 * Выполняем перебор примечаний, парою «\r\n» оканчивающихся
	 */
	for(const string & text : {
		string("# c\r\n"),
		string("a = 1\n# c\r\n"),
		string("a = 1 # хвост\r\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку успешного разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	}
}
/**
 * @brief Проверка отделения пар встроенной таблицы запятой
 *
 * @details Описание требует отделять пары встроенной таблицы запятой, а лишнюю
 * запятую в конце запрещает: «{a = 1 b = 2}» и «{a = 1,}» записями встроенной
 * таблицы не являются
 *
 */
TEST(CodecTomlReader, InlineSeparators) {
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	/**
	 * Выполняем перебор ошибочных записей встроенной таблицы
	 */
	for(const string & text : {
		string("t = {a = 1 b = 2}\n"),
		string("t = {a = 1,}\n"),
		string("t = {,}\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	}
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку разбора правильной записи встроенной таблицы
	ASSERT_EQ(consume("t = {a = 1, b = 2}\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем проверку разбора пустой встроенной таблицы
	events.clear();
	// Выполняем проверку разбора текста настроек до конца
	ASSERT_EQ(consume("t = {}\n", 0, settings, events), toml::state_t::FINISHED);
}
/**
 * @brief Проверка учёта имён ключей встроенной таблицы
 *
 * @details Ключи встроенной таблицы объявляются наравне с прочими: повтор их
 * описание запрещает, а сама таблица по закрытии скобки дополнению не подлежит.
 * Ключи встроенных таблиц, записанных значениями перечня, повторами друг другу при
 * этом не приходятся: имени у таких таблиц нет вовсе
 *
 */
TEST(CodecTomlReader, InlineDeclarations) {
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	/**
	 * Выполняем перебор ошибочных текстов настроек
	 */
	for(const string & text : {
		string("t = {a = 1, a = 2}\n"),
		string("t = {a = 1}\nt.b = 2\n"),
		string("t = {a = 1}\n[t.b]\n"),
		string("t = {a = {b = 1}}\nt.a.c = 2\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	}
	/**
	 * Выполняем перебор правильных текстов настроек
	 */
	for(const string & text : {
		string("t = [{a = 1}, {a = 2}]\n"),
		string("t = {a = 1, b = {a = 2}}\n"),
		string("t.u = {a = 1}\nt.v = 2\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку разбора текста настроек до конца
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	}
}
/**
 * @brief Проверка занятости имён составными именами ключей
 *
 * @details Составное имя ключа заводит таблицу всякой своей частью, кроме
 * последней, и описание запрещает как заводить пару поверх такой таблицы, так и
 * объявлять её заголовком. Под парой же не заводится ничего вовсе
 *
 */
TEST(CodecTomlReader, DottedOccupancy) {
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	/**
	 * Выполняем перебор ошибочных текстов настроек
	 */
	for(const string & text : {
		string("a = 1\na.b = 2\n"),
		string("a.b = 1\na = 2\n"),
		string("[fruit]\napple.color = \"red\"\n[fruit.apple]\n"),
		string("a = [1]\na.b = 2\n"),
		string("a = [1]\n[a.b]\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FAILED);
	}
	/**
	 * Выполняем перебор правильных текстов настроек
	 */
	for(const string & text : {
		string("a.b = 1\na.c = 2\n"),
		string("[fruit]\napple.color = \"red\"\n[fruit.apple.texture]\nsmooth = true\n"),
		string("[a.b]\n[a]\n"),
		string("[x.y.z.w]\n[x]\n")
	}){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку разбора текста настроек до конца
		ASSERT_EQ(consume(text, 0, settings, events), toml::state_t::FINISHED);
	}
}
/**
 * @brief Проверка запрета одиночного возврата каретки в перечне
 *
 * @details Описание отводит концом строки перевод строки и пару из возврата с
 * переводом: возврат каретки сам по себе концом строки не является нигде, и
 * пропуск пробельных знаков перечня обязан отвергать его наравне с прочими местами
 *
 */
TEST(CodecTomlReader, ArrayCarriageReturn) {
	// Настройки разбора текста настроек
	const toml::reader_t::settings_t settings;
	// Собранные события разбора
	vector <Event> events;
	// Выполняем проверку отказа разбора перечня с одиночным возвратом каретки
	ASSERT_EQ(consume("a = [1,\r2]\n", 0, settings, events), toml::state_t::FAILED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку отказа разбора перечня с возвратом каретки перед значением
	ASSERT_EQ(consume("a = [\r1]\n", 0, settings, events), toml::state_t::FAILED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку разбора перечня с парой из возврата и перевода строки
	ASSERT_EQ(consume("a = [1,\r\n2]\n", 0, settings, events), toml::state_t::FINISHED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	/**
	 * Выполняем проверку отказа разбора примечания перечня с возвратом каретки
	 *
	 * @note Описание дозволяет примечанию нести из управляющих знаков одну лишь
	 *       табуляцию, где бы примечание ни стояло
	 */
	ASSERT_EQ(consume("a = [1, # хво\rст\n2]\n", 0, settings, events), toml::state_t::FAILED);
	// Выполняем очистку собранных событий разбора
	events.clear();
	// Выполняем проверку разбора примечания перечня, окончанием строки завершённого
	ASSERT_EQ(consume("a = [1, # хвост\r\n2]\n", 0, settings, events), toml::state_t::FINISHED);
}
/**
 * @brief Проверка равномерности учёта имён одной записи
 *
 * @details Всякое объявляемое имя сличается со всеми, объявленными записью прежде, и
 * перебор их обращал бы разбор в квадратичный: встроенная таблица враждебного файла
 * настроек отнимала бы время, растущее квадратом числа ключей
 *
 */
TEST(CodecTomlReader, InlineScaling) {
	/**
	 * @brief Метод замера разбора встроенной таблицы с заданным числом ключей
	 *
	 * @param count количество ключей встроенной таблицы
	 * @return      время разбора в микросекундах
	 *
	 */
	auto measure = [](const size_t count) noexcept -> double {
		// Собираемый текст настроек со встроенной таблицей
		string text("t = {");
		/**
		 * Выполняем перебор всех ключей встроенной таблицы
		 */
		for(size_t i = 0; i < count; i++){
			/**
			 * Если ключ не первый
			 */
			if(i > 0)
				// Выполняем добавление разделителя пар встроенной таблицы
				text.append(", ");
			// Выполняем добавление очередной пары встроенной таблицы
			text.append("k").append(to_string(i)).append(" = 1");
		}
		// Выполняем закрытие встроенной таблицы
		text.append("}\n");
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Снимаем предел длины логической строки
		settings.maxLine = 0;
		// Запоминаем время начала разбора
		const auto begin = chrono::steady_clock::now();
		// Объект потокового чтения текста настроек
		toml::reader_t reader(::logger(), settings);
		// Выполняем подачу разбираемого текста настроек
		static_cast <void> (reader.feed(text.data(), text.size(), true));
		/**
		 * Выполняем перебор выданных разбором событий
		 */
		while(reader.next()){}
		// Выводим время разбора в микросекундах
		return static_cast <double> (chrono::duration_cast <chrono::microseconds> (
			chrono::steady_clock::now() - begin).count());
	};
	// Выполняем прогрев замера разбором встроенной таблицы
	static_cast <void> (measure(2000));
	// Получаем время разбора встроенной таблицы с двумя тысячами ключей
	const double lesser = measure(2000);
	// Получаем время разбора встроенной таблицы с восемью тысячами ключей
	const double greater = measure(8000);
	/**
	 * Выполняем проверку роста времени разбора
	 *
	 * @note Порог взят с запасом: замер на отладочном стенде разнится от прогона к
	 *       прогону, а ловится здесь рост квадратом - при нём отношение достигло бы
	 *       шестнадцати
	 */
	ASSERT_LT(greater, (lesser * 24.0)) << "рост времени разбора: " << lesser << " → " << greater << " мкс";
}
/**
 * @brief Проверка соответствия разбора описанию TOML версии 1.0.0
 *
 * @details Набор случаев взят из самого описания и покрывает все его разделы: имена
 * ключей, четыре записи строк, четыре системы счисления чисел, четыре вида отметок
 * времени, перечни, встроенные таблицы, таблицы и наборы таблиц. Всякий случай
 * проверяется вдобавок на независимость выдачи от нарезки текста на куски: расхождение
 * означало бы, что разбор одного и того же текста разнится от способа его подачи
 *
 */
TEST(CodecTomlReader, Conformance) {
	/**
	 * @brief Итог разбора текста настроек
	 *
	 */
	struct Outcome {
		// Состояние разбора и код ошибки
		uint32_t state, error;
		// Место обнаружения ошибки разбора
		uint32_t line, column;
		// Собранная выдача разбора
		string trace;
	};
	/**
	 * @brief Метод разбора текста настроек кусками заданного размера
	 *
	 * @param text  разбираемый текст настроек
	 * @param chunk размер куска подачи, ноль означает подачу целиком
	 * @return      итог разбора текста настроек
	 *
	 */
	auto outcome = [](const string & text, const size_t chunk) noexcept -> Outcome {
		// Собираемый итог разбора текста настроек
		Outcome result{0, 0, 0, 0, string()};
		// Настройки разбора текста настроек
		const toml::reader_t::settings_t settings;
		// Объект потокового чтения текста настроек
		toml::reader_t reader(::logger(), settings);
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
				// Выходим из цикла подачи разбираемого текста
				break;
			// Выполняем переход к следующему куску подачи
			offset += length;
			/**
			 * Выполняем перебор выданных разбором событий
			 */
			while(reader.next()){
				// Выполняем добавление разновидности события к собираемой выдаче
				result.trace.append(to_string(static_cast <uint32_t> (reader.event()))).append(":");
				// Выполняем добавление типа значения события к собираемой выдаче
				result.trace.append(to_string(static_cast <uint32_t> (reader.value().type))).append(":");
				/**
				 * Выполняем перебор составных частей имени ключа события
				 */
				for(auto & part : reader.path())
					// Выполняем добавление составной части имени к собираемой выдаче
					result.trace.append(part.name).append(".");
				// Выполняем добавление содержимого значения к собираемой выдаче
				result.trace.append(reader.value().text).append("@");
				// Выполняем добавление номера строки события к собираемой выдаче
				result.trace.append(to_string(reader.location().line)).append(",");
				// Выполняем добавление положения события в строке к собираемой выдаче
				result.trace.append(to_string(reader.location().column)).append(";");
			}
			/**
			 * Если кусок оказался последним
			 */
			if(end)
				// Выходим из цикла подачи разбираемого текста
				break;
		} while(offset <= text.size());
		/**
		 * Выполняем перебор оставшихся событий разбора
		 */
		while(reader.next()){}
		// Запоминаем состояние разбора текста настроек
		result.state = static_cast <uint32_t> (reader.state());
		// Запоминаем код ошибки разбора
		result.error = static_cast <uint32_t> (reader.error());
		// Запоминаем номер строки места обнаружения ошибки
		result.line = reader.errorLocation().line;
		// Запоминаем положение места обнаружения ошибки в строке
		result.column = reader.errorLocation().column;
		// Выводим собранный итог разбора текста настроек
		return result;
	};
	/**
	 * Выполняем перебор всех случаев, описанием отведённых
	 */
	for(const auto & item : vector <pair <string, pair <const char *, bool>>> {
		make_pair("key = \"value\"\n", make_pair("голое имя", true)),
		make_pair("1234 = \"value\"\n", make_pair("имя из цифр", true)),
		make_pair("\"127.0.0.1\" = \"value\"\n", make_pair("имя в кавычках с точкой", true)),
		make_pair("= 1\n", make_pair("пустое имя без кавычек", false)),
		make_pair("\"\" = 1\n", make_pair("пустое имя в кавычках", true)),
		make_pair("'' = 1\n", make_pair("пустое дословное имя", true)),
		make_pair("key = # ПУСТО\n", make_pair("имя без значения", false)),
		make_pair("key\n", make_pair("имя без знака равенства", false)),
		make_pair("fruit . flavor = \"banana\"\n", make_pair("пробелы вокруг точек имени", true)),
		make_pair("\"\"\"a\"\"\" = 1\n", make_pair("многострочное имя", false)),
		make_pair("a = \"я \\u00E9 \\t\"\n", make_pair("основная строка", true)),
		make_pair("a = \"\\x\"\n", make_pair("недопустимая последовательность", false)),
		make_pair("a = \"\\uD800\"\n", make_pair("суррогат в последовательности", false)),
		make_pair("a = \"перенос\nздесь\"\n", make_pair("перенос в однострочной", false)),
		make_pair("a = \"\"\"первая\nвторая\"\"\"\n", make_pair("многострочная с переносом", true)),
		make_pair("a = \"\"\"один \\\n  два\"\"\"\n", make_pair("склейка обратной чертой", true)),
		make_pair("a = \"\"\"один \\  два\"\"\"\n", make_pair("черта над пробелами без переноса", false)),
		make_pair("a = \"\"\"он сказал \"\"да\"\" \"\"\"\n", make_pair("две кавычки внутри многострочной", true)),
		make_pair("a = 'C:\\Users\\nodejs'\n", make_pair("дословная строка", true)),
		make_pair("a = '''\nдословно\\n'''\n", make_pair("многострочная дословная", true)),
		make_pair(string("a = \"\x01\"\n"), make_pair("сырой управляющий знак", false)),
		make_pair("a = \"\tтаб\"\n", make_pair("табуляция в строке", true)),
		make_pair("a = +99\n", make_pair("целое со знаком", true)),
		make_pair("a = -0\n", make_pair("ноль со знаком", true)),
		make_pair("a = 1_000_000\n", make_pair("разделители разрядов", true)),
		make_pair("a = _1\n", make_pair("разделитель в начале", false)),
		make_pair("a = 1_\n", make_pair("разделитель в конце", false)),
		make_pair("a = 1__0\n", make_pair("сдвоенный разделитель", false)),
		make_pair("a = 011\n", make_pair("незначащий ноль", false)),
		make_pair("a = 0xDEADBEEF\n", make_pair("шестнадцатеричное", true)),
		make_pair("a = -0x1\n", make_pair("шестнадцатеричное со знаком", false)),
		make_pair("a = 0o755\n", make_pair("восьмеричное", true)),
		make_pair("a = 0b11010110\n", make_pair("двоичное", true)),
		make_pair("a = 0X1\n", make_pair("приставка в верхнем регистре", false)),
		make_pair("a = 9223372036854775808\n", make_pair("выход за отрезок", false)),
		make_pair("a = -9223372036854775808\n", make_pair("наименьшее целое", true)),
		make_pair("a = 3.1415\n", make_pair("дробное", true)),
		make_pair("a = 5e+22\n", make_pair("показатель", true)),
		make_pair("a = 6.626e-34\n", make_pair("дробное с показателем", true)),
		make_pair("a = 1.\n", make_pair("точка без дробной части", false)),
		make_pair("a = .1\n", make_pair("точка без целой части", false)),
		make_pair("a = 1e\n", make_pair("показатель без цифр", false)),
		make_pair("a = -inf\n", make_pair("бесконечность", true)),
		make_pair("a = nan\n", make_pair("нечисло", true)),
		make_pair("a = true\n", make_pair("истина", true)),
		make_pair("a = True\n", make_pair("истина в верхнем регистре", false)),
		make_pair("a = 1979-05-27T07:32:00Z\n", make_pair("отметка со смещением", true)),
		make_pair("a = 1979-05-27T00:32:00-07:00\n", make_pair("отметка со смещением минусом", true)),
		make_pair("a = 1979-05-27T00:32:00.999999-07:00\n", make_pair("отметка с долей секунды", true)),
		make_pair("a = 1979-05-27 07:32:00Z\n", make_pair("отметка через пробел", true)),
		make_pair("a = 1979-05-27T07:32:00\n", make_pair("местная отметка", true)),
		make_pair("a = 1979-05-27\n", make_pair("местная дата", true)),
		make_pair("a = 07:32:00\n", make_pair("местное время", true)),
		make_pair("a = 07:32\n", make_pair("время без секунд", false)),
		make_pair("a = 2026-02-30\n", make_pair("несуществующая дата", false)),
		make_pair("a = 2024-02-29\n", make_pair("двадцать девятое февраля високосного", true)),
		make_pair("a = 2023-02-29\n", make_pair("двадцать девятое февраля обычного", false)),
		make_pair("a = 1979-05-27T23:59:60Z\n", make_pair("добавочная секунда", true)),
		make_pair("a = 1979-05-27T24:00:00Z\n", make_pair("час за пределом", false)),
		make_pair("a = [1, 2, 3]\n", make_pair("перечень чисел", true)),
		make_pair("a = [1, \"два\", 3.0]\n", make_pair("перечень разных типов", true)),
		make_pair("a = [\n1,\n2,\n]\n", make_pair("перечень несколькими строками", true)),
		make_pair("a = [1,]\n", make_pair("лишняя запятая перечня", true)),
		make_pair("a = [1,,2]\n", make_pair("сдвоенная запятая перечня", false)),
		make_pair("a = [1\n", make_pair("незакрытый перечень", false)),
		make_pair("a = [\n1, # первое\n2\n]\n", make_pair("примечание внутри перечня", true)),
		make_pair("a = { x = 1, y = 2 }\n", make_pair("встроенная таблица", true)),
		make_pair("a = {}\n", make_pair("пустая встроенная таблица", true)),
		make_pair("a = {\nx = 1}\n", make_pair("перенос во встроенной таблице", false)),
		make_pair("[a]\nx = 1\n", make_pair("таблица", true)),
		make_pair("[a.b.c]\nx = 1\n", make_pair("вложенная таблица", true)),
		make_pair("[ a . b ]\nx = 1\n", make_pair("пробелы в имени таблицы", true)),
		make_pair("[]\n", make_pair("пустое имя таблицы", false)),
		make_pair("[a]\n[a]\n", make_pair("повтор таблицы", false)),
		make_pair("a = 1\na = 2\n", make_pair("повтор ключа", false)),
		make_pair("a = 1\n[a]\n", make_pair("таблица поверх ключа", false)),
		make_pair("[[a]]\nx = 1\n[[a]]\nx = 2\n", make_pair("набор таблиц", true)),
		make_pair("[[a]]\n[a.b]\nx = 1\n", make_pair("таблица внутри набора", true)),
		make_pair("[a\n", make_pair("незакрытая таблица", false)),
		make_pair("[a] x = 1\n", make_pair("содержимое за таблицей", false)),
		make_pair("# примечание\na = 1\n", make_pair("примечание строкой", true)),
		make_pair("", make_pair("пустой текст", true)),
		make_pair(string("\xEF\xBB\xBF") + "a = 1\n", make_pair("метка порядка байтов", true)),
		make_pair("a = 1", make_pair("текст без окончания строки", true)),
		make_pair("a = 1\r\n", make_pair("окончание CRLF", true)),
		make_pair("a = 1\rb = 2\n", make_pair("одиночный возврат каретки", false))
	}){
		// Выполняем разбор текста настроек, поданного целиком
		const Outcome whole = outcome(item.first, 0);
		// Выполняем проверку соответствия итога разбора описанию
		ASSERT_EQ((whole.state == static_cast <uint32_t> (toml::state_t::FINISHED)), item.second.second)
			<< item.second.first << ": ошибка " << whole.error;
		/**
		 * Выполняем перебор размеров куска подачи разбираемого текста
		 */
		for(const size_t chunk : {static_cast <size_t> (1), static_cast <size_t> (2), static_cast <size_t> (3),
		                          static_cast <size_t> (5), static_cast <size_t> (7), static_cast <size_t> (13)}){
			// Выполняем разбор текста настроек, поданного кусками
			const Outcome piece = outcome(item.first, chunk);
			// Выполняем проверку совпадения состояния разбора
			ASSERT_EQ(piece.state, whole.state) << item.second.first << ", кусок " << chunk;
			// Выполняем проверку совпадения кода ошибки разбора
			ASSERT_EQ(piece.error, whole.error) << item.second.first << ", кусок " << chunk;
			// Выполняем проверку совпадения места обнаружения ошибки
			ASSERT_EQ(piece.line, whole.line) << item.second.first << ", кусок " << chunk;
			// Выполняем проверку совпадения положения места ошибки в строке
			ASSERT_EQ(piece.column, whole.column) << item.second.first << ", кусок " << chunk;
			// Выполняем проверку совпадения выданных разбором событий
			ASSERT_EQ(piece.trace, whole.trace) << item.second.first << ", кусок " << chunk;
		}
	}
}
/**
 * @brief Проверка набора знаков Юникода, дозволенных имени без кавычек
 *
 * @details Знаки Юникода в имени без кавычек отводит черновик следующей версии
 * описания, и набор их там задан перечнем: признавать всякий знак вне US-ASCII
 * значило бы принимать имена, которых черновик не дозволяет - со знаками
 * препинания и знаками оформления
 *
 */
TEST(CodecTomlReader, UnicodeNames) {
	/**
	 * Выполняем перебор проверяемых имён ключей
	 */
	for(const auto & item : {
		make_pair(string("сервер = 1\n"), true),
		make_pair(string("日本語 = 1\n"), true),
		make_pair(string("\xF0\x9F\x98\x80 = 1\n"), true),
		make_pair(string("a\xE2\x80\x8C""b = 1\n"), true),
		make_pair(string("a\xC2\xA9""b = 1\n"), false),
		make_pair(string("\xC2\xB7 = 1\n"), false),
		make_pair(string("\xC3\x28 = 1\n"), false)
	}){
		/**
		 * Выполняем перебор размеров куска подачи исходного текста
		 */
		for(size_t size : {size_t(1), size_t(2), size_t(3), size_t(64)}){
			// Настройки разбора текста настроек
			toml::reader_t::settings_t settings;
			// Устанавливаем признание знаков Юникода в именах без кавычек
			settings.unicode = true;
			// Объект потокового чтения текста настроек
			toml::reader_t reader(::logger(), settings);
			/**
			 * Выполняем подачу исходного текста кусками выбранного размера
			 */
			for(size_t i = 0; i < item.first.size(); i += size){
				// Получаем размер очередного куска исходного текста
				const size_t part = ((size < (item.first.size() - i)) ? size : (item.first.size() - i));
				// Выполняем подачу очередного куска исходного текста
				static_cast <void> (reader.feed(item.first.data() + i, part, (i + part) == item.first.size()));
				/**
				 * Выполняем перебор выданных разбором событий
				 */
				while(reader.next()){}
			}
			// Выполняем проверку итога разбора текста настроек
			ASSERT_EQ((reader.error() == toml::error_t::NONE), item.second)
			 << "«" << item.first << "» кусками по " << size;
		}
	}
	// Объект записи текста настроек
	toml::writer_t writer(::logger());
	// Выполняем запись имени ключа со знаком, черновиком не дозволенным
	ASSERT_TRUE(writer.key("a\xC2\xA9""b"));
	// Выполняем запись значения пары
	ASSERT_TRUE(writer.integer(1));
	// Выполняем проверку того, что имя ограждено кавычками
	ASSERT_EQ(writer.text(), string("\"a\xC2\xA9""b\" = 1\n"));
}
/**
 * @brief Проверка построения записи числа с плавающей точкой
 *
 * @details Построение записи задано правилом описания, а не набором дозволенных ей
 * знаков: запись «1.e2» несёт одни лишь знаки, числу отведённые, а числом не является -
 * десятичная точка обязана нести цифру и слева, и справа. Судить о ней по первому и
 * последнему знаку мало: разбор принимал её, а строило её приведение к числу сотней
 *
 */
TEST(CodecTomlReader, FloatGrammar) {
	/**
	 * Выполняем перебор проверяемых записей числа
	 */
	for(const auto & item : {
		make_pair(string("1.e2"), false),
		make_pair(string("1.e+2"), false),
		make_pair(string("1.e-2"), false),
		make_pair(string("+1.e2"), false),
		make_pair(string("1."), false),
		make_pair(string(".1"), false),
		make_pair(string("1e"), false),
		make_pair(string("1e+"), false),
		make_pair(string("1.2.3"), false),
		make_pair(string("1e2e3"), false),
		make_pair(string("1.2e3.4"), false),
		make_pair(string("1.0e"), false),
		make_pair(string("1.2E"), false),
		make_pair(string("0x1.8p3"), false),
		make_pair(string("1.0"), true),
		make_pair(string("1e2"), true),
		make_pair(string("1E+2"), true),
		make_pair(string("1.2e-3"), true),
		make_pair(string("6.626e-34"), true),
		make_pair(string("-0.0"), true),
		make_pair(string("0e0"), true),
		make_pair(string("0.1"), true),
		make_pair(string("1_000.000_001"), true),
		make_pair(string("inf"), true),
		make_pair(string("-inf"), true),
		make_pair(string("nan"), true)
	}){
		// Собираемый текст настроек с проверяемой записью числа
		const string text = ("a = " + item.first + "\n");
		// Объект потокового чтения текста настроек
		toml::reader_t reader(::logger());
		// Выполняем подачу разбираемого текста настроек
		static_cast <void> (reader.feed(text.data(), text.size(), true));
		/**
		 * Выполняем перебор выданных разбором событий
		 */
		while(reader.next()){}
		// Выполняем проверку итога разбора текста настроек
		ASSERT_EQ((reader.error() == toml::error_t::NONE), item.second) << "«" << item.first << "»";
	}
}
/**
 * @brief Проверка стоимости подачи текста мелкими кусками
 *
 * @details Запись, продолжения не дождавшаяся, откатывается целиком и разбирается
 * заново по приходе очередного куска. Браться за неё заново прежде знака конца строки
 * незачем: без ограды на это подача по байту обращала бы разбор одной записи в
 * квадратичный - запись в мегабайт кусками по шестьдесят четыре байта отнимала сорок
 * три секунды против пятидесяти миллисекунд кусками по шестьдесят четыре килобайта
 *
 */
TEST(CodecTomlReader, ChunkScaling) {
	/**
	 * @brief Метод замера времени разбора текста кусками заданного размера
	 *
	 * @param text разбираемый текст настроек
	 * @param size размер куска подачи исходного текста
	 * @return     время разбора текста настроек
	 *
	 */
	auto measure = [](const string & text, const size_t size) noexcept -> double {
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Снимаем предел длины логической строки
		settings.maxLine = 0;
		// Объект потокового чтения текста настроек
		toml::reader_t reader(::logger(), settings);
		// Запоминаем время начала разбора текста настроек
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем подачу исходного текста кусками выбранного размера
		 */
		for(size_t i = 0; i < text.size(); i += size){
			// Получаем размер очередного куска исходного текста
			const size_t part = ((size < (text.size() - i)) ? size : (text.size() - i));
			// Выполняем подачу очередного куска исходного текста
			static_cast <void> (reader.feed(text.data() + i, part, (i + part) == text.size()));
			/**
			 * Выполняем перебор выданных разбором событий
			 */
			while(reader.next()){}
		}
		// Выполняем проверку разбора текста настроек до конца
		EXPECT_EQ(reader.state(), toml::state_t::FINISHED);
		// Выводим время разбора текста настроек
		return chrono::duration <double, milli> (chrono::steady_clock::now() - begin).count();
	};
	// Собираемый текст настроек с одним многострочным значением
	string text("a = \"\"\"");
	// Выполняем добавление содержимого многострочного значения
	text.append(256 * 1024, 'x');
	// Выполняем добавление ограды и знака конца строки
	text.append("\"\"\"\n");
	// Получаем время разбора текста кусками по шестьдесят четыре килобайта
	const double lesser = measure(text, 64 * 1024);
	// Получаем время разбора того же текста кусками по шестьдесят четыре байта
	const double greater = measure(text, 64);
	/**
	 * Выполняем проверку того, что мелкая нарезка стоимости не удесятеряет
	 *
	 * @note Куска здесь тысячекратно мельче, и рост квадратом дал бы не десятки, а
	 *       тысячи: запас взят с четырёхкратным перекрытием против разброса замера
	 */
	ASSERT_LT(greater, ((lesser + 1.0) * 40.0))
	 << "кусками по 64 КБ " << lesser << " мс, кусками по 64 Б " << greater << " мс";
}
/**
 * @brief Проверка независимости итога разбора от нарезки при обрыве приведения
 *
 * @details Разбор откладывает запись, продолжения не дождавшуюся, до знака конца
 * строки среди пришедшего текста. Отказ приведения к UTF-8 накопленный текст
 * обрывает: знака этого уже не будет, и запись, отложенная до него, осталась бы
 * неразобранной - наружу вышел бы отказ приведения вместо ошибки самой записи, а итог
 * разбора зависел бы от нарезки
 *
 */
TEST(CodecTomlReader, TruncatedDecodingOutcome) {
	// Разбираемый текст настроек с ошибочной записью перед битой последовательностью
	const string text("x = 1\n[a.b.c]\xC3\x28\n");
	// Итог разбора текста настроек, поданного целиком
	toml::error_t expected = toml::error_t::NONE;
	// Место обнаружения ошибки разбора текста настроек, поданного целиком
	toml::location_t place;
	/**
	 * Выполняем перебор размеров куска подачи исходного текста
	 */
	for(size_t size : {size_t(0), size_t(1), size_t(2), size_t(3), size_t(5), size_t(8)}){
		// Настройки разбора текста настроек
		toml::reader_t::settings_t settings;
		// Устанавливаем наибольшее допустимое количество частей имени ключа
		settings.maxParts = 2;
		// Объект потокового чтения текста настроек
		toml::reader_t reader(::logger(), settings);
		/**
		 * Если текст подаётся целиком
		 */
		if(size == 0){
			// Выполняем подачу разбираемого текста настроек целиком
			static_cast <void> (reader.feed(text.data(), text.size(), true));
			/**
			 * Выполняем перебор выданных разбором событий
			 */
			while(reader.next()){}
			// Запоминаем итог разбора текста настроек
			expected = reader.error();
			// Запоминаем место обнаружения ошибки разбора
			place = reader.errorLocation();
			// Выполняем проверку того, что разбор ошибку обнаружил
			ASSERT_NE(expected, toml::error_t::NONE);
			// Выполняем переход к следующему размеру куска подачи
			continue;
		}
		/**
		 * Выполняем подачу исходного текста кусками выбранного размера
		 */
		for(size_t i = 0; i < text.size(); i += size){
			// Получаем размер очередного куска исходного текста
			const size_t part = ((size < (text.size() - i)) ? size : (text.size() - i));
			// Выполняем подачу очередного куска исходного текста
			static_cast <void> (reader.feed(text.data() + i, part, (i + part) == text.size()));
			/**
			 * Выполняем перебор выданных разбором событий
			 */
			while(reader.next()){}
		}
		// Выполняем проверку совпадения итога разбора с подачей целиком
		ASSERT_EQ(reader.error(), expected) << "кусками по " << size;
		// Выполняем проверку совпадения места обнаружения ошибки с подачей целиком
		ASSERT_EQ(reader.errorLocation().line, place.line) << "кусками по " << size;
		// Выполняем проверку совпадения столбца обнаружения ошибки с подачей целиком
		ASSERT_EQ(reader.errorLocation().column, place.column) << "кусками по " << size;
	}
}
/**
 * @brief Проверка учёта повторов во встроенной таблице, имени не имеющей
 *
 * @details Ключи встроенной таблицы повторами друг другу приходятся всегда, есть ли у
 * самой таблицы собственное имя или нет: описание запрещает повтор внутри одной
 * таблицы. Таблицы же соседние, значениями одного перечня записанные, друг другу
 * ключами не мешают - имена их различны
 *
 */
TEST(CodecTomlReader, AnonymousInlineDuplicates) {
	/**
	 * Выполняем перебор проверяемых текстов настроек
	 */
	for(const auto & item : {
		make_pair(string("t = [{a = 1, a = 2}]\n"), false),
		make_pair(string("t = [{a = 1, a.b = 2}]\n"), false),
		make_pair(string("t = [{a.b = 1, a = {c = 2}}]\n"), false),
		make_pair(string("t = [[{a = 1, a = 2}]]\n"), false),
		make_pair(string("t = [{b = {a = 1, a = 2}}]\n"), false),
		make_pair(string("t = [{a = 1}, {a = 2}]\n"), true),
		make_pair(string("t = [{a = 1, b = 2}]\n"), true),
		make_pair(string("t = [{a.b = 1, a.c = 2}]\n"), true),
		make_pair(string("t = [{a = 1}]\nu = [{a = 2}]\n"), true)
	}){
		/**
		 * Выполняем перебор размеров куска подачи исходного текста
		 */
		for(size_t size : {size_t(1), size_t(3), size_t(64)}){
			// Объект потокового чтения текста настроек
			toml::reader_t reader(::logger());
			/**
			 * Выполняем подачу исходного текста кусками выбранного размера
			 */
			for(size_t i = 0; i < item.first.size(); i += size){
				// Получаем размер очередного куска исходного текста
				const size_t part = ((size < (item.first.size() - i)) ? size : (item.first.size() - i));
				// Выполняем подачу очередного куска исходного текста
				static_cast <void> (reader.feed(item.first.data() + i, part, (i + part) == item.first.size()));
				/**
				 * Выполняем перебор выданных разбором событий
				 */
				while(reader.next()){}
			}
			// Выполняем проверку итога разбора текста настроек
			ASSERT_EQ((reader.error() == toml::error_t::NONE), item.second)
			 << "«" << item.first << "» кусками по " << size;
		}
	}
}
/**
 * @brief Проверка сохранения знака нулевого смещения часового пояса
 *
 * @details Описание отводит записи «-00:00» смысл, от «+00:00» отличный: первою
 * обозначено смещение неизвестное, второю - смещение, заведомо нулевое. Само смещение
 * знака не несёт, и держится он отдельным признаком
 *
 */
TEST(CodecTomlReader, NegativeZeroOffset) {
	/**
	 * Выполняем перебор проверяемых отметок времени
	 */
	for(const string & source : {
		string("1979-05-27T07:32:00-00:00"),
		string("1979-05-27T07:32:00+00:00"),
		string("1979-05-27T07:32:00Z"),
		string("1979-05-27T07:32:00-07:00"),
		string("1979-05-27T07:32:00+07:00")
	}){
		// Собираемый текст настроек с проверяемой отметкой времени
		const string text = ("a = " + source + "\n");
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора текста настроек
		ASSERT_TRUE(document.parse(text)) << "«" << source << "»";
		// Выполняем проверку того, что перезапись повторяет исходную запись
		ASSERT_EQ(document.text(), text);
	}
}
/**
 * @brief Проверка независимости разбора имени со знаками Юникода от нарезки на куски
 *
 * @details Оборванная последовательность UTF-8 от построенной ошибочно отличается
 * лишь приходом продолжения: судить о ней по накопленному тексту нельзя, покуда
 * текст не поступил целиком. Разбор длины последовательности по ведущему байту
 * собран единым телом с разбором самой последовательности - двумя телами своды
 * правил разошлись бы при первой же правке одного из них
 *
 */
TEST(CodecTomlReader, UnicodeKeyChunking) {
	// Настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Выполняем признание знаков Юникода в именах без кавычек
	settings.unicode = true;
	/**
	 * Выполняем перебор всех размеров куска подачи
	 */
	for(size_t chunk = 0; chunk < 12; chunk++){
		// Собранные события разбора
		vector <Event> events;
		// Выполняем проверку разбора имени со знаками Юникода
		ASSERT_EQ(consume("ключ = 1\n", chunk, settings, events), toml::state_t::FINISHED) << "кусок " << chunk;
		// Выполняем проверку собранного имени ключа
		ASSERT_EQ(events.at(0).path, "ключ") << "кусок " << chunk;
	}
	/**
	 * Выполняем перебор всех размеров куска подачи
	 */
	for(size_t chunk = 0; chunk < 12; chunk++){
		// Собранные события разбора
		vector <Event> events;
		/**
		 * Выполняем проверку отказа разбора имени с оборванной последовательностью
		 *
		 * @note Ведущий байт двухбайтовой последовательности продолжен пробелом:
		 *       последовательность построена ошибочно при любой нарезке текста
		 */
		ASSERT_EQ(consume("к\xD0 = 1\n", chunk, settings, events), toml::state_t::FAILED) << "кусок " << chunk;
	}
}
/**
 * @brief Проверка уплотнения накопителя разбора на тексте крупном
 *
 * @details Чтение изымает разобранное начало накопленного текста, когда оно перерастает
 *          порог, и переносит при этом пять смещений разом: основание, начало строки,
 *          начало записи, границу пробы и само смещение разбора. Ошибка в переносе этом
 *          сказалась бы местами событий и местом отказа, но лишь на текстах крупных
 *
 * @warning Заход этот пересечение трёх прогонов числило непройденным ничем: порог
 *          уплотнения велик, а тексты набора проверок, корпуса наречий и ворошителя
 *          малы - уплотнение не срабатывало ни разу, и перенос смещений не поверялся
 *
 * @note Сличаются и события, и МЕСТО ОТКАЗА за порогом уплотнения: события с местами
 *       верными выдавались бы и при потерянном основании, покуда отказа нет
 *
 * @warning Проверка стережёт ТРИ переноса из пяти - изъятие разобранного начала,
 *          основание и начало строки, - и всякий поверен снятием своим. Перенос начала
 *          записи и границы пробы ею НЕ стережётся: уплотнение не срабатывает, покуда
 *          запись открыта, - смещение разбора за неё не уходит, - и снятие переносов
 *          этих исхода не меняет вовсе. Проверять их нечем, покуда уплотнение записи
 *          открытой не касается, и подгонять проверку под них значило бы завести
 *          утверждение, ничего не стерегущее
 *
 */
TEST(CodecTomlReader, CompactsBufferOnLargeText) {
	// Собираемые настройки разбора текста настроек
	toml::reader_t::settings_t settings;
	// Разрешаем выдачу примечаний событиями разбора
	settings.emitComments = true;
	// Собираемый текст настроек крупнее порога уплотнения накопителя
	string text;
	/**
	 * Выполняем сборку текста настроек из записей с примечаниями
	 */
	for(uint32_t i = 0; i < 9000; i++)
		// Добавляем очередную запись текста настроек
		text.append("k" + std::to_string(i) + " = " + std::to_string(i) + " # примечание к записи\n");
	// Выполняем проверку того, что текст порог уплотнения перерастает
	ASSERT_GT(text.size(), 0x10000u);
	// Собранные события подачи текста целиком
	vector <Event> whole;
	// Выполняем проверку успешности разбора текста, поданного целиком
	ASSERT_EQ(consume(text, 0, settings, whole), toml::state_t::FINISHED);
	// Выполняем проверку того, что события разбора собраны
	ASSERT_FALSE(whole.empty());
	/**
	 * Выполняем перебор размеров куска подачи текста настроек
	 */
	for(const size_t chunk : {size_t(1), size_t(7), size_t(4096), size_t(65537)}){
		// Собранные события подачи текста кусками
		vector <Event> chunked;
		// Выполняем проверку успешности разбора текста, поданного кусками
		ASSERT_EQ(consume(text, chunk, settings, chunked), toml::state_t::FINISHED) << chunk;
		// Выполняем проверку количества собранных событий
		ASSERT_EQ(chunked.size(), whole.size()) << chunk;
		/**
		 * Выполняем перебор собранных событий разбора
		 */
		for(size_t i = 0; i < chunked.size(); i++){
			// Выполняем проверку разновидности очередного события
			ASSERT_EQ(chunked.at(i).event, whole.at(i).event) << chunk << ": событие " << i;
			// Выполняем проверку строки очередного события
			ASSERT_EQ(chunked.at(i).line, whole.at(i).line) << chunk << ": событие " << i;
			// Выполняем проверку столбца очередного события
			ASSERT_EQ(chunked.at(i).column, whole.at(i).column) << chunk << ": событие " << i;
			// Выполняем проверку имени ключа очередного события
			ASSERT_EQ(chunked.at(i).path, whole.at(i).path) << chunk << ": событие " << i;
			/**
			 * Выполняем проверку смещения очередного события от начала текста
			 *
			 * @note Смещение поверяет ОСНОВАНИЕ, уплотнением переносимое: строка со
			 *       столбцом считаются от начала строки и потерю основания не замечают
			 */
			ASSERT_EQ(chunked.at(i).offset, whole.at(i).offset) << chunk << ": событие " << i;
		}
	}
	/**
	 * Выполняем проверку места отказа, за порогом уплотнения стоящего
	 *
	 * @note Половина эта обязательна: события с местами верными выдавались бы и при
	 *       потерянном основании, покуда отказа нет. Место отказа считается от
	 *       основания, уплотнением перенесённого, и потеря его вскроется лишь тут
	 */
	{
		// Собираемый текст настроек с порченой записью за порогом уплотнения
		const string broken(text + "порченая строка без равенства\n");
		/**
		 * @brief Функция получения места отказа разбора
		 *
		 * @param chunk размер куска подачи, ноль означает подачу целиком
		 * @return      строка и столбец отказа разбора
		 *
		 */
		const auto refusal = [&settings](const size_t chunk, const string & broken) noexcept -> pair <uint32_t, uint32_t> {
			// Объект потокового чтения текста настроек
			toml::reader_t reader(::logger(), settings);
			// Размер подаваемого куска текста настроек
			const size_t size = (chunk > 0 ? chunk : broken.length());
			// Смещение начала очередного куска подачи
			size_t offset = 0;
			/**
			 * Выполняем подачу текста настроек кусками
			 */
			do {
				// Размер подаваемого куска текста настроек
				const size_t length = ((broken.length() - offset) < size ? (broken.length() - offset) : size);
				/**
				 * Если подача куска текста настроек не удалась
				 */
				if(!reader.feed(broken.data() + offset, length, ((offset + length) >= broken.length())))
					// Выходим из подачи текста настроек
					break;
				// Выполняем смещение начала очередного куска подачи
				offset += length;
				/**
				 * Выполняем чтение выданных разбором событий
				 */
				while(reader.next())
					// Выполняем пропуск очередного события разбора
					continue;
			} while(offset < broken.length());
			// Выводим место отказа разбора
			return make_pair(reader.errorLocation().line, reader.errorLocation().column);
		};
		// Получаем место отказа подачи текста целиком
		const pair <uint32_t, uint32_t> first = refusal(0, broken);
		// Выполняем проверку того, что отказ пришёлся на порченую запись
		ASSERT_EQ(first.first, 9001u);
		/**
		 * Выполняем перебор размеров куска подачи текста настроек
		 */
		for(const size_t chunk : {size_t(7), size_t(4096), size_t(65537)}){
			// Получаем место отказа подачи текста кусками
			const pair <uint32_t, uint32_t> second = refusal(chunk, broken);
			// Выполняем проверку строки отказа
			ASSERT_EQ(second.first, first.first) << chunk;
			// Выполняем проверку столбца отказа
			ASSERT_EQ(second.second, first.second) << chunk;
		}
		/**
		 * Выполняем проверку места отказа построения незакрытого
		 *
		 * @note Половина эта обязательна: место отказа незакрытого построения считается
		 *       от НАЧАЛА ЗАПИСИ, уплотнением переносимого особо. Отказ по знаку
		 *       недопустимому его не задевает вовсе, и потеря переноса вскроется лишь тут
		 */
		{
			// Собираемый текст настроек с построением, за порогом уплотнения незакрытым
			const string hanging(text + "hang = \"\"\"начало без конца\nвторая строка\n");
			// Получаем место отказа подачи текста целиком
			const pair <uint32_t, uint32_t> whole = refusal(0, hanging);
			// Выполняем проверку того, что отказ пришёлся на незакрытое построение
			ASSERT_EQ(whole.first, 9001u);
			/**
			 * Выполняем перебор размеров куска подачи текста настроек
			 */
			for(const size_t chunk : {size_t(7), size_t(4096), size_t(65537)}){
				// Получаем место отказа подачи текста кусками
				const pair <uint32_t, uint32_t> piece = refusal(chunk, hanging);
				// Выполняем проверку строки отказа
				ASSERT_EQ(piece.first, whole.first) << chunk;
				// Выполняем проверку столбца отказа
				ASSERT_EQ(piece.second, whole.second) << chunk;
			}
		}
	}
}
/**
 * @brief Проверка границ доли секунды у отметки времени
 *
 * @details Доля секунды хранится наносекундами, и разрядов ей отведено девять. Запись
 *          короче дополняется до наносекунд сдвигом, запись длиннее усекается: разряды
 *          сверх девяти отбрасываются, и описание дозволяет это прямо. Количество
 *          записанных разрядов хранится отдельно ради обратной записи
 *
 * @note Граница эта картою охвата не берётся: строка сравнения разрядов исполняется при
 *       всякой доле подряд, а решает тут именно число девять. Взята она перебором
 *       соседних длин - восемь, девять, десять
 *
 * @warning Порчею доказана граница: подмена «меньше девяти» на «не больше девяти»
 *          проверку валит - десятый разряд входит в наносекунды, и доля выходит вдесятеро
 *          больше. Тем проверка и стережёт САМО число, а не усечение вообще
 *
 */
TEST(CodecTomlReader, FractionBoundaries) {
	/**
	 * Записи доли секунды вместе с ожиданием разбора
	 */
	const struct {
		// Запись доли секунды отметки времени
		const char * fraction;
		// Ожидаемая доля секунды наносекундами
		uint32_t nanosecond;
		// Ожидаемое количество записанных разрядов
		uint8_t digits;
	} samples[] = {
		// Один разряд дополняется до наносекунд
		{"1", 100000000u, 1},
		// Восемь разрядов дополняются до наносекунд одним сдвигом
		{"12345678", 123456780u, 8},
		// Девять разрядов ложатся наносекундами как есть
		{"123456789", 123456789u, 9},
		// Десятый разряд отбрасывается
		{"1234567890", 123456789u, 9},
		// Разряды сверх десятого отбрасываются тоже
		{"123456789099", 123456789u, 9}
	};
	/**
	 * Выполняем перебор проверяемых записей доли секунды
	 */
	for(auto & sample : samples){
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись текста с отметкой времени
		const string text = string("a = 1979-05-27T07:32:00.") + sample.fraction + "Z\n";
		// Выполняем проверку успешности разбора текста
		ASSERT_TRUE(reader.feed(text.data(), text.size(), true)) << sample.fraction;
		// Собранная отметка времени
		toml::stamp_t stamp;
		// Признак того, что отметка времени собрана
		bool taken = false;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если событием является значение
			 */
			if(reader.event() == toml::event_t::VALUE){
				// Запоминаем собранную отметку времени
				stamp = reader.value().stamp;
				// Запоминаем признак того, что отметка времени собрана
				taken = true;
			}
		}
		// Выполняем проверку того, что отметка времени собрана
		ASSERT_TRUE(taken) << sample.fraction;
		// Выполняем проверку доли секунды отметки времени
		ASSERT_EQ(stamp.time.nanosecond, sample.nanosecond) << sample.fraction;
		// Выполняем проверку количества записанных разрядов доли секунды
		ASSERT_EQ(stamp.time.digits, sample.digits) << sample.fraction;
	}
	/**
	 * Выполняем проверку отказа доли секунды без разрядов
	 *
	 * @note Разделитель доли, разрядов за собою не имеющий, отметку времени не задаёт:
	 *       проверка держит границу снизу
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Выполняем проверку отказа разбора доли секунды без разрядов
		// Разбираемый текст с долей секунды без разрядов
		const string text("a = 1979-05-27T07:32:00.Z\n");
		// Выполняем проверку отказа разбора доли секунды без разрядов
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
	}
}

/**
 * @brief Проверка обхода, ограды и недопустимых знаков разбором строкового значения
 *
 * @details Пересечение прогона проверок с прогоном ворошителя числило непройденными
 *          пять заходов разбора строкового значения: возврат каретки сразу за
 *          открывающей оградой многострочного значения, кавычки числом свыше пяти
 *          подряд, одинокий возврат каретки в конце текста, обходы забоя и перевода
 *          страницы и обход, языку не ведомый
 *
 * @note Забой и перевод страницы стерегутся здесь с обеих сторон: запись их обходит,
 *       а чтение возвращает обратно. Без пары этой обход и разбор разошлись бы, и
 *       перезапись значения потеряла бы знак
 *
 * @warning Сырого управляющего знака области C0 тут НЕТ нарочно: отказ на него даёт
 *          страж, строковому значению предшествующий, и до стража внутри значения
 *          дело не доходит. Порча последнего проверку не роняла - ветвь эта отказ
 *          лишь изображала бы, стерегомого не задевая
 *
 */
TEST(CodecTomlReader, StringEscapesAndFences) {
	/**
	 * Выполняем проверку отбрасывания возврата каретки за открывающей оградой
	 */
	{
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора текста настроек
		ASSERT_TRUE(document.parse("k = \"\"\"\r\nтекст\"\"\"\n"));
		// Выполняем проверку того, что первый перевод строки содержимому не достался
		ASSERT_EQ(document.text({"k"}), "текст");
	}
	/**
	 * Выполняем проверку обхода забоя и перевода страницы разбором
	 */
	{
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора текста настроек
		ASSERT_TRUE(document.parse("k = \"a\\bb\\fc\"\n"));
		// Выполняем проверку того, что обходы обращены в знаки
		ASSERT_EQ(document.text({"k"}), "a\bb\fc");
	}
	/**
	 * Выполняем проверку отказа на обход, языку не известный
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись текста с обходом неведомым
		const string text = "k = \"a\\qb\"\n";
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::INVALID_ESCAPE);
	}
	/**
	 * Выполняем проверку отказа на кавычки числом свыше пяти подряд
	 *
	 * @note Пять кавычек подряд дозволены: две принадлежат содержимому, три ограде
	 */
	{
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку разбора текста с пятью кавычками подряд
		ASSERT_TRUE(document.parse("k = \"\"\"a\"\"\"\"\"\n"));
		// Выполняем проверку того, что две кавычки достались содержимому
		ASSERT_EQ(document.text({"k"}), "a\"\"");
	}{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись текста с кавычками числом свыше пяти
		const string text = "k = \"\"\"a\"\"\"\"\"\"\n";
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::INVALID_VALUE);
	}
	/**
	 * Выполняем проверку отказа на одинокий возврат каретки в конце текста
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись текста, возвратом каретки оборванная
		const string text = "k = \"\"\"a\r";
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::INVALID_CHARACTER);
	}
}
/**
 * @brief Проверка кодов отказа, ни одной проверкой не сличаемых
 *
 * @details Одиннадцать кодов отказа набор проверок не сличал ни разу: отказ проверялся
 *          самим отказом, а причина его - нет. Код отказа есть договор кодека с
 *          потребителем наравне с самим отказом: по нему потребитель судит, чинить ли
 *          текст, просить ли новый кусок либо поднимать предел, и код неверный уводит
 *          его в сторону, тогда как отказ остаётся на месте
 *
 * @note Имена таблиц и ключей взяты латиницей нарочно: имя простого ключа TOML
 *       дозволяет одни лишь знаки ASCII, и имя кириллицей даёт отказ пустого имени
 *       прежде всякого иного - на этом заходы попались трижды
 *
 * @note Кода INTERNAL тут нет: он есть застава последнего рубежа, доводом
 *       недостижимая. Кода OVERFLOW_LIMIT нет тоже: разбор не выдаёт его нигде - он
 *       заведён заделом на будущее
 *
 * @note Кода UNEXPECTED_EOF тут нет по доводу измеренному: обрыв текста разбор метит
 *       кодом ТОЧНЕЙШИМ из подходящих - «a = [1, 2» даёт UNCLOSED_ARRAY, «a = » даёт
 *       MISSING_VALUE, - и код обрыва остаётся запасным на случай, когда точнее
 *       сказать нечего. Заходом его подобрать не удалось
 *
 */
TEST(CodecTomlReader, RefusalCodes) {
	/**
	 * @brief Описание проверяемого захода отказа
	 *
	 */
	struct probe_t {
		// Пояснение проверяемого захода
		const char * note;
		// Разбираемый текст настроек
		const char * text;
		// Ожидаемый код отказа разбора
		toml::error_t error;
	};
	// Набор проверяемых заходов отказа
	static const probe_t PROBES[] = {
		{"имя без разделителя", "a 1\n", toml::error_t::MISSING_EQUALS},
		{"строка незакрытая", "a = \"value\n", toml::error_t::UNTERMINATED_STRING},
		{"объявление таблицы оборвано скобкой", "[", toml::error_t::UNCLOSED_TABLE},
		{"за именем таблицы знак лишний (имя латиницей: имя простого ключа TOML только ASCII)", "[table;]\n", toml::error_t::INVALID_TABLE},
		{"имя встроенной таблицы объявлено дважды", "a = {b = 1, b = 2}\n", toml::error_t::DUPLICATE_KEY}
	};
	/**
	 * Выполняем перебор проверяемых заходов отказа
	 */
	for(auto & probe : PROBES){
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись разбираемого текста
		const string text(probe.text);
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true)) << probe.note;
		// Выполняем проверку выданного кода отказа разбора
		ASSERT_EQ(reader.error(), probe.error) << probe.note;
	}
	/**
	 * Выполняем проверку отказа объявления таблицы дважды
	 *
	 * @note Отказ этот выдаёт дерево настроек, а не разбор: разбор объявления таблиц
	 *       не помнит, помнит их построение дерева
	 */
	{
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(document.parse("[table]\n[table]\n"));
		// Выполняем проверку выданного кода отказа разбора
		ASSERT_EQ(document.error(), toml::error_t::DUPLICATE_TABLE);
	}
	/**
	 * Выполняем проверку отказа дополнения встроенной таблицы
	 *
	 * @note Отказ этот выдаёт дерево настроек: разбор о том, что имя «a» объявлено
	 *       встроенной таблицей строкою выше, не помнит
	 */
	{
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(document.parse("a = {b = 1}\na.c = 2\n"));
		// Выполняем проверку выданного кода отказа разбора
		ASSERT_EQ(document.error(), toml::error_t::EXTEND_INLINE_TABLE);
	}
	/**
	 * Выполняем проверку отказа перечня таблиц поверх таблицы
	 */
	{
		// Объект дерева настроек
		toml::document_t document(::logger());
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(document.parse("[table]\n[[table]]\n"));
		// Выполняем проверку выданного кода отказа разбора
		ASSERT_EQ(document.error(), toml::error_t::APPEND_TO_TABLE);
	}
}
/**
 * @brief Проверка событий объявления набора таблиц и встроенной таблицы
 *
 * @details Виды событий ARRAY_TABLE, INLINE_OPEN и INLINE_CLOSE набор проверок не
 *          сличал ни разу: потребитель потокового разбора разбирает выдачу по виду
 *          события, и вид неверный уводит его в сторону молча - выдача остаётся, а
 *          смысл её меняется. Набор таблиц потребитель тем самым принял бы за таблицу
 *          обычную, а встроенную таблицу - за перечень
 *
 * @note Судится ряд событий дословно вместе с именами ключей: вид события без имени
 *       ключа договора не выражает - по имени потребитель и строит своё дерево
 *
 */
TEST(CodecTomlReader, ArrayTableAndInlineEvents) {
	// Объект потокового чтения текста
	toml::reader_t reader(::logger());
	// Разбираемый текст с набором таблиц и встроенной таблицей
	const string text = "[[items]]\nname = \"первый\"\npoint = {x = 1, y = 2}\n";
	// Выполняем проверку успешности разбора текста
	ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
	// Собираемый ряд событий разбора
	string result;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		// Выполняем запись вида очередного события числом
		result.append(std::to_string(static_cast <uint32_t> (reader.event())));
		/**
		 * Если событие имя ключа несёт
		 */
		if(reader.event() == toml::event_t::KEY){
			/**
			 * Выполняем перебор составных частей имени ключа
			 */
			for(auto & part : reader.path())
				// Выполняем запись составной части имени ключа
				result.append("«").append(part.name).append("»");
		}
		// Выполняем запись разделителя событий
		result.append("\n");
	}
	// Выполняем проверку собранного ряда событий разбора
	ASSERT_EQ(result,
		// Объявление очередной таблицы набора таблиц
		string(std::to_string(static_cast <uint32_t> (toml::event_t::ARRAY_TABLE))).append("\n")
		// Имя простого ключа вместе со значением его
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::KEY))).append("«name»\n")
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::VALUE))).append("\n")
		// Имя ключа встроенной таблицы вместе с её открытием
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::KEY))).append("«point»\n")
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::INLINE_OPEN))).append("\n")
		// Пары встроенной таблицы
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::KEY))).append("«x»\n")
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::VALUE))).append("\n")
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::KEY))).append("«y»\n")
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::VALUE))).append("\n")
		// Закрытие встроенной таблицы
		.append(std::to_string(static_cast <uint32_t> (toml::event_t::INLINE_CLOSE))).append("\n"));
}
/**
 * @brief Проверка события завершения разбора
 *
 * @details Виды события FINISH и NONE набор проверок не сличал ни разу, а ими
 *          потребитель отличает разбор, дошедший до конца, от оборванного отказом и от
 *          не начатого вовсе: перебор событий кончается в обоих случаях одинаково, и
 *          без этих видов потребителю остаётся судить по коду ошибки, тогда как
 *          договор даёт признак прямой
 *
 * @note Вид NONE выдаётся ДО перебора, вид FINISH - после него: оба видны вне цикла
 *       разбора, выдачей они не являются и в перебор не попадают
 *
 */
TEST(CodecTomlReader, FinishEvent) {
	/**
	 * Выполняем проверку вида события до начала перебора
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись разбираемого текста
		const string text = "k = 1\n";
		// Выполняем проверку успешности подачи текста
		ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
		// Выполняем проверку того, что событие до перебора не определено
		ASSERT_EQ(reader.event(), toml::event_t::NONE);
	}
	/**
	 * Выполняем проверку события завершения у текста годного
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись разбираемого текста
		const string text = "[table]\nk = 1\n";
		// Выполняем проверку успешности разбора текста
		ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next());
		// Выполняем проверку отсутствия ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::NONE);
		// Выполняем проверку того, что разбор дошёл до конца
		ASSERT_EQ(reader.event(), toml::event_t::FINISH);
	}
	/**
	 * Выполняем проверку события у текста, отказом оборванного
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись текста с именем без разделителя
		const string text = "k 1\n";
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), toml::error_t::MISSING_EQUALS);
		// Выполняем проверку того, что событие завершения не выдано
		ASSERT_NE(reader.event(), toml::event_t::FINISH);
	}
}
/**
 * @brief Проверка отказа отметки времени с хвостом за обозначением пояса
 *
 * @details Знак «Z» обозначает часовой пояс UTC и стоит последним: содержимое за ним
 *          отметке не принадлежит, и запись такая отметкою не является вовсе. Строку
 *          этого отказа пересечение трёх прогонов числило слепой - отметки разбирались
 *          записанные верно
 *
 */
TEST(CodecTomlReader, StampRefusesTailAfterZone) {
	// Дерево настроек
	toml::document_t document(::logger());
	// Выполняем проверку разбора отметки времени, записанной верно
	ASSERT_TRUE(document.parse("stamp = 2026-08-30T10:00:00Z\n"));
	// Второе дерево настроек
	toml::document_t broken(::logger());
	// Выполняем проверку отказа разбора отметки с хвостом за обозначением пояса
	ASSERT_FALSE(broken.parse("stamp = 2026-08-30T10:00:00Zx\n"));
}
/**
 * @brief Проверка того, что копия переживает подачу, а вид нет
 *
 * @details Виды, чтением отдаваемые, ссылаются на память разбора и живут лишь до
 * следующего обращения к @c next() либо @c feed(): подача перевыделяет хранилище, и вид,
 * удержанный прежде, указывает на память освобождённую
 *
 * @warning Проверка закрепляет ВЕРНЫЙ образец работы, а не беду: обращение к
 *          обесцененному виду есть неопределённое поведение, и проверять его нельзя -
 *          оно закрепляло бы не договор кодека, а сегодняшнее поведение выделителя
 *          памяти. Беда замерена отдельным щупом под надзором за памятью:
 *          `heap-use-after-free` на двухстах подачах, и щуп в набор не идёт
 *
 * @note Ручательство это стояло у одного лишь тела из пяти, а соседи молчали. Молчание
 *       рядом с громким предупреждением соседа само по себе вводит в заблуждение -
 *       довод Василия, проверенный замером и принятый
 *
 * @warning Имя ключа взято ЛАТИНИЦЕЙ намеренно: простое имя наречия TOML дозволяет одни
 *          лишь знаки ASCII, и имя кириллицей даёт отказ пустого имени прежде всякого
 *          иного. На этом проверка попалась при написании - событий значения не
 *          выдавалось вовсе, а копия выходила пустой
 *
 */
TEST(CodecTomlReader, CopySurvivesFeedUnlikeView) {
	// Объект потокового чтения текста настроек
	toml::reader_t reader(::logger());
	// Первый кусок исходного текста
	const string first = "key = \"значение\"\n";
	/**
	 * Выполняем подачу первого куска исходного текста
	 *
	 * @note Кусок подаётся ПОСЛЕДНИМ намеренно: чтение TOML откладывает запись, покуда
	 *       текст вправе прирасти, и при незакрытой подаче события значения не выдаёт
	 *       вовсе - замерено, копия выходила пустой. Хранилище разбора при этом всё
	 *       равно перевыделяется подачей следующего чтения того же объекта
	 */
	reader.feed(first.data(), first.size(), true);
	// Снятая КОПИЯ содержимого, а не вид на него
	string kept;
	/**
	 * Выполняем перебор событий разбора первого куска
	 */
	while(reader.next()){
		/**
		 * Если событием является свойство со значением
		 */
		if(reader.event() == toml::event_t::VALUE)
			// Снимаем копию содержимого, а не вид на него
			kept.assign(reader.value().text);
	}
	// Выполняем проверку снятой копии
	ASSERT_EQ(kept, "значение");
	// Собираемый второй кусок исходного текста
	string more;
	/**
	 * Выполняем сборку второго куска, хранилище разбора перевыделяющего
	 */
	for(uint32_t i = 0; i < 200; i++)
		// Выполняем добавление очередной записи ко второму куску
		more.append("k" + std::to_string(i) + " = \"значение\"\n");
	/**
	 * Выполняем сброс чтения и подачу второго куска исходного текста
	 *
	 * @note Сброс обязателен: чтение, текст целиком получившее, дальнейших подач не
	 *       принимает. Хранилище разбора он освобождает, и вид, удержанный прежде,
	 *       обесценивается им же
	 */
	reader.clear();
	// Выполняем подачу второго куска исходного текста
	reader.feed(more.data(), more.size(), true);
	// Выполняем перебор событий разбора второго куска
	while(reader.next());
	// Выполняем проверку того, что разбор второго куска удался
	ASSERT_EQ(reader.error(), toml::error_t::NONE);
	/**
	 * Выполняем проверку того, что снятая копия подачу пережила
	 *
	 * @note Вид на том же месте её бы не пережил - оттого договор и велит держать копию
	 */
	ASSERT_EQ(kept, "значение");
}
/**
 * @brief Проверка того, что виды копии чтения указывают в исходник
 *
 * @details Свёртки событий хранятся готовыми видами в строки самого чтения, и заводитель
 * копии, неявный, переносит их как есть: виды копии продолжают указывать в память
 * ИСХОДНОГО объекта. Покуда исходник жив, копия читается верно
 *
 * @warning Переживи копия исходник - обращение к её видам есть обращение к
 *          освобождённому: надзор за памятью валит это `stack-use-after-scope`, замерено
 *          щупом. Проверка закрепляет ВЕРНЫЙ образец - копия при живом исходнике, - а
 *          беду не читает ни разу: она есть неопределённое поведение
 *
 * @note Довод принесён Василием от кодеков JSON и XML, где виды хранятся указаниями в
 *       своё хранилище и копия перебазирует их сама собою. Здесь устройство иное, и
 *       ручательство обратное. Правка договора вынесена владельцу
 *
 */
TEST(CodecTomlReader, CopyViewsPointIntoSource) {
	// Объект потокового чтения текста настроек
	toml::reader_t reader(::logger());
	// Исходный текст настроек
	const string text = "key = \"значение\"\n";
	// Выполняем подачу исходного текста целиком
	reader.feed(text.data(), text.size(), true);
	/**
	 * Выполняем перебор событий до искомого
	 */
	while(reader.next()){
		// Если событие искомое - выходим из перебора
		if(reader.event() == toml::event_t::VALUE)
			// Выполняем прекращение перебора событий
			break;
	}
	// Выполняем проверку того, что событие получено
	ASSERT_EQ(reader.event(), toml::event_t::VALUE);
	// Копия чтения, заводителем неявным собранная
	toml::reader_t copy = reader;
	// Выполняем проверку того, что копия читается верно при живом исходнике
	ASSERT_EQ(copy.value().text, "значение");
	/**
	 * Выполняем проверку того, что вид копии указывает в память ИСХОДНИКА
	 *
	 * @note Сличаются АДРЕСА, а не содержимое: содержимое совпало бы и у перебазированных
	 *       видов, и отличить одно от другого можно лишь указателем
	 */
	ASSERT_EQ(static_cast <const void *> (copy.value().text.data()),
	          static_cast <const void *> (reader.value().text.data()));
}
/**
 * @brief Проверка имени пары, разрезанного границей куска посреди знака
 *
 * @details Знак, границей куска разорванный, от знака битого отличается лишь приходом
 *          продолжения: судить о нём по накопленному тексту нельзя, покуда текст не
 *          поступил целиком. Оттого разбор имени просит продолжения, а не отвергает
 *          текст, и строку эту пересечение трёх прогонов числило слепой - имена
 *          подавались кусками, знаков не разрезающими
 *
 */
TEST(CodecTomlReader, NameSplitInsideCharacter) {
	// Объект чтения текста настроек
	toml::reader_t reader(::logger());
	// Имя пары, знаками Юникода записанное
	const string name = "имя";
	// Первый кусок текста настроек, знак имени разрезающий
	const string first = string("\"") + name.substr(0, 3);
	// Второй кусок текста настроек, знак завершающий
	const string second = name.substr(3) + "\" = 1\n";
	// Выполняем подачу первого куска текста, концом текста не объявленного
	ASSERT_TRUE(reader.feed(first.data(), first.size(), false));
	// Выполняем подачу второго куска текста концом текста
	ASSERT_TRUE(reader.feed(second.data(), second.size(), true));
	// Выполняем снятие событий разбора
	while(reader.next());
	// Выполняем проверку того, что разбор отказом не сорван
	ASSERT_EQ(reader.error(), toml::error_t::NONE);
}
/**
 * @brief Проверка отказов разбора, покрытием не задетых
 *
 * @details Заходы эти пересечение трёх прогонов числило слепыми: ни набор проверок,
 * ни ворошитель, ни корпус наречий не подавали ни возврата каретки без перевода
 * строки, ни суррогата управляющей последовательностью, ни числа сверх отрезка
 * значений хранилища. Отказ, ни разу не исполненный, обещания своего не даёт
 *
 * @note Судится вместе с кодом отказа, а не одним лишь отказом: код уходит
 *       потребителю причиною, и подмена его увела бы его искать иное
 *
 */
TEST(CodecTomlReader, RefusalsNotCoveredBefore) {
	/**
	 * @brief Описание проверяемого захода отказа
	 *
	 */
	struct probe_t {
		// Пояснение проверяемого захода
		const char * note;
		// Разбираемый текст настроек
		const char * text;
		// Ожидаемый код отказа разбора
		toml::error_t error;
	};
	// Набор проверяемых заходов отказа
	static const probe_t PROBES[] = {
		{"возврат каретки без перевода строки внутри значения", "a = \"до\rпосле\"\n", toml::error_t::INVALID_CHARACTER},
		{"возврат каретки без перевода строки в конце текста", "a = 1\r", toml::error_t::INVALID_CHARACTER},
		{"суррогат управляющей последовательностью", "a = \"\\uD800\"\n", toml::error_t::INVALID_ESCAPE},
		{"кодовое значение свыше предела Юникода", "a = \"\\U00110000\"\n", toml::error_t::INVALID_ESCAPE},
		{"целое число сверх отрезка значений хранилища", "a = 0xFFFFFFFFFFFFFFFFFF\n", toml::error_t::NUMBER_OVERFLOW},
		{"число с плавающей точкой сверх отрезка значений", "a = 1e999\n", toml::error_t::NUMBER_OVERFLOW},
		{"смещение часового пояса без минут", "a = 1979-05-27T07:32:00+01\n", toml::error_t::INVALID_DATETIME},
		{"значение за знаком равенства отсутствует", "a = ", toml::error_t::MISSING_VALUE},
		{"перечень значений незакрыт", "a = [1,", toml::error_t::UNCLOSED_ARRAY},
		{"управляющий знак сырым внутри строкового значения", "a = \"до\x01после\"\n", toml::error_t::INVALID_CHARACTER},
		{"управляющий знак сырым внутри строки многострочной", "a = \"\"\"до\x01после\"\"\"\n", toml::error_t::INVALID_CHARACTER},
		{"запись за смещением часового пояса не исчерпана", "a = 1979-05-27T07:32:00+01:00x\n", toml::error_t::INVALID_DATETIME}
	};
	/**
	 * Выполняем перебор проверяемых заходов отказа
	 */
	for(auto & probe : PROBES){
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись разбираемого текста
		const string text(probe.text);
		// Выполняем проверку отказа разбора текста
		EXPECT_FALSE(reader.feed(text.data(), text.size(), true)) << probe.note;
		// Выполняем проверку выданного кода отказа разбора
		EXPECT_EQ(reader.error(), probe.error) << probe.note;
	}
}
/**
 * @brief Проверка договора чтения после отказа и после завершения разбора
 *
 * @details Разбор, отказом прекращённый либо завершённый, продолжения не принимает:
 * подача следом означала бы текст, начало которого разобрано, а хвост отброшен молча.
 * Настройки посреди разбора не меняются тем же доводом
 *
 */
TEST(CodecTomlReader, FeedRefusedAfterEnd) {
	/**
	 * Выполняем проверку отказа подачи следом за разбором завершённым
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись разбираемого текста
		const string text("a = 1\n");
		// Выполняем разбор текста настроек целиком
		ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
		/**
		 * Выполняем перебор всех событий разбора до исчерпания очереди
		 *
		 * @warning Перебор этот обязателен: подача текста лишь копит его, а состояние
		 *          завершённого разбора запоминается САМИМ разбором, идущим у выдачи
		 *          событий. Без перебора подача следом проходила бы сторожа насквозь -
		 *          замерено покрытием, строка отказа оставалась слепой
		 */
		while(reader.next());
		// Выполняем проверку отказа подачи следом за завершением разбора
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
		// Выполняем проверку отказа смены настроек следом за завершением разбора
		ASSERT_FALSE(reader.settings(reader.settings()));
	}
	/**
	 * Выполняем проверку отказа подачи следом за отказом разбора
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись разбираемого текста
		const string text("a 1\n");
		// Выполняем проверку отказа разбора текста настроек
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
		// Выполняем проверку отказа подачи следом за отказом разбора
		ASSERT_FALSE(reader.feed(text.data(), text.size(), true));
	}
	/**
	 * Выполняем проверку отказа смены настроек посреди разбора
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись начала разбираемого текста
		const string text("a = 1\n");
		// Выполняем подачу начала разбираемого текста
		ASSERT_TRUE(reader.feed(text.data(), text.size(), false));
		// Получаем текущие настройки разбора текста настроек
		toml::reader_t::settings_t settings = reader.settings();
		// Запоминаем предел длины строки разбираемого текста настроек
		settings.maxLine = 4;
		// Выполняем проверку отказа смены настроек посреди разбора
		ASSERT_FALSE(reader.settings(settings));
		// Выполняем проверку того, что определённая кодировка выдаётся
		ASSERT_EQ(reader.encoding(), toml::encoding_t::UTF8);
	}
}
/**
 * @brief Проверка разбора текста, поданного по одному байту
 *
 * @details Договор потокового разбора у AWH один: выдача не зависит от того, как
 * исходный текст нарезан на куски. Подача по одному байту кладёт границу куска
 * посреди всякой записи - имени, значения, объявления таблицы, встроенной таблицы,
 * примечания, - и разбор обязан откладывать решение до прихода продолжения
 *
 * @note Тексту нарочно даны все виды записей разом: откладывание решения устроено у
 *       каждой из них своё, и текст из одних лишь пар оставлял бы прочие нетронутыми
 *
 */
TEST(CodecTomlReader, SingleByteChunks) {
	// Разбираемый текст настроек со всеми видами записей
	const string text =
		"# примечание\r\n"
		"key = \"значение\"\r\n"
		"[table]\r\n"
		"nested.pair = 1\r\n"
		"array = [1, 2, 3]\r\n"
		"inline = {a = 1, b = \"два\"}\r\n"
		"stamp = 1979-05-27 07:32:00\r\n"
		"[[set]]\r\n"
		"name = \'дословно\' # хвостовое примечание\r\n";
	// Собранный ряд видов событий разбора текста, поданного целиком
	vector <toml::event_t> whole;
	/**
	 * Выполняем разбор текста, поданного целиком
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Выполняем подачу разбираемого текста целиком
		ASSERT_TRUE(reader.feed(text.data(), text.size(), true));
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next())
			// Выполняем запоминание вида очередного события разбора
			whole.push_back(reader.event());
		// Выполняем проверку того, что разбор отказом не завершился
		ASSERT_EQ(reader.error(), toml::error_t::NONE);
	}
	// Собранный ряд видов событий разбора текста, поданного по одному байту
	vector <toml::event_t> parted;
	/**
	 * Выполняем разбор текста, поданного по одному байту
	 */
	{
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		/**
		 * Выполняем подачу разбираемого текста по одному байту
		 */
		for(size_t i = 0; i < text.size(); i++){
			// Выполняем подачу очередного байта разбираемого текста
			ASSERT_TRUE(reader.feed(text.data() + i, 1, ((i + 1) >= text.size()))) << i;
			/**
			 * Выполняем перебор событий, очередным байтом добытых
			 */
			while(reader.next())
				// Выполняем запоминание вида очередного события разбора
				parted.push_back(reader.event());
		}
		// Выполняем проверку того, что разбор отказом не завершился
		ASSERT_EQ(reader.error(), toml::error_t::NONE);
	}
	// Выполняем проверку совпадения рядов событий обеих подач
	ASSERT_EQ(whole, parted);
	// Выполняем проверку того, что разбор событиями не пуст
	ASSERT_GT(whole.size(), static_cast <size_t> (10));
}
/**
 * @brief Проверка разбора многострочной записи, границей куска разорванной
 *
 * @details Разбор записи заводится лишь тогда, когда строка накоплена целиком, - и
 * оттого записи однострочные посреди себя накопленного текста не исчерпывают вовсе.
 * Записи же многострочные - перечень значений да строка в тройных кавычках - строкою
 * не оканчиваются, и накопленный текст вправе исчерпаться посреди них: разбор обязан
 * отложить решение до прихода продолжения, а не оборвать запись
 *
 * @note Проверяется подачею двумя кусками с границей ВНУТРИ записи: подача целиком
 *       этих путей не задевает, а подача по одному байту останавливается на первой
 *       же строке - до многострочных записей она не доходит
 *
 */
TEST(CodecTomlReader, MultilineRecordSplitAcrossChunks) {
	/**
	 * @brief Описание проверяемой многострочной записи
	 *
	 */
	struct probe_t {
		// Пояснение проверяемой записи
		const char * note;
		// Начало разбираемого текста настроек
		const char * head;
		// Продолжение разбираемого текста настроек
		const char * tail;
	};
	// Набор проверяемых многострочных записей
	static const probe_t PROBES[] = {
		{"перечень значений", "list = [\n1,\n", "2\n]\n"},
		{"перечень значений с примечанием", "list = [\n1, # первое\n", "2\n]\n"},
		{"строка в тройных кавычках", "text = \"\"\"первая\n", "вторая\"\"\"\n"},
		{"дословная строка в тройных кавычках", "text = '''первая\n", "вторая'''\n"},
		{"перечень внутри перечня", "list = [\n[1,\n", "2],\n3]\n"},
		{"встроенная таблица внутри перечня", "list = [\n{a = 1},\n", "{b = 2}\n]\n"}
	};
	/**
	 * Выполняем перебор проверяемых многострочных записей
	 */
	for(auto & probe : PROBES){
		// Собираемая запись начала разбираемого текста
		const string head(probe.head);
		// Собираемая запись продолжения разбираемого текста
		const string tail(probe.tail);
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Выполняем подачу начала разбираемого текста
		EXPECT_TRUE(reader.feed(head.data(), head.size(), false)) << probe.note;
		/**
		 * Выполняем перебор событий, началом текста добытых
		 */
		while(reader.next());
		// Выполняем проверку того, что разбор отказом не завершился
		EXPECT_EQ(reader.error(), toml::error_t::NONE) << probe.note;
		// Выполняем подачу продолжения разбираемого текста
		EXPECT_TRUE(reader.feed(tail.data(), tail.size(), true)) << probe.note;
		/**
		 * Выполняем перебор событий, продолжением текста добытых
		 */
		while(reader.next());
		// Выполняем проверку того, что разбор отказом не завершился
		EXPECT_EQ(reader.error(), toml::error_t::NONE) << probe.note;
	}
}
/**
 * @brief Проверка отказов разбора внутри записи многострочной
 *
 * @details Запись многострочная разбирается иначе однострочной: строка накоплена
 * целиком, а запись за неё выходит, и заходы отказа внутри такой записи стоят свои.
 * Покрытием они не задевались - наборы подавали негодное однострочными записями
 *
 * @note Судится вместе с кодом отказа: код уходит потребителю причиною, и подмена
 *       его увела бы его искать иное
 *
 */
TEST(CodecTomlReader, RefusalsInsideMultilineRecord) {
	/**
	 * @brief Описание проверяемого захода отказа
	 *
	 */
	struct probe_t {
		// Пояснение проверяемого захода
		const char * note;
		// Разбираемый текст настроек
		const char * text;
		// Ожидаемый код отказа разбора
		toml::error_t error;
	};
	// Набор проверяемых заходов отказа
	static const probe_t PROBES[] = {
		{"возврат каретки без перевода строки в строке многострочной", "text = \"\"\"до\rпосле\"\"\"\n", toml::error_t::INVALID_CHARACTER},
		{"суррогат управляющей последовательностью в строке многострочной", "text = \"\"\"до\\uD800после\"\"\"\n", toml::error_t::INVALID_ESCAPE},
		{"последовательность неизвестная в строке многострочной", "text = \"\"\"до\\qпосле\"\"\"\n", toml::error_t::INVALID_ESCAPE},
		{"возврат каретки без перевода строки внутри перечня", "list = [\n1\r", toml::error_t::INVALID_CHARACTER},
		{"целое число сверх отрезка значений хранилища", "a = 99999999999999999999\n", toml::error_t::NUMBER_OVERFLOW},
		{"знак числа без самого числа", "a = +\n", toml::error_t::INVALID_NUMBER},
		{"приставка системы счисления без разрядов", "a = 0x\n", toml::error_t::INVALID_NUMBER},
		{"разделитель разрядов в конце числа", "a = 1_\n", toml::error_t::INVALID_NUMBER},
		{"смещение часового пояса записано неполно", "a = 1979-05-27T07:32:00+01:0\n", toml::error_t::INVALID_DATETIME},
		{"час смещения часового пояса за пределом суток", "a = 1979-05-27T07:32:00+24:00\n", toml::error_t::INVALID_DATETIME},
		{"минуты смещения часового пояса за пределом часа", "a = 1979-05-27T07:32:00+01:60\n", toml::error_t::INVALID_DATETIME},
		{"суррогат длинной последовательностью в строке однострочной", "a = \"\\U0000D800\"\n", toml::error_t::INVALID_ESCAPE},
		{"кодовое значение свыше предела длинной последовательностью", "a = \"\\U7FFFFFFF\"\n", toml::error_t::INVALID_ESCAPE},
		{"суррогат в строке многострочной длинной последовательностью", "a = \"\"\"\\U0000D800\"\"\"\n", toml::error_t::INVALID_ESCAPE}
	};
	/**
	 * Выполняем перебор проверяемых заходов отказа
	 */
	for(auto & probe : PROBES){
		// Объект потокового чтения текста
		toml::reader_t reader(::logger());
		// Собираемая запись разбираемого текста
		const string text(probe.text);
		// Выполняем проверку отказа разбора текста
		EXPECT_FALSE(reader.feed(text.data(), text.size(), true)) << probe.note;
		// Выполняем проверку выданного кода отказа разбора
		EXPECT_EQ(reader.error(), probe.error) << probe.note;
	}
}
