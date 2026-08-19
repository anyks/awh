/**
 * @file cbor.cpp
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
 *        записи средствами реализации libcbor
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
#include <cbor.h>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Собираемая запись стенда
 *
 * @details Низкоуровневая укладка libcbor пишет в поданный буфер и выводит число
 *          уложенных октетов. Буфер растёт дописыванием: сборка дерева значений
 *          мерила бы выделение памяти под всякий узел, а не саму укладку
 *
 */
static std::string produced;

/**
 * @brief Признак отказа сборки записи
 *
 */
static bool failed = false;

/**
 * @brief Метод укладки собранных октетов в собираемую запись
 *
 * @param size число уложенных октетов
 *
 */
static inline void commit(const size_t size) noexcept {
	/**
	 * Если уложить единицу не удалось, заведённое место обращается вспять целиком:
	 * молчаливое усечение обратило бы отказ укладки в порченую запись
	 */
	if(size == 0){
		// Выполняем установку признака отказа сборки
		failed = true;
		// Выполняем возврат заведённого места
		produced.resize(produced.size() - 16);
		// Выходим из работы
		return;
	}
	// Выполняем усечение собираемой записи по числу уложенных октетов
	produced.resize(produced.size() - (16 - size));
}
/**
 * @brief Метод заведения места под очередную единицу записи
 *
 * @return указатель на заведённое место
 *
 */
static inline unsigned char * place() noexcept {
	// Выполняем заведение места под ведущую запись единицы
	produced.resize(produced.size() + 16);
	// Выводим указатель на заведённое место
	return (reinterpret_cast <unsigned char *> (&produced[0]) + (produced.size() - 16));
}
/**
 * @brief Функция укладки строки
 *
 * @param value  укладываемая строка
 * @param length длина укладываемой строки
 *
 */
static void put(const char * value, const size_t length) noexcept {
	// Выполняем укладку длины строки
	commit(cbor_encode_string_start(length, place(), 16));
	// Выполняем укладку октетов строки
	produced.append(value, length);
}
/**
 * @brief Функция укладки строки
 *
 * @param value укладываемая строка
 *
 */
static void put(const char * value) noexcept {
	// Выполняем укладку строки
	put(value, ::strlen(value));
}
/**
 * @brief Функция сборки ветви образца с глубокой вложенностью
 *
 * @note Числа укладываются работами `cbor_encode_uint` и `cbor_encode_negint`, а не
 *       работами с приписью `64`: последние пишут восемь октетов всегда, и запись
 *       вышла бы вдвое крупнее нужного - сличалась бы не реализация, а выбор работы
 *
 * @param depth оставшаяся глубина вложенности
 *
 */
