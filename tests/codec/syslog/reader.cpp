/**
 * @file reader.cpp
 * @date 2026-09-07
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
 * @brief Автоматические тесты потокового чтения сообщений системного журнала — отделения границ
 *        даты, самоопределения описания, разбора структурированных данных и независимости
 *        разбора от нарезки текста на куски
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/syslog/syslog.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Внутренние служебные объекты
 *
 * @details Типы держатся в БЕЗЫМЯННОМ пространстве намеренно: проверки всех кодеков
 *          собираются одной программой, и тип с внешним связыванием сталкивался бы с
 *          одноимённым типом соседнего кодека - порчей кучи вдали от места
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
	struct SilentSysLog {
		/**
		 * @brief Функция получения объекта фреймворка проверок
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
		SilentSysLog() noexcept : log(&SilentSysLog::framework()) {
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
		static SilentSysLog silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}

	/**
	 * @brief Событие разбора, щупом собираемое
	 *
	 */
	typedef struct Event {
		// Вид события разбора
		syslog::event_t event;
		// Поле заголовка, событием выданное
		syslog::field_t field;
		// Имя ключа события
		string key;
		// Значение события
		string value;
	} event_t;

	/**
	 * @brief Метод сбора событий разбора записи
	 *
	 * @param text     разбираемый текст записи
	 * @param chunk    размер куска подачи, нулём означающий подачу целиком
	 * @param settings настройки разбора записей
	 * @param events   собранные события разбора
	 * @return         состояние чтения по окончании разбора
	 */
	syslog::state_t collect(const string & text, const size_t chunk,
	                        const syslog::reader_t::settings_t & settings, vector <event_t> & events) noexcept {
		// Выполняем очистку собранных событий разбора
		events.clear();
		// Выполняем создание объекта чтения записей
		syslog::reader_t reader(&SilentSysLog::framework(), ::logger());
		// Выполняем установку настроек разбора записей
		reader.settings(settings);
		// Если текст подаётся кусками заданного размера
		if(chunk > 0){
			// Смещение начала очередного куска подачи
			size_t offset = 0;
			/**
			 * Выполняем подачу текста кусками заданного размера
			 */
			while(offset < text.size()){
				// Получаем размер очередного куска подачи
				const size_t size = ((offset + chunk) < text.size() ? chunk : (text.size() - offset));
				// Выполняем подачу очередного куска текста
				reader.feed(text.data() + offset, size, (offset + size) >= text.size());
				// Сдвигаем смещение начала очередного куска подачи
				offset += size;
				/**
				 * Выполняем перебор событий разбора поданного куска
				 */
				while(reader.next())
					// Добавляем очередное событие разбора в собранные
					events.push_back({reader.event(), reader.field(), reader.key(), reader.value()});
			}
		// Если текст подаётся целиком
		} else {
			// Выполняем подачу текста целиком
			reader.feed(text);
			/**
			 * Выполняем перебор событий разбора поданного текста
			 */
			while(reader.next())
				// Добавляем очередное событие разбора в собранные
				events.push_back({reader.event(), reader.field(), reader.key(), reader.value()});
		}
		// Выводим состояние чтения по окончании разбора
		return reader.state();
	}

	/**
	 * @brief Метод извлечения значения поля заголовка из собранных событий
	 *
	 * @param events собранные события разбора
	 * @param field  разыскиваемое поле заголовка
	 * @return       значение поля заголовка либо пустота
	 */
	string field(const vector <event_t> & events, const syslog::field_t field) noexcept {
		/**
		 * Выполняем перебор всех собранных событий разбора
		 */
		for(auto & item : events){
			// Если событие полем заголовка является и поле совпало
			if((item.event == syslog::event_t::HEADER) && (item.field == field))
				// Выводим значение поля заголовка
				return item.value;
		}
		// Выводим пустое значение поля заголовка
		return string("");
	}
}

/**
 * @brief Проверка отделения границ даты у записей устаревшего описания
 *
 * @details Проверка сличает ОТДЕЛЁННЫЙ текст даты и имя узла с ожидаемым, а не
 *          отсутствие отказа: неверная граница проходит поверку молча - дата
 *          разбирается, а имя узла оказывается чужим
 *
 * @note Подсказано владельцем 07.09.2026: видов даты много, и зависят они от системы.
 *       Разбор даты ведёт `chrono_t`, границы её ищет кодек, и ошибается именно он
 *
 */
