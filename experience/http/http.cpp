/**
 * @file http.cpp
 * @brief Высокопроизводительный инкрементальный (streaming) парсер HTTP/1.1
 *        промышленного уровня.
 *
 *  Особенности реализации:
 *   - Полностью процедурный стиль: namespace + свободные static-функции,
 *     никаких объектов/классов с методами и наследованием. Состояние хранится
 *     в обычной структуре-контейнере awh::http::Parser.
 *   - Байтовый конечный автомат (как в llhttp / nodejs http_parser): данные
 *     можно подавать любыми кусками, разрыв допустим в любом байте.
 *   - Zero-copy на горячем пути тела/чанков (bulk-append через memcpy внутри
 *     std::string::append, без посимвольной обработки тела).
 *   - constexpr-таблицы классов символов (token / header-value / target) —
 *     валидация за O(1) без ветвлений.
 *   - Защита от HTTP request smuggling: запрет одновременных Content-Length и
 *     Transfer-Encoding, запрет нескольких различающихся Content-Length,
 *     требование "chunked" быть последним кодированием.
 *   - Жёсткие настраиваемые лимиты (длина строк, число и размер заголовков,
 *     размер тела и чанков) — защита от DoS.
 *   - Поддержка всех методов, произвольных заголовков, chunked-кодирования и
 *     трейлеров, keep-alive, тела «до закрытия соединения», конвейера запросов.
 *   - Расширенный набор методов: HTTP/1.1 + PATCH + WebDAV (RFC 4918) + прочие.
 *   - Опциональный потоковый callback-API (Handler) с ZERO-COPY телом: фрагменты
 *     тела отдаются как срез входного буфера без копирования — можно обрабатывать
 *     тела любого размера без буферизации (см. поля Parser::handler/storeBody).
 *   - Быстрый «крупноблочный» разбор токенов: непрерывные участки (target, имя и
 *     значение заголовка) сканируются пачкой по lookup-таблице и копируются одним
 *     append (использует векторизованный memcpy libc); тело/чанки — bulk memcpy.
 *     Подход переносимый: одинаково эффективен на x86_64 и ARM64 (в отличие от
 *     SIMD-кода, привязанного к конкретной архитектуре).
 *
 *  ------------------------------------------------------------------------
 *  СБОРКА ИЗ ТЕРМИНАЛА (без CMake):
 *
 *   1) Собрать только парсер в объектный файл:
 *        c++ -std=c++17 -O3 -march=native -Wall -Wextra -c http.cpp -o http.o
 *
 *   2) Собрать демонстрацию (примеры использования) вместе с парсером:
 *        c++ -std=c++17 -O3 -march=native -Wall -Wextra \
 *            http.cpp http_demo.cpp -o http_demo
 *
 *   3) Запустить демонстрацию:
 *        ./http_demo
 *
 *   Отладочная сборка с санитайзерами (рекомендуется при доработке):
 *        c++ -std=c++17 -O0 -g -fsanitize=address,undefined -Wall -Wextra \
 *            http.cpp http_demo.cpp -o http_demo_dbg && ./http_demo_dbg
 *
 *   Примечания:
 *     - флаг -march=native можно убрать для переносимых бинарников;
 *     - на GNU/Linux подойдёт g++ той же командой;
 *     - стандарт C++17 обязателен (constexpr std::array, if constexpr и т.п.).
 *  ------------------------------------------------------------------------
 */

#include "http.hpp"

#include <array>
#include <cstring>

namespace awh {
	namespace http {
		// Всё внутреннее — в анонимном namespace (внутренняя компоновка).
		namespace {
			/**
			 * @brief Внутренние состояния конечного автомата.
			 */
			enum State : uint16_t {
				// общий старт (диспетчеризация по типу)
				s_start = 0,

				// ---- request-line ----
				s_req_method,
				s_req_target_start,
				s_req_target,
				s_req_http_start,
				s_req_http_H, s_req_http_HT, s_req_http_HTT, s_req_http_HTTP, s_req_http_slash,
				s_req_http_major, s_req_http_dot, s_req_http_minor,
				s_req_line_almost_done, s_req_line_lf,

				// ---- status-line ----
				s_res_http_H, s_res_http_HT, s_res_http_HTT, s_res_http_HTTP, s_res_http_slash,
				s_res_http_major, s_res_http_dot, s_res_http_minor,
				s_res_first_space, s_res_status_start, s_res_status_code,
				s_res_reason_start, s_res_reason, s_res_line_lf,

				// ---- заголовки ----
				s_header_start,
				s_header_name,
				s_header_value_ows,
				s_header_value,
				s_header_almost_done,
				s_headers_lf,

				// ---- тело ----
				s_body_identity,
				s_body_until_close,

				// ---- chunked ----
				s_chunk_size,
				s_chunk_ext,
				s_chunk_size_lf,
				s_chunk_data,
				s_chunk_data_almost_done,
				s_chunk_data_lf,

				// ---- трейлеры ----
				s_trailer_start,
				s_trailer_name,
				s_trailer_value_ows,
				s_trailer_value,
				s_trailer_almost_done,
				s_trailers_lf,

				// ---- финал ----
				s_message_done
			};

			// =====================================================================
			//  Таблицы классов символов (constexpr, считаются на этапе компиляции).
			// =====================================================================

			/// token char (RFC 7230): ALPHA / DIGIT / "!#$%&'*+-.^_`|~"
			constexpr std::array<bool, 256> makeTokenTable() {
				std::array<bool, 256> t{};
				for (int c = 0; c < 256; ++c) {
					t[c] = (c >= 'a' && c <= 'z') ||
					       (c >= 'A' && c <= 'Z') ||
					       (c >= '0' && c <= '9');
				}
				const char specials[] = "!#$%&'*+-.^_`|~";
				for (size_t i = 0; specials[i] != '\0'; ++i)
					t[static_cast<unsigned char>(specials[i])] = true;
				return t;
			}

			/// допустимый символ значения заголовка: HTAB / SP / VCHAR(0x21..0x7E) / obs-text(0x80..0xFF)
			constexpr std::array<bool, 256> makeHeaderValueTable() {
				std::array<bool, 256> t{};
				for (int c = 0; c < 256; ++c) {
					t[c] = (c == 0x09) || (c == 0x20) ||
					       (c >= 0x21 && c <= 0x7E) ||
					       (c >= 0x80 && c <= 0xFF);
				}
				return t;
			}

