/**
 * @file: stream.cpp
 * @date: 2026-07-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация потоковой компрессии — инкрементальное сжатие и распаковка данных по мере поступления,
 *        управление временем жизни контекста движка, режимами сброса буфера и финализацией потока
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Если не определён размер рабочего буфера порции данных, устанавливаем его
 */
#ifndef AWH_COMPRESSOR_STREAM_CHUNK
	/**
	 * Устанавливаем размер рабочего буфера порции данных
	 */
	#define AWH_COMPRESSOR_STREAM_CHUNK 0x4000
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Заголовочный файл для работы с GZip/Zlib/Deflate
 */
#include <zlib.h>

/**
 * Заголовочный файл для работы с Zstandard
 */
#include <zstd.h>

/**
 * Заголовочные файлы для работы с Brotli
 */
#include <brotli/decode.h>
#include <brotli/encode.h>

/**
 * Заголовочный файл для работы с LZMA
 */
#include <lzma.h>

/**
 * Заголовочный файл для работы с BZip2
 */
#include <bzlib.h>

/**
 * Заголовочный файл для работы с LZ4 (потоковый Frame-формат)
 */
#include <lz4frame.h>

/**
 * Заголовочный файл для работы с Lizard (потоковый Frame-формат)
 */
#include "lizard_frame.h"

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочный файл проекта
 */
#include <compressor/stream.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён AWH
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён компрессора
	 *
	 */
	namespace compressor {
		/**
		 * @brief Абстрактный бэкенд потокового движка
		 *
		 */
		class coder_t {
			public:
				/**
				 * @brief Метод проверки успешной инициализации контекста
				 *
				 * @return результат проверки
				 *
				 */
				virtual bool valid() const noexcept = 0;
				/**
				 * @brief Метод проверки завершённости потока
				 *
				 * @return результат проверки
				 *
				 */
				virtual bool done() const noexcept = 0;
			public:
				/**
				 * @brief Метод обработки порции данных
				 *
				 * @param buffer буфер входящих данных
				 * @param size   размер входящих данных
				 * @param flush  режим сброса
				 * @param out    буфер, куда добавляется готовый выход
				 * @return       результат операции (false при ошибке)
				 *
				 */
				virtual bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept = 0;
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~coder_t() = default;
		};
	};
};

/**
 * @brief Анонимное пространство имён для внутренних бэкендов
 *
 */
namespace {
	/**
	 * Подписываемся на пространства имён AWH
	 */
	using namespace awh;

	/**
	 * Подписываемся на пространства имён AWH::compressor
	 */
	using namespace awh::compressor;

