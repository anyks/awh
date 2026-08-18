/**
 * @file schedule.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки отбоя срока бинарного контейнера ABC — поверка штампом при обращении,
 *        отбой своим потоком и остановка, не дожидающаяся конца срока
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/abc/abc.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Проверка поверки срока штампом времени при обращении
 *
 */
TEST(ScheduleTest, DeadlineTouch) {
	// Отбой срока
	abc::schedule_t schedule;
	// Выполняем запуск отбоя срока поверкой при обращении
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::DEADLINE, 40));
	// Выполняем проверку того, что срок сразу же не наступил
	ASSERT_FALSE(schedule.touch());
	// Выполняем ожидание наступления срока
	this_thread::sleep_for(chrono::milliseconds(60));
	// Выполняем проверку наступления срока
	ASSERT_TRUE(schedule.touch());
	/**
	 * Выполняем проверку того, что срок отсчитывается наново: иначе всякое
	 * обращение после первого наступления срока валило бы фиксацию подряд
	 */
	ASSERT_FALSE(schedule.touch());
	// Выполняем остановку отбоя срока
	schedule.stop();
	// Выполняем ожидание наступления срока
	this_thread::sleep_for(chrono::milliseconds(60));
	// Выполняем проверку того, что остановленный отбой срока не отбивает
	ASSERT_FALSE(schedule.touch());
}
/**
 * @brief Проверка того, что поверка при обращении не о прочих способах
 *
 */
TEST(ScheduleTest, TouchIsDeadlineOnly) {
	// Отбой срока
	abc::schedule_t schedule;
	// Количество отбитых сроков
	atomic <size_t> beats{0};
	// Выполняем установку работы, зовомой по наступлении срока
	schedule.callback([&beats]() noexcept -> void {
		// Выполняем увеличение количества отбитых сроков
		beats++;
	});
	// Выполняем запуск отбоя срока своим потоком
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::THREAD, 20));
	/**
	 * Выполняем проверку того, что поверка при обращении срока не отбивает:
	 * срок отбивается потоком, и поверка обратила бы его в два срока разом
	 */
	ASSERT_FALSE(schedule.touch());
	// Выполняем остановку отбоя срока
	schedule.stop();
}
/**
 * @brief Проверка отбоя срока своим потоком
 *
 */
TEST(ScheduleTest, ThreadBeats) {
	// Отбой срока
	abc::schedule_t schedule;
	// Количество отбитых сроков
	atomic <size_t> beats{0};
	// Выполняем установку работы, зовомой по наступлении срока
	schedule.callback([&beats]() noexcept -> void {
		// Выполняем увеличение количества отбитых сроков
		beats++;
	});
	// Выполняем запуск отбоя срока своим потоком
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::THREAD, 20));
	// Выполняем проверку работы отбоя срока
	ASSERT_TRUE(schedule.working());
	// Выполняем ожидание нескольких сроков подряд
	this_thread::sleep_for(chrono::milliseconds(120));
	// Выполняем остановку отбоя срока
	schedule.stop();
	// Выполняем получение количества отбитых сроков
	const size_t counted = beats.load();
	// Выполняем проверку того, что срок отбит своим потоком и без обращений
	ASSERT_GT(counted, 0ul);
	// Выполняем ожидание, вдвое превышающее срок отбоя
	this_thread::sleep_for(chrono::milliseconds(60));
	// Выполняем проверку того, что остановленный отбой срока более не отбивает
	ASSERT_EQ(beats.load(), counted);
	// Выполняем проверку того, что работа отбоя срока прекращена
	ASSERT_FALSE(schedule.working());
}
/**
 * @brief Проверка того, что остановка не дожидается конца срока
 *
 * @details Ожидание идёт на условной переменной, а не выдержкой: выдержка обязала бы
 *          остановку дожидаться конца срока, а это зависание, а не остановка
 *
 */
TEST(ScheduleTest, StopDoesNotWaitDeadline) {
	// Отбой срока
	abc::schedule_t schedule;
	// Выполняем установку работы, зовомой по наступлении срока
	schedule.callback([]() noexcept -> void {
		// Работы по наступлении срока здесь нет
	});
	// Выполняем запуск отбоя срока своим потоком длиною в пять секунд
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::THREAD, 5000));
	// Выполняем получение штампа времени начала остановки
	const auto start = chrono::steady_clock::now();
	// Выполняем остановку отбоя срока
	schedule.stop();
	// Выполняем получение длительности остановки отбоя срока
	const auto spent = chrono::duration_cast <chrono::milliseconds> (chrono::steady_clock::now() - start).count();
	// Выполняем проверку того, что остановка конца срока не дожидалась
	ASSERT_LT(spent, 1000);
}
