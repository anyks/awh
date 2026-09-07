/**
 * @file dictionary.cpp
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
 * @brief Автоматические тесты словаря источников сообщений и степеней их важности —
 *        упорядоченности таблиц, замкнутости розыска и имён, наравне принятых
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <set>
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
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Проверка упорядоченности таблиц словаря по числовому коду
 *
 * @details Упорядоченность обязательна: розыск по коду ведётся прямым обращением по
 *          указателю, и нарушение порядка обратило бы его в молчаливую выдачу ЧУЖОЙ
 *          записи - отказа при том не было бы вовсе
 *
 */
TEST(CodecSysLogDictionary, Ordering) {
	// Выполняем проверку полноты словаря источников сообщений
	EXPECT_EQ(syslog::facilities::size(), 24u);
	/**
	 * Выполняем перебор всех записей словаря источников сообщений
	 */
	for(size_t i = 0; i < syslog::facilities::size(); i++){
		// Получаем очередную запись словаря источников сообщений
		const syslog::entry_t * entry = syslog::facilities::at(static_cast <uint8_t> (i));
		// Выполняем проверку наличия записи словаря
		ASSERT_NE(entry, nullptr) << "код источника: " << i;
		// Выполняем проверку того, что код записи отвечает её месту в таблице
		EXPECT_EQ(static_cast <size_t> (entry->code), i);
	}
	// Выполняем проверку полноты словаря степеней важности сообщений
	EXPECT_EQ(syslog::severities::size(), 8u);
	/**
	 * Выполняем перебор всех записей словаря степеней важности сообщений
	 */
	for(size_t i = 0; i < syslog::severities::size(); i++){
		// Получаем очередную запись словаря степеней важности сообщений
		const syslog::entry_t * entry = syslog::severities::at(static_cast <uint8_t> (i));
		// Выполняем проверку наличия записи словаря
		ASSERT_NE(entry, nullptr) << "код важности: " << i;
		// Выполняем проверку того, что код записи отвечает её месту в таблице
		EXPECT_EQ(static_cast <size_t> (entry->code), i);
	}
}

/**
 * @brief Проверка замкнутости розыска по коду и по имени
 *
 * @details Розыск размыкается молча: имя, таблицей выданное, обязано находиться в ней
 *          же. Проверка эта ловит опечатку в имени, какую никакая иная не заметит
 *
 */
TEST(CodecSysLogDictionary, Closure) {
	/**
	 * Выполняем перебор всех записей словаря источников сообщений
	 */
	for(size_t i = 0; i < syslog::facilities::size(); i++){
		// Получаем очередную запись словаря источников сообщений
		const syslog::entry_t * entry = syslog::facilities::at(static_cast <uint8_t> (i));
		// Выполняем проверку наличия записи словаря
		ASSERT_NE(entry, nullptr);
		// Выполняем розыск той же записи по имени
		const syslog::entry_t * found = syslog::facilities::find(entry->name);
		// Выполняем проверку замкнутости розыска источника сообщения
		ASSERT_NE(found, nullptr) << "имя источника: " << string(entry->name);
		// Выполняем проверку того, что розыск нашёл ту же самую запись
		EXPECT_EQ(found->code, entry->code);
		// Выполняем проверку непустоты человеческого названия
		EXPECT_FALSE(entry->title.empty()) << "имя источника: " << string(entry->name);
	}
	/**
	 * Выполняем перебор всех записей словаря степеней важности сообщений
	 */
	for(size_t i = 0; i < syslog::severities::size(); i++){
		// Получаем очередную запись словаря степеней важности сообщений
		const syslog::entry_t * entry = syslog::severities::at(static_cast <uint8_t> (i));
		// Выполняем проверку наличия записи словаря
		ASSERT_NE(entry, nullptr);
		// Выполняем розыск той же записи по имени
		const syslog::entry_t * found = syslog::severities::find(entry->name);
		// Выполняем проверку замкнутости розыска степени важности сообщения
		ASSERT_NE(found, nullptr) << "имя важности: " << string(entry->name);
		// Выполняем проверку того, что розыск нашёл ту же самую запись
		EXPECT_EQ(found->code, entry->code);
		// Выполняем проверку непустоты человеческого названия
		EXPECT_FALSE(entry->title.empty()) << "имя важности: " << string(entry->name);
	}
}

/**
 * @brief Проверка согласия словаря с ходами выдачи имени
 *
 * @details Имена держатся словарём и ТОЛЬКО им: проверка эта закрепляет, что ход
 *          `name` берёт их оттуда же, а не из второго списка, молча расходящегося
 *
 */
