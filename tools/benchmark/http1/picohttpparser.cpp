/**
 * @file picohttpparser.cpp
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
 * @brief Эталонный стенд сравнения парсера протокола HTTP/1.x с парсером picohttpparser
 *        проекта h2o — непотоковым разборщиком заголовочного блока с векторным сканированием
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
#include <picohttpparser.h>

/**
 * @brief Обвязка парсера picohttpparser под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Максимальное количество заголовков одного сообщения
	 *
	 */
	static constexpr size_t MAX_HEADERS = 64;
	/**
	 * @brief Класс разбора сообщений парсером picohttpparser
	 *
	 * @details Потоковым парсер не является: заголовочный блок разбирается
	 *          только целиком, поэтому разорванное сообщение обязана накапливать
	 *          вызывающая сторона. Кадрирования тела парсер тоже не выполняет -
	 *          выбор между Content-Length и chunked, отслеживание остатка тела и
	 *          вызов декодировщика чанков возложены на вызывающую сторону, и вся
	 *          эта работа входит в измеряемое время наравне с самим разбором
	 *
	 */
	class Engine {
		private:
			// Накопитель заголовочного блока разорванного сообщения
			std::string _pending;
			// Изменяемый буфер декодирования тела в кодировке chunked
			std::vector <char> _scratch;
			// Состояние декодировщика тела в кодировке chunked
			struct phr_chunked_decoder _decoder;
			// Размер накопителя на прошлой попытке разбора
			size_t _last;
			// Остаток тела фиксированного размера в октетах
			size_t _remaining;
			// Флаг разобранности заголовочного блока
			bool _headers;
			// Флаг кодирования тела сообщения методом chunked
			bool _chunked;
			// Флаг завершённости разбора сообщения
			bool _complete;
		private:
			/**
			 * @brief Метод определения кадрирования тела по разобранным заголовкам
			 *
			 * @note Работа, которую потоковые парсеры выполняют внутри себя:
			 *       picohttpparser отдаёт заголовки списком и о кадрировании
			 *       тела ничего не знает
			 *
			 * @param headers список разобранных заголовков
			 * @param count   количество разобранных заголовков
			 *
			 */
			void framing(const struct phr_header * headers, const size_t count) noexcept {
				/**
				 * Перебираем список разобранных заголовков
				 */
				for(size_t i = 0; i < count; i++){
					// Учитываем разобранный заголовок
					rival::account(headers[i].name_len, headers[i].value_len);
					// Если разобран заголовок размера тела сообщения
					if((headers[i].name_len == 14) && (::strncasecmp(headers[i].name, "Content-Length", 14) == 0))
						// Устанавливаем остаток тела фиксированного размера
						this->_remaining = static_cast <size_t> (::strtoull(headers[i].value, nullptr, 10));
					// Если разобран заголовок кодирования тела сообщения
					else if((headers[i].name_len == 17) && (::strncasecmp(headers[i].name, "Transfer-Encoding", 17) == 0)) {
						// Если телом сообщения применяется кодирование методом chunked
						if((headers[i].value_len >= 7) && (::strncasecmp((headers[i].value + headers[i].value_len) - 7, "chunked", 7) == 0))
							// Устанавливаем флаг кодирования тела методом chunked
							this->_chunked = true;
					}
				}
			}
			/**
			 * @brief Метод обработки фрагмента тела сообщения
			 *
			 * @param data данные фрагмента тела сообщения
			 * @param size размер фрагмента тела сообщения
			 * @return     результат обработки (false - тело разобрано с ошибкой)
			 *
			 */
			bool payload(const char * data, const size_t size) noexcept {
				// Если тело сообщения закодировано методом chunked
				if(this->_chunked){
					// Если размер буфера декодирования недостаточен
					if(this->_scratch.size() < size)
						// Выполняем увеличение буфера декодирования
						this->_scratch.resize(size);
					/**
					 * Декодировщик чанков работает по месту и портит поданный буфер,
					 * поэтому фрагмент приходится копировать: это требование интерфейса
					 * сравниваемой реализации, и стоимость копирования входит в замер
					 */
					::memcpy(this->_scratch.data(), data, size);
					// Размер декодируемого фрагмента тела
					size_t length = size;
					// Выполняем декодирование фрагмента тела сообщения
					const ssize_t result = ::phr_decode_chunked(&this->_decoder, this->_scratch.data(), &length);
					// Если фрагмент тела декодирован с ошибкой
					if(result == -1)
						// Выводим отрицательный результат
						return false;
					// Выполняем потребление декодированного фрагмента тела
					rival::consume(this->_scratch.data(), length);
					// Если тело сообщения декодировано полностью
					if(result >= 0)
						// Отмечаем завершённость разбора сообщения
						this->_complete = true;
					// Выводим положительный результат
					return true;
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
			/**
			 * @brief Метод разбора заголовочного блока сообщения
			 *
			 * @param data данные заголовочного блока
			 * @param size размер данных заголовочного блока
			 * @param last размер данных на прошлой попытке разбора
			 * @return     размер разобранного заголовочного блока (0 - блок неполон, -1 - ошибка)
			 *
			 */
			int32_t block(const char * data, const size_t size, const size_t last) noexcept {
				// Название метода запроса
				const char * method = nullptr;
				// Размер названия метода запроса
				size_t methodLength = 0;
				// Адрес запрашиваемого ресурса
				const char * path = nullptr;
				// Размер адреса запрашиваемого ресурса
				size_t pathLength = 0;
				// Младший номер версии протокола
				int32_t minor = 0;
				// Список разобранных заголовков сообщения
				struct phr_header headers[MAX_HEADERS];
				// Количество разобранных заголовков сообщения
				size_t count = MAX_HEADERS;
				// Выполняем разбор заголовочного блока сообщения
				const int32_t result = ::phr_parse_request(
					data, size, &method, &methodLength,
					&path, &pathLength, &minor, headers, &count, last
				);
				// Если заголовочный блок разобран с ошибкой
				if(result == -1)
					// Выводим признак ошибки разбора
					return -1;
				// Если заголовочный блок ещё не получен целиком
				if(result == -2)
					// Выводим признак неполноты заголовочного блока
					return 0;
				// Выполняем определение кадрирования тела сообщения
				this->framing(headers, count);
				// Отмечаем разобранность заголовочного блока
				this->_headers = true;
				// Если тело сообщения отсутствует
				if(!this->_chunked && (this->_remaining == 0))
					// Отмечаем завершённость разбора сообщения
					this->_complete = true;
				// Выводим размер разобранного заголовочного блока
				return result;
			}
		public:
			/**
			 * @brief Метод сброса состояния для разбора следующего сообщения
			 *
			 */
			void reset() noexcept {
				// Выполняем очистку накопителя заголовочного блока
				this->_pending.clear();
				// Сбрасываем состояние декодировщика тела в кодировке chunked
				this->_decoder = {};
				// Активируем потребление трейлеров декодировщиком
				this->_decoder.consume_trailer = 1;
				// Сбрасываем размер накопителя на прошлой попытке разбора
				this->_last = 0;
				// Сбрасываем остаток тела фиксированного размера
				this->_remaining = 0;
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
				// Если заголовочный блок сообщения уже разобран
				if(this->_headers)
					// Выполняем обработку фрагмента тела сообщения
					return this->payload(data, size);
				/**
				 * Если накопитель пуст, разбор выполняется прямо во входных данных:
				 * так сравниваемую реализацию и применяют, накапливая сообщение
				 * только после того, как оно оказалось разорванным
				 */
				if(this->_pending.empty()){
					// Выполняем разбор заголовочного блока во входных данных
					const int32_t result = this->block(data, size, 0);
					// Если заголовочный блок разобран с ошибкой
					if(result < 0)
						// Выводим отрицательный результат
						return false;
					// Если заголовочный блок разобран целиком
					if(result > 0){
						// Если за заголовочным блоком следует тело сообщения
						if(!this->_complete && (static_cast <size_t> (result) < size))
							// Выполняем обработку фрагмента тела сообщения
							return this->payload(data + result, size - static_cast <size_t> (result));
						// Выводим положительный результат
						return true;
					}
					// Выполняем накопление неполного заголовочного блока
					this->_pending.assign(data, size);
					// Запоминаем размер накопителя на прошлой попытке разбора
					this->_last = size;
					// Выводим положительный результат
					return true;
				}
				// Дописываем фрагмент сообщения в накопитель заголовочного блока
				this->_pending.append(data, size);
				// Выполняем разбор заголовочного блока в накопителе
				const int32_t result = this->block(this->_pending.data(), this->_pending.size(), this->_last);
				// Если заголовочный блок разобран с ошибкой
				if(result < 0)
					// Выводим отрицательный результат
					return false;
				// Если заголовочный блок ещё не получен целиком
				if(result == 0){
					// Запоминаем размер накопителя на прошлой попытке разбора
					this->_last = this->_pending.size();
					// Выводим положительный результат
					return true;
				}
				// Если за заголовочным блоком следует тело сообщения
				if(!this->_complete && (static_cast <size_t> (result) < this->_pending.size()))
					// Выполняем обработку остатка накопителя как фрагмента тела
					return this->payload(this->_pending.data() + result, this->_pending.size() - static_cast <size_t> (result));
				// Выводим положительный результат
				return true;
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
			 _pending(""), _scratch(), _decoder{}, _last(0), _remaining(0),
			 _headers(false), _chunked(false), _complete(false) {
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
