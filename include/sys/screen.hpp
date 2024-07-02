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
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_SCREEN__
#define __AWH_SCREEN__

/**
 * Стандартные модули
 */
#include <queue>
#include <mutex>
#include <ctime>
#include <chrono>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <condition_variable>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Шаблон формата данных передаваемого между потоками
	 * @tparam T данные передаваемые между потоками
	 */
	template <typename T>
	/**
	 * Screen Класс для работы с дочерним потоком
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
			// Флаг остановки работы дочернего потока
			bool _stop;
		private:
			// Состояние здоровья
			health_t _health;
		private:
			// Объект дочернего потока
			std::thread _thr;
			// Мютекс для блокировки потока
			mutex _mtx1, _mtx2;
			// Условная переменная, ожидания поступления данных
			condition_variable _cv;
		private:
			// Очередь полезной нагрузки
			queue <T> _payload;
			// Таймаут ожидания блокировки базы событий
			chrono::nanoseconds _delay;
		private:
			// Таймаут блокировки времени по умолчанию (100ms)
			static constexpr const time_t TIMEOUT = 100000000;
		private:
			// Функция обратного вызова при активации триггера
			function <void (void)> _trigger;
			// Функция обратного вызова которая срабатывает при передачи данных в дочерний поток
			function <void (const T &)> _callback;
			// Функция обратного вызова при заполнении или освобождении очереди
			function <void (const state_t, const size_t)> _state;
		private:
			/**
			 * process Метод запуска обработки поступившей задачи
			 */
			void process() noexcept {
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
					// Выполняем блокировку потока
					this->_mtx2.lock();
					// Удаляем текущее задание
					this->_payload.pop();
					// Выполняем разблокировку потока
					this->_mtx2.unlock();
					// Если функция обратного вызова установлена
					if(this->_state != nullptr)
						// Выполняем функцию обратного вызова
						this->_state(state_t::DECREMENT, this->_payload.size());
				}
			}
		private:
			/**
			 * receiving Метод получения данных
			 */
			void receiving() noexcept {
				// Запускаем бесконечный цикл
				while(!this->_stop){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем блокировку уникальным мютексом
						unique_lock <mutex> lock(this->_mtx1);
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
					} catch(const exception & error) {
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
			 * check Метод проверки на существование данных
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
			 * size Метод получения размера очереди
			 * @return размер очереди для получения
			 */
			size_t size() const noexcept {
				// Выводим размер очереди
				return this->_payload.size();
			}
			/**
			 * launched Метод проверки запущен ли в данный момент модуль
			 * @return результат проверки запущен ли модуль
			 */
			bool launched() const noexcept {
				// Выводим результат проверки
				return !this->_stop;
			}
		public:
			/**
			 * on Метод установки функции обратного вызова активации триггера
			 * @param callback функция обратного вызова для установки
			 */
			void on(function <void (void)> callback) noexcept {
				// Устанавливаем функцию обратного вызова
				this->_trigger = callback;
			}
			/**
			 * on Метод установки функции обратного вызова
			 * @param callback функция обратного вызова для установки
			 */
			void on(function <void (const T &)> callback) noexcept {
				// Устанавливаем функцию обратного вызова
				this->_callback = callback;
			}
			/**
			 * on Метод установки функции обратного вызова получения состояния очереди
			 * @param callback функция обратного вызова для установки
			 */
			void on(function <void (const state_t, const size_t)> callback) noexcept {
				// Устанавливаем функцию обратного вызова
				this->_state = callback;
			}
		public:
			/**
			 * timeout Метод установки таймаута в наносекундах
			 * @param delay значение таймаута для установки в наносекундах
			 */
			void timeout(const time_t delay = TIMEOUT) noexcept {
				// Выполняем блокировку потока
				const lock_guard <mutex> lock(this->_mtx2);
				// Выполняем установку задержки времени
				this->_delay = chrono::nanoseconds(delay);
			}
		public:
			/**
			 * send Метод отправки сообщения в экран
			 * @param data данные отправляемого сообщения
			 */
			void send(const T & data) noexcept {
				// Выполняем блокировку потока
				this->_mtx2.lock();
				// Выполняем добавление данных в очередь
				this->_payload.push(data);
				// Выполняем разблокировку потока
				this->_mtx2.unlock();
				// Если функция обратного вызова установлена
				if(this->_state != nullptr)
					// Выполняем функцию обратного вызова
					this->_state(state_t::INCREMENT, this->_payload.size());
				// Отправляем сообщение, что данные записаны
				this->_cv.notify_one();
			}
		public:
			/**
			 * stop Метод остановки работы модуля
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
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {}
			}
			/**
			 * start Метод запуска работы модуля
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
						// Создаём дочерний поток для формирования лога
						this->_thr = std::thread(&Screen::receiving, this);
						// Отсоединяемся от потока
						this->_thr.detach();
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {}
			}
		public:
			/**
			 * Оператор проверки запущен ли в данный момент модуль
			 * @return результат проверки запущен ли модуль
			 */
			operator bool() const noexcept {
				// Выводим результат проверки
				return this->launched();
			}
			/**
			 * Оператор получения размера очереди
			 * @return размер очереди для получения
			 */
			operator size_t() const noexcept {
				// Выводим результат проверки
				return this->size();
			}
		public:
			/**
			 * Оператор [=] отправки данных в экран
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
			 * Оператор [=] установки таймаута в наносекундах
			 * @param delay значение таймаута для установки в наносекундах
			 * @return      текущий объект
			 */
			Screen & operator = (const time_t delay) noexcept {
				// Выполняем установку таймаута
				this->timeout(delay);
				// Выводим значение текущего объекта
				return (* this);
			}
			/**
			 * Оператор [=] установки функции обратного вызова активации триггера
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
			 * Оператор [=] установки функции обратного вызова
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
			 * Оператор [=] установки функции обратного вызова получения состояния очереди
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
			 * Screen Конструктор
			 */
			Screen() noexcept :
			 _stop(true), _health(health_t::ALIVE),
			 _delay(chrono::nanoseconds(TIMEOUT)),
			 _trigger(nullptr), _callback(nullptr), _state(nullptr) {
				// Выполняем запуск модуля
				this->start();
			}
			/**
			 * Screen Конструктор
			 * @param health статус здоровья
			 */
			Screen(const health_t health) noexcept :
			 _stop(true), _health(health),
			 _delay(chrono::nanoseconds(TIMEOUT)),
			 _trigger(nullptr), _callback(nullptr), _state(nullptr) {
				// Если статус здоровья установлен как живой
				if(health == health_t::ALIVE)
					// Выполняем запуск модуля
					this->start();
			}
			/**
			 * ~Screen Деструктор
			 */
			~Screen() noexcept {
				// Выполняем остановку работы модуля
				this->stop();
			}
	};
	/**
	 * Шаблон формата данных передаваемого между потоками
	 * @tclass T данные передаваемые между потоками
	 */
	template <class T>
	// Создаём тип данных работы с экраном
	using screen_t = Screen <T>;
};

#endif // __AWH_SCREEN__
