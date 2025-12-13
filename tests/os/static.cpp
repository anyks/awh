/**
 * @file: static.cpp
 * @date: 2025-12-13
 * @license: GPL-3.0
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
#include "os.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
TEST_F(OSFixture, CreateOSTest){
	// Если объект работы с ОС создан
	ASSERT_TRUE(this->_os != nullptr);
	// Выполняем сброс объекта
	this->_os.reset();
	// Проверяем что объект сброшен
	ASSERT_TRUE(this->_os == nullptr);
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
TEST_F(OSFixture, ResetAndCreateOSTest){
	// Если объект работы с ОС создан
	ASSERT_TRUE(this->_os != nullptr);
	// Выполняем сброс объекта
	this->_os.reset();
	// Проверяем что объект сброшен
	ASSERT_TRUE(this->_os == nullptr);
	// Создаём объект работы с ОС
	this->_os = std::make_unique <awh::os_t> (this->_log.get());
	// Проверяем что объект создан
	ASSERT_TRUE(this->_os != nullptr);
}

/**
 * @brief Метод повторного создания объекта работы с ОС
 *
 */
TEST_F(OSFixture, ReCreateOSTest){
	// Если объект работы с ОС создан
	ASSERT_TRUE(this->_os != nullptr);
	// Создаём объект работы с ОС
	this->_os = std::make_unique <awh::os_t> (this->_log.get());
	// Проверяем что объект создан
	ASSERT_TRUE(this->_os != nullptr);
}

/**
 * @brief Тестирование методов работы с ОС
 *
 */
TEST_F(OSFixture, OSTest){
	// Если объект работы с ОС создан
	ASSERT_TRUE(this->_os != nullptr);
	/**
	 * Определяем семейство операционной системы
	 */
	switch(static_cast <uint8_t> (this->_os->family())){
		// Если операционная система принадлежит к семейству Unix
		case static_cast <uint8_t> (awh::os_t::family_t::UNIX):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: Unix" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Linux
		case static_cast <uint8_t> (awh::os_t::family_t::LINUX):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: Linux" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Windows x32
		case static_cast <uint8_t> (awh::os_t::family_t::WIND32):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: Windows x32" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Windows x64
		case static_cast <uint8_t> (awh::os_t::family_t::WIND64):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: Windows x64" << std::endl;
		break;
		// Если операционная система принадлежит к семейству MacOS X
		case static_cast <uint8_t> (awh::os_t::family_t::MACOSX):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: MacOS X" << std::endl;
		break;
		// Если операционная система принадлежит к семейству NetBSD
		case static_cast <uint8_t> (awh::os_t::family_t::NETBSD):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: NetBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству OpenBSD
		case static_cast <uint8_t> (awh::os_t::family_t::OPENBSD):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: OpenBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству FreeBSD
		case static_cast <uint8_t> (awh::os_t::family_t::FREEBSD):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: FreeBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Solaris
		case static_cast <uint8_t> (awh::os_t::family_t::SOLARIS):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: Solaris" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Illumos
		case static_cast <uint8_t> (awh::os_t::family_t::ILLUMOS):
			// Выводим информацию в стандартный вывод
			std::cout << "OS: Illumos" << std::endl;
		break;
	}
}
