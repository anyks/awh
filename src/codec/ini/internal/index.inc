/**
 * @file index.inc
 * @date 2026-11-05
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
 * @brief Внутренние тела указателей разделов и свойств — открытый хэш с
 *        ключом, слотом владеемым, общий для дерева настроек и владеющего
 *        значения
 *
 * @details Файл подключается реализациями модуля и в поставляемый набор не входит:
 *          под include/ собраны только интерфейсные заголовки. Тело метода
 *          обязано быть видно в каждой единице трансляции, зовущей метод, - иначе
 *          встраивание не работает, - а общее тело в одном месте избавляет от
 *          копий по числу зовущих файлов.
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/ini/common.hpp>

/**
 * Пространство имён фреймворка
 */
namespace awh {
	/**
	 * Пространство имён модуля кодеков
	 */
	namespace codec {
		/**
		 * Пространство имён модуля кодека текста настроек
		 */
		namespace ini {
			/**
			 * @brief Наименьшее количество слотов таблицы указателя
			 *
			 * @details Пересборка таблицы ведётся удвоением, и наименьшее количество слотов
			 *          задаёт лишь начало этого ряда. Предел намеренно мал: первую таблицу
			 *          платит всякое дерево настроек, и крупный пол обернулся бы десятками
			 *          килобайтов выделения и зануления на файл из трёх разделов, каких в
			 *          обиходе большинство. Замер 06.11.2026 парой одного окна: сборка дерева
			 *          настроек службы при поле в шестнадцать слотов быстрее в 1.29 раза.
			 *
			 */
			constexpr size_t SLOTS_MINIMUM = 16;

