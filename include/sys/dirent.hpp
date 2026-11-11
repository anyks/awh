/**
 * @file: dirent.hpp
 * @date: 2026-11-11
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

#ifndef __AWH_DIRENT__
#define __AWH_DIRENT__

/**
 * Для оснастки MSVC, не несущей заголовка dirent.h
 *
 * Обход каталогов в проекте написан под имена оснастки MinGW: _WDIR, _wdirent,
 * _wopendir, _wreaddir и _wclosedir. У нативного MSVC их нет вовсе, а заголовок
 * dirent.h он не несёт. Слой вводится здесь и повторяет те же имена поверх
 * FindFirstFileW, чтобы ветвь Windows в src/sys/fs.cpp не переписывать под
 * другую оснастку
 *
 * Имена объявлены в общем пространстве имён: вызовы идут через ::, как и у
 * оснастки MinGW
 */
#if defined(_MSC_VER)
	/**
	 * Подключаем основные заголовочные файлы
	 */
	#include <windows.h>
	#include <cstdint>
	#include <cwchar>

	/**
	 * @brief Запись каталога
	 *
	 * @details Из полей записи проект читает одно - имя узла, потому несётся
	 *          только оно: прочие поля оснастки MinGW в разбор не идут
	 *
	 */
	struct _wdirent {
		// Имя узла каталога в широких знаках
		wchar_t d_name[MAX_PATH];
	};

	/**
	 * @brief Указатель обхода каталога
	 *
	 * @details Поиск системы отдаёт первую запись уже при открытии, потому
	 *          признак первой записи хранится здесь: без него первый узел
	 *          каталога терялся бы на каждом обходе
	 *
	 */
	struct _WDIR {
		// Указатель поиска системы
		HANDLE handle;
		// Сведения о найденном узле
		WIN32_FIND_DATAW data;
		// Запись, отдаваемая читающему
		struct _wdirent entry;
		// Признак первой записи обхода
		bool first;
	};

	/**
	 * @brief Функция открытия каталога для обхода
	 *
	 * @param address адрес каталога в широких знаках
	 * @return        указатель обхода либо nullptr при отказе
	 *
	 */
	_WDIR * _wopendir(const wchar_t * address);
	/**
	 * @brief Функция чтения очередной записи каталога
	 *
	 * @param dir указатель обхода каталога
	 * @return    запись каталога либо nullptr по его исчерпании
	 *
	 */
	struct _wdirent * _wreaddir(_WDIR * dir);
	/**
	 * @brief Функция закрытия обхода каталога
	 *
	 * @param dir указатель обхода каталога
	 * @return    ноль при успехе и -1 при отказе
	 *
	 */
	int32_t _wclosedir(_WDIR * dir);
#endif

#endif