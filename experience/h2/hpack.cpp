/**
 * @file hpack.cpp
 * @brief Реализация HPACK (RFC 7541).
 *
 * Реализовано: статическая таблица, целочисленный/строковый кодек, Huffman
 * (таблица кодов из nghttp2_hd_huffman_data.c, MIT; логика написана заново),
 * динамическая таблица с вытеснением, декодер и кодер с индексацией заголовков.
 *
 * Сборка:
 *   g++ -std=c++17 -O2 -Wall -Wextra -c hpack.cpp
 */

#include "hpack.hpp"

namespace awh {
	namespace http2 {
		namespace hpack {
			/**
			 * Статическая таблица HPACK (RFC 7541, Appendix A). Индекс 0 не используется.
			 */
			namespace {
				const static_entry_t STATIC[STATIC_TABLE_SIZE + 1] = {
					{ "", "" }, // 0 — заглушка (индексация 1-based)
					{ ":authority", "" },                       // 1
					{ ":method", "GET" },                       // 2
					{ ":method", "POST" },                      // 3
					{ ":path", "/" },                           // 4
					{ ":path", "/index.html" },                 // 5
					{ ":scheme", "http" },                      // 6
					{ ":scheme", "https" },                     // 7
					{ ":status", "200" },                       // 8
					{ ":status", "204" },                       // 9
					{ ":status", "206" },                       // 10
					{ ":status", "304" },                       // 11
					{ ":status", "400" },                       // 12
					{ ":status", "404" },                       // 13
					{ ":status", "500" },                       // 14
					{ "accept-charset", "" },                   // 15
					{ "accept-encoding", "gzip, deflate" },     // 16
					{ "accept-language", "" },                  // 17
					{ "accept-ranges", "" },                    // 18
					{ "accept", "" },                           // 19
					{ "access-control-allow-origin", "" },      // 20
					{ "age", "" },                              // 21
					{ "allow", "" },                            // 22
					{ "authorization", "" },                    // 23
					{ "cache-control", "" },                    // 24
					{ "content-disposition", "" },              // 25
					{ "content-encoding", "" },                 // 26
					{ "content-language", "" },                 // 27
					{ "content-length", "" },                   // 28
					{ "content-location", "" },                 // 29
					{ "content-range", "" },                    // 30
					{ "content-type", "" },                     // 31
					{ "cookie", "" },                           // 32
					{ "date", "" },                             // 33
					{ "etag", "" },                             // 34
					{ "expect", "" },                           // 35
					{ "expires", "" },                          // 36
					{ "from", "" },                             // 37
					{ "host", "" },                             // 38
					{ "if-match", "" },                         // 39
					{ "if-modified-since", "" },                // 40
					{ "if-none-match", "" },                    // 41
					{ "if-range", "" },                         // 42
					{ "if-unmodified-since", "" },              // 43
					{ "last-modified", "" },                    // 44
					{ "link", "" },                             // 45
					{ "location", "" },                         // 46
					{ "max-forwards", "" },                     // 47
					{ "proxy-authenticate", "" },               // 48
					{ "proxy-authorization", "" },              // 49
					{ "range", "" },                            // 50
					{ "referer", "" },                          // 51
					{ "refresh", "" },                          // 52
					{ "retry-after", "" },                      // 53
					{ "server", "" },                           // 54
					{ "set-cookie", "" },                       // 55
					{ "strict-transport-security", "" },        // 56
					{ "transfer-encoding", "" },                // 57
					{ "user-agent", "" },                       // 58
					{ "vary", "" },                             // 59
					{ "via", "" },                              // 60
					{ "www-authenticate", "" }                  // 61
				};
			}

			const static_entry_t * staticTable(size_t index) noexcept {
				if((index == 0) || (index > STATIC_TABLE_SIZE)) return nullptr;
				return &STATIC[index];
			}

			// ───────────────────── Целочисленный кодек (RFC 7541 §5.1) ─────────────────────