			/**
			 * @brief Функция смешивания составного ключа разделов
			 *
			 * @details Ключ читается кусками по восемь знаков виртуальной склейки
			 *          «первая часть, нулевой байт, вторая часть»; всякий знак
			 *          приводится к нижнему регистру на лету там, где это предписано
			 *          флагом части. Склейка в строку не материализуется: смешивание
			 *          идёт посимвольно, и временного буфера не требует.
			 *
			 * @param first  первая часть ключа
			 * @param lowerF признак приведения первой части к нижнему регистру
			 * @param second вторая часть ключа
			 * @param lowerS признак приведения второй части к нижнему регистру
			 * @return       смешанное значение ключа
			 *
			 */
			static uint64_t mixed(const string_view first, const bool lowerF, const string_view second, const bool lowerS) noexcept {
				// Смешиваемое значение ключа
				uint64_t hash = 0x9E3779B97F4A7C15ull;
				// Длина первой части составного ключа
				const size_t firstSize = first.length();
				// Полная длина виртуальной склейки ключа
				const size_t total = (firstSize + 1 + second.length());
				// Собираемый кусок ключа
				uint64_t block = 0;
				// Смещение очередного знака внутри куска
				size_t shift = 0;
				// Смещение начала очередного куска
				size_t offset = 0;
				/**
				 * Выполняем перебор знаков виртуальной склейки ключа
				 */
				for(size_t i = 0; i < total; i++){
					// Очередной знак виртуальной склейки ключа
					char byte = '\0';
					/**
					 * Если знак принадлежит первой части ключа
					 */
					if(i < firstSize)
						// Читаем знак первой части с приведением по флагу
						byte = (lowerF ? ascii::toLower(first[i]) : first[i]);
					// Если знак принадлежит второй части ключа
					else if(i > firstSize)
						// Читаем знак второй части с приведением по флагу
						byte = (lowerS ? ascii::toLower(second[i - firstSize - 1]) : second[i - firstSize - 1]);
					// Выполняем подмешивание знака к собираемому куску
					block |= (static_cast <uint64_t> (static_cast <unsigned char> (byte)) << (8 * shift));
					// Выполняем переход к следующему знаку внутри куска
					shift++;
					/**
					 * Если кусок ключа заполнен
					 */
					if(shift == sizeof(uint64_t)){
						// Выполняем подмешивание куска со смещением его места
						hash ^= (block + offset + 0x9E3779B97F4A7C15ull);
						// Выполняем умножение смешиваемого значения
						hash *= 0xFF51AFD7ED558CCDull;
						// Выполняем поворот смешиваемого значения
						hash = ((hash << 29) | (hash >> 35));
						// Выполняем обнуление собираемого куска
						block = 0;
						// Выполняем обнуление смещения внутри куска
						shift = 0;
						// Запоминаем смещение начала следующего куска
						offset = (i + 1);
					}
				}
				/**
				 * Если хвост ключа короче куска остаётся
				 */
				if(shift > 0){
					// Выполняем подмешивание хвоста с длиною всего ключа
					hash ^= (block + total);
					// Выполняем умножение смешиваемого значения
					hash *= 0xFF51AFD7ED558CCDull;
				}
				// Выполняем замешивание значения напоследок
				hash ^= (hash >> 30);
				// Выполняем умножение смешиваемого значения
				hash *= 0xBF58476D1CE4E5B9ull;
				// Выполняем замешивание значения напоследок
				hash ^= (hash >> 27);
				// Выполняем умножение смешиваемого значения
				hash *= 0x94D049BB133111EBull;
				// Выполняем замешивание значения напоследок
				hash ^= (hash >> 31);
				// Выводим смешанное значение ключа
				return hash;
			}
			/**
			 * @brief Функция смешивания составного ключа свойств
			 *
			 * @details Ключ составлен порядковым номером раздела и именем свойства;
			 *          смешивание идёт именем посимвольно с приведением регистра по
			 *          флагу, а номер раздела подмешивается множителем.
			 *
			 * @param section порядковый номер раздела
			 * @param key     имя свойства
			 * @param lower   признак приведения имени к нижнему регистру
			 * @return        смешанное значение ключа
			 *
			 */
			static uint64_t mixed(const uint32_t section, const string_view key, const bool lower) noexcept {
				// Смешиваемое значение ключа
				uint64_t hash = 0x9E3779B97F4A7C15ull;
				// Собираемый кусок ключа
				uint64_t block = 0;
				// Смещение очередного знака внутри куска
				size_t shift = 0;
				// Смещение начала очередного куска
				size_t offset = 0;
				/**
				 * Выполняем перебор знаков имени свойства
				 */
				for(size_t i = 0; i < key.length(); i++){
					// Выполняем подмешивание знака имени к собираемому куску
					block |= (static_cast <uint64_t> (static_cast <unsigned char> (lower ? ascii::toLower(key[i]) : key[i])) << (8 * shift));
					// Выполняем переход к следующему знаку внутри куска
					shift++;
					/**
					 * Если кусок ключа заполнен
					 */
					if(shift == sizeof(uint64_t)){
						// Выполняем подмешивание куска со смещением его места
						hash ^= (block + offset + 0x9E3779B97F4A7C15ull);
						// Выполняем умножение смешиваемого значения
						hash *= 0xFF51AFD7ED558CCDull;
						// Выполняем поворот смешиваемого значения
						hash = ((hash << 29) | (hash >> 35));
						// Выполняем обнуление собираемого куска
						block = 0;
						// Выполняем обнуление смещения внутри куска
						shift = 0;
						// Запоминаем смещение начала следующего куска
						offset = (i + 1);
					}
				}
				/**
				 * Если хвост имени короче куска остаётся
				 */
				if(shift > 0){
					// Выполняем подмешивание хвоста с длиною всего имени
					hash ^= (block + key.length());
					// Выполняем умножение смешиваемого значения
					hash *= 0xFF51AFD7ED558CCDull;
				}
				// Выполняем подмешивание порядкового номера раздела
				hash ^= (static_cast <uint64_t> (section) * 0x9E3779B97F4A7C15ull);
				// Выполняем умножение смешиваемого значения
				hash *= 0xFF51AFD7ED558CCDull;
				// Выполняем замешивание значения напоследок
				hash ^= (hash >> 30);
				// Выполняем умножение смешиваемого значения
				hash *= 0xBF58476D1CE4E5B9ull;
				// Выполняем замешивание значения напоследок
				hash ^= (hash >> 27);
				// Выполняем умножение смешиваемого значения
				hash *= 0x94D049BB133111EBull;
				// Выполняем замешивание значения напоследок
				hash ^= (hash >> 31);
				// Выводим смешанное значение ключа
				return hash;
			}
			/**
			 * @brief Функция сличения тела слота разделов с разыскиваемым именем
			 *
			 * @param body   тело ключа слота, нормализованного
			 * @param length полная длина тела ключа слота
			 * @param first  размер первой части тела ключа слота
			 * @param source первая часть разыскиваемого имени
			 * @param lowerF признак приведения первой части к нижнему регистру
			 * @param target вторая часть разыскиваемого имени
			 * @param lowerS признак приведения второй части к нижнему регистру
			 * @return       признак равенства тела слота разыскиваемому имени
			 *
			 */
			static bool matched(const char * body, const size_t length, const size_t first,
				const string_view source, const bool lowerF, const string_view target, const bool lowerS) noexcept {
				// Длина виртуальной склейки разыскиваемого имени
				const size_t total = (source.length() + 1 + target.length());
				/**
				 * Если длина слота разыскиваемому имени не отвечает
				 */
				if((length != total) || (first != source.length()))
					// Выводим признак неравенства
					return false;
				/**
				 * Выполняем перебор знаков виртуальной склейки
				 */
				for(size_t i = 0; i < total; i++){
					// Очередной знак разыскиваемого имени
					char byte = '\0';
					/**
					 * Если знак принадлежит первой части имени
					 */
					if(i < source.length())
						// Читаем знак первой части с приведением по флагу
						byte = (lowerF ? ascii::toLower(source[i]) : source[i]);
					// Если знак принадлежит второй части имени
					else if(i > source.length())
						// Читаем знак второй части с приведением по флагу
						byte = (lowerS ? ascii::toLower(target[i - source.length() - 1]) : target[i - source.length() - 1]);
					/**
					 * Если знак тела слота разыскиваемому не равен
					 */
					if(body[i] != byte)
						// Выводим признак неравенства
						return false;
				}
				// Выводим признак равенства
				return true;
			}
			/**
			 * @brief Функция сличения тела слота свойств с разыскиваемым именем
			 *
			 * @param body   тело ключа слота, нормализованного
			 * @param length длина тела ключа слота
			 * @param source разыскиваемое имя свойства
			 * @param lower  признак приведения имени к нижнему регистру
			 * @return       признак равенства тела слота разыскиваемому имени
			 *
			 */
			static bool matched(const char * body, const size_t length, const string_view source, const bool lower) noexcept {
				/**
				 * Если длина слота разыскиваемому имени не отвечает
				 */
				if(length != source.length())
					// Выводим признак неравенства
					return false;
				/**
				 * Выполняем перебор знаков разыскиваемого имени
				 */
				for(size_t i = 0; i < length; i++)
					/**
					 * Если знак тела слота разыскиваемому не равен
					 */
					if(body[i] != (lower ? ascii::toLower(source[i]) : source[i]))
						// Выводим признак неравенства
						return false;
				// Выводим признак равенства
				return true;
			}

