/**
 * @file abc.cpp
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
 * @brief Инструмент фаззинга бинарного контейнера ABC — построение записей произвольной
 *        вложенности с точечной порчей, подача их разбору целиком и кусками произвольного
 *        размера, сборка контейнера с оглавлением и подписью, правка его на месте и уборка
 *        мусора для поиска аварийных завершений, выходов за границы буфера и расхождений
 *        разбора
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/abc/abc.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён контейнеров данных
 */
using namespace awh;
using namespace awh::codec;

/**
 * @brief Внутренние вспомогательные средства генератора (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Учёт проделанной работы
	 *
	 */
	struct Statistic {
		// Количество построенных записей
		uint64_t records;
		// Количество испорченных записей
		uint64_t corrupted;
		// Количество записей, разобранных до конца
		uint64_t survived;
		// Количество выданных разбором событий
		uint64_t events;
		// Количество собранных деревьев документа
		uint64_t trees;
		// Количество собранных контейнеров
		uint64_t containers;
		// Количество выборок записи по номеру
		uint64_t fetches;
		// Количество правок контейнера на месте
		uint64_t edits;
		// Количество уборок мусора
		uint64_t compactions;
		// Количество сошедшихся поверок подписи
		uint64_t verified;
		// Количество значений, пересобранных потоковой сборкой
		uint64_t assemblies;
		/**
		 * @brief Конструктор
		 *
		 */
		Statistic() noexcept :
		 records(0), corrupted(0), survived(0), events(0), trees(0), containers(0),
		 fetches(0), edits(0), compactions(0), verified(0), assemblies(0) {}
	} totals;

	/**
	 * @brief Событие разбора, запомненное для сличения
	 *
	 * @details Сличается вся выдача разбора целиком: расхождение хотя бы одного признака
	 *          означает зависимость разбора от нарезки записи на куски
	 *
	 */
	struct Event {
		// Разновидность события и вид значения
		uint8_t event;
		// Вид значения события
		uint32_t type;
		// Содержимое строкового либо двоичного значения
		string data;
		// Количество значений вместимого
		uint64_t count;
		// Целое без знака
		uint64_t number;
		// Целое со знаком
		int64_t integer;
		/**
		 * Двоичное представление числа с плавающей точкой
		 *
		 * @note Сличается именно представление, а не само число: нечисло само себе не
		 *       равно, и сличение чисел приняло бы два нечисла за расхождение
		 */
		uint64_t real;
		// Десятичный порядок величины
		int64_t exponent;
		// Логическое значение и признаки величины
		bool boolean, negative, indefinite;
		// Глубина вложенности события
		uint32_t depth;
		/**
		 * @brief Конструктор
		 *
		 */
		Event() noexcept :
		 event(0), type(0), count(0), number(0), integer(0), real(0),
		 exponent(0), boolean(false), negative(false), indefinite(false), depth(0) {}
		/**
		 * @brief Оператор сличения событий
		 *
		 * @param other сличаемое событие
		 * @return      признак расхождения событий
		 *
		 */
		bool operator != (const Event & other) const noexcept {
			// Выводим признак расхождения событий разбора
			return ((this->event != other.event) || (this->type != other.type) ||
			 (this->data != other.data) || (this->count != other.count) ||
			 (this->number != other.number) || (this->integer != other.integer) ||
			 (this->real != other.real) || (this->exponent != other.exponent) ||
			 (this->boolean != other.boolean) || (this->negative != other.negative) ||
			 (this->indefinite != other.indefinite) || (this->depth != other.depth));
		}
	};

	/**
	 * @brief Источник случайных величин
	 *
	 * @details Зерно закреплено намеренно: прогон обязан повторяться числом в число,
	 *          иначе найденный дефект не воспроизвести
	 *
	 */
	mt19937_64 source(0x41424331ull);

	/**
	 * @brief Функция получения случайного числа в пределах
	 *
	 * @param from нижний предел
	 * @param to   верхний предел
	 * @return     полученное число
	 *
	 */
	uint64_t number(const uint64_t from, const uint64_t to) noexcept {
		/**
		 * Если пределы сошлись
		 */
		if(from >= to)
			// Выводим нижний предел
			return from;
		// Выводим случайное число в заданных пределах
		return (from + (source() % ((to - from) + 1)));
	}
	/**
	 * @brief Функция получения случайного текста
	 *
	 * @param length длина получаемого текста
	 * @return       полученный текст
	 *
	 */
	string text(const size_t length) noexcept {
		// Собираемый текст
		string result;
		// Выполняем заведение места под собираемый текст
		result.reserve(length);
		/**
		 * Выполняем сборку текста из знаков разной ширины записи UTF-8
		 */
		for(size_t i = 0; i < length; i++){
			// Выполняем получение вида очередного знака
			const uint64_t kind = number(0, 9);
			// Если знак берётся однооктетный
			if(kind < 6)
				// Выполняем добавление однооктетного знака
				result.push_back(static_cast <char> (number(0x20, 0x7E)));
			// Иначе, если знак берётся двухоктетный
			else if(kind < 9)
				// Выполняем добавление двухоктетного знака кириллицы
				result.append("ы");
			// Иначе выполняем добавление четырёхоктетного знака
			else result.append("𝄞");
		}
		// Выводим собранный текст
		return result;
	}
	/**
	 * @brief Функция построения значения записи
	 *
	 * @param writer сборщик бинарной записи
	 * @param depth  оставшаяся глубина вложенности
	 * @return       признак успешного построения
	 *
	 */
	bool compose(abc::writer_t & writer, const uint32_t depth) noexcept {
		/**
		 * Выполняем получение вида строимого значения: на исходе глубины вместимые
		 * из выбора изымаются, иначе построение уходит в бесконечную вложенность
		 */
		const uint64_t kind = number(0, (depth > 0 ? 15 : 9));
		/**
		 * Определяем вид строимого значения
		 */
		switch(kind){
			// Если строится пустое значение
			case 0: return writer.nul();
			// Если строится логическое значение
			case 1: return writer.boolean(number(0, 1) == 1);
			// Если строится целое со знаком
			case 2: return writer.number(static_cast <int64_t> (source()));
			// Если строится целое без знака
			case 3: return writer.number(static_cast <uint64_t> (source()));
			// Если строится дробное число
			case 4: {
				// Выполняем получение вида дробного числа
				const uint64_t special = number(0, 9);
				// Если берётся нечисло
				if(special == 0)
					// Выполняем укладку нечисла
					return writer.number(numeric_limits <double>::quiet_NaN());
				// Если берётся бесконечность
				if(special == 1)
					// Выполняем укладку бесконечности
					return writer.number(numeric_limits <double>::infinity());
				// Выполняем укладку обыкновенного дробного числа
				return writer.number(static_cast <double> (static_cast <int64_t> (source())) / 1024.0);
			}
			// Если строится строка
			case 5: {
				// Выполняем получение содержимого строки
				const string value = text(static_cast <size_t> (number(0, 24)));
				// Выполняем укладку строки
				return writer.text(value);
			}
			// Если строятся двоичные данные
			case 6: {
				// Собираемые двоичные данные
				vector <uint8_t> value(static_cast <size_t> (number(0, 24)), 0);
				// Выполняем перебор всех октетов двоичных данных
				for(uint8_t & octet : value)
					// Выполняем установку очередного октета
					octet = static_cast <uint8_t> (number(0, 0xFF));
				// Выполняем укладку двоичных данных
				return writer.blob(value.data(), value.size());
			}
			// Если строится отметка времени
			case 7: return writer.timestamp(static_cast <int64_t> (source()));
			// Если строится опознаватель
			case 8: {
				// Собираемый опознаватель
				uint8_t value[16];
				// Выполняем перебор всех октетов опознавателя
				for(size_t i = 0; i < sizeof(value); i++)
					// Выполняем установку очередного октета
					value[i] = static_cast <uint8_t> (number(0, 0xFF));
				// Выполняем укладку опознавателя
				return writer.uuid(value, sizeof(value));
			}
			// Если строится число неограниченной ширины
			case 9: {
				// Собираемая величина числа
				vector <uint8_t> value(static_cast <size_t> (number(1, 20)), 0);
				// Выполняем перебор всех октетов величины
				for(uint8_t & octet : value)
					// Выполняем установку очередного октета
					octet = static_cast <uint8_t> (number(0, 0xFF));
				// Выполняем укладку числа неограниченной ширины
				return writer.bignum(value.data(), value.size(), (number(0, 1) == 1));
			}
			// Если строится перечень
			case 10:
			case 11: {
				// Выполняем получение количества значений перечня
				const uint64_t count = number(0, 4);
				// Выполняем получение признака неопределённой длины перечня
				const bool indefinite = (number(0, 3) == 0);
				/**
				 * Если открыть перечень не вышло
				 */
				if(!(indefinite ? writer.arrayBegin() : writer.arrayBegin(count)))
					// Выводим признак неудачного построения
					return false;
				/**
				 * Выполняем построение всех значений перечня
				 */
				for(uint64_t i = 0; i < count; i++){
					// Если построить очередное значение перечня не вышло
					if(!compose(writer, depth - 1))
						// Выводим признак неудачного построения
						return false;
				}
				// Выводим результат закрытия перечня
				return writer.arrayEnd();
			}
			/**
			 * Если строится значение, собираемое кусками
			 */
			case 13:
			case 14: {
				/**
				 * Признак того, что собирается строка, а не двоичные данные.
				 *
				 * Имя взято не «string»: оно заслонило бы собою тип строки, и сборка
				 * повалилась бы на первом же употреблении его
				 */
				const bool textual = (kind == 13);
				/**
				 * Если открыть значение, собираемое кусками, не вышло
				 */
				if(!(textual ? writer.textBegin() : writer.blobBegin()))
					// Выводим признак неудачного построения
					return false;
				// Выполняем получение количества кусков значения
				const uint64_t count = number(0, 4);
				/**
				 * Выполняем построение всех кусков значения
				 */
				for(uint64_t i = 0; i < count; i++){
					// Если собирается строка
					if(textual){
						// Выполняем получение содержимого куска строки
						const string value = text(static_cast <size_t> (number(0, 12)));
						// Если уложить кусок строки не вышло
						if(!writer.text(value))
							// Выводим признак неудачного построения
							return false;
					// Иначе собираются двоичные данные
					} else {
						// Собираемый кусок двоичных данных
						vector <uint8_t> value(static_cast <size_t> (number(0, 12)), 0);
						// Выполняем перебор всех октетов куска
						for(uint8_t & octet : value)
							// Выполняем установку очередного октета
							octet = static_cast <uint8_t> (number(0, 0xFF));
						// Если уложить кусок двоичных данных не вышло
						if(!writer.blob(value.data(), value.size()))
							// Выводим признак неудачного построения
							return false;
					}
				}
				// Выводим результат закрытия значения, собираемого кусками
				return (textual ? writer.textEnd() : writer.blobEnd());
			}
			// Если строится отображение
			case 12: {
				// Выполняем получение количества полей отображения
				const uint64_t count = number(0, 4);
				// Выполняем получение признака неопределённой длины отображения
				const bool indefinite = (number(0, 3) == 0);
				/**
				 * Если открыть отображение не вышло
				 */
				if(!(indefinite ? writer.mapBegin() : writer.mapBegin(count)))
					// Выводим признак неудачного построения
					return false;
				/**
				 * Выполняем построение всех полей отображения
				 */
				for(uint64_t i = 0; i < count; i++){
					/**
					 * Выполняем сборку имени поля отображения возрастающим: строгий вид
					 * записи имена по возрастанию и требует
					 */
					const string key = ("k" + to_string(i) + "_" + text(static_cast <size_t> (number(0, 6))));
					// Если уложить имя поля отображения не вышло
					if(!writer.text(key))
						// Выводим признак неудачного построения
						return false;
					// Если построить значение поля отображения не вышло
					if(!compose(writer, depth - 1))
						// Выводим признак неудачного построения
						return false;
				}
				// Выводим результат закрытия отображения
				return writer.mapEnd();
			}
			/**
			 * Если строится открытое расширение, заведённое потребителем
			 */
			case 15: {
				// Собираемые октеты расширения
				vector <uint8_t> value(static_cast <size_t> (number(0, 24)), 0);
				// Выполняем перебор всех октетов расширения
				for(uint8_t & octet : value)
					// Выполняем установку очередного октета
					octet = static_cast <uint8_t> (number(0, 0xFF));
				// Выполняем укладку открытого расширения
				return writer.custom(source(), value.data(), value.size());
			}
		}
		// Выводим признак неудачного построения
		return false;
	}
	/**
	 * @brief Функция построения записи контейнера
	 *
	 * @param result буфер, куда следует положить построенную запись
	 * @return       признак успешно построенной записи
	 *
	 */
	bool assemble(vector <uint8_t> & result) noexcept {
		// Сборщик бинарной записи
		abc::writer_t writer;
		// Получаем настройки сборки бинарной записи
		abc::writer_t::settings_t settings = writer.settings();
		// Выполняем установку строгого вида записи через раз
		settings.canonical = (number(0, 1) == 1);
		/**
		 * Выполняем снятие проверки строк через раз: проверка эта отвергает то, что
		 * разбору всё равно придётся встретить на носителе
		 */
		settings.validate = (number(0, 1) == 1);
		/**
		 * Выполняем объявление размаха вместимых через раз, порогом в четыре значения
		 *
		 * @details Метка размаха есть расширение проволочной записи, и порча её ложится
		 * на общий путь: восемь октетов размаха стоят посреди записи, и всякая порча
		 * записи бьёт по ним наравне с прочим. Ради этого ворошителю и нужно, чтобы
		 * метка эта в записях ВСТРЕЧАЛАСЬ, - иначе стойкость к враждебному размаху
		 * поверялась бы одними лишь нарочными проверками
		 */
		settings.spanned = ((number(0, 1) == 1) ? 4 : 0);
		/**
		 * Порог укладки содержимого ссылкой ворошителем не трогается: содержимое,
		 * уложенное ссылкой, обязано пережить выдачу записи, а строится запись из
		 * временных буферов построителя значения, кои выдачи не переживают
		 */
		// Выполняем установку настроек сборки бинарной записи
		writer.settings(settings);
		/**
		 * Если построить значение записи не вышло
		 */
		if(!compose(writer, static_cast <uint32_t> (number(0, 4))))
			// Выводим признак неудачно построенной записи
			return false;
		/**
		 * Если запись собрана не до конца
		 */
		if(!writer.complete())
			// Выводим признак неудачно построенной записи
			return false;
		// Выполняем выдачу построенной записи
		result = writer.record();
		// Выполняем увеличение количества построенных записей
		totals.records++;
		// Выводим признак успешно построенной записи
		return !result.empty();
	}
	/**
	 * @brief Функция порчи октетов
	 *
	 * @param buffer буфер портимых октетов
	 *
	 */
	void damage(vector <uint8_t> & buffer) noexcept {
		/**
		 * Если портить нечего
		 */
		if(buffer.empty())
			// Прекращаем порчу октетов
			return;
		// Выполняем получение количества портимых октетов
		const uint64_t count = number(1, 3);
		/**
		 * Выполняем порчу нескольких октетов подряд
		 */
		for(uint64_t i = 0; i < count; i++){
			/**
			 * Выполняем получение места порчи отдельным вызовом: два забора случайного
			 * в одном вызове зависели бы от порядка вычисления доводов
			 */
			const uint64_t place = number(0, static_cast <uint64_t> (buffer.size() - 1));
			// Выполняем получение вида порчи
			const uint64_t kind = number(0, 2);
			// Если октет заменяется случайным
			if(kind == 0)
				// Выполняем замену октета случайным
				buffer.at(static_cast <size_t> (place)) = static_cast <uint8_t> (number(0, 0xFF));
			// Иначе, если разряды октета обращаются
			else if(kind == 1)
				// Выполняем обращение разрядов октета
				buffer.at(static_cast <size_t> (place)) ^= 0xFF;
			// Иначе выполняем установку старшего разряда октета
			else buffer.at(static_cast <size_t> (place)) |= 0x80;
		}
		// Выполняем увеличение количества испорченных записей
		totals.corrupted++;
	}
	/**
	 * @brief Функция разбора записи с запоминанием событий
	 *
	 * @param buffer буфер разбираемой записи
	 * @param chunk  размер куска подачи, ноль - подача целиком
	 * @param events запомненные события разбора
	 * @param direct признак приёма октетов прямо в буфер разбора
	 * @return       признак записи, разобранной до конца
	 *
	 */
	bool parse(const vector <uint8_t> & buffer, const size_t chunk, vector <Event> & events, const bool direct = false) noexcept {
		// Разбиратель бинарной записи
		abc::reader_t reader;
		// Выполняем очистку запомненных событий разбора
		events.clear();
		// Смещение подачи разбираемой записи
		size_t offset = 0;
		/**
		 * Выполняем подачу разбираемой записи разбирателю
		 */
		while(offset <= buffer.size()){
			/**
			 * Выполняем получение размера подаваемого куска: при нулевом размере
			 * запись подаётся целиком
			 */
			const size_t size = ((chunk == 0) ? buffer.size() :
			 ((chunk < (buffer.size() - offset)) ? chunk : (buffer.size() - offset)));
			// Признак последнего подаваемого куска
			const bool last = ((offset + size) >= buffer.size());
			/**
			 * Выполняем подачу очередного куска разбираемой записи.
			 *
			 * События, набранные до отказа, выбираются и при отказе: разбор их уже
			 * выдал, и бросать их значило бы считать выдачу зависящей от нарезки там,
			 * где зависит лишь миг обнаружения отказа
			 */
			bool accepted = false;
			/**
			 * Если октеты принимаются прямо в буфер разбора. Место запрашивается с
			 * запасом, а принято бывает меньше: путь этот обязан выдать те же события,
			 * что и подача своим буфером
			 */
			if(direct){
				// Выполняем выдачу места под приём октетов записи
				void * place = reader.reserve(size + static_cast <size_t> (number(0, 8)));
				// Если место под приём октетов выдано
				if(place != nullptr){
					// Если принимаемые октеты не пусты
					if(size > 0)
						// Выполняем приём октетов записи прямо в выданное место
						::memcpy(place, buffer.data() + offset, size);
					// Выполняем подачу принятых октетов разбирателю
					accepted = reader.commit(size, last);
				// Иначе выполняем подачу октетов записи своим буфером
				} else accepted = reader.feed(buffer.data() + offset, size, last);
			// Иначе выполняем подачу октетов записи своим буфером
			} else accepted = reader.feed(buffer.data() + offset, size, last);
			/**
			 * Выполняем выдачу всех событий, какие набрались подачей
			 */
			while(reader.next()){
				// Запоминаемое событие разбора
				Event event;
				// Выполняем получение значения события разбора
				const abc::reader_t::value_t value = reader.value();
				// Выполняем запоминание разновидности события
				event.event = static_cast <uint8_t> (reader.event());
				// Выполняем запоминание вида значения события
				event.type = static_cast <uint32_t> (value.type);
				// Выполняем запоминание содержимого значения
				event.data.assign(value.data.data(), value.data.size());
				// Выполняем запоминание количества значений вместимого
				event.count = value.count;
				// Выполняем запоминание целого без знака
				event.number = value.number;
				// Выполняем запоминание целого со знаком
				event.integer = value.integer;
				/**
				 * Выполняем запоминание двоичного представления дробного числа:
				 * нечисло само себе не равно, и сличение чисел сочло бы его расхождением
				 */
				::memcpy(& event.real, & value.real, sizeof(event.real));
				// Выполняем запоминание десятичного порядка величины
				event.exponent = value.exponent;
				// Выполняем запоминание логического значения
				event.boolean = value.boolean;
				// Выполняем запоминание признака величины меньше нуля
				event.negative = value.negative;
				// Выполняем запоминание признака неопределённой длины
				event.indefinite = value.indefinite;
				// Выполняем запоминание глубины вложенности события
				event.depth = reader.depth();
				// Выполняем запоминание события разбора
				events.push_back(::std::move(event));
				// Выполняем увеличение количества выданных событий
				totals.events++;
			}
			/**
			 * Если подача отвечена отказом либо разбор оборвался
			 */
			if(!accepted || (reader.error() != abc::error_t::NONE))
				// Выводим признак записи, разобранной не до конца
				return false;
			/**
			 * Если запись подана целиком
			 */
			if(last)
				// Прекращаем подачу разбираемой записи
				break;
			// Выполняем сдвиг смещения подачи на размер поданного куска
			offset += size;
		}
		// Выводим признак записи, разобранной до конца
		return true;
	}
	/**
	 * @brief Функция сличения разбора записи при разной нарезке на куски
	 *
	 * @param buffer буфер разбираемой записи
	 *
	 */
	void slicing(const vector <uint8_t> & buffer) noexcept {
		// События разбора записи, поданной целиком
		vector <Event> whole;
		// Выполняем разбор записи, поданной целиком
		const bool survived = parse(buffer, 0, whole);
		/**
		 * Если запись разобрана до конца
		 */
		if(survived)
			// Выполняем увеличение количества разобранных записей
			totals.survived++;
		// Размеры кусков подачи, какими сличается разбор
		const size_t sizes[] = {1, 2, 3, 5, 7, 13, 64};
		/**
		 * Выполняем перебор всех размеров кусков подачи
		 */
		for(const size_t size : sizes){
			/**
			 * Выполняем сличение обоих путей подачи: своим буфером и приёмом октетов
			 * прямо в буфер разбора. Выдача событий от пути подачи зависеть не вправе
			 */
			for(uint8_t pass = 0; pass < 2; pass++){
			// Название пути подачи разбираемой записи
			const char * path = ((pass == 1) ? "direct" : "buffered");
			// События разбора записи, поданной кусками
			vector <Event> sliced;
			// Выполняем разбор записи, поданной кусками
			const bool result = parse(buffer, size, sliced, (pass == 1));
			/**
			 * Если исход разбора разошёлся с разбором записи, поданной целиком
			 */
			if(result != survived){
				// Выводим сообщение о расхождении исхода разбора
				::fprintf(stderr, "abc fuzz: parsing outcome differs at chunk size %zu (%s)\n", size, path);
				// Выполняем выход с признаком расхождения
				::exit(1);
			}
			/**
			 * Если количество событий разошлось
			 */
			if(sliced.size() != whole.size()){
				// Выводим сообщение о расхождении количества событий
				::fprintf(stderr, "abc fuzz: event count differs at chunk size %zu (%s): %zu against %zu\n",
				 size, path, sliced.size(), whole.size());
				// Выполняем выход с признаком расхождения
				::exit(1);
			}
			/**
			 * Выполняем перебор всех событий разбора
			 */
			for(size_t i = 0; i < sliced.size(); i++){
				/**
				 * Если событие разошлось с событием разбора записи, поданной целиком
				 */
				if(sliced.at(i) != whole.at(i)){
					// Выводим сообщение о расхождении события разбора
					::fprintf(stderr, "abc fuzz: event %zu differs at chunk size %zu (%s)\n", i, size, path);
					// Выводим признаки события разбора записи, поданной кусками
					::fprintf(stderr, "  sliced: event=%u type=%u data=%zu count=%llu number=%llu integer=%lld real=%llx exp=%lld bool=%d neg=%d ind=%d depth=%u\n",
					 sliced.at(i).event, sliced.at(i).type, sliced.at(i).data.size(),
					 (unsigned long long) sliced.at(i).count, (unsigned long long) sliced.at(i).number,
					 (long long) sliced.at(i).integer, (unsigned long long) sliced.at(i).real,
					 (long long) sliced.at(i).exponent, sliced.at(i).boolean ? 1 : 0,
					 sliced.at(i).negative ? 1 : 0, sliced.at(i).indefinite ? 1 : 0, sliced.at(i).depth);
					// Выводим признаки события разбора записи, поданной целиком
					::fprintf(stderr, "  whole:  event=%u type=%u data=%zu count=%llu number=%llu integer=%lld real=%llx exp=%lld bool=%d neg=%d ind=%d depth=%u\n",
					 whole.at(i).event, whole.at(i).type, whole.at(i).data.size(),
					 (unsigned long long) whole.at(i).count, (unsigned long long) whole.at(i).number,
					 (long long) whole.at(i).integer, (unsigned long long) whole.at(i).real,
					 (long long) whole.at(i).exponent, whole.at(i).boolean ? 1 : 0,
					 whole.at(i).negative ? 1 : 0, whole.at(i).indefinite ? 1 : 0, whole.at(i).depth);
					// Выполняем выход с признаком расхождения
					::exit(1);
				}
			}
			}
		}
	}
	/**
	 * @brief Функция сборки дерева документа и перезаписи его
	 *
	 * @param buffer буфер разбираемой записи
	 *
	 */
	/**
	 * @brief Функция пересборки владеющего значения потоковой сборкой
	 *
	 * @details Сборка вызовами обязана давать то же значение, что и разбор записи: путь
	 *          к открытому вместимому она ведёт сама, и расхождение означало бы, что
	 *          путь этот теряется на какой-то глубине
	 *
	 * @note Имя поля отображения сборка принимает всякого НЕВМЕСТИМОГО вида, ровно как
	 *       и сама запись. Отвергается ею лишь пустая строка именем: сборка ведёт путь
	 *       сама, и пустое имя в ней неотличимо от неназначенного
	 *
	 * @param builder потоковая сборка, какою ведётся пересборка
	 * @param value   пересобираемое владеющее значение
	 * @return        признак того, что значение потоковой сборке выразимо
	 *
	 */
	bool rebuild(abc::builder_t & builder, const abc::value_t & value) noexcept {
		/**
		 * Если пересобирается отображение
		 */
		if(value.is(abc::type_t::MAP)){
			// Если завести отображение не вышло
			if(!builder.map())
				// Выводим признак невыразимого значения
				return false;
			/**
			 * Выполняем перебор всех пар отображения
			 */
			for(size_t i = 0; i < value.size(); i++){
				/**
				 * Если назначить имя очередного поля отображения не вышло
				 *
				 * @note Имя поля сборка принимает всякого невместимого вида, ровно как и
				 *       запись: пустая строка именем ею отвергается, и такая запись
				 *       сличением пропускается
				 */
				if(!builder.key(value.key(i)))
					// Выводим признак невыразимого значения
					return false;
				// Если пересобрать значение поля не вышло
				if(!rebuild(builder, value[i]))
					// Выводим признак невыразимого значения
					return false;
			}
			// Выводим признак закрытия отображения
			return builder.close();
		/**
		 * Если пересобирается массив
		 */
		} else if(value.is(abc::type_t::ARRAY)) {
			// Если завести массив не вышло
			if(!builder.array())
				// Выводим признак невыразимого значения
				return false;
			/**
			 * Выполняем перебор всех значений массива
			 */
			for(size_t i = 0; i < value.size(); i++){
				// Если пересобрать очередное значение массива не вышло
				if(!rebuild(builder, value[i]))
					// Выводим признак невыразимого значения
					return false;
			}
			// Выводим признак закрытия массива
			return builder.close();
		}
		/**
		 * Значение-одиночка ложится как есть: вид его сборке разбирать незачем
		 */
		return builder.value(value);
	}
	void tree(const vector <uint8_t> & buffer) noexcept {
		// Дерево документа
		abc::document_t document;
		/**
		 * Если разобрать запись в дерево документа не вышло
		 */
		if(!document.parse(buffer.data(), buffer.size()))
			// Прекращаем сборку дерева документа
			return;
		// Выполняем увеличение количества собранных деревьев
		totals.trees++;
		// Владеющее значение документа
		abc::value_t value;
		/**
		 * Если разобрать запись во владеющее значение не вышло
		 */
		if(!value.parse(buffer.data(), buffer.size()))
			// Прекращаем сборку дерева документа
			return;
		// Выполняем перезапись владеющего значения
		const vector <uint8_t> rewritten = value.dump();
		/**
		 * Если перезапись значения вышла пустой
		 */
		if(rewritten.empty())
			// Прекращаем сборку дерева документа
			return;
		// Владеющее значение, собранное из перезаписи
		abc::value_t repeated;
		/**
		 * Если разобрать перезапись не вышло
		 */
		if(!repeated.parse(rewritten.data(), rewritten.size())){
			// Выводим сообщение о неразбираемой перезаписи
			::fprintf(stderr, "abc fuzz: rewritten record is not parsable\n");
			// Выполняем выход с признаком расхождения
			::exit(1);
		}
		/**
		 * Если перезапись разошлась с исходным значением
		 */
		if(!(repeated == value)){
			// Выводим сообщение о расхождении перезаписи
			::fprintf(stderr, "abc fuzz: rewritten record differs from the parsed one\n");
			// Выводим октеты исходной записи
			::fprintf(stderr, "  source:    ");
			// Выполняем перебор всех октетов исходной записи
			for(const uint8_t octet : buffer)
				// Выполняем вывод очередного октета исходной записи
				::fprintf(stderr, "%02X", octet);
			// Выводим октеты перезаписи
			::fprintf(stderr, "\n  rewritten: ");
			// Выполняем перебор всех октетов перезаписи
			for(const uint8_t octet : rewritten)
				// Выполняем вывод очередного октета перезаписи
				::fprintf(stderr, "%02X", octet);
			// Выводим перевод строки
			::fprintf(stderr, "\n");
			// Выполняем выход с признаком расхождения
			::exit(1);
		}
		// Потоковая сборка владеющего значения
		abc::builder_t builder;
		/**
		 * Если значение потоковой сборке выразимо, сличаем собранное с разобранным
		 */
		if(rebuild(builder, value)){
			// Выполняем завершение потоковой сборки значения
			const abc::value_t assembled = builder.finish();
			// Выполняем увеличение количества пересобранных значений
			totals.assemblies++;
			/**
			 * Если пересобранное значение разошлось с разобранным
			 */
			if(!(assembled == value)){
				// Выводим сообщение о расхождении пересборки
				::fprintf(stderr, "abc fuzz: assembled value differs from the parsed one\n");
				// Выводим октеты исходной записи
				::fprintf(stderr, "  source: ");
				// Выполняем перебор всех октетов исходной записи
				for(const uint8_t octet : buffer)
					// Выполняем вывод очередного октета исходной записи
					::fprintf(stderr, "%02X", octet);
				// Выводим перевод строки
				::fprintf(stderr, "\n");
				// Выполняем выход с признаком расхождения
				::exit(1);
			}
		}
	}
	/**
	 * @brief Класс носителя контейнера в памяти
	 *
	 */
	class Medium {
		public:
			// Октеты контейнера, лежащие на носителе
			vector <uint8_t> data;
		public:
			/**
			 * @brief Метод чтения октетов контейнера
			 *
			 * @param offset смещение вычитываемых октетов
			 * @param size   размер вычитываемых октетов
			 * @param result буфер, куда следует положить вычитанные октеты
			 * @return       признак успешного чтения октетов
			 *
			 */
			bool read(const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept {
				// Если затребованные октеты за концом лежащих на носителе
				if((offset + size) > static_cast <uint64_t> (this->data.size()))
					// Выводим признак неудачного чтения октетов
					return false;
				// Выполняем выдачу затребованных октетов
				result.assign(this->data.begin() + static_cast <ptrdiff_t> (offset),
				 this->data.begin() + static_cast <ptrdiff_t> (offset + size));
				// Выводим признак успешного чтения октетов
				return true;
			}
			/**
			 * @brief Метод записи октетов контейнера
			 *
			 * @param offset смещение записываемых октетов
			 * @param buffer буфер записываемых октетов
			 * @param size   размер записываемых октетов
			 * @return       признак успешной записи октетов
			 *
			 */
			bool write(const uint64_t offset, const void * buffer, const size_t size) noexcept {
				// Если записываемые октеты за концом лежащих на носителе
				if((offset + size) > static_cast <uint64_t> (this->data.size()))
					// Выполняем расширение носителя под записываемые октеты
					this->data.resize(static_cast <size_t> (offset + size), 0);
				// Выполняем запись поданных октетов на носитель
				::memcpy(this->data.data() + offset, buffer, size);
				// Выводим признак успешной записи октетов
				return true;
			}
	};
	/**
	 * @brief Функция прогона контейнера целиком
	 *
	 * @param crypto     модуль шифрования, отданный потребителем
	 * @param compressor модуль сжатия, отданный потребителем
	 * @param records    записи, вносимые в собираемый контейнер
	 *
	 */
	void container(crypto_t & crypto, compressor::block_t & compressor,
	 const vector <vector <uint8_t>> & records) noexcept {
		// Сборщик контейнера
		abc::assembler_t assembler;
		// Выполняем установку модуля сжатия сборщику контейнера
		assembler.compressor(& compressor);
		// Выполняем получение признака шифрования содержимого кадров
		const bool encrypt = (number(0, 2) == 0);
		// Выполняем получение признака подписи собираемого контейнера
		const bool signing = (number(0, 1) == 1);
		/**
		 * Если содержимое кадров шифруется
		 */
		if(encrypt){
			// Выполняем установку модуля шифрования сборщику контейнера
			assembler.crypto(& crypto);
			// Получаем настройки укладки кадра
			abc::packer_t::settings_t packing = assembler.packer().settings();
			// Выполняем установку признака шифрования содержимого кадра
			packing.encrypt = true;
			// Выполняем установку настроек укладки кадра
			assembler.packer().settings(packing);
		}
		/**
		 * Если контейнер подписывается
		 */
		if(signing)
			// Выполняем объявление подписи собираемого контейнера
			assembler.sign(& crypto, "владелец");
		// Получаем настройки сборки контейнера
		abc::assembler_t::settings_t settings = assembler.settings();
		// Выполняем установку порога накопления записей
		settings.block = static_cast <size_t> (number(1, 512));
		// Выполняем установку настроек сборки контейнера
		assembler.settings(settings);
		/**
		 * Выполняем внесение всех записей в собираемый контейнер
		 */
		for(const vector <uint8_t> & item : records){
			// Выполняем получение вида содержимого вносимой записи
			const abc::payload_t kind = static_cast <abc::payload_t> (number(0, 3));
			/**
			 * Если внести очередную запись не вышло
			 */
			if(!assembler.append(item.data(), item.size(), kind))
				// Прекращаем сборку контейнера
				return;
		}
		// Носитель, несущий собранный контейнер
		Medium medium;
		/**
		 * Если завершить сборку контейнера не вышло
		 */
		if(!assembler.complete(medium.data))
			// Прекращаем сборку контейнера
			return;
		// Выполняем увеличение количества собранных контейнеров
		totals.containers++;
		// Код отказа поверки подписи владельца
		abc::error_t error = abc::error_t::NONE;
		/**
		 * Если контейнер подписан, выполняем поверку подписи владельца
		 */
		if(signing){
			/**
			 * Если подпись владельца не сошлась
			 */
			if(!abc::verify(crypto, "владелец", medium.data.data(), medium.data.size(), error)){
				// Выводим сообщение о несошедшейся подписи владельца
				::fprintf(stderr, "abc fuzz: signature of the freshly built container does not agree: %s\n",
				 abc::message(error));
				// Выполняем выход с признаком расхождения
				::exit(1);
			}
			// Выполняем увеличение количества сошедшихся поверок подписи
			totals.verified++;
		}
		// Выборщик записей контейнера
		abc::fetcher_t fetcher;
		// Выполняем установку модуля сжатия выборщику записей
		fetcher.compressor(& compressor);
		// Выполняем установку модуля шифрования выборщику записей
		fetcher.crypto(& crypto);
		/**
		 * Если открыть контейнер выборщиком записей вышло
		 */
		if(fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем чтение затребованных октетов контейнера
			return medium.read(offset, size, result);
		})){
			/**
			 * Выполняем выборку всех записей контейнера вразнобой
			 */
			for(uint64_t i = 0; i < fetcher.records(); i++){
				// Буфер выбранной записи контейнера
				vector <uint8_t> picked;
				// Выполняем получение номера выбираемой записи
				const uint64_t index = number(0, fetcher.records() - 1);
				/**
				 * Если выбрать запись контейнера не вышло
				 */
				if(!fetcher.record(index, picked))
					// Переходим к следующей выбираемой записи
					continue;
				// Выполняем увеличение количества выборок записи
				totals.fetches++;
				/**
				 * Если выбранная запись разошлась с внесённой
				 */
				if(picked != records.at(static_cast <size_t> (index))){
					// Выводим сообщение о расхождении выбранной записи
					::fprintf(stderr, "abc fuzz: fetched record %llu differs from the appended one\n",
					 static_cast <unsigned long long> (index));
					// Выполняем выход с признаком расхождения
					::exit(1);
				}
			}
		}
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем установку модуля сжатия правщику контейнера
		editor.compressor(& compressor);
		// Выполняем установку модуля шифрования правщику контейнера
		editor.crypto(& crypto);
		/**
		 * Если открыть контейнер правщиком не вышло
		 */
		if(!editor.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем чтение затребованных октетов контейнера
			return medium.read(offset, size, result);
		}, [&medium](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов контейнера
			return medium.write(offset, buffer, size);
		}, static_cast <uint64_t> (medium.data.size())))
			// Прекращаем правку контейнера
			return;
		/**
		 * Если контейнер подписан, выполняем объявление подписи правимого контейнера
		 */
		if(signing && !editor.sign(& crypto, "владелец"))
			// Прекращаем правку контейнера
			return;
		/**
		 * Выполняем несколько правок контейнера подряд
		 */
		for(uint64_t i = 0; i < number(1, 4); i++){
			// Выполняем получение вида правки контейнера
			const uint64_t kind = number(0, 2);
			// Построенная запись правки контейнера
			vector <uint8_t> item;
			// Если запись правки построить не вышло
			if(!assemble(item))
				// Прекращаем правку контейнера
				break;
			// Если запись дописывается в конец контейнера
			if(kind == 0)
				// Выполняем дописывание записи в конец контейнера
				(void) editor.append(item.data(), item.size(), abc::payload_t::MIXED);
			// Иначе, если запись контейнера правится
			else if(kind == 1)
				// Выполняем правку записи контейнера по номеру
				(void) editor.replace(number(0, (editor.records() > 0 ? editor.records() - 1 : 0)),
				 item.data(), item.size(), abc::payload_t::MIXED);
			// Иначе выполняем снос записи контейнера по номеру
			else (void) editor.erase(number(0, (editor.records() > 0 ? editor.records() - 1 : 0)));
			// Выполняем увеличение количества правок контейнера
			totals.edits++;
		}
		/**
		 * Если зафиксировать накопленные правки не вышло
		 */
		if(!editor.commit())
			// Прекращаем правку контейнера
			return;
		/**
		 * Если контейнер подписан, выполняем поверку подписи после правки
		 */
		if(signing){
			/**
			 * Если подпись владельца не сошлась
			 */
			if(!abc::verify(crypto, "владелец", medium.data.data(), medium.data.size(), error)){
				// Выводим сообщение о несошедшейся подписи правленного контейнера
				::fprintf(stderr, "abc fuzz: signature of the edited container does not agree: %s\n",
				 abc::message(error));
				// Выполняем выход с признаком расхождения
				::exit(1);
			}
			// Выполняем увеличение количества сошедшихся поверок подписи
			totals.verified++;
		}
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Полная длина убранного контейнера
		uint64_t length = 0;
		/**
		 * Если убрать мусор перестройкой контейнера вышло
		 */
		if(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::MIXED, length)){
			// Выполняем увеличение количества уборок мусора
			totals.compactions++;
			/**
			 * Если полная длина убранного контейнера разошлась с записанной
			 */
			if(length != static_cast <uint64_t> (cleaned.data.size())){
				// Выводим сообщение о расхождении длины убранного контейнера
				::fprintf(stderr, "abc fuzz: compacted container length differs from the written one\n");
				// Выполняем выход с признаком расхождения
				::exit(1);
			}
		}
	}
	/**
	 * @brief Функция прогона порченого контейнера
	 *
	 * @param crypto     модуль шифрования, отданный потребителем
	 * @param compressor модуль сжатия, отданный потребителем
	 * @param buffer     октеты порченого контейнера
	 *
	 */
	void broken(crypto_t & crypto, compressor::block_t & compressor, vector <uint8_t> buffer) noexcept {
		// Выполняем порчу октетов контейнера
		damage(buffer);
		// Носитель, несущий порченый контейнер
		Medium medium;
		// Выполняем перенесение порченых октетов носителю
		medium.data = buffer;
		// Сниматель контейнера
		abc::loader_t loader;
		// Выполняем установку модуля сжатия снимателю контейнера
		loader.compressor(& compressor);
		// Выполняем установку модуля шифрования снимателю контейнера
		loader.crypto(& crypto);
		// Выполняем подачу порченого контейнера снимателю
		(void) loader.feed(buffer.data(), buffer.size());
		// Содержимое очередного снятого кадра
		vector <uint8_t> payload;
		// Сведения об очередном снятом кадре
		abc::chunk_t chunk;
		/**
		 * Выполняем вычитывание всех кадров порченого контейнера
		 */
		while(loader.next(payload, chunk)){
			// Выполняем разбор содержимого снятого кадра
			abc::value_t value;
			// Выполняем разбор содержимого кадра во владеющее значение
			(void) value.parse(payload.data(), payload.size());
		}
		// Выборщик записей порченого контейнера
		abc::fetcher_t fetcher;
		// Выполняем установку модуля сжатия выборщику записей
		fetcher.compressor(& compressor);
		// Выполняем установку модуля шифрования выборщику записей
		fetcher.crypto(& crypto);
		/**
		 * Если открыть порченый контейнер выборщиком записей вышло
		 */
		if(fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем чтение затребованных октетов контейнера
			return medium.read(offset, size, result);
		})){
			/**
			 * Выполняем выборку записей порченого контейнера
			 */
			for(uint64_t i = 0; i < fetcher.records(); i++){
				// Буфер выбранной записи контейнера
				vector <uint8_t> picked;
				// Выполняем выборку очередной записи контейнера
				(void) fetcher.record(i, picked);
			}
		}
		// Код отказа поверки подписи владельца
		abc::error_t error = abc::error_t::NONE;
		// Выполняем поверку подписи порченого контейнера
		(void) abc::verify(crypto, "владелец", buffer.data(), buffer.size(), error);
	}
};

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Количество выполняемых итераций
	uint64_t count = 3000;
	/**
	 * Если количество итераций подано параметром
	 */
	if(argc > 1)
		// Выполняем получение количества выполняемых итераций
		count = static_cast <uint64_t> (::strtoull(argv[1], nullptr, 10));
	// Объект фреймворка
	fmk_t fmk;
	// Объект журнала
	log_t log(& fmk);
	// Выполняем снятие вывода журнала: ворошитель шумит и без него
	log.level(log_t::level_t::NONE);
	// Объект сжатия данных
	compressor::block_t compressor(& log);
	// Объект шифрования данных
	crypto_t crypto(& fmk, & log);
	// Выполняем установку соли шифрования
	crypto.salt("соль ворошителя");
	// Выполняем установку пароля шифрования
	crypto.password("пароль ворошителя");
	/**
	 * Если завести ключ владельца контейнера не вышло
	 */
	if(!crypto.generateKey("владелец", crypto_t::signature_t::ED25519)){
		// Выводим сообщение о неудачном заведении ключа владельца
		::fprintf(stderr, "abc fuzz: key of the owner cannot be generated\n");
		// Выполняем выход с признаком отказа
		return 1;
	}
	/**
	 * Выполняем все затребованные итерации ворошения
	 */
	for(uint64_t i = 0; i < count; i++){
		// Записи, вносимые в собираемый контейнер
		vector <vector <uint8_t>> records;
		// Выполняем получение количества строимых записей
		const uint64_t amount = number(1, 8);
		/**
		 * Выполняем построение всех записей итерации
		 */
		for(uint64_t j = 0; j < amount; j++){
			// Построенная запись контейнера
			vector <uint8_t> item;
			/**
			 * Если запись построить не вышло
			 */
			if(!assemble(item))
				// Переходим к следующей строимой записи
				continue;
			// Выполняем сличение разбора записи при разной нарезке на куски
			slicing(item);
			// Выполняем сборку дерева документа и перезапись его
			tree(item);
			// Выполняем запоминание построенной записи
			records.push_back(item);
			// Порченая запись контейнера
			vector <uint8_t> spoiled = item;
			// Выполняем порчу построенной записи
			damage(spoiled);
			/**
			 * Выполняем разбор порченой записи: разбор её вправе отвечать отказом,
			 * но не вправе выходить за буфер и рушить работу
			 */
			slicing(spoiled);
			// Выполняем сборку дерева документа из порченой записи
			tree(spoiled);
		}
		/**
		 * Если записи итерации построены
		 */
		if(!records.empty()){
			// Выполняем прогон контейнера целиком
			container(crypto, compressor, records);
			// Собираемый контейнер для порчи
			abc::assembler_t assembler;
			// Выполняем объявление подписи собираемого контейнера
			assembler.sign(& crypto, "владелец");
			// Выполняем перебор всех записей итерации
			for(const vector <uint8_t> & item : records)
				// Выполняем внесение очередной записи в собираемый контейнер
				(void) assembler.append(item.data(), item.size(), abc::payload_t::TEXT);
			// Буфер собранного контейнера
			vector <uint8_t> buffer;
			/**
			 * Если завершить сборку контейнера вышло
			 */
			if(assembler.complete(buffer))
				// Выполняем прогон порченого контейнера
				broken(crypto, compressor, buffer);
		}
	}
	// Выводим учёт проделанной работы
	::printf("abc fuzz: %llu records (%llu corrupted), %llu parsed to the end, %llu events, "
	 "%llu trees, %llu containers, %llu fetches, %llu edits, %llu compactions, %llu signatures verified, "
	 "%llu values assembled\n",
	 static_cast <unsigned long long> (totals.records), static_cast <unsigned long long> (totals.corrupted),
	 static_cast <unsigned long long> (totals.survived), static_cast <unsigned long long> (totals.events),
	 static_cast <unsigned long long> (totals.trees), static_cast <unsigned long long> (totals.containers),
	 static_cast <unsigned long long> (totals.fetches), static_cast <unsigned long long> (totals.edits),
	 static_cast <unsigned long long> (totals.compactions), static_cast <unsigned long long> (totals.verified),
	 static_cast <unsigned long long> (totals.assemblies));
	// Выводим успешное завершение работы
	return 0;
}
