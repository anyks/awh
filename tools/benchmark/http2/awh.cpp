/**
 * @file: awh.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения протокола HTTP/2 библиотеки AWH — та же
 *        нагрузка и тот же прогон, что и у сравниваемых реализаций
 *
 * @details Показатели библиотеки снимает её собственный набор бенчмарков, но в
 *          сравнении участвует именно этот стенд: он проводит реализацию через тот
 *          же драйвер прогона, что и остальных, и снимает с неё ту же контрольную
 *          сумму. Иначе разница в обвязке замера осталась бы неотделима от
 *          разницы в самих реализациях
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
#include <proto/http/parser/http2/http.hpp>

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Обвязка реализации библиотеки AWH под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Функция получения объекта фреймворка окружения стенда
	 *
	 * @note Объекты окружения создаются при первом обращении: порядок статической
	 *       инициализации между единицами трансляции не определён, а фреймворк
	 *       зависит от таблиц чужих модулей
	 *
	 * @return объект фреймворка
	 *
	 */
	static const awh::fmk_t * fmk() noexcept {
		// Объект фреймворка окружения стенда
		static awh::fmk_t result;
		// Выводим объект фреймворка
		return &result;
	}
	/**
	 * @brief Функция получения объекта логирования окружения стенда
	 *
	 * @return объект логирования
	 *
	 */
	static const awh::log_t * logger() noexcept {
		// Объект логирования окружения стенда
		static awh::log_t result(::fmk());
		// Отключаем вывод логов: часть сценариев намеренно упирается в лимиты
		result.level(awh::log_t::level_t::NONE);
		// Выводим объект логирования
		return &result;
	}
	/**
	 * @brief Класс сжатия заголовков реализацией библиотеки AWH
	 *
	 */
	class Codec {
		private:
			// Объект кодера заголовков сравниваемой реализации
			h2::hpack::encoder_t _encoder;
			// Объект декодера заголовков сравниваемой реализации
			h2::hpack::decoder_t _decoder;
		private:
			// Буфер закодированного блока заголовков
			std::string _block;
			// Наборы заголовков в представлении сравниваемой реализации
			std::vector <std::vector <h2::hpack::field_t>> _sets;
			// Декодированные заголовки очередного блока
			std::vector <h2::hpack::field_view_t> _fields;
		public:
			/**
			 * @brief Метод сброса состояния кодера и декодера
			 *
			 */
			void restart() noexcept {
				// Пересоздаём объект кодера заголовков
				this->_encoder = h2::hpack::encoder_t(rival::TABLE_SIZE);
				// Пересоздаём объект декодера заголовков
				this->_decoder = h2::hpack::decoder_t(rival::TABLE_SIZE);
				/**
				 * Отключаем автоматическое определение чувствительных заголовков:
				 * признак чувствительности задан нагрузкой одинаково для всех
				 * реализаций, и политика кодера сравнение искажать не должна
				 */
				this->_encoder.sensitiveHeuristic(false);
			}
			/**
			 * @brief Метод перевода наборов заголовков в представление реализации
			 *
			 * @param sets наборы заголовков эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние наборы заголовков
				this->_sets.clear();
				// Резервируем память под наборы заголовков
				this->_sets.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов заголовков нагрузки
				 */
				for(const auto & set : sets){
					// Набор заголовков в представлении сравниваемой реализации
					std::vector <h2::hpack::field_t> fields;
					// Резервируем память под набор заголовков
					fields.reserve(set.size());
					/**
					 * Выполняем перебор всех заголовков набора
					 */
					for(const auto & field : set)
						// Дописываем очередной заголовок набора
						fields.emplace_back(field.name, field.value, field.sensitive);
					// Дописываем сформированный набор заголовков
					this->_sets.push_back(::std::move(fields));
				}
			}
			/**
			 * @brief Метод кодирования блока заголовков
			 *
			 * @param index номер набора заголовков
			 * @return      размер закодированного блока заголовков
			 *
			 */
			size_t encode(const size_t index) noexcept {
				// Очищаем буфер закодированного блока
				this->_block.clear();
				// Кодируем блок заголовков запроса
				this->_encoder.encode(this->_sets[index], this->_block, true);
				// Выводим размер закодированного блока заголовков
				return this->_block.size();
			}
			/**
			 * @brief Метод декодирования блока заголовков
			 *
			 * @param block закодированный блок заголовков
			 * @return      результат декодирования
			 *
			 */
			bool decode(const std::string & block) noexcept {
				// Код ошибки декодирования
				h2::error_t error = h2::error_t::NO_ERROR;
				// Если блок заголовков декодирован с ошибкой
				if(this->_decoder.decode(block, this->_fields, 0, error) != h2::status_t::OK)
					// Выводим отрицательный результат
					return false;
				/**
				 * Выполняем перебор всех декодированных заголовков
				 */
				for(const auto & field : this->_fields)
					// Учитываем декодированный заголовок
					rival::account(field.name.size(), field.value.size());
				// Выводим положительный результат
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Codec() noexcept : _encoder(rival::TABLE_SIZE), _decoder(rival::TABLE_SIZE) {
				// Резервируем память под буфер блока заголовков
				this->_block.reserve(4096);
			}
	};
	/**
	 * @brief Класс разбора входящего потока реализацией библиотеки AWH
	 *
	 */
	class Server {
		private:
			// Объект разборщика сервера
			parser_http2_t _server;
		private:
			// Количество разобранных запросов
			size_t _handled;
			// Объём принятого тела
			size_t _accepted;
		private:
			// Минимальный ответ сервера: заголовки без тела
			std::vector <h2::hpack::field_t> _answer;
		public:
			/**
			 * @brief Метод получения количества разобранных запросов
			 *
			 * @return количество разобранных запросов
			 *
			 */
			size_t handled() const noexcept {
				// Выводим количество разобранных запросов
				return this->_handled;
			}
			/**
			 * @brief Метод получения объёма принятого тела
			 *
			 * @return объём принятого тела
			 *
			 */
			size_t accepted() const noexcept {
				// Выводим объём принятого тела
				return this->_accepted;
			}
			/**
			 * @brief Метод подачи порции входящего потока
			 *
			 * @param data данные порции входящего потока
			 * @param size размер порции входящего потока
			 *
			 */
			void feed(const char * data, const size_t size) noexcept {
				// Выполняем разбор порции входящего потока
				this->_server.parse(data, size);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param answering отвечать на принятые запросы
			 *
			 */
			explicit Server(const bool answering) noexcept :
			 _server(direct_t::REQUEST, ::fmk(), ::logger()), _handled(0), _accepted(0) {
				// Дописываем псевдо-заголовок статуса ответа
				this->_answer.emplace_back(":status", "200");
				// Дописываем заголовок длины содержимого
				this->_answer.emplace_back("content-length", "0");
				// Устанавливаем функцию обратного вызова записи исходящих байт
				this->_server.on(parser_http2_t::write_callback_t([](const void *, const size_t) noexcept {}));
				// Устанавливаем функцию обратного вызова заголовка сообщения
				this->_server.on(parser_http2_t::header_callback_t([](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
					// Учитываем разобранный заголовок
					rival::account(name.size(), value.size());
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова тела сообщения
				this->_server.on(parser_http2_t::data_callback_t([this](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
					// Выполняем потребление фрагмента тела сообщения
					rival::consume(buffer, size);
					// Суммируем объём принятого тела
					this->_accepted += size;
					// Продолжаем разбор
					return true;
				}));
				/**
				 * Устанавливаем функцию обратного вызова провайдера заголовков. Сервер обязан
				 * ответить: без ответа поток остаётся полуоткрытым, такие потоки копятся
				 * и упираются в лимит одновременных потоков - ровно как у настоящего сервера
				 */
				this->_server.on(parser_http2_t::provider_callback_t([this, answering](const uint32_t sid, const provider_t * provider, const bool) noexcept -> bool {
					// Если получен провайдер запроса
					if(provider != nullptr){
						// Считаем разобранный запрос
						this->_handled++;
						// Если на принятые запросы требуется отвечать
						if(answering)
							// Отправляем минимальный ответ с завершением потока
							this->_server.sendHeaders(sid, this->_answer, true);
					}
					// Продолжаем разбор
					return true;
				}));
				// Отправляем преамбулу соединения сервера
				this->_server.sendPreface();
			}
	};
	/**
	 * @brief Класс полного обмена парой реализаций библиотеки AWH
	 *
	 */
	class Pair {
		private:
			// Объект разборщика клиента
			parser_http2_t _client;
			// Объект разборщика сервера
			parser_http2_t _server;
		private:
			// Количество завершённых обменов
			size_t _completed;
		private:
			// Заголовки ответа сервера, подготовленные заранее
			std::vector <h2::hpack::field_t> _answer;
			// Наборы заголовков запроса в представлении сравниваемой реализации
			std::vector <std::vector <h2::hpack::field_t>> _sets;
		public:
			/**
			 * @brief Метод получения количества завершённых обменов
			 *
			 * @return количество завершённых обменов
			 *
			 */
			size_t completed() const noexcept {
				// Выводим количество завершённых обменов
				return this->_completed;
			}
			/**
			 * @brief Метод прокачки исходящих очередей обеих сторон
			 *
			 * @details Реализация отдаёт исходящие байты функцией обратного вызова
			 *          сразу при отправке, поэтому прокачивать нечего
			 *
			 */
			void pump() noexcept {}
			/**
			 * @brief Метод перевода наборов заголовков в представление реализации
			 *
			 * @param sets наборы заголовков эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние наборы заголовков
				this->_sets.clear();
				// Резервируем память под наборы заголовков
				this->_sets.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов заголовков нагрузки
				 */
				for(const auto & set : sets){
					// Набор заголовков в представлении сравниваемой реализации
					std::vector <h2::hpack::field_t> fields;
					// Резервируем память под набор заголовков
					fields.reserve(set.size());
					/**
					 * Выполняем перебор всех заголовков набора
					 */
					for(const auto & field : set)
						// Дописываем очередной заголовок набора
						fields.emplace_back(field.name, field.value, field.sensitive);
					// Дописываем сформированный набор заголовков
					this->_sets.push_back(::std::move(fields));
				}
			}
			/**
			 * @brief Метод открытия потока с отправкой запроса
			 *
			 * @param index номер набора заголовков запроса
			 * @return      результат открытия потока
			 *
			 */
			bool open(const size_t index) noexcept {
				// Выделяем идентификатор нового потока клиента
				const uint32_t sid = this->_client.nextStreamId();
				// Если пространство идентификаторов исчерпано
				if(sid == 0)
					// Выводим отрицательный результат
					return false;
				// Отправляем заголовки запроса с завершением потока
				this->_client.sendHeaders(sid, this->_sets[index], true);
				// Выводим положительный результат
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Pair() noexcept :
			 _client(direct_t::RESPONSE, ::fmk(), ::logger()), _server(direct_t::REQUEST, ::fmk(), ::logger()), _completed(0) {
				// Формируем заголовки ответа сервера
				for(const auto & field : rival::response(0))
					// Дописываем очередной заголовок ответа
					this->_answer.emplace_back(field.name, field.value, field.sensitive);
				// Устанавливаем функцию обратного вызова заголовка запроса
				this->_server.on(parser_http2_t::header_callback_t([](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
					// Учитываем разобранный заголовок запроса
					rival::account(name.size(), value.size());
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова заголовка ответа
				this->_client.on(parser_http2_t::header_callback_t([](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
					// Учитываем разобранный заголовок ответа
					rival::account(name.size(), value.size());
					// Продолжаем разбор
					return true;
				}));
				// Соединяем разборщики каналами записи: исходящие байты одного разбирает другой
				this->_client.on(parser_http2_t::write_callback_t([this](const void * buffer, const size_t size) noexcept {
					// Подаём исходящие байты клиента на разбор серверу
					this->_server.parse(buffer, size);
				}));
				// Устанавливаем функцию обратного вызова записи исходящих байт сервера
				this->_server.on(parser_http2_t::write_callback_t([this](const void * buffer, const size_t size) noexcept {
					// Подаём исходящие байты сервера на разбор клиенту
					this->_client.parse(buffer, size);
				}));
				// Устанавливаем функцию обратного вызова провайдера заголовков сервера
				this->_server.on(parser_http2_t::provider_callback_t([this](const uint32_t sid, const provider_t * provider, const bool) noexcept -> bool {
					// Если получен провайдер запроса
					if(provider != nullptr){
						// Отправляем заголовки ответа
						this->_server.sendHeaders(sid, this->_answer, false);
						// Отправляем тело ответа с завершением потока
						this->_server.sendData(sid, rival::payload().data(), rival::payload().size(), true);
					}
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова провайдера заголовков клиента
				this->_client.on(parser_http2_t::provider_callback_t([this](const uint32_t, const provider_t * provider, const bool) noexcept -> bool {
					// Если получен провайдер ответа - считаем завершённый обмен
					if(provider != nullptr)
						// Считаем завершённый обмен
						this->_completed++;
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова тела сообщения клиента
				this->_client.on(parser_http2_t::data_callback_t([](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
					// Выполняем потребление фрагмента тела ответа
					rival::consume(buffer, size);
					// Продолжаем разбор
					return true;
				}));
				// Отправляем преамбулу соединения клиента
				this->_client.sendPreface();
				// Отправляем преамбулу соединения сервера
				this->_server.sendPreface();
			}
	};
};

/**
 * Подключаем общий набор сценариев эталонных стендов
 */
#include "scenarios.hpp"
