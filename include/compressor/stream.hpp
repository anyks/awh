/**
 * @file: stream.hpp
 * @date: 2026-07-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля потоковой компрессии — публичный API класса compressor::Stream, выполняющего
 *        инкрементальное сжатие и распаковку данных по мере их поступления с управлением временем жизни тяжёлого
 *        контекста движка и финализацией потока
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_COMPRESSOR_STREAM__
#define __AWH_COMPRESSOR_STREAM__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "types.hpp"
#include "../sys/log.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён компрессора
	 *
	 */
	namespace compressor {
		/**
		 * @brief Внутренний бэкенд потокового движка (объявление, реализация скрыта)
		 *
		 */
		class coder_t;
		/**
		 * @brief Класс потоковой (streaming) компрессии/декомпрессии данных
		 *
		 * @details Один объект обслуживает один поток/соединение и не является
		 *          потокобезопасным. Тяжёлый контекст движка жив только от создания
		 *          объекта до финализации (finish) или разрушения.
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Stream {
			private:
				// Направление операции (компрессия/декомпрессия)
				event_t _event;
				// Метод компрессии
				method_t _method;
			private:
				// Живой контекст движка (nullptr => поток невалиден)
				unique_ptr <coder_t> _coder;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод проверки завершённости потока
				 *
				 * @details Для невалидного потока метод возвращает true: обрабатывать больше нечего.
				 *          Чтобы отличить корректно финализированный поток от сломанного (после
				 *          ошибки в push контекст освобождается), следует дополнительно проверить valid().
				 *
				 * @return true, если поток финализирован (после finish) либо невалиден
				 *
				 */
				bool done() const noexcept;
				/**
				 * @brief Метод проверки валидности потока
				 *
				 * @return true, если движок поддержал потоковый режим и контекст создан
				 *
				 */
				bool valid() const noexcept;
			public:
				/**
				 * @brief Метод получения направления операции потока
				 *
				 * @return направление операции
				 *
				 */
				event_t event() const noexcept;
				/**
				 * @brief Метод получения метода компрессии потока
				 *
				 * @return метод компрессии
				 *
				 */
				method_t method() const noexcept;
			public:
				/**
				 * @brief Шаблон метода принудительного выдавливания накопленного
				 *
				 * @tparam T тип контейнера результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод принудительного выдавливания накопленного (SYNC-flush)
				 *
				 * @param result контейнер, куда помещается готовый выход
				 *
				 */
				void flush(T & result) noexcept;
				/**
				 * @brief Шаблон метода финализации потока
				 *
				 * @tparam T тип контейнера результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод финализации потока (дожать хвост, завершить)
				 *
				 * @param result контейнер, куда помещается остаток данных
				 *
				 */
				void finish(T & result) noexcept;
			public:
				/**
				 * @brief Шаблон метода подачи порции данных в поток
				 *
				 * @tparam T тип контейнера результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод подачи порции данных в поток
				 *
				 * @param buffer буфер данных для обработки
				 * @param result контейнер, куда помещается готовый выход этой порции
				 * @param flush  режим сброса данных
				 *
				 */
				void push(string_view buffer, T & result, const flush_t flush = flush_t::NONE) noexcept;
				/**
				 * @brief Шаблон метода подачи порции данных в поток
				 *
				 * @tparam T тип контейнера результата
				 *
				 */
				template <typename T>
				/**
				 * @brief Метод подачи порции данных в поток
				 *
				 * @param buffer буфер данных для обработки
				 * @param size   размер данных для обработки
				 * @param result контейнер, куда помещается готовый выход этой порции
				 * @param flush  режим сброса данных
				 *
				 */
				void push(const void * buffer, const size_t size, T & result, const flush_t flush = flush_t::NONE) noexcept;
			public:
				/**
				 * @brief Оператор перемещения
				 *
				 */
				Stream & operator = (Stream &&) noexcept;
				/**
				 * @brief Запрещаем копирование
				 *
				 * @return результат операции
				 *
				 */
				Stream & operator = (const Stream &) = delete;
			public:
				/**
				 * @brief Конструктор перемещения
				 *
				 */
				explicit Stream(Stream &&) noexcept;
				/**
				 * @brief Запрещаем копирование
				 *
				 */
				explicit Stream(const Stream &) = delete;
			public:
				/**
				 * @brief Конструктор пустого (невалидного) потока
				 *
				 */
				explicit Stream() noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param method метод компрессии
				 * @param event  направление операции
				 * @param params параметры инициализации
				 * @param log    объект для работы с логами
				 *
				 */
				explicit Stream(const method_t method, const event_t event, const params_t & params, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Stream() noexcept;
		} stream_t;
	};
};

#endif // __AWH_COMPRESSOR_STREAM__
