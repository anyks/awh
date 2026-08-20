/**
 * @file compressor.cpp
 * @date 2026-01-21
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
 * @brief Реализация блочной (one-shot) компрессии —
 *        сжатие и распаковка данных целиком за один вызов с управлением контекстами GZip, Zlib, Deflate, Brotli, LZ4,
 *        Zstd, LZma, BZip2, Lzip и других поддерживаемых алгоритмов
 *
 * @copyright Copyright © 2026
 *
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
 * @brief Полное определение непрозрачного контекста потока Deflate
 *
 * @details Обёртка над потоком zlib, скрывающая прямую зависимость от zlib в публичном заголовке
 *
 */
struct awh::compressor::deflate_stream_t {
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
	 *
	 */
	template <typename F>
	/**
	 * @brief Класс RAII-обёртки для гарантированного освобождения ресурса
	 *
	 * @details Вызывает переданную функцию очистки при выходе из области видимости
	 *          (в том числе при возникновении исключения), если не был вызван dismiss().
	 *
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
			 *
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
	 *
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
	 *
	 */
	template <typename F>
	/**
	 * @brief Функция создания RAII-обёртки освобождения ресурса
	 *
	 * @param fn функция освобождения ресурса
	 * @return   созданная RAII-обёртка
	 *
	 */
	static scope_exit_t <F> makeScopeExit(F fn) noexcept {
		// Возвращаем созданную RAII-обёртку
		return scope_exit_t <F> (std::move(fn));
	}
	/**
	 * @brief Класс хранителя контекстов Zstandard
	 *
	 * @details Заведение контекста стоит дороже самого сжатия кадра: образец из 256
	 *          кадров по 64 КБ идёт 46 МБ/с у OpenBSD при заведении контекста на всякий
	 *          кадр и 2356 МБ/с при его переиспользовании, у FreeBSD - 1029 против 3298.
	 *          Плату эту берёт возврат памяти контекста ядру, а не само сжатие
	 *
	 * @note Контекст держится в памяти ПОТОКА, а не в самом объекте компрессии: объект
	 *       делится между потоками по договору модуля, а контекст движка изменяем и
	 *       общим быть не может. Освобождается он вместе с потоком
	 *
	 */
	class Zstd {
		private:
			// Контекст потока компрессии
			ZSTD_CStream * _compress;
			// Контекст потока декомпрессии
			ZSTD_DStream * _decompress;
		public:
			/**
			 * @brief Метод извлечения контекста потока компрессии
			 *
			 * @return контекст потока компрессии либо nullptr, если завести его не удалось
			 *
			 */
			ZSTD_CStream * compress() noexcept {
				// Если контекст потока компрессии ещё не заведён
				if(this->_compress == nullptr)
					// Выполняем заведение контекста потока компрессии
					this->_compress = ::ZSTD_createCStream();
				// Выводим контекст потока компрессии
				return this->_compress;
			}
			/**
			 * @brief Метод извлечения контекста потока декомпрессии
			 *
			 * @return контекст потока декомпрессии либо nullptr, если завести его не удалось
			 *
			 */
			ZSTD_DStream * decompress() noexcept {
				// Если контекст потока декомпрессии ещё не заведён
				if(this->_decompress == nullptr)
					// Выполняем заведение контекста потока декомпрессии
					this->_decompress = ::ZSTD_createDStream();
				// Выводим контекст потока декомпрессии
				return this->_decompress;
			}
		public:
			/**
			 * @brief Метод снятия разросшегося контекста потока компрессии
			 *
			 * @details Держать между кадрами имеет смысл лишь скромный контекст. Уровень
			 *          сжатия задаёт размер словаря, и у наивысшего уровня контекст
			 *          занимает сотни мегабайт: удержание такого не ускоряет, а
			 *          выедает память. У OpenBSD, где предел данных процесса 1.5 ГБ,
			 *          удержанный контекст наивысшего уровня отнимал память у LZMA, и
			 *          сжатие ею отвечало отказом
			 *
			 * @param limit предел размера удерживаемого контекста в октетах
			 *
			 */
			void trimCompress(const size_t limit) noexcept {
				// Если контекст потока компрессии заведён и предел им превышен
				if((this->_compress != nullptr) && (::ZSTD_sizeof_CStream(this->_compress) > limit))
					// Выполняем снятие контекста потока компрессии
					this->resetCompress();
			}
			/**
			 * @brief Метод снятия разросшегося контекста потока декомпрессии
			 *
			 * @param limit предел размера удерживаемого контекста в октетах
			 *
			 */
			void trimDecompress(const size_t limit) noexcept {
				// Если контекст потока декомпрессии заведён и предел им превышен
				if((this->_decompress != nullptr) && (::ZSTD_sizeof_DStream(this->_decompress) > limit))
					// Выполняем снятие контекста потока декомпрессии
					this->resetDecompress();
			}
		public:
			/**
			 * @brief Метод снятия контекста потока компрессии
			 *
			 * @details Зовётся при отказе: движок мог остаться посреди кадра, и хоть
			 *          инициализация следующего кадра его и сбрасывает, держать после
			 *          отказа заведомо испорченный контекст незачем
			 *
			 */
			void resetCompress() noexcept {
				// Если контекст потока компрессии заведён
				if(this->_compress != nullptr){
					// Выполняем освобождение контекста потока компрессии
					::ZSTD_freeCStream(this->_compress);
					// Выполняем сброс контекста потока компрессии
					this->_compress = nullptr;
				}
			}
			/**
			 * @brief Метод снятия контекста потока декомпрессии
			 *
			 */
			void resetDecompress() noexcept {
				// Если контекст потока декомпрессии заведён
				if(this->_decompress != nullptr){
					// Выполняем освобождение контекста потока декомпрессии
					::ZSTD_freeDStream(this->_decompress);
					// Выполняем сброс контекста потока декомпрессии
					this->_decompress = nullptr;
				}
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Zstd() noexcept : _compress(nullptr), _decompress(nullptr) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~Zstd() noexcept {
				// Выполняем снятие контекста потока компрессии
				this->resetCompress();
				// Выполняем снятие контекста потока декомпрессии
				this->resetDecompress();
			}
	};
	/**
	 * @brief Функция извлечения контекстов Zstandard потока исполнения
	 *
	 * @return контексты Zstandard потока исполнения
	 *
	 */
	/**
	 * @brief Предел размера удерживаемого между кадрами контекста Zstandard
	 *
	 * @details Контекст уровня по умолчанию укладывается в единицы мегабайт, контекст
	 *          наивысшего уровня занимает сотни. Удерживается лишь первый
	 *
	 */
	static constexpr size_t ZSTD_CONTEXT_LIMIT = (8 * 1024 * 1024);

