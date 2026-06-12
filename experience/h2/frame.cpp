/**
 * @file frame.cpp
 * @brief Реализация framing-слоя HTTP/2 (RFC 9113 §4–6).
 *
 * Сборка:
 *   g++ -std=c++17 -O2 -Wall -Wextra -c frame.cpp
 */

#include "frame.hpp"

#include <algorithm>

namespace awh {
	namespace http2 {
		namespace frame {
			/**
			 * Внутренние помощники чтения/записи в сетевом (big-endian) порядке.
			 */
			namespace {
				inline uint16_t rd16(const uint8_t * p) noexcept {
					return static_cast <uint16_t> ((static_cast <uint16_t> (p[0]) << 8) | p[1]);
				}
				inline uint32_t rd24(const uint8_t * p) noexcept {
					return (static_cast <uint32_t> (p[0]) << 16) | (static_cast <uint32_t> (p[1]) << 8) | p[2];
				}
				inline uint32_t rd32(const uint8_t * p) noexcept {
					return (static_cast <uint32_t> (p[0]) << 24) | (static_cast <uint32_t> (p[1]) << 16) |
					       (static_cast <uint32_t> (p[2]) << 8)  |  static_cast <uint32_t> (p[3]);
				}
				inline void wr16(std::string & out, uint16_t v) noexcept {
					out.push_back(static_cast <char> ((v >> 8) & 0xFF));
					out.push_back(static_cast <char> (v & 0xFF));
				}
				inline void wr24(std::string & out, uint32_t v) noexcept {
					out.push_back(static_cast <char> ((v >> 16) & 0xFF));
					out.push_back(static_cast <char> ((v >> 8) & 0xFF));
					out.push_back(static_cast <char> (v & 0xFF));
				}
				inline void wr32(std::string & out, uint32_t v) noexcept {
					out.push_back(static_cast <char> ((v >> 24) & 0xFF));
					out.push_back(static_cast <char> ((v >> 16) & 0xFF));
					out.push_back(static_cast <char> ((v >> 8) & 0xFF));
					out.push_back(static_cast <char> (v & 0xFF));
				}
				/**
				 * Дописать 9-байтовый заголовок фрейма.
				 */
				inline void wrHeader(std::string & out, uint32_t length, frame_t type, uint8_t flags, uint32_t streamId) noexcept {
					wr24(out, length);
					out.push_back(static_cast <char> (type));
					out.push_back(static_cast <char> (flags));
					wr32(out, streamId & proto::STREAM_ID_MASK);
				}
				/**
				 * Снять padding с нагрузки фреймов DATA/HEADERS/PUSH_PROMISE.
				 *
				 * При наличии флага PADDED первый байт — Pad Length; в хвосте идёт padding.
				 * @return false, если Pad Length некорректен (>= оставшейся длины) — PROTOCOL_ERROR.
				 */
				inline bool stripPadding(bool padded, const uint8_t *& p, size_t & len) noexcept {
					if(!padded) return true;
					if(len < 1) return false;
					const uint8_t padLen = p[0];
					p += 1;
					len -= 1;
					if(padLen > len) return false; // padding не помещается в нагрузку
					len -= padLen;
					return true;
				}
			}

			// ───────────────────────── Разбор ─────────────────────────

			bool parseHeader(const uint8_t * data, size_t size, header_t & out) noexcept {
				if(size < proto::FRAME_HEADER_SIZE) return false;
				out.length   = rd24(data);
				out.type     = static_cast <frame_t> (data[3]);
				out.flags    = data[4];
				out.streamId = rd32(data + 5) & proto::STREAM_ID_MASK;
				return true;
			}

