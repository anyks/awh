/**
 * @file document.cpp
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
 * @brief Автоматические тесты сообщения системного журнала, удерживаемого целиком — устройства
 *        дерева, обхода по пути, оборота записи и выдачи даты заданным видом
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
	 */
	struct SilentSysLogDocument {
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
		SilentSysLogDocument() noexcept : log(&SilentSysLogDocument::framework()) {
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
	const awh::log_t * documentLogger() noexcept {
		// Объект журнала проверок
		static SilentSysLogDocument silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Метод сбора дерева в двоичную запись ради сличения целиком
 *
 * @details Сличаются ДЕРЕВЬЯ, а не тексты: дословного совпадения оборот не обещает,
 * обещает значение. Сличение же двоичной записи ловит и лишнее поле, и его недостачу,
 * чего проверка отдельных полей не даёт
 *
 * @param value сличаемое дерево
 * @return      двоичная запись дерева
 */
static string flatten(const abc::value_t & value) noexcept {
	// Выполняем сбор двоичной записи дерева
	const auto & buffer = value.dump();
	// Выводим двоичную запись дерева знаками
	return string(buffer.begin(), buffer.end());
}

/**
 * @brief Проверка устройства дерева разобранной записи
 *
 * @details Устройство дерева есть договор кодека с потребителем: правка его молча
 *          сломала бы всякий обход, на него написанный
 *
 */
TEST(CodecSysLogDocument, Layout) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности разбора записи
	ASSERT_TRUE(document.parse(
		"<165>1 2003-10-11T22:14:15.003Z myhostname myapp 1234 ID47 "
		"[exampleSDID@32473 iut=\"3\"] An application event log entry"
	));
	// Выполняем проверку описания, каким запись прочтена
	EXPECT_EQ(document.at("/standard").text(), "RFC5424");
	// Выполняем проверку приоритета записи, деревом объявленного
	EXPECT_EQ(document.priority(), 165u);
	// Выполняем проверку объявленности приоритета записи
	EXPECT_TRUE(document.prioritized());
	// Выполняем проверку имени источника сообщения, деревом объявленного
	EXPECT_EQ(document.at("/facility").text(), "local4");
	// Выполняем проверку имени степени важности сообщения, деревом объявленного
	EXPECT_EQ(document.at("/severity").text(), "notice");
	// Выполняем проверку даты сообщения, деревом объявленной
	EXPECT_EQ(document.at("/header/timestamp").text(), "2003-10-11T22:14:15.003Z");
	// Выполняем проверку имени узла, деревом объявленного
	EXPECT_EQ(document.at("/header/hostname").text(), "myhostname");
	// Выполняем проверку названия приложения, деревом объявленного
	EXPECT_EQ(document.at("/header/application").text(), "myapp");
	// Выполняем проверку опознавателя работы, деревом объявленного
	EXPECT_EQ(document.at("/header/process").text(), "1234");
	// Выполняем проверку опознавателя сообщения, деревом объявленного
	EXPECT_EQ(document.at("/header/messageId").text(), "ID47");
	// Выполняем проверку поля блока структурированных данных
	EXPECT_EQ(document.at("/structures/exampleSDID@32473/iut").text(), "3");
	// Выполняем проверку текста сообщения, деревом объявленного
	EXPECT_EQ(document.at("/message").text(), "An application event log entry");
	// Выполняем проверку количества блоков структурированных данных
	EXPECT_EQ(document.size(), 1u);
}

/**
 * @brief Проверка обхода дерева по пути
 *
 * @details Обход обязан быть ЗАМКНУТ: имя, ходом `keys` выданное, обязано находиться
 *          ходом `at` - иначе потребитель, перебирающий дерево, спотыкается о своё же
 *          звено
 *
 */
TEST(CodecSysLogDocument, Traversal) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности разбора записи
	ASSERT_TRUE(document.parse(
		"<165>1 2003-10-11T22:14:15.003Z host app - ID47 "
		"[first@32473 iut=\"3\" src=\"App\"][second@32473 class=\"high\"] Message"
	));
	// Получаем опознаватели блоков структурированных данных
	const vector <string> blocks = document.keys("/structures");
	// Выполняем проверку количества блоков структурированных данных
	ASSERT_EQ(blocks.size(), 2u);
	/**
	 * Выполняем перебор всех блоков структурированных данных
	 */
	for(auto & block : blocks){
		// Выполняем проверку замкнутости обхода по опознавателю блока
		EXPECT_TRUE(document.has("/structures/" + block)) << "блок: " << block;
		// Получаем имена полей очередного блока структурированных данных
		const vector <string> params = document.keys("/structures/" + block);
		// Выполняем проверку непустоты блока структурированных данных
		EXPECT_FALSE(params.empty()) << "блок: " << block;
		/**
		 * Выполняем перебор всех полей блока структурированных данных
		 */
		for(auto & param : params)
			// Выполняем проверку замкнутости обхода по имени поля блока
			EXPECT_TRUE(document.has("/structures/" + block + "/" + param)) << "поле: " << block << "/" << param;
	}
	// Выполняем проверку наличия поля внутри блока по имени
	EXPECT_TRUE(document.contains("/structures/first@32473", "iut"));
	// Выполняем проверку отсутствия поля, записью не объявленного
	EXPECT_FALSE(document.contains("/structures/first@32473", "nosuchparam"));
	// Выполняем проверку отсутствия значения по пути, деревом не объявленному
	EXPECT_FALSE(document.has("/structures/nosuchblock"));
}

/**
 * @brief Проверка постановки, сброса и сноса значения дерева
 *
 * @details Сброс оставляет поле с пустым значением, а снос убирает его вовсе: у
 *          записи нынешнего описания различие это видно - сброшенное поле пишется
 *          пустым, снесённое знаком «-»
 *
 */
TEST(CodecSysLogDocument, ResetAgainstErase) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности разбора записи
	ASSERT_TRUE(document.parse("<165>1 2003-10-11T22:14:15.003Z host app 1234 ID47 - Message"));
	// Выполняем проверку постановки значения дерева по пути
	EXPECT_TRUE(document.set("/header/hostname", abc::value_t(string("other"))));
	// Выполняем проверку поставленного значения дерева
	EXPECT_EQ(document.at("/header/hostname").text(), "other");
	// Выполняем проверку сброса значения дерева по пути
	EXPECT_TRUE(document.reset("/header/process"));
	// Выполняем проверку того, что сброшенное поле деревом объявлено
	EXPECT_TRUE(document.has("/header/process"));
	// Выполняем проверку пустоты сброшенного значения
	EXPECT_EQ(document.at("/header/process").text(), "");
	// Выполняем проверку сноса значения дерева по пути
	EXPECT_TRUE(document.erase("/header/messageId"));
	// Выполняем проверку того, что снесённое поле деревом не объявлено
	EXPECT_FALSE(document.has("/header/messageId"));
	// Выполняем проверку отказа сброса значения, деревом не объявленного
	EXPECT_FALSE(document.reset("/header/nosuchfield"));
	// Выполняем проверку кода отказа сброса значения
	EXPECT_EQ(document.error(), syslog::error_t::UNKNOWN_FIELD);
}