TEST(CodecSysLogReader, DateBoundaries) {
	/**
	 * @brief Случай отделения границ даты
	 *
	 */
	struct Case {
		// Разбираемая запись
		const char * record;
		// Ожидаемый отделённый текст даты
		const char * date;
		// Ожидаемое имя узла
		const char * host;
	};
	// Случаи отделения границ даты, видами её перебранные
	const Case CASES[] = {
		{"<13>Oct  9 22:14:15 host app: msg", "Oct  9 22:14:15", "host"},
		{"<13>Oct 09 22:14:15 host app: msg", "Oct 09 22:14:15", "host"},
		{"<13>Oct 9 22:14:15 host app: msg", "Oct 9 22:14:15", "host"},
		{"<13>Oct 22 12:34:56 2011 host app: msg", "Oct 22 12:34:56 2011", "host"},
		{"<13>Sat Jan  8 20:07:41 2011 host app: msg", "Sat Jan  8 20:07:41 2011", "host"},
		{"<13>Sat Jan  08 20:07:41 2011 host app: msg", "Sat Jan  08 20:07:41 2011", "host"},
		{"<13>Sat Jan 8 20:07:41 2011 host app: msg", "Sat Jan 8 20:07:41 2011", "host"},
		{"<45>2024-10-04 13:29:47 host app: msg", "2024-10-04 13:29:47", "host"},
		{"<45>2003-10-11T22:14:15.003Z host app: msg", "2003-10-11T22:14:15.003Z", "host"},
		{"<45>2003-10-11T22:14:15Z host app: msg", "2003-10-11T22:14:15Z", "host"},
		{"<45>2003-10-11T22:14:15+03:00 host app: msg", "2003-10-11T22:14:15+03:00", "host"},
		{"<10>2023-12-25T15:29:22.000003-07:00 host app: msg", "2023-12-25T15:29:22.000003-07:00", "host"},
		/**
		 * Имя узла ЧЕТЫРЬМЯ ЦИФРАМИ: годом его считать нельзя
		 *
		 * @note Двусмысленность эта разрешается грамматикой описания, а не
		 *       правдоподобием года: RFC 3164 ставит между датой и меткой приложения
		 *       имя узла, оттого слово из четырёх цифр есть год лишь тогда, когда за
		 *       ним следует НЕ метка. Правдоподобие же («больше 1970») было бы
		 *       гаданием: журналы несут и даты сбитых часов
		 */
		{"<13>Oct 22 12:34:56 1234 app: msg", "Oct 22 12:34:56", "1234"},
		{"<45>Oct 22 12:34:56 127.0.0.1 app: msg", "Oct 22 12:34:56", "127.0.0.1"}
	};
	// Настройки разбора записей строгим сличением
	syslog::reader_t::settings_t settings;
	// Устанавливаем строгое сличение разбираемой записи с описанием
	settings.mode = syslog::mode_t::STRONG;
	// Собранные события разбора
	vector <event_t> events;
	/**
	 * Выполняем перебор всех случаев отделения границ даты
	 */
	for(auto & item : CASES){
		// Выполняем сбор событий разбора очередной записи
		const syslog::state_t state = collect(item.record, 0, settings, events);
		// Выполняем проверку успешности разбора записи
		EXPECT_NE(state, syslog::state_t::FAILED) << "запись: " << item.record;
		// Выполняем проверку отделённого текста даты сообщения
		EXPECT_EQ(field(events, syslog::field_t::TIMESTAMP), string(item.date)) << "запись: " << item.record;
		// Выполняем проверку отделённого имени узла
		EXPECT_EQ(field(events, syslog::field_t::HOSTNAME), string(item.host)) << "запись: " << item.record;
	}
}

/**
 * @brief Проверка самоопределения описания записи
 *
 * @details Описание, самоопределением полученное, ЗАПОМИНАЕТСЯ: спрос отвечает
 *          опознанным описанием, а не настройкой `AUTO`, - то есть так же, как если бы
 *          описание задали вручную
 *
 */