			status_t decodeInteger(const uint8_t * data, size_t size, uint8_t prefixBits, uint64_t & value, size_t & consumed) noexcept {
				if(size < 1) return status_t::INCOMPLETE;
				const uint8_t prefixMax = static_cast <uint8_t> ((1u << prefixBits) - 1);
				uint64_t result = data[0] & prefixMax;
				size_t pos = 1;
				if(result < prefixMax){
					value = result;
					consumed = pos;
					return status_t::OK;
				}
				// Продолжение: 7 бит на байт, старший бит — признак продолжения.
				uint32_t shift = 0;
				while(true){
					if(pos >= size) return status_t::INCOMPLETE;
					const uint8_t b = data[pos++];
					// Защита от переполнения uint64_t (RFC 7541 §5.1 рекомендует ограничивать).
					if(shift >= 64) return status_t::ERROR;
					const uint64_t add = static_cast <uint64_t> (b & 0x7F) << shift;
					if(add > (UINT64_MAX - result)) return status_t::ERROR;
					result += add;
					if((b & 0x80) == 0) break;
					shift += 7;
				}
				value = result;
				consumed = pos;
				return status_t::OK;
			}

			void encodeInteger(std::string & out, uint64_t value, uint8_t prefixBits, uint8_t prefixValue) noexcept {
				const uint8_t prefixMax = static_cast <uint8_t> ((1u << prefixBits) - 1);
				const uint8_t high = prefixValue & static_cast <uint8_t> (~prefixMax);
				if(value < prefixMax){
					out.push_back(static_cast <char> (high | static_cast <uint8_t> (value)));
					return;
				}
				out.push_back(static_cast <char> (high | prefixMax));
				value -= prefixMax;
				while(value >= 0x80){
					out.push_back(static_cast <char> ((value & 0x7F) | 0x80));
					value >>= 7;
				}
				out.push_back(static_cast <char> (value));
			}

