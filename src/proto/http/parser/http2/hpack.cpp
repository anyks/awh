/**
 * @file: hpack.cpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация кодека HPACK (RFC 7541) — статическая и динамическая таблицы заголовков,
 *        кодирование и декодирование целых с префиксом,
 *        Хаффман-кодирование и разбор блоков заголовков с защитой от decompression bomb
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <utility>
#include <unordered_map>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http2/hpack.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные таблицы и функции (внутренняя компоновка)
 *
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
	 *
	 */
	const hpack::static_entry_t STATIC[hpack::STATIC_TABLE_SIZE + 1] = {
		{ "", "" }, // 0 - заглушка (индексация 1-based)
		{ ":authority", "" },                    //  1
		{ ":method", "GET" },                    //  2
		{ ":method", "POST" },                   //  3
		{ ":path", "/" },                        //  4
		{ ":path", "/index.html" },              //  5
		{ ":scheme", "http" },                   //  6
		{ ":scheme", "https" },                  //  7
		{ ":status", "200" },                    //  8
		{ ":status", "204" },                    //  9
		{ ":status", "206" },                   // 10
		{ ":status", "304" },                   // 11
		{ ":status", "400" },                   // 12
		{ ":status", "404" },                   // 13
		{ ":status", "500" },                   // 14
		{ "accept-charset", "" },               // 15
		{ "accept-encoding", "gzip, deflate" }, // 16
		{ "accept-language", "" },              // 17
		{ "accept-ranges", "" },                // 18
		{ "accept", "" },                       // 19
		{ "access-control-allow-origin", "" },  // 20
		{ "age", "" },                          // 21
		{ "allow", "" },                        // 22
		{ "authorization", "" },                // 23
		{ "cache-control", "" },                // 24
		{ "content-disposition", "" },          // 25
		{ "content-encoding", "" },             // 26
		{ "content-language", "" },             // 27
		{ "content-length", "" },               // 28
		{ "content-location", "" },             // 29
		{ "content-range", "" },                // 30
		{ "content-type", "" },                 // 31
		{ "cookie", "" },                       // 32
		{ "date", "" },                         // 33
		{ "etag", "" },                         // 34
		{ "expect", "" },                       // 35
		{ "expires", "" },                      // 36
		{ "from", "" },                         // 37
		{ "host", "" },                         // 38
		{ "if-match", "" },                     // 39
		{ "if-modified-since", "" },            // 40
		{ "if-none-match", "" },                // 41
		{ "if-range", "" },                     // 42
		{ "if-unmodified-since", "" },          // 43
		{ "last-modified", "" },                // 44
		{ "link", "" },                         // 45
		{ "location", "" },                     // 46
		{ "max-forwards", "" },                 // 47
		{ "proxy-authenticate", "" },           // 48
		{ "proxy-authorization", "" },          // 49
		{ "range", "" },                        // 50
		{ "referer", "" },                      // 51
		{ "refresh", "" },                      // 52
		{ "retry-after", "" },                  // 53
		{ "server", "" },                       // 54
		{ "set-cookie", "" },                   // 55
		{ "strict-transport-security", "" },    // 56
		{ "transfer-encoding", "" },            // 57
		{ "user-agent", "" },                   // 58
		{ "vary", "" },                         // 59
		{ "via", "" },                          // 60
		{ "www-authenticate", "" }              // 61
	};

	/**
	 * @brief Функция вычисления хеша пары название-значение заголовка
	 *
	 * @details Хеш названия передаётся готовым: вызывающая сторона считает его
	 *          и для поиска по названию, и для поиска полного совпадения,
	 *          а считать его дважды на каждый заголовок незачем
	 *
	 * @param name  хеш названия заголовка
	 * @param value значение заголовка
	 * @return      хеш пары название-значение
	 *
	 */
	size_t pairHash(const size_t name, string_view value) noexcept {
		// Выводим хеш пары название-значение
		return ((name * 31) ^ std::hash <string_view> {}(value));
	}
	/**
	 * @brief Функция получения (один раз) индекса статической таблицы по названию заголовка
	 *
	 * @details Записи с одинаковым названием в статической таблице идут подряд
	 *          (:method 2-3, :path 4-5, :scheme 6-7, :status 8-14), поэтому индекс
	 *          хранит только первую из них: поиск полного совпадения продолжается
	 *          линейно от неё, пока совпадает название.
	 *
	 * @return индекс статической таблицы по названию заголовка
	 *
	 */
	/**
	 * @brief Структура записи индекса статической таблицы
	 *
	 */
	struct static_range_t {
		// Индекс первой записи с этим названием
		uint32_t first;
		// Количество записей подряд с этим названием
		uint32_t count;
	};
	/**
	 * @brief Функция получения (один раз) индекса статической таблицы
	 *
	 * @details Ключом служит хеш названия, а не само название: тот же хеш нужен
	 *          и для поиска в динамической таблице, и считать его дважды на каждый
	 *          заголовок незачем. Совпадение хешей проверяется сравнением названия
	 *          с первой записью диапазона.
	 *          Хранится не только первый индекс, но и длина диапазона: записи
	 *          с одинаковым названием идут подряд (:method 2-3, :path 4-5,
	 *          :scheme 6-7, :status 8-14), и поиск полного совпадения продолжается
	 *          по значениям, не сверяя название заново на каждом шаге
	 *
	 * @return индекс статической таблицы по хешу названия заголовка
	 *
	 */
	const unordered_map <size_t, static_range_t> & staticNames() noexcept {
		// Строим индекс лениво при первом обращении
		static const unordered_map <size_t, static_range_t> index = [](){
			// Результат работы функции
			unordered_map <size_t, static_range_t> result;
			// Резервируем память под все записи статической таблицы
			result.reserve(hpack::STATIC_TABLE_SIZE);
			/**
			 * Выполняем перебор всех записей статической таблицы
			 */
			for(size_t i = 1; i <= hpack::STATIC_TABLE_SIZE; ++i){
				// Вычисляем хеш названия очередной записи
				const size_t hash = std::hash <string_view> {}(STATIC[i].name);
				// Выполняем поиск диапазона записей с этим названием
				const auto j = result.find(hash);
				// Если диапазон с этим названием ещё не заведён
				if(j == result.end())
					// Заводим диапазон из одной записи
					result.emplace(hash, static_range_t{static_cast <uint32_t> (i), 1});
				// Если диапазон продолжается той же записью - удлиняем его
				else if(STATIC[j->second.first].name == STATIC[i].name)
					// Удлиняем диапазон записей с этим названием
					j->second.count++;
			}
			// Выводим построенный индекс
			return result;
		}();
		// Выводим индекс статической таблицы
		return index;
	}
	/**
	 * @brief Структура записи таблицы Huffman-кодов (RFC 7541 Appendix B)
	 *
	 * @details Поле code хранит код, выровненный по старшему биту 32-битного слова.
	 *          Численные значения таблицы скопированы из nghttp2_hd_huffman_data.c
	 *          (nghttp2, лицензия MIT); логика кодера/декодера написана заново.
	 *          EOS (символ 256) опущен: при кодировании хвост добивается единичными
	 *          битами (это префикс EOS), при декодировании EOS внутри потока - ошибка.
	 *
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
	 * @details У листа sym >= 0, у внутреннего узла sym == -1.
	 *
	 */
	struct huff_node_t {
		// Декодированный символ (или -1 для внутреннего узла)
		int16_t sym;
		// Индексы дочерних узлов для бит 0 и 1 (-1 если отсутствует)
		int32_t child[2];
	};

	/**
	 * @brief Структура шага табличного декодирования по полубайту
	 *
	 * @details Минимальная длина кода Huffman - 5 бит, поэтому за один шаг в 4 бита
	 *          может быть завершён не более одного символа.
	 *
	 */
	struct huff_step_t {
		// Индекс узла дерева после шага
		int16_t next;
		// Декодированный символ (или -1, если символ не завершён)
		int16_t sym;
		// Признак недопустимой кодовой последовательности
		bool fail;
	};
	/**
	 * @brief Структура табличного декодера Huffman
	 *
	 */
	struct huff_table_t {
		// Таблица переходов: 16 шагов (по одному на значение полубайта) для каждого узла
		vector <huff_step_t> steps;
		// Длина пути от корня до узла в битах (для проверки хвостового заполнения)
		vector <uint8_t> depth;
		// Признак того, что путь от корня до узла состоит только из единичных бит
		vector <bool> ones;
	};
	/**
	 * @brief Функция построения (один раз) дерева декодирования из таблицы кодов
	 *
	 * @return дерево декодирования Huffman
	 *
	 */
	const vector <huff_node_t> & huffTree() noexcept {
		// Строим дерево декодирования лениво при первом обращении
		static const vector <huff_node_t> tree = [](){
			// Результат работы функции
			vector <huff_node_t> nodes;
			// Создаём корень дерева
			nodes.push_back(huff_node_t{ -1, { -1, -1 } });
			/**
			 * Выполняем перебор всех символов таблицы кодов
			 */
			for(int32_t sym = 0; sym < 256; ++sym){
				// Получаем код текущего символа
				const uint32_t code = HUFF[sym].code;
				// Получаем длину кода текущего символа в битах
				const uint8_t nbits = HUFF[sym].nbits;
				// Начинаем спуск от корня дерева
				int32_t curent = 0;
				/**
				 * Выполняем перебор всех бит кода
				 */
				for(uint8_t i = 0; i < nbits; ++i){
					// Извлекаем очередной бит кода (от старшего к младшему)
					const int32_t bit = ((code >> (31 - i)) & 1u);
					// Если дочерний узел для этого бита ещё не создан
					if(nodes[curent].child[bit] < 0){
						// Создаём новый внутренний узел
						nodes.push_back(huff_node_t{ -1, { -1, -1 } });
						// Привязываем созданный узел к текущему
						nodes[curent].child[bit] = static_cast <int32_t> (nodes.size() - 1);
					}
					// Спускаемся в дочерний узел
					curent = nodes[curent].child[bit];
				}
				// Помечаем достигнутый узел как лист с декодированным символом
				nodes[curent].sym = static_cast <int16_t> (sym);
			}
			// Выводим построенное дерево
			return nodes;
		}();
		// Выводим дерево декодирования
		return tree;
	}
	/**
	 * @brief Функция построения (один раз) табличного декодера Huffman
	 *
	 * @details Обрабатывает вход полубайтами вместо побитового спуска по дереву:
	 *          два обращения к таблице на байт вместо восьми разыменований узлов.
	 *
	 * @return табличный декодер Huffman
	 *
	 */
	const huff_table_t & huffTable() noexcept {
		// Строим таблицу декодирования лениво при первом обращении
		static const huff_table_t table = [](){
			// Результат работы функции
			huff_table_t result;
			// Получаем дерево декодирования Huffman
			const vector <huff_node_t> & tree = huffTree();
			// Получаем количество узлов дерева
			const size_t count = tree.size();
			// Резервируем память под таблицу переходов
			result.steps.resize(count * 16);
			// Резервируем память под длины путей до узлов
			result.depth.assign(count, 0);
			// Резервируем память под признаки единичных путей
			result.ones.assign(count, true);
			/**
			 * Вычисляем длину пути и признак единичных бит для каждого узла обходом
			 * в ширину: у дерева каждый узел достижим единственным путём от корня
			 */
			vector <int32_t> queue = { 0 };
			// Текущая позиция обхода
			size_t position = 0;
			/**
			 * Выполняем обход всех узлов дерева
			 */
			while(position < queue.size()){
				// Получаем очередной узел дерева
				const int32_t node = queue[position++];
				/**
				 * Выполняем перебор обоих потомков узла
				 */
				for(int32_t bit = 0; bit < 2; ++bit){
					// Получаем индекс дочернего узла
					const int32_t child = tree[node].child[bit];
					// Если дочерний узел отсутствует - переходим к следующему
					if(child < 0)
						// Переходим к следующему потомку
						continue;
					// Наращиваем длину пути до дочернего узла
					result.depth[child] = static_cast <uint8_t> (result.depth[node] + 1);
					// Путь единичный, если единичен путь родителя и бит перехода равен единице
					result.ones[child] = (result.ones[node] && (bit == 1));
					// Добавляем дочерний узел в очередь обхода
					queue.push_back(child);
				}
			}
			/**
			 * Выполняем перебор всех узлов дерева
			 */
			for(size_t node = 0; node < count; ++node){
				/**
				 * Выполняем перебор всех значений полубайта
				 */
				for(size_t value = 0; value < 16; ++value){
					// Получаем ссылку на заполняемый шаг таблицы
					huff_step_t & step = result.steps[(node * 16) + value];
					// Символ за шаг ещё не декодирован
					step.sym = -1;
					// Последовательность пока корректна
					step.fail = false;
					// Начинаем шаг с текущего узла
					int32_t current = static_cast <int32_t> (node);
					/**
					 * Выполняем перебор всех бит полубайта (от старшего к младшему)
					 */
					for(int32_t i = 3; i >= 0; --i){
						// Извлекаем очередной бит полубайта
						const int32_t bit = static_cast <int32_t> ((value >> i) & 1u);
						// Спускаемся в дочерний узел по значению бита
						current = tree[current].child[bit];
						// Если дочерний узел отсутствует - последовательность недопустима
						if(current < 0){
							// Фиксируем недопустимую последовательность
							step.fail = true;
							// Прекращаем обработку шага
							break;
						}
						// Если достигнут лист - символ декодирован
						if(tree[current].sym >= 0){
							// Запоминаем декодированный символ
							step.sym = tree[current].sym;
							// Возвращаемся к корню дерева
							current = 0;
						}
					}
					// Запоминаем узел, достигнутый по завершению шага
					step.next = (step.fail ? 0 : static_cast <int16_t> (current));
				}
			}
			// Выводим построенную таблицу
			return result;
		}();
		// Выводим табличный декодер
		return table;
	}
	/**
	 * @brief Функция декодирования HPACK-строки (литерал или Huffman) начиная с позиции pos
	 *
	 * @param data   входной буфер
	 * @param size   доступно байт
	 * @param pos    текущая позиция разбора (сдвигается)
	 * @param output выходной буфер декодированной строки
	 * @param error  код ошибки протокола
	 * @return       результат декодирования (OK/INCOMPLETE/ERROR)
	 *
	 */
	status_t decodeStringRaw(const uint8_t * data, const size_t size, size_t & pos, string & output, error_t & error) noexcept {
		// Если данных для разбора не осталось
		if(pos >= size)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Извлекаем признак Huffman-кодирования строки (старший бит)
		const bool huffman = ((data[pos] & 0x80) != 0);
		// Количество прочитанных байт
		size_t used = 0;
		// Длина строки
		uint64_t length = 0;
		// Выполняем декодирование длины строки (префикс 7 бит)
		const status_t status = hpack::prefixed::decode(data + pos, size - pos, 7, length, used);
		// Если декодирование длины не удалось
		if(status != status_t::OK){
			// Если зафиксирована ошибка декодирования
			if(status == status_t::ERROR)
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
			// Выводим статус декодирования
			return status;
		}
		// Сдвигаем позицию за длину строки
		pos += used;
		/**
		 * Без сложения pos + length: оно переполняет size_t при враждебно большом length
		 * (длина строки приходит из недоверенных данных) и обходит проверку границ
		 */
		if(length > static_cast <uint64_t> (size - pos))
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Указатель на данные строки
		const uint8_t * str = (data + pos);
		// Если строка закодирована Huffman'ом
		if(huffman){
			// Если декодирование Huffman-строки не удалось
			if(!hpack::huffman::decode(str, static_cast <size_t> (length), output)){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
		// Строка передана литералом - копируем как есть
		} else output.assign(reinterpret_cast <const char *> (str), static_cast <size_t> (length));
		// Сдвигаем позицию за данные строки
		pos += static_cast <size_t> (length);
		// Строка декодирована
		return status_t::OK;
	}
	/**
	 * @brief Функция декодирования HPACK-строки (литерал или Huffman) с дописыванием в арену
	 *
	 * @param data    входной буфер
	 * @param size    доступно байт
	 * @param pos     текущая позиция разбора (сдвигается)
	 * @param arena   арена декодированных строк (строка дописывается в конец)
	 * @param scratch переиспользуемый буфер Huffman-декодирования
	 * @param offset  смещение декодированной строки в арене
	 * @param length  длина декодированной строки
	 * @param error   код ошибки протокола
	 * @return        результат декодирования (OK/INCOMPLETE/ERROR)
	 *
	 */
	status_t decodeString(const uint8_t * data, const size_t size, size_t & pos, string & arena, string & scratch, size_t & offset, size_t & length, error_t & error) noexcept {
		// Запоминаем смещение начала строки в арене
		offset = arena.size();
		// Сбрасываем длину декодированной строки
		length = 0;
		// Выполняем декодирование строки во временный буфер
		const status_t status = decodeStringRaw(data, size, pos, scratch, error);
		// Если декодирование строки не удалось
		if(status != status_t::OK)
			// Выводим статус декодирования
			return status;
		// Дописываем декодированную строку в арену
		arena.append(scratch);
		// Устанавливаем длину декодированной строки
		length = scratch.size();
		// Строка декодирована
		return status_t::OK;
	}
	/**
	 * @brief Функция проверки названия заголовка на чувствительность (RFC 7541 §7.1.3)
	 *
	 * @details Такие заголовки кодер всегда трактует как чувствительные.
	 *
	 * @param name название заголовка
	 * @return     результат проверки
	 *
	 */
	bool isSensitiveName(string_view name) noexcept {
		// Чувствительными считаются заголовки авторизации и cookie
		return (
			(name == "cookie") || (name == "set-cookie") ||
			(name == "authorization") || (name == "proxy-authorization")
		);
	}
	/**
	 * @brief Функция кодирования HPACK-строки (литерал или Huffman)
	 *
	 * @param output     выходной буфер
	 * @param str          кодируемая строка
	 * @param useHuffman применять Huffman-кодирование, если оно короче литерала
	 *
	 */
	/**
	 * @brief Функция кодирования строки Huffman'ом с известной длиной результата
	 *
	 * @details Функция внутренняя намеренно: длина результата принимается на веру,
	 *          и ошибка в ней означала бы запись за границу буфера. Наружу отдаётся
	 *          только двухаргументный вариант, вычисляющий длину сам
	 *
	 * @param input  кодируемая строка
	 * @param output выходной буфер закодированной строки
	 * @param length длина строки после кодирования, полученная из huffman::length()
	 *
	 */
	void huffmanEncode(string_view input, string & output, const size_t length) noexcept {
		// Число накопленных бит
		int32_t count = 0;
		// Битовый аккумулятор
		uint64_t bytes = 0;
		// Запоминаем позицию, с которой дописывается закодированная строка
		const size_t offset = output.size();
		/**
		 * Расширяем выходной буфер под точную длину закодированной строки. Запись
		 * ведётся указателем, а не методом push_back: тот в стандартной библиотеке
		 * не встраивается, и на каждый выданный байт приходился бы вызов через
		 * границу динамической библиотеки
		 */
		output.resize(offset + length);
		// Указатель на текущую позицию записи
		char * cursor = (&output[0] + offset);
		/**
		 * Выполняем перебор всех символов строки
		 */
		for(uint8_t letter : input){
			// Получаем длину кода текущего символа в битах
			const uint8_t nbits = HUFF[letter].nbits;
			// Получаем код текущего символа с правым выравниванием
			const uint32_t code = (HUFF[letter].code >> (32 - nbits));
			// Накапливаем код в битовом аккумуляторе
			bytes = ((bytes << nbits) | code);
			// Наращиваем число накопленных бит
			count += nbits;
			/**
			 * Выполняем выгрузку целых байтов из аккумулятора
			 */
			while(count >= 8){
				// Уменьшаем число накопленных бит на байт
				count -= 8;
				// Дописываем очередной байт закодированной строки
				(* cursor++) = static_cast <char> ((bytes >> count) & 0xFF);
			}
		}
		// Если в аккумуляторе остались биты
		if(count > 0){
			// Число недостающих до байта бит
			const int32_t rem = (8 - count);
			// Добиваем хвост единичными битами (префикс EOS)
			bytes = ((bytes << rem) | ((1u << rem) - 1));
			// Дописываем последний байт закодированной строки
			(* cursor++) = static_cast <char> (bytes & 0xFF);
		}
		// Усекаем буфер до фактически записанной длины
		output.resize(static_cast <size_t> (cursor - &output[0]));
	}
	void encodeStringLiteral(string & output, string_view str, const bool useHuffman) noexcept {
		/**
		 * Вычисляем длину строки после Huffman-кодирования, только если оно разрешено:
		 * это полный проход по строке, и при выключенном сжатии он был бы напрасным
		 */
		const size_t length = (useHuffman ? hpack::huffman::length(str) : 0);
		// Если Huffman-кодирование разрешено и даёт выигрыш по размеру
		if(useHuffman && (length < str.size())){
			/**
			 * Дописываем длину строки с флагом Huffman (H = 1). Длина известна заранее,
			 * поэтому промежуточный буфер не нужен: строка кодируется прямо в выходной,
			 * а прежняя редакция заводила под неё временную строку - то есть выделение
			 * памяти на каждый кодируемый литерал
			 */
			hpack::prefixed::encode(output, length, 7, 0x80);
			// Дописываем закодированную строку прямо в выходной буфер
			::huffmanEncode(str, output, length);
		// Кодируем строку литералом
		} else {
			// Дописываем длину строки без флага Huffman (H = 0)
			hpack::prefixed::encode(output, str.size(), 7, 0x00);
			// Дописываем строку как есть
			output.append(str.data(), str.size());
		}
	}
};

