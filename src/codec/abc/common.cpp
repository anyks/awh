/**
 * @file common.cpp
 * @date 2026-08-18
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
 * @brief Файл реализации общих объявлений бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the common declarations of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/common.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <limits>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция получения текстового описания кода отказа
 *
 * @param error код отказа разбора
 * @return      текстовое описание кода отказа
 *
 */
const char * awh::codec::abc::message(const error_t error) noexcept {
	/**
	 * Определяем код отказа разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE):
			return "no error";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL):
			return "internal parsing error";
		// Если запись оборвалась посреди значения
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF):
			return "record has ended in the middle of a value";
		// Если метка не опознана
		case static_cast <uint8_t> (error_t::UNKNOWN_TAG):
			return "tag is not recognised";
		// Если метка отведена под будущее
		case static_cast <uint8_t> (error_t::RESERVED_TAG):
			return "tag is reserved for the future";
		// Если объявленная длина недопустима
		case static_cast <uint8_t> (error_t::INVALID_LENGTH):
			return "declared length is inadmissible";
		// Если строка не отвечает кодировке UTF-8
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			return "string does not conform to the UTF-8 encoding";
		// Если число не представимо затребованным видом
		case static_cast <uint8_t> (error_t::NUMBER_OUT_OF_RANGE):
			return "number is not representable by the demanded kind";
		// Если запись числа неограниченной ширины повреждена
		case static_cast <uint8_t> (error_t::INVALID_BIGNUM):
			return "record of a number of an unlimited width is corrupted";
		// Если запись десятичного числа повреждена
		case static_cast <uint8_t> (error_t::INVALID_DECIMAL):
			return "record of a decimal number is corrupted";
		// Если конец вместимого встречен вне вместимого неопределённой длины
		case static_cast <uint8_t> (error_t::UNBALANCED_BREAK):
			return "end of a container is met outside of a container of an indefinite length";
		// Если отображение оборвалось на имени
		case static_cast <uint8_t> (error_t::MISSING_VALUE):
			return "mapping has ended on a name, there is no value after it";
		// Если имя поля отображения объявлено повторно
		case static_cast <uint8_t> (error_t::DUPLICATE_KEY):
			return "name of a field of a mapping is declared repeatedly";
		// Если глубина вложенности превышает допустимую
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED):
			return "depth of the nesting exceeds the admissible one";
		// Если длина строкового значения превышает допустимую
		case static_cast <uint8_t> (error_t::STRING_TOO_LONG):
			return "length of a string value exceeds the admissible one";
		// Если длина двоичного значения превышает допустимую
		case static_cast <uint8_t> (error_t::BLOB_TOO_LONG):
			return "length of a binary value exceeds the admissible one";
		// Если количество узлов документа превышает допустимое
		case static_cast <uint8_t> (error_t::TOO_MANY_NODES):
			return "number of the nodes of the document exceeds the admissible one";
		// Если за окончанием документа стоят октеты
		case static_cast <uint8_t> (error_t::TRAILING_OCTETS):
			return "octets after the end of the document";
		// Если запись пуста, а документ затребован
		case static_cast <uint8_t> (error_t::EMPTY_RECORD):
			return "record is empty while a document is demanded";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT):
			return "limit set by the settings of the parsing is exceeded";
		// Если именем поля отображения стоит вместимое
		case static_cast <uint8_t> (error_t::INVALID_KEY):
			return "a container stands as the name of a field of a mapping";
		// Если имена полей отображения идут не по возрастанию
		case static_cast <uint8_t> (error_t::UNORDERED_KEY):
			return "names of the fields of a mapping do not go in the ascending order";
		// Если неопределённая длина встречена при строгом виде записи
		case static_cast <uint8_t> (error_t::INDEFINITE_REFUSED):
			return "an indefinite length at the strict kind of the record";
		// Если вместимое не закрыто либо закрыто лишний раз
		case static_cast <uint8_t> (error_t::UNBALANCED_CONTAINER):
			return "container is not closed or is closed once too many";
		// Если значений вместимого больше объявленного
		case static_cast <uint8_t> (error_t::CONTAINER_OVERFLOW):
			return "there are more values of a container than declared";
		// Если заголовок не несёт опознавательной записи контейнера
		case static_cast <uint8_t> (error_t::INVALID_MAGIC):
			return "header does not carry the identifying record of the container";
		// Если вид записи контейнера не поддерживается
		case static_cast <uint8_t> (error_t::INVALID_VERSION):
			return "kind of the record of the container is not supported";
		// Если контрольная сумма заголовка не сошлась
		case static_cast <uint8_t> (error_t::INVALID_CHECKSUM):
			return "checksum of the header does not agree";
		// Если заголовок оборван
		case static_cast <uint8_t> (error_t::TRUNCATED_HEADER):
			return "header is truncated, its octets are lacking";
		// Если кадр оборван
		case static_cast <uint8_t> (error_t::TRUNCATED_CHUNK):
			return "chunk is truncated, its octets are lacking";
		// Если заголовок кадра не опознан
		case static_cast <uint8_t> (error_t::INVALID_CHUNK):
			return "header of the chunk is not recognised";
		// Если сжатие либо разжатие кадра отвечено отказом
		case static_cast <uint8_t> (error_t::COMPRESSION_FAILED):
			return "compression or decompression of the chunk is refused";
		// Если шифрование либо расшифровка кадра отвечены отказом
		case static_cast <uint8_t> (error_t::ENCRYPTION_FAILED):
			return "encryption or decryption of the chunk is refused";
		// Если оглавление контейнера не объявлено заголовком
		case static_cast <uint8_t> (error_t::MISSING_INDEX):
			return "index of the container is not declared by the header";
		// Если запись оглавления повреждена либо указывает за тело
		case static_cast <uint8_t> (error_t::INVALID_INDEX):
			return "row of the index is corrupted or points beyond the body";
		// Если работа чтения октетов контейнера отвечена отказом
		case static_cast <uint8_t> (error_t::UNREADABLE_SOURCE):
			return "work of the reading of the octets of the container is refused";
		// Если работа записи октетов контейнера отвечена отказом
		case static_cast <uint8_t> (error_t::UNWRITABLE_SINK):
			return "work of the writing of the octets of the container is refused";
		// Если запись снесена правкой контейнера
		case static_cast <uint8_t> (error_t::MISSING_RECORD):
			return "record is erased by an editing of the container";
		// Если запись подписи оборвана
		case static_cast <uint8_t> (error_t::TRUNCATED_SIGNATURE):
			return "record of the signature is truncated, its octets are lacking";
		// Если запись подписи повреждена
		case static_cast <uint8_t> (error_t::INVALID_SIGNATURE):
			return "record of the signature is corrupted";
		// Если подпись владельца контейнером не объявлена
		case static_cast <uint8_t> (error_t::UNSIGNED_CONTAINER):
			return "signature of the owner is not declared by the container";
		// Если подпись владельца контейнера не сошлась
		case static_cast <uint8_t> (error_t::REFUSED_SIGNATURE):
			return "signature of the owner of the container does not agree";
		// Если выработка подписи владельца отвечена отказом
		case static_cast <uint8_t> (error_t::SIGNING_FAILED):
			return "production of the signature of the owner is refused";
		// Если значение, собираемое кусками, несёт кусок иного вида
		case static_cast <uint8_t> (error_t::INVALID_SEGMENT):
			return "value assembled by the chunks carries a chunk of another kind";
		// Если запись открытого расширения повреждена
		case static_cast <uint8_t> (error_t::INVALID_EXTENSION):
			return "record of the open extension is damaged";
		// Если метка несёт запись шире наименьшей при строгом виде записи
		case static_cast <uint8_t> (error_t::NON_MINIMAL_TAG):
			return "tag carries a record wider than the smallest one at the strict kind of the record";
	}
	// Выводим результат по умолчанию
	return "unknown error";
}
/**
 * @brief Функция получения названия вида узла
 *
 * @param kind вид узла документа
 * @return     название вида узла
 *
 */
const char * awh::codec::abc::name(const kind_t kind) noexcept {
	/**
	 * Определяем вид узла документа
	 */
	switch(static_cast <uint8_t> (kind)){
		// Если узел не определён
		case static_cast <uint8_t> (kind_t::NONE):
			return "none";
		// Если узел является пустым значением
		case static_cast <uint8_t> (kind_t::NUL):
			return "null";
		// Если узел является логическим значением
		case static_cast <uint8_t> (kind_t::BOOL):
			return "bool";
		// Если узел является числом
		case static_cast <uint8_t> (kind_t::NUMBER):
			return "number";
		// Если узел является строкой
		case static_cast <uint8_t> (kind_t::STRING):
			return "string";
		// Если узел является двоичными данными
		case static_cast <uint8_t> (kind_t::BLOB):
			return "blob";
		// Если узел является отметкой времени
		case static_cast <uint8_t> (kind_t::TIME):
			return "time";
		// Если узел является опознавателем
		case static_cast <uint8_t> (kind_t::UUID):
			return "uuid";
		// Если узел является массивом
		case static_cast <uint8_t> (kind_t::ARRAY):
			return "array";
		// Если узел является отображением
		case static_cast <uint8_t> (kind_t::MAP):
			return "map";
		// Если узел является открытым расширением
		case static_cast <uint8_t> (kind_t::CUSTOM):
			return "custom";
	}
	// Выводим результат по умолчанию
	return "none";
}
/**
 * @brief Функция получения названия вида значения
 *
 * @param type вид значения документа
 * @return     название вида значения
 *
 */
const char * awh::codec::abc::name(const type_t type) noexcept {
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint32_t> (type)){
		// Если значения нет вовсе
		case static_cast <uint32_t> (type_t::UNDEFINED):
			return "undefined";
		// Если значение является пустым
		case static_cast <uint32_t> (type_t::NUL):
			return "null";
		// Если значение является логическим
		case static_cast <uint32_t> (type_t::BOOL):
			return "bool";
		// Если значение является строкой
		case static_cast <uint32_t> (type_t::STRING):
			return "string";
		// Если значение является двоичными данными
		case static_cast <uint32_t> (type_t::BLOB):
			return "blob";
		// Если значение является массивом
		case static_cast <uint32_t> (type_t::ARRAY):
			return "array";
		// Если значение является отображением
		case static_cast <uint32_t> (type_t::MAP):
			return "map";
		// Если значение является отметкой времени
		case static_cast <uint32_t> (type_t::TIME):
			return "time";
		// Если значение является опознавателем
		case static_cast <uint32_t> (type_t::UUID):
			return "uuid";
		// Если значение является целым со знаком шириною в один октет
		case static_cast <uint32_t> (type_t::INT8):
			return "int8";
		// Если значение является целым со знаком шириною в два октета
		case static_cast <uint32_t> (type_t::INT16):
			return "int16";
		// Если значение является целым со знаком шириною в четыре октета
		case static_cast <uint32_t> (type_t::INT32):
			return "int32";
		// Если значение является целым со знаком шириною в восемь октетов
		case static_cast <uint32_t> (type_t::INT64):
			return "int64";
		// Если значение является целым без знака шириною в один октет
		case static_cast <uint32_t> (type_t::UINT8):
			return "uint8";
		// Если значение является целым без знака шириною в два октета
		case static_cast <uint32_t> (type_t::UINT16):
			return "uint16";
		// Если значение является целым без знака шириною в четыре октета
		case static_cast <uint32_t> (type_t::UINT32):
			return "uint32";
		// Если значение является целым без знака шириною в восемь октетов
		case static_cast <uint32_t> (type_t::UINT64):
			return "uint64";
		// Если значение является дробным одинарной точности
		case static_cast <uint32_t> (type_t::FLOAT):
			return "float";
		// Если значение является дробным двойной точности
		case static_cast <uint32_t> (type_t::DOUBLE):
			return "double";
		// Если значение является целым, не вместимым ни в один родной вид
		case static_cast <uint32_t> (type_t::EXTENDED):
			return "extended";
		// Если значение является десятичным
		case static_cast <uint32_t> (type_t::DECIMAL):
			return "decimal";
		// Если значение является открытым расширением
		case static_cast <uint32_t> (type_t::CUSTOM):
			return "custom";
	}
	// Выводим результат по умолчанию
	return "undefined";
}
/**
 * @brief Функция получения вида узла по виду значения
 *
 * @param type вид значения документа
 * @return     вид узла документа
 *
 */
