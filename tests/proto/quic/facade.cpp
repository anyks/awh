/**
 * @file facade.cpp
 * @date 2026-07-28
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
 * @brief Интеграционные тесты фасадов QUIC клиента и сервера — проверка сквозного
 *        обмена данными поверх реальных UDP-сокетов в одном процессе (loopback).
 *
 * @details База событий едина на весь процесс: цикл событий запускает и блокирует
 *          ровно один юнит-лаунчер. Поэтому сервер поднимается лаунчером в фоновом
 *          потоке, а клиент стартует и подключается уже из серверного статус-коллбэка
 *          (тот же поток цикла, до входа в poll) - так весь data-plane остаётся на
 *          одном потоке. Тестовый поток лишь ожидает завершения обмена (future) и по
 *          сторожевому таймауту будит цикл вызовом stop() (кросс-тред безопасно).
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <string>
#include <thread>

/**
 * Подключаем заголовочный файл тестовой фикстуры протокола QUIC (переиспользуем окружение безопасности)
 */
#include "quic.hpp"

/**
 * Подключаем заголовочные файлы фасадов клиента и сервера
 */
#include "../../../include/client/client.hpp"
#include "../../../include/server/server.hpp"

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён заполнителей связывания функций
 */
using namespace std::placeholders;

/**
 * @brief Внутренние средства интеграционного окружения фасадов QUIC
 *
 */
namespace {
	/**
	 * @brief Разновидность сценария сквозного обмена
	 *
	 */
	enum class scenario_t : uint8_t {
		ECHO         = 0x01, // Эхо потока: клиент шлёт нагрузку с FIN, сервер возвращает её обратно
		BACKPRESSURE = 0x02, // Обратное давление: малые водяные метки, частичный приём, сигнал writable
		DATA_SOURCE  = 0x03, // Источник данных: движок сам тянет тело потока у источника клиента
		DATAGRAM     = 0x04  // Датаграмма приложения (RFC 9221): ненадёжная доставка вне потока
	};
	/**
	 * @brief Разновидность завершения соединения после обмена
	 *
	 */
	enum class teardown_t : uint8_t {
		NONE         = 0x00, // Обмен завершается штатной остановкой цикла (без явного закрытия соединения)
		CLIENT_CLOSE = 0x01, // Клиент закрывает соединение close(); проверяется disconnect на сервере
		SERVER_CLOSE = 0x02  // Сервер закрывает соединение close(cid); проверяется disconnect на клиенте
	};
	/**
	 * @brief Конфигурация прогона сквозного обмена
	 *
	 */
	struct config_t {
		// Разновидность сценария обмена
		scenario_t scenario = scenario_t::ECHO;
		// Разновидность завершения соединения после обмена
		teardown_t teardown = teardown_t::NONE;
		// Размер полезной нагрузки потока (октеты)
		size_t payload = 64;
		// Верхняя водяная метка буфера отправки клиента (0 - без ограничения)
		size_t sendHigh = 0;
		// Нижняя водяная метка буфера отправки клиента
		size_t sendLow = 0;
		// Таймаут простоя соединения в миллисекундах (низкий - для сценария неудачного подключения)
		uint32_t idleTimeout = 30000;
		// Смещение целевого порта клиента относительно порта сервера (ненулевое - подключение в пустоту)
		uint16_t targetPortOffset = 0;
		/**
		 * Уведомление о перегрузке пути (ECN): датаграммы уходят помеченными
		 *
		 * @note По умолчанию выключено, и тогда путь маркировки не проверяется вовсе.
		 *       Метка едет управляющими данными отправки, а ширина значения для IPv4
		 *       у систем разная: ошибись в ней - и отправка отвечает отказом либо
		 *       теряет метку молча (RFC 9000 §13.4)
		 */
		bool ecn = false;
	};
	/**
	 * @brief Итоги прогона сквозного обмена
	 *
	 */
	struct result_t {
		// Клиент установил соединение с сервером
		bool connected = false;
		// Обмен завершён штатно (не по сторожевому таймауту)
		bool completed = false;
		// Прогон прерван сторожевым таймаутом
		bool timedOut = false;
		// Цикл событий не откликнулся на остановку и был оставлен работать
		bool unstoppable = false;
		// Эхо-ответ побайтово совпал с отправленной нагрузкой
		bool streamEchoMatched = false;
		// Возвращённое клиентским send() число октетов первой отправки
		size_t firstSendReturn = 0;
		// Сигнал writable сработал на клиенте (буфер отправки освободился)
		bool writableFired = false;
		// Датаграмма-эхо побайтово совпала с отправленной
		bool datagramMatched = false;
		// Сервер получил флаг завершения потока (FIN)
		bool serverFin = false;
		// Число октетов, принятых сервером в потоке
		size_t serverBytes = 0;
		// Принятые сервером данные потока совпали с ожидаемым образцом
		bool serverPatternOk = true;
		// Клиент получил оповещение о завершении соединения (disconnect)
		bool clientDisconnectFired = false;
		// Сервер получил оповещение о завершении соединения (disconnect)
		bool serverDisconnectFired = false;
	};
	// Предел ожидания отклика цикла событий на остановку в секундах
	static constexpr uint32_t SHUTDOWN_TIMEOUT = 5;

