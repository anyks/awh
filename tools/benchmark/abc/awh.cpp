/**
 * @file awh.cpp
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
 * @brief Эталонный стенд сравнения бинарного контейнера ABC — сборка записи
 *        средствами библиотеки AWH и разбор её с полным обходом собранного дерева
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <codec/abc/writer.hpp>
#include <codec/abc/document.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Признак снятия проверки строк на соответствие кодировке UTF-8
 *
 */
static bool relaxed = false;

/**
 * @brief Функция получения объекта для работы с логами
 *
 * @details Кодек связку берёт конструктором, и журнал ему обязателен. Замерам он не
 *          нужен вовсе, оттого вывод записей здесь снят целиком: работа журнала при
 *          отказе исказила бы замер, а отказов на замерах и не бывает
 *
 * @return объект для работы с логами
 *
 */
static const awh::log_t * logger() noexcept {
	// Объект фреймворка
	static awh::fmk_t fmk;
	// Объект для работы с логами
	static awh::log_t log(& fmk);
	// Снимаем вывод записей журнала
	log.level(awh::log_t::level_t::NONE);
	// Выводим объект для работы с логами
	return & log;
}

/**
 * @brief Функция сборки ветви образца с глубокой вложенностью
 *
 * @param writer сборка бинарной записи
 * @param depth  оставшаяся глубина вложенности
 * @return       признак успешности сборки
 *
 */
static bool branch(awh::codec::abc::writer_t & writer, const uint32_t depth) noexcept {
	/**
	 * Если глубина вложенности исчерпана
	 */
	if(depth == 0){
		// Выполняем укладку начала отображения листа ветви
		if(!writer.mapBegin(static_cast <uint64_t> (1)))
			// Выводим признак неудачной сборки
			return false;
		// Выполняем укладку имени поля листа ветви
		if(!writer.text("value"))
			// Выводим признак неудачной сборки
			return false;
		// Выполняем укладку значения листа ветви
		if(!writer.number(static_cast <uint64_t> (1)))
			// Выводим признак неудачной сборки
			return false;
		// Выводим результат закрытия отображения листа ветви
		return writer.mapEnd();
	}
	// Выполняем получение номера яруса вложенности
	const uint32_t level = (rival::NESTED_DEPTH - depth);
	// Выполняем сборку имени поля яруса вложенности
	const std::string key = ("k" + std::to_string(level));
	// Выполняем укладку начала отображения яруса вложенности
	if(!writer.mapBegin(static_cast <uint64_t> (1)))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку имени поля яруса вложенности
	if(!writer.text(key))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку начала массива яруса вложенности
	if(!writer.arrayBegin(static_cast <uint64_t> (2)))
		// Выводим признак неудачной сборки
		return false;
	// Если собрать вложенную ветвь не удалось
	if(!branch(writer, depth - 1))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку начала отображения соседа вложенной ветви
	if(!writer.mapBegin(static_cast <uint64_t> (1)))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку имени поля соседа вложенной ветви
	if(!writer.text("n"))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку значения соседа вложенной ветви
	if(!writer.number(static_cast <uint64_t> (level)))
		// Выводим признак неудачной сборки
		return false;
	// Если закрыть отображение соседа вложенной ветви не удалось
	if(!writer.mapEnd())
		// Выводим признак неудачной сборки
		return false;
	// Если закрыть массив яруса вложенности не удалось
	if(!writer.arrayEnd())
		// Выводим признак неудачной сборки
		return false;
	// Выводим результат закрытия отображения яруса вложенности
	return writer.mapEnd();
}
/**
 * @brief Функция сборки малой записи образца
 *
 * @param writer сборка бинарной записи
 * @return       признак успешности сборки
 *
 */