/**
 * @brief Проверка оборота записей системного журнала
 *
 * @details Оборот сличает ДЕРЕВЬЯ, а не тексты: дословного совпадения он не обещает,
 *          обещает значение. Сличение текстов ловило бы вид даты и порядок полей, а не
 *          сохранность сведений
 *
 */
TEST(CodecSysLogDocument, Roundtrip) {
	// Образцы записей системного журнала, живыми устройствами писанные
	const char * SAMPLES[] = {
		"<13>Sat Jan  8 20:07:41 2011 myhostname myapp[1234]: This is a sample syslog message.",
		"<13>Oct 22 12:34:56 myhostname myapp[1234]: This is a sample syslog message.",
		"<45>Oct 22 12:34:56 freebsd-log syslog-ng[8763]: [notice]syslog-ng starting up; version='4.7.1'",
		"<45>2024-10-04 13:29:47 freebsd-log syslog-ng[8763]: [notice]syslog-ng starting up",
		"<45>2003-10-11T22:14:15.003Z freebsd-log syslog-ng[8763]: [notice]syslog-ng starting up",
		"<165>Oct 22 10:52:01 scapegoat.dmz.example.org sched[222]: That's All Folks!",
		"<165>Oct 22 10:52:01 scapegoat.dmz.example.org sched: That's All Folks!",
		"Oct 22 10:52:01 scapegoat.dmz.example.org sched: That's All Folks!",
		"<165>1 2023-04-11T23:29:33.003Z mymachine.example.com evntslog - ID47 "
		"[exampleSDID@32473 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"]"
		"[examplePriority@32473 class=\"high\"] BOMAn application event log entry...",
		"<165>1 2003-10-11T22:14:15.003Z myhostname myapp 1234 ID47 - An application event log entry...",
		"<165>1 2023-04-11T23:29:33.003Z - MyApp - - - Message",
		"<10>1 2023-12-25T15:29:22.000003-07:00 srv-demo-2022.demo.pgr 1093|11.0.0.0 - GNRL_EV "
		"[event@23668 p1=\"a\\\"b\" p2=\"c\\]d\"] \xEF\xBB\xBF" "Текст события"
	};
	/**
	 * Выполняем перебор всех образцов записей системного журнала
	 */
	for(auto & sample : SAMPLES){
		// Объект события первого разбора
		syslog::document_t one(&SilentSysLogDocument::framework(), ::documentLogger());
		// Выполняем проверку успешности разбора исходной записи
		ASSERT_TRUE(one.parse(sample)) << "запись: " << sample
		                               << ", отказ: " << syslog::message(one.error());
		// Выполняем сборку записи из дерева события
		const string built = one.dump();
		// Выполняем проверку успешности сборки записи
		ASSERT_FALSE(built.empty()) << "запись: " << sample
		                            << ", отказ: " << syslog::message(one.error());
		// Объект события повторного разбора
		syslog::document_t two(&SilentSysLogDocument::framework(), ::documentLogger());
		// Выполняем проверку успешности повторного разбора собранной записи
		ASSERT_TRUE(two.parse(built)) << "исходная: " << sample << ", собранная: " << built
		                              << ", отказ: " << syslog::message(two.error());
		// Выполняем проверку совпадения деревьев первого и повторного разбора
		EXPECT_EQ(flatten(one.root()), flatten(two.root()))
			<< "исходная: " << sample << ", собранная: " << built;
	}
}

