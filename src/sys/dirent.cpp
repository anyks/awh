/**
 * @file: dirent.cpp
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

/**
 * Для оснастки MSVC, не несущей заголовка dirent.h
 */
#if defined(_MSC_VER)
	/**
	 * Подключаем заголовочные файлы
	 */
	#include <string>
	#include <cstdint>
	#include <sys/dirent.hpp>

	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Функция открытия каталога для обхода
	 *
	 * @param address адрес каталога в широких знаках
	 * @return        указатель обхода либо nullptr при отказе
	 *
	 */
	_WDIR * _wopendir(const wchar_t * address){
		// Если адрес каталога не задан
		if(address == nullptr)
			// Отказываем в открытии обхода
			return nullptr;
		// Образец поиска по каталогу
		wstring sample(address);
		// Если адрес не пуст и не кончается разделителем
		if(!sample.empty() && (sample.back() != L'\\') && (sample.back() != L'/'))
			// Дописываем разделитель перед образцом имени
			sample.push_back(L'\\');
		// Дописываем образец любого имени узла
		sample.append(L"*");
		// Сведения о первом найденном узле
		WIN32_FIND_DATAW data;
		// Открываем поиск по образцу
		HANDLE handle = ::FindFirstFileW(sample.c_str(), &data);
		// Если поиск открыть не удалось
		if(handle == INVALID_HANDLE_VALUE)
			// Отказываем в открытии обхода
			return nullptr;
		// Создаём указатель обхода каталога
		_WDIR * dir = new _WDIR();
		// Сохраняем указатель поиска системы
		dir->handle = handle;
		// Сохраняем сведения о первом узле
		dir->data = data;
		// Первую запись уже отдало открытие поиска
		dir->first = true;
		// Отдаём указатель обхода
		return dir;
	}

	/**
	 * @brief Функция чтения очередной записи каталога
	 *
	 * @param dir указатель обхода каталога
	 * @return    запись каталога либо nullptr по его исчерпании
	 *
	 */
	struct _wdirent * _wreaddir(_WDIR * dir){
		// Если указатель обхода не задан
		if(dir == nullptr)
			// Отказываем в чтении записи
			return nullptr;
		// Если запись не первая и следующий узел не найден
		if(!dir->first && !::FindNextFileW(dir->handle, &dir->data))
			// Сообщаем об исчерпании каталога
			return nullptr;
		// Первая запись отдана, дальше читает поиск
		dir->first = false;
		// Переносим имя узла в запись обхода
		::wcsncpy(dir->entry.d_name, dir->data.cFileName, (MAX_PATH - 1));
		/**
		 * Последний знак оставляем завершающим
		 *
		 * @note wcsncpy завершающий знак не дописывает, а имя узла длиннее
		 *       буфера обрезалось бы без него
		 *
		 */
		dir->entry.d_name[MAX_PATH - 1] = L'\0';
		// Отдаём запись каталога
		return &dir->entry;
	}

	/**
	 * @brief Функция закрытия обхода каталога
	 *
	 * @param dir указатель обхода каталога
	 * @return    ноль при успехе и -1 при отказе
	 *
	 */
	int32_t _wclosedir(_WDIR * dir){
		// Если указатель обхода не задан
		if(dir == nullptr)
			// Отказываем в закрытии обхода
			return -1;
		// Закрываем поиск системы
		::FindClose(dir->handle);
		// Освобождаем указатель обхода
		delete dir;
		// Сообщаем об успехе закрытия
		return 0;
	}
#endif