static bool tiny(awh::codec::abc::writer_t & writer) noexcept {
	// Выполняем укладку начала отображения малой записи
	if(!writer.mapBegin(static_cast <uint64_t> (5)))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку признака деятельности записи
	if(!writer.text("active") || !writer.boolean(true))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку величины записи
	if(!writer.text("amount") || !writer.number(static_cast <double> (42.5)))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку опознавателя записи
	if(!writer.text("id") || !writer.number(static_cast <uint64_t> (17)))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку названия записи
	if(!writer.text("name") || !writer.text("Товар"))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку имени поля меток записи
	if(!writer.text("tags"))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку начала массива меток записи
	if(!writer.arrayBegin(static_cast <uint64_t> (2)))
		// Выводим признак неудачной сборки
		return false;
	// Выполняем укладку первой метки записи
	if(!writer.text("один") || !writer.text("два"))
		// Выводим признак неудачной сборки
		return false;
	// Если закрыть массив меток записи не удалось
	if(!writer.arrayEnd())
		// Выводим признак неудачной сборки
		return false;
	// Выводим результат закрытия отображения малой записи
	return writer.mapEnd();
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
	// Сборка бинарной записи
	awh::codec::abc::writer_t writer(::logger());
	/**
	 * Если проверка строк на соответствие кодировке снята.
	 *
	 * Сличаемые реализации строк не проверяют вовсе, и снятие проверки показывает,
	 * во что она обходится. По умолчанию проверка стоит: молчаливая укладка негодной
	 * строки означала бы запись, чей разбор отвечает отказом у самого же владельца
	 */
	if(relaxed){
		// Выполняем получение настроек сборки записи
		awh::codec::abc::writer_t::settings_t settings = writer.settings();
		// Выполняем снятие проверки строк на соответствие кодировке
		settings.validate = false;
		// Выполняем установку настроек сборки записи
		writer.settings(settings);
	}
	// Признак успешности сборки записи
	bool result = true;
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
			// Выполняем укладку начала массива записей
			result = writer.arrayBegin(static_cast <uint64_t> (objects.size()));
			/**
			 * Выполняем укладку всех записей образца
			 */
			for(size_t i = 0; result && (i < objects.size()); i++){
				// Выполняем получение очередной записи образца
				const rival::object_t & object = objects.at(i);
				// Выполняем укладку начала отображения записи
				result = (writer.mapBegin(static_cast <uint64_t> (6)) &&
				 writer.text(fields[0]) && writer.boolean(object.active) &&
				 writer.text(fields[1]) && writer.number(object.amount) &&
				 writer.text(fields[2]) && writer.text(object.city) &&
				 writer.text(fields[3]) && writer.number(object.id) &&
				 writer.text(fields[4]) && writer.text(object.name) &&
				 writer.text(fields[5]) && writer.nul() && writer.mapEnd());
			}
			// Если укладка записей успешна
			if(result)
				// Выполняем укладку конца массива записей
				result = writer.arrayEnd();
		} break;
		/**
		 * Если собирается перечень чисел
		 */
		case static_cast <uint8_t> (rival::scene_t::NUMBERS): {
			// Выполняем получение образца с преобладанием чисел
			const auto & numbers = rival::numbers();
			// Выполняем укладку начала массива чисел
			result = writer.arrayBegin(static_cast <uint64_t> (numbers.size()));
			/**
			 * Выполняем укладку всех чисел образца
			 */
			for(size_t i = 0; result && (i < numbers.size()); i++){
				// Выполняем получение очередного числа образца
				const rival::number_t & number = numbers.at(i);
				/**
				 * Определяем разновидность числа образца
				 */
				switch(static_cast <uint8_t> (number.kind)){
					// Если числом является целое со знаком
					case static_cast <uint8_t> (rival::numeric_t::INTEGER):
						result = writer.number(number.integer);
					break;
					// Если числом является дробное
					case static_cast <uint8_t> (rival::numeric_t::REAL):
						result = writer.number(number.real);
					break;
					// Если числом является целое без знака
					default: result = writer.number(number.natural);
				}
			}
			// Если укладка чисел успешна
			if(result)
				// Выполняем укладку конца массива чисел
				result = writer.arrayEnd();
		} break;
		/**
		 * Если собирается перечень строк
		 */
		case static_cast <uint8_t> (rival::scene_t::STRINGS): {
			// Выполняем получение образца с преобладанием строк
			const auto & strings = rival::strings();
			// Выполняем укладку начала массива строк
			result = writer.arrayBegin(static_cast <uint64_t> (strings.size()));
			/**
			 * Выполняем укладку всех строк образца
			 */
			for(size_t i = 0; result && (i < strings.size()); i++)
				// Выполняем укладку очередной строки образца
				result = writer.text(strings.at(i));
			// Если укладка строк успешна
			if(result)
				// Выполняем укладку конца массива строк
				result = writer.arrayEnd();
		} break;
		/**
		 * Если собирается перечень двоичных значений
		 */
		case static_cast <uint8_t> (rival::scene_t::BLOBS): {
			// Выполняем получение образца с преобладанием двоичных значений
			const auto & blobs = rival::blobs();
			// Выполняем укладку начала массива двоичных значений
			result = writer.arrayBegin(static_cast <uint64_t> (blobs.size()));
			/**
			 * Выполняем укладку всех двоичных значений образца
			 */
			for(size_t i = 0; result && (i < blobs.size()); i++)
				// Выполняем укладку очередного двоичного значения образца
				result = writer.blob(blobs.at(i).data(), blobs.at(i).size());
			// Если укладка двоичных значений успешна
			if(result)
				// Выполняем укладку конца массива двоичных значений
				result = writer.arrayEnd();
		} break;
		/**
		 * Если собирается образец с глубокой вложенностью
		 */
		case static_cast <uint8_t> (rival::scene_t::NESTED): {
			// Выполняем укладку начала массива ветвей
			result = writer.arrayBegin(static_cast <uint64_t> (rival::NESTED_COUNT));
			/**
			 * Выполняем укладку всех ветвей образца
			 */
			for(size_t i = 0; result && (i < rival::NESTED_COUNT); i++)
				// Выполняем укладку очередной ветви образца
				result = branch(writer, rival::NESTED_DEPTH);
			// Если укладка ветвей успешна
			if(result)
				// Выполняем укладку конца массива ветвей
				result = writer.arrayEnd();
		} break;
		// Если собирается малая запись образца
		default: result = tiny(writer);
	}
	/**
	 * Если сборка записи отвечена отказом
	 */
	if(!result)
		// Выводим признак неудачной сборки
		return false;
	// Выполняем выдачу собранной записи
	record.assign(reinterpret_cast <const char *> (writer.record().data()), writer.record().size());
	// Выводим признак успешной сборки
	return true;
}
/**
 * @brief Функция обхода собранного дерева документа
 *
 * @param value обходимое значение документа
 *
 */