			// ───────────────────── Huffman (RFC 7541 Appendix B) ─────────────────────
			//
			// Таблица кодов (code, nbits) — каноническая из RFC 7541 Appendix B; численные
			// значения скопированы из nghttp2/lib/nghttp2_hd_huffman_data.c (лицензия MIT).
			// Поле code хранит код, выровненный по старшему биту 32-битного слова.
			// EOS (символ 256) опущен: при кодировании хвост добивается единичными битами
			// (это префикс EOS), при декодировании EOS внутри потока — ошибка.
			//
			// Логика кодера/декодера написана заново на C++17: дерево декодирования строится
			// один раз из таблицы кодов (ленивая инициализация).
			namespace {
				struct huff_sym_t {
					uint8_t  nbits; // длина кода в битах (5..30)
					uint32_t code;  // код, выровненный по старшему биту
				};
				const huff_sym_t HUFF[256] = {
					{13,0xffc00000u},{23,0xffffb000u},{28,0xfffffe20u},{28,0xfffffe30u},
					{28,0xfffffe40u},{28,0xfffffe50u},{28,0xfffffe60u},{28,0xfffffe70u},
					{28,0xfffffe80u},{24,0xffffea00u},{30,0xfffffff0u},{28,0xfffffe90u},
					{28,0xfffffea0u},{30,0xfffffff4u},{28,0xfffffeb0u},{28,0xfffffec0u},
					{28,0xfffffed0u},{28,0xfffffee0u},{28,0xfffffef0u},{28,0xffffff00u},
					{28,0xffffff10u},{28,0xffffff20u},{30,0xfffffff8u},{28,0xffffff30u},
					{28,0xffffff40u},{28,0xffffff50u},{28,0xffffff60u},{28,0xffffff70u},
					{28,0xffffff80u},{28,0xffffff90u},{28,0xffffffa0u},{28,0xffffffb0u},
					{6,0x50000000u},{10,0xfe000000u},{10,0xfe400000u},{12,0xffa00000u},
					{13,0xffc80000u},{6,0x54000000u},{8,0xf8000000u},{11,0xff400000u},
					{10,0xfe800000u},{10,0xfec00000u},{8,0xf9000000u},{11,0xff600000u},
					{8,0xfa000000u},{6,0x58000000u},{6,0x5c000000u},{6,0x60000000u},
					{5,0x0u},{5,0x08000000u},{5,0x10000000u},{6,0x64000000u},
					{6,0x68000000u},{6,0x6c000000u},{6,0x70000000u},{6,0x74000000u},
					{6,0x78000000u},{6,0x7c000000u},{7,0xb8000000u},{8,0xfb000000u},
					{15,0xfff80000u},{6,0x80000000u},{12,0xffb00000u},{10,0xff000000u},
					{13,0xffd00000u},{6,0x84000000u},{7,0xba000000u},{7,0xbc000000u},
					{7,0xbe000000u},{7,0xc0000000u},{7,0xc2000000u},{7,0xc4000000u},
					{7,0xc6000000u},{7,0xc8000000u},{7,0xca000000u},{7,0xcc000000u},
					{7,0xce000000u},{7,0xd0000000u},{7,0xd2000000u},{7,0xd4000000u},
					{7,0xd6000000u},{7,0xd8000000u},{7,0xda000000u},{7,0xdc000000u},
					{7,0xde000000u},{7,0xe0000000u},{7,0xe2000000u},{7,0xe4000000u},
					{8,0xfc000000u},{7,0xe6000000u},{8,0xfd000000u},{13,0xffd80000u},
					{19,0xfffe0000u},{13,0xffe00000u},{14,0xfff00000u},{6,0x88000000u},
					{15,0xfffa0000u},{5,0x18000000u},{6,0x8c000000u},{5,0x20000000u},
					{6,0x90000000u},{5,0x28000000u},{6,0x94000000u},{6,0x98000000u},
					{6,0x9c000000u},{5,0x30000000u},{7,0xe8000000u},{7,0xea000000u},
					{6,0xa0000000u},{6,0xa4000000u},{6,0xa8000000u},{5,0x38000000u},
					{6,0xac000000u},{7,0xec000000u},{6,0xb0000000u},{5,0x40000000u},
					{5,0x48000000u},{6,0xb4000000u},{7,0xee000000u},{7,0xf0000000u},
					{7,0xf2000000u},{7,0xf4000000u},{7,0xf6000000u},{15,0xfffc0000u},
					{11,0xff800000u},{14,0xfff40000u},{13,0xffe80000u},{28,0xffffffc0u},
					{20,0xfffe6000u},{22,0xffff4800u},{20,0xfffe7000u},{20,0xfffe8000u},
					{22,0xffff4c00u},{22,0xffff5000u},{22,0xffff5400u},{23,0xffffb200u},
					{22,0xffff5800u},{23,0xffffb400u},{23,0xffffb600u},{23,0xffffb800u},
					{23,0xffffba00u},{23,0xffffbc00u},{24,0xffffeb00u},{23,0xffffbe00u},
					{24,0xffffec00u},{24,0xffffed00u},{22,0xffff5c00u},{23,0xffffc000u},
					{24,0xffffee00u},{23,0xffffc200u},{23,0xffffc400u},{23,0xffffc600u},
					{23,0xffffc800u},{21,0xfffee000u},{22,0xffff6000u},{23,0xffffca00u},
					{22,0xffff6400u},{23,0xffffcc00u},{23,0xffffce00u},{24,0xffffef00u},
					{22,0xffff6800u},{21,0xfffee800u},{20,0xfffe9000u},{22,0xffff6c00u},
					{22,0xffff7000u},{23,0xffffd000u},{23,0xffffd200u},{21,0xfffef000u},
					{23,0xffffd400u},{22,0xffff7400u},{22,0xffff7800u},{24,0xfffff000u},
					{21,0xfffef800u},{22,0xffff7c00u},{23,0xffffd600u},{23,0xffffd800u},
					{21,0xffff0000u},{21,0xffff0800u},{22,0xffff8000u},{21,0xffff1000u},
					{23,0xffffda00u},{22,0xffff8400u},{23,0xffffdc00u},{23,0xffffde00u},
					{20,0xfffea000u},{22,0xffff8800u},{22,0xffff8c00u},{22,0xffff9000u},
					{23,0xffffe000u},{22,0xffff9400u},{22,0xffff9800u},{23,0xffffe200u},
					{26,0xfffff800u},{26,0xfffff840u},{20,0xfffeb000u},{19,0xfffe2000u},
					{22,0xffff9c00u},{23,0xffffe400u},{22,0xffffa000u},{25,0xfffff600u},
					{26,0xfffff880u},{26,0xfffff8c0u},{26,0xfffff900u},{27,0xfffffbc0u},
					{27,0xfffffbe0u},{26,0xfffff940u},{24,0xfffff100u},{25,0xfffff680u},
					{19,0xfffe4000u},{21,0xffff1800u},{26,0xfffff980u},{27,0xfffffc00u},
					{27,0xfffffc20u},{26,0xfffff9c0u},{27,0xfffffc40u},{24,0xfffff200u},
					{21,0xffff2000u},{21,0xffff2800u},{26,0xfffffa00u},{26,0xfffffa40u},
					{28,0xffffffd0u},{27,0xfffffc60u},{27,0xfffffc80u},{27,0xfffffca0u},
					{20,0xfffec000u},{24,0xfffff300u},{20,0xfffed000u},{21,0xffff3000u},
					{22,0xffffa400u},{21,0xffff3800u},{21,0xffff4000u},{23,0xffffe600u},
					{22,0xffffa800u},{22,0xffffac00u},{25,0xfffff700u},{25,0xfffff780u},
					{24,0xfffff400u},{24,0xfffff500u},{26,0xfffffa80u},{23,0xffffe800u},
					{26,0xfffffac0u},{27,0xfffffcc0u},{26,0xfffffb00u},{26,0xfffffb40u},
					{27,0xfffffce0u},{27,0xfffffd00u},{27,0xfffffd20u},{27,0xfffffd40u},
					{27,0xfffffd60u},{28,0xffffffe0u},{27,0xfffffd80u},{27,0xfffffda0u},
					{27,0xfffffdc0u},{27,0xfffffde0u},{27,0xfffffe00u},{26,0xfffffb80u}
				};

