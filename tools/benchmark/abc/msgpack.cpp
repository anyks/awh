/**
 * @file msgpack.cpp
 * @date 2026-08-19
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
 * @brief Эталонный стенд сравнения бинарного контейнера ABC — сборка и разбор
 *        записи средствами реализации msgpack-c
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <msgpack.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Функция укладки строки
 *
 * @param packer сборка записи
 * @param value  укладываемая строка
 *
 */
static void put(msgpack_packer * packer, const std::string & value) noexcept {
	// Выполняем укладку длины строки
	msgpack_pack_str(packer, value.size());
	// Выполняем укладку октетов строки
	msgpack_pack_str_body(packer, value.data(), value.size());
}
/**
 * @brief Функция укладки строки
 *
 * @param packer сборка записи
 * @param value  укладываемая строка
 *
 */
static void put(msgpack_packer * packer, const char * value) noexcept {
	// Выполняем получение длины укладываемой строки
	const size_t length = ::strlen(value);
	// Выполняем укладку длины строки
	msgpack_pack_str(packer, length);
	// Выполняем укладку октетов строки
	msgpack_pack_str_body(packer, value, length);
}
/**
 * @brief Функция сборки ветви образца с глубокой вложенностью
 *
 * @param packer сборка записи
 * @param depth  оставшаяся глубина вложенности
 *
 */
static void branch(msgpack_packer * packer, const uint32_t depth) noexcept {
	/**
	 * Если глубина вложенности исчерпана
	 */
	if(depth == 0){
		// Выполняем укладку отображения листа ветви
		msgpack_pack_map(packer, 1);
		// Выполняем укладку имени поля листа ветви
		put(packer, "value");
		// Выполняем укладку значения листа ветви
		msgpack_pack_uint64(packer, 1);
		// Выходим из работы
		return;
	}
	// Выполняем получение номера яруса вложенности
	const uint32_t level = (rival::NESTED_DEPTH - depth);
	// Выполняем сборку имени поля яруса вложенности
	const std::string key = ("k" + std::to_string(level));
	// Выполняем укладку отображения яруса вложенности
	msgpack_pack_map(packer, 1);
	// Выполняем укладку имени поля яруса вложенности
	put(packer, key);
	// Выполняем укладку массива яруса вложенности
	msgpack_pack_array(packer, 2);
	// Выполняем укладку вложенной ветви
	branch(packer, depth - 1);
	// Выполняем укладку отображения соседа вложенной ветви
	msgpack_pack_map(packer, 1);
	// Выполняем укладку имени поля соседа вложенной ветви
	put(packer, "n");
	// Выполняем укладку значения соседа вложенной ветви
	msgpack_pack_uint64(packer, level);
}
/**
 * @brief Функция сборки записи из образца содержимого
 *
 * @param scene  разновидность сценария стенда
 * @param record собираемая запись
 * @return       признак успешности сборки
 *
 */
