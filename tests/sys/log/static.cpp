/**
 * @file static.cpp
 * @date 2025-12-12
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
 * @brief Статические тесты модуля логирования — проверка создания и сброса объекта модуля,
 *        а также корректности форматирования сообщений по уровням важности, работы приёмников вывода и ротации файлов
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "log.hpp"

/**
 * @brief Тесты удаления модуля логов
 *
 */
TEST_F(LogFixture, CreateLogTest){
	// Проверяем создание объекта логов
	ASSERT_TRUE(this->_log != nullptr);
	// Проверяем сброс объекта логов
	this->_log.reset();
	// Проверяем успешность сброса объекта логов
	ASSERT_TRUE(this->_log == nullptr);
}

/**
 * @brief Тесты пересоздания модуля логов
 *
 */
TEST_F(LogFixture, ResetAndCreateLogTest){
	// Проверяем создание объекта логов
	ASSERT_TRUE(this->_log != nullptr);
	// Проверяем сброс объекта логов
	this->_log.reset();
	// Проверяем успешность сброса объекта логов
	ASSERT_TRUE(this->_log == nullptr);
	// Создаём объект логов заново
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Проверяем создание объекта логов
	ASSERT_TRUE(this->_log != nullptr);
}

/**
 * @brief Тесты пересоздания модуля логов
 *
 */
TEST_F(LogFixture, ReCreateLogTest){
	// Проверяем создание объекта логов
	ASSERT_TRUE(this->_log != nullptr);
	// Создаём объект логов заново
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Проверяем создание объекта логов
	ASSERT_TRUE(this->_log != nullptr);
}

/**
 * @brief Тесты установки режимов логов
 *
 */
TEST_F(LogFixture, ModeLogTest){
	/**
	 * Устанавливаем режимы формирвоания логов
	 */
	this->_log->mode({
		awh::log_t::mode_t::FILE,
		awh::log_t::mode_t::SYSLOG,
		awh::log_t::mode_t::CONSOLE,
		awh::log_t::mode_t::DEFERRED
	});
	// Проверяем установленные режимы логов
	ASSERT_TRUE(this->_log->mode().size() == 4);
	/**
	 * Проверяем корректность установленных режимов логов
	 */
	for(auto & mode : this->_log->mode())
		// Проверяем корректность режима логов
		ASSERT_TRUE(
			(mode == awh::log_t::mode_t::FILE) ||
			(mode == awh::log_t::mode_t::SYSLOG) ||
			(mode == awh::log_t::mode_t::CONSOLE) ||
			(mode == awh::log_t::mode_t::DEFERRED)
		);
}

/**
 * @brief Тесты установки формата логов
 *
 */
TEST_F(LogFixture, FormatLogTest){
	// Устанавливаем формат лога
	this->_log->format("%a %h %e %Y %H:%M:%S");
	// Проверяем установленный формат лога
	ASSERT_EQ("%a %h %e %Y %H:%M:%S", this->_log->format());
}

/**
 * @brief Тесты других методов логов
 *
 */
TEST_F(LogFixture, OtherLogTest){
	// Активируем асинхронный режим работы логов
	this->_log->async(true);
	// Деактивируем асинхронный режим работы логов
	this->_log->async(false);
	// Устанавливаем название сервиса для вывода лога
	this->_log->name("anyks");
	// Устанавливаем максимальный размер файла логов
	this->_log->maxSize(4096);
	// Устанавливаем размер текста для формирования разделителя
	this->_log->sepSize(1024);
	// Устанавливаем уровень логирования
	this->_log->level(awh::log_t::level_t::ALL);
	// Устанавливаем путь к файлу для сохранения логов
	this->_log->filename("/tmp/test.log");
	// Устанавливаем разделитель сообщений логирования
	this->_log->separator(awh::log_t::separator_t::ALWAYS);
	// Проверяем успешное выполнение методов
	ASSERT_TRUE(true);
}
