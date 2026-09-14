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
#include <codec/syslog/syslog.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <clocale>
#include <cstring>
#include <cstdlib>
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <num/lexical/lexical.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Страж локали записи чисел
 *
 * @details Локаль есть настройка ПРОЦЕССА, а проверки всех кодеков идут одною
 *          программою: оставленная чужая локаль утекла бы к соседним проверкам и
 *          обратилась бы там «плавающим» отказом вдали от места
 *
 * @warning Возврат ведётся деструктором, а не строкою в конце проверки: всякий
 *          ASSERT_* выходит из проверки немедленно, и возврат, записанный последней
 *          строкою, по дороге отказа НЕ выполняется вовсе. Замечено 08.09.2026
 *
 */
struct LocaleGuard {
	// Локаль записи чисел, действовавшая до подмены
	string previous;
	/**
	 * @brief Конструктор
	 *
	 */
	LocaleGuard() noexcept : previous(::setlocale(LC_NUMERIC, nullptr)) {}
	/**
	 * @brief Деструктор
	 *
	 */
	~LocaleGuard() noexcept {
		// Выполняем возврат действовавшей локали записи чисел
		::setlocale(LC_NUMERIC, this->previous.c_str());
	}
};

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
	syslog::writer_t writer;
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
	syslog::writer_t writer;
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
	syslog::writer_t writer;
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

/**
 * @brief Проверка глухоты записи чисел к локали
 *
 * @details Дробное значение блока данных обращается в знаки ходом оснастки, а не
 *          `snprintf`, и потому десятичным знаком записи остаётся точка при всякой
 *          локали. Описание RFC 5424 иного знака не знает вовсе: запись с запятой
 *          читается уже не числом
 *
 * @warning Локаль ставится ПОСЛЕ заведения оснастки, а не до: `fmk_t` ставит свою в
 *          конструкторе и установку, сделанную раньше, перекрывает. Опыт, поставленный
 *          до заведения оснастки, показывает точку ВСЕГДА и глухоту не проверяет вовсе.
 *          Установлено 08.09.2026 щупом, где условие не заводилось ни на macOS, ни на
 *          Debian, пока порядок не был исправлен
 *
 * @note Пропуск здесь - свойство системы, а не пробел проверки: у musl локалей нет
 *       вовсе, а у OpenBSD не заведён разряд LC_NUMERIC (замерено 08.09.2026 на
 *       10.100.1.145: 68 локалей, `de_DE.UTF-8` принимается, десятичный знак неизменно
 *       точка). Глухота там оттого НЕ ПРОВЕРЕНА, и зелёный прогон обещания не несёт
 *
 */