TEST(CodecSysLogReader, Detection) {
	// Выполняем создание объекта чтения записей
	syslog::reader_t reader(&SilentSysLog::framework(), ::logger());
	// Выполняем подачу записи нынешнего описания
	reader.feed("<165>1 2003-10-11T22:14:15.003Z host app 1234 ID47 - Message");
	/**
	 * Выполняем перебор событий разбора записи
	 */
	while(reader.next()){}
	// Выполняем проверку опознания нынешнего описания
	EXPECT_EQ(reader.standard(), syslog::standard_t::RFC5424);
	// Выполняем проверку номера описания, записью объявленного
	EXPECT_EQ(reader.version(), 1u);
	// Выполняем сброс состояния чтения записей
	reader.reset();
	// Выполняем подачу записи устаревшего описания
	reader.feed("<165>Oct 22 10:52:01 host app: Message");
	/**
	 * Выполняем перебор событий разбора записи
	 */
	while(reader.next()){}
	// Выполняем проверку опознания устаревшего описания
	EXPECT_EQ(reader.standard(), syslog::standard_t::RFC3164);
	// Выполняем проверку того, что номер описания устаревшей записью не объявлен
	EXPECT_EQ(reader.version(), 0u);
}

/**
 * @brief Проверка разбора приставки приоритета
 *
 * @details Источник есть частное от деления приоритета на восемь, важность - остаток:
 *          оттого они вычисляются, а не хранятся
 *
 */
TEST(CodecSysLogReader, Priority) {
	// Выполняем создание объекта чтения записей
	syslog::reader_t reader(&SilentSysLog::framework(), ::logger());
	// Выполняем подачу записи с приставкой приоритета
	reader.feed("<165>Oct 22 10:52:01 host app: Message");
	/**
	 * Выполняем перебор событий разбора записи
	 */
	while(reader.next()){}
	// Выполняем проверку объявленности приоритета записи
	EXPECT_TRUE(reader.prioritized());
	// Выполняем проверку значения приоритета записи
	EXPECT_EQ(reader.priority(), 165u);
	// Выполняем проверку источника сообщения: 165 / 8 = 20
	EXPECT_EQ(reader.facility(), syslog::facility_t::LOCAL4);
	// Выполняем проверку степени важности сообщения: 165 % 8 = 5
	EXPECT_EQ(reader.severity(), syslog::severity_t::NOTICE);
}

/**
 * @brief Проверка разбора записи без приставки приоритета
 *
 * @details Описание приставку требует, живые же устройства шлют записи и без неё, а
 *          сборщики журналов такие записи принимают. Отвергать их значило бы терять
 *          события, оттого отсутствие приставки отказом НЕ является
 *
 */
TEST(CodecSysLogReader, MissingPriority) {
	// Выполняем создание объекта чтения записей
	syslog::reader_t reader(&SilentSysLog::framework(), ::logger());
	// Выполняем подачу записи без приставки приоритета
	reader.feed("Oct 22 10:52:01 host app: Message");
	/**
	 * Выполняем перебор событий разбора записи
	 */
	while(reader.next()){}
	// Выполняем проверку того, что разбор отказом не завершился
	EXPECT_NE(reader.state(), syslog::state_t::FAILED);
	// Выполняем проверку того, что приоритет записью не объявлен
	EXPECT_FALSE(reader.prioritized());
	// Выполняем проверку выдачи нуля необъявленным приоритетом
	EXPECT_EQ(reader.priority(), 0u);
}

/**
 * @brief Проверка разбора блоков структурированных данных
 *
 * @details Блоки идут подряд, без разделителя между ними, и всякий взят в квадратные
 *          скобки; отменяются внутри значения ровно три знака, описанием названные
 *
 */
