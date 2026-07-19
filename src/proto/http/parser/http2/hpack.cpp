/**
 * @file: hpack.cpp
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

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http2/hpack.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Внутренние вспомогательные таблицы и функции (внутренняя компоновка)
 */
namespace {
	/**
	 * Используем пространство имён внутренних слоёв протокола HTTP/2
	 */
	using namespace awh::http::h2;

	/**
	 * @brief Статическая таблица HPACK (RFC 7541, Appendix A)
	 *
	 * @note Индекс 0 не используется - заглушка для 1-based индексации
	 */
	const hpack::static_entry_t STATIC[hpack::STATIC_TABLE_SIZE + 1] = {
		{ "", "" }, // 0 - заглушка (индексация 1-based)
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

	/**
	 * @brief Структура записи таблицы Huffman-кодов (RFC 7541 Appendix B)
	 *
	 * @details Поле code хранит код, выровненный по старшему биту 32-битного слова.
	 *          Численные значения таблицы скопированы из nghttp2_hd_huffman_data.c
	 *          (nghttp2, лицензия MIT); логика кодера/декодера написана заново.
	 *          EOS (символ 256) опущен: при кодировании хвост добивается единичными
	 *          битами (это префикс EOS), при декодировании EOS внутри потока - ошибка.
	 */
	struct huff_sym_t {
		// Длина кода в битах (5..30)
		uint8_t nbits;
		// Код, выровненный по старшему биту
		uint32_t code;
	};

	/**
	 * @brief Таблица Huffman-кодов для всех 256 значений байта (RFC 7541 Appendix B)
	 *
	 */
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

	/**
	 * @brief Структура узла дерева декодирования Huffman
	 *
	 * @details У листа sym >= 0, у внутреннего узла sym == -1
	 */
	struct huff_node_t {
		// Декодированный символ (или -1 для внутреннего узла)
		int16_t sym;
		// Индексы дочерних узлов для бит 0 и 1 (-1 если отсутствует)
		int32_t child[2];
	};

	/**
	 * @brief Функция построения (один раз) дерева декодирования из таблицы кодов
	 *
	 * @return дерево декодирования Huffman
	 */
	const vector <huff_node_t> & huffTree() noexcept {
		// Строим дерево декодирования лениво при первом обращении
		static const vector <huff_node_t> tree = [](){
			// Результат работы функции
			vector <huff_node_t> t;
			// Создаём корень дерева
			t.push_back(huff_node_t{ -1, { -1, -1 } });
			/**
			 * Выполняем перебор всех символов таблицы кодов
			 */
			for(int32_t sym = 0; sym < 256; ++sym){
				// Получаем код текущего символа
				const uint32_t code = HUFF[sym].code;
				// Получаем длину кода текущего символа в битах
				const uint8_t nbits = HUFF[sym].nbits;
				// Начинаем спуск от корня дерева
				int32_t cur = 0;
				/**
				 * Выполняем перебор всех бит кода
				 */
				for(uint8_t i = 0; i < nbits; ++i){
					// Извлекаем очередной бит кода (от старшего к младшему)
					const int32_t bit = ((code >> (31 - i)) & 1u);
					// Если дочерний узел для этого бита ещё не создан
					if(t[cur].child[bit] < 0){
						// Создаём новый внутренний узел
						t.push_back(huff_node_t{ -1, { -1, -1 } });
						// Привязываем созданный узел к текущему
						t[cur].child[bit] = static_cast <int32_t> (t.size() - 1);
					}
					// Спускаемся в дочерний узел
					cur = t[cur].child[bit];
				}
				// Помечаем достигнутый узел как лист с декодированным символом
				t[cur].sym = static_cast <int16_t> (sym);
			}
			// Выводим построенное дерево
			return t;
		}();
		// Выводим дерево декодирования
		return tree;
	}

	/**
	 * @brief Функция декодирования HPACK-строки (литерал или Huffman) начиная с позиции pos
	 *
	 * @param data входной буфер
	 * @param size доступно байт
	 * @param pos  текущая позиция разбора (сдвигается)
	 * @param out  выходной буфер декодированной строки
	 * @param err  код ошибки протокола
	 * @return     результат декодирования (OK/INCOMPLETE/ERROR)
	 */
	status_t decodeString(const uint8_t * data, const size_t size, size_t & pos, string & out, error_t & err) noexcept {
		// Если данных для разбора не осталось
		if(pos >= size)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Извлекаем признак Huffman-кодирования строки (старший бит)
		const bool huffman = ((data[pos] & 0x80) != 0);
		// Длина строки
		uint64_t len = 0;
		// Количество прочитанных байт
		size_t used = 0;
		// Выполняем декодирование длины строки (префикс 7 бит)
		const status_t st = hpack::decodeInteger(data + pos, size - pos, 7, len, used);
		// Если декодирование длины не удалось
		if(st != status_t::OK){
			// Если зафиксирована ошибка декодирования
			if(st == status_t::ERROR)
				// Фиксируем ошибку состояния HPACK
				err = error_t::COMPRESSION_ERROR;
			// Выводим статус декодирования
			return st;
		}
		// Сдвигаем позицию за длину строки
		pos += used;
		/**
		 * Без сложения pos + len: оно переполняет size_t при враждебно большом len
		 * (длина строки приходит из недоверенных данных) и обходит проверку границ
		 */
		if(len > static_cast <uint64_t> (size - pos))
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Указатель на данные строки
		const uint8_t * str = (data + pos);
		// Если строка закодирована Huffman'ом
		if(huffman){
			// Если декодирование Huffman-строки не удалось
			if(!hpack::huffmanDecode(str, static_cast <size_t> (len), out)){
				// Фиксируем ошибку состояния HPACK
				err = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
		// Строка передана литералом - копируем как есть
		} else out.assign(reinterpret_cast <const char *> (str), static_cast <size_t> (len));
		// Сдвигаем позицию за данные строки
		pos += static_cast <size_t> (len);
		// Строка декодирована
		return status_t::OK;
	}

	/**
	 * @brief Функция проверки названия заголовка на чувствительность (RFC 7541 §7.1.3)
	 *
	 * @details Такие заголовки кодер всегда трактует как чувствительные
	 *
	 * @param n название заголовка
	 * @return  результат проверки
	 */
	bool isSensitiveName(string_view n) noexcept {
		// Чувствительными считаются заголовки авторизации и cookie
		return (
			(n == "authorization") || (n == "proxy-authorization") ||
			(n == "cookie") || (n == "set-cookie")
		);
	}
	/**
	 * @brief Функция кодирования HPACK-строки (литерал или Huffman)
	 *
	 * @param out        выходной буфер
	 * @param s          кодируемая строка
	 * @param useHuffman применять Huffman-кодирование, если оно короче литерала
	 */
	void encodeStringLiteral(string & out, string_view s, const bool useHuffman) noexcept {
		// Если Huffman-кодирование разрешено и даёт выигрыш по размеру
		if(useHuffman && (hpack::huffmanLength(s) < s.size())){
			// Буфер закодированной строки
			string enc;
			// Выполняем Huffman-кодирование строки
			hpack::huffmanEncode(s, enc);
			// Дописываем длину строки с флагом Huffman (H = 1)
			hpack::encodeInteger(out, enc.size(), 7, 0x80);
			// Дописываем закодированную строку
			out.append(enc);
		// Кодируем строку литералом
		} else {
			// Дописываем длину строки без флага Huffman (H = 0)
			hpack::encodeInteger(out, s.size(), 7, 0x00);
			// Дописываем строку как есть
			out.append(s.data(), s.size());
		}
	}
}

/**
 * @brief Функция получения записи статической таблицы по индексу 1..61 (RFC 7541 Appendix A)
 *
 * @param index индекс записи (1-based); 0 или > 61 - невалиден
 * @return      указатель на запись либо nullptr
 */
const awh::http::h2::hpack::static_entry_t * awh::http::h2::hpack::staticTable(const size_t index) noexcept {
	// Если индекс за пределами статической таблицы
	if((index == 0) || (index > STATIC_TABLE_SIZE))
		// Запись не найдена
		return nullptr;
	// Выводим запись статической таблицы
	return &::STATIC[index];
}
/**
 * @brief Функция декодирования целого с префиксом переменной длины (RFC 7541 §5.1)
 *
 * @param data       входной буфер
 * @param size       доступно байт
 * @param prefixBits размер префикса в битах (1..8)
 * @param value      декодированное значение
 * @param consumed   количество прочитанных байт
 * @return           результат декодирования (OK / INCOMPLETE - мало данных / ERROR - переполнение)
 */
awh::http::h2::status_t awh::http::h2::hpack::decodeInteger(const uint8_t * data, const size_t size, const uint8_t prefixBits, uint64_t & value, size_t & consumed) noexcept {
	// Если данных для разбора нет
	if(size < 1)
		// Данных недостаточно
		return status_t::INCOMPLETE;
	// Максимальное значение, помещающееся в префикс
	const uint8_t prefixMax = static_cast <uint8_t> ((1u << prefixBits) - 1);
	// Извлекаем значение префикса
	uint64_t result = (data[0] & prefixMax);
	// Текущая позиция разбора
	size_t pos = 1;
	// Если значение поместилось в префикс целиком
	if(result < prefixMax){
		// Устанавливаем декодированное значение
		value = result;
		// Устанавливаем количество прочитанных байт
		consumed = pos;
		// Значение декодировано
		return status_t::OK;
	}
	// Текущий сдвиг продолжения (7 бит на байт, старший бит - признак продолжения)
	uint32_t shift = 0;
	/**
	 * Выполняем чтение байтов продолжения
	 */
	while(true){
		// Если данные закончились посреди продолжения
		if(pos >= size)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Извлекаем очередной байт продолжения
		const uint8_t b = data[pos++];
		// Если сдвиг превысил разрядность (защита от переполнения uint64_t, RFC 7541 §5.1)
		if(shift >= 64)
			// Фиксируем переполнение
			return status_t::ERROR;
		// Вычисляем добавку из 7 бит очередного байта
		const uint64_t add = (static_cast <uint64_t> (b & 0x7F) << shift);
		// Если добавка переполняет результат
		if(add > (UINT64_MAX - result))
			// Фиксируем переполнение
			return status_t::ERROR;
		// Накапливаем результат
		result += add;
		// Если признак продолжения сброшен - значение прочитано
		if((b & 0x80) == 0)
			// Выходим из цикла чтения
			break;
		// Сдвигаемся на следующие 7 бит
		shift += 7;
	}
	// Устанавливаем декодированное значение
	value = result;
	// Устанавливаем количество прочитанных байт
	consumed = pos;
	// Значение декодировано
	return status_t::OK;
}
/**
 * @brief Функция кодирования целого с префиксом переменной длины (RFC 7541 §5.1)
 *
 * @param out         выходной буфер
 * @param value       кодируемое значение
 * @param prefixBits  размер префикса в битах (1..8)
 * @param prefixValue значение старших бит первого байта
 */
void awh::http::h2::hpack::encodeInteger(string & out, uint64_t value, const uint8_t prefixBits, const uint8_t prefixValue) noexcept {
	// Максимальное значение, помещающееся в префикс
	const uint8_t prefixMax = static_cast <uint8_t> ((1u << prefixBits) - 1);
	// Старшие биты первого байта (за пределами префикса)
	const uint8_t high = (prefixValue & static_cast <uint8_t> (~prefixMax));
	// Если значение помещается в префикс целиком
	if(value < prefixMax){
		// Дописываем единственный байт со значением в префиксе
		out.push_back(static_cast <char> (high | static_cast <uint8_t> (value)));
		// Выходим из функции
		return;
	}
	// Дописываем первый байт с заполненным префиксом
	out.push_back(static_cast <char> (high | prefixMax));
	// Вычитаем часть значения, ушедшую в префикс
	value -= prefixMax;
	/**
	 * Выполняем запись байтов продолжения (7 бит на байт)
	 */
	while(value >= 0x80){
		// Дописываем очередные 7 бит с признаком продолжения
		out.push_back(static_cast <char> ((value & 0x7F) | 0x80));
		// Сдвигаем значение на записанные биты
		value >>= 7;
	}
	// Дописываем последний байт без признака продолжения
	out.push_back(static_cast <char> (value));
}
/**
 * @brief Функция декодирования Huffman-строки (RFC 7541 Appendix B)
 *
 * @param data входной буфер
 * @param size доступно байт
 * @param out  выходной буфер декодированной строки
 * @return     результат декодирования (false - некорректная последовательность, COMPRESSION_ERROR)
 */
bool awh::http::h2::hpack::huffmanDecode(const uint8_t * data, const size_t size, string & out) noexcept {
	// Получаем дерево декодирования Huffman
	const vector <::huff_node_t> & tree = ::huffTree();
	// Текущий узел дерева
	int32_t cur = 0;
	// Длина текущего незавершённого пути в битах
	int32_t padLen = 0;
	// Признак того, что незавершённый путь состоит из единичных бит
	bool padOnes = true;
	/**
	 * Выполняем перебор всех байтов входного буфера
	 */
	for(size_t i = 0; i < size; ++i){
		// Извлекаем очередной байт
		const uint8_t byte = data[i];
		/**
		 * Выполняем перебор всех бит байта (от старшего к младшему)
		 */
		for(int32_t b = 7; b >= 0; --b){
			// Извлекаем очередной бит
			const int32_t bit = ((byte >> b) & 1);
			// Спускаемся в дочерний узел по значению бита
			cur = tree[cur].child[bit];
			// Если дочерний узел отсутствует - недопустимая кодовая последовательность
			if(cur < 0)
				// Фиксируем ошибку декодирования
				return false;
			// Наращиваем длину незавершённого пути
			++padLen;
			// Обновляем признак единичных бит пути
			padOnes = (padOnes && (bit == 1));
			// Если достигнут лист - символ декодирован
			if(tree[cur].sym >= 0){
				// Дописываем декодированный символ
				out.push_back(static_cast <char> (tree[cur].sym));
				// Возвращаемся к корню дерева
				cur = 0;
				// Сбрасываем длину незавершённого пути
				padLen = 0;
				// Сбрасываем признак единичных бит пути
				padOnes = true;
			}
		}
	}
	/**
	 * Корректный конец: либо точно на границе символа, либо хвост из <= 7
	 * единичных бит (префикс EOS). Иначе - COMPRESSION_ERROR
	 */
	if(cur != 0){
		// Если хвост длиннее 7 бит или содержит нулевые биты
		if((padLen > 7) || !padOnes)
			// Фиксируем ошибку декодирования
			return false;
	}
	// Строка декодирована
	return true;
}
/**
 * @brief Функция кодирования строки Huffman'ом (RFC 7541 Appendix B)
 *
 * @param in  кодируемая строка
 * @param out выходной буфер закодированной строки
 */
void awh::http::h2::hpack::huffmanEncode(string_view in, string & out) noexcept {
	// Битовый аккумулятор
	uint64_t buf = 0;
	// Число накопленных бит
	int32_t cnt = 0;
	/**
	 * Выполняем перебор всех символов строки
	 */
	for(uint8_t c : in){
		// Получаем длину кода текущего символа в битах
		const uint8_t nbits = ::HUFF[c].nbits;
		// Получаем код текущего символа с правым выравниванием
		const uint32_t code = (::HUFF[c].code >> (32 - nbits));
		// Накапливаем код в битовом аккумуляторе
		buf = ((buf << nbits) | code);
		// Наращиваем число накопленных бит
		cnt += nbits;
		/**
		 * Выполняем выгрузку целых байтов из аккумулятора
		 */
		while(cnt >= 8){
			// Уменьшаем число накопленных бит на байт
			cnt -= 8;
			// Дописываем очередной байт закодированной строки
			out.push_back(static_cast <char> ((buf >> cnt) & 0xFF));
		}
	}
	// Если в аккумуляторе остались биты
	if(cnt > 0){
		// Число недостающих до байта бит
		const int32_t rem = (8 - cnt);
		// Добиваем хвост единичными битами (префикс EOS)
		buf = ((buf << rem) | ((1u << rem) - 1));
		// Дописываем последний байт закодированной строки
		out.push_back(static_cast <char> (buf & 0xFF));
	}
}
/**
 * @brief Функция вычисления длины строки в байтах после Huffman-кодирования
 *
 * @param in строка для вычисления
 * @return   длина строки после кодирования
 */
size_t awh::http::h2::hpack::huffmanLength(string_view in) noexcept {
	// Суммарная длина кодов в битах
	size_t bits = 0;
	/**
	 * Выполняем перебор всех символов строки
	 */
	for(uint8_t c : in)
		// Наращиваем суммарную длину кодов
		bits += ::HUFF[c].nbits;
	// Выводим длину строки в байтах (с округлением вверх)
	return ((bits + 7) / 8);
}
/**
 * @brief Метод вытеснения записей с конца, пока размер не уложится в лимит
 *
 */
void awh::http::h2::hpack::DynamicTable::evict() noexcept {
	/**
	 * Выполняем вытеснение записей, пока размер таблицы превышает лимит
	 */
	while((this->_size > this->_maxSize) && !this->_entries.empty()){
		// Получаем самую старую запись таблицы
		const field_t & back = this->_entries.back();
		// Уменьшаем суммарный размер таблицы на размер записи (RFC 7541 §4.1)
		this->_size -= static_cast <uint32_t> (back.name.size() + back.value.size() + 32);
		// Удаляем самую старую запись таблицы
		this->_entries.pop_back();
	}
}
/**
 * @brief Метод добавления записи в начало таблицы (с вытеснением старых при нехватке места)
 *
 * @param name  название заголовка
 * @param value значение заголовка
 */
void awh::http::h2::hpack::DynamicTable::add(string_view name, string_view value) noexcept {
	// Вычисляем размер добавляемой записи (RFC 7541 §4.1)
	const uint32_t entrySize = static_cast <uint32_t> (name.size() + value.size() + 32);
	// Если запись больше всей таблицы - таблица очищается, запись не добавляется (RFC 7541 §4.4)
	if(entrySize > this->_maxSize){
		// Очищаем все записи таблицы
		this->_entries.clear();
		// Сбрасываем суммарный размер таблицы
		this->_size = 0;
		// Выходим из метода
		return;
	}
	// Добавляем запись в начало таблицы
	this->_entries.emplace_front(string(name), string(value));
	// Наращиваем суммарный размер таблицы
	this->_size += entrySize;
	// Вытесняем старые записи при нехватке места
	this->evict();
}
/**
 * @brief Метод доступа к записи по индексу (1-based внутри динамической части)
 *
 * @param index индекс записи
 * @return      указатель на запись либо nullptr
 */
const awh::http::h2::hpack::field_t * awh::http::h2::hpack::DynamicTable::at(const size_t index) const noexcept {
	// Если индекс за пределами таблицы
	if((index < 1) || (index > this->_entries.size()))
		// Запись не найдена
		return nullptr;
	// Выводим запись таблицы
	return &this->_entries[index - 1];
}
/**
 * @brief Метод изменения максимального размера таблицы (Dynamic Table Size Update)
 *
 * @param maxSize новый максимальный размер таблицы
 */
void awh::http::h2::hpack::DynamicTable::setMaxSize(const uint32_t maxSize) noexcept {
	// Устанавливаем новый лимит размера таблицы
	this->_maxSize = maxSize;
	// Вытесняем лишние записи
	this->evict();
}
/**
 * @brief Метод получения количества записей таблицы
 *
 * @return количество записей таблицы
 */
size_t awh::http::h2::hpack::DynamicTable::count() const noexcept {
	// Выводим количество записей таблицы
	return this->_entries.size();
}
/**
 * @brief Метод получения текущего суммарного размера таблицы
 *
 * @return текущий суммарный размер таблицы
 */
uint32_t awh::http::h2::hpack::DynamicTable::size() const noexcept {
	// Выводим текущий суммарный размер таблицы
	return this->_size;
}
/**
 * @brief Метод получения лимита размера таблицы
 *
 * @return лимит размера таблицы
 */
uint32_t awh::http::h2::hpack::DynamicTable::maxSize() const noexcept {
	// Выводим лимит размера таблицы
	return this->_maxSize;
}
/**
 * @brief Конструктор
 *
 * @param maxSize максимальный размер таблицы
 */
awh::http::h2::hpack::DynamicTable::DynamicTable(const uint32_t maxSize) noexcept :
 _size(0), _maxSize(maxSize) {}
/**
 * @brief Метод декодирования одного блока заголовков целиком
 *
 * @param block       блок заголовков (уже собранный из HEADERS + CONTINUATION)
 * @param out         декодированные заголовки
 * @param maxListSize лимит суммарного размера списка (защита от decompression bomb); 0 - без лимита
 * @param err         код ошибки протокола (COMPRESSION_ERROR / ENHANCE_YOUR_CALM)
 * @return            результат декодирования (OK/ERROR)
 */
awh::http::h2::status_t awh::http::h2::hpack::Decoder::decode(string_view block, vector <field_t> & out, const uint64_t maxListSize, error_t & err) noexcept {
	// Указатель на данные блока заголовков
	const uint8_t * data = reinterpret_cast <const uint8_t *> (block.data());
	// Размер блока заголовков
	const size_t size = block.size();
	// Текущая позиция разбора
	size_t pos = 0;
	// Суммарный размер распакованного списка заголовков
	uint64_t listSize = 0;
	/**
	 * @brief Функция получения пары (name, value) по объединённому индексу (статическая + динамическая таблицы)
	 *
	 * @param index     объединённый индекс записи
	 * @param name      название заголовка
	 * @param value     значение заголовка
	 * @param needValue требуется ли извлекать значение
	 * @return          результат получения записи
	 */
	auto resolve = [this](const uint64_t index, string & name, string & value, const bool needValue) noexcept -> bool {
		// Если индекс невалиден
		if(index == 0)
			// Запись не найдена
			return false;
		// Если индекс принадлежит статической таблице
		if(index <= STATIC_TABLE_SIZE){
			// Получаем запись статической таблицы
			const static_entry_t * e = staticTable(static_cast <size_t> (index));
			// Если запись не найдена
			if(e == nullptr)
				// Запись не найдена
				return false;
			// Извлекаем название заголовка
			name.assign(e->name);
			// Если требуется значение заголовка
			if(needValue)
				// Извлекаем значение заголовка
				value.assign(e->value);
			// Запись получена
			return true;
		}
		// Получаем запись динамической таблицы
		const field_t * e = this->_table.at(static_cast <size_t> (index - STATIC_TABLE_SIZE));
		// Если запись не найдена
		if(e == nullptr)
			// Запись не найдена
			return false;
		// Извлекаем название заголовка
		name = e->name;
		// Если требуется значение заголовка
		if(needValue)
			// Извлекаем значение заголовка
			value = e->value;
		// Запись получена
		return true;
	};
	/**
	 * @brief Функция учёта размера декодированного заголовка (защита от decompression bomb)
	 *
	 * @param f декодированный заголовок
	 * @return  результат учёта (false - лимит превышен)
	 */
	auto account = [&listSize, maxListSize](const field_t & f) noexcept -> bool {
		// Наращиваем суммарный размер списка заголовков (RFC 7541 §4.1)
		listSize += (f.name.size() + f.value.size() + 32);
		// Проверяем что лимит размера списка не превышен
		return ((maxListSize == 0) || (listSize <= maxListSize));
	};
	/**
	 * Выполняем разбор всего блока заголовков
	 */
	while(pos < size){
		// Извлекаем первый байт представления
		const uint8_t b = data[pos];
		// Если это Indexed Header Field (RFC 7541 §6.1, префикс 7 бит)
		if(b & 0x80){
			// Объединённый индекс записи
			uint64_t index = 0;
			// Количество прочитанных байт
			size_t used = 0;
			// Выполняем декодирование индекса
			const status_t st = decodeInteger(data + pos, size - pos, 7, index, used);
			// Если декодирование индекса не удалось
			if(st != status_t::OK){
				// Если зафиксирована ошибка декодирования
				if(st == status_t::ERROR)
					// Фиксируем ошибку состояния HPACK
					err = error_t::COMPRESSION_ERROR;
				// Выводим статус декодирования
				return st;
			}
			// Сдвигаем позицию за индекс
			pos += used;
			// Создаём объект заголовка
			field_t f;
			// Если получение записи по индексу не удалось
			if(!resolve(index, f.name, f.value, true)){
				// Фиксируем ошибку состояния HPACK
				err = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Если лимит размера списка превышен
			if(!account(f)){
				// Фиксируем превышение лимита (decompression bomb)
				err = error_t::ENHANCE_YOUR_CALM;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Дописываем декодированный заголовок
			out.push_back(::move(f));
		// Если это Literal с инкрементальной индексацией (RFC 7541 §6.2.1, префикс 6 бит)
		} else if(b & 0x40) {
			// Объединённый индекс записи
			uint64_t index = 0;
			// Количество прочитанных байт
			size_t used = 0;
			// Если декодирование индекса не удалось
			if(decodeInteger(data + pos, size - pos, 6, index, used) != status_t::OK){
				// Фиксируем ошибку состояния HPACK
				err = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Сдвигаем позицию за индекс
			pos += used;
			// Создаём объект заголовка
			field_t f;
			// Если название заголовка передано индексом
			if(index != 0){
				// Если получение записи по индексу не удалось
				if(!resolve(index, f.name, f.value, false)){
					// Фиксируем ошибку состояния HPACK
					err = error_t::COMPRESSION_ERROR;
					// Выводим ошибку декодирования
					return status_t::ERROR;
				}
			// Если декодирование названия заголовка не удалось
			} else if(::decodeString(data, size, pos, f.name, err) != status_t::OK)
				// Выводим ошибку декодирования
				return status_t::ERROR;
			// Если декодирование значения заголовка не удалось
			if(::decodeString(data, size, pos, f.value, err) != status_t::OK)
				// Выводим ошибку декодирования
				return status_t::ERROR;
			// Добавляем заголовок в динамическую таблицу
			this->_table.add(f.name, f.value);
			// Если лимит размера списка превышен
			if(!account(f)){
				// Фиксируем превышение лимита (decompression bomb)
				err = error_t::ENHANCE_YOUR_CALM;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Дописываем декодированный заголовок
			out.push_back(::move(f));
		// Если это Literal без индексации / never indexed (RFC 7541 §6.2.2/§6.2.3, префикс 4 бита)
		} else if((b & 0x20) == 0) {
			// Объединённый индекс записи
			uint64_t index = 0;
			// Количество прочитанных байт
			size_t used = 0;
			// Если декодирование индекса не удалось
			if(decodeInteger(data + pos, size - pos, 4, index, used) != status_t::OK){
				// Фиксируем ошибку состояния HPACK
				err = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Сдвигаем позицию за индекс
			pos += used;
			// Создаём объект заголовка
			field_t f;
			// Если название заголовка передано индексом
			if(index != 0){
				// Если получение записи по индексу не удалось
				if(!resolve(index, f.name, f.value, false)){
					// Фиксируем ошибку состояния HPACK
					err = error_t::COMPRESSION_ERROR;
					// Выводим ошибку декодирования
					return status_t::ERROR;
				}
			// Если декодирование названия заголовка не удалось
			} else if(::decodeString(data, size, pos, f.name, err) != status_t::OK)
				// Выводим ошибку декодирования
				return status_t::ERROR;
			// Если декодирование значения заголовка не удалось
			if(::decodeString(data, size, pos, f.value, err) != status_t::OK)
				// Выводим ошибку декодирования
				return status_t::ERROR;
			// Если лимит размера списка превышен
			if(!account(f)){
				// Фиксируем превышение лимита (decompression bomb)
				err = error_t::ENHANCE_YOUR_CALM;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Дописываем декодированный заголовок
			out.push_back(::move(f));
		// Если это Dynamic Table Size Update (RFC 7541 §6.3, префикс 5 бит)
		} else {
			// Новый размер динамической таблицы
			uint64_t newSize = 0;
			// Количество прочитанных байт
			size_t used = 0;
			// Если декодирование нового размера не удалось
			if(decodeInteger(data + pos, size - pos, 5, newSize, used) != status_t::OK){
				// Фиксируем ошибку состояния HPACK
				err = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Сдвигаем позицию за размер
			pos += used;
			// Если новый размер превышает наш SETTINGS_HEADER_TABLE_SIZE (RFC 7541 §6.3)
			if(newSize > this->_protocolMaxSize){
				// Фиксируем ошибку состояния HPACK
				err = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Применяем новый размер динамической таблицы
			this->_table.setMaxSize(static_cast <uint32_t> (newSize));
		}
	}
	// Блок заголовков декодирован
	return status_t::OK;
}
/**
 * @brief Метод получения динамической таблицы пира
 *
 * @return динамическая таблица пира
 */
awh::http::h2::hpack::dynamic_table_t & awh::http::h2::hpack::Decoder::table() noexcept {
	// Выводим динамическую таблицу пира
	return this->_table;
}
/**
 * @brief Метод установки верхней границы для Dynamic Table Size Update (RFC 7541 §6.3)
 *
 * @param size верхняя граница размера таблицы
 */
void awh::http::h2::hpack::Decoder::setProtocolMaxSize(const uint32_t size) noexcept {
	// Устанавливаем верхнюю границу размера таблицы
	this->_protocolMaxSize = size;
}
/**
 * @brief Конструктор
 *
 * @param maxTableSize максимальный размер динамической таблицы
 */
awh::http::h2::hpack::Decoder::Decoder(const uint32_t maxTableSize) noexcept :
 _table(maxTableSize), _protocolMaxSize(maxTableSize) {}
/**
 * @brief Метод поиска заголовка в статической + динамической таблицах
 *
 * @param name      название искомого заголовка
 * @param value     значение искомого заголовка
 * @param nameIndex индекс совпадения только по имени
 * @return          индекс полного совпадения (имя+значение)
 */
uint64_t awh::http::h2::hpack::Encoder::lookup(string_view name, string_view value, uint64_t & nameIndex) const noexcept {
	// Сбрасываем индекс совпадения по имени
	nameIndex = 0;
	/**
	 * Выполняем поиск в статической таблице (индексы 1..61)
	 */
	for(size_t i = 1; i <= STATIC_TABLE_SIZE; ++i){
		// Если название заголовка не совпадает
		if(::STATIC[i].name != name)
			// Переходим к следующей записи
			continue;
		// Если совпадение по имени ещё не найдено
		if(nameIndex == 0)
			// Запоминаем индекс совпадения по имени
			nameIndex = i;
		// Если значение заголовка тоже совпадает
		if(::STATIC[i].value == value)
			// Выводим индекс полного совпадения
			return i;
	}
	/**
	 * Выполняем поиск в динамической таблице (индексы 62..): [0] - самая свежая запись
	 */
	for(size_t j = 1; j <= this->_table.count(); ++j){
		// Получаем очередную запись динамической таблицы
		const field_t * e = this->_table.at(j);
		// Если запись отсутствует или название заголовка не совпадает
		if((e == nullptr) || (e->name != name))
			// Переходим к следующей записи
			continue;
		// Вычисляем объединённый индекс записи
		const uint64_t idx = (STATIC_TABLE_SIZE + j);
		// Если совпадение по имени ещё не найдено
		if(nameIndex == 0)
			// Запоминаем индекс совпадения по имени
			nameIndex = idx;
		// Если значение заголовка тоже совпадает
		if(e->value == value)
			// Выводим индекс полного совпадения
			return idx;
	}
	// Полное совпадение не найдено
	return 0;
}
/**
 * @brief Метод начала кодирования блока заголовков
 *
 * @param out выходной буфер блока заголовков
 */
void awh::http::h2::hpack::Encoder::begin(string & out) noexcept {
	// Если требуется отправить Dynamic Table Size Update (RFC 7541 §4.2: обязан идти в самом начале блока)
	if(this->_sizeUpdatePending){
		// Дописываем Dynamic Table Size Update (паттерн 001xxxxx)
		encodeInteger(out, this->_pendingSize, 5, 0x20);
		// Сбрасываем признак ожидающего update
		this->_sizeUpdatePending = false;
	}
}
/**
 * @brief Метод кодирования одного заголовка (zero-copy, без владения строками)
 *
 * @param name       название заголовка
 * @param value      значение заголовка
 * @param out        выходной буфер блока заголовков
 * @param sensitive  чувствительное значение (Literal Never Indexed, RFC 7541 §7.1.3)
 * @param useHuffman применять Huffman-кодирование к строкам
 */
void awh::http::h2::hpack::Encoder::encode(string_view name, string_view value, string & out, const bool sensitive, const bool useHuffman) noexcept {
	// Индекс совпадения только по имени
	uint64_t nameIndex = 0;
	// Если значение заголовка чувствительное (явно или по названию заголовка)
	if(sensitive || ::isSensitiveName(name)){
		/**
		 * Literal Never Indexed (RFC 7541 §6.2.3, префикс 4 бита, паттерн 0001xxxx).
		 * Значение не индексируется и не попадает в динамическую таблицу;
		 * для имени допускается ссылка на индекс (только имя)
		 */
		this->lookup(name, value, nameIndex);
		// Дописываем индекс имени (или 0)
		encodeInteger(out, nameIndex, 4, 0x10);
		// Если совпадение по имени не найдено
		if(nameIndex == 0)
			// Дописываем название заголовка строкой
			::encodeStringLiteral(out, name, useHuffman);
		// Дописываем значение заголовка строкой
		::encodeStringLiteral(out, value, useHuffman);
		// В таблицу заголовок НЕ добавляем
		return;
	}
	// Выполняем поиск заголовка в таблицах
	const uint64_t fullIndex = this->lookup(name, value, nameIndex);
	// Если найдено полное совпадение (имя и значение уже в таблице)
	if(fullIndex != 0){
		// Дописываем Indexed Header Field (RFC 7541 §6.1)
		encodeInteger(out, fullIndex, 7, 0x80);
		// Кодирование заголовка завершено
		return;
	}
	/**
	 * Literal с инкрементальной индексацией (RFC 7541 §6.2.1, префикс 6 бит, старший бит 0x40).
	 * nameIndex == 0 - имя кодируется строкой; иначе ссылаемся на существующее имя
	 */
	encodeInteger(out, nameIndex, 6, 0x40);
	// Если совпадение по имени не найдено
	if(nameIndex == 0)
		// Дописываем название заголовка строкой
		::encodeStringLiteral(out, name, useHuffman);
	// Дописываем значение заголовка строкой
	::encodeStringLiteral(out, value, useHuffman);
	/**
	 * Добавляем в свою динамическую таблицу - декодер пира сделает то же,
	 * поэтому индексы остаются синхронными
	 */
	this->_table.add(name, value);
}
/**
 * @brief Метод кодирования списка заголовков
 *
 * @param fields     заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
 * @param out        выходной буфер блока заголовков
 * @param useHuffman применять Huffman-кодирование к строкам
 */
void awh::http::h2::hpack::Encoder::encode(const vector <field_t> & fields, string & out, const bool useHuffman) noexcept {
	// Дописываем отложенный Dynamic Table Size Update (если требуется)
	this->begin(out);
	/**
	 * Выполняем перебор всех кодируемых заголовков
	 */
	for(const field_t & f : fields)
		// Кодируем очередной заголовок
		this->encode(f.name, f.value, out, f.sensitive, useHuffman);
}
/**
 * @brief Метод получения собственной динамической таблицы
 *
 * @return собственная динамическая таблица
 */
awh::http::h2::hpack::dynamic_table_t & awh::http::h2::hpack::Encoder::table() noexcept {
	// Выводим собственную динамическую таблицу
	return this->_table;
}
/**
 * @brief Метод изменения максимального размера своей динамической таблицы (RFC 7541 §4.2)
 *
 * @param size новый максимальный размер таблицы
 */
void awh::http::h2::hpack::Encoder::setMaxTableSize(const uint32_t size) noexcept {
	// Если размер таблицы не изменился - ничего не делаем
	if(size == this->_table.maxSize())
		// Выходим из метода
		return;
	// Применяем новый размер сразу (с вытеснением)
	this->_table.setMaxSize(size);
	// Отмечаем что в начале следующего блока нужен Dynamic Table Size Update
	this->_sizeUpdatePending = true;
	// Запоминаем значение размера для отправляемого update
	this->_pendingSize = size;
}
/**
 * @brief Конструктор
 *
 * @param maxTableSize максимальный размер динамической таблицы
 */
awh::http::h2::hpack::Encoder::Encoder(const uint32_t maxTableSize) noexcept :
 _table(maxTableSize), _sizeUpdatePending(false), _pendingSize(0) {}