TEST(CodecSysLogWriter, LocaleNumbers) {
	/**
	 * Выполняем заведение оснастки ЗАРАНЕЕ, до всякой установки локали
	 *
	 * @warning Без этого проверка зелена ЛОЖНО: `fmk_t` ставит свою локаль в
	 *          конструкторе, а объект оснастки здесь заводится при первом обращении -
	 *          то есть внутри цикла, уже после установки чужой локали. Число писалось бы
	 *          тогда при точке, восстановленной оснасткой, и глухота не проверялась бы
	 *          вовсе. Замерено 08.09.2026: до заведения оснастки `de_DE.UTF-8` даёт «,»,
	 *          после - «.»
	 */
	awh::fmk::initialize();
	// Страж возврата локали записи чисел
	const LocaleGuard guard;
	// Количество проверенных локалей с иным десятичным знаком
	uint32_t checked = 0;
	/**
	 * Выполняем перебор названий локали с иным знаком десятичной точки
	 *
	 * @note Названия эти у разных систем свои, и ни одно не признаётся всюду: у Debian
	 *       ходит «de_DE.utf8», у MS Windows - «German_Germany». Локали «fa_IR» и
	 *       «ar_SA» взяты особо: десятичным знаком там служит «٫» (U+066B), занимающий
	 *       в UTF-8 два байта
	 */
	for(const char * name : {"de_DE.UTF-8", "de_DE.utf8", "German_Germany.1252", "German_Germany", "fa_IR.UTF-8", "ar_SA.UTF-8"}){
		// Если установить очередную локаль не удалось
		if(::setlocale(LC_NUMERIC, name) == nullptr)
			// Выполняем переход к следующей локали
			continue;
		// Если знаком десятичной точки установленной локали точка всё же осталась
		if(::localeconv()->decimal_point[0] == '.')
			// Выполняем переход к следующей локали
			continue;
		// Дерево собираемого события
		abc::value_t tree;
		// Ставим дату отправки записи в дерево события
		tree.place("/header/timestamp") = abc::value_t(string("2003-10-11T22:14:15.003Z"));
		// Ставим имя узла отправителя в дерево события
		tree.place("/header/hostname") = abc::value_t(string("host"));
		// Ставим дробное значение блоком данных в дерево события
		tree.place("/structures/id@1/k") = abc::value_t(0.1);
		// Ставим дробное значение с большим количеством разрядов в дерево события
		tree.place("/structures/id@1/m") = abc::value_t(2986.808299);
		// Настройки записи событий
		syslog::writer_t::settings_t settings;
		// Устанавливаем описание записи современным
		settings.standard = syslog::standard_t::RFC5424;
		// Собранная запись системного журнала
		string result = "";
		// Выполняем проверку успешности сборки записи под чужой локалью
		ASSERT_TRUE(build(tree, settings, result)) << name;
		/**
		 * Выполняем проверку того, что чужая локаль держалась ВО ВРЕМЯ записи
		 *
		 * @note Сторож этот и есть то, чем проверка отличается от собственной видимости:
		 *       сбрось её кто-нибудь до записи числа - и точка в выдаче доказывала бы
		 *       лишь то, что локаль была точкою, а вовсе не глухоту кодека
		 */
		ASSERT_NE(::localeconv()->decimal_point[0], '.') << name << ": локаль сброшена до записи числа";
		/**
		 * Выполняем проверку годности записанных чисел обратным разбором
		 *
		 * @warning Сличать здесь ВИД записи нельзя, лишь значение: кратчайшее
		 *          представление числа держится не всюду. У OpenIndiana под локалью
		 *          «ar_SA.UTF-8», где десятичным знаком служит «٫» (U+066B, два байта в
		 *          UTF-8), оснастка выдаёт «0.10000000000000001» вместо «0.1», тогда как
		 *          под однобайтовой «,» у той же машины выходит «0.1». Знак при этом
		 *          ТОЧКА в обоих случаях, то есть глухота к локали цела, а разнится лишь
		 *          длина записи. Замерено 08.09.2026, донесено владельцу «sys/fmk»
		 *
		 * @note Предмет проверки - глухота к локали, а не вид числа: запись с чужим
		 *       десятичным знаком читается уже не числом, а запись длинная читается тем
		 *       же значением
		 */
		for(const pair <string, double> & expected : {make_pair(string("k"), 0.1), make_pair(string("m"), 2986.808299)}){
			// Место начала значения очередного поля блока данных
			const size_t begin = result.find(expected.first + "=\"");
			// Выполняем проверку наличия очередного поля блока данных в записи
			ASSERT_NE(begin, string::npos) << name << ": " << result;
			// Место окончания значения очередного поля блока данных
			const size_t end = result.find('"', begin + expected.first.size() + 2);
			// Выполняем проверку замкнутости значения очередного поля блока данных
			ASSERT_NE(end, string::npos) << name << ": " << result;
			// Записанное значение очередного поля блока данных
			const string written = result.substr(begin + expected.first.size() + 2, end - begin - expected.first.size() - 2);
			// Выполняем проверку того, что чужой десятичный знак в запись не попал
			EXPECT_EQ(written.find(::localeconv()->decimal_point), string::npos) << name << ": " << written;
			/**
			 * Обратно прочитанное значение очередного поля блока данных
			 *
			 * @warning Читается оно `lexical_t`, а НЕ `strtod`: тот сам локали
			 *          подвластен и под немецкой читает «0.1» нулём, обращая проверку
			 *          глухоты в проверку того же самого разряда локали с другой
			 *          стороны. Установлено 08.09.2026: проверка падала на своей же
			 *          машине, показывая 0 вместо 0.1
			 */
			double parsed = 0.;
			// Выполняем обратное чтение записанного значения без оглядки на локаль
			awh::lexical_t::fromChars(written.c_str(), written.c_str() + written.size(), parsed);
			// Выполняем проверку того, что записанное значение читается тем же числом
			EXPECT_DOUBLE_EQ(parsed, expected.second) << name << ": " << written;
		}
		// Выполняем учёт проверенной локали
		checked++;
	}
	// Если ни одной локали с иным десятичным знаком в системе не нашлось
	if(checked == 0)
		// Объявляем проверку пропущенной с указанием причины
		GTEST_SKIP() << "локали с иным десятичным знаком в системе нет";
}

/**
 * @brief Проверка обращения значений всех видов в знаки записи
 *
 * @details Значение блока структурированных данных по описанию RFC 5424 есть строка, а
 *          дерево событий держит виды языка - логический, целые обоих знаков, дробный.
 *          Писатель обращает их в знаки сам, и у всякого вида ветвь своя
 *
 * @warning Ветви эти стояли в карте покрытия непокрытыми ВСЕ, кроме строковой: проверки
 *          писателя подавали в дерево одни строки, и обращение видов, объявленное
 *          свойством кодека, не поверялось ничем. Найдено 08.09.2026 разбором карты по
 *          доводу Василия - непокрытая ветвь неотличима от недостижимой, покуда не
 *          замерена
 *
 * @note Логическое значение пишется словами «true» и «false», а не числами: описание
 *       записи чисел от слов не отличает, и выбор этот наружу видим
 *
 */