			/// допустимый символ request-target: VCHAR без пробела (0x21..0x7E)
			constexpr std::array<bool, 256> makeTargetTable() {
				std::array<bool, 256> t{};
				for (int c = 0x21; c <= 0x7E; ++c)
					t[c] = true;
				return t;
			}

			/// допустимый символ reason-phrase: HTAB / SP / VCHAR / obs-text (как value заголовка)
			constexpr std::array<bool, 256> makeReasonTable() {
				return makeHeaderValueTable();
			}

			constexpr auto kTokenTable  = makeTokenTable();
			constexpr auto kValueTable  = makeHeaderValueTable();
			constexpr auto kTargetTable = makeTargetTable();
			constexpr auto kReasonTable = makeReasonTable();

			inline bool isToken(unsigned char c)  noexcept { return kTokenTable[c]; }
			inline bool isValueCh(unsigned char c) noexcept { return kValueTable[c]; }
			inline bool isTargetCh(unsigned char c) noexcept { return kTargetTable[c]; }
			inline bool isReasonCh(unsigned char c) noexcept { return kReasonTable[c]; }

			inline bool isDigit(unsigned char c) noexcept { return c >= '0' && c <= '9'; }

			inline int hexVal(unsigned char c) noexcept {
				if (c >= '0' && c <= '9') return c - '0';
				if (c >= 'a' && c <= 'f') return c - 'a' + 10;
				if (c >= 'A' && c <= 'F') return c - 'A' + 10;
				return -1;
			}

			inline char lower(char c) noexcept {
				return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
			}

			/// Сравнение строки с литералом без учёта регистра (литерал в нижнем регистре).
			bool iequalsLit(const char * s, size_t n, const char * litLower) noexcept {
				for (size_t i = 0; i < n; ++i) {
					if (litLower[i] == '\0') return false;
					if (lower(s[i]) != litLower[i]) return false;
				}
				return litLower[n] == '\0';
			}

			bool iequalsLit(const std::string & s, const char * litLower) noexcept {
				return iequalsLit(s.data(), s.size(), litLower);
			}

			// =====================================================================
			//  Вспомогательные мутаторы состояния.
			// =====================================================================

			inline void setError(Parser & p, Error e) noexcept {
				p.error = e;
			}

			// --- Безопасные обёртки вызова пользовательских callback'ов ---
			// Возвращают false и выставляют Error::ABORTED, если callback прервал разбор.

			inline bool fireHook(Parser & p, HookCb cb) noexcept {
				if (cb && !cb(p, p.userData)) { setError(p, Error::ABORTED); return false; }
				return true;
			}
			inline bool fireData(Parser & p, DataCb cb, const char * d, size_t n) noexcept {
				if (cb && !cb(p, p.userData, d, n)) { setError(p, Error::ABORTED); return false; }
				return true;
			}
			inline bool fireField(Parser & p, FieldCb cb,
			                      const char * nm, size_t nl, const char * vl, size_t vln) noexcept {
				if (cb && !cb(p, p.userData, nm, nl, vl, vln)) { setError(p, Error::ABORTED); return false; }
				return true;
			}
			inline bool fireStatus(Parser & p, StatusCb cb,
			                       uint16_t code, const char * r, size_t rl) noexcept {
				if (cb && !cb(p, p.userData, code, r, rl)) { setError(p, Error::ABORTED); return false; }
				return true;
			}
			inline bool fireSize(Parser & p, SizeCb cb, uint64_t sz) noexcept {
				if (cb && !cb(p, p.userData, sz)) { setError(p, Error::ABORTED); return false; }
				return true;
			}

			/// Завершить разбор сообщения.
			inline void completeMessage(Parser & p) noexcept {
				p.message.complete = true;
				p.state = s_message_done;
				if (p.handler) fireHook(p, p.handler->onMessageComplete);
			}

			Method classifyMethod(const std::string & m) noexcept {
				// Диспетчеризация по длине (operator== сравнивает размер первым).
				switch (m.size()) {
					case 3:
						if (m == "GET") return Method::GET;
						if (m == "PUT") return Method::PUT;
						if (m == "ACL") return Method::ACL;
						if (m == "PRI") return Method::PRI;
						break;
					case 4:
						if (m == "HEAD") return Method::HEAD;
						if (m == "POST") return Method::POST;
						if (m == "COPY") return Method::COPY;
						if (m == "LOCK") return Method::LOCK;
						if (m == "MOVE") return Method::MOVE;
						if (m == "BIND") return Method::BIND;
						if (m == "LINK") return Method::LINK;
						break;
					case 5:
						if (m == "TRACE") return Method::TRACE;
						if (m == "PATCH") return Method::PATCH;
						if (m == "MKCOL") return Method::MKCOL;
						if (m == "MERGE") return Method::MERGE;
						if (m == "PURGE") return Method::PURGE;
						break;
					case 6:
						if (m == "DELETE") return Method::DELETE_;
						if (m == "SEARCH") return Method::SEARCH;
						if (m == "UNLOCK") return Method::UNLOCK;
						if (m == "REBIND") return Method::REBIND;
						if (m == "UNBIND") return Method::UNBIND;
						if (m == "REPORT") return Method::REPORT;
						if (m == "NOTIFY") return Method::NOTIFY;
						if (m == "SOURCE") return Method::SOURCE;
						if (m == "UNLINK") return Method::UNLINK;
						break;
					case 7:
						if (m == "CONNECT") return Method::CONNECT;
						if (m == "OPTIONS") return Method::OPTIONS;
						break;
					case 8:
						if (m == "PROPFIND") return Method::PROPFIND;
						if (m == "CHECKOUT") return Method::CHECKOUT;
						if (m == "M-SEARCH") return Method::MSEARCH;
						break;
					case 9:
						if (m == "PROPPATCH") return Method::PROPPATCH;
						if (m == "SUBSCRIBE") return Method::SUBSCRIBE;
						break;
					case 10:
						if (m == "MKACTIVITY") return Method::MKACTIVITY;
						if (m == "MKCALENDAR") return Method::MKCALENDAR;
						break;
					case 11:
						if (m == "UNSUBSCRIBE") return Method::UNSUBSCRIBE;
						break;
				}
				return Method::UNKNOWN;
			}

