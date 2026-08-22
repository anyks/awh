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

#endif // __AWH_TESTS_ALLOC__
