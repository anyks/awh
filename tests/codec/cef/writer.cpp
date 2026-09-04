/**
 * @file writer.cpp
 * @date 2026-09-04
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
 * @brief Автоматические тесты записи событий в запись CEF — постановки отмены знаков порознь
 *        по областям, сборки заголовка и расширения, обращения с вложенным значением и
 *        записи повторяющегося ключа перечнем
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
#include <codec/cef/cef.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной программою
 *
 */
namespace {
	/**
	 * @brief Объект окружения проверок записи событий
	 *
	 */
	struct EnvCefWriter {
		// Объект фреймворка проверок
		awh::fmk_t fmk;
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		EnvCefWriter() noexcept : log(&this->fmk) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта окружения проверок
	 *
	 * @return объект окружения проверок
	 *
	 */
	EnvCefWriter & writerEnvironment() noexcept {
		// Объект окружения проверок
		static EnvCefWriter env;
		// Выводим объект окружения проверок
		return env;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Метод сборки записи CEF из разобранной записи
 *
 * @param text     разбираемая запись CEF
 * @param settings настройки записи событий
 * @return         собранная заново запись CEF
 *
 */
static string rewriteCef(const string & text, const cef::writer_t::settings_t & settings = cef::writer_t::settings_t()) noexcept {
	// Объект события CEF
	cef::document_t doc(&::writerEnvironment().fmk, &::writerEnvironment().log);
	// Устанавливаем настройки записи событий
	doc.settings(settings);
	// Если разбор записи отказом завершился
	if(!doc.parse(text))
		// Выводим пустую запись
		return "";
	// Выводим собранную заново запись CEF
	return doc.dump();
}

/**
 * @brief Проверка постановки отмены знаков порознь по областям записи
 *
 * @details Описание ArcSight требует отменять в заголовке прямую черту и обратную
 *          косую, а в расширении - знак равенства, и прямо оговаривает, что черта в
 *          расширении отмены не требует
 *
 */
TEST(CodecCefWriter, Escaping) {
	// Выполняем проверку постановки отмены прямой черты в заголовке
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|security|threatmanager|1.0|100|detected a \| in message|10|act=blocked a |)"),
		"CEF:0|security|threatmanager|1.0|100|detected a \\| in message|10|act=blocked a |\n"
	);
	// Выполняем проверку постановки отмены обратной косой в заголовке
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|security|tm|1.0|100|detected a \\ in packet|10|action=blocked a \)"),
		"CEF:0|security|tm|1.0|100|detected a \\\\ in packet|10|action=blocked a \\\\\n"
	);
	// Выполняем проверку постановки отмены знака равенства в расширении
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|A|B|C|D|E|1|originsicname=CN\=chr-cpsg-01)"),
		"CEF:0|A|B|C|D|E|1|originsicname=CN\\=chr-cpsg-01\n"
	);
	// Выполняем проверку постановки перевода строки отменяющей последовательностью
	EXPECT_EQ(
		::rewriteCef(R"(CEF:0|A|B|C|D|E|1|msg=Detected a threat.\nNo action needed.)"),
		"CEF:0|A|B|C|D|E|1|msg=Detected a threat.\\nNo action needed.\n"
	);
}

/**
 * @brief Проверка сборки записи с приставкой syslog
 *
 */
TEST(CodecCefWriter, Syslog) {
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Выполняем проверку сборки записи с приставкой syslog
	EXPECT_EQ(
		::rewriteCef("Feb 17 15:30:15 host CEF:0|A|B|C|D|E|1|src=1.2.3.4", settings),
		"Feb 17 15:30:15 host CEF:0|A|B|C|D|E|1|src=1.2.3.4\n"
	);
	// Отключаем запись приставки syslog перед словом «CEF:»
	settings.syslog = false;
	// Выполняем проверку сборки записи без приставки syslog
	EXPECT_EQ(
		::rewriteCef("Feb 17 15:30:15 host CEF:0|A|B|C|D|E|1|src=1.2.3.4", settings),
		"CEF:0|A|B|C|D|E|1|src=1.2.3.4\n"
	);
	// Отключаем запись знака конца строки за записью
	settings.terminate = false;
	// Выполняем проверку сборки записи без знака конца строки
	EXPECT_EQ(
		::rewriteCef("CEF:0|A|B|C|D|E|1|src=1.2.3.4", settings),
		"CEF:0|A|B|C|D|E|1|src=1.2.3.4"
	);
}

