/**
 * @file: net.cpp
 * @date: 2023-02-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Если мы используем порядок байтов Little Endian или Big Endian
 */
#if __BYTE_ORDER__ && __ORDER_LITTLE_ENDIAN__
	/**
	 * Макрос проверки порядка поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
/**
 * Если мы используем порядок байтов Little Endian
 */
#elif __LITTLE_ENDIAN__ || _LITTLE_ENDIAN
	/**
	 * Включаем макрос поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN 1
/**
 * Если мы используем порядок байтов Big Endian
 */
#elif __BIG_ENDIAN__ || _BIG_ENDIAN
	/**
	 * Отключаем макрос поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN 0
#else
	/**
	 * @brief Проверка поддержки Little Endian
	 *
	 * @return результат проверки
	 */
	static inline bool isLittleEndian() noexcept {
		// Тестируем порядок байтов
		uint16_t test = 1;
		// Возвращаем результат проверки
		return ((* reinterpret_cast <uint8_t *> (&test)) == 1);
	}
	/**
	 * Макрос проверки поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN isLittleEndian()
#endif

/**
 * Стандартные модули
 */
#include <cmath>
#include <bitset>
#include <limits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>

/**
 * Подключаем заголовочный файл
 */
#include <net/net.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем статические функции в пространство имён
 */