	static Zstd & zstdContexts() noexcept {
		// Контексты Zstandard потока исполнения
		static thread_local Zstd result;
		// Выводим контексты Zstandard потока исполнения
		return result;
	}
	/**
	 * @brief Шаблон функции проверки распакованных данных на превышение допустимого предела
	 *
	 * @tparam T тип контейнера результата
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция проверки распакованных данных на превышение допустимого предела
	 *
	 * @details Ограждает от недоверенного кадра, разворачивающегося в гигабайты: без
	 *          предела память отводилась бы по указанию отправляющей стороны. Договор
	 *          общий с потоковым режимом, где ту же работу делает coder_t::overflowed
	 *
	 * @param result контейнер с распакованными данными
	 * @param tag    название движка для записи в лог
	 * @param log    объект для работы с логами
	 * @return       результат проверки
	 *
	 */
	static bool overflowed(const T & result, const char * tag, const log_t * log) noexcept {
		// Если объём распакованных данных допустимого предела не превышает
		if(static_cast <uint64_t> (result.size()) <= static_cast <uint64_t> (AWH_COMPRESSOR_MAX_OUTPUT))
			// Выводим отрицательный результат
			return false;
		// Записываем ошибку в лог
		log->print("%s: %s", log_t::flag_t::WARNING, tag, "Decompressed data exceeds the allowed limit");
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки объёма распакованных данных на превышение допустимого предела
	 *
	 * @details Заведена для движков, отводящих буфер наперёд: у них длина контейнера
	 *          есть отведённое место, а не собранные данные, и сверять предел следует
	 *          с объёмом, движком выданным
	 *
	 * @param produced объём распакованных данных
	 * @param tag      название движка для записи в лог
	 * @param log      объект для работы с логами
	 * @return         результат проверки
	 *
	 */
	static bool overflowed(const size_t produced, const char * tag, const log_t * log) noexcept {
		// Если объём распакованных данных допустимого предела не превышает
		if(static_cast <uint64_t> (produced) <= static_cast <uint64_t> (AWH_COMPRESSOR_MAX_OUTPUT))
			// Выводим отрицательный результат
			return false;
		// Записываем ошибку в лог
		log->print("%s: %s", log_t::flag_t::WARNING, tag, "Decompressed data exceeds the allowed limit");
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Шаблон функции работы с компрессором LZma
	 *
	 * @tparam T сигнатура функции
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором LZma
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param level  пресет компрессии (0 - 9)
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 *
	 */
	static void lzma(const void * buffer, const size_t size, const int32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
						lzma_options_lzma options{};
						// Выполняем нормализацию переданного пресета компрессии
						const uint32_t preset = (((level >= 0) && (level <= 9)) ? static_cast <uint32_t> (level) : static_cast <uint32_t> (LZMA_PRESET_DEFAULT));
						// Если применить пресет компрессии не удалось
						if(::lzma_lzma_preset(&options, preset)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
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
						// Инициализируем фильтры компрессора LZma
						const lzma_filter filters[] = {
							{LZMA_FILTER_LZMA2, &options},
							{LZMA_VLI_UNKNOWN, nullptr}
						};
						// Актуальный размер сжатых данных
						size_t actual = 0;
						// Вычисляем максимально возможный размер сжатых данных (с учётом заголовка/подвала контейнера)
						const size_t bound = ::lzma_stream_buffer_bound(size);
						// Выделяем буфер памяти нужного нам размера
						result.resize(bound, 0);
						// Выполняем компрессию буфера данных
						lzma_ret rv = ::lzma_stream_buffer_encode(const_cast <lzma_filter *> (filters), LZMA_CHECK_CRC32, nullptr, reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (&result[0]), &actual, bound);
						// Если мы получили ошибку
						if(rv != LZMA_OK){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
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
						uint64_t memlimit = AWH_COMPRESSOR_LZMA_MEMLIMIT;
						// Позиции в буферах и актуальный размер данных результата
						size_t inpos = 0, outpos = 0, actual = 0;
						// Размер распакованных данных в разрядности движка
						uint64_t expected = 0;
						/**
						 * Подвал контейнера занимает 12 октетов, и на входе короче него
						 * вычитание ушло бы за ноль беззнаковой разрядностью, уводя указатель за буфер
						 */
						if(size < 12)
							// Переходим к выводу ошибки
							goto Error;
						// Смещаем указатель в буфере на подвал
						ptr = (const_cast <char *> (reinterpret_cast <const char *> (buffer)) + (size - 12));
						// Список флагов потока LZma
						lzma_stream_flags flags;
						// Пытаемся декодировать подвал архива
						if(::lzma_stream_footer_decode(&flags, reinterpret_cast <uint8_t *> (ptr)) != LZMA_OK)
							// Переходим к выводу ошибки
							goto Error;
						/**
						 * Сличаем смещения, а не указатели: вычитание испорченного размера
						 * увело бы указатель за начало буфера ещё до сравнения, а такая
						 * арифметика неопределена сама по себе, независимо от того, чем кончится сличение
						 */
						if(flags.backward_size > (size - 12))
							// Переходим к выводу ошибки
							goto Error;
						// Смещаем указатель в буфере на начало индекса
						ptr -= flags.backward_size;
						// Выполняем декодирование буфера LZma
						if(::lzma_index_buffer_decode(&index, &memlimit, nullptr, reinterpret_cast <uint8_t *> (ptr), &inpos, size - (ptr - reinterpret_cast <const char *> (buffer))) != LZMA_OK)
							// Переходим к выводу ошибки
							goto Error;
						// Сбрасываем позицию во входящем буфере
						inpos = 0;
						// Сбрасываем лимит доступной памяти
						memlimit = AWH_COMPRESSOR_LZMA_MEMLIMIT;
						/**
						 * Размер снимается в разрядность движка, а не в разрядность памяти:
						 * приведение к размеру памяти на 32-разрядной сборке обрезало бы
						 * старшую половину, и подделанный подвал прошёл бы стража с обрезком
						 */
						// Получаем размер результирующего буфера данных
						expected = ::lzma_index_uncompressed_size(index);
						/**
						 * Отвергаем нулевой размер и размер свыше допустимого предела: и то,
						 * и другое означает подделанный подвал. Нулевой распакованный размер
						 * законным контейнером не бывает — пустой вход модуль до движка не доводит
						 */
						if((expected == 0) || (expected > static_cast <uint64_t> (AWH_COMPRESSOR_MAX_OUTPUT)) || (expected > static_cast <uint64_t> (SIZE_MAX)))
							// Переходим к выводу ошибки
							goto Error;
						// Снимаем размер в разрядность памяти сборки
						actual = static_cast <size_t> (expected);
						// Выделяем буфер памяти нужного нам размера
						result.resize(actual, 0);
						// Выполняем декомпрессию буфера бинарных данных
						if(::lzma_stream_buffer_decode(&memlimit, 0, nullptr, reinterpret_cast <const uint8_t *> (buffer), &inpos, size, reinterpret_cast <uint8_t *> (&result[0]), &outpos, actual) == LZMA_OK){
							/**
							 * Буфер режется по выписанному движком, а не по размеру из подвала:
							 * величины эти у исправного кадра совпадают, но берётся первая -
							 * подвал приходит от отправляющей стороны, а выписанное движок знает
							 * сам, и расхождение дало бы наружу хвост неписаных октетов
							 */
							// Корректируем размер результирующего буфера по выписанному движком
							result.resize(outpos);
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
							log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
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
					log->debug("LZMA: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
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
	 * @brief Функция получения собранного движком BZip2 объёма данных
	 *
	 * @details Счётчик собранного разложен движком на две половины по 32 бита,
	 *          и чтение одной лишь младшей обрезало бы объём свыше четырёх гигабайт
	 *
	 * @param stream объект потока BZip2
	 * @return       собранный объём данных
	 *
	 */
	static uint64_t produced(const bz_stream & stream) noexcept {
		// Выводим собранный объём, склеенный из обеих половин счётчика
		return ((static_cast <uint64_t> (stream.total_out_hi32) << 32) | static_cast <uint64_t> (stream.total_out_lo32));
	}
	/**
	 * @brief Шаблон функции работы с компрессором BZip2
	 *
	 * @tparam T сигнатура функции
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором BZip2
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param level  размер рабочего блока в единицах по 100 килобайт (1 - 9)
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 *
	 */
	static void bzip2(const void * buffer, const size_t size, const int32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
						// Выполняем нормализацию размера рабочего блока
						const int32_t block = (((level >= 1) && (level <= 9)) ? level : 9);
						// Выполняем инициализацию потока
						if(::BZ2_bzCompressInit(&stream, block, 0, 0) != BZ_OK){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
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
						uint64_t capacity = (static_cast <uint64_t> (size) + (static_cast <uint64_t> (size) / 100) + 600);
						// Минимальный размер буфера должен быть не менее 1024 байт
						if(capacity < 1024)
							// Устанавливаем минимальный размер буфера
							capacity = 1024;
						/**
						 * Разрядность указателя ниже разрядности расчёта: на 32-разрядной сборке
						 * оценка худшего случая за размер памяти выйти может, и приведение её
						 * обрезало бы величину, а не отвергло. Отвергаем прямо
						 */
						// Если оценка выходит за разрядность памяти сборки
						if(capacity > static_cast <uint64_t> (SIZE_MAX)){
							// Записываем ошибку в лог
							log->print("Bzip2: %s", log_t::flag_t::WARNING, "Invalid compression size");
							// Выходим из функции
							return;
						}
						// Выделяем память на результирующий буфер
						result.resize(static_cast <size_t> (capacity), 0);
						// Заполняем входные данные буфера
						stream.next_in = const_cast <char *> (reinterpret_cast <const char *> (buffer));
						// Указываем размер входного буфера
						stream.avail_in = static_cast <uint32_t> (size);
						// Устанавливаем буфер для получения результата
						stream.next_out = reinterpret_cast <char *> (&result[0]);
						// Устанавливаем максимальный размер буфера, не выходя за разрядность движка
						stream.avail_out = static_cast <uint32_t> (::min <uint64_t> (static_cast <uint64_t> (result.size()), static_cast <uint64_t> (UINT32_MAX)));
						// Результат выполнения компрессии
						int32_t ret = BZ_OK;
						// Переменная подсчёта сжатых данных
						size_t produced = 0;
						/**
						 * Выполняем компрессию до завершения данных
						 */
						do {
							// Запоминаем остаток непринятого входа до захода
							const uint32_t remaining = stream.avail_in;
							// Запоминаем собранный движком объём до захода
							const uint64_t collected = driver::produced(stream);
							// Выполняем компрессию ещё одной порции данных
							ret = ::BZ2_bzCompress(&stream, BZ_FINISH);
							/**
							 * Заход, ничего не взявший и ничего не выдавший при незаполненном
							 * выходе, не возьмёт и не выдаст их и впредь: доводы захода те же
							 * самые. Заполненный выход застоем не является - там движку и правда
							 * недостаёт места, и работа его добавляет
							 */
							if((ret == BZ_FINISH_OK) && (stream.avail_out > 0) && (stream.avail_in == remaining) && (driver::produced(stream) == collected)){
								// Выполняем очистку буфера данных
								result.clear();
								// Записываем ошибку в лог
								log->print("Bzip2: %s", log_t::flag_t::WARNING, "Error during data compression");
								// Выходим из функции
								return;
							}
							// Если нужно больше места для данных
							if(ret == BZ_FINISH_OK){
								// Нужно больше места — расширяем буфер
								produced = static_cast <size_t> (driver::produced(stream));
								/**
								 * Удвоение считается с запасом по разрядности и режется по размеру
								 * памяти сборки: обрезанная величина дала бы буфер меньше собранного,
								 * а разность длины и собранного ушла бы в переполнение - движок
								 * получил бы окно записи за концом буфера
								 */
								// Вычисляем новую длину буфера, не выходя за разрядность памяти
								const uint64_t enlarged = ::min <uint64_t> (static_cast <uint64_t> (result.size()) * 2, static_cast <uint64_t> (SIZE_MAX));
								// Если места под запись не прибавилось, работу продолжать нельзя
								if(enlarged <= static_cast <uint64_t> (produced)){
									// Выполняем очистку результата
									result.clear();
									// Записываем ошибку в лог
									log->print("Bzip2: %s", log_t::flag_t::WARNING, "Error during data compression");
									// Выходим из функции
									return;
								}
								// Увеличиваем буфер в два раза
								result.resize(static_cast <size_t> (enlarged));
								// Устанавливаем максимальный размер буфера, не выходя за разрядность движка
								stream.avail_out = static_cast <uint32_t> (::min <uint64_t> (static_cast <uint64_t> (result.size() - produced), static_cast <uint64_t> (UINT32_MAX)));
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
									log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
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
						result.resize(static_cast <size_t> (driver::produced(stream)));
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
								log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
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
						/**
						 * Начальный размер буфера — эвристика, и предел ей не указ: буфер
						 * есть отведённое место, а не собранные данные, и отвергать по нему
						 * значило бы отвергать законный кадр, до предела не доходящий.
						 * Само отведение всё же ограничивается сверху пределом, чтобы
						 * догадка не забрала памяти больше, чем работе позволено выдать
						 */
						// Начальный размер буфера — эвристика
						const size_t capacity = static_cast <size_t> (::min <uint64_t> (::max <uint64_t> (1024, static_cast <uint64_t> (size) * 2), static_cast <uint64_t> (AWH_COMPRESSOR_MAX_OUTPUT)));
						// Выделяем память на результирующий буфер
						result.resize(capacity, 0);
						// Результат выполнения компрессии
						int32_t ret = BZ_OK;
						/**
						 * Выполняем компрессию всех данных
						 */
						do {
							// Получаем собранный движком объём данных
							const size_t collected = static_cast <size_t> (driver::produced(stream));
							// Запоминаем остаток неразобранного входа до захода
							const uint32_t remaining = stream.avail_in;
							// Убедимся, что есть место для записи
							if(collected >= result.size()){
								/**
								 * Предел сверяется с объёмом, движком выданным, а не с длиной
								 * буфера: буфер растёт удвоением и предел перешагивает, его не
								 * достигнув, - сверка по нему отвергала бы законный кадр, до
								 * предела не дошедший
								 */
								// Если распакованные данные превысили допустимый предел
								if(driver::overflowed(collected, "Bzip2", log)){
									// Выполняем очистку результата
									result.clear();
									// Выходим из функции
									return;
								}
								/**
								 * Удвоение предел перешагивает, не достигнув его: прежде чем счесть
								 * данные повреждёнными, отводим буфер ровно по пределу и пробуем ещё раз
								 */
								// Увеличиваем буфер в два раза, не выходя за допустимый предел
								result.resize(static_cast <size_t> (::min <uint64_t> (static_cast <uint64_t> (result.size()) * 2, static_cast <uint64_t> (AWH_COMPRESSOR_MAX_OUTPUT))));
								// Если места под запись не прибавилось, предел исчерпан
								if(collected >= result.size()){
									// Записываем ошибку в лог
									log->print("%s: %s", log_t::flag_t::WARNING, "Bzip2", "Decompressed data exceeds the allowed limit");
									// Выполняем очистку результата
									result.clear();
									// Выходим из функции
									return;
								}
							}
							// Устанавливаем буфер для получения результата
							stream.next_out = reinterpret_cast <char *> (&result[0] + collected);
							// Устанавливаем максимальный размер буфера, не выходя за разрядность движка
							stream.avail_out = static_cast <uint32_t> (::min <uint64_t> (static_cast <uint64_t> (result.size() - collected), static_cast <uint64_t> (UINT32_MAX)));
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
									log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
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
									log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Truncated or corrupted data");
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
							 * Заход, оставивший неразобранный вход нетронутым и не выдавший ни
							 * октета выхода, не разберёт его и впредь: доводы захода те же самые.
							 * Тот же страж стоит у потокового кодека
							 */
							if((ret != BZ_STREAM_END) && (stream.avail_in > 0) && (stream.avail_in == remaining) && (static_cast <size_t> (driver::produced(stream)) == collected)){
								// Выполняем очистку буфера данных
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Bzip2: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							}
						/**
						 * Если данные ещё не извлечены
						 */
						} while(ret != BZ_STREAM_END);
						// Обрезаем до фактически распакованного размера
						result.resize(static_cast <size_t> (driver::produced(stream)));
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
					log->debug("Bzip2: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
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
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция работы с компрессором Brotli
	 *
	 * @param buffer буфер данных для компрессии
	 * @param size   размер данных для компрессии
	 * @param level  качество компрессии (0 - 11)
	 * @param event  событие выполнения операции
	 * @param result строка куда следует положить результат
	 * @param log    объект для работы с логами
	 *
	 */
	static void brotli(const void * buffer, const size_t size, const int32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
								log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
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
						// Устанавливаем качество компрессии (если оно укладывается в допустимый диапазон)
						if((level >= BROTLI_MIN_QUALITY) && (level <= BROTLI_MAX_QUALITY))
							// Выполняем установку качества компрессии
							::BrotliEncoderSetParameter(encoder, BROTLI_PARAM_QUALITY, static_cast <uint32_t> (level));
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(size);
						/**
						 * Выполняем сжатие данных
						 */
						while(!::BrotliEncoderIsFinished(encoder)){
							// Запоминаем остаток неразобранного входа до захода
							const size_t remaining = sizeInput;
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
									log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data compression");
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
							const size_t produced = (data.size() - sizeOutput);
							// Если данные получены, формируем результирующий буфер
							if(produced > 0){
								// Получаем буфер полученных данных
								const char * chunk = reinterpret_cast <const char *> (&data[0]);
								// Формируем результирующий буфер бинарных данных
								result.insert(result.end(), chunk, chunk + produced);
							}
							/**
							 * Заход, ничего не взявший и ничего не выдавший, не возьмёт и не
							 * выдаст их и впредь: доводы захода те же самые. Заход, выставивший
							 * признак завершённости, под стража не идёт - работа на нём и кончается
							 */
							if(!::BrotliEncoderIsFinished(encoder) && !::BrotliEncoderHasMoreOutput(encoder) && (sizeInput == remaining) && (produced == 0)){
								// Выполняем очистку результата
								result.clear();
								// Записываем ошибку в лог
								log->print("Brotli: %s", log_t::flag_t::WARNING, "Error during data compression");
								// Выходим из функции
								return;
							}
						}
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Полный размер обработанных данных и размер очередной полученной порции
						size_t total = 0, produced = 0;
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
								log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
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
							// Запоминаем остаток неразобранного входа до захода
							const size_t remaining = sizeInput;
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
									log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
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
							produced = (data.size() - sizeOutput);
							// Если данные получены, формируем результирующий буфер
							if(produced > 0){
								// Получаем буфер полученных данных
								const char * chunk = reinterpret_cast <const char *> (&data[0]);
								// Формируем результирующий буфер бинарных данных
								result.insert(result.end(), chunk, chunk + produced);
								// Если распакованные данные превысили допустимый предел
								if(driver::overflowed(result, "Brotli", log)){
									// Выполняем очистку результата
									result.clear();
									// Выходим из функции
									return;
								}
							}
							/**
							 * Движок просит места под выход, но выхода не даёт и входа не убавляет:
							 * доводы следующего захода те же самые, и просьба эта не кончится
							 */
							if((sizeInput == remaining) && (produced == 0)){
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Brotli: %s", log_t::flag_t::WARNING, "Error during data decompression");
								#endif
								// Выходим из функции
								return;
							}
						}
						// Если декомпрессия данных выполнена не удачно (в т.ч. усечённый вход — NEEDS_MORE_INPUT)
						if(ret != BROTLI_DECODER_RESULT_SUCCESS){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data decompression");
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
					log->debug("Brotli: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
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
	 *
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
	 *
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
				/**
				 * Распакованный размер записан в самом кадре, и движок отводит по нему память
				 * прежде, чем разбирать данные: без сверки крошечный кадр недоверенной стороны
				 * заставил бы отвести гигабайты. Прочие движки о том же событии узнают по ходу
				 * накопления, здесь же оно известно наперёд - тем и пользуемся
				 */
				// Если производится декомпрессия данных
				if(event == compressor::event_t::DECODE){
					// Распакованный размер, записанный в кадре
					size_t expected = 0;
					// Если распакованный размер из кадра не извлекается, кадр повреждён
					if(!snappy::GetUncompressedLength(reinterpret_cast <const char *> (buffer), size, &expected)){
						// Записываем ошибку в лог
						log->print("%s: %s", log_t::flag_t::WARNING, "Snappy", "Error during data decompression");
						// Выходим из функции
						return;
					}
					// Если распакованные данные превысят допустимый предел
					if(driver::overflowed(expected, "Snappy", log))
						// Выходим из функции
						return;
				}
				// Результат выполнения операции
				bool ok = false;
				/**
				 * Если результат уже требуемого типа, движок пишет прямо в него — без промежуточной копии
				 */
				if constexpr(is_same <T, string>::value){
					/**
					 * Определяем событие выполнения операции
					 */
					switch(static_cast <uint8_t> (event)){
						// Если необходимо выполнить компрессию данных
						case static_cast <uint8_t> (compressor::event_t::ENCODE):
							// Выполняем компрессию данных
							ok = (snappy::Compress(reinterpret_cast <const char *> (buffer), size, &result) > 0);
						break;
						// Если необходимо выполнить декомпрессию данных
						case static_cast <uint8_t> (compressor::event_t::DECODE):
							// Выполняем декомпрессию данных
							ok = snappy::Uncompress(reinterpret_cast <const char *> (buffer), size, &result);
						break;
					}
				/**
				 * Для остальных типов контейнера используем промежуточный буфер
				 */
				} else {
					// Временный промежуточный буфер данных
					string data = "";
					/**
					 * Определяем событие выполнения операции
					 */
					switch(static_cast <uint8_t> (event)){
						// Если необходимо выполнить компрессию данных
						case static_cast <uint8_t> (compressor::event_t::ENCODE):
							// Выполняем компрессию данных
							ok = (snappy::Compress(reinterpret_cast <const char *> (buffer), size, &data) > 0);
						break;
						// Если необходимо выполнить декомпрессию данных
						case static_cast <uint8_t> (compressor::event_t::DECODE):
							// Выполняем декомпрессию данных
							ok = snappy::Uncompress(reinterpret_cast <const char *> (buffer), size, &data);
						break;
					}
					// Если результат получен, формируем его
					if(ok && !data.empty())
						// Формируем результат
						result.assign(data.begin(), data.end());
				}
				// Если операция не выполнена
				if(!ok){
					// Выполняем очистку блока с результатом
					result.clear();
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("Snappy: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during data processing");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог в лог
						log->print("Snappy: %s", log_t::flag_t::WARNING, "Error during data processing");
					#endif
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
	 *
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
	 *
	 */
	static void density(const void * buffer, const size_t size, const int32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
		// Если буфер данных передан
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку блока с результатом
				result.clear();
				// Максимальный размер выходного буфера
				constexpr uint64_t MAX_OUTPUT_SIZE = AWH_COMPRESSOR_MAX_OUTPUT;
				/**
				 * Определяем событие выполнения операции
				 */
				switch(static_cast <uint8_t> (event)){
					// Если необходимо выполнить компрессию данных
					case static_cast <uint8_t> (compressor::event_t::ENCODE): {
						// Выполняем получение размер результирующего буфера
						const uint_fast64_t actual = ::density_compress_safe_size(size);
						/**
						 * Предел выхода здесь не сторожит: он заведён от усиления недоверенного
						 * входа, а на сжатии выход ограничен входом, который подала сама
						 * вызывающая сторона - отказ был бы ложным
						 */
						/**
						 * Оценка сличается и с размером памяти сборки: движок считает её в своей
						 * разрядности, а буфер отводится в разрядности памяти - на 32-разрядной
						 * сборке приведение обрезало бы величину, и движок, получив под запись
						 * необрезанную, ушёл бы за конец буфера
						 */
						// Если размер выделить не удалось либо он выходит за разрядность памяти
						if((actual == 0) || (actual > static_cast <uint_fast64_t> (SIZE_MAX))){
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
						if((status.state != DENSITY_STATE_OK) || (status.bytesWritten == 0)){
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
						/**
						 * Движок принимает ожидаемый размер РАСПАКОВАННЫХ данных, а формат его
						 * не несёт: степень сжатия заранее неизвестна, поэтому предположение
						 * наращивается, пока движку хватает места
						 */
						uint_fast64_t expected = (static_cast <uint_fast64_t> (size) * 4);
						// Признак попытки на всём допустимом пределе
						bool clamped = false;
						/**
						 * Выполняем извлечение данных пока не извлечём
						 */
						for(;;){
							// Выполняем получение размера результирующего буфера
							uint_fast64_t actual = ::density_decompress_safe_size(expected);
							/**
							 * Удвоение предположения предел перешагивает, не достигнув его: прежде
							 * чем счесть данные повреждёнными, отводим буфер ровно по пределу и
							 * пробуем ещё раз - тот же порядок у LZ4 и Lizard
							 */
							// Если предположение предел перешагнуло, а попытки на нём ещё не было
							if((actual > MAX_OUTPUT_SIZE) && !clamped){
								// Отводим буфер ровно по допустимому пределу
								actual = MAX_OUTPUT_SIZE;
								// Отмечаем попытку на всём допустимом пределе
								clamped = true;
							}
							// Если размер выделить не удалось либо он превышает допустимый предел
							if((actual == 0) || (actual > MAX_OUTPUT_SIZE)){
								// Выполняем очистку блока с результатом
								result.clear();
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
							// Если движку не хватило места, наращиваем предположение и повторяем
							if(status.state == DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL){
								// Выполняем удвоение ожидаемого размера распакованных данных
								expected <<= 1;
								// Переходим к следующей попытке
								continue;
							}
							// Если мы получили ошибку
							if((status.state != DENSITY_STATE_OK) || (status.bytesWritten == 0)){
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
							result.resize(static_cast <size_t> (status.bytesWritten));
							// Выходим из цикла
							break;
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
	 *
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
	 *
	 */
	static void lizard(const void * buffer, const size_t size, const int32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
						/**
						 * Предел выхода здесь не сторожит: он заведён от усиления недоверенного
						 * входа, а на сжатии выход ограничен входом, который подала сама
						 * вызывающая сторона - отказ был бы ложным
						 */
						// Если размер выделен
						if(actual <= 0){
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
						/**
						 * Верхний предел размера выходного буфера (защита от повреждённых данных).
						 * Предел взят постоянным, а не кратным размеру входа: у обоих форматов
						 * степень сжатия ничем сверху не ограничена, и кратный предел отвергал бы
						 * законный сильно сжатый кадр.
						 *
						 * Общий предел режется здесь разрядностью самого движка: длина буфера
						 * уходит ему знаковым 32-разрядным числом, и предел выше этой величины -
						 * а он настраивается сборкой - обратился бы при передаче в отрицательный
						 */
						constexpr size_t MAX_OUTPUT_SIZE = static_cast <size_t> (::min <uint64_t> (static_cast <uint64_t> (AWH_COMPRESSOR_MAX_OUTPUT), static_cast <uint64_t> (INT32_MAX)));
						/**
						 * Начальная догадка взята с запасом: движок отвечает одним отрицательным
						 * числом и на нехватку места, и на порчу, поэтому каждая недостача стоит
						 * полной распаковки заново. Догадка вдвое от входа не покрывает даже
						 * обычной степени сжатия, и честный кадр платил бы несколькими заходами
						 */
						// Начальный размер выходного буфера
						size_t capacity = static_cast <size_t> (::min <uint64_t> (::max <uint64_t> (static_cast <uint64_t> (size) * 4, 0x10000), static_cast <uint64_t> (MAX_OUTPUT_SIZE)));
						// Признак попытки на всём допустимом пределе
						bool clamped = false;
						/**
						 * Выполняем извлечение данных пока не извлечём
						 */
						for(;;){
							/**
							 * Удвоение предел перешагивает, не достигнув его: прежде чем счесть
							 * данные повреждёнными, отводим буфер ровно по пределу и пробуем ещё раз
							 */
							if((capacity > MAX_OUTPUT_SIZE) && !clamped){
								// Отводим буфер ровно по допустимому пределу
								capacity = MAX_OUTPUT_SIZE;
								// Отмечаем попытку на всём допустимом пределе
								clamped = true;
							}
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
	 *
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
	 *
	 */
	static void lz4(const void * buffer, const size_t size, const int32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
						/**
						 * Уровень трактуется так же, как его трактует кадровый формат LZ4:
						 * от LZ4HC_CLEVEL_MIN включается режим высокой степени сжатия, а
						 * отрицательное значение задаёт ускорение быстрого режима.
						 * Один смысл на оба режима избавляет от переворота направления уровня
						 */
						if(level >= LZ4HC_CLEVEL_MIN)
							// Выполняем компрессию буфера бинарных данных режимом высокой степени сжатия
							actual = ::LZ4_compress_HC(reinterpret_cast <const char *> (buffer), reinterpret_cast <char *> (&result[0]), size, actual, level);
						// Если задан быстрый режим компрессии
						else actual = ::LZ4_compress_fast(reinterpret_cast <const char *> (buffer), reinterpret_cast <char *> (&result[0]), size, actual, ((level < 0) ? (1 - level) : 1));
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
						/**
						 * Верхний предел размера выходного буфера (защита от повреждённых данных).
						 * Предел взят постоянным, а не кратным размеру входа: у обоих форматов
						 * степень сжатия ничем сверху не ограничена, и кратный предел отвергал бы
						 * законный сильно сжатый кадр.
						 *
						 * Общий предел режется здесь разрядностью самого движка: длина буфера
						 * уходит ему знаковым 32-разрядным числом, и предел выше этой величины -
						 * а он настраивается сборкой - обратился бы при передаче в отрицательный
						 */
						constexpr size_t MAX_OUTPUT_SIZE = static_cast <size_t> (::min <uint64_t> (static_cast <uint64_t> (AWH_COMPRESSOR_MAX_OUTPUT), static_cast <uint64_t> (INT32_MAX)));
						/**
						 * Начальная догадка взята с запасом: движок отвечает одним отрицательным
						 * числом и на нехватку места, и на порчу, поэтому каждая недостача стоит
						 * полной распаковки заново. Догадка вдвое от входа не покрывает даже
						 * обычной степени сжатия, и честный кадр платил бы несколькими заходами
						 */
						// Начальный размер выходного буфера
						size_t capacity = static_cast <size_t> (::min <uint64_t> (::max <uint64_t> (static_cast <uint64_t> (size) * 4, 0x10000), static_cast <uint64_t> (MAX_OUTPUT_SIZE)));
						// Признак попытки на всём допустимом пределе
						bool clamped = false;
						/**
						 * Выполняем извлечение данных пока не извлечём
						 */
						for(;;){
							/**
							 * Удвоение предел перешагивает, не достигнув его: прежде чем счесть
							 * данные повреждёнными, отводим буфер ровно по пределу и пробуем ещё раз
							 */
							if((capacity > MAX_OUTPUT_SIZE) && !clamped){
								// Отводим буфер ровно по допустимому пределу
								capacity = MAX_OUTPUT_SIZE;
								// Отмечаем попытку на всём допустимом пределе
								clamped = true;
							}
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
	 *
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
	 *
	 */
	static void zstd(const void * buffer, const size_t size, const int32_t level, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
						// Выполняем извлечение контекста потока из памяти потока исполнения
						ZSTD_CStream * ctx = driver::zstdContexts().compress();
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
						/**
						 * Контекст живёт в памяти потока исполнения и между кадрами не
						 * рушится: заведение его стоит дороже самого сжатия. Снимаем его
						 * лишь при отказе, а на успешном выходе страж отзывается
						 */
						auto guard = driver::makeScopeExit([]() noexcept {
							// Выполняем снятие контекста потока компрессии
							driver::zstdContexts().resetCompress();
						});
						// Резервируем память под результат для снижения числа реаллокаций
						result.reserve(size);
						// Выполняем инициализацию потока
						size_t status = ::ZSTD_initCStream(ctx, ((level != -1) ? level : ZSTD_CLEVEL_DEFAULT));
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
								// Запоминаем положение разбора до захода
								const size_t consumed = input.pos;
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
								/**
								 * Заход, не взявший ни октета входа и не выдавший ни октета выхода,
								 * не возьмёт и не выдаст их и впредь: доводы захода те же самые
								 */
								if((input.pos == consumed) && (output.pos == 0)){
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
										// Записываем ошибку в лог
										log->print("Zstandard: %s", log_t::flag_t::WARNING, "Error during data compression");
									#endif
									// Выходим из функции
									return;
								}
							}
							// Увеличиваем смещение в исходном буфере необработанных данных
							offset += actual;
						}
						/**
						 * Завершаем поток. Движок отвечает количеством оставшегося к записи,
						 * поэтому вызов повторяется, пока эпилог кадра не выписан целиком
						 */
						do {
							// Сбрасываем позицию буфера
							output.pos = 0;
							// Завершаем поток
							status = ::ZSTD_endStream(ctx, &output);
							// Если мы получили ошибку завершения потока
							if(::ZSTD_isError(status))
								// Выходим из цикла
								break;
							// Выполняем формирование полученных данных
							result.insert(result.end(), data.get(), data.get() + output.pos);
							/**
							 * Заход, не выдавший ни октета выхода, не выдаст их и впредь: буфер
							 * выхода отведён движком по его же мерке и пуст к началу всякого
							 * захода, а доводы захода те же самые. Тот же страж стоит у самого
							 * сжатия выше - здесь его недоставало, и движок, объявивший остаток
							 * к записи, но его не пишущий, вращал бы цикл без конца
							 */
							// Если движок продвижения не сделал
							if((status > 0) && (output.pos == 0)){
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Error during stream finalization");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log->print("Zstandard: %s", log_t::flag_t::WARNING, "Error during stream finalization");
								#endif
								// Выходим из функции
								return;
							}
						/**
						 * Пока эпилог кадра не выписан целиком
						 */
						} while(status > 0);
						// Если мы получили ошибку завершения потока
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
						// Кадр выписан целиком - контекст годен под следующий, страж отзывается
						guard.dismiss();
						// Выполняем снятие контекста, коли тот разросся сверх предела
						driver::zstdContexts().trimCompress(driver::ZSTD_CONTEXT_LIMIT);
					} break;
					// Если необходимо выполнить декомпрессию данных
					case static_cast <uint8_t> (compressor::event_t::DECODE): {
						// Выполняем извлечение контекста потока из памяти потока исполнения
						ZSTD_DStream * ctx = driver::zstdContexts().decompress();
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
						/**
						 * Контекст живёт в памяти потока исполнения и между кадрами не
						 * рушится: заведение его стоит дороже самой распаковки. Снимаем
						 * его лишь при отказе, а на успешном выходе страж отзывается
						 */
						auto guard = driver::makeScopeExit([]() noexcept {
							// Выполняем снятие контекста потока декомпрессии
							driver::zstdContexts().resetDecompress();
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
								// Запоминаем положение разбора до захода
								const size_t consumed = input.pos;
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
								// Если распакованные данные превысили допустимый предел
								if(driver::overflowed(result, "Zstandard", log)){
									// Выполняем очистку результата
									result.clear();
									// Выходим из функции
									return;
								}
								/**
								 * Заход, не взявший ни октета входа и не выдавший ни октета выхода,
								 * не возьмёт и не выдаст их и впредь: доводы захода те же самые.
								 * Тот же страж стоит у потокового кодека
								 */
								if((input.pos == consumed) && (output.pos == 0)){
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
							}
							// Увеличиваем смещение в исходном буфере необработанных данных
							offset += actual;
						}
						/**
						 * Вход исчерпан, но накопленное движком могло не уместиться в выходной
						 * буфер: дожимаем пустой подачей, пока движок продолжает выдавать.
						 * Без этого полный кадр, чей хвост остался при движке, был бы сочтён усечённым
						 */
						if(status != 0){
							// Выполняем создание пустого буфера входящих данных
							ZSTD_inBuffer input = {reinterpret_cast <const char *> (buffer), 0, 0};
							/**
							 * Дожимаем накопленное движком
							 */
							do {
								// Сбрасываем позицию буфера
								output.pos = 0;
								// Выполняем декомпрессию накопленных данных
								status = ::ZSTD_decompressStream(ctx, &output, &input);
								// Если мы получили ошибку декомпрессии
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
								// Если распакованные данные превысили допустимый предел
								if(driver::overflowed(result, "Zstandard", log)){
									// Выполняем очистку результата
									result.clear();
									// Выходим из функции
									return;
								}
							/**
							 * Пока кадр не завершён и движок продолжает выдавать накопленное
							 */
							} while((status != 0) && (output.pos > 0));
						}
						/**
						 * Ненулевой ответ движка означает, что кадр не завершён: движок ждёт
						 * продолжения, которого больше нет — вход усечён либо испорчен
						 */
						if(status != 0){
							// Выполняем очистку результата
							result.clear();
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								log->debug("Zstandard: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Truncated or corrupted data");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог в лог
								log->print("Zstandard: %s", log_t::flag_t::WARNING, "Truncated or corrupted data");
							#endif
						// Если кадр разобран целиком
						} else
							// Кадр разобран целиком - контекст годен под следующий, страж отзывается
							guard.dismiss();
						// Выполняем снятие контекста, коли тот разросся сверх предела
						driver::zstdContexts().trimDecompress(driver::ZSTD_CONTEXT_LIMIT);
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
	 *
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
	 *
	 */
	static void gzip(const void * buffer, const size_t size, const int32_t level, const int16_t wbits, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
						if(::deflateInit2(&zs, level, Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK){
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
							// Смещение в результирующем буфере
							size_t offset = 0;
							// Результат выполнения компрессии
							int32_t ret = Z_OK;
							/**
							 * Выходное окно движка ограничено его разрядностью, а оценка размера
							 * её превысить может: сжатие ведётся окнами, пока кадр не будет дожат
							 */
							do {
								// Устанавливаем размер выходного окна
								const uInt window = static_cast <uInt> (::min <uint64_t> (static_cast <uint64_t> (maxSize - offset), static_cast <uint64_t> (UINT32_MAX)));
								// Устанавливаем максимальный размер буфера
								zs.avail_out = window;
								// Устанавливаем буфер для получения результата
								zs.next_out = reinterpret_cast <Bytef *> (&result[0] + offset);
								// Выполняем сжатие данных
								ret = ::deflate(&zs, Z_FINISH);
								// Увеличиваем смещение на собранный движком объём
								offset += static_cast <size_t> (window - zs.avail_out);
							/**
							 * Пока кадр не дожат и выходное окно заполняется целиком
							 */
							} while((ret == Z_OK) && (zs.avail_out == 0) && (offset < maxSize));
							// Если мы успешно завершили сжатие
							if(ret == Z_STREAM_END)
								// Корректируем размер результирующего буфера
								result.resize(offset);
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
								/**
								 * Исчерпание входа без конца потока движок объявляет отсутствием
								 * продвижения на следующем заходе, и звать это порчей нельзя:
								 * кадр недополучен, а не испорчен. Выходим из цикла, чтобы имя
								 * случаю дала проверка конца потока, стоящая ниже
								 */
								if((ret == Z_BUF_ERROR) && (zs.avail_in == 0))
									// Выходим из цикла
									break;
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
								if(produced > 0){
									// Формируем результирующий буфер бинарных данных
									result.insert(result.end(), &output[0], &output[0] + produced);
									// Если распакованные данные превысили допустимый предел
									if(driver::overflowed(result, "GZip", log)){
										// Выполняем очистку результата
										result.clear();
										// Выходим из функции
										return;
									}
								}
							/**
							 * Если данные ещё не извлечены
							 */
							} while(ret == Z_OK);
							/**
							 * Формат RFC 1952 несёт признак конца потока и контрольную сумму,
							 * поэтому исчерпание входа без конца потока означает усечённый кадр
							 */
							if(ret != Z_STREAM_END){
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("GZip: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Truncated or corrupted data");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("GZip: %s", log_t::flag_t::WARNING, "Truncated or corrupted data");
								#endif
							}
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
	 *
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
	 *
	 */
	static void zlib(const void * buffer, const size_t size, const int32_t level, const int16_t wbits, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
						if(::deflateInit2(&zs, level, Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK){
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
							// Смещение в результирующем буфере
							size_t offset = 0;
							// Результат выполнения компрессии
							int32_t ret = Z_OK;
							/**
							 * Выходное окно движка ограничено его разрядностью, а оценка размера
							 * её превысить может: сжатие ведётся окнами, пока кадр не будет дожат
							 */
							do {
								// Устанавливаем размер выходного окна
								const uInt window = static_cast <uInt> (::min <uint64_t> (static_cast <uint64_t> (maxSize - offset), static_cast <uint64_t> (UINT32_MAX)));
								// Указываем размер выходного буфера
								zs.avail_out = window;
								// Заполняем буфер выходными данными
								zs.next_out = reinterpret_cast <Bytef *> (&result[0] + offset);
								// Выполняем компрессию данных
								ret = ::deflate(&zs, Z_FINISH);
								// Увеличиваем смещение на собранный движком объём
								offset += static_cast <size_t> (window - zs.avail_out);
							/**
							 * Пока кадр не дожат и выходное окно заполняется целиком
							 */
							} while((ret == Z_OK) && (zs.avail_out == 0) && (offset < maxSize));
							// Если компрессия данных выполнена
							if(ret == Z_STREAM_END)
								// Устанавливаем реальный размер результирующего буфера
								result.resize(offset);
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
								/**
								 * Исчерпание входа без конца потока движок объявляет отсутствием
								 * продвижения на следующем заходе, и звать это порчей нельзя:
								 * кадр недополучен, а не испорчен. Выходим из цикла, чтобы имя
								 * случаю дала проверка конца потока, стоящая ниже
								 */
								if((ret == Z_BUF_ERROR) && (zs.avail_in == 0))
									// Выходим из цикла
									break;
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
									// Выходим из функции
									return;
								}
								// Вычисляем количество декомпрессированных данных
								const size_t produced = (data.size() - static_cast <size_t> (zs.avail_out));
								// Если декомпрессировано хоть что-то
								if(produced > 0){
									// Получаем буфер данных
									const char * chunk = reinterpret_cast <const char *> (&data[0]);
									// Добавляем декомпрессированные данные в результат
									result.insert(result.end(), chunk, chunk + produced);
									// Если распакованные данные превысили допустимый предел
									if(driver::overflowed(result, "Zlib", log)){
										// Выполняем очистку результата
										result.clear();
										// Выходим из функции
										return;
									}
								}
							/**
							 * Пока нет конца потока
							 */
							} while(ret == Z_OK);
							/**
							 * Формат RFC 1950 несёт признак конца потока и контрольную сумму,
							 * поэтому исчерпание входа без конца потока означает усечённый кадр
							 */
							if(ret != Z_STREAM_END){
								// Выполняем очистку результата
								result.clear();
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									log->debug("Zlib: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, level, wbits, static_cast <uint16_t> (event)), log_t::flag_t::WARNING, "Truncated or corrupted data");
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог в лог
									log->print("Zlib: %s", log_t::flag_t::WARNING, "Truncated or corrupted data");
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
	 *
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
	 *
	 */
	static void deflate(const void * buffer, const size_t size, const int32_t level, const int16_t wbits, const bool streaming, z_stream & stream, const compressor::event_t event, T & result, const log_t * log) noexcept {
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
				/**
				 * Создаём выходной буфер. Верхний предел заведён затем, чтобы крупное
				 * сообщение не оплачивало заполнение нулями буфера в два своих размера:
				 * оба цикла ниже наращивают буфер сами, как только его перестаёт хватать
				 */
				vector <Bytef> output(::min <size_t> (::max <size_t> (0xFF, size * 2), (AWH_COMPRESSOR_CHUNK_BUFFER_SIZE * 4)));
				// Результат проверки декомпрессии
				int32_t ret = Z_OK;
				// Признак доведённого до конца сообщения
				bool completed = false;
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
							ret = ::deflateInit2(&local, level, Z_DEFLATED, static_cast <int32_t> (-1 * wbits), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
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
						auto guard = driver::makeScopeExit([&local, &stream, streaming, &completed]() noexcept {
							// Если не используется streaming
							if(!streaming)
								// Выполняем завершение работы с локальным потоком
								::deflateEnd(&local);
							/**
							 * Переиспользуемый контекст остаётся жить дальше, поэтому недоведённое
							 * до конца сообщение обязано быть с него снято: иначе следующее сжатие
							 * продолжило бы поток с середины брошенного
							 */
							else if(!completed)
								// Выполняем сброс переиспользуемого потока к исходному состоянию
								::deflateReset(&stream);
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
							if(zs->avail_out == 0){
								/**
								 * Удвоение считается с запасом по разрядности и режется по размеру
								 * памяти сборки: обрезанная величина дала бы буфер меньше прежнего,
								 * и работа пошла бы по кругу на сжимающемся окне записи
								 */
								// Вычисляем новую длину рабочего буфера, не выходя за разрядность памяти
								const uint64_t enlarged = ::min <uint64_t> (static_cast <uint64_t> (output.size()) * 2, static_cast <uint64_t> (SIZE_MAX));
								// Если места под запись не прибавилось, работу продолжать нельзя
								if(enlarged <= static_cast <uint64_t> (output.size())){
									// Выполняем очистку блока с результатом
									result.clear();
									// Записываем ошибку в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Working buffer cannot be enlarged");
									// Выходим из функции
									return;
								}
								// Увеличиваем размер выходного буфера
								output.resize(static_cast <size_t> (enlarged));
							}
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
							if(zs->avail_out == 0){
								/**
								 * Удвоение считается с запасом по разрядности и режется по размеру
								 * памяти сборки: обрезанная величина дала бы буфер меньше прежнего,
								 * и работа пошла бы по кругу на сжимающемся окне записи
								 */
								// Вычисляем новую длину рабочего буфера, не выходя за разрядность памяти
								const uint64_t enlarged = ::min <uint64_t> (static_cast <uint64_t> (output.size()) * 2, static_cast <uint64_t> (SIZE_MAX));
								// Если места под запись не прибавилось, работу продолжать нельзя
								if(enlarged <= static_cast <uint64_t> (output.size())){
									// Выполняем очистку блока с результатом
									result.clear();
									// Записываем ошибку в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Working buffer cannot be enlarged");
									// Выходим из функции
									return;
								}
								// Увеличиваем размер выходного буфера
								output.resize(static_cast <size_t> (enlarged));
							}
						/**
						 * Пока выходной буфер полностью заполнен
						 */
						} while(zs->avail_out == 0);
						// Отмечаем сообщение доведённым до конца
						completed = true;
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
						auto guard = driver::makeScopeExit([&local, &stream, streaming, &completed]() noexcept {
							// Если не используется streaming
							if(!streaming)
								// Выполняем завершение работы с локальным потоком
								::inflateEnd(&local);
							/**
							 * Переиспользуемый контекст остаётся жить дальше, поэтому недоведённое
							 * до конца сообщение обязано быть с него снято: иначе следующая распаковка
							 * продолжила бы поток с середины брошенного
							 */
							else if(!completed)
								// Выполняем сброс переиспользуемого потока к исходному состоянию
								::inflateReset(&stream);
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
							/**
							 * Заполнение выходного буфера ровно по краю уводит работу на ещё один
							 * заход: вход к тому времени съеден, и движок отвечает Z_BUF_ERROR -
							 * продвижения нет, потому что и делать больше нечего. Отказом это не
							 * является, и у потокового кодека тот же код уже допущен. Кадр raw
							 * DEFLATE конца потока не несёт вовсе (RFC 7692), так что выход по
							 * исчерпании работы здесь законен.
							 * Допускается один лишь этот случай: тот же код с неразобранным
							 * входом означает, что движок дальше не идёт, а данные ещё есть, -
							 * сообщение недоведено, и объявлять его законченным нельзя, иначе
							 * переиспользуемый контекст остался бы с брошенным потоком внутри
							 */
							// Если возникает ошибка
							if((ret != Z_OK) && (ret != Z_STREAM_END) && !((ret == Z_BUF_ERROR) && (zs->avail_in == 0))){
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
							if(produced > 0){
								// Добавляем декомпрессированные данные в результат
								result.insert(result.end(), &output[0], &output[0] + produced);
								// Если распакованные данные превысили допустимый предел
								if(driver::overflowed(result, "Deflate", log)){
									// Выполняем очистку результата
									result.clear();
									// Выходим из функции
									return;
								}
							}
							// Если выходной буфер заполнен, увеличиваем его размер
							if(zs->avail_out == 0){
								/**
								 * Удвоение считается с запасом по разрядности и режется по размеру
								 * памяти сборки: обрезанная величина дала бы буфер меньше прежнего,
								 * и работа пошла бы по кругу на сжимающемся окне записи
								 */
								// Вычисляем новую длину рабочего буфера, не выходя за разрядность памяти
								const uint64_t enlarged = ::min <uint64_t> (static_cast <uint64_t> (output.size()) * 2, static_cast <uint64_t> (SIZE_MAX));
								// Если места под запись не прибавилось, работу продолжать нельзя
								if(enlarged <= static_cast <uint64_t> (output.size())){
									// Выполняем очистку блока с результатом
									result.clear();
									// Записываем ошибку в лог
									log->print("Deflate: %s", log_t::flag_t::WARNING, "Working buffer cannot be enlarged");
									// Выходим из функции
									return;
								}
								// Увеличиваем размер выходного буфера
								output.resize(static_cast <size_t> (enlarged));
							}
						/**
						 * Пока нет конца потока и есть входные данные
						 */
						/**
						 * Пока есть входные данные либо выходной буфер полон: движок мог
						 * упереться в место как раз на исчерпании входа, и накопленное осталось бы при нём
						 */
						} while((ret == Z_OK) && ((zs->avail_in > 0) || (zs->avail_out == 0)));
						// Отмечаем сообщение доведённым до конца
						completed = true;
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
awh::compressor::Block::BufferDeflate::BufferDeflate() noexcept :
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
awh::compressor::Block::Window::Window() noexcept : wbits(0) {}

/**
 * @brief Конструктор
 *
 */
awh::compressor::Block::Deflate::Deflate() noexcept : wbits(0) {}

/**
 * @brief Метод установки уровня компрессии
 *
 * @param level уровень компрессии
 *
 */
void awh::compressor::Block::level(const level_t level) noexcept {
	/**
	 * Определяем переданный уровень компрессии
	 */
	switch(static_cast <uint8_t> (level)){
		// Выполняем установку максимального уровня компрессии
		case static_cast <uint8_t> (level_t::BEST): {
			// Выполняем установку максимальной степени сжатия Lz4
			this->_level[0] = LZ4HC_CLEVEL_MAX;
			// Выполняем установку уровня компрессии семейства Zlib (GZip, Zlib, Deflate)
			this->_level[1] = Z_BEST_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = static_cast <uint32_t> (::ZSTD_maxCLevel());
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_MAX_CLEVEL;
			// Выполняем установку уровня компрессии Lion (Density) — самый сильный алгоритм
			this->_level[4] = DENSITY_ALGORITHM_LION;
			// Выполняем установку качества компрессии Brotli
			this->_level[5] = BROTLI_MAX_QUALITY;
			// Выполняем установку пресета компрессии LZMA
			this->_level[6] = 9;
			// Выполняем установку размера рабочего блока BZip2
			this->_level[7] = 9;
		} break;
		// Выполняем установку уровня компрессии на максимальную производительность
		case static_cast <uint8_t> (level_t::SPEED): {
			// Выполняем установку ускорения Lz4 (отрицательный уровень задаёт ускорение)
			this->_level[0] = -2;
			// Выполняем установку уровня компрессии семейства Zlib (GZip, Zlib, Deflate)
			this->_level[1] = Z_BEST_SPEED;
			// Выполняем установку уровня минимальной компрессии Zstandard
			this->_level[2] = 1;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_MIN_CLEVEL;
			// Выполняем установку уровня компрессии Chameleon (Density) — самый быстрый алгоритм
			this->_level[4] = DENSITY_ALGORITHM_CHAMELEON;
			// Выполняем установку качества компрессии Brotli
			this->_level[5] = BROTLI_MIN_QUALITY;
			// Выполняем установку пресета компрессии LZMA
			this->_level[6] = 1;
			// Выполняем установку размера рабочего блока BZip2
			this->_level[7] = 1;
		} break;
		// Выполняем установку нормального уровня компрессии
		case static_cast <uint8_t> (level_t::NORMAL): {
			// Выполняем установку уровня компрессии Lz4 по умолчанию
			this->_level[0] = 0;
			// Выполняем установку уровня компрессии семейства Zlib (GZip, Zlib, Deflate)
			this->_level[1] = Z_DEFAULT_COMPRESSION;
			// Выполняем установку уровня компрессии Zstandard по умолчанию
			this->_level[2] = ZSTD_CLEVEL_DEFAULT;
			// Выполняем установку уровня компрессии Lizard
			this->_level[3] = LIZARD_DEFAULT_CLEVEL;
			// Выполняем установку уровня компрессии Cheetah (Density) — сбалансированный алгоритм
			this->_level[4] = DENSITY_ALGORITHM_CHEETAH;
			// Выполняем установку качества компрессии Brotli
			this->_level[5] = 5;
			// Выполняем установку пресета компрессии LZMA
			this->_level[6] = LZMA_PRESET_DEFAULT;
			// Выполняем установку размера рабочего блока BZip2
			this->_level[7] = 5;
		} break;
	}
	/**
	 * Переиспользуемый контекст компрессии держит уровень внутри себя: он был задан
	 * при заведении контекста, и смена таблицы сама по себе его не меняет. Движок
	 * умеет подменять параметры на живом потоке — новый уровень вступит в силу со
	 * следующего сообщения, а накопленное к этому времени выдавливается движком
	 */
	if(this->_deflate.takeover.compress.load(std::memory_order_acquire)){
		// Выполняем подмену параметров живого контекста компрессии
		const int32_t ret = ::deflateParams(&this->_deflate.buffer.compress->stream, this->_level[1].load(std::memory_order_acquire), Z_DEFAULT_STRATEGY);
		// Если подменить параметры не удалось
		if(ret != Z_OK){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Deflate stream parameters are not changed", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (level)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Deflate stream parameters are not changed", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод установки размера скользящего окна Zlib
 *
 * @param wbits размер скользящего окна
 *
 */
void awh::compressor::Block::wbitsZlib(const int16_t wbits) noexcept {
	/**
	 * Нижняя граница — девять по тому же доводу, что и у GZip: сжатие с окном в
	 * восемь разрядов движок заводит, но поднимает окно до девяти молча, а разбор
	 * тем же значением получившийся поток уже не берёт
	 */
	// Если размер скользящего окна лежит вне допустимого промежутка
	if((wbits < 9) || (wbits > MAX_WBITS)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Zlib window bits are out of range", __PRETTY_FUNCTION__, make_tuple(wbits), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Zlib window bits are out of range", log_t::flag_t::WARNING);
		#endif
		// Выходим из функции, оставляя прежнее значение
		return;
	}
	// Устанавливаем размер скользящего окна Zlib
	this->_zlib.wbits = wbits;
}
/**
 * @brief Метод установки размера скользящего окна GZip
 *
 * @param wbits размер скользящего окна
 *
 */
void awh::compressor::Block::wbitsGZip(const int16_t wbits) noexcept {
	/**
	 * Нижняя граница — девять, а не восемь: окно в восемь разрядов движок не заводит
	 * ни «сырым» потоком, ни потоком с заголовком gzip, а у формата zlib заводит
	 * сжатие, но поднимает окно до девяти молча - и разбор тем же значением
	 * получившийся поток уже не берёт. Восьмёрка, словом, не работает нигде
	 */
	// Если размер скользящего окна лежит вне допустимого промежутка
	if((wbits < 9) || (wbits > MAX_WBITS)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("GZip window bits are out of range", __PRETTY_FUNCTION__, make_tuple(wbits), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("GZip window bits are out of range", log_t::flag_t::WARNING);
		#endif
		// Выходим из функции, оставляя прежнее значение
		return;
	}
	// Устанавливаем размер скользящего окна GZip
	this->_gzip.wbits = wbits;
}
/**
 * @brief Метод установки размера скользящего окна Deflate
 *
 * @param wbits размер скользящего окна
 * @return      результат установки размера
 *
 */
bool awh::compressor::Block::wbitsDeflate(const int16_t wbits) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Значение вне допустимого промежутка отвергается до записи: иначе отказ
	 * обнаруживался бы лишь при работе, да и то не всегда — при выключенном
	 * переиспользовании контекста установщик отвечал бы успехом.
	 *
	 * Нижняя граница — девять, общая у всего семейства: «сырой» поток с окном в
	 * восемь разрядов движок не заводит вовсе, отвечая отказом доводов. RFC 7692
	 * восьмёрку в согласовании допускает, но выполнить её нечем, и принять её
	 * значило бы оставить объект с настройкой, при которой Deflate не работает
	 */
	if((wbits < 9) || (wbits > MAX_WBITS)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Deflate window bits are out of range", __PRETTY_FUNCTION__, make_tuple(wbits), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Deflate window bits are out of range", log_t::flag_t::WARNING);
		#endif
		// Выходим из функции, оставляя прежнее значение
		return result;
	}
	// Запоминаем прежний размер скользящего окна
	const int16_t previous = this->_deflate.wbits.load(std::memory_order_acquire);
	// Устанавливаем размер скользящего окна Deflate
	this->_deflate.wbits = wbits;
	// Выполняем пересборку контекстов LZ77 для компрессии
	result = this->takeoverDeflate(event_t::ENCODE, this->_deflate.takeover.compress.load(std::memory_order_acquire));
	// Выполняем пересборку контекстов LZ77 для декомпрессии
	result = (this->takeoverDeflate(event_t::DECODE, this->_deflate.takeover.decompress.load(std::memory_order_acquire)) && result);
	/**
	 * Пересборка идёт двумя половинами, и удавшаяся половина при отказе второй
	 * оставила бы объект наполовину переиспользующим контекст: снимаем обе разом,
	 * чтобы обе стороны работали на локальных контекстах одинаково
	 */
	if(!result){
		/**
		 * Прежнее значение возвращается на место: не вернув его, работа оставила бы
		 * объект с размером окна, на котором пересборка только что и отказала, -
		 * и всякое следующее сжатие Deflate отказывало бы уже на своём контексте
		 */
		// Возвращаем прежний размер скользящего окна
		this->_deflate.wbits = previous;
		// Снимаем флаг переиспользования контекста компрессии
		this->takeoverDeflate(event_t::ENCODE, false);
		// Снимаем флаг переиспользования контекста декомпрессии
		this->takeoverDeflate(event_t::DECODE, false);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод включения/отключения переиспользования контекста Deflate
 *
 * @param event событие выполнения операции
 * @param flag  флаг переиспользования контекста компрессии/декомпрессии
 * @return      результат установки флага
 *
 */
bool awh::compressor::Block::takeoverDeflate(const event_t event, const bool flag) noexcept {
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
				// Извлекаем переиспользуемый контекст Deflate
				z_stream & buffer = this->_deflate.buffer.compress->stream;
				// Если уже выделена память для компрессора
				if(this->_deflate.takeover.compress.load(std::memory_order_acquire))
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
					if(!(result = (::deflateInit2(&buffer, this->_level[1], Z_DEFLATED, static_cast <int32_t> (-1 * this->_deflate.wbits), MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK))){
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
						// Сбрасываем флаг переиспользования контекста, так как рабочего потока больше нет
						this->_deflate.takeover.compress.store(false, std::memory_order_release);
						// Выходим из функции
						return result;
					}
				}
				// Устанавливаем переданный флаг
				this->_deflate.takeover.compress.store(flag, std::memory_order_release);
			} break;
			// Выполняем установку флага переиспользования контекста декомпрессии
			case static_cast <uint8_t> (event_t::DECODE): {
				// Извлекаем переиспользуемый контекст Deflate
				z_stream & buffer = this->_deflate.buffer.decompress->stream;
				// Если уже выделена память для декомпрессора
				if(this->_deflate.takeover.decompress.load(std::memory_order_acquire))
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
					if(!(result = (::inflateInit2(&buffer, static_cast <int32_t> (-1 * this->_deflate.wbits)) == Z_OK))){
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
						// Сбрасываем флаг переиспользования контекста, так как рабочего потока больше нет
						this->_deflate.takeover.decompress.store(false, std::memory_order_release);
						// Выходим из функции
						return result;
					}
				}
				// Устанавливаем переданный флаг
				this->_deflate.takeover.decompress.store(flag, std::memory_order_release);
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
 *
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
 *
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
		// Для GZip (RFC 1952)
		case static_cast <uint8_t> (method_t::GZIP): {
			// Устанавливаем размер окна
			params.wbits = this->_gzip.wbits.load(std::memory_order_acquire);
			// Устанавливаем уровень компрессии
			params.level = this->_level[1].load(std::memory_order_acquire);
		} break;
		// Для Deflate (RFC 1951)
		case static_cast <uint8_t> (method_t::DEFLATE): {
			// Устанавливаем размер окна
			params.wbits = this->_deflate.wbits.load(std::memory_order_acquire);
			// Устанавливаем уровень компрессии
			params.level = this->_level[1].load(std::memory_order_acquire);
		} break;
		// Для Zlib (RFC 1950)
		case static_cast <uint8_t> (method_t::ZLIB): {
			// Устанавливаем размер окна
			params.wbits = this->_zlib.wbits.load(std::memory_order_acquire);
			// Устанавливаем уровень компрессии
			params.level = this->_level[1].load(std::memory_order_acquire);
		} break;
		// Для Zstandard
		case static_cast <uint8_t> (method_t::ZSTD):
			// Устанавливаем уровень компрессии
			params.level = this->_level[2].load(std::memory_order_acquire);
		break;
		// Для LZ4
		case static_cast <uint8_t> (method_t::LZ4):
			// Устанавливаем уровень компрессии
			params.level = this->_level[0].load(std::memory_order_acquire);
		break;
		// Для Lizard
		case static_cast <uint8_t> (method_t::LIZARD):
			// Устанавливаем уровень компрессии
			params.level = this->_level[3].load(std::memory_order_acquire);
		break;
		// Для LZMA
		case static_cast <uint8_t> (method_t::LZMA):
			// Устанавливаем пресет компрессии
			params.level = this->_level[6].load(std::memory_order_acquire);
		break;
		// Для BZip2
		case static_cast <uint8_t> (method_t::BZIP2):
			// Устанавливаем размер рабочего блока в единицах по 100 килобайт
			params.level = this->_level[7].load(std::memory_order_acquire);
		break;
		// Для Brotli
		case static_cast <uint8_t> (method_t::BROTLI):
			// Устанавливаем качество компрессии
			params.level = this->_level[5].load(std::memory_order_acquire);
		break;
	}
	// Создаём и возвращаем потоковую сессию
	return stream_t(method, event, params, this->_log);
}
/**
 * @brief Шаблон метода компрессии данных
 *
 * @tparam T тип возвращаемого результата
 *
 */
template <typename T>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 *
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
 *
 */
template <typename A, typename B>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 *
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
 *
 */
template <typename T>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 *
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
 *
 */
template <typename T>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @param result контейнер куда следует положить результат
 *
 */
void awh::compressor::Block::compress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept {
	/**
	 * Очищаем результат до всякой проверки: наружу не должно уйти прежнее содержимое
	 * контейнера ни на пустом входе, ни на незаданном методе
	 */
	result.clear();
	/**
	 * Подача без буфера при ненулевом размере — ошибка вызывающей стороны, а не
	 * пустой вход: работа получила указание разобрать данные, которых ей не дали.
	 * Договор здесь общий с потоковым режимом, где та же подача отвергается с записью
	 */
	if((buffer == nullptr) && (size > 0)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Compressor: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Buffer is not passed");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Compressor: %s", log_t::flag_t::WARNING, "Buffer is not passed");
		#endif
		// Выходим из функции
		return;
	}
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Если размер входного буфера выбранному движку не по разрядности
		if(!compressor::fits(size, method)){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Compressor: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Input buffer is too large for the selected method");
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Compressor: %s", log_t::flag_t::WARNING, "Input buffer is too large for the selected method");
			#endif
			// Выходим из функции
			return;
		}
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
				driver::lzma(buffer, size, this->_level[6], event_t::ENCODE, result, this->_log);
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
				driver::bzip2(buffer, size, this->_level[7], event_t::ENCODE, result, this->_log);
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
				driver::brotli(buffer, size, this->_level[5], event_t::ENCODE, result, this->_log);
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
				// Выполняем компрессию данных методом Deflate
				driver::deflate(buffer, size, this->_level[1], this->_deflate.wbits, this->_deflate.takeover.compress.load(std::memory_order_acquire), this->_deflate.buffer.compress->stream, event_t::ENCODE, result, this->_log);
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
			/**
			 * Значение вне перечисления методов получить можно только приведением,
			 * и сквозным проходом оно быть не должно: результат остаётся пустым
			 */
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Compressor: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Unknown compression method");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Compressor: %s", log_t::flag_t::WARNING, "Unknown compression method");
				#endif
			}
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
 *
 */
template <typename T>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 *
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
 *
 */
template <typename A, typename B>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 *
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
 *
 */
template <typename T>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 *
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
 *
 */
template <typename T>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @param result контейнер куда следует положить результат
 *
 */
void awh::compressor::Block::decompress(const void * buffer, const size_t size, const method_t method, T & result) const noexcept {
	/**
	 * Очищаем результат до всякой проверки: наружу не должно уйти прежнее содержимое
	 * контейнера ни на пустом входе, ни на незаданном методе
	 */
	result.clear();
	/**
	 * Подача без буфера при ненулевом размере — ошибка вызывающей стороны, а не
	 * пустой вход: работа получила указание разобрать данные, которых ей не дали.
	 * Договор здесь общий с потоковым режимом, где та же подача отвергается с записью
	 */
	if((buffer == nullptr) && (size > 0)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Compressor: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Buffer is not passed");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Compressor: %s", log_t::flag_t::WARNING, "Buffer is not passed");
		#endif
		// Выходим из функции
		return;
	}
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Если размер входного буфера выбранному движку не по разрядности
		if(!compressor::fits(size, method)){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Compressor: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Input buffer is too large for the selected method");
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Compressor: %s", log_t::flag_t::WARNING, "Input buffer is too large for the selected method");
			#endif
			// Выходим из функции
			return;
		}
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
				driver::lzma(buffer, size, this->_level[6], event_t::DECODE, result, this->_log);
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
				driver::bzip2(buffer, size, this->_level[7], event_t::DECODE, result, this->_log);
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
				driver::brotli(buffer, size, this->_level[5], event_t::DECODE, result, this->_log);
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
				// Выполняем декомпрессию данных методом Deflate
				driver::deflate(buffer, size, this->_level[1], this->_deflate.wbits, this->_deflate.takeover.decompress.load(std::memory_order_acquire), this->_deflate.buffer.decompress->stream, event_t::DECODE, result, this->_log);
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
			/**
			 * Значение вне перечисления методов получить можно только приведением,
			 * и сквозным проходом оно быть не должно: результат остаётся пустым
			 */
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Compressor: %s", __PRETTY_FUNCTION__, make_tuple(buffer, size, static_cast <uint16_t> (method)), log_t::flag_t::WARNING, "Unknown compression method");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Compressor: %s", log_t::flag_t::WARNING, "Unknown compression method");
				#endif
			}
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
 *
 */
awh::compressor::Block::Block(const log_t * log) noexcept :
 _level{
	0,
	Z_DEFAULT_COMPRESSION,
	ZSTD_CLEVEL_DEFAULT,
	LIZARD_DEFAULT_CLEVEL,
	DENSITY_ALGORITHM_CHEETAH,
	5,
	LZMA_PRESET_DEFAULT,
	5
}, _log(log) {
	// Выделяем память под переиспользуемый контекст компрессии Deflate
	this->_deflate.buffer.compress = new deflate_stream_t();
	// Выделяем память под переиспользуемый контекст декомпрессии Deflate
	this->_deflate.buffer.decompress = new deflate_stream_t();
	// Устанавливаем размер скользящего окна GZip по умолчанию
	this->_gzip.wbits = static_cast <int16_t> (MAX_WBITS);
	// Устанавливаем размер скользящего окна Deflate по умолчанию
	this->_deflate.wbits = static_cast <int16_t> (MAX_WBITS);
	// Устанавливаем размер скользящего окна Zlib по умолчанию
	this->_zlib.wbits = static_cast <int16_t> (MAX_WBITS);
}
/**
 * @brief Деструктор
 *
 */
awh::compressor::Block::~Block() noexcept {
	// Если выделена память для компрессора
	if(this->_deflate.takeover.compress.load(std::memory_order_acquire))
		// Завершаем работу компрессора Deflate
		::deflateEnd(&this->_deflate.buffer.compress->stream);
	// Если выделена память для декомпрессора
	if(this->_deflate.takeover.decompress.load(std::memory_order_acquire))
		// Завершаем работу декомпрессора Deflate
		::inflateEnd(&this->_deflate.buffer.decompress->stream);
	// Освобождаем память переиспользуемого контекста компрессии Deflate
	delete this->_deflate.buffer.compress;
	// Освобождаем память переиспользуемого контекста декомпрессии Deflate
	delete this->_deflate.buffer.decompress;
}