				/// Узел дерева декодирования: sym >= 0 у листа, -1 у внутреннего узла.
				struct huff_node_t {
					int16_t sym;
					int32_t child[2];
				};

				/// Построить (один раз) дерево декодирования из таблицы кодов.
				const std::vector <huff_node_t> & huffTree() noexcept {
					static const std::vector <huff_node_t> tree = [](){
						std::vector <huff_node_t> t;
						t.push_back(huff_node_t{ -1, { -1, -1 } }); // корень
						for(int sym = 0; sym < 256; ++sym){
							const uint32_t code = HUFF[sym].code;
							const uint8_t  nbits = HUFF[sym].nbits;
							int32_t cur = 0;
							for(int i = 0; i < nbits; ++i){
								const int bit = (code >> (31 - i)) & 1u;
								if(t[cur].child[bit] < 0){
									t.push_back(huff_node_t{ -1, { -1, -1 } });
									t[cur].child[bit] = static_cast <int32_t> (t.size() - 1);
								}
								cur = t[cur].child[bit];
							}
							t[cur].sym = static_cast <int16_t> (sym);
						}
						return t;
					}();
					return tree;
				}
			}

			bool huffmanDecode(const uint8_t * data, size_t size, std::string & out) noexcept {
				const std::vector <huff_node_t> & tree = huffTree();
				int32_t cur = 0;     // текущий узел
				int     padLen = 0;  // длина текущего незавершённого пути в битах
				bool    padOnes = true;
				for(size_t i = 0; i < size; ++i){
					const uint8_t byte = data[i];
					for(int b = 7; b >= 0; --b){
						const int bit = (byte >> b) & 1;
						cur = tree[cur].child[bit];
						if(cur < 0) return false; // недопустимая кодовая последовательность
						++padLen;
						padOnes = padOnes && (bit == 1);
						if(tree[cur].sym >= 0){ // достигнут лист — символ декодирован
							out.push_back(static_cast <char> (tree[cur].sym));
							cur = 0;
							padLen = 0;
							padOnes = true;
						}
					}
				}
				// Корректный конец: либо точно на границе символа, либо хвост из <= 7
				// единичных бит (префикс EOS). Иначе — COMPRESSION_ERROR.
				if(cur != 0){
					if((padLen > 7) || !padOnes) return false;
				}
				return true;
			}

			void huffmanEncode(std::string_view in, std::string & out) noexcept {
				uint64_t buf = 0; // битовый аккумулятор
				int cnt = 0;      // число накопленных бит
				for(unsigned char c : in){
					const uint8_t  nbits = HUFF[c].nbits;
					const uint32_t code  = HUFF[c].code >> (32 - nbits); // правое выравнивание
					buf = (buf << nbits) | code;
					cnt += nbits;
					while(cnt >= 8){
						cnt -= 8;
						out.push_back(static_cast <char> ((buf >> cnt) & 0xFF));
					}
				}
				if(cnt > 0){ // добиваем хвост единичными битами (префикс EOS)
					const int rem = 8 - cnt;
					buf = (buf << rem) | ((1u << rem) - 1);
					out.push_back(static_cast <char> (buf & 0xFF));
				}
			}

			size_t huffmanLength(std::string_view in) noexcept {
				size_t bits = 0;
				for(unsigned char c : in) bits += HUFF[c].nbits;
				return (bits + 7) / 8;
			}

			// ───────────────────── Динамическая таблица (RFC 7541 §2.3.2) ─────────────────────

			DynamicTable::DynamicTable(uint32_t maxSize) noexcept : _maxSize(maxSize) {}

