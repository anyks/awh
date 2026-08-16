/**
 * @file document.cpp
 * @date 2026-08-16
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
 * @brief Сценарии измерения работы с деревом документа JSON — сборка дерева, обход его,
 *        обращение по имени поля, извлечение чисел и перезапись в текст
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include "json.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера JSON
 */
using namespace awh::benchmark::notation;

/**
 * @brief Внутренние параметры и сценарии бенчмарков работы с деревом документа
 *
 */
namespace {
	/**
	 * @brief Количество разбираемых мелких документов
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество разбираемых крупных документов
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество разбираемых документов с преобладанием одного вида содержимого
	 */
	static constexpr size_t FOCUSED_ROUNDS = 20;

	/**
	 * @brief Порог пропускной способности сборки дерева ответа службы
	 *
	 * @details Пороги назначены по наименьшему из показателей отладочных стендов с
	 *          двукратным запасом на разброс между прогонами
	 *
	 */
	static constexpr double BUILD_RESPONSE_THRESHOLD = 10.0;
	/**
	 * @brief Порог пропускной способности сборки дерева крупного документа
	 */
	static constexpr double BUILD_LARGE_THRESHOLD = 30.0;
	/**
	 * @brief Порог пропускной способности сборки дерева с преобладанием чисел
	 */
	static constexpr double BUILD_NUMBERS_THRESHOLD = 20.0;
	/**
	 * @brief Порог пропускной способности обхода собранного дерева
	 */
	static constexpr double WALK_LARGE_THRESHOLD = 60.0;
	/**
	 * @brief Порог пропускной способности перезаписи дерева в текст
	 */
	static constexpr double DUMP_LARGE_THRESHOLD = 40.0;
	/**
	 * @brief Порог задержки обращения к полю объекта по имени в микросекундах
	 *
	 * @details Сценарий этот стережёт устройство обращения по имени: мелкие объекты
	 *          разыскиваются перебором детей, а крупные - отображением, заводимым по
	 *          требованию. Заведение отображения на всякое обращение уронило бы
	 *          показатель на порядок
	 *
	 */
	static constexpr double ACCESS_LATENCY_THRESHOLD = 4.0;
	/**
	 * @brief Порог расхода выделений памяти на сборку одного дерева
	 *
	 * @details Дерево хранится сплошным перечнем узлов, а знаки всех строк и имён - одним
	 *          хранилищем: расход на документ обязан оставаться считанными единицами.
	 *          Рост его означал бы, что узлы или имена заводят память поодиночке
	 *
	 */
	static constexpr double BUILD_ALLOCATIONS_THRESHOLD = 12.0;
	/**
	 * @brief Порог выигрыша от переиспользования объекта документа
	 *
	 * @details Вместилища дерева и хранилище знаков разбор переживают, и потребителю,
	 *          разбирающему много документов подряд, надлежит держать один объект.
	 *          Сценарий этот стережёт сохранность памяти между разборами: пропажа её
	 *          обратила бы выигрыш в единицу
	 *
	 */
	static constexpr double REUSE_GAIN_THRESHOLD = 1.1;

