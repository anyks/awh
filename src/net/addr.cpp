/**
 * @file: addr.cpp
 * @date: 2025-10-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля работы с сетевыми адресами — разбор, нормализация, сравнение и форматирование IPv4,
 *        IPv6 и MAC-адресов, вычисление префиксов и масок сети,
 *        определение типа адреса и его принадлежности зарезервированным диапазонам
 *
 * @copyright: Copyright © 2025
 *
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
	 *
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
 * Стандартные заголовочные файлы
 */
#include <bitset>
#include <limits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/addr.hpp>
#include <sys/ascii.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Сокращаем пространство имён проверок символов таблицы ASCII
 *
 * @details Проверки эти прежде были заведены здесь местными копиями. Нужны они
 *          не только адресам - имена заголовков HTTP, схемы URI и числа
 *          разбираются теми же проверками, - и держать копию в каждом модуле
 *          значило бы разойтись между ними при первой же правке.
 *
 */
namespace ascii = awh::ascii;

/**
 * @brief Инкапсулируем статические функции в пространство имён
 *
 */
namespace {
	/**
	 * @brief Вспомогательная функция для получения размера буфера
	 *
	 * @tparam T размер буфера данных
	 *
	 */
	template <size_t T = 4>
	/**
	 * @brief Вспомогательная функция конвертации IPv6-адреса в нужный порядок байт
	 *
	 * @param src исходный IPv6-адрес
	 * @param dst результирующий IPv6-адрес
	 *
	 */
	void convertEndian(const uint8_t src[T], uint8_t dst[T]) noexcept {
		/**
		 * Если порядок байт Little Endian
		 */
		#if IS_LITTLE_ENDIAN
			// На little-endian: переворачиваем все T байт
			::reverse_copy(src, src + T, dst);
		/**
		 * Если порядок байт Big Endian
		 */
		#else
			// На big-endian: ничего не делаем
			::memcpy(dst, src, T);
		#endif
	}
	/**
	 * @brief Вспомогательная функция для обрезки пробелов в строке
	 *
	 * @param text исходный текст для обрезки
	 * @return     обрезанный текст
	 *
	 */
	string_view trim(string_view text) noexcept {
		// Позиции начала и конца обрезанной строки
		size_t i = 0, j = text.size();
		/**
		 * Выполняем обрезку пробелов в начале и конце строки
		 */
		while((i < j) && ((text[i] == '\0') || ascii::isSpace(text[i])))
			// Увеличиваем позицию начала строки
			++i;
		/**
		 * Обрезаем пробелы в конце строки
		 */
		while((j > i) && ((text[j - 1] == '\0') || ascii::isSpace(text[j - 1])))
			// Уменьшаем позицию конца строки
			--j;
		// Возвращаем обрезанную строку
		return text.substr(i, j - i);
	}
	/**
	 * @brief Вспомогательная функция для проверки префикса строки
	 *
	 * @param text исходный текст для проверки
	 * @param pfx  префикс для проверки
	 * @return     результат проверки
	 *
	 */
	bool startWith(string_view text, string_view pfx) noexcept {
		// Возвращаем результат проверки
		return (
			(text.size() >= pfx.size()) &&
			::equal(pfx.begin(), pfx.end(), text.begin())
		);
	}
	/**
	 * @brief Вспомогательная функция для проверки суффикса строки
	 *
	 * @param text исходный текст для проверки
	 * @param sfx  суффикс для проверки
	 * @return     результат проверки
	 *
	 */
	bool endWith(string_view text, string_view sfx) noexcept {
		// Возвращаем результат проверки
		return (
			(text.size() >= sfx.size()) &&
			::equal(text.end() - sfx.size(), text.end(), sfx.begin())
		);
	}
	/**
	 * @brief Класс массива частей адреса постоянной ёмкости
	 *
	 * @details Разбиение адреса на части выполняется на каждое создание события
	 *          и на каждое принятое подключение, а динамический массив на этом
	 *          пути набирал ёмкость удвоением с нуля - три выделения памяти на
	 *          один разбор. Частей же у адреса не бывает больше восьми: четыре
	 *          октета у IPv4 и восемь хекстетов у IPv6. Ёмкость взята на одну
	 *          больше предельной, чтобы разбор отличал адрес с лишними частями
	 *          от допустимого, а не молча его обрезал.
	 *
	 */
	typedef class Parts {
		public:
			// Предельное количество частей адреса с запасом на признак превышения
			static constexpr size_t CAPACITY = 9;
		private:
			// Количество занятых частей адреса
			size_t _count;
			// Части адреса
			string_view _items[CAPACITY];
		public:
			/**
			 * @brief Метод очистки массива частей адреса
			 *
			 */
			void clear() noexcept {
				// Сбрасываем количество занятых частей адреса
				this->_count = 0;
			}
			/**
			 * @brief Метод получения количества частей адреса
			 *
			 * @return количество частей адреса
			 *
			 */
			size_t size() const noexcept {
				// Выводим количество занятых частей адреса
				return this->_count;
			}
			/**
			 * @brief Метод проверки заполненности массива частей адреса
			 *
			 * @return результат проверки
			 *
			 */
			bool full() const noexcept {
				// Массив заполнен, если заняты все части
				return (this->_count >= CAPACITY);
			}
		public:
			/**
			 * @brief Метод добавления части адреса
			 *
			 * @param item часть адреса
			 *
			 */
			void add(string_view item) noexcept {
				// Добавляем часть адреса, если для неё есть место
				if(this->_count < CAPACITY)
					// Запоминаем очередную часть адреса
					this->_items[this->_count++] = item;
			}
		public:
			/**
			 * @brief Оператор получения части адреса по индексу
			 *
			 * @param index индекс части адреса
			 * @return      часть адреса
			 *
			 */
			string_view operator [] (const size_t index) const noexcept {
				// Выводим часть адреса по индексу
				return this->_items[index];
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Parts() noexcept : _count(0) {}
	} parts_t;
	/**
	 * @brief Класс массива слов IPv6-адреса постоянной ёмкости
	 *
	 * @details Сборка IPv6-адреса из хекстетов заводила три динамических массива:
	 *          слова левой части, слова правой части и полный набор слов - три
	 *          выделения памяти на один разбор. Слов же у адреса ровно восемь, и
	 *          известно это до начала разбора. Ёмкость взята с запасом на два
	 *          слова: часть адреса со встроенным IPv4 даёт сразу пару слов, и
	 *          запас позволяет разбору отличить адрес с лишними словами от
	 *          допустимого, а не молча его обрезать.
	 *
	 */
	typedef class Words {
		public:
			// Предельное количество слов адреса с запасом на признак превышения
			static constexpr size_t CAPACITY = 10;
		private:
			// Количество занятых слов адреса
			size_t _count;
			// Слова адреса
			uint16_t _items[CAPACITY];
		public:
			/**
			 * @brief Метод очистки массива слов адреса
			 *
			 */
			void clear() noexcept {
				// Сбрасываем количество занятых слов адреса
				this->_count = 0;
			}
			/**
			 * @brief Метод получения количества слов адреса
			 *
			 * @return количество слов адреса
			 *
			 */
			size_t size() const noexcept {
				// Выводим количество занятых слов адреса
				return this->_count;
			}
			/**
			 * @brief Метод проверки заполненности массива слов адреса
			 *
			 * @return результат проверки
			 *
			 */
			bool full() const noexcept {
				// Массив заполнен, если заняты все слова
				return (this->_count >= CAPACITY);
			}
		public:
			/**
			 * @brief Метод добавления слова адреса
			 *
			 * @param item слово адреса
			 *
			 */
			void add(const uint16_t item) noexcept {
				// Добавляем слово адреса, если для него есть место
				if(this->_count < CAPACITY)
					// Запоминаем очередное слово адреса
					this->_items[this->_count++] = item;
			}
		public:
			/**
			 * @brief Оператор получения слова адреса по индексу
			 *
			 * @param index индекс слова адреса
			 * @return      слово адреса
			 *
			 */
			uint16_t operator [] (const size_t index) const noexcept {
				// Выводим слово адреса по индексу
				return this->_items[index];
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Words() noexcept : _count(0), _items{} {}
	} words_t;
	/**
	 * @brief Вспомогательная функция для разбиения строки по разделителю
	 *
	 * @param text   исходный текст для разбиения
	 * @param delim  разделитель для разбиения
	 * @param result результирующий массив частей строки
	 * @param max    максимальное количество частей строки
	 *
	 */
	void split(string_view text, const char delim, parts_t & result, const size_t max = parts_t::CAPACITY) noexcept {
		// Очищаем результирующий массив частей строки
		result.clear();
		// Позиция в строке, её размер и следующая позиция разделителя
		size_t pos = 0, next = 0, length = text.size();
		/**
		 * Пока не достигнут конец строки и не превышено максимальное количество частей строки
		 */
		while((pos <= length) && (result.size() < max) && !result.full()){
			// Ищем следующую позицию разделителя
			next = ((pos < length) ? text.find(delim, pos) : string_view::npos);
			// Если разделитель не найден
			if(next == string_view::npos)
				// Устанавливаем позицию конца строки
				next = length;
			// Добавляем часть строки в результирующий массив
			result.add(text.substr(pos, next - pos));
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
	 *
	 */
	uint8_t detectBase(string_view token, const bool allowNonDecimal) noexcept {
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
			if(::all_of(token.begin() + 1, token.end(), ascii::isOctal))
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
	 *
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
				if(!ascii::isDigit(letter))
					// Возвращаем ошибку
					return false;
				// Получаем числовое значение символа
				dig = (letter - '0');
			// Если основание восьмеричное
			} else if(base == 8) {
				// Проверяем восьмеричный символ
				if(!ascii::isOctal(letter))
					// Возвращаем ошибку
					return false;
				// Получаем числовое значение символа
				dig = (letter - '0');
			// Если основание шестнадцатеричное
			} else if(base == 16) {
				// Проверяем шестнадцатеричный символ
				dig = ascii::hexValue(letter);
				// Если символ не шестнадцатеричный
				if(dig < 0)
					// Возвращаем ошибку
					return false;
			// Иначе неверное основание системы счисления
			} else return false;
			// Проверка переполнения
			if(result > ((numeric_limits <uint64_t>::max() - static_cast <uint64_t> (dig)) / static_cast <uint64_t> (base)))
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
	 *
	 * @note Результат принимается указателем на буфер размером в четыре байта, а
	 *       не динамическим массивом: разбор пишет ровно четыре октета, и размер
	 *       буфера известен до его начала
	 *
	 */
	bool parseIPv4(string_view ip, uint8_t * result, const bool allowLegacy, const bool allowNonDecimal) noexcept {
		// Части строки IP-адреса
		parts_t octets;
		// Разбиваем строку IP-адреса на части
		::split(ip, '.', octets);
		// Если частей ровно 4
		if(octets.size() == 4){
			// Основание системы счисления
			uint8_t base = 0;
			// Значение результата парсинга
			uint64_t value = 0;
			/**
			 * Каждая часть в 0..255, при allowNonDecimal можно 0x/0..oct
			 */
			for(uint8_t i = 0; i < 4; ++i){
				// Текущая часть строки IP-адреса
				auto octet = octets[i];
				// Если часть пустая
				if(octet.empty())
					// Возвращаем ошибку
					return false;
				// Определяем основание системы счисления
				base = ::detectBase(octet, allowNonDecimal);
				// Сбрасываем значение результата парсинга
				value = 0;
				// Парсим часть строки IP-адреса
				if(!::parse64(octet, base, value))
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
			const uint8_t baseA = ::detectBase(octets[0], allowNonDecimal);
			const uint8_t baseB = ::detectBase(octets[1], allowNonDecimal);
			const uint8_t baseC = ::detectBase(octets[2], allowNonDecimal);
			// Парсим первый октет строки IP-адреса
			if(!::parse64(octets[0], baseA, a) || (a > 0xFF))
				// Возвращаем ошибку
				return false;
			// Парсим второй октет строки IP-адреса
			if(!::parse64(octets[1], baseB, b) || (b > 0xFF))
				// Возвращаем ошибку
				return false;
			// Парсим третий октет строки IP-адреса
			if(!::parse64(octets[2], baseC, c) || (c > 0xFFFF))
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
			const uint8_t baseA = ::detectBase(octets[0], allowNonDecimal);
			const uint8_t baseB = ::detectBase(octets[1], allowNonDecimal);
			// Парсим первый октет строки IP-адреса
			if(!::parse64(octets[0], baseA, a) || (a > 0xFF))
				// Возвращаем ошибку
				return false;
			// Парсим второй октет строки IP-адреса
			if(!::parse64(octets[1], baseB, b) || (b > 0xFFFFFF))
				// Возвращаем ошибку
				return false;
			// Формируем 32-битный адрес
			const uint32_t addr = ((static_cast <uint32_t> (a) << 24) | static_cast <uint32_t> (b));
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
			const uint8_t base = ::detectBase(octets[0], allowNonDecimal);
			// Парсим единственный октет строки IP-адреса
			if(!::parse64(octets[0], base, d) || (d > 0xFFFFFFFFull))
				// Возвращаем ошибку
				return false;
			// Формируем 32-битный адрес
			const uint32_t addr = static_cast <uint32_t> (d);
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
	 *
	 * @note Результат принимается массивом постоянного размера, а не динамическим:
	 *       октетов у десятичного квадрата ровно четыре, и заводить под них
	 *       выделение памяти незачем
	 *
	 */
	bool parseIPv4DecQuad(string_view ip, uint8_t (& result)[4]) noexcept {
		// Части строки IP-адреса
		parts_t octets;
		// Разбиваем строку IP-адреса на октеты
		::split(ip, '.', octets);
		// Проверяем количество октетов строки IP-адреса
		if(octets.size() != 4)
			// Возвращаем ошибку
			return false;
		// Значение результата парсинга
		uint64_t value = 0;
		/**
		 * Парсим каждый октет строки IP-адреса
		 */
		for(uint8_t i = 0; i < 4; ++i){
			// Текущий октет строки IP-адреса
			auto octet = octets[i];
			// Проверяем октет строки IP-адреса
			if(octet.empty() || ((octet.size() > 1) && (octet[0] == '+')))
				// Возвращаем ошибку
				return false;
			// Проверяем каждый символ октета строки IP-адреса
			if(!::all_of(octet.begin(), octet.end(), ascii::isDigit))
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
			if(!::parse64(octet, 10, value))
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
	 *
	 * @note Результат принимается указателем на буфер размером в шестнадцать байт,
	 *       а не динамическим массивом: разбор пишет ровно восемь слов, и размер
	 *       буфера известен до его начала
	 *
	 */
	bool parseIPv6(string_view ip, uint8_t * result, const bool allowEmbeddedV4) noexcept {
		// Специальный случай "::"
		if(ip.compare("::") == 0){
			// Зануляем результат
			::memset(result, 0, 16);
			// Возвращаем успешный результат парсинга
			return true;
		}
		// Найдём "::" (не более одного)
		size_t dcol = ip.find("::");
		// Проверяем, найден ли "::"
		const bool hasDcol = (dcol != string_view::npos);
		// Части левой и правой части адреса
		parts_t leftParts, rightParts;
		// Разбиваем адрес на левую и правую части
		if(hasDcol){
			// Получаем левую часть IP-адреса
			string_view left = ip.substr(0, dcol);
			// Получаем правую часть IP-адреса
			string_view right = ip.substr(dcol + 2);
			// Если левая часть не пуста
			if(!left.empty())
				// Разбиваем левую часть на хекстеты
				::split(left, ':', leftParts);
			// Если правая часть не пуста
			if(!right.empty())
				// Разбиваем правую часть на хекстеты
				::split(right, ':', rightParts);
		// Иначе разбиваем весь адрес на хекстеты
		} else ::split(ip, ':', leftParts);
		/**
		 * @brief Парсинг хекстета IPv6-адреса
		 *
		 * @param hextet строка с хекстетом
		 * @param result значение полученного хекстета
		 * @return       результат выполнения парсинга
		 *
		 */
		auto parseHextet = [](string_view hextet, uint16_t & result) noexcept -> bool {
			// Проверяем длину хекстета
			if(hextet.empty() || (hextet.size() > 4))
				// Возвращаем ошибку
				return false;
			// Парсим хекстет посимвольно
			uint32_t value = 0;
			/**
			 * Проходим по каждому символу хекстета
			 */
			for(char letter : hextet){
				// Проверяем шестнадцатеричный символ
				if(!ascii::isHex(letter))
					// Возвращаем ошибку
					return false;
				// Обновляем значение хекстета
				value = ((value << 4) | static_cast <uint32_t> (ascii::hexValue(letter)));
				// Проверяем переполнение хекстета
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
		 * @brief Парсинг IPv4-адреса в виде двух хекстетов
		 *
		 * @param ip      строка с IPv4-адресом
		 * @param hextet1 первый хекстет
		 * @param hextet2 второй хекстет
		 * @return        результат выполнения парсинга
		 *
		 */
		auto parseIPv4TailAsTwoHextets = [](string_view ip, uint16_t & hextet1, uint16_t & hextet2) noexcept -> bool {
			// Спарсеный IPv4-адрес
			uint8_t v4[4] = {0};
			// Парсим IPv4-адрес
			if(!::parseIPv4DecQuad(ip, v4))
				// Возвращаем ошибку
				return false;
			// Преобразуем первый хекстет
			hextet1 = static_cast <uint16_t> ((static_cast <uint16_t> (v4[0]) << 8) | v4[1]);
			// Преобразуем второй хекстет
			hextet2 = static_cast <uint16_t> ((static_cast <uint16_t> (v4[2]) << 8) | v4[3]);
			// Возвращаем успешный результат парсинга
			return true;
		};
		// Собранные слова IPv6-адреса
		words_t words;
		// Первый и второй хекстеты IPv4-адреса
		uint16_t hextet1 = 0, hextet2 = 0;
		/**
		 * Разбираем левую часть
		 */
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
				// Сбрасываем значение хекстетов
				hextet1 = 0, hextet2 = 0;
				// Парсим IPv4 как два хекстета
				if(!parseIPv4TailAsTwoHextets(part, hextet1, hextet2))
					// Возвращаем ошибку
					return false;
				// Добавляем хекстеты в IPv6-адрес
				words.add(hextet1);
				words.add(hextet2);
			// Иначе парсим обычный хекстет
			} else {
				// Сбрасываем значение хекстета
				hextet1 = 0;
				// Парсим хекстет
				if(!parseHextet(part, hextet1))
					// Возвращаем ошибку
					return false;
				// Добавляем хекстет в IPv6-адрес
				words.add(hextet1);
			}
		}
		// Количество слов в правой части
		uint8_t rightWordsCount = 0;
		// Собранные слова правой части IPv6-адреса
		words_t rightWords;
		// Разбираем правую часть при наличии
		if(hasDcol){
			/**
			 * Разобрать правую часть; если там IPv4 — только в самом последнем токене
			 */
			for(uint8_t i = 0; i < static_cast <uint8_t> (rightParts.size()); ++i){
				// Текущий токен правой части
				auto part = rightParts[i];
				// Проверяем пустой токен
				if(part.empty())
					// Пустой токен допустим только как часть "::"
					return false;
				// Проверяем встроенный IPv4-адрес
				if(allowEmbeddedV4 && (i == static_cast <uint8_t> (rightParts.size() - 1)) && (part.find('.') != string_view::npos)){
					// Сбрасываем значение хекстетов
					hextet1 = 0, hextet2 = 0;
					// Парсим IPv4 как два хекстета
					if(!parseIPv4TailAsTwoHextets(part, hextet1, hextet2))
						// Возвращаем ошибку
						return false;
					// Добавляем хекстеты в правую часть IPv6-адреса
					rightWords.add(hextet1);
					rightWords.add(hextet2);
				// Иначе парсим обычный хекстет
				} else {
					// Сбрасываем значение хекстета
					hextet1 = 0;
					// Парсим хекстет
					if(!parseHextet(part, hextet1))
						// Возвращаем ошибку
						return false;
					// Добавляем хекстет в правую часть IPv6-адреса
					rightWords.add(hextet1);
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
			// Позиция очередного слова полного адреса
			uint8_t index = 0;
			/**
			 * @brief Запись очередного слова полного адреса в результат парсинга
			 *
			 * @param word слово полного адреса
			 *
			 * @note Полный набор слов адреса прежде собирался отдельным массивом,
			 *       а тот заводил третье выделение памяти на разбор. Слова же
			 *       ложатся в результат по порядку - слева направо, - и
			 *       промежуточный набор им не нужен
			 *
			 */
			auto store = [&index, &result](const uint16_t word) noexcept -> void {
				/**
				 * Выполняем формирование результата парсинга
				 */
				result[2 * index] = static_cast <uint8_t> ((word >> 8) & 0xFF);
				result[2 * index + 1] = static_cast <uint8_t> (word & 0xFF);
				// Переходим к следующему слову полного адреса
				++index;
			};
			/**
			 * Записываем слова левой части адреса
			 */
			for(uint8_t i = 0; i < static_cast <uint8_t> (words.size()); ++i)
				// Записываем очередное слово левой части адреса
				store(words[i]);
			/**
			 * Записываем нулевые слова на место сжатия
			 */
			for(uint8_t i = 0; i < zerosToInsert; ++i)
				// Записываем очередное нулевое слово
				store(0);
			/**
			 * Записываем слова правой части адреса
			 */
			for(uint8_t i = 0; i < rightWordsCount; ++i)
				// Записываем очередное слово правой части адреса
				store(rightWords[i]);
			// Возвращаем успешный результат парсинга
			return true;
		// Иначе без сжатия
		} else {
			// Без сжатия — либо 8 слов, либо 6 слов + IPv4 уже переведённый в 2 слова
			if(words.size() != 8)
				// Возвращаем ошибку
				return false;
			/**
			 * Перебираем все слова полного адреса
			 */
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
		bool allowZoneId       = true; // %zone, %25 в [ ]
		bool allowLegacyV4     = true; // a.b.c, a.b, a и др.
		bool allowBrackets     = true; // [addr]
		bool allowEmbeddedV4   = true; // a:b:c:d:e:f:w.x.y.z
		bool allowNonDecimalV4 = true; // 0xHEX, 0... OCT
	} options_t;
	/**
	 * @brief Функция парсинга IPv4-адреса из строки
	 *
	 * @param ip      строка с IPv4-адресом
	 * @param result  результат парсинга в бинарном виде
	 * @param options опции парсинга
	 * @return        результат выполнения парсинга
	 *
	 * @note Результат принимается указателем на буфер размером в четыре байта:
	 *       проверка адреса заводила под него динамический массив, а выделение
	 *       памяти ради проверки, ничего наружу не отдающей, есть работа впустую
	 *
	 */
	bool ipv4(string_view ip, uint8_t * result, const options_t & options = {}) noexcept {
		// Тримминг строки IP-адреса
		auto addr = ::trim(ip);
		// Обрабатываем скобки
		if(addr.empty())
			// Возвращаем ошибку
			return false;
		// Парсим IPv4-адрес
		if(!::parseIPv4(addr, result, options.allowLegacyV4, options.allowNonDecimalV4))
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
	 *
	 * @note Результат принимается указателем на буфер размером в шестнадцать байт:
	 *       проверка адреса заводила под него динамический массив, а выделение
	 *       памяти ради проверки, ничего наружу не отдающей, есть работа впустую
	 *
	 */
	bool ipv6(string_view ip, uint8_t * result, string & zone, const options_t & options = {}) noexcept {
		// Тримминг строки IP-адреса
		auto addr = ::trim(ip);
		// Обрабатываем скобки
		if(addr.empty())
			// Возвращаем ошибку
			return false;
		// Проверяем квадратные скобки вокруг адреса
		if(options.allowBrackets && ::startWith(addr, "[") && ::endWith(addr, "]"))
			// Убираем скобки
			addr = addr.substr(1, addr.size() - 2);
		// Если разрешён zone-id
		if(options.allowZoneId){
			// Внутри скобок RFC 6874 требует %25 как экранированный '%'
			size_t pos = addr.find("%25");
			// Если найдено значение "%25"
			if(pos != string_view::npos){
				// Извлекаем зону
				zone = addr.substr(pos + 3);
				// Обрезаем основной IP-адрес
				addr = addr.substr(0, pos);
			// Иначе ищем обычный '%'
			} else {
				// Ищем обычный '%'
				pos = addr.find('%');
				// Если найдено значение '%'
				if(pos != string_view::npos){
					// Извлекаем зону
					zone = addr.substr(pos + 1);
					// Обрезаем основной IP-адрес
					addr = addr.substr(0, pos);
				}
			}
		}
		// Парсим IPv6-адрес
		if(!::parseIPv6(addr, result, options.allowEmbeddedV4))
			// Возвращаем ошибку
			return false;
		// Возвращаем успешный результат парсинга
		return true;
	}
	/**
	 * @brief Функция поиска самой длинной последовательности нулевых хекстетов IPv6 (минимум 2)
	 *
	 * @param hexets массив из 8 хекстетов IPv6-адреса
	 * @param begin  индекс начала найденной последовательности (-1 если не найдено)
	 * @param length длина найденной последовательности (1 если подходящей нет)
	 *
	 */
	void findZeroRun(const uint16_t hexets[8], int16_t & begin, int16_t & length) noexcept {
		// Сбрасываем результат поиска
		begin = -1, length = 1;
		/**
		 * Поиск лучшей последовательности нулевых хекстетов
		 */
		for(int16_t i = 0; i < 8;){
			// Если текущий хекстет равен нулю
			if(hexets[i] == 0){
				// Ищем длину последовательности нулевых хекстетов
				int16_t j = i;
				/**
				 * Продолжаем, пока не дойдём до конца массива или не встретим ненулевой хекстет
				 */
				while((j < 8) && (hexets[j] == 0))
					// Увеличиваем длину последовательности
					j++;
				// Если текущая последовательность длиннее найденной ранее
				if((j - i) > length){
					// Запоминаем новую лучшую последовательность
					begin = i;
					// Запоминаем длину последовательности
					length = (j - i);
				}
				// Продвигаем основной индекс вперёд
				i = j;
			// Если текущий хекстет НЕ равен нулю, смещаем индекс вперёд
			} else i++;
		}
	}
	/**
	 * @brief Функция формирования строки IPv6-адреса со сжатием нулевых групп
	 *
	 * @param out    буфер назначения (должен быть достаточного размера)
	 * @param hexets массив из 8 хекстетов IPv6-адреса
	 * @param conv   спецификатор системы счисления ('X' - hex, 'u' - dec, 'o' - oct)
	 * @param delim  разделитель групп
	 * @return       количество записанных символов
	 *
	 */
	int32_t emitIPv6(char * out, const uint16_t hexets[8], const char conv, const char delim) noexcept {
		// Текущая позиция записи
		int32_t pos = 0;
		/**
		 * @brief Запись одной группы в заданной системе счисления
		 *
		 * @param i индекс группы
		 *
		 */
		auto group = [&](const int16_t i) noexcept -> void {
			/**
			 * Определяем систему счисления
			 */
			switch(conv){
				// Шестнадцатеричный формат
				case 'X': pos += ::sprintf(out + pos, "%X", hexets[i]); break;
				// Десятичный формат
				case 'u': pos += ::sprintf(out + pos, "%u", hexets[i]); break;
				// Восьмеричный формат
				case 'o': pos += ::sprintf(out + pos, "%o", hexets[i]); break;
			}
		};
		// Лямбда записи разделителя
		auto sep = [&]() noexcept -> void { out[pos++] = delim; };
		// Индексы начала и длины самой длинной нулевой последовательности
		int16_t begin = -1, length = 1;
		// Выполняем поиск лучшей последовательности нулевых хекстетов
		::findZeroRun(hexets, begin, length);
		// Если сжатие не применяется
		if(length <= 1){
			/**
			 * Выводим все хекстеты подряд через разделитель
			 */
			for(int16_t i = 0; i < 8; ++i){
				// Если это не первая группа, добавляем разделитель
				if(i > 0) sep();
				// Записываем группу
				group(i);
			}
		// Если сжатие в начале (формат ::xxx)
		} else if(begin == 0) {
			// Записываем "::"
			sep(); sep();
			/**
			 * Выводим оставшиеся хекстеты
			 */
			for(int16_t i = length; i < 8; ++i){
				// Перед всеми, кроме первого после "::", добавляем разделитель
				if(i != length) sep();
				// Записываем группу
				group(i);
			}
		// Если сжатие в конце (формат xxx::)
		} else if((begin + length) == 8) {
			/**
			 * Выводим хекстеты до сжатия
			 */
			for(int16_t i = 0; i < begin; ++i){
				// Если это не первая группа, добавляем разделитель
				if(i > 0) sep();
				// Записываем группу
				group(i);
			}
			// Завершаем "::"
			sep(); sep();
		// Если сжатие в середине (формат xxx::xxx)
		} else {
			/**
			 * Выводим хекстеты до сжатия
			 */
			for(int16_t i = 0; i < begin; ++i){
				// Если это не первая группа, добавляем разделитель
				if(i > 0) sep();
				// Записываем группу
				group(i);
			}
			// Добавляем "::"
			sep(); sep();
			/**
			 * Выводим хекстеты после сжатия
			 */
			for(int16_t i = (begin + length); i < 8; ++i){
				// Записываем группу
				group(i);
				// После всех, кроме последней группы, добавляем разделитель
				if(i != 7) sep();
			}
		}
		// Завершаем строку нулевым символом
		out[pos] = '\0';
		// Возвращаем количество записанных символов
		return pos;
	}
	/**
	 * @brief Функция формирования опций парсинга в зависимости от строгого режима
	 *
	 * @param strict флаг строгого режима
	 * @return       сформированные опции парсинга
	 *
	 */
	options_t makeOptions(const bool strict) noexcept {
		// Опции парсинга по умолчанию (либеральный режим)
		options_t options;
		// Если включён строгий режим
		if(strict){
			// Запрещаем legacy-формы IPv4 (a.b.c, a.b, a)
			options.allowLegacyV4 = false;
			// Запрещаем не-десятичные системы счисления для IPv4 (0x..., 0...)
			options.allowNonDecimalV4 = false;
		}
		// Возвращаем сформированные опции
		return options;
	}
	/**
	 * @brief Вспомогательная функция получения признака разновидности адреса
	 *
	 * @param type разновидность адреса
	 * @return     признак разновидности адреса
	 *
	 */
	constexpr uint16_t mark(const awh::Network_Address::type_t type) noexcept {
		// Возвращаем признак разновидности адреса
		return static_cast <uint16_t> (1 << static_cast <uint8_t> (type));
	}
	/**
	 * @brief Признаки принадлежности символа алфавитам разновидностей адреса
	 *
	 */
	enum class letter_t : uint8_t {
		LETTER_IPV4 = 0x01, // Символ допустим в IPv4-адресе
		LETTER_IPV6 = 0x02  // Символ допустим в IPv6-адресе
	};
	/**
	 * @brief Вспомогательная функция формирования таблицы алфавитов адресов
	 *
	 * @details Снятие примет обходит строку посимвольно, и обход этот стоит на
	 *          пути каждого определения разновидности адреса. Три ветвящиеся
	 *          проверки на символ обходились дороже, чем сама проверка адреса
	 *          на IPv4, поэтому принадлежность символа алфавитам сведена в
	 *          таблицу: тело обхода становится одним чтением из таблицы и одним
	 *          наложением на накопитель.
	 *
	 * @return сформированная таблица алфавитов
	 *
	 */
	array <uint8_t, 256> makeAlphabet() noexcept {
		// Формируемая таблица алфавитов
		array <uint8_t, 256> result = {0};
		/**
		 * Проходим по всем возможным значениям символа
		 */
		for(size_t i = 0; i < result.size(); ++i){
			// Текущий символ таблицы
			const char letter = static_cast <char> (i);
			// Проверяем шестнадцатеричный символ
			const bool hex = ascii::isHex(letter);
			// Если символ входит в алфавит IPv4-адреса
			if(hex || (letter == '.') || (letter == 'x') || (letter == 'X'))
				// Устанавливаем признак алфавита IPv4-адреса
				result[i] |= static_cast <uint8_t> (letter_t::LETTER_IPV4);
			// Если символ входит в алфавит IPv6-адреса
			if(hex || (letter == ':') || (letter == '.'))
				// Устанавливаем признак алфавита IPv6-адреса
				result[i] |= static_cast <uint8_t> (letter_t::LETTER_IPV6);
		}
		// Возвращаем сформированную таблицу алфавитов
		return result;
	}
	// Таблица принадлежности символов алфавитам разновидностей адреса
	const array <uint8_t, 256> ALPHABET = ::makeAlphabet();
	/**
	 * @brief Структура примет строки адреса
	 *
	 */
	typedef struct Marks {
		bool ipv4; // Строка способна оказаться IPv4-адресом
		bool ipv6; // Строка способна оказаться IPv6-адресом
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Marks() noexcept :
		 ipv4(false), ipv6(false) {}
	} marks_t;
	/**
	 * @brief Вспомогательная функция снятия примет строки адреса
	 *
	 * @details Приметы снимаются за один обход строки, и обход этот один на обе
	 *          разновидности: набор допустимых символов у них разный, но
	 *          пересекающийся, и разделять обход значило бы читать строку дважды.
	 *
	 *          Разбор IPv4-адреса читает только цифры своей системы счисления и
	 *          точку-разделитель, а самая широкая из систем - шестнадцатеричная
	 *          с приставкой "0x". Разбор IPv6-адреса читает только
	 *          шестнадцатеричные цифры, двоеточие-разделитель и точку встроенного
	 *          IPv4-адреса; квадратные скобки и зона снимаются до разбора,
	 *          поэтому здесь они снимаются тоже.
	 *
	 *          Кроме алфавита, у IPv6-адреса проверяется число двоеточий. Без
	 *          сжатия адрес состоит ровно из восьми слов, то есть из семи
	 *          двоеточий, либо из шести слов и встроенного IPv4-адреса, то есть
	 *          из шести двоеточий. Со сжатием двоеточий не менее двух. Примета
	 *          эта отсеивает аппаратный адрес, у которого двоеточий пять, - иначе
	 *          его пришлось бы разбирать как IPv6-адрес впустую.
	 *
	 * @note Зона отсекается по первому знаку процента, тогда как разбор ищет
	 *       сперва экранированный "%25". Отсечение по первому знаку оставляет
	 *       строку короче, то есть примета выходит мягче разбора, а не строже:
	 *       лишний кандидат стоит одной проверки, недостающий изменил бы ответ
	 *
	 * @param text строка для снятия примет
	 * @return     снятые приметы строки
	 *
	 */
	marks_t scan(string_view text) noexcept {
		// Снятые приметы строки
		marks_t result;
		// Пустая строка адресом не является
		if(text.empty())
			// Возвращаем снятые приметы строки
			return result;
		// Снимаем квадратные скобки
		if((text.size() > 1) && (text.front() == '[') && (text.back() == ']'))
			// Получаем строку без квадратных скобок
			text = text.substr(1, text.size() - 2);
		// Количество двоеточий
		size_t colons = 0;
		// Предыдущий символ строки
		char previous = 0;
		// Признак сжатия адреса
		bool doubled = false;
		// Размер строки адреса
		const size_t length = text.size();
		/**
		 * Накопитель признаков алфавитов, общих всем символам строки
		 *
		 * @note Строка, опустевшая после снятия квадратных скобок, адресом не
		 *       является, и накопитель для неё обнуляется сразу
		 *
		 */
		uint8_t accumulated = ((length > 0) ? (static_cast <uint8_t> (letter_t::LETTER_IPV4) | static_cast <uint8_t> (letter_t::LETTER_IPV6)) : 0);
		/**
		 * Проходим по каждому символу строки один раз
		 */
		for(size_t i = 0; i < length; ++i){
			// Текущий символ строки
			const char letter = text[i];
			// Знак процента открывает зону адреса, а зона разбору не подлежит
			if(letter == '%')
				// Прекращаем снятие примет строки
				break;
			// Накладываем признаки алфавитов очередного символа на накопитель
			accumulated &= ::ALPHABET[static_cast <uint8_t> (letter)];
			// Если символ является разделителем хекстетов
			if(letter == ':'){
				// Увеличиваем количество двоеточий
				++colons;
				// Если предыдущий символ был разделителем хекстетов
				if(previous == ':')
					// Запоминаем признак сжатия адреса
					doubled = true;
			}
			// Запоминаем предыдущий символ строки
			previous = letter;
		}
		// Устанавливаем примету IPv4-адреса
		result.ipv4 = ((accumulated & static_cast <uint8_t> (letter_t::LETTER_IPV4)) != 0);
		/**
		 * Устанавливаем примету IPv6-адреса: без сжатия двоеточий бывает шесть
		 * или семь, со сжатием - не менее двух
		 */
		result.ipv6 = (
			((accumulated & static_cast <uint8_t> (letter_t::LETTER_IPV6)) != 0) &&
			(doubled ? (colons >= 2) : ((colons == 7) || (colons == 6)))
		);
		// Возвращаем снятые приметы строки
		return result;
	}
	/**
	 * @brief Вспомогательная функция проверки алфавита аппаратного адреса
	 *
	 * @param text проверяемая строка
	 * @return     результат проверки
	 *
	 */
	bool macAlphabet(string_view text) noexcept {
		/**
		 * Проходим по каждому символу строки
		 */
		for(char letter : text){
			// Если символ не входит в алфавит аппаратного адреса
			if(!ascii::isHex(letter) && (letter != ':') && (letter != '-'))
				// Возвращаем результат проверки
				return false;
		}
		// Возвращаем результат проверки
		return true;
	}
	/**
	 * @brief Функция определения возможных разновидностей адреса
	 *
	 * @details Определение разновидности адреса устроено перебором: строка
	 *          проверяется на каждую разновидность по очереди, пока какая-нибудь
	 *          не подойдёт. Платится при этом за неудачные попытки, а не за
	 *          удачную, и адрес файловой системы обходился впятеро дороже
	 *          IPv4-адреса лишь потому, что стоял в переборе седьмым.
	 *
	 *          Строка, однако, сама сообщает, чем она может быть, и сообщает за
	 *          один свой обход: различают разновидности всего четыре приметы -
	 *          наличие двоеточия, наличие косой черты, алфавит и первый символ.
	 *          Функция снимает эти приметы и возвращает набор разновидностей,
	 *          которыми строка вообще способна оказаться, а перебор проверяет
	 *          только их, сохраняя прежний порядок обхода.
	 *
	 * @note Набор обязан быть надмножеством: разновидность, проверка на которую
	 *       могла бы пройти, обязана в набор попасть. Лишняя разновидность в
	 *       наборе стоит одной проверки впустую, недостающая изменила бы ответ,
	 *       поэтому всякая примета здесь выведена из условий самой проверки, а
	 *       при сомнении разновидность оставляется в наборе
	 *
	 * @param addr строка адреса
	 * @return     набор признаков разновидностей, которыми адрес может оказаться
	 *
	 */
	uint16_t classify(string_view addr) noexcept {
		// Разновидности адреса
		using type_t = awh::Network_Address::type_t;
		// Доменное имя является последним прибежищем перебора и из набора не выпадает
		uint16_t result = ::mark(type_t::FQDN);
		// Если адрес не передан
		if(addr.empty())
			// Возвращаем набор признаков разновидностей
			return result;
		/**
		 * Снимаем приметы строки: проверка на IPv4-адрес и на IPv6-адрес
		 * обрезает пробелы, поэтому приметы снимаются с обрезанной строки
		 */
		const marks_t marks = ::scan(::trim(addr));
		// Если строка способна оказаться IPv4-адресом
		if(marks.ipv4)
			// Устанавливаем признак IPv4-адреса
			result |= ::mark(type_t::IPV4);
		// Если строка способна оказаться IPv6-адресом
		if(marks.ipv6)
			// Устанавливаем признак IPv6-адреса
			result |= ::mark(type_t::IPV6);
		/**
		 * Проверка на аппаратный адрес пробелов не обрезает и требует строки
		 * ровно в двенадцать или семнадцать символов
		 */
		if(((addr.size() == 12) || (addr.size() == 17)) && ::macAlphabet(addr))
			// Устанавливаем признак аппаратного адреса
			result |= ::mark(type_t::MAC);
		// Ищем разделитель адреса и маски сети
		const size_t slash = addr.find('/');
		/**
		 * Разделитель в начале строки оставил бы адрес сети пустым, а пустой
		 * адрес разбору не поддаётся, поэтому сетью такая строка быть не может
		 */
		if((slash != string_view::npos) && (slash > 0)){
			// Снимаем приметы адреса сети без маски
			const marks_t network = ::scan(::trim(addr.substr(0, slash)));
			// Если адрес сети способен оказаться IPv4-адресом
			if(network.ipv4)
				// Устанавливаем признак адреса сети IPv4
				result |= ::mark(type_t::NETV4);
			// Если адрес сети способен оказаться IPv6-адресом
			if(network.ipv6)
				// Устанавливаем признак адреса сети IPv6
				result |= ::mark(type_t::NETV6);
		}
		/**
		 * Проверка на URL-адрес требует приставки протокола, а приставка эта
		 * сравнивается без учёта регистра
		 */
		if((addr.size() >= 8) && ((addr.front() == 'h') || (addr.front() == 'H')))
			// Устанавливаем признак URL-адреса
			result |= ::mark(type_t::URL);
		/**
		 * Проверка на адрес файловой системы сама состоит из сравнений первых
		 * символов, поэтому приметой её служат ровно её собственные условия
		 */
		if((addr.front() == '/') || (addr.front() == '~') || (addr.front() == '.') ||
		   ((addr.size() >= 2) && (addr[0] == '\\') && (addr[1] == '\\')) ||
		   ((addr.size() >= 3) && (addr[1] == ':') && ((addr[2] == '\\') || (addr[2] == '/'))))
			// Устанавливаем признак адреса файловой системы
			result |= ::mark(type_t::FS);
		// Возвращаем набор признаков разновидностей
		return result;
	}
};

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Network_Address::LocalNet::LocalNet(const fmk_t * fmk, const log_t * log) noexcept :
 reserved(false), prefix(0),
 end(make_unique <Network_Address> (fmk, log)),
 begin(make_unique <Network_Address> (fmk, log)) {}

/**
 * @brief Метод проверки заполненности буфера
 *
 * @return результат проверки
 *
 */
bool awh::Network_Address::Buffer::empty() const noexcept {
	// Буфер пуст, если не занято ни одного байта
	return (this->_size == 0);
}
/**
 * @brief Метод получения размера буфера
 *
 * @return размер буфера
 *
 */
size_t awh::Network_Address::Buffer::size() const noexcept {
	// Выводим количество занятых байт буфера
	return this->_size;
}
/**
 * @brief Метод очистки буфера
 *
 */
void awh::Network_Address::Buffer::clear() noexcept {
	// Сбрасываем количество занятых байт буфера
	this->_size = 0;
}
/**
 * @brief Метод изменения размера буфера
 *
 * @param size  новый размер буфера
 * @param value значение заполнения добавленных байт
 *
 * @note Размер сверх ёмкости обрезается: адреса длиннее
 *       шестнадцати байт не бывает, и запрос такого размера
 *       означал бы ошибку вызывающей стороны
 *
 */
void awh::Network_Address::Buffer::resize(const size_t size, const uint8_t value) noexcept {
	// Получаем размер буфера в пределах его ёмкости
	const size_t actual = ((size < CAPACITY) ? size : CAPACITY);
	/**
		* Заполняем добавленные байты буфера
		*/
	for(size_t i = this->_size; i < actual; ++i)
		// Устанавливаем значение заполнения очередного байта
		this->_data[i] = value;
	// Запоминаем новый размер буфера
	this->_size = actual;
}
/**
 * @brief Метод получения указателя на данные буфера
 *
 * @return указатель на данные буфера
 *
 */
uint8_t * awh::Network_Address::Buffer::data() noexcept {
	// Выводим указатель на данные буфера
	return this->_data;
}
/**
 * @brief Метод получения указателя на данные буфера
 *
 * @return указатель на данные буфера
 *
 */
const uint8_t * awh::Network_Address::Buffer::data() const noexcept {
	// Выводим указатель на данные буфера
	return this->_data;
}
/**
 * @brief Оператор получения байта буфера по индексу
 *
 * @param index индекс байта буфера
 * @return      байт буфера
 *
 */
uint8_t & awh::Network_Address::Buffer::operator [] (const size_t index) noexcept {
	// Выводим байт буфера по индексу
	return this->_data[index];
}
/**
 * @brief Оператор получения байта буфера по индексу
 *
 * @param index индекс байта буфера
 * @return      байт буфера
 *
 */
const uint8_t & awh::Network_Address::Buffer::operator [] (const size_t index) const noexcept {
	// Выводим байт буфера по индексу
	return this->_data[index];
}
/**
 * @brief Конструктор
 *
 */
awh::Network_Address::Buffer::Buffer() noexcept : _size(0), _data{0} {}

/**
 * @brief Метод инициализации списка локальных адресов
 *
 */
void awh::Network_Address::initLocalNet() noexcept {
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки данных IP-адреса
 *
 */
void awh::Network_Address::clear() noexcept {
	// Выполняем сброс буфера данных
	this->_buffer.clear();
	// Устанавливаем тип IP-адреса
	this->_type = type_t::NONE;
}
/**
 * @brief Метод проверки соответствия адреса зеркалу IPv6 => IPv4
 *
 * @return результат проверки
 *
 */
bool awh::Network_Address::broadcastIPv6ToIPv4() const noexcept {
	// Переменная результата
	bool result = false;
	// Если бинарный буфер данных является полноценным IPv6-адресом
	if(this->_buffer.size() == 16){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём временный буфер данных для сравнения
			vector <uint16_t> buffer(6);
			// Устанавливаем хекстет маски
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения зоны IPv6 адреса
 *
 * @return зона IPv6 адреса
 *
 */
const string & awh::Network_Address::zone() const noexcept {
	// Возвращаем результат
	return this->_zone;
}
/**
 * @brief Метод установки зоны IPv6 адреса
 *
 * @param zone зона IPv6 адреса для установки
 *
 */
void awh::Network_Address::zone(string_view zone) noexcept {
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(zone), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод извлечения типа IP-адреса
 *
 * @return тип IP-адреса
 *
 */
awh::Network_Address::type_t awh::Network_Address::type() const noexcept {
	// Выполняем тип IP-адреса
	return this->_type;
}
/**
 * @brief Метод установки типа IP-адреса
 *
 * @param type тип IP-адреса для установки
 *
 */
void awh::Network_Address::type(const type_t type) noexcept {
	// Выполняем установку типа IP-адреса
	this->_type = type;
}
/**
 * @brief Метод извлечения флага строгого режима парсинга/проверки адресов
 *
 * @return флаг строгого режима
 *
 */
bool awh::Network_Address::strict() const noexcept {
	// Возвращаем флаг строгого режима
	return this->_strict;
}
/**
 * @brief Метод установки строгого режима парсинга/проверки адресов
 *
 * @param mode флаг строгого режима (в строгом режиме для IPv4 запрещены legacy-формы
 *             [a.b.c, a.b, a] и не-десятичные системы счисления [0x..., 0...])
 *
 */
void awh::Network_Address::strict(const bool mode) noexcept {
	// Выполняем установку флага строгого режима
	this->_strict = mode;
}
/**
 * @brief Метод определения типа хоста
 *
 * @param host хост для определения
 * @return     определённый тип хоста
 *
 */
awh::Network_Address::type_t awh::Network_Address::host(string_view host) const noexcept {
	// Результат полученных данных
	type_t result = type_t::NONE;
	// Если хост передан
	if(!host.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * IPv4-адрес стоит в переборе первым, и выигрыша от примет он не
			 * получил бы никакого, а обход строки ради их снятия обошёлся бы ему
			 * в треть его собственной цены. Поэтому проверяется он сразу, а от
			 * заведомо чужих строк отгораживается поиском двоеточия: двоеточия
			 * в IPv4-адресе не бывает, а поиск одного символа обходится много
			 * дешевле снятия примет.
			 *
			 * Косая черта такой заставки не удостоена намеренно: её разбор
			 * IPv4-адреса отвергает на первом же символе части, тогда как
			 * двоеточие встречается у половины разновидностей - у IPv6-адреса,
			 * у аппаратного адреса, у URL-адреса, у сети IPv6 и у оконного
			 * адреса файловой системы
			 */
			if(host.find(':') == string_view::npos){
				// Если строка оказалась IPv4-адресом
				if(this->check(host, type_t::IPV4))
					// Возвращаем результат
					return type_t::IPV4;
			}
			/**
			 * Снимаем приметы строки: набор разновидностей, которыми она вообще
			 * способна оказаться, определяется за один её обход, и перебор ниже
			 * проверяет только их, а не все оставшиеся подряд
			 */
			const uint16_t candidates = ::classify(host);
			/**
			 * Выполняем полную проверку всех остальных типов хостов
			 */
			for(uint8_t i = 1; i < 9; i++){
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
				// Если тип не определён, завершаем перебор
				if(result == type_t::NONE)
					// Возвращаем результат
					return result;
				// Если разновидность из набора примет выпала, проверять её незачем
				if(!(candidates & ::mark(result)))
					// Переходим к следующей разновидности адреса
					continue;
				// Если проверка пройдена успешно
				if(this->check(host, result))
					// Возвращаем результат
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(host), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения аппаратного адреса в чистом виде
 *
 * @return аппаратный адрес в чистом виде
 *
 */
array <uint8_t, 6> awh::Network_Address::mac() const noexcept {
	// Переменная результата
	array <uint8_t, 6> result = {0};
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки аппаратного адреса в чистом виде
 *
 * @param addr аппаратный адрес в чистом виде
 *
 */
void awh::Network_Address::mac(const array <uint8_t, 6> & addr) noexcept {
	// Если MAC адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем тип MAC адреса
			this->_type = type_t::MAC;
			// Выполняем выделение памяти для MAC адреса
			this->_buffer.resize(6, 0);
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(addr.front(), addr.back()),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выполняем очистку буфера данных
	} else this->_buffer.clear();
}
/**
 * @brief Метод извлечения адреса IPv4 в чистом виде
 *
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 * @return       адрес IPv4 в чистом виде
 *
 */
uint32_t awh::Network_Address::v4(const endian_t endian) const noexcept {
	// Переменная результата
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса IPv4 в чистом виде
 *
 * @param addr   адрес IPv4 в чистом виде
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 *
 */
void awh::Network_Address::v4(const uint32_t addr, const endian_t endian) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем тип IP-адреса
		this->_type = type_t::IPV4;
		// Выполняем выделение памяти для IPv4 адреса
		this->_buffer.resize(4, 0);
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
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(addr, static_cast <uint16_t> (endian)),
				log_t::flag_t::CRITICAL, error.what()
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод извлечения адреса IPv6 в чистом виде
 *
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 * @return       адрес IPv6 в чистом виде
 *
 */
array <uint8_t, 16> awh::Network_Address::v6(const endian_t endian) const noexcept {
	// Переменная результата
	array <uint8_t, 16> result = {0};
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса IPv6 в чистом виде
 *
 * @param addr   адрес IPv6 в чистом виде
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 *
 */
void awh::Network_Address::v6(const array <uint8_t, 16> & addr, const endian_t endian) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем тип IP-адреса
		this->_type = type_t::IPV6;
		// Выполняем выделение памяти для IPv6 адреса
		this->_buffer.resize(16, 0);
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
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(addr.front(), addr.back(), static_cast <uint16_t> (endian)),
				log_t::flag_t::CRITICAL, error.what()
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод извлечения адреса в чистом виде
 *
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 * @return       адрес в чистом виде
 *
 */
unique_ptr <awh::net::addr_t> awh::Network_Address::source(const endian_t endian) const noexcept {
	// Переменная результата
	unique_ptr <awh::net::addr_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип адреса
		 */
		switch(this->_buffer.size()){
			// Если адрес является MAC-адресом
			case 6: {
				// Выполняем выделение памяти для MAC-адреса
				result = make_unique <net::addr_mac_t> ();
				// Выполняем перевод бинарного буфера MAC-адреса в числовой вид
				::memcpy(&awh_cast <net::addr_mac_t *> (result.get())->address[0], &this->_buffer[0], this->_buffer.size());
			} break;
			// Если адрес является IPv4-адресом
			case 4: {
				// Выполняем выделение памяти для IPv4-адреса
				result = make_unique <net::addr_net_ipv4_t> ();
				/**
				 * Определяем какой порядок следования байт установлен
				 */
				switch(static_cast <uint8_t> (endian)){
					// Если установлен порядок следования байт от старшего к младшему
					case static_cast <uint8_t> (endian_t::BIG):
						// Получаем буфер данных IP-адреса
						::convertEndian(&this->_buffer[0], reinterpret_cast <uint8_t *> (&awh_cast <net::addr_net_ipv4_t *> (result.get())->address));
					break;
					// Если установлен порядок следования байт от младшего к старшему
					case static_cast <uint8_t> (endian_t::LITTLE):
						// Выполняем копирование данных адреса IPv4
						::memcpy(&awh_cast <net::addr_net_ipv4_t *> (result.get())->address, &this->_buffer[0], this->_buffer.size());
					break;
				}
			} break;
			// Если адрес является IPv6-адресом
			case 16: {
				// Выполняем выделение памяти для IPv6-адреса
				result = make_unique <net::addr_net_ipv6_t> ();
				/**
				 * Определяем какой порядок следования байт установлен
				 */
				switch(static_cast <uint8_t> (endian)){
					// Если установлен порядок следования байт от старшего к младшему
					case static_cast <uint8_t> (endian_t::BIG):
						// Получаем буфер данных IP-адреса
						::convertEndian <16> (&this->_buffer[0], reinterpret_cast <uint8_t *> (&awh_cast <net::addr_net_ipv6_t *> (result.get())->address[0]));
					break;
					// Если установлен порядок следования байт от младшего к старшему
					case static_cast <uint8_t> (endian_t::LITTLE):
						// Выполняем копирование данных адреса IPv6
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (result.get())->address[0], &this->_buffer[0], this->_buffer.size());
					break;
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
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(static_cast <uint16_t> (endian)),
				log_t::flag_t::CRITICAL, error.what()
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса в чистом виде
 *
 * @param value  адрес в чистом виде для установки
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 *
 */
void awh::Network_Address::source(const net::addr_t * value, const endian_t endian) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если передан пустой адрес
		if(value == nullptr)
			// Выходим из функции
			return;
		/**
		 * Определяем тип адреса
		 */
		switch(value->size){
			// Если адрес является MAC-адресом
			case 6: {
				// Устанавливаем тип MAC адреса
				this->_type = type_t::MAC;
				// Выполняем выделение памяти для MAC адреса
				this->_buffer.resize(6, 0);
				// Выполняем копирование данных адреса MAC
				::memcpy(&this->_buffer[0], &awh_cast <const net::addr_mac_t *> (value)->address[0], 6);
			} break;
			// Если адрес является IPv4
			case 4: {
				// Устанавливаем тип IP-адреса
				this->_type = type_t::IPV4;
				// Выполняем выделение памяти для IPv4 адреса
				this->_buffer.resize(4, 0);
				/**
				 * Определяем какой порядок следования байт установлен
				 */
				switch(static_cast <uint8_t> (endian)){
					// Если установлен порядок следования байт от старшего к младшему
					case static_cast <uint8_t> (endian_t::BIG):
						// Устанавливаем буфер данных IP-адреса
						::convertEndian(reinterpret_cast <const uint8_t *> (&awh_cast <const net::addr_net_ipv4_t *> (value)->address), &this->_buffer[0]);
					break;
					// Если установлен порядок следования байт от младшего к старшему
					case static_cast <uint8_t> (endian_t::LITTLE):
						// Выполняем копирование данных адреса IPv4
						::memcpy(&this->_buffer[0], &awh_cast <const net::addr_net_ipv4_t *> (value)->address, 4);
					break;
				}
			} break;
			// Если адрес является IPv6-адресом
			case 16: {
				// Устанавливаем тип IP-адреса
				this->_type = type_t::IPV6;
				// Выполняем выделение памяти для IPv6 адреса
				this->_buffer.resize(16, 0);
				/**
				 * Определяем какой порядок следования байт установлен
				 */
				switch(static_cast <uint8_t> (endian)){
					// Если установлен порядок следования байт от старшего к младшему
					case static_cast <uint8_t> (endian_t::BIG):
						// Устанавливаем буфер данных IP-адреса
						::convertEndian <16> (reinterpret_cast <const uint8_t *> (&awh_cast <const net::addr_net_ipv6_t *> (value)->address[0]), &this->_buffer[0]);
					break;
					// Если установлен порядок следования байт от младшего к старшему
					case static_cast <uint8_t> (endian_t::LITTLE):
						// Выполняем копирование данных адреса IPv6
						::memcpy(&this->_buffer[0], &awh_cast <const net::addr_net_ipv6_t *> (value)->address[0], 16);
					break;
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
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(static_cast <uint16_t> (endian)),
				log_t::flag_t::CRITICAL, error.what()
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод проверки валидности IP-адреса
 *
 * @param addr адрес аппаратный или интернет подключения для проверки
 * @param type тип адреса аппаратного или интернет подключения для проверки
 * @return     результат проверки
 *
 */
bool awh::Network_Address::check(const string_view addr, const type_t type) const noexcept {
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
					// Если длина MAC-адреса равна 12 символам
					if(addr.length() == 12){
						/**
						 * Выполняем проверку каждого символа MAC-адреса
						 */
						for(char letter : addr){
							// Если символ не является шестнадцатеричным
							if(!ascii::isHex(letter))
								// Возвращаем результат проверки
								return false;
						}
						// Возвращаем результат проверки
						return true;
					}
					/**
					 * Выполняем проверку каждого символа MAC-адреса
					 */
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
							if(!ascii::isHex(addr[i]))
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
					uint8_t buffer[4] = {0};
					// Выполняем проверку IP-адреса IPv4
					return ::ipv4(addr, buffer, ::makeOptions(this->_strict));
				}
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Временное значение зоны для проверки IP-адреса
					string zone = "";
					// Временный буфер для проверки IP-адреса
					uint8_t buffer[16] = {0};
					// Выполняем проверку IP-адреса IPv6
					return ::ipv6(addr, buffer, zone, ::makeOptions(this->_strict));
				}
				// Если IP-адрес определён как NetV4
				case static_cast <uint8_t> (type_t::NETV4): {
					// Разделяем адрес и маску сети
					const auto pos = addr.find('/');
					// Если разделитель найден
					if(pos != string::npos){
						// Временный буфер для проверки IP-адреса
						uint8_t buffer[4] = {0};
						// Получаем IP-адрес без маски сети
						const string_view ip = addr.substr(0, pos);
						// Получаем маску переданной сети
						const string_view suffix = addr.substr(pos + 1);
						// Проверяем является ли суффикс числом
						if(::all_of(suffix.begin(), suffix.end(), ascii::isDigit)){
							// Получаем префикс сети
							if(this->_fmk->atoi <uint8_t> (suffix.data(), suffix.length()) > 32)
								// Если префикс сети больше допустимого значения
								return false;
						// Если суффикс не является числом и не является корректной маской сети
						} else if(!::ipv4(suffix, buffer, ::makeOptions(this->_strict)))
							// Возвращаем результат проверки
							return false;
						// Зануляем структуру
						::memset(buffer, 0, sizeof(buffer));
						// Выполняем проверку IP-адреса IPv4
						return ::ipv4(ip, buffer, ::makeOptions(this->_strict));
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
						uint8_t buffer[16] = {0};
						// Получаем IP-адрес без маски сети
						const string_view ip = addr.substr(0, pos);
						// Получаем маску переданной сети
						const string_view suffix = addr.substr(pos + 1);
						// Проверяем является ли суффикс числом
						if(::all_of(suffix.begin(), suffix.end(), ascii::isDigit)){
							// Получаем префикс сети
							if(this->_fmk->atoi <uint8_t> (suffix.data(), suffix.length()) > 128)
								// Если префикс сети больше допустимого значения
								return false;
						// Если суффикс не является числом и не является корректной маской сети
						} else if(!::ipv6(suffix, buffer, zone, ::makeOptions(this->_strict)))
							// Возвращаем результат проверки
							return false;
						// Зануляем структуру
						::memset(buffer, 0, sizeof(buffer));
						// Выполняем проверку IP-адреса IPv6
						return ::ipv6(ip, buffer, zone, ::makeOptions(this->_strict));
					}
				} break;
				// Если адрес принадлежит к URL-адресам
				case static_cast <uint8_t> (type_t::URL): {
					// Проверяем длину URL-адреса
					if(addr.length() < 8)
						// Если длина URL-адреса некорректна
						return false;
					/**
					 * Проверяем префикс URL-адреса
					 *
					 * @note Сравнение ведётся по представлению строки, а не по
					 *       указателю на её данные: представление длину знает,
					 *       а указатель нулевым символом завершён не обязан
					 *
					 */
					return (
						this->_fmk->compare("http://", addr.substr(0, 7)) ||
						this->_fmk->compare("https://", addr.substr(0, 8))
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
					// Unix: начинается с / или ~ или .
					return ((addr.front() == '/') || (addr.front() == '~') || (addr.front() == '.'));
				}
				// Если адрес принадлежит к доменным именам
				case static_cast <uint8_t> (type_t::FQDN): {
					// Создаём представление строки для проверки
					if(addr.length() > 253)
						// Если длина доменного имени превышает допустимое значение
						return false;
					/**
					 * Разрешаем localhost как особый случай
					 *
					 * @note Сравнение ведётся по представлению строки, а не по
					 *       указателю на её данные: указатель представления
					 *       нулевым символом завершён не обязан, и сравнение по
					 *       нему читало за пределами представления
					 *
					 */
					if(this->_fmk->compare("localhost", addr))
						// Если адрес равен localhost
						return true;
					// Начальное и конечное значение итератора
					size_t start = 0, end = 0;
					/**
					 * Выполняем проверку каждой метки доменного имени
					 */
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
						/**
						 * Проверяем каждый символ метки
						 */
						for(char c : label){
							// Если символ не является допустимым
							if(!(ascii::isAlnum(c) || (c == '-')))
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(addr, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат проверки адреса
	return false;
}
/**
 * @brief Метод наложения маски сети
 *
 * @param mask маска сети для наложения
 * @param addr тип получаемого адреса
 *
 */
void awh::Network_Address::impose(string_view mask, const addr_t addr) noexcept {
	// Выполняем наложение маски сети
	this->impose(mask, addr, this->_type);
}
/**
 * @brief Метод наложения маски сети
 *
 * @param mask маска сети для наложения
 * @param addr тип получаемого адреса
 * @param type тип адреса аппаратного или интернет подключения
 *
 */
void awh::Network_Address::impose(string_view mask, const addr_t addr, const type_t type) noexcept {
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
 *
 */
void awh::Network_Address::impose(const uint8_t prefix, const addr_t addr) noexcept {
	// Выполняем наложение префикса адреса
	this->impose(prefix, addr, this->_type);
}
/**
 * @brief Метод наложения префикса
 *
 * @param prefix префикс для наложения
 * @param addr   тип получаемого адреса
 * @param type   тип адреса аппаратного или интернет подключения
 *
 */
void awh::Network_Address::impose(const uint8_t prefix, const addr_t addr, const type_t type) noexcept {
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
						// Определяем номер октета (число полных октетов в префиксе)
						const uint8_t num = (prefix / 8);
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
									bitset <8> bits(oct);
									/**
									 * Зануляем все лишние элементы
									 */
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
									// Данные хекстета
									uint8_t oct = 0;
									// Получаем нужное нам значение октета
									::memcpy(&oct, &this->_buffer[0] + num, sizeof(oct));
									// Переводим октет в бинарный вид
									bitset <8> bits(oct);
									/**
									 * Зануляем все лишние элементы
									 */
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
						// Определяем номер хекстета (число полных хекстетов в префиксе)
						const uint8_t num = (prefix / 16);
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
									// Данные хекстета
									uint16_t hex = 0;
									// Получаем нужное нам значение хекстета
									::memcpy(&hex, &this->_buffer[0] + (num * 2), sizeof(hex));
									// Переводим хекстет в бинарный вид
									bitset <16> bits(hex);
									/**
									 * Зануляем все лишние элементы
									 */
									for(uint8_t i = (16 - (prefix % 16)); i < 16; i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение хекстета
									hex = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение хекстета
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
									// Данные хекстета
									uint16_t hex = 0;
									// Получаем нужное нам значение хекстета
									::memcpy(&hex, &this->_buffer[0] + (num * 2), sizeof(hex));
									// Переводим хекстет в бинарный вид
									bitset <16> bits(hex);
									/**
									 * Зануляем все лишние элементы
									 */
									for(uint8_t i = 0; i < (16 - (prefix % 16)); i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение хекстета
									hex = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение хекстета
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(prefix, static_cast <uint16_t> (addr), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
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
 *
 */
uint8_t awh::Network_Address::mask2Prefix(string_view mask) const noexcept {
	// Выполняем преобразование маски сети в префикс адреса
	return this->mask2Prefix(mask, this->_type);
}
/**
 * @brief Метод перевода маски сети в префикс адреса
 *
 * @param mask маска сети для перевода
 * @param type тип адреса аппаратного или интернет подключения
 * @return     полученный префикс адреса
 *
 */
uint8_t awh::Network_Address::mask2Prefix(string_view mask, const type_t type) const noexcept {
	// Переменная результата
	uint8_t result = 0;
	// Если маска сети передана
	if(!mask.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_addr_t net(this->_fmk, this->_log);
			// Выполняем парсинг маски
			if(net.parse(mask) && (type == net.type())){
				// Бинарный контейнер
				bitset <8> bits;
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (type)){
					// Если IP-адрес определён как IPv4
					case static_cast <uint8_t> (type_t::IPV4): {
						// Получаем значение маски в виде адреса
						const uint32_t num = net.v4();
						/**
						 * Выполняем перебор всего значения буфера
						 */
						for(uint8_t i = 0; i < 4; i++){
							// Переводим хекстет в бинарный вид
							bits = (reinterpret_cast <const uint8_t *> (&num))[i];
							// Выполняем подсчёт префикса
							result += bits.count();
						}
					} break;
					// Если IP-адрес определён как IPv6
					case static_cast <uint8_t> (type_t::IPV6): {
						// Получаем значение маски в виде адреса
						const array <uint8_t, 16> num = net.v6();
						/**
						 * Выполняем перебор всего значения буфера
						 */
						for(uint8_t i = 0; i < 16; i++){
							// Переводим хекстет в бинарный вид
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(mask, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод преобразования префикса адреса в маску сети
 *
 * @param prefix префикс адреса для преобразования
 * @return       полученная маска сети
 *
 */
string awh::Network_Address::prefix2Mask(const uint8_t prefix) const noexcept {
	// Выполняем перевод префикса адреса в маску сети
	return this->prefix2Mask(prefix, this->_type);
}
/**
 * @brief Метод преобразования префикса адреса в маску сети
 *
 * @param prefix префикс адреса для преобразования
 * @param type   тип адреса аппаратного или интернет подключения
 * @return       полученная маска сети
 *
 */
string awh::Network_Address::prefix2Mask(const uint8_t prefix, const type_t type) const noexcept {
	// Переменная результата
	string result = "";
	// Если маска сети передана
	if(prefix > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_addr_t net(this->_fmk, this->_log);
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
							// Возвращаем полученный адрес
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
							// Возвращаем полученный адрес
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 *
 */
bool awh::Network_Address::range(const Network_Address & begin, const Network_Address & end, string_view mask) const noexcept {
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
 *
 */
bool awh::Network_Address::range(const Network_Address & begin, const Network_Address & end, string_view mask, const type_t type) const noexcept {
	// Переменная результата
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
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 *
 */
bool awh::Network_Address::range(const Network_Address & begin, const Network_Address & end, const uint8_t prefix) const noexcept {
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
 *
 */
bool awh::Network_Address::range(const Network_Address & begin, const Network_Address & end, const uint8_t prefix, const type_t type) const noexcept {
	// Переменная результата
	bool result = false;
	// Если типы адресов совпадают
	if((type == begin.type()) && (type == end.type())){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объекты сетевых модулей
			net_addr_t net1(this->_fmk, this->_log),
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 *
 */
bool awh::Network_Address::range(string_view begin, string_view end, string_view mask) const noexcept {
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
 *
 */
bool awh::Network_Address::range(string_view begin, string_view end, string_view mask, const type_t type) const noexcept {
	// Переменная результата
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
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 *
 */
bool awh::Network_Address::range(string_view begin, string_view end, const uint8_t prefix) const noexcept {
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
 *
 */
bool awh::Network_Address::range(string_view begin, string_view end, const uint8_t prefix, const type_t type) const noexcept {
	// Переменная результата
	bool result = false;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && !begin.empty() && !end.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объекты сетевых модулей
			net_addr_t net1(this->_fmk, this->_log),
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(begin, end, static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @return        результат проверки
 *
 */
bool awh::Network_Address::mapping(string_view network) const noexcept {
	// Выполняем проверку соответствия IP-адреса указанной сети
	return this->mapping(network, this->_type);
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param type    тип адреса аппаратного или интернет подключения
 * @return        результат проверки
 *
 */
bool awh::Network_Address::mapping(string_view network, const type_t type) const noexcept {
	// Переменная результата
	bool result = false;
	// Если адрес сети передан
	if((result = !network.empty())){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_addr_t net(this->_fmk, this->_log);
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
							array <uint8_t, 4> nwk, addr;
							// Получаем значение адреса сети
							const uint32_t ip1 = net.v4();
							// Получаем значение текущего адреса
							const uint32_t ip2 = this->v4();
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&nwk[0], &ip1, sizeof(ip1));
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&addr[0], &ip2, sizeof(ip2));
							// Индекс последнего значащего октета сети (хвостовые нули трактуются как wildcard)
							int8_t last = -1;
							/**
							 * Ищем последний ненулевой октет сети
							 */
							for(int8_t i = 3; i >= 0; --i){
								// Если октет сети значащий
								if(nwk[i] != 0){
									// Запоминаем индекс и прекращаем поиск
									last = i;
									// Выходим из цикла
									break;
								}
							}
							// Сопоставляем все значащие октеты сети с адресом
							result = true;
							/**
							 * Перебираем значащие октеты сети
							 */
							for(int8_t i = 0; i <= last; ++i){
								// Если октет адреса не совпадает с октетом сети
								if(addr[i] != nwk[i]){
									// Фиксируем несоответствие и выходим
									result = false;
									// Выходим из цикла
									break;
								}
							}
						} break;
						// Если IP-адрес определён как IPv6
						case static_cast <uint8_t> (type_t::IPV6): {
							// Буфер данных текущего адреса
							array <uint16_t, 8> nwk, addr;
							// Получаем значение адреса сети
							const array <uint8_t, 16> & ip1 = net.v6();
							// Получаем значение текущего адреса
							const array <uint8_t, 16> & ip2 = this->v6();
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&nwk[0], &ip1[0], sizeof(ip1));
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&addr[0], &ip2[0], sizeof(ip2));
							// Индекс последнего значащего хекстета сети (хвостовые нули трактуются как wildcard)
							int8_t last = -1;
							/**
							 * Ищем последний ненулевой хекстет сети
							 */
							for(int8_t i = 7; i >= 0; --i){
								// Если хекстет сети значащий
								if(nwk[i] != 0){
									// Запоминаем индекс и прекращаем поиск
									last = i;
									// Выходим из цикла
									break;
								}
							}
							// Сопоставляем все значащие хекстеты сети с адресом
							result = true;
							/**
							 * Перебираем значащие хекстеты сети
							 */
							for(int8_t i = 0; i <= last; ++i){
								// Если хекстет адреса не совпадает с хекстетом сети
								if(addr[i] != nwk[i]){
									// Фиксируем несоответствие и выходим
									result = false;
									// Выходим из цикла
									break;
								}
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(network, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param mask    маска сети для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 *
 */
bool awh::Network_Address::mapping(string_view network, string_view mask, const addr_t addr) const noexcept {
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
 *
 */
bool awh::Network_Address::mapping(const string_view network, const string_view mask, const addr_t addr, const type_t type) const noexcept {
	// Переменная результата
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
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param prefix  префикс для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 *
 */
bool awh::Network_Address::mapping(string_view network, const uint8_t prefix, const addr_t addr) const noexcept {
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
 *
 */
bool awh::Network_Address::mapping(string_view network, const uint8_t prefix, const addr_t addr, const type_t type) const noexcept {
	// Переменная результата
	bool result = false;
	// Если адрес сети передан
	if((result = (!network.empty() && (prefix > 0)))){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_addr_t net(this->_fmk, this->_log);
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
							// Возвращаем результат проверки
							return (ip == nwk);
						}
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
							// Возвращаем результат проверки
							return (::memcmp(&ip[0], &nwk[0], sizeof(ip)) == 0);
						}
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(network, prefix, static_cast <uint16_t> (addr), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод определения принадлежности адреса
 *
 * @return флаг принадлежности адреса
 *
 */
awh::Network_Address::own_t awh::Network_Address::own() const noexcept {
	// Переменная результата
	own_t result = own_t::NONE;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_addr_t net(this->_fmk, this->_log);
			// Выполняем инициализацию списка локальных адресов
			const_cast <net_addr_t *> (this)->initLocalNet();
			// Выполняем группировку нужного нам вида адресов
			auto ret = this->_localsNet.equal_range(this->_type);
			/**
			 * Перебираем все локальные адреса
			 */
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
									return own_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return own_t::LAN;
							}
						// Если диапазон адресов для этой проверки не установлен
						} else {
							// Устанавливаем префикс сети
							net.impose(i->second.prefix, addr_t::NETWORK);
							// Если проверяемые сети совпадают
							if(net.v4() == i->second.begin->v4()){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return own_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return own_t::LAN;
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
									return own_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return own_t::LAN;
							}
						// Если диапазон адресов для этой проверки не установлен
						} else {
							// Устанавливаем префикс сети
							net.impose(i->second.prefix, addr_t::NETWORK);
							// Если проверяемые сети совпадают
							if(::memcmp(&net.v6()[0], &i->second.begin->v6()[0], 16) == 0){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return own_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return own_t::LAN;
							}
						}
					} break;
				}
			}
			// Если результат не определён
			if(result == own_t::NONE)
				// Устанавливаем, что файл ялвяется глобальным
				result = own_t::WAN;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Получение записи в формате ARPA
 *
 * @return запись в формате ARPA
 *
 */
string awh::Network_Address::arpa() const noexcept {
	// Переменная результата
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
				/**
				 * Переходим по всему массиву
				 */
				for(int8_t i = (static_cast <int8_t> (this->_buffer.size()) - 1); i > -1; i--){
					// Если строка уже существует, добавляем разделитель
					if(!result.empty())
						// Добавляем разделитель
						result.append(1, '.');
					// Добавляем текущий октет в результат
					result.append(::to_string(this->_buffer[i]));
				}
				// Добавляем запись ARPA
				result.append(".in-addr.arpa");
			} break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6): {
				/**
				 * @brief Шестнадцатеричные цифры
				 *
				 */
				static constexpr char hex[] = "0123456789abcdef";
				// 32 hex-цифры + 31 точка + ".ip6.arpa" = ~72 символов
				result.reserve(64 + 10);
				/**
				 * IPv6: каждый байт → две hex-цифры, в обратном порядке битов (но не байтов!)
				 * RFC 3596: каждый nibble (полубайт) отдельно, в обратном порядке байтов
				 */
				for(int8_t i = 15; i >= 0; --i){
					// Младший полубайт → сначала
					result.append(1, hex[this->_buffer[i] & 0x0F]);
					// Добавляем разделитель
					result.append(1, '.');
					// Старший полубайт
					result.append(1, hex[(this->_buffer[i] >> 4) & 0x0F]);
					// Добавляем разделитель
					result.append(1, '.');
				}
				// Добавляем запись ARPA
				result.append("ip6.arpa");
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки записи в формате ARPA
 *
 * @param addr адрес в формате ARPA (1.0.168.192.in-addr.arpa)
 * @return     результат установки записи
 *
 */
bool awh::Network_Address::arpa(string_view addr) noexcept {
	// Если запись передана
	if(!addr.empty() && (addr.size() > 13)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск суффикса записи
			size_t pos = addr.rfind(".arpa");
			// Если суффикс найден
			if(pos != string::npos){
				// Ещем следующую точку перед суффиксом
				pos = addr.rfind('.', pos - 1);
				// Если точка найдена
				if(pos != string::npos){
					// Если мы нашли суффикс IPv4
					if(this->_fmk->compare(".in-addr.arpa", addr.substr(pos))){
						// Проверяем, что перед суффиксом ровно 4 метки (3 точки), иначе формат неверен
						if(::count(addr.begin(), addr.begin() + pos, '.') != 3)
							// Возвращаем ошибку
							return false;
						// Выполняем очистку буфера данных
						this->_buffer.clear();
						// Выполняем инициализацию буфера
						this->_buffer.resize(4, 0);
						// Позиция разделителя и индекс октета
						size_t begin = 0, index = 3;
						/**
						 * Выполняем поиск разделителя
						 */
						for(uint8_t i = 0; i < (pos + 1); i++){
							/**
							 * Определяем текущий символ
							 */
							if((addr[i] == '.') || (i == pos)){
								// Извлекаем полученное число
								this->_buffer[index] = this->_fmk->atoi <uint8_t> (addr.data() + begin, i - begin);
								// Выполняем смещение
								begin = (i + 1);
								// Уменьшаем смещение индекса
								index--;
							}
						}
						// Устанавливаем тип адреса
						this->_type = type_t::IPV4;
						// Возвращаем true
						return true;
					// Если мы нашли суффикс IPv6
					} else if(this->_fmk->compare(".ip6.arpa", addr.substr(pos))) {
						// Извлекаем основную часть (без ".ip6.arpa")
						string_view data(addr.data(), addr.size() - 9);
						// Должно быть ровно 63 символа: 32 hex + 31 точка
						if(data.size() != 63)
							// Неверный формат записи
							return false;
						// Текущее значение символа
						char letter = 0;
						// Значение символа в числовом формате
						int32_t value = 0;
						// Позиция для прохода по строке и текущее значение индекса
						uint8_t pos = 0, index = 0;
						// Выполняем очистку буфера данных
						this->_buffer.clear();
						// Выполняем инициализацию буфера
						this->_buffer.resize(16, 0);
						/**
						 * Проходим по 32 nibble: 0..31
						 */
						for(uint8_t i = 0; i < 32; ++i){
							// Позиция символа (каждый nibble + точка, кроме последнего)
							pos = (i * 2);
							// Проверяем выход за границы строки
							if(pos >= static_cast <uint8_t> (data.size()))
								// Неверный формат записи
								return false;
							// Проверяем наличие точки (кроме последнего nibble)
							if((i < 31) && (((pos + 1) >= static_cast <uint8_t> (data.size())) || (data[pos + 1] != '.')))
								// Неверный формат записи
								return false;
							// Текущий символ
							letter = data[pos];
							// Преобразуем символ в числовой формат
							value = ascii::hexValue(letter);
							// Если преобразование не удалось
							if(value == -1)
								// Неверный формат записи
								return false;
							// Определяем, в какой байт и в какой nibble писать
							index = (15 - (i / 2)); // байты идут от 15 к 0
							// Записываем значение nibble в соответствующий байт
							if(i % 2 == 0)
								// Записываем младший nibble
								this->_buffer[index] = ((this->_buffer[index] & 0xF0) | value);
							// Иначе записываем старший nibble
							else this->_buffer[index] = ((this->_buffer[index] & 0x0F) | (value << 4));
						}
						// Устанавливаем тип адреса
						this->_type = type_t::IPV6;
						// Возвращаем true
						return true;
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выполняем очистку буфера данных
	} else this->_buffer.clear();
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод парсинга адреса
 *
 * @param addr адрес аппаратный или интернет подключения для парсинга
 * @return     результат работы парсинга
 *
 */
bool awh::Network_Address::parse(string_view addr) noexcept {
	// Если адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Выполняем полную проверку всех типов адресов
			 */
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
						this->_buffer.resize(4, 0);
						// Выполняем парсинг IPv4 адреса
						if(::ipv4(addr, this->_buffer.data(), ::makeOptions(this->_strict))){
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
						this->_buffer.resize(16, 0);
						// Выполняем парсинг IPv6 адреса
						if(::ipv6(addr, this->_buffer.data(), this->_zone, ::makeOptions(this->_strict))){
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
						this->_buffer.resize(6, 0);
						// Формируем нуль-терминированную строку (string_view может не быть завершён нулём)
						const string str(addr);
						// Выполняем парсинг MAC адреса
						const int32_t pos = ::sscanf(
							str.c_str(),
							"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
							&this->_buffer[0] + 0, &this->_buffer[0] + 1, &this->_buffer[0] + 2,
							&this->_buffer[0] + 3, &this->_buffer[0] + 4, &this->_buffer[0] + 5, &last
						);
						// Если MAC адрес удано распарсен
						if((pos == 6) && (static_cast <int32_t> (str.size()) == last)){
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выполняем очистку буфера данных
	} else this->_buffer.clear();
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод парсинга адреса
 *
 * @param addr адрес аппаратный или интернет подключения для парсинга
 * @param type тип адреса аппаратного или интернет подключения для парсинга
 * @return     результат работы парсинга
 *
 */
bool awh::Network_Address::parse(string_view addr, const type_t type) noexcept {
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
					this->_buffer.resize(4, 0);
					// Выполняем парсинг IPv4 адреса
					if(::ipv4(addr, this->_buffer.data(), ::makeOptions(this->_strict))){
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
					this->_buffer.resize(16, 0);
					// Выполняем парсинг IPv6 адреса
					if(::ipv6(addr, this->_buffer.data(), this->_zone, ::makeOptions(this->_strict))){
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
					this->_buffer.resize(6, 0);
					// Формируем нуль-терминированную строку (string_view может не быть завершён нулём)
					const string str(addr);
					// Выполняем парсинг MAC-адреса
					const int32_t pos = ::sscanf(
						str.c_str(),
						"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
						&this->_buffer[0] + 0, &this->_buffer[0] + 1, &this->_buffer[0] + 2,
						&this->_buffer[0] + 3, &this->_buffer[0] + 4, &this->_buffer[0] + 5, &last
					);
					// Если MAC-адрес удано распарсен
					if((pos == 6) && (static_cast <int32_t> (str.size()) == last)){
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
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(addr, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выполняем очистку буфера данных
	} else this->_buffer.clear();
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод извлечения данных IP-адреса
 *
 * @param size  размер формата формирования IP-адреса
 * @param flag  флаг форматирования IP-адреса
 * @param delim разделитель формата формирования IP-адреса
 * @return      сформированная строка IP-адреса
 *
 */
string awh::Network_Address::print(const format_size_t size, const format_flag_t flag, const char delim) const noexcept {
	// Переменная результата
	string result = "";
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Позиция записи
			int32_t pos = 0;
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
						/**
						 * Определяем размер формата вывода IPv4 адреса
						 */
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
											pos = ::sprintf(
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
					/**
					 * Определяем размер формата вывода IPv4 адреса
					 */
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
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%u%c%u%c%u%c%u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(8);
										// Выполняем получение IPv4 адреса в 16-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%X%c%X%c%X%c%X",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 8-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%o%c%o%c%o%c%o",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если разрешено выводить хвост адреса в формате IPv4
								case static_cast <uint8_t> (format_flag_t::HEX_IPV4): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
											&result[0],
											"%u%u%u%u",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(22);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
											&result[0],
											"::FFFF:%u%c%u%c%u%c%u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если разрешено выводить только формат IPv6
								case static_cast <uint8_t> (format_flag_t::HEX_IPV6): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = ':';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(8);
										// Выполняем получение IPv4 адреса в формате IPv6
										pos = ::sprintf(
											&result[0],
											"%X%X",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(16);
										// Выполняем получение IPv4 адреса в формате IPv6
										pos = ::sprintf(
											&result[0],
											"%c%cFFFF%c%X%c%X", delim, delim, delim,
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3]
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
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%02u%c%02u%c%02u%c%02u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(8);
										// Выполняем получение IPv4 адреса в 16-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%02X%c%02X%c%02X%c%02X",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 8-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%02o%c%02o%c%02o%c%02o",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если разрешено выводить хвост адреса в формате IPv4
								case static_cast <uint8_t> (format_flag_t::HEX_IPV4): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
											&result[0],
											"%02u%02u%02u%02u",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(22);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
											&result[0],
											"::FFFF:%02u%c%02u%c%02u%c%02u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если разрешено выводить только формат IPv6
								case static_cast <uint8_t> (format_flag_t::HEX_IPV6): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = ':';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(8);
										// Выполняем получение IPv4 адреса в формате IPv6
										pos = ::sprintf(
											&result[0],
											"%02X%02X",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(16);
										// Выполняем получение IPv4 адреса в формате IPv6
										pos = ::sprintf(
											&result[0],
											"%c%cFFFF%c%02X%c%02X", delim, delim, delim,
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3]
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
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%03u%c%03u%c%03u%c%03u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 16-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%03X%c%03X%c%03X%c%03X",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 8-м формате
										pos = ::sprintf(
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
										pos = ::sprintf(
											&result[0],
											"%03o%c%03o%c%03o%c%03o",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если разрешено выводить хвост адреса в формате IPv4
								case static_cast <uint8_t> (format_flag_t::HEX_IPV4): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = '.';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(12);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
											&result[0],
											"%03u%03u%03u%03u",
											this->_buffer[0], this->_buffer[1],
											this->_buffer[2], this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(22);
										// Выполняем получение IPv4 адреса в 10-м формате
										pos = ::sprintf(
											&result[0],
											"::FFFF:%03u%c%03u%c%03u%c%03u",
											this->_buffer[0], delim, this->_buffer[1], delim,
											this->_buffer[2], delim, this->_buffer[3]
										);
									}
								} break;
								// Если разрешено выводить только формат IPv6
								case static_cast <uint8_t> (format_flag_t::HEX_IPV6): {
									// Если разделитель установлен по умолчанию
									if(delim == -1)
										// Устанавливаем стандартный разделитель
										const_cast <char &> (delim) = ':';
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(8);
										// Выполняем получение IPv4 адреса в формате IPv6
										pos = ::sprintf(
											&result[0],
											"%04X%04X",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(16);
										// Выполняем получение IPv4 адреса в формате IPv6
										pos = ::sprintf(
											&result[0],
											"%c%cFFFF%c%04X%c%04X", delim, delim, delim,
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3]
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
					/**
					 * Определяем размер формата вывода IPv4 адреса
					 */
					switch(static_cast <uint8_t> (size)){
						// Если размер не указан
						case static_cast <uint8_t> (format_size_t::NONE):
						// Если размер указан как короткий
						case static_cast <uint8_t> (format_size_t::SHORT): {
							// Хексеты IPv6
							uint16_t hexets[8];
							/**
							 * 1. Преобразуем байты в 8 хекстетов
							 */
							for(uint8_t i = 0; i < 8; ++i)
								// Заполняем хекстеты из нашего бинарного буфера
								hexets[i] = ((static_cast <uint16_t> (this->_buffer[i * 2]) << 8) | this->_buffer[i * 2 + 1]);
							/**
							 * Определяем формат вывода IP-адреса
							 */
							switch(static_cast <uint8_t> (flag)){
								// Если формат не указан
								case static_cast <uint8_t> (format_flag_t::NONE):
								// Если формат указан в 16-м виде
								case static_cast <uint8_t> (format_flag_t::HEX):
								// Если разрешено выводить хвост адреса в формате IPv4
								case static_cast <uint8_t> (format_flag_t::HEX_IPV4):
								// Если разрешено выводить только формат IPv6
								case static_cast <uint8_t> (format_flag_t::HEX_IPV6): {
									// Перераспределяем объект результата под максимальный размер HEX-формата (x:x:x:x:x:x:w.x.y.z)
									result.resize(45);
									// 2. Проверка IPv4-встраивания (опционально)
									bool useIPv4 = false;
									// Если формат зеркального вещания IPv4 => IPv6 активен
									if((flag != format_flag_t::NONE) && (flag != format_flag_t::HEX_IPV6)){
										// Проверяем на зеркальное вещание IPv6 => IPv4
										useIPv4 = (
											(
												(hexets[0] == 0) && (hexets[1] == 0) &&
												(hexets[2] == 0) && (hexets[3] == 0) &&
												(hexets[4] == 0) && (hexets[5] == 0xFFFF)
											) || (
												(hexets[0] == 0) && (hexets[1] == 0) &&
												(hexets[2] == 0) && (hexets[3] == 0) &&
												(hexets[4] == 0) && (hexets[5] == 0)
											)
										);
									}
									// Если нужно использовать зеркальное вещание IPv6 => IPv4
									if(useIPv4 || (flag == format_flag_t::HEX_IPV4)){
										// Простой вывод для IPv4-суффикса
										if((hexets[0] == 0) && (hexets[1] == 0) && (hexets[2] == 0) && (hexets[3] == 0) && (hexets[4] == 0)){
											// Если хекстет 5 равен FFFF
											if(hexets[5] == 0xFFFF)
												// Возвращаем результат в формате ::FFFF:w.x.y.z
												pos = ::sprintf(&result[0], "%c%cFFFF%c%u.%u.%u.%u", delim, delim, delim, this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]);
											// Возвращаем результат в формате ::w.x.y.z
											else pos = ::sprintf(&result[0], "%c%c%u.%u.%u.%u", delim, delim, this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]);
										// Возвращаем результат в формате x:x:x:x:x:x:w.x.y.z
										} else pos = ::sprintf(&result[0], "%X%c%X%c%X%c%X%c%X%c%X%c%u.%u.%u.%u", hexets[0], delim, hexets[1], delim, hexets[2], delim, hexets[3], delim, hexets[4], delim, hexets[5], delim, this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]);
									// Если нужно использовать стандартный вывод IPv6
									} else
										// Формируем сжатую строку IPv6 в шестнадцатеричном виде
										pos = ::emitIPv6(&result[0], hexets, 'X', delim);
								} break;
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Перераспределяем объект результата под максимальный размер DECIMAL-формата (8*5 + 7)
									result.resize(48);
									// Формируем сжатую строку IPv6 в десятичном виде
									pos = ::emitIPv6(&result[0], hexets, 'u', delim);
								} break;
								// Если формат указан в 8-м виде
								case static_cast <uint8_t> (format_flag_t::OCTAL): {
									// Перераспределяем объект результата под максимальный размер OCTAL-формата (8*6 + 7)
									result.resize(56);
									// Формируем сжатую строку IPv6 в восьмеричном виде
									pos = ::emitIPv6(&result[0], hexets, 'o', delim);
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
								case static_cast <uint8_t> (format_flag_t::HEX):
								// Если разрешено выводить только формат IPv6
								case static_cast <uint8_t> (format_flag_t::HEX_IPV6): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(32);
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
								// Если разрешено выводить хвост адреса в формате IPv4
								case static_cast <uint8_t> (format_flag_t::HEX_IPV4): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(39);
										// Возвращаем результат в формате xxxxxxwx.y.z
										pos = ::sprintf(
											&result[0],
											"%02X%02X%02X%02X%02X%02X%u.%u.%u.%u",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3],
											(this->_buffer[4] << 8) | this->_buffer[5],
											(this->_buffer[6] << 8) | this->_buffer[7],
											(this->_buffer[8] << 8) | this->_buffer[9],
											(this->_buffer[10] << 8) | this->_buffer[11],
											this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(45);
										// Возвращаем результат в формате x:x:x:x:x:x:w.x.y.z
										pos = ::sprintf(
											&result[0],
											"%02X%c%02X%c%02X%c%02X%c%02X%c%02X%c%u.%u.%u.%u",
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3], delim,
											(this->_buffer[4] << 8) | this->_buffer[5], delim,
											(this->_buffer[6] << 8) | this->_buffer[7], delim,
											(this->_buffer[8] << 8) | this->_buffer[9], delim,
											(this->_buffer[10] << 8) | this->_buffer[11], delim,
											this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]
										);
									}
								} break;
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(45);
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
								case static_cast <uint8_t> (format_flag_t::HEX):
								// Если разрешено выводить только формат IPv6
								case static_cast <uint8_t> (format_flag_t::HEX_IPV6): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(32);
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
								// Если разрешено выводить хвост адреса в формате IPv4
								case static_cast <uint8_t> (format_flag_t::HEX_IPV4): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(39);
										// Возвращаем результат в формате xxxxxxwx.y.z
										pos = ::sprintf(
											&result[0],
											"%04X%04X%04X%04X%04X%04X%u.%u.%u.%u",
											(this->_buffer[0] << 8) | this->_buffer[1],
											(this->_buffer[2] << 8) | this->_buffer[3],
											(this->_buffer[4] << 8) | this->_buffer[5],
											(this->_buffer[6] << 8) | this->_buffer[7],
											(this->_buffer[8] << 8) | this->_buffer[9],
											(this->_buffer[10] << 8) | this->_buffer[11],
											this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]
										);
									// Если разделитель указан
									} else {
										// Перераспределяем объект результата
										result.resize(45);
										// Возвращаем результат в формате x:x:x:x:x:x:w.x.y.z
										pos = ::sprintf(
											&result[0],
											"%04X%c%04X%c%04X%c%04X%c%04X%c%04X%c%u.%u.%u.%u",
											(this->_buffer[0] << 8) | this->_buffer[1], delim,
											(this->_buffer[2] << 8) | this->_buffer[3], delim,
											(this->_buffer[4] << 8) | this->_buffer[5], delim,
											(this->_buffer[6] << 8) | this->_buffer[7], delim,
											(this->_buffer[8] << 8) | this->_buffer[9], delim,
											(this->_buffer[10] << 8) | this->_buffer[11], delim,
											this->_buffer[12], this->_buffer[13], this->_buffer[14], this->_buffer[15]
										);
									}
								} break;
								// Если формат указан в 10-м виде
								case static_cast <uint8_t> (format_flag_t::DECIMAL): {
									// Если разделитель не указан
									if(delim == 0){
										// Перераспределяем объект результата
										result.resize(45);
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
										// Выполняем получение IPv6 адреса в 16-м формате
										pos = ::sprintf(
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
			// Если позиция заполнения больше нуля
			if(pos > 0)
				// Обрезаем результат по фактической длине
				result.resize(pos);
			// Если результат не пустой
			if(!result.empty()){
				// Определяем размер строки
				const size_t length = result.length();
				// Позиции начала и конца обрезанной строки
				size_t i = 0, j = length;
				/**
				 * Выполняем обрезку пробелов в начале и конце строки
				 */
				while((i < j) && ((result[i] == '\0') || ascii::isSpace(result[i])))
					// Увеличиваем позицию начала строки
					++i;
				/**
				 * Обрезаем пробелы в конце строки
				 */
				while((j > i) && ((result[j - 1] == '\0') || ascii::isSpace(result[j - 1])))
					// Уменьшаем позицию конца строки
					--j;
				// Если необходимо удалить определённое количество символов с конца строки
				if(j < length)
					// Удаляем лишние символы с конца строки
					result.erase(j);
				// Если нужно удалить определённое количество символов с начала строки
				if(i > 0)
					// Удаляем лишние символы с начала строки
					result.erase(0, i);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					make_tuple(static_cast <uint16_t> (size), static_cast <uint16_t> (flag), static_cast <uint16_t> (delim)),
					log_t::flag_t::CRITICAL, error.what()
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Если тип адреса не определён
	} else {
		/**
		 * Определяем тип IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если - это не IP-адрес, а MAC-адрес
			case static_cast <uint8_t> (type_t::MAC):
				// Устанавливаем MAC-адрес по умолчанию
				result = "00:00:00:00:00:00";
			break;
			// Если IP-адрес определён как IPv4
			case static_cast <uint8_t> (type_t::IPV4):
				// Устанавливаем IPv4-адрес по умолчанию
				result = "0.0.0.0";
			break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6):
				// Устанавливаем IPv6-адрес по умолчанию
				result = "::";
			break;
		}
	}
	// Если результат не пустой и зона адреса определена
	if(!result.empty() && !this->_zone.empty())
		// Добавляем зону адреса к результату
		result.append('%' + this->_zone);
	// Возвращаем результат
	return result;
}
/**
 * @brief Оператор вывода IP-адреса в качестве строки
 *
 * @return IP-адрес в качестве строки
 *
 */
awh::Network_Address::operator string() const noexcept {
	// Возвращаем данные IP-адреса в виде строки
	return this->print();
}
/**
 * @brief Оператор [<] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 *
 */
bool awh::Network_Address::operator < (const net_addr_t & addr) const noexcept {
	// Переменная результата
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Оператор [>] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 *
 */
bool awh::Network_Address::operator > (const net_addr_t & addr) const noexcept {
	// Переменная результата
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Оператор [<=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 *
 */
bool awh::Network_Address::operator <= (const net_addr_t & addr) const noexcept {
	// Переменная результата
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Оператор [>=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 *
 */
bool awh::Network_Address::operator >= (const net_addr_t & addr) const noexcept {
	// Переменная результата
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Оператор [!=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 *
 */
bool awh::Network_Address::operator != (const net_addr_t & addr) const noexcept {
	// Возвращаем результат как инверсию оператора равенства (учитывает совпадение типов)
	return !(* this == addr);
}
/**
 * @brief Оператор [==] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 *
 */
bool awh::Network_Address::operator == (const net_addr_t & addr) const noexcept {
	// Переменная результата
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Оператор присваивания присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 *
 */
awh::Network_Address & awh::Network_Address::operator = (const net_addr_t & addr) noexcept {
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор присваивания присвоения IP-адреса
 *
 * @param ip адрес для присвоения
 * @return   текущий объект
 *
 */
awh::Network_Address & awh::Network_Address::operator = (string_view ip) noexcept {
	// Выполняем установку IP-адреса
	this->parse(ip);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор присваивания установки типа IP-адреса
 *
 * @param type тип IP-адреса для установки
 * @return     текущий объект
 *
 */
awh::Network_Address & awh::Network_Address::operator = (const type_t type) noexcept {
	// Устанавливаем тип IP-адреса
	this->type(type);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор присваивания присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 *
 */
awh::Network_Address & awh::Network_Address::operator = (const uint32_t addr) noexcept {
	// Устанавливаем IPv4
	this->v4(addr);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор присваивания присвоения MAC-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 *
 */
awh::Network_Address & awh::Network_Address::operator = (const array <uint8_t, 6> & addr) noexcept {
	// Устанавливаем MAC-адрес
	this->mac(addr);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор присваивания присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 *
 */
awh::Network_Address & awh::Network_Address::operator = (const array <uint8_t, 16> & addr) noexcept {
	// Устанавливаем IPv6
	this->v6(addr);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Network_Address::Network_Address(const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _strict(false), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Network_Address::~Network_Address() noexcept {}
/**
 * @brief Оператор [>>] чтения из потока IP-адреса
 *
 * @param is   поток для чтения
 * @param addr адрес для присвоения
 *
 */
istream & awh::operator >> (istream & is, net_addr_t & addr) noexcept {
	// Адрес интернет-подключения
	string ip = "";
	// Считываем адрес интернет-подключения
	is >> ip;
	// Если адрес интернет-подключения получен
	if(!ip.empty())
		// Устанавливаем IP-адрес
		addr.parse(ip);
	// Возвращаем результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток IP-адреса
 *
 * @param os   поток куда нужно вывести данные
 * @param addr адрес для присвоения
 *
 */
ostream & awh::operator << (ostream & os, const net_addr_t & addr) noexcept {
	// Записываем в поток IP-адрес
	os << addr.print();
	// Возвращаем результат
	return os;
}