awh::codec::abc::kind_t awh::codec::abc::kind(const type_t type) noexcept {
	// Если вид значения является числом любого вида
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::NUMBER))
		// Выводим вид узла числа
		return kind_t::NUMBER;
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint32_t> (type)){
		// Если значение является пустым
		case static_cast <uint32_t> (type_t::NUL):
			return kind_t::NUL;
		// Если значение является логическим
		case static_cast <uint32_t> (type_t::BOOL):
			return kind_t::BOOL;
		// Если значение является строкой
		case static_cast <uint32_t> (type_t::STRING):
			return kind_t::STRING;
		// Если значение является двоичными данными
		case static_cast <uint32_t> (type_t::BLOB):
			return kind_t::BLOB;
		// Если значение является отметкой времени
		case static_cast <uint32_t> (type_t::TIME):
			return kind_t::TIME;
		// Если значение является опознавателем
		case static_cast <uint32_t> (type_t::UUID):
			return kind_t::UUID;
		// Если значение является массивом
		case static_cast <uint32_t> (type_t::ARRAY):
			return kind_t::ARRAY;
		// Если значение является отображением
		case static_cast <uint32_t> (type_t::MAP):
			return kind_t::MAP;
		// Если значение является открытым расширением
		case static_cast <uint32_t> (type_t::CUSTOM):
			return kind_t::CUSTOM;
	}
	// Выводим результат по умолчанию
	return kind_t::NONE;
}
/**
 * @brief Функция сборки ведущего октета значения
 *
 * @param group  крупный вид проволочной записи
 * @param detail подробность метки
 * @return       собранный ведущий октет
 *
 */
