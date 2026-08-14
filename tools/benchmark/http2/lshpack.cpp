/**
 * @file lshpack.cpp
 * @date 2026-07-26
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
 * @brief Эталонный стенд сравнения сжатия заголовков HPACK реализацией ls-hpack —
 *        та же нагрузка и тот же прогон, что и у остальных стендов
 *
 * @details ls-hpack - кодек заголовков LiteSpeed, применяемый в lsquic и
 *          OpenLiteSpeed. Реализацией протокола он не является: разбирать поток
 *          кадров и вести обмен им нечем, поэтому сценарии уровня соединения
 *          стенд не выполняет
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочный файл сравниваемой реализации
 */
extern "C" {
	#include <lshpack.h>
};

/**
 * Признак поддержки стендом сценариев уровня соединения
 */
#define RIVAL_SESSIONS 0

/**
 * Причина невыполнения сценариев уровня соединения
 */
#define RIVAL_SESSIONS_REASON "реализация протокола отсутствует: это кодек заголовков"

/**
 * @brief Функции подменённого аллокатора сравниваемой реализации
 *
 * @details Реализация написана на языке C и выделяет память вызовом malloc,
 *          который общим механизмом перехвата не ловится, а собственного
 *          интерфейса аллокатора не предоставляет. Зато её исходные тексты
 *          собираются стендом, и вызовы аллокатора подставляются при сборке
 *          определениями препроцессора - сами исходные тексты не изменяются
 *
 */
extern "C" {
	/**
	 * @brief Функция выделения памяти сравниваемой реализацией
	 *
	 * @param size размер выделяемой памяти
	 * @return     указатель на выделенную память
	 *
	 */
	void * rivalMalloc(size_t size){
		// Учитываем выполненное выделение памяти
		rival::note(size);
		// Выполняем выделение памяти
		return ::malloc(size);
	}
	/**
	 * @brief Функция освобождения памяти сравниваемой реализацией
	 *
	 * @param ptr указатель на освобождаемую память
	 *
	 */
	void rivalFree(void * ptr){
		// Выполняем освобождение памяти
		::free(ptr);
	}
	/**
	 * @brief Функция выделения обнулённой памяти сравниваемой реализацией
	 *
	 * @param count количество элементов
	 * @param size  размер одного элемента
	 * @return      указатель на выделенную память
	 *
	 */
	void * rivalCalloc(size_t count, size_t size){
		// Учитываем выполненное выделение памяти
		rival::note(count * size);
		// Выполняем выделение обнулённой памяти
		return ::calloc(count, size);
	}
	/**
	 * @brief Функция перераспределения памяти сравниваемой реализацией
	 *
	 * @param ptr  указатель на перераспределяемую память
	 * @param size новый размер памяти
	 * @return     указатель на выделенную память
	 *
	 */
	void * rivalRealloc(void * ptr, size_t size){
		// Учитываем выполненное выделение памяти
		rival::note(size);
		// Выполняем перераспределение памяти
		return ::realloc(ptr, size);
	}
};

