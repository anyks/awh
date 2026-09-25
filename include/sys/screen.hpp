/**
 * @file: screen.hpp
 * @date: 2024-07-02
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_SCREEN__
#define __AWH_SCREEN__

/**
 * Стандартные модули
 */
#include <queue>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <condition_variable>

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные модули
	 */
	#include <unistd.h>
#endif

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Шаблон формата данных передаваемого между потоками
	 *
	 * @tparam T данные передаваемые между потоками
	 */
	template <typename T>
	/**
	 * @brief Класс для работы с дочерним потоком
	 *
	 */
	class Screen {
		public:
			/**
			 * Состояние очереди
			 */
			enum class state_t : uint8_t {
				NONE      = 0x00, // Состояние очереди не установленно
				INCREMENT = 0x01, // Увеличение очереди
				DECREMENT = 0x02  // Уменьшение очереди
			};
			/**
			 * Состояние здоровья
			 */
			enum class health_t : uint8_t {
				DEAD  = 0x00, // Мёртвый
				ALIVE = 0x01  // Живой
			};
		private:
			// Флаг остановки работы дочернего потока (читается и пишется из разных потоков)
			std::atomic <bool> _stop;
		private:
			// Идентификатор потока
			uint64_t _id;
			// Идентификатор процесса, запустившего поток: после fork потока в дочернем процессе нет
			int64_t _owner;
		private:
			// Состояние здоровья
			health_t _health;
		private:
			// Мютекс для блокировки потока
			std::mutex _mtx;
			// Мютекс ожидания данных
			std::mutex _locker;
			// Объект дочернего потока
			std::thread _thr;
			// Условная переменная, ожидания поступления данных
			std::condition_variable _cv;
		private:
			// Очередь полезной нагрузки
			std::queue <T> _payload;
			// Таймаут ожидания блокировки базы событий
			std::chrono::nanoseconds _delay;
		private:
			/**
			 * Таймаут блокировки времени по умолчанию (100ms)
			 */
			static constexpr const uint64_t TIMEOUT = 0x5F5E100;
		private:
			/**
			 * Функция обратного вызова при активации триггера
			 */
			function <void (void)> _trigger;
			/**
			 * Функция обратного вызова которая срабатывает при передачи данных в дочерний поток
			 */
			function <void (const T &)> _callback;
			/**
			 * Функция обратного вызова при заполнении или освобождении очереди
			 */
			function <void (const state_t, const size_t)> _state;
		private:
			/**
			 * @brief Метод запуска обработки поступившей задачи
			 *
			 */
			void process() noexcept {
				// Если не производится остановка
				if(!this->_stop){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Если функция обратного вызова триггера установлена
						if(this->_trigger != nullptr)
							// Выполняем функцию обратного вызова
							std::apply(this->_trigger, std::make_tuple());
						// Если данные в очереди существуют
						if(!this->_payload.empty()){
							// Извлекаем данные полезной нагрузки
							const auto & payload = this->_payload.front();
							// Если функция подписки на логи установлена, выводим результат
							if(this->_callback != nullptr)
								// Выводим сообщение лога всем подписавшимся
								std::apply(this->_callback, std::make_tuple(payload));
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Удаляем текущее задание
							this->_payload.pop();
							// Выполняем разблокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_state != nullptr)
								// Выполняем функцию обратного вызова
								std::apply(this->_state, std::make_tuple(state_t::DECREMENT, this->_payload.size()));
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", error.what());
						#endif
					}
				}
			}
		private:
			/**
			 * @brief Метод получения данных
			 *
			 */
			void receiving() noexcept {
				/**
				 * Запускаем бесконечный цикл
				 */
				while(!this->_stop){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем блокировку уникальным мютексом
						unique_lock <std::mutex> lock(this->_locker);
						// Выполняем ожидание на поступление новых заданий
						this->_cv.wait_for(lock, this->_delay, std::bind(&Screen::check, this));
						// Обработку выполняем без мютекса ожидания, иначе отправитель ждал бы её окончания
						lock.unlock();
						// Выполняем запуск обработки поступившей задачи
						this->process();
						// Если произведена остановка
						if(this->_stop)
							// Выходим из цикла
							break;
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception &) {
						// Выполняем запуск обработки поступившей задачи
						this->process();
						// Если произведена остановка
						if(this->_stop)
							// Выходим из цикла
							break;
					}
				}
			}
		private:
			/**
			 * @brief Метод получения идентификатора текущего процесса
			 *
			 * @return идентификатор процесса (в MS Windows fork нет, всегда 0)
			 */
			static int64_t pid() noexcept {
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Выводим идентификатор процесса
					return static_cast <int64_t> (::getpid());
				/**
				 * Для операционной системы MS Windows
				 */
				#else
					// Выводим значение по умолчанию
					return 0;
				#endif
			}
		private:
			/**
			 * @brief Метод проверки на существование данных
			 *
			 * @return результат проверки
			 */
			bool check() noexcept {
				// Если произведена остановка выходим
				if(this->_stop)
					// Выходим из функции
					return true;
				// Выполняем проверку на наличие полезной нагрузки
				return !this->_payload.empty();
			}
		public:
			/**
			 * @brief Метод получения идентификатора потока
			 *
			 * @return идентификатор потока
			 */
			uint64_t id() const noexcept {
				// Выводим идентификатор потока
				return this->_id;
			}
		public:
			/**
			 * @brief Метод получения размера очереди
			 *
			 * @return размер очереди для получения
			 */
			size_t size() const noexcept {
				// Выводим размер очереди
				return this->_payload.size();
			}
			/**
			 * @brief Метод проверки запущен ли в данный момент модуль
			 *
			 * @return результат проверки запущен ли модуль
			 */
			bool launched() const noexcept {
				// Выводим результат проверки
				return !this->_stop;
			}
		public:
			/**
			 * @brief Метод установки функции обратного вызова активации триггера
			 *
			 * @param callback функция обратного вызова для установки
			 */
			void on(function <void (void)> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Устанавливаем функцию обратного вызова
					this->_trigger = callback;
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
			/**
			 * @brief Метод установки функции обратного вызова
			 *
			 * @param callback функция обратного вызова для установки
			 */
			void on(function <void (const T &)> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Устанавливаем функцию обратного вызова
					this->_callback = callback;
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
			/**
			 * @brief Метод установки функции обратного вызова получения состояния очереди
			 *
			 * @param callback функция обратного вызова для установки
			 */
			void on(function <void (const state_t, const size_t)> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Устанавливаем функцию обратного вызова
					this->_state = callback;
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
		public:
			/**
			 * @brief Метод установки таймаута в миллисекундах
			 *
			 * @param delay значение таймаута для установки в миллисекундах
			 */
			void timeout(const uint32_t delay) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем установку задержки времени
					this->_delay = std::chrono::nanoseconds(static_cast <uint64_t> (delay) * 1000000);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
			/**
			 * @brief Метод установки таймаута в наносекундах
			 *
			 * @param delay значение таймаута для установки в наносекундах
			 */
			void timeout(const uint64_t delay = TIMEOUT) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем установку задержки времени
					this->_delay = std::chrono::nanoseconds(delay);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
		public:
			/**
			 * @brief Метод отправки сообщения в экран
			 *
			 * @param data данные отправляемого сообщения
			 */
			void send(T && data) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					this->_mtx.lock();
					// Выполняем добавление данных в очередь
					this->_payload.push(std::forward <T> (data));
					// Выполняем разблокировку потока
					this->_mtx.unlock();
					// Если функция обратного вызова установлена
					if(this->_state != nullptr)
						// Выполняем функцию обратного вызова
						std::apply(this->_state, std::make_tuple(state_t::INCREMENT, this->_payload.size()));
					/**
					 * Проходим через мютекс ожидания: задание кладётся под другим мютексом, и без этого
					 * побудка терялась, если поток уже проверил очередь, но ещё не уснул, — он спал весь
					 * свой срок ожидания (до ближайшего таймера, вплоть до секунд)
					 */
					{
						// Выполняем синхронизацию с ожидающим потоком
						const lock_guard <std::mutex> lock(this->_locker);
					}
					// Отправляем сообщение, что данные записаны
					this->_cv.notify_one();
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
			/**
			 * @brief Метод отправки сообщения в экран
			 *
			 * @param data данные отправляемого сообщения
			 */
			void send(const T & data) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					this->_mtx.lock();
					// Выполняем добавление данных в очередь
					this->_payload.push(data);
					// Выполняем разблокировку потока
					this->_mtx.unlock();
					// Если функция обратного вызова установлена
					if(this->_state != nullptr)
						// Выполняем функцию обратного вызова
						std::apply(this->_state, std::make_tuple(state_t::INCREMENT, this->_payload.size()));
					/**
					 * Проходим через мютекс ожидания: задание кладётся под другим мютексом, и без этого
					 * побудка терялась, если поток уже проверил очередь, но ещё не уснул, — он спал весь
					 * свой срок ожидания (до ближайшего таймера, вплоть до секунд)
					 */
					{
						// Выполняем синхронизацию с ожидающим потоком
						const lock_guard <std::mutex> lock(this->_locker);
					}
					// Отправляем сообщение, что данные записаны
					this->_cv.notify_one();
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
		public:
			/**
			 * @brief Метод остановки работы модуля
			 *
			 */
			void stop() noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Если работа модуля запущена
					if(!this->_stop){
						/**
						 * Флаг ставится под мьютексом ожидания: иначе поток мог проверить флаг,
						 * не успеть уснуть и пропустить пробуждение
						 */
						{
							// Выполняем блокировку мьютекса ожидания
							const lock_guard <std::mutex> lock(this->_locker);
							// Устанавливаем флаг остановки работы модуля
							this->_stop = true;
						}
						// Отправляем сообщение, что работа остановлена
						this->_cv.notify_all();
						// Если поток ещё не завершён
						if(this->_thr.joinable()){
							/**
							 * В дочернем процессе после fork потока нет, есть только копия объекта: pthread_detach вернёт ESRCH,
							 * detach() бросит исключение, а разрушение присоединяемого std::thread вызовет std::terminate.
							 * Описатель переносим в объект, который намеренно не разрушается (один описатель на дочерний процесс)
							 */
							if(this->_owner != pid())
								// Переносим описатель потока в неразрушаемый объект
								static_cast <void> (new std::thread(std::move(this->_thr)));
							// Если остановка пришла из самого потока, ожидание было бы взаимоблокировкой
							else if(this->_thr.get_id() == std::this_thread::get_id())
								// Отсоединяемся от потока
								this->_thr.detach();
							/**
							 * Иначе дожидаемся завершения потока. Прежде поток отсоединялся при запуске, join здесь
							 * всегда падал, и поток переживал объект: ожидание на разрушенной условной переменной
							 * завершало процесс исключением «condition_variable timed_wait failed»
							 */
							else this->_thr.join();
						}
						// Выполняем сброс идентификатора потока
						this->_id = 0;
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception &) {
					/**
					 * Пропускаем полученную ошибку: метод вызывается и в деструкторе
					 */
				}
			}
			/**
			 * @brief Метод запуска работы модуля
			 *
			 */
			void start() noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Если работа модуля ещё не запущена
					if(this->_stop){
						// Снимаем флаг остановки работы модуля
						this->_stop = false;
						// Запоминаем процесс, запустивший поток
						this->_owner = pid();
						// Создаём объект хэширования
						std::hash <std::thread::id> hasher;
						// Создаём дочерний поток для формирования лога
						this->_thr = std::thread(&Screen::receiving, this);
						// Выполняем получение идентификатора потока (поток не отсоединяем: stop() дожидается его завершения)
						this->_id = hasher(this->_thr.get_id());
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
		public:
			/**
			 * @brief Оператор проверки запущен ли в данный момент модуль
			 *
			 * @return результат проверки запущен ли модуль
			 */
			operator bool() const noexcept {
				// Выводим результат проверки
				return this->launched();
			}
			/**
			 * @brief Оператор получения размера очереди
			 *
			 * @return размер очереди для получения
			 */
			operator size_t() const noexcept {
				// Выводим результат проверки
				return this->size();
			}
		public:
			/**
			 * @brief Оператор [=] отправки данных в экран
			 *
			 * @param data данные отправляемого сообщения
			 * @return     текущий объект
			 */
			Screen & operator = (T && data) noexcept {
				// Выполняем отправку данных в экран
				this->send(std::forward <T> (data));
				// Выводим значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор [=] отправки данных в экран
			 *
			 * @param data данные отправляемого сообщения
			 * @return     текущий объект
			 */
			Screen & operator = (const T & data) noexcept {
				// Выполняем отправку данных в экран
				this->send(data);
				// Выводим значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор [=] установки таймаута в наносекундах
			 *
			 * @param delay значение таймаута для установки в наносекундах
			 * @return      текущий объект
			 */
			Screen & operator = (const uint64_t delay) noexcept {
				// Выполняем установку таймаута
				this->timeout(delay);
				// Выводим значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор [=] установки таймаута в миллисекундах
			 *
			 * @param delay значение таймаута для установки в миллисекундах
			 * @return      текущий объект
			 */
			Screen & operator = (const uint32_t delay) noexcept {
				// Выполняем установку таймаута
				this->timeout(delay);
				// Выводим значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор [=] установки функции обратного вызова активации триггера
			 *
			 * @param callback функция обратного вызова для установки
			 * @return         текущий объект
			 */
			Screen & operator = (function <void (void)> callback) noexcept {
				// Выполняем установку функции обратного вызова
				this->on(callback);
				// Выводим значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор [=] установки функции обратного вызова
			 *
			 * @param callback функция обратного вызова для установки
			 * @return         текущий объект
			 */
			Screen & operator = (function <void (const T &)> callback) noexcept {
				// Выполняем установку функции обратного вызова
				this->on(callback);
				// Выводим значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор [=] установки функции обратного вызова получения состояния очереди
			 *
			 * @param callback функция обратного вызова для установки
			 * @return         текущий объект
			 */
			Screen & operator = (function <void (const state_t, const size_t)> callback) noexcept {
				// Выполняем установку функции обратного вызова
				this->on(callback);
				// Выводим значение текущего объекта
				return (* this);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Screen() noexcept :
			 _stop(true), _id(0), _owner(0), _health(health_t::ALIVE),
			 _delay(std::chrono::nanoseconds(TIMEOUT)),
			 _trigger(nullptr), _callback(nullptr), _state(nullptr) {
				// Выполняем запуск модуля
				this->start();
			}
			/**
			 * @brief Конструктор
			 *
			 * @param health статус здоровья
			 */
			Screen(const health_t health) noexcept :
			 _stop(true), _id(0), _owner(0), _health(health),
			 _delay(std::chrono::nanoseconds(TIMEOUT)),
			 _trigger(nullptr), _callback(nullptr), _state(nullptr) {
				// Если статус здоровья установлен как живой
				if(health == health_t::ALIVE)
					// Выполняем запуск модуля
					this->start();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Screen() noexcept {
				// Выполняем остановку работы модуля
				this->stop();
			}
	};
	/**
	 * @brief Шаблон формата данных передаваемого между потоками
	 *
	 * @tparam T данные передаваемые между потоками
	 */
	template <class T>
	/**
	 * Создаём тип данных работы с экраном
	 */
	using screen_t = Screen <T>;
};

#endif // __AWH_SCREEN__
