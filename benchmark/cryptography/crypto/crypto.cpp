/**
 * @file crypto.cpp
 * @date 2026-08-01
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
 * @brief Общее окружение бенчмарков модуля криптографии — эталонный буфер данных,
 *        объект криптографии, контрольная сумма прогонов и формирование сведений о замере
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков модуля криптографии
 */
#include "crypto.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::crypto::details(const outcome_t & output) noexcept {
	// Буфер формирования сведений о прогоне
	char buffer[224];
	// Вычисляем среднее время выполнения одной операции в наносекундах
	const double nanoseconds = ((output.operations > 0)
	 ? ((output.seconds * 1e9) / static_cast <double> (output.operations)) : 0.0);
	// Вычисляем пропускную способность шифрования в мебиоктетах в секунду
	const double bandwidth = (perBytes(output) / 1048576.0);
	// Выполняем формирование сведений о прогоне
	::snprintf(
		buffer, sizeof(buffer),
		"операций: %zu, данных: %zu октетов, время: %.3f с, на операцию: %.1f нс, "
		"пропускная способность: %.1f МиБ/с, выделений: %zu (%zu октетов)",
		output.operations, output.size, output.seconds, nanoseconds,
		bandwidth, output.allocations, output.allocated
	);
	// Выводим сведения о прогоне
	return string(buffer);
}
/**
 * @brief Функция извлечения количества операций в секунду
 *
 * @param output итоги прогона сценария
 * @return       количество операций в секунду
 *
 */
double awh::benchmark::crypto::perSecond(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим нулевое количество операций в секунду
		return 0.0;
	// Выводим количество операций в секунду
	return (static_cast <double> (output.operations) / output.seconds);
}
/**
 * @brief Функция извлечения пропускной способности шифрования
 *
 * @param output итоги прогона сценария
 * @return       количество обработанных октетов в секунду
 *
 */
double awh::benchmark::crypto::perBytes(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим нулевую пропускную способность
		return 0.0;
	// Выводим количество обработанных октетов в секунду
	return ((static_cast <double> (output.operations) * static_cast <double> (output.size)) / output.seconds);
}
/**
 * @brief Функция извлечения количества выделений памяти на одну операцию
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну операцию
 *
 */
double awh::benchmark::crypto::perOperation(const outcome_t & output) noexcept {
	// Если операции не выполнялись
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну операцию
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::crypto::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонного буфера данных
 *
 * @return эталонный буфер данных
 *
 */
const vector <uint8_t> & awh::benchmark::crypto::buffer() noexcept {
	/**
	 * @brief Функция формирования эталонного буфера данных
	 *
	 * @return эталонный буфер данных
	 *
	 */
	static const vector <uint8_t> result = []() noexcept -> vector <uint8_t> {
		// Эталонный буфер данных
		vector <uint8_t> data(LARGE_SIZE, 0);
		/**
		 * Выполняем заполнение эталонного буфера данных значащими октетами
		 */
		for(size_t i = 0; i < data.size(); i++)
			// Заполняем очередной октет эталонного буфера данных
			data[i] = static_cast <uint8_t> ((i * 131) ^ (i >> 3));
		// Выводим эталонный буфер данных
		return data;
	}();
	// Выводим эталонный буфер данных
	return result;
}
/**
 * @brief Функция получения объекта фреймворка
 *
 * @return объект фреймворка
 *
 */
const awh::fmk_t * awh::benchmark::crypto::framework() noexcept {
	// Объект фреймворка
	static const awh::fmk_t result;
	// Выводим объект фреймворка
	return &result;
}
/**
 * @brief Функция получения объекта работы с логами
 *
 * @return объект работы с логами
 *
 */
const awh::log_t * awh::benchmark::crypto::logger() noexcept {
	// Объект работы с логами
	static const awh::log_t result(framework());
	// Выводим объект работы с логами
	return &result;
}
/**
 * @brief Функция получения эталонного объекта криптографии
 *
 * @return эталонный объект криптографии
 *
 */
awh::crypto_t & awh::benchmark::crypto::engine() noexcept {
	/**
	 * Объект заведён единожды и общий для всех сценариев: вывод ключа стоит ста тысяч
	 * итераций, и заводить объект в каждом сценарии значило бы мерить этот вывод всюду
	 * вместо самой измеряемой работы. Цена вывода измеряется отдельным сценарием
	 */
	// Эталонный объект криптографии
	static awh::crypto_t result(framework(), logger());
	// Признак выполненного заведения объекта криптографии
	static const bool ready = []() noexcept -> bool {
		// Устанавливаем пароль шифрования
		result.password("benchmark password");
		// Устанавливаем соль вывода ключа
		result.salt("benchmark salt");
		// Устанавливаем режим блочного шифрования с проверкой подлинности
		result.mode(awh::crypto_t::mode_t::GCM);
		// Выводим признак выполненного заведения
		return true;
	}();
	// Снимаем предупреждение о неиспользуемом признаке заведения
	(void) ready;
	// Выводим эталонный объект криптографии
	return result;
}
