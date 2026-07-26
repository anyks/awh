/**
 * @file: nghttp2-hpack.cpp
 * @date: 2026-07-27
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
 * Пробник: дифференциальная сверка HPACK-кодека с эталонной реализацией nghttp2.
 * Наш кодер проверяется распаковщиком nghttp2 и наоборот, на одной и той же
 * последовательности блоков - то есть с общим состоянием динамических таблиц
 */

#include <string>
#include <vector>
#include <random>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <nghttp2/nghttp2.h>

#include <proto/http/parser/http2/http.hpp>

using namespace awh;
using namespace awh::http;

/**
 * @brief Структура сверяемого заголовка
 *
 */
typedef struct Field {
	// Название заголовка
	std::string name;
	// Значение заголовка
	std::string value;
} field_t;

// Количество обнаруженных расхождений
static size_t failures = 0;

/**
 * @brief Функция печати расхождения
 *
 * @param stage описание проверки
 * @param index номер блока заголовков
 * @param text  текст расхождения
 */
static void mismatch(const char * stage, const size_t index, const std::string & text) noexcept {
	// Наращиваем счётчик расхождений
	failures++;
	// Печатаем расхождение
	std::cout << "РАСХОЖДЕНИЕ [" << stage << "] блок " << index << ": " << text << std::endl;
}

/**
 * @brief Функция формирования псевдослучайного набора заголовков
 *
 * @param rng   генератор псевдослучайных чисел
 * @param index номер блока заголовков
 * @return      сформированный набор заголовков
 */
static std::vector <field_t> generate(std::mt19937 & rng, const size_t index) noexcept {
	// Результат работы функции - набор заголовков
	std::vector <field_t> result;
	// Дописываем псевдо-заголовки запроса (значения из статической таблицы и вне её)
	result.push_back({":method", ((rng() % 2) == 0 ? "GET" : "PROPFIND")});
	result.push_back({":scheme", ((rng() % 2) == 0 ? "https" : "ftp")});
	result.push_back({":path", ((rng() % 3) == 0 ? "/" : ("/resource/" + std::to_string(rng() % 1000)))});
	result.push_back({":authority", "example.com"});
	// Таблица названий заголовков: часть есть в статической таблице, часть нет
	static const char * names[] = {
		"accept", "accept-encoding", "user-agent", "cookie", "authorization",
		"x-custom-header", "x-trace-id", "if-none-match", "referer", "x-b3-traceid"
	};
	// Количество дописываемых заголовков
	const size_t count = (rng() % 8);
	/**
	 * Выполняем формирование всех заголовков набора
	 */
	for(size_t i = 0; i < count; i++){
		// Выбираем название заголовка
		const std::string name = names[rng() % 10];
		// Формируем значение заголовка
		std::string value;
		/**
		 * Выбираем вид значения заголовка
		 */
		switch(rng() % 5){
			// Короткое повторяющееся значение (попадёт в динамическую таблицу)
			case 0: value = "gzip, deflate"; break;
			// Пустое значение
			case 1: value = ""; break;
			// Длинное значение
			case 2: value = std::string(1 + (rng() % 600), static_cast <char> ('a' + (rng() % 26))); break;
			// Значение со всеми печатными символами
			case 3: {
				/**
				 * Выполняем формирование значения из печатных символов
				 */
				for(uint16_t letter = 0x20; letter < 0x7F; letter++)
					// Дописываем очередной символ значения
					value.push_back(static_cast <char> (letter));
			} break;
			// Уникальное значение блока
			default: value = ("value-" + std::to_string(index) + "-" + std::to_string(i));
		}
		// Дописываем заголовок в набор
		result.push_back({name, value});
	}
	// Выводим сформированный набор заголовков
	return result;
}

/**
 * @brief Функция распаковки блока распаковщиком nghttp2
 *
 * @param inflater объект распаковщика nghttp2
 * @param block    распаковываемый блок заголовков
 * @param output   распакованный набор заголовков
 * @return         результат распаковки
 */
