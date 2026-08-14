/**
 * @file callback.hpp
 * @date 2026-01-21
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
 * \~russian
 * @brief Заголовочный файл модуля функций обратного вызова — класс Callback, реализующий типобезопасное хранилище
 *        колбэков произвольных сигнатур с адресацией по идентификатору или имени,
 *        итератором обхода и потокобезопасным вызовом
 *
 * \~english
 * @brief Header file of the callback function module — the Callback class, which implements a type-safe storage
 *        of callbacks of arbitrary signatures addressed by identifier or by name,
 *        with a traversal iterator and a thread-safe call
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CALLBACK__
#define __AWH_CALLBACK__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <utility>
#include <functional>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "fmk.hpp"
#include "log.hpp"
#include "locker.hpp"
#include "../cryptography/crypto.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс работы с функциями обратного вызова
	 *
	 * \~english
	 * @brief Class for working with callback functions
	 *
	 * \~
	 */
	class Callback {
		public:
			/**
			 * \~russian
			 * @brief Основные события для функций обратного вызова
			 *
			 * \~english
			 * @brief Main events for the callback functions
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE = 0x00, // Событие не установленно
				SET  = 0x01, // Событие установки функции
				DEL  = 0x02, // Событие удаления функции
				RUN  = 0x03  // Событие запуска функции
			};
		private:
			/**
			 * \~russian
			 * @brief Структура базовой функции
			 *
			 * \~english
			 * @brief Structure of the base function
			 *
			 * \~
			 */
			struct Function {
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				virtual ~Function() noexcept = default;
			};
			/**
			 * \~russian
			 * @brief Шаблон базовой функции
			 *
			 * @tparam A сигнатура функции
			 *
			 * \~english
			 * @brief Template of the base function
			 * @tparam A signature of the function
			 *
			 * \~
			 */
			template <typename A>
			/**
			 * \~russian
			 * @brief Структура базовой функции
			 *
			 * \~english
			 * @brief Structure of the base function
			 *
			 * \~
			 */
			struct BasicFunction : Function {
				/**
				 * \~russian
				 * @brief Функция обратного вызова
				 *
				 * \~english
				 * @brief Callback function
				 *
				 * \~
				 */
				std::function <A> fn;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fn функция обратного вызова для установки
				 *
				 * \~english
				 * @brief Constructor
				 * @param fn callback function to set
				 *
				 * \~
				 */
				explicit BasicFunction(std::function <A> fn) noexcept : fn(std::move(fn)) {}
			};
		public:
			/**
			 * \~russian
			 * @brief Тип идентификатора события
			 *
			 * \~english
			 * @brief Type of the identifier of an event
			 *
			 * \~
			 */
			using id_t = uint32_t;
			/**
			 * \~russian
			 * @brief Создаём тип данных функции обратного вызова
			 *
			 * \~english
			 * @brief Create the data type of the callback function
			 *
			 * \~
			 */
			using fn_t = std::shared_ptr <Function>;
		public:
			/**
			 * \~russian
			 * @brief Итератор как вложенный класс
			 *
			 * \~english
			 * @brief Iterator as a nested class
			 *
			 * \~
			 */
			typedef class Iterator {
				public:
					/**
					 * \~russian
					 * @brief Создаём необходимые нам типы данных
					 *
					 * \~english
					 * @brief Create the data types we need
					 *
					 * \~
					 */
					using value_type        = fn_t;
					using pointer           = fn_t *;
					using reference         = fn_t &;
					using difference_type   = std::ptrdiff_t;
					using iterator_category = std::forward_iterator_tag;
				public:
					/**
					 * \~russian
					 * @brief Создаём тип данных итератора
					 *
					 * \~english
					 * @brief Create the data type of the iterator
					 *
					 * \~
					 */
					using iterator = unordered_map <id_t, fn_t>::iterator;
				private:
					// Текущее значение итератора
					iterator _it;
				private:
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Оператор извлечения указателя заголовка
					 *
					 * @return указатель заголовка
					 *
					 * \~english
					 * @brief Header pointer extraction operator
					 * @return header pointer
					 *
					 * \~
					 */
					pointer operator -> () const noexcept {
						// Возвращаем результат
						return &this->_it->second;
					}
					/**
					 * \~russian
					 * @brief Оператор разыменования заголовка
					 *
					 * @return значение заголовка
					 *
					 * \~english
					 * @brief Header dereference operator
					 * @return header value
					 *
					 * \~
					 */
					reference operator * () const noexcept {
						// Возвращаем результат
						return this->_it->second;
					}
				public:
					/**
					 * \~russian
					 * @brief Оператор смещения вперед
					 *
					 * @return значение текущего итератора
					 *
					 * \~english
					 * @brief Forward shift operator
					 * @return value of the current iterator
					 *
					 * \~
					 */
					Iterator & operator ++ () noexcept {
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем смещение итератора
							++this->_it;
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception & error) {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
							#endif
						}
						// Возвращаем результат
						return (* this);
					}
					/**
					 * \~russian
					 * @brief Оператор постинкрементного смещения вперед
					 *
					 * @return значение итератора до смещения
					 *
					 * \~english
					 * @brief Postfix forward shift operator
					 * @return value of the iterator before the shift
					 *
					 * \~
					 */
					Iterator operator ++ (const int32_t) noexcept {
						// Сохраняем текущее состояние итератора
						Iterator current(* this);
						// Выполняем смещение текущего итератора вперёд
						++(* this);
						// Возвращаем сохранённое состояние итератора
						return current;
					}
				public:
					/**
					 * \~russian
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Iterator equality comparison operator
					 * @param other iterator to compare with
					 * @return      result of the comparison
					 *
					 * \~
					 */
					bool operator == (const Iterator & other) const noexcept {
						// Возвращаем результат
						return (this->_it == other._it);
					}
					/**
					 * \~russian
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Iterator inequality comparison operator
					 * @param other iterator to compare with
					 * @return      result of the comparison
					 *
					 * \~
					 */
					bool operator != (const Iterator & other) const noexcept {
						// Возвращаем результат
						return (this->_it != other._it);
					}
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param it  итератор для установки
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param it  iterator to set
					 * @param log object for working with logs
					 *
					 * \~
					 */
					explicit Iterator(iterator it, const log_t * log) noexcept : _it(it), _log(log) {}
			} iterator_t;
		private:
			// Объект работы с криптографией
			crypto_t _crypto;
		private:
			// Хранилище распределения по названиям
			unordered_map <id_t, fn_t> _callbacks;
		private:
			// Объект холдера для блокировки основного потока
			mutable lock_state_t <std::recursive_mutex> _mtx;
		private:
			/**
			 * \~russian
			 * @brief Функция обратного вызова при получении события установки или удаления функции
			 *
			 * @param флаг типа события
			 * @param идентификатор функции
			 * @param функция обратного вызова в чистом виде
			 *
			 * \~english
			 * @brief Callback function on the receipt of an event of setting or removing a function
			 * @param the flag of the kind of the event
			 * @param the identifier of the function
			 * @param the callback function in its pure kind
			 *
			 * \~
			 */
			std::function <void (const event_t, const id_t, const fn_t &)> _callback;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода безопасного захвата сразу двух объектов блокировки
			 *
			 * @tparam Fn тип исполняемого функционала под захваченными блокировками
			 *
			 * \~english
			 * @brief Template of the method of safely capturing two lock objects at once
			 * @tparam Fn type of the functionality performed under the captured locks
			 *
			 * \~
			 */
			template <typename Fn>
			/**
			 * \~russian
			 * @brief Метод безопасного захвата сразу двух объектов блокировки
			 *
			 * @details Блокировки захватываются в детерминированном порядке (по адресу объекта),
			 *          что исключает взаимоблокировку (deadlock) при встречных операциях над двумя контейнерами.
			 *
			 * @param first  первый объект блокировки
			 * @param second второй объект блокировки
			 * @param fn     исполняемый функционал под захваченными блокировками
			 *
			 * \~english
			 * @brief Method of safely capturing two lock objects at once
			 * @details The locks are captured in a deterministic order (by the address of the object),
			 *          which rules out a deadlock on counter operations over two containers.
			 * @param first  first lock object
			 * @param second second lock object
			 * @param fn     functionality performed under the captured locks
			 *
			 * \~
			 */
			static void _dualLock(lock_state_t <std::recursive_mutex> & first, lock_state_t <std::recursive_mutex> & second, Fn && fn) {
				// Если оба объекта блокировки совпадают (операция над одним и тем же контейнером)
				if(&first == &second){
					// Выполняем захват единственной блокировки
					const locker_t <std::recursive_mutex> lock(first, locker_t <std::recursive_mutex>::mode_t::SHARED);
					// Выполняем исполняемый функционал
					fn();
				// Если первый объект блокировки предшествует второму по адресу
				} else if(std::less <const void *> {}(&first, &second)) {
					// Выполняем захват первой блокировки
					const locker_t <std::recursive_mutex> lock1(first, locker_t <std::recursive_mutex>::mode_t::SHARED);
					// Выполняем захват второй блокировки
					const locker_t <std::recursive_mutex> lock2(second, locker_t <std::recursive_mutex>::mode_t::SHARED);
					// Выполняем исполняемый функционал
					fn();
				// Если второй объект блокировки предшествует первому по адресу
				} else {
					// Выполняем захват второй блокировки
					const locker_t <std::recursive_mutex> lock1(second, locker_t <std::recursive_mutex>::mode_t::SHARED);
					// Выполняем захват первой блокировки
					const locker_t <std::recursive_mutex> lock2(first, locker_t <std::recursive_mutex>::mode_t::SHARED);
					// Выполняем исполняемый функционал
					fn();
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Метод генерации идентификатора функции
			 *
			 * @param name название функции для генерации идентификатора
			 * @return     сгенерированный идентификатор функции
			 *
			 * \~english
			 * @brief Method of generating the identifier of a function
			 * @param name name of the function to generate the identifier from
			 * @return     the generated identifier of the function
			 *
			 * \~
			 */
			id_t id(string_view name) const noexcept {
				// Переменная результата
				id_t result = 0;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Если название функции передано
					if(!name.empty()){
						// Если размер имени умещается в 4 байт
						if(name.size() <= 4)
							// Выполняем копирование данных имени
							::memcpy(&result, name.data(), name.size());
						// Получаем идентификатор обратного вызова
						else return this->_crypto.hash <id_t> (name);
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Возвращаем результат
				return result;
			}
		public:
			/**
			 * \~russian
			 * @brief Метод проверки на пустоту контейнера
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the container for emptiness
			 * @return result of the check
			 *
			 * \~
			 */
			bool empty() const noexcept {
				// Выполняем блокировку потока
				const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::SHARED);
				// Возвращаем результат проверки
				return this->_callbacks.empty();
			}
		public:
			/**
			 * \~russian
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 *
			 * \~english
			 * @brief Method of setting the thread safety of the work
			 * @param mode flag of the thread safety mode
			 *
			 * \~
			 */
			void threadSafety(const bool mode) noexcept {
				// Устанавливаем режим безопасности потоков
				this->_mtx.enabled = mode;
			}
		public:
			/**
			 * \~russian
			 * @brief Метод получения дампа функций обратного вызова
			 *
			 * @return выводим созданный блок дампа контейнера
			 *
			 * \~english
			 * @brief Method of getting the dump of the callback functions
			 * @return we yield the built dump block of the container
			 *
			 * \~
			 */
			const unordered_map <id_t, fn_t> & dump() const noexcept {
				// Формируем дамп функций обратного вызова
				return this->_callbacks;
			}
			/**
			 * \~russian
			 * @brief Метод установки дампа функций обратного вызова
			 *
			 * @param callbacks дамп данных функций обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the dump of the callback functions
			 * @param callbacks dump of the data of the callback functions
			 *
			 * \~
			 */
			void dump(const unordered_map <id_t, fn_t> & callbacks) noexcept {
				// Если данные функций обратного вызова переданы
				if(!callbacks.empty()){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем блокировку потока
						const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
						// Устанавливаем новые данные функциий обратного вызова
						this->_callbacks = callbacks;
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Метод очистки контейнера
			 *
			 * \~english
			 * @brief Method of clearing the container
			 *
			 * \~
			 */
			void clear() noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
					// Выполняем очистку списка функций обратного вызова
					this->_callbacks.clear();
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
		private:
			/**
			 * \~russian
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return   результат проверки
			 *
			 * \~english
			 * @brief Method of checking the presence of a callback function
			 * @param id identifier of the callback function
			 * @return   result of the check
			 *
			 * \~
			 */
			bool _is(const id_t id) const noexcept {
				// Выполняем блокировку потока
				const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::SHARED);
				// Возвращаем результат проверки
				return ((id > 0) && (this->_callbacks.find(id) != this->_callbacks.end()));
			}
		public:
			/**
			 * \~russian
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 *
			 *
			 * \~english
			 * @brief Method of checking the presence of a callback function
			 * @param name name of the callback function
			 * @return     result of the check
			 *
			 * \~
			 */
			bool is(string_view name) const noexcept {
				// Выполняем првоерку существования функции обратного вызова
				return this->_is(this->id(name));
			}
			/**
			 * \~russian
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 *
			 *
			 * \~english
			 * @brief Method of checking the presence of a callback function
			 * @param name name of the callback function
			 * @return     result of the check
			 *
			 * \~
			 */
			bool is(const string & name) const noexcept {
				// Выполняем првоерку существования функции обратного вызова
				return this->_is(this->id(name));
			}
			/**
			 * \~russian
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 *
			 *
			 * \~english
			 * @brief Method of checking the presence of a callback function
			 * @param name name of the callback function
			 * @return     result of the check
			 *
			 * \~
			 */
			bool is(const char * name) const noexcept {
				// Выполняем првоерку существования функции обратного вызова
				return (name != nullptr ? this->_is(this->id(name)) : false);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода проверки наличия функции обратного вызова
			 *
			 * @tparam T тип идентификатора функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of checking the presence of a callback function
			 * @tparam T type of the identifier of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return   результат проверки
			 *
			 * \~english
			 * @brief Method of checking the presence of a callback function
			 * @param id identifier of the callback function
			 * @return   result of the check
			 *
			 * \~
			 */
			bool is(const T id) const noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем првоерку существования функции обратного вызова
					return this->_is(static_cast <id_t> (id));
				// Возвращаем значение по умолчанию
				return false;
			}
		private:
			/**
			 * \~russian
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 *
			 * \~english
			 * @brief Method of removing a callback function
			 * @param id identifier of the callback function
			 *
			 * \~
			 */
			void _erase(const id_t id) noexcept {
				/**
				 * Если идентификатор функции не передан
				 */
				if(id == 0)
					// Выходим из функции
					return;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					{
						// Выполняем блокировку потока
						const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
						// Выполняем поиск существующей функции обратного вызова
						auto i = this->_callbacks.find(id);
						// Если функция существует
						if(i != this->_callbacks.end())
							// Удаляем функцию обратного вызова
							this->_callbacks.erase(i);
					}
					// Если системная функция обратного вызова установлена
					if(this->_callback != nullptr)
						// Выполняем системную функцию обратного вызова
						this->_callback(event_t::DEL, id, nullptr);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param name функция обратного вызова для удаления
			 *
			 *
			 * \~english
			 * @brief Method of removing a callback function
			 * @param name callback function to remove
			 *
			 * \~
			 */
			void erase(string_view name) noexcept {
				// Выполняем удаление функции обратного вызова
				this->_erase(this->id(name));
			}
			/**
			 * \~russian
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param name функция обратного вызова для удаления
			 *
			 *
			 * \~english
			 * @brief Method of removing a callback function
			 * @param name callback function to remove
			 *
			 * \~
			 */
			void erase(const string & name) noexcept {
				// Выполняем удаление функции обратного вызова
				this->_erase(this->id(name));
			}
			/**
			 * \~russian
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param name функция обратного вызова для удаления
			 *
			 *
			 * \~english
			 * @brief Method of removing a callback function
			 * @param name callback function to remove
			 *
			 * \~
			 */
			void erase(const char * name) noexcept {
				// Если название функции обратного вызова передано
				if(name != nullptr)
					// Выполняем удаление функции обратного вызова
					this->_erase(this->id(name));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода удаления функции обратного вызова
			 *
			 * @tparam T тип идентификатора функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of removing a callback function
			 * @tparam T type of the identifier of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 *
			 * \~english
			 * @brief Method of removing a callback function
			 * @param id identifier of the callback function
			 *
			 * \~
			 */
			void erase(const T id) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем удаление функции обратного вызова
					this->_erase(static_cast <id_t> (id));
			}
		private:
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param id1 идентификатор первой функции
			 * @param id2 идентификатор второй функции
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param id1 identifier of the first function
			 * @param id2 identifier of the second function
			 *
			 * \~
			 */
			void _swap(const id_t id1, const id_t id2) noexcept {
				/**
				 * Если идентификаторы функций не переданы
				 */
				if((id1 == 0) || (id2 == 0))
					// Выходим из функции
					return;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
					// Выполняем поиск первой функции
					auto i = this->_callbacks.find(id1);
					// Выполняем поиск второй функции
					auto j = this->_callbacks.find(id2);
					// Если функции обратных вызовов получены
					if((i != _callbacks.end()) && (j != _callbacks.end()))
						// Выполняем обмен функциями
						std::swap(i->second, j->second);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id1, id2), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param id1     идентификатор первой функции
			 * @param id2     идентификатор второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param id1     identifier of the first function
			 * @param id2     identifier of the second function
			 * @param storage storage of the functions the function should be obtained from
			 *
			 * \~
			 */
			void _swap(const id_t id1, const id_t id2, Callback & storage) noexcept {
				/**
				 * Если идентификаторы функций не переданы
				 */
				if((id1 == 0) || (id2 == 0) || storage.empty())
					// Выходим из функции
					return;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем безопасный захват блокировок текущего и стороннего контейнеров
					Callback::_dualLock(this->_mtx, storage._mtx, [&]() {
						// Выполняем поиск первой функции
						auto i = this->_callbacks.find(id1);
						// Выполняем поиск второй функции
						auto j = storage._callbacks.find(id2);
						// Если функции обратных вызовов получены
						if((i != this->_callbacks.end()) && (j != storage._callbacks.end()))
							// Выполняем обмен функциями
							std::swap(i->second, j->second);
					});
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id1, id2), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param storage storage of the functions the function should be obtained from
			 *
			 * \~
			 */
			void swap(Callback & storage) noexcept {
				// Если обмен выполняется с самим собой
				if(this == &storage)
					// Выходим из функции
					return;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем безопасный захват блокировок текущего и стороннего контейнеров
					Callback::_dualLock(this->_mtx, storage._mtx, [&]() {
						// Выполняем обмен функциями
						this->_callbacks.swap(storage._callbacks);
					});
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param name1 name of the first function
			 * @param name2 name of the second function
			 *
			 * \~
			 */
			void swap(string_view name1, string_view name2) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2));
			}
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param name1 name of the first function
			 * @param name2 name of the second function
			 *
			 * \~
			 */
			void swap(const string & name1, const string & name2) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2));
			}
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param name1 name of the first function
			 * @param name2 name of the second function
			 *
			 * \~
			 */
			void swap(const char * name1, const char * name2) noexcept {
				// Если названия переданы
				if((name1 != nullptr) && (name2 != nullptr))
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->id(name1), this->id(name2));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода обмена функциями
			 *
			 * @tparam T тип идентификатора функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of swapping the functions
			 * @tparam T type of the identifier of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param id1 идентификатор первой функции
			 * @param id2 идентификатор второй функции
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param id1 identifier of the first function
			 * @param id2 identifier of the second function
			 *
			 * \~
			 */
			void swap(const T id1, const T id2) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем обмен функциями обратного вызова
					this->_swap(static_cast <id_t> (id1), static_cast <id_t> (id2));
			}
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param name1   name of the first function
			 * @param name2   name of the second function
			 * @param storage storage of the functions the function should be obtained from
			 *
			 * \~
			 */
			void swap(string_view name1, string_view name2, Callback & storage) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2), storage);
			}
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param name1   name of the first function
			 * @param name2   name of the second function
			 * @param storage storage of the functions the function should be obtained from
			 *
			 * \~
			 */
			void swap(const string & name1, const string & name2, Callback & storage) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2), storage);
			}
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param name1   name of the first function
			 * @param name2   name of the second function
			 * @param storage storage of the functions the function should be obtained from
			 *
			 * \~
			 */
			void swap(const char * name1, const char * name2, Callback & storage) noexcept {
				// Если названия переданы
				if((name1 != nullptr) && (name2 != nullptr) && !storage.empty())
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->id(name1), this->id(name2), storage);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода обмена функциями
			 *
			 * @tparam T тип идентификатора функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of swapping the functions
			 * @tparam T type of the identifier of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод обмена функциями
			 *
			 * @param id1     идентификатор первой функции
			 * @param id2     идентификатор второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 *
			 * \~english
			 * @brief Method of swapping the functions
			 * @param id1     identifier of the first function
			 * @param id2     identifier of the second function
			 * @param storage storage of the functions the function should be obtained from
			 *
			 * \~
			 */
			void swap(const T id1, const T id2, Callback & storage) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем обмен функциями обратного вызова
					this->_swap(static_cast <id_t> (id1), static_cast <id_t> (id2), storage);
			}
		private:
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id      идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param id      identifier of the copied function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			id_t _set(const id_t id, const Callback & storage) noexcept {
				/**
				 * Если идентификатор функции не передан или внешний контейнер пустой
				 */
				if((id == 0) || storage.empty())
					// Выходим из функции
					return 0;
				// Переменная результата установки функции обратного вызова
				id_t result = 0;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем безопасный захват блокировок текущего и стороннего контейнеров
					Callback::_dualLock(this->_mtx, const_cast <Callback &> (storage)._mtx, [&]() {
						// Выполняем поиск функции обратного вызова
						auto i = storage._callbacks.find(id);
						// Если функция в внешнем хранилище найдена
						if(i != storage._callbacks.end()){
							// Выполняем поиск существующей функции обратного вызова
							auto j = this->_callbacks.find(id);
							// Если функция такая уже существует
							if(j != this->_callbacks.end()){
								// Устанавливаем новую функцию обратного вызова
								j->second = i->second;
								// Если системная функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем системную функцию обратного вызова
									this->_callback(event_t::SET, id, j->second);
							// Если функция ещё не существует
							} else {
								// Создаём новую функцию
								auto ret = this->_callbacks.emplace(id, i->second);
								// Если системная функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем системную функцию обратного вызова
									this->_callback(event_t::SET, id, ret.first->second);
							}
							// Запоминаем идентификатор callback
							result = id;
						}
					});
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Выводим результат установки функции обратного вызова
				return result;
			}
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id1     идентификатор копируемой функции
			 * @param id2     новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param id1     identifier of the copied function
			 * @param id2     new identifier of the obtained function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			id_t _set(const id_t id1, const id_t id2, const Callback & storage) noexcept {
				/**
				 * Если идентификаторы функций не передан или внешний контейнер пустой
				 */
				if((id1 == 0) || (id2 == 0) || storage.empty())
					// Выходим из функции
					return 0;
				// Переменная результата установки функции обратного вызова
				id_t result = 0;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем безопасный захват блокировок текущего и стороннего контейнеров
					Callback::_dualLock(this->_mtx, const_cast <Callback &> (storage)._mtx, [&]() {
						// Выполняем поиск указанной функции в переданном хранилище
						auto i = storage._callbacks.find(id1);
						// Если функция в хранилище получена
						if(i != storage._callbacks.end()){
							/**
							 * Копируем функцию обратного вызова локально, чтобы не зависеть от валидности
							 * итератора при возможном рехэше контейнера (актуально при storage == this)
							 */
							const fn_t callback = i->second;
							// Выполняем поиск существующей функции обратного вызова
							auto j = this->_callbacks.find(id2);
							// Если функция такая уже существует
							if(j != this->_callbacks.end()){
								// Устанавливаем новую функцию обратного вызова
								j->second = callback;
								// Если системная функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем системную функцию обратного вызова
									this->_callback(event_t::SET, id2, j->second);
							// Если функция ещё не существует
							} else {
								// Создаём новую функцию
								auto ret = this->_callbacks.emplace(id2, callback);
								// Если системная функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем системную функцию обратного вызова
									this->_callback(event_t::SET, id2, ret.first->second);
							}
							// Запоминаем идентификатор callback
							result = id2;
						}
					});
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id1, id2), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Выводим результат установки функции обратного вызова
				return result;
			}
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param id       идентификатор устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a callback function in its pure form
			 * @param id       identifier of the function being set
			 * @param callback callback function being set
			 * @return         identifier of the added callback function
			 *
			 * \~
			 */
			id_t _set(const id_t id, const fn_t & callback) noexcept {
				/**
				 * Если идентификатор функции не передан или внешний контейнер пустой
				 */
				if((id == 0) || (callback == nullptr))
					// Выходим из функции
					return 0;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
					// Выполняем поиск функции обратного вызова
					auto i = this->_callbacks.find(id);
					// Если функция найдена в списке
					if(i != this->_callbacks.end()){
						// Выполняем замену функции обратного вызова
						i->second = callback;
						// Если системная функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем системную функцию обратного вызова
							this->_callback(event_t::SET, id, i->second);
					// Если функция не найдена
					} else {
						// Выполняем установку функции обратного вызова
						auto ret = this->_callbacks.emplace(id, callback);
						// Если системная функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем системную функцию обратного вызова
							this->_callback(event_t::SET, id, ret.first->second);
					}
					// Возвращаем идентификатор callback
					return id;
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Выходим из функции
				return 0;
			}
		public:
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param name    name of the first function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(string_view name, const Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), storage) : 0);
			}
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param name    name of the first function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const string & name, const Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), storage) : 0);
			}
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param name    name of the first function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const char * name, const Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((name != nullptr) ? this->_set(this->id(name), storage) : 0);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода установки функции из одного хранилища в текущее
			 *
			 * @tparam T тип идентификатора функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of setting a function from one storage into the current one
			 * @tparam T type of the identifier of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id      идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param id      identifier of the copied function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const T id, const Callback & storage) noexcept -> id_t {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v<T>)
					// Выполняем установку функции обратного вызова
					return this->_set(static_cast <id_t> (id), storage);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param name1   name of the copied function
			 * @param name2   new name of the obtained function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(string_view name1, string_view name2, Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((!name1.empty() && !name2.empty()) ? this->_set(this->id(name1), this->id(name2), storage) : 0);
			}
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param name1   name of the copied function
			 * @param name2   new name of the obtained function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const string & name1, const string & name2, Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((!name1.empty() && !name2.empty()) ? this->_set(this->id(name1), this->id(name2), storage) : 0);
			}
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param name1   name of the copied function
			 * @param name2   new name of the obtained function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const char * name1, const char * name2, Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (((name1 != nullptr) && (name2 != nullptr)) ? this->_set(this->id(name1), this->id(name2), storage) : 0);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода установки функции из одного хранилища в текущее
			 *
			 * @tparam T тип идентификатора функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of setting a function from one storage into the current one
			 * @tparam T type of the identifier of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id1     идентификатор копируемой функции
			 * @param id2     новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a function from one storage into the current one
			 * @param id1     identifier of the copied function
			 * @param id2     new identifier of the obtained function
			 * @param storage storage of the functions the function should be obtained from
			 * @return        identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const T id1, const T id2, const Callback & storage) noexcept -> id_t {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем установку функции обратного вызова
					return this->_set(static_cast <id_t> (id1), static_cast <id_t> (id2), storage);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a callback function in its pure form
			 * @param name     name of the function being set
			 * @param callback callback function being set
			 * @return         identifier of the added callback function
			 *
			 * \~
			 */
			auto set(string_view name, const fn_t & callback) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), callback) : 0);
			}
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a callback function in its pure form
			 * @param name     name of the function being set
			 * @param callback callback function being set
			 * @return         identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const string & name, const fn_t & callback) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), callback) : 0);
			}
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a callback function in its pure form
			 * @param name     name of the function being set
			 * @param callback callback function being set
			 * @return         identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const char * name, const fn_t & callback) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((name != nullptr) ? this->_set(this->id(name), callback) : 0);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода установки функции обратного вызова в чистом виде
			 *
			 * @tparam T тип идентификатора функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of setting a callback function in its pure form
			 * @tparam T type of the identifier of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param id       идентификатор устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of setting a callback function in its pure form
			 * @param id       identifier of the function being set
			 * @param callback callback function being set
			 * @return         identifier of the added callback function
			 *
			 * \~
			 */
			auto set(const T id, const fn_t & callback) noexcept -> id_t {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем установку функции обратного вызова
					return this->_set(static_cast <id_t> (id), callback);
				// Возвращаем значение по умолчанию
				return 0;
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода получения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 * \~english
			 * @brief Template of the method of obtaining a callback function
			 * @tparam T type of the signature of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получения функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return   функция обратного вызова если существует
			 *
			 * \~english
			 * @brief Method of obtaining a callback function
			 * @param id identifier of the callback function
			 * @return   the callback function if it exists
			 *
			 * \~
			 */
			auto _get(const id_t id) const noexcept -> function <T> {
				// Если идентификатор функции не передан
				if(id == 0)
					// Возвращаем пустое значение
					return {};
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::SHARED);
					// Выполняем поиск фиункции обратного вызова
					auto i = this->_callbacks.find(id);
					// Если функция обратного вызова найдена
					if(i != this->_callbacks.end()){
						// Выполняем извлечение функции обратного вызова
						auto * callback = awh_cast <const BasicFunction <T> *>(i->second.get());
						// Если функция обратного вызова не содержит данных
						if((callback == nullptr) || (callback->fn == nullptr))
							// Возвращаем пустое значение
							return {};
						// Возвращаем запрашиваемую функцию обратного вызова
						return callback->fn;
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Возвращаем пустое значение
				return {};
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of getting a callback function
			 * @tparam T type of the signature of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of getting a callback function
			 * @param name name of the callback function
			 * @return     the requested callback function
			 *
			 * \~
			 */
			auto get(string_view name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (this->id(name));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of getting a callback function
			 * @tparam T type of the signature of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of getting a callback function
			 * @param name name of the callback function
			 * @return     the requested callback function
			 *
			 * \~
			 */
			auto get(const string & name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (this->id(name));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of getting a callback function
			 * @tparam T type of the signature of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of getting a callback function
			 * @param name name of the callback function
			 * @return     the requested callback function
			 *
			 * \~
			 */
			auto get(const char * name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return ((name != nullptr) ? this->_get <T> (this->id(name)) : function <T> {});
			}
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 *
			 * \~english
			 * @brief Template of the method of getting a callback function
			 * @tparam T type of the signature of the function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of getting a callback function
			 * @param id identifier of the callback function
			 * @return    the requested callback function
			 *
			 * \~
			 */
			auto get(const id_t id) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (id);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam A тип идентификатора функции
			 * @tparam B тип сигнатуры функции
			 *
			 * \~english
			 * @brief Template of the method of getting a callback function
			 * @tparam A type of the identifier of the function
			 * @tparam B type of the signature of the function
			 *
			 * \~
			 */
			template <typename A, typename B>
			/**
			 * \~russian
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of getting a callback function
			 * @param id identifier of the callback function
			 * @return    the requested callback function
			 *
			 * \~
			 */
			auto get(const A id) const noexcept -> function <B> {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <A> || std::is_enum_v <A>)
					// Выполняем получение функции обратного вызова
					return this->_get <B> (static_cast <id_t>(id));
				// Возвращаем значение по умолчанию
				return {};
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam T type of the callback function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id идентификатор функкции обратного вызова
			 * @param fn функция обратного вызова для добавления
			 * @return   идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param id identifier of the callback function
			 * @param fn callback function to add
			 * @return   identifier of the added callback function
			 *
			 * \~
			 */
			id_t _on(const id_t id, function <T> fn) noexcept {
				// Если идентификатор функции или сама функция обратного вызова не переданы
				if((id == 0) || (fn == nullptr))
					// Выходим из функции
					return 0;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
					// Выполняем поиск существующей функции обратного вызова
					auto i = this->_callbacks.find(id);
					// Если функция обратного вызова найдена
					if(i != this->_callbacks.end()){
						// Выполняем замену функции обратного вызова
						i->second = std::make_shared <BasicFunction <T>> (std::move(fn));
						// Если системная функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем системную функцию обратного вызова
							this->_callback(event_t::SET, id, i->second);
					// Если функция обратного вызова не найдена
					} else {
						// Выполняем установку новой функции обратного вызова
						auto ret = this->_callbacks.emplace(id, std::make_shared <BasicFunction <T>> (std::move(fn)));
						// Если системная функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем системную функцию обратного вызова
							this->_callback(event_t::SET, id, ret.first->second);
					}
					// Возвращаем идентификатор callback
					return id;
				/**
				 * Если возникает ошибка выделения памяти
				 */
				} catch(const bad_alloc &) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, "Memory allocation error");
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
					#endif
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Выходим из функции
				return 0;
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Func      callback function to set
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name     название функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param name     name of the callback function
			 * @param callback callback function to attach
			 * @param args     arguments of the callback function
			 * @return         identifier of the attached callback function
			 *
			 * \~
			 */
			id_t on(string_view name, Func && callback, Args &&... args) noexcept {
				// Если название функции обратного вызова не передано
				if(name.empty())
					// Выходим из функции
					return 0;
				// Формируем функцию обратного вызова для подключения
				std::function <Signature> fn = std::bind(std::forward <Func> (callback), std::forward <Args> (args)...);
				// Выполняем подключение функции обратного вызова (идентификатор получаем напрямую из string_view)
				return this->_on <Signature> (this->id(name), std::move(fn));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Func      callback function to set
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name     название функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param name     name of the callback function
			 * @param callback callback function to attach
			 * @param args     arguments of the callback function
			 * @return         identifier of the attached callback function
			 *
			 * \~
			 */
			id_t on(const string & name, Func && callback, Args &&... args) noexcept {
				// Выполняем подключение функции обратного вызова
				return (!name.empty() ? this->on <Signature> (name.data(), std::forward <Func> (callback), std::forward <Args> (args)...) : 0);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Func      callback function to set
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name     название функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param name     name of the callback function
			 * @param callback callback function to attach
			 * @param args     arguments of the callback function
			 * @return         identifier of the attached callback function
			 *
			 * \~
			 */
			id_t on(const char * name, Func && callback, Args &&... args) noexcept {
				// Если название функции обратного вызова не передано
				if(name == nullptr)
					// Выходим из функции
					return 0;
				// Формируем функцию обратного вызова для подключения
				std::function <Signature> fn = std::bind(std::forward <Func> (callback), std::forward <Args> (args)...);
				// Выполняем подключение функции обратного вызова
				return this->_on <Signature> (this->id(name), std::move(fn));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Func      callback function to set
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id       идентификатор функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param id       identifier of the callback function
			 * @param callback callback function to attach
			 * @param args     arguments of the callback function
			 * @return         identifier of the attached callback function
			 *
			 * \~
			 */
			id_t on(const id_t id, Func && callback, Args &&... args) noexcept {
				// Если идентификатор функции обратного вызова не передан
				if(id == 0)
					// Выходим из функции
					return 0;
				// Формируем функцию обратного вызова для подключения
				std::function <Signature> fn = std::bind(std::forward <Func> (callback), std::forward <Args> (args)...);
				// Выполняем подключение функции обратного вызова
				return this->_on <Signature> (id, std::move(fn));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A         тип идентификатора функции обратного вызова
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam A         type of the identifier of the callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Func      callback function to set
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename A, typename Signature, typename Func, typename... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id       идентификатор функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param id       identifier of the callback function
			 * @param callback callback function to attach
			 * @param args     arguments of the callback function
			 * @return         identifier of the attached callback function
			 *
			 * \~
			 */
			id_t on(const A id, Func && callback, Args &&... args) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <A> || std::is_enum_v <A>)
					// Выполняем подключение функции обратного вызова
					return this->on <Signature> (static_cast <id_t> (id), std::forward <Func> (callback), std::forward <Args> (args)...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam T type of the callback function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param name name of the callback function
			 * @param fn   callback function to add
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			id_t on(string_view name, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return (!name.empty() ? this->_on <T> (this->id(name), std::move(fn)) : 0);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam T type of the callback function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param name name of the callback function
			 * @param fn   callback function to add
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			id_t on(const string & name, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return (!name.empty() ? this->_on <T> (this->id(name), std::move(fn)) : 0);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam T type of the callback function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param name name of the callback function
			 * @param fn   callback function to add
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			id_t on(const char * name, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return ((name != nullptr) ? this->_on <T> (this->id(name), std::move(fn)) : 0);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam T type of the callback function
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id идентификатор функкции обратного вызова
			 * @param fn функция обратного вызова для добавления
			 * @return   идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param id identifier of the callback function
			 * @param fn callback function to add
			 * @return   identifier of the added callback function
			 *
			 * \~
			 */
			id_t on(const id_t id, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return this->_on <T> (id, std::move(fn));
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A тип идентификатора функции обратного вызова
			 * @tparam B тип функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the method of attaching a callback function
			 * @tparam A type of the identifier of the callback function
			 * @tparam B type of the callback function
			 *
			 * \~
			 */
			template <typename A, typename B>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id идентификатор функкции обратного вызова
			 * @param fn функция обратного вызова для добавления
			 * @return   идентификатор добавленной функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of attaching a callback function
			 * @param id identifier of the callback function
			 * @param fn callback function to add
			 * @return   identifier of the added callback function
			 *
			 * \~
			 */
			id_t on(const A id, function <B> fn) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <A> || std::is_enum_v <A>)
					// Выполняем подключение функции обратного вызова
					return this->_on <B> (static_cast <id_t> (id), std::move(fn));
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Метод установки функции обратного события на получения событий модуля
			 *
			 * @param callback функция обратного вызова для установки
			 *
			 * \~english
			 * @brief Method of setting the callback of the events for receiving the events of the module
			 * @param callback callback function to set
			 *
			 * \~
			 */
			void on(function <void (const event_t, const id_t, const fn_t &)> callback) noexcept {
				// Выполняем блокировку потока
				const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
				// Выполняем установку функции обратного вызова
				this->_callback = std::move(callback);
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of performing a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename... Args>
			/**
			 * \~russian
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param id   идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of performing a callback function
			 * @param id   identifier of the callback function
			 * @param args arguments of the callback function
			 * @return     result of performing the callback function
			 *
			 * \~
			 */
			auto _call(const id_t id, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Формируем тип данных результата выполнения функции обратного вызова
				using Result = std::invoke_result_t <std::function <Signature>, Args...>;
				// Если идентификатор функции обратного вызова не передан
				if(id == 0){
					// Если результат функции обратного вызова не возвращается
					if constexpr (std::is_void_v <Result>)
						// Завершаем работу функции обратного вызова
						return;
					// Возвращаем пустое значение callback
					else return Result{};
				}
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					/**
					 * Удерживаем владение функцией обратного вызова, чтобы она не была удалена
					 * во время её выполнения вне блокировки (в том числе при само-удалении из callback'а)
					 */
					fn_t holder;
					// Извлечённая функция обратного вызова запрашиваемой сигнатуры
					const BasicFunction <Signature> * callback = nullptr;
					{
						// Выполняем блокировку потока
						const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::SHARED);
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(id);
						// Если функция обратного вызова не найдена
						if(i == this->_callbacks.end()){
							// Если результат функции обратного вызова не возвращается
							if constexpr (std::is_void_v <Result>)
								// Завершаем работу функции обратного вызова
								return;
							// Возвращаем пустое значение callback
							else return Result{};
						}
						// Выполняем извлечение функции обратного вызова
						callback = awh_cast <const BasicFunction <Signature> *> (i->second.get());
						// Если функция обратного вызова не содержит данных
						if((callback == nullptr) || (callback->fn == nullptr)){
							// Если результат функции обратного вызова не возвращается
							if constexpr (std::is_void_v <Result>)
								// Завершаем работу функции обратного вызова
								return;
							// Возвращаем пустое значение callback
							else return Result{};
						}
						// Удерживаем владение функцией обратного вызова на время выполнения вне блокировки
						holder = i->second;
						// Если системная функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем системную функцию обратного вызова
							this->_callback(event_t::RUN, id, i->second);
					}
					/**
					 * Выполняем функцию обратного вызова уже вне блокировки: holder удерживает её владение,
					 * поэтому пользовательский код выполняется без удержания мьютекса (исключаем внешние дедлоки)
					 */
					// Если результат функции обратного вызова не возвращается
					if constexpr (std::is_void_v <Result>)
						// Выполняем функцию обратного вызова
						callback->fn(std::forward <Args> (args)...);
					// Выполняем функцию обратного вызова с возвратом результата
					else return callback->fn(std::forward <Args> (args)...);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
					// Если результат функции обратного вызова не возвращается
					if constexpr (std::is_void_v <Result>)
						// Завершаем работу функции обратного вызова
						return;
					// Возвращаем пустое значение callback
					else return Result{};
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Метод выполнения всех функций обратного вызова
			 *
			 * \~english
			 * @brief Method of performing all the callback functions
			 *
			 * \~
			 */
			void call() const noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					/**
					 * Снимаем копию списка функций обратного вызова. Снимок удерживает владение функциями
					 * (shared_ptr), поэтому их можно безопасно выполнять уже вне блокировки, а изменение
					 * контейнера из callback'а (добавление/удаление) не инвалидирует обход
					 */
					vector <std::pair <id_t, fn_t>> snapshot;
					// Локальная копия системной функции обратного вызова (читаем её под блокировкой)
					function <void (const event_t, const id_t, const fn_t &)> systemCallback;
					{
						// Выполняем блокировку потока
						const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::SHARED);
						// Формируем снимок контейнера функций обратного вызова
						snapshot.assign(this->_callbacks.begin(), this->_callbacks.end());
						// Сохраняем копию системной функции обратного вызова
						systemCallback = this->_callback;
					}
					/**
					 * Выполняем перебор всех функций обратного вызова в снимке контейнера (уже вне блокировки)
					 */
					for(const auto & [id, cb] : snapshot){
						// Если мы извлекли функцию обратного вызова
						if(auto * callback = awh_cast <const BasicFunction <void ()> *>(cb.get())){
							// Если функция обратного вызова установлена
							if(callback->fn != nullptr){
								// Если системная функция обратного вызова установлена
								if(systemCallback != nullptr)
									// Выполняем системную функцию обратного вызова
									systemCallback(event_t::RUN, id, cb);
								// Выполняем функцию обратного вызова
								callback->fn();
							}
						}
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of performing a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename... Args>
			/**
			 * \~russian
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of performing a callback function
			 * @param name name of the callback function
			 * @param args arguments of the callback function
			 * @return     result of performing the callback function
			 *
			 * \~
			 */
			auto call(string_view name, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Формируем тип данных результата выполнения функции обратного вызова
				using Result = std::invoke_result_t <std::function <Signature>, Args...>;
				// Если название функции обратного вызова не передано
				if(name.empty()){
					// Если результат функции обратного вызова не возвращается
					if constexpr (std::is_void_v <Result>)
						// Завершаем работу функции обратного вызова
						return;
					// Возвращаем пустое значение callback
					else return Result{};
				}
				// Выполняем функцию обратного вызова (идентификатор получаем напрямую из string_view)
				return this->_call <Signature> (this->id(name), std::forward <Args> (args)...);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of performing a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename... Args>
			/**
			 * \~russian
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of performing a callback function
			 * @param name name of the callback function
			 * @param args arguments of the callback function
			 * @return     result of performing the callback function
			 *
			 * \~
			 */
			auto call(const string & name, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Выполняем функцию обратного вызова
				return this->call <Signature> (name.c_str(), std::forward <Args> (args)...);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of performing a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename... Args>
			/**
			 * \~russian
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of performing a callback function
			 * @param name name of the callback function
			 * @param args arguments of the callback function
			 * @return     result of performing the callback function
			 *
			 * \~
			 */
			auto call(const char * name, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Формируем тип данных результата выполнения функции обратного вызова
				using Result = std::invoke_result_t <std::function <Signature>, Args...>;
				// Если название функции обратного вызова не передано
				if(name == nullptr){
					// Если результат функции обратного вызова не возвращается
					if constexpr (std::is_void_v <Result>)
						// Завершаем работу функции обратного вызова
						return;
					// Возвращаем пустое значение callback
					else return Result{};
				}
				// Выполняем функцию обратного вызова
				return this->_call <Signature> (this->id(name), std::forward <Args> (args)...);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Template of the method of performing a callback function
			 * @tparam Signature signature of the callback function
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename Signature, typename... Args>
			/**
			 * \~russian
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param id   идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of performing a callback function
			 * @param id   identifier of the callback function
			 * @param args arguments of the callback function
			 * @return     result of performing the callback function
			 *
			 * \~
			 */
			auto call(const id_t id, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Выполняем функцию обратного вызова
				return this->_call <Signature> (id, std::forward <Args> (args)...);
			}
			/**
			 * \~russian
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam A         тип идентификатора функции
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the method of performing a callback function
			 * @tparam A         type of the identifier of the function
			 * @tparam Signature signature of the callback function
			 * @tparam Args      arguments of the callback function
			 *
			 * \~
			 */
			template <typename A, typename Signature, typename... Args>
			/**
			 * \~russian
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param id   идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 *
			 * \~english
			 * @brief Method of performing a callback function
			 * @param id   identifier of the callback function
			 * @param args arguments of the callback function
			 * @return     result of performing the callback function
			 *
			 * \~
			 */
			auto call(const A id, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <A> || std::is_enum_v <A>)
					// Выполняем функцию обратного вызова
					return this->_call <Signature> (static_cast <id_t> (id), std::forward <Args> (args)...);
				// Если на вход мы получили какое-то другое значение
				else {
					// Формируем тип данных результата выполнения функции обратного вызова
					using Result = std::invoke_result_t <std::function <Signature>, Args...>;
					// Если результат функции обратного вызова не возвращается
					if constexpr (std::is_void_v <Result>)
						// Завершаем работу функции обратного вызова
						return;
					// Возвращаем пустое значение callback
					else return Result{};
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Метод получения конечного итератора
			 *
			 * @return конечный итератор
			 *
			 * \~english
			 * @brief Method of getting the end iterator
			 * @return end iterator
			 *
			 * \~
			 */
			iterator_t end() noexcept {
				// Возвращаем результат
				return iterator_t(this->_callbacks.end(), this->_log);
			}
			/**
			 * \~russian
			 * @brief Метод получение начального итератора
			 *
			 * @return начальный итератор
			 *
			 * \~english
			 * @brief Method of getting the begin iterator
			 * @return begin iterator
			 *
			 * \~
			 */
			iterator_t begin() noexcept {
				// Возвращаем результат
				return iterator_t(this->_callbacks.begin(), this->_log);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор перемещения контейнера функций обратного вызова
			 *
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        текущее значение объекта
			 *
			 * \~english
			 * @brief Move operator of the container of the callback functions
			 * @param storage storage of the functions the function should be obtained from
			 * @return        the current value of the object
			 *
			 * \~
			 */
			Callback & operator = (Callback && storage) noexcept {
				// Если перемещение выполняется из самого себя
				if(this == &storage)
					// Возвращаем значение текущего объекта
					return (* this);
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем безопасный захват блокировок текущего и стороннего контейнеров
					Callback::_dualLock(this->_mtx, storage._mtx, [&]() {
						// Выполняем перемещение функций обратного вызова
						this->_callbacks = std::move(storage._callbacks);
					});
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Возвращаем значение текущего объекта
				return (* this);
			}
			/**
			 * \~russian
			 * @brief Оператор копирование контейнера функций обратного вызова
			 *
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        текущее значение объекта
			 *
			 * \~english
			 * @brief Copy operator of the container of the callback functions
			 * @param storage storage of the functions the function should be obtained from
			 * @return        the current value of the object
			 *
			 * \~
			 */
			Callback & operator = (const Callback & storage) noexcept {
				// Если копирование выполняется из самого себя
				if(this == &storage)
					// Возвращаем значение текущего объекта
					return (* this);
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем безопасный захват блокировок текущего и стороннего контейнеров
					Callback::_dualLock(this->_mtx, const_cast <Callback &> (storage)._mtx, [&]() {
						// Выполняем копирование функций обратного вызова
						this->_callbacks = storage._callbacks;
					});
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Возвращаем значение текущего объекта
				return (* this);
			}
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Callback(const fmk_t * fmk, const log_t * log) noexcept :
			 _crypto(fmk, log), _fmk(fmk), _log(log) {
				// Деактивируем мьютекс на время инициализации
				this->_mtx.enabled = false;
			}
	};
	/**
	 * \~russian
	 * @brief Создаём более осознанный тип данных контейнера функций обратного вызова
	 *
	 * \~english
	 * @brief Create a more meaningful data type of the container of the callback functions
	 *
	 * \~
	 */
	using callback_t = Callback;
};

#endif // __AWH_CALLBACK__
