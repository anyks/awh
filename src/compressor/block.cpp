/**
 * @file: compressor.cpp
 * @date: 2026-01-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Заголовочные файлы для работы с LZ4
 */
#include <lz4.h>
#include <lz4hc.h>

/**
 * Заголовочный файл для работы с GZip
 */
#include <zlib.h>

/**
 * Заголовочный файл для работы с Zstandard
 */
#include <zstd.h>

/**
 * Заголовочный файл для работы с BZip2
 */
#include <bzlib.h>

/**
 * Заголовочный файл для работы с LZma
 */
#include <lzma.h>

/**
 * Заголовочный файл для работы с Snappy
 */
#include <snappy.h>

/**
 * Заголовочный файл для работы с Density
 */
#include <density_api.h>

/**
 * Заголовочные файлы для работы с Brotli
 */
#include <brotli/decode.h>
#include <brotli/encode.h>

/**
 * Заголовочные файлы для работы с Lizard
 */
#include "lizard_compress.h"
#include "lizard_decompress.h"

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstring>
#include <csignal>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <compressor/block.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Полное определение непрозрачного контекста потока GZip
 *
 * @details Обёртка над потоком zlib, скрывающая прямую зависимость от zlib в публичном заголовке
 */
struct awh::compressor::gzip_stream_t {
	// Поток zlib
	z_stream stream;
};

/**
 * Если размер буфера чанка не определён
 */
#ifndef AWH_COMPRESSOR_CHUNK_BUFFER_SIZE
	/**
	 * Устанавливаем размер буфера чанка для компрессии/декомпрессии
	 */
	#define AWH_COMPRESSOR_CHUNK_BUFFER_SIZE 0x4000
#endif

/**
 * @brief пространство имён драйвера
 *
 */
namespace driver {
	/**
	 * Подписываемся на пространства имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Шаблон RAII-обёртки для гарантированного освобождения ресурса
	 *
	 * @tparam F тип функции освобождения ресурса
	 */
	template <typename F>
	/**
	 * @brief Класс RAII-обёртки для гарантированного освобождения ресурса
	 *
	 * @details Вызывает переданную функцию очистки при выходе из области видимости
	 *          (в том числе при возникновении исключения), если не был вызван dismiss().
	 */
	class Scope_Exit {
		private:
			// Функция освобождения ресурса
			F _fn;
			// Флаг активности обёртки
			bool _active;
		public:
			/**
			 * @brief Метод отмены вызова функции освобождения ресурса
			 *
			 */
			void dismiss() noexcept {
				// Деактивируем вызов функции очистки
				this->_active = false;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fn функция освобождения ресурса
			 */
			explicit Scope_Exit(F fn) noexcept : _fn(::move(fn)), _active(true) {}
			/**
			 * @brief Запрещаем копирование обёртки
			 *
			 */
			Scope_Exit(const Scope_Exit &) = delete;
			/**
			 * @brief Запрещаем присваивание обёртки
			 *
			 */
			Scope_Exit & operator = (const Scope_Exit &) = delete;
			/**
			 * @brief Деструктор
			 *
			 */
			~Scope_Exit() noexcept {
				// Если обёртка активна, выполняем освобождение ресурса
				if(this->_active)
					// Выполняем вызов функции очистки
					this->_fn();
			}
	};

	/**
	 * @brief Шаблон класса компрессора данных
	 *
	 * @tparam F тип функции освобождения ресурса
	 */
	template <typename F>
	/**
	 * Создаём тип данных работы с локом
	 */
	using scope_exit_t = Scope_Exit <F>;