	/**
	 * @brief Класс бэкенда Zlib (deflate/gzip/zlib)
	 *
	 */
	class zlib_coder_t : public coder_t {
		private:
			// Флаг успешной инициализации
			bool _init;
			// Флаг завершённости потока
			bool _done;
			// Флаг направления (компрессия)
			event_t _event;
		private:
			// Поток Zlib
			z_stream _zs;
		public:
			/**
			 * @brief Метод проверки успешной инициализации контекста
			 *
			 * @return результат проверки
			 *
			 */
			bool valid() const noexcept override {
				// Выводим результат проверки
				return this->_init;
			}
			/**
			 * @brief Метод проверки завершённости потока
			 *
			 * @return результат проверки
			 *
			 */
			bool done() const noexcept override {
				// Выводим результат проверки
				return this->_done;
			}
			/**
			 * @brief Метод обработки порции данных
			 *
			 * @param buffer буфер входящих данных
			 * @param size   размер входящих данных
			 * @param flush  режим сброса
			 * @param out    буфер, куда добавляется готовый выход
			 * @return       результат операции
			 *
			 */
			bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept override {
				// Если контекст не инициализирован
				if(!this->_init)
					// Выводим отрицательный результат
					return false;
				// Если поток уже завершён
				if(this->_done)
					// Выводим положительный результат (ничего не делаем)
					return true;
				// Результат операции
				int32_t ret = Z_OK;
				// Рабочий буфер
				Bytef work[AWH_COMPRESSOR_STREAM_CHUNK];
				// Устанавливаем входной размер данных
				this->_zs.avail_in = static_cast <uInt> (size);
				// Устанавливаем входной буфер
				this->_zs.next_in = reinterpret_cast <Bytef *> (const_cast <void *> (buffer));
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Определяем режим сброса
						const int32_t zflush = (flush == flush_t::FINISH ? Z_FINISH : (flush == flush_t::SYNC ? Z_SYNC_FLUSH : Z_NO_FLUSH));
						/**
						 * Выполняем сжатие пока не опустошим выходной буфер
						 */
						do {
							// Устанавливаем выходной буфер
							this->_zs.next_out = work;
							// Устанавливаем размер выходного буфера
							this->_zs.avail_out = static_cast <uInt> (AWH_COMPRESSOR_STREAM_CHUNK);
							// Выполняем сжатие
							ret = ::deflate(&this->_zs, zflush);
							// Если возникла критическая ошибка
							if(ret == Z_STREAM_ERROR)
								// Выводим отрицательный результат
								return false;
							// Вычисляем количество произведённых данных
							const size_t produced = (AWH_COMPRESSOR_STREAM_CHUNK - this->_zs.avail_out);
							// Добавляем данные в результат
							if(produced > 0)
								// Добавляем данные в результат
								out.insert(out.end(), reinterpret_cast <char *> (work), reinterpret_cast <char *> (work) + produced);
						/**
						 * Пока выходной буфер полностью заполнен либо финализация ещё не дошла до конца потока
						 */
						} while((this->_zs.avail_out == 0) || ((zflush == Z_FINISH) && (ret == Z_OK)));
						// Устанавливаем флаг завершённости потока (если был вызван flush=FINISH и достигнут конец потока)
						this->_done = ((zflush == Z_FINISH) && (ret == Z_STREAM_END));
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						/**
						 * Выполняем распаковку пока есть данные
						 */
						do {
							// Устанавливаем выходной буфер
							this->_zs.next_out = work;
							// Устанавливаем размер выходного буфера
							this->_zs.avail_out = static_cast <uInt> (AWH_COMPRESSOR_STREAM_CHUNK);
							// Выполняем распаковку
							ret = ::inflate(&this->_zs, Z_NO_FLUSH);
							// Если возникла ошибка
							if((ret != Z_OK) && (ret != Z_STREAM_END) && (ret != Z_BUF_ERROR))
								// Выводим отрицательный результат
								return false;
							// Вычисляем количество произведённых данных
							const size_t produced = (AWH_COMPRESSOR_STREAM_CHUNK - this->_zs.avail_out);
							// Добавляем данные в результат
							if(produced > 0)
								// Добавляем данные в результат
								out.insert(out.end(), reinterpret_cast <char *> (work), reinterpret_cast <char *> (work) + produced);
							// Если поток завершён
							if((this->_done = (ret == Z_STREAM_END)))
								// Выходим из цикла
								break;
							// Если больше нет входных данных, выходим
							if((ret == Z_BUF_ERROR) && (this->_zs.avail_in == 0))
								// Выходим из цикла
								break;
						/**
						 * Пока есть входные данные или выходной буфер заполнен
						 */
						} while((this->_zs.avail_in > 0) || (this->_zs.avail_out == 0));
					} break;
				}
				// Выводим результат операции
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param method метод компрессии
			 * @param event  направление операции
			 * @param params параметры инициализации
			 *
			 */
			explicit zlib_coder_t(const method_t method, const event_t event, const params_t & params) noexcept :
			 _init(false), _done(false), _event(event) {
				// Вычисляем размер скользящего окна в зависимости от метода
				int32_t windowBits = 0;
				// Обнуляем весь поток Zlib
				::memset(&this->_zs, 0, sizeof(this->_zs));
				// Инициализируем аллокаторы
				this->_zs.zalloc = Z_NULL;
				this->_zs.zfree  = Z_NULL;
				this->_zs.opaque = Z_NULL;
				/**
				 * Определяем метод компрессии
				 */
				switch(static_cast <uint8_t> (method)){
					// Для GZip добавляем смещение заголовка
					case static_cast <uint8_t> (method_t::GZIP):
						// Устанавливаем размер скользящего окна с учётом смещения заголовка
						windowBits = (static_cast <int32_t> (params.wbits) + 16);
					break;
					// Для Zlib (RFC 1950) — положительное значение
					case static_cast <uint8_t> (method_t::ZLIB):
						// Устанавливаем размер скользящего окна без смещения заголовка
						windowBits = static_cast <int32_t> (params.wbits);
					break;
					// Для Deflate — «сырой» формат (отрицательное значение)
					default:
						// Устанавливаем размер скользящего окна без смещения заголовка
						windowBits = -1 * static_cast <int32_t> (params.wbits);
				}
				// Результат инициализации
				int32_t ret = 0;
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE):
						// Инициализируем контекст Zlib для компрессии
						ret = ::deflateInit2(&this->_zs, params.level, Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
					break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE):
						// Инициализируем контекст Zlib для декомпрессии
						ret = ::inflateInit2(&this->_zs, windowBits);
					break;
				}
				// Запоминаем результат инициализации
				this->_init = (ret == Z_OK);
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~zlib_coder_t() noexcept override {
				// Если контекст был инициализирован
				if(this->_init){
					/**
					 * Определяем направление операции
					 */
					switch(static_cast <uint8_t> (this->_event)){
						// Для компрессии
						case static_cast <uint8_t> (event_t::ENCODE):
							// Завершаем работу контекста
							::deflateEnd(&this->_zs);
						break;
						// Для декомпрессии
						case static_cast <uint8_t> (event_t::DECODE):
							// Завершаем работу контекста
							::inflateEnd(&this->_zs);
						break;
					}
				}
			}
	};
	/**
	 * @brief Класс бэкенда Zstandard
	 *
	 */
	class zstd_coder_t : public coder_t {
		private:
			// Флаг завершённости потока
			bool _done;
		private:
			// Флаг направления (компрессия)
			event_t _event;
		private:
			// Контекст компрессии
			ZSTD_CStream * _cctx;
			// Контекст декомпрессии
			ZSTD_DStream * _dctx;
		public:
			/**
			 * @brief Метод проверки успешной инициализации контекста
			 *
			 * @return результат проверки
			 *
			 */
			bool valid() const noexcept override {
				// Выводим результат проверки
				return (this->_event == event_t::ENCODE ? (this->_cctx != nullptr) : (this->_dctx != nullptr));
			}
			/**
			 * @brief Метод проверки завершённости потока
			 *
			 * @return результат проверки
			 *
			 */
			bool done() const noexcept override {
				// Выводим результат проверки
				return this->_done;
			}
		public:
			/**
			 * @brief Метод обработки порции данных
			 *
			 * @param buffer буфер входящих данных
			 * @param size   размер входящих данных
			 * @param flush  режим сброса
			 * @param out    буфер, куда добавляется готовый выход
			 * @return       результат операции
			 *
			 */
			bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept override {
				// Рабочий буфер
				char work[AWH_COMPRESSOR_STREAM_CHUNK];
				// Формируем входной буфер
				ZSTD_inBuffer input = {buffer, size, 0};
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Если контекст не создан
						if(this->_cctx == nullptr)
							// Выводим отрицательный результат
							return false;
						// Флаг завершения операции
						bool finished = false;
						// Определяем режим сброса
						const ZSTD_EndDirective op = (flush == flush_t::FINISH ? ZSTD_e_end : (flush == flush_t::SYNC ? ZSTD_e_flush : ZSTD_e_continue));
						/**
						 * Выполняем сжатие
						 */
						do {
							// Формируем выходной буфер
							ZSTD_outBuffer output = {work, AWH_COMPRESSOR_STREAM_CHUNK, 0};
							// Выполняем сжатие
							const size_t remaining = ::ZSTD_compressStream2(this->_cctx, &output, &input, op);
							// Если возникла ошибка
							if(::ZSTD_isError(remaining))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(output.pos > 0)
								// Добавляем данные в результат
								out.insert(out.end(), work, work + output.pos);
							// Определяем завершение операции
							finished = (op == ZSTD_e_continue ? (input.pos == input.size) : (remaining == 0));
						/**
						 * Пока операция не завершена
						 */
						} while(!finished);
						// Устанавливаем флаг завершённости потока
						this->_done = (op == ZSTD_e_end);
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Если контекст не создан
						if(this->_dctx == nullptr)
							// Выводим отрицательный результат
							return false;
						/**
						 * Выполняем распаковку пока есть входные данные
						 */
						while(input.pos < input.size){
							// Формируем выходной буфер
							ZSTD_outBuffer output = {work, AWH_COMPRESSOR_STREAM_CHUNK, 0};
							// Выполняем распаковку
							const size_t ret = ::ZSTD_decompressStream(this->_dctx, &output, &input);
							// Если возникла ошибка
							if(::ZSTD_isError(ret))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(output.pos > 0)
								// Добавляем данные в результат
								out.insert(out.end(), work, work + output.pos);
							// Устанавливаем флаг завершённости потока
							this->_done = (ret == 0);
							// Если данных больше нет и ничего не произведено, выходим
							if((output.pos == 0) && (input.pos == input.size))
								// Выходим из цикла
								break;
						}
					} break;
				}
				// Выводим результат операции
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param event  направление операции
			 * @param params параметры инициализации
			 *
			 */
			explicit zstd_coder_t(const event_t event, const params_t & params) noexcept :
			 _done(false), _event(event), _cctx(nullptr), _dctx(nullptr) {
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Создаём контекст компрессии
						this->_cctx = ::ZSTD_createCStream();
						// Инициализируем контекст уровнем компрессии
						if(this->_cctx != nullptr)
							// Инициализируем контекст уровнем компрессии
							::ZSTD_initCStream(this->_cctx, params.level);
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Создаём контекст декомпрессии
						this->_dctx = ::ZSTD_createDStream();
						// Инициализируем контекст
						if(this->_dctx != nullptr)
							// Инициализируем контекст
							::ZSTD_initDStream(this->_dctx);
					} break;
				}
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~zstd_coder_t() noexcept override {
				// Освобождаем контекст компрессии
				if(this->_cctx != nullptr)
					// Освобождаем контекст компрессии
					::ZSTD_freeCStream(this->_cctx);
				// Освобождаем контекст декомпрессии
				if(this->_dctx != nullptr)
					// Освобождаем контекст декомпрессии
					::ZSTD_freeDStream(this->_dctx);
			}
	};
	/**
	 * @brief Класс бэкенда Brotli
	 *
	 */
	class brotli_coder_t : public coder_t {
		private:
			// Флаг завершённости потока
			bool _done;
		private:
			// Флаг направления (компрессия)
			event_t _event;
		private:
			// Контекст энкодера
			BrotliEncoderState * _enc;
			// Контекст декодера
			BrotliDecoderState * _dec;
		public:
			/**
			 * @brief Метод проверки успешной инициализации контекста
			 *
			 * @return результат проверки
			 *
			 */
			bool valid() const noexcept override {
				// Выводим результат проверки
				return (this->_event == event_t::ENCODE ? (this->_enc != nullptr) : (this->_dec != nullptr));
			}
			/**
			 * @brief Метод проверки завершённости потока
			 *
			 * @return результат проверки
			 *
			 */
			bool done() const noexcept override {
				// Выводим результат проверки
				return this->_done;
			}
		public:
			/**
			 * @brief Метод обработки порции данных
			 *
			 * @param buffer буфер входящих данных
			 * @param size   размер входящих данных
			 * @param flush  режим сброса
			 * @param out    буфер, куда добавляется готовый выход
			 * @return       результат операции
			 *
			 */
			bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept override {
				// Размер входных данных
				size_t availIn = size;
				// Рабочий буфер
				uint8_t work[AWH_COMPRESSOR_STREAM_CHUNK];
				// Получаем входящий буфер
				const uint8_t * nextIn = reinterpret_cast <const uint8_t *> (buffer);
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Если контекст не создан
						if(this->_enc == nullptr)
							// Выводим отрицательный результат
							return false;
						// Определяем режим сброса
						const BrotliEncoderOperation op = (flush == flush_t::FINISH ? BROTLI_OPERATION_FINISH : (flush == flush_t::SYNC ? BROTLI_OPERATION_FLUSH : BROTLI_OPERATION_PROCESS));
						/**
						 * Выполняем сжатие
						 */
						do {
							// Получаем выходной буфер
							uint8_t * nextOut = work;
							// Получаем размер выходного буфера
							size_t availOut = AWH_COMPRESSOR_STREAM_CHUNK;
							// Выполняем сжатие
							if(!::BrotliEncoderCompressStream(this->_enc, op, &availIn, &nextIn, &availOut, &nextOut, nullptr))
								// Выводим отрицательный результат
								return false;
							// Вычисляем количество произведённых данных
							const size_t produced = (AWH_COMPRESSOR_STREAM_CHUNK - availOut);
							// Добавляем данные в результат
							if(produced > 0)
								// Добавляем данные в результат
								out.insert(out.end(), reinterpret_cast <char *> (work), reinterpret_cast <char *> (work) + produced);
						/**
						 * Пока есть входные данные или готовый выход
						 */
						} while((availIn > 0) || ::BrotliEncoderHasMoreOutput(this->_enc));
						// Устанавливаем флаг завершённости потока
						this->_done = ((op == BROTLI_OPERATION_FINISH) && ::BrotliEncoderIsFinished(this->_enc));
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Если контекст не создан
						if(this->_dec == nullptr)
							// Выводим отрицательный результат
							return false;
						// Результат распаковки
						BrotliDecoderResult ret = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
						/**
						 * Выполняем распаковку
						 */
						do {
							// Получаем выходной буфер
							uint8_t * nextOut = work;
							// Получаем размер выходного буфера
							size_t availOut = AWH_COMPRESSOR_STREAM_CHUNK;
							// Выполняем распаковку
							ret = ::BrotliDecoderDecompressStream(this->_dec, &availIn, &nextIn, &availOut, &nextOut, nullptr);
							// Вычисляем количество произведённых данных
							const size_t produced = (AWH_COMPRESSOR_STREAM_CHUNK - availOut);
							// Добавляем данные в результат
							if(produced > 0)
								// Добавляем данные в результат
								out.insert(out.end(), reinterpret_cast <char *> (work), reinterpret_cast <char *> (work) + produced);
							// Если возникла ошибка
							if(ret == BROTLI_DECODER_RESULT_ERROR)
								// Выводим отрицательный результат
								return false;
						/**
						 * Пока требуется место в выходном буфере
						 */
						} while(ret == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT);
						// Устанавливаем флаг завершённости потока
						this->_done = (ret == BROTLI_DECODER_RESULT_SUCCESS);
					} break;
				}
				// Выводим результат операции
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param event  направление операции
			 * @param params параметры инициализации
			 *
			 */
			explicit brotli_coder_t(const event_t event, const params_t & params) noexcept :
			 _done(false), _event(event), _enc(nullptr), _dec(nullptr) {
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Создаём контекст энкодера
						this->_enc = ::BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
						// Если контекст энкодера создан и качество компрессии укладывается в допустимый диапазон
						if((this->_enc != nullptr) && (params.level > BROTLI_MIN_QUALITY) && (params.level <= BROTLI_MAX_QUALITY))
							// Выполняем установку качества компрессии
							::BrotliEncoderSetParameter(this->_enc, BROTLI_PARAM_QUALITY, static_cast <uint32_t> (params.level));
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE):
						// Создаём контекст декодера
						this->_dec = ::BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
					break;
				}
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~brotli_coder_t() noexcept override {
				// Освобождаем контекст энкодера
				if(this->_enc != nullptr)
					// Освобождаем контекст энкодера
					::BrotliEncoderDestroyInstance(this->_enc);
				// Освобождаем контекст декодера
				if(this->_dec != nullptr)
					// Освобождаем контекст декодера
					::BrotliDecoderDestroyInstance(this->_dec);
			}
	};
	/**
	 * @brief Класс бэкенда LZMA (формат .xz)
	 *
	 */
	class lzma_coder_t : public coder_t {
		private:
			// Флаг успешной инициализации
			bool _init;
			// Флаг завершённости потока
			bool _done;
			// Направление операции
			event_t _event;
		private:
			// Поток LZMA
			lzma_stream _strm;
		public:
			/**
			 * @brief Метод проверки успешной инициализации контекста
			 *
			 * @return результат проверки
			 *
			 */
			bool valid() const noexcept override {
				// Выводим результат проверки
				return this->_init;
			}
			/**
			 * @brief Метод проверки завершённости потока
			 *
			 * @return результат проверки
			 *
			 */
			bool done() const noexcept override {
				// Выводим результат проверки
				return this->_done;
			}
		public:
			/**
			 * @brief Метод обработки порции данных
			 *
			 * @param buffer буфер входящих данных
			 * @param size   размер входящих данных
			 * @param flush  режим сброса
			 * @param out    буфер, куда добавляется готовый выход
			 * @return       результат операции
			 *
			 */
			bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept override {
				// Если контекст не инициализирован
				if(!this->_init)
					// Выводим отрицательный результат
					return false;
				// Если поток уже завершён
				if(this->_done)
					// Выводим положительный результат (ничего не делаем)
					return true;
				// Устанавливаем входной размер данных
				this->_strm.avail_in = size;
				// Устанавливаем входной буфер
				this->_strm.next_in = reinterpret_cast <const uint8_t *> (buffer);
				// Определяем действие
				lzma_action action = LZMA_RUN;
				// Для энкодера учитываем режим сброса
				if(this->_event == event_t::ENCODE)
					// Определяем действие в зависимости от режима сброса
					action = (flush == flush_t::FINISH ? LZMA_FINISH : (flush == flush_t::SYNC ? LZMA_SYNC_FLUSH : LZMA_RUN));
				// Результат операции
				lzma_ret ret = LZMA_OK;
				// Рабочий буфер
				uint8_t work[AWH_COMPRESSOR_STREAM_CHUNK];
				/**
				 * Выполняем обработку пока есть данные или заполнен выходной буфер
				 */
				do {
					// Устанавливаем выходной буфер
					this->_strm.next_out = work;
					// Устанавливаем размер выходного буфера
					this->_strm.avail_out = AWH_COMPRESSOR_STREAM_CHUNK;
					// Выполняем обработку
					ret = ::lzma_code(&this->_strm, action);
					// Если возникла ошибка
					if((ret != LZMA_OK) && (ret != LZMA_STREAM_END))
						// Выводим отрицательный результат
						return false;
					// Вычисляем количество произведённых данных
					const size_t produced = (AWH_COMPRESSOR_STREAM_CHUNK - this->_strm.avail_out);
					// Добавляем данные в результат
					if(produced > 0)
						// Добавляем данные в результат
						out.insert(out.end(), reinterpret_cast <char *> (work), reinterpret_cast <char *> (work) + produced);
					// Если движок сигнализировал о завершении
					if(ret == LZMA_STREAM_END){
						/**
						 * Для энкодера в режиме SYNC-flush LZMA_STREAM_END означает лишь завершение текущего сброса,
						 * а не всего потока, поэтому поток завершённым не помечаем и просто прерываем цикл обработки
						 */
						if((this->_event == event_t::ENCODE) && (flush == flush_t::SYNC))
							// Прерываем цикл обработки, не помечая поток завершённым
							break;
						// Помечаем поток завершённым и выходим из цикла обработки
						this->_done = true;
						// Выходим из цикла
						break;
					}
				/**
				 * Пока есть входные данные или выходной буфер полностью заполнен
				 */
				} while((this->_strm.avail_in > 0) || (this->_strm.avail_out == 0));
				// Выводим результат операции
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param event  направление операции
			 * @param params параметры инициализации
			 *
			 */
			explicit lzma_coder_t(const event_t event, const params_t & params) noexcept :
			 _init(false), _done(false), _event(event) {
				// Инициализируем поток значением по умолчанию
				const lzma_stream tmp = LZMA_STREAM_INIT;
				// Устанавливаем поток
				this->_strm = tmp;
				// Результат инициализации контекста
				lzma_ret ret = LZMA_PROG_ERROR;
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Определяем пресет компрессии
						const uint32_t preset = ((params.level >= 0) && (params.level <= 9) ? static_cast <uint32_t> (params.level) : static_cast <uint32_t> (LZMA_PRESET_DEFAULT));
						// Инициализируем энкодер
						ret = ::lzma_easy_encoder(&this->_strm, preset, LZMA_CHECK_CRC32);
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE):
						// Инициализируем декодер
						ret = ::lzma_stream_decoder(&this->_strm, UINT64_MAX, 0);
					break;
				}
				// Запоминаем результат инициализации
				this->_init = (ret == LZMA_OK);
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~lzma_coder_t() noexcept override {
				// Если контекст был инициализирован, освобождаем его
				if(this->_init)
					// Освобождаем контекст LZMA
					::lzma_end(&this->_strm);
			}
	};
	/**
	 * @brief Класс бэкенда Бэкенд BZip2
	 *
	 */
	class bzip2_coder_t : public coder_t {
		private:
			// Флаг успешной инициализации
			bool _init;
			// Флаг завершённости потока
			bool _done;
			// Направление операции
			event_t _event;
		private:
			// Поток BZip2
			bz_stream _strm;
		public:
			/**
			 * @brief Метод проверки успешной инициализации контекста
			 *
			 * @return результат проверки
			 *
			 */
			bool valid() const noexcept override {
				// Выводим результат проверки
				return this->_init;
			}
			/**
			 * @brief Метод проверки завершённости потока
			 *
			 * @return результат проверки
			 *
			 */
			bool done() const noexcept override {
				// Выводим результат проверки
				return this->_done;
			}
		public:
			/**
			 * @brief Метод обработки порции данных
			 *
			 * @param buffer буфер входящих данных
			 * @param size   размер входящих данных
			 * @param flush  режим сброса
			 * @param out    буфер, куда добавляется готовый выход
			 * @return       результат операции
			 *
			 */
			bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept override {
				// Если контекст не инициализирован
				if(!this->_init)
					// Выводим отрицательный результат
					return false;
				// Если поток уже завершён
				if(this->_done)
					// Выводим положительный результат (ничего не делаем)
					return true;
				// Устанавливаем входной размер данных
				this->_strm.avail_in = static_cast <uint32_t> (size);
				// Устанавливаем входные данные
				this->_strm.next_in = const_cast <char *> (reinterpret_cast <const char *> (buffer));
				// Рабочий буфер
				char work[AWH_COMPRESSOR_STREAM_CHUNK];
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Результат операции
						int32_t ret = BZ_OK;
						// Флаг завершения операции
						bool finished = false;
						// Определяем действие
						const int32_t action = (flush == flush_t::FINISH ? BZ_FINISH : (flush == flush_t::SYNC ? BZ_FLUSH : BZ_RUN));
						/**
						 * Выполняем сжатие
						 */
						do {
							// Устанавливаем выходной буфер
							this->_strm.next_out = work;
							// Устанавливаем размер выходного буфера
							this->_strm.avail_out = static_cast <uint32_t> (AWH_COMPRESSOR_STREAM_CHUNK);
							// Выполняем сжатие
							ret = ::BZ2_bzCompress(&this->_strm, action);
							// Если возникла ошибка
							if((ret != BZ_RUN_OK) && (ret != BZ_FLUSH_OK) && (ret != BZ_FINISH_OK) && (ret != BZ_STREAM_END))
								// Выводим отрицательный результат
								return false;
							// Вычисляем количество произведённых данных
							const size_t produced = (AWH_COMPRESSOR_STREAM_CHUNK - this->_strm.avail_out);
							// Добавляем данные в результат
							if(produced > 0)
								// Добавляем данные в результат
								out.insert(out.end(), work, work + produced);
							// Если поток завершён
							if((this->_done = (ret == BZ_STREAM_END)))
								// Завершаем операцию
								finished = true;
							// Для обычной подачи данных операция завершена после потребления входа
							else if(action == BZ_RUN)
								// Завершаем операцию, если входные данные полностью обработаны и выходной буфер не заполнен
								finished = ((this->_strm.avail_in == 0) && (this->_strm.avail_out != 0));
							// Для сброса операция завершена, когда буфер выдавлен полностью
							else if(action == BZ_FLUSH)
								// Завершаем операцию только после полного выдавливания буфера
								finished = ((ret == BZ_RUN_OK) && (this->_strm.avail_out != 0));
						/**
						 * Пока операция не завершена
						 */
						} while(!finished);
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Результат операции
						int32_t ret = BZ_OK;
						/**
						 * Выполняем распаковку пока есть данные
						 */
						do {
							// Устанавливаем выходной буфер
							this->_strm.next_out = work;
							// Устанавливаем размер выходного буфера
							this->_strm.avail_out = static_cast <uint32_t> (AWH_COMPRESSOR_STREAM_CHUNK);
							// Выполняем распаковку
							ret = ::BZ2_bzDecompress(&this->_strm);
							// Если возникла ошибка
							if((ret != BZ_OK) && (ret != BZ_STREAM_END))
								// Выводим отрицательный результат
								return false;
							// Вычисляем количество произведённых данных
							const size_t produced = (AWH_COMPRESSOR_STREAM_CHUNK - this->_strm.avail_out);
							// Добавляем данные в результат
							if(produced > 0)
								// Добавляем данные в результат
								out.insert(out.end(), work, work + produced);
							// Если поток завершён
							if((this->_done = (ret == BZ_STREAM_END)))
								// Выходим из цикла
								break;
						/**
						 * Пока есть входные данные или выходной буфер полностью заполнен
						 */
						} while((this->_strm.avail_in > 0) || (this->_strm.avail_out == 0));
					} break;
				}
				// Выводим результат операции
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param event  направление операции
			 * @param params параметры инициализации
			 *
			 */
			explicit bzip2_coder_t(const event_t event, const params_t & params) noexcept :
			 _init(false), _done(false), _event(event) {
				// Результат инициализации контекста
				int32_t ret = BZ_OK;
				// Обнуляем поток
				::memset(&this->_strm, 0, sizeof(this->_strm));
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Определяем размер блока (100k * blockSize100k)
						const int32_t block = (((params.level >= 1) && (params.level <= 9)) ? params.level : 9);
						// Инициализируем энкодер
						ret = ::BZ2_bzCompressInit(&this->_strm, block, 0, 0);
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE):
						// Инициализируем декодер
						ret = ::BZ2_bzDecompressInit(&this->_strm, 0, 0);
					break;
				}
				// Запоминаем результат инициализации
				this->_init = (ret == BZ_OK);
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~bzip2_coder_t() noexcept override {
				// Если контекст был инициализирован, освобождаем его
				if(this->_init){
					/**
					 * Определяем направление операции
					 */
					switch(static_cast <uint8_t> (this->_event)){
						// Для компрессии
						case static_cast <uint8_t> (event_t::ENCODE):
							// Завершаем работу контекста
							::BZ2_bzCompressEnd(&this->_strm);
						break;
						// Для декомпрессии
						case static_cast <uint8_t> (event_t::DECODE):
							// Завершаем работу контекста
							::BZ2_bzDecompressEnd(&this->_strm);
						break;
					}
				}
			}
	};
	/**
	 * @brief Класс бэкенда LZ4 (потоковый Frame-формат)
	 *
	 */
	class lz4_coder_t : public coder_t {
		private:
			// Флаг успешной инициализации
			bool _init;
			// Флаг завершённости потока
			bool _done;
			// Флаг записи заголовка кадра
			bool _begun;
			// Направление операции
			event_t _event;
		private:
			// Контекст компрессии
			LZ4F_cctx * _cctx;
			// Контекст декомпрессии
			LZ4F_dctx * _dctx;
		private:
			// Параметры компрессии
			LZ4F_preferences_t _prefs;
		public:
			/**
			 * @brief Метод проверки успешной инициализации контекста
			 *
			 * @return результат проверки
			 *
			 */
			bool valid() const noexcept override {
				// Выводим результат проверки
				return this->_init;
			}
			/**
			 * @brief Метод проверки завершённости потока
			 *
			 * @return результат проверки
			 *
			 */
			bool done() const noexcept override {
				// Выводим результат проверки
				return this->_done;
			}
		public:
			/**
			 * @brief Метод обработки порции данных
			 *
			 * @param buffer буфер входящих данных
			 * @param size   размер входящих данных
			 * @param flush  режим сброса
			 * @param out    буфер, куда добавляется готовый выход
			 * @return       результат операции
			 *
			 */
			bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept override {
				// Если контекст не инициализирован
				if(!this->_init)
					// Выводим отрицательный результат
					return false;
				// Если поток уже завершён
				if(this->_done)
					// Выводим положительный результат (ничего не делаем)
					return true;
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Вычисляем максимально возможный размер выходных данных
						const size_t bound = ::LZ4F_compressBound(size, &this->_prefs);
						// Рабочий буфер (с запасом на заголовок и завершение кадра)
						vector <char> work(bound + LZ4F_HEADER_SIZE_MAX + 8, 0);
						// Если заголовок кадра ещё не записан
						if(!this->_begun){
							// Записываем заголовок кадра
							const size_t bytes = ::LZ4F_compressBegin(this->_cctx, &work[0], work.size(), &this->_prefs);
							// Если возникла ошибка
							if(::LZ4F_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем заголовок в результат
							if(bytes > 0)
								// Добавляем заголовок в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
							// Устанавливаем флаг записи заголовка
							this->_begun = true;
						}
						// Если есть входные данные
						if(size > 0){
							// Выполняем сжатие порции данных
							const size_t bytes = ::LZ4F_compressUpdate(this->_cctx, &work[0], work.size(), buffer, size, nullptr);
							// Если возникла ошибка
							if(::LZ4F_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(bytes > 0)
								// Добавляем данные в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
						}
						// Если требуется сброс данных
						if(flush == flush_t::SYNC){
							// Выполняем выдавливание буфера
							const size_t bytes = ::LZ4F_flush(this->_cctx, &work[0], work.size(), nullptr);
							// Если возникла ошибка
							if(::LZ4F_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(bytes > 0)
								// Добавляем данные в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
						// Если требуется финализация потока
						} else if(flush == flush_t::FINISH) {
							// Выполняем завершение кадра
							const size_t bytes = ::LZ4F_compressEnd(this->_cctx, &work[0], work.size(), nullptr);
							// Если возникла ошибка
							if(::LZ4F_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(bytes > 0)
								// Добавляем данные в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
							// Устанавливаем флаг завершённости
							this->_done = true;
						}
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Остаток входных данных
						size_t remaining = size;
						// Рабочий буфер
						char work[AWH_COMPRESSOR_STREAM_CHUNK];
						// Формируем указатели на входные данные
						const char * src = reinterpret_cast <const char *> (buffer);
						/**
						 * Выполняем распаковку пока есть входные данные
						 */
						do {
							// Устанавливаем размер потребляемых и производимых данных
							size_t srcSize = remaining;
							// Устанавливаем размер выходного буфера
							size_t dstSize = AWH_COMPRESSOR_STREAM_CHUNK;
							// Выполняем распаковку
							const size_t ret = ::LZ4F_decompress(this->_dctx, work, &dstSize, src, &srcSize, nullptr);
							// Если возникла ошибка
							if(::LZ4F_isError(ret))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(dstSize > 0)
								// Добавляем данные в результат
								out.insert(out.end(), &work[0], &work[0] + dstSize);
							// Смещаем указатель входных данных
							src += srcSize;
							// Уменьшаем остаток входных данных
							remaining -= srcSize;
							// Если кадр полностью декодирован
							if((this->_done = (ret == 0)))
								// Выходим из цикла
								break;
							// Если данные не потребляются и не производятся, выходим
							if((srcSize == 0) && (dstSize == 0))
								// Выходим из цикла
								break;
						/**
						 * Пока есть входные данные
						 */
						} while(remaining > 0);
					} break;
				}
				// Выводим результат операции
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param event  направление операции
			 * @param params параметры инициализации
			 *
			 */
			explicit lz4_coder_t(const event_t event, const params_t & params) noexcept :
			 _init(false), _done(false), _begun(false), _event(event), _cctx(nullptr), _dctx(nullptr) {
				// Обнуляем параметры
				::memset(&this->_prefs, 0, sizeof(this->_prefs));
				// Устанавливаем уровень компрессии
				this->_prefs.compressionLevel = params.level;
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Создаём контекст компрессии
						const LZ4F_errorCode_t ret = ::LZ4F_createCompressionContext(&this->_cctx, LZ4F_VERSION);
						// Запоминаем результат инициализации
						this->_init = (!::LZ4F_isError(ret) && (this->_cctx != nullptr));
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Создаём контекст декомпрессии
						const LZ4F_errorCode_t ret = ::LZ4F_createDecompressionContext(&this->_dctx, LZ4F_VERSION);
						// Запоминаем результат инициализации
						this->_init = (!::LZ4F_isError(ret) && (this->_dctx != nullptr));
					} break;
				}
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~lz4_coder_t() noexcept override {
				// Освобождаем контекст компрессии
				if(this->_cctx != nullptr)
					// Освобождаем контекст компрессии
					::LZ4F_freeCompressionContext(this->_cctx);
				// Освобождаем контекст декомпрессии
				if(this->_dctx != nullptr)
					// Освобождаем контекст декомпрессии
					::LZ4F_freeDecompressionContext(this->_dctx);
			}
	};
	/**
	 * @brief Класс бэкенда Lizard (потоковый Frame-формат)
	 *
	 */
	class lizard_coder_t : public coder_t {
		private:
			// Флаг успешной инициализации
			bool _init;
			// Флаг завершённости потока
			bool _done;
			// Флаг записи заголовка кадра
			bool _begun;
			// Направление операции
			event_t _event;
		private:
			// Параметры компрессии
			LizardF_preferences_t _prefs;
		private:
			// Контекст компрессии
			LizardF_compressionContext_t _cctx;
			// Контекст декомпрессии
			LizardF_decompressionContext_t _dctx;
		public:
			/**
			 * @brief Метод проверки успешной инициализации контекста
			 *
			 * @return результат проверки
			 *
			 */
			bool valid() const noexcept override {
				// Выводим результат проверки
				return this->_init;
			}
			/**
			 * @brief Метод проверки завершённости потока
			 *
			 * @return результат проверки
			 *
			 */
			bool done() const noexcept override {
				// Выводим результат проверки
				return this->_done;
			}
		public:
			/**
			 * @brief Метод обработки порции данных
			 *
			 * @param buffer буфер входящих данных
			 * @param size   размер входящих данных
			 * @param flush  режим сброса
			 * @param out    буфер, куда добавляется готовый выход
			 * @return       результат операции
			 *
			 */
			bool code(const void * buffer, const size_t size, const flush_t flush, vector <char> & out) noexcept override {
				// Если контекст не инициализирован
				if(!this->_init)
					// Выводим отрицательный результат
					return false;
				// Если поток уже завершён
				if(this->_done)
					// Выводим положительный результат (ничего не делаем)
					return true;
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Вычисляем максимально возможный размер выходных данных
						const size_t bound = ::LizardF_compressBound(size, &this->_prefs);
						// Рабочий буфер (с запасом на заголовок и завершение кадра)
						vector <char> work(bound + 32, 0);
						// Если заголовок кадра ещё не записан
						if(!this->_begun){
							// Записываем заголовок кадра
							const size_t bytes = ::LizardF_compressBegin(this->_cctx, &work[0], work.size(), &this->_prefs);
							// Если возникла ошибка
							if(::LizardF_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем заголовок в результат
							if(bytes > 0)
								// Добавляем заголовок в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
							// Устанавливаем флаг записи заголовка
							this->_begun = true;
						}
						// Если есть входные данные
						if(size > 0){
							// Выполняем сжатие порции данных
							const size_t bytes = ::LizardF_compressUpdate(this->_cctx, &work[0], work.size(), buffer, size, nullptr);
							// Если возникла ошибка
							if(::LizardF_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(bytes > 0)
								// Добавляем данные в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
						}
						// Если требуется сброс данных
						if(flush == flush_t::SYNC){
							// Выполняем выдавливание буфера
							const size_t bytes = ::LizardF_flush(this->_cctx, &work[0], work.size(), nullptr);
							// Если возникла ошибка
							if(::LizardF_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(bytes > 0)
								// Добавляем данные в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
						// Если требуется финализация потока
						} else if(flush == flush_t::FINISH) {
							// Выполняем завершение кадра
							const size_t bytes = ::LizardF_compressEnd(this->_cctx, &work[0], work.size(), nullptr);
							// Если возникла ошибка
							if(::LizardF_isError(bytes))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(bytes > 0)
								// Добавляем данные в результат
								out.insert(out.end(), &work[0], &work[0] + bytes);
							// Устанавливаем флаг завершённости
							this->_done = true;
						}
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Остаток входных данных
						size_t remaining = size;
						// Рабочий буфер
						char work[AWH_COMPRESSOR_STREAM_CHUNK];
						// Формируем указатель на входные данные
						const char * src = reinterpret_cast <const char *> (buffer);
						/**
						 * Выполняем распаковку пока есть входные данные
						 */
						do {
							// Устанавливаем входной размер данных
							size_t srcSize = remaining;
							// Устанавливаем размер выходного буфера
							size_t dstSize = AWH_COMPRESSOR_STREAM_CHUNK;
							// Выполняем распаковку
							const size_t ret = ::LizardF_decompress(this->_dctx, work, &dstSize, src, &srcSize, nullptr);
							// Если возникла ошибка
							if(::LizardF_isError(ret))
								// Выводим отрицательный результат
								return false;
							// Добавляем данные в результат
							if(dstSize > 0)
								// Добавляем данные в результат
								out.insert(out.end(), work, work + dstSize);
							// Смещаем указатель входных данных
							src += srcSize;
							// Уменьшаем остаток входных данных
							remaining -= srcSize;
							// Если кадр полностью декодирован
							if((this->_done = (ret == 0)))
								// Выходим из цикла
								break;
							// Если данные не потребляются и не производятся, выходим
							if((srcSize == 0) && (dstSize == 0))
								// Выходим из цикла
								break;
						/**
						 * Пока есть входные данные
						 */
						} while(remaining > 0);
					} break;
				}
				// Выводим результат операции
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param event  направление операции
			 * @param params параметры инициализации
			 *
			 */
			explicit lizard_coder_t(const event_t event, const params_t & params) noexcept :
			 _init(false), _done(false), _begun(false), _event(event), _cctx(nullptr), _dctx(nullptr) {
				// Обнуляем параметры
				::memset(&this->_prefs, 0, sizeof(this->_prefs));
				// Устанавливаем уровень компрессии
				this->_prefs.compressionLevel = params.level;
				/**
				 * Определяем направление операции
				 */
				switch(static_cast <uint8_t> (this->_event)){
					// Для компрессии
					case static_cast <uint8_t> (event_t::ENCODE): {
						// Создаём контекст компрессии
						const LizardF_errorCode_t ret = ::LizardF_createCompressionContext(&this->_cctx, LIZARDF_VERSION);
						// Запоминаем результат инициализации
						this->_init = (!::LizardF_isError(ret) && (this->_cctx != nullptr));
					} break;
					// Для декомпрессии
					case static_cast <uint8_t> (event_t::DECODE): {
						// Создаём контекст декомпрессии
						const LizardF_errorCode_t ret = ::LizardF_createDecompressionContext(&this->_dctx, LIZARDF_VERSION);
						// Запоминаем результат инициализации
						this->_init = (!::LizardF_isError(ret) && (this->_dctx != nullptr));
					} break;
				}
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~lizard_coder_t() noexcept override {
				// Освобождаем контекст компрессии
				if(this->_cctx != nullptr)
					// Освобождаем контекст компрессии
					::LizardF_freeCompressionContext(this->_cctx);
				// Освобождаем контекст декомпрессии
				if(this->_dctx != nullptr)
					// Освобождаем контекст декомпрессии
					::LizardF_freeDecompressionContext(this->_dctx);
			}
	};
	/**
	 * @brief Функция создания бэкенда потокового движка
	 *
	 * @param method метод компрессии
	 * @param event  направление операции
	 * @param params параметры инициализации
	 * @return       созданный бэкенд или nullptr, если метод не поддерживает потоковый режим
	 *
	 */
	static unique_ptr <coder_t> makeCoder(const method_t method, const event_t event, const params_t & params) noexcept {
		/**
		 * Определяем метод компрессии
		 */
		switch(static_cast <uint8_t> (method)){
			// Если методы принадлежат к семейству Zlib
			case static_cast <uint8_t> (method_t::GZIP):
			case static_cast <uint8_t> (method_t::ZLIB):
			case static_cast <uint8_t> (method_t::DEFLATE):
				// Создаём бэкенд Zlib
				return unique_ptr <coder_t> (new zlib_coder_t(method, event, params));
			// Если метод принадлежит к семейству Zstandard
			case static_cast <uint8_t> (method_t::ZSTD):
				// Создаём бэкенд Zstandard
				return unique_ptr <coder_t> (new zstd_coder_t(event, params));
			// Если метод принадлежит к семейству Brotli
			case static_cast <uint8_t> (method_t::BROTLI):
				// Создаём бэкенд Brotli
				return unique_ptr <coder_t> (new brotli_coder_t(event, params));
			// Если метод принадлежит к семейству LZMA
			case static_cast <uint8_t> (method_t::LZMA):
				// Создаём бэкенд LZMA
				return unique_ptr <coder_t> (new lzma_coder_t(event, params));
			// Если метод принадлежит к семейству BZip2
			case static_cast <uint8_t> (method_t::BZIP2):
				// Создаём бэкенд BZip2
				return unique_ptr <coder_t> (new bzip2_coder_t(event, params));
			// Если метод принадлежит к семейству LZ4
			case static_cast <uint8_t> (method_t::LZ4):
				// Создаём бэкенд LZ4
				return unique_ptr <coder_t> (new lz4_coder_t(event, params));
			// Если метод принадлежит к семейству Lizard
			case static_cast <uint8_t> (method_t::LIZARD):
				// Создаём бэкенд Lizard
				return unique_ptr <coder_t> (new lizard_coder_t(event, params));
		}
		// Для остальных методов потоковый режим не поддерживается
		return nullptr;
	}
};

/**
 * @brief Метод проверки завершённости потока
 *
 * @return результат проверки
 *
 */
bool awh::compressor::Stream::done() const noexcept {
	// Выводим результат проверки
	return (this->_coder != nullptr ? this->_coder->done() : true);
}
/**
 * @brief Метод проверки валидности потока
 *
 * @return результат проверки
 *
 */
bool awh::compressor::Stream::valid() const noexcept {
	// Выводим результат проверки
	return (this->_coder != nullptr);
}
/**
 * @brief Метод получения направления операции потока
 *
 * @return направление операции
 *
 */
awh::compressor::event_t awh::compressor::Stream::event() const noexcept {
	// Выводим направление операции
	return this->_event;
}
/**
 * @brief Метод получения метода компрессии потока
 *
 * @return метод компрессии
 *
 */
awh::compressor::method_t awh::compressor::Stream::method() const noexcept {
	// Выводим метод компрессии
	return this->_method;
}
/**
 * @brief Шаблон метода принудительного выдавливания накопленного
 *
 * @tparam T тип контейнера результата
 *
 */
template <typename T>
/**
 * @brief Метод принудительного выдавливания накопленного
 *
 * @param result контейнер, куда помещается готовый выход
 *
 */
void awh::compressor::Stream::flush(T & result) noexcept {
	// Выполняем подачу пустой порции с режимом SYNC
	this->push <T> (nullptr, 0, result, flush_t::SYNC);
}
/**
 * @brief Явные специализации шаблона метода выдавливания
 *
 */
template void awh::compressor::Stream::flush <string> (string &) noexcept;
template void awh::compressor::Stream::flush <vector <char>> (vector <char> &) noexcept;
template void awh::compressor::Stream::flush <vector <uint8_t>> (vector <uint8_t> &) noexcept;
/**
 * @brief Шаблон метода финализации потока
 *
 * @tparam T тип контейнера результата
 *
 */
template <typename T>
/**
 * @brief Метод финализации потока
 *
 * @param result контейнер, куда помещается остаток данных
 *
 */
void awh::compressor::Stream::finish(T & result) noexcept {
	// Выполняем подачу пустой порции с режимом FINISH
	this->push <T> (nullptr, 0, result, flush_t::FINISH);
}
/**
 * @brief Явные специализации шаблона метода финализации
 *
 */
template void awh::compressor::Stream::finish <string> (string &) noexcept;
template void awh::compressor::Stream::finish <vector <char>> (vector <char> &) noexcept;
template void awh::compressor::Stream::finish <vector <uint8_t>> (vector <uint8_t> &) noexcept;
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
void awh::compressor::Stream::push(string_view buffer, T & result, const flush_t flush) noexcept {
	// Делегируем базовой реализации
	this->push <T> (buffer.data(), buffer.size(), result, flush);
}
/**
 * @brief Явные специализации шаблона метода подачи порции данных
 *
 */
template void awh::compressor::Stream::push <string> (string_view, string &, const flush_t) noexcept;
template void awh::compressor::Stream::push <vector <char>> (string_view, vector <char> &, const flush_t) noexcept;
template void awh::compressor::Stream::push <vector <uint8_t>> (string_view, vector <uint8_t> &, const flush_t) noexcept;
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
void awh::compressor::Stream::push(const void * buffer, const size_t size, T & result, const flush_t flush) noexcept {
	// Выполняем очистку результата
	result.clear();
	// Если поток невалиден, выходим
	if(this->_coder == nullptr)
		// Выходим из функции
		return;
	/**
	 * Если результат уже требуемого типа, кодер пишет прямо в него — без промежуточной копии
	 */
	if constexpr(is_same <T, vector <char>>::value){
		// Выполняем обработку порции данных
		if(!this->_coder->code(buffer, size, flush, result)){
			// Освобождаем контекст (поток становится невалидным)
			this->_coder.reset();
			// Очищаем результат
			result.clear();
			// Выходим
			return;
		}
	/**
	 * Для остальных типов контейнера используем промежуточный буфер
	 */
	} else {
		// Буфер готового выхода
		vector <char> out;
		// Выполняем обработку порции данных
		if(!this->_coder->code(buffer, size, flush, out)){
			// Освобождаем контекст (поток становится невалидным)
			this->_coder.reset();
			// Очищаем результат
			result.clear();
			// Выходим
			return;
		}
		// Если данные получены, формируем результат
		if(!out.empty())
			// Формируем результат
			result.assign(out.begin(), out.end());
	}
}
/**
 * @brief Явные специализации шаблона метода подачи порции данных
 *
 */
template void awh::compressor::Stream::push <string> (const void *, const size_t, string &, const flush_t) noexcept;
template void awh::compressor::Stream::push <vector <char>> (const void *, const size_t, vector <char> &, const flush_t) noexcept;
template void awh::compressor::Stream::push <vector <uint8_t>> (const void *, const size_t, vector <uint8_t> &, const flush_t) noexcept;
/**
 * @brief Оператор перемещения
 *
 * @param stream объект для перемещения
 * @return       текущий объект
 *
 */
awh::compressor::Stream & awh::compressor::Stream::operator = (stream_t && stream) noexcept {
	// Если объект не совпадает
	if(this != &stream){
		// Устанавливаем объект для работы с логами
		this->_log = stream._log;
		// Устанавливаем направление операции
		this->_event = stream._event;
		// Устанавливаем метод компрессии
		this->_method = stream._method;
		// Перемещаем данные
		this->_coder = ::move(stream._coder);
		// Сбрасываем направление операции
		stream._event = event_t::NONE;
		// Сбрасываем метод компрессии
		stream._method = method_t::NONE;
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Конструктор перемещения
 *
 */
awh::compressor::Stream::Stream(stream_t && stream) noexcept :
 _event(stream._event), _method(stream._method),
 _coder(::move(stream._coder)), _log(stream._log) {
	// Сбрасываем направление операции у источника
	stream._event = event_t::NONE;
	// Сбрасываем метод компрессии у источника
	stream._method = method_t::NONE;
}
/**
 * @brief Конструктор пустого (невалидного) потока
 *
 */
awh::compressor::Stream::Stream() noexcept :
 _event(event_t::NONE), _method(method_t::NONE),
 _coder(nullptr), _log(nullptr) {}
/**
 * @brief Конструктор
 *
 * @param method метод компрессии
 * @param event  направление операции
 * @param params параметры инициализации
 * @param log    объект для работы с логами
 *
 */
awh::compressor::Stream::Stream(const method_t method, const event_t event, const params_t & params, const log_t * log) noexcept :
 _event(event), _method(method), _coder(nullptr), _log(log) {
	// Создаём бэкенд для указанного метода
	this->_coder = makeCoder(method, event, params);
	// Если бэкенд создан, но контекст не инициализирован — сбрасываем
	if((this->_coder != nullptr) && !this->_coder->valid())
		// Сбрасываем бэкенд (поток становится невалидным)
		this->_coder.reset();
}
/**
 * @brief Деструктор
 *
 */
awh::compressor::Stream::~Stream() noexcept {}
