/**
 * @file session.cpp
 * @brief Реализация сессии HTTP/2 — скелет конечного автомата (RFC 9113 §5).
 *
 * Реализовано: разбор preface, буферизация неполных фреймов, диспетчеризация всех
 * типов фреймов, обвязка HPACK (HEADERS + CONTINUATION), SETTINGS/PING/GOAWAY,
 * state machine, flow control, DoS-защиты, HTTP-семантика, server push.
 *
 * Сборка:
 *   g++ -std=c++17 -O2 -Wall -Wextra -c session.cpp
 */

#include "session.hpp"

#include <algorithm>

namespace awh {
	namespace http2 {
		Session::Session(endpoint_t endpoint, const callbacks_t & callbacks, void * user) noexcept
		 : _endpoint(endpoint), _cb(callbacks), _user(user) {
			// Клиент инициирует нечётные потоки (1,3,5...), сервер — чётные (push: 2,4,6...).
			_nextStreamId = (endpoint == endpoint_t::CLIENT) ? 1 : 2;
			// Сервер ожидает клиентский preface; клиент отправляет его сам через submitPreface().
			_prefaceReceived = (endpoint == endpoint_t::SERVER) ? false : true;
			// Значения по умолчанию для rate-лимитов (как в nghttp2: burst 1000, rate 33/с для reset).
			_rstLimit.init(1000, 33);
			_ctrlLimit.init(1000, 100);
		}

		void Session::submitPreface() noexcept {
			if(_endpoint == endpoint_t::CLIENT)
				_output.append(proto::PREFACE.data(), proto::PREFACE.size());
			submitSettings(_local);
		}

		void Session::submitSettings(const settings_t & settings) noexcept {
			_local = settings;
			const frame::setting_entry_t items[] = {
				{ setting_t::HEADER_TABLE_SIZE,      _local.headerTableSize },
				{ setting_t::ENABLE_PUSH,            _local.enablePush },
				{ setting_t::INITIAL_WINDOW_SIZE,    static_cast <uint32_t> (_local.initialWindowSize) },
				{ setting_t::MAX_FRAME_SIZE,         _local.maxFrameSize }
			};
			frame::serializeSettings(_output, items, sizeof(items) / sizeof(items[0]), false);
		}

		void Session::submitWindowUpdate(uint32_t streamId, uint32_t increment) noexcept {
			frame::serializeWindowUpdate(_output, streamId, increment);
		}

		void Session::submitRstStream(uint32_t streamId, error_t code) noexcept {
			frame::serializeRstStream(_output, streamId, code);
			_streams.erase(streamId);
		}

		void Session::submitGoaway(error_t code, std::string_view debug) noexcept {
			frame::serializeGoaway(_output, _lastStreamId, code, debug);
			_goawaySent = true;
		}

		void Session::consumePending(size_t n) noexcept {
			if(n >= _output.size()) _output.clear();
			else _output.erase(0, n);
			// Выходной буфер просел — возможно, освободилось место под отложенные данные.
			if(_output.size() < _outputHighWater) pump();
		}

		stream_t & Session::stream(uint32_t id) noexcept {
			stream_t & s = _streams[id];
			if(s.id == 0){
				s.id = id;
				s.localWindow  = _local.initialWindowSize;
				s.remoteWindow = _remote.initialWindowSize;
			}
			return s;
		}

		stream_t * Session::findStream(uint32_t id) noexcept {
			const auto it = _streams.find(id);
			return (it == _streams.end()) ? nullptr : &it->second;
		}

