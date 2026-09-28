/**
 * @file static.cpp
 * @date 2025-12-13
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
 * @brief Статические тесты модуля работы с операционной системой — проверка создания и сброса объекта модуля,
 *        а также корректности определения семейства и версии ОС,
 *        получения сведений о процессоре и памяти и лимитов процесса
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <memory>

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
	this->_os = std::make_unique <awh::os_t> ();
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
	this->_os = std::make_unique <awh::os_t> ();
	// Проверяем что объект создан
	ASSERT_TRUE(this->_os != nullptr);
}

/**
 * @brief Метод определения названия операционной системы
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
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Unix" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Linux
		case static_cast <uint8_t> (awh::os_t::family_t::LINUX):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Linux" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Windows x32
		case static_cast <uint8_t> (awh::os_t::family_t::WIND32):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Windows x32" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Windows x64
		case static_cast <uint8_t> (awh::os_t::family_t::WIND64):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Windows x64" << std::endl;
		break;
		// Если операционная система принадлежит к семейству macOS
		case static_cast <uint8_t> (awh::os_t::family_t::MACOSX):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: macOS" << std::endl;
		break;
		// Если операционная система принадлежит к семейству NetBSD
		case static_cast <uint8_t> (awh::os_t::family_t::NETBSD):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: NetBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству OpenBSD
		case static_cast <uint8_t> (awh::os_t::family_t::OPENBSD):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: OpenBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству FreeBSD
		case static_cast <uint8_t> (awh::os_t::family_t::FREEBSD):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: FreeBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Solaris
		case static_cast <uint8_t> (awh::os_t::family_t::SOLARIS):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Solaris" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Illumos
		case static_cast <uint8_t> (awh::os_t::family_t::ILLUMOS):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Illumos" << std::endl;
		break;
	}
}

/**
 * @brief Метод определения архитектуры процессора
 *
 */
TEST_F(OSFixture, ArchitectureOSTest){
	// Если объект работы с ОС создан
	ASSERT_TRUE(this->_os != nullptr);
	/**
	 * Определяем архитектуру процессора
	 */
	switch(static_cast <uint8_t> (this->_os->architecture())){
		// Если архитектура процессора принадлежит к i386
		case static_cast <uint8_t> (awh::os_t::cpu_t::X86):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: x86" << std::endl;
		break;
		// Если архитектура процессора принадлежит к ARM32
		case static_cast <uint8_t> (awh::os_t::cpu_t::ARM):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: ARM" << std::endl;
		break;
		// Если архитектура процессора принадлежит к PowerPC
		case static_cast <uint8_t> (awh::os_t::cpu_t::PPC):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: PowerPC" << std::endl;
		break;
		// Если архитектура процессора принадлежит к MIPS
		case static_cast <uint8_t> (awh::os_t::cpu_t::MIPS):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: MIPS" << std::endl;
		break;
		// Если архитектура процессора принадлежит к ARM64
		case static_cast <uint8_t> (awh::os_t::cpu_t::ARM64):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: ARM64" << std::endl;
		break;
		// Если архитектура процессора принадлежит к x86_64
		case static_cast <uint8_t> (awh::os_t::cpu_t::AMD64):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: x86_64" << std::endl;
		break;
		// Если архитектура процессора принадлежит к Эльбрус
		case static_cast <uint8_t> (awh::os_t::cpu_t::E2K):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: E2K" << std::endl;
		break;
		// Если архитектура процессора не определена
		case static_cast <uint8_t> (awh::os_t::cpu_t::UNKNOWN):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: Unknown" << std::endl;
		break;
	}
}

/**
 * Для операционных систем семейства Unix
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Системные заголовочные файлы
	 */
	#include <grp.h>

	/**
	 * @brief Нуль - законный идентификатор, отказ - (uid_t) -1
	 *
	 * @details Прежде отказ разрешения имени тоже был нулём, и отличить его от root
	 *          (и от группы с идентификатором 0: wheel в BSD и macOS, root в Linux) было
	 *          нельзя: chown к root молча не выполнялся, а к несуществующему пользователю
	 *          отказывал без сообщения о причине
	 */
	TEST_F(OSFixture, IdentifierNotFoundTest){
		// Если объект работы с ОС создан
		ASSERT_TRUE(this->_os != nullptr);
		// Название группы с идентификатором 0
		const struct group * zero = ::getgrgid(0);
		// Группа с идентификатором 0 обязана быть
		ASSERT_TRUE(zero != nullptr);
		// Идентификатор root - нуль
		ASSERT_EQ(this->_os->uid("root"), static_cast <uid_t> (0));
		// Идентификатор группы 0 - нуль
		ASSERT_EQ(this->_os->group(zero->gr_name), static_cast <gid_t> (0));
		// Несуществующий пользователь - отказ
		ASSERT_EQ(this->_os->uid("no-such-user-zz"), static_cast <uid_t> (-1));
		// Группа несуществующего пользователя - отказ
		ASSERT_EQ(this->_os->gid("no-such-user-zz"), static_cast <gid_t> (-1));
		// Несуществующая группа - отказ
		ASSERT_EQ(this->_os->group("no-such-group-zz"), static_cast <gid_t> (-1));
	}
#endif
