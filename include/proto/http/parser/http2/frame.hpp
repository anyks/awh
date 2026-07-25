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
 * Подключаем заголовочные файлы проекта
 */
#include "h2.hpp"
#include "../../../../sys/global.hpp"

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
				 * @brief Структура параметра SETTINGS (идентификатор + значение)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Setting {
					// Идентификатор параметра
					setting_t id;
					// Значение параметра
					uint32_t value;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Setting() noexcept;
				} setting_entry_t;
				/**
				 * @brief Структура полезной нагрузки DATA (RFC 9113 §6.1), padding уже снят
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Data {
					// Флаг завершения потока
					bool endStream;
					// Данные тела (zero-copy во входной буфер)
					string_view data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Data() noexcept;
				} data_t;
				/**
				 * @brief Структура полезной нагрузки PRIORITY (RFC 9113 §6.3)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Priority {
					// Флаг эксклюзивной зависимости потока
					bool exclusive;
					// Вес потока
					uint8_t weight;
					// Идентификатор потока, от которого зависит текущий
					uint32_t streamDep;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Priority() noexcept;
				} priority_t;
				/**
				 * @brief Структура полезной нагрузки GOAWAY (RFC 9113 §6.8)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Goaway {
					// Код ошибки завершения соединения
					error_t code;
					// Наибольший идентификатор обработанного потока
					uint32_t lastStreamId;
					// Необязательные отладочные данные (zero-copy во входной буфер)
					string_view debugData;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Goaway() noexcept;
				} goaway_t;
				/**
				 * @brief Структура разобранного заголовка фрейма (RFC 9113 §4.1)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header {
					// Тип фрейма
					frame_t type;
					// Флаги фрейма (семантика зависит от типа)
					uint8_t flags;
					// Длина полезной нагрузки (24 бита)
					uint32_t length;
					// Идентификатор потока (31 бит)
					uint32_t streamId;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Header() noexcept;
				} header_t;
				/**
				 * @brief Структура полезной нагрузки HEADERS (RFC 9113 §6.2), padding уже снят
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Headers {
					// Флаг эксклюзивной зависимости потока
					bool exclusive;
					// Флаг завершения потока
					bool endStream;
					// Флаг завершения блока заголовков
					bool endHeaders;
					// Флаг наличия полей приоритета (RFC 7540, deprecated)
					bool hasPriority;
					// Вес потока (фактический вес = weight + 1)
					uint8_t weight;
					// Идентификатор потока, от которого зависит текущий
					uint32_t streamDep;
					// Фрагмент блока заголовков HPACK (zero-copy во входной буфер)
					string_view block;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Headers() noexcept;
				} headers_t;
				/**
				 * @brief Структура полезной нагрузки PUSH_PROMISE (RFC 9113 §6.6), padding уже снят
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Push_Promise {
					// Флаг завершения блока заголовков
					bool endHeaders;
					// Идентификатор обещанного потока
					uint32_t promisedStreamId;
					// Фрагмент блока заголовков HPACK (zero-copy во входной буфер)
					string_view block;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Push_Promise() noexcept;
				} push_promise_t;

				/**
				 * @brief Пространство имён функций разбора полезной нагрузки фреймов HTTP/2 (RFC 9113 §4-6)
				 *
				 */
				namespace parser {
					/**
					 * @brief Функция разбора 9-байтового заголовка фрейма
					 *
					 * @param data   входной буфер
					 * @param size   доступно байт
					 * @param output разобранный заголовок фрейма
					 * @return       результат разбора (true - в буфере было достаточно байт и заголовок разобран)
					 */
					__AWH_SHARED_EXPORT__ bool header(const uint8_t * data, const size_t size, header_t & output) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки DATA
					 *
					 * @note Параметр payload должен указывать на ровно h.length байт нагрузки
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола (PROTOCOL_ERROR на некорректном padding)
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t data(const header_t & header, const uint8_t * payload, data_t & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки PING (требует ровно 8 байт opaque-данных)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param opaque  извлечённые opaque-данные
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t ping(const header_t & header, const uint8_t * payload, uint8_t opaque[8], error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки RST_STREAM (требует ровно 4 байта)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param code    код ошибки, с которым сброшен поток
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t rstStream(const header_t & header, const uint8_t * payload, error_t & code, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки GOAWAY (минимум 8 байт)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t goaway(const header_t & header, const uint8_t * payload, goaway_t & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки HEADERS (с учётом padding и приоритета)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t headers(const header_t & header, const uint8_t * payload, headers_t & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки PRIORITY (требует ровно 5 байт)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t priority(const header_t & header, const uint8_t * payload, priority_t & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки WINDOW_UPDATE (требует ровно 4 байта)
					 *
					 * @note Нулевой инкремент - PROTOCOL_ERROR (на уровне потока - RST_STREAM)
					 *
					 * @param header    заголовок фрейма
					 * @param payload   полезная нагрузка фрейма
					 * @param increment извлечённый инкремент окна
					 * @param error     код ошибки протокола
					 * @return          результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t windowUpdate(const header_t & header, const uint8_t * payload, uint32_t & increment, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки PUSH_PROMISE (с учётом padding)
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t pushPromise(const header_t & header, const uint8_t * payload, push_promise_t & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки SETTINGS (длина кратна 6)
					 *
					 * @note Для ACK-фрейма нагрузка должна быть пустой. Параметры дописываются в output
					 *
					 * @param header  заголовок фрейма
					 * @param payload полезная нагрузка фрейма
					 * @param output  список разобранных параметров
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t settings(const header_t & header, const uint8_t * payload, vector <setting_entry_t> & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки CONTINUATION (фрагмент блока заголовков)
					 *
					 * @param header     заголовок фрейма
					 * @param payload    полезная нагрузка фрейма
					 * @param block      фрагмент блока заголовков (zero-copy во входной буфер)
					 * @param endHeaders флаг завершения блока заголовков
					 * @param err        код ошибки протокола
					 * @return           результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t continuation(const header_t & header, const uint8_t * payload, string_view & block, bool & endHeaders, error_t & err) noexcept;
					/**
					 * @brief Функция разбора полезной нагрузки PRIORITY_UPDATE (RFC 9218 §7.1)
					 *
					 * @note Кадр относится к соединению (stream id == 0), а приоритизируемый
					 *       поток указан в первых 4 октетах нагрузки
					 *
					 * @param header   заголовок фрейма
					 * @param payload  полезная нагрузка фрейма
					 * @param streamId идентификатор приоритизируемого потока
					 * @param value    значение поля приоритета (zero-copy во входной буфер)
					 * @param error    код ошибки протокола
					 * @return         результат разбора (OK/ERROR)
					 */
					__AWH_SHARED_EXPORT__ status_t priorityUpdate(const header_t & header, const uint8_t * payload, uint32_t & streamId, string_view & value, error_t & error) noexcept;
				};

				/**
				 * @brief Пространство имён функций сборки полезной нагрузки фреймов HTTP/2 (RFC 9113 §4-6)
				 *
				 */
				namespace serialize {
					/**
					 * @brief Функция сборки фрейма PING (заголовок + нагрузка дописываются в output)
					 *
					 * @param output выходной буфер соединения
					 * @param opaque произвольные opaque-данные (8 байт)
					 * @param ack    флаг подтверждения получения PING пира
					 */
					__AWH_SHARED_EXPORT__ void ping(string & output, const uint8_t opaque[8], const bool ack) noexcept;
					/**
					 * @brief Функция сборки фрейма RST_STREAM (заголовок + нагрузка дописываются в output)
					 *
					 * @param output   выходной буфер соединения
					 * @param streamId идентификатор потока
					 * @param error    код ошибки, с которым сбрасывается поток
					 */
					__AWH_SHARED_EXPORT__ void rstStream(string & output, const uint32_t streamId, const error_t error) noexcept;
					/**
					 * @brief Функция сборки фрейма WINDOW_UPDATE (заголовок + нагрузка дописываются в output)
					 *
					 * @param output    выходной буфер соединения
					 * @param streamId  идентификатор потока (0 - окно всего соединения)
					 * @param increment инкремент окна flow control
					 */
					__AWH_SHARED_EXPORT__ void windowUpdate(string & output, const uint32_t streamId, const uint32_t increment) noexcept;
					/**
					 * @brief Функция сборки фрейма DATA (заголовок + нагрузка дописываются в output)
					 *
					 * @param output    выходной буфер соединения
					 * @param streamId  идентификатор потока
					 * @param data      данные тела
					 * @param endStream флаг завершения потока
					 */
					__AWH_SHARED_EXPORT__ void data(string & output, const uint32_t streamId, string_view data, const bool endStream) noexcept;
					/**
					 * @brief Функция сборки фрейма SETTINGS (заголовок + нагрузка дописываются в output)
					 *
					 * @param output выходной буфер соединения
					 * @param items  список параметров (для ACK игнорируется)
					 * @param count  количество параметров
					 * @param ack    флаг подтверждения получения SETTINGS пира
					 */
					__AWH_SHARED_EXPORT__ void settings(string & output, const setting_entry_t * items, const size_t count, const bool ack) noexcept;
					/**
					 * @brief Функция сборки фрейма GOAWAY (заголовок + нагрузка дописываются в output)
					 *
					 * @param output       выходной буфер соединения
					 * @param lastStreamId наибольший идентификатор обработанного потока
					 * @param error        код ошибки завершения соединения
					 * @param debugData    необязательные отладочные данные
					 */
					__AWH_SHARED_EXPORT__ void goaway(string & output, const uint32_t lastStreamId, const error_t error, string_view debugData) noexcept;
					/**
					 * @brief Функция сборки фрейма CONTINUATION (заголовок + нагрузка дописываются в output)
					 *
					 * @param output     выходной буфер соединения
					 * @param streamId   идентификатор потока
					 * @param block      фрагмент блока заголовков HPACK
					 * @param endHeaders флаг завершения блока заголовков
					 */
					__AWH_SHARED_EXPORT__ void continuation(string & output, const uint32_t streamId, string_view block, const bool endHeaders) noexcept;
					/**
					 * @brief Функция сборки фрейма HEADERS (заголовок + нагрузка дописываются в output)
					 *
					 * @param output     выходной буфер соединения
					 * @param streamId   идентификатор потока
					 * @param block      фрагмент блока заголовков HPACK
					 * @param endStream  флаг завершения потока
					 * @param endHeaders флаг завершения блока заголовков
					 */
					__AWH_SHARED_EXPORT__ void headers(string & output, const uint32_t streamId, string_view block, const bool endStream, const bool endHeaders) noexcept;
					/**
					 * @brief Функция сборки фрейма PRIORITY (заголовок + нагрузка дописываются в output)
					 *
					 * @param output    выходной буфер соединения
					 * @param streamId  идентификатор потока
					 * @param exclusive флаг эксклюзивной зависимости потока
					 * @param streamDep идентификатор потока, от которого зависит текущий
					 * @param weight    вес потока
					 */
					__AWH_SHARED_EXPORT__ void priority(string & output, const uint32_t streamId, const bool exclusive, const uint32_t streamDep, const uint8_t weight) noexcept;
					/**
					 * @brief Функция сборки фрейма PRIORITY_UPDATE (RFC 9218 §7.1)
					 *
					 * @param output   выходной буфер соединения
					 * @param streamId идентификатор приоритизируемого потока
					 * @param value    значение поля приоритета (структурированный словарь, например "u=2, i")
					 */
					__AWH_SHARED_EXPORT__ void priorityUpdate(string & output, const uint32_t streamId, string_view value) noexcept;
					/**
					 * @brief Функция сборки HPACK-блока в HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10)
					 *
					 * @note END_STREAM, если задан, ставится только на первый HEADERS -
					 *       даже если блок продолжается в CONTINUATION
					 *
					 * @param output          выходной буфер соединения
					 * @param streamId        идентификатор потока
					 * @param block           блок заголовков HPACK целиком
					 * @param endStream       флаг завершения потока
					 * @param maxFramePayload максимальный размер полезной нагрузки одного фрейма (SETTINGS_MAX_FRAME_SIZE пира)
					 */
					__AWH_SHARED_EXPORT__ void headerBlock(string & output, const uint32_t streamId, string_view block, const bool endStream, const uint32_t maxFramePayload) noexcept;
					/**
					 * @brief Функция сборки фрейма PUSH_PROMISE (заголовок + нагрузка дописываются в output)
					 *
					 * @param output           выходной буфер соединения
					 * @param streamId         идентификатор ассоциированного потока клиента
					 * @param promisedStreamId идентификатор обещанного потока
					 * @param block            фрагмент блока заголовков HPACK
					 * @param endHeaders       флаг завершения блока заголовков
					 */
					__AWH_SHARED_EXPORT__ void pushPromise(string & output, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const bool endHeaders) noexcept;
					/**
					 * @brief Функция сборки HPACK-блока обещанного запроса в PUSH_PROMISE + CONTINUATION
					 *
					 * @note Первый фрейм резервирует 4 октета под Promised Stream ID;
					 *       остаток блока уходит в CONTINUATION при необходимости
					 *
					 * @param output           выходной буфер соединения
					 * @param streamId         идентификатор ассоциированного потока клиента
					 * @param promisedStreamId идентификатор обещанного потока
					 * @param block            блок заголовков HPACK целиком
					 * @param maxFramePayload  максимальный размер полезной нагрузки одного фрейма
					 */
					__AWH_SHARED_EXPORT__ void pushPromiseBlock(string & output, const uint32_t streamId, const uint32_t promisedStreamId, string_view block, const uint32_t maxFramePayload) noexcept;
				};
			}
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP2_FRAME__
