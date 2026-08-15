/**
 * @file writer.cpp
 * @date 2026-08-15
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
 * @brief Сценарии измерения записи чисел в строковое представление — запись целых чисел
 *        и запись чисел с плавающей точкой кратчайшим обратимым представлением,
 *        а также сличение с подбором точности через snprintf с обратным чтением
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков модуля разбора чисел
 */
#include "parser.hpp"

/**
 * Подключаем стандартные заголовочные файлы
 */
#include <cstdio>
#include <limits>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля разбора чисел
 */
using namespace awh::benchmark::parser;

/**
 * @brief Внутренние параметры и сценарии бенчмарков записи чисел
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев записи
	 *
	 */
	static constexpr size_t WRITE_ROUNDS = 2000000;
	/**
	 * @brief Количество операций сценария подбора точности
	 *
	 * @details Подбор точности выполняет до семнадцати записей числа с обратным
	 *          чтением каждой из них, отчего на порядок медленнее прямой записи
	 *          и требует меньшего количества операций для того же времени прогона
	 *
	 */
	static constexpr size_t SEARCH_ROUNDS = 200000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 200000;
	/**
	 * @brief Порог скорости записи числа с плавающей точкой
	 *
	 * @details Пороги откалиброваны по неоптимизированной сборке с четырёхкратным
	 *          запасом - они ловят регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double WRITE_DOUBLE_THRESHOLD = 1280000.0;
	/**
	 * @brief Порог скорости записи числа с плавающей точкой одинарной точности
	 *
	 */
	static constexpr double WRITE_FLOAT_THRESHOLD = 1650000.0;
	/**
	 * @brief Порог скорости записи целого числа
	 *
	 */
	static constexpr double WRITE_INTEGER_THRESHOLD = 6900000.0;
	/**
	 * @brief Порог отношения скорости записи к подбору точности
	 *
	 * @details Показатель отражает выигрыш прямой записи перед подбором точности
	 *          через snprintf с обратным чтением каждой пробы. Порог взят вчетверо
	 *          ниже измеренного в неоптимизированной сборке, как и у остальных
	 *          сценариев: он ловит возврат к подбору точности, а не колебания окружения
	 *
	 */
	static constexpr double WRITE_SPEEDUP_THRESHOLD = 2.0;
	/**
	 * @brief Порог количества выделений памяти на одну запись
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Метод формирования выборки чисел с плавающей точкой
	 *
	 * @return выборка чисел с плавающей точкой
	 *
	 */
	static const vector <double> & samples() noexcept {
		/**
		 * Выборка чисел с плавающей точкой различного вида
		 */
		static const vector <double> result = {
			1.0, -1.0, 0.5, 0.3, 123.456, 100.0, 1e6, 1e-5, 3.14159265358979,
			2.718281828459045, 1e300, 5e-324, 1.7976931348623157e308,
			2.2250738585072014e-308, 6.02214076e23, -273.15, 0.1, 1234567.891,
			9007199254740992.0, -2.5e-10
		};
		// Выводим выборку чисел с плавающей точкой
		return result;
	}

	/**
	 * @brief Функция прогона сценария записи числа с плавающей точкой
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeDouble() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем выборку записываемых чисел
		const vector <double> & values = samples();
		// Хранилище записи числа
		char buffer[awh::lexical::maxRecordLength <double> ()];
		// Накопитель длин сформированных записей
		size_t accumulator = 0;
		// Индекс записываемого числа выборки
		size_t index = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(WRITE_ROUNDS, [&]() noexcept {
			// Выполняем запись очередного числа выборки
			const awh::lexical::output_t <char> output = awh::lexical_t::toChars(
				buffer, buffer + sizeof(buffer), values[index]
			);
			// Накапливаем длину сформированной записи
			accumulator += static_cast <size_t> (output.ptr - buffer);
			// Выполняем переход к следующему числу выборки
			index = ((index + 1) % values.size());
			// Запрещаем вынос измеряемой записи за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция прогона сценария записи числа одинарной точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeFloat() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем выборку записываемых чисел
		const vector <double> & values = samples();
		// Хранилище записи числа
		char buffer[awh::lexical::maxRecordLength <float> ()];
		// Накопитель длин сформированных записей
		size_t accumulator = 0;
		// Индекс записываемого числа выборки
		size_t index = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(WRITE_ROUNDS, [&]() noexcept {
			// Выполняем запись очередного числа выборки
			const awh::lexical::output_t <char> output = awh::lexical_t::toChars(
				buffer, buffer + sizeof(buffer), static_cast <float> (values[index])
			);
			// Накапливаем длину сформированной записи
			accumulator += static_cast <size_t> (output.ptr - buffer);
			// Выполняем переход к следующему числу выборки
			index = ((index + 1) % values.size());
			// Запрещаем вынос измеряемой записи за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция прогона сценария записи целого числа
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeInteger() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Хранилище записи числа
		char buffer[32];
		// Накопитель длин сформированных записей
		size_t accumulator = 0;
		// Записываемое значение числа
		int64_t value = 1;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(WRITE_ROUNDS, [&]() noexcept {
			// Выполняем запись очередного числа
			const awh::lexical::output_t <char> output = awh::lexical_t::toChars(
				buffer, buffer + sizeof(buffer), value
			);
			// Накапливаем длину сформированной записи
			accumulator += static_cast <size_t> (output.ptr - buffer);
			// Выполняем смену записываемого значения числа
			value = ((value * 1103515245) + 12345);
			// Запрещаем вынос измеряемой записи за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Метод подбора кратчайшей записи числа через обратное чтение
	 *
	 * @details Способ этот применялся сборщиками текстовых форматов до появления
	 *          прямой записи: точность наращивается по одной цифре, и каждая проба
	 *          читается обратно для сличения с исходным числом. На числах, требующих
	 *          семнадцати значащих цифр, способ выполняет семнадцать записей и
	 *          семнадцать разборов вместо одной записи.
	 *
	 * @param value  записываемое число
	 * @param buffer хранилище записи числа
	 * @param size   размер хранилища записи числа
	 * @return       длина сформированной записи
	 *
	 */
	static size_t searchShortest(const double value, char * buffer, const size_t size) noexcept {
		// Длина сформированной записи числа
		int32_t length = 0;
		/**
		 * Выполняем подбор кратчайшей записи числа наращиванием точности
		 */
		for(int32_t digits = 1; digits <= static_cast <int32_t> (numeric_limits <double>::max_digits10); digits++){
			// Выполняем запись числа очередной точностью
			length = ::snprintf(buffer, size, "%.*g", digits, value);
			// Если запись числа выполнить не удалось
			if((length <= 0) || (static_cast <size_t> (length) >= size))
				// Выполняем прекращение подбора точности записи
				break;
			// Прочитанное обратно значение записанного числа
			double back = 0.;
			// Выполняем разбор записанного числа
			const awh::lexical_t::result_t <char> res = awh::lexical_t::fromChars(
				buffer, buffer + static_cast <size_t> (length), back
			);
			// Если запись читается обратно тем же самым числом
			if(static_cast <bool> (res) && (res.ptr == (buffer + static_cast <size_t> (length))) && (back == value))
				// Выполняем прекращение подбора точности записи
				break;
		}
		// Выводим длину сформированной записи числа
		return ((length > 0) ? static_cast <size_t> (length) : 0);
	}

	/**
	 * @brief Функция прогона сценария подбора кратчайшей записи обратным чтением
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t searchDouble() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем выборку записываемых чисел
		const vector <double> & values = samples();
		// Хранилище записи числа
		char buffer[64];
		// Накопитель длин сформированных записей
		size_t accumulator = 0;
		// Индекс записываемого числа выборки
		size_t index = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(SEARCH_ROUNDS, [&]() noexcept {
			// Выполняем подбор кратчайшей записи очередного числа выборки
			accumulator += searchShortest(values[index], buffer, sizeof(buffer));
			// Выполняем переход к следующему числу выборки
			index = ((index + 1) % values.size());
			// Запрещаем вынос измеряемого подбора за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator);
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция прогона сценария отношения скорости записи к подбору точности
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeSpeedup() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон сценария прямой записи
		const awh::benchmark::result_t direct = writeDouble();
		// Выполняем прогон сценария подбора точности
		const awh::benchmark::result_t search = searchDouble();
		// Устанавливаем отношение скоростей записи
		result.value = ((search.value > 0.0) ? (direct.value / search.value) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = ("прямая запись: " + to_string(static_cast <uint64_t> (direct.value)) +
		                  " операций/с, подбор точности: " + to_string(static_cast <uint64_t> (search.value)) + " операций/с");
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция прогона сценария учёта выделений памяти при записи
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем выборку записываемых чисел
		const vector <double> & values = samples();
		// Хранилище записи числа
		char buffer[awh::lexical::maxRecordLength <double> ()];
		// Накопитель длин сформированных записей
		size_t accumulator = 0;
		// Индекс записываемого числа выборки
		size_t index = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(ALLOCATION_ROUNDS, [&]() noexcept {
			// Выполняем запись очередного числа выборки
			const awh::lexical::output_t <char> output = awh::lexical_t::toChars(
				buffer, buffer + sizeof(buffer), values[index]
			);
			// Накапливаем длину сформированной записи
			accumulator += static_cast <size_t> (output.ptr - buffer);
			// Выполняем переход к следующему числу выборки
			index = ((index + 1) % values.size());
			// Запрещаем вынос измеряемой записи за пределы цикла
			barrier(&accumulator);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += static_cast <uint64_t> (accumulator);
		// Вычисляем количество выделений памяти на одну операцию
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий записи числа с плавающей точкой
	static const bool gWriteDouble = awh::benchmark::add(
		"lexical/write/double", "операций/с", WRITE_DOUBLE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::writeDouble
	);
	// Регистрируем сценарий записи числа одинарной точности
	static const bool gWriteFloat = awh::benchmark::add(
		"lexical/write/float", "операций/с", WRITE_FLOAT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::writeFloat
	);
	// Регистрируем сценарий записи целого числа
	static const bool gWriteInteger = awh::benchmark::add(
		"lexical/write/integer", "операций/с", WRITE_INTEGER_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::writeInteger
	);
	// Регистрируем сценарий отношения скорости записи к подбору точности
	static const bool gSpeedup = awh::benchmark::add(
		"lexical/write/speedup-over-search", "раз", WRITE_SPEEDUP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::writeSpeedup
	);
	// Регистрируем сценарий учёта выделений памяти при записи
	static const bool gWriteAllocations = awh::benchmark::add(
		"lexical/write/allocations-per-write", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::writeAllocations
	);
};
