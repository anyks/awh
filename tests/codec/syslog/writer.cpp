/**
 * @file writer.cpp
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
 * @brief Автоматические тесты записи событий в сообщение системного журнала — сборки заголовка,
 *        постановки отмены знаков, приведения даты и обращения с отсутствующими полями
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

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
	struct SilentSysLogWriter {
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
		SilentSysLogWriter() noexcept : log(&SilentSysLogWriter::framework()) {
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
	const awh::log_t * writerLogger() noexcept {
		// Объект журнала проверок
		static SilentSysLogWriter silent;
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
 * @brief Метод сборки записи из дерева заданными настройками
 *
 * @param value    дерево контейнера ABC
 * @param settings настройки записи событий
 * @param result   собранная запись системного журнала
 * @return         признак успешности сборки записи
 */
static bool build(const abc::value_t & value, const syslog::writer_t::settings_t & settings, string & result) noexcept {
	// Объект записи событий
	syslog::writer_t writer(&SilentSysLogWriter::framework(), ::writerLogger());
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выводим признак успешности сборки записи
	return writer.write(value, result);
}

/**
 * @brief Проверка сборки записи устаревшего описания
 *
 * @details Метка приложения закрывается двоеточием, а опознаватель работы берётся в
 *          квадратные скобки: без метки же ни того, ни другого в записи нет вовсе
 *
 */
