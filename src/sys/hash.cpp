/**
 * @file: hash.cpp
 * @date: 2023-02-11
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
 * Подключаем заголовочный файл
 */
#include <sys/hash.hpp>

/**
 * Подключаем A-NTS
 */
#include <nts/cipher/aes.hpp>
#include <nts/codec/base64.hpp>
#include <nts/digest/hash.hpp>
#include <nts/digest/hmac.hpp>
#include <nts/digest/pbkdf2.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Устанавливаем уровень сжатия
 */
static constexpr uint16_t DEFAULT_MEM_LEVEL = 4;
/**
 * Размер буфера чанка в байтах
 */
static constexpr uint32_t CHUNK_BUFFER_SIZE = 0x4000;

/**
 * @brief Шаблон функции хэширования текста
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция хэширования текста
 *
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result результат хэширования
 */
static void hashing(const string & text, const awh::hash_t::type_t type, T & result) noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Выполняем отображение типа хэш-суммы на алгоритм A-NTS: нумерация MD5…SHA512 у них совпадает
			const nts::digest::hash_t algorithm = static_cast <nts::digest::hash_t> (static_cast <uint8_t> (type));
			// Получаем длину суммы
			const size_t length = nts::digest::length(algorithm);
			// Если алгоритм не распознан, выходим
			if(length == 0)
				// Выходим из функции
				return;
			// Выделяем память для промежуточных значений
			vector <uint8_t> digest(length, 0);
			// Выделяем память для буфера данных
			result.resize((length * 2) + 1, 0);
			// Выполняем расчет суммы
			if(!nts::digest::hash(algorithm, nts::view_t(reinterpret_cast <const uint8_t *> (text.data()), text.size()), digest.data(), length)){
				// Выполняем очистку блока с результатом
				result.clear();
				// Выходим из функции
				return;
			}
			/**
			 * Заполняем строку данными хэш-суммы
			 */
			for(size_t i = 0; i < length; i++)
				// Формируем данные хэш-суммы
				::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
			// Удаляем последний символ
			result.pop_back();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции хэширования текста с ключом
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция хэширования текста с ключом
 *
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result результат хэширования
 */
static void hmac(const string & key, const string & text, const awh::hash_t::type_t type, T & result) noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Выполняем отображение типа хэш-суммы на алгоритм A-NTS: нумерация MD5…SHA512 у них совпадает
			const nts::digest::hash_t algorithm = static_cast <nts::digest::hash_t> (static_cast <uint8_t> (type));
			// Получаем длину подписи
			const size_t length = nts::digest::length(algorithm);
			// Если алгоритм не распознан, выходим
			if(length == 0)
				// Выходим из функции
				return;
			// Выделяем память для промежуточных значений
			vector <uint8_t> digest(length, 0);
			// Выделяем память для буфера данных
			result.resize((length * 2) + 1, 0);
			// Выполняем получение подписи
			if(!nts::digest::hmac(algorithm, nts::view_t(reinterpret_cast <const uint8_t *> (key.data()), key.size()),
				nts::view_t(reinterpret_cast <const uint8_t *> (text.data()), text.size()), digest.data(), length)){
				// Выполняем очистку блока с результатом
				result.clear();
				// Выходим из функции
				return;
			}
			/**
			 * Заполняем строку данными подписи
			 */
			for(size_t i = 0; i < length; i++)
				// Формируем данные хэш-суммы
				::sprintf(&result[i * 2], "%02x", static_cast <uint32_t> (digest[i]));
			// Удаляем последний символ
			result.pop_back();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Функция кодирования данных в BASE64
 *
 * @tparam T сигнатура функции
 *
 */
template <typename T>
/**
 * @brief Функция кодирования данных в BASE64
 *
 * @param buffer буфер данных для кодирования
 * @param size   размер данных для кодирования
 * @param result строка куда следует положить результат
 *
 */
static void encode64(const char * buffer, const size_t size, T & result) noexcept {
	// Байты, которые следует записать
	const nts::view_t data(reinterpret_cast <const uint8_t *> (buffer), size);
	// Запись, собранная наружным модулем
	string text = "";
	/**
	 * Заполнение ставим явным: прежний вывод писал «=» на конце и не переносил строк,
	 * и без заполнений строки хешей в настройках перестали бы читаться
	 */
	if(nts::codec::encode(data, text, true)){
		// Выполняем перенос записи в буфер вызывающего
		result.assign(text.begin(), text.end());
		// Выходим из функции
		return;
	}
	// Выполняем очистку блока с результатом
	result.clear();
}
/**
 * @brief Шаблон функции декодирования BASE64
 *
 * @tparam T сигнатура функции
 *
 */
template <typename T>
/**
 * @brief Функция декодирования BASE64
 *
 * @param buffer буфер данных для декодирования
 * @param size   размер данных для декодирования
 * @param result строка куда следует положить результат
 * @return       результат декодирования
 *
 */
static bool decode64(const char * buffer, const size_t size, T & result) noexcept {
	// Запись, которую следует разобрать
	const nts::view_t data(reinterpret_cast <const uint8_t *> (buffer), size);
	// Байты, разобранные наружным модулем
	vector <uint8_t> bytes;
	/**
	 * Пробельные знаки разрешаем пропускать: прежний разбор мирился с переносами строк
	 * внутри записи, и отказ на них сломал бы чтение настроенных значений. Сам разбор при
	 * этом стал строгим — посторонний знак, «=» вне хвоста и ненулевые отброшенные разряды
	 * отвергаются, тогда как прежний разбор их молча съедал
	 */
	if(nts::codec::decode(data, bytes, nts::codec::alphabet_t::STANDARD, true)){
		// Выполняем перенос разобранных байтов в буфер вызывающего
		result.assign(bytes.begin(), bytes.end());
		// Выводим результат: пустого разбора считать успехом нельзя
		return !result.empty();
	}
	// Выполняем очистку блока с результатом
	result.clear();
	// Выводим результат
	return false;
}
/**
 * @brief Функция шифрования и расшифрования AES в режиме CFB128
 *
 * @param state  объект стейта шифрования
 * @param data   входные данные
 * @param size   размер входных данных
 * @param output место, куда следует положить результат
 * @param encode признак кодирования (true - шифрование, false - расшифрование)
 * @return       результат выполнения операции
 *
 */
static bool cfb128(awh::hash_t::state_t & state, const uint8_t * data, const size_t size, uint8_t * output, const bool encode) noexcept {
	// Создаём объект шифрования AES
	nts::cipher::aes_t aes;
	// Выполняем установку ключа шифрования
	if(!aes.key(nts::view_t(state.key, state.size)))
		// Выводим результат
		return false;
	// Определяем длину регистра обратной связи
	const size_t length = sizeof(state.ivec);
	// Положение в регистре обратной связи
	size_t position = static_cast <size_t> (state.num);
	/**
	 * Переходим по всем байтам данных
	 */
	for(size_t offset = 0; offset < size; offset++) {
		/**
		 * В начале регистра гамма вырабатывается заново: E(K, регистр). При неполном
		 * прохождении регистра гамма остаётся в ebuf, поэтому регистр не теряется
		 */
		if(position == 0) {
			// Выполняем шифрование регистра обратной связи
			if(!aes.encrypt(state.ivec, state.ebuf))
				// Выводим результат
				return false;
		}
		// Определяем текущий байт входных данных
		const uint8_t current = data[offset];
		// Выполняем наложение гаммы
		output[offset] = state.ebuf[position] ^ current;
		/**
		 * Обратная связь идёт по шифртексту в обе стороны: при расшифровании это входной байт,
		 * при шифровании — только что полученный
		 */
		state.ivec[position] = (encode ? output[offset] : current);
		// Переходим к следующей позиции регистра
		position = (position + 1) % length;
	}
	// Запоминаем положение в регистре для следующего куска потока
	state.num = static_cast <int32_t> (position);
	// Выводим результат
	return true;
}
/**
 * @brief Шаблон функции хэширования данных
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция хэширования данных
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param state  объект стейта шифрования
 * @param result строка куда следует положить результат
 */