		/**
		 * Валидация HTTP-семантики блока заголовков (RFC 9113 §8.1–8.3, §8.2.2).
		 * Возвращает NO_ERROR, если блок корректен, иначе PROTOCOL_ERROR (malformed).
		 */
		namespace {
			bool isLowercaseName(const std::string & n) noexcept {
				if(n.empty()) return false;
				for(const char ch : n) if((ch >= 'A') && (ch <= 'Z')) return false; // имена обязаны быть в нижнем регистре
				return true;
			}
			bool isConnectionSpecific(const std::string & n) noexcept {
				// RFC 9113 §8.2.2 — запрещённые в HTTP/2 connection-specific заголовки.
				return (n == "connection") || (n == "proxy-connection") || (n == "keep-alive")
				    || (n == "transfer-encoding") || (n == "upgrade");
			}
			error_t validateHeaders(const std::vector <hpack::field_t> & fields, bool isRequest, bool isTrailers) noexcept {
				bool seenRegular = false;
				bool hasMethod = false, hasScheme = false, hasPath = false, hasAuthority = false, hasStatus = false;
				std::string method;
				for(const hpack::field_t & f : fields){
					const std::string & n = f.name;
					if(n.empty()) return error_t::PROTOCOL_ERROR;
					if(n[0] == ':'){
						if(seenRegular) return error_t::PROTOCOL_ERROR; // псевдо-заголовок после обычного
						if(isTrailers)  return error_t::PROTOCOL_ERROR; // псевдо-заголовки запрещены в трейлерах
						if(isRequest){
							if(n == ":method"){ if(hasMethod) return error_t::PROTOCOL_ERROR; hasMethod = true; method = f.value; }
							else if(n == ":scheme"){ if(hasScheme) return error_t::PROTOCOL_ERROR; hasScheme = true; }
							else if(n == ":path"){ if(hasPath) return error_t::PROTOCOL_ERROR; hasPath = true; if(f.value.empty()) return error_t::PROTOCOL_ERROR; }
							else if(n == ":authority"){ if(hasAuthority) return error_t::PROTOCOL_ERROR; hasAuthority = true; }
							else return error_t::PROTOCOL_ERROR; // неизвестный/неуместный псевдо-заголовок
						} else {
							if(n == ":status"){ if(hasStatus) return error_t::PROTOCOL_ERROR; hasStatus = true; }
							else return error_t::PROTOCOL_ERROR;
						}
					} else {
						seenRegular = true;
						if(!isLowercaseName(n)) return error_t::PROTOCOL_ERROR;
						if(isConnectionSpecific(n)) return error_t::PROTOCOL_ERROR;
						if((n == "te") && (f.value != "trailers")) return error_t::PROTOCOL_ERROR; // TE допускает только "trailers"
					}
				}
				if(isTrailers) return error_t::NO_ERROR;
				if(isRequest){
					if(method == "CONNECT"){
						if(!hasAuthority) return error_t::PROTOCOL_ERROR; // CONNECT требует :authority
						if(hasScheme || hasPath) return error_t::PROTOCOL_ERROR; // и запрещает :scheme/:path
					} else if(!hasMethod || !hasScheme || !hasPath) return error_t::PROTOCOL_ERROR;
				} else if(!hasStatus) return error_t::PROTOCOL_ERROR; // ответ обязан содержать :status
				return error_t::NO_ERROR;
			}
		}