TEST(CodecSysLogReader, Structured) {
	// Настройки разбора записей
	syslog::reader_t::settings_t settings;
	// Собранные события разбора
	vector <event_t> events;
	// Выполняем сбор событий разбора записи с двумя блоками данных
	const syslog::state_t state = collect(
		"<165>1 2023-04-11T23:29:33.003Z host app - ID47 "
		"[first@32473 iut=\"3\" src=\"Application\"][second@32473 class=\"high\"] Message",
		0, settings, events
	);
	// Выполняем проверку успешности разбора записи
	EXPECT_NE(state, syslog::state_t::FAILED);
	// Опознаватели блоков структурированных данных, разбором выданные
	vector <string> blocks;
	// Поля блоков структурированных данных, разбором выданные
	vector <string> params;
	/**
	 * Выполняем перебор всех собранных событий разбора
	 */
	for(auto & item : events){
		// Если событие опознавателем блока является
		if(item.event == syslog::event_t::STRUCTURE)
			// Добавляем опознаватель блока в собранные
			blocks.push_back(item.key);
		// Если событие полем блока является
		else if(item.event == syslog::event_t::PARAM)
			// Добавляем поле блока в собранные
			params.push_back(item.key + "=" + item.value);
	}
	// Выполняем проверку количества блоков структурированных данных
	ASSERT_EQ(blocks.size(), 2u);
	// Выполняем проверку опознавателя первого блока
	EXPECT_EQ(blocks.at(0), "first@32473");
	// Выполняем проверку опознавателя второго блока
	EXPECT_EQ(blocks.at(1), "second@32473");
	// Выполняем проверку количества полей блоков структурированных данных
	ASSERT_EQ(params.size(), 3u);
	// Выполняем проверку первого поля первого блока
	EXPECT_EQ(params.at(0), "iut=3");
	// Выполняем проверку второго поля первого блока
	EXPECT_EQ(params.at(1), "src=Application");
	// Выполняем проверку поля второго блока
	EXPECT_EQ(params.at(2), "class=high");
}

/**
 * @brief Проверка снятия отмены знаков со значений структурированных данных
 *
 * @details Отменяются РОВНО ТРИ знака, описанием названные: кавычка, закрывающая
 *          скобка и сама обратная косая. Косая перед иным знаком остаётся как есть, и
 *          снимать её значило бы портить содержимое
 *
 */
TEST(CodecSysLogReader, Unescaping) {
	// Настройки разбора записей
	syslog::reader_t::settings_t settings;
	// Собранные события разбора
	vector <event_t> events;
	// Выполняем сбор событий разбора записи с отменой знаков
	collect(
		"<10>1 2023-12-25T15:29:22Z host app - ID "
		"[event@23668 p1=\"a\\\"b\" p2=\"c\\]d\" p3=\"e\\\\f\" p4=\"g\\nh\"] Message",
		0, settings, events
	);
	// Значения полей блока структурированных данных
	vector <string> values;
	/**
	 * Выполняем перебор всех собранных событий разбора
	 */
	for(auto & item : events){
		// Если событие полем блока является
		if(item.event == syslog::event_t::PARAM)
			// Добавляем значение поля блока в собранные
			values.push_back(item.value);
	}
	// Выполняем проверку количества полей блока структурированных данных
	ASSERT_EQ(values.size(), 4u);
	// Выполняем проверку снятия отмены с кавычки
	EXPECT_EQ(values.at(0), "a\"b");
	// Выполняем проверку снятия отмены с закрывающей скобки
	EXPECT_EQ(values.at(1), "c]d");
	// Выполняем проверку снятия отмены с обратной косой
	EXPECT_EQ(values.at(2), "e\\f");
	/**
	 * Выполняем проверку того, что косая перед иным знаком остаётся
	 *
	 * @note Описание отменяемых знаков перечисляет ровно три и снятие косой перед
	 *       прочими ЗАПРЕЩАЕТ прямо: «\n» есть два знака, а не перевод строки
	 */
	EXPECT_EQ(values.at(3), "g\\nh");
}

/**
 * @brief Проверка снятия метки порядка байтов с текста сообщения
 *
 * @details Метка есть признак записи текста в UTF-8, а не часть самого текста: оставь
 *          её на месте - и текст, потребителю выданный, начинался бы тремя лишними
 *          октетами
 *
 */
TEST(CodecSysLogReader, ByteOrderMark) {
	// Настройки разбора записей
	syslog::reader_t::settings_t settings;
	// Собранные события разбора
	vector <event_t> events;
	// Выполняем сбор событий разбора записи с меткой порядка байтов
	collect("<10>1 2023-12-25T15:29:22Z host app - ID - \xEF\xBB\xBF" "Текст", 0, settings, events);
	// Текст сообщения, разбором выданный
	string message = "";
	/**
	 * Выполняем перебор всех собранных событий разбора
	 */
	for(auto & item : events){
		// Если событие текстом сообщения является
		if(item.event == syslog::event_t::MESSAGE)
			// Запоминаем текст сообщения
			message = item.value;
	}
	// Выполняем проверку снятия метки порядка байтов с текста сообщения
	EXPECT_EQ(message, "Текст");
	// Выключаем снятие метки порядка байтов
	settings.bom = false;
	// Выполняем сбор событий разбора той же записи
	collect("<10>1 2023-12-25T15:29:22Z host app - ID - \xEF\xBB\xBF" "Текст", 0, settings, events);
	/**
	 * Выполняем перебор всех собранных событий разбора
	 */
	for(auto & item : events){
		// Если событие текстом сообщения является
		if(item.event == syslog::event_t::MESSAGE)
			// Запоминаем текст сообщения
			message = item.value;
	}
	// Выполняем проверку сохранения метки порядка байтов при выключенном снятии
	EXPECT_EQ(message, "\xEF\xBB\xBF" "Текст");
}

