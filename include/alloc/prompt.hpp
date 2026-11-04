/**
 * @file prompt.hpp
 * @date 2026-09-15
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
 * \~russian
 * @brief Заголовочный файл приёма пароля с терминала без эха
 *
 * @section prompt_decisions Намеренные решения
 *
 * @details <b>Читается ТЕРМИНАЛ, а не поток ввода.</b> Поток ввода приложению вправе
 *          подменить кто угодно - конвейер, перенаправление из файла, чужой процесс, - и
 *          пароль тогда пришёл бы не от человека, а откуда придётся, да ещё и с
 *          включённым эхом. Оттого приём идёт прямо через устройство терминала
 *          (`/dev/tty` у POSIX, консоль у MS Windows), и отсутствие терминала есть
 *          ОТКАЗ, а не повод читать поток.
 *
 *          <b>Пароль ложится прямо в приёмник тайн.</b> Промежуточного вместилища нет
 *          вовсе: всякое перевыделение его оставляло бы в куче копию, какую никто не
 *          затирает. Ёмкость объявляется заранее - у ввода она известна, - и набор
 *          сверх неё отвечает отказом, а не растит приёмник.
 *
 *          <b>Эхо снимается, а не подменяется звёздочками.</b> Число знаков пароля само
 *          по себе сведение, и отдавать его наблюдателю через плечо незачем. Так же
 *          поступает `sudo`, и привычка эта у людей уже есть.
 *
 *          <b>Настройки терминала возвращаются деструктором.</b> Уход из приёма бывает
 *          и ранним, и по исключению, а терминал, оставленный без эха, делает
 *          непригодной всю сессию человека, а не только наше приложение.
 *
 *          <b>Отмены набора среди исходов НЕТ, и это установлено опытом.</b> Нажатие
 *          прерывания знаком до нас не доходит ни у одной системы: строчный разбор
 *          обращает его в сигнал, и процесс умирает, не дойдя до нашего разбора знаков.
 *          Замерено 15.09.2026 под псевдотерминалом - подача `0x03` убивает щуп посреди
 *          набора. Исход `CANCELLED` стоял здесь и был снят как недостижимый: путь к
 *          нему не замкнут ни на одной платформе, а не «замкнут, да редок».
 *
 *          Снять строчный разбор (`ICANON`), чтобы знак дошёл, мы не вправе: вместе с
 *          ним у человека уходит обычный выход из набора, и приложение перестаёт
 *          убиваться прерыванием.
 *
 *          <b>Чего приём НЕ покрывает.</b> Тот же сигнал деструктора не зовёт, и
 *          терминал остаётся без эха - лечится это `stty sane` у человека. Ставить ради
 *          того обработчик сигналов мы не вправе: у приложения может быть свой, и он
 *          важнее нашего.
 *
 * \~english
 * @brief Header file of the password intake from the terminal with the echo off
 *
 * @section prompt_decisions Intentional decisions
 *
 * @details <b>The TERMINAL is read, not the input stream.</b> The input stream may be
 *          replaced by anyone - a pipeline, a file redirect, a foreign process, - and
 *          the password would then come not from a person but from wherever, with the
 *          echo still on. Hence the intake goes directly through the terminal device
 *          (`/dev/tty` on POSIX, console on MS Windows), and the absence of a terminal
 *          is a REFUSAL, not a reason to read the stream.
 *
 *          <b>The password goes directly into the secret vessel.</b> There is no
 *          intermediate storage at all: every reallocation of it would leave a copy in
 *          the heap that nobody wipes. Capacity is declared in advance - at input it is
 *          known, - and input beyond it answers with a refusal, not by growing the
 *          vessel.
 *
 *          <b>Echo is removed, not replaced with asterisks.</b> The number of password
 *          characters is itself information, and there is no reason to hand it to an
 *          observer looking over the shoulder. `sudo` does the same, and people are
 *          already used to it.
 *
 *          <b>Terminal settings are returned by the destructor.</b> Exit from the intake
 *          can be early or by exception, and a terminal left without echo makes the
 *          person's entire session unusable, not just our application.
 *
 *          <b>There is no CANCELLED outcome, and this was established by experiment.</b>
 *          The interrupt key press does not reach us on any system: line editing turns it
 *          into a signal, and the process dies before our character parsing. Measured
 *          2026-09-15 under a pseudoterminal - feeding `0x03` kills the probe mid-input.
 *          The `CANCELLED` outcome stood here and was removed as unreachable: the path to
 *          it is not closed on any platform, not "closed but rare".
 *
 *          Removing line editing (`ICANON`) so the key would reach us is not allowed:
 *          together with it the person loses the usual way to exit input, and the
 *          application stops being killable by interrupt.
 *
 *          <b>What the intake does NOT cover.</b> The same signal does not run the
 *          destructor, and the terminal is left without echo - the person fixes this with
 *          `stty sane`. Installing a signal handler for that is not allowed: the
 *          application may have its own, and it matters more than ours.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "vessel.hpp"
#include "../sys/macro/global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Namespace of the memory allocator
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Пространство имён приёма пароля с терминала
		 *
		 * \~english
		 * @brief Namespace of the password intake from the terminal
		 *
		 * \~
		 */
		namespace prompt {
			/**
			 * \~russian
			 * @brief Исход приёма пароля
			 *
			 * \~english
			 * @brief Outcome of the password intake
			 *
			 * \~
			 */
			enum class result_t : uint8_t {
				ACCEPTED = 0x00, // Пароль набран и принят
				/**
				 * \~russian
				 * @brief Набор превысил ёмкость приёмника
				 *
				 * @details Имя взято ТОЛЬКО не `OVERFLOW`: тот занят макросом у `<math.h>`, и
				 *          перечисление с таким именем не собирается вовсе - собиратель видит
				 *          на его месте число три. Наш сличатель имён ловит ровно этот разряд.
				 *
				 * \~english
				 * @brief A set exceeded a capacity of a vessel
				 *
				 * @details A name is taken ONLY not `OVERFLOW`: that one is occupied by a macro at `<math.h>`, and
				 *          an enumeration with such a name does not build at all - a builder sees
				 *          a number three at its place. Our name comparator catches exactly this kind.
				 *
				 * \~
				 */
				TOOLONG = 0x01,
				NOTERM  = 0x02, // Терминала нет: читать неоткуда
				FAILED  = 0x03  // Приём не удался по вине окружения
			};
			/**
			 * \~russian
			 * @brief Функция приёма пароля с терминала без эха
			 *
			 * @note Приёмник обязан быть заведён ЗАРАНЕЕ: ёмкость его и есть верхняя
			 *       граница набора, и знает её зовущий, а не мы
			 *
			 * @note Прежнее содержимое приёмника снимается: приём идёт с начала
			 *
			 * @param vessel приёмник, куда ложится пароль
			 * @param hint   подсказка, печатаемая человеку, либо nullptr
			 * @return       исход приёма пароля
			 *
			 * \~english
			 * @brief Function of accepting a password from the terminal with the echo off
			 *
			 * @note The vessel must be created IN ADVANCE: its capacity is the upper bound of
			 *       input, and it is known to the caller, not to us
			 *
			 * @note The previous content of the vessel is removed: the intake starts from the beginning
			 *
			 * @param vessel vessel the password is put into
			 * @param hint   hint printed to the human, or nullptr
			 * @return       outcome of the password intake
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ result_t read(vessel_t & vessel, const char * hint = nullptr) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки наличия терминала
			 *
			 * @note Спрашивать полезно ДО заведения приёмника: нет терминала - нет и
			 *       смысла занимать защищённую память
			 *
			 * @return признак доступного терминала
			 *
			 * \~english
			 * @brief Function of checking whether a terminal is available
			 *
			 * @note It is useful to ask BEFORE creating the vessel: no terminal - no point
			 *       in taking protected memory
			 *
			 * @return flag of an available terminal
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool available() noexcept;
		}
	};
};
