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
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

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

/**
 * @brief Проверка отказа обращения по номеру, размер счёта переполняющему
 *
 * @details Номер звена пути разбирается посимвольно, и беззнаковый счёт при
 *          переполнении заворачивается по кругу МОЛЧА: звено «18446744073709551616»
 *          (2^64) обращалось бы нулём, а «...617» - единицею. Обращение к
 *          несуществующему номеру отвечало бы оттого СУЩЕСТВУЮЩИМ значением перечня, и
 *          отвечало бы годным
 *
 * @note Разряд находки тот же, что у соседних кодеков, а исход иной: там `std::stoull`
 *       бросал исключение из хода, объявленного `noexcept`, и приложение гибло; здесь
 *       гибели нет, зато потребитель получает чужое значение вместо отказа. Молчаливо
 *       неверный ответ хуже громкого отказа: заметить его нечем
 *
 * @warning Половина проверки стережёт ОБРАТНОЕ - что номер, в счёт помещающийся,
 *          разыскивается по-прежнему. Без неё «отказ на переполнении» достигался бы
 *          отказом на всяком номере вовсе
 *
 */
TEST(CodecSysLogDocument, OversizedPathIndexIsRefused) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Перечень из двух значений, в дерево ставимый
	abc::value_t list(abc::kind_t::ARRAY);
	// Ставим первое значение в перечень
	ASSERT_TRUE(list.push(abc::value_t(string("первое"))));
	// Ставим второе значение в перечень
	ASSERT_TRUE(list.push(abc::value_t(string("второе"))));
	// Выполняем постановку перечня в дерево события
	ASSERT_TRUE(document.set("/list", list));
	// Значение, по пути извлекаемое
	string text = "";
	// Выполняем проверку того, что номер, в счёт помещающийся, разыскивается
	ASSERT_TRUE(document.at("/list/0").valid());
	// Выполняем извлечение значения по помещающемуся номеру
	ASSERT_TRUE(document.at("/list/0").value(text));
	// Выполняем проверку того, что по номеру выдано то самое значение
	EXPECT_EQ(text, "первое");
	// Выполняем проверку того, что номер, в счёт помещающийся, за пределами перечня негоден
	EXPECT_FALSE(document.at("/list/5").valid());
	/**
	 * Выполняем перебор номеров, размер счёта переполняющих
	 *
	 * @note Первый заворачивается нулём, второй - единицею, третий - произвольно:
	 *       все три указывают на значения, в перечне СУЩЕСТВУЮЩИЕ
	 */
	for(const char * path : {"/list/18446744073709551616", "/list/18446744073709551617", "/list/99999999999999999999999999"})
		// Выполняем проверку негодности значения по переполняющему номеру
		EXPECT_FALSE(document.at(path).valid()) << path;
}

/**
 * @brief Проверка обращения к блоку, чьё имя несёт знаки разделителя пути
 *
 * @details Опознаватель блока структурированных данных RFC 5424 косой черты и тильды не
 *          запрещает, а путь к значению дерева строится ими же: косая черта разделяет
 *          звенья, тильда открывает отмену. Оттого имена звеньев отменяются по уложению
 *          JSON Pointer - «~1» есть косая черта, «~0» есть тильда, - и отмена эта обязана
 *          работать в ОБЕ стороны: ход `keys` выдаёт имена отменёнными, а ход `at` их
 *          принимает
 *
 * @warning Свойство это было достижимо и работало, но не закреплено НИЧЕМ: карта
 *          покрытия показывала ветвь отмены непокрытой. Разряд опасен тем, что непокрытая
 *          ветвь неотличима от недостижимой - у соседнего кодека ровно такая оказалась
 *          мёртвой вовсе, будучи объявленной наружу. Проверка заведена 08.09.2026 по
 *          доводу Василия: спрашивать надо не «отвергает ли неведомое имя», а «находит ли
 *          хоть что-нибудь»
 *
 * @note Обратная половина - обращение по имени БЕЗ отмены - обязана отказывать: приняв и
 *       отменённое, и сырое имя разом, кодек объявил бы два разных пути одним значением
 *
 */