static void hashing(const char * buffer, const size_t size, const awh::hash_t::cipher_t cipher, const awh::hash_t::event_t event, awh::hash_t::state_t & state, T & result) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			/**
			 * Определяем тип шифрования
			 */
			switch(static_cast <uint8_t> (cipher)){
				// Если производится работы с BASE64
				case static_cast <uint8_t> (awh::hash_t::cipher_t::BASE64): {
					/**
					 * Определяем событие кодирование или декодирование
					 */
					switch(static_cast <uint8_t> (event)){
						// Если производится кодирование данных
						case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE):
							// Выполняем кодирование в BASE64
							::encode64(buffer, size, result);
						break;
						// Если производится декодирование данных
						case static_cast <uint8_t> (awh::hash_t::event_t::DECODE):
							// Выполняем декодирование из BASE64
							::decode64(buffer, size, result);
						break;
					}
				} break;
				// Если производится работы с AES128
				case static_cast <uint8_t> (awh::hash_t::cipher_t::AES128):
				// Если производится работы с AES192
				case static_cast <uint8_t> (awh::hash_t::cipher_t::AES192):
				// Если производится работы с AES256
				case static_cast <uint8_t> (awh::hash_t::cipher_t::AES256): {
					// Выделяем память под результат: в CFB128 шифртекст равен данным по длине
					result.resize(size, 0);
					// Выполняем шифрование или расшифрование данных
					if(!::cfb128(state, reinterpret_cast <const uint8_t *> (buffer), size,
						reinterpret_cast <uint8_t *> (result.data()), (event == awh::hash_t::event_t::ENCODE)))
						// Выполняем очистку блока с результатом
						result.clear();
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции работы с компрессором LZma
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция работы с компрессором LZma
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 * @param limit  максимальный размер извлекаемых данных (0 - без ограничения)
 */
static void lzma(const char * buffer, const size_t size, const awh::hash_t::event_t event, T & result, const size_t limit = 0) noexcept {
	/**
	 * @brief Если буфер данных передан
	 *
	 * Нижней границы размера нет: прежнее условие (size > 80) отвергало корректные
	 * малые потоки .xz от других узлов и не давало сжимать короткие данные.
	 * Минимальный размер потока при декодировании проверяется ниже по заголовку и подвалу
	 *
	 */
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			/**
			 * Определяем событие выполнения операции
			 */
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Инициализируем опции компрессора LZma
					static const lzma_options_lzma options = {
						1u << 20u, nullptr, 0, LZMA_LC_DEFAULT, LZMA_LP_DEFAULT,
						LZMA_PB_DEFAULT, LZMA_MODE_FAST, 128, LZMA_MF_HC3, 4
					};
					// Инициализируем фильтры компрессора LZma
					static const lzma_filter filters[] = {
						{LZMA_FILTER_LZMA2, const_cast <lzma_options_lzma *> (&options)},
						{LZMA_VLI_UNKNOWN, nullptr}
					};
					// Актуальный размер сжатых данных
					size_t actual = 0;
					// Выделяем буфер памяти нужного нам размера
					result.resize(::lzma_stream_buffer_bound(size), 0);
					// Выполняем компрессию буфера данных
					lzma_ret rv = ::lzma_stream_buffer_encode(const_cast <lzma_filter *> (filters), LZMA_CHECK_NONE, nullptr, reinterpret_cast <const uint8_t *> (buffer), size, reinterpret_cast <uint8_t *> (result.data()), &actual, result.size());
					// Если мы получили ошибку
					if(rv != LZMA_OK){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Корректируем размер результирующего буфера
					result.resize(actual);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Указатель позиции в буфере для распаковки
					char * ptr = nullptr;
					// Индекс потока LZma компрессора
					lzma_index * index = nullptr;
					// Лимит доступной памяти
					uint64_t memlimit = 0x8000000;
					// Позиции в буферах и актуальный размер данных результата
					size_t inpos = 0, outpos = 0, actual = 0;
					// Если данных меньше, чем занимают заголовок и подвал потока
					if(size < static_cast <size_t> (LZMA_STREAM_HEADER_SIZE * 2))
						// Переходим к выводу ошибки
						goto Error;
					// Смещаем указатель в буфере на подвал
					ptr = (const_cast <char *> (buffer) + size - LZMA_STREAM_HEADER_SIZE);
					// Список флагов потока LZma
					lzma_stream_flags flags;
					// Пытаемся декодировать подвал архива
					if(::lzma_stream_footer_decode(&flags, reinterpret_cast <uint8_t *> (ptr)) != LZMA_OK)
						// Переходим к выводу ошибки
						goto Error;
					// Если буфер данных испорчен (индекс не помещается между заголовком и подвалом)
					if(flags.backward_size > static_cast <lzma_vli> (size - (LZMA_STREAM_HEADER_SIZE * 2)))
						// Переходим к выводу ошибки
						goto Error;
					// Смещаем указатель в буфере на индекс
					ptr -= flags.backward_size;
					// Выполняем декодирование буфера LZma
					if(::lzma_index_buffer_decode(&index, &memlimit, nullptr, reinterpret_cast <uint8_t *> (ptr), &inpos, size - (ptr - buffer)) != LZMA_OK)
						// Переходим к выводу ошибки
						goto Error;
					// Сбрасываем иозицию во входящем буфере
					inpos = 0;
					// Сбрасываем лимит доступной памяти
					memlimit = 0x8000000;
					// Получаем размер результирующего буфера данных
					actual = ::lzma_index_uncompressed_size(index);
					/**
					 * Размер результата берётся из индекса архива, то есть задаётся отправителем,
					 * поэтому перед выделением памяти сверяем его с допустимым пределом
					 */
					if((limit > 0) && (actual > limit))
						// Переходим к выводу ошибки
						goto Error;
					// Выделяем буфер памяти нужного нам размера
					result.resize(actual, 0);
					// Выполняем декомпрессию буфера бинарных данных
					if(::lzma_stream_buffer_decode(&memlimit, 0, nullptr, reinterpret_cast <const uint8_t *> (buffer), &inpos, size, reinterpret_cast <uint8_t *> (result.data()), &outpos, actual) == LZMA_OK){
						// Выполняем закрытие индекса компрессора LZma
						::lzma_index_end(index, nullptr);
						// Выходим из функции
						return;
					}
					// Устанавливаем метку вывода ошибки
					Error:
					// Если индекс компрессора LZma был создан
					if(index != nullptr)
						// Выполняем закрытие индекса компрессора LZma
						::lzma_index_end(index, nullptr);
					// Выполняем очистку результата
					result.clear();
					// Выходим из функции
					return;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции работы с компрессором BZip2
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция работы с компрессором BZip2
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 * @param limit  максимальный размер извлекаемых данных (0 - без ограничения)
 */
static void bzip2(const char * buffer, const size_t size, const awh::hash_t::event_t event, T & result, const size_t limit = 0) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Результат выполнения компрессии
			int32_t rv = BZ_OK;
			// Выполняем создание объекта потока
			bz_stream stream;
			// Выполняем зануление параметров потока
			stream.bzfree  = nullptr;
			stream.opaque  = nullptr;
			stream.bzalloc = nullptr;
			/**
			 * Определяем событие выполнения операции
			 */
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Выполняем инициализацию потока
					if(::BZ2_bzCompressInit(&stream, 5, 0, 0) != BZ_OK){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выделяем память на результирующий буфер
					result.resize(size + (size / 100) + 600, 0);
					// Указываем размер входного буфера
					stream.avail_in = static_cast <uint32_t> (size);
					// Заполняем входные данные буфера
					stream.next_in = const_cast <char *> (buffer);
					// Устанавливаем буфер для получения результата
					stream.next_out = result.data();
					// Устанавливаем максимальный размер буфера
					stream.avail_out = static_cast <uint32_t> (result.size());
					/**
					 * Выполняем компрессию буфера бинарных данных
					 */
					while((rv = ::BZ2_bzCompress(&stream, BZ_FINISH)) != BZ_STREAM_END){
						// Выполняем ещё одну попытку компрессии
						rv = ::BZ2_bzCompress(&stream, BZ_FINISH);
						// Если произошла ошибка компрессии
						if((rv != BZ_FINISH_OK) && (rv != BZ_STREAM_END))
							// Выходим из цикла
							break;
					}
					// Если данные обработаны удачно
					if((rv == BZ_FINISH_OK) || (rv == BZ_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + (result.size() - stream.avail_out), result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Выполняем очистку объекта потока
					::BZ2_bzCompressEnd(&stream);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Выполняем инициализацию потока
					if(::BZ2_bzDecompressInit(&stream, 0, 0) != BZ_OK){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Заполняем входные данные буфера
					stream.next_in = const_cast <char *> (buffer);
					// Указываем размер входного буфера
					stream.avail_in = static_cast <uint32_t> (size);
					/**
					 * @brief Количество уже извлечённых данных
					 *
					 * Размеры храним в size_t: 32-битный счётчик при удвоении
					 * переполнялся после 2 ГиБ, буфер сжимался, и запись уходила за его конец
					 *
					 */
					size_t offset = 0;
					/**
					 * Буфер допускаем на один байт больше предела: так результат ровно
					 * в размер предела извлекается успешно, а превышение хотя бы на байт
					 * обнаруживается по заполнению буфера
					 */
					const size_t capacity = (((limit > 0) && (limit < std::numeric_limits <size_t>::max())) ? (limit + 1) : limit);
					// Размер буфера извлечённых данных
					size_t actual = (size * 2);
					// Если размер буфера превышает допустимый предел
					if((capacity > 0) && (actual > capacity))
						// Ограничиваем размер буфера пределом
						actual = capacity;
					// Выделяем память на результирующий буфер
					result.resize(actual, 0);
					/**
					 * Выполняем компрессию всех данных
					 */
					do {
						// Если место для извлечения данных закончилось
						if((actual - offset) == 0){
							// Если предел извлекаемых данных уже достигнут
							if((capacity > 0) && (actual >= capacity)){
								// Помечаем декомпрессию как неудачную
								rv = BZ_MEM_ERROR;
								// Выходим из цикла
								break;
							}
							// Увеличиваем буфер исходящих данных в два раза
							actual *= 2;
							// Если размер буфера превышает допустимый предел
							if((capacity > 0) && (actual > capacity))
								// Ограничиваем размер буфера пределом
								actual = capacity;
							// Выделяем пмять для буфера извлечения данных
							result.resize(actual, 0);
						}
						// Запоминаем размер оставшихся входящих данных
						const uint32_t availIn = stream.avail_in;
						// Устанавливаем буфер для получения результата
						stream.next_out = (result.data() + offset);
						// Устанавливаем максимальный размер буфера
						stream.avail_out = static_cast <uint32_t> (std::min(actual - offset, static_cast <size_t> (std::numeric_limits <uint32_t>::max())));
						// Запоминаем размер свободного места в буфере
						const uint32_t availOut = stream.avail_out;
						// Выполняем декомпрессию
						rv = ::BZ2_bzDecompress(&stream);
						// Увеличиваем количество извлечённых данных
						offset += (availOut - stream.avail_out);
						// Если мы завершили сбор данных
						if((rv == BZ_STREAM_END) || (rv == BZ_FINISH_OK))
							// Выходим из цикла
							break;
						/**
						 * Обрезанный поток: входящие данные закончились, а BZ2_bzDecompress
						 * продолжает возвращать BZ_OK, ничего не извлекая. Без этой проверки
						 * цикл крутится бесконечно, поэтому такой поток считаем ошибкой
						 */
						if((rv == BZ_OK) && (availIn == stream.avail_in) && (availOut == stream.avail_out)){
							// Помечаем декомпрессию как неудачную
							rv = BZ_UNEXPECTED_EOF;
							// Выходим из цикла
							break;
						}
					/**
					 * Если данные ещё не извлечены
					 */
					} while(rv == BZ_OK);
					// Если извлечённые данные превышают допустимый предел
					if((limit > 0) && (offset > limit))
						// Помечаем декомпрессию как неудачную
						rv = BZ_MEM_ERROR;
					// Если данные обработаны удачно
					if((rv == BZ_FINISH_OK) || (rv == BZ_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + offset, result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Выполняем очистку объекта потока
					::BZ2_bzDecompressEnd(&stream);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции работы с компрессором Brotli
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция работы с компрессором Brotli
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 * @param limit  максимальный размер извлекаемых данных (0 - без ограничения)
 */
static void brotli(const char * buffer, const size_t size, const awh::hash_t::event_t event, T & result, const size_t limit = 0) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Получаем размер бинарного буфера входящих данных
			size_t sizeInput = size;
			// Создаём временный буфер данных
			vector <uint8_t> data(CHUNK_BUFFER_SIZE, 0);
			// Получаем бинарный буфер входящих данных
			const uint8_t * nextInput = reinterpret_cast <const uint8_t *> (buffer);
			/**
			 * Определяем событие выполнения операции
			 */
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Инициализируем стейт энкодера Brotli
					BrotliEncoderState * encoder = ::BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
					/**
					 * Выполняем сжатие данных
					 */
					while(!::BrotliEncoderIsFinished(encoder)){
						// Получаем размер буфера закодированных бинарных данных
						size_t sizeOutput = data.size();
						// Получаем буфер закодированных бинарных данных
						uint8_t * nextOutput = data.data();
						// Если сжатие данных закончено, то завершаем работу
						if(!::BrotliEncoderCompressStream(encoder, BROTLI_OPERATION_FINISH, &sizeInput, &nextInput, &sizeOutput, &nextOutput, nullptr))
							// Выходим из цикла
							break;
						// Получаем размер полученных данных
						const size_t size = (data.size() - sizeOutput);
						// Если данные получены, формируем результирующий буфер
						if(size > 0){
							// Получаем буфер данных
							const char * buffer = reinterpret_cast <const char *> (data.data());
							// Формируем результирующий буфер бинарных данных
							result.insert(result.end(), buffer, buffer + size);
						}
					}
					// Освобождаем память энкодера
					::BrotliEncoderDestroyInstance(encoder);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Полный размер обработанных данных
					size_t total = 0, size = 0;
					// Активируем работу декодера
					BrotliDecoderResult rbr = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
					// Инициализируем стейт декодера Brotli
					BrotliDecoderState * decoder = ::BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
					// Если декодер не создан
					if(decoder == nullptr){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					/**
					 * Если декодеру есть с чем работать
					 */
					while(rbr == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT){
						// Получаем размер буфера декодированных бинарных данных
						size_t sizeOutput = data.size();
						// Получаем буфер декодированных бинарных данных
						char * nextOutput = reinterpret_cast <char *> (data.data());
						// Выполняем декодирование бинарных данных
						rbr = ::BrotliDecoderDecompressStream(decoder, &sizeInput, &nextInput, &sizeOutput, reinterpret_cast <uint8_t **> (&nextOutput), &total);
						// Если декодирование данных не выполнено
						if(rbr == BROTLI_DECODER_RESULT_ERROR)
							// Выходим из цикла
							break;
						// Получаем размер полученных данных
						size = (data.size() - sizeOutput);
						// Если извлечённые данные превышают допустимый предел
						if((limit > 0) && ((result.size() + size) > limit)){
							// Помечаем декомпрессию как неудачную
							rbr = BROTLI_DECODER_RESULT_ERROR;
							// Выходим из цикла
							break;
						}
						// Если данные получены, формируем результирующий буфер
						if(size > 0){
							// Получаем буфер данных
							const char * buffer = reinterpret_cast <const char *> (data.data());
							// Формируем результирующий буфер бинарных данных
							result.insert(result.end(), buffer, buffer + size);
						}
					}
					/**
					 * Успехом считается только полностью завершённый поток: результат
					 * BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT означает, что поток обрезан
					 */
					if(rbr != BROTLI_DECODER_RESULT_SUCCESS)
						// Выполняем очистку результата
						result.clear();
					// Освобождаем память декодера
					::BrotliDecoderDestroyInstance(decoder);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции работы с компрессором Lz4
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция работы с компрессором Lz4
 *
 * @param buffer буфер данных
 * @param size   размер данных
 * @param level  уровень компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 * @param limit  максимальный размер извлекаемых данных (0 - без ограничения)
 */
static void lz4(const char * buffer, const size_t size, const uint32_t level, const awh::hash_t::event_t event, T & result, const size_t limit = 0) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			/**
			 * Определяем событие выполнения операции
			 */
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Выполняем получение размер результирующего буфера
					int32_t actual = ::LZ4_compressBound(size);
					// Если размер выделен
					if(actual <= 0){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выделяем буфер памяти нужного нам размера
					result.resize(actual, 0);
					// Выполняем компрессию буфера бинарных данных
					actual = ::LZ4_compress_fast(buffer, result.data(), size, actual, level);
					/**
					 * @brief Если компрессия не выполнена
					 *
					 * Результат больше исходных данных допустим: буфер выделен по LZ4_compressBound,
					 * а малые и несжимаемые данные в Lz4 всегда немного растут. Прежний отказ
					 * при росте больше чем на 10% оставлял пустой результат для коротких тел
					 *
					 */
					if(actual <= 0){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Корректируем размер результирующего буфера
					result.resize(actual);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					/**
					 * @brief Если размер входящих данных не помещается в формат Lz4
					 *
					 * Блочный формат Lz4 не хранит размер исходных данных, а ошибка
					 * LZ4_decompress_safe не отличает нехватку места от испорченных данных.
					 * Поэтому буфер наращиваем только до верхней границы: Lz4 не сжимает
					 * сильнее чем в 255 раз, а также не выходим за допустимый предел.
					 * Без этой границы испорченные данные раздували буфер бесконечно
					 *
					 */
					if(size > static_cast <size_t> (LZ4_MAX_INPUT_SIZE)){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Максимальный размер буфера извлечённых данных
					size_t bound = ((size * 255) + 16);
					// Если верхняя граница превышает допустимый предел
					if((limit > 0) && (bound > limit))
						// Ограничиваем верхнюю границу пределом
						bound = limit;
					// Если верхняя граница не помещается в формат Lz4
					if(bound > static_cast <size_t> (std::numeric_limits <int32_t>::max()))
						// Ограничиваем верхнюю границу форматом Lz4
						bound = static_cast <size_t> (std::numeric_limits <int32_t>::max());
					// Размер буфера извлечённых данных
					size_t length = std::min(size * 2, bound);
					/**
					 * Выполняем извлечение данных пока не извлечём
					 */
					for(;;){
						// Выделяем буфер памяти нужного нам размера
						result.resize(length, 0);
						// Выполняем получение размер результирующего буфера
						int32_t actual = static_cast <int32_t> (result.size());
						// Выполняем декомпрессию буфера бинарных данных
						actual = ::LZ4_decompress_safe(buffer, result.data(), static_cast <int32_t> (size), actual);
						// Если компрессия не выполнена из-за отсутствия памяти
						if(actual < 0){
							// Если верхняя граница уже достигнута
							if(length >= bound){
								// Выполняем очистку результата
								result.clear();
								// Выходим из функции
								return;
							}
							// Увеличиваем буфер в два раза, но не выше верхней границы
							length = std::min(length * 2, bound);
						// Если компрессия не выполнена
						} else if(actual == 0){
							// Выполняем очистку результата
							result.clear();
							// Выходим из функции
							return;
						// Если данные извлечены удачно
						} else {
							// Корректируем размер результирующего буфера
							result.resize(actual);
							// Выходим из цикла
							break;
						}
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции работы с компрессором Zstandard
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция работы с компрессором Zstandard
 *
 * @param buffer буфер данных
 * @param size   размер данных
 * @param level  уровень компрессии
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 * @param limit  максимальный размер извлекаемых данных (0 - без ограничения)
 */
static void zstd(const char * buffer, const size_t size, const uint32_t level, const awh::hash_t::event_t event, T & result, const size_t limit = 0) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			/**
			 * Определяем событие выполнения операции
			 */
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Выполняем создание контекста потока
					ZSTD_CStream * ctx = ::ZSTD_createCStream();
					// Если контекст потока создан
					if(ctx == nullptr){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выполняем инициализацию потока
					size_t status = ::ZSTD_initCStream(ctx, level);
					// Если мы получили ошибку инициализации
					if(::ZSTD_isError(status)){
						// Выполняем удаление потока
						::ZSTD_freeCStream(ctx);
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Zstandard: %s\n\n", ::ZSTD_getErrorName(status));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Инициализируем переменные смещения в буфере и актуальный размер данных
					size_t offset = 0, actual = 0;
					// Получаем длину итогового буфера данных
					const size_t length = ::ZSTD_CStreamOutSize();
					// Выполняем инициализацию итогового буфера данных
					const auto data = std::make_unique <char []> (length);
					// Выполняем создание буфера исходящих данных
					ZSTD_outBuffer output = {data.get(), length, 0};
					/**
					 * Выполняем обработку всех входящих данных
					 */
					while(offset < size){
						// Определяем актуальный размер данных
						actual = (((size - offset) > static_cast <size_t> (::ZSTD_CStreamInSize())) ? static_cast <size_t> (::ZSTD_CStreamInSize()) : (size - offset));
						// Выполняем создание буфера данных для входящих сжатых данных
						ZSTD_inBuffer input = {buffer + offset, actual, 0};
						/**
						 * Выполняем обработку до тех пор пока все не обработаем
						 */
						while(input.pos < input.size){
							// Сбрасываем позицию буфера
							output.pos = 0;
							// Выполняем компрессию полученных данных
							status = ::ZSTD_compressStream(ctx, &output, &input);
							// Если мы получили ошибку инициализации
							if(::ZSTD_isError(status)){
								// Выполняем удаление потока
								::ZSTD_freeCStream(ctx);
								// Выводим сообщение об ошибке
								::fprintf(stderr, "ERROR! Zstandard: %s\n\n", ::ZSTD_getErrorName(status));
								// Выполняем очистку результата
								result.clear();
								// Выходим из функции
								return;
							}
							// Выполняем формирование полученных данных
							result.insert(result.end(), data.get(), data.get() + output.pos);
						}
						// Увеличиваем смещение в исходном буфере необработанных данных
						offset += actual;
					}
					// Сбрасываем позицию буфера
					output.pos = 0;
					// Завершаем поток
					status = ::ZSTD_endStream(ctx, &output);
					// Если мы получили ошибку инициализации
					if(::ZSTD_isError(status)){
						// Выполняем удаление потока
						::ZSTD_freeCStream(ctx);
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Zstandard: %s\n\n", ::ZSTD_getErrorName(status));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выполняем формирование полученных данных
					result.insert(result.end(), data.get(), data.get() + output.pos);
					// Выполняем удаление потока
					::ZSTD_freeCStream(ctx);
					/*
					// Выполняем получение размер результирующего буфера
					size_t actual = ::ZSTD_compressBound(size);
					// Если размер выделен
					if(actual == 0){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выделяем буфер памяти нужного нам размера
					result.resize(actual, 0);
					// Выполняем компрессию буфера данных
					actual = ::ZSTD_compress(result.data(), actual, buffer, size, level);
					// Если мы получили ошибку
					if(::ZSTD_isError(actual)){
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Zstandard: %s\n\n", ::ZSTD_getErrorName(actual));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Корректируем размер результирующего буфера
					result.resize(actual);
					*/
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Выполняем создание контекста потока
					ZSTD_DStream * ctx = ::ZSTD_createDStream();
					// Если контекст потока создан
					if(ctx == nullptr){
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Выполняем инициализацию потока
					size_t status = ::ZSTD_initDStream(ctx);
					// Если мы получили ошибку инициализации
					if(::ZSTD_isError(status)){
						// Выполняем удаление потока
						::ZSTD_freeDStream(ctx);
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Zstandard: %s\n\n", ::ZSTD_getErrorName(status));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Инициализируем переменные смещения в буфере и актуальный размер данных
					size_t offset = 0, actual = 0;
					// Получаем длину итогового буфера данных
					const size_t length = ::ZSTD_DStreamOutSize();
					// Выполняем инициализацию итогового буфера данных
					const auto data = std::make_unique <char []> (length);
					// Выполняем создание буфера исходящих данных
					ZSTD_outBuffer output = {data.get(), length, 0};
					/**
					 * Выполняем обработку всех входящих данных
					 */
					while(offset < size){
						// Определяем актуальный размер данных
						actual = (((size - offset) > static_cast <size_t> (::ZSTD_DStreamInSize())) ? static_cast <size_t> (::ZSTD_DStreamInSize()) : (size - offset));
						// Выполняем создание буфера данных для входящих сжатых данных
						ZSTD_inBuffer input = {buffer + offset, actual, 0};
						/**
						 * Выполняем обработку до тех пор пока все не обработаем
						 */
						while(input.pos < input.size){
							// Сбрасываем позицию буфера
							output.pos = 0;
							// Выполняем декомпрессию полученных данных
							status = ::ZSTD_decompressStream(ctx, &output, &input);
							// Если мы получили ошибку инициализации
							if(::ZSTD_isError(status)){
								// Выполняем удаление потока
								::ZSTD_freeDStream(ctx);
								// Выводим сообщение об ошибке
								::fprintf(stderr, "ERROR! Zstandard: %s\n\n", ::ZSTD_getErrorName(status));
								// Выполняем очистку результата
								result.clear();
								// Выходим из функции
								return;
							}
							// Если извлечённые данные превышают допустимый предел
							if((limit > 0) && ((result.size() + output.pos) > limit)){
								// Выполняем удаление потока
								::ZSTD_freeDStream(ctx);
								// Выполняем очистку результата
								result.clear();
								// Выходим из функции
								return;
							}
							// Выполняем формирование полученных данных
							result.insert(result.end(), data.get(), data.get() + output.pos);
						}
						// Увеличиваем смещение в исходном буфере необработанных данных
						offset += actual;
					}
					// Выполняем удаление потока
					::ZSTD_freeDStream(ctx);
					/**
					 * Ненулевой статус после разбора всех входящих данных означает,
					 * что последний фрейм не завершён, то есть поток обрезан
					 */
					if(status != 0)
						// Выполняем очистку результата
						result.clear();
					/*
					// Получаем размер будущего фрейма (определяем размер контента)
					size_t actual = ::ZSTD_getFrameContentSize(buffer, size);
					// Если размер контента не получен или неизвестен
					if((actual == 0) || (actual == ZSTD_CONTENTSIZE_UNKNOWN) || (actual == ZSTD_CONTENTSIZE_ERROR))
						// Выполняем перерасчёт итогового размера
						actual = (size * 5);
					// Выделяем буфер памяти нужного нам размера
					result.resize(actual, 0);
					// Выполняем декомпрессию буфера данных
					actual = ::ZSTD_decompress(result.data(), actual, buffer, size);
					// Если мы получили ошибку
					if(::ZSTD_isError(actual)){
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Zstandard: %s\n\n", ::ZSTD_getErrorName(actual));
						// Выполняем очистку результата
						result.clear();
						// Выходим из функции
						return;
					}
					// Корректируем размер результирующего буфера
					result.resize(actual);
					*/
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции работы с компрессором GZip
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция работы с компрессором GZip
 *
 * @param buffer буфер данных
 * @param size   размер данных
 * @param level  уровень компрессии
 * @param wbit   размер скользящего окна
 * @param event  событие выполнения операции
 * @param result строка куда следует положить результат
 * @param limit  максимальный размер извлекаемых данных (0 - без ограничения)
 */
static void gzip(const char * buffer, const size_t size, const uint32_t level, const int16_t wbit, const awh::hash_t::event_t event, T & result, const size_t limit = 0) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Создаем поток zip
			z_stream zs;
			// Заполняем его нулями
			::memset(&zs, 0, sizeof(zs));
			/**
			 * Определяем событие выполнения операции
			 */
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					// Результирующий размер данных
					int32_t rv = Z_OK;
					// Если поток инициализировать не удалось, выходим
					if(::deflateInit2(&zs, level, Z_DEFLATED, wbit | 16, MOD_GZIP_ZLIB_CFACTOR, Z_DEFAULT_STRATEGY) == Z_OK){
						// Указываем размер входного буфера
						zs.avail_in = static_cast <uint32_t> (size);
						// Заполняем входные данные буфера
						zs.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
						// Выделяем память на результирующий буфер
						result.resize(::deflateBound(&zs, static_cast <uLong> (size)), 0);
						/**
						 * Выполняем компрессию всех данных
						 */
						do {
							// Устанавливаем буфер для получения результата
							zs.next_out = reinterpret_cast <Bytef *> (result.data() + zs.total_out);
							// Устанавливаем максимальный размер буфера
							zs.avail_out = static_cast <uint32_t> (result.size() - zs.total_out);
							// Выполняем сжатие
							rv = ::deflate(&zs, Z_FINISH);
							// Если произошла ошибка компрессии
							if((rv != Z_OK) && (rv != Z_STREAM_END))
								// Выходим из цикла
								break;
						/**
						 * Если данные ещё не сжаты
						 */
						} while(rv == Z_OK);
					}
					// Если данные обработаны удачно
					if((rv == Z_OK) || (rv == Z_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + (result.size() - zs.avail_out), result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Завершаем сжатие
					::deflateEnd(&zs);
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					// Результирующий размер данных
					int32_t rv = Z_OK;
					// Количество уже извлечённых данных
					size_t offset = 0;
					// Если поток инициализировать не удалось, выходим
					if(::inflateInit2(&zs, wbit | 16) == Z_OK){
						// Указываем размер входного буфера
						zs.avail_in = static_cast <uint32_t> (size);
						// Заполняем входные данные буфера
						zs.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (buffer));
						/**
						 * Размеры храним в size_t: 32-битный счётчик при удвоении
						 * переполнялся после 2 ГиБ, буфер сжимался, и запись уходила за его конец.
						 * Счётчик zs.total_out (uLong) в MS Windows тоже 32-битный, поэтому
						 * количество извлечённых данных считаем сами
						 *
						 * Буфер допускаем на один байт больше предела: так результат ровно
						 * в размер предела извлекается успешно, а превышение хотя бы на байт
						 * обнаруживается по заполнению буфера
						 */
						const size_t capacity = (((limit > 0) && (limit < std::numeric_limits <size_t>::max())) ? (limit + 1) : limit);
						// Размер буфера извлечённых данных
						size_t actual = (size * 2);
						// Если размер буфера превышает допустимый предел
						if((capacity > 0) && (actual > capacity))
							// Ограничиваем размер буфера пределом
							actual = capacity;
						// Выделяем память на результирующий буфер
						result.resize(actual, 0);
						/**
						 * Выполняем декомпрессию всех данных
						 */
						do {
							// Если место для извлечения данных закончилось
							if((actual - offset) == 0){
								// Если предел извлекаемых данных уже достигнут
								if((capacity > 0) && (actual >= capacity)){
									// Помечаем декомпрессию как неудачную
									rv = Z_MEM_ERROR;
									// Выходим из цикла
									break;
								}
								// Увеличиваем буфер исходящих данных в два раза
								actual *= 2;
								// Если размер буфера превышает допустимый предел
								if((capacity > 0) && (actual > capacity))
									// Ограничиваем размер буфера пределом
									actual = capacity;
								// Выделяем пмять для буфера извлечения данных
								result.resize(actual, 0);
							}
							// Устанавливаем буфер для получения результата
							zs.next_out = reinterpret_cast <Bytef *> (result.data() + offset);
							// Устанавливаем максимальный размер буфера
							zs.avail_out = static_cast <uInt> (std::min(actual - offset, static_cast <size_t> (std::numeric_limits <uInt>::max())));
							// Запоминаем размер свободного места в буфере
							const uInt availOut = zs.avail_out;
							// Выполняем расжатие
							rv = ::inflate(&zs, 0);
							// Увеличиваем количество извлечённых данных
							offset += (availOut - zs.avail_out);
							// Если мы завершили сбор данных
							if(rv == Z_STREAM_END)
								// Выходим из цикла
								break;
						/**
						 * Если данные ещё не извлечены
						 */
						} while(rv == Z_OK);
					}
					// Если извлечённые данные превышают допустимый предел
					if((limit > 0) && (offset > limit))
						// Помечаем декомпрессию как неудачную
						rv = Z_MEM_ERROR;
					// Если данные обработаны удачно
					if((rv == Z_OK) || (rv == Z_STREAM_END))
						// Добавляем оставшиеся данные в список
						result.erase(result.begin() + offset, result.end());
					// Выполняем очистку буфера данных
					else result.clear();
					// Завершаем расжатие
					::inflateEnd(&zs);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Шаблон функции работы с компрессором Deflate
 *
 * @tparam T сигнатура функции
 */
template <typename T>
/**
 * @brief Функция работы с компрессором Deflate
 *
 * @param buffer   буфер данных для компрессии
 * @param size     размер данных для компрессии
 * @param level    уровень компрессии
 * @param wbit     размер скользящего окна
 * @param takeOver флаг переиспользования контекста
 * @param stream   объект потока для работы компрессора
 * @param event    событие выполнения операции
 * @param result   строка куда следует положить результат
 * @param limit    максимальный размер извлекаемых данных (0 - без ограничения)
 */
static void deflate(const char * buffer, const size_t size, const uint32_t level, const int16_t wbit, const bool takeOver, z_stream & stream, const awh::hash_t::event_t event, T & result, const size_t limit = 0) noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку блока с результатом
			result.clear();
			// Результат проверки декомпрессии
			int32_t rv = Z_OK;
			// Создаем поток zip
			z_stream zs = {0};
			// Обнуляем структуру
			zs.zfree  = Z_NULL;
			zs.zalloc = Z_NULL;
			zs.opaque = Z_NULL;
			/**
			 * Буфер выходных данных не меньше размера чанка. При буфере в несколько байт
			 * маркер сброса Z_SYNC_FLUSH/Z_FULL_FLUSH (5 байт) заполнял его целиком,
			 * zlib после этого выпускал новый пустой блок при каждом вызове,
			 * и сжатие данных до 5 байт зацикливалось с бесконечным ростом памяти
			 */
			vector <Bytef> tmp(std::max(size, static_cast <size_t> (CHUNK_BUFFER_SIZE)), 0);
			/**
			 * Определяем событие выполнения операции
			 */
			switch(static_cast <uint8_t> (event)){
				// Если необходимо выполнить компрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::ENCODE): {
					/**
					 * Намеренно выдаём голый DEFLATE (RFC 1951), а не обёртку zlib (RFC 1950),
					 * хотя RFC 9110 для HTTP "deflate" предписывает zlib: декодер прежних версий AWH
					 * открывает поток с отрицательным окном и на заголовке zlib получит ошибку.
					 * Браузеры принимают оба вида, а наш декодер теперь опознаёт оба.
					 * Стратегия Z_DEFAULT_STRATEGY вместо Z_HUFFMAN_ONLY: только Хаффман без поиска
					 * совпадений давал тело в 2.5 раза больше, чем GZip, формат потока от этого не меняется
					 */
					// Если поток инициализировать не удалось, выходим
					if(takeOver || (::deflateInit2(&zs, level, Z_DEFLATED, -1 * wbit, DEFAULT_MEM_LEVEL, Z_DEFAULT_STRATEGY) == Z_OK)){
						// Если поток декомпрессора не создан ранее
						if(!takeOver){
							// Устанавливаем количество доступных данных
							zs.avail_in = static_cast <uint32_t> (size);
							// Устанавливаем буфер с данными для шифрования
							zs.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
						// Если нужно переиспользовать поток декомпрессора
						} else {
							// Устанавливаем количество доступных данных
							stream.avail_in = static_cast <uint32_t> (size);
							// Устанавливаем буфер с данными для шифрования
							stream.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
						}
						/**
						 * Выполняем компрессию всех данных
						 */
						do {
							// Если поток декомпрессора не создан ранее
							if(!takeOver){
								// Устанавливаем буфер для записи шифрованных данных
								zs.next_out = tmp.data();
								// Устанавливаем количество доступных данных для записи
								zs.avail_out = static_cast <uint32_t> (tmp.size());
								// Выполняем сжатие данных
								rv = ::deflate(&zs, Z_SYNC_FLUSH);
							// Если нужно переиспользовать поток декомпрессора
							} else {
								// Устанавливаем буфер для записи шифрованных данных
								stream.next_out = tmp.data();
								// Устанавливаем количество доступных данных для записи
								stream.avail_out = static_cast <uint32_t> (tmp.size());
								// Выполняем сжатие данных
								rv = ::deflate(&stream, Z_FULL_FLUSH);
							}
							// Если данные обработаны удачно
							if((rv == Z_OK) || (rv == Z_STREAM_END))
								// Добавляем оставшиеся данные в список
								result.insert(result.end(), tmp.begin(), tmp.begin() + (static_cast <uint32_t> (tmp.size()) - (!takeOver ? zs.avail_out : stream.avail_out)));
							// Если данные не могут быть обработанны, то выходим
							else break;
						/**
						 * Если ещё не все данные сжаты
						 */
						} while(rv != Z_STREAM_END);
						// Закрываем поток
						if(!takeOver)
							// Завершаем работу
							::deflateEnd(&zs);
					}
				} break;
				// Если необходимо выполнить декомпрессию данных
				case static_cast <uint8_t> (awh::hash_t::event_t::DECODE): {
					/**
					 * По RFC 9110 (8.4.1.2) кодирование "deflate" в HTTP это формат zlib (RFC 1950),
					 * но многие серверы (и прежние версии AWH) отдают голый DEFLATE (RFC 1951),
					 * поэтому при разовой декомпрессии принимаем оба вида: обёртку zlib опознаём
					 * по заголовку (CM = 8, CINFO <= 7, (CMF * 256 + FLG) кратно 31). Контекст
					 * WebSocket с переиспользованием (takeover) всегда голый (RFC 7692) и не проверяется.
					 * Голый поток с таким заголовком возможен лишь при хранимом блоке с ненулевыми
					 * битами выравнивания, для этого случая при ошибке разбора zlib повторяем разбор как голый
					 */
					bool zlib = (
						!takeOver && (size > 1) &&
						((static_cast <uint8_t> (buffer[0]) & 0x0F) == 0x08) &&
						((static_cast <uint8_t> (buffer[0]) >> 4) <= 0x07) &&
						((((static_cast <uint32_t> (static_cast <uint8_t> (buffer[0])) << 8) | static_cast <uint8_t> (buffer[1])) % 31) == 0)
					);
					// Флаг превышения допустимого предела извлекаемых данных
					bool overflow = false;
					/**
					 * Выполняем декомпрессию данных
					 */
					do {
						// Если поток инициализировать не удалось, выходим
						if(takeOver || (::inflateInit2(&zs, (zlib ? MAX_WBITS : -1 * wbit)) == Z_OK)){
							// Если поток декомпрессора не создан ранее
							if(!takeOver){
								// Устанавливаем количество доступных данных
								zs.avail_in = static_cast <uint32_t> (size);
								// Копируем входящий буфер для дешифровки
								zs.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
							// Если нужно переиспользовать поток декомпрессора
							} else {
								// Устанавливаем количество доступных данных
								stream.avail_in = static_cast <uint32_t> (size);
								// Копируем входящий буфер для дешифровки
								stream.next_in = const_cast <Bytef *> (reinterpret_cast <const Bytef *> (buffer));
							}
							/**
							 * Выполняем декомпрессию всех данных
							 */
							do {
								// Если поток декомпрессора не создан ранее
								if(!takeOver){
									// Устанавливаем буфер для записи дешифрованных данных
									zs.next_out = tmp.data();
									// Устанавливаем количество доступных данных для записи
									zs.avail_out = static_cast <uint32_t> (tmp.size());
									// Выполняем декомпрессию данных
									rv = ::inflate(&zs, Z_NO_FLUSH);
								// Если нужно переиспользовать поток декомпрессора
								} else {
									// Устанавливаем буфер для записи дешифрованных данных
									stream.next_out = tmp.data();
									// Устанавливаем количество доступных данных для записи
									stream.avail_out = static_cast <uint32_t> (tmp.size());
									// Выполняем декомпрессию данных
									rv = ::inflate(&stream, Z_SYNC_FLUSH);
								}
								// Если данные обработаны удачно
								if((rv == Z_OK) || (rv == Z_STREAM_END)){
									// Получаем размер извлечённых данных
									const size_t length = (tmp.size() - (!takeOver ? zs.avail_out : stream.avail_out));
									// Если извлечённые данные превышают допустимый предел
									if((limit > 0) && ((result.size() + length) > limit)){
										// Выполняем очистку результата
										result.clear();
										// Запоминаем превышение предела
										overflow = true;
										// Выходим из цикла
										break;
									}
									// Добавляем оставшиеся данные в список
									result.insert(result.end(), tmp.begin(), tmp.begin() + length);
								// Если данные не могут быть обработанны, то выходим
								} else break;
							/**
							 * Если ещё не все данные извлечены
							 */
							} while(rv != Z_STREAM_END);
							// Очищаем выделенную память для декомпрессора
							if(!takeOver)
								// Завершаем работу
								::inflateEnd(&zs);
						}
						// Если данные опознаны как zlib, но разобрать их не удалось
						if(zlib && !overflow && (rv != Z_OK) && (rv != Z_STREAM_END) && (rv != Z_BUF_ERROR)){
							// Снимаем флаг формата zlib
							zlib = false;
							// Сбрасываем результат проверки декомпрессии
							rv = Z_OK;
							// Выполняем очистку блока с результатом
							result.clear();
							// Заполняем поток нулями
							::memset(&zs, 0, sizeof(zs));
							// Обнуляем структуру
							zs.zfree  = Z_NULL;
							zs.zalloc = Z_NULL;
							zs.opaque = Z_NULL;
						// Выходим из цикла
						} else break;
					/**
					 * Повторяем разбор данных как голого DEFLATE
					 */
					} while(true);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку блока с результатом
			result.clear();
		}
	}
}
/**
 * @brief Метод инициализации AES шифрования
 *
 * @param cipher тип шифрования (AES128, AES192, AES256)
 * @return       результат инициализации
 */
bool awh::Hash::cipher(const cipher_t cipher) noexcept {
	// Определяем длину ключа шифрования
	size_t size = 0;
	/**
	 * Определяем длину шифрования
	 */
	switch(static_cast <uint16_t> (cipher)){
		// Устанавливаем шифрование в 128
		case static_cast <uint16_t> (cipher_t::AES128):
			// Устанавливаем длину ключа
			size = 16;
		break;
		// Устанавливаем шифрование в 192
		case static_cast <uint16_t> (cipher_t::AES192):
			// Устанавливаем длину ключа
			size = 24;
		break;
		// Устанавливаем шифрование в 256
		case static_cast <uint16_t> (cipher_t::AES256):
			// Устанавливаем длину ключа
			size = 32;
		break;
		// Если ничего не выбрано, сбрасываем
		default: return false;
	}
	/**
	 * @brief Выделяем память под ключ и вектор регистра
	 *
	 * Ключ и вектор выводятся одним отсчётом: PBKDF2 отдаёт поток байтов, который мы
	 * делим на ключ и на регистр обратной связи целиком, без усечения вектора до половины блока
	 *
	 */
	vector <uint8_t> secret(size + sizeof(this->_state.ivec), 0);
	// Выполняем выработку ключа и вектора из пароля и соли
	if(!nts::digest::pbkdf2(nts::digest::hash_t::SHA256,
		nts::view_t(reinterpret_cast <const uint8_t *> (this->_password.data()), this->_password.size()),
		nts::view_t(reinterpret_cast <const uint8_t *> (this->_salt.data()), this->_salt.size()),
		(this->_rounds > 0 ? static_cast <uint32_t> (this->_rounds) : 1), secret.data(), secret.size())){
		// Выводим сообщение об ошибке
		this->_log->print("%s", log_t::flag_t::CRITICAL, "Key for encryption/decryption could not be derived");
		// Выходим из функции
		return false;
	}
	// Устанавливаем длину ключа шифрования
	this->_state.size = size;
	// Копируем данные ключа
	::memcpy(this->_state.key, secret.data(), size);
	// Обнуляем номер
	this->_state.num = 0;
	// Копируем данные вектора регистра обратной связи
	::memcpy(this->_state.ivec, secret.data() + size, sizeof(this->_state.ivec));
	// Сообщаем что всё удачно
	return true;
}
/**
 * @brief Метод удаления хвостовых данных
 *
 * @param buffer буфер для удаления хвоста
 */
void awh::Hash::rmTail(vector <char> & buffer) const noexcept {
	/**
	 * @brief Если сообщение является финальным
	 *
	 * Хвост снимаем только с конца буфера: поиск первого вхождения 00 00 FF FF
	 * обрезал данные, если такая последовательность встречалась внутри сжатого потока
	 * (хранимый блок DEFLATE копирует несжимаемые данные как есть)
	 *
	 */
	if((buffer.size() > sizeof(this->_btype)) && equal(this->_btype, this->_btype + sizeof(this->_btype), buffer.end() - sizeof(this->_btype)))
		// Удаляем хвостовой список байт из буфера данных
		buffer.erase(buffer.end() - sizeof(this->_btype), buffer.end());
}
/**
 * @brief Метод добавления хвостовых данных
 *
 * @param buffer буфер для добавления хвоста
 */
void awh::Hash::setTail(vector <char> & buffer) const noexcept {
	// Добавляем хвостовой буфер в полезную нагрузку
	buffer.insert(buffer.end(), this->_btype, this->_btype + sizeof(this->_btype));
}
/**
 * @brief Шаблон метода хэширования текста
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод хэширования текста
 *
 * @param text текст для хэширования
 * @param type тип хэш-суммы
 * @return     результат хэширования
 */
auto awh::Hash::hashing(const string & text, const type_t type) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если текст передан
	if(!text.empty())
		// Выполняем хэширование
		this->hashing(text, type, result);
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода хэширования текста
 */
template string awh::Hash::hashing <string> (const string &, const type_t) const noexcept;
template vector <char> awh::Hash::hashing <vector <char>> (const string &, const type_t) const noexcept;
/**
 * @brief Метод хэширования текста
 *
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result строка куда следует положить результат
 */
void awh::Hash::hashing(const string & text, const type_t type, string & result) const noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		// Выполняем хэширование
		::hashing(text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Text hashing \"%s\" could not be performed", log_t::flag_t::WARNING, text.c_str());
	}
}
/**
 * @brief Метод хэширования текста
 *
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Hash::hashing(const string & text, const type_t type, vector <char> & result) const noexcept {
	// Если текст для хэширования передан
	if(!text.empty()){
		// Выполняем хэширование
		::hashing(text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Text hashing \"%s\" could not be performed", log_t::flag_t::WARNING, text.c_str());
	}
}
/**
 * @brief Шаблон метода хэширования текста с ключом
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод хэширования текста с ключом
 *
 * @param key  ключ для подписи
 * @param text текст для хэширования
 * @param type тип хэш-суммы
 * @return     результат хэширования
 */
auto awh::Hash::hmac(const string & key, const string & text, const type_t type) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если текст передан
	if(!text.empty())
		// Выполняем хэширование
		this->hmac(key, text, type, result);
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода хэширования текста с ключом
 */
template string awh::Hash::hmac(const string &, const string &, const type_t) const noexcept;
template vector <char> awh::Hash::hmac(const string &, const string &, const type_t) const noexcept;
/**
 * @brief Метод хэширования текста с ключом
 *
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result строка куда следует положить результат
 */
void awh::Hash::hmac(const string & key, const string & text, const type_t type, string & result) const noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		// Выполняем хэширование
		::hmac(key, text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Key \"%s\" and text \"%s\" hashing  could not be performed", log_t::flag_t::WARNING, key.c_str(), text.c_str());
	}
}
/**
 * @brief Метод хэширования текста с ключом
 *
 * @param key    ключ для подписи
 * @param text   текст для хэширования
 * @param type   тип хэш-суммы
 * @param result буфер куда следует положить результат
 */
void awh::Hash::hmac(const string & key, const string & text, const type_t type, vector <char> & result) const noexcept {
	// Если ключ и текст для хэширования переданы
	if(!key.empty() && !text.empty()){
		// Выполняем хэширование
		::hmac(key, text, type, result);
		// Если хэширование не вышло
		if(result.empty())
			// Выводим сообщение об ошибке
			this->_log->print("Key \"%s\" and text \"%s\" hashing  could not be performed", log_t::flag_t::WARNING, key.c_str(), text.c_str());
	}
}
/**
 * @brief Шаблон метода кодирования
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод кодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @return       результат кодирования
 */
auto awh::Hash::encode(const char * buffer, const size_t size, const cipher_t cipher) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем кодирование
		this->encode(buffer, size, cipher, result);
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода кодирования
 */
template string awh::Hash::encode(const char *, const size_t, const cipher_t) const noexcept;
template vector <char> awh::Hash::encode(const char *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Метод кодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result строка куда следует положить результат
 */
void awh::Hash::encode(const char * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем кодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				// Если кодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode data into AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * @brief Метод кодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Hash::encode(const char * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем кодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				// Если кодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode \"%s\" string data into BASE64 format", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::ENCODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to encode data into AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * @brief Шаблон метода декодирования
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод декодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @return       результат кодирования
 */
auto awh::Hash::decode(const char * buffer, const size_t size, const cipher_t cipher) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем декодирование
		this->decode(buffer, size, cipher, result);
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода декодирования
 */
template string awh::Hash::decode(const char *, const size_t, const cipher_t) const noexcept;
template vector <char> awh::Hash::decode(const char *, const size_t, const cipher_t) const noexcept;
/**
 * @brief Метод декодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result строка куда следует положить результат
 */
void awh::Hash::decode(const char * buffer, const size_t size, const cipher_t cipher, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем декодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				// Если декодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to decode data from AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * @brief Метод декодирования
 *
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @param cipher тип шифрования (BASE64, AES128, AES192, AES256)
 * @param result буфер куда следует положить результат
 */
void awh::Hash::decode(const char * buffer, const size_t size, const cipher_t cipher, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем тип шифрования
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если производится работы с BASE64
			case static_cast <uint8_t> (hash_t::cipher_t::BASE64): {
				// Выполняем декодирование строки BASE64
				::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				// Если декодирование не вышло
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Unable to extract data from BASE64 encoded \"%s\" hash", log_t::flag_t::WARNING, string(buffer, size).c_str());
			} break;
			// Если производится работы с AES128
			case static_cast <uint8_t> (hash_t::cipher_t::AES128):
			// Если производится работы с AES192
			case static_cast <uint8_t> (hash_t::cipher_t::AES192):
			// Если производится работы с AES256
			case static_cast <uint8_t> (hash_t::cipher_t::AES256): {
				// Если пароль установлен
				if(!this->_password.empty()){
					// Выполняем инициализацию AES
					if(const_cast <hash_t *> (this)->cipher(cipher))
						// Выполняем шифрование данных
						::hashing(buffer, size, cipher, event_t::DECODE, const_cast <state_t &> (this->_state), result);
				}
				// Если кодирование не вышло
				if(result.empty()){
					// Выводим сообщение об ошибке
					this->_log->print("Unable to decode data from AES", log_t::flag_t::WARNING);
					// Выводим тот же самый буфер как он был передан
					result.assign(buffer, buffer + size);
				}
			} break;
		}
	}
}
/**
 * @brief Шаблон метода компрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 */
auto awh::Hash::compress(const char * buffer, const size_t size, const method_t method) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем кодирование
		this->compress(buffer, size, method, result);
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода компрессии данных
 */
template string awh::Hash::compress(const char *, const size_t, const method_t) const noexcept;
template vector <char> awh::Hash::compress(const char *, const size_t, const method_t) const noexcept;
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @param result строка куда следует положить результат
 */
void awh::Hash::compress(const char * buffer, const size_t size, const method_t method, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод компрессии данных
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метод компрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем компрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем компрессию данных методом LZma
				::lzma(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем компрессию данных методом BZip2
				::bzip2(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				::brotli(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем компрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverCompress, this->_zdef, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * @brief Метод компрессии данных
 *
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @param result буфер куда следует положить результат
 */
void awh::Hash::compress(const char * buffer, const size_t size, const method_t method, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод компрессии данных
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метод компрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем компрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем компрессию данных методом LZma
				::lzma(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем компрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем компрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем компрессию данных методом BZip2
				::bzip2(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем компрессию данных методом Brotli
				::brotli(buffer, size, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем компрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverCompress, this->_zdef, event_t::ENCODE, result);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "compress failed");
			} break;
			// Если метод компрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * @brief Шаблон метода декомпрессии данных
 *
 * @tparam T тип возвращаемого результата
 */
template <typename T>
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 */
auto awh::Hash::decompress(const char * buffer, const size_t size, const method_t method) const noexcept -> T {
	// Результат работы функции
	T result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0))
		// Выполняем декомпрессию
		this->decompress(buffer, size, method, result);
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода декомпрессии данных
 */
template string awh::Hash::decompress(const char *, const size_t, const method_t) const noexcept;
template vector <char> awh::Hash::decompress(const char *, const size_t, const method_t) const noexcept;
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @param result строка куда следует положить результат
 */
void awh::Hash::decompress(const char * buffer, const size_t size, const method_t method, string & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод декомпрессии данных
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метод декомпрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем декомпрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем декомпрессию данных методом LZma
				::lzma(buffer, size, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем декомпрессию данных методом BZip2
				::bzip2(buffer, size, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				::brotli(buffer, size, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем декомпрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverDecompress, this->_zinf, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * @brief Метод декомпрессии данных
 *
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @param result буфер куда следует положить результат
 */
void awh::Hash::decompress(const char * buffer, const size_t size, const method_t method, vector <char> & result) const noexcept {
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем метод декомпрессии данных
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метод декомпрессии установлен Lz4
			case static_cast <uint8_t> (method_t::LZ4): {
				// Выполняем декомпрессию данных методом Lz4
				::lz4(buffer, size, this->_level[0], event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Lz4: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен LZma
			case static_cast <uint8_t> (method_t::LZMA): {
				// Выполняем декомпрессию данных методом LZma
				::lzma(buffer, size, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("LZma: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Zstandard
			case static_cast <uint8_t> (method_t::ZSTD): {
				// Выполняем декомпрессию данных методом Zstandard
				::zstd(buffer, size, this->_level[2], event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Zstandard: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен GZip
			case static_cast <uint8_t> (method_t::GZIP): {
				// Выполняем декомпрессию данных методом GZip
				::gzip(buffer, size, this->_level[1], this->_wbit, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("GZip: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен BZip2
			case static_cast <uint8_t> (method_t::BZIP2): {
				// Выполняем декомпрессию данных методом BZip2
				::bzip2(buffer, size, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("BZip2: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Brotli
			case static_cast <uint8_t> (method_t::BROTLI): {
				// Выполняем декомпрессию данных методом Brotli
				::brotli(buffer, size, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Brotli: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии установлен Deflate
			case static_cast <uint8_t> (method_t::DEFLATE): {
				// Выполняем декомпрессию данных методом Deflate
				::deflate(buffer, size, this->_level[1], this->_wbit, this->_takeOverDecompress, this->_zinf, event_t::DECODE, result, this->_limitDecompress);
				// Если результат не получен
				if(result.empty())
					// Выводим сообщение об ошибке
					this->_log->print("Deflate: %s", log_t::flag_t::WARNING, "decompress failed");
			} break;
			// Если метод декомпрессии не установлен
			case static_cast <uint8_t> (method_t::NONE):
				// Выводим переданный буфер данных
				result.assign(buffer, buffer + size);
			break;
		}
	}
}
/**
 * @brief Метод установки размера скользящего окна
 *
 * @param wbit размер скользящего окна
 */
void awh::Hash::wbit(const int16_t wbit) noexcept {
	// Устанавливаем размер скользящего окна
	this->_wbit = wbit;
	// Выполняем пересборку контекстов LZ77 для компрессии
	this->takeoverCompress(this->_takeOverCompress);
	// Выполняем пересборку контекстов LZ77 для декомпрессии
	this->takeoverDecompress(this->_takeOverDecompress);
}
/**
 * @brief Метод установки количества раундов шифрования
 *
 * @param round количество раундов шифрования
 */
void awh::Hash::round(const int32_t round) noexcept {
	// Устанавливаем количество раундов шифрования
	this->_rounds = round;
}
/**
 * @brief Метод установки уровня компрессии
 *
 * @param level уровень компрессии
 */
void awh::Hash::level(const level_t level) noexcept {
	/**
	 * Определяем переданный уровень компрессии
	 */
	switch(static_cast <uint8_t> (level)){
		// Выполняем установку максимального уровня компрессии
		case static_cast <uint8_t> (level_t::BEST): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 0;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 100;
		} break;
		// Выполняем установку уровень компрессии на максимальную производительность
		case static_cast <uint8_t> (level_t::SPEED): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 3;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_BEST_SPEED;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = ZSTD_CLEVEL_DEFAULT;
		} break;
		// Выполняем установку нормального уровня компрессии
		case static_cast <uint8_t> (level_t::NORMAL): {
			// Выполняем установку уровня максимальной компрессии Lz4
			this->_level[0] = 1;
			// Выполняем установку уровня компрессии GZip
			this->_level[1] = Z_DEFAULT_COMPRESSION;
			// Выполняем установку уровня максимальной компрессии Zstandard
			this->_level[2] = 22;
		} break;
	}
}
/**
 * @brief Метод установки соли шифрования
 *
 * @param salt соль для шифрования
 */
void awh::Hash::salt(const string & salt) noexcept {
	// Если соль передана
	this->_salt = salt;
}
/**
 * @brief Метод установки пароля шифрования
 *
 * @param password пароль шифрования
 */
void awh::Hash::password(const string & password) noexcept {
	// Если пароль передан
	this->_password = password;
}
/**
 * @brief Метод установки флага переиспользования контекста компрессии
 *
 * @param flag флаг переиспользования контекста компрессии
 */
void awh::Hash::takeoverCompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->_takeOverCompress && !flag)
		// Очищаем выделенную память для компрессора
		::deflateEnd(&this->_zdef);
	// Если флаг установлен
	if(!this->_takeOverCompress && flag){
		// Заполняем его нулями потока для компрессора
		::memset(&this->_zdef, 0, sizeof(this->_zdef));
		// Обнуляем структуру потока для компрессора
		this->_zdef.zalloc = Z_NULL;
		this->_zdef.zfree  = Z_NULL;
		this->_zdef.opaque = Z_NULL;
		/**
		 * Стратегия Z_DEFAULT_STRATEGY вместо Z_HUFFMAN_ONLY: поиск совпадений резко уменьшает
		 * сжатые данные, поток остаётся голым DEFLATE (RFC 7692) и читается любым участником
		 */
		// Если поток инициализировать не удалось, выходим
		if(::deflateInit2(&this->_zdef, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -1 * this->_wbit, DEFAULT_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK){
			// Выводим сообщение об ошибке
			this->_log->print("Deflate stream is not create", log_t::flag_t::CRITICAL);
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Выходим из приложения
				::raise(SIGINT);
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
	}
	// Устанавливаем переданный флаг
	this->_takeOverCompress = flag;
}
/**
 * @brief Метод установки флага переиспользования контекста декомпрессии
 *
 * @param flag флаг переиспользования контекста декомпрессии
 */
void awh::Hash::takeoverDecompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->_takeOverDecompress && !flag)
		// Очищаем выделенную память для декомпрессора
		::inflateEnd(&this->_zinf);
	// Если флаг установлен
	if(!this->_takeOverDecompress && flag){
		// Заполняем его нулями потока для декомпрессора
		::memset(&this->_zinf, 0, sizeof(this->_zinf));
		// Обнуляем структуру потока для декомпрессора
		this->_zinf.avail_in = 0;
		this->_zinf.zalloc   = Z_NULL;
		this->_zinf.zfree    = Z_NULL;
		this->_zinf.opaque   = Z_NULL;
		this->_zinf.next_in  = Z_NULL;
		// Если поток инициализировать не удалось, выходим
		if(::inflateInit2(&this->_zinf, -1 * this->_wbit) != Z_OK){
			// Выводим сообщение об ошибке
			this->_log->print("Inflate stream is not create", log_t::flag_t::CRITICAL);
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Выходим из приложения
				::raise(SIGINT);
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
	}
	// Устанавливаем переданный флаг
	this->_takeOverDecompress = flag;
}
/**
 * @brief Метод установки максимального размера извлекаемых данных при декомпрессии
 *
 * @param size максимальный размер извлекаемых данных в байтах
 */
void awh::Hash::limitDecompress(const size_t size) noexcept {
	// Устанавливаем максимальный размер извлекаемых данных
	this->_limitDecompress = size;
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 */
awh::Hash::Hash(const log_t * log) noexcept :
 _wbit(MAX_WBITS), _rounds(5),
 _level{1, Z_DEFAULT_COMPRESSION, ZSTD_CLEVEL_DEFAULT},
 _salt{""}, _password{""}, _takeOverCompress(false), _takeOverDecompress(false),
 _limitDecompress(AWH_MAX_BODY_SIZE), _btype{0x00, 0x00, 0xFF, 0xFF}, _zinf({0}), _zdef({0}), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Hash::~Hash() noexcept {
	// Очищаем выделенную память для компрессора
	if(this->_takeOverCompress)
		// Завершаем работу
		::deflateEnd(&this->_zdef);
	// Очищаем выделенную память для декомпрессора
	if(this->_takeOverDecompress)
		// Завершаем работу
		::inflateEnd(&this->_zinf);
}