static void walk(const awh::codec::abc::document_t::value_t & value) noexcept {
	/**
	 * Определяем вид обходимого значения документа
	 */
	switch(static_cast <uint8_t> (value.kind())){
		// Если значение является пустым
		case static_cast <uint8_t> (awh::codec::abc::kind_t::NUL):
			// Выполняем учёт прочитанного пустого значения
			rival::nothing();
		break;
		/**
		 * Если значение является логическим
		 */
		case static_cast <uint8_t> (awh::codec::abc::kind_t::BOOL): {
			// Прочитанное логическое значение
			bool result = false;
			// Выполняем извлечение логического значения
			(void) value.value(result);
			// Выполняем учёт прочитанного логического значения
			rival::consume(result);
		} break;
		/**
		 * Если значение является числом
		 */
		case static_cast <uint8_t> (awh::codec::abc::kind_t::NUMBER): {
			// Прочитанное число
			double result = 0.0;
			// Выполняем извлечение числа
			(void) value.value(result);
			// Выполняем учёт прочитанного числа
			rival::consume(result);
		} break;
		// Если значение является строкой либо двоичными данными
		case static_cast <uint8_t> (awh::codec::abc::kind_t::STRING):
		case static_cast <uint8_t> (awh::codec::abc::kind_t::BLOB):
			// Выполняем учёт прочитанного содержимого значения
			rival::consume(value.data().data(), value.data().size());
		break;
		/**
		 * Если значение является вместимым.
		 *
		 * Обход ведётся первым значением вместе с переходом к соседу: обращение по
		 * номеру пропускает узлы, стоящие до затребованного, и обход им обошёлся бы
		 * дороже квадрата. Имя поля отображения обходится тем же чередом, ибо стоит
		 * оно таким же узлом, что и значение
		 */
		case static_cast <uint8_t> (awh::codec::abc::kind_t::ARRAY):
		case static_cast <uint8_t> (awh::codec::abc::kind_t::MAP): {
			/**
			 * Выполняем обход всех значений вместимого
			 */
			for(auto item = value.begin(); item.valid(); item = item.next())
				// Выполняем обход очередного значения вместимого
				walk(item);
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
	// Дерево документа
	awh::codec::abc::document_t document(::logger());
	/**
	 * Если разобрать запись не удалось
	 */
	if(!document.parse(record.data(), record.size()))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем обход собранного дерева документа
	walk(document.root());
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
	/**
	 * Выполняем перебор всех параметров запуска
	 */
	for(int32_t i = 1; i < argc; i++){
		// Если параметром является снятие проверки строк
		if(::strcmp(argv[i], "--relaxed") == 0)
			// Выполняем установку признака снятия проверки строк
			relaxed = true;
	}
	// Работы стенда, сличаемые с прочими стендами
	const rival::stand_t stand{writing, reading};
	// Выполняем прогон всех сценариев стенда
	return rival::drive(stand, argc, argv);
}
