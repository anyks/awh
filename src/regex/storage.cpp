/**
 * @file: storage.cpp
 * @date: 2026-08-04
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Файл реализации хранилища собранных регулярных выражений —
 *        запись собранных выражений последовательностью байтов и восстановление
 *        их из записи без повторного разбора и компиляции
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <regex/storage.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Наибольший допустимый размер записи хранилища
 *
 * @details Размер ограничен затем, чтобы запись испорченная, объявляющая
 *          количество несообразное, не привела к попытке размещения памяти
 *          объёмом непомерным до чтения самого содержимого.
 *
 */
static constexpr size_t MAX_STORAGE = (1024ull * 1024ull * 1024ull);

/**
 * @brief Наибольшее допустимое количество выражений записи
 *
 */
static constexpr uint32_t MAX_EXPRESSIONS = 0x400000;

/**
 * @brief Пространство имён вспомогательных функций записи и чтения
 *
 */
namespace {
	/**
	 * @brief Функция записи восьмиразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write8(const uint8_t value, string & result) noexcept {
		// Выполняем запись числа
		result.push_back(static_cast <char> (value));
	}
	/**
	 * @brief Функция записи шестнадцатиразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write16(const uint16_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 16; shift += 8)
			// Выполняем запись очередного байта числа
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	/**
	 * @brief Функция записи тридцатидвухразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write32(const uint32_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 32; shift += 8)
			// Выполняем запись очередного байта числа
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	/**
	 * @brief Функция записи шестидесятичетырёхразрядного числа
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	inline void write64(const uint64_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 64; shift += 8)
			// Выполняем запись очередного байта числа
			result.push_back(static_cast <char> ((value >> shift) & 0xFF));
	}
	/**
	 * @brief Функция записи числа переменной длины
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 * @details Число записывается семью разрядами на байт, старший разряд байта
	 *          отмечает продолжение. Значения малые - а таковы почти все адреса,
	 *          номера и границы диапазонов - занимают один-два байта взамен
	 *          четырёх, отчего запись сокращается втрое, а чтение ускоряется
	 *          соразмерно объёму прочитанного.
	 *
	 */
	inline void writeVar(uint32_t value, string & result) noexcept {
		/**
		 * Выполняем запись числа семиразрядными долями
		 */
		while(value >= 0x80) {
			// Выполняем запись очередной доли числа с отметкой продолжения
			result.push_back(static_cast <char> ((value & 0x7F) | 0x80));
			// Выполняем сдвиг числа к следующей доле
			value >>= 7;
		}
		// Выполняем запись последней доли числа
		result.push_back(static_cast <char> (value));
	}
	/**
	 * @brief Функция чтения числа переменной длины
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool readVar(string_view data, size_t & offset, uint32_t & value) noexcept {
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа семиразрядными долями
		 */
		for(uint8_t shift = 0; shift < 35; shift += 7) {
			/**
			 * Если запись оборвана до завершения числа
			 */
			if(offset >= data.size())
				// Выводим результат чтения числа
				return false;
			// Получаем очередной байт числа
			const uint8_t letter = static_cast <uint8_t> (data[offset++]);
			/**
			 * Если доля числа разрядность его превышает
			 */
			if((shift == 28) && ((letter & 0x7F) > 0x0F))
				// Выводим результат чтения числа
				return false;
			// Выполняем добавление доли числа
			value |= (static_cast <uint32_t> (letter & 0x7F) << shift);
			/**
			 * Если байт продолжения не отмечает
			 */
			if((letter & 0x80) == 0)
				// Выводим результат чтения числа
				return true;
		}
		// Выводим результат чтения числа
		return false;
	}
	/**
	 * @brief Функция чтения числа переменной длины без проверки границ
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @return       прочитанное число
	 *
	 * @details Проверка границ вынесена наружу и выполняется единожды на блок
	 *          записей: она обходится дороже самого чтения, а блок известной
	 *          длины проверяется одним сравнением. Вызывать эту функцию
	 *          дозволено лишь после проверки, что в записи довольно байтов на
	 *          весь блок с запасом наибольшей длины числа.
	 *
	 */
	inline uint32_t readFast(string_view data, size_t & offset) noexcept {
		// Получаем первый байт числа
		uint8_t letter = static_cast <uint8_t> (data[offset++]);
		/**
		 * Если число уместилось в один байт
		 */
		if((letter & 0x80) == 0)
			// Выводим прочитанное число
			return static_cast <uint32_t> (letter);
		// Накопленное значение числа
		uint32_t result = static_cast <uint32_t> (letter & 0x7F);
		/**
		 * Выполняем чтение оставшихся долей числа
		 */
		for(uint8_t shift = 7; shift < 35; shift += 7) {
			// Получаем очередной байт числа
			letter = static_cast <uint8_t> (data[offset++]);
			// Выполняем добавление доли числа
			result |= (static_cast <uint32_t> (letter & 0x7F) << shift);
			/**
			 * Если байт продолжения не отмечает
			 */
			if((letter & 0x80) == 0)
				// Выводим прочитанное число
				return result;
		}
		// Выводим прочитанное число
		return result;
	}
	/**
	 * @brief Функция записи последовательности символов
	 *
	 * @param value  записываемая последовательность
	 * @param result запись хранилища
	 *
	 */
	inline void writeText(const string & value, string & result) noexcept {
		// Выполняем запись длины последовательности
		writeVar(static_cast <uint32_t> (value.size()), result);
		// Выполняем запись содержимого последовательности
		result.append(value);
	}
	/**
	 * @brief Функция чтения восьмиразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read8(string_view data, size_t & offset, uint8_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 1) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем чтение числа
		value = static_cast <uint8_t> (data[offset++]);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения шестнадцатиразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read16(string_view data, size_t & offset, uint16_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 2) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 16; shift += 8)
			// Выполняем чтение очередного байта числа
			value |= (static_cast <uint16_t> (static_cast <uint8_t> (data[offset++])) << shift);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения тридцатидвухразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read32(string_view data, size_t & offset, uint32_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 4) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 32; shift += 8)
			// Выполняем чтение очередного байта числа
			value |= (static_cast <uint32_t> (static_cast <uint8_t> (data[offset++])) << shift);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения шестидесятичетырёхразрядного числа
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	inline bool read64(string_view data, size_t & offset, uint64_t & value) noexcept {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if((offset + 8) > data.size())
			// Выводим результат чтения числа
			return false;
		// Выполняем сброс прочитанного числа
		value = 0;
		/**
		 * Выполняем чтение числа байтами от младшего
		 */
		for(uint8_t shift = 0; shift < 64; shift += 8)
			// Выполняем чтение очередного байта числа
			value |= (static_cast <uint64_t> (static_cast <uint8_t> (data[offset++])) << shift);
		// Выводим результат чтения числа
		return true;
	}
	/**
	 * @brief Функция чтения последовательности символов
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанная последовательность
	 * @return       результат чтения последовательности
	 *
	 */
	inline bool readText(string_view data, size_t & offset, string & value) noexcept {
		// Длина прочитанной последовательности
		uint32_t length = 0;
		/**
		 * Если чтение длины последовательности не выполнено
		 */
		if(!readVar(data, offset, length))
			// Выводим результат чтения последовательности
			return false;
		/**
		 * Если запись оборвана до завершения последовательности
		 */
		if((offset + static_cast <size_t> (length)) > data.size())
			// Выводим результат чтения последовательности
			return false;
		// Выполняем чтение содержимого последовательности
		value.assign(data.substr(offset, length));
		// Выполняем перенос позиции чтения записи
		offset += static_cast <size_t> (length);
		// Выводим результат чтения последовательности
		return true;
	}
	/**
	 * @brief Функция опознания устройства машины
	 *
	 * @return опознание устройства машины
	 *
	 * @details Опознание собирается из порядка байтов машины и размеров
	 *          структур, образом памяти записываемых. Запись, порождённая
	 *          машиной устройства иного, прочитана была бы неверно, поэтому
	 *          восстановление сличает опознание со своим.
	 *
	 */
	inline uint16_t platform() noexcept {
		// Проба порядка байтов машины
		const uint32_t probe = 0x01020304;
		// Собираемое опознание устройства машины
		uint16_t result = static_cast <uint16_t> ((* reinterpret_cast <const uint8_t *> (&probe)) & 0x0F);
		// Выполняем добавление размера инструкции программы
		result = static_cast <uint16_t> (result | (static_cast <uint16_t> (sizeof(awh::regex::instruction_t)) << 4));
		// Выполняем добавление размера диапазона кодовых значений
		result = static_cast <uint16_t> (result ^ (static_cast <uint16_t> (sizeof(awh::regex::range_t)) << 10));
		// Выполняем добавление размера ссылки на класс символов
		result = static_cast <uint16_t> (result ^ (static_cast <uint16_t> (sizeof(awh::regex::classref_t)) << 12));
		// Выполняем добавление размера ссылки на свойство Юникода
		result = static_cast <uint16_t> (result ^ (static_cast <uint16_t> (sizeof(awh::regex::property_t)) << 14));
		// Выводим опознание устройства машины
		return result;
	}
	/**
	 * @brief Функция выравнивания записи по границе восьми байтов
	 *
	 * @param result запись хранилища
	 *
	 */
	inline void align(string & result) noexcept {
		/**
		 * Выполняем добавление байтов заполнения до границы
		 */
		while((result.size() % 8) != 0)
			// Выполняем добавление байта заполнения
			result.push_back('\0');
	}
	/**
	 * @brief Функция записи набора образом памяти
	 *
	 * @tparam T       тип записи набора
	 * @param  records записываемый набор
	 * @param  result  запись хранилища
	 *
	 * @details Набор пишется образом памяти затем, чтобы восстановление его
	 *          свелось к установке обзора на участок записи, минуя размещение
	 *          и перенос. Участок выравнивается по границе восьми байтов:
	 *          обзор выдаёт записи типом, а обращение к ним по указанию
	 *          невыровненному допустимо не всякой машиной.
	 *
	 */
	template <typename T>
	inline void writeRegion(const awh::regex::Sequence <T> & records, string & result) noexcept {
		// Выполняем запись количества записей набора
		writeVar(static_cast <uint32_t> (records.size()), result);
		// Выполняем выравнивание записи по границе восьми байтов
		align(result);
		/**
		 * Если набор записей не пуст
		 */
		if(!records.empty())
			// Выполняем запись набора образом памяти
			result.append(reinterpret_cast <const char *> (records.data()), (records.size() * sizeof(T)));
	}
	/**
	 * @brief Функция восстановления набора из образа памяти
	 *
	 * @tparam T       тип записи набора
	 * @param  data    запись хранилища
	 * @param  offset  позиция чтения записи
	 * @param  limit   наибольшее допустимое количество записей набора
	 * @param  records восстанавливаемый набор
	 * @return         результат восстановления набора
	 *
	 * @details Набор восстанавливается обзором участка записи, если участок
	 *          выровнен по границе, требуемой типом записи. Выравнивание
	 *          соблюдается записью, но запись приходит извне и начало её
	 *          в памяти границе отвечать не обязано, поэтому невыровненный
	 *          участок переносится в собственное содержимое набора.
	 *
	 */
	template <typename T>
	inline bool readRegion(string_view data, size_t & offset, const size_t limit, awh::regex::Sequence <T> & records) noexcept {
		// Количество записей восстанавливаемого набора
		uint32_t count = 0;
		/**
		 * Если чтение количества записей набора не выполнено
		 */
		if(!readVar(data, offset, count))
			// Выводим результат восстановления набора
			return false;
		/**
		 * Если количество записей набора превышает допустимое
		 */
		if(static_cast <size_t> (count) > limit)
			// Выводим результат восстановления набора
			return false;
		/**
		 * Выполняем пропуск байтов заполнения до границы восьми байтов
		 */
		while((offset % 8) != 0) {
			/**
			 * Если запись оборвана до границы выравнивания
			 */
			if(offset >= data.size())
				// Выводим результат восстановления набора
				return false;
			// Переходим к следующему байту записи
			offset++;
		}
		// Получаем размер участка записи, занимаемого набором
		const size_t size = (static_cast <size_t> (count) * sizeof(T));
		/**
		 * Если участок записи за её пределы выходит
		 */
		if(size > (data.size() - offset))
			// Выводим результат восстановления набора
			return false;
		// Получаем указание на начало участка записи
		const char * region = (data.data() + offset);
		/**
		 * Если участок записи границе, требуемой типом записи, отвечает
		 */
		if((reinterpret_cast <uintptr_t> (region) % alignof(T)) == 0)
			// Выполняем установку обзора участка записи
			records.attach(reinterpret_cast <const T *> (region), static_cast <size_t> (count));
		/**
		 * Если участок записи границе не отвечает
		 */
		else {
			// Выполняем очистку восстанавливаемого набора
			records.clear();
			// Выполняем размещение записей набора
			records.resize(static_cast <size_t> (count));
			/**
			 * Если набор записей не пуст
			 */
			if(count > 0)
				// Выполняем перенос участка записи в содержимое набора
				::memcpy(&records.at(0), region, size);
		}
		// Переходим за участок записи, занимаемый набором
		offset += size;
		// Выводим результат восстановления набора
		return true;
	}
	/**
	 * @brief Функция вычисления контрольной суммы записи
	 *
	 * @param data запись хранилища
	 * @return     вычисленная контрольная сумма записи
	 *
	 * @details Сумма служит обнаружению порчи записи, а не защите от подмены
	 *          намеренной: восстановление проверяет содержимое поля за полем и
	 *          на сумму в этом не полагается.
	 *
	 */
	inline uint64_t checksum(string_view data) noexcept {
		// Накопленная контрольная сумма записи
		uint64_t result = 0xCBF29CE484222325ull;
		// Получаем позицию чтения записи
		size_t offset = 0;
		/**
		 * Выполняем перебор записи восьмибайтовыми долями
		 *
		 * @details Сумма считается словами, а не байтами: запись достигает
		 *          мегабайтов, и побайтовый обход её обходился дороже разбора
		 *          всего содержимого. Доли собираются из байтов явно, а не
		 *          переносом памяти, дабы значение суммы не зависело от порядка
		 *          байтов машины и запись оставалась переносимой.
		 */
		for(; (offset + 8) <= data.size(); offset += 8) {
			// Собираемая доля записи
			uint64_t block = 0;
			/**
			 * Выполняем сборку доли записи байтами от младшего
			 */
			for(uint8_t shift = 0; shift < 64; shift += 8)
				// Выполняем добавление очередного байта доли
				block |= (static_cast <uint64_t> (static_cast <uint8_t> (data[offset + (shift >> 3)])) << shift);
			// Выполняем смешивание доли записи
			result ^= block;
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
			// Выполняем перемешивание накопленной суммы
			result ^= (result >> 29);
		}
		/**
		 * Выполняем перебор остатка записи побайтно
		 */
		for(; offset < data.size(); offset++) {
			// Выполняем смешивание очередного байта записи
			result ^= static_cast <uint64_t> (static_cast <uint8_t> (data[offset]));
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
		}
		// Выводим вычисленную контрольную сумму записи
		return result;
	}
};