uint8_t awh::codec::abc::tag(const group_t group, const uint8_t detail) noexcept {
	// Выполняем сборку ведущего октета, укладывая крупный вид в три старших разряда
	return static_cast <uint8_t> ((static_cast <uint8_t> (group) << 5) | (detail & 0x1F));
}
/**
 * @brief Функция извлечения крупного вида из ведущего октета
 *
 * @param tag ведущий октет значения
 * @return    крупный вид проволочной записи
 *
 */
awh::codec::abc::group_t awh::codec::abc::group(const uint8_t tag) noexcept {
	// Выводим крупный вид, снятый с трёх старших разрядов ведущего октета
	return static_cast <group_t> (tag >> 5);
}
/**
 * @brief Функция извлечения подробности из ведущего октета
 *
 * @param tag ведущий октет значения
 * @return    подробность метки
 *
 */
uint8_t awh::codec::abc::detail(const uint8_t tag) noexcept {
	// Выводим подробность, снятую с пяти младших разрядов ведущего октета
	return static_cast <uint8_t> (tag & 0x1F);
}
/**
 * @brief Функция получения ширины записи, ведомой подробностью метки
 *
 * @param detail подробность метки
 * @param result ширина ведомой записи в октетах
 * @return       признак опознания подробности
 *
 */
