/**
 * @file: io.cpp
 * @date: 2025-12-15
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"

/**
 * @brief Метод инициализации тестовой среды
 *
 */
void IoFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Создаём объект асинхронного движка ввода-вывода
	this->_io = std::make_unique <awh::engine::io_t> (this->_fmk.get(), this->_log.get());
	// Создаём объект транспортного уровня безопасности
	this->_coder = std::make_unique <awh::tls::coder_t> (this->_fmk.get(), this->_log.get());
	/**
	 * Для операционной системы Linux или FreeBSD
	 */
	#if __linux__ || __FreeBSD__
		// Объект управления SCTP протоколом
		this->_sctp = std::make_unique <awh::engine::sctp_t> (this->_fmk.get(), this->_log.get());
	#endif
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void IoFixture::TearDown() {}