/**
 * @brief Метод записи программы регулярного выражения
 *
 * @param program записываемая программа
 * @param result  запись хранилища
 *
 */
void awh::regex::Storage::save(const program_t & program, string & result) const noexcept {
	// Выполняем запись опознания программы
	write64(program.id, result);
	// Выполняем запись количества захватывающих групп
	writeVar(program.captures, result);
	// Выполняем запись количества ячеек состояния
	writeVar(program.cells, result);
	// Выполняем запись набора режимов компиляции
	writeVar(program.flags, result);
	// Выполняем запись признаков программы
	write8(static_cast <uint8_t> (program.plain ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.sweeping ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.anchored ? 1 : 0), result);
	// Выполняем запись последовательности символов выражения
	writeText(program.text, result);
	// Выполняем запись признаков предварительного отбора позиций
	write8(static_cast <uint8_t> (program.prefilter.active ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.prefilter.utf ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.prefilter.unique ? 1 : 0), result);
	write8(static_cast <uint8_t> (program.prefilter.letter), result);
	/**
	 * Выполняем перебор набора допустимых начальных байтов
	 *
	 * @details Набор пишется битовой картой, а не байтом на признак: признаков
	 *          в нём двести пятьдесят шесть на каждую программу, и запись их
	 *          порознь обходилась восьмикратно дороже как местом, так и
	 *          временем чтения - на каждый признак приходилась своя проверка
	 *          границ записи.
	 */
	for(size_t i = 0; i < 256; i += 8) {
		// Собираемая доля битовой карты
		uint8_t block = 0;
		/**
		 * Выполняем сборку доли битовой карты
		 */
		for(uint8_t bit = 0; bit < 8; bit++)
			// Выполняем установку признака допустимости байта
			block |= (program.prefilter.bytes[i + bit] ? static_cast <uint8_t> (1 << bit) : 0);
		// Выполняем запись доли битовой карты
		write8(block, result);
	}
	// Выполняем запись литерала, присутствующего в любом совпадении
	writeText(program.prefilter.literal, result);
	// Выполняем запись последовательности начала любого совпадения
	writeText(program.prefilter.leading, result);
	// Выполняем запись набора инструкций программы образом памяти
	writeRegion(program.instructions, result);
	// Выполняем запись хранилища ссылок на классы символов образом памяти
	writeRegion(program.classes, result);
	// Выполняем запись сплошного набора диапазонов образом памяти
	writeRegion(program.ranges, result);
	// Выполняем запись сплошного набора свойств Юникода образом памяти
	writeRegion(program.properties, result);
	// Выполняем запись хранилища последовательностей символов образом памяти
	writeRegion(program.strings, result);
}
/**
 * @brief Метод восстановления программы регулярного выражения
 *
 * @param data    запись хранилища
 * @param offset  позиция чтения записи
 * @param program восстанавливаемая программа
 * @return        результат восстановления программы
 *
 */
