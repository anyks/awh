/**
 * @file: nginx.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения парсера протокола HTTP/1.x с парсером сервера nginx —
 *        потоковым конечным автоматом, работающим по буферу заголовков ограниченного размера
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем обвязку окружения сравниваемой реализации
 */
extern "C" {
	#include <ngx_config.h>
	#include <ngx_core.h>
	#include <ngx_http.h>
};

/**
 * @brief Обвязка парсера сервера nginx под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Размер буфера заголовочного блока в октетах
	 *
	 * @details Повторяет размер буфера сервера, задаваемый директивой
	 *          `large_client_header_buffers`: разбор заголовков идёт по буферу
	 *          фиксированного размера, потому что парсер сохраняет указатели
	 *          в него между вызовами и перевыделение буфера их обесценило бы.
	 *          Отсюда же и известное ограничение сервера на суммарный размер
	 *          заголовков, о превышении которого он отвечает кодом 494
	 *
	 */
	static constexpr size_t HEADER_BUFFER = 8192;
	/**
	 * @brief Размер пула памяти разбора в октетах
	 *
	 */
	static constexpr size_t POOL_SIZE = 65536;
	/**
	 * @brief Класс разбора сообщений парсером сервера nginx
	 *
	 * @details Парсер сервера отдаёт разобранные части сообщения не функциями
	 *          обратного вызова, а границами во входном буфере: разбор строки
	 *          запроса и каждой строки заголовка завершается возвратом из функции,
	 *          а вызывающая сторона забирает результат из полей состояния запроса.
	 *          Кадрирование тела парсер тоже не выполняет: выбор между
	 *          Content-Length и chunked остаётся за сервером
	 *
	 */
	class Engine {
		private:
			// Буфер заголовочного блока сообщения
			std::vector <u_char> _header;
			// Память пула разбора
			std::vector <u_char> _arena;
			// Состояние разбираемого запроса
			ngx_http_request_t _request;
			// Буфер разбора заголовочного блока
			ngx_buf_t _buffer;
			// Подключение разбираемого запроса
			ngx_connection_t _connection;
			// Журнал разбираемого запроса
			ngx_log_t _log;
			// Пул памяти разбора
			ngx_pool_t _pool;
			// Состояние разбора тела в кодировке chunked
			ngx_http_chunked_t _chunk;
			// Размер занятой части буфера заголовочного блока
			size_t _used;
			// Остаток тела фиксированного размера в октетах
			size_t _remaining;
			// Флаг разобранности строки запроса
			bool _line;
			// Флаг разобранности заголовочного блока
			bool _headers;
			// Флаг кодирования тела сообщения методом chunked
			bool _chunked;
			// Флаг завершённости разбора сообщения
			bool _complete;
		private:
			/**
			 * @brief Метод учёта разобранного заголовка сообщения
			 *
			 * @note Работа, которую потоковые парсеры выполняют внутри себя:
			 *       парсер сервера о кадрировании тела ничего не знает, и выбор
			 *       между Content-Length и chunked выполняет сервер
			 *
			 */
			void header() noexcept {
				// Вычисляем размер названия заголовка
				const size_t name = static_cast <size_t> (this->_request.header_name_end - this->_request.header_name_start);
				// Вычисляем размер значения заголовка
				const size_t value = static_cast <size_t> (this->_request.header_end - this->_request.header_start);
				// Учитываем разобранный заголовок
				rival::account(name, value);
				// Если разобран заголовок размера тела сообщения
				if((name == 14) && (::strncasecmp(reinterpret_cast <const char *> (this->_request.header_name_start), "Content-Length", 14) == 0))
					// Устанавливаем остаток тела фиксированного размера
					this->_remaining = static_cast <size_t> (::strtoull(reinterpret_cast <const char *> (this->_request.header_start), nullptr, 10));
				// Если разобран заголовок кодирования тела сообщения
				else if((name == 17) && (::strncasecmp(reinterpret_cast <const char *> (this->_request.header_name_start), "Transfer-Encoding", 17) == 0)) {
					// Если телом сообщения применяется кодирование методом chunked
					if((value >= 7) && (::strncasecmp(reinterpret_cast <const char *> (this->_request.header_end - 7), "chunked", 7) == 0))
						// Устанавливаем флаг кодирования тела методом chunked
						this->_chunked = true;
				}
			}
			/**
			 * @brief Метод разбора заголовочного блока сообщения
			 *
			 * @return результат разбора (false - заголовочный блок разобран с ошибкой)
			 *
			 */
			bool block() noexcept {
				// Если строка запроса ещё не разобрана
				if(!this->_line){
					// Выполняем разбор строки запроса
					const ngx_int_t result = ::ngx_http_parse_request_line(&this->_request, &this->_buffer);
					// Если строка запроса ещё не получена целиком
					if(result == NGX_AGAIN)
						// Выводим положительный результат
						return true;
					// Если строка запроса разобрана с ошибкой
					if(result != NGX_OK)
						// Выводим отрицательный результат
						return false;
					// Отмечаем разобранность строки запроса
					this->_line = true;
					// Сбрасываем состояние разбора для перехода к заголовкам
					this->_request.state = 0;
				}
				/**
				 * Выполняем разбор строк заголовочного блока
				 */
				while(!this->_headers){
					// Выполняем разбор очередной строки заголовка
					const ngx_int_t result = ::ngx_http_parse_header_line(&this->_request, &this->_buffer, 1);
					// Если строка заголовка разобрана
					if(result == NGX_OK){
						// Выполняем учёт разобранного заголовка
						this->header();
						// Переходим к следующей строке заголовка
						continue;
					}
					// Если заголовочный блок получен целиком
					if(result == NGX_HTTP_PARSE_HEADER_DONE){
						// Отмечаем разобранность заголовочного блока
						this->_headers = true;
						// Если тело сообщения отсутствует
						if(!this->_chunked && (this->_remaining == 0))
							// Отмечаем завершённость разбора сообщения
							this->_complete = true;
						// Выводим положительный результат
						return true;
					}
					// Если заголовочный блок ещё не получен целиком
					if(result == NGX_AGAIN)
						// Выводим положительный результат
						return true;
					// Выводим отрицательный результат
					return false;
				}
				// Выводим положительный результат
				return true;
			}
			/**
			 * @brief Метод обработки фрагмента тела сообщения
			 *
			 * @param data данные фрагмента тела сообщения
			 * @param size размер фрагмента тела сообщения
			 * @return     результат обработки (false - тело разобрано с ошибкой)
			 *
			 */
			bool payload(const u_char * data, const size_t size) noexcept {
				// Если тело сообщения закодировано методом chunked
				if(this->_chunked){
					// Буфер разбора фрагмента тела сообщения
					ngx_buf_t buffer;
					// Устанавливаем начало фрагмента тела сообщения
					buffer.pos = const_cast <u_char *> (data);
					// Устанавливаем конец фрагмента тела сообщения
					buffer.last = (buffer.pos + size);
					/**
					 * Выполняем разбор кадров тела сообщения
					 */
					while(true){
						// Выполняем разбор очередного кадра тела сообщения
						const ngx_int_t result = ::ngx_http_parse_chunked(&this->_request, &buffer, &this->_chunk, 0);
						// Если получен фрагмент данных очередного кадра
						if(result == NGX_OK){
							// Вычисляем размер доступной части данных кадра
							size_t length = static_cast <size_t> (buffer.last - buffer.pos);
							// Если доступная часть данных превышает остаток кадра
							if(static_cast <off_t> (length) > this->_chunk.size)
								// Ограничиваем размер доступной части остатком кадра
								length = static_cast <size_t> (this->_chunk.size);
							// Выполняем потребление фрагмента данных кадра
							rival::consume(buffer.pos, length);
							// Уменьшаем остаток данных кадра
							this->_chunk.size -= static_cast <off_t> (length);
							// Выполняем сдвиг границы разбора
							buffer.pos += length;
							// Переходим к следующему кадру тела сообщения
							continue;
						}
						// Если тело сообщения получено целиком
						if(result == NGX_DONE){
							// Отмечаем завершённость разбора сообщения
							this->_complete = true;
							// Выводим положительный результат
							return true;
						}
						// Если тело сообщения ещё не получено целиком
						if(result == NGX_AGAIN)
							// Выводим положительный результат
							return true;
						// Выводим отрицательный результат
						return false;
					}
				}
				// Вычисляем размер потребляемой части фрагмента тела
				const size_t length = ((size < this->_remaining) ? size : this->_remaining);
				// Выполняем потребление фрагмента тела сообщения
				rival::consume(data, length);
				// Уменьшаем остаток тела фиксированного размера
				this->_remaining -= length;
				// Если тело сообщения получено полностью
				if(this->_remaining == 0)
					// Отмечаем завершённость разбора сообщения
					this->_complete = true;
				// Выводим положительный результат
				return true;
			}
		public:
			/**
			 * @brief Метод сброса состояния для разбора следующего сообщения
			 *
			 */
			void reset() noexcept {
				// Выполняем очистку состояния разбираемого запроса
				this->_request = {};
				// Устанавливаем подключение разбираемого запроса
				this->_request.connection = &this->_connection;
				// Устанавливаем пул памяти разбора
				this->_request.pool = &this->_pool;
				// Сбрасываем границу занятой памяти пула
				this->_pool.last = this->_arena.data();
				// Устанавливаем границу памяти пула
				this->_pool.end = (this->_arena.data() + this->_arena.size());
				// Сбрасываем состояние разбора тела в кодировке chunked
				this->_chunk = {};
				// Устанавливаем начало буфера заголовочного блока
				this->_buffer.pos = this->_header.data();
				// Устанавливаем конец буфера заголовочного блока
				this->_buffer.last = this->_header.data();
				// Сбрасываем размер занятой части буфера заголовочного блока
				this->_used = 0;
				// Сбрасываем остаток тела фиксированного размера
				this->_remaining = 0;
				// Сбрасываем флаг разобранности строки запроса
				this->_line = false;
				// Сбрасываем флаг разобранности заголовочного блока
				this->_headers = false;
				// Сбрасываем флаг кодирования тела методом chunked
				this->_chunked = false;
				// Сбрасываем флаг завершённости разбора сообщения
				this->_complete = false;
			}
			/**
			 * @brief Метод подачи фрагмента сообщения
			 *
			 * @param data данные фрагмента сообщения
			 * @param size размер фрагмента сообщения
			 * @return     результат разбора (false - фрагмент разобран с ошибкой)
			 *
			 */
			bool feed(const char * data, const size_t size) noexcept {
				// Размер уже обработанной части фрагмента сообщения
				size_t offset = 0;
				// Если заголовочный блок сообщения ещё не разобран
				if(!this->_headers){
					// Вычисляем размер свободной части буфера заголовочного блока
					const size_t room = (this->_header.size() - this->_used);
					// Вычисляем размер копируемой в буфер части фрагмента
					const size_t length = ((size < room) ? size : room);
					/**
					 * Заголовочный блок обязан лежать в одном буфере целиком: парсер
					 * сохраняет указатели в него между вызовами, и накопление
					 * разорванного сообщения возложено на вызывающую сторону
					 */
					::memcpy(this->_header.data() + this->_used, data, length);
					// Увеличиваем размер занятой части буфера заголовочного блока
					this->_used += length;
					// Устанавливаем конец разбираемых данных
					this->_buffer.last = (this->_header.data() + this->_used);
					// Запоминаем размер обработанной части фрагмента сообщения
					offset = length;
					// Если заголовочный блок разобран с ошибкой
					if(!this->block())
						// Выводим отрицательный результат
						return false;
					// Если заголовочный блок ещё не разобран целиком
					if(!this->_headers)
						// Выводим положительный результат
						return true;
					// Вычисляем размер остатка буфера заголовочного блока
					const size_t rest = static_cast <size_t> (this->_buffer.last - this->_buffer.pos);
					// Если за заголовочным блоком в буфере следует начало тела
					if(!this->_complete && (rest > 0)){
						// Если начало тела сообщения разобрано с ошибкой
						if(!this->payload(this->_buffer.pos, rest))
							// Выводим отрицательный результат
							return false;
					}
				}
				// Если фрагмент сообщения обработан целиком
				if(offset >= size)
					// Выводим положительный результат
					return true;
				// Если разбор сообщения уже завершён
				if(this->_complete)
					// Выводим положительный результат
					return true;
				// Выполняем обработку остатка фрагмента как части тела сообщения
				return this->payload(reinterpret_cast <const u_char *> (data) + offset, size - offset);
			}
			/**
			 * @brief Метод проверки завершённости разбора сообщения
			 *
			 * @return результат проверки (true - сообщение разобрано целиком)
			 *
			 */
			bool complete() const noexcept {
				// Выводим флаг завершённости разбора сообщения
				return this->_complete;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Engine() noexcept :
			 _header(HEADER_BUFFER), _arena(POOL_SIZE), _request{}, _buffer{},
			 _connection{}, _log{}, _pool{}, _chunk{}, _used(0), _remaining(0),
			 _line(false), _headers(false), _chunked(false), _complete(false) {
				// Устанавливаем журнал подключения
				this->_connection.log = &this->_log;
				// Выполняем сброс состояния разбора
				this->reset();
			}
	};
};

