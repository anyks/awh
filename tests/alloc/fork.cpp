/**
 * @file fork.cpp
 * @date 2026-08-22
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
 * @brief Тесты ветвления процесса: потомок обязан выдавать память, а не вставать
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл главного модуля тестов
 */
#include "../main.hpp"

/**
 * Подключаем заголовочный файл набора
 */
#include "suite.hpp"

/**
 * Стандартные модули
 */
#include <vector>
#include <thread>
#include <atomic>
#include <cstdlib>

/**
 * Ветвление процесса есть лишь у систем Unix
 *
 * У MS Windows его нет вовсе: кластер порождает отдельные процессы, каждый со своим
 * распределителем, - приводить в порядок там нечего
 */
#if !defined(_WIN32) && !defined(_WIN64) && !AWH_ALLOC_SANITIZED

/**
 * Стандартные заголовочные файлы
 */
#include <unistd.h>
#include <sys/wait.h>

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод работы потомка после ветвления
	 *
	 * @note Ответ идёт КОДОМ ВОЗВРАТА, а не печатью: буфер печати достаётся потомку по
	 *       наследству и при `_exit` выбрасывается несброшенным - потомок, печатавший
	 *       ответ, выглядит молчащим
	 *
	 * @return код возврата потомка
	 *
	 */
	static int descendant() noexcept {
		// Выданные блоки
		std::vector <void *> alive;
		/**
		 * Перебираем блоки нагрузки
		 */
		for(size_t i = 0; i < 4096; i++){
			// Выдаём блок изменчивого размера
			void * block = ::malloc(64 + (i % 1024));
			// Если выдать память не вышло
			if(block == nullptr)
				// Отвечаем отказом выдачи
				return 2;
			// Записываем блок
			::memset(block, static_cast <int> (i & 0xFF), 64);
			// Запоминаем выданный блок
			alive.push_back(block);
		}
		/**
		 * Перебираем выданные блоки
		 */
		for(size_t i = 0; i < alive.size(); i++){
			// Получаем содержимое блока
			const uint8_t * bytes = reinterpret_cast <const uint8_t *> (alive[i]);
			// Содержимое обязано уцелеть
			if(bytes[0] != static_cast <uint8_t> (i & 0xFF))
				// Отвечаем порчей содержимого
				return 3;
			// Освобождаем блок
			::free(alive[i]);
		}
		// Отвечаем успехом
		return 0;
	}
	/**
	 * @brief Метод ожидания потомка
	 *
	 * @param pid    номер процесса потомка
	 * @param status код возврата потомка
	 * @return       признак дождавшегося родителя
	 *
	 */
	static bool await(const pid_t pid, int32_t & status) noexcept {
		// Выводим признак дождавшегося родителя
		return (::waitpid(pid, &status, 0) == pid);
	}
};

/**
 * @brief Тест выдачи памяти потомком после ветвления
 *
 * @note Ветвление переносит СОСТОЯНИЕ, но не ПОТОКИ: замок, захваченный не пережившим
 *       ветвление потоком, остался бы захваченным навсегда, и первая же выдача памяти
 *       у потомка встала бы намертво. Оттого зависание здесь обязано кончаться отказом
 *       проверки, а не вечным ожиданием - его обрывает будильник потомка
 *
 */
TEST_F(AllocFixture, DescendantAllocatesAfterFork){
	// Ветвим процесс
	const pid_t pid = ::fork();
	// Ветвление обязано удаться
	ASSERT_NE(pid, -1);
	// Если это потомок
	if(pid == 0){
		/**
		 * Заводим будильник
		 *
		 * Заклинивший замок вешает потомка навсегда, и родитель ждал бы его вечно:
		 * будильник обращает зависание в отказ проверки
		 */
		::alarm(30);
		// Отвечаем родителю кодом возврата
		::_exit(::descendant());
	}
	// Код возврата потомка
	int32_t status = 0;
	// Родитель обязан дождаться потомка
	ASSERT_TRUE(::await(pid, status));
	// Потомок обязан завершиться сам, а не быть снятым сигналом
	ASSERT_TRUE(WIFEXITED(status)) << "потомок снят сигналом: зависание либо падение";
	// Потомок обязан ответить успехом
	EXPECT_EQ(WEXITSTATUS(status), 0);
}
/**
 * @brief Тест ветвления при работающих потоках
 *
 * @note Случай этот и есть опасный: потоки, бравшие замки распределителя прямо в миг
 *       ветвления, потомку не достаются, а взятые ими замки - достаются
 *
 */
TEST_F(AllocFixture, DescendantAllocatesAfterForkUnderLoad){
	// Признак работы потоков нагрузки
	std::atomic <bool> running(true);
	// Потоки нагрузки
	std::vector <std::thread> workers;
	/**
	 * Заводим потоки нагрузки
	 */
	for(size_t i = 0; i < 4; i++){
		// Заводим поток нагрузки
		workers.emplace_back([&running]() noexcept {
			/**
			 * Держим выдачу памяти занятой, пока родитель ветвится
			 */
			while(running.load(std::memory_order_relaxed)){
				// Выдаём блок
				void * block = ::malloc(4096);
				// Освобождаем блок
				::free(block);
			}
		});
	}
	// Ветвим процесс под нагрузкой
	const pid_t pid = ::fork();
	// Если это потомок
	if(pid == 0){
		// Заводим будильник: заклинивший замок иначе вешает потомка навсегда
		::alarm(30);
		// Отвечаем родителю кодом возврата
		::_exit((pid == -1) ? 4 : ::descendant());
	}
	// Останавливаем потоки нагрузки
	running.store(false, std::memory_order_relaxed);
	/**
	 * Перебираем потоки нагрузки
	 */
	for(auto & worker : workers)
		// Дожидаемся потока нагрузки
		worker.join();
	// Ветвление обязано удаться
	ASSERT_NE(pid, -1);
	// Код возврата потомка
	int32_t status = 0;
	// Родитель обязан дождаться потомка
	ASSERT_TRUE(::await(pid, status));
	// Потомок обязан завершиться сам, а не быть снятым сигналом
	ASSERT_TRUE(WIFEXITED(status)) << "потомок снят сигналом: замок пережил ветвление захваченным";
	// Потомок обязан ответить успехом
	EXPECT_EQ(WEXITSTATUS(status), 0);
}

#endif // !_WIN32 && !AWH_ALLOC_SANITIZED