TEST(CodecSysLogWriter, ValueKinds) {
	// Дерево собираемого события
	abc::value_t tree;
	// Ставим дату отправки записи в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("2003-10-11T22:14:15.003Z"));
	// Ставим имя узла отправителя в дерево события
	tree.place("/header/hostname") = abc::value_t(string("host"));
	// Ставим логическое значение истины блоком данных
	tree.place("/structures/id@1/yes") = abc::value_t(true);
	// Ставим логическое значение лжи блоком данных
	tree.place("/structures/id@1/no") = abc::value_t(false);
	// Ставим целое значение со знаком блоком данных
	tree.place("/structures/id@1/signed") = abc::value_t(static_cast <int64_t> (-42));
	// Ставим целое значение без знака блоком данных
	tree.place("/structures/id@1/unsigned") = abc::value_t(static_cast <uint64_t> (42));
	// Ставим дробное значение блоком данных
	tree.place("/structures/id@1/real") = abc::value_t(0.5);
	// Ставим строковое значение блоком данных
	tree.place("/structures/id@1/text") = abc::value_t(string("знаки"));
	// Настройки записи событий
	syslog::writer_t::settings_t settings;
	// Устанавливаем описание записи современным
	settings.standard = syslog::standard_t::RFC5424;
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи всеми видами значений
	ASSERT_TRUE(build(tree, settings, result)) << result;
	// Выполняем проверку обращения логической истины словом
	EXPECT_NE(result.find("yes=\"true\""), string::npos) << result;
	// Выполняем проверку обращения логической лжи словом
	EXPECT_NE(result.find("no=\"false\""), string::npos) << result;
	// Выполняем проверку обращения целого значения со знаком
	EXPECT_NE(result.find("signed=\"-42\""), string::npos) << result;
	// Выполняем проверку обращения целого значения без знака
	EXPECT_NE(result.find("unsigned=\"42\""), string::npos) << result;
	// Выполняем проверку обращения дробного значения
	EXPECT_NE(result.find("real=\"0.5\""), string::npos) << result;
	// Выполняем проверку обращения строкового значения
	EXPECT_NE(result.find("text=\"знаки\""), string::npos) << result;
	/**
	 * Выполняем проверку замкнутости оборота по всем видам значений разом
	 *
	 * @note Сличается ДЕРЕВО, а не текст: значения блоков по описанию строковы, и
	 *       обратный разбор отдаёт их знаками - тем же, какими они и записаны
	 */
	{
		// Объект события, собранную запись читающий
		syslog::document_t document;
		// Выполняем проверку успешности обратного разбора собранной записи
		ASSERT_TRUE(document.parse(result)) << result;
		// Значение, по пути извлекаемое
		string text = "";
		// Выполняем извлечение обращённого дробного значения
		ASSERT_TRUE(document.at("/structures/id@1/real").value(text));
		// Выполняем проверку того, что дробное значение прочиталось тем же
		EXPECT_EQ(text, "0.5");
		// Выполняем извлечение обращённого логического значения
		ASSERT_TRUE(document.at("/structures/id@1/yes").value(text));
		// Выполняем проверку того, что логическое значение прочиталось словом
		EXPECT_EQ(text, "true");
	}
}

/**
 * @brief Проверка отказов записи по непредставимым именам и значениям блоков
 *
 * @details Имя поля блока структурированных данных по RFC 5424 не вправе нести пробела,
 *          знака равенства, закрывающей скобки и кавычки: все они разделяют запись, и
 *          поле с таким именем прочиталось бы иначе, чем записано. Значение же вложенное
 *          - карта либо перечень - записью не выражается вовсе, ибо поле блока строково
 *
 * @warning Ветви эти стояли в карте покрытия непокрытыми ЦЕЛИКОМ - и отказ, и пропуск по
 *          настройке. Меж тем настройка `nested` объявлена наружу и обещает выбор между
 *          отказом и пропуском; необеспеченное обещание тут было бы видно лишь
 *          потребителю
 *
 * @note Проверяется не только отказ, но и КОД его: отказ по имени и отказ по значению
 *       различаются кодами, и потребитель по ним судит, чинить ему имя или вид значения
 *
 */
TEST(CodecSysLogWriter, UnrepresentableParams) {
	/**
	 * Отказ по непредставимому имени поля блока
	 */
	{
		// Дерево собираемого события
		abc::value_t tree;
		// Ставим дату отправки записи в дерево события
		tree.place("/header/timestamp") = abc::value_t(string("2003-10-11T22:14:15.003Z"));
		// Ставим имя узла отправителя в дерево события
		tree.place("/header/hostname") = abc::value_t(string("host"));
		// Ставим поле блока с пробелом в имени: запись такого имени не выражает
		tree.place("/structures/id@1")["имя с пробелом"] = abc::value_t(string("v"));
		// Настройки записи событий
		syslog::writer_t::settings_t settings;
		// Устанавливаем описание записи современным
		settings.standard = syslog::standard_t::RFC5424;
		// Объект записи событий
		syslog::writer_t writer;
		// Устанавливаем настройки записи событий
		writer.settings(settings);
		// Собранная запись системного журнала
		string result = "";
		// Выполняем проверку отказа сборки записи непредставимым именем поля
		ASSERT_FALSE(writer.write(tree, result));
		// Выполняем проверку того, что отказ назван кодом ошибочного имени поля
		EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PARAM_NAME);
		// Устанавливаем пропуск непредставимого вовсе
		settings.nested = syslog::nested_t::SKIP;
		// Устанавливаем изменённые настройки записи событий
		writer.settings(settings);
		// Выполняем проверку успешности сборки записи пропуском непредставимого имени
		ASSERT_TRUE(writer.write(tree, result)) << result;
		// Выполняем проверку того, что поле с непредставимым именем в запись не попало
		EXPECT_EQ(result.find("имя с пробелом"), string::npos) << result;
	}
	/**
	 * Отказ по вложенному значению поля блока
	 */
	{
		// Дерево собираемого события
		abc::value_t tree;
		// Ставим дату отправки записи в дерево события
		tree.place("/header/timestamp") = abc::value_t(string("2003-10-11T22:14:15.003Z"));
		// Ставим имя узла отправителя в дерево события
		tree.place("/header/hostname") = abc::value_t(string("host"));
		// Ставим годное поле блока рядом с негодным
		tree.place("/structures/id@1/plain") = abc::value_t(string("v"));
		// Ставим вложенное значение полем блока: запись его не выражает
		tree.place("/structures/id@1/nested/inner") = abc::value_t(string("w"));
		// Настройки записи событий
		syslog::writer_t::settings_t settings;
		// Устанавливаем описание записи современным
		settings.standard = syslog::standard_t::RFC5424;
		// Объект записи событий
		syslog::writer_t writer;
		// Устанавливаем настройки записи событий
		writer.settings(settings);
		// Собранная запись системного журнала
		string result = "";
		// Выполняем проверку отказа сборки записи вложенным значением поля
		ASSERT_FALSE(writer.write(tree, result));
		// Выполняем проверку того, что отказ назван кодом вложенного значения
		EXPECT_EQ(writer.error(), syslog::error_t::NESTED_VALUE);
		// Устанавливаем пропуск вложенного значения вовсе
		settings.nested = syslog::nested_t::SKIP;
		// Устанавливаем изменённые настройки записи событий
		writer.settings(settings);
		// Выполняем проверку успешности сборки записи пропуском вложенного значения
		ASSERT_TRUE(writer.write(tree, result)) << result;
		// Выполняем проверку того, что вложенное значение в запись не попало
		EXPECT_EQ(result.find("nested"), string::npos) << result;
		// Выполняем проверку того, что годное поле рядом с ним записано по-прежнему
		EXPECT_NE(result.find("plain=\"v\""), string::npos) << result;
	}
}