bool awh::codec::abc::width(const uint8_t detail, uint8_t & result) noexcept {
	// Выполняем сброс ширины ведомой записи
	result = 0;
	// Если подробность несёт само значение
	if(detail <= INLINE_LIMIT)
		// Сообщаем, что ведомой записи за меткой нет
		return true;
	/**
	 * Определяем подробность метки
	 */
	switch(detail){
		// Если за меткой стоит один октет
		case (INLINE_LIMIT + 1): {
			// Выполняем установку ширины ведомой записи
			result = 1;
			// Сообщаем, что подробность опознана
			return true;
		}
		// Если за меткой стоят два октета
		case (INLINE_LIMIT + 2): {
			// Выполняем установку ширины ведомой записи
			result = 2;
			// Сообщаем, что подробность опознана
			return true;
		}
		// Если за меткой стоят четыре октета
		case (INLINE_LIMIT + 3): {
			// Выполняем установку ширины ведомой записи
			result = 4;
			// Сообщаем, что подробность опознана
			return true;
		}
		// Если за меткой стоят восемь октетов
		case (INLINE_LIMIT + 4): {
			// Выполняем установку ширины ведомой записи
			result = 8;
			// Сообщаем, что подробность опознана
			return true;
		}
	}
	// Сообщаем, что подробность ширины не ведёт
	return false;
}
/**
 * @brief Функция получения наименьшей подробности, вмещающей значение
 *
 * @param value укладываемое значение
 * @return      подробность метки
 *
 */
