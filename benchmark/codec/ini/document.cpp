/**
 * @file document.cpp
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
 * @brief Бенчмарки дерева настроек INI — сборка дерева, поиск значений по имени,
 *        стоимость множества разделов и обратная запись дерева в текст настроек
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
 * @brief Внутренние параметры и сценарии бенчмарков дерева настроек
 *
 */
namespace {
	/**
	 * @brief Количество собираемых мелких деревьев настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 10000;
	/**
	 * @brief Количество собираемых крупных деревьев настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 4;
	/**
	 * @brief Количество собираемых деревьев со множеством разделов
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 8;
	/**
	 * @brief Количество обращений к значениям при замере поиска
	 *
	 */
	static constexpr size_t LOOKUP_ROUNDS = 200000;

	/**
	 * @brief Пороги пропускной способности работы с деревом в мегабайтах в секунду
	 *
	 * @details Пороги назначены по замеру на отладочных стендах 10.08.2026: берётся
	 *          наименьший из снятых там показателей и делится надвое - запас на
	 *          разброс между прогонами. Самым медленным стендом оказался OpenBSD
	 *          amd64; прогнаны также FreeBSD amd64, NetBSD amd64, Fedora amd64 и
	 *          Fedora ARM64
	 *
	 */
	static constexpr double BUILD_SERVICE_THRESHOLD = 6.3;
	/**
	 * @brief Порог пропускной способности сборки крупного дерева настроек
	 *
	 */
	static constexpr double BUILD_LARGE_THRESHOLD = 8.6;
	/**
	 * @brief Порог пропускной способности сборки дерева со множеством разделов
	 *
	 * @details Сценарий этот стережёт устройство поиска разделов: раскладка их по
	 *          свёртке имени держит стоимость заведения раздела постоянной, а
	 *          попарное сличение имён обращает сборку дерева в квадратичную. На
	 *          четырёх мегабайтах, где разделов набирается под сотню тысяч, разница
	 *          между тем и другим - тысячекратная
	 *
	 */
	static constexpr double BUILD_SECTIONS_THRESHOLD = 4.2;
	/**
	 * @brief Порог пропускной способности обратной записи дерева в текст
	 *
	 */
	static constexpr double WRITE_BACK_THRESHOLD = 35.0;
	/**
	 * @brief Порог пропускной способности обхода дерева со множеством разделов
	 *
	 * @details Сценарий этот стережёт выдачу перечня имён свойств: перечень собран
	 *          указателями при перестроении, и обход раздела стоит числа его
	 *          свойств. Перебор же всех записей дерева ради каждого раздела
	 *          обращает обход в квадратичный - на сотне тысяч разделов это
	 *          полтораста раз медленнее, и порог такую подмену отличает
	 *
	 * @note Дефект этот был не выдуман, а пойман: стенд сравнения
	 *       `tools/benchmark/ini` показал 0.25 МБ/с там, где ныне 40. В настоящем
	 *       же сценарии квадратичный обход давал бы около одного мегабайта в секунду,
	 *       тогда как самый медленный стенд даёт тридцать два, - порог отличает одно
	 *       от другого с двойным запасом в обе стороны
	 *
	 */
	static constexpr double WALK_SECTIONS_THRESHOLD = 10.0;
	/**
	 * @brief Порог количества обращений к значениям в секунду
	 *
	 * @details Поиск значения ведётся раскладкой по свёртке имени раздела и имени
	 *          свойства: показатель этот стережёт её сохранность. Перебор записей
	 *          дерева при каждом обращении уронил бы его на порядки
	 *
	 */
	static constexpr double LOOKUP_THRESHOLD = 440000.0;
	/**
	 * @brief Количество свойств, заводимых при замере сборки вызовами
	 *
	 */
	static constexpr size_t ASSEMBLY_PROPERTIES = 20000;
	/**
	 * @brief Количество собираемых вызовами деревьев настроек
	 *
	 */
	static constexpr size_t ASSEMBLY_ROUNDS = 8;
	/**
	 * @brief Порог количества установок значения в секунду
	 *
	 * @details Сценарий этот стережёт приращение указателей поиска при заведении
	 *          свойства: запись встаёт в конец перечня, порядковые номера прочих
	 *          записей не сдвигаются, и перестраивать указатели целиком незачем.
	 *          Перестроение же на всякой установке обращает сборку дерева вызовами
	 *          в квадратичную
	 *
	 * @note Замерено, а не выдумано: перестроение указателей на всякой установке
	 *       давало 1 223 установки в секунду против нынешних 5 604 669 - на двадцати
	 *       тысячах свойств это в четыре с половиной тысячи раз медленнее и
	 *       четыреста миллионов выделений памяти на дерево против сорока тысяч.
	 *       Порог поставлен между: он вдвое ниже показателя самого медленного стенда
	 *       и в триста раз выше квадратичного пути
	 *
	 */
	static constexpr double ASSEMBLY_THRESHOLD = 400000.0;