TEST(CodecSysLogWriter, Legacy) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки: сличать удобнее без него
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим приоритет записи в дерево события
	tree.place("/priority") = abc::value_t(static_cast <uint64_t> (13));
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим имя узла в дерево события
	tree.place("/header/hostname") = abc::value_t(string("myhostname"));
	// Ставим название приложения в дерево события
	tree.place("/header/application") = abc::value_t(string("myapp"));
	// Ставим опознаватель работы в дерево события
	tree.place("/header/process") = abc::value_t(string("1234"));
	// Ставим текст сообщения в дерево события
	tree.place("/message") = abc::value_t(string("Sample message"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку собранной записи устаревшего описания
	EXPECT_EQ(result, "<13>Oct 22 12:34:56 myhostname myapp[1234]: Sample message");
	// Выполняем снос опознавателя работы из дерева события
	EXPECT_TRUE(tree.place("/header").erase(string("process")));
	// Выполняем проверку успешности сборки записи без опознавателя работы
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку собранной записи без опознавателя работы
	EXPECT_EQ(result, "<13>Oct 22 12:34:56 myhostname myapp: Sample message");
}

/**
 * @brief Проверка сборки записи нынешнего описания
 *
 * @details Поля заголовка нынешнего описания ПОЗИЦИОННЫ, и отсутствующее поле пишется
 *          знаком «-», а не пропускается: пропуск сдвинул бы все следующие, и запись
 *          разбиралась бы, но означала иное
 *
 */
TEST(CodecSysLogWriter, Modern) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим приоритет записи в дерево события
	tree.place("/priority") = abc::value_t(static_cast <uint64_t> (165));
	// Ставим номер описания записи в дерево события
	tree.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33.003Z"));
	// Ставим название приложения в дерево события
	tree.place("/header/application") = abc::value_t(string("MyApp"));
	// Ставим текст сообщения в дерево события
	tree.place("/message") = abc::value_t(string("Message"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку постановки знаков «-» на месте отсутствующих полей
	EXPECT_EQ(result, "<165>1 2023-04-11T23:29:33.003Z - MyApp - - - Message");
}

/**
 * @brief Проверка постановки отмены знаков в значениях структурированных данных
 *
 * @details Отменяются РОВНО ТРИ знака, описанием названные: кавычка, закрывающая
 *          скобка и сама обратная косая. Ни заголовок, ни текст сообщения отмены не
 *          знают вовсе, и ставить её там значило бы портить содержимое
 *
 */
TEST(CodecSysLogWriter, Escaping) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим приоритет записи в дерево события
	tree.place("/priority") = abc::value_t(static_cast <uint64_t> (10));
	// Ставим номер описания записи в дерево события
	tree.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("2023-12-25T15:29:22Z"));
	// Ставим имя узла в дерево события
	tree.place("/header/hostname") = abc::value_t(string("host"));
	// Ставим поле блока с кавычкой в дерево события
	tree.place("/structures/event@23668/p1") = abc::value_t(string("a\"b"));
	// Ставим поле блока с закрывающей скобкой в дерево события
	tree.place("/structures/event@23668/p2") = abc::value_t(string("c]d"));
	// Ставим поле блока с обратной косой в дерево события
	tree.place("/structures/event@23668/p3") = abc::value_t(string("e\\f"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку постановки отмены ровно трёх знаков
	EXPECT_EQ(result, "<10>1 2023-12-25T15:29:22Z host - - - "
	                  "[event@23668 p1=\"a\\\"b\" p2=\"c\\]d\" p3=\"e\\\\f\"]");
	// Выключаем постановку отмены знаков
	settings.escape = false;
	// Выполняем проверку успешности сборки записи без отмены знаков
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку того, что значения поставлены как есть
	EXPECT_EQ(result, "<10>1 2023-12-25T15:29:22Z host - - - "
	                  "[event@23668 p1=\"a\"b\" p2=\"c]d\" p3=\"e\\f\"]");
}

/**
 * @brief Проверка постановки метки порядка байтов перед текстом сообщения
 *
 * @details Метка ставится ЛИШЬ перед текстом, ASCII не являющимся: описание требует её
 *          признаком записи в UTF-8, а перед текстом ASCII она наращивала бы три
 *          октета на всякую запись без нужды
 *
 */
TEST(CodecSysLogWriter, ByteOrderMark) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим приоритет записи в дерево события
	tree.place("/priority") = abc::value_t(static_cast <uint64_t> (10));
	// Ставим номер описания записи в дерево события
	tree.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("2023-12-25T15:29:22Z"));
	// Ставим текст сообщения знаками ASCII в дерево события
	tree.place("/message") = abc::value_t(string("plain text"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку отсутствия метки перед текстом знаками ASCII
	EXPECT_EQ(result.find(string(syslog::BOM)), string::npos);
	// Ставим текст сообщения, знаками ASCII не исчерпываемый
	tree.place("/message") = abc::value_t(string("Текст события"));
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку постановки метки перед текстом, ASCII не являющимся
	EXPECT_NE(result.find(string(syslog::BOM)), string::npos);
	// Выключаем постановку метки порядка байтов
	settings.bom = false;
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку отсутствия метки при выключенной её постановке
	EXPECT_EQ(result.find(string(syslog::BOM)), string::npos);
}

/**
 * @brief Проверка того, что приставка приоритета НЕ домысливается
 *
 * @details Дерево, приоритета не несущее, собрано из записи, приставки не имевшей:
 *          дописать её значило бы объявить источник с важностью, каких отправитель не
 *          объявлял
 *
 * @note Найдено щупом оборота 07.09.2026: запись без приставки возвращалась из оборота
 *       с приставкой «<13>», взятой из настройки писателя
 *
 */
TEST(CodecSysLogWriter, PriorityIsNotInvented) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("Oct 22 10:52:01"));
	// Ставим имя узла в дерево события
	tree.place("/header/hostname") = abc::value_t(string("host"));
	// Ставим название приложения в дерево события
	tree.place("/header/application") = abc::value_t(string("app"));
	// Ставим текст сообщения в дерево события
	tree.place("/message") = abc::value_t(string("Message"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку отсутствия приставки приоритета у собранной записи
	EXPECT_EQ(result, "Oct 22 10:52:01 host app: Message");
}

/**
 * @brief Проверка сборки приоритета из источника и важности сообщения
 *
 * @details Приоритет есть источник, восьмикратно взятый, и важность: из частей он
 *          собирается лишь за неимением целого. Имена принимаются наравне с числами -
 *          службы журналов пишут правила отбора именами
 *
 */
TEST(CodecSysLogWriter, PriorityFromParts) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим источник сообщения числом в дерево события
	tree.place("/facility") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим важность сообщения числом в дерево события
	tree.place("/severity") = abc::value_t(static_cast <uint64_t> (5));
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("Oct 22 10:52:01"));
	// Ставим имя узла в дерево события
	tree.place("/header/hostname") = abc::value_t(string("host"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку сборки приоритета из чисел: 1 * 8 + 5 = 13
	EXPECT_EQ(result.compare(0, 5, "<13>O"), 0) << "запись: " << result;
	// Ставим источник сообщения именем в дерево события
	tree.place("/facility") = abc::value_t(string("user"));
	// Ставим важность сообщения именем в дерево события
	tree.place("/severity") = abc::value_t(string("notice"));
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку сборки приоритета из имён
	EXPECT_EQ(result.compare(0, 5, "<13>O"), 0) << "запись: " << result;
	// Ставим важность сообщения именем, наравне принятым
	tree.place("/severity") = abc::value_t(string("warn"));
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку сборки приоритета именем, наравне принятым: 1 * 8 + 4 = 12
	EXPECT_EQ(result.compare(0, 5, "<12>O"), 0) << "запись: " << result;
}

/**
 * @brief Проверка того, что дата не переписывается вовсе
 *
 * @details Содержимое поля есть то, что положил отправитель, и менять его кодек не
 *          вправе. Прежде дата приводилась к виду RFC 3339 у записей нынешнего
 *          описания; перепись снята 07.09.2026 после того, как ворошитель показал:
 *          дата, порчей испорченная, разбиралась ЧАСТИЧНО и переписывалась в
 *          утверждение о времени, какого отправитель не делал
 *
 * @note Найдено щупом оборота 07.09.2026: «Sat Jan  8 20:07:41 2011» возвращалось из
 *       оборота как «Jan  8 20:07:41»
 *
 */
TEST(CodecSysLogWriter, DateIsNeverRewritten) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим приоритет записи в дерево события
	tree.place("/priority") = abc::value_t(static_cast <uint64_t> (13));
	// Ставим дату сообщения, год несущую, в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("Sat Jan  8 20:07:41 2011"));
	// Ставим имя узла в дерево события
	tree.place("/header/hostname") = abc::value_t(string("host"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку сохранности даты у записи устаревшего описания
	EXPECT_EQ(result, "<13>Sat Jan  8 20:07:41 2011 host");
	// Ставим дату сообщения видом RFC 3339 в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("2003-10-11T22:14:15.003Z"));
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(build(tree, settings, result));
	/**
	 * Выполняем проверку сохранности зоны отправителя
	 *
	 * @note Перепись даты означала бы приведение её к местной зоне: зона отправителя
	 *       терялась бы без всякой нужды, а журналы её несут именно затем, чтобы
	 *       знать, где составлено сообщение
	 */
	EXPECT_EQ(result, "<13>2003-10-11T22:14:15.003Z host");
}

/**
 * @brief Проверка выбора описания записи по составу дерева
 *
 * @details Настройка `AUTO` берёт RFC 5424 тогда, когда дерево несёт то, чего RFC 3164
 *          не знает вовсе: номер описания, опознаватель сообщения либо структурированные
 *          данные. Объявленное же деревом описание СТАРШЕ угаданного
 *
 */
TEST(CodecSysLogWriter, Detection) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Объект записи событий
	syslog::writer_t writer(&SilentSysLogWriter::framework(), ::writerLogger());
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим приоритет записи в дерево события
	tree.place("/priority") = abc::value_t(static_cast <uint64_t> (13));
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим имя узла в дерево события
	tree.place("/header/hostname") = abc::value_t(string("host"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(writer.write(tree, result));
	// Выполняем проверку опознания устаревшего описания
	EXPECT_EQ(writer.standard(), syslog::standard_t::RFC3164);
	// Ставим блок структурированных данных в дерево события
	tree.place("/structures/id@1/a") = abc::value_t(string("b"));
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(writer.write(tree, result));
	// Выполняем проверку опознания нынешнего описания по блокам данных
	EXPECT_EQ(writer.standard(), syslog::standard_t::RFC5424);
	// Ставим описание записи знаками в дерево события
	tree.place("/standard") = abc::value_t(string("RFC3164"));
	// Выполняем проверку успешности сборки записи
	ASSERT_TRUE(writer.write(tree, result));
	// Выполняем проверку того, что объявленное описание старше угаданного
	EXPECT_EQ(writer.standard(), syslog::standard_t::RFC3164);
}

/**
 * @brief Проверка отказов сборки записи
 *
 * @details Код отказа, никогда не поднимаемый, есть обещание, ничем не подкреплённое
 *
 */
TEST(CodecSysLogWriter, Failures) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Объект записи событий
	syslog::writer_t writer(&SilentSysLogWriter::framework(), ::writerLogger());
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Собранная запись системного журнала
	string result = "";
	// Дерево собираемого события с приоритетом, за предел выходящим
	abc::value_t one(abc::kind_t::MAP);
	// Ставим приоритет, за предел выходящий, в дерево события
	one.place("/priority") = abc::value_t(static_cast <uint64_t> (999));
	// Ставим дату сообщения в дерево события
	one.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Выполняем проверку отказа сборки записи
	EXPECT_FALSE(writer.write(one, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	// Дерево собираемого события с неподдерживаемым номером описания
	abc::value_t two(abc::kind_t::MAP);
	// Ставим номер описания, не поддерживаемый, в дерево события
	two.place("/header/version") = abc::value_t(static_cast <uint64_t> (2));
	// Ставим дату сообщения в дерево события
	two.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	// Выполняем проверку отказа сборки записи
	EXPECT_FALSE(writer.write(two, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::UNSUPPORTED_VERSION);
	// Дерево собираемого события с полем заголовка, пробел несущим
	abc::value_t three(abc::kind_t::MAP);
	// Ставим номер описания записи в дерево события
	three.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	three.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	/**
	 * Ставим имя узла, пробел несущее, в дерево события
	 *
	 * @note Знака пробела внутри поля запись выразить не может: поля разделяются им, а
	 *       отмены знаков заголовок не знает вовсе - запись разбиралась бы двумя полями
	 */
	three.place("/header/hostname") = abc::value_t(string("host name"));
	// Выполняем проверку отказа сборки записи
	EXPECT_FALSE(writer.write(three, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::UNREPRESENTABLE_VALUE);
	// Дерево собираемого события с вложенным значением поля
	abc::value_t four(abc::kind_t::MAP);
	// Ставим дату сообщения в дерево события
	four.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим вложенное значение именем узла в дерево события
	four.place("/header/hostname/nested") = abc::value_t(string("value"));
	// Выполняем проверку отказа сборки записи
	EXPECT_FALSE(writer.write(four, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::NESTED_VALUE);
	// Дерево собираемого события с непредставимым опознавателем блока
	abc::value_t five(abc::kind_t::MAP);
	// Ставим номер описания записи в дерево события
	five.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	five.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	// Ставим блок с опознавателем, пробел несущим, в дерево события
	five.place(string("/structures/") + "bad id" + "/a") = abc::value_t(string("b"));
	// Выполняем проверку отказа сборки записи
	EXPECT_FALSE(writer.write(five, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_STRUCTURE_ID);
}

/**
 * @brief Проверка обращения с вложенным значением по настройке
 *
 * @details Дерева произвольной глубины запись системного журнала не несёт: исход
 *          выбирает тот, кто пишет, а не кодек
 *
 */
TEST(CodecSysLogWriter, Nested) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Дерево собираемого события
	abc::value_t tree(abc::kind_t::MAP);
	// Ставим приоритет записи в дерево события
	tree.place("/priority") = abc::value_t(static_cast <uint64_t> (13));
	// Ставим дату сообщения в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	/**
	 * Ставим имя узла в дерево события
	 *
	 * @note Имя узла ставится НЕПРЕМЕННО: описание требует его, и запись без него
	 *       отвечается отказом раньше, чем дело дойдёт до вложенного значения. Проверять
	 *       вложенность надлежит на поле, обязательным НЕ являющемся, - иначе проверка
	 *       мерила бы не обращение с вложенностью, а обязательность поля
	 */
	tree.place("/header/hostname") = abc::value_t(string("host"));
	// Ставим вложенное значение названием приложения в дерево события
	tree.place("/header/application/nested") = abc::value_t(string("value"));
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку отказа сборки записи строгим обращением
	EXPECT_FALSE(build(tree, settings, result));
	// Устанавливаем пропуск вложенного значения вовсе
	settings.nested = syslog::nested_t::SKIP;
	// Выполняем проверку успешности сборки записи пропуском вложенного значения
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку того, что вложенное значение в запись не попало
	EXPECT_EQ(result.find("nested"), string::npos) << "запись: " << result;
	// Устанавливаем обращение вложенного значения в знаки
	settings.nested = syslog::nested_t::TEXT;
	// Выполняем проверку успешности сборки записи обращением вложенного значения
	ASSERT_TRUE(build(tree, settings, result));
	// Выполняем проверку того, что вложенное значение обращено в знаки
	EXPECT_NE(result.find("nested"), string::npos) << "запись: " << result;
}
