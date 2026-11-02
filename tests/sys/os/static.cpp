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
 * @brief Статические тесты модуля работы с операционной системой — проверка определения
 *        семейства и архитектуры, а также корректности разрешения имён пользователей и групп
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "os.hpp"

/**
 * @brief Проверка определения названия операционной системы
 *
 */
TEST_F(OSFixture, OSTest){
	/**
	 * Определяем семейство операционной системы
	 */
	switch(static_cast <uint8_t> (awh::os::family())){
		// Если операционная система принадлежит к семейству Unix
		case static_cast <uint8_t> (awh::os::family_t::UNIX):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Unix" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Linux
		case static_cast <uint8_t> (awh::os::family_t::LINUX):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Linux" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Windows x32
		case static_cast <uint8_t> (awh::os::family_t::WIND32):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Windows x32" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Windows x64
		case static_cast <uint8_t> (awh::os::family_t::WIND64):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Windows x64" << std::endl;
		break;
		// Если операционная система принадлежит к семейству macOS
		case static_cast <uint8_t> (awh::os::family_t::MACOSX):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: macOS" << std::endl;
		break;
		// Если операционная система принадлежит к семейству NetBSD
		case static_cast <uint8_t> (awh::os::family_t::NETBSD):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: NetBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству OpenBSD
		case static_cast <uint8_t> (awh::os::family_t::OPENBSD):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: OpenBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству FreeBSD
		case static_cast <uint8_t> (awh::os::family_t::FREEBSD):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: FreeBSD" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Solaris
		case static_cast <uint8_t> (awh::os::family_t::SOLARIS):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Solaris" << std::endl;
		break;
		// Если операционная система принадлежит к семейству Illumos
		case static_cast <uint8_t> (awh::os::family_t::ILLUMOS):
			// Записываем в лог информацию о текущей операционной системе
			std::cout << "OS: Illumos" << std::endl;
		break;
	}
}

/**
 * @brief Проверка определения архитектуры процессора
 *
 */
TEST_F(OSFixture, ArchitectureOSTest){
	/**
	 * Определяем архитектуру процессора
	 */
	switch(static_cast <uint8_t> (awh::os::architecture())){
		// Если архитектура процессора принадлежит к i386
		case static_cast <uint8_t> (awh::os::cpu_t::X86):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: x86" << std::endl;
		break;
		// Если архитектура процессора принадлежит к ARM32
		case static_cast <uint8_t> (awh::os::cpu_t::ARM):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: ARM" << std::endl;
		break;
		// Если архитектура процессора принадлежит к PowerPC
		case static_cast <uint8_t> (awh::os::cpu_t::PPC):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: PowerPC" << std::endl;
		break;
		// Если архитектура процессора принадлежит к MIPS
		case static_cast <uint8_t> (awh::os::cpu_t::MIPS):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: MIPS" << std::endl;
		break;
		// Если архитектура процессора принадлежит к ARM64
		case static_cast <uint8_t> (awh::os::cpu_t::ARM64):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: ARM64" << std::endl;
		break;
		// Если архитектура процессора принадлежит к x86_64
		case static_cast <uint8_t> (awh::os::cpu_t::AMD64):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: x86_64" << std::endl;
		break;
		// Если архитектура процессора принадлежит к Эльбрус
		case static_cast <uint8_t> (awh::os::cpu_t::E2K):
			// Записываем в лог информацию о текущем прцоессоре
			std::cout << "CPU: E2K" << std::endl;
		break;
		// Если архитектура процессора не определена
		case static_cast <uint8_t> (awh::os::cpu_t::UNKNOWN):
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
	 *
	 */
	TEST_F(OSFixture, IdentifierNotFoundTest){
		// Название группы с идентификатором 0
		const struct group * zero = ::getgrgid(0);
		// Группа с идентификатором 0 обязана быть
		ASSERT_TRUE(zero != nullptr);
		// Идентификатор root - нуль
		ASSERT_EQ(awh::os::uid("root"), static_cast <uid_t> (0));
		// Идентификатор группы 0 - нуль
		ASSERT_EQ(awh::os::group(zero->gr_name), static_cast <gid_t> (0));
		// Несуществующий пользователь - отказ
		ASSERT_EQ(awh::os::uid("no-such-user-zz"), static_cast <uid_t> (-1));
		// Группа несуществующего пользователя - отказ
		ASSERT_EQ(awh::os::gid("no-such-user-zz"), static_cast <gid_t> (-1));
		// Несуществующая группа - отказ
		ASSERT_EQ(awh::os::group("no-such-group-zz"), static_cast <gid_t> (-1));
	}
#endif
