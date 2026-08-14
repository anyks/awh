/**
 * @file io.cpp
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
 * @brief Реализация тестовой фикстуры асинхронного движка ввода-вывода —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2025
 *
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
	 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
	 */
	#if __linux__ || __FreeBSD__ || __sun
		// Объект управления SCTP протоколом
		this->_sctp = std::make_unique <awh::engine::sctp_t> (this->_fmk.get(), this->_log.get());
	#endif
}

/**
 * @brief Метод очистки тестовой среды
 *
 */
void IoFixture::TearDown(){
	/**
	 * Возвращаем тип внутренних таймеров к простому.
	 *
	 * Тип этот хранится не в объекте движка, а в переменной процесса - движок
	 * задуман единственным на процесс. Оттого проверка, переключившая таймеры
	 * на сложные, оставляет их такими всем последующим, и переключение это
	 * переживает даже собственный пропуск проверки: IoSuiteTest ставил
	 * DIFFICULT до GTEST_SKIP, а IoBandwidthDuplexTest затем валился вдвое
	 * меньшей скоростью - отказ выглядел плавающим, хотя был предрешён
	 * порядком проверок
	 */
	if(this->_io != nullptr)
		// Устанавливаем простой тип таймера для событий сетевого движка
		this->_io->setInternalTimer(awh::event::timer_t::SIMPLE);
}