			/// Разбор десятичного Content-Length с контролем переполнения.
			bool parseDecimal(const char * s, size_t n, uint64_t & out) noexcept {
				if (n == 0) return false;
				uint64_t v = 0;
				for (size_t i = 0; i < n; ++i) {
					if (!isDigit(static_cast<unsigned char>(s[i]))) return false;
					const uint64_t d = static_cast<uint64_t>(s[i] - '0');
					if (v > (UINT64_MAX - d) / 10ull) return false; // overflow
					v = v * 10ull + d;
				}
				out = v;
				return true;
			}

			/// Триминг OWS (SP/HTAB) по краям подстроки [b, e).
			void trimOWS(const char *& b, const char *& e) noexcept {
				while (b < e && (*b == ' ' || *b == '\t')) ++b;
				while (e > b && (e[-1] == ' ' || e[-1] == '\t')) --e;
			}

			/// Интерпретация Content-Length: список значений должен быть непротиворечив.
			bool applyContentLength(Parser & p, const char * b, const char * e) noexcept {
				// Значение может быть списком "5, 5" — все элементы обязаны совпадать.
				const char * cur = b;
				bool firstSet = false;
				uint64_t first = 0;
				while (cur <= e) {
					const char * comma = static_cast<const char *>(memchr(cur, ',', static_cast<size_t>(e - cur)));
					const char * tokEnd = comma ? comma : e;
					const char * tb = cur;
					const char * te = tokEnd;
					trimOWS(tb, te);
					uint64_t v = 0;
					if (!parseDecimal(tb, static_cast<size_t>(te - tb), v)) {
						setError(p, Error::INVALID_CONTENT_LENGTH);
						return false;
					}
					if (!firstSet) { first = v; firstSet = true; }
					else if (v != first) {
						setError(p, Error::CONTENT_LENGTH_CONFLICT);
						return false;
					}
					if (!comma) break;
					cur = comma + 1;
				}
				if (!firstSet) { setError(p, Error::INVALID_CONTENT_LENGTH); return false; }
				if (p.clSeen && p.clValue != first) {
					setError(p, Error::CONTENT_LENGTH_CONFLICT);
					return false;
				}
				p.clSeen = true;
				p.clValue = first;
				return true;
			}

			/// Интерпретация Transfer-Encoding (накопительно по нескольким заголовкам).
			void applyTransferEncoding(Parser & p, const char * b, const char * e) noexcept {
				p.teSeen = true;
				// Если предыдущий TE уже заканчивался на chunked, любой новый TE делает
				// chunked не последним -> ошибка кадрирования.
				if (p.teChunkedFinal) p.teInvalid = true;

				const char * cur = b;
				bool lastChunked = false;
				while (cur <= e) {
					const char * comma = static_cast<const char *>(memchr(cur, ',', static_cast<size_t>(e - cur)));
					const char * tokEnd = comma ? comma : e;
					const char * tb = cur;
					const char * te = tokEnd;
					trimOWS(tb, te);
					if (te > tb) {
						const bool isChunked = iequalsLit(tb, static_cast<size_t>(te - tb), "chunked");
						if (lastChunked) p.teInvalid = true; // chunked не последний внутри строки
						lastChunked = isChunked;
					}
					if (!comma) break;
					cur = comma + 1;
				}
				p.teChunkedFinal = lastChunked && !p.teInvalid;
			}

			/// Интерпретация Connection.
			void applyConnection(Parser & p, const char * b, const char * e) noexcept {
				const char * cur = b;
				while (cur <= e) {
					const char * comma = static_cast<const char *>(memchr(cur, ',', static_cast<size_t>(e - cur)));
					const char * tokEnd = comma ? comma : e;
					const char * tb = cur;
					const char * te = tokEnd;
					trimOWS(tb, te);
					const size_t n = static_cast<size_t>(te - tb);
					if (n > 0) {
						if (iequalsLit(tb, n, "close")) p.connClose = true;
						else if (iequalsLit(tb, n, "keep-alive")) p.connKeepAlive = true;
					}
					if (!comma) break;
					cur = comma + 1;
				}
			}

		/// Завершение текущего заголовка/трейлера: триминг, валидация, интерпретация.
		/// Может бросить (push_back/append) — перехватывается в execute().
		bool finishHeader(Parser & p) {
				// Триминг хвостовых OWS у значения (ведущие уже пропущены состоянием _ows).
				while (!p.curValue.empty() &&
				       (p.curValue.back() == ' ' || p.curValue.back() == '\t')) {
					p.curValue.pop_back();
				}

				// Лимиты на количество и суммарный размер.
				if (++p.headerCount > p.limits.maxHeaderCount) {
					setError(p, Error::TOO_MANY_HEADERS);
					return false;
				}
				p.headersTotalBytes += p.curName.size() + p.curValue.size();
				if (p.headersTotalBytes > p.limits.maxHeadersTotal) {
					setError(p, Error::HEADER_OVERFLOW);
					return false;
				}

				const char * vb = p.curValue.data();
				const char * ve = vb + p.curValue.size();

				if (p.inTrailers) {
					// Трейлеры не влияют на кадрирование — просто отдаём/сохраняем.
					if (p.handler && !fireField(p, p.handler->onTrailer,
					                            p.curName.data(), p.curName.size(),
					                            p.curValue.data(), p.curValue.size())) return false;
					if (p.storeHeaders)
						p.message.trailers.push_back(Header{p.curName, p.curValue});
				} else {
					// Интерпретация спец-заголовков (диспетчер по первой букве — дёшево).
					switch (p.curName.empty() ? '\0' : lower(p.curName[0])) {
						case 'c':
							if (iequalsLit(p.curName, "content-length")) {
								if (!applyContentLength(p, vb, ve)) return false;
							} else if (iequalsLit(p.curName, "connection")) {
								applyConnection(p, vb, ve);
							}
							break;
						case 't':
							if (iequalsLit(p.curName, "transfer-encoding"))
								applyTransferEncoding(p, vb, ve);
							break;
						default:
							break;
					}
					if (p.handler && !fireField(p, p.handler->onHeader,
					                            p.curName.data(), p.curName.size(),
					                            p.curValue.data(), p.curValue.size())) return false;
					if (p.storeHeaders)
						p.message.headers.push_back(Header{p.curName, p.curValue});
				}

				p.curName.clear();
				p.curValue.clear();
				return true;
			}

