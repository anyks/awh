/**
 * @file: awh.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения протокола HTTP/3 библиотеки AWH — та же
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
#include <proto/http/parser/http3/http.hpp>

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
	 * @brief Класс сжатия полей реализацией библиотеки AWH
	 *
	 */
	class Codec {
		private:
			// Объект кодера полей
			h3::qpack::encoder_t _encoder;
			// Объект декодера полей
			h3::qpack::decoder_t _decoder;
		private:
			// Буфер закодированной секции полей
			std::string _section;
			// Декодированные поля секции
			std::vector <h3::qpack::field_view_t> _fields;
			// Наборы полей в представлении сравниваемой реализации
			std::vector <std::vector <h3::qpack::field_t>> _sets;
		public:
			/**
			 * @brief Метод сброса состояния кодека
			 *
			 */
			void restart() noexcept {
				// Выполняем сброс состояния кодера полей
				this->_encoder.clear();
				// Выполняем сброс состояния декодера полей
				this->_decoder.clear();
				/**
				 * Ёмкость таблицы задаётся методом, а не конструктором: только метод
				 * ставит в поток кодера инструкцию изменения ёмкости, без которой
				 * таблица декодера пира осталась бы нулевой
				 */
				this->_encoder.maxCapacity(rival::TABLE_CAPACITY);
				// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
				this->_encoder.maxBlocked(rival::BLOCKED_STREAMS);
				/**
				 * Автоматическое определение чувствительных полей выключается: признак
				 * задан нагрузкой, и иначе сравнивалась бы не скорость кодирования,
				 * а осторожность каждого кодера в отношении печенья
				 */
				this->_encoder.sensitiveHeuristic(false);
			}
			/**
			 * @brief Метод перевода наборов полей в представление реализации
			 *
			 * @param sets наборы полей эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние наборы полей
				this->_sets.clear();
				// Резервируем память под наборы полей
				this->_sets.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов полей нагрузки
				 */
				for(const auto & set : sets){
					// Набор полей в представлении сравниваемой реализации
					std::vector <h3::qpack::field_t> fields;
					// Резервируем память под набор полей
					fields.reserve(set.size());
					/**
					 * Выполняем перебор всех полей набора
					 */
					for(const auto & field : set)
						// Дописываем очередное поле набора
						fields.emplace_back(field.name, field.value, field.sensitive);
					// Дописываем сформированный набор полей
					this->_sets.push_back(::std::move(fields));
				}
			}
			/**
			 * @brief Метод кодирования секции полей
			 *
			 * @param index        номер набора полей
			 * @param sid          идентификатор потока секции
			 * @param instructions объём инструкций потока кодера
			 * @return             объём закодированной секции
			 *
			 */
			size_t encode(const size_t index, const uint64_t sid, size_t & instructions) noexcept {
				// Очищаем буфер закодированной секции
				this->_section.clear();
				// Кодируем секцию полей
				this->_encoder.encode(sid, this->_sets[index], this->_section, true);
				// Запоминаем объём выставленных кодером инструкций
				instructions = this->_encoder.pending().size();
				// Освобождаем выставленные кодером инструкции
				this->_encoder.consumePending(instructions);
				// Выводим объём закодированной секции
				return this->_section.size();
			}
			/**
			 * @brief Метод подтверждения отправленной секции полей
			 *
			 * @param sid идентификатор потока секции
			 *
			 */
			void acknowledge(const uint64_t sid) noexcept {
				// Собираем подтверждение отправленной секции
				const std::string confirmation = rival::acknowledge(sid);
				// Количество разобранных октетов подтверждения
				size_t consumed = 0;
				// Код ошибки разбора подтверждения
				h3::error_t error = h3::error_t::H3_NO_ERROR;
				// Подаём подтверждение секции кодеру
				this->_encoder.decodeDecoderStream(confirmation, consumed, error);
			}
			/**
			 * @brief Метод декодирования секции полей
			 *
			 * @param item закодированная секция канонического потока
			 * @return     результат декодирования
			 *
			 */
			bool decode(const rival::encoded_t & item) noexcept {
				// Код ошибки декодирования
				h3::error_t error = h3::error_t::H3_NO_ERROR;
				// Если инструкции потока кодера есть
				if(!item.instructions.empty()){
					// Количество разобранных октетов инструкций
					size_t consumed = 0;
					// Если инструкции потока кодера разобраны с ошибкой
					if(this->_decoder.decodeEncoderStream(item.instructions, consumed, error) != h3::status_t::OK)
						// Выводим отрицательный результат
						return false;
				}
				// Если секция полей декодирована с ошибкой
				if(this->_decoder.decode(item.sid, item.section, this->_fields, 0, error) != h3::status_t::OK)
					// Выводим отрицательный результат
					return false;
				// Освобождаем выставленные декодером инструкции
				this->_decoder.consumePending(this->_decoder.pending().size());
				/**
				 * Выполняем учёт всех декодированных полей секции
				 */
				for(const auto & field : this->_fields)
					// Учитываем разобранное поле секции
					rival::account(field.name.size(), field.value.size());
				// Выводим положительный результат
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Codec() noexcept : _decoder(rival::TABLE_CAPACITY, rival::BLOCKED_STREAMS) {
				// Выполняем приведение состояния кодека к исходному
				this->restart();
			}
	};
	/**
	 * @brief Класс разбора входящего потока реализацией библиотеки AWH
	 *
	 */
	class Server {
		private:
			// Объект разборщика сервера
			parser_http3_t _parser;
		private:
			// Идентификатор следующего выдаваемого однонаправленного потока
			uint64_t _unistream;
		private:
			// Количество разобранных запросов
			size_t _handled;
			// Объём принятого тела
			size_t _accepted;
		private:
			// Поля ответа сервера, подготовленные заранее
			std::vector <h3::qpack::field_t> _answer;
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
			 * @brief Метод подачи порции октетов потока на разбор
			 *
			 * @param sid    идентификатор потока
			 * @param buffer буфер порции октетов
			 * @param size   размер порции октетов
			 * @param fin    признак завершения потока
			 *
			 */
			void feed(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
				// Подаём порцию октетов потока на разбор
				this->_parser.parse(sid, buffer, size, fin);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param requests признак сценария разбора потока запросов
			 *
			 */
			explicit Server(const bool requests) noexcept :
			 _parser(direct_t::REQUEST, ::fmk(), ::logger()), _unistream(3), _handled(0), _accepted(0) {
				// Устанавливаем функцию обратного вызова открытия однонаправленного потока
				this->_parser.on(parser_http3_t::open_callback_t([this]() noexcept -> int64_t {
					// Выделяем идентификатор однонаправленного потока
					const int64_t sid = static_cast <int64_t> (this->_unistream);
					// Продвигаем идентификатор следующего однонаправленного потока
					this->_unistream += 4;
					// Выводим идентификатор открытого потока
					return sid;
				}));
				// Устанавливаем функцию обратного вызова записи исходящих октетов потока
				this->_parser.on(parser_http3_t::write_callback_t([](const uint64_t, const void *, const size_t, const bool) noexcept {
					// Исходящие октеты стенду не нужны: измеряется разбор входящего потока
				}));
				// Устанавливаем функцию обратного вызова поля секции
				this->_parser.on(parser_http3_t::header_callback_t([](const uint64_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
					// Учитываем разобранное поле запроса
					rival::account(name.size(), value.size());
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова фрагмента тела потока
				this->_parser.on(parser_http3_t::data_callback_t([this](const uint64_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
					// Суммируем объём принятого тела
					this->_accepted += size;
					// Выполняем потребление фрагмента тела
					rival::consume(buffer, size);
					// Продолжаем разбор
					return true;
				}));
				// Если выполняется сценарий разбора потока запросов
				if(requests){
					// Формируем поля минимального ответа сервера
					this->_answer.emplace_back(":status", "200");
					// Дописываем поле длины содержимого
					this->_answer.emplace_back("content-length", "0");
					/**
					 * Устанавливаем функцию обратного вызова провайдера полей. Сервер обязан
					 * ответить: без ответа поток остаётся полуоткрытым, такие потоки копятся
					 * и упираются в лимит одновременных потоков - ровно как у настоящего сервера
					 */
					this->_parser.on(parser_http3_t::provider_callback_t([this](const uint64_t sid, const provider_t * provider, const bool) noexcept -> bool {
						// Если получен провайдер запроса
						if(provider != nullptr){
							// Считаем разобранный запрос
							this->_handled++;
							// Отправляем минимальный ответ с завершением потока
							this->_parser.sendHeaders(sid, this->_answer, true);
						}
						// Продолжаем разбор
						return true;
					}));
				}
				// Отправляем параметры соединения сервера
				this->_parser.sendSettings();
			}
	};
	/**
	 * @brief Класс полного обмена парой реализаций библиотеки AWH
	 *
	 */
	class Pair {
		private:
			// Объект разборщика клиента
			parser_http3_t _client;
			// Объект разборщика сервера
			parser_http3_t _server;
		private:
			// Идентификатор следующего однонаправленного потока клиента
			uint64_t _clientUni;
			// Идентификатор следующего однонаправленного потока сервера
			uint64_t _serverUni;
			// Идентификатор следующего двунаправленного потока клиента
			uint64_t _bidi;
		private:
			// Количество завершённых обменов
			size_t _completed;
		private:
			// Поля ответа сервера, подготовленные заранее
			std::vector <h3::qpack::field_t> _answer;
			// Наборы полей запроса в представлении сравниваемой реализации
			std::vector <std::vector <h3::qpack::field_t>> _sets;
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
			 * @details Реализация отдаёт исходящие октеты функцией обратного вызова
			 *          сразу при отправке, поэтому прокачивать нечего
			 *
			 */
			void pump() noexcept {}
			/**
			 * @brief Метод перевода наборов полей в представление реализации
			 *
			 * @param sets наборы полей эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние наборы полей
				this->_sets.clear();
				// Резервируем память под наборы полей
				this->_sets.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов полей нагрузки
				 */
				for(const auto & set : sets){
					// Набор полей в представлении сравниваемой реализации
					std::vector <h3::qpack::field_t> fields;
					// Резервируем память под набор полей
					fields.reserve(set.size());
					/**
					 * Выполняем перебор всех полей набора
					 */
					for(const auto & field : set)
						// Дописываем очередное поле набора
						fields.emplace_back(field.name, field.value, field.sensitive);
					// Дописываем сформированный набор полей
					this->_sets.push_back(::std::move(fields));
				}
			}
			/**
			 * @brief Метод открытия потока с отправкой запроса
			 *
			 * @param index номер набора полей запроса
			 * @return      результат открытия потока
			 *
			 */
			bool open(const size_t index) noexcept {
				// Выделяем идентификатор нового двунаправленного потока клиента
				const uint64_t sid = this->_bidi;
				// Продвигаем идентификатор следующего двунаправленного потока
				this->_bidi += 4;
				// Отправляем секцию полей запроса с завершением потока
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
			 _client(direct_t::RESPONSE, ::fmk(), ::logger()), _server(direct_t::REQUEST, ::fmk(), ::logger()),
			 _clientUni(2), _serverUni(3), _bidi(0), _completed(0) {
				// Формируем поля ответа сервера
				for(const auto & field : rival::response(0))
					// Дописываем очередное поле ответа
					this->_answer.emplace_back(field.name, field.value, field.sensitive);
				// Устанавливаем функцию обратного вызова открытия однонаправленного потока клиента
				this->_client.on(parser_http3_t::open_callback_t([this]() noexcept -> int64_t {
					// Выделяем идентификатор однонаправленного потока клиента
					const int64_t sid = static_cast <int64_t> (this->_clientUni);
					// Продвигаем идентификатор следующего однонаправленного потока
					this->_clientUni += 4;
					// Выводим идентификатор открытого потока
					return sid;
				}));
				// Устанавливаем функцию обратного вызова открытия однонаправленного потока сервера
				this->_server.on(parser_http3_t::open_callback_t([this]() noexcept -> int64_t {
					// Выделяем идентификатор однонаправленного потока сервера
					const int64_t sid = static_cast <int64_t> (this->_serverUni);
					// Продвигаем идентификатор следующего однонаправленного потока
					this->_serverUni += 4;
					// Выводим идентификатор открытого потока
					return sid;
				}));
				// Соединяем разборщики каналами записи: исходящие октеты одного разбирает другой
				this->_client.on(parser_http3_t::write_callback_t([this](const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
					// Подаём исходящие октеты клиента на разбор серверу
					this->_server.parse(sid, buffer, size, fin);
				}));
				// Устанавливаем функцию обратного вызова записи исходящих октетов сервера
				this->_server.on(parser_http3_t::write_callback_t([this](const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
					// Подаём исходящие октеты сервера на разбор клиенту
					this->_client.parse(sid, buffer, size, fin);
				}));
				// Устанавливаем функцию обратного вызова поля запроса
				this->_server.on(parser_http3_t::header_callback_t([](const uint64_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
					// Учитываем разобранное поле запроса
					rival::account(name.size(), value.size());
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова поля ответа
				this->_client.on(parser_http3_t::header_callback_t([](const uint64_t, const std::string_view name, const std::string_view value, const parser_t::part_t) noexcept -> bool {
					// Учитываем разобранное поле ответа
					rival::account(name.size(), value.size());
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова провайдера полей сервера
				this->_server.on(parser_http3_t::provider_callback_t([this](const uint64_t sid, const provider_t * provider, const bool) noexcept -> bool {
					// Если получен провайдер запроса
					if(provider != nullptr){
						// Отправляем секцию полей ответа
						this->_server.sendHeaders(sid, this->_answer, false);
						// Отправляем тело ответа с завершением потока
						this->_server.sendData(sid, rival::payload().data(), rival::payload().size(), true);
					}
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова провайдера полей клиента
				this->_client.on(parser_http3_t::provider_callback_t([this](const uint64_t, const provider_t * provider, const bool) noexcept -> bool {
					// Если получен провайдер ответа - считаем завершённый обмен
					if(provider != nullptr)
						// Считаем завершённый обмен
						this->_completed++;
					// Продолжаем разбор
					return true;
				}));
				// Устанавливаем функцию обратного вызова тела сообщения клиента
				this->_client.on(parser_http3_t::data_callback_t([](const uint64_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
					// Выполняем потребление фрагмента тела ответа
					rival::consume(buffer, size);
					// Продолжаем разбор
					return true;
				}));
				// Отправляем параметры соединения клиента
				this->_client.sendSettings();
				// Отправляем параметры соединения сервера
				this->_server.sendSettings();
			}
	};
};

/**
 * Подключаем общий набор сценариев эталонных стендов
 */
#include "scenarios.hpp"