TEST(CodecSysLogDocument, EscapedStructureIdentifiers) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем разбор записи с опознавателями, несущими знаки разделителя пути
	ASSERT_TRUE(document.parse("<34>1 2003-10-11T22:14:15.003Z host - - - [a/b@1 k=\"v\"][c~d@2 m=\"w\"] текст"));
	// Выполняем проверку числа разобранных блоков структурированных данных
	ASSERT_EQ(document.size(), 2u);
	// Значение, по пути извлекаемое
	string text = "";
	// Выполняем проверку годности значения блока, имя какого несёт косую черту
	ASSERT_TRUE(document.at("/structures/a~1b@1/k").valid());
	// Выполняем извлечение значения блока с косой чертой в имени
	ASSERT_TRUE(document.at("/structures/a~1b@1/k").value(text));
	// Выполняем проверку того, что выдано то самое значение
	EXPECT_EQ(text, "v");
	// Выполняем проверку годности значения блока, имя какого несёт тильду
	ASSERT_TRUE(document.at("/structures/c~0d@2/m").valid());
	// Выполняем извлечение значения блока с тильдой в имени
	ASSERT_TRUE(document.at("/structures/c~0d@2/m").value(text));
	// Выполняем проверку того, что выдано то самое значение
	EXPECT_EQ(text, "w");
	// Перечень опознавателей блоков структурированных данных
	const vector <string> keys = document.keys("/structures");
	// Выполняем проверку числа выданных опознавателей
	ASSERT_EQ(keys.size(), 2u);
	// Выполняем проверку того, что имена выданы ОТМЕНЁННЫМИ и годятся обращению
	EXPECT_EQ(keys.front(), "a~1b@1");
	// Выполняем проверку отмены тильды у второго опознавателя
	EXPECT_EQ(keys.back(), "c~0d@2");
	// Выполняем проверку того, что сырое имя с косой чертой обращению не годится
	EXPECT_FALSE(document.at("/structures/a/b@1/k").valid());
}

/**
 * @brief Проверка всех перегрузок хода настроек и хода описания записи
 *
 * @details Ход `settings` объявлен ТРЕМЯ телами: получение настроек чтения, установка их
 *          и установка настроек записи. Проверялось же одно из трёх, а два стояли в карте
 *          покрытия непокрытыми прямо рядом
 *
 * @warning Разряд этот назван Василием 08.09.2026 и встречен им за день трижды: «тел ДВА,
 *          а позван один». Карта покрытия показывает такие места соседними строками, и
 *          опознаются они не чтением проверок, а розыском по карте: проверка зовёт ход по
 *          имени, а какое из тел позвано - из её текста не видно вовсе
 *
 * @note Проверяется не сам вызов, а СЛЕДСТВИЕ его: установленные настройки обязаны
 *       вернуться тем же ходом получения и изменить поведение разбора. Проверка, лишь
 *       зовущая ход, покрытие даёт, а свойства не поверяет
 *
 */
TEST(CodecSysLogDocument, SettingsOverloads) {
	// Объект события, удерживаемого целиком
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Настройки чтения записей, документу устанавливаемые
	syslog::reader_t::settings_t reader;
	// Устанавливаем описание записи современным
	reader.standard = syslog::standard_t::RFC5424;
	// Устанавливаем снятие отмены знаков блоков данных
	reader.unescape = false;
	// Выполняем проверку успешности установки настроек чтения
	ASSERT_TRUE(document.settings(reader));
	// Выполняем проверку того, что настройки чтения вернулись теми же
	EXPECT_EQ(document.settings().standard, syslog::standard_t::RFC5424);
	// Выполняем проверку того, что снятие отмены знаков сохранено
	EXPECT_FALSE(document.settings().unescape);
	// Настройки записи событий, документу устанавливаемые
	syslog::writer_t::settings_t writer;
	// Устанавливаем отказ от приставки приоритета
	writer.prefix = false;
	// Выполняем установку настроек записи событий
	document.settings(writer);
	// Выполняем проверку успешности разбора записи современного описания
	ASSERT_TRUE(document.parse("<34>1 2003-10-11T22:14:15.003Z host app - - - текст"));
	// Выполняем проверку того, что описание записи опознано и выдаётся ходом описания
	EXPECT_EQ(document.standard(), syslog::standard_t::RFC5424);
	// Выполняем проверку того, что настройка записи применена: приставки в сборе нет
	EXPECT_EQ(document.dump().find('<'), string::npos) << document.dump();
}