/**
 * @brief Обвязка реализации ls-hpack под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Размер буфера закодированного блока заголовков в октетах
	 *
	 */
	static constexpr size_t BLOCK_BUFFER = 16384;
	/**
	 * @brief Размер арены декодированного блока заголовков в октетах
	 *
	 */
	static constexpr size_t ARENA_BUFFER = 16384;

	/**
	 * @brief Структура заголовка в представлении сравниваемой реализации
	 *
	 * @details Реализация принимает заголовок единым буфером с названием и
	 *          значением подряд, а обращается к ним смещениями. Буфер
	 *          формируется до замера: сборка представления - работа
	 *          приложения, и в измеряемое время попадать не должна
	 *
	 */
	typedef struct Header {
		// Буфер названия и значения заголовка подряд
		std::string buffer;
		// Длина названия заголовка
		size_t name;
		// Длина значения заголовка
		size_t value;
		// Признак чувствительного значения
		bool sensitive;
	} header_t;
	/**
	 * @brief Класс сжатия заголовков реализацией ls-hpack
	 *
	 */
	class Codec {
		private:
			// Объект кодера заголовков сравниваемой реализации
			struct lshpack_enc _encoder;
			// Объект декодера заголовков сравниваемой реализации
			struct lshpack_dec _decoder;
		private:
			// Признак созданного кодера и декодера заголовков
			bool _created;
		private:
			// Буфер закодированного блока заголовков
			std::vector <unsigned char> _block;
			// Арена декодированного блока заголовков
			std::vector <char> _arena;
		private:
			// Наборы заголовков в представлении сравниваемой реализации
			std::vector <std::vector <header_t>> _sets;
		private:
			/**
			 * @brief Метод освобождения кодера и декодера заголовков
			 *
			 */
			void release() noexcept {
				// Если кодер и декодер заголовков ещё не созданы
				if(!this->_created)
					// Выходим без освобождения кодера и декодера
					return;
				// Освобождаем объект кодера заголовков
				::lshpack_enc_cleanup(&this->_encoder);
				// Освобождаем объект декодера заголовков
				::lshpack_dec_cleanup(&this->_decoder);
				// Снимаем признак созданного кодера и декодера
				this->_created = false;
			}
		public:
			/**
			 * @brief Метод сброса состояния кодера и декодера
			 *
			 * @note Размер динамической таблицы устанавливать не требуется:
			 *       реализация создаёт таблицу размером 4096 октетов, что и есть
			 *       значение SETTINGS_HEADER_TABLE_SIZE по умолчанию
			 *
			 */
			void restart() noexcept {
				// Освобождаем прежние кодер и декодер заголовков
				this->release();
				// Создаём объект кодера заголовков
				::lshpack_enc_init(&this->_encoder);
				// Создаём объект декодера заголовков
				::lshpack_dec_init(&this->_decoder);
				// Устанавливаем признак созданного кодера и декодера
				this->_created = true;
			}
			/**
			 * @brief Метод перевода наборов заголовков в представление реализации
			 *
			 * @param sets наборы заголовков эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние наборы заголовков
				this->_sets.clear();
				// Резервируем память под наборы заголовков
				this->_sets.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов заголовков нагрузки
				 */
				for(const auto & set : sets){
					// Набор заголовков в представлении сравниваемой реализации
					std::vector <header_t> fields;
					// Резервируем память под набор заголовков
					fields.reserve(set.size());
					/**
					 * Выполняем перебор всех заголовков набора
					 */
					for(const auto & field : set)
						// Дописываем очередной заголовок набора
						fields.push_back(header_t{
							(field.name + field.value), field.name.size(), field.value.size(), field.sensitive
						});
					// Дописываем сформированный набор заголовков
					this->_sets.push_back(::std::move(fields));
				}
			}
			/**
			 * @brief Метод кодирования блока заголовков
			 *
			 * @param index номер набора заголовков
			 * @return      размер закодированного блока заголовков
			 *
			 */
			size_t encode(const size_t index) noexcept {
				// Указатель на начало буфера закодированного блока
				unsigned char * begin = this->_block.data();
				// Указатель на конец буфера закодированного блока
				unsigned char * end = (begin + this->_block.size());
				// Указатель на текущую позицию в буфере закодированного блока
				unsigned char * cursor = begin;
				/**
				 * Выполняем перебор всех заголовков набора
				 */
				for(const auto & field : this->_sets[index]){
					// Заголовок в представлении сравниваемой реализации
					lsxpack_header_t header;
					/**
					 * Формируем представление заголовка заново на каждый блок:
					 * реализация кеширует в нём хеши названия и значения, и
					 * переиспользование дало бы ей работу, которой остальные
					 * реализации не выполняют
					 */
					::lsxpack_header_set_offset2(
						&header, field.buffer.data(), 0, field.name, field.name, field.value
					);
					// Если значение заголовка чувствительное
					if(field.sensitive)
						// Помечаем заголовок не подлежащим индексации
						header.flags = LSXPACK_NEVER_INDEX;
					// Запоминаем прежнюю позицию в буфере закодированного блока
					unsigned char * previous = cursor;
					// Выполняем кодирование очередного заголовка набора
					cursor = ::lshpack_enc_encode(&this->_encoder, cursor, end, &header);
					// Если заголовок закодирован с ошибкой
					if(cursor <= previous)
						// Выводим нулевой размер закодированного блока
						return 0;
				}
				// Выводим размер закодированного блока заголовков
				return static_cast <size_t> (cursor - begin);
			}
			/**
			 * @brief Метод декодирования блока заголовков
			 *
			 * @param block закодированный блок заголовков
			 * @return      результат декодирования
			 *
			 */
			bool decode(const std::string & block) noexcept {
				// Указатель на неразобранный остаток блока заголовков
				const unsigned char * cursor = reinterpret_cast <const unsigned char *> (block.data());
				// Указатель на конец блока заголовков
				const unsigned char * end = (cursor + block.size());
				// Смещение очередного заголовка в арене
				size_t offset = 0;
				/**
				 * Выполняем декодирование блока заголовков целиком
				 */
				while(cursor < end){
					// Декодированный заголовок
					lsxpack_header_t header;
					// Готовим представление заголовка под декодирование в арену
					::lsxpack_header_prepare_decode(&header, this->_arena.data(), offset, (this->_arena.size() - offset));
					// Если заголовок декодирован с ошибкой
					if(::lshpack_dec_decode(&this->_decoder, &cursor, end, &header) != LSHPACK_OK)
						// Выводим отрицательный результат
						return false;
					// Учитываем декодированный заголовок
					rival::account(header.name_len, header.val_len);
					// Продвигаем смещение очередного заголовка в арене
					offset += ::lsxpack_header_get_dec_size(&header);
				}
				// Выводим положительный результат
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Codec() noexcept : _created(false) {
				// Выделяем память под буфер блока заголовков с запасом
				this->_block.resize(BLOCK_BUFFER);
				// Выделяем память под арену декодированного блока с запасом
				this->_arena.resize(ARENA_BUFFER);
				// Выполняем создание кодера и декодера заголовков
				this->restart();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Codec() noexcept {
				// Освобождаем кодер и декодер заголовков
				this->release();
			}
	};
};

/**
 * Подключаем общий набор сценариев эталонных стендов
 */
#include "scenarios.hpp"
