/**
 * @file lsqpack.cpp
 * @date 2026-07-27
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
 * @brief Эталонный стенд сравнения кодека полей QPACK реализации ls-qpack — та же
 *        нагрузка и тот же прогон, что и у остальных сравниваемых реализаций
 *
 * @details Реализация покрывает только сжатие полей: разбора кадров и состояния
 *          соединения в ней нет, поэтому сценарии уровня соединения стендом
 *          не выполняются
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы сравниваемой реализации
 */
extern "C" {
	#include <lsqpack.h>
	#include <lsxpack_header.h>
};

/**
 * Признак поддержки стендом сценариев уровня соединения
 */
#define RIVAL_SESSIONS 0

/**
 * Причина невыполнения сценариев уровня соединения
 */
#define RIVAL_SESSIONS_REASON "реализация покрывает только сжатие полей"

/**
 * @brief Обвязка реализации ls-qpack под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Размер буфера инструкций потока кодера одной секции
	 *
	 */
	static constexpr size_t ENCODER_BUFFER = 4096;
	/**
	 * @brief Размер буфера закодированной секции полей
	 *
	 */
	static constexpr size_t SECTION_BUFFER = 8192;
	/**
	 * @brief Размер буфера декодированных полей секции
	 *
	 */
	static constexpr size_t DECODED_BUFFER = 8192;

	/**
	 * @brief Структура контекста разбора секции полей
	 *
	 */
	typedef struct Context {
		// Буфер декодированных полей секции
		char storage[DECODED_BUFFER];
		// Заголовок разбираемого поля секции
		lsxpack_header_t header;
	} context_t;

	/**
	 * @brief Функция подготовки места под очередное декодированное поле
	 *
	 * @param ctx    контекст разбора секции полей
	 * @param header заголовок разбираемого поля
	 * @param space  требуемый объём места
	 * @return       подготовленный заголовок поля
	 *
	 */
	static lsxpack_header_t * prepare(void * ctx, lsxpack_header_t * header, size_t space) noexcept {
		// Получаем контекст разбора секции полей
		context_t * context = static_cast <context_t *> (ctx);
		// Если требуемое место в буфер не помещается
		if(space > sizeof(context->storage))
			// Выводим отсутствие места под поле
			return nullptr;
		/**
		 * Заголовок предоставлен - разбор поля уже начат, и место лишь расширяется.
		 * Переподготавливать его нельзя: подготовка обнуляет структуру целиком,
		 * а в ней к этому моменту уже записана длина разобранного названия
		 */
		if(header != nullptr){
			/**
			 * Обновляем только объём доступного места: буфер один и тот же, а поле
			 * длины значения служит подготовке подсказкой о доступном месте
			 */
			header->val_len = static_cast <lsxpack_strlen_t> (space);
			// Выводим заголовок поля с обновлённым объёмом места
			return header;
		}
		// Готовим заголовок поля к разбору
		::lsxpack_header_prepare_decode(&context->header, context->storage, 0, space);
		// Выводим подготовленный заголовок поля
		return &context->header;
	}
	/**
	 * @brief Функция обработки декодированного поля секции
	 *
	 * @param header заголовок декодированного поля
	 * @return       результат обработки
	 *
	 */
	static int process(void *, lsxpack_header_t * header) noexcept {
		// Учитываем разобранное поле секции
		rival::account(header->name_len, header->val_len);
		// Продолжаем разбор
		return 0;
	}
	/**
	 * @brief Функция обработки разблокировки секции полей
	 *
	 * @details Блокировка потока в сценариях не наступает: инструкции потока кодера
	 *          подаются перед секцией, которую они обеспечивают
	 *
	 */
	static void unblocked(void *) noexcept {}
	/**
	 * @brief Функция получения набора функций обратного вызова декодера
	 *
	 * @return набор функций обратного вызова декодера
	 *
	 */
	static const lsqpack_dec_hset_if * handlers() noexcept {
		// Набор функций обратного вызова декодера
		static const lsqpack_dec_hset_if result = {&::unblocked, &::prepare, &::process};
		// Выводим набор функций обратного вызова декодера
		return &result;
	}
	/**
	 * @brief Класс сжатия полей реализацией ls-qpack
	 *
	 */
	class Codec {
		private:
			// Объект кодера полей реализации
			lsqpack_enc _encoder;
			// Объект декодера полей реализации
			lsqpack_dec _decoder;
		private:
			// Признак созданного кодера полей
			bool _started;
		private:
			// Буфер инструкций потока кодера
			unsigned char _instructions[ENCODER_BUFFER];
			// Буфер закодированной секции полей
			unsigned char _section[SECTION_BUFFER];
			// Буфер префикса закодированной секции полей
			unsigned char _prefix[32];
			// Буфер инструкций потока декодера
			unsigned char _feedback[LSQPACK_LONGEST_HEADER_ACK * 4];
		private:
			// Хранилища полей наборов в представлении реализации
			std::vector <std::vector <std::string>> _storage;
			// Наборы полей в представлении реализации
			std::vector <std::vector <lsxpack_header_t>> _sets;
			// Признаки чувствительных полей наборов
			std::vector <std::vector <bool>> _flags;
		private:
			// Контекст разбора секции полей
			context_t _context;
		private:
			/**
			 * @brief Метод освобождения объектов кодека
			 *
			 */
			void dispose() noexcept {
				// Если объекты кодека созданы
				if(this->_started){
					// Освобождаем объект кодера полей
					::lsqpack_enc_cleanup(&this->_encoder);
					// Освобождаем объект декодера полей
					::lsqpack_dec_cleanup(&this->_decoder);
					// Снимаем признак созданных объектов кодека
					this->_started = false;
				}
			}
		public:
			/**
			 * @brief Метод сброса состояния кодека
			 *
			 */
			void restart() noexcept {
				// Освобождаем прежние объекты кодека
				this->dispose();
				// Размер инструкции изменения ёмкости динамической таблицы
				size_t length = sizeof(this->_instructions);
				/**
				 * Создаём объект кодера полей реализации: инструкция изменения ёмкости
				 * выставляется самим кодером при создании
				 */
				::lsqpack_enc_init(
					&this->_encoder, nullptr, static_cast <unsigned> (rival::TABLE_CAPACITY),
					static_cast <unsigned> (rival::TABLE_CAPACITY), static_cast <unsigned> (rival::BLOCKED_STREAMS),
					static_cast <lsqpack_enc_opts> (0), this->_instructions, &length
				);
				// Создаём объект декодера полей реализации
				::lsqpack_dec_init(
					&this->_decoder, nullptr, static_cast <unsigned> (rival::TABLE_CAPACITY),
					static_cast <unsigned> (rival::BLOCKED_STREAMS), ::handlers(), static_cast <lsqpack_dec_opts> (0)
				);
				// Запоминаем создание объектов кодека
				this->_started = true;
			}
			/**
			 * @brief Метод перевода наборов полей в представление реализации
			 *
			 * @param sets наборы полей эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние хранилища полей
				this->_storage.clear();
				// Очищаем прежние наборы полей
				this->_sets.clear();
				// Очищаем прежние признаки чувствительных полей
				this->_flags.clear();
				// Резервируем память под хранилища полей
				this->_storage.reserve(sets.size());
				// Резервируем память под наборы полей
				this->_sets.reserve(sets.size());
				// Резервируем память под признаки чувствительных полей
				this->_flags.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов полей нагрузки
				 */
				for(const auto & set : sets){
					// Хранилище полей набора
					std::vector <std::string> storage;
					// Признаки чувствительных полей набора
					std::vector <bool> flags;
					// Резервируем память под хранилище полей набора
					storage.reserve(set.size());
					// Резервируем память под признаки чувствительных полей
					flags.reserve(set.size());
					/**
					 * Выполняем перебор всех полей набора
					 *
					 * Реализация принимает поле одним буфером, в котором название
					 * и значение разделены двумя октетами: собираем их заранее
					 */
					for(const auto & field : set){
						// Дописываем название и значение поля одним буфером
						storage.push_back(field.name + ": " + field.value);
						// Дописываем признак чувствительности поля
						flags.push_back(field.sensitive);
					}
					// Заголовки полей набора в представлении реализации
					std::vector <lsxpack_header_t> headers;
					// Резервируем память под заголовки полей набора
					headers.reserve(set.size());
					/**
					 * Выполняем формирование всех заголовков полей набора
					 */
					for(size_t i = 0; i < set.size(); i++){
						// Заголовок очередного поля набора
						lsxpack_header_t header;
						// Готовим заголовок поля к кодированию
						::lsxpack_header_set_offset2(
							&header, storage[i].data(), 0, set[i].name.size(),
							(set[i].name.size() + 2), set[i].value.size()
						);
						// Дописываем заголовок поля в набор
						headers.push_back(header);
					}
					// Дописываем хранилище полей набора
					this->_storage.push_back(::std::move(storage));
					// Дописываем заголовки полей набора
					this->_sets.push_back(::std::move(headers));
					// Дописываем признаки чувствительных полей набора
					this->_flags.push_back(::std::move(flags));
				}
				/**
				 * Заголовки полей ссылаются на хранилища, а перемещение вектора строк
				 * оставляет их данные на месте: адреса, взятые до перемещения, остаются
				 * действительными. Пересобираем их всё равно - на случай, если реализация
				 * контейнера окажется иной
				 */
				for(size_t i = 0; i < this->_sets.size(); i++){
					/**
					 * Выполняем перебор всех полей набора
					 */
					for(size_t j = 0; j < this->_sets[i].size(); j++)
						// Восстанавливаем указатель на хранилище поля
						this->_sets[i][j].buf = this->_storage[i][j].data();
				}
			}
			/**
			 * @brief Метод кодирования секции полей
			 *
			 * @param index        номер набора полей
			 * @param sid          идентификатор потока секции
			 * @param instructions объём инструкций потока кодера
			 * @return             объём закодированной секции
			 *
			 */
			size_t encode(const size_t index, const uint64_t sid, size_t & instructions) noexcept {
				// Обнуляем объём инструкций потока кодера
				instructions = 0;
				// Если начать секцию полей не удалось
				if(::lsqpack_enc_start_header(&this->_encoder, sid, 0) != 0)
					// Выводим нулевой объём закодированной секции
					return 0;
				// Занятая часть буфера закодированной секции
				size_t occupied = 0;
				/**
				 * Выполняем кодирование всех полей секции
				 */
				for(size_t i = 0; i < this->_sets[index].size(); i++){
					// Доступный объём буфера инструкций потока кодера
					size_t room = (sizeof(this->_instructions) - instructions);
					// Доступный объём буфера закодированной секции
					size_t space = (sizeof(this->_section) - occupied);
					// Выполняем кодирование очередного поля секции
					if(::lsqpack_enc_encode(
						&this->_encoder, (this->_instructions + instructions), &room,
						(this->_section + occupied), &space, &this->_sets[index][i],
						static_cast <lsqpack_enc_flags> (this->_flags[index][i] ? LQEF_NEVER_INDEX : 0)
					) != LQES_OK)
						// Выводим нулевой объём закодированной секции
						return 0;
					// Наращиваем объём инструкций потока кодера
					instructions += room;
					// Наращиваем занятую часть буфера закодированной секции
					occupied += space;
				}
				// Завершаем секцию полей записью её префикса
				const ssize_t prefix = ::lsqpack_enc_end_header(&this->_encoder, this->_prefix, sizeof(this->_prefix), nullptr);
				// Если префикс секции полей записать не удалось
				if(prefix <= 0)
					// Выводим нулевой объём закодированной секции
					return 0;
				// Выводим объём закодированной секции
				return (occupied + static_cast <size_t> (prefix));
			}
			/**
			 * @brief Метод подтверждения отправленной секции полей
			 *
			 * @param sid идентификатор потока секции
			 *
			 */
			void acknowledge(const uint64_t sid) noexcept {
				// Собираем подтверждение отправленной секции
				const std::string confirmation = rival::acknowledge(sid);
				// Подаём подтверждение секции кодеру
				::lsqpack_enc_decoder_in(&this->_encoder, reinterpret_cast <const unsigned char *> (confirmation.data()), confirmation.size());
			}
			/**
			 * @brief Метод декодирования секции полей
			 *
			 * @param item закодированная секция канонического потока
			 * @return     результат декодирования
			 *
			 */
			bool decode(const rival::encoded_t & item) noexcept {
				// Если инструкции потока кодера есть
				if(!item.instructions.empty()){
					// Выполняем разбор инструкций потока кодера
					if(::lsqpack_dec_enc_in(&this->_decoder, reinterpret_cast <const unsigned char *> (item.instructions.data()), item.instructions.size()) != 0)
						// Выводим отрицательный результат
						return false;
				}
				// Текущая позиция разбора секции полей
				const unsigned char * cursor = reinterpret_cast <const unsigned char *> (item.section.data());
				// Доступный объём буфера инструкций потока декодера
				size_t room = sizeof(this->_feedback);
				// Выполняем разбор секции полей
				const lsqpack_read_header_status status = ::lsqpack_dec_header_in(
					&this->_decoder, &this->_context, item.sid, item.section.size(),
					&cursor, item.section.size(), this->_feedback, &room
				);
				// Выводим результат разбора секции полей
				return (status == LQRHS_DONE);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Codec() noexcept : _started(false) {
				// Выполняем приведение состояния кодека к исходному
				this->restart();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Codec() noexcept {
				// Освобождаем объекты кодека
				this->dispose();
			}
	};
};

/**
 * Подключаем общий набор сценариев эталонных стендов
 */
#include "scenarios.hpp"
