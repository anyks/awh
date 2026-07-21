/**
 * @file: connection.hpp
 * @date: 2026-07-21
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_PROTO_QUIC_CONNECTION__
#define __AWH_PROTO_QUIC_CONNECTION__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "quic.hpp"
#include "frame.hpp"
#include "packet.hpp"
#include "crypto.hpp"
#include "handshake.hpp"
#include "../../sys/global.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 */
	namespace quic {
		/**
		 * @brief Класс соединения QUIC (RFC 9000 §5)
		 *
		 * @details Слой соединения над хендшейк-машиной: приём и сборка UDP-датаграмм
		 *          целиком - разбор коалесцированных пакетов, снятие/установка защиты,
		 *          диспетчеризация фреймов, пространства номеров пакетов, генерация
		 *          подтверждений (ACK), CRYPTO-потоки со сборкой по смещениям и
		 *          завершение соединения (CONNECTION_CLOSE), восстановление потерь
		 *          по RFC 9002 (оценка RTT, детект потерь по порогам номера и
		 *          времени, таймер PTO с ретрансмиссией), а также потоки приложения
		 *          (RFC 9000 §2-§4): открытие потоков, отправка и сборка данных
		 *          STREAM по смещениям, flow control соединения и потоков
		 *          (MAX_DATA/MAX_STREAM_DATA), лимиты потоков (MAX_STREAMS),
		 *          аварийное завершение (RESET_STREAM/STOP_SENDING). Работает
		 *          без ввода-вывода и собственных таймеров (sans-IO): датаграммы
		 *          передаются через read(), исходящие извлекаются через write(),
		 *          текущее время передаётся параметром, дедлайн ближайшего события
		 *          отдаёт timeout(), просроченные таймеры обрабатывает tick().
		 */
		typedef class __AWH_SHARED_EXPORT__ Connection {
			public:
				/**
				 * @brief Состояния соединения
				 *
				 */
				enum class state_t : uint8_t {
					NONE        = 0x00, // Соединение не начато
					HANDSHAKING = 0x01, // Выполняется хендшейк
					CONNECTED   = 0x02, // Соединение установлено
					CLOSING     = 0x03, // Локальный эндпоинт завершил соединение
					DRAINING    = 0x04  // Удалённый эндпоинт завершил соединение
				};
			public:
				/**
				 * @brief Максимальный размер исходящей UDP-датаграммы (RFC 9000 §14.1)
				 *
				 */
				static constexpr size_t MAX_DATAGRAM_SIZE = 1200;
				/**
				 * @brief Лимит буфера сборки CRYPTO-потока одного уровня (RFC 9000 §7.5)
				 *
				 */
				static constexpr size_t MAX_CRYPTO_BUFFER = 65536;
			private:
				/**
				 * @brief Количество пространств номеров пакетов (RFC 9000 §12.3)
				 *
				 */
				static constexpr size_t SPACES = 3;
				/**
				 * @brief Пространство номеров пакетов (RFC 9000 §12.3)
				 *
				 */
				enum class space_t : uint8_t {
					INITIAL     = 0x00, // Пространство пакетов Initial
					HANDSHAKE   = 0x01, // Пространство пакетов Handshake
					APPLICATION = 0x02  // Пространство пакетов приложения (0-RTT и 1-RTT)
				};
			private:
				/**
				 * @brief Структура отправленного блока данных потока приложения (для ретрансмиссии)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Chunk {
					// Идентификатор потока
					uint64_t sid;
					// Смещение данных в потоке
					uint64_t offset;
					// Флаг завершения потока (FIN)
					bool fin;
					// Данные потока приложения
					string data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Chunk() noexcept;
				} chunk_t;
				/**
				 * @brief Структура учёта отправленного ack-eliciting пакета (RFC 9002 §A.1)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Sent {
					// Номер отправленного пакета
					uint64_t pn;
					// Время отправки пакета в миллисекундах
					uint64_t time;
					// Флаг наличия фрейма HANDSHAKE_DONE в пакете
					bool handshakeDone;
					// Отправленные CRYPTO-данные пакета со смещениями (для ретрансмиссии)
					vector <std::pair <uint64_t, string>> crypto;
					// Отправленные блоки данных потоков приложения (для ретрансмиссии)
					vector <chunk_t> stream;
					// Отправленные управляющие фреймы: тип и идентификатор потока (для повтора при потере)
					vector <std::pair <frame_t, uint64_t>> control;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Sent() noexcept;
				} sent_t;
				/**
				 * @brief Структура состояния одного потока приложения (RFC 9000 §3)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Stream {
					// Смещение начала буфера исходящих данных в потоке
					uint64_t txOffset;
					// Буфер исходящих данных потока (ожидают упаковки в пакеты)
					string txBuffer;
					// Лимит отправки потока от удалённого эндпоинта (MAX_STREAM_DATA)
					uint64_t txMax;
					// Флаг постановки завершения потока приложением (FIN)
					bool txFin;
					// Флаг выполненной отправки завершения потока (FIN)
					bool txFinSent;
					// Флаг необходимости отправки фрейма RESET_STREAM
					bool txReset;
					// Флаг выполненной отправки фрейма RESET_STREAM
					bool txResetSent;
					// Код ошибки приложения фрейма RESET_STREAM
					uint64_t txResetCode;
					// Флаг необходимости отправки фрейма STREAM_DATA_BLOCKED
					bool txBlocked;
					// Смещение непрерывно собранных входящих данных потока
					uint64_t rxOffset;
					// Наибольшее принятое смещение данных потока (для flow control)
					uint64_t rxHigh;
					// Буфер сборки входящих данных потока по смещениям
					map <uint64_t, string> rxBuffer;
					// Собранные непрерывные данные потока (ожидают выдачи приложению)
					string rxReady;
					// Анонсированный лимит приёма потока (MAX_STREAM_DATA)
					uint64_t rxMax;
					// Флаг необходимости отправки обновлённого лимита MAX_STREAM_DATA
					bool rxMaxQueued;
					// Флаг наличия финального размера потока (принят FIN или RESET_STREAM)
					bool rxFin;
					// Финальный размер потока в октетах
					uint64_t rxFinal;
					// Флаг выданного приложению завершения потока (FIN)
					bool rxFinDelivered;
					// Флаг аварийного завершения потока удалённым эндпоинтом (RESET_STREAM)
					bool rxReset;
					// Код ошибки приложения принятого фрейма RESET_STREAM
					uint64_t rxResetCode;
					// Флаг необходимости отправки фрейма STOP_SENDING
					bool stopQueued;
					// Флаг выполненной отправки фрейма STOP_SENDING
					bool stopSent;
					// Код ошибки приложения фрейма STOP_SENDING
					uint64_t stopCode;
					// Флаг учтённого завершения потока в лимите MAX_STREAMS
					bool credited;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Stream() noexcept;
				} stream_data_t;
				/**
				 * @brief Структура состояния одного пространства номеров пакетов
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Space {
					// Номер следующего отправляемого пакета
					uint64_t txPn;
					// Наибольший номер пакета, подтверждённый пиром
					uint64_t largestAcked;
					// Флаг наличия подтверждений от пира
					bool hasAcked;
					// Наибольший принятый номер пакета
					uint64_t largestRx;
					// Флаг приёма хотя бы одного пакета
					bool hasRx;
					// Флаг необходимости отправки подтверждения (принят ack-eliciting пакет)
					bool ackElicited;
					// Диапазоны принятых номеров пакетов в порядке убывания
					vector <frame::range_t> ranges;
					// Смещение начала буфера исходящих CRYPTO-данных в потоке уровня
					uint64_t txOffset;
					// Буфер исходящих CRYPTO-данных уровня (ожидают упаковки в пакеты)
					string txBuffer;
					// Смещение непрерывно собранных входящих CRYPTO-данных уровня
					uint64_t rxOffset;
					// Буфер сборки входящих CRYPTO-данных по смещениям
					map <uint64_t, string> rxBuffer;
					// Список отправленных и ещё не подтверждённых ack-eliciting пакетов
					vector <sent_t> sent;
					// Очередь ретрансмиссии CRYPTO-данных со смещениями (RFC 9002 §6.3)
					vector <std::pair <uint64_t, string>> rtxQueue;
					// Время детекта потерь пакетов пространства (RFC 9002 §6.1.2)
					uint64_t lossTime;
					// Флаг взведённого таймера детекта потерь
					bool hasLossTime;
					// Время отправки последнего ack-eliciting пакета (RFC 9002 §6.2.1)
					uint64_t lastElicited;
					// Флаг наличия отправленных ack-eliciting пакетов
					bool hasElicited;
					// Флаг необходимости отправки зондирующего фрейма PING (RFC 9002 §6.2.4)
					bool pingQueued;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Space() noexcept;
				} space_data_t;
			private:
				// Роль локального эндпоинта на соединении
				endpoint_t _endpoint;
				// Состояние соединения
				state_t _state;
			private:
				// Идентификатор соединения локального эндпоинта
				cid_t _scid;
				// Идентификатор соединения удалённого эндпоинта
				cid_t _dcid;
				// Флаг обновления DCID по первому ответу сервера (RFC 9000 §7.2)
				bool _dcidUpdated;
			private:
				// Флаг подтверждения хендшейка (RFC 9001 §4.1.2)
				bool _confirmed;
				// Флаг необходимости отправки фрейма HANDSHAKE_DONE (только сервер)
				bool _handshakeDone;
			private:
				// Флаг необходимости отправки фрейма PATH_RESPONSE
				bool _pathResponse;
				// Данные проверки пути для фрейма PATH_RESPONSE
				uint8_t _pathData[proto::PATH_DATA_SIZE];
			private:
				// Код ошибки завершения соединения
				uint64_t _closeCode;
				// Флаг ошибки приложения при завершении соединения
				bool _closeApp;
				// Флаг постановки завершения соединения в очередь отправки
				bool _closeQueued;
				// Флаг выполненной отправки фрейма CONNECTION_CLOSE
				bool _closeSent;
				// Причина завершения соединения
				string _closeReason;
			private:
				// Код ошибки транспорта соединения (локальной или полученной от пира)
				error_t _error;
			private:
				// Текущее время последнего вызова в миллисекундах
				uint64_t _now;
			private:
				// Последняя измеренная задержка приёма-передачи (RFC 9002 §5)
				uint64_t _latestRtt;
				// Минимальная измеренная задержка приёма-передачи
				uint64_t _minRtt;
				// Сглаженная задержка приёма-передачи
				uint64_t _smoothedRtt;
				// Вариативность задержки приёма-передачи
				uint64_t _rttVar;
				// Флаг наличия первого измерения задержки приёма-передачи
				bool _rttSampled;
				// Счётчик срабатываний таймера PTO без подтверждений (RFC 9002 §6.2.1)
				uint8_t _ptoCount;
			private:
				// Машина криптографического хендшейка
				handshake_t _handshake;
				// Локальные транспортные параметры (дополняются идентификаторами при старте)
				quic::params::params_t _params;
				// Транспортные параметры удалённого эндпоинта (после завершения хендшейка)
				quic::params::params_t _remote;
			private:
				// Состояния пространств номеров пакетов
				space_data_t _spaces[SPACES];
			private:
				// Список потоков приложения по идентификаторам
				map <uint64_t, stream_data_t> _streams;
				// Очередь ретрансмиссии блоков данных потоков приложения
				vector <chunk_t> _streamRtx;
			private:
				// Количество открытых локально двунаправленных потоков
				uint64_t _openedBidi;
				// Количество открытых локально однонаправленных потоков
				uint64_t _openedUni;
				// Лимит удалённого эндпоинта на локально открываемые двунаправленные потоки (MAX_STREAMS)
				uint64_t _maxBidiRemote;
				// Лимит удалённого эндпоинта на локально открываемые однонаправленные потоки (MAX_STREAMS)
				uint64_t _maxUniRemote;
			private:
				// Количество потоков, открытых удалённым эндпоинтом (двунаправленных)
				uint64_t _acceptedBidi;
				// Количество потоков, открытых удалённым эндпоинтом (однонаправленных)
				uint64_t _acceptedUni;
				// Анонсированный лимит на двунаправленные потоки удалённого эндпоинта
				uint64_t _maxBidiLocal;
				// Анонсированный лимит на однонаправленные потоки удалённого эндпоинта
				uint64_t _maxUniLocal;
				// Флаг необходимости отправки обновлённого лимита MAX_STREAMS (двунаправленные)
				bool _maxBidiQueued;
				// Флаг необходимости отправки обновлённого лимита MAX_STREAMS (однонаправленные)
				bool _maxUniQueued;
			private:
				// Количество отправленных данных потоков приложения (flow control соединения)
				uint64_t _txData;
				// Лимит отправки данных соединения от удалённого эндпоинта (MAX_DATA)
				uint64_t _txMaxData;
				// Флаг необходимости отправки фрейма DATA_BLOCKED
				bool _txDataBlocked;
				// Сумма наибольших принятых смещений потоков (flow control соединения)
				uint64_t _rxData;
				// Количество выданных приложению данных потоков
				uint64_t _rxConsumed;
				// Анонсированный лимит приёма данных соединения (MAX_DATA)
				uint64_t _rxMaxData;
				// Флаг необходимости отправки обновлённого лимита MAX_DATA
				bool _rxMaxDataQueued;
			private:
				/**
				 * @brief Метод определения пространства номеров пакетов по уровню шифрования
				 *
				 * @param level уровень шифрования
				 * @return      пространство номеров пакетов
				 */
				space_t space(const level_t level) const noexcept;
				/**
				 * @brief Метод регистрации принятого номера пакета в диапазонах пространства
				 *
				 * @param space пространство номеров пакетов
				 * @param pn    принятый номер пакета
				 */
				void record(const space_t space, const uint64_t pn) noexcept;
				/**
				 * @brief Метод проверки повторного приёма номера пакета
				 *
				 * @param space пространство номеров пакетов
				 * @param pn    принятый номер пакета
				 * @return      результат проверки (true - пакет уже был принят)
				 */
				bool duplicate(const space_t space, const uint64_t pn) const noexcept;
			private:
				/**
				 * @brief Метод перекладывания исходящих CRYPTO-данных хендшейка в буферы пространств
				 *
				 */
				void pull() noexcept;
				/**
				 * @brief Метод постановки завершения соединения с ошибкой транспорта в очередь
				 *
				 * @param error код ошибки транспорта
				 */
				void fail(const error_t error) noexcept;
			private:
				/**
				 * @brief Метод сброса ключей уровня вместе с состоянием восстановления потерь (RFC 9001 §4.9)
				 *
				 * @param level уровень шифрования
				 */
				void discard(const level_t level) noexcept;
				/**
				 * @brief Метод обновления оценки задержки приёма-передачи (RFC 9002 §5.3)
				 *
				 * @param sample измеренная задержка приёма-передачи
				 * @param delay  задержка подтверждения удалённого эндпоинта
				 */
				void rtt(const uint64_t sample, const uint64_t delay) noexcept;
				/**
				 * @brief Метод повторной постановки содержимого пакета в очереди отправки (RFC 9002 §6.3)
				 *
				 * @param space  пространство номеров пакетов
				 * @param packet учётная запись потерянного либо зондируемого пакета
				 */
				void requeue(const space_t space, const sent_t & packet) noexcept;
				/**
				 * @brief Метод детекта потерянных пакетов пространства (RFC 9002 §6.1)
				 *
				 * @note Потерянные CRYPTO-данные и фрейм HANDSHAKE_DONE ставятся
				 *       в очередь ретрансмиссии
				 *
				 * @param space пространство номеров пакетов
				 */
				void detect(const space_t space) noexcept;
				/**
				 * @brief Метод постановки зондирующих данных пространства в очередь (RFC 9002 §6.2.4)
				 *
				 * @param space пространство номеров пакетов
				 */
				void probe(const space_t space) noexcept;
				/**
				 * @brief Метод вычисления интервала таймера PTO (RFC 9002 §6.2.1)
				 *
				 * @param space пространство номеров пакетов
				 * @return     интервал таймера PTO в миллисекундах
				 */
				uint64_t interval(const space_t space) const noexcept;
				/**
				 * @brief Метод вычисления дедлайна таймера PTO пространства (RFC 9002 §6.2.1)
				 *
				 * @param space пространство номеров пакетов
				 * @return      дедлайн таймера PTO в миллисекундах (0 - таймер не взведён)
				 */
				uint64_t deadline(const space_t space) const noexcept;
			private:
				/**
				 * @brief Метод проверки возможности отправки данных в поток локальным эндпоинтом
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки (true - отправка допустима)
				 */
				bool sendable(const uint64_t sid) const noexcept;
				/**
				 * @brief Метод проверки возможности приёма данных потока локальным эндпоинтом
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки (true - приём допустим)
				 */
				bool receivable(const uint64_t sid) const noexcept;
				/**
				 * @brief Метод получения начального лимита приёма потока из локальных параметров
				 *
				 * @param sid идентификатор потока
				 * @return    начальный лимит приёма потока в октетах
				 */
				uint64_t rxWindow(const uint64_t sid) const noexcept;
				/**
				 * @brief Метод получения начального лимита отправки потока из параметров удалённого эндпоинта
				 *
				 * @param sid идентификатор потока
				 * @return    начальный лимит отправки потока в октетах
				 */
				uint64_t txWindow(const uint64_t sid) const noexcept;
				/**
				 * @brief Метод поиска либо создания потока по принятому фрейму (RFC 9000 §3.2)
				 *
				 * @note Проверяет допустимость идентификатора и лимиты потоков;
				 *       поток удалённого эндпоинта создаётся неявно
				 *
				 * @param sid   идентификатор потока
				 * @param error код ошибки транспорта
				 * @return      состояние потока (nullptr - нарушение протокола)
				 */
				stream_data_t * accept(const uint64_t sid, error_t & error) noexcept;
				/**
				 * @brief Метод обработки принятого фрейма STREAM (RFC 9000 §19.8)
				 *
				 * @param frame принятый фрейм данных потока приложения
				 * @return      результат обработки (OK/ERROR)
				 */
				status_t inputStream(const frame::stream_t & frame) noexcept;
				/**
				 * @brief Метод применения транспортных параметров удалённого эндпоинта после хендшейка
				 *
				 * @return результат применения (true - параметры применены)
				 */
				bool established() noexcept;
				/**
				 * @brief Метод учёта завершения потока удалённого эндпоинта в лимите MAX_STREAMS
				 *
				 * @param sid    идентификатор потока
				 * @param stream состояние потока
				 */
				void credit(const uint64_t sid, stream_data_t & stream) noexcept;
			private:
				/**
				 * @brief Метод обработки входящих CRYPTO-данных со сборкой по смещениям
				 *
				 * @param level  уровень шифрования пакета с CRYPTO-фреймом
				 * @param offset смещение данных в потоке криптографического хендшейка
				 * @param data   данные CRYPTO-фрейма
				 * @return       результат обработки (OK/ERROR)
				 */
				status_t input(const level_t level, const uint64_t offset, string_view data) noexcept;
				/**
				 * @brief Метод разбора и диспетчеризации фреймов расшифрованной нагрузки пакета
				 *
				 * @param level уровень шифрования пакета
				 * @param data  буфер расшифрованной нагрузки
				 * @param size  размер расшифрованной нагрузки
				 * @return      результат разбора (OK/ERROR)
				 */
				status_t frames(const level_t level, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод сборки нагрузки очередного пакета уровня шифрования
				 *
				 * @param level  уровень шифрования пакета
				 * @param budget доступно октетов в датаграмме для нагрузки
				 * @param output собранная нагрузка пакета (фреймы)
				 * @param meta   учётная запись пакета для восстановления потерь
				 * @param elicit флаг наличия ack-eliciting фреймов в нагрузке
				 * @return       результат сборки (true - нагрузка не пустая)
				 */
				bool payload(const level_t level, const size_t budget, string & output, sent_t & meta, bool & elicit) noexcept;
				/**
				 * @brief Метод вычисления размера заголовка пакета уровня шифрования
				 *
				 * @param level  уровень шифрования пакета
				 * @param length значение поля Length (номер пакета + нагрузка + тег AEAD)
				 * @param pnSize размер кодирования номера пакета в октетах
				 * @return       размер заголовка пакета в октетах
				 */
				size_t headerSize(const level_t level, const uint64_t length, const size_t pnSize) const noexcept;
				/**
				 * @brief Метод сборки и защиты пакета уровня шифрования (заголовок + нагрузка)
				 *
				 * @param output  выходной буфер датаграммы (пакет дописывается)
				 * @param level   уровень шифрования пакета
				 * @param payload нагрузка пакета (фреймы)
				 * @return        результат сборки (false - ошибка криптографической библиотеки)
				 */
				bool seal(string & output, const level_t level, string_view payload) noexcept;
			public:
				/**
				 * @brief Метод установки списка поддерживаемых ALPN-протоколов
				 *
				 * @param protocols список поддерживаемых ALPN-протоколов (например "h3")
				 */
				void alpn(const vector <string> & protocols) noexcept;
				/**
				 * @brief Метод извлечения согласованного ALPN-протокола
				 *
				 * @return согласованный ALPN-протокол (пусто - согласование не выполнено)
				 */
				string alpn() const noexcept;
			public:
				/**
				 * @brief Метод установки доменного имени удалённого сервера (SNI)
				 *
				 * @param sni доменное имя удалённого сервера
				 */
				void serverNameIndication(string_view sni) noexcept;
				/**
				 * @brief Метод установки сертификата и приватного ключа локального узла
				 *
				 * @param certificate сертификат в формате PEM
				 * @param privateKey  приватный ключ в формате PEM
				 */
				void certificate(string_view certificate, string_view privateKey) noexcept;
				/**
				 * @brief Метод установки проверки сертификата удалённого узла
				 *
				 * @param mode режим проверки сертификата удалённого узла
				 */
				void verify(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
				 *
				 * @note Идентификаторы соединения (initial_source_connection_id и
				 *       original_destination_connection_id) заполняются автоматически
				 *
				 * @param params локальные транспортные параметры
				 */
				void params(const quic::params::params_t & params) noexcept;
				/**
				 * @brief Метод извлечения транспортных параметров удалённого узла (RFC 9000 §7.4)
				 *
				 * @param params транспортные параметры удалённого узла
				 * @param error  код ошибки транспорта
				 * @return       результат извлечения (OK/INCOMPLETE/ERROR)
				 */
				status_t peer(quic::params::params_t & params, error_t & error) const noexcept;
			public:
				/**
				 * @brief Метод начала соединения клиентом
				 *
				 * @note Генерирует случайные идентификаторы соединения, выводит ключи
				 *       Initial и формирует ClientHello - первая датаграмма будет
				 *       доступна через write()
				 *
				 * @return результат начала соединения (OK/ERROR)
				 */
				status_t connect() noexcept;
			public:
				/**
				 * @brief Метод обработки входящей UDP-датаграммы
				 *
				 * @note Датаграмма может содержать несколько коалесцированных пакетов.
				 *       Сервер инициализируется первым пакетом Initial клиента.
				 *       Пакеты, которые невозможно расшифровать, отбрасываются
				 *
				 * @param data буфер входящей UDP-датаграммы
				 * @param size размер входящей UDP-датаграммы
				 * @param now  текущее время в миллисекундах
				 * @return     результат обработки (OK/ERROR)
				 */
				status_t read(const uint8_t * data, const size_t size, const uint64_t now) noexcept;
				/**
				 * @brief Метод сборки исходящей UDP-датаграммы
				 *
				 * @note Собирает пакеты доступных уровней шифрования с коалесценцией.
				 *       Датаграмма с пакетом Initial дополняется до 1200 октетов
				 *       (RFC 9000 §14.1). Вызывается циклически до пустого результата
				 *
				 * @param output буфер исходящей UDP-датаграммы (очищается)
				 * @param now    текущее время в миллисекундах
				 * @return       результат сборки (true - датаграмма готова к отправке)
				 */
				bool write(string & output, const uint64_t now) noexcept;
			public:
				/**
				 * @brief Метод получения дедлайна ближайшего события таймера (RFC 9002 §6)
				 *
				 * @note Вызывающий код взводит таймер на полученное время и по его
				 *       истечении вызывает tick(), затем отправляет датаграммы write()
				 *
				 * @return дедлайн ближайшего события в миллисекундах (0 - таймер не требуется)
				 */
				uint64_t timeout() const noexcept;
				/**
				 * @brief Метод обработки просроченных таймеров (RFC 9002 §6.2)
				 *
				 * @note По таймеру детекта потерь выполняется детект и ретрансмиссия,
				 *       по таймеру PTO - постановка зондирующих данных в очередь
				 *
				 * @param now текущее время в миллисекундах
				 */
				void tick(const uint64_t now) noexcept;
			public:
				/**
				 * @brief Значение недопустимого идентификатора потока
				 *
				 */
				static constexpr uint64_t INVALID_STREAM = 0xFFFFFFFFFFFFFFFF;
				/**
				 * @brief Метод открытия нового потока приложения (RFC 9000 §2.1)
				 *
				 * @note Доступен после установления соединения; лимит удалённого
				 *       эндпоинта (MAX_STREAMS) проверяется автоматически
				 *
				 * @param unidirectional флаг однонаправленного потока
				 * @return               идентификатор потока (INVALID_STREAM - открытие невозможно)
				 */
				uint64_t open(const bool unidirectional) noexcept;
				/**
				 * @brief Метод постановки данных потока в очередь отправки (RFC 9000 §2.2)
				 *
				 * @note Данные буферизируются и упаковываются во фреймы STREAM
				 *       с учётом flow control при сборке датаграмм write()
				 *
				 * @param sid  идентификатор потока
				 * @param data данные потока приложения
				 * @param fin  флаг завершения потока (FIN)
				 * @return     результат постановки (OK/ERROR)
				 */
				status_t send(const uint64_t sid, string_view data, const bool fin) noexcept;
				/**
				 * @brief Метод получения списка потоков с данными для приложения
				 *
				 * @return список идентификаторов потоков с собранными данными либо завершением
				 */
				vector <uint64_t> readable() const noexcept;
				/**
				 * @brief Метод выдачи собранных данных потока приложению (RFC 9000 §2.2)
				 *
				 * @note Выдаются только непрерывно собранные данные; лимиты flow
				 *       control обновляются автоматически по мере потребления
				 *
				 * @param sid    идентификатор потока
				 * @param output собранные данные потока (дописываются)
				 * @param fin    флаг завершения потока удалённым эндпоинтом (FIN)
				 * @return       результат выдачи (OK/ERROR - поток неизвестен либо сброшен)
				 */
				status_t receive(const uint64_t sid, string & output, bool & fin) noexcept;
				/**
				 * @brief Метод аварийного завершения отправки потока (RFC 9000 §2.4)
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки приложения
				 */
				void reset(const uint64_t sid, const uint64_t code) noexcept;
				/**
				 * @brief Метод запроса прекращения передачи удалённым эндпоинтом (RFC 9000 §3.5)
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки приложения
				 */
				void stop(const uint64_t sid, const uint64_t code) noexcept;
				/**
				 * @brief Метод проверки аварийного завершения потока удалённым эндпоинтом
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки приложения принятого фрейма RESET_STREAM
				 * @return     результат проверки (true - поток сброшен удалённым эндпоинтом)
				 */
				bool aborted(const uint64_t sid, uint64_t & code) const noexcept;
			public:
				/**
				 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
				 *
				 * @param code   код ошибки приложения
				 * @param reason человекочитаемая причина завершения
				 */
				void close(const uint64_t code, string_view reason) noexcept;
			public:
				/**
				 * @brief Метод получения состояния соединения
				 *
				 * @return состояние соединения
				 */
				state_t state() const noexcept;
				/**
				 * @brief Метод получения кода ошибки транспорта соединения
				 *
				 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
				 */
				error_t error() const noexcept;
			public:
				/**
				 * @brief Метод получения идентификатора соединения локального эндпоинта
				 *
				 * @return идентификатор соединения локального эндпоинта
				 */
				const cid_t & scid() const noexcept;
				/**
				 * @brief Метод получения идентификатора соединения удалённого эндпоинта
				 *
				 * @return идентификатор соединения удалённого эндпоинта
				 */
				const cid_t & dcid() const noexcept;
			public:
				/**
				 * @brief Метод доступа к машине криптографического хендшейка
				 *
				 * @return машина криптографического хендшейка
				 */
				const handshake_t & handshake() const noexcept;
			public:
				/**
				 * Запрещаем копирование и перемещение (хендшейк-машина владеет
				 * TLS-соединением с обратным указателем)
				 */
				Connection(const Connection &) = delete;
				Connection(Connection &&) = delete;
				Connection & operator = (const Connection &) = delete;
				Connection & operator = (Connection &&) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param endpoint роль локального эндпоинта на соединении
				 */
				explicit Connection(const endpoint_t endpoint) noexcept;
		} connection_t;
	};
};

#endif // __AWH_PROTO_QUIC_CONNECTION__
