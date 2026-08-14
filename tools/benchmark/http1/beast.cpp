/**
 * @file beast.cpp
 * @date 2026-07-26
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
 * @brief Эталонный стенд сравнения парсера протокола HTTP/1.x с парсером Boost.Beast —
 *        потоковым разборщиком, складывающим разобранное сообщение в собственный контейнер
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <optional>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

/**
 * @brief Обвязка парсера Boost.Beast под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Размер приёмника тела сообщения в октетах
	 *
	 */
	static constexpr size_t SINK_SIZE = 65536;
	/**
	 * @brief Класс разбора сообщений парсером Boost.Beast
	 *
	 * @details Единственная сравниваемая реализация, складывающая разобранное
	 *          сообщение в собственный контейнер: строка запроса и заголовки
	 *          копируются в объект сообщения и остаются доступны после разбора.
	 *          Тело при этом отдаётся без копирования - разбор ведётся в режиме
	 *          `buffer_body`, в котором приёмник тела предоставляет вызывающая
	 *          сторона. Удобный режим `string_body` собирал бы тело в строку
	 *          целиком, и сравнение измеряло бы уже не разбор, а копирование
	 *
	 */
	class Engine {
		private:
			// Объект парсера сравниваемой реализации
			std::optional <boost::beast::http::parser <true, boost::beast::http::buffer_body>> _parser;
			// Приёмник фрагментов тела сообщения
			std::vector <char> _sink;
			// Накопитель неразобранного остатка сообщения
			std::string _pending;
			// Размер разобранной части накопителя
			size_t _offset;
			// Флаг учтённости заголовков сообщения
			bool _counted;
			// Флаг ошибки разбора сообщения
			bool _failed;
			// Флаг завершённости разбора сообщения
			bool _complete;
		private:
			/**
			 * @brief Метод учёта разобранных заголовков сообщения
			 *
			 * @note Заголовки отдаются не по одному в момент разбора, а списком в
			 *       составе разобранного сообщения, поэтому учёт выполняется один
			 *       раз по завершении заголовочного блока
			 *
			 */
			void headers() noexcept {
				// Отмечаем учтённость заголовков сообщения
				this->_counted = true;
				/**
				 * Перебираем разобранные заголовки сообщения
				 */
				for(const auto & header : this->_parser->get().base())
					// Учитываем разобранный заголовок
					rival::account(header.name_string().size(), header.value().size());
			}
		public:
			/**
			 * @brief Метод сброса состояния для разбора следующего сообщения
			 *
			 * @note Операции сброса парсер не предоставляет: состояние возвращается
			 *       в исходное пересозданием объекта, вместе с которым уничтожается
			 *       и контейнер разобранного сообщения
			 *
			 */
			void reset() noexcept {
				// Выполняем очистку накопителя неразобранного остатка
				this->_pending.clear();
				// Сбрасываем размер разобранной части накопителя
				this->_offset = 0;
				// Сбрасываем флаг ошибки разбора сообщения
				this->_failed = false;
				// Сбрасываем флаг учтённости заголовков сообщения
				this->_counted = false;
				// Сбрасываем флаг завершённости разбора сообщения
				this->_complete = false;
				// Выполняем пересоздание объекта парсера
				this->_parser.emplace();
				// Снимаем ограничение на размер тела сообщения
				this->_parser->body_limit(boost::none);
				// Снимаем ограничение на размер заголовочного блока
				this->_parser->header_limit(0xFFFFFFFF);
				// Включаем разбор тела сообщения без ожидания запроса потребителя
				this->_parser->eager(true);
			}
			/**
			 * @brief Метод разбора непрерывной области данных сообщения
			 *
			 * @param data данные области сообщения
			 * @param size размер области сообщения
			 * @return     размер разобранной части области сообщения
			 *
			 */
			size_t drain(const char * data, const size_t size) noexcept {
				// Размер разобранной части области сообщения
				size_t result = 0;
				/**
				 * Выполняем разбор области данных сообщения
				 */
				while(result < size){
					// Если разбор сообщения уже завершён
					if(this->_parser->is_done())
						// Прекращаем разбор области данных сообщения
						break;
					// Устанавливаем приёмник фрагментов тела сообщения
					this->_parser->get().body().data = this->_sink.data();
					// Устанавливаем размер приёмника фрагментов тела сообщения
					this->_parser->get().body().size = this->_sink.size();
					// Код ошибки разбора области данных сообщения
					boost::beast::error_code error;
					// Выполняем разбор области данных сообщения
					const size_t length = this->_parser->put(boost::asio::const_buffer(data + result, size - result), error);
					// Вычисляем размер полученного фрагмента тела сообщения
					const size_t received = (this->_sink.size() - this->_parser->get().body().size);
					// Если фрагмент тела сообщения получен
					if(received > 0)
						// Выполняем потребление фрагмента тела сообщения
						rival::consume(this->_sink.data(), received);
					// Увеличиваем размер разобранной части области сообщения
					result += length;
					// Если заголовочный блок разобран, а заголовки ещё не учтены
					if(!this->_counted && this->_parser->is_header_done())
						// Выполняем учёт разобранных заголовков сообщения
						this->headers();
					// Если приёмник тела сообщения заполнен либо получен конец кадра
					if((error == boost::beast::http::error::need_buffer) || (error == boost::beast::http::error::end_of_chunk))
						// Переходим к разбору следующей части области сообщения
						continue;
					// Если разобранных данных для продолжения недостаточно
					if(error == boost::beast::http::error::need_more)
						// Прекращаем разбор до получения следующего фрагмента
						break;
					// Если область данных сообщения разобрана с ошибкой
					if(error){
						// Отмечаем ошибку разбора сообщения
						this->_failed = true;
						// Прекращаем разбор области данных сообщения
						break;
					}
					// Если разбор области данных сообщения не продвинулся
					if(length == 0)
						// Прекращаем разбор до получения следующего фрагмента
						break;
				}
				// Если разбор сообщения завершён
				if(this->_parser->is_done())
					// Отмечаем завершённость разбора сообщения
					this->_complete = true;
				// Выводим размер разобранной части области сообщения
				return result;
			}
			/**
			 * @brief Метод подачи фрагмента сообщения
			 *
			 * @note Парсер неразобранный остаток входа у себя не удерживает и
			 *       требует предъявить его повторно вместе со следующим фрагментом,
			 *       поэтому накопление разорванного сообщения возложено на
			 *       вызывающую сторону: в составе сервера эту роль выполняет
			 *       приёмный буфер сокета
			 *
			 * @param data данные фрагмента сообщения
			 * @param size размер фрагмента сообщения
			 * @return     результат разбора (false - фрагмент разобран с ошибкой)
			 *
			 */
			bool feed(const char * data, const size_t size) noexcept {
				/**
				 * Если накопитель пуст, разбор выполняется прямо во входных данных:
				 * накапливать приходится только неразобранный остаток
				 */
				if(this->_pending.empty()){
					// Выполняем разбор входных данных
					const size_t length = this->drain(data, size);
					// Если входные данные разобраны не полностью
					if(!this->_failed && !this->_complete && (length < size)){
						// Выполняем накопление неразобранного остатка
						this->_pending.assign(data + length, size - length);
						// Сбрасываем размер разобранной части накопителя
						this->_offset = 0;
					}
					// Выводим результат разбора фрагмента сообщения
					return !this->_failed;
				}
				// Дописываем фрагмент сообщения в накопитель
				this->_pending.append(data, size);
				// Выполняем разбор неразобранного остатка накопителя
				this->_offset += this->drain(this->_pending.data() + this->_offset, this->_pending.size() - this->_offset);
				// Если накопитель разобран целиком
				if(this->_offset >= this->_pending.size()){
					// Выполняем очистку накопителя
					this->_pending.clear();
					// Сбрасываем размер разобранной части накопителя
					this->_offset = 0;
				}
				// Выводим результат разбора фрагмента сообщения
				return !this->_failed;
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
			 _parser(), _sink(SINK_SIZE), _pending(""), _offset(0),
			 _counted(false), _failed(false), _complete(false) {
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
