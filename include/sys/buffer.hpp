/**
 * @file: buffer.hpp
 * @date: 2024-03-31
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_WEB_BUFFER__
#define __AWH_WEB_BUFFER__

/**
 * Стандартные модули
 */
#include <vector>
#include <memory>
#include <string>
#include <cstring>

/**
 * Разрешаем сборку под Windows
 */
#include <sys/global.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Buffer Клас динамического смарт-буфера
	 */
	typedef class AWHSHARED_EXPORT Buffer {
		public:
			/**
			 * Режим работы буфера
			 */
			enum class mode_t : uint8_t {
				COPY    = 0x01, // Копировать полученные данные
				NO_COPY = 0x02  // Не копировать полученные данные
			};
		public:
			// Создаём новый тип данных буфера
			typedef const char * data_t;
		private:
			// Режим работы буфера
			mode_t _mode;
		private:
			// Размер буфера
			size_t _size;
			// Смещение в буфере
			size_t _offset;
		private:
			// Данные входящего буфера
			data_t _buffer;
		private:
			// Постоянный буфер ожидания
			vector <char> _data;
		private:
			// Временный буфер входящих данных
			std::unique_ptr <char []> _tmp;
		public:
			/**
			 * clear Метод очистки буфера данных
			 */
			void clear() noexcept;
		public:
			/**
			 * empty Метод проверки на пустоту буфера
			 * @return результат проверки
			 */
			bool empty() const noexcept;
		public:
			/**
			 * size Метод получения размера данных в буфере
			 * @return размер данных в буфере
			 */
			size_t size() const noexcept;
		public:
			/**
			 * data Метод получения бинарных данных буфера
			 * @return бинарные данные буфера
			 */
			data_t data() const noexcept;
		public:
			/**
			 * commit Метод фиксации изменений в буфере для перехода к следующей итерации
			 */
			void commit() noexcept;
		public:
			/**
			 * erase Метод удаления из буфера байт
			 * @param size количество байт для удаления
			 */
			void erase(const size_t size) noexcept;
		public:
			/**
			 * emplace Метод добавления нового сырого буфера
			 * @param buffer бинарный буфер данных для добавления
			 * @param size   размер бинарного буфера данных
			 */
			void emplace(data_t buffer, const size_t size) noexcept;
		public:
			/**
			 * operator Получения размера данных в буфере
			 * @return размер данных в буфере
			 */
			operator size_t() const noexcept;
			/**
			 * operator Получения бинарных данных буфера
			 * @return бинарные данные буфера
			 */
			operator data_t() const noexcept;
			/**
			 * operator Получения режима работы буфера
			 * @return режим работы буфера
			 */
			operator mode_t() const noexcept;
		public:
			/**
			 * Оператор [=] установки режима работы буфера
			 * @param mode режим работы буфера для установки
			 * @return     текущий объект
			 */
			Buffer & operator = (const mode_t mode) noexcept;
		public:
			/**
			 * Buffer Конструктор
			 */
			Buffer() noexcept : _mode(mode_t::NO_COPY), _size(0), _offset(0), _buffer(nullptr), _tmp(nullptr) {}
			/**
			 * Buffer Конструктор
			 * @param mode режим работы буфера
			 */
			Buffer(const mode_t mode) noexcept : _mode(mode), _size(0), _offset(0), _buffer(nullptr), _tmp(nullptr) {}
			/**
			 * ~Buffer Деструктор
			 */
			~Buffer() noexcept {}
	} buffer_t;
};

#endif // __AWH_WEB_BUFFER__