			/// Решение о наличии тела для ответа по статус-коду / методу HEAD.
		bool responseHasNoBody(const Parser & p) noexcept {
			const uint16_t code = p.message.statusCode;
			if (p.responseToHead) return true;
			// Успешный (2xx) ответ на CONNECT открывает туннель — тела нет.
			if (p.responseToConnect && code >= 200 && code < 300) return true;
			if (code >= 100 && code < 200) return true; // 1xx
			if (code == 204 || code == 304) return true;
			return false;
		}

		/// Выбор способа кадрирования тела после завершения заголовков.
		/// Может бросить (reserve) — перехватывается в execute().
		void beginBody(Parser & p) {
				// Финальные семантические проверки безопасности.
				if (p.teSeen && p.clSeen) {
					setError(p, Error::CONTENT_LENGTH_CONFLICT); // защита от smuggling
					return;
				}
				if (p.teInvalid) {
					setError(p, Error::INVALID_TRANSFER_ENCODING);
					return;
				}

				p.message.chunked = p.teSeen && p.teChunkedFinal;
				p.message.hasContentLength = p.clSeen;
				p.message.contentLength = p.clValue;

				// keep-alive по версии + Connection.
				const bool http11 = (p.message.versionMajor == 1 && p.message.versionMinor >= 1);
				if (http11) p.message.keepAlive = !p.connClose;
				else if (p.message.versionMajor == 1 && p.message.versionMinor == 0)
					p.message.keepAlive = p.connKeepAlive && !p.connClose;
				else p.message.keepAlive = false;

				// Заголовки полностью разобраны и осмыслены.
				if (p.handler && !fireHook(p, p.handler->onHeadersComplete)) return;

				// Ответы без тела по правилам RFC.
				if (p.type == Type::RESPONSE && responseHasNoBody(p)) {
					completeMessage(p);
					return;
				}

			if (p.message.chunked) {
				p.chunkSize = 0;
				p.chunkDigits = 0;
				p.chunkLineBytes = 0;
				p.state = s_chunk_size;
				return;
			}

			if (p.clSeen) {
				if (p.clValue == 0) { completeMessage(p); return; }
				if (p.clValue > p.limits.maxBodySize) {
					setError(p, Error::BODY_OVERFLOW);
					return;
				}
				p.bytesRemaining = p.clValue;
				// Предвыделяем не больше maxBodyPrealloc, чтобы анонсированный
				// (но ещё не доставленный) Content-Length не приводил к усилению
				// потребления памяти; дальше буфер растёт по факту прихода данных.
				if (p.storeBody) {
					const uint64_t cap = p.clValue < p.limits.maxBodyPrealloc
					                     ? p.clValue : static_cast<uint64_t>(p.limits.maxBodyPrealloc);
					p.message.body.reserve(static_cast<size_t>(cap));
				}
				p.state = s_body_identity;
				return;
			}

			// Transfer-Encoding есть, но не chunked-финальный.
			if (p.teSeen) {
				if (p.type == Type::REQUEST) {
					setError(p, Error::INVALID_TRANSFER_ENCODING); // запрос нельзя кадрировать
					return;
				}
				// Тело кадрируется закрытием соединения => оно не переиспользуемо.
				p.message.keepAlive = false;
				p.state = s_body_until_close; // ответ — читаем до закрытия
				return;
			}

			// Нет ни CL, ни TE.
			if (p.type == Type::REQUEST) {
				completeMessage(p); // у запроса по умолчанию тела нет
				return;
			}
			// Ответ: тело до закрытия соединения => keep-alive невозможен.
			p.message.keepAlive = false;
			p.state = s_body_until_close;
		}

			/// Завершение строки размера чанка.
			void onChunkSizeComplete(Parser & p) noexcept {
				if (p.chunkDigits == 0) {
					setError(p, Error::INVALID_CHUNK_SIZE);
					return;
				}
			if (p.chunkSize == 0) {
				// last-chunk -> трейлеры. Даём трейлерам собственный бюджет лимитов
				// (число/суммарный размер), но по-прежнему ограниченный — защита от DoS.
				p.headerCount = 0;
				p.headersTotalBytes = 0;
				p.inTrailers = true;
				p.state = s_trailer_start;
				return;
			}
				if (p.chunkSize > p.limits.maxChunkSize) {
					setError(p, Error::CHUNK_OVERFLOW);
					return;
				}
				if (p.handler && !fireSize(p, p.handler->onChunkHeader, p.chunkSize)) return;
				p.bytesRemaining = p.chunkSize;
				p.state = s_chunk_data;
			}

			/// Завершение статус-строки ответа: переход к заголовкам + onStatus.
			inline void endStatusLine(Parser & p) noexcept {
				p.state = s_header_start;
				if (p.handler)
					fireStatus(p, p.handler->onStatus, p.message.statusCode,
					           p.message.reason.data(), p.message.reason.size());
			}
		} // anonymous namespace

		// =========================================================================
		//  Публичный API.
		// =========================================================================

		void init(Parser & p, Type type) noexcept {
			p.limits = Limits{};
			p.message = Message{};
			p.message.type = type;
			p.error = Error::NONE;
			p.responseToHead = false;
			p.responseToConnect = false;
			p.handler = nullptr;
			p.userData = nullptr;
			p.storeBody = true;
			p.storeHeaders = true;
			p.type = type;
			p.state = s_start;

			p.curName.clear();
			p.curValue.clear();
			p.headerCount = 0;
			p.headersTotalBytes = 0;
			p.lineBytes = 0;
			p.chunkLineBytes = 0;
			p.bytesRemaining = 0;
			p.chunkSize = 0;
			p.chunkDigits = 0;
			p.clSeen = false;
			p.clValue = 0;
			p.teSeen = false;
			p.teChunkedFinal = false;
			p.teInvalid = false;
			p.connClose = false;
			p.connKeepAlive = false;
			p.inTrailers = false;
		}