uint8_t awh::codec::abc::fit(const uint64_t value) noexcept {
	// Если значение укладывается в саму метку
	if(value <= static_cast <uint64_t> (INLINE_LIMIT))
		// Выводим значение подробностью метки
		return static_cast <uint8_t> (value);
	// Если значение укладывается в один октет
	else if(value <= static_cast <uint64_t> (numeric_limits <uint8_t>::max()))
		// Выводим подробность записи шириною в один октет
		return static_cast <uint8_t> (INLINE_LIMIT + 1);
	// Если значение укладывается в два октета
	else if(value <= static_cast <uint64_t> (numeric_limits <uint16_t>::max()))
		// Выводим подробность записи шириною в два октета
		return static_cast <uint8_t> (INLINE_LIMIT + 2);
	// Если значение укладывается в четыре октета
	else if(value <= static_cast <uint64_t> (numeric_limits <uint32_t>::max()))
		// Выводим подробность записи шириною в четыре октета
		return static_cast <uint8_t> (INLINE_LIMIT + 3);
	// Выводим подробность записи шириною в восемь октетов
	return static_cast <uint8_t> (INLINE_LIMIT + 4);
}

/**
 * @brief Функция выдачи очередного звена пути
 *
 * @details Разбор пути держится здесь, в одном месте на всех: правила эти нужны четверым -
 *          извлечению и заведению у владеющего значения, извлечению и опросу наличия у
 *          дерева разбора. Расписаны они у объявления работы
 *
 * @note Смещение, равное `npos`, означает исчерпанный путь. Заведено оно затем, что
 *       смещение, равное длине пути, законно и означает ПОСЛЕДНЕЕ звено, пустое: путь
 *       `a/` суть два звена, второе из них с пустым именем
 *
 * @note Дверей у признака недействительности ТРИ, и щупом 04.09.2026 проверена всякая:
 *       извлечение у курсора дерева и у владеющего значения стережёт
 *       `CodecAbcDocument.EscapedNamesAreReachableByPath`, а заведение по пути -
 *       `CodecAbcValue.PlacingByAnInvalidPathSpoilsNothing`. Третья дверь до того дня
 *       была ДЕКОРАТИВНА: обе ветви заслона выдавали одно и то же, и запись по
 *       недействительному пути затирала существующее поле
 *
 * @param path   разбираемый путь
 * @param offset смещение разбора, изменяемое работой
 * @param result выдаваемое звено пути
 * @return       признак выданного звена
 *
 */
