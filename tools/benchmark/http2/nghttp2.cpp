/**
 * @file nghttp2.cpp
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
 * @brief Эталонный стенд сравнения протокола HTTP/2 реализацией nghttp2 —
 *        та же нагрузка и тот же прогон, что и у остальных стендов
 *
 * @details nghttp2 - реализация, на которой работают curl, Apache httpd и
 *          Node.js: собственного HTTP/2 у них нет, они подключают эту библиотеку.
 *          Поэтому её показатель и есть показатель перечисленных приложений
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * @brief Обвязка реализации nghttp2 под интерфейс прогона сценариев
 *
 */
namespace {
	/**
	 * @brief Функция выделения памяти сравниваемой реализацией
	 *
	 * @note Реализация написана на языке C и выделяет память вызовом malloc,
	 *       который общим механизмом перехвата не ловится. Зато она
	 *       предоставляет собственный интерфейс аллокатора, и учёт ведётся
	 *       через него - точно, а не оценочно
	 *
	 * @param size размер выделяемой памяти
	 * @return     указатель на выделенную память
	 *
	 */
	static void * memoryMalloc(size_t size, void *) noexcept {
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
	static void memoryFree(void * ptr, void *) noexcept {
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
	static void * memoryCalloc(size_t count, size_t size, void *) noexcept {
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
	static void * memoryRealloc(void * ptr, size_t size, void *) noexcept {
		// Учитываем выполненное выделение памяти
		rival::note(size);
		// Выполняем перераспределение памяти
		return ::realloc(ptr, size);
	}
	/**
	 * @brief Функция получения аллокатора сравниваемой реализации
	 *
	 * @return аллокатор сравниваемой реализации
	 *
	 */
	static nghttp2_mem * memory() noexcept {
		// Аллокатор сравниваемой реализации
		static nghttp2_mem result = {
			nullptr, &::memoryMalloc, &::memoryFree, &::memoryCalloc, &::memoryRealloc
		};
		// Выводим аллокатор сравниваемой реализации
		return &result;
	}
	/**
	 * @brief Функция перевода набора заголовков в представление реализации
	 *
	 * @param fields набор заголовков эталонной нагрузки
	 * @return       набор заголовков в представлении реализации
	 *
	 */
	static std::vector <nghttp2_nv> convert(const std::vector <rival::field_t> & fields) noexcept {
		// Результат работы функции - набор заголовков в представлении реализации
		std::vector <nghttp2_nv> result;
		// Резервируем память под набор заголовков
		result.reserve(fields.size());
		/**
		 * Выполняем перебор всех заголовков набора
		 */
		for(const auto & field : fields)
			// Дописываем очередной заголовок набора
			result.push_back(nghttp2_nv{
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.name.data())),
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.value.data())),
				field.name.size(), field.value.size(),
				static_cast <uint8_t> (field.sensitive ? NGHTTP2_NV_FLAG_NO_INDEX : NGHTTP2_NV_FLAG_NONE)
			});
		// Выводим набор заголовков в представлении реализации
		return result;
	}
	/**
	 * @brief Класс сжатия заголовков реализацией nghttp2
	 *
	 */
	class Codec {
		private:
			// Объект кодера заголовков сравниваемой реализации
			nghttp2_hd_deflater * _deflater;
			// Объект декодера заголовков сравниваемой реализации
			nghttp2_hd_inflater * _inflater;
		private:
			// Буфер закодированного блока заголовков
			std::vector <uint8_t> _block;
		private:
			// Наборы заголовков эталонной нагрузки
			std::vector <std::vector <rival::field_t>> _storage;
			// Наборы заголовков в представлении сравниваемой реализации
			std::vector <std::vector <nghttp2_nv>> _sets;
		public:
			/**
			 * @brief Метод сброса состояния кодера и декодера
			 *
			 */
			void restart() noexcept {
				// Если кодер заголовков уже создан - удаляем его
				if(this->_deflater != nullptr)
					// Удаляем объект кодера заголовков
					::nghttp2_hd_deflate_del(this->_deflater);
				// Если декодер заголовков уже создан - удаляем его
				if(this->_inflater != nullptr)
					// Удаляем объект декодера заголовков
					::nghttp2_hd_inflate_del(this->_inflater);
				// Создаём объект кодера заголовков
				::nghttp2_hd_deflate_new2(&this->_deflater, rival::TABLE_SIZE, ::memory());
				// Создаём объект декодера заголовков
				::nghttp2_hd_inflate_new2(&this->_inflater, ::memory());
				// Устанавливаем размер динамической таблицы декодера
				::nghttp2_hd_inflate_change_table_size(this->_inflater, rival::TABLE_SIZE);
			}
			/**
			 * @brief Метод перевода наборов заголовков в представление реализации
			 *
			 * @param sets наборы заголовков эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Запоминаем наборы заголовков эталонной нагрузки: представление
				// реализации ссылается на строки, а не владеет ими
				this->_storage = sets;
				// Очищаем прежние наборы заголовков
				this->_sets.clear();
				// Резервируем память под наборы заголовков
				this->_sets.reserve(this->_storage.size());
				/**
				 * Выполняем перебор всех наборов заголовков нагрузки
				 */
				for(const auto & set : this->_storage)
					// Дописываем очередной набор заголовков
					this->_sets.push_back(::convert(set));
			}
			/**
			 * @brief Метод кодирования блока заголовков
			 *
			 * @param index номер набора заголовков
			 * @return      размер закодированного блока заголовков
			 *
			 */
			size_t encode(const size_t index) noexcept {
				// Набор заголовков в представлении сравниваемой реализации
				const std::vector <nghttp2_nv> & fields = this->_sets[index];
				// Выполняем кодирование блока заголовков
				const nghttp2_ssize size = ::nghttp2_hd_deflate_hd2(
					this->_deflater, this->_block.data(), this->_block.size(), fields.data(), fields.size()
				);
				// Выводим размер закодированного блока заголовков
				return ((size > 0) ? static_cast <size_t> (size) : 0);
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
				const uint8_t * data = reinterpret_cast <const uint8_t *> (block.data());
				// Размер неразобранного остатка блока заголовков
				size_t size = block.size();
				/**
				 * Выполняем декодирование блока заголовков до последнего заголовка
				 */
				for(;;){
					// Декодированный заголовок
					nghttp2_nv field;
					// Флаги результата декодирования
					int32_t flags = 0;
					// Выполняем декодирование очередного заголовка блока
					const nghttp2_ssize used = ::nghttp2_hd_inflate_hd3(
						this->_inflater, &field, &flags, data, size, 1
					);
					// Если заголовок декодирован с ошибкой
					if(used < 0)
						// Выводим отрицательный результат
						return false;
					// Продвигаем указатель на неразобранный остаток блока
					data += used;
					// Уменьшаем размер неразобранного остатка блока
					size -= static_cast <size_t> (used);
					// Если декодирован очередной заголовок
					if(flags & NGHTTP2_HD_INFLATE_EMIT)
						// Учитываем декодированный заголовок
						rival::account(field.namelen, field.valuelen);
					// Если блок заголовков декодирован целиком
					if(flags & NGHTTP2_HD_INFLATE_FINAL){
						// Завершаем декодирование блока заголовков
						::nghttp2_hd_inflate_end_headers(this->_inflater);
						// Прекращаем декодирование блока заголовков
						break;
					}
					// Если неразобранный остаток блока исчерпан
					if((used == 0) && (size == 0))
						// Прекращаем декодирование блока заголовков
						break;
				}
				// Выводим положительный результат
				return true;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Codec() noexcept : _deflater(nullptr), _inflater(nullptr) {
				// Выделяем память под буфер блока заголовков с запасом
				this->_block.resize(16384);
				// Выполняем создание кодера и декодера заголовков
				this->restart();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Codec() noexcept {
				// Если кодер заголовков создан - удаляем его
				if(this->_deflater != nullptr)
					// Удаляем объект кодера заголовков
					::nghttp2_hd_deflate_del(this->_deflater);
				// Если декодер заголовков создан - удаляем его
				if(this->_inflater != nullptr)
					// Удаляем объект декодера заголовков
					::nghttp2_hd_inflate_del(this->_inflater);
			}
	};
	/**
	 * @brief Класс разбора входящего потока реализацией nghttp2
	 *
	 */
	class Server {
		private:
			// Объект сессии сервера сравниваемой реализации
			nghttp2_session * _session;
		private:
			// Количество разобранных запросов
			size_t _handled;
			// Объём принятого тела
			size_t _accepted;
		private:
			// Признак ответа на принятые запросы
			bool _answering;
		private:
			// Минимальный ответ сервера: заголовки без тела
			std::vector <rival::field_t> _storage;
			// Минимальный ответ сервера в представлении реализации
			std::vector <nghttp2_nv> _answer;
		private:
			/**
			 * @brief Метод обработки принятого кадра
			 *
			 * @param frame принятый кадр
			 *
			 */
			void receive(const nghttp2_frame * frame) noexcept {
				// Если принят не кадр заголовков запроса с завершением потока
				if((frame->hd.type != NGHTTP2_HEADERS) || ((frame->hd.flags & NGHTTP2_FLAG_END_STREAM) == 0))
					// Выходим без обработки принятого кадра
					return;
				// Считаем разобранный запрос
				this->_handled++;
				// Если на принятые запросы требуется отвечать
				if(this->_answering)
					// Отправляем минимальный ответ с завершением потока
					::nghttp2_submit_response2(this->_session, frame->hd.stream_id, this->_answer.data(), this->_answer.size(), nullptr);
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
			 * @brief Метод подачи порции входящего потока
			 *
			 * @note Реализация не отдаёт исходящие байты сама: очередь отправки
			 *       прокачивается вызывающей стороной, и прокачка входит в
			 *       измеряемое время - так же, как у остальных она входит в
			 *       измеряемое время внутри реализации
			 *
			 * @param data данные порции входящего потока
			 * @param size размер порции входящего потока
			 *
			 */
			void feed(const char * data, const size_t size) noexcept {
				// Выполняем разбор порции входящего потока
				::nghttp2_session_mem_recv2(this->_session, reinterpret_cast <const uint8_t *> (data), size);
				// Выполняем прокачку очереди отправки
				::nghttp2_session_send(this->_session);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param answering отвечать на принятые запросы
			 *
			 */
			explicit Server(const bool answering) noexcept :
			 _session(nullptr), _handled(0), _accepted(0), _answering(answering) {
				// Дописываем псевдо-заголовок статуса ответа
				this->_storage.emplace_back(":status", "200");
				// Дописываем заголовок длины содержимого
				this->_storage.emplace_back("content-length", "0");
				// Переводим ответ сервера в представление сравниваемой реализации
				this->_answer = ::convert(this->_storage);
				// Набор функций обратного вызова сессии
				nghttp2_session_callbacks * callbacks = nullptr;
				// Создаём набор функций обратного вызова сессии
				::nghttp2_session_callbacks_new(&callbacks);
				// Устанавливаем функцию обратного вызова записи исходящих байт
				::nghttp2_session_callbacks_set_send_callback2(callbacks, [](nghttp2_session *, const uint8_t *, size_t length, int32_t, void *) noexcept -> nghttp2_ssize {
					// Отбрасываем исходящие байты сессии
					return static_cast <nghttp2_ssize> (length);
				});
				// Устанавливаем функцию обратного вызова заголовка сообщения
				::nghttp2_session_callbacks_set_on_header_callback(callbacks, [](nghttp2_session *, const nghttp2_frame *, const uint8_t *, size_t namelen, const uint8_t *, size_t valuelen, uint8_t, void *) noexcept -> int32_t {
					// Учитываем разобранный заголовок
					rival::account(namelen, valuelen);
					// Продолжаем разбор
					return 0;
				});
				// Устанавливаем функцию обратного вызова тела сообщения
				::nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, [](nghttp2_session *, uint8_t, int32_t, const uint8_t * data, size_t len, void * user) noexcept -> int32_t {
					// Выполняем потребление фрагмента тела сообщения
					rival::consume(data, len);
					// Суммируем объём принятого тела
					static_cast <Server *> (user)->_accepted += len;
					// Продолжаем разбор
					return 0;
				});
				// Устанавливаем функцию обратного вызова принятого кадра
				::nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, [](nghttp2_session *, const nghttp2_frame * frame, void * user) noexcept -> int32_t {
					// Выполняем обработку принятого кадра
					static_cast <Server *> (user)->receive(frame);
					// Продолжаем разбор
					return 0;
				});
				// Создаём объект сессии сервера
				::nghttp2_session_server_new3(&this->_session, callbacks, this, nullptr, ::memory());
				// Удаляем набор функций обратного вызова сессии
				::nghttp2_session_callbacks_del(callbacks);
				// Отправляем преамбулу соединения сервера
				::nghttp2_submit_settings(this->_session, NGHTTP2_FLAG_NONE, nullptr, 0);
				// Выполняем прокачку очереди отправки
				::nghttp2_session_send(this->_session);
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Server() noexcept {
				// Если сессия сервера создана - удаляем её
				if(this->_session != nullptr)
					// Удаляем объект сессии сервера
					::nghttp2_session_del(this->_session);
			}
	};
	/**
	 * @brief Класс полного обмена парой реализаций nghttp2
	 *
	 */
	class Pair {
		private:
			// Объект сессии клиента сравниваемой реализации
			nghttp2_session * _client;
			// Объект сессии сервера сравниваемой реализации
			nghttp2_session * _server;
		private:
			// Количество завершённых обменов
			size_t _completed;
		private:
			// Заголовки ответа сервера эталонной нагрузки
			std::vector <rival::field_t> _storage;
			// Заголовки ответа сервера в представлении реализации
			std::vector <nghttp2_nv> _answer;
		private:
			// Наборы заголовков запроса эталонной нагрузки
			std::vector <std::vector <rival::field_t>> _keeper;
			// Наборы заголовков запроса в представлении реализации
			std::vector <std::vector <nghttp2_nv>> _sets;
		private:
			/**
			 * @brief Метод передачи исходящих байт сессии её пиру
			 *
			 * @param source сессия-источник исходящих байт
			 * @param target сессия-получатель исходящих байт
			 * @return       признак переданных байт
			 *
			 */
			bool transfer(nghttp2_session * source, nghttp2_session * target) noexcept {
				// Признак переданных байт
				bool result = false;
				/**
				 * Выполняем передачу всех исходящих байт сессии
				 */
				for(;;){
					// Указатель на буфер исходящих байт сессии
					const uint8_t * data = nullptr;
					// Получаем очередную порцию исходящих байт сессии
					const nghttp2_ssize size = ::nghttp2_session_mem_send2(source, &data);
					// Если исходящих байт больше нет
					if(size <= 0)
						// Прекращаем передачу исходящих байт
						break;
					// Подаём исходящие байты сессии на разбор её пиру
					::nghttp2_session_mem_recv2(target, data, static_cast <size_t> (size));
					// Запоминаем факт переданных байт
					result = true;
				}
				// Выводим признак переданных байт
				return result;
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
			 * @details Реализация исходящие байты сама не отдаёт: их забирает
			 *          вызывающая сторона, и обмен идёт до тех пор, пока обе
			 *          стороны не перестанут порождать байты
			 *
			 */
			void pump() noexcept {
				/**
				 * Выполняем обмен байтами до исчерпания обеих очередей отправки
				 */
				for(;;){
					// Выполняем передачу исходящих байт клиента серверу
					const bool request = this->transfer(this->_client, this->_server);
					// Выполняем передачу исходящих байт сервера клиенту
					const bool response = this->transfer(this->_server, this->_client);
					// Если ни одна из сторон байт не породила
					if(!request && !response)
						// Прекращаем обмен байтами
						break;
				}
			}
			/**
			 * @brief Метод перевода наборов заголовков в представление реализации
			 *
			 * @param sets наборы заголовков эталонной нагрузки
			 *
			 */
			void prepare(const std::vector <std::vector <rival::field_t>> & sets) noexcept {
				// Запоминаем наборы заголовков эталонной нагрузки
				this->_keeper = sets;
				// Очищаем прежние наборы заголовков
				this->_sets.clear();
				// Резервируем память под наборы заголовков
				this->_sets.reserve(this->_keeper.size());
				/**
				 * Выполняем перебор всех наборов заголовков нагрузки
				 */
				for(const auto & set : this->_keeper)
					// Дописываем очередной набор заголовков
					this->_sets.push_back(::convert(set));
			}
			/**
			 * @brief Метод открытия потока с отправкой запроса
			 *
			 * @param index номер набора заголовков запроса
			 * @return      результат открытия потока
			 *
			 */
			bool open(const size_t index) noexcept {
				// Набор заголовков запроса в представлении сравниваемой реализации
				const std::vector <nghttp2_nv> & fields = this->_sets[index];
				// Отправляем заголовки запроса с завершением потока
				return (::nghttp2_submit_request2(this->_client, nullptr, fields.data(), fields.size(), nullptr, nullptr) > 0);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Pair() noexcept : _client(nullptr), _server(nullptr), _completed(0) {
				// Формируем заголовки ответа сервера
				this->_storage = rival::response(0);
				// Переводим заголовки ответа в представление сравниваемой реализации
				this->_answer = ::convert(this->_storage);
				// Набор функций обратного вызова сессии клиента
				nghttp2_session_callbacks * callbacks = nullptr;
				// Создаём набор функций обратного вызова сессии клиента
				::nghttp2_session_callbacks_new(&callbacks);
				// Устанавливаем функцию обратного вызова заголовка сообщения
				::nghttp2_session_callbacks_set_on_header_callback(callbacks, [](nghttp2_session *, const nghttp2_frame *, const uint8_t *, size_t namelen, const uint8_t *, size_t valuelen, uint8_t, void *) noexcept -> int32_t {
					// Учитываем разобранный заголовок
					rival::account(namelen, valuelen);
					// Продолжаем разбор
					return 0;
				});
				// Устанавливаем функцию обратного вызова тела сообщения
				::nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, [](nghttp2_session *, uint8_t, int32_t, const uint8_t * data, size_t len, void *) noexcept -> int32_t {
					// Выполняем потребление фрагмента тела ответа
					rival::consume(data, len);
					// Продолжаем разбор
					return 0;
				});
				// Устанавливаем функцию обратного вызова принятого кадра
				::nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, [](nghttp2_session *, const nghttp2_frame * frame, void * user) noexcept -> int32_t {
					// Если принят кадр заголовков ответа - считаем завершённый обмен
					if((frame->hd.type == NGHTTP2_HEADERS) && (frame->headers.cat == NGHTTP2_HCAT_RESPONSE))
						// Считаем завершённый обмен
						static_cast <Pair *> (user)->_completed++;
					// Продолжаем разбор
					return 0;
				});
				// Создаём объект сессии клиента
				::nghttp2_session_client_new3(&this->_client, callbacks, this, nullptr, ::memory());
				// Удаляем набор функций обратного вызова сессии клиента
				::nghttp2_session_callbacks_del(callbacks);
				// Создаём набор функций обратного вызова сессии сервера
				::nghttp2_session_callbacks_new(&callbacks);
				// Устанавливаем функцию обратного вызова заголовка сообщения
				::nghttp2_session_callbacks_set_on_header_callback(callbacks, [](nghttp2_session *, const nghttp2_frame *, const uint8_t *, size_t namelen, const uint8_t *, size_t valuelen, uint8_t, void *) noexcept -> int32_t {
					// Учитываем разобранный заголовок
					rival::account(namelen, valuelen);
					// Продолжаем разбор
					return 0;
				});
				// Устанавливаем функцию обратного вызова принятого кадра
				::nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, [](nghttp2_session * session, const nghttp2_frame * frame, void * user) noexcept -> int32_t {
					// Если принят не кадр заголовков запроса с завершением потока
					if((frame->hd.type != NGHTTP2_HEADERS) || ((frame->hd.flags & NGHTTP2_FLAG_END_STREAM) == 0))
						// Продолжаем разбор
						return 0;
					// Получаем объект полного обмена парой реализаций
					Pair * pair = static_cast <Pair *> (user);
					// Провайдер тела ответа сервера
					nghttp2_data_provider2 provider;
					// Источник тела ответа не используется
					provider.source.ptr = nullptr;
					// Устанавливаем функцию обратного вызова чтения тела ответа
					provider.read_callback = [](nghttp2_session *, int32_t, uint8_t * buffer, size_t length, uint32_t * flags, nghttp2_data_source *, void *) noexcept -> nghttp2_ssize {
						// Получаем размер тела ответа
						const size_t size = rival::payload().size();
						// Если тело ответа в буфер не помещается
						if(length < size)
							// Прекращаем чтение тела ответа
							return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
						// Копируем тело ответа в буфер отправки
						::memcpy(buffer, rival::payload().data(), size);
						// Помечаем тело ответа завершённым
						(* flags) |= NGHTTP2_DATA_FLAG_EOF;
						// Выводим размер тела ответа
						return static_cast <nghttp2_ssize> (size);
					};
					// Отправляем заголовки ответа с телом
					::nghttp2_submit_response2(session, frame->hd.stream_id, pair->_answer.data(), pair->_answer.size(), &provider);
					// Продолжаем разбор
					return 0;
				});
				// Создаём объект сессии сервера
				::nghttp2_session_server_new3(&this->_server, callbacks, this, nullptr, ::memory());
				// Удаляем набор функций обратного вызова сессии сервера
				::nghttp2_session_callbacks_del(callbacks);
				// Отправляем преамбулу соединения клиента
				::nghttp2_submit_settings(this->_client, NGHTTP2_FLAG_NONE, nullptr, 0);
				// Отправляем преамбулу соединения сервера
				::nghttp2_submit_settings(this->_server, NGHTTP2_FLAG_NONE, nullptr, 0);
				// Выполняем прокачку исходящих очередей обеих сторон
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
					::nghttp2_session_del(this->_client);
				// Если сессия сервера создана - удаляем её
				if(this->_server != nullptr)
					// Удаляем объект сессии сервера
					::nghttp2_session_del(this->_server);
			}
	};
};

/**
 * Подключаем общий набор сценариев эталонных стендов
 */
#include "scenarios.hpp"