	/**
	 * @brief Функция обхода собранного дерева документа
	 *
	 * @param value обходимое значение документа
	 * @return      количество обойдённых значений
	 *
	 */
	static uint64_t walk(const awh::codec::json::document_t::value_t & value) noexcept {
		// Количество обойдённых значений
		uint64_t result = 1;
		/**
		 * Определяем вид обходимого значения
		 */
		switch(static_cast <uint8_t> (value.kind())){
			/**
			 * Если значение является числом
			 */
			case static_cast <uint8_t> (awh::codec::json::kind_t::NUMBER): {
				// Извлекаемое число
				double number = 0.;
				// Выполняем извлечение числа
				value.value(number);
				// Выполняем учёт извлечённого числа
				result += static_cast <uint64_t> (number != 0.);
			} break;
			/**
			 * Если значение является строкой
			 */
			case static_cast <uint8_t> (awh::codec::json::kind_t::STRING):
				// Выполняем учёт длины строкового значения
				result += static_cast <uint64_t> (value.text().size());
			break;
			/**
			 * Если значение является вместилищем
			 */
			case static_cast <uint8_t> (awh::codec::json::kind_t::ARRAY):
			case static_cast <uint8_t> (awh::codec::json::kind_t::OBJECT): {
				/**
				 * Выполняем перебор всех значений вместилища
				 */
				for(auto item = value.begin(); item.valid(); item = item.next())
					// Выполняем обход очередного значения вместилища
					result += ::walk(item);
			} break;
		}
		// Выводим количество обойдённых значений
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева документа
	 *
	 * @param text   разбираемый текст документа
	 * @param rounds количество разбираемых документов
	 * @return       результат измерения
	 *
	 */
	static awh::benchmark::result_t building(const string & text, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Объект дерева документа
		awh::codec::json::document_t doc;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), rounds, [&text, &doc]() noexcept {
			// Выполняем сборку дерева документа
			return static_cast <uint64_t> (doc.parse(text) ? doc.root().size() : 0);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки дерева ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildResponse() noexcept {
		// Выводим результат измерения
		return ::building(response(), SMALL_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария сборки дерева крупного документа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildLarge() noexcept {
		// Выводим результат измерения
		return ::building(large(), LARGE_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария сборки дерева с преобладанием чисел
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildNumbers() noexcept {
		// Выводим результат измерения
		return ::building(numbers(), FOCUSED_ROUNDS);
	}
	/**
	 * @brief Функция прогона сценария обхода собранного дерева
	 *
	 * @details Дерево собирается однажды до замера: обход мерится отдельно от сборки,
	 *          ибо сложенные вместе они скрывают, какая из двух работ подорожала
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t walkLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем разбираемый текст документа
		const string & text = large();
		// Объект дерева документа
		awh::codec::json::document_t doc;
		/**
		 * Если сборка дерева документа завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонного документа завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&doc]() noexcept {
			// Выполняем обход собранного дерева документа
			return ::walk(doc.root());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария перезаписи дерева в текст
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t dumpLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем разбираемый текст документа
		const string & text = large();
		// Объект дерева документа
		awh::codec::json::document_t doc;
		/**
		 * Если сборка дерева документа завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонного документа завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), LARGE_ROUNDS, [&doc]() noexcept {
			// Выполняем перезапись дерева документа в текст
			return static_cast <uint64_t> (doc.dump().size());
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки обращения к полю объекта по имени
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t accessLatency() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем разбираемый текст документа
		const string & text = config();
		// Объект дерева документа
		awh::codec::json::document_t doc;
		/**
		 * Если сборка дерева документа завершилась отказом
		 */
		if(!doc.parse(text)){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "сборка дерева эталонного документа завершилась отказом";
			// Выводим результат измерения
			return result;
		}
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&doc]() noexcept {
			// Количество разысканных полей объекта
			uint64_t result = 0;
			// Выполняем учёт обращения к полю с именем службы
			result += static_cast <uint64_t> (doc["service"].valid());
			// Выполняем учёт обращения к полю с версией службы
			result += static_cast <uint64_t> (doc["version"].valid());
			// Выполняем учёт обращения к полю с узлами службы
			result += static_cast <uint64_t> (doc["nodes"].valid());
			// Выполняем учёт обращения к отсутствующему полю
			result += static_cast <uint64_t> (doc["missing"].valid());
			// Выводим количество разысканных полей объекта
			return result;
		});
		// Устанавливаем измеренную задержку обращения
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на сборку дерева
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t buildAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем разбираемый текст документа
		const string & text = config();
		// Объект дерева документа
		awh::codec::json::document_t doc;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(text.size(), SMALL_ROUNDS, [&text, &doc]() noexcept {
			// Выполняем сборку дерева документа
			return static_cast <uint64_t> (doc.parse(text) ? doc.root().size() : 0);
		});
		/**
		 * Если учёт выделений памяти неработоспособен
		 */
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренный расход выделений памяти
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария выигрыша от переиспользования объекта документа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t reuseGain() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем разбираемый текст документа
		const string & text = response();
		// Объект дерева документа, переживающий разборы
		awh::codec::json::document_t kept;
		// Выполняем прогон разбора переиспользуемым объектом документа
		const outcome_t reused = measure(text.size(), SMALL_ROUNDS, [&text, &kept]() noexcept {
			// Выполняем сборку дерева документа
			return static_cast <uint64_t> (kept.parse(text) ? kept.root().size() : 0);
		});
		// Выполняем прогон разбора объектом документа, заводимым на всякий текст
		const outcome_t fresh = measure(text.size(), SMALL_ROUNDS, [&text]() noexcept {
			// Объект дерева документа
			awh::codec::json::document_t doc;
			// Выполняем сборку дерева документа
			return static_cast <uint64_t> (doc.parse(text) ? doc.root().size() : 0);
		});
		// Получаем пропускную способность разбора объектом, заводимым на всякий текст
		const double once = perSecond(fresh);
		/**
		 * Если разбор объектом, заводимым на всякий текст, не состоялся
		 */
		if(once <= 0.0){
			// Устанавливаем признак негодности измерения
			result.invalid = true;
			// Устанавливаем причину негодности измерения
			result.reason = "разбор объектом, заводимым на всякий текст, не состоялся";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренный выигрыш от переиспользования объекта документа
		result.value = (perSecond(reused) / once);
		// Устанавливаем сведения о прогоне
		result.details = details(reused);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки дерева ответа службы
	 */
	static const bool RESPONSE_REGISTERED = awh::benchmark::add(
		"codec/json: сборка дерева ответа службы", "МБ/с", BUILD_RESPONSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildResponse
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева крупного документа
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/json: сборка дерева крупного документа", "МБ/с", BUILD_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildLarge
	);
	/**
	 * Выполняем регистрацию сценария сборки дерева с преобладанием чисел
	 */
	static const bool NUMBERS_REGISTERED = awh::benchmark::add(
		"codec/json: сборка дерева чисел", "МБ/с", BUILD_NUMBERS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, buildNumbers
	);
	/**
	 * Выполняем регистрацию сценария обхода собранного дерева
	 */
	static const bool WALK_REGISTERED = awh::benchmark::add(
		"codec/json: обход собранного дерева", "МБ/с", WALK_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, walkLarge
	);
	/**
	 * Выполняем регистрацию сценария перезаписи дерева в текст
	 */
	static const bool DUMP_REGISTERED = awh::benchmark::add(
		"codec/json: перезапись дерева в текст", "МБ/с", DUMP_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, dumpLarge
	);
	/**
	 * Выполняем регистрацию сценария задержки обращения к полю объекта по имени
	 */
	static const bool ACCESS_REGISTERED = awh::benchmark::add(
		"codec/json: задержка обращения по имени поля", "мкс/док.", ACCESS_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, accessLatency
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку дерева
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/json: выделения на сборку дерева", "выд./док.", BUILD_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, buildAllocations
	);
	/**
	 * Выполняем регистрацию сценария выигрыша от переиспользования объекта документа
	 */
	static const bool REUSE_REGISTERED = awh::benchmark::add(
		"codec/json: выигрыш от переиспользования документа", "раз", REUSE_GAIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, reuseGain
	);
};
