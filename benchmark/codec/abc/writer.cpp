/**
 * @file writer.cpp
 * @date 2026-08-19
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
 * @brief Замеры сборки записи бинарного контейнера ABC
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include "abc.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::benchmark::binary;

/**
 * @brief Внутренние параметры сценариев сборки записи
 *
 */
namespace {
	/**
	 * @brief Количество собираемых записей ответа службы
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20000;
	/**
	 * @brief Количество собираемых крупных записей
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 8;
	/**
	 * @brief Количество однородных отображений крупной записи
	 *
	 */
	static constexpr size_t LARGE_COUNT = 100000;
	/**
	 * @brief Количество укладываемых строк сценария строк
	 *
	 */
	static constexpr size_t STRING_COUNT = 200000;
	/**
	 * @brief Порог укладки содержимого ссылкой в октетах
	 *
	 */
	static constexpr size_t REFERENCE_LIMIT = 64;
	/**
	 * @brief Размер значения, укладываемого ссылкой
	 *
	 */
	static constexpr size_t REFERENCE_SIZE = 4096;
	/**
	 * @brief Количество значений, укладываемых ссылкой
	 *
	 */
	static constexpr size_t REFERENCE_COUNT = 4096;

	/**
	 * @brief Порог пропускной способности сборки крупной записи
	 *
	 */
	static constexpr double WRITE_LARGE_THRESHOLD = 6.0;
	/**
	 * @brief Порог пропускной способности сборки строк
	 *
	 */
	static constexpr double WRITE_STRINGS_THRESHOLD = 20.0;
	/**
	 * @brief Порог количества выделений памяти на сборку записи
	 *
	 * @details Сборка ведёт запись одним вместилищем, растущим удвоением: расход на
	 *          запись обязан оставаться считанными единицами, а рост его означал бы,
	 *          что вместилище заводится заново либо растёт по одному значению
	 *
	 */
	static constexpr double WRITE_ALLOCATIONS_THRESHOLD = 48.0;
	/**
	 * @brief Порог задержки сборки записи ответа службы в микросекундах
	 *
	 */
	static constexpr double WRITE_SERVICE_LATENCY_THRESHOLD = 20.0;
	/**
	 * @brief Порог выигрыша от укладки содержимого ссылкой
	 *
	 * @details Измеряется отношение времени сборки с копированием ко времени сборки с
	 *          укладкой ссылкой. Отношение двух прогонов на одной машине от её
	 *          быстродействия не зависит, и порог ему можно назначить впритык
	 *
	 * @note Показатель стережёт устройство, а не скорость: укладка ссылкой на то и
	 *       заведена, чтобы крупное содержимое в запись не копировалось вовсе, и
	 *       выигрыш ниже единицы означал бы, что копия всё-таки делается
	 *
	 */
	static constexpr double WRITE_REFERENCE_THRESHOLD = 1.5;