			/**
			 * @brief Класс слота указателя разделов
			 *
			 * @details Ключ до 40 знаков лежит в самом слоте, рядом со значением, и сличение
			 *          при поиске не ходит в чужую память. Ключ длиннее слотом не вмещается,
			 *          переносится хранилищем слота, и слот держит указатель на него.
			 *
			 */
			class Index::Slot {
				public:
					/**
					 * @brief Наибольшая длина ключа, слотом вмещаемая
					 *
					 */
					static constexpr uint16_t EMBEDDED = 40;
				public:
					// Признак занятости слота
					bool occupied;
					// Размер первой части тела ключа слота
					uint16_t first;
					// Полная длина тела ключа слота
					uint16_t length;
					// Порядковый номер раздела, слотом разыскиваемый
					uint32_t value;
				public:
					/**
					 * Ключ слота
					 *
					 * @note Читается тело слота, покуда длина ключа вмещаема, и указатель на
					 *       перенесённый ключ - свыше того
					 *
					 */
					union {
						// Тело слота, короткий ключ держащее
						char body[EMBEDDED];
						// Начало ключа, хранилищем перенесённого
						char * owned;
					} key;
					/**
					 * @brief Метод получения начала тела ключа слота
					 *
					 * @return адрес начала тела ключа слота
					 *
					 */
					const char * data() const noexcept {
						// Выводим начало тела ключа, слотом хранимого
						return ((this->length <= EMBEDDED) ? this->key.body : this->key.owned);
					}
					/**
					 * @brief Метод освобождения перенесённого тела ключа слота
					 *
					 */
					void release() noexcept {
						/**
						* Если слот занят и ключ его перенесён
						*/
						if(this->occupied && (this->length > EMBEDDED))
							// Выполняем освобождение перенесённого ключа
							delete[] this->key.owned;
					}
				public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Slot() noexcept : occupied(false), first(0), length(0), value(0) {}
			};
			/**
			 * @brief Класс слота указателя свойств
			 *
			 * @details Перечень объявлений до четырёх записей лежит в самом слоте, свыше
			 *          того переносится хранилищем слота. Ключ до 32 знаков лежит в самом
			 *          слоте, свыше того переносится хранилищем слота. Слот снятого
			 *          свойства помечается могильником: проба линейная, и выброшенный слот
			 *          оборвал бы цепочку розыска стоящих за ним.
			 *
			 */
			class Records::Slot {
				public:
					/**
					 * @brief Состояние свободы слота
					 *
					 */
					static constexpr uint8_t FREE = 0;
					/**
					 * @brief Состояние занятости слота
					 *
					 */
					static constexpr uint8_t USED = 1;
					/**
					 * @brief Состояние могильника слота
					 *
					 */
					static constexpr uint8_t DEAD = 2;
					/**
					 * @brief Наибольшее число записей, слотом вмещаемое
					 *
					 */
					static constexpr uint8_t INLINE = 4;
					/**
					 * @brief Наибольшая длина ключа, слотом вмещаемая
					 *
					 */
					static constexpr uint16_t EMBEDDED = 32;
				public:
					// Состояние слота
					uint8_t state;
					// Количество записей перечня объявлений
					uint8_t count;
					// Длина тела ключа слота
					uint16_t length;
					// Порядковый номер раздела, которому свойство принадлежит
					uint32_t section;
				public:
					/**
					 * Перечень объявлений слота
					 *
					 * @note Читается перечень слота, покуда записей не более предела,
					 *       и указатель на перенесённый перечень - свыше того
					 *
					 */
					union {
						// Перечень объявлений, слотом держимый
						uint32_t local[INLINE];
						// Начало перечня объявлений, хранилищем перенесённого
						uint32_t * owned;
					} items;
					/**
					 * Ключ слота
					 *
					 * @note Читается тело слота, покуда длина ключа вмещаема, и указатель на
					 *       перенесённый ключ - свыше того
					 *
					 */
					union {
						// Тело слота, короткий ключ держащее
						char body[EMBEDDED];
						// Начало ключа, хранилищем перенесённого
						char * owned;
					} key;
				public:
					/**
					 * @brief Метод получения начала тела ключа слота
					 *
					 * @return адрес начала тела ключа слота
					 *
					 */
					const char * data() const noexcept {
						// Выводим начало тела ключа, слотом хранимого
						return ((this->length <= EMBEDDED) ? this->key.body : this->key.owned);
					}
					/**
					 * @brief Метод получения начала перечня объявлений слота
					 *
					 * @return адрес первой записи перечня объявлений слота
					 *
					 */
					const uint32_t * itemsData() const noexcept {
						// Выводим начало перечня объявлений, слотом хранимого
						return ((this->count <= INLINE) ? this->items.local : this->items.owned);
					}
				public:
					/**
					 * @brief Метод освобождения перенесённых данных слота
					 *
					 */
					void release() noexcept {
						/**
						 * Если слот занят
						 */
						if(this->state == USED){
							/**
							 * Если тело ключа перенесено
							 */
							if(this->length > EMBEDDED)
								// Выполняем освобождение перенесённого ключа
								delete[] this->key.owned;
							/**
							 * Если перечень объявлений перенесён
							 */
							if(this->count > INLINE)
								// Выполняем освобождение перенесённого перечня
								delete[] this->items.owned;
						}
					}
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Slot() noexcept : state(FREE), count(0), length(0), section(0) {}
			};
			/**
			 * @brief Очистка указателя разделов
			 *
			 */
			void Index::clear() noexcept {
				/**
				 * Выполняем перебор всех слотов указателя разделов
				 */
				for(size_t i = 0; i < this->capacity; i++)
					// Выполняем освобождение перенесённых данных очередного слота
					this->slots[i].release();
				/**
				 * Если таблица слотов заведена
				 */
				if(this->slots != nullptr)
					// Выполняем опустошение слотов таблицы
					::memset(this->slots, 0, (this->capacity * sizeof(Slot)));
				// Выполняем сброс занятости указателя разделов
				this->occupied = 0;
			}
			/**
			 * @brief Метод пересборки таблицы слотов указателя разделов
			 *
			 * @param wanted требуемое количество слотов
			 *
			 */
			void Index::rehash(const size_t wanted) noexcept {
				// Количество слотов пересобранной таблицы
				size_t capacity = SLOTS_MINIMUM;
				/**
				 * Выполняем подбор количества слотов степенью двойки
				 */
				while(capacity < wanted)
					// Выполняем удвоение количества слотов
					capacity <<= 1;
				// Слоты пересобранной таблицы
				Slot * slots = new Slot[capacity];
				// Выполняем опустошение слотов пересобранной таблицы
				::memset(slots, 0, (capacity * sizeof(Slot)));
				// Количество занятых слотов пересобранной таблицы
				size_t occupied = 0;
				/**
				 * Выполняем перенос занятых слотов прежней таблицы
				 */
				for(size_t i = 0; i < this->capacity; i++){
					// Очередной слот прежней таблицы
					Slot & source = this->slots[i];
					/**
					 * Если очередной слот прежней таблицы пуст
					 */
					if(!source.occupied)
						// Выполняем пропуск пустого слота
						continue;
					// Смешанное значение ключа переносимого слота
					const size_t hash = mixed(string_view(source.data(), source.first), false,
						string_view(source.data() + source.first + 1, source.length - source.first - 1), false);
					// Положение очередного слота пробы пересобранной таблицы
					size_t position = (hash & (capacity - 1));
					/**
					 * Выполняем розыск свободного слота пересобранной таблицы
					 */
					while(slots[position].occupied)
						// Выполняем переход к соседнему слоту пробы
						position = ((position + 1) & (capacity - 1));
					// Выполняем перенос очередного слота пересобранной таблицей
					slots[position] = source;
					// Выполняем учёт занятого слота
					occupied++;
				}
				// Выполняем освобождение слотов прежней таблицы
				delete[] this->slots;
				// Выполняем принятие слотов пересобранной таблицы
				this->slots = slots;
				// Выполняем принятие количества слотов пересобранной таблицы
				this->capacity = capacity;
				// Выполняем принятие занятости пересобранной таблицы
				this->occupied = occupied;
			}
			/**
			 * @brief Метод розыска раздела по частям имени
			 *
			 * @param first   имя разыскиваемого раздела
			 * @param lowerF  признак приведения имени раздела к нижнему регистру
			 * @param second  имя разыскиваемого подраздела
			 * @param lowerS  признак приведения имени подраздела к нижнему регистру
			 * @return        порядковый номер найденного раздела либо обозначение отсутствующего
			 *
			 */
			uint32_t Index::locate(const string_view first, const bool lowerF, const string_view second, const bool lowerS) const noexcept {
				/**
				 * Если таблица слотов не заведена
				 */
				if(this->capacity == 0)
					// Выводим обозначение отсутствующего раздела
					return NO_RECORD;
				// Положение очередного слота пробы
				size_t cursor = (mixed(first, lowerF, second, lowerS) & (this->capacity - 1));
				/**
				 * Выполняем перебор слотов пробы
				 */
				while(this->slots[cursor].occupied){
					// Очередной слот пробы
					const Slot & slot = this->slots[cursor];
					/**
					 * Если тело слота разыскиваемому имени равно
					 */
					if(matched(slot.data(), slot.length, slot.first, first, lowerF, second, lowerS))
						// Выводим порядковый номер найденного раздела
						return slot.value;
					// Выполняем переход к соседнему слоту пробы
					cursor = ((cursor + 1) & (this->capacity - 1));
				}
				// Выводим обозначение отсутствующего раздела
				return NO_RECORD;
			}
			/**
			 * @brief Метод установки раздела частями имени
			 *
			 * @param first   имя устанавливаемого раздела
			 * @param lowerF  признак приведения имени раздела к нижнему регистру
			 * @param second  имя устанавливаемого подраздела
			 * @param lowerS  признак приведения имени подраздела к нижнему регистру
			 * @param value   порядковый номер устанавливаемого раздела
			 * @return        признак того, что раздел установлен этой установкой
			 *
			 */
			bool Index::emplace(const string_view first, const bool lowerF, const string_view second, const bool lowerS, const uint32_t value) noexcept {
				/**
				 * Если занятость таблицы подошла к половине её слотов
				 *
				 * @note Половина - предел пробе линейной: свыше его средняя проба растёт
				 *       лавинно, и розыск перестаёт быть розыском по соседним слотам
				 *
				 */
				if((this->occupied * 2) >= this->capacity)
					// Выполняем пересборку таблицы слотов удвоением
					this->rehash((this->capacity > 0) ? (this->capacity * 2) : SLOTS_MINIMUM);
				// Полная длина тела устанавливаемого ключа
				const size_t total = (first.length() + 1 + second.length());
				// Положение очередного слота пробы
				size_t cursor = (mixed(first, lowerF, second, lowerS) & (this->capacity - 1));
				/**
				 * Выполняем перебор слотов пробы
				 */
				while(this->slots[cursor].occupied){
					// Очередной слот пробы
					const Slot & slot = this->slots[cursor];
					/**
					 * Если тело слота устанавливаемому имени равно
					 */
					if(matched(slot.data(), slot.length, slot.first, first, lowerF, second, lowerS))
						// Выводим признак того, что раздел установлен не был
						return false;
					// Выполняем переход к соседнему слоту пробы
					cursor = ((cursor + 1) & (this->capacity - 1));
				}
				// Слот, именем занимаемый
				Slot & slot = this->slots[cursor];
				// Выполняем занятие слота пробы
				slot.occupied = true;
				// Выполняем запоминание длины тела ключа
				slot.length = static_cast <uint16_t> (total);
				// Выполняем запоминание размера первой части тела ключа
				slot.first = static_cast <uint16_t> (first.length());
				// Выполняем запоминание порядкового номера раздела
				slot.value = value;
				/**
				 * Если тело ключа слотом не вмещается
				 */
				if(total > Slot::EMBEDDED)
					// Выполняем принятие хранилища перенесённого ключа
					slot.key.owned = new char[total];
				// Выполняем перенос первой части ключа в тело слота
				const size_t firstSize = first.length();
				/**
				 * Выполняем перебор знаков первой части ключа
				 */
				for(size_t i = 0; i < firstSize; i++)
					// Выполняем перенос очередного знака с приведением по флагу
					const_cast <char *>(slot.data())[i] = (lowerF ? ascii::toLower(first[i]) : first[i]);
				// Выполняем перенос разделителя частей ключа
				const_cast <char *>(slot.data())[firstSize] = '\0';
				/**
				 * Выполняем перебор знаков второй части ключа
				 */
				for(size_t i = 0; i < second.length(); i++)
					// Выполняем перенос очередного знака с приведением по флагу
					const_cast <char *>(slot.data())[firstSize + 1 + i] = (lowerS ? ascii::toLower(second[i]) : second[i]);
				// Выполняем учёт занятого слота
				this->occupied++;
				// Выводим признак того, что раздел установлен
				return true;
			}
			/**
			 * @brief Оператор присваивания перемещением
			 *
			 * @param index перемещаемый указатель разделов
			 * @return      приёмник присваивания
			 *
			 */
			Index & Index::operator = (Index && index) noexcept {
				/**
				 * Если приёмник присваивания и его источник есть один указатель
				 */
				if(this != &index){
					// Выполняем очистку приёмника присваивания
					this->clear();
					// Выполняем освобождение слотов приёмника присваивания
					delete[] this->slots;
					// Выполняем перенос слотов указателя разделов
					this->slots = index.slots;
					// Выполняем перенос количества слотов указателя разделов
					this->capacity = index.capacity;
					// Выполняем перенос занятости указателя разделов
					this->occupied = index.occupied;
					// Выполняем сброс количества слотов перемещённого указателя
					index.capacity = 0;
					// Выполняем сброс занятости перемещённого указателя
					index.occupied = 0;
					// Выполняем опустошение перемещённого указателя разделов
					index.slots = nullptr;
				}
				// Выводим приёмник присваивания
				return (* this);
			}
			/**
			 * @brief Оператор присваивания копированием
			 *
			 * @param index копируемый указатель разделов
			 * @return      приёмник присваивания
			 *
			 */
			Index & Index::operator = (const Index & index) noexcept {
				/**
				 * Если приёмник присваивания и его источник есть один указатель
				 */
				if(this != &index){
					// Выполняем очистку приёмника присваивания
					this->clear();
					// Выполняем освобождение слотов приёмника присваивания
					delete[] this->slots;
					// Выполняем принятие количества слотов копируемого указателя
					this->capacity = index.capacity;
					// Выполняем принятие занятости копируемого указателя
					this->occupied = index.occupied;
					/**
					 * Если копируемый указатель пуст
					 */
					if(this->capacity == 0){
						// Выполняем опустошение приёмника присваивания
						this->slots = nullptr;
						// Выводим приёмник присваивания
						return (* this);
					}
					// Выполняем выделение слотов приёмника присваивания
					this->slots = new Slot[this->capacity];
					/**
					 * Выполняем перенос занятых слотов копируемого указателя
					 */
					for(size_t i = 0; i < this->capacity; i++){
						// Очередной слот копируемого указателя
						const Slot & source = index.slots[i];
						/**
						 * Если очередной слот копируемого указателя пуст
						 */
						if(!source.occupied)
							// Выполняем пропуск пустого слота
							continue;
						// Очередной слот приёмника присваивания
						Slot & target = this->slots[i];
						// Выполняем занятие слота приёмника присваивания
						target.occupied = true;
						// Выполняем перенос длины тела ключа
						target.length = source.length;
						// Выполняем перенос размера первой части тела ключа
						target.first = source.first;
						// Выполняем перенос порядкового номера раздела
						target.value = source.value;
						/**
						 * Если тело ключа слотом не вмещается
						 */
						if(source.length > Slot::EMBEDDED)
							// Выполняем принятие хранилища перенесённого ключа
							target.key.owned = new char[source.length];
						// Выполняем перенос тела ключа целиком
						::memcpy(const_cast <char *>(target.data()), source.data(), source.length);
					}
				}
				// Выводим приёмник присваивания
				return (* this);
			}
			/**
			 * @brief Конструктор
			 *
			 */
			Index::Index() noexcept : capacity(0), occupied(0), slots(nullptr) {}
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param index перемещаемый указатель разделов
			 *
			 */
			Index::Index(Index && index) noexcept : capacity(index.capacity), occupied(index.occupied), slots(index.slots) {
				// Выполняем сброс количества слотов перемещённого указателя
				index.capacity = 0;
				// Выполняем сброс занятости перемещённого указателя
				index.occupied = 0;
				// Выполняем опустошение перемещённого указателя разделов
				index.slots = nullptr;
			}
			/**
			 * @brief Конструктор копирования
			 *
			 * @param index копируемый указатель разделов
			 *
			 */
			Index::Index(const Index & index) noexcept : capacity(0), occupied(0), slots(nullptr) {
				// Выполняем присваивание копируемым указателем
				(* this) = index;
			}
			/**
			 * @brief Деструктор
			 *
			 */
			Index::~Index() noexcept {
				// Выполняем очистку указателя разделов
				this->clear();
				// Выполняем освобождение слотов указателя разделов
				delete[] this->slots;
			}
			/**
			 * @brief Метод получения количества записей перечня
			 *
			 * @return количество записей перечня объявлений
			 *
			 */
			size_t Records::Group::size() const noexcept {
				// Выводим количество записей перечня объявлений
				return this->count;
			}
			/**
			 * @brief Метод проверки перечня объявлений на пустоту
			 *
			 * @return признак пустого перечня объявлений
			 *
			 */
			bool Records::Group::empty() const noexcept {
				// Выводим признак пустого перечня объявлений
				return (this->count == 0);
			}
			/**
			 * @brief Метод получения порядкового номера записи перечня
			 *
			 * @param index порядковый номер записи перечня объявлений
			 * @return      порядковый номер записи дерева настроек
			 *
			 */
			uint32_t Records::Group::operator [] (const size_t index) const noexcept {
				// Выводим порядковый номер записи дерева настроек
				return this->items[index];
			}
			/**
			 * @brief Метод получения начала перечня объявлений
			 *
			 * @return адрес первой записи перечня объявлений
			 *
			 */
			const uint32_t * Records::Group::begin() const noexcept {
				// Выводим адрес первой записи перечня объявлений
				return this->items;
			}
			/**
			 * @brief Метод получения конца перечня объявлений
			 *
			 * @return адрес за последней записью перечня объявлений
			 *
			 */
			const uint32_t * Records::Group::end() const noexcept {
				// Выводим адрес за последней записью перечня объявлений
				return (this->items + this->count);
			}
			/**
			 * @brief Очистка указателя свойств
			 *
			 */
			void Records::clear() noexcept {
				/**
				 * Выполняем перебор всех слотов указателя свойств
				 */
				for(size_t i = 0; i < this->capacity; i++)
					// Выполняем освобождение перенесённых данных очередного слота
					this->slots[i].release();
				/**
				 * Если таблица слотов заведена
				 */
				if(this->slots != nullptr)
					// Выполняем опустошение слотов таблицы
					::memset(this->slots, 0, (this->capacity * sizeof(Slot)));
				// Выполняем сброс занятости указателя свойств
				this->occupied = 0;
				// Выполняем сброс количества могильников указателя свойств
				this->buried = 0;
			}
			/**
			 * @brief Метод пересборки таблицы слотов указателя свойств
			 *
			 * @param wanted требуемое количество слотов
			 *
			 */
			void Records::rehash(const size_t wanted) noexcept {
				// Количество слотов пересобранной таблицы
				size_t capacity = SLOTS_MINIMUM;
				/**
				 * Выполняем подбор количества слотов степенью двойки
				 */
				while(capacity < wanted)
					// Выполняем удвоение количества слотов
					capacity <<= 1;
				// Слоты пересобранной таблицы
				Slot * slots = new Slot[capacity];
				// Выполняем опустошение слотов пересобранной таблицы
				::memset(slots, 0, (capacity * sizeof(Slot)));
				// Количество занятых слотов пересобранной таблицы
				size_t occupied = 0;
				/**
				 * Выполняем перенос занятых слотов прежней таблицы
				 *
				 * @note Могильники пересборкой сбрасываются: пробы пересчитываются
				 *       заново, и цепочки, могильниками державшиеся, собираются вновь
				 *
				 */
				for(size_t i = 0; i < this->capacity; i++){
					// Очередной слот прежней таблицы
					const Slot & source = this->slots[i];
					/**
					 * Если очередной слот прежней таблицы занят не является
					 */
					if(source.state != Slot::USED)
						// Выполняем пропуск слота
						continue;
					// Смешанное значение ключа переносимого слота
					const size_t hash = mixed(source.section, string_view(source.data(), source.length), false);
					// Положение очередного слота пробы пересобранной таблицы
					size_t position = (hash & (capacity - 1));
					/**
					 * Выполняем розыск свободного слота пересобранной таблицы
					 */
					while(slots[position].state != Slot::FREE)
						// Выполняем переход к соседнему слоту пробы
						position = ((position + 1) & (capacity - 1));
					// Выполняем перенос состояния слота
					slots[position].state = Slot::USED;
					// Выполняем перенос длины тела ключа
					slots[position].length = source.length;
					// Выполняем перенос количества записей перечня
					slots[position].count = source.count;
					// Выполняем перенос порядкового номера раздела
					slots[position].section = source.section;
					/**
					 * Если тело ключа слотом не вмещается
					 */
					if(source.length > Slot::EMBEDDED)
						// Выполняем принятие хранилища перенесённого ключа
						slots[position].key.owned = new char[source.length];
					// Выполняем перенос тела ключа целиком
					::memcpy(const_cast <char *>(slots[position].data()), source.data(), source.length);
					/**
					 * Если перечень объявлений слотом вмещается
					 */
					if(source.count <= Slot::INLINE)
						// Выполняем перенос перечня объявлений слотом
						::memcpy(slots[position].items.local, source.items.local, (source.count * sizeof(uint32_t)));
					// Если перечень объявлений слотом не вмещается
					else {
						// Выполняем выделение перечня объявлений переносом
						slots[position].items.owned = new uint32_t[source.count];
						// Выполняем перенос записей перенесённого перечня
						::memcpy(slots[position].items.owned, source.items.owned, (source.count * sizeof(uint32_t)));
					}
					// Выполняем учёт занятого слота
					occupied++;
				}
				// Выполняем освобождение слотов прежней таблицы
				this->clear();
				// Выполняем освобождение памяти слотов прежней таблицы
				delete[] this->slots;
				// Выполняем принятие слотов пересобранной таблицы
				this->slots = slots;
				// Выполняем принятие количества слотов пересобранной таблицы
				this->capacity = capacity;
				// Выполняем принятие занятости пересобранной таблицы
				this->occupied = occupied;
				// Выполняем сброс могильников пересобранной таблицы
				this->buried = 0;
			}
			/**
			 * @brief Метод розыска перечня объявлений свойства
			 *
			 * @param section порядковый номер раздела, которому свойство принадлежит
			 * @param key     имя разыскиваемого свойства
			 * @param lower   признак приведения имени свойства к нижнему регистру
			 * @param result  вид на перечень объявлений при успехе розыска
			 * @return        признак того, что перечень объявлений разыскан
			 *
			 */
			bool Records::locate(const uint32_t section, const string_view key, const bool lower, group_t & result) const noexcept {
				/**
				 * Если таблица слотов не заведена
				 */
				if(this->capacity == 0)
					// Выводим признак того, что перечень объявлений не разыскан
					return false;
				// Положение очередного слота пробы
				size_t cursor = (mixed(section, key, lower) & (this->capacity - 1));
				/**
				 * Выполняем перебор слотов пробы
				 */
				while(this->slots[cursor].state != Slot::FREE){
					// Очередной слот пробы
					const Slot & slot = this->slots[cursor];
					/**
					 * Если слот пробы занят и принадлежит тому же разделу
					 */
					if((slot.state == Slot::USED) && (slot.section == section)){
						/**
						 * Если тело слота разыскиваемому имени равно
						 */
						if(matched(slot.data(), slot.length, key, lower)){
							// Выполняем наполнение вида на перечень объявлений
							result.items = slot.itemsData();
							// Выполняем наполнение вида количеством записей перечня
							result.count = slot.count;
							// Выводим признак того, что перечень объявлений разыскан
							return true;
						}
					}
					// Выполняем переход к соседнему слоту пробы
					cursor = ((cursor + 1) & (this->capacity - 1));
				}
				// Выводим признак того, что перечень объявлений не разыскан
				return false;
			}
			/**
			 * @brief Метод добавления записи к перечню объявлений свойства
			 *
			 * @param section порядковый номер раздела, которому свойство принадлежит
			 * @param key     имя свойства
			 * @param lower   признак приведения имени свойства к нижнему регистру
			 * @param record  добавляемый порядковый номер записи дерева настроек
			 * @return        вид на перечень объявлений свойства после добавления
			 *
			 */
			Records::group_t Records::append(const uint32_t section, const string_view key, const bool lower, const uint32_t record) noexcept {
				// Вид на перечень объявлений свойства
				group_t result = {nullptr, 0};
				/**
				 * Если занятость таблицы подошла к половине её слотов
				 *
				 * @note В занятость идут и могильники: проба линейная, и могильник
				 *       удлиняет её наравне с занятым слотом
				 *
				 */
				if(((this->occupied + this->buried) * 2) >= this->capacity)
					// Выполняем пересборку таблицы слотов удвоением
					this->rehash((this->capacity > 0) ? (this->capacity * 2) : SLOTS_MINIMUM);
				// Положение очередного слота пробы
				size_t cursor = (mixed(section, key, lower) & (this->capacity - 1));
				// Положение первого могильника пробы
				size_t dead = this->capacity;
				/**
				 * Выполняем перебор слотов пробы
				 */
				while(this->slots[cursor].state != Slot::FREE){
					// Очередной слот пробы
					const Slot & slot = this->slots[cursor];
					/**
					 * Если очередной слот пробы могильником является
					 */
					if(slot.state == Slot::DEAD){
						/**
						 * Если первый могильник пробы ещё не запомнен
						 */
						if(dead == this->capacity)
							// Запоминаем положение первого могильника пробы
							dead = cursor;
					// Если слот пробы принадлежит тому же разделу
					} else if(slot.section == section){
						/**
						 * Если тело слота устанавливаемому имени равно
						 */
						if(matched(slot.data(), slot.length, key, lower)){
							// Слот, записью дополняемый
							Slot & target = this->slots[cursor];
							/**
							 * Если перечень объявлений слотом вмещается
							 */
							if(target.count < Slot::INLINE)
								// Выполняем добавление записи перечнем слота
								target.items.local[target.count] = record;
							// Если перечень объявлений слотом не вмещается
							else {
								// Перечень объявлений, хранилищем перенесённый
								uint32_t * owned = new uint32_t[target.count + 1];
								// Выполняем перенос записей прежнего перечня объявлений
								::memcpy(owned, target.itemsData(), (target.count * sizeof(uint32_t)));
								/**
								 * Если прежний перечень объявлений перенесён
								 */
								if(target.count > Slot::INLINE)
									// Выполняем освобождение перенесённого перечня объявлений
									delete[] target.items.owned;
								// Выполняем принятие хранилища перенесённого перечня
								target.items.owned = owned;
								// Выполняем добавление записи перечнем хранилища
								target.items.owned[target.count] = record;
							}
							// Выполняем учёт записи перечня объявлений
							target.count++;
							// Выполняем наполнение вида на перечень объявлений
							result.items = target.itemsData();
							// Выполняем наполнение вида количеством записей перечня
							result.count = target.count;
							// Выводим вид на перечень объявлений свойства
							return result;
						}
					}
					// Выполняем переход к соседнему слоту пробы
					cursor = ((cursor + 1) & (this->capacity - 1));
				}
				/**
				 * Если проба mogильник на пути встретила
				 */
				if(dead != this->capacity)
					// Принимаем могильник слотом устанавливаемого свойства
					cursor = dead;
				// Слот, именем занимаемый
				Slot & slot = this->slots[cursor];
				/**
				 * Если слот могильником был
				 */
				if(slot.state == Slot::DEAD)
					// Выполняем снятие могильника
					this->buried--;
				// Выполняем занятие слота пробы
				slot.state = Slot::USED;
				// Выполняем запоминание длины тела ключа
				slot.length = static_cast <uint16_t> (key.length());
				// Выполняем запоминание порядкового номера раздела
				slot.section = section;
				// Выполняем запоминание количества записей перечня
				slot.count = 1;
				/**
				 * Если тело ключа слотом не вмещается
				 */
				if(key.length() > Slot::EMBEDDED)
					// Выполняем принятие хранилища перенесённого ключа
					slot.key.owned = new char[key.length()];
				/**
				 * Выполняем перебор знаков устанавливаемого имени
				 */
				for(size_t i = 0; i < key.length(); i++)
					// Выполняем перенос очередного знака с приведением по флагу
					const_cast <char *>(slot.data())[i] = (lower ? ascii::toLower(key[i]) : key[i]);
				// Выполняем запоминание порядкового номера записи
				slot.items.local[0] = record;
				// Выполняем учёт занятого слота
				this->occupied++;
				// Выполняем наполнение вида на перечень объявлений
				result.items = slot.itemsData();
				// Выполняем наполнение вида количеством записей перечня
				result.count = slot.count;
				// Выводим вид на перечень объявлений свойства
				return result;
			}
			/**
			 * @brief Метод снятия свойства из указателя свойств
			 *
			 * @param section порядковый номер раздела, которому свойство принадлежит
			 * @param key     имя снимаемого свойства
			 * @param lower   признак приведения имени свойства к нижнему регистру
			 * @return        признак того, что свойство снято этим снятием
			 *
			 */
			bool Records::remove(const uint32_t section, const string_view key, const bool lower) noexcept {
				/**
				 * Если таблица слотов не заведена
				 */
				if(this->capacity == 0)
					// Выводим признак того, что свойство снято не было
					return false;
				// Положение очередного слота пробы
				size_t cursor = (mixed(section, key, lower) & (this->capacity - 1));
				/**
				 * Выполняем перебор слотов пробы
				 */
				while(this->slots[cursor].state != Slot::FREE){
					// Очередной слот пробы
					Slot & slot = this->slots[cursor];
					/**
					 * Если слот пробы занят и принадлежит тому же разделу
					 */
					if((slot.state == Slot::USED) && (slot.section == section)){
						/**
						 * Если тело слота снимаемому имени равно
						 */
						if(matched(slot.data(), slot.length, key, lower)){
							// Выполняем освобождение перенесённых данных слота
							slot.release();
							// Выполняем помечение слота могильником
							slot.state = Slot::DEAD;
							// Выполняем снятие слота с учёта занятости
							this->occupied--;
							// Выполняем учёт могильника
							this->buried++;
							// Выводим признак того, что свойство снято
							return true;
						}
					}
					// Выполняем переход к соседнему слоту пробы
					cursor = ((cursor + 1) & (this->capacity - 1));
				}
				// Выводим признак того, что свойство снято не было
				return false;
			}
			/**
			 * @brief Оператор присваивания перемещением
			 *
			 * @param records перемещаемый указатель свойств
			 * @return       приёмник присваивания
			 *
			 */
			Records & Records::operator = (Records && records) noexcept {
				/**
				 * Если приёмник присваивания и его источник есть один указатель
				 */
				if(this != &records){
					// Выполняем очистку приёмника присваивания
					this->clear();
					// Выполняем освобождение слотов приёмника присваивания
					delete[] this->slots;
					// Выполняем перенос слотов указателя свойств
					this->slots = records.slots;
					// Выполняем перенос количества слотов указателя свойств
					this->capacity = records.capacity;
					// Выполняем перенос занятости указателя свойств
					this->occupied = records.occupied;
					// Выполняем перенос могильников указателя свойств
					this->buried = records.buried;
					// Выполняем сброс количества слотов перемещённого указателя
					records.capacity = 0;
					// Выполняем сброс занятости перемещённого указателя
					records.occupied = 0;
					// Выполняем сброс могильников перемещённого указателя
					records.buried = 0;
					// Выполняем опустошение перемещённого указателя свойств
					records.slots = nullptr;
				}
				// Выводим приёмник присваивания
				return (* this);
			}
			/**
			 * @brief Оператор присваивания копированием
			 *
			 * @param records копируемый указатель свойств
			 * @return       приёмник присваивания
			 *
			 */
			Records & Records::operator = (const Records & records) noexcept {
				/**
				 * Если приёмник присваивания и его источник есть один указатель
				 */
				if(this != &records){
					// Выполняем очистку приёмника присваивания
					this->clear();
					// Выполняем освобождение слотов приёмника присваивания
					delete[] this->slots;
					// Выполняем принятие количества слотов копируемого указателя
					this->capacity = records.capacity;
					// Выполняем принятие занятости копируемого указателя
					this->occupied = records.occupied;
					// Выполняем принятие могильников копируемого указателя
					this->buried = records.buried;
					/**
					 * Если копируемый указатель пуст
					 */
					if(this->capacity == 0){
						// Выполняем опустошение приёмника присваивания
						this->slots = nullptr;
						// Выводим приёмник присваивания
						return (* this);
					}
					// Выполняем выделение слотов приёмника присваивания
					this->slots = new Slot[this->capacity];
					/**
					 * Выполняем перенос слотов копируемого указателя
					 */
					for(size_t i = 0; i < this->capacity; i++){
						// Очередной слот копируемого указателя
						const Slot & source = records.slots[i];
						// Очередной слот приёмника присваивания
						Slot & target = this->slots[i];
						// Выполняем перенос состояния слота
						target.state = source.state;
						/**
						 * Если очередной слот копируемого указателя занят не является
						 */
						if(source.state != Slot::USED)
							// Выполняем пропуск слота
							continue;
						// Выполняем перенос длины тела ключа
						target.length = source.length;
						// Выполняем перенос порядкового номера раздела
						target.section = source.section;
						// Выполняем перенос количества записей перечня
						target.count = source.count;
						/**
						 * Если тело ключа слотом не вмещается
						 */
						if(source.length > Slot::EMBEDDED)
							// Выполняем принятие хранилища перенесённого ключа
							target.key.owned = new char[source.length];
						// Выполняем перенос тела ключа целиком
						::memcpy(const_cast <char *>(target.data()), source.data(), source.length);
						/**
						 * Если перечень объявлений слотом вмещается
						 */
						if(source.count <= Slot::INLINE)
							// Выполняем перенос перечня объявлений слотом
							::memcpy(target.items.local, source.items.local, (source.count * sizeof(uint32_t)));
						// Если перечень объявлений слотом не вмещается
						else {
							// Выполняем выделение перечня объявлений переносом
							target.items.owned = new uint32_t[source.count];
							// Выполняем перенос записей перенесённого перечня
							::memcpy(target.items.owned, source.items.owned, (source.count * sizeof(uint32_t)));
						}
					}
				}
				// Выводим приёмник присваивания
				return (* this);
			}
			/**
			 * @brief Конструктор
			 *
			 */
			Records::Records() noexcept : capacity(0), occupied(0), buried(0), slots(nullptr) {}
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param records перемещаемый указатель свойств
			 *
			 */
			Records::Records(Records && records) noexcept : capacity(records.capacity), occupied(records.occupied), buried(records.buried), slots(records.slots) {
				// Выполняем сброс количества слотов перемещённого указателя
				records.capacity = 0;
				// Выполняем сброс занятости перемещённого указателя
				records.occupied = 0;
				// Выполняем сброс могильников перемещённого указателя
				records.buried = 0;
				// Выполняем опустошение перемещённого указателя свойств
				records.slots = nullptr;
			}
			/**
			 * @brief Конструктор копирования
			 *
			 * @param records копируемый указатель свойств
			 *
			 */
			Records::Records(const Records & records) noexcept : capacity(0), occupied(0), buried(0), slots(nullptr) {
				// Выполняем присваивание копируемым указателем
				(* this) = records;
			}
			/**
			 * @brief Деструктор
			 *
			 */
			Records::~Records() noexcept {
				// Выполняем очистку указателя свойств
				this->clear();
				// Выполняем освобождение слотов указателя свойств
				delete[] this->slots;
			}
		};
	};
};
