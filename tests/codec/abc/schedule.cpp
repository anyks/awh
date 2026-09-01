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
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Функция извлечения объекта журнала проверок
	 *
	 * @details Журнал заводится единожды на весь набор и гасится: проверки отказов
	 *          выводили бы записью всякий свой отказ, а их тут большинство. Гашение
	 *          это - настройка журнала, а не молчание модуля: модуль доносит как
	 *          обычно, а показывать ли - решает журнал
	 *
	 * @return объект журнала проверок
	 *
	 */
	[[maybe_unused]] const log_t * logger() noexcept {
		// Объект фреймворка проверок
		static fmk_t fmk;
		// Объект журнала проверок
		static log_t log(& fmk);
		// Признак выполненной настройки журнала
		static const bool ready = [](){
			// Выполняем гашение вывода журнала проверок
			log.level(log_t::level_t::NONE);
			// Выводим признак выполненной настройки
			return true;
		}();
		// Снимаем неиспользуемый признак настройки
		(void) ready;
		// Выводим объект журнала проверок
		return & log;
	}
};

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
/**
 * @brief Проверка отказов запуска отбоя срока
 *
 * @details Молчаливое согласие на негодный довод обратило бы отбой срока в холостое
 * кручение: поток с нулевым сроком не спит вовсе и съедает ядро целиком
 *
 */
TEST(ScheduleTest, StartRefusals) {
	// Отбой срока
	abc::schedule_t schedule;
	// Выполняем проверку отказа запуска отбоя срока без способа отбоя
	ASSERT_FALSE(schedule.start(abc::schedule_t::mode_t::NONE, 40));
	// Выполняем проверку того, что отбой срока не работает
	ASSERT_FALSE(schedule.working());
	// Выполняем проверку отказа запуска отбоя срока с нулевым сроком
	ASSERT_FALSE(schedule.start(abc::schedule_t::mode_t::THREAD, 0));
	// Выполняем проверку того, что отбой срока не работает
	ASSERT_FALSE(schedule.working());
	// Выполняем проверку того, что поверка при обращении срока не отбивает
	ASSERT_FALSE(schedule.touch());
	// Выполняем проверку способа отбоя срока
	ASSERT_EQ(schedule.mode(), abc::schedule_t::mode_t::NONE);
	// Выполняем запуск отбоя срока поверкой при обращении
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::DEADLINE, 40));
	/**
	 * Выполняем проверку того, что отказавший запуск прежний отбой ОСТАНОВИЛ:
	 * запуск первым делом останавливает прежний, и отказ его позднее уже не
	 * возвращает - потребитель обязан знать, что отбоя более нет
	 */
	ASSERT_FALSE(schedule.start(abc::schedule_t::mode_t::THREAD, 0));
	// Выполняем проверку того, что прежний отбой срока остановлен
	ASSERT_FALSE(schedule.working());
	// Выполняем ожидание наступления прежнего срока
	this_thread::sleep_for(chrono::milliseconds(60));
	// Выполняем проверку того, что остановленный отбой срока не отбивает
	ASSERT_FALSE(schedule.touch());
}
/**
 * @brief Проверка смены способа отбоя срока запуском наново
 *
 * @details Запуск наново обязан остановить прежний отбой: два потока отбоя на одном
 * объекте отбивали бы срок вдвое чаще объявленного
 *
 */