			void DynamicTable::add(std::string_view name, std::string_view value) noexcept {
				const uint32_t entrySize = static_cast <uint32_t> (name.size() + value.size() + 32);
				// Запись больше всей таблицы → таблица очищается, запись не добавляется (§4.4).
				if(entrySize > _maxSize){
					_entries.clear();
					_size = 0;
					return;
				}
				_entries.push_front(field_t{ std::string(name), std::string(value) });
				_size += entrySize;
				evict();
			}

			const field_t * DynamicTable::at(size_t index) const noexcept {
				if((index < 1) || (index > _entries.size())) return nullptr;
				return &_entries[index - 1];
			}

			void DynamicTable::setMaxSize(uint32_t maxSize) noexcept {
				_maxSize = maxSize;
				evict();
			}

			void DynamicTable::evict() noexcept {
				while((_size > _maxSize) && !_entries.empty()){
					const field_t & back = _entries.back();
					_size -= static_cast <uint32_t> (back.name.size() + back.value.size() + 32);
					_entries.pop_back();
				}
			}

			// ───────────────────── Декодер ─────────────────────

			namespace {
				/**
				 * Декодировать HPACK-строку (литерал или Huffman) начиная с позиции pos.
				 */
				status_t decodeString(const uint8_t * data, size_t size, size_t & pos, std::string & out, error_t & err) noexcept {
					if(pos >= size) return status_t::INCOMPLETE;
					const bool huffman = (data[pos] & 0x80) != 0;
					uint64_t len = 0;
					size_t used = 0;
					const status_t st = decodeInteger(data + pos, size - pos, 7, len, used);
					if(st != status_t::OK) { if(st == status_t::ERROR) err = error_t::COMPRESSION_ERROR; return st; }
					pos += used;
					if(pos + len > size) return status_t::INCOMPLETE;
					const uint8_t * str = data + pos;
					if(huffman){
						if(!huffmanDecode(str, static_cast <size_t> (len), out)){
							err = error_t::COMPRESSION_ERROR;
							return status_t::ERROR;
						}
					} else out.assign(reinterpret_cast <const char *> (str), static_cast <size_t> (len));
					pos += static_cast <size_t> (len);
					return status_t::OK;
				}
			}

			status_t Decoder::decode(std::string_view block, std::vector <field_t> & out, uint64_t maxListSize, error_t & err) noexcept {
				const uint8_t * data = reinterpret_cast <const uint8_t *> (block.data());
				const size_t size = block.size();
				size_t pos = 0;
				uint64_t listSize = 0;

				// Получить пару (name,value) по объединённому индексу (статическая + динамическая).
				auto resolve = [&](uint64_t index, std::string & name, std::string & value, bool needValue) noexcept -> bool {
					if(index == 0) return false;
					if(index <= STATIC_TABLE_SIZE){
						const static_entry_t * e = staticTable(static_cast <size_t> (index));
						if(e == nullptr) return false;
						name.assign(e->name);
						if(needValue) value.assign(e->value);
						return true;
					}
					const field_t * e = _table.at(static_cast <size_t> (index - STATIC_TABLE_SIZE));
					if(e == nullptr) return false;
					name = e->name;
					if(needValue) value = e->value;
					return true;
				};

				auto account = [&](const field_t & f) noexcept -> bool {
					listSize += f.name.size() + f.value.size() + 32;
					return (maxListSize == 0) || (listSize <= maxListSize);
				};

				while(pos < size){
					const uint8_t b = data[pos];
					if(b & 0x80){
						// 6.1 Indexed Header Field (префикс 7 бит).
						uint64_t index = 0; size_t used = 0;
						const status_t st = decodeInteger(data + pos, size - pos, 7, index, used);
						if(st != status_t::OK){ if(st == status_t::ERROR) err = error_t::COMPRESSION_ERROR; return st; }
						pos += used;
						field_t f;
						if(!resolve(index, f.name, f.value, true)){ err = error_t::COMPRESSION_ERROR; return status_t::ERROR; }
						if(!account(f)){ err = error_t::ENHANCE_YOUR_CALM; return status_t::ERROR; }
						out.push_back(std::move(f));
					} else if(b & 0x40){
						// 6.2.1 Literal с инкрементальной индексацией (префикс 6 бит).
						uint64_t index = 0; size_t used = 0;
						if(decodeInteger(data + pos, size - pos, 6, index, used) != status_t::OK){ err = error_t::COMPRESSION_ERROR; return status_t::ERROR; }
						pos += used;
						field_t f;
						if(index != 0){
							if(!resolve(index, f.name, f.value, false)){ err = error_t::COMPRESSION_ERROR; return status_t::ERROR; }
						} else if(decodeString(data, size, pos, f.name, err) != status_t::OK) return status_t::ERROR;
						if(decodeString(data, size, pos, f.value, err) != status_t::OK) return status_t::ERROR;
						_table.add(f.name, f.value); // добавляем в динамическую таблицу
						if(!account(f)){ err = error_t::ENHANCE_YOUR_CALM; return status_t::ERROR; }
						out.push_back(std::move(f));
					} else if((b & 0x20) == 0){
						// 6.2.2 / 6.2.3 Literal без индексации / never indexed (префикс 4 бита).
						uint64_t index = 0; size_t used = 0;
						if(decodeInteger(data + pos, size - pos, 4, index, used) != status_t::OK){ err = error_t::COMPRESSION_ERROR; return status_t::ERROR; }
						pos += used;
						field_t f;
						if(index != 0){
							if(!resolve(index, f.name, f.value, false)){ err = error_t::COMPRESSION_ERROR; return status_t::ERROR; }
						} else if(decodeString(data, size, pos, f.name, err) != status_t::OK) return status_t::ERROR;
						if(decodeString(data, size, pos, f.value, err) != status_t::OK) return status_t::ERROR;
						if(!account(f)){ err = error_t::ENHANCE_YOUR_CALM; return status_t::ERROR; }
						out.push_back(std::move(f));
					} else {
						// 6.3 Dynamic Table Size Update (префикс 5 бит).
						uint64_t newSize = 0; size_t used = 0;
						if(decodeInteger(data + pos, size - pos, 5, newSize, used) != status_t::OK){ err = error_t::COMPRESSION_ERROR; return status_t::ERROR; }
						pos += used;
						// TODO(этап 2): проверить newSize <= согласованный SETTINGS_HEADER_TABLE_SIZE.
						_table.setMaxSize(static_cast <uint32_t> (newSize));
					}
				}
				return status_t::OK;
			}

