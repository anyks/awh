/**
 * @file: frame.hpp
 * @date: 2026-07-19
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

#ifndef __AWH_HTTP_PARSER_HTTP2_FRAME__
#define __AWH_HTTP_PARSER_HTTP2_FRAME__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем наши заголовочные файлы
 */
#include "h2.hpp"

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
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Пространство имён внутренних слоёв протокола HTTP/2
		 *
		 */
		namespace h2 {
			/**
			 * @brief Пространство имён framing-слоя HTTP/2 (RFC 9113 §4-6): разбор и сборка фреймов
			 *
			 * @details Разбор zero-copy: полезная нагрузка отдаётся как string_view с указателем
			 *          во входной буфер. Сборка дописывает байты в string (выходной буфер соединения).
			 *          Слой не хранит состояния соединения - это чистые функции над байтами. Логика
			 *          состояний потоков, flow control и HPACK живёт в http.hpp / hpack.hpp.
			 */
			namespace frame {
				/**
				 * @brief Структура разобранного заголовка фрейма (RFC 9113 §4.1)
				 *
				 */
				typedef struct Header {
					// Длина полезной нагрузки (24 бита)
					uint32_t length;
					// Тип фрейма
					frame_t type;
					// Флаги фрейма (семантика зависит от типа)
					uint8_t flags;
					// Идентификатор потока (31 бит)
					uint32_t streamId;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Header() noexcept :
					 length(0), type(frame_t::DATA),
					 flags(flag::NONE), streamId(0) {}
				} header_t;
				/**
				 * @brief Структура параметра SETTINGS (идентификатор + значение)
				 *
				 */
				typedef struct Setting {
					// Идентификатор параметра
					setting_t id;
					// Значение параметра
					uint32_t value;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Setting() noexcept :
					 id(setting_t::HEADER_TABLE_SIZE), value(0) {}
				} setting_entry_t;
				/**
				 * @brief Структура полезной нагрузки DATA (RFC 9113 §6.1), padding уже снят
				 *
				 */
				typedef struct Data {
					// Данные тела (zero-copy во входной буфер)
					string_view data;
					// Флаг завершения потока
					bool endStream;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Data() noexcept : data{}, endStream(false) {}
				} data_t;
				/**
				 * @brief Структура полезной нагрузки HEADERS (RFC 9113 §6.2), padding уже снят
				 *
				 */
				typedef struct Headers {
					// Фрагмент блока заголовков HPACK (zero-copy во входной буфер)
					string_view block;
					// Флаг завершения потока
					bool endStream;
					// Флаг завершения блока заголовков
					bool endHeaders;
					// Флаг наличия полей приоритета (RFC 7540, deprecated)
					bool hasPriority;
					// Флаг эксклюзивной зависимости потока
					bool exclusive;
					// Идентификатор потока, от которого зависит текущий
					uint32_t streamDep;
					// Вес потока (фактический вес = weight + 1)
					uint8_t weight;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Headers() noexcept :
					 block{}, endStream(false), endHeaders(false),
					 hasPriority(false), exclusive(false),
					 streamDep(0), weight(0) {}
				} headers_t;
				/**
				 * @brief Структура полезной нагрузки PRIORITY (RFC 9113 §6.3)
				 *
				 */
				typedef struct Priority {
					// Флаг эксклюзивной зависимости потока
					bool exclusive;
					// Идентификатор потока, от которого зависит текущий
					uint32_t streamDep;
					// Вес потока
					uint8_t weight;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Priority() noexcept :
					 exclusive(false), streamDep(0), weight(0) {}
				} priority_t;
				/**
				 * @brief Структура полезной нагрузки PUSH_PROMISE (RFC 9113 §6.6), padding уже снят
				 *
				 */
				typedef struct Push_Promise {
					// Идентификатор обещанного потока
					uint32_t promisedStreamId;
					// Фрагмент блока заголовков HPACK (zero-copy во входной буфер)
					string_view block;
					// Флаг завершения блока заголовков
					bool endHeaders;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Push_Promise() noexcept :
					 promisedStreamId(0), block{}, endHeaders(false) {}
				} push_promise_t;
				/**
				 * @brief Структура полезной нагрузки GOAWAY (RFC 9113 §6.8)
				 *
				 */
				typedef struct Goaway {
					// Наибольший идентификатор обработанного потока
					uint32_t lastStreamId;
					// Код ошибки завершения соединения
					error_t code;
					// Необязательные отладочные данные (zero-copy во входной буфер)
					string_view debugData;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Goaway() noexcept :
					 lastStreamId(0), code(error_t::NO_ERROR), debugData{} {}
				} goaway_t;
				/**
				 * @brief Функция разбора 9-байтового заголовка фрейма
				 *
				 * @param data входной буфер
				 * @param size доступно байт
				 * @param out  разобранный заголовок фрейма
				 * @return     результат разбора (true - в буфере было достаточно байт и заголовок разобран)
				 */
				bool parseHeader(const uint8_t * data, const size_t size, header_t & out) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки DATA
				 *
				 * @note Параметр payload должен указывать на ровно h.length байт нагрузки
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param out     разобранная полезная нагрузка
				 * @param err     код ошибки протокола (PROTOCOL_ERROR на некорректном padding)
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parseData(const header_t & h, const uint8_t * payload, data_t & out, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки HEADERS (с учётом padding и приоритета)
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param out     разобранная полезная нагрузка
				 * @param err     код ошибки протокола
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parseHeaders(const header_t & h, const uint8_t * payload, headers_t & out, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки PRIORITY (требует ровно 5 байт)
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param out     разобранная полезная нагрузка
				 * @param err     код ошибки протокола
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parsePriority(const header_t & h, const uint8_t * payload, priority_t & out, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки RST_STREAM (требует ровно 4 байта)
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param code    код ошибки, с которым сброшен поток
				 * @param err     код ошибки протокола
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parseRstStream(const header_t & h, const uint8_t * payload, error_t & code, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки SETTINGS (длина кратна 6)
				 *
				 * @note Для ACK-фрейма нагрузка должна быть пустой. Параметры дописываются в out
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param out     список разобранных параметров
				 * @param err     код ошибки протокола
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parseSettings(const header_t & h, const uint8_t * payload, vector <setting_entry_t> & out, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки PUSH_PROMISE (с учётом padding)
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param out     разобранная полезная нагрузка
				 * @param err     код ошибки протокола
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parsePushPromise(const header_t & h, const uint8_t * payload, push_promise_t & out, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки PING (требует ровно 8 байт opaque-данных)
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param opaque  извлечённые opaque-данные
				 * @param err     код ошибки протокола
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parsePing(const header_t & h, const uint8_t * payload, uint8_t opaque[8], error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки GOAWAY (минимум 8 байт)
				 *
				 * @param h       заголовок фрейма
				 * @param payload полезная нагрузка фрейма
				 * @param out     разобранная полезная нагрузка
				 * @param err     код ошибки протокола
				 * @return        результат разбора (OK/ERROR)
				 */
				status_t parseGoaway(const header_t & h, const uint8_t * payload, goaway_t & out, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки WINDOW_UPDATE (требует ровно 4 байта)
				 *
				 * @note Нулевой инкремент - PROTOCOL_ERROR (на уровне потока - RST_STREAM)
				 *
				 * @param h         заголовок фрейма
				 * @param payload   полезная нагрузка фрейма
				 * @param increment извлечённый инкремент окна
				 * @param err       код ошибки протокола
				 * @return          результат разбора (OK/ERROR)
				 */
				status_t parseWindowUpdate(const header_t & h, const uint8_t * payload, uint32_t & increment, error_t & err) noexcept;
				/**
				 * @brief Функция разбора полезной нагрузки CONTINUATION (фрагмент блока заголовков)
				 *
				 * @param h          заголовок фрейма
				 * @param payload    полезная нагрузка фрейма
				 * @param block      фрагмент блока заголовков (zero-copy во входной буфер)
				 * @param endHeaders флаг завершения блока заголовков
				 * @param err        код ошибки протокола
				 * @return           результат разбора (OK/ERROR)
				 */
				status_t parseContinuation(const header_t & h, const uint8_t * payload, string_view & block, bool & endHeaders, error_t & err) noexcept;
				/**
				 * @brief Функция сборки фрейма DATA (заголовок + нагрузка дописываются в out)
				 *
				 * @param out       выходной буфер соединения
				 * @param streamId  идентификатор потока
				 * @param data      данные тела
				 * @param endStream флаг завершения потока
				 */
				void serializeData(string & out, const uint32_t streamId, string_view data, const bool endStream) noexcept;
				/**
				 * @brief Функция сборки фрейма HEADERS (заголовок + нагрузка дописываются в out)
				 *
				 * @param out        выходной буфер соединения
				 * @param streamId   идентификатор потока
				 * @param block      фрагмент блока заголовков HPACK
				 * @param endStream  флаг завершения потока
				 * @param endHeaders флаг завершения блока заголовков
				 */
				void serializeHeaders(string & out, const uint32_t streamId, string_view block, const bool endStream, const bool endHeaders) noexcept;
				/**
				 * @brief Функция сборки фрейма CONTINUATION (заголовок + нагрузка дописываются в out)
				 *
				 * @param out        выходной буфер соединения
				 * @param streamId   идентификатор потока
				 * @param block      фрагмент блока заголовков HPACK
				 * @param endHeaders флаг завершения блока заголовков
				 */
				void serializeContinuation(string & out, const uint32_t streamId, string_view block, const bool endHeaders) noexcept;
				/**
				 * @brief Функция сборки HPACK-блока в HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10)
				 *
				 * @note END_STREAM, если задан, ставится только на первый HEADERS -
				 *       даже если блок продолжается в CONTINUATION
				 *
				 * @param out             выходной буфер соединения
				 * @param streamId        идентификатор потока
				 * @param block           блок заголовков HPACK целиком
				 * @param endStream       флаг завершения потока
				 * @param maxFramePayload максимальный размер полезной нагрузки одного фрейма (SETTINGS_MAX_FRAME_SIZE пира)
				 */
				void serializeHeaderBlock(string & out, const uint32_t streamId, string_view block, const bool endStream, const uint32_t maxFramePayload) noexcept;
				/**
				 * @brief Функция сборки HPACK-блока обещанного запроса в PUSH_PROMISE + CONTINUATION
				 *
				 * @note Первый фрейм резервирует 4 октета под Promised Stream ID;
				 *       остаток блока уходит в CONTINUATION при необходимости
				 *
				 * @param out              выходной буфер соединения
				 * @param streamId         идентификатор ассоциированного потока клиента
				 * @param promisedStreamId идентификатор обещанного потока
				 * @param block            блок заголовков HPACK целиком
				 * @param maxFramePayload  максимальный размер полезной нагрузки одного фрейма
				 */
				void serializePushPromiseBlock(string & out, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const uint32_t maxFramePayload) noexcept;
				/**
				 * @brief Функция сборки фрейма PRIORITY (заголовок + нагрузка дописываются в out)
				 *
				 * @param out       выходной буфер соединения
				 * @param streamId  идентификатор потока
				 * @param exclusive флаг эксклюзивной зависимости потока
				 * @param streamDep идентификатор потока, от которого зависит текущий
				 * @param weight    вес потока
				 */
				void serializePriority(string & out, const uint32_t streamId, const bool exclusive, const uint32_t streamDep, const uint8_t weight) noexcept;
				/**
				 * @brief Функция сборки фрейма RST_STREAM (заголовок + нагрузка дописываются в out)
				 *
				 * @param out      выходной буфер соединения
				 * @param streamId идентификатор потока
				 * @param code     код ошибки, с которым сбрасывается поток
				 */
				void serializeRstStream(string & out, const uint32_t streamId, const error_t code) noexcept;
				/**
				 * @brief Функция сборки фрейма SETTINGS (заголовок + нагрузка дописываются в out)
				 *
				 * @param out   выходной буфер соединения
				 * @param items список параметров (для ACK игнорируется)
				 * @param count количество параметров
				 * @param ack   флаг подтверждения получения SETTINGS пира
				 */
				void serializeSettings(string & out, const setting_entry_t * items, const size_t count, const bool ack) noexcept;
				/**
				 * @brief Функция сборки фрейма PUSH_PROMISE (заголовок + нагрузка дописываются в out)
				 *
				 * @param out              выходной буфер соединения
				 * @param streamId         идентификатор ассоциированного потока клиента
				 * @param promisedStreamId идентификатор обещанного потока
				 * @param block            фрагмент блока заголовков HPACK
				 * @param endHeaders       флаг завершения блока заголовков
				 */
				void serializePushPromise(string & out, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const bool endHeaders) noexcept;
				/**
				 * @brief Функция сборки фрейма PING (заголовок + нагрузка дописываются в out)
				 *
				 * @param out    выходной буфер соединения
				 * @param opaque произвольные opaque-данные (8 байт)
				 * @param ack    флаг подтверждения получения PING пира
				 */
				void serializePing(string & out, const uint8_t opaque[8], const bool ack) noexcept;
				/**
				 * @brief Функция сборки фрейма GOAWAY (заголовок + нагрузка дописываются в out)
				 *
				 * @param out          выходной буфер соединения
				 * @param lastStreamId наибольший идентификатор обработанного потока
				 * @param code         код ошибки завершения соединения
				 * @param debugData    необязательные отладочные данные
				 */
				void serializeGoaway(string & out, const uint32_t lastStreamId, const error_t code, string_view debugData) noexcept;
				/**
				 * @brief Функция сборки фрейма WINDOW_UPDATE (заголовок + нагрузка дописываются в out)
				 *
				 * @param out       выходной буфер соединения
				 * @param streamId  идентификатор потока (0 - окно всего соединения)
				 * @param increment инкремент окна flow control
				 */
				void serializeWindowUpdate(string & out, const uint32_t streamId, const uint32_t increment) noexcept;
			}
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2_FRAME__