/**
 * @brief Проверка записи повторяющегося ключа расширения
 *
 * @details Повтор ключа держится в дереве перечнем, а записывается вновь всяким своим
 *          появлением: обратное обратило бы перечень в одно значение и потеряло бы
 *          повтор молча
 *
 */
TEST(CodecCefWriter, DuplicateKeys) {
	// Выполняем проверку записи повторяющегося ключа расширения
	EXPECT_EQ(
		::rewriteCef("CEF:0|A|B|C|D|E|1|ad.x=1 ad.x=2 ad.x=3"),
		"CEF:0|A|B|C|D|E|1|ad.x=1 ad.x=2 ad.x=3\n"
	);
}

/**
 * @brief Проверка обращения с вложенным значением
 *
 * @details Дерева произвольной глубины запись CEF не несёт: исход выбирает тот, кто
 *          пишет, а не кодек
 *
 */
TEST(CodecCefWriter, Nested) {
	// Объект записи событий
	cef::writer_t writer(&::writerEnvironment().fmk, &::writerEnvironment().log);
	// Настройки записи событий
	cef::writer_t::settings_t settings;
	// Заводим дерево события отображением
	abc::value_t root(abc::kind_t::MAP);
	// Заводим поля заголовка записи отображением
	root.place("/header") = abc::value_t(abc::kind_t::MAP);
	// Ставим поле заголовка поставщика устройства
	root.place("/header/vendor") = abc::value_t(string("A"));
	// Ставим поле заголовка изделия поставщика
	root.place("/header/product") = abc::value_t(string("B"));
	// Ставим поле заголовка редакции изделия
	root.place("/header/release") = abc::value_t(string("C"));
	// Ставим поле заголовка опознавателя события
	root.place("/header/signature") = abc::value_t(string("D"));
	// Ставим поле заголовка имени события
	root.place("/header/name") = abc::value_t(string("E"));
	// Ставим поле заголовка важности события
	root.place("/header/severity") = abc::value_t(static_cast <int64_t> (1));
	// Заводим пары расширения записи отображением
	root.place("/extension") = abc::value_t(abc::kind_t::MAP);
	// Ставим пару расширения с простым значением
	root.place("/extension/src") = abc::value_t(string("1.2.3.4"));
	// Ставим пару расширения со значением вложенным
	root.place("/extension/nested") = abc::value_t(abc::kind_t::MAP);
	// Собираемая запись CEF
	string result;
	// Устанавливаем отказ на вложенное значение
	settings.nested = cef::nested_t::STRICT;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку отказа записи на вложенном значении
	EXPECT_FALSE(writer.write(root, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::NESTED_VALUE);
	// Устанавливаем пропуск вложенного значения вовсе
	settings.nested = cef::nested_t::SKIP;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Выполняем проверку записи с пропуском вложенного значения
	EXPECT_TRUE(writer.write(root, result));
	// Выполняем проверку того, что вложенное значение в запись не попало
	EXPECT_EQ(result, "CEF:0|A|B|C|D|E|1|src=1.2.3.4\n");
}

/**
 * @brief Проверка отклонения дерева неверного устройства
 *
 */
TEST(CodecCefWriter, Failures) {
	// Объект записи событий
	cef::writer_t writer(&::writerEnvironment().fmk, &::writerEnvironment().log);
	// Собираемая запись CEF
	string result;
	// Заводим дерево события последовательностью знаков
	const abc::value_t plain(string("это не событие"));
	// Выполняем проверку отказа записи на дереве неверного устройства
	EXPECT_FALSE(writer.write(plain, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::UNREPRESENTABLE_VALUE);
	// Заводим дерево события отображением без заголовка
	abc::value_t empty(abc::kind_t::MAP);
	// Выполняем проверку отказа записи на дереве без заголовка
	EXPECT_FALSE(writer.write(empty, result));
	// Выполняем проверку кода отказа записи
	EXPECT_EQ(writer.error(), cef::error_t::INCOMPLETE_HEADER);
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