/**
 * @brief Функция получения записи статической таблицы по индексу 1..61 (RFC 7541 Appendix A)
 *
 * @param index индекс записи (1-based); 0 или > 61 - невалиден
 * @return      указатель на запись либо nullptr
 *
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
 * @brief Конструктор
 *
 */
awh::http::h2::hpack::Field_View::Field_View() noexcept :
 name{""}, value{""}, sensitive(false) {}

/**
 * @brief Конструктор
 *
 */
awh::http::h2::hpack::Field::Field() noexcept :
 name{""}, value{""}, sensitive(false) {}
/**
 * @brief Конструктор
 *
 * @param name  название заголовка
 * @param value значение заголовка
 *
 */
awh::http::h2::hpack::Field::Field(string name, string value) noexcept :
 name(::move(name)), value(::move(value)), sensitive(false) {}
/**
 * @brief Конструктор
 *
 * @param name      название заголовка
 * @param value     значение заголовка
 * @param sensitive флаг чувствительного значения
 *
 */
awh::http::h2::hpack::Field::Field(string name, string value, const bool sensitive) noexcept :
 name(::move(name)), value(::move(value)), sensitive(sensitive) {}

/**
 * @brief Функция вычисления длины строки в байтах после Huffman-кодирования
 *
 * @param input строка для вычисления
 * @return      длина строки после кодирования
 *
 */