static void branch(const uint32_t depth) noexcept {
	/**
	 * Если глубина вложенности исчерпана
	 */
	if(depth == 0){
		// Выполняем укладку отображения листа ветви
		commit(cbor_encode_map_start(1, place(), 16));
		// Выполняем укладку имени поля листа ветви
		put("value");
		// Выполняем укладку значения листа ветви
		commit(cbor_encode_uint(1, place(), 16));
		// Выходим из работы
		return;
	}
	// Выполняем получение номера яруса вложенности
	const uint32_t level = (rival::NESTED_DEPTH - depth);
	// Выполняем сборку имени поля яруса вложенности
	const std::string key = ("k" + std::to_string(level));
	// Выполняем укладку отображения яруса вложенности
	commit(cbor_encode_map_start(1, place(), 16));
	// Выполняем укладку имени поля яруса вложенности
	put(key.data(), key.size());
	// Выполняем укладку массива яруса вложенности
	commit(cbor_encode_array_start(2, place(), 16));
	// Выполняем укладку вложенной ветви
	branch(depth - 1);
	// Выполняем укладку отображения соседа вложенной ветви
	commit(cbor_encode_map_start(1, place(), 16));
	// Выполняем укладку имени поля соседа вложенной ветви
	put("n");
	// Выполняем укладку значения соседа вложенной ветви
	commit(cbor_encode_uint(level, place(), 16));
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
	// Выполняем очистку собираемой записи
	produced.clear();
	// Выполняем сброс признака отказа сборки
	failed = false;
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
			commit(cbor_encode_array_start(objects.size(), place(), 16));
			/**
			 * Выполняем укладку всех записей образца
			 */
			for(size_t i = 0; i < objects.size(); i++){
				// Выполняем получение очередной записи образца
				const rival::object_t & object = objects.at(i);
				// Выполняем укладку отображения записи
				commit(cbor_encode_map_start(6, place(), 16));
				// Выполняем укладку признака деятельности записи
				put(fields[0]);
				// Выполняем укладку значения признака деятельности
				commit(cbor_encode_bool(object.active, place(), 16));
				// Выполняем укладку величины записи
				put(fields[1]);
				// Выполняем укладку значения величины
				commit(cbor_encode_double(object.amount, place(), 16));
				// Выполняем укладку города записи
				put(fields[2]);
				// Выполняем укладку значения города
				put(object.city.data(), object.city.size());
				// Выполняем укладку опознавателя записи
				put(fields[3]);
				// Выполняем укладку значения опознавателя
				commit(cbor_encode_uint(object.id, place(), 16));
				// Выполняем укладку названия записи
				put(fields[4]);
				// Выполняем укладку значения названия
				put(object.name.data(), object.name.size());
				// Выполняем укладку заметки записи
				put(fields[5]);
				// Выполняем укладку пустого значения заметки
				commit(cbor_encode_null(place(), 16));
			}
		} break;
		/**
		 * Если собирается перечень чисел
		 */
		case static_cast <uint8_t> (rival::scene_t::NUMBERS): {
			// Выполняем получение образца с преобладанием чисел
			const auto & numbers = rival::numbers();
			// Выполняем укладку массива чисел
			commit(cbor_encode_array_start(numbers.size(), place(), 16));
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
					/**
					 * Если числом является целое со знаком.
					 *
					 * Запись CBOR несёт величину, уменьшенную на единицу: значению `-1`
					 * отвечает записанный нуль
					 */
					case static_cast <uint8_t> (rival::numeric_t::INTEGER):
						commit(cbor_encode_negint(static_cast <uint64_t> (-(number.integer + 1)), place(), 16));
					break;
					// Если числом является дробное
					case static_cast <uint8_t> (rival::numeric_t::REAL):
						commit(cbor_encode_double(number.real, place(), 16));
					break;
					// Если числом является целое без знака
					default: commit(cbor_encode_uint(number.natural, place(), 16));
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
			commit(cbor_encode_array_start(strings.size(), place(), 16));
			/**
			 * Выполняем укладку всех строк образца
			 */
			for(size_t i = 0; i < strings.size(); i++)
				// Выполняем укладку очередной строки образца
				put(strings.at(i).data(), strings.at(i).size());
		} break;
		/**
		 * Если собирается перечень двоичных значений
		 */
		case static_cast <uint8_t> (rival::scene_t::BLOBS): {
			// Выполняем получение образца с преобладанием двоичных значений
			const auto & blobs = rival::blobs();
			// Выполняем укладку массива двоичных значений
			commit(cbor_encode_array_start(blobs.size(), place(), 16));
			/**
			 * Выполняем укладку всех двоичных значений образца
			 */
			for(size_t i = 0; i < blobs.size(); i++){
				// Выполняем укладку длины очередного двоичного значения
				commit(cbor_encode_bytestring_start(blobs.at(i).size(), place(), 16));
				// Выполняем укладку октетов очередного двоичного значения
				produced.append(reinterpret_cast <const char *> (blobs.at(i).data()), blobs.at(i).size());
			}
		} break;
		/**
		 * Если собирается образец с глубокой вложенностью
		 */
		case static_cast <uint8_t> (rival::scene_t::NESTED): {
			// Выполняем укладку массива ветвей
			commit(cbor_encode_array_start(rival::NESTED_COUNT, place(), 16));
			/**
			 * Выполняем укладку всех ветвей образца
			 */
			for(size_t i = 0; i < rival::NESTED_COUNT; i++)
				// Выполняем укладку очередной ветви образца
				branch(rival::NESTED_DEPTH);
		} break;
		/**
		 * Если собирается малая запись образца
		 */
		default: {
			// Выполняем укладку отображения малой записи
			commit(cbor_encode_map_start(5, place(), 16));
			// Выполняем укладку признака деятельности записи
			put("active");
			// Выполняем укладку значения признака деятельности
			commit(cbor_encode_bool(true, place(), 16));
			// Выполняем укладку величины записи
			put("amount");
			// Выполняем укладку значения величины
			commit(cbor_encode_double(42.5, place(), 16));
			// Выполняем укладку опознавателя записи
			put("id");
			// Выполняем укладку значения опознавателя
			commit(cbor_encode_uint(17, place(), 16));
			// Выполняем укладку названия записи
			put("name");
			// Выполняем укладку значения названия
			put("Товар");
			// Выполняем укладку меток записи
			put("tags");
			// Выполняем укладку массива меток записи
			commit(cbor_encode_array_start(2, place(), 16));
			// Выполняем укладку первой метки записи
			put("один");
			// Выполняем укладку второй метки записи
			put("два");
		}
	}
	/**
	 * Если сборка записи отвечена отказом
	 */
	if(failed)
		// Выводим признак неудачной сборки
		return false;
	// Выполняем выдачу собранной записи
	record.assign(produced);
	// Выводим признак успешной сборки
	return !record.empty();
}
/**
 * @brief Функция обхода разобранного значения
 *
 * @param item обходимое значение записи
 *
 */