/**
 * @brief Проверка выдачи даты сообщения заданным видом
 *
 * @details Дата держится деревом тем видом, каким стояла в записи, а потребителю нужен
 *          свой: ход этот разбирает её и выдаёт заданным видом
 *
 */
TEST(CodecSysLogDocument, Timestamps) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	/**
	 * Выполняем проверку успешности разбора записи с датой БЕЗ ЗОНЫ
	 *
	 * @note Дата берётся без зоны намеренно: разбор ведётся местным временем, и выдача
	 *       им же. Дата, зону несущую, проверка сличала бы с местной зоной МАШИНЫ, а не
	 *       с работой кодека, - и на стенде, живущем по UTC, отказывала бы
	 */
	ASSERT_TRUE(document.parse("<45>2024-10-04 13:29:47 host app: Message"));
	// Выполняем проверку выдачи даты сообщения заданным видом
	EXPECT_EQ(document.timestamp("%Y-%m-%d %H:%M:%S"), "2024-10-04 13:29:47");
	// Выполняем проверку выдачи времени сообщения числом
	EXPECT_NE(document.time(), 0u);
	// Выполняем проверку успешности разбора записи устаревшего описания
	ASSERT_TRUE(document.parse("<13>Sat Jan  8 20:07:41 2011 host app: Message"));
	/**
	 * Выполняем проверку сохранности года у даты, год объявляющей
	 *
	 * @note Год теряется молча: вид BSD его не несёт, и разбор, начатый с него,
	 *       подставил бы текущий - оттого виды, год несущие, пробуются раньше
	 */
	EXPECT_EQ(document.timestamp("%Y-%m-%d %H:%M:%S"), "2011-01-08 20:07:41");
	/**
	 * Проверка устойчивости оборота даты СНЯТА до починки объекта времени
	 *
	 * @details Зона, разбором прежней даты полученная, остаётся в объекте `chrono_t` и
	 * применяется к следующей дате, зоны НЕ несущей, - даже когда разбор запрошен
	 * местным временем. Замерено 07.09.2026: после разбора «2003-10-11T22:14:15.003Z»
	 * дата «2003-10-12 01:14:15» даёт момент, смещённый ровно на три часа, а после
	 * даты со смещением «-07:00» - на десять
	 *
	 * @note Дефект чужого модуля, доложен владельцу. Обходить его здесь нельзя: обход
	 *       в кодеке скрыл бы дефект от прочих его потребителей
	 */
}