		void reset(Parser & p) noexcept {
			// Сохраняем конфигурацию соединения (нужно для keep-alive/конвейера).
			// Per-message флаги responseToHead/responseToConnect НЕ сохраняем —
			// они относятся к конкретному запросу, а не к соединению.
			const Limits limits = p.limits;
			const Type type = p.type;
			const Handler * handler = p.handler;
			void * userData = p.userData;
			const bool storeBody = p.storeBody;
			const bool storeHeaders = p.storeHeaders;

			// Переиспользуем выделенную память контейнеров (clear сохраняет capacity)
			// вместо пересоздания Message{} — меньше нагрузки на аллокатор в keep-alive.
			p.message.method = Method::UNKNOWN;
			p.message.methodName.clear();
			p.message.target.clear();
			p.message.statusCode = 0;
			p.message.reason.clear();
			p.message.versionMajor = 0;
			p.message.versionMinor = 0;
			p.message.headers.clear();
			p.message.trailers.clear();
			p.message.body.clear();
			p.message.chunked = false;
			p.message.keepAlive = true;
			p.message.hasContentLength = false;
			p.message.contentLength = 0;
			p.message.complete = false;
			p.message.type = type;
			p.error = Error::NONE;

			p.limits = limits;
			p.type = type;
			p.responseToHead = false;
			p.responseToConnect = false;
			p.handler = handler;
			p.userData = userData;
			p.storeBody = storeBody;
			p.storeHeaders = storeHeaders;
			p.state = s_start;

			p.curName.clear();
			p.curValue.clear();
			p.headerCount = 0;
			p.headersTotalBytes = 0;
			p.lineBytes = 0;
			p.chunkLineBytes = 0;
			p.bytesRemaining = 0;
			p.chunkSize = 0;
			p.chunkDigits = 0;
			p.clSeen = false;
			p.clValue = 0;
			p.teSeen = false;
			p.teChunkedFinal = false;
			p.teInvalid = false;
			p.connClose = false;
			p.connKeepAlive = false;
			p.inTrailers = false;
		}

