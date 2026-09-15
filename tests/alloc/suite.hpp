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

#pragma once

/**
 * Признак выдачи, идущей МИМО нас
 *
 * Значит он не «набор собран под санитайзером», а «выдачу забрал себе надзиратель»:
 * первое само по себе второго не влечёт, и слипание их стоило набору охвата.
 *
 * У macOS выдача под надзирателем и вправду не наша: подмену имён модуль там снимает
 * САМ (довод при `AWH_ALLOC_SANITIZER` в src/alloc/alloc.cpp), а зону забирает
 * надзиратель. Проверять нашу выдачу там нечего - её попросту нет, - и набор
 * утверждает ровно это, а не молчит.
 *
 * У систем ELF всё иначе: подмена идёт связыванием, надзиратель её не отменяет, и
 * выдачу по-прежнему ведём МЫ. Условием тут стоял один лишь надзиратель, и набор под
 * ним ждал чужой выдачи там, где она своя. Прежде расхождение это не всплывало: модули
 * распределителя собирались надзираемыми, процесс валился прежде первой проверки, и
 * увидеть ожидание было негде. Сняв надзор с одних лишь этих модулей (довод при
 * `set_source_files_properties` в CMakeLists.txt), набор дошёл до конца и ожидание
 * вскрылось - 15.09.2026 на Debian 12, `resolve()` отвечал нашими 1024 там, где
 * ожидался нуль
 */
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
	#define AWH_ALLOC_SANITIZER 1
#elif defined(__has_feature)
	#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
		#define AWH_ALLOC_SANITIZER 1
	#else
		#define AWH_ALLOC_SANITIZER 0
	#endif
#else
	#define AWH_ALLOC_SANITIZER 0
#endif

#if (defined(__APPLE__) || defined(__MACH__)) && AWH_ALLOC_SANITIZER
	#define AWH_ALLOC_SANITIZED 1
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