/**
 * @brief Проверка независимости разбора от нарезки текста на куски
 *
 * @details Разбор обязан давать ОДИН И ТОТ ЖЕ ряд событий при любом размере куска
 *          подачи: расхождение означало бы, что состояние разбора между кусками
 *          теряется - и терялось бы оно молча, ибо отказа при том нет
 *
 */
TEST(CodecSysLogReader, Chunked) {
	// Разбираемый поток записей системного журнала
	const string text =
		"<165>Oct 22 10:52:01 host sched[222]: That's All Folks!\n"
		"<165>1 2023-04-11T23:29:33.003Z machine.example.com evntslog - ID47 "
		"[exampleSDID@32473 iut=\"3\" eventSource=\"Application\"] An application event log entry\n"
		"<13>Sat Jan  8 20:07:41 2011 myhostname myapp[1234]: This is a sample syslog message.\n";
	// Настройки разбора записей
	syslog::reader_t::settings_t settings;
	// События разбора, поданные текстом целиком
	vector <event_t> expect;
	// Выполняем сбор событий разбора текста целиком
	const syslog::state_t state = collect(text, 0, settings, expect);
	// Выполняем проверку успешности разбора текста
	EXPECT_NE(state, syslog::state_t::FAILED);
	// Выполняем проверку непустоты собранных событий разбора
	ASSERT_FALSE(expect.empty());
	// События разбора, поданные текстом кусками
	vector <event_t> actual;
	/**
	 * Выполняем перебор размеров куска подачи текста
	 */
	for(size_t chunk = 1; chunk <= 32; chunk++){
		// Выполняем сбор событий разбора текста кусками заданного размера
		const syslog::state_t current = collect(text, chunk, settings, actual);
		// Выполняем проверку успешности разбора текста кусками
		EXPECT_NE(current, syslog::state_t::FAILED) << "размер куска: " << chunk;
		// Выполняем проверку количества событий разбора
		ASSERT_EQ(actual.size(), expect.size()) << "размер куска: " << chunk;
		/**
		 * Выполняем перебор всех собранных событий разбора
		 */
		for(size_t i = 0; i < expect.size(); i++){
			// Выполняем проверку вида очередного события разбора
			EXPECT_EQ(actual.at(i).event, expect.at(i).event) << "размер куска: " << chunk << ", событие: " << i;
			// Выполняем проверку поля заголовка очередного события разбора
			EXPECT_EQ(actual.at(i).field, expect.at(i).field) << "размер куска: " << chunk << ", событие: " << i;
			// Выполняем проверку имени ключа очередного события разбора
			EXPECT_EQ(actual.at(i).key, expect.at(i).key) << "размер куска: " << chunk << ", событие: " << i;
			// Выполняем проверку значения очередного события разбора
			EXPECT_EQ(actual.at(i).value, expect.at(i).value) << "размер куска: " << chunk << ", событие: " << i;
		}
	}
}

/**
 * @brief Проверка того, что объявленные отказы разбора и вправду поднимаются
 *
 * @details Код отказа, никогда не поднимаемый, есть обещание, ничем не подкреплённое:
 *          проверка эта требует от всякого случая своего кода, а не общего отказа
 *
 */