TEST(CodecSysLogDictionary, Agreement) {
	/**
	 * Выполняем перебор всех источников сообщений
	 */
	for(size_t i = 0; i < syslog::facilities::size(); i++){
		// Получаем очередную запись словаря источников сообщений
		const syslog::entry_t * entry = syslog::facilities::at(static_cast <uint8_t> (i));
		// Выполняем проверку наличия записи словаря
		ASSERT_NE(entry, nullptr);
		// Выполняем проверку согласия словаря с ходом выдачи имени источника
		EXPECT_EQ(string(syslog::name(static_cast <syslog::facility_t> (i))), string(entry->name));
	}
	/**
	 * Выполняем перебор всех степеней важности сообщений
	 */
	for(size_t i = 0; i < syslog::severities::size(); i++){
		// Получаем очередную запись словаря степеней важности сообщений
		const syslog::entry_t * entry = syslog::severities::at(static_cast <uint8_t> (i));
		// Выполняем проверку наличия записи словаря
		ASSERT_NE(entry, nullptr);
		// Выполняем проверку согласия словаря с ходом выдачи имени важности
		EXPECT_EQ(string(syslog::name(static_cast <syslog::severity_t> (i))), string(entry->name));
	}
}

/**
 * @brief Проверка розыска без разбора величины букв
 *
 * @details Службы журналов пишут имена и строчными, и прописными: правило отбора,
 *          написанное «LOCAL0», означает то же, что и «local0»
 *
 */
TEST(CodecSysLogDictionary, CaseInsensitive) {
	// Выполняем проверку розыска источника сообщения прописными буквами
	const syslog::entry_t * entry = syslog::facilities::find("LOCAL0");
	// Выполняем проверку наличия найденной записи словаря
	ASSERT_NE(entry, nullptr);
	// Выполняем проверку кода найденного источника сообщения
	EXPECT_EQ(entry->code, static_cast <uint8_t> (syslog::facility_t::LOCAL0));
	// Выполняем проверку розыска степени важности буквами вперемешку
	entry = syslog::severities::find("WaRnInG");
	// Выполняем проверку наличия найденной записи словаря
	ASSERT_NE(entry, nullptr);
	// Выполняем проверку кода найденной степени важности сообщения
	EXPECT_EQ(entry->code, static_cast <uint8_t> (syslog::severity_t::WARNING));
}

/**
 * @brief Проверка имён, службами журналов принятых наравне с основными
 *
 * @details Имена эти назначены не описанием, а службами журналов, и правила отбора
 *          пишутся ими наравне с основными: отвергать их значило бы отвергать
 *          настройки, которыми живые службы пользуются повседневно
 *
 */
TEST(CodecSysLogDictionary, Aliases) {
	// Выполняем розыск степени важности именем, наравне принятым
	const syslog::entry_t * entry = syslog::severities::find("error");
	// Выполняем проверку наличия найденной записи словаря
	ASSERT_NE(entry, nullptr);
	// Выполняем проверку кода найденной степени важности сообщения
	EXPECT_EQ(entry->code, static_cast <uint8_t> (syslog::severity_t::ERROR));
	// Выполняем розыск предостережения именем, наравне принятым
	entry = syslog::severities::find("warn");
	// Выполняем проверку наличия найденной записи словаря
	ASSERT_NE(entry, nullptr);
	// Выполняем проверку кода найденной степени важности сообщения
	EXPECT_EQ(entry->code, static_cast <uint8_t> (syslog::severity_t::WARNING));
	// Выполняем розыск наивысшей важности именем, наравне принятым
	entry = syslog::severities::find("panic");
	// Выполняем проверку наличия найденной записи словаря
	ASSERT_NE(entry, nullptr);
	// Выполняем проверку кода найденной степени важности сообщения
	EXPECT_EQ(entry->code, static_cast <uint8_t> (syslog::severity_t::EMERGENCY));
	/**
	 * Выполняем проверку того, что имя, наравне принятое, выдаётся ОСНОВНЫМ
	 *
	 * @note Иначе оборот менял бы имя: запись, прочтённая с «error», писалась бы
	 *       обратно с «error», а не с «err», и два вида одного и того же расходились бы
	 */
	EXPECT_EQ(string(entry->name), "emerg");
}

/**
 * @brief Проверка выдачи отсутствия неизвестным кодам и именам
 *
 */
TEST(CodecSysLogDictionary, Unknown) {
	// Выполняем проверку выдачи отсутствия коду источника, за предел выходящему
	EXPECT_EQ(syslog::facilities::at(24), nullptr);
	// Выполняем проверку выдачи отсутствия коду важности, за предел выходящему
	EXPECT_EQ(syslog::severities::at(8), nullptr);
	// Выполняем проверку выдачи отсутствия имени источника, словарю неведомому
	EXPECT_EQ(syslog::facilities::find("nosuchfacility"), nullptr);
	// Выполняем проверку выдачи отсутствия имени важности, словарю неведомому
	EXPECT_EQ(syslog::severities::find("nosuchseverity"), nullptr);
	// Выполняем проверку выдачи отсутствия пустому имени
	EXPECT_EQ(syslog::facilities::find(""), nullptr);
	/**
	 * Выполняем проверку того, что имя источника не находится в словаре важностей
	 *
	 * @note Словари РАЗНЫЕ, и смешение их дало бы приоритет, отправителем не
	 *       объявленный: «kern» есть источник, но не важность
	 */
	EXPECT_EQ(syslog::severities::find("kern"), nullptr);
}