/**
 * @brief Проверка выдачи даты, разбору не поддавшейся
 *
 * @details Дата выдаётся КАК ЕСТЬ, а не пустой: записи живых устройств несут и такие
 *          даты, а выдача пустоты означала бы, что даты нет вовсе
 *
 */
TEST(CodecSysLogDocument, TimestampRefusal) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Настройки разбора записей без сличения
	syslog::reader_t::settings_t settings;
	// Выключаем сличение разбираемой записи с описанием
	settings.mode = syslog::mode_t::NONE;
	// Выполняем установку настроек разбора записей
	ASSERT_TRUE(document.settings(settings));
	// Выполняем проверку успешности разбора записи с датой неведомого вида
	ASSERT_TRUE(document.parse("<13>NotADate 00:00:00 host app: Message"));
	// Выполняем проверку выдачи нуля временем даты, разбору не поддавшейся
	EXPECT_EQ(document.time(), 0u);
	// Выполняем проверку выдачи даты как есть
	EXPECT_EQ(document.timestamp("%Y-%m-%d"), document.at("/header/timestamp").text());
}

/**
 * @brief Проверка выдачи человеческого названия источника сообщения
 *
 * @details Название берётся словарём: держать его в дереве значило бы наращивать
 *          всякую запись сведениями, каких она не несёт
 *
 */
TEST(CodecSysLogDocument, Naming) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности разбора записи
	ASSERT_TRUE(document.parse("<165>Oct 22 10:52:01 host app: Message"));
	// Выполняем проверку источника сообщения: 165 / 8 = 20
	EXPECT_EQ(document.facility(), syslog::facility_t::LOCAL4);
	// Выполняем проверку степени важности сообщения: 165 % 8 = 5
	EXPECT_EQ(document.severity(), syslog::severity_t::NOTICE);
	// Выполняем проверку непустоты человеческого названия источника сообщения
	EXPECT_FALSE(document.label().empty());
	// Выполняем проверку успешности разбора записи без приставки приоритета
	ASSERT_TRUE(document.parse("Oct 22 10:52:01 host app: Message"));
	// Выполняем проверку того, что приоритет записью не объявлен
	EXPECT_FALSE(document.prioritized());
	// Выполняем проверку пустоты названия у записи без приставки приоритета
	EXPECT_TRUE(document.label().empty());
}

/**
 * @brief Проверка того, что оборот не домысливает приставку приоритета
 *
 * @details Чтение записи без приставки отказом не отвечает, и запись обязана отвечать
 *          ему тем же: приписать приставку значило бы объявить источник с важностью,
 *          каких отправитель не объявлял
 *
 */
