/**
 * @file: nghttp3.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения протокола HTTP/3 реализации nghttp3 — та же
 *        нагрузка и тот же прогон, что и у остальных сравниваемых реализаций
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Обвязка реализации nghttp3 под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Функция выделения памяти с учётом выполненных выделений
	 *
	 * @details Реализация написана на языке C и выделяет память через malloc,
	 *          мимо операторов языка C++. Собственный интерфейс аллокатора -
	 *          единственная точка, где её выделения можно учесть наравне
	 *          с остальными
	 *
	 * @param size объём выделяемой памяти
	 * @return     указатель на выделенную память
	 *
	 */
	static void * allocate(size_t size, void *) noexcept {
		// Учитываем выполненное выделение памяти
		rival::note(size);
		// Выполняем выделение памяти
		return ::malloc(size);
	}
	/**
	 * @brief Функция освобождения памяти
	 *
	 * @param ptr указатель на освобождаемую память
	 *
	 */
	static void release(void * ptr, void *) noexcept {
		// Выполняем освобождение памяти
		::free(ptr);
	}
	/**
	 * @brief Функция выделения обнулённой памяти с учётом выполненных выделений
	 *
	 * @param count количество элементов
	 * @param size  размер одного элемента
	 * @return      указатель на выделенную память
	 *
	 */
	static void * allocateZeroed(size_t count, size_t size, void *) noexcept {
		// Учитываем выполненное выделение памяти
		rival::note(count * size);
		// Выполняем выделение обнулённой памяти
		return ::calloc(count, size);
	}
	/**
	 * @brief Функция изменения размера выделенной памяти
	 *
	 * @param ptr  указатель на изменяемую память
	 * @param size новый объём памяти
	 * @return     указатель на выделенную память
	 *
	 */
	static void * reallocate(void * ptr, size_t size, void *) noexcept {
		// Учитываем выполненное выделение памяти
		rival::note(size);
		// Выполняем изменение размера выделенной памяти
		return ::realloc(ptr, size);
	}
	/**
	 * @brief Функция получения аллокатора реализации с учётом выделений памяти
	 *
	 * @return аллокатор реализации
	 *
	 */
	static const nghttp3_mem * memory() noexcept {
		// Аллокатор реализации с учётом выделений памяти
		static nghttp3_mem result = {nullptr, &::allocate, &::release, &::allocateZeroed, &::reallocate};
		// Выводим аллокатор реализации
		return &result;
	}
	/**
	 * @brief Функция перевода набора полей в представление реализации
	 *
	 * @param fields набор полей эталонной нагрузки
	 * @return       набор полей в представлении реализации
	 *
	 */
	static std::vector <nghttp3_nv> convert(const std::vector <rival::field_t> & fields) noexcept {
		// Результат работы функции - набор полей в представлении реализации
		std::vector <nghttp3_nv> result;
		// Резервируем память под набор полей
		result.reserve(fields.size());
		/**
		 * Выполняем перебор всех полей набора
		 */
		for(const auto & field : fields)
			// Дописываем очередное поле набора
			result.push_back(nghttp3_nv{
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.name.data())),
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.value.data())),
				field.name.size(), field.value.size(),
				static_cast <uint8_t> (field.sensitive ? NGHTTP3_NV_FLAG_NEVER_INDEX : NGHTTP3_NV_FLAG_NONE)
			});
		// Выводим набор полей в представлении реализации
		return result;
	}
	/**
	 * @brief Функция получения параметров соединения реализации
	 *
	 * @return параметры соединения реализации
	 *
	 */
	static const nghttp3_settings * options() noexcept {
		// Параметры соединения реализации
		static nghttp3_settings result;
		// Признак заполненности параметров соединения
		static bool filled = false;
		// Если параметры соединения ещё не заполнены
		if(!filled){
			// Заполняем параметры соединения значениями по умолчанию
			::nghttp3_settings_default(&result);
			// Устанавливаем ёмкость динамической таблицы QPACK
			result.qpack_max_dtable_capacity = rival::TABLE_CAPACITY;
			// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
			result.qpack_blocked_streams = rival::BLOCKED_STREAMS;
			// Запоминаем заполненность параметров соединения
			filled = true;
		}
		// Выводим параметры соединения реализации
		return &result;
	}
	/**
	 * @brief Класс сжатия полей реализацией nghttp3
	 *
	 */
	class Codec {
		private:
			// Объект кодера полей реализации
			nghttp3_qpack_encoder * _encoder;
			// Объект декодера полей реализации
			nghttp3_qpack_decoder * _decoder;
		private:
			// Буфер префикса закодированной секции полей
			nghttp3_buf _prefix;
			// Буфер представлений полей секции
			nghttp3_buf _lines;
			// Буфер инструкций потока кодера
			nghttp3_buf _instructions;
			// Буфер инструкций потока декодера
			uint8_t _feedback[512];
		private:
			// Наборы полей в представлении реализации
			std::vector <std::vector <nghttp3_nv>> _sets;
		private:
			/**
			 * @brief Метод освобождения объектов кодека
			 *
			 */
			void dispose() noexcept {
				// Если кодер полей создан
				if(this->_encoder != nullptr){
					// Удаляем объект кодера полей
					::nghttp3_qpack_encoder_del(this->_encoder);
					// Сбрасываем объект кодера полей
					this->_encoder = nullptr;
				}
				// Если декодер полей создан
				if(this->_decoder != nullptr){
					// Удаляем объект декодера полей
					::nghttp3_qpack_decoder_del(this->_decoder);
					// Сбрасываем объект декодера полей
					this->_decoder = nullptr;
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
				// Создаём объект кодера полей реализации
				::nghttp3_qpack_encoder_new(&this->_encoder, rival::TABLE_CAPACITY, ::memory());
				// Устанавливаем ёмкость динамической таблицы кодера
				::nghttp3_qpack_encoder_set_max_dtable_capacity(this->_encoder, rival::TABLE_CAPACITY);
				// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
				::nghttp3_qpack_encoder_set_max_blocked_streams(this->_encoder, rival::BLOCKED_STREAMS);
				// Создаём объект декодера полей реализации
				::nghttp3_qpack_decoder_new(&this->_decoder, rival::TABLE_CAPACITY, rival::BLOCKED_STREAMS, ::memory());
			}
			/**
			 * @brief Метод перевода наборов полей в представление реализации
			 *
			 * @param sets наборы полей эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние наборы полей
				this->_sets.clear();
				// Резервируем память под наборы полей
				this->_sets.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов полей нагрузки
				 */
				for(const auto & set : sets)
					// Дописываем очередной набор полей
					this->_sets.push_back(::convert(set));
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
				// Сбрасываем буфер префикса закодированной секции
				::nghttp3_buf_reset(&this->_prefix);
				// Сбрасываем буфер представлений полей секции
				::nghttp3_buf_reset(&this->_lines);
				// Сбрасываем буфер инструкций потока кодера
				::nghttp3_buf_reset(&this->_instructions);
				// Выполняем кодирование секции полей
				if(::nghttp3_qpack_encoder_encode(
					this->_encoder, &this->_prefix, &this->_lines, &this->_instructions,
					static_cast <int64_t> (sid), this->_sets[index].data(), this->_sets[index].size()
				) != 0){
					// Обнуляем объём инструкций потока кодера
					instructions = 0;
					// Выводим нулевой объём закодированной секции
					return 0;
				}
				// Запоминаем объём инструкций потока кодера
				instructions = ::nghttp3_buf_len(&this->_instructions);
				// Выводим объём закодированной секции
				return (::nghttp3_buf_len(&this->_prefix) + ::nghttp3_buf_len(&this->_lines));
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
				::nghttp3_qpack_encoder_read_decoder(this->_encoder, reinterpret_cast <const uint8_t *> (confirmation.data()), confirmation.size());
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
					if(::nghttp3_qpack_decoder_read_encoder(
						this->_decoder, reinterpret_cast <const uint8_t *> (item.instructions.data()), item.instructions.size()
					) < 0)
						// Выводим отрицательный результат
						return false;
				}
				// Контекст разбора секции полей потока
				nghttp3_qpack_stream_context * context = nullptr;
				// Создаём контекст разбора секции полей потока
				if(::nghttp3_qpack_stream_context_new(&context, static_cast <int64_t> (item.sid), ::memory()) != 0)
					// Выводим отрицательный результат
					return false;
				// Текущая позиция разбора секции полей
				const uint8_t * cursor = reinterpret_cast <const uint8_t *> (item.section.data());
				// Остаток неразобранной части секции полей
				size_t left = item.section.size();
				// Результат разбора секции полей
				bool result = true;
				/**
				 * Выполняем разбор всех полей секции
				 */
				for(;;){
					// Разобранное поле секции
					nghttp3_qpack_nv nv;
					// Флаги разбора поля секции
					uint8_t flags = NGHTTP3_QPACK_DECODE_FLAG_NONE;
					// Выполняем разбор очередного поля секции
					const nghttp3_ssize read = ::nghttp3_qpack_decoder_read_request(this->_decoder, context, &nv, &flags, cursor, left, 1);
					// Если разбор поля секции не удался
					if(read < 0){
						// Запоминаем неудачный разбор секции
						result = false;
						// Прекращаем разбор секции полей
						break;
					}
					// Сдвигаем позицию разбора секции полей
					cursor += read;
					// Уменьшаем остаток неразобранной части секции
					left -= static_cast <size_t> (read);
					// Если разобрано очередное поле секции
					if((flags & NGHTTP3_QPACK_DECODE_FLAG_EMIT) != 0){
						// Получаем представление названия поля
						const nghttp3_vec name = ::nghttp3_rcbuf_get_buf(nv.name);
						// Получаем представление значения поля
						const nghttp3_vec value = ::nghttp3_rcbuf_get_buf(nv.value);
						// Учитываем разобранное поле секции
						rival::account(name.len, value.len);
						// Освобождаем название поля
						::nghttp3_rcbuf_decref(nv.name);
						// Освобождаем значение поля
						::nghttp3_rcbuf_decref(nv.value);
					}
					// Если секция полей разобрана целиком
					if((flags & NGHTTP3_QPACK_DECODE_FLAG_FINAL) != 0)
						// Прекращаем разбор секции полей
						break;
					// Если разбор не продвинулся и полей больше нет
					if((read == 0) && (left == 0))
						// Прекращаем разбор секции полей
						break;
				}
				// Удаляем контекст разбора секции полей потока
				::nghttp3_qpack_stream_context_del(context);
				/**
				 * Забираем накопленные декодером инструкции потока: без этого буфер
				 * инструкций рос бы неограниченно, а у сравниваемых реализаций
				 * подтверждения забираются каждой секцией
				 */
				const size_t length = ::nghttp3_qpack_decoder_get_decoder_streamlen(this->_decoder);
				// Если накопленные декодером инструкции есть
				if((length > 0) && (length <= sizeof(this->_feedback))){
					// Буфер инструкций потока декодера
					nghttp3_buf buffer;
					// Устанавливаем начало буфера инструкций
					buffer.begin = this->_feedback;
					// Устанавливаем позицию чтения буфера инструкций
					buffer.pos = this->_feedback;
					// Устанавливаем позицию записи буфера инструкций
					buffer.last = this->_feedback;
					// Устанавливаем конец буфера инструкций
					buffer.end = (this->_feedback + sizeof(this->_feedback));
					// Забираем накопленные декодером инструкции потока
					::nghttp3_qpack_decoder_write_decoder(this->_decoder, &buffer);
				}
				// Выводим результат разбора секции полей
				return result;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Codec() noexcept : _encoder(nullptr), _decoder(nullptr) {
				// Выполняем инициализацию буфера префикса закодированной секции
				::nghttp3_buf_init(&this->_prefix);
				// Выполняем инициализацию буфера представлений полей секции
				::nghttp3_buf_init(&this->_lines);
				// Выполняем инициализацию буфера инструкций потока кодера
				::nghttp3_buf_init(&this->_instructions);
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
				// Освобождаем буфер префикса закодированной секции
				::nghttp3_buf_free(&this->_prefix, ::memory());
				// Освобождаем буфер представлений полей секции
				::nghttp3_buf_free(&this->_lines, ::memory());
				// Освобождаем буфер инструкций потока кодера
				::nghttp3_buf_free(&this->_instructions, ::memory());
			}
	};
	/**
	 * @brief Класс разбора входящего потока реализацией nghttp3
	 *
	 */
	class Server {
		private:
			// Объект сессии сервера реализации
			nghttp3_conn * _conn;
		private:
			// Количество разобранных запросов
			size_t _handled;
			// Объём принятого тела
			size_t _accepted;
		private:
			// Признак ответа на принятые запросы
			bool _answering;
		private:
			// Минимальный ответ сервера: поля без тела
			std::vector <rival::field_t> _storage;
			// Минимальный ответ сервера в представлении реализации
			std::vector <nghttp3_nv> _answer;
		private:
			/**
			 * @brief Метод прокачки очереди отправки сессии
			 *
			 * @details Реализация не отдаёт исходящие октеты сама: очередь отправки
			 *          прокачивается стендом, и прокачка входит в измеряемое время -
			 *          так же, как у остальных она входит в измеряемое время внутри
			 *          самой реализации
			 *
			 */
			void drain() noexcept {
				/**
				 * Выгружаем исходящие октеты, пока сессия их отдаёт: за один вызов
				 * возвращаются октеты только одного потока
				 */
				for(;;){
					// Векторы исходящих октетов потока
					nghttp3_vec vec[8];
					// Идентификатор потока исходящих октетов
					int64_t sid = -1;
					// Признак завершения потока
					int fin = 0;
					// Выполняем выгрузку исходящих октетов сессии
					const nghttp3_ssize count = ::nghttp3_conn_writev_stream(this->_conn, &sid, &fin, vec, 8);
					// Если исходящих октетов больше нет
					if((count <= 0) && (fin == 0))
						// Прекращаем выгрузку
						return;
					// Суммарный размер выгруженных октетов
					size_t total = 0;
					/**
					 * Выполняем подсчёт объёма всех векторов исходящих октетов
					 */
					for(nghttp3_ssize i = 0; i < count; i++)
						// Суммируем объём очередного вектора
						total += vec[i].len;
					// Извещаем сессию о принятых транспортом октетах
					if(::nghttp3_conn_add_write_offset(this->_conn, sid, total) != 0)
						// Прекращаем выгрузку
						return;
				}
			}
		public:
			/**
			 * @brief Метод получения количества разобранных запросов
			 *
			 * @return количество разобранных запросов
			 *
			 */
			size_t handled() const noexcept {
				// Выводим количество разобранных запросов
				return this->_handled;
			}
			/**
			 * @brief Метод получения объёма принятого тела
			 *
			 * @return объём принятого тела
			 *
			 */
			size_t accepted() const noexcept {
				// Выводим объём принятого тела
				return this->_accepted;
			}
			/**
			 * @brief Метод подачи порции октетов потока на разбор
			 *
			 * @param sid    идентификатор потока
			 * @param buffer буфер порции октетов
			 * @param size   размер порции октетов
			 * @param fin    признак завершения потока
			 *
			 */
			void feed(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
				// Выполняем разбор порции октетов потока
				::nghttp3_conn_read_stream(
					this->_conn, static_cast <int64_t> (sid),
					reinterpret_cast <const uint8_t *> (buffer), size, (fin ? 1 : 0)
				);
				// Выполняем прокачку очереди отправки
				this->drain();
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param answering отвечать на принятые запросы
			 *
			 */
			explicit Server(const bool answering) noexcept :
			 _conn(nullptr), _handled(0), _accepted(0), _answering(answering) {
				// Дописываем псевдо-поле статуса ответа
				this->_storage.emplace_back(":status", "200");
				// Дописываем поле длины содержимого
				this->_storage.emplace_back("content-length", "0");
				// Переводим ответ сервера в представление реализации
				this->_answer = ::convert(this->_storage);
				// Набор функций обратного вызова сессии
				nghttp3_callbacks callbacks;
				// Обнуляем набор функций обратного вызова сессии
				::memset(&callbacks, 0, sizeof(callbacks));
				// Устанавливаем функцию обратного вызова принятого поля секции
				callbacks.recv_header = [](nghttp3_conn *, int64_t, int32_t, nghttp3_rcbuf * name, nghttp3_rcbuf * value, uint8_t, void *, void *) noexcept -> int {
					// Получаем представление названия поля
					const nghttp3_vec key = ::nghttp3_rcbuf_get_buf(name);
					// Получаем представление значения поля
					const nghttp3_vec val = ::nghttp3_rcbuf_get_buf(value);
					// Учитываем разобранное поле секции
					rival::account(key.len, val.len);
					// Продолжаем разбор
					return 0;
				};
				// Устанавливаем функцию обратного вызова принятых данных тела
				callbacks.recv_data = [](nghttp3_conn *, int64_t, const uint8_t * data, size_t size, void * user, void *) noexcept -> int {
					// Выполняем потребление фрагмента тела сообщения
					rival::consume(data, size);
					// Суммируем объём принятого тела
					static_cast <Server *> (user)->_accepted += size;
					// Продолжаем разбор
					return 0;
				};
				// Устанавливаем функцию обратного вызова завершения секции полей
				callbacks.end_headers = [](nghttp3_conn * conn, int64_t sid, int, void * user, void *) noexcept -> int {
					// Получаем объект сервера
					Server * server = static_cast <Server *> (user);
					// Считаем разобранный запрос
					server->_handled++;
					// Если на принятые запросы требуется отвечать
					if(server->_answering)
						// Отправляем минимальный ответ с завершением потока
						::nghttp3_conn_submit_response(conn, sid, server->_answer.data(), server->_answer.size(), nullptr);
					// Продолжаем разбор
					return 0;
				};
				// Создаём объект сессии сервера реализации
				::nghttp3_conn_server_new(&this->_conn, &callbacks, ::options(), ::memory(), this);
				// Однонаправленные потоки сервера нумеруются как 3, 7, 11 (RFC 9000 §2.1)
				::nghttp3_conn_bind_control_stream(this->_conn, 3);
				// Привязываем потоки инструкций кодека QPACK сессии сервера
				::nghttp3_conn_bind_qpack_streams(this->_conn, 7, 11);
				// Выполняем прокачку очереди отправки
				this->drain();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Server() noexcept {
				// Если сессия сервера создана - удаляем её
				if(this->_conn != nullptr)
					// Удаляем объект сессии сервера
					::nghttp3_conn_del(this->_conn);
			}
	};
	/**
	 * @brief Класс полного обмена парой реализаций nghttp3
	 *
	 */
	class Pair {
		private:
			// Объект сессии клиента реализации
			nghttp3_conn * _client;
			// Объект сессии сервера реализации
			nghttp3_conn * _server;
		private:
			// Идентификатор следующего двунаправленного потока клиента
			uint64_t _bidi;
		private:
			// Количество завершённых обменов
			size_t _completed;
		private:
			// Поля ответа сервера
			std::vector <rival::field_t> _storage;
			// Поля ответа сервера в представлении реализации
			std::vector <nghttp3_nv> _answer;
			// Наборы полей запроса в представлении реализации
			std::vector <std::vector <nghttp3_nv>> _sets;
		private:
			/**
			 * @brief Метод передачи исходящих октетов стороны её собеседнику
			 *
			 * @param source сторона, чьи исходящие октеты передаются
			 * @param target сторона, разбирающая переданные октеты
			 * @return       признак состоявшейся передачи
			 *
			 */
			static bool transfer(nghttp3_conn * source, nghttp3_conn * target) noexcept {
				// Признак состоявшейся передачи
				bool moved = false;
				/**
				 * Выгружаем исходящие октеты, пока сессия их отдаёт
				 */
				for(;;){
					// Векторы исходящих октетов потока
					nghttp3_vec vec[8];
					// Идентификатор потока исходящих октетов
					int64_t sid = -1;
					// Признак завершения потока
					int fin = 0;
					// Выполняем выгрузку исходящих октетов сессии
					const nghttp3_ssize count = ::nghttp3_conn_writev_stream(source, &sid, &fin, vec, 8);
					// Если исходящих октетов больше нет
					if((count <= 0) && (fin == 0))
						// Выводим признак состоявшейся передачи
						return moved;
					// Суммарный размер выгруженных октетов
					size_t total = 0;
					/**
					 * Выполняем передачу всех векторов исходящих октетов
					 */
					for(nghttp3_ssize i = 0; i < count; i++){
						// Подаём очередной вектор на разбор собеседнику
						::nghttp3_conn_read_stream(target, sid, vec[i].base, vec[i].len, ((fin != 0) && ((i + 1) == count)) ? 1 : 0);
						// Суммируем объём очередного вектора
						total += vec[i].len;
					}
					/**
					 * Пустая выгрузка с признаком завершения потока: передавать нечего,
					 * но собеседник обязан узнать о завершении
					 */
					if((count == 0) && (fin != 0))
						// Извещаем собеседника о завершении потока
						::nghttp3_conn_read_stream(target, sid, nullptr, 0, 1);
					// Извещаем сессию о принятых транспортом октетах
					if(::nghttp3_conn_add_write_offset(source, sid, total) != 0)
						// Выводим признак состоявшейся передачи
						return moved;
					// Запоминаем состоявшуюся передачу
					moved = true;
				}
			}
		public:
			/**
			 * @brief Метод получения количества завершённых обменов
			 *
			 * @return количество завершённых обменов
			 *
			 */
			size_t completed() const noexcept {
				// Выводим количество завершённых обменов
				return this->_completed;
			}
			/**
			 * @brief Метод прокачки исходящих очередей обеих сторон
			 *
			 */
			void pump() noexcept {
				/**
				 * Прокачка ограничена сверху: обе стороны вправе отвечать на разбор
				 * отправкой, и без границы переписка могла бы не закончиться
				 */
				for(size_t guard = 0; guard < 64; guard++){
					// Передаём исходящие октеты клиента серверу
					const bool request = transfer(this->_client, this->_server);
					// Передаём исходящие октеты сервера клиенту
					const bool response = transfer(this->_server, this->_client);
					// Если передавать больше нечего
					if(!request && !response)
						// Прекращаем прокачку
						return;
				}
			}
			/**
			 * @brief Метод перевода наборов полей в представление реализации
			 *
			 * @param sets наборы полей эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Очищаем прежние наборы полей
				this->_sets.clear();
				// Резервируем память под наборы полей
				this->_sets.reserve(sets.size());
				/**
				 * Выполняем перебор всех наборов полей нагрузки
				 */
				for(const auto & set : sets)
					// Дописываем очередной набор полей
					this->_sets.push_back(::convert(set));
			}
			/**
			 * @brief Метод открытия потока с отправкой запроса
			 *
			 * @param index номер набора полей запроса
			 * @return      результат открытия потока
			 *
			 */
			bool open(const size_t index) noexcept {
				// Выделяем идентификатор нового двунаправленного потока клиента
				const int64_t sid = static_cast <int64_t> (this->_bidi);
				// Продвигаем идентификатор следующего двунаправленного потока
				this->_bidi += 4;
				// Отправляем секцию полей запроса с завершением потока
				return (::nghttp3_conn_submit_request(this->_client, sid, this->_sets[index].data(), this->_sets[index].size(), nullptr, nullptr) == 0);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Pair() noexcept : _client(nullptr), _server(nullptr), _bidi(0), _completed(0) {
				// Формируем поля ответа сервера
				this->_storage = rival::response(0);
				// Переводим ответ сервера в представление реализации
				this->_answer = ::convert(this->_storage);
				// Набор функций обратного вызова сессии клиента
				nghttp3_callbacks client;
				// Обнуляем набор функций обратного вызова сессии клиента
				::memset(&client, 0, sizeof(client));
				// Набор функций обратного вызова сессии сервера
				nghttp3_callbacks server;
				// Обнуляем набор функций обратного вызова сессии сервера
				::memset(&server, 0, sizeof(server));
				// Устанавливаем функцию обратного вызова принятого поля секции
				client.recv_header = server.recv_header = [](nghttp3_conn *, int64_t, int32_t, nghttp3_rcbuf * name, nghttp3_rcbuf * value, uint8_t, void *, void *) noexcept -> int {
					// Получаем представление названия поля
					const nghttp3_vec key = ::nghttp3_rcbuf_get_buf(name);
					// Получаем представление значения поля
					const nghttp3_vec val = ::nghttp3_rcbuf_get_buf(value);
					// Учитываем разобранное поле секции
					rival::account(key.len, val.len);
					// Продолжаем разбор
					return 0;
				};
				// Устанавливаем функцию обратного вызова принятых данных тела ответа
				client.recv_data = [](nghttp3_conn *, int64_t, const uint8_t * data, size_t size, void *, void *) noexcept -> int {
					// Выполняем потребление фрагмента тела ответа
					rival::consume(data, size);
					// Продолжаем разбор
					return 0;
				};
				// Устанавливаем функцию обратного вызова завершения секции полей ответа
				client.end_headers = [](nghttp3_conn *, int64_t, int, void * user, void *) noexcept -> int {
					// Считаем завершённый обмен
					static_cast <Pair *> (user)->_completed++;
					// Продолжаем разбор
					return 0;
				};
				// Устанавливаем функцию обратного вызова завершения секции полей запроса
				server.end_headers = [](nghttp3_conn * conn, int64_t sid, int, void * user, void *) noexcept -> int {
					// Получаем объект пары
					Pair * pair = static_cast <Pair *> (user);
					// Отправляем поля ответа с телом
					::nghttp3_conn_submit_response(conn, sid, pair->_answer.data(), pair->_answer.size(), &pair->reader());
					// Продолжаем разбор
					return 0;
				};
				// Создаём объект сессии клиента реализации
				::nghttp3_conn_client_new(&this->_client, &client, ::options(), ::memory(), this);
				// Создаём объект сессии сервера реализации
				::nghttp3_conn_server_new(&this->_server, &server, ::options(), ::memory(), this);
				// Однонаправленные потоки клиента нумеруются как 2, 6, 10
				::nghttp3_conn_bind_control_stream(this->_client, 2);
				// Привязываем потоки инструкций кодека QPACK сессии клиента
				::nghttp3_conn_bind_qpack_streams(this->_client, 6, 10);
				// Однонаправленные потоки сервера нумеруются как 3, 7, 11
				::nghttp3_conn_bind_control_stream(this->_server, 3);
				// Привязываем потоки инструкций кодека QPACK сессии сервера
				::nghttp3_conn_bind_qpack_streams(this->_server, 7, 11);
				// Выполняем начальную прокачку исходящих очередей обеих сторон
				this->pump();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Pair() noexcept {
				// Если сессия клиента создана - удаляем её
				if(this->_client != nullptr)
					// Удаляем объект сессии клиента
					::nghttp3_conn_del(this->_client);
				// Если сессия сервера создана - удаляем её
				if(this->_server != nullptr)
					// Удаляем объект сессии сервера
					::nghttp3_conn_del(this->_server);
			}
		public:
			/**
			 * @brief Метод получения источника тела ответа сервера
			 *
			 * @details Без источника тела сессия завершила бы поток сразу после полей,
			 *          а объявленная в них длина тела осталась бы неподтверждённой:
			 *          сравниваемые реализации отдают тело, и эта - обязана тоже
			 *
			 * @return источник тела ответа сервера
			 *
			 */
			nghttp3_data_reader & reader() noexcept {
				// Источник тела ответа сервера
				static nghttp3_data_reader result = {
					[](nghttp3_conn *, int64_t, nghttp3_vec * vec, size_t veccnt, uint32_t * flags, void *, void *) noexcept -> nghttp3_ssize {
						// Если векторов для выдачи тела не предоставлено
						if(veccnt == 0)
							// Тело выдать некуда
							return 0;
						// Устанавливаем указатель на тело ответа
						vec[0].base = reinterpret_cast <uint8_t *> (const_cast <char *> (rival::payload().data()));
						// Устанавливаем длину тела ответа
						vec[0].len = rival::payload().size();
						// Помечаем тело выданным целиком
						(* flags) = NGHTTP3_DATA_FLAG_EOF;
						// Выводим количество заполненных векторов
						return 1;
					}
				};
				// Выводим источник тела ответа сервера
				return result;
			}
	};
};

/**
 * Подключаем общий набор сценариев эталонных стендов
 */
#include "scenarios.hpp"