	/**
	 * @brief Функция сборки записи однородных отображений
	 *
	 * @param count количество собираемых отображений
	 * @return      размер собранной записи в октетах
	 *
	 */
	static uint64_t compose(const size_t count) noexcept {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer;
		// Если открыть массив однородных отображений не удалось
		if(!writer.arrayBegin(static_cast <uint64_t> (count)))
			// Выводим нулевой размер собранной записи
			return 0;
		/**
		 * Выполняем укладку всех отображений записи
		 */
		for(size_t i = 0; i < count; i++){
			// Выполняем укладку очередного отображения записи
			if(!(writer.mapBegin(static_cast <uint64_t> (6)) &&
			     writer.text("active") && writer.boolean((i % 3) != 0) &&
			     writer.text("amount") && writer.number(static_cast <double> (i % 9973) + 0.25) &&
			     writer.text("city") && writer.text("Москва") &&
			     writer.text("id") && writer.number(static_cast <uint64_t> (i)) &&
			     writer.text("name") && writer.text("Товар") &&
			     writer.text("note") && writer.nul() && writer.mapEnd()))
				// Выводим нулевой размер собранной записи
				return 0;
		}
		// Если закрыть массив однородных отображений не удалось
		if(!writer.arrayEnd())
			// Выводим нулевой размер собранной записи
			return 0;
		// Выводим размер собранной записи
		return static_cast <uint64_t> (writer.record().size());
	}
	/**
	 * @brief Функция сборки записи ответа службы
	 *
	 * @return размер собранной записи в октетах
	 *
	 */
	static uint64_t tiny() noexcept {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer;
		// Выполняем укладку ответа службы
		if(!(writer.mapBegin(static_cast <uint64_t> (6)) &&
		     writer.text("active") && writer.boolean(true) &&
		     writer.text("amount") && writer.number(static_cast <double> (42.5)) &&
		     writer.text("id") && writer.number(static_cast <uint64_t> (17)) &&
		     writer.text("name") && writer.text("Товар") &&
		     writer.text("note") && writer.nul() &&
		     writer.text("tags") && writer.arrayBegin(static_cast <uint64_t> (2)) &&
		     writer.text("один") && writer.text("два") && writer.arrayEnd() && writer.mapEnd()))
			// Выводим нулевой размер собранной записи
			return 0;
		// Выводим размер собранной записи
		return static_cast <uint64_t> (writer.record().size());
	}
	/**
	 * @brief Функция сборки записи с преобладанием строк
	 *
	 * @param validate признак проверки строк на соответствие кодировке
	 * @return         размер собранной записи в октетах
	 *
	 */
	static uint64_t textual(const bool validate) noexcept {
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer;
		// Выполняем получение настроек сборки записи
		awh::codec::abc::writer_t::settings_t settings = writer.settings();
		// Выполняем установку признака проверки строк на соответствие кодировке
		settings.validate = validate;
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Укладываемая строка записи
		static const string value = "Значение по-русски и по-японски 漢字";
		// Если открыть массив строк не удалось
		if(!writer.arrayBegin(static_cast <uint64_t> (STRING_COUNT)))
			// Выводим нулевой размер собранной записи
			return 0;
		/**
		 * Выполняем укладку всех строк записи
		 */
		for(size_t i = 0; i < STRING_COUNT; i++){
			// Если уложить очередную строку записи не удалось
			if(!writer.text(value))
				// Выводим нулевой размер собранной записи
				return 0;
		}
		// Если закрыть массив строк не удалось
		if(!writer.arrayEnd())
			// Выводим нулевой размер собранной записи
			return 0;
		// Выводим размер собранной записи
		return static_cast <uint64_t> (writer.record().size());
	}
	/**
	 * @brief Функция сборки записи крупных двоичных значений
	 *
	 * @param reference признак укладки содержимого ссылкой
	 * @return          размер собранной записи в октетах
	 *
	 */
	static uint64_t referenced(const bool reference) noexcept {
		// Укладываемое крупное значение записи
		static const vector <uint8_t> value(REFERENCE_SIZE, 0x5A);
		// Сборка бинарной записи
		awh::codec::abc::writer_t writer;
		// Выполняем получение настроек сборки записи
		awh::codec::abc::writer_t::settings_t settings = writer.settings();
		// Выполняем установку порога укладки содержимого ссылкой
		settings.reference = (reference ? REFERENCE_LIMIT : 0);
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
		// Если открыть массив крупных значений не удалось
		if(!writer.arrayBegin(static_cast <uint64_t> (REFERENCE_COUNT)))
			// Выводим нулевой размер собранной записи
			return 0;
		/**
		 * Выполняем укладку всех крупных значений записи
		 */
		for(size_t i = 0; i < REFERENCE_COUNT; i++){
			// Если уложить очередное крупное значение не удалось
			if(!writer.blob(value.data(), value.size()))
				// Выводим нулевой размер собранной записи
				return 0;
		}
		// Если закрыть массив крупных значений не удалось
		if(!writer.arrayEnd())
			// Выводим нулевой размер собранной записи
			return 0;
		/**
		 * Выводим длину собранной записи.
		 *
		 * Берётся именно длина, а не размер вместилища: содержимое, уложенное ссылкой,
		 * в него не копируется вовсе, и размер вместилища был бы у двух прогонов разным
		 */
		return static_cast <uint64_t> (writer.length());
	}
	/**
	 * @brief Функция прогона сценария сборки крупной записи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeLarge() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем получение размера собираемой записи
		const size_t bytes = static_cast <size_t> (::compose(LARGE_COUNT));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, LARGE_ROUNDS, []() noexcept {
			// Выполняем сборку крупной записи
			return ::compose(LARGE_COUNT);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария сборки строк
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeStrings() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем получение размера собираемой записи
		const size_t bytes = static_cast <size_t> (::textual(true));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, LARGE_ROUNDS, []() noexcept {
			// Выполняем сборку записи с преобладанием строк
			return ::textual(true);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на сборку
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем получение размера собираемой записи
		const size_t bytes = static_cast <size_t> (::compose(LARGE_COUNT));
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, LARGE_ROUNDS, []() noexcept {
			// Выполняем сборку крупной записи
			return ::compose(LARGE_COUNT);
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Если учёт выделений памяти не работает
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное количество выделений памяти на одну запись
		result.value = perDocument(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария выигрыша от укладки содержимого ссылкой
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeReference() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем получение размера собираемой записи
		const size_t bytes = static_cast <size_t> (::referenced(false));
		// Выполняем прогон сборки с копированием содержимого
		const outcome_t copied = measure(bytes, LARGE_ROUNDS, []() noexcept {
			// Выполняем сборку записи с копированием содержимого
			return ::referenced(false);
		});
		// Выполняем прогон сборки с укладкой содержимого ссылкой
		const outcome_t linked = measure(bytes, LARGE_ROUNDS, []() noexcept {
			// Выполняем сборку записи с укладкой содержимого ссылкой
			return ::referenced(true);
		});
		/**
		 * Если время сборки с укладкой ссылкой не измерено
		 */
		if(linked.seconds <= 0.0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "время сборки с укладкой ссылкой не измерено";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренный выигрыш от укладки содержимого ссылкой
		result.value = (copied.seconds / linked.seconds);
		// Устанавливаем сведения о прогоне
		result.details = details(linked);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария задержки сборки записи ответа службы
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t latencyService() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем получение размера собираемой записи
		const size_t bytes = static_cast <size_t> (::tiny());
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(bytes, SMALL_ROUNDS, []() noexcept {
			// Выполняем сборку записи ответа службы
			return ::tiny();
		});
		// Если сценарий работы не выполнил
		if(!worked(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренную задержку сборки записи
		result.value = perLatency(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки крупной записи
	 */
	static const bool LARGE_REGISTERED = awh::benchmark::add(
		"codec/abc: сборка крупной записи", "МБ/с", WRITE_LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeLarge
	);
	/**
	 * Выполняем регистрацию сценария сборки строк
	 */
	static const bool STRINGS_REGISTERED = awh::benchmark::add(
		"codec/abc: сборка строк", "МБ/с", WRITE_STRINGS_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeStrings
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/abc: выделения на сборку", "выд./зап.", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
	/**
	 * Выполняем регистрацию сценария выигрыша от укладки содержимого ссылкой
	 */
	static const bool REFERENCE_REGISTERED = awh::benchmark::add(
		"codec/abc: выигрыш от укладки ссылкой", "раз", WRITE_REFERENCE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeReference
	);
	/**
	 * Выполняем регистрацию сценария задержки сборки записи ответа службы
	 */
	static const bool SERVICE_LATENCY_REGISTERED = awh::benchmark::add(
		"codec/abc: задержка сборки ответа службы", "мкс/зап.", WRITE_SERVICE_LATENCY_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, latencyService
	);
};