		size_t execute(Parser & p, const char * data, size_t len, Status & status) noexcept {
			if (p.error != Error::NONE) { status = Status::ERROR; return 0; }
			if (p.state == s_message_done) { status = Status::COMPLETE; return 0; }

			// Парсер копит токены/тело в std::string/std::vector, поэтому теоретически
			// возможен std::bad_alloc. Чтобы не нарушать noexcept-контракт (и не ронять
			// процесс через std::terminate), перехватываем всё и отдаём Error::INTERNAL.
			size_t i = 0;
			try {

			// Признак конца потока.
			if (len == 0) {
				if (p.state == s_body_until_close) {
					completeMessage(p);
					status = Status::COMPLETE;
					return 0;
				}
				status = Status::OK;
				return 0;
			}

			while (i < len) {
				const unsigned char ch = static_cast<unsigned char>(data[i]);

				// --- bulk-состояния (тело/чанк): обрабатываем большими блоками ---
				if (p.state == s_body_identity) {
					const uint64_t avail = static_cast<uint64_t>(len - i);
					const uint64_t take = avail < p.bytesRemaining ? avail : p.bytesRemaining;
					if (take > 0) {
						if (p.handler && !fireData(p, p.handler->onBody, data + i, static_cast<size_t>(take))) {
							status = Status::ERROR; return i;
						}
						if (p.storeBody) p.message.body.append(data + i, static_cast<size_t>(take));
					}
					i += static_cast<size_t>(take);
					p.bytesRemaining -= take;
					if (p.bytesRemaining == 0) {
						completeMessage(p);
						status = (p.error == Error::NONE) ? Status::COMPLETE : Status::ERROR;
						return i;
					}
					continue;
				}
				if (p.state == s_body_until_close) {
					const size_t avail = len - i;
					if (p.storeBody && p.message.body.size() + avail > p.limits.maxBodySize) {
						setError(p, Error::BODY_OVERFLOW);
						status = Status::ERROR;
						return i;
					}
					if (avail > 0) {
						if (p.handler && !fireData(p, p.handler->onBody, data + i, avail)) {
							status = Status::ERROR; return i;
						}
						if (p.storeBody) p.message.body.append(data + i, avail);
					}
					i += avail;
					continue; // завершение — только по EOF (len == 0)
				}
				if (p.state == s_chunk_data) {
					const uint64_t avail = static_cast<uint64_t>(len - i);
					const uint64_t take = avail < p.bytesRemaining ? avail : p.bytesRemaining;
					if (p.storeBody && p.message.body.size() + take > p.limits.maxBodySize) {
						setError(p, Error::BODY_OVERFLOW);
						status = Status::ERROR;
						return i;
					}
					if (take > 0) {
						if (p.handler && !fireData(p, p.handler->onBody, data + i, static_cast<size_t>(take))) {
							status = Status::ERROR; return i;
						}
						if (p.storeBody) p.message.body.append(data + i, static_cast<size_t>(take));
					}
					i += static_cast<size_t>(take);
					p.bytesRemaining -= take;
					if (p.bytesRemaining == 0)
						p.state = s_chunk_data_almost_done;
					continue;
				}

				// --- крупноблочное сканирование токенов (переносимый «SIMD») ---
				// Непрерывный участок допустимых символов сканируется по lookup-таблице
				// и копируется одним append; на разделителе управление уходит в автомат.
				if (p.state == s_req_target) {
					size_t j = i;
					while (j < len && isTargetCh(static_cast<unsigned char>(data[j]))) ++j;
					if (j > i) {
						const size_t run = j - i;
						if (p.lineBytes + run > p.limits.maxRequestLine) {
							setError(p, Error::URL_OVERFLOW); status = Status::ERROR; return i;
						}
						p.lineBytes += run;
						p.message.target.append(data + i, run);
						i = j;
						continue;
					}
				} else if (p.state == s_header_name || p.state == s_trailer_name) {
					size_t j = i;
					while (j < len && isToken(static_cast<unsigned char>(data[j]))) ++j;
					if (j > i) {
						const size_t run = j - i;
						if (p.curName.size() + run > p.limits.maxHeaderName) {
							setError(p, Error::HEADER_OVERFLOW); status = Status::ERROR; return i;
						}
						p.curName.append(data + i, run);
						i = j;
						continue;
					}
				} else if (p.state == s_header_value || p.state == s_trailer_value) {
					size_t j = i;
					while (j < len && isValueCh(static_cast<unsigned char>(data[j]))) ++j;
					if (j > i) {
						const size_t run = j - i;
						if (p.curValue.size() + run > p.limits.maxHeaderValue) {
							setError(p, Error::HEADER_OVERFLOW); status = Status::ERROR; return i;
						}
						p.curValue.append(data + i, run);
						i = j;
						continue;
					}
				}

				// --- посимвольные состояния ---
				switch (static_cast<State>(p.state)) {
					case s_start: {
						if (p.handler && !fireHook(p, p.handler->onMessageBegin)) break;
						if (p.type == Type::REQUEST) {
							if (!isToken(ch)) { setError(p, Error::INVALID_METHOD); break; }
							p.message.methodName.push_back(static_cast<char>(ch));
							p.lineBytes = 1;
							p.state = s_req_method;
						} else {
							if (ch != 'H') { setError(p, Error::INVALID_VERSION); break; }
							p.lineBytes = 1;
							p.state = s_res_http_H;
						}
						break;
					}

					// ------------------- request-line -------------------
					case s_req_method: {
						if (ch == ' ') {
							p.message.method = classifyMethod(p.message.methodName);
							p.state = s_req_target_start;
						} else if (isToken(ch)) {
							if (++p.lineBytes > p.limits.maxRequestLine) { setError(p, Error::URL_OVERFLOW); break; }
							p.message.methodName.push_back(static_cast<char>(ch));
						} else {
							setError(p, Error::INVALID_METHOD);
						}
						break;
					}
					case s_req_target_start: {
						if (ch == ' ') break; // толерантно пропускаем лишние пробелы
						if (!isTargetCh(ch)) { setError(p, Error::INVALID_TARGET); break; }
						p.message.target.push_back(static_cast<char>(ch));
						if (++p.lineBytes > p.limits.maxRequestLine) { setError(p, Error::URL_OVERFLOW); break; }
						p.state = s_req_target;
						break;
					}
					case s_req_target: {
						if (ch == ' ') {
							if (p.handler && !fireData(p, p.handler->onTarget,
							                           p.message.target.data(), p.message.target.size())) break;
							p.state = s_req_http_start;
							break;
						}
						if (!isTargetCh(ch)) { setError(p, Error::INVALID_TARGET); break; }
						if (++p.lineBytes > p.limits.maxRequestLine) { setError(p, Error::URL_OVERFLOW); break; }
						p.message.target.push_back(static_cast<char>(ch));
						break;
					}
					case s_req_http_start:
						if (ch == ' ') break;
						if (ch != 'H') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_req_http_H; break;
					case s_req_http_H:
						if (ch != 'T') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_req_http_HT; break;
					case s_req_http_HT:
						if (ch != 'T') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_req_http_HTT; break;
					case s_req_http_HTT:
						if (ch != 'P') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_req_http_HTTP; break;
					case s_req_http_HTTP:
						if (ch != '/') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_req_http_slash; break;
					case s_req_http_slash:
						if (!isDigit(ch)) { setError(p, Error::INVALID_VERSION); break; }
						p.message.versionMajor = static_cast<uint8_t>(ch - '0');
						p.state = s_req_http_dot; break;
					case s_req_http_major:
						// (зарезервировано; major читается в slash)
						setError(p, Error::INTERNAL); break;
					case s_req_http_dot:
						if (ch != '.') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_req_http_minor; break;
					case s_req_http_minor:
						if (!isDigit(ch)) { setError(p, Error::INVALID_VERSION); break; }
						p.message.versionMinor = static_cast<uint8_t>(ch - '0');
						p.state = s_req_line_almost_done; break;
					case s_req_line_almost_done:
						if (ch == '\r') { p.state = s_req_line_lf; break; }
						if (ch == '\n') { p.state = s_header_start; break; }
						setError(p, Error::INVALID_VERSION); break;
					case s_req_line_lf:
						if (ch != '\n') { setError(p, Error::INVALID_EOL); break; }
						p.state = s_header_start; break;

					// ------------------- status-line -------------------
					case s_res_http_H:
						if (ch != 'T') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_res_http_HT; break;
					case s_res_http_HT:
						if (ch != 'T') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_res_http_HTT; break;
					case s_res_http_HTT:
						if (ch != 'P') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_res_http_HTTP; break;
					case s_res_http_HTTP:
						if (ch != '/') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_res_http_slash; break;
					case s_res_http_slash:
						if (!isDigit(ch)) { setError(p, Error::INVALID_VERSION); break; }
						p.message.versionMajor = static_cast<uint8_t>(ch - '0');
						p.state = s_res_http_dot; break;
					case s_res_http_major:
						setError(p, Error::INTERNAL); break;
					case s_res_http_dot:
						if (ch != '.') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_res_http_minor; break;
					case s_res_http_minor:
						if (!isDigit(ch)) { setError(p, Error::INVALID_VERSION); break; }
						p.message.versionMinor = static_cast<uint8_t>(ch - '0');
						p.state = s_res_first_space; break;
					case s_res_first_space:
						// После версии обязателен ровно один SP (RFC 7230 §3.1.2).
						if (ch != ' ') { setError(p, Error::INVALID_VERSION); break; }
						p.state = s_res_status_start; break;
					case s_res_status_start:
						if (ch == ' ') break; // дополнительные пробелы перед кодом — толерантно
						if (!isDigit(ch)) { setError(p, Error::INVALID_STATUS); break; }
						p.message.statusCode = static_cast<uint16_t>(ch - '0');
						p.chunkDigits = 1; // переиспользуем как счётчик цифр кода
						p.state = s_res_status_code; break;
					case s_res_status_code: {
						if (isDigit(ch)) {
							if (p.chunkDigits >= 3) { setError(p, Error::INVALID_STATUS); break; }
							p.message.statusCode = static_cast<uint16_t>(p.message.statusCode * 10 + (ch - '0'));
							++p.chunkDigits;
							break;
						}
						if (p.chunkDigits != 3) { setError(p, Error::INVALID_STATUS); break; }
						if (ch == ' ') { p.state = s_res_reason_start; break; }
						if (ch == '\r') { p.state = s_res_line_lf; break; }
						if (ch == '\n') { endStatusLine(p); break; }
						setError(p, Error::INVALID_STATUS); break;
					}
					case s_res_reason_start:
						if (ch == '\r') { p.state = s_res_line_lf; break; }
						if (ch == '\n') { endStatusLine(p); break; }
						if (!isReasonCh(ch)) { setError(p, Error::INVALID_STATUS); break; }
						p.message.reason.push_back(static_cast<char>(ch));
						p.state = s_res_reason; break;
					case s_res_reason:
						if (ch == '\r') { p.state = s_res_line_lf; break; }
						if (ch == '\n') { endStatusLine(p); break; }
						if (!isReasonCh(ch)) { setError(p, Error::INVALID_STATUS); break; }
						if (p.message.reason.size() >= p.limits.maxRequestLine) { setError(p, Error::URL_OVERFLOW); break; }
						p.message.reason.push_back(static_cast<char>(ch));
						break;
					case s_res_line_lf:
						if (ch != '\n') { setError(p, Error::INVALID_EOL); break; }
						endStatusLine(p); break;

					// ------------------- заголовки -------------------
					case s_header_start:
						if (ch == '\r') { p.state = s_headers_lf; break; }
						if (ch == '\n') { beginBody(p); break; }
						if (ch == ' ' || ch == '\t') { setError(p, Error::INVALID_HEADER_TOKEN); break; } // obs-fold
						if (!isToken(ch)) { setError(p, Error::INVALID_HEADER_TOKEN); break; }
						p.curName.clear();
						p.curName.push_back(static_cast<char>(ch));
						p.state = s_header_name; break;
					case s_header_name:
						if (ch == ':') { p.curValue.clear(); p.state = s_header_value_ows; break; }
						if (!isToken(ch)) { setError(p, Error::INVALID_HEADER_TOKEN); break; }
						if (p.curName.size() >= p.limits.maxHeaderName) { setError(p, Error::HEADER_OVERFLOW); break; }
						p.curName.push_back(static_cast<char>(ch));
						break;
					case s_header_value_ows:
						if (ch == ' ' || ch == '\t') break;       // пропуск ведущих OWS
						if (ch == '\r') { p.state = s_header_almost_done; break; }
						if (ch == '\n') { if (!finishHeader(p)) break; p.state = s_header_start; break; }
						if (!isValueCh(ch)) { setError(p, Error::INVALID_HEADER_VALUE); break; }
						p.curValue.push_back(static_cast<char>(ch));
						p.state = s_header_value; break;
					case s_header_value:
						if (ch == '\r') { p.state = s_header_almost_done; break; }
						if (ch == '\n') { if (!finishHeader(p)) break; p.state = s_header_start; break; }
						if (!isValueCh(ch)) { setError(p, Error::INVALID_HEADER_VALUE); break; }
						if (p.curValue.size() >= p.limits.maxHeaderValue) { setError(p, Error::HEADER_OVERFLOW); break; }
						p.curValue.push_back(static_cast<char>(ch));
						break;
					case s_header_almost_done:
						if (ch != '\n') { setError(p, Error::INVALID_EOL); break; }
						if (!finishHeader(p)) break;
						p.state = s_header_start; break;
					case s_headers_lf:
						if (ch != '\n') { setError(p, Error::INVALID_EOL); break; }
						beginBody(p); break;

					// ------------------- chunked -------------------
					case s_chunk_size: {
						if (++p.chunkLineBytes > p.limits.maxChunkLine) { setError(p, Error::CHUNK_OVERFLOW); break; }
						const int hv = hexVal(ch);
						if (hv >= 0) {
							if (p.chunkSize > (UINT64_MAX >> 4)) { setError(p, Error::CHUNK_OVERFLOW); break; }
							p.chunkSize = (p.chunkSize << 4) | static_cast<uint64_t>(hv);
							++p.chunkDigits;
							break;
						}
						if (ch == ';') { p.state = s_chunk_ext; break; }
						if (ch == '\r') { p.state = s_chunk_size_lf; break; }
						if (ch == '\n') { onChunkSizeComplete(p); break; }
						setError(p, Error::INVALID_CHUNK_SIZE); break;
					}
					case s_chunk_ext:
						if (++p.chunkLineBytes > p.limits.maxChunkLine) { setError(p, Error::CHUNK_OVERFLOW); break; }
						if (ch == '\r') { p.state = s_chunk_size_lf; break; }
						if (ch == '\n') { onChunkSizeComplete(p); break; }
						// chunk-ext игнорируем (валидируем только на печатаемость).
						if (ch < 0x20 && ch != '\t') { setError(p, Error::INVALID_CHUNK_SIZE); break; }
						break;
					case s_chunk_size_lf:
						if (ch != '\n') { setError(p, Error::INVALID_EOL); break; }
						onChunkSizeComplete(p); break;
					case s_chunk_data_almost_done:
						if (ch == '\r') { p.state = s_chunk_data_lf; break; }
						if (ch == '\n') {
							if (p.handler && !fireHook(p, p.handler->onChunkComplete)) break;
							p.chunkSize = 0; p.chunkDigits = 0; p.chunkLineBytes = 0; p.state = s_chunk_size; break;
						}
						setError(p, Error::INVALID_CHUNK_TERMINATOR); break;
					case s_chunk_data_lf:
						if (ch != '\n') { setError(p, Error::INVALID_CHUNK_TERMINATOR); break; }
						if (p.handler && !fireHook(p, p.handler->onChunkComplete)) break;
						p.chunkSize = 0; p.chunkDigits = 0; p.chunkLineBytes = 0; p.state = s_chunk_size; break;

					// ------------------- трейлеры -------------------
					case s_trailer_start:
						if (ch == '\r') { p.state = s_trailers_lf; break; }
						if (ch == '\n') { completeMessage(p); break; }
						if (ch == ' ' || ch == '\t') { setError(p, Error::INVALID_HEADER_TOKEN); break; }
						if (!isToken(ch)) { setError(p, Error::INVALID_HEADER_TOKEN); break; }
						p.curName.clear();
						p.curName.push_back(static_cast<char>(ch));
						p.state = s_trailer_name; break;
					case s_trailer_name:
						if (ch == ':') { p.curValue.clear(); p.state = s_trailer_value_ows; break; }
						if (!isToken(ch)) { setError(p, Error::INVALID_HEADER_TOKEN); break; }
						if (p.curName.size() >= p.limits.maxHeaderName) { setError(p, Error::HEADER_OVERFLOW); break; }
						p.curName.push_back(static_cast<char>(ch));
						break;
					case s_trailer_value_ows:
						if (ch == ' ' || ch == '\t') break;
						if (ch == '\r') { p.state = s_trailer_almost_done; break; }
						if (ch == '\n') { if (!finishHeader(p)) break; p.state = s_trailer_start; break; }
						if (!isValueCh(ch)) { setError(p, Error::INVALID_HEADER_VALUE); break; }
						p.curValue.push_back(static_cast<char>(ch));
						p.state = s_trailer_value; break;
					case s_trailer_value:
						if (ch == '\r') { p.state = s_trailer_almost_done; break; }
						if (ch == '\n') { if (!finishHeader(p)) break; p.state = s_trailer_start; break; }
						if (!isValueCh(ch)) { setError(p, Error::INVALID_HEADER_VALUE); break; }
						if (p.curValue.size() >= p.limits.maxHeaderValue) { setError(p, Error::HEADER_OVERFLOW); break; }
						p.curValue.push_back(static_cast<char>(ch));
						break;
					case s_trailer_almost_done:
						if (ch != '\n') { setError(p, Error::INVALID_EOL); break; }
						if (!finishHeader(p)) break;
						p.state = s_trailer_start; break;
					case s_trailers_lf:
						if (ch != '\n') { setError(p, Error::INVALID_EOL); break; }
						completeMessage(p); break;

					default:
						setError(p, Error::INTERNAL); break;
				}

				if (p.error != Error::NONE) { status = Status::ERROR; return i; }
				if (p.state == s_message_done) { status = Status::COMPLETE; return i + 1; }
				++i;
			}

			status = Status::OK;
			return i;

			} catch (...) {
				// Сбой аллокации или исключение из пользовательского callback'а.
				setError(p, Error::INTERNAL);
				status = Status::ERROR;
				return i;
			}
		}