size_t awh::http::h2::hpack::huffman::length(string_view input) noexcept {
	// Суммарная длина кодов в битах
	size_t bits = 0;
	/**
	 * Выполняем перебор всех символов строки
	 */
	for(uint8_t letter : input)
		// Наращиваем суммарную длину кодов
		bits += ::HUFF[letter].nbits;
	// Выводим длину строки в байтах (с округлением вверх)
	return ((bits + 7) / 8);
}
/**
 * @brief Функция кодирования строки Huffman'ом (RFC 7541 Appendix B)
 *
 * @param input  кодируемая строка
 * @param output выходной буфер закодированной строки
 *
 */
void awh::http::h2::hpack::huffman::encode(string_view input, string & output) noexcept {
	// Выполняем кодирование с вычисленной длиной результата
	::huffmanEncode(input, output, length(input));
}
/**
 * @brief Функция декодирования Huffman-строки (RFC 7541 Appendix B)
 *
 * @param data   входной буфер
 * @param size   доступно байт
 * @param output выходной буфер декодированной строки
 * @return       результат декодирования (false - некорректная последовательность, COMPRESSION_ERROR)
 *
 */
bool awh::http::h2::hpack::huffman::decode(const uint8_t * data, const size_t size, string & output) noexcept {
	// Получаем табличный декодер Huffman
	const ::huff_table_t & table = ::huffTable();
	// Текущий узел дерева
	int32_t current = 0;
	/**
	 * Расширяем буфер под оценку сверху: короче пяти бит кодов в таблице
	 * RFC 7541 Appendix B нет, значит символов не больше, чем бит делённых на пять.
	 * Запись ведётся указателем, а не методом push_back: тот в стандартной
	 * библиотеке не встраивается, и на каждый декодированный символ приходился бы
	 * вызов через границу динамической библиотеки
	 */
	const size_t bound = (((size * 8) / 5) + 1);
	// Если разрядности буфера не хватает - расширяем его
	if(output.size() < bound)
		// Расширяем буфер до оценки сверху
		output.resize(bound);
	// Указатель на начало выходного буфера
	char * const begin = &output[0];
	// Указатель на текущую позицию записи
	char * cursor = begin;
	/**
	 * Выполняем перебор всех байтов входного буфера
	 */
	for(size_t i = 0; i < size; ++i){
		// Извлекаем очередной байт
		const uint8_t byte = data[i];
		// Получаем шаг декодирования старшего полубайта
		const ::huff_step_t & high = table.steps[(static_cast <size_t> (current) * 16) + (byte >> 4)];
		// Если получена недопустимая кодовая последовательность
		if(high.fail){
			// Очищаем выходной буфер: содержимое недостоверно
			output.clear();
			// Фиксируем ошибку декодирования
			return false;
		}
		// Если на шаге декодирован символ
		if(high.sym >= 0)
			// Дописываем декодированный символ
			(* cursor++) = static_cast <char> (high.sym);
		// Переходим в достигнутый узел дерева
		current = high.next;
		// Получаем шаг декодирования младшего полубайта
		const ::huff_step_t & low = table.steps[(static_cast <size_t> (current) * 16) + (byte & 0x0F)];
		// Если получена недопустимая кодовая последовательность
		if(low.fail){
			// Очищаем выходной буфер: содержимое недостоверно
			output.clear();
			// Фиксируем ошибку декодирования
			return false;
		}
		// Если на шаге декодирован символ
		if(low.sym >= 0)
			// Дописываем декодированный символ
			(* cursor++) = static_cast <char> (low.sym);
		// Переходим в достигнутый узел дерева
		current = low.next;
	}
	/**
	 * Корректный конец: либо точно на границе символа, либо хвост из <= 7
	 * единичных бит (префикс EOS). Иначе - COMPRESSION_ERROR
	 */
	if(current != 0){
		// Если хвост длиннее 7 бит или содержит нулевые биты
		if((table.depth[current] > 7) || !table.ones[current]){
			// Очищаем выходной буфер: содержимое недостоверно
			output.clear();
			// Фиксируем ошибку декодирования
			return false;
		}
	}
	// Усекаем буфер до фактически декодированной длины
	output.resize(static_cast <size_t> (cursor - begin));
	// Строка декодирована
	return true;
}