	/**
	 * @brief Функция сборки дерева настроек
	 *
	 * @param text     разбираемый текст настроек
	 * @param settings настройки дерева настроек
	 * @return         количество объявленных разделов
	 *
	 */
	static uint64_t build(const string & text, const awh::codec::ini::document_t::settings_t & settings) noexcept {
		// Дерево настроек
		awh::codec::ini::document_t document;
		/**
		 * Если разбор текста настроек выполнить не удалось
		 */
		if(!document.parse(text, settings))
			// Выводим нулевое количество объявленных разделов
			return 0;
		// Выводим количество объявленных разделов
		return static_cast <uint64_t> (document.size());
	}
	/**
	 * @brief Функция получения собранного дерева настроек службы
	 *
	 * @note Дерево собирается однократно до замера: сценарии поиска и обратной
	 *       записи измеряют свою работу, а не стоимость разбора
	 *
	 * @return собранное дерево настроек
	 *
	 */
	static const awh::codec::ini::document_t & prepared() noexcept {
		// Собираемое дерево настроек
		static const awh::codec::ini::document_t result = []() noexcept -> awh::codec::ini::document_t {
			// Собираемое дерево настроек
			awh::codec::ini::document_t result;
			// Выполняем разбор эталонного текста настроек
			result.parse(service());
			// Выводим собранное дерево настроек
			return result;
		}();
		// Выводим собранное дерево настроек
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева настроек службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = service();
		// Настройки дерева настроек
		const awh::codec::ini::document_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем сборку дерева настроек
			return ::build(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки крупного дерева настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = large();
		// Настройки дерева настроек
		const awh::codec::ini::document_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем сборку дерева настроек
			return ::build(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева со множеством разделов
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildSections() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = sections();
		// Настройки дерева настроек
		const awh::codec::ini::document_t::settings_t settings;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), FOCUSED_ROUNDS, [&text, &settings]() noexcept {
			// Выполняем сборку дерева настроек
			return ::build(text, settings);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария обратной записи дерева в текст настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeBack() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем собранное дерево настроек
		const awh::codec::ini::document_t & document = ::prepared();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(service().size(), SMALL_ROUNDS, [&document]() noexcept {
			// Выполняем обратную запись дерева в текст настроек
			return static_cast <uint64_t> (document.text().size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария обхода дерева со множеством разделов
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t walkSections() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = sections();
		// Собираемое дерево настроек
		awh::codec::ini::document_t document;
		/**
		 * Если разбор текста настроек выполнить не удалось
		 */
		if(!document.parse(text))
			// Выводим нулевой результат измерения
			return result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), FOCUSED_ROUNDS, [&document]() noexcept {
			// Накопитель длин прочитанных значений
			uint64_t result = 0;
			/**
			 * Выполняем перебор всех объявленных разделов
			 */
			for(const awh::codec::ini::name_t & section : document.sections()){
				/**
				 * Выполняем перебор всех имён свойств раздела
				 */
				for(const string_view & key : document.keys(section.section, section.subsection))
					// Выполняем накопление длины значения очередного свойства
					result += document.get(key, section.section, section.subsection).size();
			}
			// Выводим накопитель длин прочитанных значений
			return result;
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария поиска значений по имени
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t lookup() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем собранное дерево настроек
		const awh::codec::ini::document_t & document = ::prepared();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, LOOKUP_ROUNDS, [&document]() noexcept {
			// Накопитель длин найденных значений
			uint64_t result = 0;
			// Выполняем поиск значения в первом разделе
			result += document.get("host", "server").size();
			// Выполняем поиск значения в среднем разделе
			result += document.get("level", "logging").size();
			// Выполняем поиск значения в последнем разделе
			result += document.get("ciphers", "security").size();
			// Выполняем поиск заведомо отсутствующего значения
			result += document.get("missing", "security").size();
			// Выводим накопитель длин найденных значений
			return result;
		});
		/**
		 * Устанавливаем измеренное значение
		 *
		 * @note Измеряется количество обращений в секунду, а не мегабайты: поиск
		 *       разбором текста не занят, и пропускная способность к нему неприменима
		 */
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations) / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция получения имён свойств, заводимых при замере сборки вызовами
	 *
	 * @note Имена собираются однократно до замера: сборка их внутри измеряемого
	 *       цикла вносила бы в замер стоимость работы со строкой вместо стоимости
	 *       заведения свойства
	 *
	 * @return перечень имён заводимых свойств
	 *
	 */
	static const vector <string> & names() noexcept {
		// Собираемый перечень имён заводимых свойств
		static const vector <string> result = []() noexcept -> vector <string> {
			// Собираемый перечень имён заводимых свойств
			vector <string> result;
			// Выполняем выделение памяти под перечень имён
			result.reserve(ASSEMBLY_PROPERTIES);
			/**
			 * Выполняем сборку требуемого количества имён свойств
			 */
			for(size_t i = 0; i < ASSEMBLY_PROPERTIES; i++)
				// Выполняем добавление очередного имени к перечню
				result.push_back("key" + std::to_string(i));
			// Выводим собранный перечень имён заводимых свойств
			return result;
		}();
		// Выводим перечень имён заводимых свойств
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева вызовами установки значения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t assembly() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем перечень имён заводимых свойств
		const vector <string> & keys = names();
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, ASSEMBLY_ROUNDS, [&keys]() noexcept {
			// Собираемое дерево настроек
			awh::codec::ini::document_t document;
			// Накопитель итогов заведения свойств
			uint64_t result = 0;
			/**
			 * Выполняем заведение всех свойств раздела
			 */
			for(const string & key : keys)
				// Выполняем установку значения очередного свойства
				result += static_cast <uint64_t> (document.set(key, "значение", "bulk"));
			// Выводим накопитель итогов заведения свойств
			return result;
		});
		/**
		 * Устанавливаем измеренное значение
		 *
		 * @note Измеряется количество установок значения в секунду, а не мегабайты:
		 *       сборка вызовами разбором текста не занята, и пропускная способность
		 *       к ней неприменима
		 */
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations * ASSEMBLY_PROPERTIES) / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки дерева настроек службы
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/ini: сборка дерева настроек службы", "МБ/с", BUILD_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildService
	);
	/**
	 * Выполняем регистрацию сценария сборки крупного дерева настроек
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/ini: сборка крупного дерева", "МБ/с", BUILD_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildLarge
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева со множеством разделов
	 */
	static const bool SECTIONS_REGISTERED = awh::benchmark::add(
		"codec/ini: сборка дерева со множеством разделов", "МБ/с", BUILD_SECTIONS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildSections
	);
	/**
	 * Выполняем регистрацию сценария обратной записи дерева в текст настроек
	 */
	static const bool WRITE_BACK_REGISTERED = awh::benchmark::add(
		"codec/ini: обратная запись дерева", "МБ/с", WRITE_BACK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeBack
	);
	/**
	 * Выполняем регистрацию сценария обхода дерева со множеством разделов
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/ini: обход дерева со множеством разделов", "МБ/с", WALK_SECTIONS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, walkSections
	);
	/**
	 * Выполняем регистрацию сценария поиска значений по имени
	 */
	static const bool LOOKUP_REGISTERED = awh::benchmark::add(
		"codec/ini: поиск значений по имени", "обр./с", LOOKUP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, lookup
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева вызовами установки значения
	 */
	static const bool ASSEMBLY_REGISTERED = awh::benchmark::add(
		"codec/ini: сборка дерева вызовами", "уст./с", ASSEMBLY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, assembly
	);
};