bool awh::codec::abc::segment(const string_view path, size_t & offset, string_view & result,
 string & buffer, bool & invalid) noexcept {
	// Выполняем сброс признака недействительности пути
	invalid = false;
	// Выполняем сброс выдаваемого звена пути
	result = string_view();
	/**
	 * Если путь исчерпан
	 */
	if(offset == string_view::npos)
		// Сообщаем, что звеньев пути более нет
		return false;
	/**
	 * Если разбор пути только начат
	 */
	if(offset == 0){
		/**
		 * Если путь пуст, он означает ВЕСЬ документ и звеньев не даёт вовсе
		 */
		if(path.empty()){
			// Выполняем объявление пути исчерпанным
			offset = string_view::npos;
			// Сообщаем, что звеньев пути нет
			return false;
		}
		/**
		 * Если путь начат косою чертой, она есть приставка указателя и звена не даёт
		 */
		if(path.front() == '/')
			// Выполняем пропуск приставки указателя
			offset = 1;
	}
	// Выполняем поиск конца выдаваемого звена пути
	const size_t bound = path.find('/', offset);
	// Выполняем получение длины выдаваемого звена пути
	const size_t length = ((bound == string_view::npos) ? (path.size() - offset) : (bound - offset));
	// Выполняем выдачу звена пути
	result = path.substr(offset, length);
	// Выполняем сдвиг смещения разбора за выданное звено пути
	offset = ((bound == string_view::npos) ? string_view::npos : (bound + 1));
	/**
	 * Если звено пути отменяющих записей не несёт, выдаётся оно как есть
	 *
	 * @note Вместилище отменённого заводится лишь по нужде: путь без знака отмены -
	 *       случай обычный, и отведения памяти он не требует вовсе
	 */
	if(result.find('~') == string_view::npos)
		// Сообщаем, что звено пути выдано
		return true;
	// Выполняем перенос звена пути во вместилище отменённого
	buffer.assign(result);
	/**
	 * Выполняем снятие отменяющих записей звена пути
	 *
	 * @note Порядок снятия предписан RFC 6901: снятие `~0` прежде `~1` обратило бы
	 *       записанное `~01` в косую черту вместо `~1`. Оттого разбор идёт слева
	 *       направо один раз, а не двумя проходами
	 *
	 * @note Продолжение розыска сдвигается ЗА подставленный знак: розыск с прежнего
	 *       места снял бы подставленный знак отмены вторично, обратив `~001` в `~1`
	 *       вместо `~01`
	 *
	 */
	for(size_t i = buffer.find('~'); i != string::npos; i = buffer.find('~', i + 1)){
		/**
		 * Если за знаком отмены не осталось знаков, путь недействителен
		 */
		if((i + 1) >= buffer.size()){
			// Выполняем объявление пути недействительным
			invalid = true;
			// Сообщаем, что звена пути не выдано
			return false;
		}
		/**
		 * Определяем отменяющую запись звена пути
		 */
		switch(buffer[i + 1]){
			// Если записана косая черта
			case '1':
				// Выполняем подмену отменяющей записи косой чертой
				buffer.replace(i, 2, "/");
			break;
			// Если записан сам знак отмены
			case '0':
				// Выполняем подмену отменяющей записи знаком отмены
				buffer.replace(i, 2, "~");
			break;
			/**
			 * Если за знаком отмены стоит иное, путь недействителен
			 *
			 * @note Строгость эта предписана стандартом и взята единою со всею рамкой:
			 *       у указателя JSON знак отмены, за каким не стоит нуля либо единицы,
			 *       отвергается тем же порядком. Мягкость здесь означала бы, что имя
			 *       `t~x` достижимо двояко, а `t~1x` - неоднозначно
			 */
			default: {
				// Выполняем объявление пути недействительным
				invalid = true;
				// Сообщаем, что звена пути не выдано
				return false;
			}
		}
	}
	// Выполняем выдачу звена пути с снятыми отменяющими записями
	result = buffer;
	// Сообщаем, что звено пути выдано
	return true;
}

/**
 * @brief Метод разбора звена пути на номер значения
 *
 * @param segment разбираемое звено пути
 * @param result  разобранный номер значения
 * @return        признак того, что звено является номером
 *
 */
bool awh::codec::abc::indexed(const string_view segment, size_t & result) noexcept {
	// Выполняем сброс разобранного номера значения
	result = 0;
	// Если звено пути пусто
	if(segment.empty())
		// Сообщаем, что звено номером не является
		return false;
	/**
	 * Если запись номера имеет ведущий нуль, номером она не является, а является
	 * именем поля. Правило это взято у RFC 6901: без него `01` и `1` означали бы
	 * одно и то же, и путь перестал бы задавать значение однозначно
	 */
	if((segment.size() > 1) && (segment.front() == '0'))
		// Сообщаем, что звено номером не является
		return false;
	/**
	 * Выполняем перебор всех знаков звена пути
	 */
	for(const char letter : segment){
		// Если знак цифрой не является
		if((letter < '0') || (letter > '9')){
			// Выполняем сброс разобранного номера значения
			result = 0;
			// Сообщаем, что звено номером не является
			return false;
		}
		// Выполняем получение разряда номера значения
		const size_t digit = static_cast <size_t> (letter - '0');
		/**
		 * Если накопление разряда переполнило бы номер, отвергаем запись целиком.
		 * Приведение её к пределу выдало бы значение с иным номером, а вместимого
		 * такой длины не бывает вовсе
		 */
		if(result > ((numeric_limits <size_t>::max() - digit) / 10)){
			// Выполняем сброс разобранного номера значения
			result = 0;
			// Сообщаем, что звено номером не является
			return false;
		}
		// Выполняем накопление разряда номера значения
		result = ((result * 10) + digit);
	}
	// Сообщаем, что звено является номером
	return true;
}
