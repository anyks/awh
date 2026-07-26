/**
 * @file: screen.hpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SCREEN__
#define __AWH_SCREEN__

/**
 * Стандартные заголовочные файлы
 */
#include <queue>
#include <mutex>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>

/**
 * Системный заголовочный файл (для получения идентификатора процесса)
 */
#include <unistd.h>

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
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
			 * @brief Состояние очереди
			 *
			 */
			enum class state_t : uint8_t {
				NONE      = 0x00, // Состояние очереди не установленно
				INCREMENT = 0x01, // Увеличение очереди
				DECREMENT = 0x02  // Уменьшение очереди
			};
			/**
			 * @brief Состояние здоровья
			 *
			 */
			enum class health_t : uint8_t {
				DEAD  = 0x00, // Мёртвый
				ALIVE = 0x01  // Живой
			};
			/**
			 * @brief Политика поведения при переполнении очереди
			 *
			 */
			enum class overflow_t : uint8_t {
				WAIT     = 0x00, // Блокировать поставщика до появления свободного места
				DROP_NEW = 0x01, // Отбрасывать новое поступившее сообщение
				DROP_OLD = 0x02  // Вытеснять самое старое сообщение из очереди
			};
		private:
			/**
			 * @brief Таймаут блокировки времени по умолчанию (100ms)
			 *
			 */
			static constexpr const uint64_t TIMEOUT = 0x64;
		private:
			// Идентификатор потока
			uint64_t _id;
		private:
			// Максимальный размер очереди (0 - очередь не ограничена)
			size_t _capacity;
		private:
			// Состояние здоровья
			health_t _health;
		private:
			// Политика поведения при переполнении очереди
			overflow_t _overflow;
		private:
			// Флаг остановки работы дочернего потока
			std::atomic_bool _stop;
		private:
			// Идентификатор процесса, которому принадлежат примитивы синхронизации
			std::atomic <pid_t> _pid;
		private:
			// Очередь полезной нагрузки
			std::queue <T> _payload;
			// Таймаут ожидания блокировки базы событий
			std::chrono::milliseconds _delay;
		private:
			// Объект дочернего потока
			std::unique_ptr <std::thread> _thr;
		private:
			/**
			 * Примитивы синхронизации хранятся через указатели, чтобы их можно было
			 * безопасно пересоздать в дочернем процессе после вызова fork(), не
			 * разрушая унаследованные (и потенциально захваченные) объекты родителя.
			 */
			// Мютекс защиты очереди и условных переменных
			std::unique_ptr <std::mutex> _mtx;
			// Условная переменная ожидания поступления данных
			std::unique_ptr <std::condition_variable> _cv;
			// Условная переменная ожидания появления свободного места в очереди
			std::unique_ptr <std::condition_variable> _space;
		private:
			/**
			 * @brief Функция обратного вызова при активации триггера
			 *
			 */
			std::function <void ()> _trigger;
			/**
			 * @brief Функция обратного вызова которая срабатывает при передачи данных в дочерний поток
			 *
			 * @param data данные передаваемые в дочерний поток
			 */
			std::function <void (const T &)> _callback;
			/**
			 * @brief Функция обратного вызова при заполнении или освобождении очереди
			 *
			 * @param state состояние очереди (увеличение/уменьшение)
			 * @param size  размер очереди после изменения
			 */
			std::function <void (const state_t, const size_t)> _state;
		private:
			/**
			 * @brief Метод гарантирования существования рабочих примитивов синхронизации с учётом смены процесса
			 *
			 * @note Выполняет ленивое создание примитивов и их пересоздание после fork()
			 */
			void _ensureProcess() noexcept {
				// Получаем идентификатор текущего процесса
				const pid_t pid = ::getpid();
				// Если идентификатор процесса не совпадает (например, после fork)
				if(this->_pid.load(std::memory_order_acquire) != pid){
					// Сбрасываем идентификатор потока
					this->_id = 0;
					/**
					 * Унаследованные от родителя примитивы синхронизации и поток в дочернем
					 * процессе использовать нельзя: рабочего потока здесь не существует, а
					 * мьютекс мог остаться захваченным в момент fork(). Поэтому отказываемся
					 * от владения ими БЕЗ разрушения (release), чтобы не вызвать ни join()
					 * несуществующего потока, ни разрушение захваченного мьютекса.
					 */
					// Отказываемся от унаследованного потока (объект родителя)
					if(this->_thr != nullptr)
						// Освобождаем владение без вызова деструктора
						this->_thr.release();
					// Отказываемся от унаследованных примитивов синхронизации
					if(this->_mtx != nullptr)
						// Освобождаем владение без вызова деструктора
						this->_mtx.release();
					// Отказываемся от унаследованной условной переменной данных
					if(this->_cv != nullptr)
						// Освобождаем владение без вызова деструктора
						this->_cv.release();
					// Отказываемся от унаследованной условной переменной свободного места
					if(this->_space != nullptr)
						// Освобождаем владение без вызова деструктора
						this->_space.release();
					// Очищаем унаследованную очередь сообщений (они принадлежат родителю)
					std::queue <T> ().swap(this->_payload);
					// Помечаем работу остановленной (поток будет поднят заново при необходимости)
					this->_stop.store(true, std::memory_order_release);
					// Запоминаем идентификатор текущего процесса
					this->_pid.store(pid, std::memory_order_release);
				}
				// Если мютекс ещё не создан, создаём рабочие примитивы по требованию
				if(this->_mtx == nullptr)
					// Создаём рабочий мьютекс
					this->_mtx = std::make_unique <std::mutex> ();
				// Если условная переменная данных не создана
				if(this->_cv == nullptr)
					// Создаём условную переменную ожидания данных
					this->_cv = std::make_unique <std::condition_variable> ();
				// Если условная переменная свободного места не создана
				if(this->_space == nullptr)
					// Создаём условную переменную ожидания свободного места
					this->_space = std::make_unique <std::condition_variable> ();
			}
		private:
			/**
			 * @brief Метод применения политики переполнения очереди
			 *
			 * @param lock захваченная блокировка мьютекса очереди
			 * @return     результат: true - можно добавлять данные, false - сообщение отбрасывается
			 */
			bool _admit(std::unique_lock <std::mutex> & lock) noexcept {
				// Если очередь не ограничена либо свободное место есть, разрешаем добавление
				if((this->_capacity == 0) || (this->_payload.size() < this->_capacity))
					// Разрешаем добавление данных
					return true;
				/**
				 * Определяем политику поведения при переполнении очереди
				 */
				switch(static_cast <uint8_t> (this->_overflow)){
					// Если новое сообщение нужно отбросить
					case static_cast <uint8_t> (overflow_t::DROP_NEW):
						// Сообщаем что добавлять данные не нужно
						return false;
					// Если нужно вытеснить самые старые сообщения
					case static_cast <uint8_t> (overflow_t::DROP_OLD): {
						// Вытесняем старые сообщения пока не появится свободное место
						while(this->_payload.size() >= this->_capacity)
							// Удаляем самое старое сообщение
							this->_payload.pop();
						// Разрешаем добавление данных
						return true;
					}
					// Если поставщика нужно заблокировать до появления места
					case static_cast <uint8_t> (overflow_t::WAIT): {
						// Ожидаем появления свободного места либо остановки работы
						this->_space->wait(lock, [this]() noexcept -> bool {
							// Просыпаемся при остановке или при наличии свободного места
							return (this->_stop.load(std::memory_order_acquire) || (this->_payload.size() < this->_capacity));
						});
						// Добавляем данные только если работа не остановлена
						return !this->_stop.load(std::memory_order_acquire);
					}
				}
				// Разрешаем добавление данных
				return true;
			}
		private:
			/**
			 * @brief Метод вывода ошибки работы дочернего потока
			 *
			 * @param error объект перехваченной ошибки
			 */
			void _error([[maybe_unused]] const std::exception & error) const noexcept {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		private:
			/**
			 * @brief Метод получения данных (тело дочернего потока)
			 *
			 */
			void receiving() noexcept {
				/**
				 * Запускаем цикл обработки до явной остановки
				 */
				while(!this->_stop.load(std::memory_order_acquire)){
					// Локальная копия функции активации триггера
					std::function <void ()> trigger = nullptr;
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						{
							// Выполняем блокировку мьютекса очереди
							std::unique_lock <std::mutex> lock(* this->_mtx);
							// Ожидаем поступления данных либо срабатывания таймаута
							this->_cv->wait_for(lock, this->_delay, [this]() noexcept -> bool {
								// Просыпаемся при остановке или при наличии данных
								return (this->_stop.load(std::memory_order_acquire) || !this->_payload.empty());
							});
							// Запоминаем функцию активации триггера
							trigger = this->_trigger;
						}
						// Если функция обратного вызова триггера установлена
						if(trigger != nullptr)
							// Выполняем функцию обратного вызова
							trigger();
						/**
						 * Полностью разбираем накопленную очередь за одно пробуждение
						 */
						for(;;){
							// Объект извлекаемой полезной нагрузки
							T payload;
							// Флаг наличия извлечённых данных
							bool exist = false;
							// Размер очереди после извлечения
							size_t size = 0;
							// Локальная копия функции обратного вызова
							std::function <void (const T &)> callback = nullptr;
							// Локальная копия функции обратного вызова состояния очереди
							std::function <void (const state_t, const size_t)> state = nullptr;
							{
								// Выполняем блокировку мьютекса очереди
								std::unique_lock <std::mutex> lock(* this->_mtx);
								// Если данные в очереди существуют
								if(!this->_payload.empty()){
									// Извлекаем данные полезной нагрузки
									payload = std::move(this->_payload.front());
									// Удаляем обработанное сообщение из очереди
									this->_payload.pop();
									// Получаем размер очереди после извлечения
									size = this->_payload.size();
									// Запоминаем функцию обратного вызова
									callback = this->_callback;
									// Запоминаем функцию обратного вызова состояния очереди
									state = this->_state;
									// Устанавливаем флаг наличия данных
									exist = true;
								}
							}
							// Если данные из очереди извлечены не были, завершаем разбор
							if(!exist)
								// Выходим из цикла разбора очереди
								break;
							// Если поставщик ожидает свободного места, уведомляем его
							if(this->_capacity > 0)
								// Сообщаем об освобождении места в очереди
								this->_space->notify_one();
							// Если функция подписки на данные установлена
							if(callback != nullptr)
								// Рассылаем сообщение подписчику
								callback(payload);
							// Если функция обратного вызова состояния установлена
							if(state != nullptr)
								// Выполняем функцию обратного вызова
								state(state_t::DECREMENT, size);
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const std::exception & error) {
						// Выводим перехваченную ошибку
						this->_error(error);
					}
				}
			}
		public:
			/**
			 * @brief Метод получения идентификатора потока
			 *
			 * @return идентификатор потока
			 */
			uint64_t id() const noexcept {
				// Возвращаем идентификатор потока
				return this->_id;
			}
		public:
			/**
			 * @brief Метод получения размера очереди
			 *
			 * @return размер очереди для получения
			 */
			size_t size() const noexcept {
				// Возвращаем размер очереди
				return this->_payload.size();
			}
			/**
			 * @brief Метод проверки запущен ли в данный момент модуль
			 *
			 * @return результат проверки запущен ли модуль
			 */
			bool launched() const noexcept {
				// Возвращаем результат проверки
				return !this->_stop.load(std::memory_order_acquire);
			}
		public:
			/**
			 * @brief Метод установки максимального размера очереди
			 *
			 * @param size максимальный размер очереди (0 - очередь не ограничена)
			 */
			void capacity(const size_t size) noexcept {
				// Выполняем установку максимального размера очереди
				this->_capacity = size;
			}
			/**
			 * @brief Метод установки политики поведения при переполнении очереди
			 *
			 * @param overflow политика поведения при переполнении очереди
			 */
			void overflow(const overflow_t overflow) noexcept {
				// Выполняем установку политики поведения при переполнении очереди
				this->_overflow = overflow;
			}
		public:
			/**
			 * @brief Метод установки функции обратного вызова активации триггера
			 *
			 * @param callback функция обратного вызова для установки
			 */
			void on(std::function <void ()> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Гарантируем существование рабочих примитивов синхронизации
					this->_ensureProcess();
					// Выполняем блокировку мьютекса очереди
					std::unique_lock <std::mutex> lock(* this->_mtx);
					// Устанавливаем функцию обратного вызова
					this->_trigger = callback;
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception & error) {
					// Выводим перехваченную ошибку
					this->_error(error);
				}
			}
			/**
			 * @brief Метод установки функции обратного вызова
			 *
			 * @param callback функция обратного вызова для установки
			 */
			void on(std::function <void (const T &)> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Гарантируем существование рабочих примитивов синхронизации
					this->_ensureProcess();
					// Выполняем блокировку мьютекса очереди
					std::unique_lock <std::mutex> lock(* this->_mtx);
					// Устанавливаем функцию обратного вызова
					this->_callback = callback;
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception & error) {
					// Выводим перехваченную ошибку
					this->_error(error);
				}
			}
			/**
			 * @brief Метод установки функции обратного вызова получения состояния очереди
			 *
			 * @param callback функция обратного вызова для установки
			 */
			void on(std::function <void (const state_t, const size_t)> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Гарантируем существование рабочих примитивов синхронизации
					this->_ensureProcess();
					// Выполняем блокировку мьютекса очереди
					std::unique_lock <std::mutex> lock(* this->_mtx);
					// Устанавливаем функцию обратного вызова
					this->_state = callback;
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception & error) {
					// Выводим перехваченную ошибку
					this->_error(error);
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
					// Гарантируем существование рабочих примитивов синхронизации
					this->_ensureProcess();
					// Выполняем блокировку мьютекса очереди
					std::unique_lock <std::mutex> lock(* this->_mtx);
					// Выполняем установку задержки времени
					this->_delay = std::chrono::milliseconds(delay);
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception & error) {
					// Выводим перехваченную ошибку
					this->_error(error);
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
					// Гарантируем существование рабочих примитивов синхронизации
					this->_ensureProcess();
					// Размер очереди после добавления данных
					size_t size = 0;
					{
						// Выполняем блокировку мьютекса очереди
						std::unique_lock <std::mutex> lock(* this->_mtx);
						// Применяем политику переполнения; если сообщение отбрасывается, выходим
						if(!this->_admit(lock))
							// Выходим из метода (сообщение отброшено)
							return;
						// Выполняем добавление данных в очередь
						this->_payload.push(std::move(data));
						// Получаем размер очереди после добавления
						size = this->_payload.size();
					}
					// Если функция обратного вызова состояния установлена
					if(this->_state != nullptr)
						// Выполняем функцию обратного вызова
						this->_state(state_t::INCREMENT, size);
					// Отправляем сообщение, что данные записаны
					this->_cv->notify_one();
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception & error) {
					// Выводим перехваченную ошибку
					this->_error(error);
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
					// Гарантируем существование рабочих примитивов синхронизации
					this->_ensureProcess();
					// Размер очереди после добавления данных
					size_t size = 0;
					{
						// Выполняем блокировку мьютекса очереди
						std::unique_lock <std::mutex> lock(* this->_mtx);
						// Применяем политику переполнения; если сообщение отбрасывается, выходим
						if(!this->_admit(lock))
							// Выходим из метода (сообщение отброшено)
							return;
						// Выполняем добавление данных в очередь
						this->_payload.push(data);
						// Получаем размер очереди после добавления
						size = this->_payload.size();
					}
					// Если функция обратного вызова состояния установлена
					if(this->_state != nullptr)
						// Выполняем функцию обратного вызова
						this->_state(state_t::INCREMENT, size);
					// Отправляем сообщение, что данные записаны
					this->_cv->notify_one();
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception & error) {
					// Выводим перехваченную ошибку
					this->_error(error);
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
					// Гарантируем существование рабочих примитивов синхронизации (учёт fork)
					this->_ensureProcess();
					// Если работа модуля запущена
					if(!this->_stop.load(std::memory_order_acquire)){
						// Устанавливаем флаг остановки работы модуля
						this->_stop.store(true, std::memory_order_release);
						// Пробуждаем все ожидающие условные переменные
						this->_cv->notify_all();
						// Пробуждаем поставщиков, ожидающих свободного места
						this->_space->notify_all();
						// Если рабочий поток существует и может быть присоединён
						if((this->_thr != nullptr) && this->_thr->joinable())
							// Дожидаемся завершения работы потока
							this->_thr->join();
						// Удаляем объект рабочего потока
						this->_thr.reset(nullptr);
						// Выполняем сброс идентификатора потока
						this->_id = 0;
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception &) {
					/**
					 * Пропускаем полученную ошибку.
					 *
					 * Этот метод вызывается также в деструкторе,
					 * по этому ошибку выводить не надо, так-как она всплывает всегда
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
					// Гарантируем существование рабочих примитивов синхронизации (учёт fork)
					this->_ensureProcess();
					// Если работа модуля ещё не запущена
					if(this->_stop.load(std::memory_order_acquire)){
						// Снимаем флаг остановки работы модуля
						this->_stop.store(false, std::memory_order_release);
						// Создаём дочерний поток для обработки полезной нагрузки
						this->_thr = std::make_unique <std::thread> (&Screen::receiving, this);
						// Создаём объект хэширования
						std::hash <std::thread::id> hasher;
						// Выполняем получение идентификатора потока
						this->_id = hasher(this->_thr->get_id());
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const std::exception & error) {
					// Выводим перехваченную ошибку
					this->_error(error);
				}
			}
		public:
			/**
			 * @brief Оператор проверки запущен ли в данный момент модуль
			 *
			 * @return результат проверки запущен ли модуль
			 */
			operator bool() const noexcept {
				// Возвращаем результат проверки
				return this->launched();
			}
			/**
			 * @brief Оператор получения размера очереди
			 *
			 * @return размер очереди для получения
			 */
			operator size_t() const noexcept {
				// Возвращаем результат проверки
				return this->size();
			}
		public:
			/**
			 * @brief Оператор присваивания отправки данных в экран
			 *
			 * @param data данные отправляемого сообщения
			 * @return     текущий объект
			 */
			Screen & operator = (T && data) noexcept {
				// Выполняем отправку данных в экран
				this->send(std::forward <T> (data));
				// Возвращаем значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор присваивания отправки данных в экран
			 *
			 * @param data данные отправляемого сообщения
			 * @return     текущий объект
			 */
			Screen & operator = (const T & data) noexcept {
				// Выполняем отправку данных в экран
				this->send(data);
				// Возвращаем значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор присваивания установки таймаута в миллисекундах
			 *
			 * @param delay значение таймаута для установки в миллисекундах
			 * @return      текущий объект
			 */
			Screen & operator = (const uint32_t delay) noexcept {
				// Выполняем установку таймаута
				this->timeout(delay);
				// Возвращаем значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор присваивания установки функции обратного вызова активации триггера
			 *
			 * @param callback функция обратного вызова для установки
			 * @return         текущий объект
			 */
			Screen & operator = (std::function <void ()> callback) noexcept {
				// Выполняем установку функции обратного вызова
				this->on(callback);
				// Возвращаем значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор присваивания установки функции обратного вызова
			 *
			 * @param callback функция обратного вызова для установки
			 * @return         текущий объект
			 */
			Screen & operator = (std::function <void (const T &)> callback) noexcept {
				// Выполняем установку функции обратного вызова
				this->on(callback);
				// Возвращаем значение текущего объекта
				return (* this);
			}
			/**
			 * @brief Оператор присваивания установки функции обратного вызова получения состояния очереди
			 *
			 * @param callback функция обратного вызова для установки
			 * @return         текущий объект
			 */
			Screen & operator = (std::function <void (const state_t, const size_t)> callback) noexcept {
				// Выполняем установку функции обратного вызова
				this->on(callback);
				// Возвращаем значение текущего объекта
				return (* this);
			}
		public:
			/**
			 * @brief Оператор копирования
			 *
			 */
			Screen & operator = (const Screen &) = delete;
			/**
			 * @brief Конструктор копирования
			 *
			 */
			Screen(const Screen &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Screen() noexcept :
			 _id(0), _capacity(0),
			 _health(health_t::ALIVE), _overflow(overflow_t::WAIT),
			 _stop(true), _pid(::getpid()),
			 _delay(std::chrono::milliseconds(TIMEOUT)),
			 _thr(nullptr), _mtx(nullptr), _cv(nullptr), _space(nullptr),
			 _trigger(nullptr), _callback(nullptr), _state(nullptr) {
				// Выполняем запуск модуля
				this->start();
			}
			/**
			 * @brief Конструктор
			 *
			 * @param health статус здоровья
			 */
			explicit Screen(const health_t health) noexcept :
			 _id(0), _capacity(0),
			 _health(health), _overflow(overflow_t::WAIT),
			 _stop(true), _pid(::getpid()),
			 _delay(std::chrono::milliseconds(TIMEOUT)),
			 _thr(nullptr), _mtx(nullptr), _cv(nullptr), _space(nullptr),
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