			// ───────────────────── Кодер ─────────────────────

			namespace {
				void encodeStringLiteral(std::string & out, std::string_view s, bool useHuffman) noexcept {
					if(useHuffman && (huffmanLength(s) < s.size())){
						std::string enc;
						huffmanEncode(s, enc);
						encodeInteger(out, enc.size(), 7, 0x80); // H = 1
						out.append(enc);
					} else {
						encodeInteger(out, s.size(), 7, 0x00); // H = 0
						out.append(s.data(), s.size());
					}
				}
			}

			void Encoder::encode(const std::vector <field_t> & fields, std::string & out, bool useHuffman) noexcept {
				// Поиск заголовка в статической + динамической таблицах.
				// Возвращает индекс полного совпадения (имя+значение) или, если его нет,
				// индекс совпадения только по имени. 0 — совпадения нет.
				auto lookup = [&](const field_t & f, uint64_t & nameIndex) noexcept -> uint64_t {
					nameIndex = 0;
					// Статическая таблица (индексы 1..61).
					for(size_t i = 1; i <= STATIC_TABLE_SIZE; ++i){
						if(STATIC[i].name != f.name) continue;
						if(nameIndex == 0) nameIndex = i;
						if(STATIC[i].value == f.value) return i;
					}
					// Динамическая таблица (индексы 62..): [0] — самая свежая.
					for(size_t j = 1; j <= _table.count(); ++j){
						const field_t * e = _table.at(j);
						if((e == nullptr) || (e->name != f.name)) continue;
						const uint64_t idx = STATIC_TABLE_SIZE + j;
						if(nameIndex == 0) nameIndex = idx;
						if(e->value == f.value) return idx;
					}
					return 0;
				};

				for(const field_t & f : fields){
					uint64_t nameIndex = 0;
					const uint64_t fullIndex = lookup(f, nameIndex);
					if(fullIndex != 0){
						// 6.1 Indexed Header Field — имя и значение уже в таблице.
						encodeInteger(out, fullIndex, 7, 0x80);
						continue;
					}
					// 6.2.1 Literal с инкрементальной индексацией (префикс 6 бит, старший бит 0x40).
					// nameIndex == 0 → имя кодируется строкой; иначе ссылаемся на существующее имя.
					encodeInteger(out, nameIndex, 6, 0x40);
					if(nameIndex == 0) encodeStringLiteral(out, f.name, useHuffman);
					encodeStringLiteral(out, f.value, useHuffman);
					// Добавляем в свою динамическую таблицу — декодер пира сделает то же,
					// поэтому индексы остаются синхронными.
					_table.add(f.name, f.value);
				}
			}
		}
	}
}