namespace {
	/**
	 * @brief Вспомогательная функция для получения размера буфера
	 *
	 * @tparam T размер буфера данных
	 */
	template <size_t T = 4>
	/**
	 * @brief Вспомогательная функция конвертации IPv6-адреса в нужный порядок байт
	 *
	 * @param src исходный IPv6-адрес
	 * @param dst результирующий IPv6-адрес
	 */
	void convertEndian(const uint8_t src[T], uint8_t dst[T]) noexcept {
		/**
		 * Если порядок байт Little Endian
		 */
		#if IS_LITTLE_ENDIAN
			// На little-endian: переворачиваем все T байт
			std::reverse_copy(src, src + T, dst);
		/**
		 * Если порядок байт Big Endian
		 */
		#else
			// На big-endian: ничего не делаем
			::memcpy(dst, src, T);
		#endif
	}
	/**
	 * @brief Вспомогательная функция для проверки шестнадцатеричного символа
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 */
	bool ishex(const char letter) noexcept {
		// Возвращаем результат проверки
		return (
			((letter >= '0') && (letter <= '9')) ||
			((letter >= 'a') && (letter <= 'f')) ||
			((letter >= 'A') && (letter <= 'F'))
		);
	}
	/**
	 * @brief Вспомогательная функция для получения числового значения шестнадцатеричного символа
	 *
	 * @param letter проверяемый символ
	 * @return       числовое значение символа
	 */
	int32_t hexval(const char letter) noexcept {
		// Проверяем шестнадцатеричный символ
		if((letter >= '0') && (letter <= '9'))
			// Возвращаем числовое значение символа
			return (letter - '0');
		// Проверяем шестнадцатеричный символ в нижнем регистре
		if((letter >= 'a') && (letter <= 'f'))
			// Возвращаем числовое значение символа
			return (10 + (letter - 'a'));
		// Проверяем шестнадцатеричный символ в верхнем регистре
		if((letter >= 'A') && (letter <= 'F'))
			// Возвращаем числовое значение символа
			return (10 + (letter - 'A'));
		// Возвращаем ошибку
		return -1;
	}
	/**
	 * @brief Вспомогательная функция для проверки десятичного символа
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 */
	bool isdec(const char letter) noexcept {
		// Возвращаем результат проверки
		return ((letter >= '0') && (letter <= '9'));
	}
	/**
	 * @brief Вспомогательная функция для проверки восьмеричного символа
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 */
	bool isoct(const char letter) noexcept {
		// Возвращаем результат проверки
		return ((letter >= '0') && (letter <= '7'));
	}
	/**
	 * @brief Вспомогательная функция для обрезки пробелов в строке
	 *
	 * @param text исходный текст для обрезки
	 * @return     обрезанный текст
	 */
	string_view trim(string_view text) noexcept {
		// Позиции начала и конца обрезанной строки
		size_t i = 0, j = text.size();
		/**
		 * Выполняем обрезку пробелов в начале и конце строки
		 */
		while(i < j && std::isspace(static_cast <uint8_t> (text[i]))) ++i;
		/**
		 * Обрезаем пробелы в конце строки
		 */
		while(j > i && std::isspace(static_cast <uint8_t> (text[j-1]))) --j;
		// Возвращаем обрезанную строку
		return text.substr(i, j - i);
	}
	/**
	 * @brief Вспомогательная функция для проверки префикса строки
	 *
	 * @param text исходный текст для проверки
	 * @param pfx  префикс для проверки
	 * @return     результат проверки
	 */
	bool startWith(string_view text, string_view pfx) noexcept {
		// Возвращаем результат проверки
		return ((text.size() >= pfx.size()) && std::equal(pfx.begin(), pfx.end(), text.begin()));
	}
	/**
	 * @brief Вспомогательная функция для проверки суффикса строки
	 *
	 * @param text исходный текст для проверки
	 * @param sfx  суффикс для проверки
	 * @return     результат проверки
	 */
	bool endWith(string_view text, string_view sfx) noexcept {
		// Возвращаем результат проверки
		return ((text.size() >= sfx.size()) && std::equal(text.end() - sfx.size(), text.end(), sfx.begin()));
	}
	/**
	 * @brief Вспомогательная функция для разбиения строки по разделителю
	 *
	 * @param text   исходный текст для разбиения
	 * @param delim  разделитель для разбиения
	 * @param result результирующий массив частей строки
	 * @param max    максимальное количество частей строки
	 */
	void split(string_view text, const char delim, vector <string_view> & result, const size_t max = numeric_limits <size_t>::max()) noexcept {
		// Очищаем результирующий массив частей строки
		result.clear();
		// Позиция в строке, её размер и следующая позиция разделителя
		size_t pos = 0, length = text.size(), next = 0;
		/**
		 * Пока не достигнут конец строки и не превышено максимальное количество частей строки
		 */
		while((pos <= length) && (result.size() < max)){
			// Ищем следующую позицию разделителя
			next = ((pos < length) ? text.find(delim, pos) : string_view::npos);
			// Если разделитель не найден
			if(next == string_view::npos)
				// Устанавливаем позицию конца строки
				next = length;
			// Добавляем часть строки в результирующий массив
			result.emplace_back(text.substr(pos, next - pos));
			// Устанавливаем позицию следующего символа после разделителя
			pos = (next + 1);
			// Если достигнут конец строки
			if(next == length)
				// Прекращаем разбиение строки
				break;
		}
	}
	/**
	 * @brief Вспомогательная функция определения основания для legacy IPv4 части
	 * 
	 * @param token           строка части IP-адреса
	 * @param allowNonDecimal флаг разрешения не-десятичной системы счисления
	 * @return                основание системы счисления
	 */
	uint8_t detectBase(string_view token, bool allowNonDecimal) noexcept {
		// Если не разрешена не-десятичная система счисления
		if(!allowNonDecimal)
			// Возвращаем десятичную систему счисления
			return 10;
		// Проверяем префиксы системы счисления
		if((token.size() >= 2) && (token[0] == '0') && ((token[1] == 'x') || (token[1] == 'X')))
			// Возвращаем шестнадцатеричную систему счисления
			return 16;
		// Проверяем восьмеричную систему счисления
		if((token.size() > 1) && (token[0] == '0')){
			// Если только 0-7 -> oct, иначе decimal
			if(std::all_of(token.begin()+1, token.end(), isoct))
				// Возвращаем восьмеричную систему счисления
				return 8;
		}
		// Возвращаем десятичную систему счисления
		return 10;
	}
	/**
	 * @brief Вспомогательная функция для парсинга 64-битного числа из строки
	 *
	 * @param token  строка для парсинга
	 * @param base   основание системы счисления
	 * @param result результат парсинга
	 * @return       результат выполнения парсинга
	 */
	bool parse64(string_view token, const uint8_t base, uint64_t & result) noexcept {
		// Если строка пуста
		if(token.empty())
			// Возвращаем ошибку
			return false;
		// Инициализируем результат парсинга
		result = 0;
		// Парсим строку посимвольно
		size_t i = 0;
		// Если основание шестнадцатеричное
		if((base == 16) && (token.size() >= 2) && (token[0] == '0') && ((token[1] == 'x') || (token[1] == 'X')))
			// Начинаем парсинг с третьего символа
			i = 2;
		// Переменная для текущего символа
		char letter = 0;
		// Переменная для числового значения символа
		int32_t dig = -1;
		/**
		 * Парсим строку посимвольно
		 */
		for(; i < token.size(); ++i){
			// Инициализируем числовое значение символа
			dig = -1;
			// Текущий символ строки
			letter = token[i];
			// Проверяем символ в зависимости от основания системы счисления
			if(base == 10){
				// Проверяем десятичный символ
				if(!isdec(letter))
					// Возвращаем ошибку
					return false;
				// Получаем числовое значение символа
				dig = (letter - '0');
			// Если основание восьмеричное
			} else if(base == 8) {
				// Проверяем восьмеричный символ
				if(!isoct(letter))
					// Возвращаем ошибку
					return false;
				// Получаем числовое значение символа
				dig = (letter - '0');
			// Если основание шестнадцатеричное
			} else if(base == 16) {
				// Проверяем шестнадцатеричный символ
				dig = hexval(letter);
				// Если символ не шестнадцатеричный
				if(dig < 0)
					// Возвращаем ошибку
					return false;
			// Иначе неверное основание системы счисления
			} else return false;
			// Проверка переполнения
			if(result > ((numeric_limits<uint64_t>::max() - static_cast <uint64_t> (dig)) / static_cast <uint64_t> (base)))
				// Возвращаем ошибку
				return false;
			// Обновляем результат парсинга
			result = (result * static_cast <uint64_t> (base) + static_cast <uint64_t> (dig));
		}
		// Возвращаем успешный результат парсинга
		return true;
	}
	/**
	 * @brief Вспомогательная функция для парсинга всех форм IPv4-адреса
	 *
	 * @param ip              строка с IPv4-адресом
	 * @param result          результат парсинга в виде массива байт
	 * @param allowLegacy     флаг разрешения legacy форм
	 * @param allowNonDecimal флаг разрешения не-десятичной системы счисления
	 * @return                результат выполнения парсинга
	 */
	bool parseIPv4(string_view ip, vector <uint8_t> & result, const bool allowLegacy, const bool allowNonDecimal) noexcept {
		// Части строки IP-адреса
		vector <string_view> octets;
		// Разбиваем строку IP-адреса на части
		split(ip, '.', octets);
		// Если частей ровно 4
		if(octets.size() == 4){
			// Основание системы счисления
			uint8_t base = 0;
			// Значение результата парсинга
			uint64_t value = 0;
			// Каждая часть в 0..255, при allowNonDecimal можно 0x/0..oct
			for(uint8_t i = 0; i < 4; ++i){
				// Текущая часть строки IP-адреса
				auto octet = octets[i];
				// Если часть пустая
				if(octet.empty())
					// Возвращаем ошибку
					return false;
				// Определяем основание системы счисления
				base = detectBase(octet, allowNonDecimal);
				// Сбрасываем значение результата парсинга
				value = 0;
				// Парсим часть строки IP-адреса
				if(!parse64(octet, base, value))
					// Возвращаем ошибку
					return false;
				// Проверяем диапазон значения части
				if(value > 255)
					// Возвращаем ошибку
					return false;
				// Устанавливаем значение части в результат парсинга
				result[i] = static_cast <uint8_t> (value);
			}
			// Возвращаем успешный результат парсинга
			return true;
		}
		// Если legacy формы не разрешены
		if(!allowLegacy)
			// Возвращаем ошибку
			return false;
		// legacy формы октетов
		uint64_t a = 0, b = 0, c = 0, d = 0;
		// Проверяем форму с 3-мя октетами
		if(octets.size() == 3){
			// a.b.c  => a(8), b(8), c(16)
			uint8_t baseA = detectBase(octets[0], allowNonDecimal);
			uint8_t baseB = detectBase(octets[1], allowNonDecimal);
			uint8_t baseC = detectBase(octets[2], allowNonDecimal);
			// Парсим первый октет строки IP-адреса
			if(!parse64(octets[0], baseA, a) || (a > 0xFF))
				// Возвращаем ошибку
				return false;
			// Парсим второй октет строки IP-адреса
			if(!parse64(octets[1], baseB, b) || (b > 0xFF))
				// Возвращаем ошибку
				return false;
			// Парсим третий октет строки IP-адреса
			if(!parse64(octets[2], baseC, c) || (c > 0xFFFF))
				// Возвращаем ошибку
				return false;
			// Формируем 32-битный адрес
			const uint32_t addr = (
				(static_cast <uint32_t> (a) << 24) |
				(static_cast <uint32_t> (b) << 16) |
				static_cast <uint32_t> (c)
			);
			/**
			 * Формируем результат парсинга
			 */
			result[0] = static_cast <uint8_t> ((addr >> 24) & 0xFF);
			result[1] = static_cast <uint8_t> ((addr >> 16) & 0xFF);
			result[2] = static_cast <uint8_t> ((addr >> 8) & 0xFF);
			result[3] = static_cast <uint8_t> (addr & 0xFF);
			// Возвращаем успешный результат парсинга
			return true;
		// Если форма с 2-мя октетами
		} else if(octets.size() == 2) {
			// a.b => a(8), b(24)
			uint8_t baseA = detectBase(octets[0], allowNonDecimal);
			uint8_t baseB = detectBase(octets[1], allowNonDecimal);
			// Парсим первый октет строки IP-адреса
			if(!parse64(octets[0], baseA, a) || (a > 0xFF))
				// Возвращаем ошибку
				return false;
			// Парсим второй октет строки IP-адреса
			if(!parse64(octets[1], baseB, b) || (b > 0xFFFFFF))
				// Возвращаем ошибку
				return false;
			// Формируем 32-битный адрес
			uint32_t addr = (static_cast <uint32_t> (a) << 24) | static_cast <uint32_t> (b);
			/**
			 * Формируем результат парсинга
			 */
			result[0] = static_cast <uint8_t> ((addr >> 24) & 0xFF);
			result[1] = static_cast <uint8_t> ((addr >> 16) & 0xFF);
			result[2] = static_cast <uint8_t> ((addr >> 8) & 0xFF);
			result[3] = static_cast <uint8_t> (addr & 0xFF);
			// Возвращаем успешный результат парсинга
			return true;
		// Если форма с 1-м октетом
		} else if(octets.size() == 1) {
			// Получаем основание системы счисления
			uint8_t base = detectBase(octets[0], allowNonDecimal);
			// Парсим единственный октет строки IP-адреса
			if(!parse64(octets[0], base, d) || (d > 0xFFFFFFFFull))
				// Возвращаем ошибку
				return false;
			// Формируем 32-битный адрес
			uint32_t addr = static_cast <uint32_t> (d);
			/**
			 * Формируем результат парсинга
			 */
			result[0] = static_cast <uint8_t> ((addr >> 24) & 0xFF);
			result[1] = static_cast <uint8_t> ((addr >> 16) & 0xFF);
			result[2] = static_cast <uint8_t> ((addr >> 8) & 0xFF);
			result[3] = static_cast <uint8_t> (addr & 0xFF);
			// Возвращаем успешный результат парсинга
			return true;
		}
		// Возвращаем ошибку
		return false;
	}
	/**
	 * @brief Вспомогательная функция для парсинга десятичного квадрата IPv4-адреса
	 *
	 * @param ip     строка с десятичным квадратом IPv4-адреса
	 * @param result результат парсинга в виде массива байт
	 * @return       результат выполнения парсинга
	 */
	bool parseIPv4DecQuad(string_view ip, vector <uint8_t> & result) noexcept {
		// Части строки IP-адреса
		vector <string_view> octets;
		// Разбиваем строку IP-адреса на октеты
		split(ip, '.', octets);
		// Проверяем количество октетов строки IP-адреса
		if(octets.size() != 4)
			// Возвращаем ошибку
			return false;
		// Значение результата парсинга
		uint64_t value = 0;
		// Парсим каждый октет строки IP-адреса
		for(uint8_t i = 0; i < 4; ++i){
			// Текущий октет строки IP-адреса
			auto octet = octets[i];
			// Проверяем октет строки IP-адреса
			if(octet.empty() || ((octet.size() > 1) && (octet[0] == '+')))
				// Возвращаем ошибку
				return false;
			// Проверяем каждый символ октета строки IP-адреса
			if(!std::all_of(octet.begin(), octet.end(), isdec))
				// Возвращаем ошибку
				return false;
			// Разрешаем ведущие нули, но это всё равно десятичный формат
			if((octet.size() > 1) && (octet[0] == '0')){
				/**
				 * Разрешаем ведущие нули, но это всё равно десятичный формат
				 */
			}
			// Сбрасываем значение результата парсинга
			value = 0;
			// Парсим октет строки IP-адреса
			if(!parse64(octet, 10, value))
				// Возвращаем ошибку
				return false;
			// Проверяем диапазон значения октета
			if(value > 255)
				// Возвращаем ошибку
				return false;
			// Устанавливаем значение октета в результат парсинга
			result[i] = static_cast <uint8_t> (value);
		}
		// Возвращаем успешный результат парсинга
		return true;
	}
	/**
	 * @brief Вспомогательная функция для парсинга IPv6-адреса
	 *
	 * @param ip              строка с IPv6-адресом
	 * @param result          результат парсинга в виде массива байт
	 * @param allowEmbeddedV4 флаг разрешения встроенного IPv4-адреса
	 * @return                результат выполнения парсинга
	 */
	bool parseIPv6(string_view ip, vector <uint8_t> & result, const bool allowEmbeddedV4) noexcept {
		// Специальный случай "::"
		if(ip.compare("::") == 0){
			// Зануляем результат
			::memset(&result[0], 0, result.size());
			// Возвращаем успешный результат парсинга
			return true;
		}
		// Найдём "::" (не более одного)
		size_t dcol = ip.find("::");
		// Проверяем, найден ли "::"
		const bool hasDcol = (dcol != string_view::npos);
		// Части левой и правой части адреса
		vector <string_view> leftParts, rightParts;
		// Разбиваем адрес на левую и правую части
		if(hasDcol){
			// Получаем левую часть IP-адреса
			string_view left = ip.substr(0, dcol);
			// Получаем правую часть IP-адреса
			string_view right = ip.substr(dcol + 2);
			// Если левая часть не пуста
			if(!left.empty())
				// Разбиваем левую часть на хексеты
				split(left, ':', leftParts);
			// Если правая часть не пуста
			if(!right.empty())
				// Разбиваем правую часть на хексеты
				split(right, ':', rightParts);
		// Иначе разбиваем весь адрес на хексеты
		} else split(ip, ':', leftParts);
		/**
		 * @brief Парсинг хексета IPv6-адреса
		 *
		 * @param hextet строка с хексетом
		 * @param result значение полученного хексета
		 * @return       результат выполнения парсинга
		 */
		auto parseHextet = [](string_view hextet, uint16_t & result) noexcept -> bool {
			// Проверяем длину хексета
			if(hextet.empty() || (hextet.size() > 4))
				// Возвращаем ошибку
				return false;
			// Парсим хексет посимвольно
			uint32_t value = 0;
			// Проходим по каждому символу хексета
			for(char letter : hextet){
				// Проверяем шестнадцатеричный символ
				if(!ishex(letter))
					// Возвращаем ошибку
					return false;
				// Обновляем значение хексета
				value = ((value << 4) | static_cast <uint32_t> (hexval(letter)));
				// Проверяем переполнение хексета
				if(value > 0xFFFF)
					// Возвращаем ошибку
					return false;
			}
			// Устанавливаем результат парсинга
			result = static_cast <uint16_t> (value);
			// Возвращаем успешный результат парсинга
			return true;
		};
		/**
		 * @brief Парсинг IPv4-адреса в виде двух хексетов
		 *
		 * @param ip строка с IPv4-адресом
		 * @param h1 первый хексет
		 * @param h2 второй хексет
		 * @return   результат выполнения парсинга
		 */
		auto parseIPv4TailAsTwoHextets = [](string_view ip, uint16_t & h1, uint16_t & h2) noexcept -> bool {
			// Спарсеный IPv4-адрес
			vector <uint8_t> v4(4, 0);
			// Парсим IPv4-адрес
			if(!parseIPv4DecQuad(ip, v4))
				// Возвращаем ошибку
				return false;
			// Преобразуем первый хексет
			h1 = static_cast <uint16_t> ((static_cast <uint16_t> (v4[0]) << 8) | v4[1]);
			// Преобразуем второй хексет
			h2 = static_cast <uint16_t> ((static_cast <uint16_t> (v4[2]) << 8) | v4[3]);
			// Возвращаем успешный результат парсинга
			return true;
		};
		// Собранные слова IPv6-адреса
		vector <uint16_t> words;
		// Резервируем место под 8 слов
		words.reserve(8);
		// Первый и второй хексеты IPv4-адреса
		uint16_t hextet1 = 0, hextet2 = 0;
		// Разбираем левую часть
		for(uint8_t i = 0; i < static_cast <uint8_t> (leftParts.size()); ++i){
			// Текущий токен левой части
			auto part = leftParts[i];
			// Проверяем пустой токен
			if(part.empty())
				// Пустой токен допустим только как часть "::"
				return false;
			// Проверяем встроенный IPv4-адрес
			if(allowEmbeddedV4 && (part.find('.') != string_view::npos)){
				// IPv4 разрешён только если это последний токен слева и при отсутствии правой части
				if((i != static_cast <uint8_t> (leftParts.size() - 1)) || hasDcol)
					// Возвращаем ошибку
					return false;
				// Сбрасываем значение хексетов
				hextet1 = 0, hextet2 = 0;
				// Парсим IPv4 как два хексета
				if(!parseIPv4TailAsTwoHextets(part, hextet1, hextet2))
					// Возвращаем ошибку
					return false;
				// Добавляем хексеты в IPv6-адрес
				words.push_back(hextet1);
				words.push_back(hextet2);
			// Иначе парсим обычный хексет
			} else {
				// Сбрасываем значение хексета
				hextet1 = 0;
				// Парсим хексет
				if(!parseHextet(part, hextet1))
					// Возвращаем ошибку
					return false;
				// Добавляем хексет в IPv6-адрес
				words.push_back(hextet1);
			}
		}
		// Количество слов в правой части
		uint8_t rightWordsCount = 0;
		// Собранные слова правой части IPv6-адреса
		vector <uint16_t> rightWords;
		// Разбираем правую часть при наличии
		if(hasDcol){
			// Разобрать правую часть; если там IPv4 — только в самом последнем токене
			for(uint8_t i = 0; i < static_cast <uint8_t> (rightParts.size()); ++i){
				// Текущий токен правой части
				auto part = rightParts[i];
				// Проверяем пустой токен
				if(part.empty())
					// Пустой токен допустим только как часть "::"
					return false;
				// Проверяем встроенный IPv4-адрес
				if(allowEmbeddedV4 && (i == static_cast <uint8_t> (rightParts.size() - 1)) && (part.find('.') != string_view::npos)){
					// Сбрасываем значение хексетов
					hextet1 = 0, hextet2 = 0;
					// Парсим IPv4 как два хексета
					if(!parseIPv4TailAsTwoHextets(part, hextet1, hextet2))
						// Возвращаем ошибку
						return false;
					// Добавляем хексеты в правую часть IPv6-адреса
					rightWords.push_back(hextet1);
					rightWords.push_back(hextet2);
				// Иначе парсим обычный хексет
				} else {
					// Сбрасываем значение хексета
					hextet1 = 0;
					// Парсим хексет
					if(!parseHextet(part, hextet1))
						// Возвращаем ошибку
						return false;
					// Добавляем хексет в правую часть IPv6-адреса
					rightWords.push_back(hextet1);
				}
			}
			// Запоминаем количество слов в правой части
			rightWordsCount = static_cast <uint8_t> (rightWords.size());
		}
		// Сборка полного адреса
		if(hasDcol){
			// Со сжатием — общее количество слов должно быть не более 8
			if((static_cast <uint8_t> (words.size()) + rightWordsCount) > 8)
				// Возвращаем ошибку
				return false;
			// Вычисляем количество нулевых слов для вставки
			const uint8_t zerosToInsert = (8 - (static_cast <uint8_t> (words.size()) + rightWordsCount));
			// Полный набор слов IPv6-адреса
			vector <uint16_t> full;
			// Резервируем место под 8 слов
			full.reserve(8);
			// Собираем полный адрес
			full.insert(full.end(), words.begin(), words.end());
			// Вставляем нулевые слова
			for(uint8_t i = 0; i < zerosToInsert; ++i)
				// Вставляем нулевое слово
				full.push_back(0);
			// Добавляем правые слова
			full.insert(full.end(), rightWords.begin(), rightWords.end());
			// Проверяем размер полного адреса
			if(full.size() != 8)
				// Возвращаем ошибку
				return false;
			// Перебираем все слова полного адреса
			for(uint8_t i = 0; i < 8; ++i){
				/**
				 * Выполняем формирование результата парсинга
				 */
				result[2 * i] = static_cast <uint8_t> ((full[i] >> 8) & 0xFF);
				result[2 * i + 1] = static_cast <uint8_t> (full[i] & 0xFF);
			}
			// Возвращаем успешный результат парсинга
			return true;
		// Иначе без сжатия
		} else {
			// Без сжатия — либо 8 слов, либо 6 слов + IPv4 уже переведённый в 2 слова
			if(words.size() != 8)
				// Возвращаем ошибку
				return false;
			// Перебираем все слова полного адреса
			for(uint8_t i = 0; i < 8; ++i){
				/**
				 * Выполняем формирование результата парсинга
				 */
				result[2 * i]   = static_cast <uint8_t> ((words[i] >> 8) & 0xFF);
				result[2 * i + 1] = static_cast <uint8_t> (words[i] & 0xFF);
			}
			// Возвращаем успешный результат парсинга
			return true;
		}
	}
	/**
	 * @brief Структура опций парсинга IPv4-адреса
	 * 
	 */
	typedef struct Options {
		bool allowLegacyV4 = true;     // a.b.c, a.b, a и др.
		bool allowNonDecimalV4 = true; // 0xHEX, 0... OCT
		bool allowBrackets = true;     // [addr]
		bool allowZoneId = true;       // %zone, %25 в [ ]
		bool allowEmbeddedV4 = true;   // a:b:c:d:e:f:w.x.y.z
	} options_t;
	/**
	 * @brief Функция парсинга IPv4-адреса из строки
	 *
	 * @param ip      строка с IPv4-адресом
	 * @param result  результат парсинга в бинарном виде
	 * @param options опции парсинга
	 * @return        результат выполнения парсинга
	 */
	bool ipv4(string_view ip, vector <uint8_t> & result, const options_t & options = {}) noexcept {
		// Тримминг строки IP-адреса
		auto addr = trim(ip);
		// Обрабатываем скобки
		if(addr.empty())
			// Возвращаем ошибку
			return false;
		// Парсим IPv4-адрес
		if(!parseIPv4(addr, result, options.allowLegacyV4, options.allowNonDecimalV4))
			// Возвращаем ошибку
			return false;
		//  Возвращаем успешный результат парсинга
		return true;
	}
	/**
	 * @brief Функция парсинга IPv6-адреса из строки
	 *
	 * @param ip      строка с IPv6-адресом
	 * @param result  результат парсинга в бинарном виде
	 * @param zone    зона (zone-id) IPv6-адреса
	 * @param options опции парсинга
	 * @return        результат выполнения парсинга
	 */
	bool ipv6(string_view ip, vector <uint8_t> & result, string & zone, const options_t & options = {}) noexcept {
		// Тримминг строки IP-адреса
		auto addr = trim(ip);
		// Обрабатываем скобки
		if(addr.empty())
			// Возвращаем ошибку
			return false;
		// Проверяем квадратные скобки вокруг адреса
		if(options.allowBrackets && startWith(addr, "[") && endWith(addr, "]"))
			// Убираем скобки
			addr = addr.substr(1, addr.size() - 2);
		// Если разрешён zone-id
		if(options.allowZoneId){
			// Внутри скобок RFC 6874 требует %25 как экранированный '%'
			size_t pos = addr.find("%25");
			// Если найдено значение "%25"
			if(pos != string_view::npos){
				// Извлекаем зону
				zone = ::move(string(addr.substr(pos + 3)));
				// Обрезаем основной IP-адрес
				addr = addr.substr(0, pos);
			// Иначе ищем обычный '%'
			} else {
				// Ищем обычный '%'
				pos = addr.find('%');
				// Если найдено значение '%'
				if(pos != string_view::npos){
					// Извлекаем зону
					zone = ::move(string(addr.substr(pos + 1)));
					// Обрезаем основной IP-адрес
					addr = addr.substr(0, pos);
				}
			}
		}
		// Парсим IPv6-адрес
		if(!parseIPv6(addr, result, options.allowEmbeddedV4))
			// Возвращаем ошибку
			return false;
		// Возвращаем успешный результат парсинга
		return true;
	}
};