	/**
	 * @brief Метод доступа к хранилищу неостановленных фасадов
	 *
	 * @details Фасад, чей цикл событий не откликнулся на остановку, разрушать нельзя:
	 *          его поток продолжает обращаться к полям объекта. Такие фасады
	 *          складываются сюда и живут до конца работы процесса.
	 *
	 * @note Хранилище намеренно не опустошается: это не утечка по недосмотру, а
	 *       единственный безопасный исход, когда поток остановить не удалось
	 *
	 * @return хранилище фасадов, оставленных работать
	 *
	 */
	static std::vector <std::pair <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::client_t>>> & abandoned() noexcept {
		// Хранилище фасадов, оставленных работать до конца работы процесса
		static std::vector <std::pair <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::client_t>>> result;
		// Выводим хранилище фасадов
		return result;
	}

	/**
	 * @brief Класс интеграционного окружения фасадов QUIC (loopback клиент ↔ сервер)
	 *
	 */
	class Harness {
		private:
			// Объект фреймворка
			awh::fmk_t * _fmk;
			// Объект для работы с логами
			awh::log_t * _log;
			// Тестовое окружение транспортной безопасности (общий самоподписанный сертификат)
			QuicSecurity * _security;
		private:
			// Конфигурация прогона
			config_t _config;
			// Итоги прогона
			result_t _result;
		private:
			// Порт прослушивания сервера на локальной петле
			uint16_t _port;
		private:
			// Фасад сервера QUIC
			std::unique_ptr <awh::server_t> _server;
			// Фасад клиента QUIC
			std::unique_ptr <awh::client_t> _client;
		private:
			// Идентификатор потока приложения клиента
			uint64_t _sid;
			// Смещение отправки/выдачи нагрузки клиентом (обратное давление и источник данных)
			size_t _offset;
			// Смещение сверки принятого сервером образца (сценарий источника данных)
			size_t _recvOffset;
		private:
			// Обмен потоком завершён (эхо потока принято, сценарий датаграммы)
			bool _streamDone;
			// Обмен датаграммой завершён (эхо датаграммы принято, сценарий датаграммы)
			bool _datagramDone;
			// Отправленная клиентом нагрузка (эталон для сверки эхо-ответа)
			std::string _payload;
			// Накопленный клиентом эхо-ответ потока
			std::string _received;
			// Отправленная клиентом датаграмма (эталон для сверки эхо-ответа)
			std::string _datagram;
		private:
			// Флаг однократной остановки цикла (защита от повторного stop)
			std::atomic <bool> _finishing;
		private:
			/**
			 * @brief Метод формирования детерминированной нагрузки заданного размера
			 *
			 * @param size размер нагрузки
			 * @return     сформированная нагрузка
			 *
			 */
			std::string makePayload(const size_t size) const noexcept {
				// Результирующая нагрузка
				std::string result;
				// Резервируем память под нагрузку
				result.resize(size);
				// Заполняем нагрузку детерминированным образцом
				for(size_t i = 0; i < size; i++)
					// Записываем октет образца
					result[i] = static_cast <char> ((i * 31 + 7) & 0xFF);
				// Выводим сформированную нагрузку
				return result;
			}
			/**
			 * @brief Метод завершения обмена и остановки цикла событий
			 *
			 * @note Останавливает цикл лаунчер-сервер: вызывается из коллбэка на потоке
			 *       цикла, поэтому просто помечает обмен завершённым и будит базу через
			 *       stop(). Защищён от повторного вызова атомарным флагом
			 *
			 */
			void finish() noexcept {
				// Ожидаемое состояние флага завершения (обмен ещё не завершён)
				bool expected = false;
				// Атомарно захватываем однократное завершение обмена
				if(this->_finishing.compare_exchange_strong(expected, true)){
					// Помечаем обмен завершённым штатно
					this->_result.completed = true;
					// Останавливаем работу цикла событий сервера-лаунчера
					this->_server->stop();
				}
			}
			/**
			 * @brief Метод завершения обмена сценария датаграммы по готовности обоих обменов
			 *
			 * @note Обмен потоком (надёжный) и датаграммой (ненадёжный) идут параллельно,
			 *       поэтому цикл останавливается лишь когда завершены оба
			 *
			 */
			void maybeFinish() noexcept {
				// Если завершены обмен потоком и обмен датаграммой
				if(this->_streamDone && this->_datagramDone)
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса сервера
			 *
			 * @param status новый статус сервера
			 *
			 */
			void serverStatus(const event::status_t status) noexcept {
				// Если сервер перешёл в рабочее состояние (лаунчер запустил цикл событий)
				if(status == event::status_t::LAUNCHED){
					// Переводим сервер в режим прослушивания входящих соединений
					this->_server->listen(100);
					// Поднимаем клиента: старт не блокирует (лаунчер уже сервер), статус-коллбэк клиента инициирует подключение
					this->_client->start();
				}
			}
			/**
			 * @brief Метод обработки собранных данных потока приложения на стороне сервера
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param sid  идентификатор потока приложения
			 * @param data собранные данные потока
			 * @param fin  флаг завершения потока
			 *
			 */
			void serverStream(const event::id_t cid, const uint64_t sid, const std::string & data, const bool fin) noexcept {
				// Накапливаем число принятых сервером октетов потока
				this->_result.serverBytes += data.size();
				// Запоминаем получение сервером флага завершения потока
				if(fin)
					// Устанавливаем флаг завершения потока
					this->_result.serverFin = true;
				/**
				 * Определяем сценарий обмена
				 */
				switch(static_cast <uint8_t> (this->_config.scenario)){
					// Для сценариев эхо и датаграммы возвращаем данные потока обратно клиенту
					case static_cast <uint8_t> (scenario_t::ECHO):
					case static_cast <uint8_t> (scenario_t::DATAGRAM):
						// Отправляем принятые данные обратно клиенту тем же потоком с сохранением флага завершения
						this->_server->send(cid, sid, data.data(), data.size(), fin);
						// Для сценария закрытия сервером: после эха с FIN закрываем соединение
						if(fin && (this->_config.teardown == teardown_t::SERVER_CLOSE))
							// Закрываем соединение с клиентом (disconnect ждём на клиенте)
							this->_server->close(cid);
					break;
					// Для сценария обратного давления сервер лишь считает объём и завершает по достижению нагрузки
					case static_cast <uint8_t> (scenario_t::BACKPRESSURE): {
						// Если сервер принял всю ожидаемую нагрузку - завершаем обмен
						if(this->_result.serverBytes >= this->_config.payload)
							// Завершаем обмен и останавливаем цикл событий
							this->finish();
					} break;
					// Для сценария источника данных сверяем принятый образец и завершаем по FIN
					case static_cast <uint8_t> (scenario_t::DATA_SOURCE): {
						// Сверяем принятый образец с ожидаемым (нагрузка формируется тем же генератором)
						for(size_t i = 0; i < data.size(); i++){
							// Ожидаемый октет образца по абсолютному смещению принятого сервером тела
							const char expected = static_cast <char> (((this->_recvOffset + i) * 31 + 7) & 0xFF);
							// Если принятый октет не совпал с ожидаемым
							if(data[i] != expected){
								// Помечаем несовпадение образца
								this->_result.serverPatternOk = false;
								// Прерываем сверку
								break;
							}
						}
						// Сдвигаем абсолютное смещение принятого сервером образца
						this->_recvOffset += data.size();
						// Если поток завершён удалённым клиентом - завершаем обмен
						if(fin)
							// Завершаем обмен и останавливаем цикл событий
							this->finish();
					} break;
				}
			}
			/**
			 * @brief Метод обработки принятой сервером датаграммы приложения
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param data данные датаграммы приложения
			 *
			 */
			void serverDatagram(const event::id_t cid, const std::string & data) noexcept {
				// Отправляем датаграмму обратно клиенту (эхо)
				this->_server->datagram(cid, data.data(), data.size());
			}
			/**
			 * @brief Метод обработки завершения соединения на стороне сервера
			 *
			 * @param cid   идентификатор сессии соединения
			 * @param error код завершения соединения
			 *
			 */
			void serverDisconnect([[maybe_unused]] const event::id_t cid, [[maybe_unused]] const quic::error_t error) noexcept {
				// Запоминаем оповещение сервера о завершении соединения
				this->_result.serverDisconnectFired = true;
				// Для сценария закрытия клиентом обмен завершается по этому оповещению
				if(this->_config.teardown == teardown_t::CLIENT_CLOSE)
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса клиента
			 *
			 * @param status новый статус клиента
			 *
			 */
			void clientStatus(const event::status_t status) noexcept {
				// Если клиент перешёл в рабочее состояние - подключаемся к серверу
				if(status == event::status_t::LAUNCHED)
					// Выполняем подключение клиента к удалённому серверу
					this->_client->connect();
			}
			/**
			 * @brief Метод обработки подключения клиента к серверу
			 *
			 * @param ok результат подключения к серверу
			 *
			 */
			void clientConnect(const bool ok) noexcept {
				// Запоминаем результат установки соединения
				this->_result.connected = ok;
				// Если подключение к серверу не выполнено - завершаем обмен
				if(!ok){
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
					// Выходим из метода
					return;
				}
				// Открываем двунаправленный поток приложения для обмена нагрузкой
				this->_sid = this->_client->open(false);
				// Если поток приложения открыть не удалось - завершаем обмен
				if(this->_sid == awh::quic::connection_t::INVALID_STREAM){
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
					// Выходим из метода
					return;
				}
				/**
				 * Определяем сценарий обмена
				 */
				switch(static_cast <uint8_t> (this->_config.scenario)){
					// Сценарий простого эхо: отправляем всю нагрузку одним потоком с флагом завершения
					case static_cast <uint8_t> (scenario_t::ECHO):
						// Отправляем нагрузку серверу и запоминаем принятое число октетов
						this->_result.firstSendReturn = this->_client->send(this->_sid, this->_payload.data(), this->_payload.size(), true);
					break;
					// Сценарий обратного давления: включаем водяные метки и шлём крупную нагрузку одним заходом
					case static_cast <uint8_t> (scenario_t::BACKPRESSURE): {
						// Устанавливаем водяные метки буфера отправки потоков клиента
						this->_client->sendWaterMarks(this->_config.sendHigh, this->_config.sendLow);
						// Отправляем всю нагрузку одним заходом без флага завершения: сверх верхней метки приём частичный
						this->_result.firstSendReturn = this->_client->send(this->_sid, this->_payload.data(), this->_payload.size(), false);
						// Сдвигаем смещение отправленной нагрузки на принятое число октетов
						this->_offset = this->_result.firstSendReturn;
					} break;
					// Сценарий источника данных: движок сам тянет тело потока у источника по мере освобождения места
					case static_cast <uint8_t> (scenario_t::DATA_SOURCE): {
						// Полный объём тела потока источника
						const size_t total = this->_config.payload;
						// Регистрируем источник данных потока: движок вызывает его по мере освобождения места
						this->_client->dataSource(this->_sid, [this, total](const uint64_t, uint8_t * buffer, const size_t capacity, bool & eof) noexcept -> int64_t {
							// Оставшийся объём тела потока источника
							const size_t remaining = (total - this->_offset);
							// Число октетов к выдаче за текущий вызов
							const size_t n = ((capacity < remaining) ? capacity : remaining);
							// Заполняем буфер детерминированным образцом по абсолютному смещению
							for(size_t i = 0; i < n; i++)
								// Записываем октет образца
								buffer[i] = static_cast <uint8_t> (((this->_offset + i) * 31 + 7) & 0xFF);
							// Сдвигаем смещение выданного тела потока
							this->_offset += n;
							// Если тело потока выдано полностью - сигнализируем конец данных
							if(this->_offset >= total)
								// Устанавливаем флаг конца данных
								eof = true;
							// Выводим число выданных октетов
							return static_cast <int64_t> (n);
						});
					} break;
					// Сценарий датаграммы: отправляем поток с FIN и датаграмму приложения
					case static_cast <uint8_t> (scenario_t::DATAGRAM): {
						// Отправляем нагрузку потоком с флагом завершения (сервер вернёт её эхом)
						this->_result.firstSendReturn = this->_client->send(this->_sid, this->_payload.data(), this->_payload.size(), true);
						// Получаем согласованный хендшейком предельный размер отправляемой датаграммы
						const size_t limit = this->_client->datagrams();
						// Если удалённый сервер поддерживает датаграммы и нагрузка помещается в предел
						if((limit > 0) && (this->_datagram.size() <= limit))
							// Отправляем датаграмму приложения серверу
							this->_client->datagram(this->_datagram.data(), this->_datagram.size());
						// Если датаграммы не поддерживаются - помечаем обмен датаграммой завершённым (эхо не ждём)
						else this->_datagramDone = true;
					} break;
				}
			}
			/**
			 * @brief Метод обработки собранных данных потока приложения на стороне клиента (эхо-ответ)
			 *
			 * @param sid  идентификатор потока приложения
			 * @param data собранные данные потока
			 * @param fin  флаг завершения потока
			 *
			 */
			void clientStream([[maybe_unused]] const uint64_t sid, const std::string & data, const bool fin) noexcept {
				// Накапливаем принятый от сервера эхо-ответ (поток может прийти частями)
				this->_received.append(data);
				// Пока поток не завершён удалённым сервером - ждём остальные части
				if(!fin)
					// Выходим из метода
					return;
				// Сверяем принятый эхо-ответ с отправленной нагрузкой побайтово
				this->_result.streamEchoMatched = (this->_received == this->_payload);
				/**
				 * Для сценария датаграммы обмен потоком и датаграммой идёт параллельно:
				 * поток надёжный, датаграмма - нет, поэтому завершаем обмен лишь когда
				 * готовы оба (иначе гонка - эхо датаграммы может опередить эхо потока)
				 */
				if(this->_config.scenario == scenario_t::DATAGRAM){
					// Помечаем обмен потоком завершённым
					this->_streamDone = true;
					// Завершаем обмен, если завершён и обмен датаграммой
					this->maybeFinish();
					// Выходим из метода
					return;
				}
				// Для сценария закрытия клиентом: инициируем закрытие соединения (disconnect ждём на сервере)
				if(this->_config.teardown == teardown_t::CLIENT_CLOSE){
					// Закрываем соединение с сервером
					this->_client->close();
					// Выходим из метода - обмен завершится по оповещению disconnect на сервере
					return;
				}
				// Для сценария закрытия сервером ждём оповещения disconnect на клиенте
				if(this->_config.teardown == teardown_t::SERVER_CLOSE)
					// Выходим из метода - обмен завершится по оповещению disconnect на клиенте
					return;
				// Завершаем обмен и останавливаем цикл событий
				this->finish();
			}
			/**
			 * @brief Метод обработки освобождения буфера отправки потока клиента (сигнал writable)
			 *
			 * @param sid идентификатор потока приложения
			 *
			 */
			void clientWritable([[maybe_unused]] const uint64_t sid) noexcept {
				// Запоминаем срабатывание сигнала готовности буфера отправки
				this->_result.writableFired = true;
				// Сигнал writable значим лишь для сценария обратного давления
				if(this->_config.scenario != scenario_t::BACKPRESSURE)
					// Выходим из метода
					return;
				// Оставшийся к отправке объём нагрузки
				const size_t remaining = (this->_payload.size() - this->_offset);
				// Если нагрузка отправлена полностью - ждать нечего
				if(remaining == 0)
					// Выходим из метода
					return;
				// Досылаем оставшуюся нагрузку без флага завершения: приём вновь может быть частичным
				const size_t accepted = this->_client->send(this->_sid, this->_payload.data() + this->_offset, remaining, false);
				// Сдвигаем смещение отправленной нагрузки на принятое число октетов
				this->_offset += accepted;
			}
			/**
			 * @brief Метод обработки принятой клиентом датаграммы приложения (эхо-ответ)
			 *
			 * @param data данные принятой датаграммы
			 *
			 */
			void clientDatagram(const std::string & data) noexcept {
				// Сверяем принятую датаграмму-эхо с отправленной побайтово
				this->_result.datagramMatched = (data == this->_datagram);
				// Помечаем обмен датаграммой завершённым
				this->_datagramDone = true;
				// Завершаем обмен, если завершён и обмен потоком
				this->maybeFinish();
			}
			/**
			 * @brief Метод обработки завершения соединения на стороне клиента
			 *
			 * @param error код завершения соединения
			 *
			 */
			void clientDisconnect([[maybe_unused]] const quic::error_t error) noexcept {
				// Запоминаем оповещение клиента о завершении соединения
				this->_result.clientDisconnectFired = true;
				// Обмен завершается по этому оповещению для закрытия сервером и для неудачного подключения
				if((this->_config.teardown == teardown_t::SERVER_CLOSE) || !this->_result.connected)
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
			}
		private:
			/**
			 * @brief Метод подготовки локальных транспортных параметров соединения QUIC
			 *
			 * @return сформированные транспортные параметры
			 *
			 */
			awh::quic::params::params_t makeParams() const noexcept {
				// Локальные транспортные параметры соединения QUIC
				awh::quic::params::params_t params;
				// Устанавливаем таймаут простоя соединения в миллисекундах
				params.maxIdleTimeout = this->_config.idleTimeout;
				// Устанавливаем лимит данных соединения
				params.initialMaxData = 1048576;
				// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
				params.initialMaxStreamDataBidiLocal = 262144;
				// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
				params.initialMaxStreamDataBidiRemote = 262144;
				// Устанавливаем лимит данных однонаправленных потоков
				params.initialMaxStreamDataUni = 262144;
				// Устанавливаем лимит числа двунаправленных потоков
				params.initialMaxStreamsBidi = 100;
				// Устанавливаем лимит числа однонаправленных потоков
				params.initialMaxStreamsUni = 100;
				// Устанавливаем предельный размер принимаемой датаграммы приложения
				params.maxDatagramFrameSize = 1200;
				// Выводим сформированные транспортные параметры
				return params;
			}
		public:
			/**
			 * @brief Метод выполнения прогона сквозного обмена
			 *
			 * @param timeout сторожевой таймаут прогона в миллисекундах
			 * @return        итоги прогона
			 *
			 */
			result_t execute(const uint32_t timeout) noexcept {
				// Формируем нагрузку потока заданного размера
				this->_payload = this->makePayload(this->_config.payload);
				// Формируем нагрузку датаграммы приложения
				this->_datagram = std::string("AWH QUIC facade datagram echo probe");
				// Локальные транспортные параметры соединения
				const awh::quic::params::params_t params = this->makeParams();
				// Создаём фасад сервера QUIC на серверном шаблоне контекста безопасности
				this->_server = std::make_unique <awh::server_t> (this->_security->context(awh::quic::endpoint_t::SERVER), &this->_security->coder(), this->_fmk, this->_log);
				// Создаём событие сервера транспорта QUIC поверх UDP
				this->_server->init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::QUIC);
				// Устанавливаем локальные транспортные параметры соединений сервера
				this->_server->params(params);
				// Устанавливаем режим уведомления о перегрузке пути сервера
				this->_server->ecn(this->_config.ecn);
				// Устанавливаем хост сервера на локальной петле
				this->_server->setHost("127.0.0.1");
				// Устанавливаем порт прослушивания сервера
				this->_server->setPort(this->_port);
				// Регистрируем коллбэк изменения статуса сервера
				this->_server->on <void (const event::status_t)> ("status", &Harness::serverStatus, this, _1);
				// Регистрируем коллбэк собранных данных потока приложения сервера
				this->_server->on <void (const event::id_t, const uint64_t, const std::string &, const bool)> ("stream", &Harness::serverStream, this, _1, _2, _3, _4);
				// Регистрируем коллбэк принятой сервером датаграммы приложения
				this->_server->on <void (const event::id_t, const std::string &)> ("datagram", &Harness::serverDatagram, this, _1, _2);
				// Регистрируем коллбэк завершения соединения на стороне сервера
				this->_server->on <void (const event::id_t, const quic::error_t)> ("disconnect", &Harness::serverDisconnect, this, _1, _2);
				// Создаём фасад клиента QUIC на транспорте клиентского шаблона контекста безопасности
				this->_client = std::make_unique <awh::client_t> (this->_security->coder().transport(this->_security->context(awh::quic::endpoint_t::CLIENT)), &this->_security->coder(), this->_fmk, this->_log);
				// Создаём событие клиента транспорта QUIC поверх UDP
				this->_client->init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::QUIC);
				// Устанавливаем локальные транспортные параметры соединения клиента
				this->_client->params(params);
				// Устанавливаем режим уведомления о перегрузке пути клиента
				this->_client->ecn(this->_config.ecn);
				// Устанавливаем адрес удалённого сервера на локальной петле
				this->_client->setTarget("127.0.0.1");
				// Устанавливаем порт удалённого сервера
				this->_client->setTargetPort(this->_port + this->_config.targetPortOffset);
				// Регистрируем коллбэк изменения статуса клиента
				this->_client->on <void (const event::status_t)> ("status", &Harness::clientStatus, this, _1);
				// Регистрируем коллбэк подключения клиента к серверу
				this->_client->on <void (const bool)> ("connect", &Harness::clientConnect, this, _1);
				// Регистрируем коллбэк собранных данных потока приложения клиента
				this->_client->on <void (const uint64_t, const std::string &, const bool)> ("stream", &Harness::clientStream, this, _1, _2, _3);
				// Регистрируем коллбэк освобождения буфера отправки потока клиента
				this->_client->on <void (const uint64_t)> ("writable", &Harness::clientWritable, this, _1);
				// Регистрируем коллбэк принятой клиентом датаграммы приложения
				this->_client->on <void (const std::string &)> ("datagram", &Harness::clientDatagram, this, _1);
				// Регистрируем коллбэк завершения соединения на стороне клиента
				this->_client->on <void (const quic::error_t)> ("disconnect", &Harness::clientDisconnect, this, _1);
				// Обещание завершения работы фонового потока цикла событий
				std::promise <void> finished;
				// Ожидание завершения работы фонового потока цикла событий
				std::future <void> waiter = finished.get_future();
				// Запускаем сервер-лаунчер в фоновом потоке (start блокирует до остановки цикла)
				std::thread worker([this, &finished]() noexcept {
					// Запускаем цикл событий сервера (блокирует поток до вызова stop)
					this->_server->start();
					// Сигнализируем завершение работы фонового потока
					finished.set_value();
				});
				// Ожидаем завершения обмена в пределах сторожевого таймаута
				if(waiter.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::timeout){
					// Помечаем прерывание прогона сторожевым таймаутом
					this->_result.timedOut = true;
					// Будим цикл событий сервера-лаунчера для завершения фонового потока
					this->_server->stop();
					/**
					 * Ожидаем фактического завершения работы фонового потока, но не бесконечно
					 *
					 * @details Цикл событий может не откликнуться на остановку - так ведёт себя
					 *          движок epoll, когда его поток стоит в блокирующем приёме и
					 *          пробуждение до него не доходит. Безусловное ожидание превращало
					 *          бы сторожевой таймаут в вечное зависание всего набора: тест не
					 *          отказывал, а замирал, унося с собой и все следующие за ним.
					 *
					 * @note Не дождавшись, поток и фасады намеренно оставляются жить до конца
					 *       работы процесса: разрушать их под работающим потоком нельзя
					 */
					if(waiter.wait_for(std::chrono::seconds(SHUTDOWN_TIMEOUT)) == std::future_status::timeout){
						// Помечаем неостановимый цикл событий
						this->_result.unstoppable = true;
						// Отпускаем фоновый поток, не дожидаясь его завершения
						worker.detach();
						// Продлеваем жизнь фасадов до конца работы процесса
						abandoned().emplace_back(std::move(this->_server), std::move(this->_client));
						// Выводим итоги прогона
						return this->_result;
					}
				}
				// Дожидаемся завершения фонового потока
				worker.join();
				// Выводим итоги прогона
				return this->_result;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param config   конфигурация прогона
			 * @param port     порт прослушивания сервера на локальной петле
			 * @param fmk      объект фреймворка
			 * @param log      объект для работы с логами
			 * @param security тестовое окружение транспортной безопасности
			 *
			 */
			Harness(const config_t & config, const uint16_t port, awh::fmk_t * fmk, awh::log_t * log, QuicSecurity * security) noexcept :
			 _fmk(fmk), _log(log), _security(security), _config(config), _port(port),
			 _sid(awh::quic::connection_t::INVALID_STREAM), _offset(0), _recvOffset(0),
			 _streamDone(false), _datagramDone(false), _finishing(false) {}
	};
};

/**
 * @brief Класс фикстуры интеграционных тестов фасадов QUIC
 *
 */
class QuicFacadeTest : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект для работы с логами
		std::unique_ptr <awh::log_t> _log;
		// Тестовое окружение транспортной безопасности
		std::unique_ptr <QuicSecurity> _security;
	protected:
		/**
		 * @brief Метод подбора порта прослушивания на локальной петле
		 *
		 * @note Порт выводится из идентификатора процесса, чтобы снизить вероятность
		 *       коллизии при параллельных прогонах на одной машине
		 *
		 * @return подобранный порт
		 *
		 */
		uint16_t pickPort() const noexcept {
			// Выводим порт в диапазоне 30000..49999 на основе идентификатора процесса
			return static_cast <uint16_t> (30000 + (static_cast <uint32_t> (::getpid()) % 20000));
		}
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp() override {
			// Инициализируем объект фреймворка
			this->_fmk = std::make_unique <awh::fmk_t> ();
			// Инициализируем объект логирования
			this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
			// Отключаем вывод логов в тестовом окружении
			this->_log->level(awh::log_t::level_t::NONE);
			// Инициализируем тестовое окружение транспортной безопасности
			this->_security = std::make_unique <QuicSecurity> (this->_fmk.get(), this->_log.get());
		}
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown() override {}
};

