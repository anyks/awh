/**
 * @file: callback.hpp
 * @date: 2026-01-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля функций обратного вызова — класс Callback, реализующий типобезопасное хранилище
 *        колбэков произвольных сигнатур с адресацией по идентификатору или имени,
 *        итератором обхода и потокобезопасным вызовом
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс работы с функциями обратного вызова
	 *
	 */
	class Callback {
		public:
			/**
			 * @brief Основные события для функций обратного вызова
			 *
			 */
			enum class event_t : uint8_t {
				NONE = 0x00, // Событие не установленно
				SET  = 0x01, // Событие установки функции
				DEL  = 0x02, // Событие удаления функции
				RUN  = 0x03  // Событие запуска функции
			};
		private:
			/**
			 * @brief Структура базовой функции
			 *
			 */
			struct Function {
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Function() noexcept = default;
			};
			/**
			 * @brief Шаблон базовой функции
			 *
			 * @tparam A сигнатура функции
			 *
			 */
			template <typename A>
			/**
			 * @brief Структура базовой функции
			 *
			 */
			struct BasicFunction : Function {
				/**
				 * @brief Функция обратного вызова
				 *
				 */
				std::function <A> fn;
				/**
				 * @brief Конструктор
				 *
				 * @param fn функция обратного вызова для установки
				 *
				 */
				explicit BasicFunction(std::function <A> fn) noexcept : fn(std::move(fn)) {}
			};
		public:
			/**
			 * @brief Тип идентификатора события
			 *
			 */
			using id_t = uint32_t;
			/**
			 * @brief Создаём тип данных функции обратного вызова
			 *
			 */
			using fn_t = std::shared_ptr <Function>;
		public:
			/**
			 * @brief Итератор как вложенный класс
			 *
			 */
			typedef class Iterator {
				public:
					/**
					 * @brief Создаём необходимые нам типы данных
					 *
					 */
					using value_type        = fn_t;
					using pointer           = fn_t *;
					using reference         = fn_t &;
					using difference_type   = std::ptrdiff_t;
					using iterator_category = std::forward_iterator_tag;
				public:
					/**
					 * @brief Создаём тип данных итератора
					 *
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
					 * @brief Оператор извлечения указателя заголовка
					 *
					 * @return указатель заголовка
					 *
					 */
					pointer operator -> () const noexcept {
						// Возвращаем результат
						return &this->_it->second;
					}
					/**
					 * @brief Оператор разыменования заголовка
					 *
					 * @return значение заголовка
					 *
					 */
					reference operator * () const noexcept {
						// Возвращаем результат
						return this->_it->second;
					}
				public:
					/**
					 * @brief Оператор смещения вперед
					 *
					 * @return значение текущего итератора
					 *
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
					 * @brief Оператор постинкрементного смещения вперед
					 *
					 * @return значение итератора до смещения
					 *
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
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator == (const Iterator & other) const noexcept {
						// Возвращаем результат
						return (this->_it == other._it);
					}
					/**
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator != (const Iterator & other) const noexcept {
						// Возвращаем результат
						return (this->_it != other._it);
					}
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param it  итератор для установки
					 * @param log объект для работы с логами
					 *
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
			 * @brief Функция обратного вызова при получении события установки или удаления функции
			 *
			 * @param флаг типа события
			 * @param идентификатор функции
			 * @param функция обратного вызова в чистом виде
			 *
			 */
			std::function <void (const event_t, const id_t, const fn_t &)> _callback;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Шаблон метода безопасного захвата сразу двух объектов блокировки
			 *
			 * @tparam Fn тип исполняемого функционала под захваченными блокировками
			 *
			 */
			template <typename Fn>
			/**
			 * @brief Метод безопасного захвата сразу двух объектов блокировки
			 *
			 * @details Блокировки захватываются в детерминированном порядке (по адресу объекта),
			 *          что исключает взаимоблокировку (deadlock) при встречных операциях над двумя контейнерами.
			 *
			 * @param first  первый объект блокировки
			 * @param second второй объект блокировки
			 * @param fn     исполняемый функционал под захваченными блокировками
			 *
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
			 * @brief Метод генерации идентификатора функции
			 *
			 * @param name название функции для генерации идентификатора
			 * @return     сгенерированный идентификатор функции
			 *
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
			 * @brief Метод проверки на пустоту контейнера
			 *
			 * @return результат проверки
			 *
			 */
			bool empty() const noexcept {
				// Выполняем блокировку потока
				const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::SHARED);
				// Возвращаем результат проверки
				return this->_callbacks.empty();
			}
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 *
			 */
			void threadSafety(const bool mode) noexcept {
				// Устанавливаем режим безопасности потоков
				this->_mtx.enabled = mode;
			}
		public:
			/**
			 * @brief Метод получения дампа функций обратного вызова
			 *
			 * @return выводим созданный блок дампа контейнера
			 *
			 */
			const unordered_map <id_t, fn_t> & dump() const noexcept {
				// Формируем дамп функций обратного вызова
				return this->_callbacks;
			}
			/**
			 * @brief Метод установки дампа функций обратного вызова
			 *
			 * @param callbacks дамп данных функций обратного вызова
			 *
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
			 * @brief Метод очистки контейнера
			 *
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
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return   результат проверки
			 *
			 */
			bool _is(const id_t id) const noexcept {
				// Выполняем блокировку потока
				const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::SHARED);
				// Возвращаем результат проверки
				return ((id > 0) && (this->_callbacks.find(id) != this->_callbacks.end()));
			}
		public:
			/**
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 *
			 */
			bool is(string_view name) const noexcept {
				// Выполняем првоерку существования функции обратного вызова
				return this->_is(this->id(name));
			}
			/**
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 *
			 */
			bool is(const string & name) const noexcept {
				// Выполняем првоерку существования функции обратного вызова
				return this->_is(this->id(name));
			}
			/**
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 *
			 */
			bool is(const char * name) const noexcept {
				// Выполняем првоерку существования функции обратного вызова
				return (name != nullptr ? this->_is(this->id(name)) : false);
			}
			/**
			 * @brief Шаблон метода проверки наличия функции обратного вызова
			 *
			 * @tparam T тип идентификатора функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод проверки наличия функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return   результат проверки
			 *
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
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 *
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
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param name функция обратного вызова для удаления
			 *
			 */
			void erase(string_view name) noexcept {
				// Выполняем удаление функции обратного вызова
				this->_erase(this->id(name));
			}
			/**
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param name функция обратного вызова для удаления
			 *
			 */
			void erase(const string & name) noexcept {
				// Выполняем удаление функции обратного вызова
				this->_erase(this->id(name));
			}
			/**
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param name функция обратного вызова для удаления
			 *
			 */
			void erase(const char * name) noexcept {
				// Если название функции обратного вызова передано
				if(name != nullptr)
					// Выполняем удаление функции обратного вызова
					this->_erase(this->id(name));
			}
			/**
			 * @brief Шаблон метода удаления функции обратного вызова
			 *
			 * @tparam T тип идентификатора функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод удаления функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 *
			 */
			void erase(const T id) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем удаление функции обратного вызова
					this->_erase(static_cast <id_t> (id));
			}
		private:
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param id1 идентификатор первой функции
			 * @param id2 идентификатор второй функции
			 *
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
			 * @brief Метод обмена функциями
			 *
			 * @param id1     идентификатор первой функции
			 * @param id2     идентификатор второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
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
			 * @brief Метод обмена функциями
			 *
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
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
			 * @brief Метод обмена функциями
			 *
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 *
			 */
			void swap(string_view name1, string_view name2) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2));
			}
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 *
			 */
			void swap(const string & name1, const string & name2) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2));
			}
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 *
			 */
			void swap(const char * name1, const char * name2) noexcept {
				// Если названия переданы
				if((name1 != nullptr) && (name2 != nullptr))
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->id(name1), this->id(name2));
			}
			/**
			 * @brief Шаблон метода обмена функциями
			 *
			 * @tparam T тип идентификатора функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param id1 идентификатор первой функции
			 * @param id2 идентификатор второй функции
			 *
			 */
			void swap(const T id1, const T id2) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем обмен функциями обратного вызова
					this->_swap(static_cast <id_t> (id1), static_cast <id_t> (id2));
			}
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 */
			void swap(string_view name1, string_view name2, Callback & storage) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2), storage);
			}
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 */
			void swap(const string & name1, const string & name2, Callback & storage) noexcept {
				// Выполняем обмен функциями обратного вызова
				this->_swap(this->id(name1), this->id(name2), storage);
			}
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 */
			void swap(const char * name1, const char * name2, Callback & storage) noexcept {
				// Если названия переданы
				if((name1 != nullptr) && (name2 != nullptr) && !storage.empty())
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->id(name1), this->id(name2), storage);
			}
			/**
			 * @brief Шаблон метода обмена функциями
			 *
			 * @tparam T тип идентификатора функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод обмена функциями
			 *
			 * @param id1     идентификатор первой функции
			 * @param id2     идентификатор второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 *
			 */
			void swap(const T id1, const T id2, Callback & storage) noexcept {
				// Если мы получили на вход число
				if constexpr (std::is_arithmetic_v <T> || std::is_enum_v <T>)
					// Выполняем обмен функциями обратного вызова
					this->_swap(static_cast <id_t> (id1), static_cast <id_t> (id2), storage);
			}
		private:
			/**
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id      идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id1     идентификатор копируемой функции
			 * @param id2     новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param id       идентификатор устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(string_view name, const Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), storage) : 0);
			}
			/**
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(const string & name, const Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), storage) : 0);
			}
			/**
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(const char * name, const Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((name != nullptr) ? this->_set(this->id(name), storage) : 0);
			}
			/**
			 * @brief Шаблон метода установки функции из одного хранилища в текущее
			 *
			 * @tparam T тип идентификатора функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id      идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(string_view name1, string_view name2, Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((!name1.empty() && !name2.empty()) ? this->_set(this->id(name1), this->id(name2), storage) : 0);
			}
			/**
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(const string & name1, const string & name2, Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((!name1.empty() && !name2.empty()) ? this->_set(this->id(name1), this->id(name2), storage) : 0);
			}
			/**
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(const char * name1, const char * name2, Callback & storage) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (((name1 != nullptr) && (name2 != nullptr)) ? this->_set(this->id(name1), this->id(name2), storage) : 0);
			}
			/**
			 * @brief Шаблон метода установки функции из одного хранилища в текущее
			 *
			 * @tparam T тип идентификатора функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод установки функции из одного хранилища в текущее
			 *
			 * @param id1     идентификатор копируемой функции
			 * @param id2     новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(string_view name, const fn_t & callback) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), callback) : 0);
			}
			/**
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(const string & name, const fn_t & callback) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return (!name.empty() ? this->_set(this->id(name), callback) : 0);
			}
			/**
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
			 */
			auto set(const char * name, const fn_t & callback) noexcept -> id_t {
				// Выполняем установку функции обратного вызова
				return ((name != nullptr) ? this->_set(this->id(name), callback) : 0);
			}
			/**
			 * @brief Шаблон метода установки функции обратного вызова в чистом виде
			 *
			 * @tparam T тип идентификатора функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод установки функции обратного вызова в чистом виде
			 *
			 * @param id       идентификатор устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Шаблон метода получения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return   функция обратного вызова если существует
			 *
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
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 *
			 */
			auto get(string_view name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (this->id(name));
			}
			/**
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 *
			 */
			auto get(const string & name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (this->id(name));
			}
			/**
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 *
			 */
			auto get(const char * name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return ((name != nullptr) ? this->_get <T> (this->id(name)) : function <T> {});
			}
			/**
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam T тип сигнатуры функции
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 *
			 */
			auto get(const id_t id) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (id);
			}
			/**
			 * @brief Шаблон метода извлечения функции обратного вызова
			 *
			 * @tparam A тип идентификатора функции
			 * @tparam B тип сигнатуры функции
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод извлечения функции обратного вызова
			 *
			 * @param id идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id идентификатор функкции обратного вызова
			 * @param fn функция обратного вызова для добавления
			 * @return   идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name     название функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name     название функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
			 */
			id_t on(const string & name, Func && callback, Args &&... args) noexcept {
				// Выполняем подключение функции обратного вызова
				return (!name.empty() ? this->on <Signature> (name.data(), std::forward <Func> (callback), std::forward <Args> (args)...) : 0);
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name     название функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename Func, typename... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id       идентификатор функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A         тип идентификатора функции обратного вызова
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Func      функция обратного вызова для установки
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename A, typename Signature, typename Func, typename... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id       идентификатор функции обратного вызова
			 * @param callback функция обратного вызова для подключения
			 * @param args     аргументы фукнции обратного вызова
			 * @return         идентификатор подключённо функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			id_t on(string_view name, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return (!name.empty() ? this->_on <T> (this->id(name), std::move(fn)) : 0);
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			id_t on(const string & name, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return (!name.empty() ? this->_on <T> (this->id(name), std::move(fn)) : 0);
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 */
			id_t on(const char * name, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return ((name != nullptr) ? this->_on <T> (this->id(name), std::move(fn)) : 0);
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T тип функции обратного вызова
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id идентификатор функкции обратного вызова
			 * @param fn функция обратного вызова для добавления
			 * @return   идентификатор добавленной функции обратного вызова
			 *
			 */
			id_t on(const id_t id, function <T> fn) noexcept {
				// Выполняем подключение функции обратного вызова
				return this->_on <T> (id, std::move(fn));
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A тип идентификатора функции обратного вызова
			 * @tparam B тип функции обратного вызова
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param id идентификатор функкции обратного вызова
			 * @param fn функция обратного вызова для добавления
			 * @return   идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Метод установки функции обратного события на получения событий модуля
			 *
			 * @param callback функция обратного вызова для установки
			 *
			 */
			void on(function <void (const event_t, const id_t, const fn_t &)> callback) noexcept {
				// Выполняем блокировку потока
				const locker_t <std::recursive_mutex> lock(this->_mtx, locker_t <std::recursive_mutex>::mode_t::EXCLUSIVE);
				// Выполняем установку функции обратного вызова
				this->_callback = std::move(callback);
			}
		private:
			/**
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename... Args>
			/**
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param id   идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
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
			 * @brief Метод выполнения всех функций обратного вызова
			 *
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
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename... Args>
			/**
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
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
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename... Args>
			/**
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 */
			auto call(const string & name, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Выполняем функцию обратного вызова
				return this->call <Signature> (name.c_str(), std::forward <Args> (args)...);
			}
			/**
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename... Args>
			/**
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param name название функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
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
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename Signature, typename... Args>
			/**
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param id   идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
			 */
			auto call(const id_t id, Args &&... args) const noexcept -> std::invoke_result_t <std::function <Signature>, Args...> {
				// Выполняем функцию обратного вызова
				return this->_call <Signature> (id, std::forward <Args> (args)...);
			}
			/**
			 * @brief Шаблон метода выполнения финкции обратного вызова
			 *
			 * @tparam A         тип идентификатора функции
			 * @tparam Signature сигнатура функции обратного вызова
			 * @tparam Args      аргументы функции обратного вызова
			 *
			 */
			template <typename A, typename Signature, typename... Args>
			/**
			 * @brief Метод выполнения функции обратного вызова
			 *
			 * @param id   идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции обратного вызова
			 *
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
			 * @brief Метод получения конечного итератора
			 *
			 * @return конечный итератор
			 *
			 */
			iterator_t end() noexcept {
				// Возвращаем результат
				return iterator_t(this->_callbacks.end(), this->_log);
			}
			/**
			 * @brief Метод получение начального итератора
			 *
			 * @return начальный итератор
			 *
			 */
			iterator_t begin() noexcept {
				// Возвращаем результат
				return iterator_t(this->_callbacks.begin(), this->_log);
			}
		public:
			/**
			 * @brief Оператор перемещения контейнера функций обратного вызова
			 *
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        текущее значение объекта
			 *
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
			 * @brief Оператор копирование контейнера функций обратного вызова
			 *
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        текущее значение объекта
			 *
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
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Callback(const fmk_t * fmk, const log_t * log) noexcept :
			 _crypto(fmk, log), _fmk(fmk), _log(log) {
				// Деактивируем мьютекс на время инициализации
				this->_mtx.enabled = false;
			}
	};
	/**
	 * @brief Создаём более осознанный тип данных контейнера функций обратного вызова
	 *
	 */
	using callback_t = Callback;
};

#endif // __AWH_CALLBACK__
