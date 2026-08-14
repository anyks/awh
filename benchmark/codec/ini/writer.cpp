/**
 * @file writer.cpp
 * @date 2026-08-10
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
 * @brief Бенчмарки записи текста настроек INI — сборка файла настроек службы,
 *        стоимость ограждения значений кавычками и управляющими последовательностями
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков контейнера INI
 */
#include "ini.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера INI
 */
using namespace awh::benchmark::config;

/**
 * @brief Внутренние параметры и сценарии бенчмарков записи текста настроек
 *
 */
namespace {
	/**
	 * @brief Количество собираемых файлов настроек
	 *
	 */
	static constexpr size_t WRITE_ROUNDS = 20000;
	/**
	 * @brief Количество свойств собираемого крупного раздела
	 *
	 */
	static constexpr size_t BULK_KEYS = 2000;

	/**
	 * @brief Пороги пропускной способности записи в мегабайтах в секунду
	 *
	 * @details Пороги назначены по замеру на отладочных стендах 10.08.2026: берётся
	 *          наименьший из снятых там показателей и делится надвое - запас на
	 *          разброс между прогонами. Самым медленным стендом оказался OpenBSD
	 *          amd64; прогнаны также FreeBSD amd64, NetBSD amd64, Fedora amd64 и
	 *          Fedora ARM64
	 *
	 */
	static constexpr double WRITE_SERVICE_THRESHOLD = 30.0;
	/**
	 * @brief Порог пропускной способности записи значений, требующих ограждения
	 *
	 * @details Сценарий этот стережёт устройство ограждения значений: нужда в
	 *          кавычках определяется одним проходом по значению, и повторный проход
	 *          на каждый его знак уронил бы показатель вдвое
	 *
	 */
	static constexpr double WRITE_ESCAPED_THRESHOLD = 50.0;

	/**
	 * @brief Функция получения перечня записываемых значений свойств
	 *
	 * @note Значения подобраны требующими ограждения: пробельная обвязка, знак
	 *       примечания, кавычка и знак конца строки - каждое из них ведёт запись
	 *       по своему пути
	 *
	 * @return перечень записываемых значений свойств
	 *
	 */
	static const vector <string> & values() noexcept {
		// Собираемый перечень записываемых значений свойств
		static const vector <string> result = {
			"  обвязка пробелами  ", "значение;с;точками", "значение \"в кавычках\"",
			"первая строка\nвторая строка", "путь\\к\\файлу", "обычное значение"
		};
		// Выводим перечень записываемых значений свойств
		return result;
	}
	/**
	 * @brief Функция сборки файла настроек службы
	 *
	 * @return размер собранного текста настроек
	 *
	 */
	static uint64_t write() noexcept {
		// Объект записи текста настроек
		awh::codec::ini::writer_t writer;
		// Выполняем запись примечания
		writer.comment("собрано приложением");
		// Выполняем запись объявления раздела
		writer.section("server");
		// Выполняем запись свойства с обозначением узла
		writer.property("host", "127.0.0.1");
		// Выполняем запись свойства с номером порта
		writer.number("port", 8080);
		// Выполняем запись свойства с количеством обработчиков
		writer.number("workers", 4);
		// Выполняем запись свойства с задержкой ожидания
		writer.number("timeout", 30);
		// Выполняем запись объявления раздела
		writer.section("paths");
		// Выполняем запись свойства с корневым путём
		writer.property("root", "/opt/awh");
		// Выполняем запись свойства с путём записей
		writer.property("logs", "/var/log/awh");
		// Выполняем запись свойства с путём хранилища
		writer.property("cache", "/var/cache/awh");
		// Выполняем запись объявления раздела
		writer.section("logging");
		// Выполняем запись свойства с уровнем подробности
		writer.property("level", "debug");
		// Выполняем запись свойства с признаком защищённого соединения
		writer.number("secure", true);
		// Выводим размер собранного текста настроек
		return static_cast <uint64_t> (writer.text().size());
	}
	/**
	 * @brief Функция сборки раздела со значениями, требующими ограждения
	 *
	 * @return размер собранного текста настроек
	 *
	 */
	static uint64_t escaped() noexcept {
		// Получаем настройки записи наречия настроек Git
		const awh::codec::ini::writer_t::settings_t settings = awh::codec::ini::writer_t::settings_t::git();
		// Объект записи текста настроек
		awh::codec::ini::writer_t writer(settings);
		// Получаем перечень записываемых значений свойств
		const vector <string> & values = ::values();
		// Выполняем запись объявления раздела
		writer.section("bulk");
		/**
		 * Выполняем запись свойств собираемого раздела
		 */
		for(size_t i = 0; i < BULK_KEYS; i++)
			// Выполняем запись очередного свойства раздела
			writer.property("key", values.at(i % values.size()));
		// Выводим размер собранного текста настроек
		return static_cast <uint64_t> (writer.text().size());
	}
	/**
	 * @brief Функция прогона сценария сборки файла настроек службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем размер собираемого текста настроек
		const size_t bytes = static_cast <size_t> (::write());
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, WRITE_ROUNDS, []() noexcept {
			// Выполняем сборку файла настроек службы
			return ::write();
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи значений, требующих ограждения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeEscaped() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем размер собираемого текста настроек
		const size_t bytes = static_cast <size_t> (::escaped());
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, (WRITE_ROUNDS / 100), []() noexcept {
			// Выполняем сборку раздела со значениями, требующими ограждения
			return ::escaped();
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки файла настроек службы
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/ini: запись настроек службы", "МБ/с", WRITE_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeService
	);
	/**
	 * Выполняем регистрацию сценария записи значений, требующих ограждения
	 */
	static const bool ESCAPED_REGISTERED = awh::benchmark::add(
		"codec/ini: запись ограждаемых значений", "МБ/с", WRITE_ESCAPED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeEscaped
	);
};
