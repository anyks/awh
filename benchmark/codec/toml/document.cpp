/**
 * @file: document.cpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Бенчмарки дерева настроек TOML — сборка дерева разбором, обход его значений,
 *        правка записей и обратная запись дерева в текст настроек
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include "toml.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера TOML
 */
using namespace awh::benchmark::settings;

/**
 * @brief Внутренние параметры и сценарии бенчмарков дерева настроек
 *
 */
namespace {
	/**
	 * @brief Количество собираемых деревьев мелкого файла настроек
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 8000;
	/**
	 * @brief Количество собираемых деревьев крупного файла настроек
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 4;
	/**
	 * @brief Количество собираемых деревьев текста со множеством таблиц
	 *
	 */
	static constexpr size_t FOCUSED_ROUNDS = 12;
	/**
	 * @brief Количество правок, вносимых в дерево настроек за один прогон
	 *
	 */
	static constexpr size_t EDIT_COUNT = 200;
	/**
	 * Количество правок сценария долгой правки дерева настроек
	 *
	 * @note Число взято с запасом против порога уплотнения: уплотнение приходит по
	 *       накоплении мусора вровень с живым, и прогон, до него не дошедший, роста
	 *       памяти не показал бы вовсе
	 */
	static constexpr size_t LASTING_COUNT = 20000;

	/**
	 * @brief Порог пропускной способности сборки дерева мелкого файла настроек
	 *
	 * @details Пороги назначены по замеру на рабочей машине 12.08.2026 с запасом
	 *          вчетверо: отладочные стенды отстают от неё вчетверо-впятеро
	 *
	 */
	static constexpr double TREE_SERVICE_THRESHOLD = 5.0;
	/**
	 * @brief Порог пропускной способности сборки дерева крупного файла настроек
	 *
	 * @note Запас взят двукратным к самому медленному из известных стендов: на
	 *       OpenBSD 7.9/amd64 сценарий даёт 3.15 МБ/с против 23.95 у рабочей машины
	 *       (13.08.2026). Просадка эта принадлежит машине, а не модулю - у соседних
	 *       кодеков на той же машине отношение хуже: xml 8.0 раза, ini 8.1 раза,
	 *       toml 7.6 раза
	 *
	 */
	static constexpr double TREE_LARGE_THRESHOLD = 1.5;
	/**
	 * @brief Порог пропускной способности сборки дерева со множеством таблиц
	 *
	 * @details Показатель этот стережёт указатели дерева: поиск таблицы попарным
	 *          сличением имён обращает сборку в квадратичную, и на тексте из одних
	 *          лишь объявлений таблиц падение видно сразу
	 *
	 */
	static constexpr double TREE_TABLES_THRESHOLD = 3.0;
	/**
	 * @brief Порог пропускной способности перезаписи дерева настроек
	 *
	 */
	static constexpr double TREE_REWRITE_THRESHOLD = 4.0;
	/**
	 * @brief Порог задержки правки дерева настроек в микросекундах
	 *
	 * @details Правка ищет запись по составному имени и правит её на месте, а
	 *          заведение отсутствующей пары перестраивает указатели поиска: рост
	 *          показателя означает, что перестроение стало выполняться и там, где
	 *          состав записей не менялся
	 *
	 */
	static constexpr double TREE_EDIT_THRESHOLD = 4.0;
	/**
	 * Порог задержки одной правки при долгой правке дерева в микросекундах
	 *
	 * @note Показатель этот назначен цене уплотнения: мусор, правкою накопленный,
	 *       дерево собирает заново по накоплении его вровень с живым, и уплотнение,
	 *       приходящее слишком часто, видно здесь ростом задержки. Объёмом выделенной
	 *       памяти цену эту не измерить: удержания он не меряет вовсе, и дерево без
	 *       уплотнения даёт по нему те же четыре с половиной сотни октетов на правку,
	 *       что и дерево с ним - замер это и показал
	 */
	static constexpr double TREE_LASTING_THRESHOLD = 6.0;