static bool inflate(nghttp2_hd_inflater * inflater, const std::string & block, std::vector <field_t> & output) noexcept {
	// Указатель на данные блока
	const uint8_t * data = reinterpret_cast <const uint8_t *> (block.data());
	// Оставшийся размер блока
	size_t size = block.size();
	/**
	 * Выполняем распаковку всего блока заголовков
	 */
	for(;;){
		// Распакованный заголовок
		nghttp2_nv nv;
		// Флаги распаковки
		int flags = 0;
		// Выполняем распаковку очередного заголовка
		const ssize_t bytes = ::nghttp2_hd_inflate_hd2(inflater, &nv, &flags, data, size, 1);
		// Если распаковка завершилась ошибкой
		if(bytes < 0)
			// Распаковка не удалась
			return false;
		// Сдвигаем указатель на распакованные байты
		data += bytes;
		// Уменьшаем оставшийся размер блока
		size -= static_cast <size_t> (bytes);
		// Если распакован очередной заголовок
		if(flags & NGHTTP2_HD_INFLATE_EMIT)
			// Дописываем распакованный заголовок
			output.push_back({
				std::string(reinterpret_cast <const char *> (nv.name), nv.namelen),
				std::string(reinterpret_cast <const char *> (nv.value), nv.valuelen)
			});
		// Если блок распакован полностью
		if(flags & NGHTTP2_HD_INFLATE_FINAL){
			// Завершаем распаковку блока
			::nghttp2_hd_inflate_end_headers(inflater);
			// Прекращаем распаковку
			break;
		}
		// Если данные блока закончились
		if((size == 0) && ((flags & NGHTTP2_HD_INFLATE_EMIT) == 0))
			// Прекращаем распаковку
			break;
	}
	// Распаковка выполнена
	return true;
}

