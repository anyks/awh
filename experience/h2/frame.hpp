/**
 * @file frame.hpp
 * @brief Framing-слой HTTP/2 (RFC 9113 §4–6): разбор и сборка фреймов.
 *
 * Разбор zero-copy: полезная нагрузка отдаётся как std::string_view с указателем
 * во входной буфер. Сборка пишет байты в std::string (выходной буфер соединения).
 *
 * Слой не хранит состояния соединения — это чистые функции над байтами. Логика
 * состояний потоков, flow control и HPACK живёт в session.hpp / hpack.hpp.
 */

#ifndef AWH_EXPERIENCE_H2_FRAME_HPP
#define AWH_EXPERIENCE_H2_FRAME_HPP

#include "h2.hpp"

#include <vector>

namespace awh {
	namespace http2 {
		namespace frame {
			/**
			 * @brief Разобранный заголовок фрейма (RFC 9113 §4.1).
			 */
			struct header_t {
				uint32_t length   = 0;                  // длина полезной нагрузки (24 бита)
				frame_t  type     = frame_t::DATA;      // тип фрейма
				uint8_t  flags    = flag::NONE;         // флаги (семантика зависит от типа)
				uint32_t streamId = 0;                  // идентификатор потока (31 бит)
			};

			/**
			 * @brief Параметр SETTINGS (id + значение).
			 */
			struct setting_entry_t {
				setting_t id;
				uint32_t  value;
			};

			/**
			 * @brief Полезная нагрузка DATA (§6.1), padding уже снят.
			 */
			struct data_t {
				std::string_view data;          // данные тела (zero-copy)
				bool             endStream = false;
			};

			/**
			 * @brief Полезная нагрузка HEADERS (§6.2), padding уже снят.
			 */
			struct headers_t {
				std::string_view block;             // фрагмент блока заголовков (HPACK), zero-copy
				bool             endStream  = false;
				bool             endHeaders = false;
				// поля приоритета (только если был флаг PRIORITY) — RFC 7540, deprecated
				bool             hasPriority = false;
				bool             exclusive   = false;
				uint32_t         streamDep   = 0;
				uint8_t          weight      = 0;   // фактический вес = weight + 1
			};

			/**
			 * @brief Полезная нагрузка PRIORITY (§6.3).
			 */
			struct priority_t {
				bool     exclusive = false;
				uint32_t streamDep = 0;
				uint8_t  weight    = 0;
			};

			/**
			 * @brief Полезная нагрузка PUSH_PROMISE (§6.6), padding уже снят.
			 */
			struct push_promise_t {
				uint32_t         promisedStreamId = 0;
				std::string_view block;                 // фрагмент блока заголовков (HPACK)
				bool             endHeaders = false;
			};

			/**
			 * @brief Полезная нагрузка GOAWAY (§6.8).
			 */
			struct goaway_t {
				uint32_t         lastStreamId = 0;
				error_t          code         = error_t::NO_ERROR;
				std::string_view debugData;             // необязательные отладочные данные
			};

			// ───────────────────────── Разбор (parse) ─────────────────────────

