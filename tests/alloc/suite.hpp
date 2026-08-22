/**
 * @file suite.hpp
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
 * @brief Общий заголовочный файл набора тестов распределителя памяти
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_TESTS_ALLOC__
#define __AWH_TESTS_ALLOC__

/**
 * Признак сборки под санитайзером
 *
 * Санитайзеры подменяют выдачу памяти собою, и уступить её нам не могут: захват там
 * отвечает отказом. Проверять нашу выдачу под ними нечего - её попросту нет, - и
 * набор утверждает ровно это, а не молчит
 */
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
	#define AWH_ALLOC_SANITIZED 1
#elif defined(__has_feature)
	#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
		#define AWH_ALLOC_SANITIZED 1
	#else
		#define AWH_ALLOC_SANITIZED 0
	#endif
#else
	#define AWH_ALLOC_SANITIZED 0
#endif

/**
 * @brief Метод получения признака состоявшегося захвата
 *
 * @return признак состоявшегося захвата выдачи памяти процесса
 *
 */
bool __awh_alloc_captured__() noexcept;

/**
 * Подключаем наши модули
 */
#include <alloc/alloc.hpp>

/**
 * @brief Класс фикстуры проверок распределителя памяти
 *
 * @note Настройки распределителя живут в переменных ПРОЦЕССА, а набор идёт одним
 *       процессом: настройка, заданная одной проверкой и не снятая после, досталась бы
 *       соседней и сделала бы её отказ невоспроизводимым. Оттого фикстура снимает
 *       слепок настроек до проверки и возвращает его после
 *
 */
class AllocFixture : public testing::Test {
	protected:
		// Слепок настроек распределителя до проверки
		awh::alloc::options_t _restore;
	protected:
		/**
		 * @brief Метод инициализации тестовой среды
		 *
		 */
		void SetUp() override {
			// Снимаем слепок действующих настроек
			this->_restore = awh::alloc::Allocator::options();
		}
		/**
		 * @brief Метод очистки тестовой среды
		 *
		 */
		void TearDown() override {
			// Возвращаем настройки, действовавшие до проверки
			awh::alloc::Allocator::options(this->_restore);
		}
};

#endif // __AWH_TESTS_ALLOC__