TEST(ScheduleTest, RestartReplaces) {
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
	// Выполняем проверку способа отбоя срока
	ASSERT_EQ(schedule.mode(), abc::schedule_t::mode_t::THREAD);
	// Выполняем ожидание нескольких сроков подряд
	this_thread::sleep_for(chrono::milliseconds(120));
	/**
	 * Выполняем проверку того, что срок отбит своим потоком НЕ ОДНАЖДЫ: отбой обязан
	 * быть повторяющимся, а поток, отбивший срок единожды и вставший, прошёл бы
	 * сличение с нулём зелёным
	 *
	 * @note Порог взят вдвое, а не всемеро: за 120 мс при сроке 20 мс отбоев ждётся
	 *       шесть, замерено четыре-пять на свободной машине, и порог держит замедление
	 *       вдвое с половиной. Строгий порог ловил бы занятость хозяина, а не дефект
	 */
	ASSERT_GE(beats.load(), 2ul);
	// Выполняем запуск отбоя срока поверкой при обращении наново
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::DEADLINE, 40));
	// Выполняем проверку смены способа отбоя срока
	ASSERT_EQ(schedule.mode(), abc::schedule_t::mode_t::DEADLINE);
	// Выполняем получение количества отбитых сроков
	const size_t counted = beats.load();
	// Выполняем ожидание, вдвое превышающее прежний срок отбоя
	this_thread::sleep_for(chrono::milliseconds(120));
	/**
	 * Выполняем проверку того, что поток прежнего отбоя срока остановлен: работа
	 * по наступлении срока при поверке при обращении не зовётся вовсе
	 */
	ASSERT_EQ(beats.load(), counted);
	// Выполняем проверку наступления срока поверкой при обращении
	ASSERT_TRUE(schedule.touch());
	// Выполняем остановку отбоя срока
	schedule.stop();
}
/**
 * @brief Проверка безобидности лишней остановки отбоя срока
 *
 * @details Остановка зовётся и деструктором, и запуском наново, оттого повторная
 * остановка неминуема и валить работу не вправе
 *
 */
TEST(ScheduleTest, StopIsHarmless) {
	// Отбой срока
	abc::schedule_t schedule;
	// Выполняем остановку незапущенного отбоя срока
	schedule.stop();
	// Выполняем проверку того, что отбой срока не работает
	ASSERT_FALSE(schedule.working());
	// Выполняем запуск отбоя срока своим потоком
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::THREAD, 20));
	// Выполняем остановку отбоя срока
	schedule.stop();
	// Выполняем повторную остановку остановленного отбоя срока
	schedule.stop();
	// Выполняем проверку того, что отбой срока не работает
	ASSERT_FALSE(schedule.working());
	/**
	 * Выполняем проверку того, что остановленный отбой срока запускается наново:
	 * поток отбоя заведён и снят, и второе заведение обязано пройти
	 */
	ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::THREAD, 20));
	// Выполняем проверку работы отбоя срока
	ASSERT_TRUE(schedule.working());
	// Выполняем остановку отбоя срока
	schedule.stop();
}
/**
 * @brief Проверка остановки отбоя срока деструктором
 *
 * @details Поток отбоя срока держит указатель на объект, и уход объекта из области
 * видимости без снятия потока обратил бы отбой в обращение к снесённому
 *
 */
TEST(ScheduleTest, DestructorStops) {
	// Количество отбитых сроков
	atomic <size_t> beats{0};
	/**
	 * Выполняем заведение отбоя срока в области видимости, откуда он уходит
	 * деструктором, а не остановкой
	 */
	{
		// Отбой срока
		abc::schedule_t schedule;
		// Выполняем установку работы, зовомой по наступлении срока
		schedule.callback([&beats]() noexcept -> void {
			// Выполняем увеличение количества отбитых сроков
			beats++;
		});
		// Выполняем запуск отбоя срока своим потоком
		ASSERT_TRUE(schedule.start(abc::schedule_t::mode_t::THREAD, 20));
		// Выполняем ожидание нескольких сроков подряд
		this_thread::sleep_for(chrono::milliseconds(120));
		// Выполняем проверку того, что срок отбит своим потоком
		ASSERT_GT(beats.load(), 0ul);
	}
	// Выполняем получение количества отбитых сроков
	const size_t counted = beats.load();
	// Выполняем ожидание, вдвое превышающее срок отбоя
	this_thread::sleep_for(chrono::milliseconds(120));
	// Выполняем проверку того, что снесённый отбой срока более не отбивает
	ASSERT_EQ(beats.load(), counted);
}