/**
 * @brief Функция кодирования целого с префиксом переменной длины (RFC 7541 §5.1)
 *
 * @param output      выходной буфер
 * @param value       кодируемое значение
 * @param prefixBits  размер префикса в битах (1..8)
 * @param prefixValue значение старших бит первого байта
 *
 */
void awh::http::h2::hpack::prefixed::encode(string & output, uint64_t value, const uint8_t prefixBits, const uint8_t prefixValue) noexcept {
	// Максимальное значение, помещающееся в префикс
	const uint8_t prefixMax = static_cast <uint8_t> ((1u << prefixBits) - 1);
	// Старшие биты первого байта (за пределами префикса)
	const uint8_t high = (prefixValue & static_cast <uint8_t> (~prefixMax));
	// Если значение помещается в префикс целиком
	if(value < prefixMax){
		// Дописываем единственный байт со значением в префиксе
		output.push_back(static_cast <char> (high | static_cast <uint8_t> (value)));
		// Выходим из функции
		return;
	}
	/**
	 * Собираем представление в буфере на стеке и дописываем одним вызовом.
	 * Побайтовая дозапись обошлась бы вызовом в стандартную библиотеку на каждый байт.
	 *
	 * Ёмкость буфера считается по предельному значению: первый байт с заполненным
	 * префиксом, затем байты продолжения по семь бит на 64-разрядное значение.
	 * После вычитания префикса остаётся до 64 значащих бит, что даёт девять байтов
	 * с признаком продолжения и один без него - одиннадцать октетов вместе с первым
	 */
	char buffer[11];
	// Позиция записи в буфере представления
	size_t offset = 0;
	// Дописываем первый байт с заполненным префиксом
	buffer[offset++] = static_cast <char> (high | prefixMax);
	// Вычитаем часть значения, ушедшую в префикс
	value -= prefixMax;
	/**
	 * Выполняем запись байтов продолжения (7 бит на байт)
	 */
	while(value >= 0x80){
		// Дописываем очередные 7 бит с признаком продолжения
		buffer[offset++] = static_cast <char> ((value & 0x7F) | 0x80);
		// Сдвигаем значение на записанные биты
		value >>= 7;
	}
	// Дописываем последний байт без признака продолжения
	buffer[offset++] = static_cast <char> (value);
	// Дописываем собранное представление в выходной буфер
	output.append(buffer, offset);
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
 *
 */
awh::http::h2::status_t awh::http::h2::hpack::prefixed::decode(const uint8_t * data, const size_t size, const uint8_t prefixBits, uint64_t & value, size_t & consumed) noexcept {
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
	for(;;){
		// Если данные закончились посреди продолжения
		if(pos >= size)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Извлекаем очередной байт продолжения
		const uint8_t byte = data[pos++];
		// Если сдвиг превысил разрядность (защита от переполнения uint64_t, RFC 7541 §5.1)
		if(shift >= 64)
			// Фиксируем переполнение
			return status_t::ERROR;
		// Извлекаем очередные 7 бит значения
		const uint64_t chunk = static_cast <uint64_t> (byte & 0x7F);
		/**
		 * Если сдвиг вытолкнет старшие биты за разрядность: без этой проверки
		 * сдвиг молча отбрасывал бы биты и давал заниженное значение вместо ошибки
		 */
		if(chunk > (UINT64_MAX >> shift))
			// Фиксируем переполнение
			return status_t::ERROR;
		// Вычисляем добавку из 7 бит очередного байта
		const uint64_t add = (chunk << shift);
		// Если добавка переполняет результат
		if(add > (UINT64_MAX - result))
			// Фиксируем переполнение
			return status_t::ERROR;
		// Накапливаем результат
		result += add;
		// Если признак продолжения сброшен - значение прочитано
		if((byte & 0x80) == 0)
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
		// Если индекс записей сопровождается
		if(this->_indexing){
			// Вычисляем сквозной номер вытесняемой записи (она самая старая из живых)
			const uint64_t seq = (this->_inserts - (this->_entries.size() - 1));
			// Вычисляем хеш названия вытесняемой записи
			const size_t hashName = std::hash <string_view> {}(string_view(back.name));
			// Получаем диапазон записей индекса с тем же хешем пары название-значение
			const auto range = this->_index.equal_range(::pairHash(hashName, string_view(back.value)));
			/**
			 * Выполняем перебор записей индекса с этим хешем пары
			 */
			for(auto i = range.first; i != range.second; ++i){
				// Если найдена вытесняемая запись
				if(i->second == seq){
					// Удаляем запись из индекса
					this->_index.erase(i);
					// Прекращаем перебор записей индекса
					break;
				}
			}
			// Получаем запись индекса названий с тем же хешем названия заголовка
			const auto i = this->_names.find(hashName);
			/**
			 * Удаляем запись индекса названий, только если вытесняется именно та запись,
			 * на которую он ссылается. Вытеснение идёт с самой старой записи, а индекс
			 * названий хранит самую свежую - значит запись удаляется, когда уходит
			 * последняя запись с этим названием
			 */
			if((i != this->_names.end()) && (i->second == seq))
				// Удаляем запись из индекса названий
				this->_names.erase(i);
		}
		// Удаляем самую старую запись таблицы
		this->_entries.pop_back();
	}
}
/**
 * @brief Метод получения количества записей таблицы
 *
 * @return количество записей таблицы
 *
 */
size_t awh::http::h2::hpack::DynamicTable::count() const noexcept {
	// Выводим количество записей таблицы
	return this->_entries.size();
}
/**
 * @brief Метод получения текущего суммарного размера таблицы
 *
 * @return текущий суммарный размер таблицы
 *
 */
uint32_t awh::http::h2::hpack::DynamicTable::size() const noexcept {
	// Выводим текущий суммарный размер таблицы
	return this->_size;
}
/**
 * @brief Метод получения лимита размера таблицы
 *
 * @return лимит размера таблицы
 *
 */
uint32_t awh::http::h2::hpack::DynamicTable::maxSize() const noexcept {
	// Выводим лимит размера таблицы
	return this->_maxSize;
}
/**
 * @brief Метод изменения максимального размера таблицы (Dynamic Table Size Update)
 *
 * @param maxSize новый максимальный размер таблицы
 *
 */
void awh::http::h2::hpack::DynamicTable::setMaxSize(const uint32_t maxSize) noexcept {
	// Устанавливаем новый лимит размера таблицы
	this->_maxSize = maxSize;
	// Вытесняем лишние записи
	this->evict();
}
/**
 * @brief Метод доступа к записи по индексу (1-based внутри динамической части)
 *
 * @param index индекс записи
 * @return      указатель на запись либо nullptr
 *
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
 * @brief Метод добавления записи в начало таблицы (с вытеснением старых при нехватке места)
 *
 * @param name  название заголовка
 * @param value значение заголовка
 *
 */
uint64_t awh::http::h2::hpack::DynamicTable::findName(const size_t hashName, string_view name) const noexcept {
	// Получаем запись индекса названий с искомым хешем названия заголовка
	const auto i = this->_names.find(hashName);
	// Если запись индекса названий не найдена
	if(i == this->_names.end())
		// Выводим отсутствие совпадения по названию заголовка
		return 0;
	// Вычисляем позицию записи по сквозному номеру
	const size_t position = static_cast <size_t> (this->_inserts - i->second);
	/**
	 * Индекс названий хранит только самую свежую запись с этим названием, поэтому
	 * перебирать нечего. Совпадение хешей названий сверяется строкой: при
	 * столкновении хешей ссылка на название просто не выдаётся, и название
	 * кодируется строкой - на корректности это не сказывается
	 */
	if((position >= this->_entries.size()) || (this->_entries[position].name != name))
		// Выводим отсутствие совпадения по названию заголовка
		return 0;
	// Выводим индекс совпадения по названию заголовка
	return ((this->_inserts - i->second) + 1);
}
/**
 * @brief Метод добавления записи в начало таблицы
 *
 * @param name  название заголовка
 * @param value значение заголовка
 *
 */
void awh::http::h2::hpack::DynamicTable::add(string_view name, string_view value) noexcept {
	// Вычисляем размер добавляемой записи (RFC 7541 §4.1)
	const uint32_t entrySize = static_cast <uint32_t> (name.size() + value.size() + 32);
	// Если запись больше всей таблицы - таблица очищается, запись не добавляется (RFC 7541 §4.4)
	if(entrySize > this->_maxSize){
		// Сбрасываем суммарный размер таблицы
		this->_size = 0;
		// Очищаем индекс записей
		this->_index.clear();
		// Очищаем индекс названий
		this->_names.clear();
		// Очищаем все записи таблицы
		this->_entries.clear();
		// Выходим из метода
		return;
	}
	// Добавляем запись в начало таблицы
	this->_entries.emplace_front(string(name), string(value));
	// Наращиваем сквозной номер добавления
	this->_inserts++;
	// Если индекс записей сопровождается
	if(this->_indexing){
		// Вычисляем хеш названия добавляемой записи
		const size_t hashName = std::hash <string_view> {}(name);
		// Дописываем запись в индекс по хешу пары название-значение
		this->_index.emplace(::pairHash(hashName, value), this->_inserts);
		/**
		 * Записываем добавленную запись в индекс названий: она самая свежая
		 * с этим названием, а именно свежая и нужна для ссылки только по названию
		 */
		this->_names[hashName] = this->_inserts;
	}
	// Наращиваем суммарный размер таблицы
	this->_size += entrySize;
	// Вытесняем старые записи при нехватке места
	this->evict();
}
/**
 * @brief Метод поиска записи по названию и значению заголовка
 *
 * @param name      название искомого заголовка
 * @param value     значение искомого заголовка
 * @param nameIndex индекс совпадения только по названию заголовка
 * @return          индекс полного совпадения либо 0
 *
 */
uint64_t awh::http::h2::hpack::DynamicTable::find(string_view name, string_view value, uint64_t & nameIndex) const noexcept {
	// Выполняем поиск с вычисленным хешем названия заголовка
	return this->find(std::hash <string_view> {}(name), name, value, nameIndex, true);
}
/**
 * @brief Метод поиска записи по названию и значению заголовка с готовым хешем названия
 *
 * @param hashName  хеш названия искомого заголовка
 * @param name      название искомого заголовка
 * @param value     значение искомого заголовка
 * @param nameIndex индекс совпадения только по названию заголовка
 * @return          индекс полного совпадения либо 0
 *
 */
uint64_t awh::http::h2::hpack::DynamicTable::find(const size_t hashName, string_view name, string_view value, uint64_t & nameIndex, const bool needName) const noexcept {
	// Результат работы функции - индекс полного совпадения
	uint64_t result = 0;
	// Получаем диапазон записей индекса с искомым хешем пары название-значение
	const auto range = this->_index.equal_range(::pairHash(hashName, value));
	/**
	 * Выполняем перебор записей с искомым хешем пары. Ключом служит хеш пары,
	 * а не одного названия: заголовков с одинаковым названием и разными значениями
	 * в таблице бывают десятки (:path, cookie, set-cookie), и по хешу названия
	 * они все попадали бы в одно ведро, вырождая поиск в перебор
	 */
	for(auto i = range.first; i != range.second; ++i){
		// Получаем сквозной номер записи
		const uint64_t seq = i->second;
		// Вычисляем позицию записи по сквозному номеру
		const size_t position = static_cast <size_t> (this->_inserts - seq);
		// Если позиция вышла за пределы таблицы - запись уже вытеснена
		if(position >= this->_entries.size())
			// Переходим к следующей записи индекса
			continue;
		// Получаем запись таблицы
		const field_t & entry = this->_entries[position];
		// Совпадение хешей ещё не означает совпадения строк - сверяем и название, и значение
		if((entry.name != name) || (entry.value != value))
			// Переходим к следующей записи индекса
			continue;
		// Запоминаем наиболее свежее полное совпадение
		if(seq > result)
			// Запоминаем сквозной номер полного совпадения
			result = seq;
	}
	/**
	 * Выполняем поиск по индексу названий, только если ссылка на название ещё нужна.
	 * Название заголовка чаще всего находится в статической таблице, и вызывающая
	 * сторона уже получила индекс оттуда - тогда этот поиск отработал бы впустую
	 */
	if(needName)
		// Выполняем поиск индекса совпадения по названию заголовка
		nameIndex = this->findName(hashName, name);
	// Выводим индекс полного совпадения
	return ((result > 0) ? ((this->_inserts - result) + 1) : 0);
}
/**
 * @brief Конструктор
 *
 * @param maxSize  максимальный размер таблицы
 * @param indexing сопровождать индекс записей для поиска по названию
 *
 */
awh::http::h2::hpack::DynamicTable::DynamicTable(const uint32_t maxSize, const bool indexing) noexcept :
 _size(0), _maxSize(maxSize), _inserts(0), _indexing(indexing) {}

/**
 * @brief Метод получения динамической таблицы пира
 *
 * @return динамическая таблица пира
 *
 */
awh::http::h2::hpack::dynamic_table_t & awh::http::h2::hpack::Decoder::table() noexcept {
	// Выводим динамическую таблицу пира
	return this->_table;
}
/**
 * @brief Метод установки верхней границы для Dynamic Table Size Update (RFC 7541 §6.3)
 *
 * @note Должна равняться объявленному нами SETTINGS_HEADER_TABLE_SIZE.
 *       Превышение пиром трактуется как COMPRESSION_ERROR.
 *
 * @param size верхняя граница размера таблицы
 *
 */
void awh::http::h2::hpack::Decoder::setProtocolMaxSize(const uint32_t size) noexcept {
	// Устанавливаем верхнюю границу размера таблицы
	this->_protocolMaxSize = size;
}
/**
 * @brief Метод декодирования одного блока заголовков целиком
 *
 * @param block       блок заголовков (уже собранный из HEADERS + CONTINUATION)
 * @param output      декодированные заголовки (ссылки в арену декодера)
 * @param maxListSize лимит суммарного размера списка (защита от decompression bomb); 0 - без лимита
 * @param error       код ошибки протокола (COMPRESSION_ERROR / ENHANCE_YOUR_CALM)
 * @return            результат декодирования (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::h2::hpack::Decoder::decode(string_view block, vector <field_view_t> & output, const uint64_t maxListSize, error_t & error) noexcept {
	// Текущая позиция разбора
	size_t pos = 0;
	// Суммарный размер распакованного списка заголовков
	uint64_t listSize = 0;
	// Размер блока заголовков
	const size_t size = block.size();
	// Указатель на данные блока заголовков
	const uint8_t * data = reinterpret_cast <const uint8_t *> (block.data());
	// Сбрасываем признак превышения лимита списка заголовков
	this->_overflow = false;
	// Очищаем арену декодированных строк (выделенная ёмкость переиспользуется)
	this->_arena.clear();
	// Очищаем срезы декодированных заголовков
	this->_slices.clear();
	// Очищаем список декодированных заголовков
	output.clear();
	/**
	 * @brief Функция дописывания строки в арену декодера
	 *
	 * @param str    дописываемая строка
	 * @param offset смещение строки в арене
	 * @param length длина строки
	 *
	 */
	auto store = [this](string_view str, size_t & offset, size_t & length) noexcept -> void {
		// Запоминаем смещение строки в арене
		offset = this->_arena.size();
		// Запоминаем длину строки
		length = str.size();
		// Дописываем строку в арену
		this->_arena.append(str);
	};
	/**
	 * @brief Функция получения записи по объединённому индексу (статическая + динамическая таблицы)
	 *
	 * @param index     объединённый индекс записи
	 * @param slice     срез заголовка, заполняемый из записи
	 * @param needValue требуется ли извлекать значение
	 * @return          результат получения записи
	 *
	 */
	auto resolve = [this, &store](const uint64_t index, slice_t & slice, const bool needValue) noexcept -> bool {
		// Если индекс невалиден
		if(index == 0)
			// Запись не найдена
			return false;
		// Если индекс принадлежит статической таблице
		if(index <= STATIC_TABLE_SIZE){
			// Получаем запись статической таблицы
			const static_entry_t * entry = staticTable(static_cast <size_t> (index));
			// Если запись не найдена
			if(entry == nullptr)
				// Запись не найдена
				return false;
			// Дописываем название заголовка в арену
			store(entry->name, slice.nameOffset, slice.nameLength);
			// Если требуется значение заголовка
			if(needValue)
				// Дописываем значение заголовка в арену
				store(entry->value, slice.valueOffset, slice.valueLength);
			// Запись получена
			return true;
		}
		// Получаем запись динамической таблицы
		const field_t * field = this->_table.at(static_cast <size_t> (index - STATIC_TABLE_SIZE));
		// Если запись не найдена
		if(field == nullptr)
			// Запись не найдена
			return false;
		/**
		 * Строки таблицы копируются в арену: запись может быть вытеснена уже на следующем
		 * заголовке этого же блока, а представление обязано жить до конца разбора
		 */
		store(field->name, slice.nameOffset, slice.nameLength);
		// Если требуется значение заголовка
		if(needValue)
			// Дописываем значение заголовка в арену
			store(field->value, slice.valueOffset, slice.valueLength);
		// Запись получена
		return true;
	};
	/**
	 * @brief Функция учёта размера декодированного заголовка (защита от decompression bomb)
	 *
	 * @param slice срез декодированного заголовка
	 * @return      результат учёта (false - лимит превышен)
	 *
	 */
	auto account = [&listSize, maxListSize](const slice_t & slice) noexcept -> bool {
		// Наращиваем суммарный размер списка заголовков (RFC 7541 §4.1)
		listSize += (slice.nameLength + slice.valueLength + 32);
		// Проверяем что лимит размера списка не превышен
		return ((maxListSize == 0) || (listSize <= maxListSize));
	};
	/**
	 * @brief Функция фиксации декодированного заголовка
	 *
	 * @details Блок сверх лимита списка разбирается до конца - иначе динамическая таблица
	 *          рассинхронизируется с кодером пира и соединение придётся рвать, - но его
	 *          заголовки не сохраняются: арена откатывается к состоянию до поля, поэтому
	 *          защита от decompression bomb остаётся в силе
	 *
	 * @param slice срез декодированного заголовка
	 * @param mark  размер арены до декодирования заголовка
	 *
	 */
	auto commit = [this](const slice_t & slice, const size_t mark) noexcept -> void {
		// Если лимит списка заголовков превышен
		if(this->_overflow)
			// Откатываем арену к состоянию до декодирования заголовка
			this->_arena.resize(mark);
		// Иначе дописываем срез декодированного заголовка
		else this->_slices.push_back(slice);
	};
	/**
	 * Признак того, что Dynamic Table Size Update ещё допустим: он обязан идти
	 * в самом начале блока заголовков, до первого поля (RFC 7541 §4.2)
	 */
	bool allowSizeUpdate = true;
	/**
	 * Выполняем разбор всего блока заголовков
	 */
	while(pos < size){
		// Извлекаем первый байт представления
		const uint8_t byte = data[pos];
		// Создаём срез декодированного заголовка
		slice_t slice{};
		// Запоминаем размер арены до декодирования заголовка
		const size_t mark = this->_arena.size();
		// Если это Indexed Header Field (RFC 7541 §6.1, префикс 7 бит)
		if(byte & 0x80){
			// Дальше Dynamic Table Size Update в этом блоке недопустим
			allowSizeUpdate = false;
			// Количество прочитанных байт
			size_t used = 0;
			// Объединённый индекс записи
			uint64_t index = 0;
			/**
			 * Если декодирование индекса не удалось: блок заголовков передаётся целиком,
			 * поэтому нехватка данных посреди него - такая же ошибка состояния HPACK,
			 * как и переполнение
			 */
			if(prefixed::decode(data + pos, size - pos, 7, index, used) != status_t::OK){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Сдвигаем позицию за индекс
			pos += used;
			// Если получение записи по индексу не удалось
			if(!resolve(index, slice, true)){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Если лимит размера списка превышен - заголовки блока наружу не отдаются
			if(!account(slice))
				// Помечаем что лимит списка заголовков превышен
				this->_overflow = true;
			// Фиксируем декодированный заголовок
			commit(slice, mark);
		// Если это Literal с инкрементальной индексацией (RFC 7541 §6.2.1, префикс 6 бит)
		} else if(byte & 0x40) {
			// Дальше Dynamic Table Size Update в этом блоке недопустим
			allowSizeUpdate = false;
			// Количество прочитанных байт
			size_t used = 0;
			// Объединённый индекс записи
			uint64_t index = 0;
			// Если декодирование индекса не удалось
			if(prefixed::decode(data + pos, size - pos, 6, index, used) != status_t::OK){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Сдвигаем позицию за индекс
			pos += used;
			// Если название заголовка передано индексом
			if(index != 0){
				// Если получение записи по индексу не удалось
				if(!resolve(index, slice, false)){
					// Фиксируем ошибку состояния HPACK
					error = error_t::COMPRESSION_ERROR;
					// Выводим ошибку декодирования
					return status_t::ERROR;
				}
			// Если декодирование названия заголовка не удалось
			} else if(::decodeString(data, size, pos, this->_arena, this->_scratch, slice.nameOffset, slice.nameLength, error) != status_t::OK){
				// Фиксируем ошибку состояния HPACK (нехватка данных посреди блока - тоже ошибка)
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Если декодирование значения заголовка не удалось
			if(::decodeString(data, size, pos, this->_arena, this->_scratch, slice.valueOffset, slice.valueLength, error) != status_t::OK){
				// Фиксируем ошибку состояния HPACK (нехватка данных посреди блока - тоже ошибка)
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Добавляем заголовок в динамическую таблицу (строки копируются в саму таблицу)
			this->_table.add(
				string_view(this->_arena.data() + slice.nameOffset, slice.nameLength),
				string_view(this->_arena.data() + slice.valueOffset, slice.valueLength)
			);
			// Если лимит размера списка превышен - заголовки блока наружу не отдаются
			if(!account(slice))
				// Помечаем что лимит списка заголовков превышен
				this->_overflow = true;
			// Фиксируем декодированный заголовок
			commit(slice, mark);
		// Если это Literal без индексации / never indexed (RFC 7541 §6.2.2/§6.2.3, префикс 4 бита)
		} else if((byte & 0x20) == 0) {
			// Дальше Dynamic Table Size Update в этом блоке недопустим
			allowSizeUpdate = false;
			/**
			 * Представление Literal Never Indexed (RFC 7541 §6.2.3, паттерн 0001xxxx):
			 * значение чувствительное и при перекодировании обязано остаться таким же,
			 * иначе оно попадёт в динамическую таблицу следующего узла (RFC 7541 §7.1.3)
			 */
			slice.sensitive = ((byte & 0xF0) == 0x10);
			// Количество прочитанных байт
			size_t used = 0;
			// Объединённый индекс записи
			uint64_t index = 0;
			// Если декодирование индекса не удалось
			if(prefixed::decode(data + pos, size - pos, 4, index, used) != status_t::OK){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Сдвигаем позицию за индекс
			pos += used;
			// Если название заголовка передано индексом
			if(index != 0){
				// Если получение записи по индексу не удалось
				if(!resolve(index, slice, false)){
					// Фиксируем ошибку состояния HPACK
					error = error_t::COMPRESSION_ERROR;
					// Выводим ошибку декодирования
					return status_t::ERROR;
				}
			// Если декодирование названия заголовка не удалось
			} else if(::decodeString(data, size, pos, this->_arena, this->_scratch, slice.nameOffset, slice.nameLength, error) != status_t::OK){
				// Фиксируем ошибку состояния HPACK (нехватка данных посреди блока - тоже ошибка)
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Если декодирование значения заголовка не удалось
			if(::decodeString(data, size, pos, this->_arena, this->_scratch, slice.valueOffset, slice.valueLength, error) != status_t::OK){
				// Фиксируем ошибку состояния HPACK (нехватка данных посреди блока - тоже ошибка)
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Если лимит размера списка превышен - заголовки блока наружу не отдаются
			if(!account(slice))
				// Помечаем что лимит списка заголовков превышен
				this->_overflow = true;
			// Фиксируем декодированный заголовок
			commit(slice, mark);
		// Если это Dynamic Table Size Update (RFC 7541 §6.3, префикс 5 бит)
		} else {
			/**
			 * Update обязан идти в самом начале блока заголовков (RFC 7541 §4.2):
			 * после первого поля таблица уже используется для индексации
			 */
			if(!allowSizeUpdate){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Количество прочитанных байт
			size_t used = 0;
			// Новый размер динамической таблицы
			uint64_t newSize = 0;
			// Если декодирование нового размера не удалось
			if(prefixed::decode(data + pos, size - pos, 5, newSize, used) != status_t::OK){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Сдвигаем позицию за размер
			pos += used;
			// Если новый размер превышает наш SETTINGS_HEADER_TABLE_SIZE (RFC 7541 §6.3)
			if(newSize > this->_protocolMaxSize){
				// Фиксируем ошибку состояния HPACK
				error = error_t::COMPRESSION_ERROR;
				// Выводим ошибку декодирования
				return status_t::ERROR;
			}
			// Применяем новый размер динамической таблицы
			this->_table.setMaxSize(static_cast <uint32_t> (newSize));
		}
	}
	/**
	 * Блок разобран целиком, динамическая таблица синхронна кодеру пира, но список
	 * заголовков превысил лимит: наружу не отдаём ничего. Вызывающий вправе отвергнуть
	 * один поток вместо разрыва всего соединения (RFC 9113 §10.5.1)
	 */
	if(this->_overflow){
		// Очищаем срезы декодированных заголовков
		this->_slices.clear();
		// Фиксируем превышение лимита (decompression bomb)
		error = error_t::ENHANCE_YOUR_CALM;
		// Блок заголовков декодирован
		return status_t::OK;
	}
	// Резервируем память под представления декодированных заголовков
	output.reserve(this->_slices.size());
	/**
	 * Собираем представления: арена больше не дописывается, её буфер уже
	 * не будет перевыделен, поэтому указатели остаются действительными
	 */
	for(const slice_t & slice : this->_slices){
		// Создаём объект декодированного заголовка
		field_view_t field;
		// Устанавливаем название заголовка
		field.name = string_view(this->_arena.data() + slice.nameOffset, slice.nameLength);
		// Устанавливаем значение заголовка
		field.value = string_view(this->_arena.data() + slice.valueOffset, slice.valueLength);
		// Устанавливаем признак чувствительного значения
		field.sensitive = slice.sensitive;
		// Дописываем декодированный заголовок
		output.push_back(field);
	}
	// Блок заголовков декодирован
	return status_t::OK;
}
/**
 * @brief Метод проверки превышения лимита списка заголовков последним блоком
 *
 * @return признак превышения лимита последним декодированным блоком
 *
 */
bool awh::http::h2::hpack::Decoder::overflowed() const noexcept {
	// Выводим признак превышения лимита последним декодированным блоком
	return this->_overflow;
}
/**
 * @brief Конструктор
 *
 * @param maxTableSize максимальный размер динамической таблицы
 *
 */
awh::http::h2::hpack::Decoder::Decoder(const uint32_t maxTableSize) noexcept :
 _table(maxTableSize), _protocolMaxSize(maxTableSize), _overflow(false) {}

/**
 * @brief Метод поиска заголовка в статической + динамической таблицах
 *
 * @details Возвращает индекс полного совпадения (имя+значение) или, если его нет,
 *          заполняет индекс совпадения только по имени. 0 - совпадения нет.
 *
 * @param name      название искомого заголовка
 * @param value     значение искомого заголовка
 * @param nameIndex индекс совпадения только по имени
 * @return          индекс полного совпадения (имя+значение)
 *
 */
uint64_t awh::http::h2::hpack::Encoder::lookupName(string_view name) const noexcept {
	// Вычисляем хеш названия заголовка
	const size_t hashName = std::hash <string_view> {}(name);
	// Получаем индекс статической таблицы по хешу названия заголовка
	const auto & names = ::staticNames();
	// Выполняем поиск названия заголовка в статической таблице (индексы 1..61)
	const auto i = names.find(hashName);
	// Если название заголовка найдено в статической таблице
	if((i != names.end()) && (::STATIC[i->second.first].name == name))
		// Выводим индекс совпадения по названию заголовка
		return i->second.first;
	// Выполняем поиск названия заголовка в динамической таблице (индексы 62..)
	const uint64_t dynamicName = this->_table.findName(hashName, name);
	// Выводим объединённый индекс совпадения по названию заголовка
	return ((dynamicName > 0) ? (STATIC_TABLE_SIZE + dynamicName) : 0);
}
/**
 * @brief Метод поиска заголовка в статической + динамической таблицах
 *
 * @param name      название искомого заголовка
 * @param value     значение искомого заголовка
 * @param nameIndex индекс совпадения только по имени
 * @return          индекс полного совпадения (имя+значение)
 *
 */
uint64_t awh::http::h2::hpack::Encoder::lookup(string_view name, string_view value, uint64_t & nameIndex) const noexcept {
	// Сбрасываем индекс совпадения по имени
	nameIndex = 0;
	/**
	 * Вычисляем хеш названия заголовка один раз на оба индекса: и статический,
	 * и динамический ключуются им же, а хеширование строки - заметная доля
	 * стоимости кодирования одного заголовка
	 */
	const size_t hashName = std::hash <string_view> {}(name);
	// Получаем индекс статической таблицы по хешу названия заголовка
	const auto & names = ::staticNames();
	// Выполняем поиск названия заголовка в статической таблице (индексы 1..61)
	const auto i = names.find(hashName);
	// Если название заголовка найдено в статической таблице
	if((i != names.end()) && (::STATIC[i->second.first].name == name)){
		// Запоминаем индекс совпадения по имени
		nameIndex = i->second.first;
		/**
		 * Выполняем перебор записей с этим названием: они идут подряд, их количество
		 * известно из индекса, и сверять название заново на каждом шаге не требуется
		 */
		for(uint32_t j = 0; j < i->second.count; ++j){
			// Если значение заголовка тоже совпадает
			if(::STATIC[i->second.first + j].value == value)
				// Выводим индекс полного совпадения
				return (i->second.first + j);
		}
	}
	// Индекс совпадения по названию заголовка в динамической таблице
	uint64_t dynamicName = 0;
	// Выполняем поиск в динамической таблице по индексу (индексы 62..)
	const uint64_t dynamicFull = this->_table.find(hashName, name, value, dynamicName, (nameIndex == 0));
	// Если совпадение по имени в статической таблице не найдено
	if((nameIndex == 0) && (dynamicName > 0))
		// Запоминаем объединённый индекс совпадения по имени
		nameIndex = (STATIC_TABLE_SIZE + dynamicName);
	// Если найдено полное совпадение в динамической таблице
	if(dynamicFull > 0)
		// Выводим объединённый индекс полного совпадения
		return (STATIC_TABLE_SIZE + dynamicFull);
	// Полное совпадение не найдено
	return 0;
}
/**
 * @brief Метод начала кодирования блока заголовков
 *
 * @details Дописывает отложенный Dynamic Table Size Update (RFC 7541 §4.2),
 *          который обязан идти в самом начале блока. Вызывается один раз
 *          перед пофиледным кодированием блока.
 *
 * @param output выходной буфер блока заголовков
 *
 */
void awh::http::h2::hpack::Encoder::begin(string & output) noexcept {
	// Сбрасываем размер списка заголовков блока (счёт начинается заново)
	this->_listSize = 0;
	// Если требуется отправить Dynamic Table Size Update (RFC 7541 §4.2: обязан идти в самом начале блока)
	if(this->_sizeUpdatePending){
		/**
		 * Если между блоками размер таблицы менялся несколько раз, кодер обязан
		 * сигнализировать наименьшее значение серии и только затем итоговое (RFC 7541 §4.2)
		 */
		if(this->_pendingMinSize < this->_pendingSize)
			// Дописываем наименьший размер серии изменений
			prefixed::encode(output, this->_pendingMinSize, 5, 0x20);
		// Дописываем Dynamic Table Size Update (паттерн 001xxxxx)
		prefixed::encode(output, this->_pendingSize, 5, 0x20);
		// Сбрасываем признак ожидающего update
		this->_sizeUpdatePending = false;
	}
}
/**
 * @brief Метод получения размера закодированного списка заголовков до сжатия
 *
 * @return размер списка заголовков текущего блока до сжатия
 *
 */
uint64_t awh::http::h2::hpack::Encoder::listSize() const noexcept {
	// Выводим размер списка заголовков текущего блока до сжатия
	return this->_listSize;
}
/**
 * @brief Метод получения собственной динамической таблицы
 *
 * @return собственная динамическая таблица
 *
 */
awh::http::h2::hpack::dynamic_table_t & awh::http::h2::hpack::Encoder::table() noexcept {
	// Выводим собственную динамическую таблицу
	return this->_table;
}
/**
 * @brief Метод изменения максимального размера своей динамической таблицы (RFC 7541 §4.2)
 *
 * @param size новый максимальный размер таблицы
 *
 */
void awh::http::h2::hpack::Encoder::setMaxTableSize(const uint32_t size) noexcept {
	// Если размер таблицы не изменился - ничего не делаем
	if(size == this->_table.maxSize())
		// Выходим из метода
		return;
	// Применяем новый размер сразу (с вытеснением)
	this->_table.setMaxSize(size);
	// Если серия изменений между блоками уже началась - запоминаем её наименьший размер
	if(this->_sizeUpdatePending)
		// Обновляем наименьший размер серии изменений
		this->_pendingMinSize = ((size < this->_pendingMinSize) ? size : this->_pendingMinSize);
	// Иначе начинаем новую серию изменений
	else this->_pendingMinSize = size;
	// Запоминаем значение размера для отправляемого update
	this->_pendingSize = size;
	// Отмечаем что в начале следующего блока нужен Dynamic Table Size Update
	this->_sizeUpdatePending = true;
}
/**
 * @brief Метод управления автоматическим определением чувствительных заголовков
 *
 * @param mode режим автоматического определения
 *
 */
void awh::http::h2::hpack::Encoder::sensitiveHeuristic(const bool mode) noexcept {
	// Устанавливаем режим автоматического определения чувствительных заголовков
	this->_sensitiveHeuristic = mode;
}
/**
 * @brief Метод управления адаптивной индексацией заголовков
 *
 * @param mode режим адаптивной индексации
 *
 */
void awh::http::h2::hpack::Encoder::adaptiveIndexing(const bool mode) noexcept {
	// Сбрасываем позицию записи кольца хешей
	this->_historyIndex = 0;
	// Сбрасываем признак заполненности кольца хешей
	this->_historyWrapped = false;
	// Если адаптивная индексация выключается
	if(!mode){
		// Освобождаем кольцо хешей: пустое кольцо и означает выключенный режим
		this->_history.clear();
		// Освобождаем занятую кольцом память
		this->_history.shrink_to_fit();
		// Выходим из метода
		return;
	}
	/**
	 * Ёмкость кольца берётся от размера таблицы: столько записей она вместила бы
	 * при среднем размере заголовка втрое больше служебной надбавки. Кольцо
	 * заведомо короче рабочего набора уникальных значений, и именно поэтому
	 * разовые значения повтора внутри него не набирают
	 */
	const size_t capacity = ::max(static_cast <size_t> (1), static_cast <size_t> (this->_table.maxSize() / (32 * 3)));
	// Выделяем кольцо хешей с дополнительной ячейкой под ограничитель перебора
	this->_history.assign((capacity + 1), 0);
}
/**
 * @brief Метод кодирования списка заголовков
 *
 * @param fields     заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
 * @param output     выходной буфер блока заголовков
 * @param useHuffman применять Huffman-кодирование к строкам
 *
 */
void awh::http::h2::hpack::Encoder::encode(const vector <field_t> & fields, string & output, const bool useHuffman) noexcept {
	// Дописываем отложенный Dynamic Table Size Update (если требуется)
	this->begin(output);
	/**
	 * Выполняем перебор всех кодируемых заголовков
	 */
	for(const field_t & field : fields)
		// Кодируем очередной заголовок
		this->encode(field.name, field.value, output, field.sensitive, useHuffman);
}
/**
 * @brief Метод кодирования списка декодированных заголовков (перекодирование)
 *
 * @param fields     декодированные заголовки
 * @param output     выходной буфер блока заголовков
 * @param useHuffman применять Huffman-кодирование к строкам
 *
 */
void awh::http::h2::hpack::Encoder::encode(const vector <field_view_t> & fields, string & output, const bool useHuffman) noexcept {
	// Дописываем отложенный Dynamic Table Size Update (если требуется)
	this->begin(output);
	/**
	 * Выполняем перебор всех кодируемых заголовков
	 */
	for(const field_view_t & field : fields)
		// Кодируем очередной заголовок
		this->encode(field.name, field.value, output, field.sensitive, useHuffman);
}
/**
 * @brief Метод кодирования одного заголовка (zero-copy, без владения строками)
 *
 * @note Псевдо-заголовки :method/:path/... должны кодироваться первыми,
 *       названия заголовков - строго в нижнем регистре (RFC 9113 §8.2.1).
 *
 * @param name       название заголовка
 * @param value      значение заголовка
 * @param output     выходной буфер блока заголовков
 * @param sensitive  чувствительное значение (Literal Never Indexed, RFC 7541 §7.1.3)
 * @param useHuffman применять Huffman-кодирование к строкам
 *
 */
void awh::http::h2::hpack::Encoder::encode(string_view name, string_view value, string & output, const bool sensitive, const bool useHuffman) noexcept {
	// Наращиваем размер списка заголовков блока до сжатия (RFC 9113 §6.5.2)
	this->_listSize += (name.size() + value.size() + 32);
	// Индекс совпадения только по имени
	uint64_t nameIndex = 0;
	// Если значение заголовка чувствительное (явно или по названию заголовка)
	if(sensitive || (this->_sensitiveHeuristic && ::isSensitiveName(name))){
		/**
		 * Literal Never Indexed (RFC 7541 §6.2.3, префикс 4 бита, паттерн 0001xxxx).
		 * Значение не индексируется и не попадает в динамическую таблицу;
		 * для имени допускается ссылка на индекс (только имя)
		 */
		nameIndex = this->lookupName(name);
		// Дописываем индекс имени (или 0)
		prefixed::encode(output, nameIndex, 4, 0x10);
		// Если совпадение по имени не найдено
		if(nameIndex == 0)
			// Дописываем название заголовка строкой
			::encodeStringLiteral(output, name, useHuffman);
		// Дописываем значение заголовка строкой
		::encodeStringLiteral(output, value, useHuffman);
		// В таблицу заголовок НЕ добавляем
		return;
	}
	// Выполняем поиск заголовка в таблицах
	const uint64_t fullIndex = this->lookup(name, value, nameIndex);
	// Если найдено полное совпадение (имя и значение уже в таблице)
	if(fullIndex != 0){
		// Дописываем Indexed Header Field (RFC 7541 §6.1)
		prefixed::encode(output, fullIndex, 7, 0x80);
		// Кодирование заголовка завершено
		return;
	}
	// Определяем, стоит ли заносить заголовок в динамическую таблицу
	const bool indexing = this->indexable(name, value);
	/**
	 * Если заголовок индексируется - Literal с инкрементальной индексацией
	 * (RFC 7541 §6.2.1, префикс 6 бит, старший бит 0x40), иначе Literal without
	 * Indexing (RFC 7541 §6.2.2, префикс 4 бита, паттерн 0000xxxx).
	 *
	 * Выбор представления обязан совпасть с решением об индексации: декодер пира
	 * добавляет запись в таблицу именно по представлению, и расхождение
	 * рассинхронизировало бы таблицы, а с ними и все последующие индексы.
	 *
	 * nameIndex == 0 - имя кодируется строкой; иначе ссылаемся на существующее имя
	 */
	if(indexing)
		// Дописываем представление с инкрементальной индексацией
		prefixed::encode(output, nameIndex, 6, 0x40);
	// Если заголовок не индексируется
	else prefixed::encode(output, nameIndex, 4, 0x00);
	// Если совпадение по имени не найдено
	if(nameIndex == 0)
		// Дописываем название заголовка строкой
		::encodeStringLiteral(output, name, useHuffman);
	// Дописываем значение заголовка строкой
	::encodeStringLiteral(output, value, useHuffman);
	// Если заголовок индексируется
	if(indexing)
		/**
		 * Добавляем в свою динамическую таблицу - декодер пира сделает то же,
		 * поэтому индексы остаются синхронными
		 */
		this->_table.add(name, value);
}
/**
 * @brief Метод принятия решения об индексации заголовка
 *
 * @param name  название заголовка
 * @param value значение заголовка
 * @return      признак необходимости занести заголовок в динамическую таблицу
 *
 */
bool awh::http::h2::hpack::Encoder::indexable(const string_view name, const string_view value) noexcept {
	// Если адаптивная индексация выключена - индексируем всё подряд
	if(this->_history.empty())
		// Заголовок подлежит индексации
		return true;
	// Вычисляем хеш пары название-значение
	const uint32_t hash = static_cast <uint32_t> (
		(std::hash <string_view> {}(name) * 31) ^ std::hash <string_view> {}(value)
	);
	/**
	 * Ёмкость кольца на единицу меньше выделенного места: последняя ячейка
	 * отведена под ограничитель перебора и в историю не входит
	 */
	const size_t capacity = (this->_history.size() - 1);
	// Позиция, до которой кольцо заполнено
	const size_t last = (this->_historyWrapped ? capacity : this->_historyIndex);
	/**
	 * Ставим искомый хеш ограничителем за концом заполненной части: перебор
	 * тогда обходится без проверки границы на каждом шаге и останавливается
	 * либо на совпадении, либо на ограничителе
	 */
	this->_history[last] = hash;
	// Позиция найденного совпадения
	size_t position = 0;
	/**
	 * Выполняем поиск хеша среди ранее встреченных
	 */
	while(this->_history[position] != hash)
		// Переходим к следующей позиции кольца
		position++;
	// Запоминаем хеш заголовка в кольце
	this->_history[this->_historyIndex] = hash;
	// Продвигаем позицию записи кольца
	this->_historyIndex = ((this->_historyIndex + 1) % capacity);
	// Если кольцо заполнено целиком - запоминаем это
	this->_historyWrapped = (this->_historyWrapped || (this->_historyIndex == 0));
	/**
	 * Индексируем заголовок, если он уже встречался в пределах кольца. Пока кольцо
	 * не заполнено, индексируем всё: на старте соединения истории ещё нет, и отказ
	 * от индексации лишил бы таблицу как раз тех заголовков, которые повторяются
	 * в каждом запросе
	 */
	return ((position < last) || !this->_historyWrapped);
}
/**
 * @brief Конструктор
 *
 * @param maxTableSize максимальный размер динамической таблицы
 *
 */
awh::http::h2::hpack::Encoder::Encoder(const uint32_t maxTableSize) noexcept :
 _table(maxTableSize, true), _pendingSize(0), _pendingMinSize(0), _listSize(0),
 _historyIndex(0), _historyWrapped(false),
 _sizeUpdatePending(false), _sensitiveHeuristic(true) {
	// Включаем адаптивную индексацию заголовков
	this->adaptiveIndexing(true);
}