			status_t parseData(const header_t & h, const uint8_t * payload, data_t & out, error_t & err) noexcept {
				// DATA обязан принадлежать потоку (stream id != 0)
				if(h.streamId == 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				const uint8_t * p = payload;
				size_t len = h.length;
				if(!stripPadding(h.flags & flag::PADDED, p, len)) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				out.data      = std::string_view(reinterpret_cast <const char *> (p), len);
				out.endStream = (h.flags & flag::END_STREAM) != 0;
				return status_t::OK;
			}

			status_t parseHeaders(const header_t & h, const uint8_t * payload, headers_t & out, error_t & err) noexcept {
				if(h.streamId == 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				const uint8_t * p = payload;
				size_t len = h.length;
				if(!stripPadding(h.flags & flag::PADDED, p, len)) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				out.endStream  = (h.flags & flag::END_STREAM) != 0;
				out.endHeaders = (h.flags & flag::END_HEADERS) != 0;
				out.hasPriority = (h.flags & flag::PRIORITY) != 0;
				if(out.hasPriority){
					if(len < 5) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
					const uint32_t dep = rd32(p);
					out.exclusive = (dep & 0x80000000u) != 0;
					out.streamDep = dep & proto::STREAM_ID_MASK;
					out.weight    = p[4];
					p += 5;
					len -= 5;
				}
				out.block = std::string_view(reinterpret_cast <const char *> (p), len);
				return status_t::OK;
			}

			status_t parsePriority(const header_t & h, const uint8_t * payload, priority_t & out, error_t & err) noexcept {
				if(h.streamId == 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				if(h.length != 5) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
				const uint32_t dep = rd32(payload);
				out.exclusive = (dep & 0x80000000u) != 0;
				out.streamDep = dep & proto::STREAM_ID_MASK;
				out.weight    = payload[4];
				return status_t::OK;
			}

			status_t parseRstStream(const header_t & h, const uint8_t * payload, error_t & code, error_t & err) noexcept {
				if(h.streamId == 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				if(h.length != 4) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
				code = static_cast <error_t> (rd32(payload));
				return status_t::OK;
			}

			status_t parseSettings(const header_t & h, const uint8_t * payload, std::vector <setting_entry_t> & out, error_t & err) noexcept {
				// SETTINGS относится к соединению (stream id == 0)
				if(h.streamId != 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				if(h.flags & flag::ACK){
					// ACK обязан быть пустым
					if(h.length != 0) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
					return status_t::OK;
				}
				if((h.length % 6) != 0) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
				const size_t count = h.length / 6;
				out.reserve(out.size() + count);
				const uint8_t * p = payload;
				for(size_t i = 0; i < count; ++i){
					setting_entry_t e;
					e.id    = static_cast <setting_t> (rd16(p));
					e.value = rd32(p + 2);
					out.push_back(e);
					p += 6;
				}
				return status_t::OK;
			}

			status_t parsePushPromise(const header_t & h, const uint8_t * payload, push_promise_t & out, error_t & err) noexcept {
				if(h.streamId == 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				const uint8_t * p = payload;
				size_t len = h.length;
				if(!stripPadding(h.flags & flag::PADDED, p, len)) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				if(len < 4) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
				out.promisedStreamId = rd32(p) & proto::STREAM_ID_MASK;
				out.endHeaders = (h.flags & flag::END_HEADERS) != 0;
				out.block = std::string_view(reinterpret_cast <const char *> (p + 4), len - 4);
				return status_t::OK;
			}

			status_t parsePing(const header_t & h, const uint8_t * payload, uint8_t opaque[8], error_t & err) noexcept {
				if(h.streamId != 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				if(h.length != 8) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
				for(int i = 0; i < 8; ++i) opaque[i] = payload[i];
				return status_t::OK;
			}

			status_t parseGoaway(const header_t & h, const uint8_t * payload, goaway_t & out, error_t & err) noexcept {
				if(h.streamId != 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				if(h.length < 8) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
				out.lastStreamId = rd32(payload) & proto::STREAM_ID_MASK;
				out.code         = static_cast <error_t> (rd32(payload + 4));
				out.debugData    = std::string_view(reinterpret_cast <const char *> (payload + 8), h.length - 8);
				return status_t::OK;
			}

			status_t parseWindowUpdate(const header_t & h, const uint8_t * payload, uint32_t & increment, error_t & err) noexcept {
				if(h.length != 4) { err = error_t::FRAME_SIZE_ERROR; return status_t::ERROR; }
				increment = rd32(payload) & proto::STREAM_ID_MASK; // сбрасываем reserved-бит
				if(increment == 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				return status_t::OK;
			}

			status_t parseContinuation(const header_t & h, const uint8_t * payload, std::string_view & block, bool & endHeaders, error_t & err) noexcept {
				if(h.streamId == 0) { err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
				block = std::string_view(reinterpret_cast <const char *> (payload), h.length);
				endHeaders = (h.flags & flag::END_HEADERS) != 0;
				return status_t::OK;
			}

			// ───────────────────────── Сборка ─────────────────────────

			void serializeData(std::string & out, uint32_t streamId, std::string_view data, bool endStream) noexcept {
				wrHeader(out, static_cast <uint32_t> (data.size()), frame_t::DATA, endStream ? flag::END_STREAM : flag::NONE, streamId);
				out.append(data.data(), data.size());
			}

			void serializeHeaders(std::string & out, uint32_t streamId, std::string_view block, bool endStream, bool endHeaders) noexcept {
				uint8_t flags = flag::NONE;
				if(endStream)  flags |= flag::END_STREAM;
				if(endHeaders) flags |= flag::END_HEADERS;
				wrHeader(out, static_cast <uint32_t> (block.size()), frame_t::HEADERS, flags, streamId);
				out.append(block.data(), block.size());
			}

			void serializeContinuation(std::string & out, uint32_t streamId, std::string_view block, bool endHeaders) noexcept {
				wrHeader(out, static_cast <uint32_t> (block.size()), frame_t::CONTINUATION, endHeaders ? flag::END_HEADERS : flag::NONE, streamId);
				out.append(block.data(), block.size());
			}

			void serializeHeaderBlock(std::string & out, uint32_t streamId, std::string_view block, bool endStream, uint32_t maxFramePayload) noexcept {
				if((maxFramePayload == 0) || (maxFramePayload > proto::MAX_FRAME_LENGTH))
					maxFramePayload = proto::DEFAULT_MAX_FRAME_SIZE;
				const size_t maxChunk = static_cast <size_t> (maxFramePayload);
				if(block.size() <= maxChunk){
					serializeHeaders(out, streamId, block, endStream, true);
					return;
				}
				size_t off = 0;
				bool first = true;
				while(off < block.size()){
					const size_t chunk = std::min(block.size() - off, maxChunk);
					const bool last = (off + chunk >= block.size());
					const std::string_view frag(block.data() + off, chunk);
					if(first){
						serializeHeaders(out, streamId, frag, endStream, last);
						first = false;
					} else serializeContinuation(out, streamId, frag, last);
					off += chunk;
				}
			}

			void serializePushPromiseBlock(std::string & out, uint32_t streamId, uint32_t promisedStreamId, std::string_view block, uint32_t maxFramePayload) noexcept {
				if((maxFramePayload == 0) || (maxFramePayload > proto::MAX_FRAME_LENGTH))
					maxFramePayload = proto::DEFAULT_MAX_FRAME_SIZE;
				if(maxFramePayload < 4) maxFramePayload = 4;
				const size_t firstMax = static_cast <size_t> (maxFramePayload) - 4;
				if(block.size() <= firstMax){
					serializePushPromise(out, streamId, promisedStreamId, block, true);
					return;
				}
				const size_t firstChunk = firstMax;
				serializePushPromise(out, streamId, promisedStreamId, block.substr(0, firstChunk), false);
				size_t off = firstChunk;
				const size_t maxChunk = static_cast <size_t> (maxFramePayload);
				while(off < block.size()){
					const size_t chunk = std::min(block.size() - off, maxChunk);
					const bool last = (off + chunk >= block.size());
					serializeContinuation(out, streamId, std::string_view(block.data() + off, chunk), last);
					off += chunk;
				}
			}

			void serializePriority(std::string & out, uint32_t streamId, bool exclusive, uint32_t streamDep, uint8_t weight) noexcept {
				wrHeader(out, 5, frame_t::PRIORITY, flag::NONE, streamId);
				wr32(out, (streamDep & proto::STREAM_ID_MASK) | (exclusive ? 0x80000000u : 0));
				out.push_back(static_cast <char> (weight));
			}

			void serializeRstStream(std::string & out, uint32_t streamId, error_t code) noexcept {
				wrHeader(out, 4, frame_t::RST_STREAM, flag::NONE, streamId);
				wr32(out, static_cast <uint32_t> (code));
			}

			void serializeSettings(std::string & out, const setting_entry_t * items, size_t count, bool ack) noexcept {
				if(ack){
					wrHeader(out, 0, frame_t::SETTINGS, flag::ACK, 0);
					return;
				}
				wrHeader(out, static_cast <uint32_t> (count * 6), frame_t::SETTINGS, flag::NONE, 0);
				for(size_t i = 0; i < count; ++i){
					wr16(out, static_cast <uint16_t> (items[i].id));
					wr32(out, items[i].value);
				}
			}

			void serializePushPromise(std::string & out, uint32_t streamId, uint32_t promisedStreamId, std::string_view block, bool endHeaders) noexcept {
				wrHeader(out, static_cast <uint32_t> (block.size() + 4), frame_t::PUSH_PROMISE, endHeaders ? flag::END_HEADERS : flag::NONE, streamId);
				wr32(out, promisedStreamId & proto::STREAM_ID_MASK);
				out.append(block.data(), block.size());
			}

			void serializePing(std::string & out, const uint8_t opaque[8], bool ack) noexcept {
				wrHeader(out, 8, frame_t::PING, ack ? flag::ACK : flag::NONE, 0);
				out.append(reinterpret_cast <const char *> (opaque), 8);
			}

			void serializeGoaway(std::string & out, uint32_t lastStreamId, error_t code, std::string_view debugData) noexcept {
				wrHeader(out, static_cast <uint32_t> (debugData.size() + 8), frame_t::GOAWAY, flag::NONE, 0);
				wr32(out, lastStreamId & proto::STREAM_ID_MASK);
				wr32(out, static_cast <uint32_t> (code));
				out.append(debugData.data(), debugData.size());
			}

			void serializeWindowUpdate(std::string & out, uint32_t streamId, uint32_t increment) noexcept {
				wrHeader(out, 4, frame_t::WINDOW_UPDATE, flag::NONE, streamId);
				wr32(out, increment & proto::STREAM_ID_MASK);
			}
		}
	}
}