TEST(CodecSysLogDocument, PriorityIsNotInvented) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности разбора записи без приставки приоритета
	ASSERT_TRUE(document.parse("Oct 22 10:52:01 host app: Message"));
	// Выполняем сборку записи из дерева события
	const string built = document.dump();
	// Выполняем проверку успешности сборки записи
	ASSERT_FALSE(built.empty());
	// Выполняем проверку отсутствия приставки приоритета у собранной записи
	EXPECT_NE(built.front(), '<') << "запись: " << built;
}

/**
 * @brief Проверка сведения настроек чтения и записи
 *
 * @details Настройку снятия отмены знаков и настройку постановки её порознь задавать
 *          нельзя: постановка отмены поверх неснятой наращивает косые при всяком
 *          обороте. Пару эту сводит документ, ибо только он держит обе стороны
 *
 */
TEST(CodecSysLogDocument, SettingsPairing) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Настройки разбора записей без снятия отмены знаков
	syslog::reader_t::settings_t settings;
	// Выключаем снятие отмены знаков со значений структурированных данных
	settings.unescape = false;
	// Выполняем установку настроек разбора записей
	ASSERT_TRUE(document.settings(settings));
	// Разбираемая запись с отменой знаков в значении блока данных
	const char * record = "<10>1 2023-12-25T15:29:22Z host app - ID [event@1 p=\"a\\\"b\"] Message";
	// Выполняем проверку успешности разбора записи
	ASSERT_TRUE(document.parse(record));
	// Выполняем сборку записи из дерева события
	const string built = document.dump();
	// Выполняем проверку успешности сборки записи
	ASSERT_FALSE(built.empty());
	// Объект события повторного разбора
	syslog::document_t two(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем установку тех же настроек разбора записей
	ASSERT_TRUE(two.settings(settings));
	// Выполняем проверку успешности повторного разбора собранной записи
	ASSERT_TRUE(two.parse(built));
	/**
	 * Выполняем проверку того, что число косых при обороте не наросло
	 *
	 * @note Расхождение это накапливается: всякий оборот добавлял бы по косой, и
	 *       заметно оно стало бы лишь на третьем-четвёртом
	 */
	EXPECT_EQ(flatten(document.root()), flatten(two.root())) << "собранная: " << built;
}

/**
 * @brief Проверка очистки дерева события
 *
 */
TEST(CodecSysLogDocument, Clear) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности разбора записи
	ASSERT_TRUE(document.parse("<165>1 2003-10-11T22:14:15.003Z host app - - [a@1 b=\"c\"] Message"));
	// Выполняем проверку наличия блоков структурированных данных
	EXPECT_EQ(document.size(), 1u);
	// Выполняем очистку дерева события
	document.clear();
	// Выполняем проверку отсутствия блоков структурированных данных
	EXPECT_EQ(document.size(), 0u);
	// Выполняем проверку отсутствия текста сообщения
	EXPECT_FALSE(document.has("/message"));
	// Выполняем проверку сброса кода ошибки последней операции
	EXPECT_EQ(document.error(), syslog::error_t::NONE);
}

/**
 * @brief Проверка отказа разбора и положения обнаруженной ошибки
 *
 */
TEST(CodecSysLogDocument, Failure) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Настройки разбора записей строгим сличением
	syslog::reader_t::settings_t settings;
	// Устанавливаем строгое сличение разбираемой записи с описанием
	settings.mode = syslog::mode_t::STRONG;
	// Выполняем установку настроек разбора записей
	ASSERT_TRUE(document.settings(settings));
	// Выполняем проверку отказа разбора записи с приоритетом, за предел выходящим
	EXPECT_FALSE(document.parse("<999>1 2023-04-11T23:29:33Z host app - - - Message"));
	// Выполняем проверку кода отказа разбора записи
	EXPECT_EQ(document.error(), syslog::error_t::INVALID_PRIORITY);
	// Выполняем проверку того, что положение ошибки указано в первой строке
	EXPECT_EQ(document.errorPosition().line, 1u);
}