int main(){
	// Объект фреймворка
	fmk_t fmk;
	// Объект логов
	log_t log(&fmk);
	// Отключаем вывод логов
	log.level(log_t::level_t::NONE);
	// Инициализируем генератор псевдослучайных чисел фиксированным зерном
	std::mt19937 rng(20260727);
	// Количество проверяемых блоков заголовков
	const size_t total = 4000;
	/**
	 * Первая проверка: наш кодер против распаковщика nghttp2
	 */
	{
		// Создаём объект кодера заголовков
		h2::hpack::encoder_t encoder;
		// Объект распаковщика nghttp2
		nghttp2_hd_inflater * inflater = nullptr;
		// Создаём объект распаковщика nghttp2
		if(::nghttp2_hd_inflate_new(&inflater) != 0){
			// Печатаем ошибку создания распаковщика
			std::cout << "не удалось создать распаковщик nghttp2" << std::endl;
			// Выводим результат
			return 1;
		}
		/**
		 * Выполняем перебор всех проверяемых блоков заголовков
		 */
		for(size_t i = 0; i < total; i++){
			// Формируем набор заголовков блока
			const std::vector <field_t> fields = ::generate(rng, i);
			// Список кодируемых заголовков
			std::vector <h2::hpack::field_t> encoded;
			// Резервируем память под кодируемые заголовки
			encoded.reserve(fields.size());
			/**
			 * Выполняем перебор всех заголовков набора
			 */
			for(const field_t & field : fields)
				// Дописываем кодируемый заголовок
				encoded.emplace_back(field.name, field.value);
			// Буфер закодированного блока заголовков
			std::string block;
			// Кодируем блок заголовков
			encoder.encode(encoded, block, ((rng() & 1) != 0));
			// Распакованный набор заголовков
			std::vector <field_t> decoded;
			// Если распаковка блока не удалась
			if(!::inflate(inflater, block, decoded)){
				// Фиксируем расхождение
				::mismatch("наш кодер -> nghttp2", i, "nghttp2 не смог распаковать блок");
				// Переходим к следующему блоку
				continue;
			}
			// Если количество заголовков не совпало
			if(decoded.size() != fields.size()){
				// Фиксируем расхождение
				::mismatch("наш кодер -> nghttp2", i, ("заголовков " + std::to_string(decoded.size()) + " вместо " + std::to_string(fields.size())));
				// Переходим к следующему блоку
				continue;
			}
			/**
			 * Выполняем сверку всех распакованных заголовков
			 */
			for(size_t j = 0; j < fields.size(); j++){
				// Если название либо значение заголовка не совпало
				if((decoded[j].name != fields[j].name) || (decoded[j].value != fields[j].value)){
					// Фиксируем расхождение
					::mismatch("наш кодер -> nghttp2", i, ("заголовок " + std::to_string(j) + ": [" + decoded[j].name + ": " + decoded[j].value + "] вместо [" + fields[j].name + ": " + fields[j].value + "]"));
					// Прекращаем сверку блока
					break;
				}
			}
		}
		// Удаляем объект распаковщика nghttp2
		::nghttp2_hd_inflate_del(inflater);
	}
	// Переинициализируем генератор псевдослучайных чисел
	rng.seed(20260727);
	/**
	 * Вторая проверка: упаковщик nghttp2 против нашего декодера
	 */
	{
		// Создаём объект декодера заголовков
		h2::hpack::decoder_t decoder;
		// Объект упаковщика nghttp2
		nghttp2_hd_deflater * deflater = nullptr;
		// Создаём объект упаковщика nghttp2
		if(::nghttp2_hd_deflate_new(&deflater, 4096) != 0){
			// Печатаем ошибку создания упаковщика
			std::cout << "не удалось создать упаковщик nghttp2" << std::endl;
			// Выводим результат
			return 1;
		}
		/**
		 * Выполняем перебор всех проверяемых блоков заголовков
		 */
		for(size_t i = 0; i < total; i++){
			// Формируем набор заголовков блока
			const std::vector <field_t> fields = ::generate(rng, i);
			// Список заголовков в представлении nghttp2
			std::vector <nghttp2_nv> nva;
			// Резервируем память под заголовки
			nva.reserve(fields.size());
			/**
			 * Выполняем перебор всех заголовков набора
			 */
			for(const field_t & field : fields){
				// Формируем заголовок в представлении nghttp2
				nghttp2_nv nv;
				// Устанавливаем название заголовка
				nv.name = reinterpret_cast <uint8_t *> (const_cast <char *> (field.name.data()));
				// Устанавливаем длину названия заголовка
				nv.namelen = field.name.size();
				// Устанавливаем значение заголовка
				nv.value = reinterpret_cast <uint8_t *> (const_cast <char *> (field.value.data()));
				// Устанавливаем длину значения заголовка
				nv.valuelen = field.value.size();
				// Флаги заголовка не используются
				nv.flags = NGHTTP2_NV_FLAG_NONE;
				// Дописываем заголовок
				nva.push_back(nv);
			}
			// Вычисляем верхнюю границу размера блока
			const size_t bound = ::nghttp2_hd_deflate_bound(deflater, nva.data(), nva.size());
			// Буфер закодированного блока заголовков
			std::string block(bound, '\0');
			// Выполняем упаковку блока заголовков
			const ssize_t length = ::nghttp2_hd_deflate_hd2(deflater, reinterpret_cast <uint8_t *> (&block[0]), bound, nva.data(), nva.size());
			// Если упаковка завершилась ошибкой
			if(length < 0){
				// Фиксируем расхождение
				::mismatch("nghttp2 -> наш декодер", i, "nghttp2 не смог упаковать блок");
				// Переходим к следующему блоку
				continue;
			}
			// Обрезаем буфер до фактического размера блока
			block.resize(static_cast <size_t> (length));
			// Список декодированных заголовков
			std::vector <h2::hpack::field_view_t> decoded;
			// Код ошибки декодирования
			h2::error_t error = h2::error_t::NO_ERROR;
			// Если декодирование блока не удалось
			if(decoder.decode(block, decoded, 0, error) != h2::status_t::OK){
				// Фиксируем расхождение
				::mismatch("nghttp2 -> наш декодер", i, "наш декодер не смог разобрать блок");
				// Переходим к следующему блоку
				continue;
			}
			// Если количество заголовков не совпало
			if(decoded.size() != fields.size()){
				// Фиксируем расхождение
				::mismatch("nghttp2 -> наш декодер", i, ("заголовков " + std::to_string(decoded.size()) + " вместо " + std::to_string(fields.size())));
				// Переходим к следующему блоку
				continue;
			}
			/**
			 * Выполняем сверку всех декодированных заголовков
			 */
			for(size_t j = 0; j < fields.size(); j++){
				// Если название либо значение заголовка не совпало
				if((decoded[j].name != fields[j].name) || (decoded[j].value != fields[j].value)){
					// Фиксируем расхождение
					::mismatch("nghttp2 -> наш декодер", i, ("заголовок " + std::to_string(j) + ": [" + std::string(decoded[j].name) + ": " + std::string(decoded[j].value) + "] вместо [" + fields[j].name + ": " + fields[j].value + "]"));
					// Прекращаем сверку блока
					break;
				}
			}
		}
		// Удаляем объект упаковщика nghttp2
		::nghttp2_hd_deflate_del(deflater);
	}
	// Печатаем итог сверки
	std::cout << "сверено блоков: " << (total * 2) << ", расхождений: " << failures << std::endl;
	// Выводим результат
	return (failures == 0 ? 0 : 1);
}