	/**
	 * @brief Шаблон функции создания RAII-обёртки освобождения ресурса
	 *
	 * @tparam F тип функции освобождения ресурса
	 */
	template <typename F>
	/**
	 * @brief Функция создания RAII-обёртки освобождения ресурса
	 *
	 * @param fn функция освобождения ресурса
	 * @return   созданная RAII-обёртка
	 */
	static scope_exit_t <F> makeScopeExit(F fn) noexcept {
		// Возвращаем созданную RAII-обёртку
		return scope_exit_t <F> (std::move(fn));
	}
	/**
	 * @brief Шаблон функции работы с компрессором LZma
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором LZma
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void lzma(const void * buffer, const size_t size, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Инициализируем опции компрессора LZma
						static const lzma_options_lzma options = {
							1u << 20u, nullptr, 0, LZMA_LC_DEFAULT, LZMA_LP_DEFAULT,
							LZMA_PB_DEFAULT, LZMA_MODE_FAST, 128, LZMA_MF_HC3, 4
						};
						// Инициализируем фильтры компрессора LZma
						static const lzma_filter filters[] = {
							{LZMA_FILTER_LZMA2, const_cast <lzma_options_lzma *> (&options)},
							{LZMA_VLI_UNKNOWN, nullptr}
						};
						// Актуальный размер сжатых данных
						size_t actual = 0;
						// Вычисляем максимально возможный размер сжатых данных (с учётом заголовка/подвала контейнера)
						const size_t bound = ::lzma_stream_buffer_bound(size);
						// Выделяем буфер памяти нужного нам размера
						result.resize(bound, 0);
						// Выполняем компрессию буфера данных
						lzma_ret rv = ::lzma_stream_buffer_encode(const_cast <lzma_filter *> (filters), LZMA_CHECK_NONE, nullptr, reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (&result[0]), &actual, bound);
						// Если мы получили ошибку
						if(rv != LZMA_OK){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("LZMA: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(actual);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Указатель позиции в буфере для распаковки
						char * ptr = nullptr;
						// Индекс потока LZma компрессора
						lzma_index * index = nullptr;
						// Лимит доступной памяти
						uint64_t memlimit = 0x8000000;
						// Позиции в буферах и актуальный размер данных результата
						size_t inpos = 0, outpos = 0, actual = 0;
						// Смещаем указатель в буфере на подвал
						if((ptr = const_cast <char *> (reinterpret_cast <const char *> (buffer)) + (size - 12)) < reinterpret_cast <const char *> (buffer))
							// Переходим к выводу ошибки
							goto Error;
						// Список флагов потока LZma
						lzma_stream_flags flags;
						// Пытаемся декодировать подвал архива
						if(::lzma_stream_footer_decode(&flags, reinterpret_cast <uint8_t *> (ptr)) != LZMA_OK)
							// Переходим к выводу ошибки
							goto Error;
						// Если буфер данных испорчен
						if((ptr -= flags.backward_size) < reinterpret_cast <const char *> (buffer))
							// Переходим к выводу ошибки
							goto Error;
						// Выполняем декодирование буфера LZma
						if(::lzma_index_buffer_decode(&index, &memlimit, nullptr, reinterpret_cast <uint8_t *> (ptr), &inpos, size - (ptr - reinterpret_cast <const char *> (buffer))) != LZMA_OK)
							// Переходим к выводу ошибки
							goto Error;
						// Сбрасываем позицию во входящем буфере
						inpos = 0;
						// Сбрасываем лимит доступной памяти
						memlimit = 0x8000000;
						// Получаем размер результирующего буфера данных
						actual = ::lzma_index_uncompressed_size(index);
						// Если размер некорректен или превышает допустимый предел (защита от подделанного подвала)
						if((actual == 0) || (actual > (1ULL << 30)))
							// Переходим к выводу ошибки
							goto Error;
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем декомпрессию буфера бинарных данных
						if(::lzma_stream_buffer_decode(&memlimit, 0, nullptr, reinterpret_cast <const uint8_t *> (buffer), &inpos, size, reinterpret_cast <uint8_t *> (&result[0]), &outpos, actual) == LZMA_OK){
							// Выполняем закрытие индекса компрессора LZma
							::lzma_index_end(index, nullptr);
							// Выходим из функции
							return;
						}
						// Устанавливаем метку вывода ошибки
						Error:
						// Выполняем закрытие индекса компрессора LZma
						::lzma_index_end(index, nullptr);
						// Выполняем очистку результата
						result.clear();
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог в лог
							log->print("LZMA: %s", log_t::flag_t::WARNING, "Error during data decompression");
						#endif
						// Выходим из функции
						return;
					}
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("LZMA: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором BZip2
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором BZip2
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void bzip2(const void * buffer, const size_t size, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Выполняем создание объекта потока
				bz_stream stream{};
				// Выполняем зануление параметров потока
				stream.bzfree  = nullptr;
				stream.opaque  = nullptr;
				stream.bzalloc = nullptr;
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Выполняем инициализацию потока
						if(::BZ2_bzCompressInit(&stream, 5, 0, 0) != BZ_OK){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Bzip2: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Гарантируем освобождение потока при любом выходе из области видимости
						auto guard = driver::makeScopeExit([&stream]() noexcept {
							// Выполняем завершение работы с потоком
							::BZ2_bzCompressEnd(&stream);
						});
						/**
						 * Начальный размер выходного буфера: bzip2 может увеличить данные!
						 * Согласно документации: worst case = input + 1% + 600 bytes
						 */
						size_t capacity = (size + (size / 100) + 600);
						// Минимальный размер буфера должен быть не менее 1024 байт
						if(capacity < 1024)
							// Устанавливаем минимальный размер буфера
							capacity = 1024;
						// Выделяем память на результирующий буфер
						result.resize(capacity, 0);
						// Заполняем входные данные буфера
						stream.next_in = const_cast <char *> (reinterpret_cast <const char *> (buffer));
						// Указываем размер входного буфера
						stream.avail_in = static_cast <uint32_t> (size);
						// Устанавливаем буфер для получения результата
						stream.next_out = reinterpret_cast <char *> (&result[0]);
						// Устанавливаем максимальный размер буфера
						stream.avail_out = static_cast <uint32_t> (result.size());
						// Результат выполнения компрессии
						int32_t ret = BZ_OK;
						// Переменная подсчёта сжатых данных
						size_t produced = 0;
						/**
						 * Выполняем компрессию до завершения данных
						 */
						do {
							// Выполняем компрессию ещё одной порции данных
							ret = ::BZ2_bzCompress(&stream, BZ_FINISH);
							// Если нужно больше места для данных
							if(ret == BZ_FINISH_OK){
								// Нужно больше места — расширяем буфер
								produced = static_cast <uint32_t> (stream.total_out_lo32);
								// Увеличиваем буфер в два раза
								result.resize(result.size() * 2);
								// Устанавливаем максимальный размер буфера
								stream.avail_out = static_cast <uint32_t> (result.size() - produced);
								// Устанавливаем буфер для получения результата
								stream.next_out = reinterpret_cast <char *> (&result[0] + produced);
							// Если произошла ошибка компрессии
							} else if(ret != BZ_STREAM_END) {
								// Выполняем очистку буфера данных
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Bzip2: %s", log_t::flag_t::WARNING, "Error during data compression");
								#endif
								// Выходим из функции
								return;
							}
						/**
						 * Если данные ещё не сжаты
						 */
						} while(ret == BZ_FINISH_OK);
						// Обрезаем до реального размера
						result.resize(static_cast <uint32_t> (stream.total_out_lo32));
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Выполняем инициализацию потока
						if(::BZ2_bzDecompressInit(&stream, 0, 0) != BZ_OK){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Bzip2: %s", log_t::flag_t::WARNING, "Error during data decompression");
							#endif
							// Выходим из функции
							return;
						}
						// Гарантируем освобождение потока при любом выходе из области видимости
						auto guard = driver::makeScopeExit([&stream]() noexcept {
							// Выполняем завершение работы с потоком
							::BZ2_bzDecompressEnd(&stream);
						});
						// Заполняем входные данные буфера
						stream.next_in = const_cast <char *> (reinterpret_cast <const char *> (buffer));
						// Указываем размер входного буфера
						stream.avail_in = static_cast <uint32_t> (size);
						// Начальный размер буфера — эвристика
						const size_t capacity = ::max <size_t> (1024, size * 2);
						// Выделяем память на результирующий буфер
						result.resize(capacity, 0);
						// Результат выполнения компрессии
						int32_t ret = BZ_OK;
						/**
						 * Выполняем компрессию всех данных
						 */
						do {
							// Убедимся, что есть место для записи
							if(static_cast <size_t> (stream.total_out_lo32) >= result.size())
								// Увеличиваем буфер в два раза
								result.resize(result.size() * 2);
							// Устанавливаем буфер для получения результата
							stream.next_out = reinterpret_cast <char *> (&result[0] + stream.total_out_lo32);
							// Устанавливаем максимальный размер буфера
							stream.avail_out = static_cast <uint32_t> (result.size() - stream.total_out_lo32);
							// Выполняем декомпрессию
							ret = ::BZ2_bzDecompress(&stream);
							// Если мы завершили сбор данных
							if((ret != BZ_OK) && (ret != BZ_STREAM_END)){
								// Выполняем очистку буфера данных
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Bzip2: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							}
							// Если входные данные исчерпаны, но поток не завершён и есть свободное место в буфере — данные усечены
							if((ret == BZ_OK) && (stream.avail_in == 0) && (stream.avail_out > 0)){
								// Выполняем очистку буфера данных
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Truncated or corrupted data");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Bzip2: %s", log_t::flag_t::WARNING, "Truncated or corrupted data");
								#endif
								// Выходим из функции
								return;
							}
						/**
						 * Если данные ещё не извлечены
						 */
						} while(ret != BZ_STREAM_END);
						// Обрезаем до фактически распакованного размера
						result.resize(static_cast <size_t> (stream.total_out_lo32));
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Bzip2: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Brotli
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Brotli
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void brotli(const void * buffer, const size_t size, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Получаем размер бинарного буфера входящих данных
				size_t sizeInput = size;
				// Создаём временный буфер данных
				vector <uint8_t> data(AWH_COMPRESSOR_CHUNK_BUFFER_SIZE, 0);
				// Получаем бинарный буфер входящих данных
				const uint8_t * nextInput = reinterpret_cast <const uint8_t *> (buffer);
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Инициализируем стейт энкодера Brotli
						BrotliEncoderState * encoder = ::BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
						// Если энкодер создать не удалось
						if(encoder == nullptr){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Brotli: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Гарантируем освобождение энкодера при любом выходе из области видимости
						auto guard = driver::makeScopeExit([encoder]() noexcept {
							// Выполняем освобождение энкодера
							::BrotliEncoderDestroyInstance(encoder);
						});
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(size);
						/**
						 * Выполняем сжатие данных
						 */
						while(!::BrotliEncoderIsFinished(encoder)){
							// Получаем размер буфера закодированных бинарных данных
							size_t sizeOutput = data.size();
							// Получаем буфер закодированных бинарных данных
							uint8_t * nextOutput = &data[0];
							// Если сжатие данных закончено, то завершаем работу
							if(!::BrotliEncoderCompressStream(encoder, BROTLI_OPERATION_FINISH, &sizeInput, &nextInput, &sizeOutput, &nextOutput, nullptr)){
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Brotli: %s", log_t::flag_t::WARNING, "Error during data compression");
								#endif
								// Выходим из функции
								return;
							}
							// Получаем размер полученных данных
							const size_t size = (data.size() - sizeOutput);
							// Если данные получены, формируем результирующий буфер
							if(size > 0){
								// Получаем буфер данных
								const char * buffer = reinterpret_cast <const char *> (&data[0]);
								// Формируем результирующий буфер бинарных данных
								result.insert(result.end(), buffer, buffer + size);
							}
						}
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Полный размер обработанных данных
						size_t total = 0, size = 0;
						// Активируем работу декодера
						BrotliDecoderResult ret = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
						// Инициализируем стейт декодера Brotli
						BrotliDecoderState * decoder = ::BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
						// Если декодер создать не удалось
						if(decoder == nullptr){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Brotli: %s", log_t::flag_t::WARNING, "Error during data decompression");
							#endif
							// Выходим из функции
							return;
						}
						// Гарантируем освобождение декодера при любом выходе из области видимости
						auto guard = driver::makeScopeExit([decoder]() noexcept {
							// Выполняем освобождение декодера
							::BrotliDecoderDestroyInstance(decoder);
						});
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(sizeInput * 3);
						/**
						 * Если декодеру есть с чем работать
						 */
						while(ret == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT){
							// Получаем размер буфера декодированных бинарных данных
							size_t sizeOutput = data.size();
							// Получаем буфер декодированных бинарных данных
							char * nextOutput = reinterpret_cast <char *> (&data[0]);
							// Выполняем декодирование бинарных данных
							ret = ::BrotliDecoderDecompressStream(decoder, &sizeInput, &nextInput, &sizeOutput, reinterpret_cast <uint8_t **> (&nextOutput), &total);
							// Если декодирование данных не выполнено
							if(ret == BROTLI_DECODER_RESULT_ERROR){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Brotli: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из цикла
								break;
							}
							// Получаем размер полученных данных
							size = (data.size() - sizeOutput);
							// Если данные получены, формируем результирующий буфер
							if(size > 0){
								// Получаем буфер данных
								const char * buffer = reinterpret_cast <const char *> (&data[0]);
								// Формируем результирующий буфер бинарных данных
								result.insert(result.end(), buffer, buffer + size);
							}
						}
						// Если декомпрессия данных выполнена не удачно (в т.ч. усечённый вход — NEEDS_MORE_INPUT)
						if(ret != BROTLI_DECODER_RESULT_SUCCESS){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Brotli: %s", log_t::flag_t::WARNING, "Error during data decompression");
							#endif
							// Выполняем очистку результата
							result.clear();
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Brotli: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Snappy
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Snappy
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void snappy(const void * buffer, const size_t size, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Временный промежуточный буфер данных
				string data = "";
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE):
						// Выполняем компрессию данных
						snappy::Compress(reinterpret_cast <const char *> (buffer), size, &data);
					break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE):
						// Выполняем декомпрессию данных
						snappy::Uncompress(reinterpret_cast <const char *> (buffer), size, &data);
					break;
				}
				// Если результат получен
				if(!data.empty())
					// Формируем результат
					result.assign(data.begin(), data.end());
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Snappy: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Snappy: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Density
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Density
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param level  уровень компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 */
	static void density(const void * buffer, const size_t size, const uint32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Максимальный размер выходного буфера
				constexpr uint64_t MAX_OUTPUT_SIZE = 1ULL << 30; // 1 GiB — разумный лимит
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Выполняем получение размер результирующего буфера
						const uint_fast64_t actual = ::density_compress_safe_size(size);
						// Если размер выделен
						if((actual == 0) || (actual > MAX_OUTPUT_SIZE)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Density: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Invalid compression size");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Density: %s", log_t::flag_t::WARNING, "Invalid compression size");
							#endif
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(static_cast <size_t> (actual), 0);
						// Выполняем компрессию буфера данных
						const auto & status = ::density_compress(reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (&result[0]), actual, static_cast <DENSITY_ALGORITHM> (level));
						// Если мы получили ошибку
						if(status.bytesWritten == 0){
							// Выполняем очистку блока с результатом
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Density: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Density: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(static_cast <size_t> (status.bytesWritten));
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Выполняем получение размер результирующего буфера
						const uint_fast64_t actual = ::density_decompress_safe_size(size);
						// Если размер выделен
						if((actual == 0) || (actual > MAX_OUTPUT_SIZE)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Density: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Invalid decompression size");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Density: %s", log_t::flag_t::WARNING, "Invalid decompression size");
							#endif
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(static_cast <size_t> (actual), 0);
						// Выполняем декомпрессию буфера данных
						const auto & status = ::density_decompress(reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (&result[0]), actual);
						// Если мы получили ошибку
						if(status.bytesWritten == 0){
							// Выполняем очистку блока с результатом
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Density: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Density: %s", log_t::flag_t::WARNING, "Error during data decompression");
							#endif
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(status.bytesWritten);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Density: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Density: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Lizard
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Lizard
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param level  уровень компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 */
	static void lizard(const void * buffer, const size_t size, const uint32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Выполняем получение размер результирующего буфера
						int32_t actual = ::Lizard_compressBound(static_cast <int32_t> (size));
						// Если размер выделен
						if((actual <= 0) || (static_cast <size_t> (actual) > (1ULL << 30))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Lizard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Invalid input size");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Lizard: %s", log_t::flag_t::WARNING, "Invalid input size");
							#endif
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(static_cast <size_t> (actual), 0);
						// Выполняем компрессию буфера данных
						actual = ::Lizard_compress(reinterpret_cast <const char *> (buffer), reinterpret_cast <char *> (&result[0]), static_cast <int32_t> (size), actual, level);
						// Если мы получили ошибку
						if(actual <= 0){
							// Выполняем очистку блока с результатом
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Lizard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Lizard: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(static_cast <size_t> (actual));
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Верхний предел размера выходного буфера (защита от повреждённых данных)
						constexpr size_t MAX_OUTPUT_SIZE = (1ULL << 30);
						// Начальный размер выходного буфера
						size_t capacity = (size * 2);
						/**
						 * Выполняем извлечение данных пока не извлечём
						 */
						for(;;){
							// Если требуемый размер буфера превышает допустимый предел — данные повреждены
							if(capacity > MAX_OUTPUT_SIZE){
								// Выполняем очистку блока с результатом
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Lizard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Lizard: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							}
							// Выделяем буфер памяти нужного нам размера
							result.resize(capacity, 0);
							// Выполняем декомпрессию буфера бинарных данных
							const int32_t actual = ::Lizard_decompress_safe(reinterpret_cast <const char *> (buffer), reinterpret_cast <char *> (&result[0]), static_cast <int32_t> (size), static_cast <int32_t> (capacity));
							// Если декомпрессия не выполнена из-за отсутствия памяти
							if(actual < 0)
								// Выполняем удвоение размера выходного буфера
								capacity <<= 1;
							// Если декомпрессия не выполнена
							else if(actual == 0) {
								// Выполняем очистку блока с результатом
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Lizard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Lizard: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							// Если данные извлечены удачно
							} else {
								// Корректируем размер результирующего буфера
								result.resize(static_cast <size_t> (actual));
								// Выходим из цикла
								break;
							}
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Lizard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Lizard: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Lz4
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Lz4
	 *
	 * @param buffer буфер данных
	 * @param size   размер данных
	 * @param level  уровень компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void lz4(const void * buffer, const size_t size, const uint32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Выполняем получение размер результирующего буфера
						int32_t actual = ::LZ4_compressBound(size);
						// Если размер выделен
						if(actual <= 0){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("LZ4: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("LZ4: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем компрессию буфера бинарных данных
						actual = ::LZ4_compress_fast(reinterpret_cast <const char *> (buffer), reinterpret_cast <char *> (&result[0]), size, actual, level);
						// Если компрессия не выполнена (расширение данных при сжатии не является ошибкой)
						if(actual <= 0){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("LZ4: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("LZ4: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Корректируем размер результирующего буфера
						result.resize(actual);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Верхний предел размера выходного буфера (защита от повреждённых данных)
						constexpr size_t MAX_OUTPUT_SIZE = (1ULL << 30);
						// Начальный размер выходного буфера
						size_t capacity = (size * 2);
						/**
						 * Выполняем извлечение данных пока не извлечём
						 */
						for(;;){
							// Если требуемый размер буфера превышает допустимый предел — данные повреждены
							if(capacity > MAX_OUTPUT_SIZE){
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("LZ4: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("LZ4: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							}
							// Выделяем буфер памяти нужного нам размера
							result.resize(capacity, 0);
							// Выполняем декомпрессию буфера бинарных данных
							const int32_t actual = ::LZ4_decompress_safe(reinterpret_cast <const char *> (buffer), reinterpret_cast <char *> (&result[0]), static_cast <int32_t> (size), static_cast <int32_t> (capacity));
							// Если декомпрессия не выполнена из-за отсутствия памяти
							if(actual < 0)
								// Выполняем удвоение размера выходного буфера
								capacity <<= 1;
							// Если декомпрессия не выполнена
							else if(actual == 0) {
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("LZ4: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("LZ4: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							// Если данные извлечены удачно
							} else {
								// Корректируем размер результирующего буфера
								result.resize(static_cast <size_t> (actual));
								// Выходим из цикла
								break;
							}
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("LZ4: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("LZ4: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором Zstandard
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Zstandard
	 *
	 * @param buffer буфер данных
	 * @param size   размер данных
	 * @param level  уровень компрессии
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void zstd(const void * buffer, const size_t size, const uint32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Выполняем создание контекста потока
						ZSTD_CStream * ctx = ::ZSTD_createCStream();
						// Если контекст потока создан
						if(ctx == nullptr){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zstandard: %s", log_t::flag_t::WARNING, "Error during data compression");
							#endif
							// Выходим из функции
							return;
						}
						// Гарантируем освобождение контекста при любом выходе из области видимости
						auto guard = driver::makeScopeExit([ctx]() noexcept {
							// Выполняем освобождение контекста потока
							::ZSTD_freeCStream(ctx);
						});
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(size);
						// Выполняем инициализацию потока
						size_t status = ::ZSTD_initCStream(ctx, level);
						// Если мы получили ошибку инициализации
						if(::ZSTD_isError(status)){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zstandard: %s", log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							#endif
							// Выходим из функции
							return;
						}
						// Инициализируем переменные смещения в буфере и актуальный размер данных
						size_t offset = 0, actual = 0;
						// Получаем длину итогового буфера данных
						const size_t length = ::ZSTD_CStreamOutSize();
						// Выполняем инициализацию итогового буфера данных
						const auto data = make_unique <char []> (length);
						// Выполняем создание буфера исходящих данных
						ZSTD_outBuffer output = {data.get(), length, 0};
						/**
						 * Выполняем обработку всех входящих данных
						 */
						while(offset < size){
							// Определяем актуальный размер данных
							actual = (((size - offset) > static_cast <size_t> (::ZSTD_CStreamInSize())) ? static_cast <size_t> (::ZSTD_CStreamInSize()) : (size - offset));
							// Выполняем создание буфера данных для входящих сжатых данных
							ZSTD_inBuffer input = {reinterpret_cast <const char *> (buffer) + offset, actual, 0};
							/**
							 * Выполняем обработку до тех пор пока все не обработаем
							 */
							while(input.pos < input.size){
								// Сбрасываем позицию буфера
								output.pos = 0;
								// Выполняем компрессию полученных данных
								status = ::ZSTD_compressStream(ctx, &output, &input);
								// Если мы получили ошибку инициализации
								if(::ZSTD_isError(status)){
									// Выполняем очистку результата
									result.clear();
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог в лог
										log->print("Zstandard: %s", log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
									#endif
									// Выходим из функции
									return;
								}
								// Выполняем формирование полученных данных
								result.insert(result.end(), data.get(), data.get() + output.pos);
							}
							// Увеличиваем смещение в исходном буфере необработанных данных
							offset += actual;
						}
						// Сбрасываем позицию буфера
						output.pos = 0;
						// Завершаем поток
						status = ::ZSTD_endStream(ctx, &output);
						// Если мы получили ошибку инициализации
						if(::ZSTD_isError(status)){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zstandard: %s", log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							#endif
							// Выходим из функции
							return;
						}
						// Выполняем формирование полученных данных
						result.insert(result.end(), data.get(), data.get() + output.pos);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Выполняем создание контекста потока
						ZSTD_DStream * ctx = ::ZSTD_createDStream();
						// Если контекст потока создан
						if(ctx == nullptr){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zstandard: %s", log_t::flag_t::WARNING, "Error during data decompression");
							#endif
							// Выходим из функции
							return;
						}
						// Гарантируем освобождение контекста при любом выходе из области видимости
						auto guard = driver::makeScopeExit([ctx]() noexcept {
							// Выполняем освобождение контекста потока
							::ZSTD_freeDStream(ctx);
						});
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(size * 3);
						// Выполняем инициализацию потока
						size_t status = ::ZSTD_initDStream(ctx);
						// Если мы получили ошибку инициализации
						if(::ZSTD_isError(status)){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zstandard: %s", log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
							#endif
							// Выходим из функции
							return;
						}
						// Инициализируем переменные смещения в буфере и актуальный размер данных
						size_t offset = 0, actual = 0;
						// Получаем длину итогового буфера данных
						const size_t length = ::ZSTD_DStreamOutSize();
						// Выполняем инициализацию итогового буфера данных
						const auto data = make_unique <char []> (length);
						// Выполняем создание буфера исходящих данных
						ZSTD_outBuffer output = {data.get(), length, 0};
						/**
						 * Выполняем обработку всех входящих данных
						 */
						while(offset < size){
							// Определяем актуальный размер данных
							actual = (((size - offset) > static_cast <size_t> (::ZSTD_DStreamInSize())) ? static_cast <size_t> (::ZSTD_DStreamInSize()) : (size - offset));
							// Выполняем создание буфера данных для входящих сжатых данных
							ZSTD_inBuffer input = {reinterpret_cast <const char *> (buffer) + offset, actual, 0};
							/**
							 * Выполняем обработку до тех пор пока все не обработаем
							 */
							while(input.pos < input.size){
								// Сбрасываем позицию буфера
								output.pos = 0;
								// Выполняем декомпрессию полученных данных
								status = ::ZSTD_decompressStream(ctx, &output, &input);
								// Если мы получили ошибку инициализации
								if(::ZSTD_isError(status)){
									// Выполняем очистку результата
									result.clear();
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог в лог
										log->print("Zstandard: %s", log_t::flag_t::WARNING, ::ZSTD_getErrorName(status));
									#endif
									// Выходим из функции
									return;
								}
								// Выполняем формирование полученных данных
								result.insert(result.end(), data.get(), data.get() + output.pos);
							}
							// Увеличиваем смещение в исходном буфере необработанных данных
							offset += actual;
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Zstandard: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции работы с компрессором GZip
	 *
	 * @tparam T сигнатура функции
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором GZip
	 *
	 * @param buffer буфер данных
	 * @param size   размер данных
	 * @param level  уровень компрессии
	 * @param wbits  размер скользящего окна
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 */
	static void gzip(const void * buffer, const size_t size, const uint32_t level, const int16_t wbits, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Создаем поток Zip
				z_stream zs{};
				// Инициализируем поля структуры
				zs.zalloc = Z_NULL;
				zs.zfree  = Z_NULL;
				zs.opaque = Z_NULL;
				// Вычисляем размер скользящего окна
				const int32_t windowBits = (wbits + 16);
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Если поток инициализировать не удалось, выходим
						if(::deflateInit2(&zs, static_cast <int32_t> (level), Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK){
							// Гарантируем освобождение потока при любом выходе из области видимости (в т.ч. при исключении)
							auto guard = driver::makeScopeExit([&zs]() noexcept {
								// Выполняем завершение работы с потоком
								::deflateEnd(&zs);
							});
							// Указываем размер входного буфера
							zs.avail_in = static_cast <uInt> (size);
							// Заполняем входные данные буфера
							zs.next_in = reinterpret_cast <Bytef *> (const_cast <void *> (buffer));
							// Оценка максимального размера (включая заголовок и хвост)
							const size_t maxSize = ::deflateBound(&zs, static_cast <uLong> (size));
							// Выделяем память на результирующий буфер
							result.resize(maxSize);
							// Устанавливаем максимальный размер буфера
							zs.avail_out = static_cast <uInt> (maxSize);
							// Устанавливаем буфер для получения результата
							zs.next_out = reinterpret_cast <Bytef *> (&result[0]);
							// Выполняем сжатие данных
							const int32_t ret = ::deflate(&zs, Z_FINISH);
							// Завершаем сжатие
							::deflateEnd(&zs);
							// Если мы успешно завершили сжатие
							if(ret == Z_STREAM_END)
								// Корректируем размер результирующего буфера
								result.resize(zs.total_out);
							// Если произошла ошибка компрессии
							else {
								// Выполняем очистку буфера данных
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("GZip: %s", log_t::flag_t::WARNING, "Error during data compression");
								#endif
							}
						// Если поток инициализировать не удалось
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error initializing compression stream");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("GZip: %s", log_t::flag_t::WARNING, "Error initializing compression stream");
							#endif
						}
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Если поток инициализировать не удалось, выходим
						if(::inflateInit2(&zs, windowBits) == Z_OK){
							// Гарантируем освобождение потока при любом выходе из области видимости (в т.ч. при исключении)
							auto guard = driver::makeScopeExit([&zs]() noexcept {
								// Выполняем завершение работы с потоком
								::inflateEnd(&zs);
							});
							// Устанавливаем размер входного буфера
							zs.avail_in = static_cast <uInt> (size);
							// Устанавливаем буфер входящих данных
							zs.next_in = reinterpret_cast <Bytef *> (const_cast <void *> (buffer));
							// Резервируем память под результат для снижения числа реаллокаций
							result.reserve(size * 3);
							// Буфер для извлечённых данных
							vector <uint8_t> output(AWH_COMPRESSOR_CHUNK_BUFFER_SIZE, 0);
							// Результат проверки декомпрессии
							int32_t ret = Z_OK;
							// Переменная подсчёта сжатых данных
							size_t produced = 0;
							/**
							 * Выполняем декомпрессию всех данных
							 */
							do {
								// Устанавливаем буфер для получения результата
								zs.next_out = &output[0];
								// Устанавливаем максимальный размер буфера
								zs.avail_out = static_cast <uInt> (output.size());
								// Выполняем декомпрессию данных
								ret = ::inflate(&zs, Z_NO_FLUSH);
								// Если произошла ошибка декомпрессии
								if((ret != Z_OK) && (ret != Z_STREAM_END)){
									// Выполняем очистку буфера данных
									result.clear();
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог в лог
										log->print("GZip: %s", log_t::flag_t::WARNING, "Error during data decompression");
									#endif
									// Выходим из функции
									return;
								}
								// Вычисляем количество извлечённых данных
								produced = (output.size() - static_cast <size_t> (zs.avail_out));
								// Если данные извлечены, формируем результирующий буфер
								if(produced > 0)
									// Формируем результирующий буфер бинарных данных
									result.insert(result.end(), &output[0], &output[0] + produced);
							/**
							 * Если данные ещё не извлечены
							 */
							} while(ret == Z_OK);
						// Если поток инициализировать не удалось
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error initializing decompression stream");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("GZip: %s", log_t::flag_t::WARNING, "Error initializing decompression stream");
							#endif
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("GZip: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции компрессии/декомпрессии данных в формате Zlib (RFC 1950)
	 *
	 * @tparam T выходной контейнер (например, string или vector <char>)
	 */
	template <typename T>
	/**
	 * @brief Функция компрессии/декомпрессии данных в формате Zlib (RFC 1950)
	 *
	 * @details Формат: 2-байтовый zlib-заголовок (CMF/FLG) + DEFLATE-поток + 4-байтовая
	 *          контрольная сумма Adler-32. В отличие от GZip не добавляет gzip-заголовок.
	 *
	 * @param buffer буфер входных данных
	 * @param size   размер входных данных
	 * @param level  уровень компрессии
	 * @param wbits  размер скользящего окна
	 * @param event  событие выполнения операции
	 * @param result выходной контейнер
	 * @param log    объект для работы с логами
	 */
	static void zlib(const void * buffer, const size_t size, const uint32_t level, const int16_t wbits, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Создаем поток Zip
				z_stream zs{};
				// Инициализируем поля структуры
				zs.zalloc = Z_NULL;
				zs.zfree  = Z_NULL;
				zs.opaque = Z_NULL;
				// Размер скользящего окна: положительный без +16 → zlib-формат (RFC 1950)
				const int32_t windowBits = static_cast <int32_t> (wbits);
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Если поток инициализировать не удалось, выходим
						if(::deflateInit2(&zs, static_cast <int32_t> (level), Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK){
							// Гарантируем освобождение потока при любом выходе из области видимости (в т.ч. при исключении)
							auto guard = driver::makeScopeExit([&zs]() noexcept {
								// Выполняем завершение работы с потоком
								::deflateEnd(&zs);
							});
							// Указываем размер входного буфера
							zs.avail_in = static_cast <uInt> (size);
							// Заполняем входные данные буфера
							zs.next_in = reinterpret_cast <Bytef *> (const_cast <void *> (buffer));
							// Оценка максимального размера (включая zlib-заголовок и Adler-32)
							const size_t maxSize = ::deflateBound(&zs, static_cast <uLong> (size));
							// Выделяем память на результирующий буфер
							result.resize(maxSize);
							// Указываем размер выходного буфера
							zs.avail_out = static_cast <uInt> (maxSize);
							// Заполняем буфер выходными данными
							zs.next_out = reinterpret_cast <Bytef *> (&result[0]);
							// Выполняем компрессию данных
							const int32_t ret = ::deflate(&zs, Z_FINISH);
							// Завершаем работу потока
							::deflateEnd(&zs);
							// Если компрессия данных выполнена
							if(ret == Z_STREAM_END)
								// Устанавливаем реальный размер результирующего буфера
								result.resize(maxSize - zs.avail_out);
							// Если компрессия данных не выполнена
							else {
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Zlib: %s", log_t::flag_t::WARNING, "Error during data compression");
								#endif
							}
						/**
						 * Если инициализация не удалась
						 */
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error initializing compression stream");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zlib: %s", log_t::flag_t::WARNING, "Error initializing compression stream");
							#endif
						}
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Инициализируем поток для декомпрессии
						if(::inflateInit2(&zs, windowBits) == Z_OK){
							// Гарантируем освобождение потока при любом выходе из области видимости (в т.ч. при исключении)
							auto guard = driver::makeScopeExit([&zs]() noexcept {
								// Выполняем завершение работы с потоком
								::inflateEnd(&zs);
							});
							// Указываем размер входного буфера
							zs.avail_in = static_cast <uInt> (size);
							// Заполняем входные данные буфера
							zs.next_in = reinterpret_cast <Bytef *> (const_cast <void *> (buffer));
							// Резервируем память под результат для снижения числа реаллокаций
							result.reserve(size * 3);
							// Создаём временный буфер данных
							vector <Bytef> data(AWH_COMPRESSOR_CHUNK_BUFFER_SIZE, 0);
							// Переменная результата
							int32_t ret = Z_OK;
							/**
							 * Выполняем декомпрессию данных
							 */
							do {
								// Указываем размер выходного буфера
								zs.avail_out = static_cast <uInt> (data.size());
								// Заполняем буфер выходными данными
								zs.next_out = &data[0];
								// Выполняем декомпрессию данных
								ret = ::inflate(&zs, Z_NO_FLUSH);
								// Если возникает ошибка
								if((ret != Z_OK) && (ret != Z_STREAM_END)){
									// Выполняем очистку результата
									result.clear();
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог в лог
										log->print("Zlib: %s", log_t::flag_t::WARNING, "Error during data decompression");
									#endif
									// Выходим из цикла
									break;
								}
								// Вычисляем количество декомпрессированных данных
								const size_t produced = (data.size() - static_cast <size_t> (zs.avail_out));
								// Если декомпрессировано хоть что-то
								if(produced > 0){
									// Получаем буфер данных
									const char * chunk = reinterpret_cast <const char *> (&data[0]);
									// Добавляем декомпрессированные данные в результат
									result.insert(result.end(), chunk, chunk + produced);
								}
							/**
							 * Пока нет конца потока и есть входные данные
							 */
							} while((ret == Z_OK) && (zs.avail_in > 0));
							// Завершаем работу потока
							::inflateEnd(&zs);
						/**
						 * Если инициализация не удалась
						 */
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error initializing decompression stream");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zlib: %s", log_t::flag_t::WARNING, "Error initializing decompression stream");
							#endif
						}
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Zlib: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief Шаблон функции компрессии/декомпрессии данных в формате raw deflate
	 *
	 * @tparam T выходной контейнер (например, string или vector <char>)
	 */
	template <typename T>
	/**
	 * @brief Функция компрессии/декомпрессии данных в формате raw deflate
	 *
	 * @details Поддерживает два режима:
	 *           - streaming == false: каждый вызов независим (контекст создаётся и уничтожается внутри).
	 *           - streaming == true:  контекст (z_stream) переиспользуется между вызовами (должен быть проинициализирован извне).
	 *
	 * @details Для WebSocket:
	 *           - streaming = true  → permessage-deflate с context takeover.
	 *           - streaming = false → permessage-deflate без context takeover (каждое сообщение независимо).
	 *
	 * @param buffer    буфер входных данных
	 * @param size      размер входных данных
	 * @param level     уровень компрессии
	 * @param wbits     размер скользящего окна
	 * @param streaming флаг потокового режима (переиспользование контекста)
	 * @param stream    объект потока zlib
	 * @param event     событие выполнения операции
	 * @param result    выходной контейнер
	 * @param log       объект для работы с логами
	 */
	static void deflate(const void * buffer, const size_t size, const uint32_t level, const int16_t wbits, const bool streaming, z_stream & stream, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Локальный поток (если не streaming)
				z_stream local{};
				// Создаем поток Zip
				z_stream * zs = (streaming ? &stream : &local);
				// Создаём выходной буфер с запасом по памяти
				vector <Bytef> output(::max <size_t> (0xFF, size * 2));
				// Результат проверки декомпрессии
				int32_t ret = Z_OK;
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Если не используется streaming
						if(!streaming){
							// Инициализируем локальный поток для компрессии
							local.zalloc = Z_NULL;
							local.zfree  = Z_NULL;
							local.opaque = Z_NULL;
							// Инициализируем поток для компрессии
							ret = ::deflateInit2(&local, static_cast <int32_t> (level), Z_DEFLATED, static_cast <int32_t> (-1 * wbits), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
							// Если инициализация не удалась, выходим
							if(ret != Z_OK){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, streaming, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error initializing compression stream");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Error initializing compression stream");
								#endif
								// Выходим из функции
								return;
							}
						}
						// Гарантируем освобождение локального потока при любом выходе (в т.ч. при исключении), кроме streaming-режима
						auto guard = driver::makeScopeExit([&local, streaming]() noexcept {
							// Если не используется streaming
							if(!streaming)
								// Выполняем завершение работы с локальным потоком
								::deflateEnd(&local);
						});
						// Устанавливаем размер входных данных
						zs->avail_in = static_cast <uInt> (size);
						// Устанавливаем буфер входящих данных
						zs->next_in = reinterpret_cast <Bytef *> (const_cast <void *> (buffer));
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(size);
						// Переменная подсчёта сжатых данных
						size_t produced = 0;
						/**
						 * Сжатие основного тела (без flush)
						 */
						while(zs->avail_in > 0){
							// Устанавливаем выходной буфер данных
							zs->next_out = &output[0];
							// Устанавливаем размер выходного буфера данных
							zs->avail_out = static_cast <uInt> (output.size());
							// Выполняем сжатие данных
							ret = ::deflate(zs, Z_NO_FLUSH);
							// Если возникает ошибка
							if(ret != Z_OK){
								// Если не используется streaming
								if(!streaming)
									// Завершаем работу локального потока
									::deflateEnd(&local);
								// Выполняем очистку блока с результатом
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, streaming, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Error during data compression");
								#endif
								// Выходим из функции
								return;
							}
							// Вычисляем количество сжатых данных
							produced = (output.size() - static_cast <size_t> (zs->avail_out));
							// Если сжато хоть что-то
							if(produced > 0)
								// Добавляем сжатые данные в результат
								result.insert(result.end(), &output[0], &output[0] + produced);
							// Если выходной буфер заполнен, увеличиваем его размер
							if(zs->avail_out == 0)
								// Увеличиваем размер выходного буфера
								output.resize(output.size() * 2);
						}
						/**
						 * Завершение сообщения: один Z_SYNC_FLUSH
						 */
						do {
							// Устанавливаем выходной буфер данных
							zs->next_out = &output[0];
							// Устанавливаем размер выходного буфера данных
							zs->avail_out = static_cast <uInt> (output.size());
							// Выполняем сжатие данных
							ret = ::deflate(zs, Z_SYNC_FLUSH);
							// Если возникает ошибка
							if(ret != Z_OK){
								// Если не используется streaming
								if(!streaming)
									// Завершаем работу локального потока
									::deflateEnd(&local);
								// Выполняем очистку блока с результатом
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, streaming, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Error during data compression");
								#endif
								// Выходим из функции
								return;
							}
							// Вычисляем количество сжатых данных
							produced = (output.size() - static_cast <size_t> (zs->avail_out));
							// Если сжато хоть что-то
							if(produced > 0)
								// Добавляем сжатые данные в результат
								result.insert(result.end(), &output[0], &output[0] + produced);
							// Если выходной буфер заполнен, увеличиваем его размер
							if(zs->avail_out == 0)
								// Увеличиваем размер выходного буфера
								output.resize(output.size() * 2);
						/**
						 * Пока выходной буфер полностью заполнен
						 */
						} while(zs->avail_out == 0);
						// Если не используется streaming
						if(!streaming)
							// Завершаем работу локального потока
							::deflateEnd(&local);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Если не используется streaming
						if(!streaming){
							// Инициализируем локальный поток для декомпрессии
							local.zalloc = Z_NULL;
							local.zfree  = Z_NULL;
							local.opaque = Z_NULL;
							// Инициализируем поток для декомпрессии
							ret = ::inflateInit2(&local, static_cast <int32_t> (-1 * wbits));
							// Если инициализация не удалась, выходим
							if(ret != Z_OK){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, streaming, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error initializing decompression stream");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Error initializing decompression stream");
								#endif
								// Выходим из функции
								return;
							}
						}
						// Гарантируем освобождение локального потока при любом выходе (в т.ч. при исключении), кроме streaming-режима
						auto guard = driver::makeScopeExit([&local, streaming]() noexcept {
							// Если не используется streaming
							if(!streaming)
								// Выполняем завершение работы с локальным потоком
								::inflateEnd(&local);
						});
						// Устанавливаем размер входных данных
						zs->avail_in = static_cast <uInt> (size);
						// Устанавливаем буфер входящих данных
						zs->next_in = reinterpret_cast <Bytef *> (const_cast <void *> (buffer));
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(size * 3);
						// Переменная подсчёта декомпрессированных данных
						size_t produced = 0;
						/**
						 * Декомпрессия данных
						 */
						do {
							// Устанавливаем выходной буфер данных
							zs->next_out = &output[0];
							// Устанавливаем размер выходного буфера данных
							zs->avail_out = static_cast <uInt> (output.size());
							// inflate игнорирует flush — всегда Z_NO_FLUSH
							ret = ::inflate(zs, Z_NO_FLUSH);
							// Если возникает ошибка
							if((ret != Z_OK) && (ret != Z_STREAM_END)){
								// Если не используется streaming
								if(!streaming)
									// Завершаем работу локального потока
									::inflateEnd(&local);
								// Выполняем очистку блока с результатом
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, streaming, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							}
							// Вычисляем количество декомпрессированных данных
							produced = (output.size() - static_cast <size_t> (zs->avail_out));
							// Если декомпрессировано хоть что-то
							if(produced > 0)
								// Добавляем декомпрессированные данные в результат
								result.insert(result.end(), &output[0], &output[0] + produced);
							// Если выходной буфер заполнен, увеличиваем его размер
							if(zs->avail_out == 0)
								// Увеличиваем размер выходного буфера
								output.resize(output.size() * 2);
						/**
						 * Пока нет конца потока и есть входные данные
						 */
						} while((ret == Z_OK) && (zs->avail_in > 0));
						// Если не используется streaming
						if(!streaming)
							// Завершаем работу локального потока
							::inflateEnd(&local);
					} break;
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выполняем очистку блока с результатом
				result.clear();
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, streaming, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог в лог
					log->print("Deflate: %s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
};

/**
 * @brief Конструктор
 *
 */
awh::compressor::Block::BufferGZip::BufferGZip() noexcept :
 compress(nullptr), decompress(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::compressor::Block::Takeover::Takeover() noexcept :
 compress(false), decompress(false) {}

/**
 * @brief Конструктор
 *
 */
awh::compressor::Block::Zlib::Zlib() noexcept : wbits(0) {}

/**
 * @brief Конструктор
 *
 */
awh::compressor::Block::GZip::GZip() noexcept : wbits(0) {}

/**
 * @brief Метод установки уровня компрессии
 *
 * @param level уровень компрессии
 */
void awh::compressor::Block::level(const level_t level) noexcept {
	// Выполняем блокировку потоков
	const locker_t <> lock(this->_mtx);
	/**
	 * Определяем переданный уровень компрессии
	 */
	switch(static_cast <uint8_t> (level)){
		// Выполняем установку максимального уровня компрессии
		case static_cast <uint8_t> (level_t::BEST): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 0;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 100;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_MAX_CLEVEL;
			// Выполняем установку уровня компрессии Cheetah (Density)
			this->_level[4] = DENSITY_ALGORITHM_CHEETAH;
		} break;
		// Выполняем установку уровня компрессии на максимальную производительность
		case static_cast <uint8_t> (level_t::SPEED): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 3;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_SPEED;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = ZSTD_CLEVEL_DEFAULT;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_MIN_CLEVEL;
			// Выполняем установку уровня компрессии LION (Density)
			this->_level[4] = DENSITY_ALGORITHM_LION;
		} break;
		// Выполняем установку нормального уровня компрессии
		case static_cast <uint8_t> (level_t::NORMAL): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 1;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_DEFAULT_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 22;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_DEFAULT_CLEVEL;
			// Выполняем установку уровня компрессии Chameleon (Density)
			this->_level[4] = DENSITY_ALGORITHM_CHAMELEON;
		} break;
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::compressor::Block::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	this->_mtx.enabled = mode;
}
/**
 * @brief Метод установки размера скользящего окна Zlib
 *
 * @param wbits размер скользящего окна
 */
void awh::compressor::Block::wbitsZlib(const int16_t wbits) noexcept {
	// Выполняем блокировку потоков
	const locker_t <> lock(this->_mtx);
	// Устанавливаем размер скользящего окна Zlib
	this->_zlib.wbits = wbits;
}
/**
 * @brief Метод установки размера скользящего окна
 *
 * @param wbits размер скользящего окна
 * @return      результат установки размера
 */
bool awh::compressor::Block::wbitsGZip(const int16_t wbits) noexcept {
	// Переменная результата
	bool result = false;
	{
		// Выполняем блокировку потоков
		const locker_t <> lock(this->_mtx);
		// Устанавливаем размер скользящего окна
		this->_gzip.wbits = wbits;
	}
	// Выполняем пересборку контекстов LZ77 для компрессии
	result = this->takeoverGZip(event_t::ENCODE, this->_gzip.takeover.compress.load(std::memory_order_acquire));
	// Если всё прошло успешно
	if(result)
		// Выполняем пересборку контекстов LZ77 для декомпрессии
		result = this->takeoverGZip(event_t::DECODE, this->_gzip.takeover.decompress.load(std::memory_order_acquire));
	// Выполняем пересборку контекстов LZ77 для декомпрессии
	else this->takeoverGZip(event_t::DECODE, this->_gzip.takeover.decompress.load(std::memory_order_acquire));
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод включения/отключения флага переиспользования контекста компрессии/декомпрессии
 *
 * @param event событие выполнения операции
 * @param flag  флаг переиспользования контекста компрессии/декомпрессии
 * @return      результат установки флага
 */
bool awh::compressor::Block::takeoverGZip(const event_t event, const bool flag) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем событие выполнения операции
		 */
		switch(static_cast <uint8_t> (event)){
			// Выполняем установку флага переиспользования контекста компрессии
			case static_cast <uint8_t> (event_t::ENCODE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Извлекаем буфер GZip
				z_stream & buffer = this->_gzip.buffer.compress->stream;
				// Если уже выделена память для компрессора
				if(this->_gzip.takeover.compress.load(std::memory_order_acquire))
					// Очищаем выделенную память для компрессора
					::deflateEnd(&buffer);
				// Если флаг установлен
				if(!(result = !flag)){
					// Заполняем его нулями потока для компрессора
					::memset(&buffer, 0, sizeof(buffer));
					// Обнуляем структуру потока для компрессора
					buffer.zalloc = Z_NULL;
					buffer.zfree  = Z_NULL;
					buffer.opaque = Z_NULL;
					// Если поток инициализировать не удалось, выходим
					if(!(result = (::deflateInit2(&buffer, this->_level[1], Z_DEFLATED, static_cast <int32_t> (-1 * this->_gzip.wbits), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Deflate stream is not create", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), flag), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Deflate stream is not create", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из функции
						return result;
					}
				}
				// Устанавливаем переданный флаг
				this->_gzip.takeover.compress.store(flag, std::memory_order_release);
			} break;
			// Выполняем установку флага переиспользования контекста декомпрессии
			case static_cast <uint8_t> (event_t::DECODE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Извлекаем буфер GZip
				z_stream & buffer = this->_gzip.buffer.decompress->stream;
				// Если уже выделена память для декомпрессора
				if(this->_gzip.takeover.decompress.load(std::memory_order_acquire))
					// Очищаем выделенную память для декомпрессора
					::inflateEnd(&buffer);
				// Если флаг установлен
				if(!(result = !flag)){
					// Заполняем его нулями потока для декомпрессора
					::memset(&buffer, 0, sizeof(buffer));
					// Обнуляем структуру потока для декомпрессора
					buffer.avail_in = 0;
					// Инициализируем остальные поля структуры потока для декомпрессора
					buffer.zalloc  = Z_NULL;
					buffer.zfree   = Z_NULL;
					buffer.opaque  = Z_NULL;
					buffer.next_in = Z_NULL;
					// Если поток инициализировать не удалось, выходим
					if(!(result = (::inflateInit2(&buffer, static_cast <int32_t> (-1 * this->_gzip.wbits)) == Z_OK))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Inflate stream is not create", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), flag), log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Inflate stream is not create", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из функции
						return result;
					}
				}
				// Устанавливаем переданный флаг
				this->_gzip.takeover.decompress.store(flag, std::memory_order_release);
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), flag), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки поддержки потокового режима методом компрессии
 *
 * @param method метод компрессии
 * @return       результат проверки
 */
bool awh::compressor::Block::streamable(const method_t method) noexcept {
	/**
	 * Определяем метод компрессии
	 */
	switch(static_cast <uint8_t> (method)){
		// Методы, поддерживающие потоковый режим
		case static_cast <uint8_t> (method_t::LZ4):
		case static_cast <uint8_t> (method_t::GZIP):
		case static_cast <uint8_t> (method_t::ZLIB):
		case static_cast <uint8_t> (method_t::ZSTD):
		case static_cast <uint8_t> (method_t::LZMA):
		case static_cast <uint8_t> (method_t::BZIP2):
		case static_cast <uint8_t> (method_t::BROTLI):
		case static_cast <uint8_t> (method_t::LIZARD):
		case static_cast <uint8_t> (method_t::DEFLATE):
			// Возвращаем положительный результат
			return true;
	}
	// Для остальных методов потоковый режим не поддерживается
	return false;
}
/**
 * @brief Метод создания потоковой сессии
 *
 * @details Создаёт объект потоковой (streaming) компрессии/декомпрессии,
 *          инициализированный текущей конфигурацией. Для методов, не
 *          поддерживающих потоковый режим, возвращается невалидный поток.
 *
 * @param method метод компрессии
 * @param event  направление операции
 * @return       объект потоковой сессии
 */
awh::compressor::stream_t awh::compressor::Block::stream(const method_t method, const event_t event) const noexcept {
	// Если метод не поддерживает потоковый режим, возвращаем невалидный поток
	if(!block_t::streamable(method))
		// Возвращаем невалидный поток
		return stream_t();
	// Формируем параметры инициализации
	params_t params;
	/**
	 * Определяем метод компрессии для выбора параметров
	 */
	switch(static_cast <uint8_t> (method)){
		// Для GZip и Deflate
		case static_cast <uint8_t> (method_t::GZIP):
		case static_cast <uint8_t> (method_t::DEFLATE): {
			// Устанавливаем размер окна
			params.wbits = this->_gzip.wbits.load(std::memory_order_acquire);
			// Устанавливаем уровень компрессии
			params.level = static_cast <int32_t> (this->_level[1].load(std::memory_order_acquire));
		} break;
		// Для Zlib (RFC 1950)
		case static_cast <uint8_t> (method_t::ZLIB): {
			// Устанавливаем размер окна
			params.wbits = this->_zlib.wbits.load(std::memory_order_acquire);
			// Устанавливаем уровень компрессии
			params.level = static_cast <int32_t> (this->_level[1].load(std::memory_order_acquire));
		} break;
		// Для Zstandard
		case static_cast <uint8_t> (method_t::ZSTD):
			// Устанавливаем уровень компрессии
			params.level = static_cast <int32_t> (this->_level[2].load(std::memory_order_acquire));
		break;
		// Для LZ4
		case static_cast <uint8_t> (method_t::LZ4):
			// Устанавливаем уровень компрессии
			params.level = static_cast <int32_t> (this->_level[0].load(std::memory_order_acquire));
		break;
		// Для Lizard
		case static_cast <uint8_t> (method_t::LIZARD):
			// Устанавливаем уровень компрессии
			params.level = static_cast <int32_t> (this->_level[3].load(std::memory_order_acquire));
		break;
		// Для LZMA
		case static_cast <uint8_t> (method_t::LZMA):
			// Устанавливаем пресет компрессии по умолчанию
			params.level = 6;
		break;
		// Для BZip2
		case static_cast <uint8_t> (method_t::BZIP2):
			// Устанавливаем размер блока (900k)
			params.level = 9;
		break;
	}
	// Создаём и возвращаем потоковую сессию
	return stream_t(method, event, params, this->_log);
}
/**
 * @brief Шаблон метода компрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 */
auto awh::compressor::Block::compress(string_view buffer, const method_t method) const noexcept -> T {
	// Переменная результата
	T result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем кодирование
		this->compress(&buffer[0], buffer.size(), method, result);
	// Возвращаем результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в строку
 *
 */
template string awh::compressor::Block::compress(string_view, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::compress(string_view, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::compress(string_view, const method_t) const noexcept;
/**
 * @brief Шаблон метода компрессии данных
 *
 * @tparam A тип возвращаемого результата
 * @tparam B тип буфера данных
 */
template <typename A, typename B>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 */
auto awh::compressor::Block::compress(const B & buffer, const method_t method) const noexcept -> A {
	// Переменная результата
	A result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем кодирование
		this->compress(&buffer[0], buffer.size(), method, result);
	// Возвращаем результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в строку
 *
 */
template string awh::compressor::Block::compress(const string &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из буфера с выводом результата в строку
 *
 */
template string awh::compressor::Block::compress(const vector <char> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::compressor::Block::compress(const vector <uint8_t> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::compress(const string &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::compress(const vector <char> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::compress(const vector <uint8_t> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::compress(const string &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::compress(const vector <char> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::compress(const vector <uint8_t> &, const method_t) const noexcept;
/**
 * @brief Шаблон метода компрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 */
auto awh::compressor::Block::compress(const void * buffer, const size_t size, const method_t method) const noexcept -> T {
	// Переменная результата
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем кодирование
		this->compress(buffer, size, method, result);
	// Возвращаем результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в строку
 *
 */
template string awh::compressor::Block::compress(const void *, const size_t, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::compress(const void *, const size_t, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::compress(const void *, const size_t, const method_t) const noexcept;
/**
 * @brief Шаблон метода компрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @param result контейнер куда следует положить результат
 */
void awh::compressor::Block::compress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод компрессии данных
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метод компрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем компрессию данных методом Lz4
				driver::lz4(buffer, size, this->_level[0], event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("LZ4: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен LZMA
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем компрессию данных методом LZMA
				driver::lzma(buffer, size, event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				driver::zstd(buffer, size, this->_level[2], event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				driver::gzip(buffer, size, this->_level[1], this->_gzip.wbits, event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Bzip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем компрессию данных методом Bzip2
				driver::bzip2(buffer, size, event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Lizard
			case static_cast <uint8_t> (method_t::LIZARD): {
				// Выполняем компрессию данных методом Lizard
				driver::lizard(buffer, size, this->_level[3], event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Lizard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Snappy
			case static_cast <uint8_t> (method_t::SNAPPY): {
				// Выполняем компрессию данных методом Snappy
				driver::snappy(buffer, size, event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Snappy: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Density
			case static_cast <uint8_t> (method_t::DENSITY): {
				// Выполняем компрессию данных методом Density
				driver::density(buffer, size, this->_level[4], event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Density: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Density: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				driver::brotli(buffer, size, event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем компрессию данных методом Deflate
				driver::deflate(buffer, size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.compress.load(std::memory_order_acquire), this->_gzip.buffer.compress->stream, event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии установлен Zlib (RFC 1950)
			case static_cast <uint8_t> (method_t::ZLIB): {
				// Выполняем компрессию данных методом Zlib
				driver::zlib(buffer, size, this->_level[1], this->_zlib.wbits, event_t::ENCODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Compress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Zlib: %s", log_t::flag_t::WARNING, "Compress failed");
					#endif
				}
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Возвращаем переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в строку
 *
 */
template void awh::compressor::Block::compress(const void *, const size_t, const method_t, string &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в буфер
 *
 */
template void awh::compressor::Block::compress(const void *, const size_t, const method_t, vector <char> &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода компрессии данных с выводом результата в бинарный буфер
 *
 */
template void awh::compressor::Block::compress(const void *, const size_t, const method_t, vector <uint8_t> &) const noexcept;
/**
 * @brief Шаблон метода декомпрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 */
auto awh::compressor::Block::decompress(string_view buffer, const method_t method) const noexcept -> T {
	// Переменная результата
	T result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем декомпрессию
		this->decompress(&buffer[0], buffer.size(), method, result);
	// Возвращаем результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в строку
 *
 */
template string awh::compressor::Block::decompress(string_view, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::decompress(string_view, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::decompress(string_view, const method_t) const noexcept;
/**
 * @brief Шаблон метода декомпрессии данных
 *
 * @tparam A тип возвращаемого результата
 * @tparam B тип буфера данных
 */
template <typename A, typename B>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 */
auto awh::compressor::Block::decompress(const B & buffer, const method_t method) const noexcept -> A {
	// Переменная результата
	A result;
	// Если буфер данных передан
	if(!buffer.empty())
		// Выполняем декомпрессию
		this->decompress(&buffer[0], buffer.size(), method, result);
	// Возвращаем результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в строку
 *
 */
template string awh::compressor::Block::decompress(const string &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из буфера с выводом результата в строку
 *
 */
template string awh::compressor::Block::decompress(const vector <char> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из бинарного буфера с выводом результата в строку
 *
 */
template string awh::compressor::Block::decompress(const vector <uint8_t> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::decompress(const string &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из буфера с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::decompress(const vector <char> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из бинарного буфера с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::decompress(const vector <uint8_t> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из строки с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::decompress(const string &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::decompress(const vector <char> &, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных из бинарного буфера с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::decompress(const vector <uint8_t> &, const method_t) const noexcept;
/**
 * @brief Шаблон метода декомпрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 */
auto awh::compressor::Block::decompress(const void * buffer, const size_t size, const method_t method) const noexcept -> T {
	// Переменная результата
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем декомпрессию
		this->decompress(buffer, size, method, result);
	// Возвращаем результат
	return result;
}
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в строку
 *
 */
template string awh::compressor::Block::decompress(const void *, const size_t, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в буфер
 *
 */
template vector <char> awh::compressor::Block::decompress(const void *, const size_t, const method_t) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в бинарный буфер
 *
 */
template vector <uint8_t> awh::compressor::Block::decompress(const void *, const size_t, const method_t) const noexcept;
/**
 * @brief Шаблон метода декомпрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @param result контейнер куда следует положить результат
 */
void awh::compressor::Block::decompress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод декомпрессии данных
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метод декомпрессии установлен LZ4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем декомпрессию данных методом LZ4
				driver::lz4(buffer, size, this->_level[0], event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("LZ4: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("LZ4: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен LZMA
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем декомпрессию данных методом LZMA
				driver::lzma(buffer, size, event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("LZMA: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				driver::zstd(buffer, size, this->_level[2], event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				driver::gzip(buffer, size, this->_level[1], this->_gzip.wbits, event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("GZip: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Bzip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем декомпрессию данных методом Bzip2
				driver::bzip2(buffer, size, event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Bzip2: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Lizard
			case static_cast <uint8_t> (method_t::LIZARD): {
				// Выполняем декомпрессию данных методом Lizard
				driver::lizard(buffer, size, this->_level[3], event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Lizard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Lizard: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Snappy
			case static_cast <uint8_t> (method_t::SNAPPY): {
				// Выполняем декомпрессию данных методом Snappy
				driver::snappy(buffer, size, event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Snappy: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Snappy: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Density
			case static_cast <uint8_t> (method_t::DENSITY): {
				// Выполняем декомпрессию данных методом Density
				driver::density(buffer, size, this->_level[4], event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Density: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Density: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				driver::brotli(buffer, size, event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем блокировку потоков
				const locker_t <> lock(this->_mtx);
				// Выполняем декомпрессию данных методом Deflate
				driver::deflate(buffer, size, this->_level[1], this->_gzip.wbits, this->_gzip.takeover.decompress.load(std::memory_order_acquire), this->_gzip.buffer.decompress->stream, event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Deflate: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии установлен Zlib (RFC 1950)
			case static_cast <uint8_t> (method_t::ZLIB): {
				// Выполняем декомпрессию данных методом Zlib
				driver::zlib(buffer, size, this->_level[1], this->_zlib.wbits, event_t::DECODE, result, this->_log);
				// Если результат операции пустой - значит произошла ошибка
				if(result.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Decompress failed");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Zlib: %s", log_t::flag_t::WARNING, "Decompress failed");
					#endif
				}
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Возвращаем переданный буфер данных
				result.assign(reinterpret_cast <const char *> (buffer), reinterpret_cast <const char *> (buffer) + size);
			break;
		}
	}
}
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в строку
 *
 */
template void awh::compressor::Block::decompress(const void *, const size_t, const method_t, string &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в буфер
 *
 */
template void awh::compressor::Block::decompress(const void *, const size_t, const method_t, vector <char> &) const noexcept;
/**
 * @brief Явный специализированный шаблон метода декомпрессии данных с выводом результата в бинарный буфер
 *
 */
template void awh::compressor::Block::decompress(const void *, const size_t, const method_t, vector <uint8_t> &) const noexcept;
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 */
awh::compressor::Block::Block(const log_t * log) noexcept :
 _level{
	1,
	Z_DEFAULT_COMPRESSION,
	ZSTD_CLEVEL_DEFAULT,
	LIZARD_DEFAULT_CLEVEL,
	DENSITY_ALGORITHM_LION
}, _log(log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
	// Выделяем память под контекст буфера GZip компрессии
	this->_gzip.buffer.compress = new gzip_stream_t();
	// Выделяем память под контекст буфера GZip декомпрессии
	this->_gzip.buffer.decompress = new gzip_stream_t();
	// Устанавливаем размер скользящего окна GZip по умолчанию
	this->_gzip.wbits = static_cast <int16_t> (MAX_WBITS);
	// Устанавливаем размер скользящего окна Zlib по умолчанию
	this->_zlib.wbits = static_cast <int16_t> (MAX_WBITS);
}
/**
 * @brief Деструктор
 *
 */
awh::compressor::Block::~Block() noexcept {
	// Если выделена память для компрессора
	if(this->_gzip.takeover.compress.load(std::memory_order_acquire))
		// Завершаем работу компрессора GZip
		::deflateEnd(&this->_gzip.buffer.compress->stream);
	// Если выделена память для декомпрессора
	if(this->_gzip.takeover.decompress.load(std::memory_order_acquire))
		// Завершаем работу декомпрессора GZip
		::inflateEnd(&this->_gzip.buffer.decompress->stream);
	// Освобождаем память контекста буфера GZip компрессии
	delete this->_gzip.buffer.compress;
	// Освобождаем память контекста буфера GZip декомпрессии
	delete this->_gzip.buffer.decompress;
}