		status_t Session::validateNewStream(uint32_t id, error_t & err) noexcept {
			// Поток инициирует пир: для нас-сервера это клиент (нечётные id),
			// для нас-клиента это сервер (чётные id, server push) — RFC 9113 §5.1.1.
			const bool peerOdd = (_endpoint == endpoint_t::SERVER);
			if(((id & 1u) != 0) != peerOdd){ err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
			// Идентификатор обязан строго возрастать (повтор/уменьшение — ошибка).
			if(id <= _lastStreamId){ err = error_t::PROTOCOL_ERROR; return status_t::ERROR; }
			return status_t::OK;
		}

		void Session::closeStream(uint32_t id, error_t code) noexcept {
			if(_cb.onStreamClose != nullptr) _cb.onStreamClose(_user, id, code);
			_streams.erase(id);
		}

		void Session::applyRemoteEndStream(stream_t & s) noexcept {
			// RFC 9113 §5.1: получение END_STREAM закрывает удалённую половину потока.
			if(s.state == stream_state_t::OPEN) s.state = stream_state_t::HALF_CLOSED_REMOTE;
			else if(s.state == stream_state_t::HALF_CLOSED_LOCAL){
				const uint32_t id = s.id;
				s.state = stream_state_t::CLOSED;
				closeStream(id, error_t::NO_ERROR); // ссылка s после этого недействительна
			}
		}

		void Session::applyLocalEndStream(stream_t & s) noexcept {
			// RFC 9113 §5.1: отправка END_STREAM закрывает локальную половину потока.
			if(s.state == stream_state_t::OPEN) s.state = stream_state_t::HALF_CLOSED_LOCAL;
			else if(s.state == stream_state_t::HALF_CLOSED_REMOTE){
				const uint32_t id = s.id;
				s.state = stream_state_t::CLOSED;
				closeStream(id, error_t::NO_ERROR); // ссылка s после этого недействительна
			}
		}

		bool Session::providerDone(const stream_t & s) const noexcept {
			return (s.provider == nullptr) || s.providerEof;
		}

		void Session::refillFromProvider(stream_t & s) noexcept {
			if((s.provider == nullptr) || s.providerEof) return;
			// Держим буфер наполненным до high-water, дёргая провайдер порциями.
			while((s.sendBuffer.size() < _streamSendHighWater) && !s.providerEof){
				uint8_t tmp[16384];
				const size_t cap = std::min(sizeof(tmp), _streamSendHighWater - s.sendBuffer.size());
				bool eof = false;
				const int64_t r = s.provider(s.providerUser, s.id, tmp, cap, &eof);
				if(r < 0){ // ошибка источника — сбрасываем поток
					submitRstStream(s.id, error_t::INTERNAL_ERROR);
					s.providerEof = true;
					return;
				}
				if(r > 0) s.sendBuffer.append(reinterpret_cast <const char *> (tmp), static_cast <size_t> (r));
				if(eof){ s.providerEof = true; s.endStreamPending = true; }
				if((r == 0) && !eof) break; // провайдер временно без данных
			}
		}

		void Session::maybeNotifyWritable(stream_t & s) noexcept {
			// Сигнал отдаём только для push-модели (submitData), не для pull-провайдера.
			if((s.provider != nullptr) || (_cb.onStreamWritable == nullptr)) return;
			if(!s.writableNotified && (s.sendBuffer.size() <= _streamSendLowWater)){
				s.writableNotified = true;
				_cb.onStreamWritable(_user, s.id);
			}
		}

		void Session::pumpStream(stream_t & s) noexcept {
			while(true){
				refillFromProvider(s);
				const size_t remaining = s.sendBuffer.size();

				// Backpressure TCP-стадии: не раздуваем выходной буфер.
				if(_output.size() >= _outputHighWater) break;

				const int32_t win = std::min(_remoteWindow, s.remoteWindow);
				const size_t cap = _outputHighWater - _output.size();
				const size_t n = std::min({ remaining,
				                            static_cast <size_t> (win > 0 ? win : 0),
				                            static_cast <size_t> (_remote.maxFrameSize),
				                            cap });

				if(n == 0){
					// Окно закрыто (данные остаются в sendBuffer) либо слать нечего.
					// Завершение потока пустым DATA с END_STREAM не списывает окно (длина 0).
					if((remaining == 0) && s.endStreamPending && providerDone(s) && !s.endStreamSent){
						frame::serializeData(_output, s.id, std::string_view{}, true);
						s.endStreamSent = true;
						applyLocalEndStream(s); // ссылка s может стать недействительной
					}
					break;
				}

				const bool last = s.endStreamPending && (n == remaining) && providerDone(s);
				frame::serializeData(_output, s.id, std::string_view(s.sendBuffer.data(), n), last);
				s.sendBuffer.erase(0, n);     // снимаем отправленный префикс (sendBuffer = только неотправленное)
				_remoteWindow  -= static_cast <int32_t> (n);
				s.remoteWindow -= static_cast <int32_t> (n);

				maybeNotifyWritable(s);

				if(last){
					s.endStreamSent = true;
					applyLocalEndStream(s); // ссылка s может стать недействительной
					break;
				}
			}
		}

		void Session::pump() noexcept {
			if(_inPump) return; // защита от реентерабельности (onStreamWritable -> submitData -> pump)
			_inPump = true;
			// Снимок идентификаторов: pumpStream может удалить поток из карты.
			std::vector <uint32_t> ids;
			ids.reserve(_streams.size());
			for(const auto & kv : _streams) ids.push_back(kv.first);
			for(const uint32_t id : ids){
				stream_t * s = findStream(id);
				if(s != nullptr) pumpStream(*s);
			}
			_inPump = false;
		}

		void Session::replenishReceiveWindow(stream_t * s, uint32_t consumed) noexcept {
			// Окно приёма соединения (всегда 65535, не зависит от SETTINGS пира).
			_localWindow -= static_cast <int32_t> (consumed);
			if(_localWindow < (proto::DEFAULT_WINDOW_SIZE / 2)){
				const uint32_t delta = static_cast <uint32_t> (proto::DEFAULT_WINDOW_SIZE - _localWindow);
				frame::serializeWindowUpdate(_output, 0, delta);
				_localWindow += static_cast <int32_t> (delta);
			}
			// Окно приёма потока (начальное = наш SETTINGS_INITIAL_WINDOW_SIZE).
			if(s != nullptr){
				s->localWindow -= static_cast <int32_t> (consumed);
				if(s->localWindow < (_local.initialWindowSize / 2)){
					const uint32_t delta = static_cast <uint32_t> (_local.initialWindowSize - s->localWindow);
					frame::serializeWindowUpdate(_output, s->id, delta);
					s->localWindow += static_cast <int32_t> (delta);
				}
			}
		}

		void Session::submitHeaders(uint32_t streamId, const std::vector <hpack::field_t> & fields, bool endStream) noexcept {
			std::string block;
			_encoder.encode(fields, block, true);
			frame::serializeHeaderBlock(_output, streamId, block, endStream, _remote.maxFrameSize);
			stream_t & s = stream(streamId);
			if(s.state == stream_state_t::IDLE) s.state = stream_state_t::OPEN; // мы инициируем поток
			// Ответ на собственный push: reserved(local) -> half-closed(remote).
			else if(s.state == stream_state_t::RESERVED_LOCAL) s.state = stream_state_t::HALF_CLOSED_REMOTE;
			if(endStream){
				s.endStreamSent = true;
				applyLocalEndStream(s);
			}
		}

		uint32_t Session::submitPushPromise(uint32_t associatedStreamId, const std::vector <hpack::field_t> & request) noexcept {
			if(_endpoint != endpoint_t::SERVER) return 0;   // push инициирует только сервер
			if(_remote.enablePush == 0) return 0;            // клиент запретил push (SETTINGS_ENABLE_PUSH=0)
			// Ассоциированный поток должен быть открыт пиром и ещё жив.
			stream_t * assoc = findStream(associatedStreamId);
			if(assoc == nullptr) return 0;
			if((assoc->state != stream_state_t::OPEN) && (assoc->state != stream_state_t::HALF_CLOSED_REMOTE)) return 0;
			// Резервируем чётный push-поток.
			const uint32_t promisedId = _nextStreamId;
			_nextStreamId += 2;
			std::string block;
			_encoder.encode(request, block, true);
			frame::serializePushPromiseBlock(_output, associatedStreamId, promisedId, block, _remote.maxFrameSize);
			stream_t & ps = stream(promisedId);
			ps.state = stream_state_t::RESERVED_LOCAL;
			return promisedId;
		}

		size_t Session::submitData(uint32_t streamId, const void * data, size_t len, bool endStream) noexcept {
			stream_t * s = findStream(streamId);
			if(s == nullptr) return 0; // неизвестный/закрытый поток
			// Принимаем столько, сколько влезает до high-water (частичный приём + счётчик).
			const size_t room = (s->sendBuffer.size() < _streamSendHighWater) ? (_streamSendHighWater - s->sendBuffer.size()) : 0;
			const size_t take = std::min(len, room);
			if(take > 0) s->sendBuffer.append(static_cast <const char *> (data), take);
			// END_STREAM помечаем только когда принят весь финальный фрагмент.
			if(endStream && (take == len)) s->endStreamPending = true;
			if(s->sendBuffer.size() > _streamSendLowWater) s->writableNotified = false; // взвести сигнал снова
			pump();
			return take;
		}

		void Session::setDataProvider(uint32_t streamId, data_provider_t provider, void * user) noexcept {
			stream_t * s = findStream(streamId);
			if(s == nullptr) return;
			s->provider = provider;
			s->providerUser = user;
			s->providerEof = false;
			pump();
		}

		status_t Session::fail(error_t code, const char * message) noexcept {
			if(_cb.onError != nullptr) _cb.onError(_user, code, message);
			submitGoaway(code);
			return status_t::ERROR;
		}

		status_t Session::feed(const uint8_t * data, size_t size) noexcept {
			if((data != nullptr) && (size > 0)) _input.append(reinterpret_cast <const char *> (data), size);

			const uint8_t * buf = reinterpret_cast <const uint8_t *> (_input.data());
			const size_t total = _input.size();
			size_t pos = 0;

			// Сервер: сначала принимаем клиентский connection preface (24 октета).
			if(!_prefaceReceived){
				if(total - pos < proto::PREFACE.size()) return status_t::OK; // ждём больше данных
				if(std::string_view(reinterpret_cast <const char *> (buf + pos), proto::PREFACE.size()) != proto::PREFACE){
					_input.clear();
					return fail(error_t::PROTOCOL_ERROR, "invalid connection preface");
				}
				pos += proto::PREFACE.size();
				_prefaceReceived = true;
			}

			// Разбор потока фреймов.
			while((total - pos) >= proto::FRAME_HEADER_SIZE){
				frame::header_t h;
				frame::parseHeader(buf + pos, total - pos, h);

				// Лимит на размер фрейма (RFC 9113 §4.2).
				if(h.length > _local.maxFrameSize){
					_input.clear();
					return fail(error_t::FRAME_SIZE_ERROR, "frame exceeds SETTINGS_MAX_FRAME_SIZE");
				}
				// Полный фрейм ещё не пришёл — ждём.
				if((total - pos) < (proto::FRAME_HEADER_SIZE + h.length)) break;

				const uint8_t * payload = buf + pos + proto::FRAME_HEADER_SIZE;

				// Пока идёт сборка блока заголовков, допустим только CONTINUATION того же потока
				// (RFC 9113 §6.10) — иначе PROTOCOL_ERROR (защита от перемешивания блоков).
				if((_hbcStream != 0) && !((h.type == frame_t::CONTINUATION) && (h.streamId == _hbcStream))){
					_input.clear();
					return fail(error_t::PROTOCOL_ERROR, "expected CONTINUATION");
				}

				const status_t st = dispatch(h, payload);
				if(st == status_t::ERROR){ _input.clear(); return status_t::ERROR; }

				pos += proto::FRAME_HEADER_SIZE + h.length;
			}

			// Убираем разобранный префикс, оставляя неполный хвост.
			if(pos > 0) _input.erase(0, pos);
			return status_t::OK;
		}

		status_t Session::dispatch(const frame::header_t & h, const uint8_t * payload) noexcept {
			error_t err = error_t::NO_ERROR;
			switch(h.type){
				case frame_t::SETTINGS: {
					std::vector <frame::setting_entry_t> items;
					if(frame::parseSettings(h, payload, items, err) != status_t::OK) return fail(err, "bad SETTINGS");
					if(h.flags & flag::ACK){ _settingsAcked = true; return status_t::OK; }
					// Защита от flood управляющими фреймами.
					_ctrlLimit.update(_now);
					if(!_ctrlLimit.drain(1)) return fail(error_t::ENHANCE_YOUR_CALM, "SETTINGS flood");
					for(const auto & e : items){
						switch(e.id){
							case setting_t::HEADER_TABLE_SIZE:      _remote.headerTableSize = e.value; break;
							case setting_t::ENABLE_PUSH:
								if(e.value > 1) return fail(error_t::PROTOCOL_ERROR, "invalid ENABLE_PUSH");
								_remote.enablePush = e.value; break;
							case setting_t::MAX_CONCURRENT_STREAMS: _remote.maxConcurrentStreams = e.value; break;
							case setting_t::INITIAL_WINDOW_SIZE: {
								if(e.value > static_cast <uint32_t> (proto::MAX_WINDOW_SIZE)) return fail(error_t::FLOW_CONTROL_ERROR, "INITIAL_WINDOW_SIZE too large");
								// RFC 9113 §6.9.2: изменение начального окна сдвигает окна отправки
								// всех открытых потоков на дельту (может стать отрицательным).
								const int32_t newInit = static_cast <int32_t> (e.value);
								const int64_t delta = static_cast <int64_t> (newInit) - _remote.initialWindowSize;
								_remote.initialWindowSize = newInit;
								for(auto & kv : _streams){
									const int64_t nw = static_cast <int64_t> (kv.second.remoteWindow) + delta;
									if(nw > proto::MAX_WINDOW_SIZE) return fail(error_t::FLOW_CONTROL_ERROR, "stream window overflow on SETTINGS");
									kv.second.remoteWindow = static_cast <int32_t> (nw);
								}
								break;
							}
							case setting_t::MAX_FRAME_SIZE:
								if((e.value < proto::MIN_MAX_FRAME_SIZE) || (e.value > proto::MAX_MAX_FRAME_SIZE)) return fail(error_t::PROTOCOL_ERROR, "invalid MAX_FRAME_SIZE");
								_remote.maxFrameSize = e.value; break;
							case setting_t::MAX_HEADER_LIST_SIZE:   _remote.maxHeaderListSize = e.value; break;
						}
					}
					frame::serializeSettings(_output, nullptr, 0, true); // ACK
					if(_cb.onSettings != nullptr) _cb.onSettings(_user);
					pump(); // изменение окон могло разблокировать отправку
					return status_t::OK;
				}
				case frame_t::PING: {
					uint8_t opaque[8];
					if(frame::parsePing(h, payload, opaque, err) != status_t::OK) return fail(err, "bad PING");
					if((h.flags & flag::ACK) == 0){
						// Защита от flood: каждый PING требует ответного PING-ACK (усиление).
						_ctrlLimit.update(_now);
						if(!_ctrlLimit.drain(1)) return fail(error_t::ENHANCE_YOUR_CALM, "PING flood");
						frame::serializePing(_output, opaque, true); // отвечаем ACK
					}
					return status_t::OK;
				}
				case frame_t::WINDOW_UPDATE: {
					uint32_t inc = 0;
					if(frame::parseWindowUpdate(h, payload, inc, err) != status_t::OK){
						// Нулевой инкремент на уровне потока — это RST_STREAM, но для скелета — соединение.
						return fail(err, "bad WINDOW_UPDATE");
					}
					if(h.streamId == 0){
						if(static_cast <int64_t> (_remoteWindow) + inc > proto::MAX_WINDOW_SIZE)
							return fail(error_t::FLOW_CONTROL_ERROR, "connection window overflow");
						_remoteWindow += static_cast <int32_t> (inc);
					} else {
						// WINDOW_UPDATE на ещё не открытом (idle) потоке — ошибка соединения (§5.1).
						stream_t * s = findStream(h.streamId);
						if((s == nullptr) && (h.streamId > _lastStreamId))
							return fail(error_t::PROTOCOL_ERROR, "WINDOW_UPDATE on idle stream");
						if(s != nullptr){
							if(static_cast <int64_t> (s->remoteWindow) + inc > proto::MAX_WINDOW_SIZE)
								return fail(error_t::FLOW_CONTROL_ERROR, "stream window overflow");
							s->remoteWindow += static_cast <int32_t> (inc);
						}
					}
					pump(); // окно открылось — дослыаем отложенные данные
					return status_t::OK;
				}
				case frame_t::GOAWAY: {
					frame::goaway_t g;
					if(frame::parseGoaway(h, payload, g, err) != status_t::OK) return fail(err, "bad GOAWAY");
					_goawayReceived = true;
					if(_cb.onGoaway != nullptr) _cb.onGoaway(_user, g.lastStreamId, g.code);
					return status_t::OK;
				}
				case frame_t::RST_STREAM: {
					error_t code = error_t::NO_ERROR;
					if(frame::parseRstStream(h, payload, code, err) != status_t::OK) return fail(err, "bad RST_STREAM");
					// RST_STREAM на ещё не открытом (idle) потоке — ошибка соединения (§5.1).
					if((findStream(h.streamId) == nullptr) && (h.streamId > _lastStreamId))
						return fail(error_t::PROTOCOL_ERROR, "RST_STREAM on idle stream");
					// Защита от Rapid Reset (CVE-2023-44487): ограничиваем частоту входящих RST_STREAM.
					_rstLimit.update(_now);
					if(!_rstLimit.drain(1)) return fail(error_t::ENHANCE_YOUR_CALM, "RST_STREAM flood (Rapid Reset)");
					closeStream(h.streamId, code);
					return status_t::OK;
				}
				case frame_t::HEADERS: {
					frame::headers_t hd;
					if(frame::parseHeaders(h, payload, hd, err) != status_t::OK) return fail(err, "bad HEADERS");

					stream_t * s = findStream(h.streamId);
					if(s == nullptr){
						// Открытие нового потока пиром: проверяем чётность и монотонность id.
						if(validateNewStream(h.streamId, err) != status_t::OK) return fail(err, "invalid new stream id");
						_lastStreamId = h.streamId;
						stream_t & ns = stream(h.streamId);
						ns.state = stream_state_t::OPEN;
						if(_cb.onStreamBegin != nullptr) _cb.onStreamBegin(_user, h.streamId);
					} else {
						// HEADERS на существующем потоке.
						switch(s->state){
							case stream_state_t::RESERVED_REMOTE:
								// Ответ на server push: reserved(remote) -> half-closed(local).
								s->state = stream_state_t::HALF_CLOSED_LOCAL;
								break;
							case stream_state_t::OPEN:
							case stream_state_t::HALF_CLOSED_LOCAL:
								// Повторный HEADERS — это трейлеры, обязаны нести END_STREAM (§8.1).
								if(s->headersDone && !hd.endStream)
									return fail(error_t::PROTOCOL_ERROR, "trailers without END_STREAM");
								break;
							case stream_state_t::HALF_CLOSED_REMOTE:
							case stream_state_t::CLOSED:
								return fail(error_t::STREAM_CLOSED, "HEADERS on closed stream");
							default:
								return fail(error_t::PROTOCOL_ERROR, "HEADERS in invalid stream state");
						}
					}
					_hbcStream    = h.streamId;
					_hbcEndStream = hd.endStream;
					_hbcBuffer.assign(hd.block.data(), hd.block.size());
					_hbcFrames    = 1;
					// Защита от CONTINUATION flood (2024): лимит размера блока заголовков.
					if(_hbcBuffer.size() > _maxHeaderBlockSize) return fail(error_t::ENHANCE_YOUR_CALM, "header block too large");
					if(hd.endHeaders){
						const status_t st = deliverHeaders();
						if(st == status_t::ERROR) return status_t::ERROR;
					}
					// иначе ждём CONTINUATION
					return status_t::OK;
				}
				case frame_t::CONTINUATION: {
					std::string_view block;
					bool endHeaders = false;
					if(_hbcStream == 0) return fail(error_t::PROTOCOL_ERROR, "unexpected CONTINUATION");
					if(frame::parseContinuation(h, payload, block, endHeaders, err) != status_t::OK) return fail(err, "bad CONTINUATION");
					// Защита от CONTINUATION flood (2024): лимиты на число фреймов и размер блока.
					if(++_hbcFrames > _maxContinuationFrames) return fail(error_t::ENHANCE_YOUR_CALM, "too many CONTINUATION frames");
					_hbcBuffer.append(block.data(), block.size());
					if(_hbcBuffer.size() > _maxHeaderBlockSize) return fail(error_t::ENHANCE_YOUR_CALM, "header block too large");
					if(endHeaders){
						const status_t st = deliverHeaders();
						if(st == status_t::ERROR) return status_t::ERROR;
					}
					return status_t::OK;
				}
				case frame_t::DATA: {
					frame::data_t d;
					if(frame::parseData(h, payload, d, err) != status_t::OK) return fail(err, "bad DATA");

					// Допустимость по состоянию потока (§5.1): данные принимаем только в
					// OPEN или HALF_CLOSED_LOCAL.
					stream_t * s = findStream(h.streamId);
					if(s == nullptr){
						// Поток ещё не открывался (idle) либо уже закрыт и удалён.
						if(h.streamId > _lastStreamId) return fail(error_t::PROTOCOL_ERROR, "DATA on idle stream");
						return fail(error_t::STREAM_CLOSED, "DATA on closed stream");
					}
					if((s->state != stream_state_t::OPEN) && (s->state != stream_state_t::HALF_CLOSED_LOCAL))
						return fail(error_t::STREAM_CLOSED, "DATA in non-open stream state");

					// Защита от flood пустыми DATA-фреймами без END_STREAM (бесполезная нагрузка).
					if((h.length == 0) && !d.endStream){
						_ctrlLimit.update(_now);
						if(!_ctrlLimit.drain(1)) return fail(error_t::ENHANCE_YOUR_CALM, "empty DATA flood");
					}

					// Flow control приёма: учитывается ПОЛНАЯ длина payload, включая padding (§6.9.1).
					if(static_cast <int32_t> (h.length) > _localWindow)
						return fail(error_t::FLOW_CONTROL_ERROR, "connection receive window exhausted");
					if(_cb.onData != nullptr)
						_cb.onData(_user, h.streamId, reinterpret_cast <const uint8_t *> (d.data.data()), d.data.size(), d.endStream);
					// Пополняем окно приёма и при просадке шлём WINDOW_UPDATE (потоку — только если он остаётся открыт).
					replenishReceiveWindow(d.endStream ? nullptr : s, h.length);
					if(d.endStream) applyRemoteEndStream(*s);
					return status_t::OK;
				}
				case frame_t::PRIORITY: {
					frame::priority_t pr;
					if(frame::parsePriority(h, payload, pr, err) != status_t::OK) return fail(err, "bad PRIORITY");
					// Приоритеты RFC 7540 deprecated — игнорируем (см. README.md).
					return status_t::OK;
				}
				case frame_t::PUSH_PROMISE: {
					// PUSH_PROMISE отправляет только сервер; принимает только клиент (§8.4).
					if(_endpoint == endpoint_t::SERVER) return fail(error_t::PROTOCOL_ERROR, "server received PUSH_PROMISE");
					// Мы запретили push своим SETTINGS_ENABLE_PUSH=0 — пир не вправе пушить.
					if(_local.enablePush == 0) return fail(error_t::PROTOCOL_ERROR, "PUSH_PROMISE while push disabled");
					frame::push_promise_t pp;
					if(frame::parsePushPromise(h, payload, pp, err) != status_t::OK) return fail(err, "bad PUSH_PROMISE");
					// Ассоциированный поток (на котором пришёл промис) должен существовать и быть живым.
					stream_t * assoc = findStream(h.streamId);
					if((assoc == nullptr) || (assoc->state == stream_state_t::CLOSED))
						return fail(error_t::PROTOCOL_ERROR, "PUSH_PROMISE on invalid stream");
					// Идентификатор обещанного потока: чётный (инициирует сервер) и строго возрастающий.
					if(validateNewStream(pp.promisedStreamId, err) != status_t::OK) return fail(err, "invalid promised stream id");
					_lastStreamId = pp.promisedStreamId;
					// Резервируем обещанный поток: reserved(remote).
					stream_t & ps = stream(pp.promisedStreamId);
					ps.state = stream_state_t::RESERVED_REMOTE;
					// Начинаем сборку блока заголовков обещанного запроса; CONTINUATION придут
					// на ассоциированном потоке (h.streamId), а заголовки относятся к обещанному.
					_hbcStream    = h.streamId;
					_hbcPromised  = pp.promisedStreamId;
					_hbcEndStream = false; // PUSH_PROMISE не несёт END_STREAM
					_hbcBuffer.assign(pp.block.data(), pp.block.size());
					_hbcFrames    = 1;
					if(_hbcBuffer.size() > _maxHeaderBlockSize) return fail(error_t::ENHANCE_YOUR_CALM, "header block too large");
					if(pp.endHeaders){
						const status_t st = deliverHeaders();
						if(st == status_t::ERROR) return status_t::ERROR;
					}
					return status_t::OK;
				}
			}
			// Неизвестный тип фрейма — по RFC 9113 §4.1 игнорируется.
			return status_t::OK;
		}

		status_t Session::deliverHeaders() noexcept {
			const uint32_t streamId = _hbcStream;
			const uint32_t promised = _hbcPromised;
			const bool endStream = _hbcEndStream;

			std::vector <hpack::field_t> fields;
			error_t err = error_t::NO_ERROR;
			const status_t st = _decoder.decode(_hbcBuffer, fields, _local.maxHeaderListSize, err);
			// Сброс состояния сборки до возможного выхода.
			_hbcStream = 0;
			_hbcPromised = 0;
			_hbcEndStream = false;
			_hbcBuffer.clear();
			_hbcFrames = 0;
			if(st != status_t::OK) return fail(err, "HPACK decode failed");

			// Блок принадлежит PUSH_PROMISE — это обещанный запрос для отдельного потока.
			if(promised != 0) return deliverPushPromise(streamId, promised, fields);

			// Поток уже создан и провалидирован в обработчике HEADERS.
			stream_t * s = findStream(streamId);
			if(s == nullptr) return fail(error_t::INTERNAL_ERROR, "stream vanished");

			// Валидация HTTP-семантики (RFC 9113 §8). Повторный HEADERS на потоке = трейлеры.
			const bool isTrailers = s->headersDone;
			const bool isRequest  = (_endpoint == endpoint_t::SERVER);
			const error_t vErr = validateHeaders(fields, isRequest, isTrailers);
			if(vErr != error_t::NO_ERROR){
				// Малформированный запрос/ответ — потоковая ошибка (§8.1.1), соединение живёт.
				frame::serializeRstStream(_output, streamId, vErr);
				closeStream(streamId, vErr);
				return status_t::OK;
			}

			if(_cb.onHeader != nullptr)
				for(const hpack::field_t & f : fields) _cb.onHeader(_user, streamId, f.name, f.value);

			s->headersDone = true;
			if(_cb.onHeadersComplete != nullptr) _cb.onHeadersComplete(_user, streamId, endStream);
			// Переход по END_STREAM может закрыть и удалить поток — выполняем последним.
			if(endStream){
				stream_t * s2 = findStream(streamId);
				if(s2 != nullptr) applyRemoteEndStream(*s2);
			}
			return status_t::OK;
		}

		status_t Session::deliverPushPromise(uint32_t associatedStreamId, uint32_t promisedStreamId, std::vector <hpack::field_t> & fields) noexcept {
			stream_t * ps = findStream(promisedStreamId);
			if(ps == nullptr) return fail(error_t::INTERNAL_ERROR, "promised stream vanished");
			// Обещанный блок — это всегда запрос (псевдо-заголовки запроса), без трейлеров.
			const error_t vErr = validateHeaders(fields, /* isRequest */ true, /* isTrailers */ false);
			if(vErr != error_t::NO_ERROR){
				// Малформированный обещанный запрос — потоковая ошибка, соединение живёт.
				frame::serializeRstStream(_output, promisedStreamId, vErr);
				closeStream(promisedStreamId, vErr);
				return status_t::OK;
			}
			if(_cb.onPushPromise != nullptr) _cb.onPushPromise(_user, associatedStreamId, promisedStreamId);
			if(_cb.onHeader != nullptr)
				for(const hpack::field_t & f : fields) _cb.onHeader(_user, promisedStreamId, f.name, f.value);
			// Внимание: headersDone НЕ выставляем — обещанный запрос пришёл фреймом PUSH_PROMISE
			// на ассоциированном потоке, а не HEADERS на этом. Реальный ответный HEADERS сервера
			// будет первым на push-потоке (иначе он ошибочно считался бы трейлерами).
			// Обещанный запрос завершён на END_HEADERS (тела у него нет); endStream=false —
			// клиент ещё ждёт ответных HEADERS сервера на этом потоке (reserved→half-closed(local)).
			if(_cb.onHeadersComplete != nullptr) _cb.onHeadersComplete(_user, promisedStreamId, false);
			return status_t::OK;
		}
	}
}