		const char * methodName(Method m) noexcept {
			switch (m) {
				case Method::GET:     return "GET";
				case Method::HEAD:    return "HEAD";
				case Method::POST:    return "POST";
				case Method::PUT:     return "PUT";
				case Method::DELETE_: return "DELETE";
				case Method::CONNECT: return "CONNECT";
				case Method::OPTIONS: return "OPTIONS";
				case Method::TRACE:   return "TRACE";
				case Method::PATCH:   return "PATCH";
				case Method::COPY:    return "COPY";
				case Method::LOCK:    return "LOCK";
				case Method::MKCOL:   return "MKCOL";
				case Method::MOVE:    return "MOVE";
				case Method::PROPFIND:  return "PROPFIND";
				case Method::PROPPATCH: return "PROPPATCH";
				case Method::SEARCH:  return "SEARCH";
				case Method::UNLOCK:  return "UNLOCK";
				case Method::BIND:    return "BIND";
				case Method::REBIND:  return "REBIND";
				case Method::UNBIND:  return "UNBIND";
				case Method::ACL:     return "ACL";
				case Method::REPORT:  return "REPORT";
				case Method::MKACTIVITY: return "MKACTIVITY";
				case Method::CHECKOUT: return "CHECKOUT";
				case Method::MERGE:   return "MERGE";
				case Method::MSEARCH: return "M-SEARCH";
				case Method::NOTIFY:  return "NOTIFY";
				case Method::SUBSCRIBE:   return "SUBSCRIBE";
				case Method::UNSUBSCRIBE: return "UNSUBSCRIBE";
				case Method::PURGE:   return "PURGE";
				case Method::MKCALENDAR:  return "MKCALENDAR";
				case Method::LINK:    return "LINK";
				case Method::UNLINK:  return "UNLINK";
				case Method::PRI:     return "PRI";
				case Method::SOURCE:  return "SOURCE";
				default:              return "UNKNOWN";
			}
		}