TEST(CodecSysLogReader, Failures) {
	/**
	 * @brief Случай отказа разбора записи
	 *
	 */
	struct Case {
		// Разбираемая запись
		const char * record;
		// Ожидаемый код отказа разбора
		syslog::error_t error;
	};
	// Случаи отказов разбора записей
	const Case CASES[] = {
		{"<999>1 2023-04-11T23:29:33Z host app - - - Message", syslog::error_t::INVALID_PRIORITY},
		{"<13>2 2023-04-11T23:29:33Z host app - - - Message", syslog::error_t::UNSUPPORTED_VERSION},
		{"<13>1 2023-04-11T23:29:33Z host app - -", syslog::error_t::INCOMPLETE_HEADER},
		{"<13>1 2023-04-11T23:29:33Z host app - - [broken@1 a=\"b\" Message", syslog::error_t::UNCLOSED_STRUCTURE},
		{"<13>1 2023-04-11T23:29:33Z host app - - [broken@1 a=b] Message", syslog::error_t::UNQUOTED_PARAM_VALUE},
		{"<13>1 2023-04-11T23:29:33Z host app - - [broken@1 a=\"b] Message", syslog::error_t::UNCLOSED_PARAM_VALUE},
		{"<13>1 2023-04-11T23:29:33Z host app - - [same@1 a=\"b\"][same@1 c=\"d\"] Message", syslog::error_t::DUPLICATE_STRUCTURE}
	};
	// Настройки разбора записей строгим сличением
	syslog::reader_t::settings_t settings;
	// Устанавливаем строгое сличение разбираемой записи с описанием
	settings.mode = syslog::mode_t::STRONG;
	/**
	 * Выполняем перебор всех случаев отказов разбора записей
	 */
	for(auto & item : CASES){
		// Выполняем создание объекта чтения записей
		syslog::reader_t reader(&SilentSysLog::framework(), ::logger());
		// Выполняем установку настроек разбора записей
		reader.settings(settings);
		// Выполняем подачу разбираемой записи
		reader.feed(item.record);
		/**
		 * Выполняем перебор событий разбора записи
		 */
		while(reader.next()){}
		// Выполняем проверку прекращения разбора отказом
		EXPECT_EQ(reader.state(), syslog::state_t::FAILED) << "запись: " << item.record;
		// Выполняем проверку кода отказа разбора записи
		EXPECT_EQ(reader.error(), item.error) << "запись: " << item.record
		                                      << ", отказ: " << syslog::message(reader.error());
	}
}

/**
 * @brief Проверка обращения с отсутствующим значением поля
 *
 * @details Знак «-» означает отсутствие значения, а пустая последовательность знаков -
 *          значение пустое: различие это описанием положено, и исход выбирает
 *          настройка, а не кодек
 *
 */
TEST(CodecSysLogReader, NilValues) {
	// Настройки разбора записей
	syslog::reader_t::settings_t settings;
	// Собранные события разбора
	vector <event_t> events;
	// Выполняем сбор событий разбора записи с отсутствующими полями
	collect("<165>1 2023-04-11T23:29:33.003Z - MyApp - - - Message", 0, settings, events);
	// Выполняем проверку пропуска отсутствующего имени узла
	EXPECT_EQ(field(events, syslog::field_t::HOSTNAME), "");
	// Устанавливаем укладку отсутствующего значения знаками «-»
	settings.nil = syslog::nil_t::LITERAL;
	// Выполняем сбор событий разбора той же записи
	collect("<165>1 2023-04-11T23:29:33.003Z - MyApp - - - Message", 0, settings, events);
	// Выполняем проверку укладки отсутствующего имени узла знаками «-»
	EXPECT_EQ(field(events, syslog::field_t::HOSTNAME), "-");
	// Выполняем проверку того, что объявленное поле настройкой не тронуто
	EXPECT_EQ(field(events, syslog::field_t::APPLICATION), "MyApp");
}

/**
 * @brief Проверка выдачи окончания текста отдельным событием
 *
 * @details Событие это видно ПОСЛЕ цикла разбора, а не внутри него: потребитель,
 *          ведущий разбор по событиям, иначе не имел бы способа узнать, что поток
 *          кончился, а не прервался на середине
 *
 */
TEST(CodecSysLogReader, Finish) {
	// Выполняем создание объекта чтения записей
	syslog::reader_t reader(&SilentSysLog::framework(), ::logger());
	// Выполняем подачу записи целиком
	reader.feed("<165>Oct 22 10:52:01 host app: Message");
	/**
	 * Выполняем перебор событий разбора записи
	 */
	while(reader.next()){}
	// Выполняем проверку состояния чтения по окончании разбора
	EXPECT_EQ(reader.state(), syslog::state_t::FINISHED);
	// Выполняем проверку вида события, разбор завершившего
	EXPECT_EQ(reader.event(), syslog::event_t::FINISH);
}
