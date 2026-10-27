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
 * @brief Реализация словаря источников сообщений и степеней их важности — розыска записи по
 *        числовому коду и по краткому имени, службами журналов употребляемому
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл средств ASCII
 *
 * @note Сличение имён словаря ведётся ими, а не своим приведением величины букв:
 *       заводить рамке замену незачем, а правила местности протокольным именам чужды
 *
 */
#include <encoding/ascii.hpp>

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include <sys/macro/suppress.hpp>

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/syslog/dictionary.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;
	/**
	 * Пространство имён контейнера SysLog
	 */
	using namespace awh::codec::syslog;

	/**
	 * @brief Таблица имён, службами журналов принятых наравне с основными
	 *
	 * @details Имена эти назначены не описанием, а службами журналов, и правила отбора
	 *          пишутся ими наравне с основными: `error` вместо `err`, `warn` вместо
	 *          `warning`, `panic` вместо `emerg`. Отвергать их значило бы отвергать
	 *          настройки, которыми живые службы пользуются повседневно.
	 *
	 */
	constexpr struct {
		// Числовой код степени важности сообщения
		uint8_t code;
		// Имя, принятое наравне с основным
		string_view name;
	} ALIASES[] = {
		{0x00, "panic"},
		{0x03, "error"},
		{0x04, "warn"}
	};
	/**
	 * @brief Таблица степеней важности сообщений, ПО КОДУ упорядоченная
	 *
	 * @details Упорядоченность обязательна тем же доводом, каким обязательна она у
	 *          таблицы источников сообщений.
	 *
	 */
	constexpr entry_t SEVERITIES[] = {
		{0x00, "emerg", "The system is unusable"},
		{0x01, "alert", "Intervention required immediately"},
		{0x02, "crit", "The condition is serious"},
		{0x03, "err", "Failure Condition"},
		{0x04, "warning", "Caution Condition"},
		{0x05, "notice", "The condition is normal, but worthy of attention"},
		{0x06, "info", "Informational message"},
		{0x07, "debug", "Debug message"}
	};
	/**
	 * @brief Таблица источников сообщений, ПО КОДУ упорядоченная
	 *
	 * @details Упорядоченность обязательна: розыск по коду ведётся прямым обращением по
	 *          указателю, и нарушение порядка обратило бы его в молчаливую выдачу чужой
	 *          записи. Порядок закреплён проверочным испытанием, а не оставлен на
	 *          усмотрение правящего таблицу.
	 *
	 * @note Имена взяты из RFC 5424, таблица 1, и совпадают с принятыми у служб журналов:
	 *       по ним настраиваются правила отбора, и расхождение сделало бы правила
	 *       несовместимыми
	 *
	 */
	constexpr entry_t FACILITIES[] = {
		{0x00, "kern", "System kernel messages"},
		{0x01, "user", "User Level Messages"},
		{0x02, "mail", "Postal service"},
		{0x03, "daemon", "System services"},
		{0x04, "auth", "Identification and security"},
		{0x05, "syslog", "Messages from the log service itself"},
		{0x06, "lpr", "Print service"},
		{0x07, "news", "News service"},
		{0x08, "uucp", "UUCP service"},
		{0x09, "cron", "Clock service"},
		{0x0A, "authpriv", "Identification and security, second set"},
		{0x0B, "ftp", "File Transfer Service"},
		{0x0C, "ntp", "Network Time Service"},
		{0x0D, "audit", "Log Surveillance Records"},
		{0x0E, "alert", "Log monitoring alarms"},
		{0x0F, "cron2", "Clock service, second set"},
		{0x10, "local0", "Local use kit No. 0"},
		{0x11, "local1", "Local use kit No. 1"},
		{0x12, "local2", "Local use kit No. 2"},
		{0x13, "local3", "Local use kit No. 3"},
		{0x14, "local4", "Local use kit No. 4"},
		{0x15, "local5", "Local use kit No. 5"},
		{0x16, "local6", "Local use kit No. 6"},
		{0x17, "local7", "Local use kit No. 7"}
	};

	/**
	 * @brief Функция сличения имён без разбора величины букв
	 *
	 * @details Сличение ведётся по знакам ASCII, а не по правилам местности - имена эти
	 *          протокольные, и правила местности обратили бы «I» турецкой раскладки в
	 *          чужую букву.
	 *
	 * @note Приведение знака к строчному виду взято у рамки - `awh::ascii::equals`, - а
	 *       не написано здесь заново: своё приведение было тем же самым до знака, и
	 *       держать вторую его запись значило бы заводить рамке замену. Правлено
	 *       09.09.2026: незачем повторять то, что в рамке уже есть
	 *
	 * @note Словарь опрашивается ходами свободными - `facilities::find("LOCAL0")`, - и
	 *       объекта рамки под рукою тут нет вовсе, оттого берётся `awh::ascii`, ходы
	 *       какого свободны и от объекта не зависят
	 *
	 * @param one первое сличаемое имя
	 * @param two второе сличаемое имя
	 * @return    признак совпадения имён
	 *
	 */
	static bool compare(const string_view one, const string_view two) noexcept {
		// Если длины имён разнятся
		if(one.size() != two.size())
			// Выводим отрицательный признак совпадения имён
			return false;
		/**
		 * Выполняем перебор всех знаков сличаемых имён
		 */
		for(size_t i = 0; i < one.size(); i++){
			// Если знаки имён разнятся видом, к величине букв безразличным
			if(!ascii::equals(one[i], two[i]))
				// Выводим отрицательный признак совпадения имён
				return false;
		}
		// Выводим положительный признак совпадения имён
		return true;
	}
};

