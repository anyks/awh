/**
 * @file scatter.cpp
 * @date 2026-09-15
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
 * \~russian
 * @brief Файл рассеяния ключевого материала по образу
 *
 * \~english
 * @brief Key material scattering file
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/scatter.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * @brief Безымянное пространство имён
 *
 */
namespace {
	/**
	 * @brief Воспроизводимый поток чисел
	 *
	 * @note Взят у `splitmix64`: чистая арифметика над шестидесятичетырёхразрядным
	 *       словом, без обращения к платформе. Оттого ряд его один на всякой системе, а
	 *       это и нужно - раскладчик и сборщик обязаны считать одну раскладку
	 *
	 */
	class stream_t {
		private:
			// Состояние потока
			uint64_t _state;
		public:
			/**
			 * @brief Метод выдачи очередного числа потока
			 *
			 * @return очередное число потока
			 *
			 */
			uint64_t next() noexcept {
				// Продвигаем состояние потока золотым сечением
				uint64_t value = (this->_state += 0x9E3779B97F4A7C15ULL);
				// Перемешиваем разряды числа
				value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
				value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
				// Выводим перемешанное число
				return (value ^ (value >> 31));
			}
			/**
			 * @brief Метод выдачи очередного байта потока
			 *
			 * @return очередной байт потока
			 *
			 */
			uint8_t byte() noexcept {
				// Выводим младший байт очередного числа потока
				return static_cast <uint8_t> (this->next() & 0xFF);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param seed зерно потока
			 *
			 */
			explicit stream_t(const uint64_t seed) noexcept : _state(seed) {}
	};
	/**
	 * @brief Метод построения перестановки мест носителя
	 *
	 * @note Обе стороны договора строят перестановку ОДНИМ и тем же ходом из одного
	 *       зерна: тасование Фишера-Йетса, обход сверху вниз. Оттого место i-го байта
	 *       секрета выходит у раскладчика и сборщика одно
	 *
	 * @note Смещение по модулю (`% count`) здесь несущественно: перестановка эта не
	 *       ключ, а маскировка, и равномерности до последнего разряда ей не нужно
	 *
	 * @param stream   поток, из которого берётся тасование
	 * @param capacity ёмкость носителя
	 * @return         перестановка мест носителя
	 *
	 */
	static std::vector <uint32_t> shuffle(stream_t & stream, const size_t capacity) noexcept {
		// Заводим перестановку мест носителя
		std::vector <uint32_t> places(capacity);
		/**
		 * Заполняем перестановку местами по порядку
		 */
		for(size_t i = 0; i < capacity; i++)
			// Записываем очередное место
			places[i] = static_cast <uint32_t> (i);
		/**
		 * Тасуем места ходом Фишера-Йетса сверху вниз
		 */
		for(size_t i = capacity; i-- > 1;){
			// Выбираем место для обмена
			const size_t j = static_cast <size_t> (stream.next() % (i + 1));
			// Обмениваем места
			const uint32_t swap = places[i];
			places[i] = places[j];
			places[j] = swap;
		}
		// Выводим перестановку мест носителя
		return places;
	}
};

/**
 * @brief Метод подмешивания якоря в зерно раскладки
 *
 * @param seed     зерно раскладки из закрытого контура
 * @param checksum контрольная сумма кода либо иной якорь, либо нуль
 * @param version  версия ключа, разводящая контуры сборок
 * @return          зерно, привязанное к якорю и версии
 *
 */
uint64_t awh::alloc::Scatter::anchor(const uint64_t seed, const uint64_t checksum, const uint64_t version) noexcept {
	/**
	 * Подмешиваем якорь тем же ходом splitmix64, каким выводится поток
	 *
	 * Смена одного разряда контрольной суммы либо версии рассеивается по всему зерну, а
	 * зерно задаёт весь поток: изменившись, оно даёт иную перестановку и иную гамму
	 */
	// Заводим поток на исходном зерне
	stream_t stream(seed);
	// Подмешиваем контрольную сумму кода
	stream_t bound(stream.next() ^ checksum);
	// Подмешиваем версию ключа и выводим привязанное зерно
	return (bound.next() ^ (version + 0x9E3779B97F4A7C15ULL));
}

/**
 * @brief Метод раскладки секрета по носителю (время сборки)
 *
 * @param secret   адрес секрета
 * @param length   длина секрета в байтах
 * @param seed     зерно раскладки из закрытого контура
 * @param capacity ёмкость носителя в байтах, не меньше длины секрета
 * @return          носитель с рассеянным секретом либо пустой при отказе
 *
 */
std::vector <uint8_t> awh::alloc::Scatter::lay(const uint8_t * secret, const size_t length, const uint64_t seed, const size_t capacity) noexcept {
	// Если раскладывать нечего либо носителю негде вместить секрет
	if((secret == nullptr) || (length == 0) || (capacity < length))
		// Отвечаем пустым носителем
		return std::vector <uint8_t> ();
	// Если ёмкость носителя не умещается в разрядность места
	if(capacity > static_cast <size_t> (UINT32_MAX))
		// Отвечаем пустым носителем
		return std::vector <uint8_t> ();
	// Заводим поток по зерну раскладки
	stream_t stream(seed);
	// Строим перестановку мест носителя
	const std::vector <uint32_t> places = ::shuffle(stream, capacity);
	// Заводим носитель заданной ёмкости
	std::vector <uint8_t> carrier(capacity);
	/**
	 * Кладём секрет на первые места перестановки под наложением гаммы
	 *
	 * В носитель ложится не сам байт секрета, а его наложение на байт потока: место
	 * секрета в образе неотличимо от заполняющего, а снять наложение нечем, не имея
	 * зерна
	 */
	for(size_t i = 0; i < length; i++)
		// Кладём наложенный байт секрета на его место
		carrier[places[i]] = static_cast <uint8_t> (secret[i] ^ stream.byte());
	/**
	 * Заполняем прочие места байтами того же потока
	 *
	 * Так носитель равномерен по всей длине, и места секрета не проступают островками
	 * среди пустоты. Заполнение идёт ПОСЛЕ гаммы: сборщик до него не доходит вовсе,
	 * оттого ряд потока у обеих сторон сходится на гамме
	 */
	for(size_t i = length; i < capacity; i++)
		// Заполняем очередное прочее место байтом потока
		carrier[places[i]] = stream.byte();
	// Выводим носитель с рассеянным секретом
	return carrier;
}
/**
 * @brief Метод вплетения секрета в предоставленную рабочую таблицу (время сборки)
 *
 * @param table  адрес рабочей таблицы, правимой на местах секрета
 * @param size   размер рабочей таблицы в байтах, не меньше длины секрета
 * @param secret адрес секрета
 * @param length длина секрета в байтах
 * @param seed   зерно раскладки из закрытого контура
 * @return        признак выполнения операции
 *
 */
bool awh::alloc::Scatter::weave(uint8_t * table, const size_t size, const uint8_t * secret, const size_t length, const uint64_t seed) noexcept {
	// Если вплетать нечего либо таблице негде вместить секрет
	if((table == nullptr) || (secret == nullptr) || (length == 0) || (size < length))
		// Отвечаем отказом
		return false;
	// Если размер таблицы не умещается в разрядность места
	if(size > static_cast <size_t> (UINT32_MAX))
		// Отвечаем отказом
		return false;
	// Заводим поток по зерну раскладки
	stream_t stream(seed);
	// Строим перестановку мест таблицы
	const std::vector <uint32_t> places = ::shuffle(stream, size);
	/**
	 * Правим секретом лишь его места под наложением гаммы
	 *
	 * Прочие места таблицы - настоящие рабочие данные - не трогаются вовсе: сборщик до
	 * них не доходит, а сохранность их и есть смысл вплетения. Ряд потока сходится с
	 * `gather`: тасование потребило то же число обращений, гамма идёт следом
	 */
	for(size_t i = 0; i < length; i++)
		// Кладём наложенный байт секрета на его место в таблице
		table[places[i]] = static_cast <uint8_t> (secret[i] ^ stream.byte());
	// Выводим результат выполнения операции
	return true;
}

/**
 * @brief Метод порождения исходного текста с носителем (время сборки)
 *
 * @param carrier носитель, полученный от `lay`
 * @param length  длина секрета в байтах
 * @param name    имя порождаемого массива носителя
 * @return         исходный текст на C++ либо пустая строка при отказе
 *
 */
std::string awh::alloc::Scatter::emit(const std::vector <uint8_t> & carrier, const size_t length, const std::string & name) noexcept {
	// Если порождать нечего
	if(carrier.empty() || (length == 0) || (length > carrier.size()) || name.empty())
		// Отвечаем пустой строкой
		return std::string();
	// Заводим порождаемый текст
	std::string result;
	// Заранее отводим место под текст с запасом
	result.reserve(carrier.size() * 6 + 256);
	// Дописываем шапку порождённого текста
	result.append("/**\n");
	result.append(" * Порождено awh::alloc::Scatter. Ни секрета, ни зерна здесь нет:\n");
	result.append(" * в носителе лежит наложение секрета на гамму, снять его нечем без зерна.\n");
	result.append(" */\n");
	// Место под строку разряда: хватает и на "0x%02X", и на "%zu" 64-разрядного (до 20 цифр)
	char cell[24];
	// Дописываем объявление массива носителя
	result.append("static const unsigned char ").append(name).append("[");
	// Дописываем ёмкость носителя
	::snprintf(cell, sizeof(cell), "%zu", carrier.size());
	result.append(cell).append("] = {\n\t");
	/**
	 * Дописываем разряды носителя по шестнадцати в строке
	 */
	for(size_t i = 0; i < carrier.size(); i++){
		// Дописываем очередной разряд носителя
		::snprintf(cell, sizeof(cell), "0x%02X", carrier[i]);
		result.append(cell);
		// Если разряд не последний
		if((i + 1) < carrier.size()){
			// Дописываем разделитель разрядов
			result.append(",");
			// Если строка заполнена шестнадцатью разрядами
			if(((i + 1) % 16) == 0)
				// Переносим строку
				result.append("\n\t");
			// Иначе дописываем пробел
			else result.append(" ");
		}
	}
	// Закрываем объявление массива носителя
	result.append("\n};\n");
	// Дописываем длину секрета
	result.append("static const size_t ").append(name).append("_length = ");
	::snprintf(cell, sizeof(cell), "%zu", length);
	result.append(cell).append(";\n");
	// Выводим порождённый текст
	return result;
}
/**
 * @brief Метод сборки секрета из носителя в приёмник тайн (запуск)
 *
 * @param carrier  адрес носителя в образе
 * @param capacity ёмкость носителя в байтах
 * @param seed     то же зерно, каким велась раскладка
 * @param length   длина секрета в байтах
 * @param vessel   приёмник, куда ложится собранный секрет
 * @return          признак выполнения операции
 *
 */
bool awh::alloc::Scatter::gather(const uint8_t * carrier, const size_t capacity, const uint64_t seed, const size_t length, vessel_t & vessel) noexcept {
	// Если собирать нечего либо носитель не вмещает секрета
	if((carrier == nullptr) || (length == 0) || (capacity < length))
		// Отвечаем отказом
		return false;
	// Если ёмкость носителя не умещается в разрядность места
	if(capacity > static_cast <size_t> (UINT32_MAX))
		// Отвечаем отказом
		return false;
	// Заводим приёмник на длину секрета
	if(!vessel.reserve(length))
		// Отвечаем отказом
		return false;
	// Заводим поток по тому же зерну
	stream_t stream(seed);
	// Строим ту же перестановку мест носителя
	const std::vector <uint32_t> places = ::shuffle(stream, capacity);
	/**
	 * Собираем секрет с первых мест перестановки, снимая наложение
	 *
	 * Ряд потока у сборщика сходится с рядом раскладчика: тасование потребило то же
	 * число обращений, а гамма идёт следом. Сборщик до заполнения прочих мест не
	 * доходит, и снимать ему нечего лишнего
	 */
	for(size_t i = 0; i < length; i++){
		// Снимаем наложение с байта секрета
		const uint8_t byte = static_cast <uint8_t> (carrier[places[i]] ^ stream.byte());
		// Льём собранный байт в приёмник
		if(!vessel.pour(byte)){
			// Затираем и освобождаем приёмник
			vessel.wipe();
			// Отвечаем отказом
			return false;
		}
	}
	// Выводим результат выполнения операции
	return true;
}
