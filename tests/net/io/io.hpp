/**
 * @file io.hpp
 * @date 2025-12-15
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
 * @brief Заголовочный файл тестовой фикстуры асинхронного движка ввода-вывода —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2025
 *
 */
 
#ifndef __AWH_IO_TESTS__
#define __AWH_IO_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/net/io.hpp"
#include "../../../include/net/addr.hpp"
#include "../../../include/sys/fs.hpp"
#include "../../../include/cryptography/tls/coder.hpp"

/**
 * @brief Класс фикстуры для тестов асинхронного движка ввода-вывода
 *
 */
class IoFixture : public testing::Test {
	protected:
		
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект асинхронного движка ввода-вывода
		std::unique_ptr <awh::engine::io_t> _io;
		// Объект транспортного уровня безопасности
		std::unique_ptr <awh::tls::coder_t> _coder;
		/**
		 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
		 */
		#if __linux__ || __FreeBSD__ || __sun
			// Объект управления SCTP протоколом
			std::unique_ptr <awh::engine::sctp_t> _sctp;
		#endif
	public:
		/**
		 * @brief Метод инициализации тестовой среды
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестовой среды
		 *
		 */
		void TearDown();
};

#endif // __AWH_IO_TESTS__