/**
 * @brief Метод инициализации списка локальных адресов
 *
 */
void awh::Net::initLocalNet() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если список локальных адресов пустой
		if(this->_localsNet.empty()){
			{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 128;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 128;
				// Устанавливаем IP-адрес
				localNet.begin->parse("::1");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("2001::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("2001:db8::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 96;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("64:ff9b::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 16;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("2002::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 10;
				// Устанавливаем IP-адрес начала диапазона
				localNet.begin->parse("fe80::");
				// Устанавливаем IP-адрес конца диапазона
				localNet.end->parse("febf::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 10;
				// Устанавливаем IP-адрес начала диапазона
				localNet.begin->parse("fec0::");
				// Устанавливаем IP-адрес конца диапазона
				localNet.end->parse("feff::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 7;
				// Устанавливаем IP-адрес
				localNet.begin->parse("fc00::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("ff00::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("0.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("0.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 10;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("100.64.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 16;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("169.254.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 4;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("224.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("224.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("224.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("239.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 4;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("240.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("255.255.255.255");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем IP-адрес
				localNet.begin->parse("10.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем IP-адрес
				localNet.begin->parse("127.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 12;
				// Устанавливаем IP-адрес
				localNet.begin->parse("172.16.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 29;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.170");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.171");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.2.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.88.99.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.88.99.1");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 16;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.168.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("198.51.100.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 15;
				// Устанавливаем IP-адрес
				localNet.begin->parse("198.18.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("203.0.113.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод заполнения недостающих элементов нулями
 *
 * @param num  число для заполнения нулями
 * @param size максимальная длина строки
 * @return     полученное число строки
 */
string && awh::Net::zerro(string && num, const uint8_t size) const noexcept {
	// Если число меньше максимальной длины строки
	if(static_cast <uint8_t> (num.size()) < size){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём строку для добавления
			const string result(size - static_cast <uint8_t> (num.size()), '0');
			// Добавляем недостающие нули в наше число
			num.insert(num.begin(), result.begin(), result.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(num, size),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return ::move(num);
}
/**
 * @brief Метод очистки данных IP-адреса
 *
 */
void awh::Net::clear() noexcept {
	// Выполняем сброс буфера данных
	this->_buffer.clear();
	// Устанавливаем тип IP-адреса
	this->_type = type_t::NONE;
}
/**
 * @brief Метод проверки соответствия адреса зеркалу IPv6 => IPv4
 *
 * @return результат проверки
 */
bool awh::Net::broadcastIPv6ToIPv4() const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём временный буфер данных для сравнения
			vector <uint16_t> buffer(6);
			// Устанавливаем хексет маски
			buffer[5] = 0xFFFF;
			// Если буфер данных принадлежит к вещанию IPv6 => IPv4
			result = (::memcmp(&buffer[0], &this->_buffer[0], (buffer.size() * 2)) == 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения зоны IPv6 адреса
 *
 * @return зона IPv6 адреса
 */
const string & awh::Net::zone() const noexcept {
	// Выводим результат
	return this->_zone;
}
/**
 * @brief Метод установки зоны IPv6 адреса
 *
 * @param zone зона IPv6 адреса для установки
 */
void awh::Net::zone(const string & zone) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем установку зоны IPv6 адреса
		this->_zone = zone;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(zone), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод извлечения типа IP-адреса
 *
 * @return тип IP-адреса
 */
awh::Net::type_t awh::Net::type() const noexcept {
	// Выполняем тип IP-адреса
	return this->_type;
}
/**
 * @brief Метод установки типа IP-адреса
 *
 * @param type тип IP-адреса для установки
 */
void awh::Net::type(const type_t type) noexcept {
	// Выполняем установку типа IP-адреса
	this->_type = type;
}
/**
 * @brief Метод определения типа хоста
 *
 * @param host хост для определения
 * @return     определённый тип хоста
 */
awh::Net::type_t awh::Net::host(const string & host) const noexcept {
	// Результат полученных данных
	type_t result = type_t::NONE;
	// Если хост передан
	if(!host.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем полную проверку всех типов хостов
			for(uint8_t i = 0; i < 9; i++){
				/**
				 * Устанавливаем тип для проверки
				 */
				switch(i){
					// Если проверяем IPv4-адрес
					case 0: result = type_t::IPV4; break;
					// Если проверяем IPv6-адрес
					case 1: result = type_t::IPV6; break;
					// Если проверяем MAC-адрес
					case 2: result = type_t::MAC; break;
					// Если проверяем сеть IPv4
					case 3: result = type_t::NETV4; break;
					// Если проверяем сеть IPv6
					case 4: result = type_t::NETV6; break;
					// Если проверяем URL-адрес
					case 5: result = type_t::URL; break;
					// Если проверяем адрес файловой системы
					case 6: result = type_t::FS; break;
					// Если проверяем доменное имя
					case 7: result = type_t::FQDN; break;
					// По умолчанию устанавливаем тип NONE
					default: result = type_t::NONE;
				}
				// Если проверка пройдена успешно
				if((result != type_t::NONE) && this->check(host, result))
					// Выводим результат
					return result;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(host), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения аппаратного адреса в чистом виде
 *
 * @return аппаратный адрес в чистом виде
 */
std::array <uint8_t, 6> awh::Net::mac() const noexcept {
	// Результат работы функции
	std::array <uint8_t, 6> result = {0};
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 6){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перевод бинарного буфера MAC-адреса в числовой вид
			::memcpy(&result[0], &this->_buffer[0], this->_buffer.size());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки аппаратного адреса в чистом виде
 *
 * @param addr аппаратный адрес в чистом виде
 */
void awh::Net::mac(const std::array <uint8_t, 6> & addr) noexcept {
	// Если MAC адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти для MAC адреса
			this->_buffer.resize(6);
			// Устанавливаем тип MAC адреса
			this->_type = type_t::MAC;
			// Выполняем копирование данных адреса MAC
			::memcpy(&this->_buffer[0], &addr[0], this->_buffer.size());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr.front(), addr.back()),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Извлечения адреса IPv4 в чистом виде
 *
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 * @return       адрес IPv4 в чистом виде
 */
uint32_t awh::Net::v4(const endian_t endian) const noexcept {
	// Результат работы функции
	uint32_t result = 0;
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 4){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Получаем буфер данных IP-адреса
					::convertEndian(&this->_buffer[0], reinterpret_cast <uint8_t *> (&result));
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv4
					::memcpy(&result, &this->_buffer[0], this->_buffer.size());
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки адреса IPv4 в чистом виде
 *
 * @param addr   адрес IPv4 в чистом виде
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 */
void awh::Net::v4(const uint32_t addr, const endian_t endian) noexcept {
	// Если IPv4 адрес передан
	if(addr > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти для IPv4 адреса
			this->_buffer.resize(4);
			// Устанавливаем тип IP-адреса
			this->_type = type_t::IPV4;
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Устанавливаем буфер данных IP-адреса
					::convertEndian(reinterpret_cast <const uint8_t *> (&addr), &this->_buffer[0]);
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv4
					::memcpy(&this->_buffer[0], &addr, sizeof(addr));
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr, static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Извлечения адреса IPv6 в чистом виде
 *
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 * @return       адрес IPv6 в чистом виде
 */
std::array <uint8_t, 16> awh::Net::v6(const endian_t endian) const noexcept {
	// Результат работы функции
	std::array <uint8_t, 16> result;
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 16){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Получаем буфер данных IP-адреса
					::convertEndian <16> (&this->_buffer[0], reinterpret_cast <uint8_t *> (&result[0]));
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv6
					::memcpy(&result[0], &this->_buffer[0], this->_buffer.size());
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки адреса IPv6 в чистом виде
 *
 * @param addr   адрес IPv6 в чистом виде
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 */
void awh::Net::v6(const std::array <uint8_t, 16> & addr, const endian_t endian) noexcept {
	// Если IPv6 адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти для IPv6 адреса
			this->_buffer.resize(16);
			// Устанавливаем тип IP-адреса
			this->_type = type_t::IPV6;
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Устанавливаем буфер данных IP-адреса
					::convertEndian <16> (reinterpret_cast <const uint8_t *> (&addr[0]), &this->_buffer[0]);
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv6
					::memcpy(&this->_buffer[0], &addr[0], sizeof(addr));
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr.front(), addr.back()),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод проверки валидности IP-адреса
 *
 * @param addr адрес аппаратный или интернет подключения для проверки
 * @param type тип адреса аппаратного или интернет подключения для проверки
 * @return     результат проверки
 */
bool awh::Net::check(const string_view addr, const type_t type) const noexcept {
	// Если адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если адрес является адресом MAC
				case static_cast <uint8_t> (type_t::MAC): {
					// Проверяем длину MAC-адреса
					if((addr.length() != 17) && (addr.length() != 12))
						// Если длина MAC-адреса некорректна
						return false;
					/**
					 * @brief Функция проверки является ли символ шестнадцатеричным
					 *
					 * @param c проверяемый символ
					 * @return  результат проверки
					 */
					auto ishex = [](const char c) noexcept -> bool {
						// Проверяем является ли символ шестнадцатеричным
						return (
							((c >= '0') && (c <= '9')) ||
							((c >= 'a') && (c <= 'f')) ||
							((c >= 'A') && (c <= 'F'))
						);
					};
					// Если длина MAC-адреса равна 12 символам
					if(addr.length() == 12){
						// Выполняем проверку каждого символа MAC-адреса
						for(char c : addr){
							// Если символ не является шестнадцатеричным
							if(!ishex(c))
								// Возвращаем результат проверки
								return false;
						}
						// Возвращаем результат проверки
						return true;
					}
					// Выполняем проверку каждого символа MAC-адреса
					for(size_t i = 0; i < 17; ++i){
						// Если символ не является шестнадцатеричным
						if((i % 3) == 2){
							// Если символ не является разделителем
							if((addr[i] != ':') && (addr[i] != '-'))
								// Возвращаем результат проверки
								return false;
						// Если символ является частью шестнадцатеричного числа
						} else {
							// Если символ не является шестнадцатеричным
							if(!ishex(addr[i]))
								// Возвращаем результат проверки
								return false;
						}
					}
					// Возвращаем результат проверки
					return true;
				}
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Временный буфер для проверки IP-адреса
					vector <uint8_t> buffer(4, 0);
					// Выполняем проверку IP-адреса IPv4
					return ::ipv4(addr, buffer);
				}
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Временное значение зоны для проверки IP-адреса
					string zone = "";
					// Временный буфер для проверки IP-адреса
					vector <uint8_t> buffer(16, 0);
					// Выполняем проверку IP-адреса IPv6
					return ::ipv6(addr, buffer, zone);
				}
				// Если IP-адрес определён как NetV4
				case static_cast <uint8_t> (type_t::NETV4): {
					// Разделяем адрес и маску сети
					const auto pos = addr.find('/');
					// Если разделитель найден
					if(pos != string::npos){
						// Временный буфер для проверки IP-адреса
						vector <uint8_t> buffer(4, 0);
						// Получаем IP-адрес без маски сети
						const string_view ip = addr.substr(0, pos);
						// Получаем маску переданной сети
						const string_view suffix = addr.substr(pos + 1);
						// Проверяем является ли суффикс числом
						if(std::all_of(suffix.begin(), suffix.end(), ::isdigit)){
							// Получаем префикс сети
							if(this->_fmk->atoi <uint8_t> (suffix.data(), suffix.length()) > 32)
								// Если префикс сети больше допустимого значения
								return false;
						// Если суффикс не является числом и не является корректной маской сети
						} else if(!::ipv4(suffix, buffer))
							// Возвращаем результат проверки
							return false;
						// Зануляем структуру 
						::memset(&buffer[0], 0, buffer.size());
						// Выполняем проверку IP-адреса IPv4
						return ::ipv4(ip, buffer);
					}
				} break;
				// Если IP-адрес определён как NetV6
				case static_cast <uint8_t> (type_t::NETV6): {
					// Разделяем адрес и маску сети
					const auto pos = addr.find('/');
					// Если разделитель найден
					if(pos != string::npos){
						// Временное значение зоны для проверки IP-адреса
						string zone = "";
						// Временный буфер для проверки IP-адреса
						vector <uint8_t> buffer(16, 0);
						// Получаем IP-адрес без маски сети
						const string_view ip = addr.substr(0, pos);
						// Получаем маску переданной сети
						const string_view suffix = addr.substr(pos + 1);
						// Проверяем является ли суффикс числом
						if(std::all_of(suffix.begin(), suffix.end(), ::isdigit)){
							// Получаем префикс сети
							if(this->_fmk->atoi <uint8_t> (suffix.data(), suffix.length()) > 128)
								// Если префикс сети больше допустимого значения
								return false;
						// Если суффикс не является числом и не является корректной маской сети
						} else if(!::ipv6(suffix, buffer, zone))
							// Возвращаем результат проверки
							return false;
						// Зануляем структуру 
						::memset(&buffer[0], 0, buffer.size());
						// Выполняем проверку IP-адреса IPv6
						return ::ipv6(ip, buffer, zone);
					}
				} break;
				// Если адрес принадлежит к URL-адресам
				case static_cast <uint8_t> (type_t::URL): {
					// Проверяем длину URL-адреса
					if(addr.length() < 8)
						// Если длина URL-адреса некорректна
						return false;
					// Проверяем префикс URL-адреса
					return (
						this->_fmk->compare("http://", string(addr.data(), 7).c_str()) ||
						this->_fmk->compare("https://", string(addr.data(), 8).c_str())
					);
				}
				// Если адрес принадлежит к адресу файловой системы
				case static_cast <uint8_t> (type_t::FS): {
					// Windows: X:\ или X:/
					if((addr.length() >= 3) && 
					 (((addr[1] == ':') && ((addr[2] == '\\') || (addr[2] == '/'))) ||
					  ((addr[0] == '\\') && (addr[1] == '\\'))))
						// Возвращаем результат проверки
						return true;
					// Unix: начинается с /
					return (addr.front() == '/');
				}
				// Если адрес принадлежит к доменным именам
				case static_cast <uint8_t> (type_t::FQDN): {
					// Создаём представление строки для проверки
					if(addr.length() > 253)
						// Если длина доменного имени превышает допустимое значение
						return false;
					// Разрешаем localhost как особый случай
					if(this->_fmk->compare("localhost", addr.data()))
						// Если адрес равен localhost
						return true;
					// Начальное и конечное значение итератора
					size_t start = 0, end = 0;
					// Выполняем проверку каждой метки доменного имени
					while(start < addr.length()) {
						// Находим конец текущей метки
						end = addr.find('.', start);
						// Если точка не найдена
						if(end == string::npos)
							// Устанавливаем конец на размер строки
							end = addr.length();
						// Извлекаем текущую метку
						const string_view label = addr.substr(start, end - start);
						// Проверяем корректность метки
						if(label.empty() || (label.length() > 63))
							// Если метка пустая или превышает допустимую длину
							return false;
						// Метка не должна начинаться или заканчиваться дефисом
						if((label[0] == '-') || (label.back() == '-'))
							// Если метка начинается или заканчивается дефисом
							return false;
						// Проверяем каждый символ метки
						for(char c : label){
							// Если символ не является допустимым
							if(!(std::isalnum(c) || (c == '-')))
								// Возвращаем результат проверки
								return false;
						}
						// Переходим к следующей метке
						start = (end + 1);
					}
					// Должно быть хотя бы две метки (example.com)
					return (addr.rfind('.') != string::npos);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат проверки адреса
	return false;
}
/**
 * @brief Метод наложения маски сети
 *
 * @param mask маска сети для наложения
 * @param addr тип получаемого адреса
 */
void awh::Net::impose(const string & mask, const addr_t addr) noexcept {
	// Выполняем наложение маски сети
	this->impose(mask, addr, this->_type);
}
/**
 * @brief Метод наложения маски сети
 *
 * @param mask маска сети для наложения
 * @param addr тип получаемого адреса
 * @param type тип адреса аппаратного или интернет подключения
 */
void awh::Net::impose(const string & mask, const addr_t addr, const type_t type) noexcept {
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем применение префикса
		if(prefix > 0)
			// Выполняем наложение маски сети
			this->impose(prefix, addr, type);
	}
}
/**
 * @brief Метод наложения префикса
 *
 * @param prefix префикс для наложения
 * @param addr тип получаемого адреса
 */
void awh::Net::impose(const uint8_t prefix, const addr_t addr) noexcept {
	// Выполняем наложение префикса адреса
	this->impose(prefix, addr, this->_type);
}
/**
 * @brief Метод наложения префикса
 *
 * @param prefix префикс для наложения
 * @param addr   тип получаемого адреса
 * @param type   тип адреса аппаратного или интернет подключения
 */
void awh::Net::impose(const uint8_t prefix, const addr_t addr, const type_t type) noexcept {
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && (prefix > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 32){
						// Определяем номер октета
						const uint8_t num = static_cast <uint8_t> (::ceil(static_cast <double> (prefix / 8)));
						/**
						 * Определяем тип получаемого адреса
						 */
						switch(static_cast <uint8_t> (addr)){
							// Если мы хотим получить адрес хоста
							case static_cast <uint8_t> (addr_t::HOST): {
								// Зануляем все остальные биты
								::memset(&this->_buffer[0], 0, num);
								// Если префикс не кратен 8
								if((prefix % 8) != 0){
									// Данные октета
									uint8_t oct = 0;
									// Получаем нужное нам значение октета
									::memcpy(&oct, &this->_buffer[0] + num, sizeof(oct));
									// Переводим октет в бинарный вид
									std::bitset <8> bits(oct);
									// Зануляем все лишние элементы
									for(uint8_t i = (8 - (prefix % 8)); i < 8; i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение октета
									oct = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение октета
									::memcpy(&this->_buffer[0] + num, &oct, sizeof(oct));
								}
							} break;
							// Если мы хотим получить сетевой адрес
							case static_cast <uint8_t> (addr_t::NETWORK): {
								// Если префикс кратен 8
								if((prefix % 8) == 0)
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + num, 0, this->_buffer.size() - num);
								// Если префикс не кратен 8
								else {
									// Данные хексета
									uint8_t oct = 0;
									// Получаем нужное нам значение октета
									::memcpy(&oct, &this->_buffer[0] + num, sizeof(oct));
									// Переводим октет в бинарный вид
									std::bitset <8> bits(oct);
									// Зануляем все лишние элементы
									for(uint8_t i = 0; i < (8 - (prefix % 8)); i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение октета
									oct = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение октета
									::memcpy(&this->_buffer[0] + num, &oct, sizeof(oct));
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + (num + 1), 0, this->_buffer.size() - (num + 1));
								}
							} break;
						}
					}
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 128){
						// Определяем номер хексета
						const uint8_t num = static_cast <uint8_t> (::ceil(static_cast <double> (prefix / 16)));
						/**
						 * Определяем тип получаемого адреса
						 */
						switch(static_cast <uint8_t> (addr)){
							// Если мы хотим получить адрес хоста
							case static_cast <uint8_t> (addr_t::HOST): {
								// Зануляем все остальные биты
								::memset(&this->_buffer[0], 0, (num * 2));
								// Если префикс не кратен 16
								if((prefix % 16) != 0){
									// Данные хексета
									uint16_t hex = 0;
									// Получаем нужное нам значение хексета
									::memcpy(&hex, &this->_buffer[0] + (num * 2), sizeof(hex));
									// Переводим хексет в бинарный вид
									std::bitset <16> bits(hex);
									// Зануляем все лишние элементы
									for(uint8_t i = (16 - (prefix % 16)); i < 16; i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение хексета
									hex = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение хексета
									::memcpy(&this->_buffer[0] + (num * 2), &hex, sizeof(hex));
								}
							} break;
							// Если мы хотим получить сетевой адрес
							case static_cast <uint8_t> (addr_t::NETWORK): {
								// Если префикс кратен 16
								if((prefix % 16) == 0)
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + (num * 2), 0, this->_buffer.size() - (num * 2));
								// Если префикс не кратен 16
								else {
									// Данные хексета
									uint16_t hex = 0;
									// Получаем нужное нам значение хексета
									::memcpy(&hex, &this->_buffer[0] + (num * 2), sizeof(hex));
									// Переводим хексет в бинарный вид
									std::bitset <16> bits(hex);
									// Зануляем все лишние элементы
									for(uint8_t i = 0; i < (16 - (prefix % 16)); i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение хексета
									hex = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение хексета
									::memcpy(&this->_buffer[0] + (num * 2), &hex, sizeof(hex));
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + ((num * 2) + 2), 0, this->_buffer.size() - ((num * 2) + 2));
								}
							} break;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(prefix, static_cast <uint16_t> (addr), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод перевода маски сети в префикс адреса
 *
 * @param mask маска сети для перевода
 * @return     полученный префикс адреса
 */
uint8_t awh::Net::mask2Prefix(const string & mask) const noexcept {
	// Выполняем преобразование маски сети в префикс адреса
	return this->mask2Prefix(mask, this->_type);
}
/**
 * @brief Метод перевода маски сети в префикс адреса
 *
 * @param mask маска сети для перевода
 * @param type тип адреса аппаратного или интернет подключения
 * @return     полученный префикс адреса
 */
uint8_t awh::Net::mask2Prefix(const string & mask, const type_t type) const noexcept {
	// Результат работы функции
	uint8_t result = 0;
	// Если маска сети передана
	if(!mask.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_fmk, this->_log);
			// Выполняем парсинг маски
			if(net.parse(mask) && (type == net.type())){
				// Бинарный контейнер
				std::bitset <8> bits;
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (type)){
					// Если IP-адрес определён как IPv4
					case static_cast <uint8_t> (type_t::IPV4): {
						// Получаем значение маски в виде адреса
						const uint32_t num = net.v4();
						// Выполняем перебор всего значения буфера
						for(uint8_t i = 0; i < 4; i++){
							// Переводим хексет в бинарный вид
							bits = (reinterpret_cast <const uint8_t *> (&num))[i];
							// Выполняем подсчёт префикса
							result += bits.count();
						}
					} break;
					// Если IP-адрес определён как IPv6
					case static_cast <uint8_t> (type_t::IPV6): {
						// Получаем значение маски в виде адреса
						const std::array <uint8_t, 16> num = net.v6();
						// Выполняем перебор всего значения буфера
						for(uint8_t i = 0; i < 16; i++){
							// Переводим хексет в бинарный вид
							bits = reinterpret_cast <const uint8_t *> (&num[0])[i];
							// Выполняем подсчёт префикса
							result += bits.count();
						}
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(mask, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод преобразования префикса адреса в маску сети
 *
 * @param prefix префикс адреса для преобразования
 * @return       полученная маска сети
 */
string awh::Net::prefix2Mask(const uint8_t prefix) const noexcept {
	// Выполняем перевод префикса адреса в маску сети
	return this->prefix2Mask(prefix, this->_type);
}
/**
 * @brief Метод преобразования префикса адреса в маску сети
 *
 * @param prefix префикс адреса для преобразования
 * @param type   тип адреса аппаратного или интернет подключения
 * @return       полученная маска сети
 */
string awh::Net::prefix2Mask(const uint8_t prefix, const type_t type) const noexcept {
	// Результат работы функции
	string result = "";
	// Если маска сети передана
	if(prefix > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_fmk, this->_log);
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 32){
						// Выполняем парсинг маски
						if(net.parse("255.255.255.255")){
							// Выполняем установку префикса
							net.impose(prefix, addr_t::NETWORK);
							// Выводим полученный адрес
							result = net;
						}
					}
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 128){
						// Выполняем парсинг маски
						if(net.parse("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")){
							// Выполняем установку префикса
							net.impose(prefix, addr_t::NETWORK);
							// Выводим полученный адрес
							result = net;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const string & mask) const noexcept {
	// Выполняем проверку вхождения IP-адреса в диапазон адресов
	return this->range(begin, end, mask, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @param type  тип адреса аппаратного или интернет подключения
 * @return      результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const string & mask, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем проверку вхождения адреса в диапазон адресов
		if(prefix > 0)
			// Выполняем получение результата
			result = this->range(begin, end, prefix, type);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const uint8_t prefix) const noexcept {
	// Выполняем проверку вхождения iP-адреса в диапазон адресов
	return this->range(begin, end, prefix, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @param type   тип адреса аппаратного или интернет подключения
 * @return       результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const uint8_t prefix, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если типы адресов совпадают
	if((type == begin.type()) && (type == end.type())){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объекты сетевых модулей
			net_t net1(this->_fmk, this->_log),
			      net2(this->_fmk, this->_log),
			      net3(this->_fmk, this->_log);
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Устанавливаем новое значение адреса для первого элемента
					net1 = this->v4();
					// Устанавливаем новое значение адреса для второго элемента
					net2 = begin.v4();
					// Устанавливаем новое значение адреса для третьего элемента
					net3 = end.v4();
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Устанавливаем новое значение адреса для первого элемента
					net1 = this->v6();
					// Устанавливаем новое значение адреса для второго элемента
					net2 = begin.v6();
					// Устанавливаем новое значение адреса для третьего элемента
					net3 = end.v6();
				} break;
			}
			// Извлекаем хост для первого элемента
			net1.impose(prefix, addr_t::HOST);
			// Извлекаем хост для второго элемента
			net2.impose(prefix, addr_t::HOST);
			// Извлекаем хост для третьего элемента
			net3.impose(prefix, addr_t::HOST);
			// Выполняем определение результата вхождения адреса в диапазон
			result = ((net1 >= net2) && (net1 <= net3));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const string & mask) const noexcept {
	// Выполняем проверку вхождения IP-адреса в диапазон адресов
	return this->range(begin, end, mask, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @param type  тип адреса аппаратного или интернет подключения
 * @return      результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const string & mask, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем проверку вхождения адреса в диапазон адресов
		if(prefix > 0)
			// Выполняем получение результата
			result = this->range(begin, end, prefix, type);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const uint8_t prefix) const noexcept {
	// Выполняем проверку вхождения IP-адреса в диапазон адресов
	return this->range(begin, end, prefix, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @param type   тип адреса аппаратного или интернет подключения
 * @return       результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const uint8_t prefix, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && !begin.empty() && !end.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объекты сетевых модулей
			net_t net1(this->_fmk, this->_log),
			      net2(this->_fmk, this->_log),
			      net3(this->_fmk, this->_log);
			// Устанавливаем новое значение адреса для начала и конца диапазона адресов
			net2 = begin; net3 = end;
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Устанавливаем новое значение адреса для первого элемента
					net1 = this->v4();
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6):
					// Устанавливаем новое значение адреса для первого элемента
					net1 = this->v6();
				break;
			}
			// Если типы адресов совпадают
			if((net1.type() == net2.type()) && (net1.type() == net3.type())){
				// Извлекаем хост для первого элемента
				net1.impose(prefix, addr_t::HOST);
				// Извлекаем хост для второго элемента
				net2.impose(prefix, addr_t::HOST);
				// Извлекаем хост для третьего элемента
				net3.impose(prefix, addr_t::HOST);
				// Выполняем определение результата вхождения адреса в диапазон
				result = ((net1 >= net2) && (net1 <= net3));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(begin, end, static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network) const noexcept {
	// Выполняем проверку соответствия IP-адреса указанной сети
	return this->mapping(network, this->_type);
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param type    тип адреса аппаратного или интернет подключения
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = !network.empty())){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_fmk, this->_log);
			// Если парсинг адреса сети выполнен
			if((result = net.parse(network))){
				// Если сеть и IP-адрес принадлежат одной версии сети
				if((result = (type == net.type()))){
					/**
					 * Определяем тип IP-адреса
					 */
					switch(static_cast <uint8_t> (type)){
						// Если IP-адрес определён как IPv4
						case static_cast <uint8_t> (type_t::IPV4): {
							// Буфер данных текущего адреса
							std::array <uint8_t, 4> nwk, addr;
							// Получаем значение адреса сети
							const uint32_t ip1 = net.v4();
							// Получаем значение текущего адреса
							const uint32_t ip2 = this->v4();
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&nwk[0], &ip1, sizeof(ip1));
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&addr[0], &ip2, sizeof(ip2));
							// Выполняем сравнение двух массивов
							for(uint8_t i = 0; i < 4; i++){
								// Если октет адреса соответствует октету сети
								result = ((addr[i] == nwk[i]) || (nwk[i] == 0));
								// Если проверка не вышла
								if(!result)
									// Выходим из цикла
									break;
							}
						} break;
						// Если IP-адрес определён как IPv6
						case static_cast <uint8_t> (type_t::IPV6): {
							// Буфер данных текущего адреса
							std::array <uint16_t, 8> nwk, addr;
							// Получаем значение адреса сети
							const std::array <uint8_t, 16> & ip1 = net.v6();
							// Получаем значение текущего адреса
							const std::array <uint8_t, 16> & ip2 = this->v6();
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&nwk[0], &ip1[0], sizeof(ip1));
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&addr[0], &ip2[0], sizeof(ip2));
							// Выполняем сравнение двух массивов
							for(uint8_t i = 0; i < 8; i++){
								// Если хексет адреса соответствует хексет сети
								result = ((addr[i] == nwk[i]) || (nwk[i] == 0));
								// Если проверка не вышла
								if(!result)
									// Выходим из цикла
									break;
							}
						} break;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(network, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param mask    маска сети для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const string & mask, const addr_t addr) const noexcept {
	// Выполняем проверку соответствия IP-адреса указанной сети
	return this->mapping(network, mask, addr, this->_type);
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param mask    маска сети для наложения
 * @param addr    тип получаемого адреса
 * @param type    тип адреса аппаратного или интернет подключения
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const string & mask, const addr_t addr, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = (!network.empty() && !mask.empty()))){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем проверку адреса соответствию сети
		if(prefix > 0)
			// Выполняем получение результата
			result = this->mapping(network, prefix, addr, type);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param prefix  префикс для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const uint8_t prefix, const addr_t addr) const noexcept {
	// Выполняем проверку соответствия IP-адреса указанной сети
	return this->mapping(network, prefix, addr, this->_type);
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param prefix  префикс для наложения
 * @param addr    тип получаемого адреса
 * @param type    тип адреса аппаратного или интернет подключения
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const uint8_t prefix, const addr_t addr, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = (!network.empty() && (prefix > 0)))){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_fmk, this->_log);
			// Если парсинг адреса сети выполнен
			if((result = net.parse(network))){
				// Если сеть и IP-адрес принадлежат одной версии сети
				if((result = (type == net.type()))){
					/**
					 * Определяем тип IP-адреса
					 */
					switch(static_cast <uint8_t> (type)){
						// Если IP-адрес определён как IPv4
						case static_cast <uint8_t> (type_t::IPV4): {
							// Копируем текущий IP-адрес
							net = this->v4();
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Получаем данные IPv4 текущего адреса
							const uint32_t ip = net.v4();
							// Устанавливаем данные сети
							net = network;
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Выполняем получение данных IPv4 сетевого адреса
							const uint32_t nwk = net.v4();
							// Выводим результат проверки
							return (ip == nwk);
						} break;
						// Если IP-адрес определён как IPv6
						case static_cast <uint8_t> (type_t::IPV6): {
							// Копируем текущий IP-адрес
							net = this->v6();
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Получаем данные IPv6 текущего адреса
							const auto & ip = net.v6();
							// Устанавливаем данные сети
							net = network;
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Выполняем получение данных IPv6 сетевого адреса
							const auto & nwk = net.v6();
							// Выводим результат проверки
							return (::memcmp(&ip[0], &nwk[0], sizeof(ip)) == 0);
						} break;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(network, prefix, static_cast <uint16_t> (addr), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод определения режима дислокации IP-адреса
 *
 * @return режим дислокации
 */
awh::Net::mode_t awh::Net::mode() const noexcept {
	// Результат работы функции
	mode_t result = mode_t::NONE;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_fmk, this->_log);
			// Выполняем инициализацию списка локальных адресов
			const_cast <net_t *> (this)->initLocalNet();
			// Выполняем группировку нужного нам вида адресов
			auto ret = this->_localsNet.equal_range(this->_type);
			// Перебираем все локальные адреса
			for(auto i = ret.first; i != ret.second; ++i){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_type)){
					// Если IP-адрес определён как IPv4
					case static_cast <uint8_t> (type_t::IPV4): {
						// Устанавливаем IP-адрес
						net = this->v4();
						// Если получен диапазон IP-адресов
						if(i->second.end->type() == type_t::IPV4){
							// Если адрес входит в диапазон адресов
							if((net >= (* i->second.begin.get())) && (net <= * (i->second.end.get()))){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						// Если диапазон адресов для этой проверки не установлен
						} else {
							// Устанавливаем префикс сети
							net.impose(i->second.prefix, net_t::addr_t::NETWORK);
							// Если проверяемые сети совпадают
							if(net.v4() == i->second.begin->v4()){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						}
					} break;
					// Если IP-адрес определён как IPv6
					case static_cast <uint8_t> (type_t::IPV6): {
						// Устанавливаем IP-адрес
						net = this->v6();
						// Если получен диапазон IP-адресов
						if(i->second.end->type() == type_t::IPV6){
							// Если адрес входит в диапазон адресов
							if((net >= (* i->second.begin.get())) && (net <= (* i->second.end.get()))){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						// Если диапазон адресов для этой проверки не установлен
						} else {
							// Устанавливаем префикс сети
							net.impose(i->second.prefix, net_t::addr_t::NETWORK);
							// Если проверяемые сети совпадают
							if(::memcmp(&net.v6()[0], &i->second.begin->v6()[0], 16) == 0){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						}
					} break;
				}
			}
			// Если результат не определён
			if(result == mode_t::NONE)
				// Устанавливаем, что файл ялвяется глобальным
				result = mode_t::WAN;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Получение записи в формате ARPA
 *
 * @return запись в формате ARPA
 */
string awh::Net::arpa() const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если IP-адрес определён как IPv4
			case static_cast <uint8_t> (type_t::IPV4): {
				// Переходим по всему массиву
				for(int8_t i = (static_cast <int8_t> (this->_buffer.size()) - 1); i > -1; i--){
					// Если строка уже существует, добавляем разделитель
					if(!result.empty())
						// Добавляем разделитель
						result.append(1, '.');
					// Добавляем текущий октет в результат
					result.append(std::to_string(this->_buffer[i]));
				}
				// Добавляем запись ARPA
				result.append(".in-addr.arpa");
			} break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6): {
				// Значение хексета
				uint16_t num = 0;
				// Переходим по всему массиву
				for(uint8_t i = 0; i < static_cast <uint8_t> (this->_buffer.size()); i += 2){
					// Выполняем получение значение числа
					::memcpy(&num, &this->_buffer[0] + i, sizeof(num));
					// Если строка уже существует, добавляем разделитель
					if(!result.empty())
						// Добавляем разделитель
						result.insert(result.begin(), '.');
					// Выполняем перебор полученного хексета
					for(auto & item : this->zerro(this->_fmk->itoa <uint16_t> (htons(num), 16), 4)){
						// Если последний символ не является точкой
						if(!result.empty() && (result.front() != '.'))
							// Добавляем разделитель
							result.insert(result.begin(), '.');
						// Добавляем хексет в версию
						result.insert(result.begin(), tolower(item));
					}
				}
				// Добавляем запись ARPA
				result.append(".ip6.arpa");
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки записи в формате ARPA
 *
 * @param addr адрес в формате ARPA (1.0.168.192.in-addr.arpa)
 * @return     результат установки записи
 */
bool awh::Net::arpa(const string & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если запись передана
	if(!addr.empty() && (addr.length() > 13)){
		// Если адрес является адресом IPv4
		if((result = (addr.substr(addr.length() - 13).compare(".in-addr.arpa") == 0))){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку буфера данных
				this->_buffer.clear();
				// Выполняем инициализацию буфера
				this->_buffer.resize(4);
				// Устанавливаем тип адреса
				this->_type = type_t::IPV4;
				// Позиция разделителя
				size_t start = 0, stop = 0, index = 3;
				// Получаем адрес для парсинга
				const string ip = addr.substr(0, addr.length() - 13);
				/**
				 * Выполняем поиск разделителя
				 */
				while((stop = ip.find('.', start)) != string::npos){
					// Извлекаем полученное число
					this->_buffer[index] = this->_fmk->atoi <uint8_t> (ip.c_str() + start, stop - start);
					// Выполняем смещение
					start = (stop + 1);
					// Уменьшаем смещение индекса
					index--;
				}
				// Выполняем установку последнего октета
				this->_buffer[index] = this->_fmk->atoi <uint8_t> (ip.c_str() + start, ip.length() - start);
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		// Если адрес является адресом IPv6
		} else if((result = (addr.substr(addr.length() - 9).compare(".ip6.arpa") == 0))) {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				/**
				 * @brief Структура бинарного буфера
				 *
				 */
				struct Buffer {
					// Временный буфер хексета
					uint8_t hexset[4];
					// Результирующий буфер данных
					uint16_t address[8];
					/**
					 * @brief Конструктор
					 *
					 */
					Buffer() noexcept : hexset{0,0,0,0}, address{0,0,0,0,0,0,0,0} {}
				} __attribute__((packed)) buffer;
				// Выполняем очистку буфера данных
				this->_buffer.clear();
				// Выполняем инициализацию буфера
				this->_buffer.resize(16);
				// Устанавливаем тип адреса
				this->_type = type_t::IPV6;
				// Позиция разделителя
				size_t start = 0, stop = 0;
				// Устанавливаем индекс последнего элемента
				uint8_t index1 = 4, index2 = 8;
				// Получаем адрес для парсинга
				const string & ip = addr.substr(0, addr.length() - 9);
				/**
				 * Выполняем поиск разделителя
				 */
				while((stop = ip.find('.', start)) != string::npos){
					// Выполняем установку хексета
					buffer.hexset[--index1] = static_cast <uint8_t> (ip[start]);
					// Если хексет полностью заполнен
					if(index1 == 0){
						// Добавляем хексет в список
						buffer.address[--index2] = ntohs(this->_fmk->atoi <uint16_t> (reinterpret_cast <const char *> (buffer.hexset), 4, static_cast <uint8_t> (16)));
						// Выполняем сброс индекса
						index1 = 4;
					}
					// Выполняем смещение
					start = (stop + 1);
				}
				// Выполняем установку хексета
				buffer.hexset[--index1] = static_cast <uint8_t> (ip[start]);
				// Если хексет полностью заполнен
				if(index1 == 0)
					// Добавляем хексет в список
					buffer.address[--index2] = ntohs(this->_fmk->atoi <uint16_t> (reinterpret_cast <const char *> (buffer.hexset), 4, static_cast <uint8_t> (16)));
				// Выполняем копирование бинарных данных в буфер
				::memcpy(&this->_buffer[0], buffer.address, sizeof(buffer.address));
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод парсинга адреса
 *
 * @param addr адрес аппаратный или интернет подключения для парсинга
 * @return     результат работы парсинга
 */
bool awh::Net::parse(const string & addr) noexcept {
	// Если адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем полную проверку всех типов адресов
			for(uint8_t i = 0; i < 3; i++){
				/**
				 * Устанавливаем тип для проверки
				 */
				switch(i){
					// Если проверяем IPv4-адрес
					case 0: {
						// Выполняем очистку буфера данных
						this->_buffer.clear();
						// Выполняем инициализацию буфера
						this->_buffer.resize(4);
						// Выполняем парсинг IPv4 адреса
						if(::ipv4(addr, this->_buffer)){
							// Устанавливаем тип адреса
							this->_type = type_t::IPV4;
							// Выводрим положительный результат
							return true;
						}
					} break;
					// Если проверяем IPv6-адрес
					case 1: {
						// Выполняем очистку зоны
						this->_zone.clear();
						// Выполняем очистку буфера данных
						this->_buffer.clear();
						// Выполняем инициализацию буфера
						this->_buffer.resize(16);
						// Выполняем парсинг IPv6 адреса
						if(::ipv6(addr, this->_buffer, this->_zone)){
							// Устанавливаем тип адреса
							this->_type = type_t::IPV6;
							// Выводрим положительный результат
							return true;
						}
					} break;
					// Если проверяем MAC-адрес
					case 2: {
						// Значение последнего символа
						int32_t last = -1;
						// Выполняем очистку буфера данных
						this->_buffer.clear();
						// Выполняем инициализацию буфера
						this->_buffer.resize(6);
						// Выполняем парсинг MAC адреса
						const int32_t rc = ::sscanf(
							addr.c_str(),
							"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
							&this->_buffer[0] + 0, &this->_buffer[0] + 1, &this->_buffer[0] + 2,
							&this->_buffer[0] + 3, &this->_buffer[0] + 4, &this->_buffer[0] + 5, &last
						);
						// Если MAC адрес удано распарсен
						if((rc == 6) && (static_cast <int32_t> (addr.size()) == last)){
							// Устанавливаем тип адреса
							this->_type = type_t::MAC;
							// Выводрим положительный результат
							return true;
						// Иначе выполняем очистку буфера данных
						} else this->_buffer.clear();
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод парсинга адреса
 *
 * @param addr адрес аппаратный или интернет подключения для парсинга
 * @param type тип адреса аппаратного или интернет подключения для парсинга
 * @return     результат работы парсинга
 */
bool awh::Net::parse(const string & addr, const type_t type) noexcept {
	// Если адрес аппаратный или интернет подключения передан
	if(!addr.empty() && ((type == type_t::MAC) || (type == type_t::IPV4) || (type == type_t::IPV6))){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип переданного адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес является адресом IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Выполняем инициализацию буфера
					this->_buffer.resize(4);
					// Выполняем парсинг IPv4 адреса
					if(::ipv4(addr, this->_buffer)){
						// Устанавливаем тип адреса
						this->_type = type;
						// Выводрим положительный результат
						return true;
					}
				} break;
				// Если IP-адрес является адресом IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Выполняем очистку зоны
					this->_zone.clear();
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Выполняем инициализацию буфера
					this->_buffer.resize(16);
					// Выполняем парсинг IPv6 адреса
					if(::ipv6(addr, this->_buffer, this->_zone)){
						// Устанавливаем тип адреса
						this->_type = type;
						// Выводрим положительный результат
						return true;
					}
				} break;
				// Если адрес является адресом MAC
				case static_cast <uint8_t> (type_t::MAC): {
					// Значение последнего символа
					int32_t last = -1;
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Выполняем инициализацию буфера
					this->_buffer.resize(6);
					// Выполняем парсинг MAC адреса
					const int32_t rc = ::sscanf(
						addr.c_str(),
						"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
						&this->_buffer[0] + 0, &this->_buffer[0] + 1, &this->_buffer[0] + 2,
						&this->_buffer[0] + 3, &this->_buffer[0] + 4, &this->_buffer[0] + 5, &last
					);
					// Если MAC адрес удано распарсен
					if((rc == 6) && (static_cast <int32_t> (addr.size()) == last)){
						// Устанавливаем тип адреса
						this->_type = type;
						// Выводрим положительный результат
						return true;
					// Иначе выполняем очистку буфера данных
					} else this->_buffer.clear();
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return false;
}
/**
 * @brief Метод извлечения данных IP-адреса
 *
 * @param size  размер формата формирования IP-адреса
 * @param flag  флаг форматирования IP-адреса
 * @param delim разделитель формата формирования IP-адреса
 * @return      сформированная строка IP-адреса
 */
string awh::Net::print(const format_size_t size, const format_flag_t flag, const char delim) const noexcept {
	// Результат работы функции
	string result = "";
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если - это не IP-адрес, а MAC-адрес
				case static_cast <uint8_t> (type_t::MAC): {
					// Если размера данных достаточно
					if(this->_buffer.size() >= 6){
						// Если разделитель установлен по умолчанию
						if(delim == -1)
							// Устанавливаем стандартный разделитель
							const_cast <char &> (delim) = ':';
						// Определяем размер формата вывода IPv4 адреса
						switch(static_cast <uint8_t> (size)){
							// Если размер не указан
							case static_cast <uint8_t> (format_size_t::NONE):
							// Если размер указан как средний
							case static_cast <uint8_t> (format_size_t::MIDDLE): {
								/**
								 * Определяем формат вывода MAC адреса
								 */
								switch(static_cast <uint8_t> (flag)){
									// Если формат не указан
									case static_cast <uint8_t> (format_flag_t::NONE):
									// Если формат указан в 16-м виде
									case static_cast <uint8_t> (format_flag_t::HEX): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(12);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%02X%02X%02X%02X%02X%02X",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(17);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
									// Если указан формат в десятичном виде
									case static_cast <uint8_t> (format_flag_t::DECIMAL): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(18);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%02u%02u%02u%02u%02u%02u",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(23);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%02u%c%02u%c%02u%c%02u%c%02u%c%02u",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
									// Если указан формат в восьмеричном виде
									case static_cast <uint8_t> (format_flag_t::OCTAL): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(18);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%02o%02o%02o%02o%02o%02o",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(23);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%02o%c%02o%c%02o%c%02o%c%02o%c%02o",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
								}
							} break;
							// Если размер указан как короткий
							case static_cast <uint8_t> (format_size_t::SHORT): {
								/**
								 * Определяем формат вывода MAC адреса
								 */
								switch(static_cast <uint8_t> (flag)){
									// Если формат не указан
									case static_cast <uint8_t> (format_flag_t::NONE):
									// Если формат указан в 16-м виде
									case static_cast <uint8_t> (format_flag_t::HEX): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(12);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%X%X%X%X%X%X",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(17);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%X%c%X%c%X%c%X%c%X%c%X",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
									// Если указан формат в десятичном виде
									case static_cast <uint8_t> (format_flag_t::DECIMAL): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(18);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%u%u%u%u%u%u",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(23);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%u%c%u%c%u%c%u%c%u%c%u",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
									// Если указан формат в восьмеричном виде
									case static_cast <uint8_t> (format_flag_t::OCTAL): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(18);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%o%o%o%o%o%o",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(23);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%o%c%o%c%o%c%o%c%o%c%o",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
								}
							} break;
							// Если размер указан как длинный
							case static_cast <uint8_t> (format_size_t::LONG): {
								/**
								 * Определяем формат вывода MAC адреса
								 */
								switch(static_cast <uint8_t> (flag)){
									// Если формат не указан
									case static_cast <uint8_t> (format_flag_t::NONE):
									// Если формат указан в 16-м виде
									case static_cast <uint8_t> (format_flag_t::HEX): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(18);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%03X%03X%03X%03X%03X%03X",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(23);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%03X%c%03X%c%03X%c%03X%c%03X%c%03X",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
									// Если указан формат в десятичном виде
									case static_cast <uint8_t> (format_flag_t::DECIMAL): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(18);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%03u%03u%03u%03u%03u%03u",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(23);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%03u%c%03u%c%03u%c%03u%c%03u%c%03u",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
									// Если указан формат в восьмеричном виде
									case static_cast <uint8_t> (format_flag_t::OCTAL): {
										// Если разделитель не указан
										if(delim == 0){
											// Перераспределяем объект результата
											result.resize(18);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%03o%03o%03o%03o%03o%03o",
												this->_buffer[0], this->_buffer[1],
												this->_buffer[2], this->_buffer[3],
												this->_buffer[4], this->_buffer[5]
											);
										// Если разделитель указан
										} else {
											// Перераспределяем объект результата
											result.resize(23);
											// Выполняем получение MAC адреса
											::sprintf(
												&result[0],
												"%03o%c%03o%c%03o%c%03o%c%03o%c%03o",
												this->_buffer[0], delim, this->_buffer[1], delim,
												this->_buffer[2], delim, this->_buffer[3], delim,
												this->_buffer[4], delim, this->_buffer[5]
											);
										}
									} break;
								}
							} break;
						}
					}
				} break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Если разделитель установлен по умолчанию
					if(delim == -1)
						// Устанавливаем стандартный разделитель
						const_cast <char &> (delim) = '.';
					// Определяем размер формата вывода IPv4 адреса
					switch(static_cast <uint8_t> (size)){
						// Если размер не указан
						case static_cast <uint8_t> (format_size_t::NONE):
						// Если размер указан как короткий
						case static_cast <uint8_t> (format_size_t::SHORT): {
							/**
							 * Определяем формат вывода IP-адреса
							 */
							switch(static_cast <uint8_t> (flag)){
								// Если формат не указан
								case static_cast <uint8_t> (format_flag_t::NONE):
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										::sprintf(
											&result[0],
											"%u%u%u%u",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(15);
										// Выполняем получение IPv4 адреса в 10-м формате
										::sprintf(
											&result[0],
											"%u%c%u%c%u%c%u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(8);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%X%X%X%X",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(11);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%X%c%X%c%X%c%X",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 8-м формате
										::sprintf(
											&result[0],
											"%o%o%o%o",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(15);
										// Выполняем получение IPv4 адреса в 8-м формате
										::sprintf(
											&result[0],
											"%o%c%o%c%o%c%o",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
							}
						} break;
						// Если размер указан как средний
						case static_cast <uint8_t> (format_size_t::MIDDLE): {
							/**
							 * Определяем формат вывода IP-адреса
							 */
							switch(static_cast <uint8_t> (flag)){
								// Если формат не указан
								case static_cast <uint8_t> (format_flag_t::NONE):
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										::sprintf(
											&result[0],
											"%02u%02u%02u%02u",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(15);
										// Выполняем получение IPv4 адреса в 10-м формате
										::sprintf(
											&result[0],
											"%02u%c%02u%c%02u%c%02u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(8);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%02X%02X%02X%02X",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(11);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%02X%c%02X%c%02X%c%02X",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 8-м формате
										::sprintf(
											&result[0],
											"%02o%02o%02o%02o",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(15);
										// Выполняем получение IPv4 адреса в 8-м формате
										::sprintf(
											&result[0],
											"%02o%c%02o%c%02o%c%02o",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
							}
						} break;
						// Если размер указан как длинный
						case static_cast <uint8_t> (format_size_t::LONG): {
							/**
							 * Определяем формат вывода IP-адреса
							 */
							switch(static_cast <uint8_t> (flag)){
								// Если формат не указан
								case static_cast <uint8_t> (format_flag_t::NONE):
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										::sprintf(
											&result[0],
											"%03u%03u%03u%03u",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(15);
										// Выполняем получение IPv4 адреса в 10-м формате
										::sprintf(
											&result[0],
											"%03u%c%03u%c%03u%c%03u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%03X%03X%03X%03X",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(15);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%03X%c%03X%c%03X%c%03X",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 8-м формате
										::sprintf(
											&result[0],
											"%03o%03o%03o%03o",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(15);
										// Выполняем получение IPv4 адреса в 8-м формате
										::sprintf(
											&result[0],
											"%03o%c%03o%c%03o%c%03o",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
							}
						} break;
					}
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Если разделитель установлен по умолчанию
					if(delim == -1)
						// Устанавливаем стандартный разделитель
						const_cast <char &> (delim) = ':';
					// Определяем размер формата вывода IPv4 адреса
					switch(static_cast <uint8_t> (size)){
						// Если размер не указан
						case static_cast <uint8_t> (format_size_t::NONE):
						// Если размер указан как короткий
						case static_cast <uint8_t> (format_size_t::SHORT): {
							/**
							 * Определяем формат вывода IP-адреса
							 */
							switch(static_cast <uint8_t> (flag)){
								// Если формат не указан
								case static_cast <uint8_t> (format_flag_t::NONE):
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX):
								// Если формат зеркального вещания IPv6 => IPv4 активен
								case static_cast <uint8_t> (format_flag_t::BROADCAST): {


									result.resize(64);

								
									int pos = 0;

									// 1. Собираем хексеты
									uint16_t h[8];
									for (int i = 0; i < 8; ++i) {
										h[i] = (static_cast<uint16_t>(this->_buffer[i * 2]) << 8) | this->_buffer[i * 2 + 1];
									}

									// 2. Проверка IPv4-встраивания (опционально)
									bool use_ipv4 = false;
									if (flag == format_flag_t::BROADCAST) {
										if ((h[0] == 0 && h[1] == 0 && h[2] == 0 && h[3] == 0 && h[4] == 0 && h[5] == 0xFFFF) ||
											(h[0] == 0 && h[1] == 0 && h[2] == 0 && h[3] == 0 && h[4] == 0 && h[5] == 0)) {
											use_ipv4 = true;
										}
									}

									if (use_ipv4) {
										// Простой вывод для IPv4-суффикса
										if (h[0] == 0 && h[1] == 0 && h[2] == 0 && h[3] == 0 && h[4] == 0) {
											if (h[5] == 0xFFFF) {
												pos = sprintf(&result[0], "%c%cFFFF%c%u.%u.%u.%u", delim, delim, delim, this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]);
											} else {
												pos = sprintf(&result[0], "%c%c%u.%u.%u.%u", delim, delim, this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]);
											}
										} else {
											// fallback (маловероятно)
											pos = sprintf(&result[0], "%X%c%X%c%X%c%X%c%X%c%X%c%u.%u.%u.%u",
												h[0], delim, h[1], delim, h[2], delim, h[3], delim, h[4], delim, h[5], delim, this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]);
										}
									} else {

										// 2. Находим лучшее сжатие (минимум 2 нуля)
										int best_start = -1, best_len = 1;
										for (int i = 0; i < 8; ) {
											if (h[i] == 0) {
												int j = i;
												while (j < 8 && h[j] == 0) j++;
												if (j - i > best_len) {
													best_len = j - i;
													best_start = i;
												}
												i = j;
											} else {
												i++;
											}
										}

										// 3. Формируем строку — ЕДИНСТВЕННЫЙ ПРАВИЛЬНЫЙ СПОСОБ
										if (best_len <= 1) {
											// Без сжатия
											pos = sprintf(&result[0], "%X%c%X%c%X%c%X%c%X%c%X%c%X%c%X",
												h[0], delim, h[1], delim, h[2], delim, h[3], delim, h[4], delim, h[5], delim, h[6], delim, h[7]);
										} else {
											// С сжатием — выводим ТОЛЬКО то, что нужно
											if (best_start == 0) {
												// ::xxx
												pos = sprintf(&result[0], "%c%c", delim, delim);
												for (int i = best_len; i < 8; ++i) {
													pos += sprintf(&result[0] + pos, "%c%X", (i == best_len) ? 0 : delim, h[i]);
												}
											} else if (best_start + best_len == 8) {
												// xxx::
												for (int i = 0; i < best_start; ++i) {
													pos += sprintf(&result[0] + pos, "%c%X", (i == 0) ? 0 : delim, h[i]);
												}
												pos += sprintf(&result[0] + pos, "%c%c", delim, delim);
											} else {
												// xxx::xxx
												for (int i = 0; i < best_start; ++i) {
													pos += sprintf(&result[0] + pos, "%c%X", (i == 0) ? 0 : delim, h[i]);
												}
												pos += sprintf(&result[0] + pos, "%c%c", delim, delim);
												for (int i = best_start + best_len; i < 8; ++i) {
													pos += sprintf(&result[0] + pos, "%X%c", h[i], (i == 7) ? 0 : delim);
												}
											}
										}
									}

									result.resize(pos);

									

								} break;
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {

									result.resize(64);

								
									int pos = 0;

									// 1. Собираем хексеты
									uint16_t h[8];
									for (int i = 0; i < 8; ++i) {
										h[i] = (static_cast<uint16_t>(this->_buffer[i * 2]) << 8) | this->_buffer[i * 2 + 1];
									}

									// 2. Находим лучшее сжатие (минимум 2 нуля)
									int best_start = -1, best_len = 1;
									for (int i = 0; i < 8; ) {
										if (h[i] == 0) {
											int j = i;
											while (j < 8 && h[j] == 0) j++;
											if (j - i > best_len) {
												best_len = j - i;
												best_start = i;
											}
											i = j;
										} else {
											i++;
										}
									}

									// 3. Формируем строку — ЕДИНСТВЕННЫЙ ПРАВИЛЬНЫЙ СПОСОБ
									if (best_len <= 1) {
										// Без сжатия
										pos = sprintf(&result[0], "%u%c%u%c%u%c%u%c%u%c%u%c%u%c%u",
											h[0], delim, h[1], delim, h[2], delim, h[3], delim, h[4], delim, h[5], delim, h[6], delim, h[7]);
									} else {
										// С сжатием — выводим ТОЛЬКО то, что нужно
										if (best_start == 0) {
											// ::xxx
											pos = sprintf(&result[0], "%c%c", delim, delim);
											for (int i = best_len; i < 8; ++i) {
												pos += sprintf(&result[0] + pos, "%c%u", (i == best_len) ? 0 : delim, h[i]);
											}
										} else if (best_start + best_len == 8) {
											// xxx::
											for (int i = 0; i < best_start; ++i) {
												pos += sprintf(&result[0] + pos, "%c%u", (i == 0) ? 0 : delim, h[i]);
											}
											pos += sprintf(&result[0] + pos, "%c%c", delim, delim);
										} else {
											// xxx::xxx
											for (int i = 0; i < best_start; ++i) {
												pos += sprintf(&result[0] + pos, "%c%u", (i == 0) ? 0 : delim, h[i]);
											}
											pos += sprintf(&result[0] + pos, "%c%c", delim, delim);
											for (int i = best_start + best_len; i < 8; ++i) {
												pos += sprintf(&result[0] + pos, "%u%c", h[i], (i == 7) ? 0 : delim);
											}
										}
									}
									

									result.resize(pos);

								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {

									result.resize(64);

								
									int pos = 0;

									// 1. Собираем хексеты
									uint16_t h[8];
									for (int i = 0; i < 8; ++i) {
										h[i] = (static_cast<uint16_t>(this->_buffer[i * 2]) << 8) | this->_buffer[i * 2 + 1];
									}

									// 2. Находим лучшее сжатие (минимум 2 нуля)
									int best_start = -1, best_len = 1;
									for (int i = 0; i < 8; ) {
										if (h[i] == 0) {
											int j = i;
											while (j < 8 && h[j] == 0) j++;
											if (j - i > best_len) {
												best_len = j - i;
												best_start = i;
											}
											i = j;
										} else {
											i++;
										}
									}

									// 3. Формируем строку — ЕДИНСТВЕННЫЙ ПРАВИЛЬНЫЙ СПОСОБ
									if (best_len <= 1) {
										// Без сжатия
										pos = sprintf(&result[0], "%o%c%o%c%o%c%o%c%o%c%o%c%o%c%o",
											h[0], delim, h[1], delim, h[2], delim, h[3], delim, h[4], delim, h[5], delim, h[6], delim, h[7]);
									} else {
										// С сжатием — выводим ТОЛЬКО то, что нужно
										if (best_start == 0) {
											// ::xxx
											pos = sprintf(&result[0], "%c%c", delim, delim);
											for (int i = best_len; i < 8; ++i) {
												pos += sprintf(&result[0] + pos, "%c%o", (i == best_len) ? 0 : delim, h[i]);
											}
										} else if (best_start + best_len == 8) {
											// xxx::
											for (int i = 0; i < best_start; ++i) {
												pos += sprintf(&result[0] + pos, "%c%o", (i == 0) ? 0 : delim, h[i]);
											}
											pos += sprintf(&result[0] + pos, "%c%c", delim, delim);
										} else {
											// xxx::xxx
											for (int i = 0; i < best_start; ++i) {
												pos += sprintf(&result[0] + pos, "%c%o", (i == 0) ? 0 : delim, h[i]);
											}
											pos += sprintf(&result[0] + pos, "%c%c", delim, delim);
											for (int i = best_start + best_len; i < 8; ++i) {
												pos += sprintf(&result[0] + pos, "%o%c", h[i], (i == 7) ? 0 : delim);
											}
										}
									}
									

									result.resize(pos);

								} break;
							}
						} break;
						// Если размер указан как средний
						case static_cast <uint8_t> (format_size_t::MIDDLE): {
							/**
							 * Определяем формат вывода IP-адреса
							 */
							switch(static_cast <uint8_t> (flag)){
								// Если формат не указан
								case static_cast <uint8_t> (format_flag_t::NONE):
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(32);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%X%X%X%X%X%X%X%X",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3],
											(this->_buffer[4] << 8) | this->_buffer[5],
											(this->_buffer[6] << 8) | this->_buffer[7],
											(this->_buffer[8] << 8) | this->_buffer[9],
											(this->_buffer[10] << 8) | this->_buffer[11],
											(this->_buffer[12] << 8) | this->_buffer[13],
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(39);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%X%c%X%c%X%c%X%c%X%c%X%c%X%c%X",
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3], delim,
											(this->_buffer[4] << 8) | this->_buffer[5], delim,
											(this->_buffer[6] << 8) | this->_buffer[7], delim,
											(this->_buffer[8] << 8) | this->_buffer[9], delim,
											(this->_buffer[10] << 8) | this->_buffer[11], delim,
											(this->_buffer[12] << 8) | this->_buffer[13], delim,
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									}
								} break;
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(40);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%u%u%u%u%u%u%u%u",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3],
											(this->_buffer[4] << 8) | this->_buffer[5],
											(this->_buffer[6] << 8) | this->_buffer[7],
											(this->_buffer[8] << 8) | this->_buffer[9],
											(this->_buffer[10] << 8) | this->_buffer[11],
											(this->_buffer[12] << 8) | this->_buffer[13],
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(47);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%u%c%u%c%u%c%u%c%u%c%u%c%u%c%u",
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3], delim,
											(this->_buffer[4] << 8) | this->_buffer[5], delim,
											(this->_buffer[6] << 8) | this->_buffer[7], delim,
											(this->_buffer[8] << 8) | this->_buffer[9], delim,
											(this->_buffer[10] << 8) | this->_buffer[11], delim,
											(this->_buffer[12] << 8) | this->_buffer[13], delim,
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(48);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%o%o%o%o%o%o%o%o",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3],
											(this->_buffer[4] << 8) | this->_buffer[5],
											(this->_buffer[6] << 8) | this->_buffer[7],
											(this->_buffer[8] << 8) | this->_buffer[9],
											(this->_buffer[10] << 8) | this->_buffer[11],
											(this->_buffer[12] << 8) | this->_buffer[13],
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(55);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%o%c%o%c%o%c%o%c%o%c%o%c%o%c%o",
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3], delim,
											(this->_buffer[4] << 8) | this->_buffer[5], delim,
											(this->_buffer[6] << 8) | this->_buffer[7], delim,
											(this->_buffer[8] << 8) | this->_buffer[9], delim,
											(this->_buffer[10] << 8) | this->_buffer[11], delim,
											(this->_buffer[12] << 8) | this->_buffer[13], delim,
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									}
								} break;
							}
						} break;
						// Если размер указан как длинный
						case static_cast <uint8_t> (format_size_t::LONG): {
							/**
							 * Определяем формат вывода IP-адреса
							 */
							switch(static_cast <uint8_t> (flag)){
								// Если формат не указан
								case static_cast <uint8_t> (format_flag_t::NONE):
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(32);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3],
											this->_buffer[4], this->_buffer[5],
											this->_buffer[6], this->_buffer[7],
											this->_buffer[8], this->_buffer[9],
											this->_buffer[10], this->_buffer[11],
											this->_buffer[12], this->_buffer[13],
											this->_buffer[14], this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(39);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%02X%02X%c%02X%02X%c%02X%02X%c%02X%02X%c%02X%02X%c%02X%02X%c%02X%02X%c%02X%02X",
											this->_buffer[0], this->_buffer[1], delim,
											this->_buffer[2], this->_buffer[3], delim,
											this->_buffer[4], this->_buffer[5], delim,
											this->_buffer[6], this->_buffer[7], delim,
											this->_buffer[8], this->_buffer[9], delim,
											this->_buffer[10], this->_buffer[11], delim,
											this->_buffer[12], this->_buffer[13], delim,
											this->_buffer[14], this->_buffer[15]
										);
									}
								} break;
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(40);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%03u%03u%03u%03u%03u%03u%03u%03u",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3],
											(this->_buffer[4] << 8) | this->_buffer[5],
											(this->_buffer[6] << 8) | this->_buffer[7],
											(this->_buffer[8] << 8) | this->_buffer[9],
											(this->_buffer[10] << 8) | this->_buffer[11],
											(this->_buffer[12] << 8) | this->_buffer[13],
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(47);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%03u%c%03u%c%03u%c%03u%c%03u%c%03u%c%03u%c%03u",
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3], delim,
											(this->_buffer[4] << 8) | this->_buffer[5], delim,
											(this->_buffer[6] << 8) | this->_buffer[7], delim,
											(this->_buffer[8] << 8) | this->_buffer[9], delim,
											(this->_buffer[10] << 8) | this->_buffer[11], delim,
											(this->_buffer[12] << 8) | this->_buffer[13], delim,
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(48);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%03o%03o%03o%03o%03o%03o%03o%03o",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3],
											(this->_buffer[4] << 8) | this->_buffer[5],
											(this->_buffer[6] << 8) | this->_buffer[7],
											(this->_buffer[8] << 8) | this->_buffer[9],
											(this->_buffer[10] << 8) | this->_buffer[11],
											(this->_buffer[12] << 8) | this->_buffer[13],
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(55);
										// Выполняем получение IPv4 адреса в 16-м формате
										::sprintf(
											&result[0],
											"%03o%c%03o%c%03o%c%03o%c%03o%c%03o%c%03o%c%03o",
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3], delim,
											(this->_buffer[4] << 8) | this->_buffer[5], delim,
											(this->_buffer[6] << 8) | this->_buffer[7], delim,
											(this->_buffer[8] << 8) | this->_buffer[9], delim,
											(this->_buffer[10] << 8) | this->_buffer[11], delim,
											(this->_buffer[12] << 8) | this->_buffer[13], delim,
											(this->_buffer[14] << 8) | this->_buffer[15]
										);
									}
								} break;
							}
						} break;
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (size), static_cast <uint16_t> (flag)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Если результат не пустой и зона адреса определена
	if(!result.empty() && !this->_zone.empty())
		// Добавляем зону адреса к результату
		result += ('%' + this->_zone);
	// Выводим результат
	return result;
}
/**
 * @brief Оператор вывода IP-адреса в качестве строки
 *
 * @return IP-адрес в качестве строки
 */
awh::Net::operator string() const noexcept {
	// Выводим данные IP-адреса в виде строки
	return this->print();
}
/**
 * @brief Оператор [<] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator < (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC):
					// Выполняем сравнение адресов
					result = this->_fmk->isGreater(&addr.mac()[0], &this->mac()[0], 6);
				break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) < addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем бинарное сравнение
					result = this->_fmk->isGreater(&second[0], &first[0], 16);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [>] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator > (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC):
					// Выполняем сравнение адресов
					result = this->_fmk->isGreater(&this->mac()[0], &addr.mac()[0], 6);
				break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) > addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем бинарное сравнение
					result = this->_fmk->isGreater(&first[0], &second[0], 16);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [<=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator <= (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC): {
					// Получаем данные текущего адреса MAC
					const auto & first = this->mac();
					// Получаем данные сравниваемого адреса MAC
					const auto & second = addr.mac();
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 6) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&second[0], &first[0], 6);
				} break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) <= addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 16) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&second[0], &first[0], 16);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [>=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator >= (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC): {
					// Получаем данные текущего адреса MAC
					const auto & first = this->mac();
					// Получаем данные сравниваемого адреса MAC
					const auto & second = addr.mac();
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 6) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&first[0], &second[0], 6);
				} break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) >= addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 16) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&first[0], &second[0], 16);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [!=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator != (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если MAC-адрес определён
			case static_cast <uint8_t> (type_t::MAC):
				// Выполняем сравнение адресов
				result = (::memcmp(&this->mac()[0], &addr.mac()[0], 6) != 0);
			break;
			// Если IP-адрес определён как IPv4
			case static_cast <uint8_t> (type_t::IPV4):
				// Выполняем сравнение адресов
				result = (this->v4() != addr.v4());
			break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6):
				// Выполняем сравнение адресов
				result = (::memcmp(&this->v6()[0], &addr.v6()[0], 16) != 0);
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [==] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator == (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC):
					// Выполняем сравнение адресов
					result = (::memcmp(&this->mac()[0], &addr.mac()[0], 6) == 0);
				break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4() == addr.v4());
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6):
					// Выполняем сравнение адресов
					result = (::memcmp(&this->v6()[0], &addr.v6()[0], 16) == 0);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [=] присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const net_t & addr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип IP-адреса
		 */
		switch(static_cast <uint8_t> (addr.type())){
			// Если MAC-адрес определён
			case static_cast <uint8_t> (type_t::MAC):
				// Устанавливаем MAC-адресе
				this->mac(addr.mac());
			break;
			// Если IP-адрес определён как IPv4
			case static_cast <uint8_t> (type_t::IPV4):
				// Устанавливаем IPv4 адресе
				this->v4(addr.v4());
			break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6):
				// Устанавливаем IPv6 адресе
				this->v6(addr.v6());
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присвоения IP-адреса
 *
 * @param ip адрес для присвоения
 * @return   текущий объект
 */
awh::Net & awh::Net::operator = (const string & ip) noexcept {
	// Выполняем установку IP-адреса
	this->parse(ip);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] установки типа IP-адреса
 *
 * @param type тип IP-адреса для установки
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const type_t type) noexcept {
	// Устанавливаем тип IP-адреса
	this->type(type);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const uint32_t addr) noexcept {
	// Устанавливаем IPv4
	this->v4(addr);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присвоения MAC-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const std::array <uint8_t, 6> & addr) noexcept {
	// Устанавливаем MAC-адрес
	this->mac(addr);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const std::array <uint8_t, 16> & addr) noexcept {
	// Устанавливаем IPv4
	this->v6(addr);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Net::Net(const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _fmk(fmk), _log(log) {}
/**
 * @brief Оператор [>>] чтения из потока IP-адреса
 *
 * @param is   поток для чтения
 * @param addr адрес для присвоения
 */
istream & awh::operator >> (istream & is, net_t & addr) noexcept {
	// Адрес интернет-подключения
	string ip = "";
	// Считываем адрес интернет-подключения
	is >> ip;
	// Если адрес интернет-подключения получен
	if(!ip.empty())
		// Устанавливаем IP-адрес
		addr.parse(ip);
	// Выводим результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток IP-адреса
 *
 * @param os   поток куда нужно вывести данные
 * @param addr адрес для присвоения
 */
ostream & awh::operator << (ostream & os, const net_t & addr) noexcept {
	// Записываем в поток IP-адрес
	os << addr.print();
	// Выводим результат
	return os;
}