bool awh::regex::Storage::load(string_view data, size_t & offset, program_t & program) const noexcept {
	// Выполняем очистку восстанавливаемой программы
	program.clear();
	// Признак программы, читаемый записью
	uint8_t flag = 0;
	/**
	 * Если чтение полей программы не выполнено
	 */
	if(!read64(data, offset, program.id) ||
	 !readVar(data, offset, program.captures) ||
	 !readVar(data, offset, program.cells) ||
	 !readVar(data, offset, program.flags)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Выполняем чтение признаков программы
	 */
	for(uint8_t pass = 0; pass < 3; pass++) {
		/**
		 * Если чтение очередного признака программы не выполнено
		 */
		if(!read8(data, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
		/**
		 * Определяем читаемый признак программы
		 */
		switch(pass) {
			// Выполняем установку признака выражения, сопоставляемого литералом
			case 0: program.plain = (flag != 0); break;
			// Выполняем установку признака прохода текста единственной попыткой
			case 1: program.sweeping = (flag != 0); break;
			// Выполняем установку признака привязки к позиции начала поиска
			case 2: program.anchored = (flag != 0); break;
		}
	}
	/**
	 * Если чтение последовательности символов выражения не выполнено
	 */
	if(!readText(data, offset, program.text)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Выполняем чтение признаков предварительного отбора позиций
	 */
	for(uint8_t pass = 0; pass < 4; pass++) {
		/**
		 * Если чтение очередного признака отбора позиций не выполнено
		 */
		if(!read8(data, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
		/**
		 * Определяем читаемый признак предварительного отбора позиций
		 */
		switch(pass) {
			// Выполняем установку признака действия отбора позиций
			case 0: program.prefilter.active = (flag != 0); break;
			// Выполняем установку признака разбора текста как UTF-8
			case 1: program.prefilter.utf = (flag != 0); break;
			// Выполняем установку признака единственного начального байта
			case 2: program.prefilter.unique = (flag != 0); break;
			// Выполняем установку единственного начального байта
			case 3: program.prefilter.letter = flag; break;
		}
	}
	/**
	 * Выполняем чтение битовой карты допустимых начальных байтов
	 */
	for(size_t i = 0; i < 256; i += 8) {
		// Читаемая доля битовой карты
		uint8_t block = 0;
		/**
		 * Если чтение доли битовой карты не выполнено
		 */
		if(!read8(data, offset, block)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления программы
			return false;
		}
		/**
		 * Выполняем разбор доли битовой карты
		 */
		for(uint8_t bit = 0; bit < 8; bit++)
			// Выполняем установку признака допустимости байта
			program.prefilter.bytes[i + bit] = (((block >> bit) & 0x01) != 0);
	}
	/**
	 * Если чтение последовательностей предварительного отбора не выполнено
	 */
	if(!readText(data, offset, program.prefilter.literal) ||
	 !readText(data, offset, program.prefilter.leading)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если восстановление набора инструкций программы не выполнено
	 */
	if(!readRegion(data, offset, MAX_PROGRAM, program.instructions)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если восстановление хранилища ссылок на классы символов не выполнено
	 */
	if(!readRegion(data, offset, MAX_PROGRAM, program.classes)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если восстановление сплошного набора диапазонов не выполнено
	 */
	if(!readRegion(data, offset, MAX_STORAGE, program.ranges)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если восстановление сплошного набора свойств Юникода не выполнено
	 */
	if(!readRegion(data, offset, MAX_STORAGE, program.properties)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если восстановление хранилища последовательностей символов не выполнено
	 */
	if(!readRegion(data, offset, MAX_STORAGE, program.strings)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Выводим результат восстановления программы
	return true;
}
/**
 * @brief Метод проверки правильности программы регулярного выражения
 *
 * @param program проверяемая программа
 * @return        результат проверки правильности программы
 *
 */
bool awh::regex::Storage::verify(const program_t & program) const noexcept {
	// Получаем количество инструкций программы
	const size_t count = program.instructions.size();
	/**
	 * @brief Проверка принадлежности адреса программе
	 *
	 * @param address проверяемый адрес инструкции
	 * @return        результат проверки принадлежности адреса программе
	 *
	 */
	auto inside = [&](const address_t address) noexcept -> bool {
		// Выводим результат проверки принадлежности адреса программе
		return ((address == INVALID_ADDRESS) || (static_cast <size_t> (address) < count));
	};
	/**
	 * Выполняем перебор набора инструкций программы
	 */
	for(const auto & instruction : program.instructions) {
		/**
		 * Определяем код операции инструкции программы
		 */
		switch(static_cast <uint8_t> (instruction.type)) {
			/**
			 * Если инструкция сопоставляет символ из класса символов
			 */
			case static_cast <uint8_t> (opcode_t::CLASS): {
				/**
				 * Если указание на класс символов хранилищу не принадлежит
				 */
				if(static_cast <size_t> (instruction.charclass.index) >= program.classes.size())
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет переход по двум ветвям
			 */
			case static_cast <uint8_t> (opcode_t::SPLIT): {
				/**
				 * Если адреса ветвей программе не принадлежат
				 */
				if(!inside(instruction.split.first) || !inside(instruction.split.second) ||
				 !inside(instruction.split.run) || !inside(instruction.split.lazy))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет безусловный переход
			 */
			case static_cast <uint8_t> (opcode_t::JUMP): {
				/**
				 * Если адрес перехода программе не принадлежит
				 */
				if(!inside(instruction.jump.target))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция запоминает границу захвата
			 */
			case static_cast <uint8_t> (opcode_t::SAVE): {
				/**
				 * Если номер места границы захвата выражению не принадлежит
				 *
				 * @details Набор мест несёт границы захвата и ячейки состояния
				 *          подряд, а инструкция сохранения позиции пишет и в те,
				 *          и в другие: атомарная группа сохраняет позицию начала
				 *          в ячейке, а не в границе захвата. Поэтому проверяется
				 *          принадлежность всему набору мест, а не одним лишь
				 *          границам захвата.
				 */
				if(static_cast <size_t> (instruction.save.slot) >=
				 (((static_cast <size_t> (program.captures) + 1) * 2) + static_cast <size_t> (program.cells)))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция отмечает либо откатывает точки возврата
			 */
			case static_cast <uint8_t> (opcode_t::MARK):
			case static_cast <uint8_t> (opcode_t::CUT): {
				/**
				 * Если номер ячейки отметки несообразен
				 *
				 * @details Ячейки отметок атомарных групп ведутся счётчиком своим,
				 *          отдельным от ячеек состояния повторений, и в программе
				 *          количество их не хранится: исполнение с возвратом
				 *          наращивает набор отметок по мере надобности. Поэтому
				 *          проверяется лишь сообразность номера - отметок в
				 *          выражении не более, чем инструкций.
				 */
				if(static_cast <size_t> (instruction.atomic.cell) >= MAX_PROGRAM)
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция сопоставляет захваченный текст
			 */
			case static_cast <uint8_t> (opcode_t::BACKREF): {
				/**
				 * Если номер группы выражению не принадлежит
				 */
				if(instruction.backref.number > program.captures)
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция проверяет продвижение по тексту
			 */
			case static_cast <uint8_t> (opcode_t::PROGRESS): {
				/**
				 * Если номер ячейки либо адрес завершения не принадлежат выражению
				 *
				 * @details Ячейки состояния повторений размещаются в наборе мест
				 *          следом за границами захвата, поэтому номер ячейки
				 *          отсчитывается от размера набора границ.
				 */
				if((static_cast <size_t> (instruction.progress.cell) >=
				 (((static_cast <size_t> (program.captures) + 1) * 2) + static_cast <size_t> (program.cells))) ||
				 !inside(instruction.progress.target))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет проверку окружения
			 */
			case static_cast <uint8_t> (opcode_t::LOOK): {
				/**
				 * Если адреса проверки окружения программе не принадлежат
				 */
				if(!inside(instruction.look.body) || !inside(instruction.look.target) ||
				 !inside(instruction.look.alternate))
					// Выводим результат проверки правильности программы
					return false;
				/**
				 * Если границы длины сопоставляемого текста несообразны
				 */
				if(instruction.look.least > instruction.look.most)
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет рекурсивный вызов подвыражения
			 */
			case static_cast <uint8_t> (opcode_t::CALL): {
				/**
				 * Если адрес тела либо номер группы выражению не принадлежат
				 */
				if(!inside(instruction.call.body) || (instruction.call.number > program.captures))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция выполняет условное выражение
			 */
			case static_cast <uint8_t> (opcode_t::CONDITION): {
				/**
				 * Если адреса ветвей условия программе не принадлежат
				 */
				if(!inside(instruction.condition.positive) || !inside(instruction.condition.negative))
					// Выводим результат проверки правильности программы
					return false;
				/**
				 * Если номер группы условия выражению не принадлежит
				 */
				if((instruction.condition.type == test_t::CAPTURED) &&
				 (instruction.condition.number > program.captures))
					// Выводим результат проверки правильности программы
					return false;
			} break;
		}
	}
	/**
	 * Выполняем перебор хранилища ссылок на классы символов
	 *
	 * @details Ссылка указывает на участок сплошного набора диапазонов и на
	 *          участок сплошного набора свойств. Обзор класса выдаётся по ней
	 *          без проверки границ, поскольку обращение к нему идёт на каждом
	 *          символе, поэтому принадлежность участков наборам удостоверяется
	 *          здесь, до всякого сопоставления.
	 *
	 */
	for(const auto & value : program.classes) {
		/**
		 * Если участок набора диапазонов набору не принадлежит
		 */
		if((static_cast <size_t> (value.ranges) + static_cast <size_t> (value.rangeCount)) > program.ranges.size())
			// Выводим результат проверки правильности программы
			return false;
		/**
		 * Если участок набора свойств набору не принадлежит
		 */
		if((static_cast <size_t> (value.properties) + static_cast <size_t> (value.propertyCount)) > program.properties.size())
			// Выводим результат проверки правильности программы
			return false;
	}
	/**
	 * Если количество ячеек состояния превышает допустимое
	 */
	if(static_cast <size_t> (program.cells) > MAX_PROGRAM)
		// Выводим результат проверки правильности программы
		return false;
	/**
	 * Если количество захватывающих групп превышает допустимое
	 */
	if(static_cast <size_t> (program.captures) > MAX_PROGRAM)
		// Выводим результат проверки правильности программы
		return false;
	// Выводим результат проверки правильности программы
	return true;
}
/**
 * @brief Метод записи собранных выражений
 *
 * @param expressions набор собранных выражений
 * @param result      запись хранилища
 * @return            результат записи собранных выражений
 *
 */
bool awh::regex::Storage::save(const vector <exp_t> & expressions, string & result) const noexcept {
	// Выполняем очистку записи хранилища
	result.clear();
	// Выполняем сброс кода ошибки хранилища
	this->_error = storage_error_t::NONE;
	/**
	 * Если количество записываемых выражений превышает допустимое
	 */
	if(expressions.size() > static_cast <size_t> (MAX_EXPRESSIONS)) {
		// Устанавливаем ошибку превышения размера записи
		this->_error = storage_error_t::TOO_LARGE;
		// Выводим результат записи собранных выражений
		return false;
	}
	// Содержимое записи хранилища
	string payload;
	// Выполняем запись количества собранных выражений
	writeVar(static_cast <uint32_t> (expressions.size()), payload);
	/**
	 * Выполняем перебор набора собранных выражений
	 */
	for(const auto & expression : expressions) {
		/**
		 * Если собранное выражение не установлено
		 */
		if(!expression) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат записи собранных выражений
			return false;
		}
		// Выполняем запись признаков собранного выражения
		write8(static_cast <uint8_t> (expression->backtracking ? 1 : 0), payload);
		write8(static_cast <uint8_t> (expression->ready ? 1 : 0), payload);
		write8(static_cast <uint8_t> (expression->reversible ? 1 : 0), payload);
		// Выполняем запись программы сопоставления в прямом направлении
		this->save(expression->forward, payload);
		// Выполняем запись программы сопоставления в обратном направлении
		this->save(expression->backward, payload);
		// Выполняем запись количества именованных групп выражения
		writeVar(static_cast <uint32_t> (expression->names.size()), payload);
		/**
		 * Выполняем перебор соответствия имён именованных групп
		 */
		for(const auto & item : expression->names) {
			// Выполняем запись имени именованной группы
			writeText(item.first, payload);
			// Выполняем запись количества номеров именованной группы
			writeVar(static_cast <uint32_t> (item.second.size()), payload);
			/**
			 * Выполняем перебор набора номеров именованной группы
			 */
			for(const auto & number : item.second)
				// Выполняем запись номера именованной группы
				writeVar(number, payload);
		}
	}
	/**
	 * Если размер записи хранилища превышает допустимый
	 */
	if(payload.size() > MAX_STORAGE) {
		// Устанавливаем ошибку превышения размера записи
		this->_error = storage_error_t::TOO_LARGE;
		// Выводим результат записи собранных выражений
		return false;
	}
	// Выполняем размещение записи хранилища
	result.reserve(payload.size() + 32);
	// Выполняем запись опознания записи хранилища
	write64(STORAGE_MAGIC, result);
	// Выполняем запись версии устройства записи
	write16(STORAGE_VERSION, result);
	/**
	 * Выполняем запись опознания устройства машины
	 *
	 * @details Наборы программы пишутся образом памяти, поэтому запись отвечает
	 *          порядку байтов машины и размещению полей структур, какое сборщик
	 *          волен избирать сам. Опознание несёт порядок байтов и размеры
	 *          структур, а восстановление сличает его со своим и запись чужую
	 *          отвергает. Так же поступает и эталон: запись, порождённая
	 *          «pcre2_serialize_encode», переносу между машинами не подлежит.
	 *
	 */
	write16(platform(), result);
	// Выполняем запись байтов выравнивания заголовка записи
	write32(0, result);
	// Выполняем запись размера содержимого записи
	write64(static_cast <uint64_t> (payload.size()), result);
	// Выполняем запись контрольной суммы содержимого
	write64(checksum(payload), result);
	// Выполняем запись содержимого записи хранилища
	result.append(payload);
	// Выводим результат записи собранных выражений
	return true;
}
/**
 * @brief Метод восстановления собранных выражений
 *
 * @param data   запись хранилища
 * @param result набор восстановленных выражений
 * @return       результат восстановления собранных выражений
 *
 */
bool awh::regex::Storage::load(string_view data, vector <exp_t> & result) const noexcept {
	// Выполняем восстановление собранных выражений из копии записи
	return this->restore(make_shared <const string> (data), result);
}
/**
 * @brief Метод восстановления собранных выражений с передачей записи во владение
 *
 * @param data   запись хранилища
 * @param result набор восстановленных выражений
 * @return       результат восстановления собранных выражений
 *
 */
bool awh::regex::Storage::adopt(string && data, vector <exp_t> & result) const noexcept {
	// Выполняем восстановление собранных выражений из переданной записи
	return this->restore(make_shared <const string> (::move(data)), result);
}
/**
 * @brief Метод восстановления собранных выражений из записи, взятой во владение
 *
 * @param blob   запись хранилища
 * @param result набор восстановленных выражений
 * @return       результат восстановления собранных выражений
 *
 */
bool awh::regex::Storage::restore(const shared_ptr <const string> & blob, vector <exp_t> & result) const noexcept {
	// Получаем обзор записи хранилища
	const string_view data(blob ? string_view(* blob) : string_view());
	// Выполняем очистку набора восстановленных выражений
	result.clear();
	// Выполняем сброс кода ошибки хранилища
	this->_error = storage_error_t::NONE;
	/**
	 * Если запись хранилища пуста
	 */
	if(data.empty()) {
		// Устанавливаем ошибку пустой записи
		this->_error = storage_error_t::EMPTY;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Позиция чтения записи хранилища
	size_t offset = 0;
	// Опознание записи хранилища
	uint64_t magic = 0;
	/**
	 * Если чтение опознания записи не выполнено
	 */
	if(!read64(data, offset, magic)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если опознание записи не совпадает
	 */
	if(magic != STORAGE_MAGIC) {
		// Устанавливаем ошибку несовпадения опознания записи
		this->_error = storage_error_t::BAD_MAGIC;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Версия устройства записи хранилища
	uint16_t version = 0;
	/**
	 * Если чтение версии устройства записи не выполнено
	 */
	if(!read16(data, offset, version)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если версия устройства записи не поддерживается
	 */
	if(version != STORAGE_VERSION) {
		// Устанавливаем ошибку неподдерживаемой версии записи
		this->_error = storage_error_t::BAD_VERSION;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Опознание устройства машины и байты выравнивания заголовка
	uint16_t machine = 0;
	// Байты выравнивания заголовка записи
	uint32_t padding = 0;
	/**
	 * Если чтение опознания устройства машины не выполнено
	 */
	if(!read16(data, offset, machine) || !read32(data, offset, padding)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если запись порождена машиной устройства иного
	 */
	if(machine != platform()) {
		// Устанавливаем ошибку несовпадения устройства машины
		this->_error = storage_error_t::BAD_PLATFORM;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Размер содержимого записи и контрольная сумма
	uint64_t length = 0, sum = 0;
	/**
	 * Если чтение размера содержимого либо суммы не выполнено
	 */
	if(!read64(data, offset, length) || !read64(data, offset, sum)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если объявленный размер содержимого записи не отвечает её длине
	 */
	if(length != static_cast <uint64_t> (data.size() - offset)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Получаем содержимое записи хранилища
	const string_view payload = data.substr(offset);
	/**
	 * Если контрольная сумма содержимого записи не совпадает
	 */
	if(checksum(payload) != sum) {
		// Устанавливаем ошибку несовпадения контрольной суммы
		this->_error = storage_error_t::BAD_CHECKSUM;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Выполняем сброс позиции чтения содержимого записи
	offset = 0;
	// Количество восстанавливаемых выражений
	uint32_t count = 0;
	/**
	 * Если чтение количества выражений не выполнено
	 */
	if(!readVar(payload, offset, count)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если количество выражений превышает допустимое
	 */
	if(count > MAX_EXPRESSIONS) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Выполняем размещение набора восстановленных выражений
	result.reserve(static_cast <size_t> (count));
	/**
	 * Выполняем восстановление набора выражений
	 */
	for(uint32_t i = 0; i < count; i++) {
		// Создаём восстанавливаемое выражение
		auto expression = make_shared <expression_t> ();
		// Признак выражения, читаемый записью
		uint8_t flag = 0;
		/**
		 * Если чтение признаков выражения не выполнено
		 */
		if(!read8(payload, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Выполняем установку признака исполнения выражения с возвратом
		expression->backtracking = (flag != 0);
		/**
		 * Если чтение признаков выражения не выполнено
		 */
		if(!read8(payload, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Выполняем установку признака готовности выражения
		expression->ready = (flag != 0);
		/**
		 * Если чтение признаков выражения не выполнено
		 */
		if(!read8(payload, offset, flag)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Выполняем установку признака применимости поиска начала совпадения
		expression->reversible = (flag != 0);
		/**
		 * Если восстановление программ выражения не выполнено
		 */
		if(!this->load(payload, offset, expression->forward) ||
		 !this->load(payload, offset, expression->backward))
			// Выводим результат восстановления собранных выражений
			return false;
		/**
		 * Выполняем установку держателя записи хранилища программам выражения
		 *
		 * @details Наборы программ обозревают участки записи, собственного
		 *          содержимого не имея, поэтому держатель продлевает жизнь
		 *          записи на срок жизни выражения и всех его копий.
		 *
		 */
		expression->forward.blob = blob;
		// Выполняем установку держателя записи программе обратного направления
		expression->backward.blob = blob;
		// Количество именованных групп выражения
		uint32_t names = 0;
		/**
		 * Если чтение количества именованных групп не выполнено
		 */
		if(!readVar(payload, offset, names)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		/**
		 * Если количество именованных групп превышает размер оставшейся записи
		 */
		if((static_cast <size_t> (names) * 3) > (payload.size() - offset)) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		/**
		 * Выполняем восстановление соответствия имён именованных групп
		 */
		for(uint32_t j = 0; j < names; j++) {
			// Имя именованной группы выражения
			string name;
			// Количество номеров именованной группы
			uint32_t numbers = 0;
			/**
			 * Если чтение имени именованной группы не выполнено
			 */
			if(!readText(payload, offset, name) || !readVar(payload, offset, numbers)) {
				// Устанавливаем ошибку обрыва записи
				this->_error = storage_error_t::TRUNCATED;
				// Выводим результат восстановления собранных выражений
				return false;
			}
			/**
			 * Если количество номеров превышает размер оставшейся записи
			 */
			if(static_cast <size_t> (numbers) > (payload.size() - offset)) {
				// Устанавливаем ошибку несообразного содержимого записи
				this->_error = storage_error_t::BAD_CONTENT;
				// Выводим результат восстановления собранных выражений
				return false;
			}
			// Создаём набор номеров именованной группы
			vector <uint32_t> records(static_cast <size_t> (numbers));
			/**
			 * Выполняем перебор набора номеров именованной группы
			 */
			for(auto & number : records) {
				/**
				 * Если чтение номера именованной группы не выполнено
				 */
				if(!readVar(payload, offset, number)) {
					// Устанавливаем ошибку обрыва записи
					this->_error = storage_error_t::TRUNCATED;
					// Выводим результат восстановления собранных выражений
					return false;
				}
				/**
				 * Если номер именованной группы выражению не принадлежит
				 */
				if(number > expression->forward.captures) {
					// Устанавливаем ошибку несообразного содержимого записи
					this->_error = storage_error_t::BAD_CONTENT;
					// Выводим результат восстановления собранных выражений
					return false;
				}
			}
			// Выполняем добавление именованной группы в соответствие
			expression->names.emplace(::move(name), ::move(records));
		}
		/**
		 * Если выражение собрано с режимом порождения машинного кода
		 *
		 * @details Порождённый код в записи не хранится, поэтому порождается
		 *          заново. Отказ порождения отказом восстановления не является:
		 *          выражение сопоставляется исполнением программы, как и без
		 *          режима.
		 */
		if(((expression->forward.flags & static_cast <uint32_t> (flag_t::JIT)) != 0) &&
		 ((expression->forward.flags & (static_cast <uint32_t> (flag_t::ANCHORED) |
		  static_cast <uint32_t> (flag_t::NOTEMPTY))) == 0) &&
		 !expression->backtracking && !expression->forward.plain) {
			// Создаём сопоставитель выражения в виде порождённого машинного кода
			expression->machine = make_shared <codegen_t> ();
			/**
			 * Если порождение сопоставителя выражения не выполнено
			 */
			if(!expression->machine->compile(expression->forward))
				// Выполняем сброс сопоставителя выражения
				expression->machine.reset();
		}
		// Выполняем добавление восстановленного выражения в набор
		result.push_back(::move(expression));
	}
	/**
	 * Если запись хранилища прочитана не до конца
	 */
	if(offset != payload.size()) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выполняем очистку набора восстановленных выражений
		result.clear();
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Выводим результат восстановления собранных выражений
	return true;
}
/**
 * @brief Метод извлечения кода ошибки хранилища
 *
 * @return код ошибки хранилища собранных выражений
 *
 */
awh::regex::storage_error_t awh::regex::Storage::error() const noexcept {
	// Выводим код ошибки хранилища собранных выражений
	return this->_error;
}
