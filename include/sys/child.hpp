/**
 * @file: child.hpp
 * @date: 2023-06-29
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_CHLDREN__
#define __AWH_CHLDREN__

#include <queue>
#include <mutex>
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
	 * Child Класс для работы с дочерним потоком
	 */
	class Child {
		public:
			/**
			 * Состояние очереди
			 */
			enum class state_t : uint8_t {
				INCREMENT = 0x01, // Увеличение очереди
				DECREMENT = 0x02  // Уменьшение очереди
			};
		private:
			// Флаг остановки работы дочернего потока
			bool _stop;
		private:
			// Объект дочернего потока
			std::thread _thr;
			// Мютекс для блокировки потока
			mutable mutex _mtx1, _mtx2;
			// Очередь полезной нагрузки
			mutable queue <T> _payload;
			// Условная переменная, ожидания поступления данных
			mutable condition_variable _cv;
		private:
			// Функция обратного вызова которая срабатывает при передачи данных в дочерний поток
			function <void (const T &)> _callback;
			// Функция обратного вызова при заполнении или освобождении очереди
			function <void (const state_t, const size_t)> _state;
		private:
			/**
			 * receiving Метод получения данных
			 */
			void receiving() const noexcept {
				// Запускаем бесконечный цикл
				while(!this->_stop){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем блокировку уникальным мютексом
						unique_lock <mutex> lock(this->_mtx1);
						// Выполняем ожидание на поступление новых заданий
						this->_cv.wait_for(lock, 100ms, std::bind(&Child::checkInputData, this));
						// Если произведена остановка выходим
						if(this->_stop) break;
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
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						// Если произведена остановка выходим
						if(this->_stop) break;
					}
				}
			}
		private:
			/**
			 * checkInputData Метод проверки на существование данных
			 * @return результат проверки
			 */
			bool checkInputData() const noexcept {
				// Если произведена остановка выходим
				if(this->_stop)
					// Выходим из функции
					return true;
				// Выполняем проверку на наличие полезной нагрузки
				return !this->_payload.empty();
			}
		public:
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
			 * send Метод отправки сообщения в дочерний поток
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
			 * Child Конструктор
			 */
			Child() noexcept : _stop(false), _callback(nullptr), _state(nullptr) {
				// Создаём дочерний поток для формирования лога
				this->_thr = std::thread(&Child::receiving, this);
				// Отсоединяемся от потока
				this->_thr.detach();
			}
			/**
			 * ~Child Деструктор
			 */
			~Child() noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем остановку работы дочернего потока
					this->_stop = true;
					// Отправляем сообщение, что данные записаны
					this->_cv.notify_one();
					// Дожидаемся завершения работы потока
					this->_thr.join();
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {}
			}
	};
	/**
	 * Шаблон формата данных передаваемого между потоками
	 * @tclass T данные передаваемые между потоками
	 */
	template <class T>
	// Создаём тип данных работы с дочерними потоками
	using child_t = Child <T>;
};

#endif // __AWH_CHLDREN__