/**
 * @brief Проверка видов значений, записью не выражаемых, и значения пустого
 *
 * @details Дерево событий держит виды, каких запись системного журнала не знает вовсе:
 *          двоичные данные, отметку времени, опознаватель, целое сверх родной ширины.
 *          Полем блока они непредставимы, и писатель отвечает на них отказом
 *          `UNREPRESENTABLE_VALUE`. Значение ПУСТОЕ - иное дело: оно представимо пустой
 *          записью, ибо поле блока строково, а пустая строка есть законное значение
 *
 * @warning Ветвь эта стояла в карте покрытия непокрытой, и достижимость её пришлось
 *          доказывать замером: разбор видов ловит девять из четырнадцати, а хвост за ним
 *          берёт остальные пять. Прочтя карту, ветвь легко счесть мёртвой - у соседнего
 *          кодека такая и оказалась мёртвой вовсе
 *
 * @note Пустое значение и ОТСУТСТВИЕ поля - разное: первое пишется полем с пустым
 *       значением, второго в записи нет вовсе
 *
 */
TEST(CodecSysLogWriter, UnsupportedValueKinds) {
	// Дерево собираемого события
	abc::value_t tree;
	// Ставим дату отправки записи в дерево события
	tree.place("/header/timestamp") = abc::value_t(string("2003-10-11T22:14:15.003Z"));
	// Ставим имя узла отправителя в дерево события
	tree.place("/header/hostname") = abc::value_t(string("host"));
	/**
	 * Ставим пустое значение полем блока структурированных данных
	 *
	 * @warning Пустое значение заводится ВИДОМ `kind_t::NUL`, а не конструктором без
	 *          доводов: тот даёт `UNDEFINED` - «значения нет вовсе, ссылка
	 *          недействительна», - и запись его законно отвергается как непредставимая.
	 *          Первая редакция проверки на том и упала, обвинив кодек в своей же ошибке
	 */
	tree.place("/structures/id@1/nul") = abc::value_t(abc::kind_t::NUL);
	// Настройки записи событий
	syslog::writer_t::settings_t settings;
	// Устанавливаем описание записи современным
	settings.standard = syslog::standard_t::RFC5424;
	// Собранная запись системного журнала
	string result = "";
	// Выполняем проверку успешности сборки записи с пустым значением поля
	ASSERT_TRUE(build(tree, settings, result)) << result;
	// Выполняем проверку того, что пустое значение записано пустой записью
	EXPECT_NE(result.find("nul=\"\""), string::npos) << result;
	/**
	 * Ставим отметку времени полем блока: запись её не выражает вовсе
	 *
	 * @note Взята именно отметка времени, а не двоичные данные: ПУСТОЙ `BLOB` выражается
	 *       пустой записью законно - данных в нём нет, и обращать нечего, - и отказа не
	 *       даёт. Замерено 08.09.2026: первая редакция проверки брала его и падала,
	 *       обвиняя кодек в своей же неверной догадке. Предмет проверки выбирается
	 *       замером, а не рассуждением о том, какой вид «должен» быть непредставим
	 */
	tree.place("/structures/id@1/time") = abc::value_t(abc::kind_t::TIME);
	// Объект записи событий
	syslog::writer_t writer;
	/**
	 * Выполняем перебор всех настроек обращения вложенных значений
	 *
	 * @note Настройка `nested` названа по вложенным значениям и их же касается: карты и
	 *       перечня. Отметка времени вложенной не является, и отказ по ней остаётся
	 *       отказом при ВСЯКОЙ настройке - проверено всеми тремя, а не одной
	 */
	for(const syslog::nested_t nested : {syslog::nested_t::STRICT, syslog::nested_t::SKIP, syslog::nested_t::TEXT}){
		// Устанавливаем очередную настройку обращения вложенных значений
		settings.nested = nested;
		// Устанавливаем изменённые настройки записи событий
		writer.settings(settings);
		// Выполняем проверку того, что отказ по невыразимому виду настройкою не снимается
		EXPECT_FALSE(writer.write(tree, result)) << static_cast <uint32_t> (nested);
		// Выполняем проверку того, что отказ назван кодом невыразимого значения
		EXPECT_EQ(writer.error(), syslog::error_t::UNREPRESENTABLE_VALUE) << static_cast <uint32_t> (nested);
	}
}

/**
 * @brief Проверка отказов сборки записи устаревшего описания
 *
 * @details Поля устаревшего описания позиционны и разделены пробелом: дата, имя узла и
 *          метка приложения стоят одно за другим, и отсутствие любого из первых двух
 *          сдвинуло бы все следующие. Запись, вышедшая из такого сдвига, РАЗБИРАЕТСЯ
 *          повторно - и означает иное: имя узла становится датой, приложение именем
 *          узла. Оттого отсутствие отвечается отказом, а не пропуском поля
 *
 * @note Опознаватель работы стоит ВНУТРИ метки приложения - «app[pid]:», - и без самого
 *       имени приложения места ему в записи нет вовсе. Условие смотрит на ЗНАЧЕНИЕ, а
 *       не на объявленность поля: объявленное пустым имя ведёт к той же неразборчивой
 *       записи «[pid]:», что и вовсе отсутствующее
 *
 */