			/**
			 * @brief Разобрать 9-байтовый заголовок фрейма.
			 *
			 * @param data входной буфер
			 * @param size доступно байт
			 * @param out  [out] разобранный заголовок
			 * @return true, если в буфере было >= 9 байт и заголовок разобран
			 */
			bool parseHeader(const uint8_t * data, size_t size, header_t & out) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку DATA.
			 *
			 * Параметр payload должен указывать на ровно h.length байт нагрузки.
			 * @return OK / ERROR (err заполняется при ошибке: PROTOCOL_ERROR на некорректном padding)
			 */
			status_t parseData(const header_t & h, const uint8_t * payload, data_t & out, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку HEADERS (с учётом padding и приоритета).
			 */
			status_t parseHeaders(const header_t & h, const uint8_t * payload, headers_t & out, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку PRIORITY (требует ровно 5 байт).
			 */
			status_t parsePriority(const header_t & h, const uint8_t * payload, priority_t & out, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку RST_STREAM (требует ровно 4 байта).
			 */
			status_t parseRstStream(const header_t & h, const uint8_t * payload, error_t & code, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку SETTINGS (длина кратна 6).
			 *
			 * Для ACK-фрейма нагрузка должна быть пустой. Параметры дописываются в out.
			 */
			status_t parseSettings(const header_t & h, const uint8_t * payload, std::vector <setting_entry_t> & out, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку PUSH_PROMISE (с учётом padding).
			 */
			status_t parsePushPromise(const header_t & h, const uint8_t * payload, push_promise_t & out, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку PING (требует ровно 8 байт opaque-данных).
			 */
			status_t parsePing(const header_t & h, const uint8_t * payload, uint8_t opaque[8], error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку GOAWAY (минимум 8 байт).
			 */
			status_t parseGoaway(const header_t & h, const uint8_t * payload, goaway_t & out, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку WINDOW_UPDATE (требует ровно 4 байта).
			 *
			 * Нулевой инкремент — PROTOCOL_ERROR (на уровне потока — RST_STREAM).
			 */
			status_t parseWindowUpdate(const header_t & h, const uint8_t * payload, uint32_t & increment, error_t & err) noexcept;

			/**
			 * @brief Разобрать полезную нагрузку CONTINUATION (фрагмент блока заголовков).
			 */
			status_t parseContinuation(const header_t & h, const uint8_t * payload, std::string_view & block, bool & endHeaders, error_t & err) noexcept;

			// ───────────────────────── Сборка (serialize) ─────────────────────────
			// Все функции дописывают готовый фрейм (заголовок + нагрузка) в out.

			void serializeData(std::string & out, uint32_t streamId, std::string_view data, bool endStream) noexcept;
			void serializeHeaders(std::string & out, uint32_t streamId, std::string_view block, bool endStream, bool endHeaders) noexcept;
			void serializeContinuation(std::string & out, uint32_t streamId, std::string_view block, bool endHeaders) noexcept;

			/**
			 * @brief Сериализовать HPACK-блок в HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10).
			 *
			 * @param maxFramePayload максимальный размер полезной нагрузки одного фрейма
			 *        (SETTINGS_MAX_FRAME_SIZE пира). END_STREAM, если задан, ставится только
			 *        на первый HEADERS — даже если блок продолжается в CONTINUATION.
			 */
			void serializeHeaderBlock(std::string & out, uint32_t streamId, std::string_view block, bool endStream, uint32_t maxFramePayload) noexcept;

			/**
			 * @brief Сериализовать HPACK-блок обещанного запроса в PUSH_PROMISE + CONTINUATION.
			 *
			 * Первый фрейм резервирует 4 октета под Promised Stream ID; остаток блока
			 * уходит в CONTINUATION при необходимости.
			 */
			void serializePushPromiseBlock(std::string & out, uint32_t streamId, uint32_t promisedStreamId, std::string_view block, uint32_t maxFramePayload) noexcept;
			void serializePriority(std::string & out, uint32_t streamId, bool exclusive, uint32_t streamDep, uint8_t weight) noexcept;
			void serializeRstStream(std::string & out, uint32_t streamId, error_t code) noexcept;
			void serializeSettings(std::string & out, const setting_entry_t * items, size_t count, bool ack) noexcept;
			void serializePushPromise(std::string & out, uint32_t streamId, uint32_t promisedStreamId, std::string_view block, bool endHeaders) noexcept;
			void serializePing(std::string & out, const uint8_t opaque[8], bool ack) noexcept;
			void serializeGoaway(std::string & out, uint32_t lastStreamId, error_t code, std::string_view debugData) noexcept;
			void serializeWindowUpdate(std::string & out, uint32_t streamId, uint32_t increment) noexcept;
		}
	}
}

#endif // AWH_EXPERIENCE_H2_FRAME_HPP