static void walk(const cbor_item_t * item) noexcept {
	/**
	 * Определяем вид обходимого значения записи
	 */
	switch(static_cast <uint8_t> (cbor_typeof(item))){
		// Если значение является целым без знака
		case CBOR_TYPE_UINT:
			// Выполняем учёт прочитанного числа
			rival::consume(static_cast <double> (cbor_get_int(item)));
		break;
		/**
		 * Если значение является целым со знаком.
		 *
		 * Запись CBOR несёт величину, уменьшенную на единицу: записанному нулю
		 * отвечает значение `-1`
		 */
		case CBOR_TYPE_NEGINT:
			// Выполняем учёт прочитанного числа
			rival::consume(-static_cast <double> (cbor_get_int(item)) - 1.0);
		break;
		// Если значение является двоичными данными
		case CBOR_TYPE_BYTESTRING:
			// Выполняем учёт прочитанного содержимого значения
			rival::consume(cbor_bytestring_handle(item), cbor_bytestring_length(item));
		break;
		// Если значение является строкой
		case CBOR_TYPE_STRING:
			// Выполняем учёт прочитанного содержимого значения
			rival::consume(cbor_string_handle(item), cbor_string_length(item));
		break;
		/**
		 * Если значение является массивом
		 */
		case CBOR_TYPE_ARRAY: {
			// Выполняем получение указателя значений массива
			cbor_item_t * const * items = cbor_array_handle(item);
			// Выполняем получение количества значений массива
			const size_t count = cbor_array_size(item);
			/**
			 * Выполняем обход всех значений массива
			 */
			for(size_t i = 0; i < count; i++)
				// Выполняем обход очередного значения массива
				walk(items[i]);
		} break;
		/**
		 * Если значение является отображением
		 */
		case CBOR_TYPE_MAP: {
			// Выполняем получение указателя пар отображения
			struct cbor_pair * pairs = cbor_map_handle(item);
			// Выполняем получение количества пар отображения
			const size_t count = cbor_map_size(item);
			/**
			 * Выполняем обход всех пар отображения
			 */
			for(size_t i = 0; i < count; i++){
				// Выполняем обход имени поля отображения
				walk(pairs[i].key);
				// Выполняем обход значения поля отображения
				walk(pairs[i].value);
			}
		} break;
		/**
		 * Если значение является особым
		 */
		case CBOR_TYPE_FLOAT_CTRL: {
			// Если значение является дробным
			if(cbor_is_float(item))
				// Выполняем учёт прочитанного числа
				rival::consume(cbor_float_get_float(item));
			// Иначе, если значение является логическим
			else if(cbor_is_bool(item))
				// Выполняем учёт прочитанного логического значения
				rival::consume(cbor_get_bool(item));
			// Иначе выполняем учёт прочитанного пустого значения
			else rival::nothing();
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
	// Итог разбора записи
	struct cbor_load_result result;
	// Выполняем разбор записи
	cbor_item_t * item = cbor_load(reinterpret_cast <const unsigned char *> (record.data()), record.size(), &result);
	/**
	 * Если разобрать запись не удалось
	 */
	if(item == nullptr)
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход разобранного значения записи
	walk(item);
	// Выполняем освобождение разобранного значения записи
	cbor_decref(&item);
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