/**
 * @brief Функция входа в стенд
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Получаем фильтр названий выполняемых сценариев
	const char * mask = rival::filter(argc, argv);
	// Создаём объект разбора сообщений сравниваемой реализацией
	Engine engine;
	// Выполняем сценарий разбора запроса без заголовков
	rival::execute("http1/parse/tiny-request", rival::metric_t::MESSAGES, engine, rival::tiny(), rival::TINY_ROUNDS, 0, mask);
	// Выполняем сценарий разбора запроса браузера
	rival::execute("http1/parse/typical-request", rival::metric_t::THROUGHPUT, engine, rival::typical(), rival::TYPICAL_ROUNDS, 0, mask);
	// Выполняем сценарий разбора запроса браузера при побайтовой подаче
	rival::execute("http1/parse/fragmented-request", rival::metric_t::THROUGHPUT, engine, rival::typical(), rival::FRAGMENT_ROUNDS, 1, mask);
	// Выполняем сценарий разбора тела фиксированного размера
	rival::execute("http1/parse/identity-body", rival::metric_t::THROUGHPUT, engine, rival::identity(rival::BODY_SIZE), rival::BODY_ROUNDS, 0, mask);
	// Выполняем сценарий разбора тела в кодировке chunked
	rival::execute("http1/parse/chunked-body", rival::metric_t::THROUGHPUT, engine, rival::chunked(rival::BODY_SIZE, rival::CHUNK_SIZE), rival::BODY_ROUNDS, 0, mask);
	// Выполняем сценарий учёта выделений памяти на одно разобранное сообщение
	rival::execute("http1/allocations/per-parsed-message", rival::metric_t::ALLOCATIONS, engine, rival::typical(), rival::TYPICAL_ROUNDS, 0, mask);
	// Выводим контрольную сумму обработанных данных
	rival::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}