		const char * errorName(Error e) noexcept {
			switch (e) {
				case Error::NONE:                      return "NONE";
				case Error::INTERNAL:                  return "INTERNAL";
				case Error::INVALID_METHOD:            return "INVALID_METHOD";
				case Error::INVALID_TARGET:            return "INVALID_TARGET";
				case Error::INVALID_VERSION:           return "INVALID_VERSION";
				case Error::INVALID_STATUS:            return "INVALID_STATUS";
				case Error::INVALID_HEADER_TOKEN:      return "INVALID_HEADER_TOKEN";
				case Error::INVALID_HEADER_VALUE:      return "INVALID_HEADER_VALUE";
				case Error::INVALID_CONTENT_LENGTH:    return "INVALID_CONTENT_LENGTH";
				case Error::INVALID_TRANSFER_ENCODING: return "INVALID_TRANSFER_ENCODING";
				case Error::INVALID_CHUNK_SIZE:        return "INVALID_CHUNK_SIZE";
				case Error::INVALID_CHUNK_TERMINATOR:  return "INVALID_CHUNK_TERMINATOR";
				case Error::INVALID_EOL:               return "INVALID_EOL";
				case Error::INVALID_CONSTANT:          return "INVALID_CONSTANT";
				case Error::CONTENT_LENGTH_CONFLICT:   return "CONTENT_LENGTH_CONFLICT";
				case Error::HEADER_OVERFLOW:           return "HEADER_OVERFLOW";
				case Error::URL_OVERFLOW:              return "URL_OVERFLOW";
				case Error::BODY_OVERFLOW:             return "BODY_OVERFLOW";
				case Error::CHUNK_OVERFLOW:            return "CHUNK_OVERFLOW";
				case Error::TOO_MANY_HEADERS:          return "TOO_MANY_HEADERS";
				case Error::ABORTED:                   return "ABORTED";
				default:                               return "UNKNOWN_ERROR";
			}
		}

		const Header * findHeader(const Message & m, const char * name) noexcept {
			const size_t n = std::strlen(name);
			for (const Header & h : m.headers) {
				if (h.name.size() != n) continue;
				bool eq = true;
				for (size_t i = 0; i < n; ++i) {
					if (lower(h.name[i]) != lower(name[i])) { eq = false; break; }
				}
				if (eq) return &h;
			}
			return nullptr;
		}
	}
}