TEST(CodecSysLogWriter, LegacyFailures) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Устанавливаем сборку записи устаревшим описанием
	settings.standard = syslog::standard_t::RFC3164;
	// Объект записи событий
	syslog::writer_t writer;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Собранная запись системного журнала
	string result = "";
	// Дерево собираемого события без даты сообщения вовсе
	abc::value_t one(abc::kind_t::MAP);
	// Ставим имя узла в дерево события
	one.place("/header/hostname") = abc::value_t(string("host"));
	// Выполняем проверку отказа сборки записи отсутствующей датой сообщения
	EXPECT_FALSE(writer.write(one, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INCOMPLETE_HEADER);
	// Дерево собираемого события без имени узла вовсе
	abc::value_t two(abc::kind_t::MAP);
	// Ставим дату сообщения в дерево события
	two.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Выполняем проверку отказа сборки записи отсутствующим именем узла
	EXPECT_FALSE(writer.write(two, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INCOMPLETE_HEADER);
	// Дерево собираемого события с опознавателем работы без названия приложения
	abc::value_t three(abc::kind_t::MAP);
	// Ставим дату сообщения в дерево события
	three.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим имя узла в дерево события
	three.place("/header/hostname") = abc::value_t(string("host"));
	// Ставим опознаватель работы в дерево события
	three.place("/header/process") = abc::value_t(string("4711"));
	// Выполняем проверку отказа сборки записи опознавателем без названия приложения
	EXPECT_FALSE(writer.write(three, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::UNREPRESENTABLE_VALUE);
	/**
	 * Ставим ПУСТОЕ название приложения в дерево события
	 *
	 * @note Условие смотрит на значение: объявленное пустым имя ведёт к той же
	 *       неразборчивой записи, что и вовсе отсутствующее, - отказ обязан остаться
	 */
	three.place("/header/application") = abc::value_t(string(""));
	// Выполняем проверку того, что пустое название приложения отказа не снимает
	EXPECT_FALSE(writer.write(three, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::UNREPRESENTABLE_VALUE);
	// Ставим название приложения в дерево события
	three.place("/header/application") = abc::value_t(string("app"));
	// Выполняем проверку успешности сборки записи названием приложения
	ASSERT_TRUE(writer.write(three, result));
	// Выполняем проверку того, что опознаватель работы встал внутрь метки приложения
	EXPECT_NE(result.find("app[4711]:"), string::npos) << "запись: " << result;
}

/**
 * @brief Проверка отказов сборки приставки приоритета
 *
 * @details Приоритет объявляется числом либо частями - источником и важностью, - и
 *          части принимаются как числом, так и ИМЕНЕМ: службы журналов пишут правила
 *          отбора именами, и дерево, руками собранное, несёт их же. Имя, словарю
 *          неизвестное, отвечается отказом, а не молчаливым нулём: нуль означал бы
 *          источник «kern», какого отправитель не объявлял
 *
 * @note Пределы частей проверяются ДО сборки приоритета: источник шире 23 либо важность
 *       шире 7 дали бы приоритет, в приставку не помещающийся, и отказ по итоговому
 *       числу назвал бы виновным не то поле
 *
 */
TEST(CodecSysLogWriter, PriorityFailures) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Объект записи событий
	syslog::writer_t writer;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Собранная запись системного журнала
	string result = "";
	// Дерево собираемого события с неизвестным именем источника сообщения
	abc::value_t one(abc::kind_t::MAP);
	// Ставим неизвестное имя источника сообщения в дерево события
	one.place("/facility") = abc::value_t(string("unknown"));
	// Ставим дату сообщения в дерево события
	one.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим имя узла в дерево события
	one.place("/header/hostname") = abc::value_t(string("host"));
	// Выполняем проверку отказа сборки записи неизвестным источником сообщения
	EXPECT_FALSE(writer.write(one, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	// Ставим известное имя источника сообщения в дерево события
	one.place("/facility") = abc::value_t(string("user"));
	// Ставим неизвестное имя важности сообщения в дерево события
	one.place("/severity") = abc::value_t(string("unknown"));
	// Выполняем проверку отказа сборки записи неизвестной важностью сообщения
	EXPECT_FALSE(writer.write(one, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	// Ставим источник сообщения числом, за предел выходящим, в дерево события
	one.place("/facility") = abc::value_t(static_cast <uint64_t> (24));
	// Ставим важность сообщения числом в дерево события
	one.place("/severity") = abc::value_t(static_cast <uint64_t> (5));
	// Выполняем проверку отказа сборки записи источником, за предел выходящим
	EXPECT_FALSE(writer.write(one, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	// Ставим источник сообщения числом в дерево события
	one.place("/facility") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим важность сообщения числом, за предел выходящим, в дерево события
	one.place("/severity") = abc::value_t(static_cast <uint64_t> (8));
	// Выполняем проверку отказа сборки записи важностью, за предел выходящей
	EXPECT_FALSE(writer.write(one, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	/**
	 * Дерево собираемого события с вложенным источником сообщения
	 *
	 * @note Дерево заводится ЗАНОВО, а не правится прежнее: `place` по пути
	 *       «/facility/nested» узла, уже несущего число, видом карты не делает, и
	 *       источник остался бы числом - проверка мерила бы прежний опыт вторично и
	 *       зеленела бы, что ни клади. Замерено 08.09.2026: сборка проходила успехом
	 */
	abc::value_t five(abc::kind_t::MAP);
	// Ставим источник сообщения вложенным значением в дерево события
	five.place("/facility/nested") = abc::value_t(string("value"));
	// Ставим важность сообщения числом в дерево события
	five.place("/severity") = abc::value_t(static_cast <uint64_t> (5));
	// Ставим дату сообщения в дерево события
	five.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим имя узла в дерево события
	five.place("/header/hostname") = abc::value_t(string("host"));
	// Выполняем проверку отказа сборки записи вложенным источником сообщения
	EXPECT_FALSE(writer.write(five, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	// Дерево собираемого события с вложенной важностью сообщения
	abc::value_t two(abc::kind_t::MAP);
	// Ставим источник сообщения числом в дерево события
	two.place("/facility") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим важность сообщения вложенным значением в дерево события
	two.place("/severity/nested") = abc::value_t(string("value"));
	// Ставим дату сообщения в дерево события
	two.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим имя узла в дерево события
	two.place("/header/hostname") = abc::value_t(string("host"));
	// Выполняем проверку отказа сборки записи вложенной важностью сообщения
	EXPECT_FALSE(writer.write(two, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	// Дерево собираемого события с вложенным приоритетом записи
	abc::value_t three(abc::kind_t::MAP);
	// Ставим приоритет записи вложенным значением в дерево события
	three.place("/priority/nested") = abc::value_t(string("value"));
	// Ставим дату сообщения в дерево события
	three.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
	// Ставим имя узла в дерево события
	three.place("/header/hostname") = abc::value_t(string("host"));
	// Выполняем проверку отказа сборки записи вложенным приоритетом
	EXPECT_FALSE(writer.write(three, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_PRIORITY);
	// Дерево собираемого события, отображением не являющееся
	abc::value_t four(abc::kind_t::ARRAY);
	// Выполняем проверку отказа сборки записи неверным устройством дерева
	EXPECT_FALSE(writer.write(four, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::UNREPRESENTABLE_VALUE);
}

/**
 * @brief Проверка отказа сборки записи вложенным полем заголовка
 *
 * @details Поле заголовка есть значение знаками, и дерево на его месте выразить нечем:
 *          запись отмены знаков в заголовке не знает вовсе. Отказ обязан наступать у
 *          ВСЯКОГО поля обоих описаний, а не у одного лишь имени узла: поля разбираются
 *          порознь, и заслон, поставленный у одного, о прочих ничего не говорит
 *
 * @note Ход обращения значения зовётся для каждого поля свой, и непокрытая ветвь отказа
 *       у одного из них неотличима от недостижимой, покуда не замерена. Оттого поля
 *       перебираются перечнем, а не берутся выборочно
 *
 */
TEST(CodecSysLogWriter, NestedHeaderFields) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Объект записи событий
	syslog::writer_t writer;
	// Собранная запись системного журнала
	string result = "";
	/**
	 * Выполняем перебор всех полей заголовка устаревшего описания
	 *
	 * @note Опознаватель работы стоит внутри метки приложения, и вложенным он выразим
	 *       быть не может так же, как и прочие поля
	 */
	for(const char * name : {"timestamp", "hostname", "application", "process"}){
		// Дерево собираемого события устаревшего описания
		abc::value_t tree(abc::kind_t::MAP);
		// Устанавливаем сборку записи устаревшим описанием
		settings.standard = syslog::standard_t::RFC3164;
		// Устанавливаем настройки записи событий
		writer.settings(settings);
		/**
		 * Ставим вложенное значение очередным полем заголовка ПЕРВЫМ
		 *
		 * @warning Порядок постановки значим: `place` по пути сквозь узел, уже несущий
		 *          знаки, видом карты его НЕ делает, и вложенное значение до дерева не
		 *          дошло бы вовсе. Замерено 08.09.2026: сборка проходила успехом у трёх
		 *          полей из четырёх, и проверка обвиняла кодек в своей же ошибке
		 */
		tree.place(string("/header/") + name + "/nested") = abc::value_t(string("value"));
		// Если очередным полем заголовка дата сообщения не является
		if(::strcmp(name, "timestamp") != 0)
			// Ставим дату сообщения в дерево события
			tree.place("/header/timestamp") = abc::value_t(string("Oct 22 12:34:56"));
		// Если очередным полем заголовка имя узла не является
		if(::strcmp(name, "hostname") != 0)
			// Ставим имя узла в дерево события
			tree.place("/header/hostname") = abc::value_t(string("host"));
		// Если очередным полем заголовка название приложения не является
		if(::strcmp(name, "application") != 0)
			// Ставим название приложения в дерево события
			tree.place("/header/application") = abc::value_t(string("app"));
		// Выполняем проверку отказа сборки записи вложенным полем заголовка
		EXPECT_FALSE(writer.write(tree, result)) << name;
	}
	/**
	 * Выполняем перебор всех полей заголовка нынешнего описания
	 */
	for(const char * name : {"version", "timestamp", "hostname", "application", "process", "messageId"}){
		// Дерево собираемого события нынешнего описания
		abc::value_t tree(abc::kind_t::MAP);
		// Устанавливаем сборку записи нынешним описанием
		settings.standard = syslog::standard_t::RFC5424;
		// Устанавливаем настройки записи событий
		writer.settings(settings);
		// Ставим вложенное значение очередным полем заголовка ПЕРВЫМ
		tree.place(string("/header/") + name + "/nested") = abc::value_t(string("value"));
		// Если очередным полем заголовка дата сообщения не является
		if(::strcmp(name, "timestamp") != 0)
			// Ставим дату сообщения в дерево события
			tree.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
		// Выполняем проверку отказа сборки записи вложенным полем заголовка
		EXPECT_FALSE(writer.write(tree, result)) << name;
	}
}

/**
 * @brief Проверка сборки текста сообщения и блоков данных
 *
 * @details Запись нынешнего описания несёт блоки структурированных данных позиционно, и
 *          отсутствие их выражается знаком «-», а не пропуском поля: пропуск сдвинул бы
 *          текст сообщения на место блоков. Пустой же текст сообщения выражается
 *          ПРОБЕЛОМ без знаков за ним - тем и отличается «сообщение пусто» от
 *          «сообщения нет вовсе»
 *
 * @note Вложенный текст сообщения при пропуске вложенных значений отвечается успехом
 *       БЕЗ сообщения вовсе, а при строгом обращении - отказом с кодом вложенного
 *       значения. Оба исхода намеренны, и выбор их - дело того, кто пишет
 *
 */
TEST(CodecSysLogWriter, MessageAndStructures) {
	// Настройки записи событий без знака конца строки
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Устанавливаем сборку записи нынешним описанием
	settings.standard = syslog::standard_t::RFC5424;
	// Объект записи событий
	syslog::writer_t writer;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Собранная запись системного журнала
	string result = "";
	// Дерево собираемого события без блоков структурированных данных
	abc::value_t one(abc::kind_t::MAP);
	// Ставим номер описания записи в дерево события
	one.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	one.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	// Ставим ПУСТОЙ текст сообщения в дерево события
	one.place("/message") = abc::value_t(string(""));
	// Выполняем проверку успешности сборки записи без блоков данных
	ASSERT_TRUE(writer.write(one, result));
	// Выполняем проверку того, что блоки данных выражены знаком отсутствия
	EXPECT_NE(result.find(" - "), string::npos) << "запись: " << result;
	// Выполняем проверку того, что пустой текст сообщения выражен пробелом за записью
	EXPECT_EQ(result.back(), ' ') << "запись: " << result;
	// Дерево собираемого события с вложенным текстом сообщения
	abc::value_t two(abc::kind_t::MAP);
	// Ставим номер описания записи в дерево события
	two.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	two.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	// Ставим вложенный текст сообщения в дерево события
	two.place("/message/nested") = abc::value_t(string("value"));
	// Выполняем проверку отказа сборки записи вложенным текстом сообщения
	EXPECT_FALSE(writer.write(two, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::NESTED_VALUE);
	// Устанавливаем пропуск вложенного значения вовсе
	settings.nested = syslog::nested_t::SKIP;
	// Устанавливаем изменённые настройки записи событий
	writer.settings(settings);
	// Выполняем проверку успешности сборки записи пропуском текста сообщения
	ASSERT_TRUE(writer.write(two, result));
	// Выполняем проверку того, что вложенный текст сообщения в запись не попал
	EXPECT_EQ(result.find("nested"), string::npos) << "запись: " << result;
	// Дерево собираемого события с непредставимым опознавателем блока
	abc::value_t three(abc::kind_t::MAP);
	// Ставим номер описания записи в дерево события
	three.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	three.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	// Ставим блок с опознавателем, пробел несущим, в дерево события
	three.place(string("/structures/") + "bad id" + "/a") = abc::value_t(string("b"));
	// Выполняем проверку успешности сборки записи пропуском непредставимого блока
	ASSERT_TRUE(writer.write(three, result));
	// Выполняем проверку того, что непредставимый блок в запись не попал
	EXPECT_EQ(result.find("bad id"), string::npos) << "запись: " << result;
}

/**
 * @brief Проверка краёв номера описания и пустых блоков структурированных данных
 *
 * @details Номер описания приходит из дерева отправителя и годным числом быть не обязан,
 *          а блоки данных бывают объявлены ПУСТЫМ отображением - не тем же, что
 *          отсутствие их вовсе. Обе ветви карта покрытия держала пустыми
 *
 */
TEST(CodecSysLogWriter, ModernVersionAndEmptyStructures) {
	// Настройки записи событий
	syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки
	settings.terminate = false;
	// Устанавливаем сборку записи нынешним описанием
	settings.standard = syslog::standard_t::RFC5424;
	// Объект записи событий
	syslog::writer_t writer;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Собранная запись системного журнала
	string result = "";
	// Дерево собираемого события с пустыми блоками структурированных данных
	abc::value_t empty(abc::kind_t::MAP);
	// Ставим номер описания записи в дерево события
	empty.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
	// Ставим дату сообщения в дерево события
	empty.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	// Ставим ПУСТОЕ отображение блоков структурированных данных
	empty.place("/structures") = abc::value_t(abc::kind_t::MAP);
	// Ставим текст сообщения в дерево события
	empty.place("/message") = abc::value_t(string("текст"));
	// Выполняем проверку успешности сборки записи пустыми блоками данных
	ASSERT_TRUE(writer.write(empty, result));
	/**
	 * @note Текст сообщения RFC 5424 предваряется меткой порядка байтов, оттого знак
	 *       отсутствия блоков данных стоит перед нею, а не вплотную к тексту
	 */
	// Выполняем проверку того, что пустые блоки данных выражены знаком отсутствия
	EXPECT_NE(result.find(" - - - - - "), string::npos) << "запись: " << result;
	// Выполняем проверку того, что текст сообщения записью завершается
	EXPECT_EQ(result.compare(result.size() - ::strlen("текст"), ::strlen("текст"), "текст"), 0) << "запись: " << result;
	// Дерево собираемого события с ошибочным номером описания
	abc::value_t broken(abc::kind_t::MAP);
	// Ставим ошибочный номер описания записи в дерево события
	broken.place("/header/version") = abc::value_t(string("один"));
	// Ставим дату сообщения в дерево события
	broken.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
	// Выполняем проверку отказа сборки записи ошибочным номером описания
	EXPECT_FALSE(writer.write(broken, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::INVALID_VERSION);
	// Ставим номер описания записи, поддержке не подлежащий
	broken.place("/header/version") = abc::value_t(static_cast <uint64_t> (9));
	// Выполняем проверку отказа сборки записи неподдерживаемым номером описания
	EXPECT_FALSE(writer.write(broken, result));
	// Выполняем проверку кода отказа сборки записи
	EXPECT_EQ(writer.error(), syslog::error_t::UNSUPPORTED_VERSION);
}

/**
 * @brief Проверка отказа записи текстом сообщения, запись разрывающим
 *
 * @details Запись оканчивается переводом строки, и он же ей границею служит. Текст
 *          сообщения, перевод строки несущий, разрывал запись надвое: писатель отвечал
 *          УСПЕХОМ, а оборот битой записи отвечал INCOMPLETE_HEADER и возвращал
 *          сообщение из восьми байтов тремя
 *
 * @note Возврат каретки В КОНЦЕ текста проверяется здесь же, и беда его хуже: разбор
 *       снимает его как часть границы записи, оборот отказа НЕ даёт вовсе, а сообщение
 *       возвращается короче записанного на один байт - утрата не опознаётся ничем.
 *       Внутри текста возврат каретки допустим: границы он там не образует
 *
 */
TEST(CodecSysLogWriter, MessageBreakingTheRecord) {
	// Настройки записи событий
	syslog::writer_t::settings_t settings;
	// Устанавливаем сборку записи нынешним описанием
	settings.standard = syslog::standard_t::RFC5424;
	// Собранная запись системного журнала
	string result = "";
	/**
	 * @brief Метод сборки дерева события с заданным текстом сообщения
	 *
	 * @param message текст сообщения события
	 * @return        дерево собираемого события
	 */
	const auto tree = [](const string & message) noexcept -> abc::value_t {
		// Дерево собираемого события
		abc::value_t value(abc::kind_t::MAP);
		// Ставим приоритет записи в дерево события
		value.place("/priority") = abc::value_t(static_cast <uint64_t> (13));
		// Ставим номер описания записи в дерево события
		value.place("/header/version") = abc::value_t(static_cast <uint64_t> (1));
		// Ставим дату сообщения в дерево события
		value.place("/header/timestamp") = abc::value_t(string("2023-04-11T23:29:33Z"));
		// Ставим имя узла отправителя в дерево события
		value.place("/header/hostname") = abc::value_t(string("host"));
		// Ставим текст сообщения в дерево события
		value.place("/message") = abc::value_t(message);
		// Выводим дерево собираемого события
		return value;
	};
	// Выполняем проверку успешности сборки записи годным текстом сообщения
	EXPECT_TRUE(::build(tree("Message"), settings, result));
	// Выполняем проверку отказа сборки записи текстом с переводом строки
	EXPECT_FALSE(::build(tree("Mes\nsage"), settings, result));
	// Выполняем проверку отказа сборки записи текстом с возвратом каретки в конце
	EXPECT_FALSE(::build(tree("Message\r"), settings, result));
	// Выполняем проверку успешности сборки записи возвратом каретки ВНУТРИ текста
	EXPECT_TRUE(::build(tree("Mes\rsage"), settings, result));
}

/**
 * @brief Проверка оборота записи с повтором имени поля блока
 *
 * @details Описание дозволяет повтор имени поля внутри блока прямо: «An SD-PARAM MAY be
 *          repeated multiple times inside an SD-ELEMENT» (RFC 5424, раздел 6.3.3).
 *          Разбор повтор сохраняет перечнем, а запись его выражает ПОВТОРОМ имени - по
 *          значению на каждое звено перечня
 *
 * @warning Прежде дело стояло надвое: разбор повтор терял, второе значение затирало
 *          первое; а когда разбор его сохранять научился, запись перечень выразить не
 *          могла и отвечала NESTED_VALUE - оборот записи с повтором давал ПУСТО. Обе
 *          половины закрыты аудитом 14.09.2026
 *
 */
TEST(CodecSysLogWriter, RepeatedParamRoundTrip) {
	// Разбираемая запись с повтором имени поля блока
	const string text = "<34>1 2003-10-11T22:14:15Z host app - - [a@1 k=\"one\" k=\"two\"] text";
	// Объект события syslog
	syslog::document_t document;
	// Выполняем проверку успешности разбора записи с повтором имени поля
	ASSERT_TRUE(document.parse(text));
	// Выполняем проверку того, что повтор имени поля обратился в перечень
	ASSERT_EQ(document.keys("/structures/a@1/k").size(), static_cast <size_t> (2));
	// Выполняем сборку записи обратно из дерева события
	const string rewritten = document.dump();
	// Выполняем проверку того, что собранная запись повтор имени поля несёт
	EXPECT_NE(rewritten.find("k=\"one\" k=\"two\""), string::npos) << rewritten;
	// Объект события повторного разбора
	syslog::document_t again;
	// Выполняем проверку успешности повторного разбора собранной записи
	ASSERT_TRUE(again.parse(rewritten));
	// Выполняем проверку того, что перечень значений оборот пережил
	ASSERT_EQ(again.keys("/structures/a@1/k").size(), static_cast <size_t> (2));
	// Выполняем проверку того, что первое значение поля оборот пережило
	EXPECT_EQ(again.at("/structures/a@1/k/0").text(), string("one"));
	// Выполняем проверку того, что второе значение поля оборот пережило
	EXPECT_EQ(again.at("/structures/a@1/k/1").text(), string("two"));
}