/**
 * @brief Функция розыска источника сообщения по числовому коду
 *
 * @param code числовой код источника сообщения
 * @return     запись словаря либо nullptr, если кода в словаре нет
 *
 */
const awh::codec::syslog::entry_t * awh::codec::syslog::facilities::at(const uint8_t code) noexcept {
	// Если числовой код источника сообщения за предел выходит
	if(code >= (sizeof(FACILITIES) / sizeof(FACILITIES[0])))
		// Выводим отсутствие записи словаря
		return nullptr;
	// Выводим запись словаря прямым обращением: таблица по коду упорядочена
	return &FACILITIES[code];
}
/**
 * @brief Функция розыска источника сообщения по краткому имени
 *
 * @param name краткое имя источника сообщения
 * @return     запись словаря либо nullptr, если имени в словаре нет
 *
 */
const awh::codec::syslog::entry_t * awh::codec::syslog::facilities::find(const string_view name) noexcept {
	/**
	 * Выполняем перебор всех записей словаря источников сообщений
	 *
	 * @note Розыск ведётся перебором, а не двоичным поиском: записей два десятка, и
	 *       упорядочение таблицы по имени отняло бы у розыска по коду прямое обращение
	 *       по указателю - обмен невыгодный
	 *
	 */
	for(auto & entry : FACILITIES){
		// Если имя источника сообщения совпало
		if(::compare(entry.name, name))
			// Выводим найденную запись словаря
			return &entry;
	}
	// Выводим отсутствие записи словаря
	return nullptr;
}
/**
 * @brief Функция получения количества записей словаря
 *
 * @return количество записей словаря
 *
 */
size_t awh::codec::syslog::facilities::size() noexcept {
	// Выводим количество записей словаря источников сообщений
	return (sizeof(FACILITIES) / sizeof(FACILITIES[0]));
}
/**
 * @brief Функция розыска степени важности сообщения по числовому коду
 *
 * @param code числовой код степени важности сообщения
 * @return     запись словаря либо nullptr, если кода в словаре нет
 *
 */
const awh::codec::syslog::entry_t * awh::codec::syslog::severities::at(const uint8_t code) noexcept {
	// Если числовой код степени важности сообщения за предел выходит
	if(code >= (sizeof(SEVERITIES) / sizeof(SEVERITIES[0])))
		// Выводим отсутствие записи словаря
		return nullptr;
	// Выводим запись словаря прямым обращением: таблица по коду упорядочена
	return &SEVERITIES[code];
}
/**
 * @brief Функция розыска степени важности сообщения по краткому имени
 *
 * @param name краткое имя степени важности сообщения
 * @return     запись словаря либо nullptr, если имени в словаре нет
 *
 */
const awh::codec::syslog::entry_t * awh::codec::syslog::severities::find(const string_view name) noexcept {
	/**
	 * Выполняем перебор всех записей словаря степеней важности сообщений
	 */
	for(auto & entry : SEVERITIES){
		// Если имя степени важности сообщения совпало
		if(::compare(entry.name, name))
			// Выводим найденную запись словаря
			return &entry;
	}
	/**
	 * Выполняем перебор всех имён, принятых наравне с основными
	 */
	for(auto & alias : ALIASES){
		// Если имя, наравне принятое, совпало
		if(::compare(alias.name, name))
			// Выводим запись словаря, имени отвечающую
			return &SEVERITIES[alias.code];
	}
	// Выводим отсутствие записи словаря
	return nullptr;
}
/**
 * @brief Функция получения количества записей словаря
 *
 * @return количество записей словаря
 *
 */
size_t awh::codec::syslog::severities::size() noexcept {
	// Выводим количество записей словаря степеней важности сообщений
	return (sizeof(SEVERITIES) / sizeof(SEVERITIES[0]));
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
