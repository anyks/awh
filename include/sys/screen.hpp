/**
 * @file: screen.hpp
 * @date: 2025-10-25
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
#include <chrono>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <condition_variable>

/**
 * Блокировщик потока
 */
#include "locker.hpp"

/**
 * @brief основное пространство имён
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
			// Идентификатор потока
			uint64_t _id;
		private:
			// Состояние здоровья
			health_t _health;
		private:
			// Флаг остановки работы дочернего потока
			std::atomic_bool _stop;
		private:
			// Мютекс ожидания данных
			std::mutex _locker;
		private:
			// Мютекс для блокировки потока
			lock_state_t <std::mutex> _mtx;
		private:
			// Объект дочернего потока
			std::thread _thr;
			// Условная переменная, ожидания поступления данных
			std::condition_variable _cv;
		private:
			// Очередь полезной нагрузки
			std::queue <T> _payload;
			// Таймаут ожидания блокировки базы событий
			std::chrono::milliseconds _delay;
		private:
			/**
			 * Таймаут блокировки времени по умолчанию (100ms)
			 */
			static constexpr const uint64_t TIMEOUT = 0x64;
		private:
			/**
			 * Функция обратного вызова при активации триггера
			 */
			function <void ()> _trigger;
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
							this->_trigger();
						// Если данные в очереди существуют
						if(!this->_payload.empty()){
							// Извлекаем данные полезной нагрузки
							const auto & payload = this->_payload.front();
							// Если функция подписки на логи установлена, выводим результат
							if(this->_callback != nullptr)
								// Выводим сообщение лога всем подписавшимся
								this->_callback(payload);
							{
								// Выполняем блокировку уникальным мютексом
								const locker_t lock(this->_mtx);
								// Удаляем текущее задание
								this->_payload.pop();
							}
							// Если функция обратного вызова установлена
							if(this->_state != nullptr)
								// Выполняем функцию обратного вызова
								this->_state(state_t::DECREMENT, this->_payload.size());
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
						unique_lock lock(this->_locker);
						// Выполняем ожидание на поступление новых заданий
						this->_cv.wait_for(lock, this->_delay, std::bind(&Screen::check, this));
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
			void on(function <void ()> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const locker_t lock(this->_mtx);
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
					const locker_t lock(this->_mtx);
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
					const locker_t lock(this->_mtx);
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
					const locker_t lock(this->_mtx);
					// Выполняем установку задержки времени
					this->_delay = std::chrono::milliseconds(delay);
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
					{
						// Выполняем блокировку потока
						const locker_t lock(this->_mtx);
						// Выполняем добавление данных в очередь
						this->_payload.push(std::forward <T> (data));
					}
					// Если функция обратного вызова установлена
					if(this->_state != nullptr)
						// Выполняем функцию обратного вызова
						this->_state(state_t::INCREMENT, this->_payload.size());
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
					{
						// Выполняем блокировку потока
						const locker_t lock(this->_mtx);
						// Выполняем добавление данных в очередь
						this->_payload.push(data);
					}
					// Если функция обратного вызова установлена
					if(this->_state != nullptr)
						// Выполняем функцию обратного вызова
						this->_state(state_t::INCREMENT, this->_payload.size());
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
						// Устанавливаем флаг остановки работы модуля
						this->_stop = !this->_stop;
						// Отправляем сообщение, что данные записаны
						this->_cv.notify_one();
						// Дожидаемся завершения работы потока
						this->_thr.join();
						// Выполняем сброс идентификатора потока
						this->_id = 0;
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception &) {
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
					// Если работа модуля ещё не запущена
					if(this->_stop){
						// Снимаем флаг остановки работы модуля
						this->_stop = !this->_stop;
						// Создаём объект хэширования
						std::hash <std::thread::id> hasher;
						// Создаём дочерний поток для формирования лога
						this->_thr = std::thread(&Screen::receiving, this);
						// Выполняем получение идентификатора потока
						this->_id = hasher(this->_thr.get_id());
						// Отсоединяемся от потока
						this->_thr.detach();
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
			Screen & operator = (function <void ()> callback) noexcept {
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
			explicit Screen() noexcept :
			 _stop(true), _id(0), _health(health_t::ALIVE),
			 _delay(std::chrono::milliseconds(TIMEOUT)),
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
			 _id(0), _health(health), _stop(true),
			 _delay(std::chrono::milliseconds(TIMEOUT)),
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