/**
 * @brief Тест сквозного эхо потока приложения через фасады клиента и сервера
 *
 */
TEST_F(QuicFacadeTest, ConnectAndStreamEcho){
	// Конфигурация прогона: простое эхо небольшой нагрузки
	config_t config;
	// Устанавливаем сценарий простого эхо
	config.scenario = scenario_t::ECHO;
	// Устанавливаем размер нагрузки потока
	config.payload = 256;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(15000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "handshake or echo timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что клиентский send() принял всю нагрузку целиком (без ограничений)
	EXPECT_EQ(result.firstSendReturn, config.payload);
	// Проверяем, что сервер получил флаг завершения потока
	EXPECT_TRUE(result.serverFin);
	// Проверяем, что сервер принял всю нагрузку
	EXPECT_EQ(result.serverBytes, config.payload);
	// Проверяем, что эхо-ответ побайтово совпал с отправленной нагрузкой
	EXPECT_TRUE(result.streamEchoMatched);
}

/**
 * @brief Тест обратного давления и сигнала writable через фасады клиента и сервера
 *
 */
TEST_F(QuicFacadeTest, BackpressurePartialSendAndWritable){
	// Конфигурация прогона: обратное давление с малыми водяными метками
	config_t config;
	// Устанавливаем сценарий обратного давления
	config.scenario = scenario_t::BACKPRESSURE;
	// Устанавливаем крупный размер нагрузки, заведомо превышающий верхнюю метку
	config.payload = 65536;
	// Устанавливаем верхнюю водяную метку буфера отправки
	config.sendHigh = 8192;
	// Устанавливаем нижнюю водяную метку буфера отправки
	config.sendLow = 4096;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "backpressure exchange timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что первая отправка принята частично (сверх верхней метки приём ограничен)
	EXPECT_GT(result.firstSendReturn, 0u);
	EXPECT_LT(result.firstSendReturn, config.payload);
	// Проверяем, что сигнал writable сработал (буфер отправки освобождался по мере ухода данных)
	EXPECT_TRUE(result.writableFired);
	// Проверяем, что сервер в итоге принял всю нагрузку целиком
	EXPECT_EQ(result.serverBytes, config.payload);
}

/**
 * @brief Тест вытягивания тела потока источником данных через фасады клиента и сервера
 *
 */
TEST_F(QuicFacadeTest, StreamDataSourcePull){
	// Конфигурация прогона: источник данных вытягивается движком
	config_t config;
	// Устанавливаем сценарий источника данных
	config.scenario = scenario_t::DATA_SOURCE;
	// Устанавливаем объём тела потока источника
	config.payload = 50000;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "data source exchange timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что сервер принял всё вытянутое источником тело потока
	EXPECT_EQ(result.serverBytes, config.payload);
	// Проверяем, что сервер получил флаг завершения потока (источник сигнализировал конец данных)
	EXPECT_TRUE(result.serverFin);
	// Проверяем, что принятый сервером образец совпал с ожидаемым
	EXPECT_TRUE(result.serverPatternOk);
}

/**
 * @brief Тест сквозного эхо датаграммы приложения через фасады клиента и сервера
 *
 */
TEST_F(QuicFacadeTest, DatagramEcho){
	// Конфигурация прогона: обмен потоком и датаграммой приложения
	config_t config;
	// Устанавливаем сценарий датаграммы
	config.scenario = scenario_t::DATAGRAM;
	// Устанавливаем размер нагрузки потока
	config.payload = 128;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(15000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "datagram exchange timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что эхо-ответ потока побайтово совпал с отправленной нагрузкой
	EXPECT_TRUE(result.streamEchoMatched);
	// Проверяем, что датаграмма-эхо побайтово совпала с отправленной
	EXPECT_TRUE(result.datagramMatched);
}

/**
 * @brief Тест завершения соединения по инициативе клиента (disconnect на сервере)
 *
 */
TEST_F(QuicFacadeTest, ClientCloseFiresServerDisconnect){
	// Конфигурация прогона: эхо с последующим закрытием соединения клиентом
	config_t config;
	// Устанавливаем сценарий простого эхо
	config.scenario = scenario_t::ECHO;
	// Устанавливаем закрытие соединения по инициативе клиента
	config.teardown = teardown_t::CLIENT_CLOSE;
	// Устанавливаем размер нагрузки потока
	config.payload = 256;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(15000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "client close teardown timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что эхо-ответ дошёл до закрытия
	EXPECT_TRUE(result.streamEchoMatched);
	// Проверяем, что сервер получил оповещение о завершении соединения после close() клиента
	EXPECT_TRUE(result.serverDisconnectFired);
}

/**
 * @brief Тест завершения соединения по инициативе сервера (disconnect на клиенте)
 *
 */
TEST_F(QuicFacadeTest, ServerCloseFiresClientDisconnect){
	// Конфигурация прогона: эхо с последующим закрытием соединения сервером
	config_t config;
	// Устанавливаем сценарий простого эхо
	config.scenario = scenario_t::ECHO;
	// Устанавливаем закрытие соединения по инициативе сервера
	config.teardown = teardown_t::SERVER_CLOSE;
	// Устанавливаем размер нагрузки потока
	config.payload = 256;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(15000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "server close teardown timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что клиент получил оповещение о завершении соединения после close(cid) сервера
	EXPECT_TRUE(result.clientDisconnectFired);
}

/**
 * @brief Тест неудачного подключения: сервера на целевом порту нет
 *
 */
TEST_F(QuicFacadeTest, ConnectFailureNoServer){
	// Конфигурация прогона: клиент подключается на соседний порт, где сервера нет
	config_t config;
	// Устанавливаем сценарий простого эхо
	config.scenario = scenario_t::ECHO;
	// Устанавливаем размер нагрузки потока
	config.payload = 64;
	// Смещаем целевой порт клиента - на нём никто не слушает
	config.targetPortOffset = 1;
	// Устанавливаем низкий таймаут простоя, чтобы неудачное подключение завершилось быстро
	config.idleTimeout = 3000;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(15000);
	// Проверяем, что соединение НЕ установлено (рукопожатие не с кем выполнить)
	EXPECT_FALSE(result.connected);
	// Проверяем, что эхо-ответ не получен
	EXPECT_FALSE(result.streamEchoMatched);
	// Проверяем, что сервер не принял никаких данных потока
	EXPECT_EQ(result.serverBytes, 0u);
}

/**
 * @brief Тест сквозного обмена с маркировкой датаграмм признаком перегрузки (ECN)
 *
 * @details Метка перегрузки едет управляющими данными вызова отправки, а не опцией
 *          сокета: сокет у датаграммного сервера один на все соединения, и опция
 *          метила бы датаграммы всех соединений разом. Ширина значения метки для
 *          IPv4 у систем расходится - FreeBSD берёт октет и отвергает целое, macOS
 *          берёт целое и теряет метку у октета, - поэтому ошибка в ней обрывает
 *          обмен либо остаётся незамеченной. Прогон с включённой маркировкой ловит
 *          первое: не примись управляющие данные, ни одна датаграмма не уйдёт
 *
 */
TEST_F(QuicFacadeTest, EchoWithCongestionMarking){
	// Конфигурация прогона: эхо с маркировкой исходящих датаграмм
	config_t config;
	// Устанавливаем сценарий простого эхо
	config.scenario = scenario_t::ECHO;
	// Устанавливаем размер нагрузки потока
	config.payload = 4096;
	// Включаем уведомление о перегрузке пути
	config.ecn = true;
	// Создаём интеграционное окружение фасадов
	Harness harness(config, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(15000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "marked exchange timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что сервер принял всю нагрузку
	EXPECT_EQ(result.serverBytes, config.payload);
	// Проверяем, что эхо-ответ побайтово совпал с отправленной нагрузкой
	EXPECT_TRUE(result.streamEchoMatched);
}