static bool writing(const rival::scene_t scene, std::string & record) noexcept {
	// Буфер собираемой записи
	msgpack_sbuffer buffer;
	// Выполняем заведение буфера собираемой записи
	msgpack_sbuffer_init(&buffer);
	// Сборка записи
	msgpack_packer packer;
	// Выполняем заведение сборки записи
	msgpack_packer_init(&packer, &buffer, msgpack_sbuffer_write);
	/**
	 * Определяем разновидность сценария стенда
	 */
	switch(static_cast <uint8_t> (scene)){
		/**
		 * Если собирается перечень однородных записей
		 */
		case static_cast <uint8_t> (rival::scene_t::OBJECTS): {
			// Выполняем получение образца обиходного вида
			const auto & objects = rival::objects();
			// Выполняем получение имён полей образца
			const char * const * fields = rival::fields();
			// Выполняем укладку массива записей
			msgpack_pack_array(&packer, objects.size());
			/**
			 * Выполняем укладку всех записей образца
			 */
			for(size_t i = 0; i < objects.size(); i++){
				// Выполняем получение очередной записи образца
				const rival::object_t & object = objects.at(i);
				// Выполняем укладку отображения записи
				msgpack_pack_map(&packer, 6);
				// Выполняем укладку признака деятельности записи
				put(&packer, fields[0]);
				// Выполняем укладку значения признака деятельности
				(object.active ? msgpack_pack_true(&packer) : msgpack_pack_false(&packer));
				// Выполняем укладку величины записи
				put(&packer, fields[1]);
				// Выполняем укладку значения величины
				msgpack_pack_double(&packer, object.amount);
				// Выполняем укладку города записи
				put(&packer, fields[2]);
				// Выполняем укладку значения города
				put(&packer, object.city);
				// Выполняем укладку опознавателя записи
				put(&packer, fields[3]);
				// Выполняем укладку значения опознавателя
				msgpack_pack_uint64(&packer, object.id);
				// Выполняем укладку названия записи
				put(&packer, fields[4]);
				// Выполняем укладку значения названия
				put(&packer, object.name);
				// Выполняем укладку заметки записи
				put(&packer, fields[5]);
				// Выполняем укладку пустого значения заметки
				msgpack_pack_nil(&packer);
			}
		} break;
		/**
		 * Если собирается перечень чисел
		 */
		case static_cast <uint8_t> (rival::scene_t::NUMBERS): {
			// Выполняем получение образца с преобладанием чисел
			const auto & numbers = rival::numbers();
			// Выполняем укладку массива чисел
			msgpack_pack_array(&packer, numbers.size());
			/**
			 * Выполняем укладку всех чисел образца
			 */
			for(size_t i = 0; i < numbers.size(); i++){
				// Выполняем получение очередного числа образца
				const rival::number_t & number = numbers.at(i);
				/**
				 * Определяем разновидность числа образца
				 */
				switch(static_cast <uint8_t> (number.kind)){
					// Если числом является целое со знаком
					case static_cast <uint8_t> (rival::numeric_t::INTEGER):
						msgpack_pack_int64(&packer, number.integer);
					break;
					// Если числом является дробное
					case static_cast <uint8_t> (rival::numeric_t::REAL):
						msgpack_pack_double(&packer, number.real);
					break;
					// Если числом является целое без знака
					default: msgpack_pack_uint64(&packer, number.natural);
				}
			}
		} break;
		/**
		 * Если собирается перечень строк
		 */
		case static_cast <uint8_t> (rival::scene_t::STRINGS): {
			// Выполняем получение образца с преобладанием строк
			const auto & strings = rival::strings();
			// Выполняем укладку массива строк
			msgpack_pack_array(&packer, strings.size());
			/**
			 * Выполняем укладку всех строк образца
			 */
			for(size_t i = 0; i < strings.size(); i++)
				// Выполняем укладку очередной строки образца
				put(&packer, strings.at(i));
		} break;
		/**
		 * Если собирается перечень двоичных значений
		 */
		case static_cast <uint8_t> (rival::scene_t::BLOBS): {
			// Выполняем получение образца с преобладанием двоичных значений
			const auto & blobs = rival::blobs();
			// Выполняем укладку массива двоичных значений
			msgpack_pack_array(&packer, blobs.size());
			/**
			 * Выполняем укладку всех двоичных значений образца
			 */
			for(size_t i = 0; i < blobs.size(); i++){
				// Выполняем укладку длины очередного двоичного значения
				msgpack_pack_bin(&packer, blobs.at(i).size());
				// Выполняем укладку октетов очередного двоичного значения
				msgpack_pack_bin_body(&packer, blobs.at(i).data(), blobs.at(i).size());
			}
		} break;
		/**
		 * Если собирается образец с глубокой вложенностью
		 */
		case static_cast <uint8_t> (rival::scene_t::NESTED): {
			// Выполняем укладку массива ветвей
			msgpack_pack_array(&packer, rival::NESTED_COUNT);
			/**
			 * Выполняем укладку всех ветвей образца
			 */
			for(size_t i = 0; i < rival::NESTED_COUNT; i++)
				// Выполняем укладку очередной ветви образца
				branch(&packer, rival::NESTED_DEPTH);
		} break;
		/**
		 * Если собирается малая запись образца
		 */
		default: {
			// Выполняем укладку отображения малой записи
			msgpack_pack_map(&packer, 5);
			// Выполняем укладку признака деятельности записи
			put(&packer, "active");
			// Выполняем укладку значения признака деятельности
			msgpack_pack_true(&packer);
			// Выполняем укладку величины записи
			put(&packer, "amount");
			// Выполняем укладку значения величины
			msgpack_pack_double(&packer, 42.5);
			// Выполняем укладку опознавателя записи
			put(&packer, "id");
			// Выполняем укладку значения опознавателя
			msgpack_pack_uint64(&packer, 17);
			// Выполняем укладку названия записи
			put(&packer, "name");
			// Выполняем укладку значения названия
			put(&packer, "Товар");
			// Выполняем укладку меток записи
			put(&packer, "tags");
			// Выполняем укладку массива меток записи
			msgpack_pack_array(&packer, 2);
			// Выполняем укладку первой метки записи
			put(&packer, "один");
			// Выполняем укладку второй метки записи
			put(&packer, "два");
		}
	}
	// Выполняем выдачу собранной записи
	record.assign(buffer.data, buffer.size);
	// Выполняем освобождение буфера собранной записи
	msgpack_sbuffer_destroy(&buffer);
	// Выводим признак успешной сборки
	return !record.empty();
}
/**
 * @brief Функция обхода разобранного значения
 *
 * @param object обходимое значение записи
 *
 */
