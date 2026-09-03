/**
 * @file storage.cpp
 * @date 2026-08-04
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Файл реализации хранилища собранных регулярных выражений —
 *        запись собранных выражений последовательностью байтов и восстановление
 *        их из записи без повторного разбора и компиляции
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <regex/storage.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <ctime>
#include <cstring>

/**
 * Если сборка выполняется для операционной системы семейства Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем заголовочные файлы Windows
	 */
	#include <windows.h>
/**
 * Если сборка выполняется для операционных систем семейства POSIX
 */
#else
	/**
	 * Подключаем заголовочные файлы POSIX
	 */
	#include <sys/utsname.h>
#endif

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
	 * @details Разрядность эта хранилищем сейчас не звучит: числа его укладываются
	 *          в длину переменную, а разрядности постоянные служат заголовку.
	 *          Из семейства она всё же не изымается - восемь, шестнадцать,
	 *          тридцать два и шестьдесят четыре разряда стоят набором полным,
	 *          и изъятие середины его заставило бы заводить её заново при
	 *          первой же смене состава заголовка.
	 *
	 * @param value  записываемое число
	 * @param result запись хранилища
	 *
	 */
	[[maybe_unused]] inline void write32(const uint32_t value, string & result) noexcept {
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
	 * @details Разрядность эта хранилищем сейчас не звучит - см. write32.
	 *
	 * @param data   запись хранилища
	 * @param offset позиция чтения записи
	 * @param value  прочитанное число
	 * @return       результат чтения числа
	 *
	 */
	[[maybe_unused]] inline bool read32(string_view data, size_t & offset, uint32_t & value) noexcept {
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
	 * @brief Функция опознания машины, запись породившей
	 *
	 * @return опознание машины
	 *
	 * @details Опознание собирается свойствами системы и оборудования, какие
	 *          доступны без особых прав на всех системах набора: именем узла,
	 *          именем и изданием системы, именем набора команд. Опознаётся им
	 *          машина «такая же», а не «та самая»: серийные номера платы и
	 *          процессора добывались бы по-разному на восьми семействах систем,
	 *          а кое-где требовали бы прав, отчего перенос модуля обошёлся бы
	 *          дороже приобретаемой строгости.
	 *
	 *          Опознание сличается восстановлением, ибо запись несёт машинный
	 *          код, машине порождения принадлежащий: код тот перемещаем внутри
	 *          машины, но не между машинами, и годен лишь набору команд, для
	 *          какого порождён.
	 *
	 */
	inline uint64_t hardware() noexcept {
		// Собираемое опознание машины
		uint64_t result = 0xCBF29CE484222325ull;
		/**
		 * @brief Функция примешивания последовательности к опознанию
		 *
		 * @param value примешиваемая последовательность
		 *
		 */
		auto mixing = [&result](const char * value) noexcept -> void {
			/**
			 * Если примешиваемая последовательность задана
			 */
			if(value != nullptr) {
				/**
				 * Выполняем перебор примешиваемой последовательности
				 */
				for(size_t i = 0; value[i] != '\0'; i++) {
					// Выполняем смешивание очередного байта последовательности
					result ^= static_cast <uint64_t> (static_cast <uint8_t> (value[i]));
					// Выполняем умножение накопленного опознания
					result *= 0x100000001B3ull;
				}
			}
			// Выполняем разделение примешиваемых последовательностей
			result *= 0x9E3779B97F4A7C15ull;
		};
		/**
		 * Если сборка выполняется для операционной системы семейства Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Имя узла машины
			char node[MAX_COMPUTERNAME_LENGTH + 1];
			// Размер имени узла машины
			DWORD size = static_cast <DWORD> (sizeof(node));
			/**
			 * Если имя узла машины получено
			 */
			if(::GetComputerNameA(node, &size))
				// Выполняем примешивание имени узла машины
				mixing(node);
			// Выполняем примешивание опознания системы
			mixing("Windows");
		/**
		 * Если сборка выполняется для операционных систем семейства POSIX
		 */
		#else
			// Свойства системы машины
			struct utsname system;
			/**
			 * Если свойства системы машины получены
			 */
			if(::uname(&system) == 0) {
				// Выполняем примешивание имени узла машины
				mixing(system.nodename);
				// Выполняем примешивание имени системы машины
				mixing(system.sysname);
				// Выполняем примешивание издания системы машины
				mixing(system.release);
				// Выполняем примешивание имени набора команд машины
				mixing(system.machine);
			}
		#endif
		// Выводим опознание машины
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
	// Выполняем запись предела шагов сопоставления выражения
	writeVar(program.steps, result);
	// Выполняем запись предела глубины рекурсивных вызовов выражения
	writeVar(program.depth, result);
	// Выполняем запись предела объёма памяти сопоставления выражения
	writeVar(program.heap, result);
	// Выполняем запись соглашения о переводе строки выражения
	write8(static_cast <uint8_t> (program.newline), result);
	// Выполняем запись номера ячейки отметки последней
	writeVar(program.marker, result);
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
	/**
	 * Выполняем запись удаления обязательного литерала от начала совпадения
	 *
	 * @details Удаление вычисляется разбором синтаксического дерева, какого
	 *          запись не несёт, отчего восстановить его по записи иначе
	 *          как чтением нельзя. Отсутствие же его обратило бы отбор
	 *          позиций по литералу в отбор по удалению нулевому, то есть
	 *          отвергло бы совпадения, литералом не начинаемые.
	 *
	 */
	write64(static_cast <uint64_t> (program.prefilter.distance), result);
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
	// Выполняем запись хранилища имён отметок глаголов управления
	writeRegion(program.markers, result);
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
	 !readVar(data, offset, program.flags) ||
	 !readVar(data, offset, program.steps) ||
	 !readVar(data, offset, program.depth) ||
	 !readVar(data, offset, program.heap)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Соглашение о переводе строки, читаемое записью
	uint8_t convention = 0;
	/**
	 * Если чтение соглашения о переводе строки не выполнено
	 */
	if(!read8(data, offset, convention)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если соглашение о переводе строки набором не заведено
	 */
	if(convention > static_cast <uint8_t> (newline_t::NUL)) {
		// Устанавливаем ошибку неверного устройства записи
		this->_error = storage_error_t::BAD_CONTENT;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем установку соглашения о переводе строки выражения
	program.newline = static_cast <newline_t> (convention);
	/**
	 * Если чтение номера ячейки отметки последней не выполнено
	 */
	if(!readVar(data, offset, program.marker)) {
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
	// Удаление обязательного литерала от начала совпадения
	uint64_t distance = 0;
	/**
	 * Если чтение удаления обязательного литерала не выполнено
	 */
	if(!read64(data, offset, distance)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	// Выполняем установку удаления обязательного литерала
	program.prefilter.distance = static_cast <size_t> (distance);
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
	/**
	 * Если восстановление хранилища имён отметок не выполнено
	 */
	if(!readRegion(data, offset, MAX_STORAGE, program.markers)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления программы
		return false;
	}
	/**
	 * Если правильность восстановленной программы не подтверждена
	 *
	 * @details Проверка эта - единственный заслон от записи, нарочно
	 *          подделанной: контрольная сумма ловит порчу случайную, но
	 *          подделыватель считает её заново. Наборы программы обозревают
	 *          запись прямо на месте, а адреса переходов, указания на классы
	 *          символов и участки сплошных наборов употребляются исполнением
	 *          без проверки границ, отчего запись с указаниями наружу вывела
	 *          бы чтение за пределы её самой.
	 *
	 */
	if(!this->verify(program)) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
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
	for(size_t index = 0; index < program.instructions.size(); index++) {
		// Получаем очередное указание проверяемой программы
		const auto & instruction = program.instructions[index];
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
				 * Если тело проверки окружения указывает на неё же
				 *
				 * @details Тело исполняется вложенным исполнением, и указание тела
				 *          на саму проверку дало бы вложенность бесконечную. Сборка
				 *          такого не порождает, а запись поддельная - вполне.
				 *
				 *          Заслон этот исполнение не заменяет, а дополняет: связка
				 *          проверок, указывающих одна на другую, здесь не ловится,
				 *          и стережёт её предел вложенности исполнений.
				 *
				 */
				if(static_cast <size_t> (instruction.look.body) == index)
					// Выводим результат проверки правильности программы
					return false;
				/**
				 * Если границы длины сопоставляемого текста несообразны
				 */
				if(instruction.look.least > instruction.look.most)
					// Выводим результат проверки правильности программы
					return false;
				/**
				 * Если ячейка позиции начала не отсекающей проверки не заведена
				 */
				if((instruction.look.atomic == 0) && (static_cast <size_t> (instruction.look.cell) >=
				 (((static_cast <size_t> (program.captures) + 1) * 2) + static_cast <size_t> (program.cells))))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция проверяет прогон письменности
			 */
			case static_cast <uint8_t> (opcode_t::SCRIPT): {
				/**
				 * Если ячейка позиции начала прогона письменности не заведена
				 */
				if(static_cast <size_t> (instruction.save.slot) >=
				 (((static_cast <size_t> (program.captures) + 1) * 2) + static_cast <size_t> (program.cells)))
					// Выводим результат проверки правильности программы
					return false;
			} break;
			/**
			 * Если инструкция восстанавливает позицию начала не отсекающей проверки
			 */
			case static_cast <uint8_t> (opcode_t::RESET): {
				/**
				 * Если ячейка позиции начала проверки либо адрес продолжения несообразны
				 */
				if((static_cast <size_t> (instruction.reset.cell) >=
				 (((static_cast <size_t> (program.captures) + 1) * 2) + static_cast <size_t> (program.cells))) ||
				 !inside(instruction.reset.target))
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
		/**
		 * Если порождённый сопоставитель выражения готов
		 *
		 * @details Порождённый код записывается затем, чтобы восстановление
		 *          обошлось без порождения заново: замер даёт четыре с
		 *          половиною микросекунды на выражение без него и почти
		 *          четырнадцать с ним, то есть порождение обходится втрое
		 *          дороже всего прочего восстановления. Годен код лишь набору
		 *          команд, для какого порождён, поэтому запись несёт его
		 *          опознание, а несовпадение оборачивается порождением заново.
		 *
		 */
		if(expression->machine && expression->machine->ready()) {
			// Запись порождённого сопоставителя выражения
			string machine;
			/**
			 * Если запись порождённого сопоставителя выполнена
			 */
			if(expression->machine->save(machine)) {
				// Выполняем запись признака наличия порождённого сопоставителя
				write8(1, payload);
				// Выполняем запись размера порождённого сопоставителя
				writeVar(static_cast <uint32_t> (machine.size()), payload);
				// Выполняем запись порождённого сопоставителя выражения
				payload.append(machine);
			// Выполняем запись отсутствия порождённого сопоставителя
			} else write8(0, payload);
		// Выполняем запись отсутствия порождённого сопоставителя
		} else write8(0, payload);
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
	/**
	 * Если запись хранилища подлежит сжатию
	 */
	if(this->_method != compressor::method_t::NONE) {
		/**
		 * Если обработчик сжатия записи не установлен
		 */
		if(!this->_pack) {
			// Устанавливаем ошибку отсутствия обработчика метода сжатия
			this->_error = storage_error_t::BAD_METHOD;
			// Выводим результат записи собранных выражений
			return false;
		}
		// Сжатое содержимое записи хранилища
		string packed;
		/**
		 * Если сжатие содержимого записи не выполнено
		 */
		if(!this->_pack(payload, packed) || packed.empty()) {
			// Устанавливаем ошибку невыполненного сжатия записи
			this->_error = storage_error_t::BAD_PACKING;
			// Выводим результат записи собранных выражений
			return false;
		}
		/**
		 * Выполняем зашифрование сжатого содержимого записи
		 *
		 * @details Шифрование идёт ПОСЛЕ сжатия: содержимое шифрованное сжатию
		 *          не поддаётся вовсе, и обратный порядок обратил бы сжатие
		 *          в пустую работу.
		 *
		 */
		if(this->_ciphered) {
			// Зашифрованное содержимое записи хранилища
			string sealed;
			/**
			 * Если обработчик зашифрования не установлен либо зашифрование не выполнено
			 */
			if(!this->_encrypt || !this->_encrypt(packed, sealed) || sealed.empty()) {
				// Устанавливаем ошибку невыполненного шифрования записи
				this->_error = storage_error_t::BAD_CIPHER;
				// Выводим результат записи собранных выражений
				return false;
			}
			// Выполняем замену сжатого содержимого зашифрованным
			packed = ::move(sealed);
		}
		// Выполняем размещение записи хранилища
		result.reserve(packed.size() + 64);
		// Выполняем запись опознания записи хранилища
		write64(STORAGE_MAGIC, result);
		// Выполняем запись версии устройства записи
		write16(STORAGE_VERSION, result);
		// Выполняем запись опознания устройства машины
		write16(platform(), result);
		// Выполняем запись метода сжатия записи хранилища
		write8(static_cast <uint8_t> (this->_method), result);
		// Выполняем запись признака шифрования записи хранилища
		write8((this->_ciphered ? 1 : 0), result);
		// Выполняем запись байтов выравнивания заголовка записи
		write16(0, result);
		// Выполняем запись размера сжатого содержимого записи
		write64(static_cast <uint64_t> (packed.size()), result);
		// Выполняем запись размера содержимого записи до сжатия
		write64(static_cast <uint64_t> (payload.size()), result);
		// Выполняем запись контрольной суммы сжатого содержимого
		write64(checksum(packed), result);
		// Выполняем запись опознания машины, запись породившей
		write64(hardware(), result);
		// Выполняем запись мгновения порождения записи
		write64(static_cast <uint64_t> (::time(nullptr)), result);
		// Выполняем запись срока годности записи
		write64(this->_lifetime, result);
		// Выполняем запись сжатого содержимого записи хранилища
		result.append(packed);
		// Выводим результат записи собранных выражений
		return true;
	}
	/**
	 * Выполняем зашифрование содержимого записи хранилища
	 */
	if(this->_ciphered) {
		// Зашифрованное содержимое записи хранилища
		string sealed;
		/**
		 * Если обработчик зашифрования не установлен либо зашифрование не выполнено
		 */
		if(!this->_encrypt || !this->_encrypt(payload, sealed) || sealed.empty()) {
			// Устанавливаем ошибку невыполненного шифрования записи
			this->_error = storage_error_t::BAD_CIPHER;
			// Выводим результат записи собранных выражений
			return false;
		}
		// Выполняем замену содержимого записи зашифрованным
		payload = ::move(sealed);
	}
	// Выполняем размещение записи хранилища
	result.reserve(payload.size() + 64);
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
	// Выполняем запись метода сжатия записи хранилища
	write8(static_cast <uint8_t> (compressor::method_t::NONE), result);
	// Выполняем запись признака шифрования записи хранилища
	write8((this->_ciphered ? 1 : 0), result);
	// Выполняем запись байтов выравнивания заголовка записи
	write16(0, result);
	// Выполняем запись размера содержимого записи
	write64(static_cast <uint64_t> (payload.size()), result);
	// Выполняем запись размера содержимого записи до сжатия
	write64(0, result);
	// Выполняем запись контрольной суммы содержимого
	write64(checksum(payload), result);
	/**
	 * Выполняем запись опознания машины, запись породившей
	 *
	 * @details Опознание сличается восстановлением: запись несёт машинный код,
	 *          машине порождения принадлежащий, и переносу между машинами
	 *          не подлежит.
	 *
	 */
	write64(hardware(), result);
	// Выполняем запись мгновения порождения записи
	write64(static_cast <uint64_t> (::time(nullptr)), result);
	// Выполняем запись срока годности записи
	write64(this->_lifetime, result);
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
	/**
	 * Если восстановление собранных выражений выполнено
	 */
	if(this->restoring(blob, result))
		// Выводим результат восстановления собранных выражений
		return true;
	/**
	 * Если объект журнала событий передан
	 *
	 * @details Запись хранилища приходит извне - файлом либо сетью, - и порча её
	 *          равно как и несовпадение вида записи есть происшествие, о каком
	 *          потребителю знать надлежит: выражения молча не восстановятся вовсе.
	 *
	 */
	if(this->_log != nullptr) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Storage record of %zu bytes could not be restored: error %u", __PRETTY_FUNCTION__, make_tuple(blob ? blob->size() : 0), log_t::flag_t::WARNING, (blob ? blob->size() : 0), static_cast <uint16_t> (this->_error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Storage record of %zu bytes could not be restored: error %u", log_t::flag_t::WARNING, (blob ? blob->size() : 0), static_cast <uint16_t> (this->_error));
		#endif
	}
	// Выводим результат восстановления собранных выражений
	return false;
}
/**
 * @brief Метод восстановления собранных выражений телом своим
 *
 * @param blob   запись хранилища
 * @param result набор восстановленных выражений
 * @return       результат восстановления собранных выражений
 *
 */
bool awh::regex::Storage::restoring(const shared_ptr <const string> & blob, vector <exp_t> & result) const noexcept {
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
	// Опознание устройства машины
	uint16_t machine = 0;
	// Метод сжатия записи хранилища и признак шифрования её
	uint8_t method = 0, sealed = 0;
	// Байты выравнивания заголовка записи
	uint16_t padding = 0;
	/**
	 * Если чтение опознания устройства машины не выполнено
	 */
	if(!read16(data, offset, machine) || !read8(data, offset, method) ||
	 !read8(data, offset, sealed) || !read16(data, offset, padding)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если признак шифрования записи вида недопустимого либо выравнивание не пусто
	 */
	if((sealed > 1) || (padding != 0)) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
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
	// Размер содержимого записи, размер его до сжатия и контрольная сумма
	uint64_t length = 0, origin = 0, sum = 0;
	/**
	 * Если чтение размеров содержимого либо суммы не выполнено
	 */
	if(!read64(data, offset, length) || !read64(data, offset, origin) || !read64(data, offset, sum)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Опознание машины, мгновение порождения и срок годности записи
	uint64_t owner = 0, created = 0, lifetime = 0;
	/**
	 * Если чтение опознания машины, мгновения порождения либо срока не выполнено
	 */
	if(!read64(data, offset, owner) || !read64(data, offset, created) || !read64(data, offset, lifetime)) {
		// Устанавливаем ошибку обрыва записи
		this->_error = storage_error_t::TRUNCATED;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если запись порождена машиной иной
	 *
	 * @details Опознание устройства машины сличается отдельно и говорит лишь
	 *          о размещении полей: запись же несёт машинный код, машине
	 *          порождения принадлежащий, отчего сличается и сама машина.
	 *
	 */
	if(owner != hardware()) {
		// Устанавливаем ошибку несовпадения машины записи
		this->_error = storage_error_t::BAD_MACHINE;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	/**
	 * Если срок годности записи задан
	 *
	 * @details Мгновение порождения сличается с нынешним по часам системы,
	 *          а запись из будущего просроченной не числится: часы машины
	 *          переводимы, и отказ по ним обернулся бы отказом работы.
	 *
	 */
	if(lifetime > 0) {
		// Получаем нынешнее мгновение по часам системы
		const uint64_t now = static_cast <uint64_t> (::time(nullptr));
		/**
		 * Если срок годности записи истёк
		 */
		if((now > created) && ((now - created) > lifetime)) {
			// Устанавливаем ошибку истечения срока годности записи
			this->_error = storage_error_t::EXPIRED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
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
	string_view payload = data.substr(offset);
	/**
	 * Если контрольная сумма содержимого записи не совпадает
	 */
	if(checksum(payload) != sum) {
		// Устанавливаем ошибку несовпадения контрольной суммы
		this->_error = storage_error_t::BAD_CHECKSUM;
		// Выводим результат восстановления собранных выражений
		return false;
	}
	// Держатель содержимого записи, обозреваемого наборами программ
	shared_ptr <const string> holder = blob;
	/**
	 * Если запись хранилища шифрована
	 *
	 * @details Расшифрование идёт ПЕРЕД разжатием: шифрование выполнялось
	 *          последним, а снимается первым. Расшифрованное содержимое
	 *          берётся держателем во владение - обзоры программ обращаются
	 *          к нему, а не к записи поданной.
	 *
	 */
	if(sealed > 0) {
		/**
		 * Если обработчик расшифрования записи не установлен
		 */
		if(!this->_decrypt) {
			// Устанавливаем ошибку отсутствия обработчика расшифрования
			this->_error = storage_error_t::BAD_METHOD;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Расшифрованное содержимое записи хранилища
		auto opened = make_shared <string> ();
		/**
		 * Если расшифрование содержимого записи не выполнено
		 */
		if(!this->_decrypt(payload, * opened) || opened->empty()) {
			// Устанавливаем ошибку невыполненного расшифрования записи
			this->_error = storage_error_t::BAD_CIPHER;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Выполняем передачу расшифрованного содержимого держателю
		holder = opened;
		// Выполняем установку обзора расшифрованного содержимого записи
		payload = string_view(* holder);
	}
	/**
	 * Если запись хранилища сжата
	 */
	if(method != static_cast <uint8_t> (compressor::method_t::NONE)) {
		/**
		 * Если обработчик разжатия записи не установлен
		 */
		if(!this->_unpack) {
			// Устанавливаем ошибку отсутствия обработчика метода сжатия
			this->_error = storage_error_t::BAD_METHOD;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		/**
		 * Если размер содержимого до сжатия превышает допустимый
		 */
		if(origin > static_cast <uint64_t> (MAX_STORAGE)) {
			// Устанавливаем ошибку превышения размера записи
			this->_error = storage_error_t::TOO_LARGE;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Разжатое содержимое записи хранилища
		string unpacked;
		/**
		 * Если разжатие содержимого записи не выполнено
		 */
		if(!this->_unpack(payload, unpacked)) {
			// Устанавливаем ошибку невыполненного разжатия записи
			this->_error = storage_error_t::BAD_PACKING;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		/**
		 * Если размер разжатого содержимого объявленному не отвечает
		 */
		if(static_cast <uint64_t> (unpacked.size()) != origin) {
			// Устанавливаем ошибку несообразного содержимого записи
			this->_error = storage_error_t::BAD_CONTENT;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		/**
		 * Выполняем передачу разжатого содержимого держателю
		 *
		 * @details Наборы программ обозревают участки содержимого разжатого,
		 *          а не самой записи, поэтому держателем становится оно.
		 *          Запись сжатая по выходе из восстановления не нужна вовсе.
		 */
		holder = make_shared <const string> (::move(unpacked));
		// Выполняем установку обзора разжатого содержимого записи
		payload = string_view(* holder);
	/**
	 * Если объявленный размер содержимого до сжатия не пуст
	 */
	} else if(origin != 0) {
		// Устанавливаем ошибку несообразного содержимого записи
		this->_error = storage_error_t::BAD_CONTENT;
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
		expression->forward.blob = holder;
		// Выполняем установку держателя записи программе обратного направления
		expression->backward.blob = holder;
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
		// Признак наличия порождённого сопоставителя в записи
		uint8_t machined = 0;
		/**
		 * Если чтение признака наличия порождённого сопоставителя не выполнено
		 */
		if(!read8(payload, offset, machined)) {
			// Устанавливаем ошибку обрыва записи
			this->_error = storage_error_t::TRUNCATED;
			// Выводим результат восстановления собранных выражений
			return false;
		}
		// Запись порождённого сопоставителя выражения
		string_view machine;
		/**
		 * Если запись несёт порождённый сопоставитель выражения
		 */
		if(machined != 0) {
			// Размер порождённого сопоставителя выражения
			uint32_t length = 0;
			/**
			 * Если чтение размера порождённого сопоставителя не выполнено
			 */
			if(!readVar(payload, offset, length)) {
				// Устанавливаем ошибку обрыва записи
				this->_error = storage_error_t::TRUNCATED;
				// Выводим результат восстановления собранных выражений
				return false;
			}
			/**
			 * Если порождённый сопоставитель за пределы записи выходит
			 */
			if(static_cast <size_t> (length) > (payload.size() - offset)) {
				// Устанавливаем ошибку несообразного содержимого записи
				this->_error = storage_error_t::BAD_CONTENT;
				// Выводим результат восстановления собранных выражений
				return false;
			}
			// Выполняем установку обзора порождённого сопоставителя
			machine = payload.substr(offset, static_cast <size_t> (length));
			// Переходим за запись порождённого сопоставителя
			offset += static_cast <size_t> (length);
		}
		/**
		 * Если выражение собрано с режимом порождения машинного кода
		 *
		 * @details Порождённый код восстанавливается из записи, а при её
		 *          отсутствии либо при несовпадении набора команд порождается
		 *          заново. Отказ порождения отказом восстановления не является:
		 *          выражение сопоставляется исполнением программы, как и без
		 *          режима.
		 *
		 *          Выражение, исполняемое с возвратом, порождения не лишается:
		 *          подмножество принимаемых порождением выражений с подмножеством
		 *          выражений регулярных не совпадает, и рекурсивный вызов
		 *          подвыражения порождением принимается. Условие это обязано
		 *          совпадать с условием сборки: расхождение оставило бы
		 *          восстановленное выражение без сопоставителя, сборкой
		 *          полученного.
		 */
		if(((expression->forward.flags & static_cast <uint32_t> (flag_t::JIT)) != 0) &&
		 ((expression->forward.flags & (static_cast <uint32_t> (flag_t::ANCHORED) |
		  static_cast <uint32_t> (flag_t::NOTEMPTY) | static_cast <uint32_t> (flag_t::ATSTART))) == 0) &&
		 !expression->forward.plain) {
			// Создаём сопоставитель выражения в виде порождённого машинного кода
			expression->machine = make_shared <codegen_t> (this->_log);
			// Позиция чтения записи порождённого сопоставителя
			size_t position = 0;
			// Признак восстановления порождённого сопоставителя из записи
			bool restored = false;
			/**
			 * Если запись несёт порождённый сопоставитель выражения и записи доверено
			 *
			 * @details Порождённый код записи исполняется, а не разбирается,
			 *          отчего проверкой он не оберегаем вовсе: проверить можно
			 *          данные, но не команды. Контрольная сумма ловит порчу
			 *          случайную, но подделыватель считает её заново, и запись
			 *          подделанная исполнилась бы как есть - выход за пределы
			 *          и падение внутри восстановленного кода получены опытом.
			 *          Поэтому код записи берётся лишь при установленном
			 *          признаке доверия, а без него порождается заново: это
			 *          обходится в десять микросекунд на выражение и снимает
			 *          исполнение чужих команд вовсе.
			 */
			if(this->_trusted && !machine.empty())
				// Выполняем восстановление порождённого сопоставителя из записи
				restored = expression->machine->restore(machine, position, expression->forward);
			/**
			 * Если порождение сопоставителя выражения не выполнено
			 */
			if(!restored && !expression->machine->compile(expression->forward))
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
 * @brief Метод установки доверия порождённому коду записи
 *
 * @param mode признак доверия порождённому коду записи
 *
 */
void awh::regex::Storage::trusted(const bool mode) noexcept {
	// Выполняем установку признака доверия порождённому коду записи
	this->_trusted = mode;
}
/**
 * @brief Метод извлечения кода ошибки хранилища
 *
 * @return код ошибки хранилища собранных выражений
 *
 */
void awh::regex::Storage::packer(const compressor::method_t method, packer_t pack, packer_t unpack) noexcept {
	// Выполняем установку метода сжатия записи хранилища
	this->_method = method;
	// Выполняем установку обработчика сжатия записи хранилища
	this->_pack = ::move(pack);
	// Выполняем установку обработчика разжатия записи хранилища
	this->_unpack = ::move(unpack);
}
/**
 * @brief Метод установки шифрования записи хранилища
 *
 * @param encrypt обработчик зашифрования записи хранилища
 * @param decrypt обработчик расшифрования записи хранилища
 *
 */
void awh::regex::Storage::cipher(cipher_t encrypt, cipher_t decrypt) noexcept {
	// Выполняем установку обработчика зашифрования записи хранилища
	this->_encrypt = ::move(encrypt);
	// Выполняем установку обработчика расшифрования записи хранилища
	this->_decrypt = ::move(decrypt);
	// Выполняем установку признака шифрования записи хранилища
	this->_ciphered = (static_cast <bool> (this->_encrypt) || static_cast <bool> (this->_decrypt));
}
/**
 * @brief Метод установки срока годности записи
 *
 * @param seconds срок годности записи в секундах
 *
 */
void awh::regex::Storage::lifetime(const uint64_t seconds) noexcept {
	// Выполняем установку срока годности записи
	this->_lifetime = seconds;
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
