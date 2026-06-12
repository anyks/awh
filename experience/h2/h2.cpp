/**
 * @file h2.cpp
 * @brief Вспомогательные функции базового слоя HTTP/2 (человекочитаемые имена).
 */

#include "h2.hpp"

namespace awh {
	namespace http2 {
		const char * frameName(frame_t type) noexcept {
			switch(type){
				case frame_t::DATA:          return "DATA";
				case frame_t::HEADERS:       return "HEADERS";
				case frame_t::PRIORITY:      return "PRIORITY";
				case frame_t::RST_STREAM:    return "RST_STREAM";
				case frame_t::SETTINGS:      return "SETTINGS";
				case frame_t::PUSH_PROMISE:  return "PUSH_PROMISE";
				case frame_t::PING:          return "PING";
				case frame_t::GOAWAY:        return "GOAWAY";
				case frame_t::WINDOW_UPDATE: return "WINDOW_UPDATE";
				case frame_t::CONTINUATION:  return "CONTINUATION";
			}
			return "UNKNOWN";
		}

		const char * errorName(error_t code) noexcept {
			switch(code){
				case error_t::NO_ERROR:            return "NO_ERROR";
				case error_t::PROTOCOL_ERROR:      return "PROTOCOL_ERROR";
				case error_t::INTERNAL_ERROR:      return "INTERNAL_ERROR";
				case error_t::FLOW_CONTROL_ERROR:  return "FLOW_CONTROL_ERROR";
				case error_t::SETTINGS_TIMEOUT:    return "SETTINGS_TIMEOUT";
				case error_t::STREAM_CLOSED:       return "STREAM_CLOSED";
				case error_t::FRAME_SIZE_ERROR:    return "FRAME_SIZE_ERROR";
				case error_t::REFUSED_STREAM:      return "REFUSED_STREAM";
				case error_t::CANCEL:              return "CANCEL";
				case error_t::COMPRESSION_ERROR:   return "COMPRESSION_ERROR";
				case error_t::CONNECT_ERROR:       return "CONNECT_ERROR";
				case error_t::ENHANCE_YOUR_CALM:   return "ENHANCE_YOUR_CALM";
				case error_t::INADEQUATE_SECURITY: return "INADEQUATE_SECURITY";
				case error_t::HTTP_1_1_REQUIRED:   return "HTTP_1_1_REQUIRED";
			}
			return "UNKNOWN_ERROR";
		}
	}
}