	/**
	 * @brief Функция сборки дерева настроек разбором текста
	 *
	 * @param text разбираемый текст настроек
	 * @return     количество объявленных таблиц собранного дерева
	 *
	 */
	static uint64_t build(const string & text) noexcept {
		// Объект дерева настроек
		awh::codec::toml::document_t document;
		/**
		 * Если разбор текста настроек не удался
		 */
		if(!document.parse(text))
			// Выводим нулевое количество объявленных таблиц
			return 0;
		// Выводим количество объявленных таблиц собранного дерева
		return static_cast <uint64_t> (document.size());
	}
	/**
	 * @brief Функция прогона сценария сборки дерева заданного текста настроек
	 *
	 * @param text   разбираемый текст настроек
	 * @param rounds количество собираемых деревьев настроек
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t building(const string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), rounds, [&text]() noexcept {
			// Выполняем сборку дерева настроек
			return ::build(text);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева мелкого файла настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeService() noexcept {
		// Выполняем прогон сценария сборки дерева мелкого файла настроек
		return ::building(service(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария сборки дерева крупного файла настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeLarge() noexcept {
		// Выполняем прогон сценария сборки дерева крупного файла настроек
		return ::building(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария сборки дерева со множеством таблиц
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeTables() noexcept {
		// Выполняем прогон сценария сборки дерева со множеством таблиц
		return ::building(tables(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария перезаписи дерева настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeRewrite() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Разбираемый текст настроек
		const string & text = large();
		// Объект дерева настроек
		awh::codec::toml::document_t document;
		/**
		 * Если разбор текста настроек не удался
		 *
		 * @note Сборка дерева выполняется вне замера: измеряется здесь одна лишь
		 *       перезапись, и стоимость разбора её показатель искажала бы
		 */
		if(!document.parse(text)){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор эталонного текста настроек не удался";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&document]() noexcept {
			// Выполняем перезапись дерева настроек
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
	 * @brief Функция прогона сценария правки дерева настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeEdit() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект дерева настроек
		awh::codec::toml::document_t document;
		/**
		 * Если разбор текста настроек не удался
		 */
		if(!document.parse(service())){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор эталонного текста настроек не удался";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, EDIT_COUNT, [&document]() noexcept {
			/**
			 * Выполняем установку значения объявленной пары
			 *
			 * @note Правится объявленная пара: состав записей при этом не меняется, и
			 *       перестроения указателей поиска правка требовать не должна
			 */
			document.set({"server", "port"}, static_cast <int64_t> (9090));
			// Выводим количество объявленных таблиц дерева настроек
			return static_cast <uint64_t> (document.size());
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "правка не выполнила ни одной операции";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера задержки правки при долгой правке дерева
	 *
	 * @details Правка замещает узел значения новым, а прежний оставляет мусором: без
	 * уплотнения дерева память растёт неограниченно при неизменном его составе, а
	 * уплотнение, приходящее слишком часто, обходится задержкою правки
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t treeLasting() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект дерева настроек
		awh::codec::toml::document_t document;
		/**
		 * Если разбор текста настроек не удался
		 */
		if(!document.parse(service())){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "разбор эталонного текста настроек не удался";
			// Выводим результат измерения
			return result;
		}
		// Порядковый номер очередной правки дерева настроек
		size_t number = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(0, LASTING_COUNT, [&document, &number]() noexcept {
			// Выполняем установку значения объявленной пары
			document.set({"server", "port"}, static_cast <int64_t> (9090 + (number++ % 2)));
			// Выводим количество объявленных таблиц дерева настроек
			return static_cast <uint64_t> (document.size());
		});
		/**
		 * Если ни одной операции не выполнено
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "долгая правка не выполнила ни одной операции";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренное значение
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки дерева мелкого файла настроек
	 */
	static const bool SERVICE_REGISTERED = awh::benchmark::add(
		"codec/toml: сборка дерева настроек службы", "МБ/с", TREE_SERVICE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeService
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева крупного файла настроек
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/toml: сборка дерева крупного файла", "МБ/с", TREE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeLarge
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева со множеством таблиц
	 */
	static const bool TABLES_REGISTERED = awh::benchmark::add(
		"codec/toml: сборка дерева со множеством таблиц", "МБ/с", TREE_TABLES_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeTables
	);
	/**
	 * Выполняем регистрацию сценария перезаписи дерева настроек
	 */
	static const bool REWRITE_REGISTERED = awh::benchmark::add(
		"codec/toml: перезапись дерева настроек", "МБ/с", TREE_REWRITE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, treeRewrite
	);
	/**
	 * Выполняем регистрацию сценария правки дерева настроек
	 */
	static const bool EDIT_REGISTERED = awh::benchmark::add(
		"codec/toml: правка значения дерева", "мкс/правку", TREE_EDIT_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, treeEdit
	);
	/**
	 * Выполняем регистрацию сценария долгой правки дерева настроек
	 */
	static const bool LASTING_REGISTERED = awh::benchmark::add(
		"codec/toml: долгая правка дерева", "мкс/правку", TREE_LASTING_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, treeLasting
	);
};
