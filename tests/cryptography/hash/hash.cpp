/**
 * @file: hash.cpp
 * @date: 2026-07-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля хэширования — создание объектов тестового
 *        окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "hash.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void HashFixture::SetUp(){
	// Создаём объект хэширования данных
	this->_hash = std::make_unique <awh::hash_t> ();
	// Выделяем память под буфер данных для хэширования
	this->_buffer.resize(4096, 0);
	/**
	 * Выполняем заполнение буфера данных для хэширования
	 */
	for(size_t i = 0; i < this->_buffer.size(); i++)
		// Заполняем очередной байт буфера данных
		this->_buffer[i] = static_cast <uint8_t> ((i * 131) ^ (i >> 3));
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void HashFixture::TearDown(){
	// Выполняем очистку буфера данных для хэширования
	this->_buffer.clear();
}