/**
 * @brief Проверка оборота события через файл
 *
 * @details Ходы `load` и `save` стояли в карте покрытия непокрытыми оба: проверки вели
 *          разбор и сбор записи знаками, файла не касаясь вовсе. Меж тем ходы эти
 *          объявлены наружу и потребителю даны
 *
 * @note Сличаются ДЕРЕВЬЯ, а не тексты: оборот через файл обязан давать то же событие, а
 *       не ту же последовательность знаков
 *
 */
TEST(CodecSysLogDocument, FileRoundtrip) {
	// Объект события, записываемого в файл
	syslog::document_t source(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности разбора записи
	ASSERT_TRUE(source.parse("<34>1 2003-10-11T22:14:15.003Z host app 1 ID47 [a@1 k=\"v\"] текст"));
	// Адрес временного файла оборота
	const string filename = "/tmp/awh-syslog-roundtrip.log";
	// Выполняем проверку успешности записи события в файл
	ASSERT_TRUE(source.save(filename));
	// Объект события, из файла читаемого
	syslog::document_t target(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности чтения события из файла
	ASSERT_TRUE(target.load(filename));
	// Выполняем проверку того, что оборот через файл дал то же дерево
	EXPECT_EQ(source.root(), target.root());
	// Выполняем снос временного файла оборота
	::remove(filename.c_str());
	// Выполняем проверку отказа чтения из несуществующего файла
	EXPECT_FALSE(target.load(filename));
	// Выполняем проверку того, что отказ назван кодом открытия файла
	EXPECT_EQ(target.error(), syslog::error_t::FILE_NOT_OPENED);
}

/**
 * @brief Проверка спросов у пустого дерева события
 *
 * @details Спрос у дерева, записи не несущего, отвечает НЕОПРЕДЕЛЁННОСТЬЮ, а не гибелью
 *          и не выдумкой: описание отвечается `AUTO`, приоритет нулём, дата пустой
 *          строкой. Потребитель волен спросить раньше чтения - и обязан различить
 *          «поля нет» по самому ответу
 *
 * @note Отдельно проверяется дата: `timestamp` спрашивает `time`, и оба хода читают одно
 *       поле. Ответь один пустотою, а другой выдумкой - и запись, из дерева собранная,
 *       несла бы утверждение о времени, какого никто не делал
 *
 */
TEST(CodecSysLogDocument, EmptyDocumentQueries) {
	// Объект работы с деревом события
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку того, что описание пустого дерева неопределённо
	EXPECT_EQ(document.standard(), syslog::standard_t::AUTO);
	// Выполняем проверку того, что приоритет пустого дерева нулевой
	EXPECT_EQ(document.priority(), static_cast <uint32_t> (0));
	// Выполняем проверку того, что дата пустого дерева пуста
	EXPECT_TRUE(document.timestamp().empty());
	// Выполняем проверку того, что время пустого дерева неопределённо
	EXPECT_EQ(document.time(), static_cast <uint64_t> (0));
	// Выполняем проверку того, что блоков структурированных данных нет вовсе
	EXPECT_EQ(document.size(), static_cast <size_t> (0));
	// Выполняем проверку того, что звеньев пути у пустого дерева нет вовсе
	EXPECT_TRUE(document.keys("/").empty());
	// Выполняем проверку того, что сбор записи из пустого дерева отказом завершается
	EXPECT_TRUE(document.dump().empty());
}

/**
 * @brief Проверка обхода дерева события по пути
 *
 * @details Путь звеньев не содержащий - пустой либо корневой - отвечается отказом
 *          отсутствия поля: ставить значение НЕКУДА, и сносить нечего. Отвечать успехом
 *          значило бы объявить, что действие произведено, тогда как дерево не тронуто
 *
 * @note Постановка значения вместилище звена ЗАВОДИТ, а снос - нет: первое достраивает
 *       дерево по пути, второе снимает уже стоящее. Оттого путь сквозь звено, картой не
 *       являющееся, постановке не помеха, а сносу - отказ отсутствия поля
 *
 */
TEST(CodecSysLogDocument, PathEdges) {
	// Объект работы с деревом события
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку отказа постановки значения пустым путём
	EXPECT_FALSE(document.set("", abc::value_t(string("value"))));
	// Выполняем проверку кода отказа постановки значения
	EXPECT_EQ(document.error(), syslog::error_t::UNKNOWN_FIELD);
	// Выполняем проверку отказа постановки значения корневым путём
	EXPECT_FALSE(document.set("/", abc::value_t(string("value"))));
	// Выполняем проверку отказа сноса значения пустым путём
	EXPECT_FALSE(document.erase(""));
	// Выполняем проверку кода отказа сноса значения
	EXPECT_EQ(document.error(), syslog::error_t::UNKNOWN_FIELD);
	// Выполняем проверку успешности постановки значения знаками
	ASSERT_TRUE(document.set("/header/hostname", abc::value_t(string("host"))));
	/**
	 * Выполняем проверку постановки значения сквозь звено, картой не являющееся
	 *
	 * @note Имя узла поставлено знаками, и путь сквозь него требует вместилища: оно
	 *       ЗАВОДИТСЯ картой, а прежнее значение знаками теряется. Такова цена того,
	 *       что путь достраивает дерево - и оттого проверяется прямо
	 */
	ASSERT_TRUE(document.set("/header/hostname/nested", abc::value_t(string("deep"))));
	// Выполняем проверку того, что вместилище звена заведено картой
	EXPECT_EQ(document.at("/header/hostname/nested").text(), string("deep"));
	// Выполняем проверку отказа сноса значения сквозь звено, картой не являющееся
	EXPECT_FALSE(document.erase("/header/hostname/nested/deeper"));
	// Выполняем проверку кода отказа сноса значения
	EXPECT_EQ(document.error(), syslog::error_t::UNKNOWN_FIELD);
	// Ставим блок структурированных данных в дерево события
	ASSERT_TRUE(document.set("/structures/id@1/param", abc::value_t(string("value"))));
	// Выполняем проверку выдачи звеньев пути отображения
	const vector <string> keys = document.keys("/structures/id@1");
	// Выполняем проверку числа звеньев пути отображения
	ASSERT_EQ(keys.size(), static_cast <size_t> (1));
	// Выполняем проверку имени звена пути отображения
	EXPECT_EQ(keys.front(), string("param"));
	// Выполняем проверку выдачи звеньев пути корня дерева события
	EXPECT_FALSE(document.keys("/").empty());
}

/**
 * @brief Проверка отказов работы с файлом и настройками
 *
 * @details Запись в файл, открыть какой не удалось, отвечается отказом, а не тихим
 *          успехом: потребитель, успех получивший, считал бы событие сохранённым.
 *          Настройки же разбора принимаются лишь у чтения, ничего ещё не принявшего:
 *          дерево, запись уже прочтённое, держит хранилище разбора непустым, и смена
 *          правил посреди потока отвечается отказом
 *
 * @note Отменяющая запись пути снимается лишь у знаков «~0» и «~1»; знак отмены с иной
 *       записью за ним берётся знаком как есть - имя поля, «~» несущее, тем и выразимо
 *
 */
TEST(CodecSysLogDocument, FailurePaths) {
	// Объект работы с деревом события
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Ставим имя узла знаками в дерево события
	ASSERT_TRUE(document.set("/header/hostname", abc::value_t(string("host"))));
	/**
	 * Выполняем проверку отказа сноса значения сквозь звено, картой не являющееся
	 *
	 * @note Звеном сквозного прохода взято поле, ЗНАКАМИ объявленное, а путь взят на
	 *       звено ГЛУБЖЕ: обход идёт по звеньям, кроме последнего, и путь «…/nested»
	 *       кончался бы раньше, чем дошёл до строки, - отказ приходил бы концом хода,
	 *       без кода ошибки вовсе. Замерено 08.09.2026: код отвечался пустым
	 */
	EXPECT_FALSE(document.erase("/header/hostname/nested/deeper"));
	// Выполняем проверку кода отказа сноса значения
	EXPECT_EQ(document.error(), syslog::error_t::UNKNOWN_FIELD);
	/**
	 * Ставим имя поля со знаком отмены, отменяющей записью НЕ являющимся
	 *
	 * @note Знак «~» с цифрой «9» за ним записью отмены не является, и оба знака берутся
	 *       именем как есть
	 */
	ASSERT_TRUE(document.set("/structures/id@1/a~9b", abc::value_t(string("value"))));
	// Выполняем проверку того, что имя со знаком отмены прочитано как есть
	EXPECT_EQ(document.at("/structures/id@1/a~9b").text(), string("value"));
	// Выполняем проверку отказа записи события в непригодный файл
	EXPECT_FALSE(document.save("/nonexistent-directory-awh/syslog.log"));
	// Выполняем чтение записи системного журнала деревом события
	ASSERT_TRUE(document.parse("<165>Oct 22 10:52:01 host app: Message"));
	// Настройки разбора записей
	syslog::reader_t::settings_t settings;
	/**
	 * Выполняем проверку отказа смены настроек у дерева, запись прочтённое
	 *
	 * @note Хранилище разбора остаётся непустым и по окончании чтения: смена правил
	 *       посреди потока дала бы записи, разными правилами прочтённые
	 */
	EXPECT_FALSE(document.settings(settings));
}

/**
 * @brief Проверка того, что каталог, поданный вместо файла, отвергается
 *
 * @details Каталог на POSIX открывается потоком УСПЕШНО и читается признаками конца -
 *          теми же, какими отзывается пустой файл. Кодек, каталога не распознавший,
 *          принимает его за файл пустой и отвечает ИСТИНОЙ с пустым деревом: ложный
 *          успех вместо отказа, отличить какой потребителю нечем
 *
 * @note Проверка стоит ЗДЕСЬ, а не только в наборе сличения кодеков: набор тот требует
 *       собранной третьей стороны и на отладочных машинах неприменим вовсе, а поведение
 *       это от системы ЗАВИСИТ - у MS Windows каталог не открывается потоком, и заслон,
 *       поставленный после открытия, был бы там мёртв. Восемь систем раскладки поверяют
 *       его отсюда
 *
 * @note Половина проверки отдана обратному: годный файл обязан читаться по-прежнему. Без
 *       неё «отказ на каталоге» достигался бы отказом на всяком пути вовсе
 *
 */
TEST(CodecSysLogDocument, DirectoryIsRefused) {
	// Адрес заводимого каталога подачи
	const string directory = "./syslog-directory-probe";
	// Выполняем снос каталога от прежнего прогона
	::rmdir(directory.c_str());
	// Выполняем заведение каталога подачи
	ASSERT_EQ(::mkdir(directory.c_str(), 0755), 0);
	// Объект работы с деревом события
	syslog::document_t document(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку отказа чтения каталога, поданного вместо файла
	EXPECT_FALSE(document.load(directory));
	// Выполняем проверку того, что отказ назван кодом чтения файла
	EXPECT_EQ(document.error(), syslog::error_t::FILE_NOT_READ);
	// Выполняем проверку того, что дерево события пустым осталось
	EXPECT_EQ(document.size(), static_cast <size_t> (0));
	// Выполняем снос заведённого каталога
	::rmdir(directory.c_str());
	// Адрес заводимого файла записи системного журнала
	const string filename = "./syslog-directory-probe.log";
	// Объект работы с деревом события для записи в файл
	syslog::document_t source(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем чтение записи системного журнала деревом события
	ASSERT_TRUE(source.parse("<165>Oct 22 10:52:01 host app: Message"));
	// Выполняем запись события в файл
	ASSERT_TRUE(source.save(filename));
	// Объект работы с деревом события для чтения из файла
	syslog::document_t target(&SilentSysLogDocument::framework(), ::documentLogger());
	// Выполняем проверку успешности чтения годного файла
	ASSERT_TRUE(target.load(filename)) << static_cast <uint32_t> (target.error());
	// Выполняем проверку того, что имя узла из файла прочтено
	EXPECT_EQ(target.at("/header/hostname").text(), string("host"));
	// Выполняем снос заведённого файла
	::remove(filename.c_str());
}