static void walk(const msgpack_object & object) noexcept {
	/**
	 * Определяем вид обходимого значения записи
	 */
	switch(static_cast <uint32_t> (object.type)){
		// Если значение является пустым
		case MSGPACK_OBJECT_NIL:
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		// Если значение является логическим
		case MSGPACK_OBJECT_BOOLEAN:
			// Выполняем учёт прочитанного логического значения
			rival::consume(object.via.boolean);
		break;
		// Если значение является целым без знака
		case MSGPACK_OBJECT_POSITIVE_INTEGER:
			// Выполняем учёт прочитанного числа
			rival::consume(static_cast <double> (object.via.u64));
		break;
		// Если значение является целым со знаком
		case MSGPACK_OBJECT_NEGATIVE_INTEGER:
			// Выполняем учёт прочитанного числа
			rival::consume(static_cast <double> (object.via.i64));
		break;
		// Если значение является дробным
		case MSGPACK_OBJECT_FLOAT32:
		case MSGPACK_OBJECT_FLOAT64:
			// Выполняем учёт прочитанного числа
			rival::consume(object.via.f64);
		break;
		// Если значение является строкой
		case MSGPACK_OBJECT_STR:
			// Выполняем учёт прочитанного содержимого значения
			rival::consume(object.via.str.ptr, object.via.str.size);
		break;
		// Если значение является двоичными данными
		case MSGPACK_OBJECT_BIN:
			// Выполняем учёт прочитанного содержимого значения
			rival::consume(object.via.bin.ptr, object.via.bin.size);
		break;
		/**
		 * Если значение является массивом
		 */
		case MSGPACK_OBJECT_ARRAY: {
			/**
			 * Выполняем обход всех значений массива
			 */
			for(uint32_t i = 0; i < object.via.array.size; i++)
				// Выполняем обход очередного значения массива
				walk(object.via.array.ptr[i]);
		} break;
		/**
		 * Если значение является отображением
		 */
		case MSGPACK_OBJECT_MAP: {
			/**
			 * Выполняем обход всех пар отображения
			 */
			for(uint32_t i = 0; i < object.via.map.size; i++){
				// Выполняем обход имени поля отображения
				walk(object.via.map.ptr[i].key);
				// Выполняем обход значения поля отображения
				walk(object.via.map.ptr[i].val);
			}
		} break;
	}
}
/**
 * @brief Функция разбора записи вместе с полным обходом
 *
 * @param scene  разновидность сценария стенда
 * @param record разбираемая запись
 * @return       признак успешности разбора
 *
 */
static bool reading(const rival::scene_t scene, const std::string & record) noexcept {
	// Разновидность сценария стенда работе разбора безразлична
	(void) scene;
	// Разобранное значение записи
	msgpack_unpacked unpacked;
	// Выполняем заведение разобранного значения записи
	msgpack_unpacked_init(&unpacked);
	// Смещение разбора записи
	size_t offset = 0;
	// Выполняем разбор записи
	const msgpack_unpack_return result = msgpack_unpack_next(&unpacked, record.data(), record.size(), &offset);
	/**
	 * Если разобрать запись не удалось
	 */
	if(result != MSGPACK_UNPACK_SUCCESS){
		// Выполняем освобождение разобранного значения записи
		msgpack_unpacked_destroy(&unpacked);
		// Выводим признак неудачного разбора
		return false;
	}
	// Выполняем обход разобранного значения записи
	walk(unpacked.data);
	// Выполняем освобождение разобранного значения записи
	msgpack_unpacked_destroy(&unpacked);
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Функция запуска стенда
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Работы стенда, сличаемые с прочими стендами
	const rival::stand_t stand{writing, reading};
	// Выполняем прогон всех сценариев стенда
	return rival::drive(stand, argc, argv);
}
