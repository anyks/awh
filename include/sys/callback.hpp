/**
 * @file: callback.hpp
 * @date: 2025-06-25
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

#ifndef __AWH_CALLBACK__
#define __AWH_CALLBACK__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>
#include <tuple>
#include <memory>
#include <string>
#include <cstring>
#include <functional>

/**
 * Наши модули
 */
#include <sys/log.hpp>
#include <cityhash/city.h>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Callback Класс работы с функциями обратного вызова
	 */
	typedef class Callback {
		public:
			/**
			 * Основные события для функций обратного вызова
			 */
			enum class event_t : uint8_t {
				NONE = 0x00, // Событие не установленно
				SET  = 0x01, // Событие установки функции
				DEL  = 0x02, // Событие удаления функции
				RUN  = 0x03  // Событие запуска функции
			};
		private:
			/**
			 * Function Структура базовой функции
			 */
			struct Function {
				/**
				 * ~Function Деструктор
				 */
				virtual ~Function() noexcept {}
			};
			/**
			 * @tparam Шаблон базовой функции
			 * @param A сигнатура функции
			 */
			template <typename A>
			/**
			 * BasicFunction Структура базовой функции
			 */
			struct BasicFunction : Function {
				// Функция обратного вызова
				function <A> fn;
				/**
				 * BasicFunction Конструктор
				 * @param fn функция обратного вызова для установки
				 */
				BasicFunction(function <A> fn) noexcept : fn(fn) {}
			};
		public:
			/**
			 * Создаём тип данных функции обратного вызова
			 */
			typedef std::shared_ptr <Function> fn_t;
		private:
			// Мютекс для блокировки основного потока
			std::mutex _mtx;
		private:
			// Хранилище распределения по названиям
			std::map <uint64_t, fn_t> _callbacks;
		private:
			/**
			 * Функция обратного вызова при получении события установки или удаления функции
			 * @param флаг типа события
			 * @param идентификатор функции
			 * @param функция обратного вызова в чистом виде
			 */
			function <void (const event_t, const uint64_t, const fn_t &)> _callback;
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * fid Метод генерации идентификатора функции
			 * @param name название функции для генерации идентификатора
			 * @return     сгенерированный идентификатор функции
			 */
			uint64_t fid(const string & name) const noexcept {
				// Результат работы функции
				uint64_t result = 0;
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Если размер имени умещается в 8 байт
					if(name.size() <= 8)
						// Выполняем копирование данных имени
						::memcpy(&result, name.data(), name.size());
					// Получаем идентификатор обратного вызова
					else return ::CityHash64(name.c_str(), name.size());
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Выводим полученный результат
				return result;
			}
		public:
			/**
			 * empty Метод проверки на пустоту контейнера
			 * @return результат проверки
			 */
			bool empty() const noexcept {
				// Выводим результат проверки
				return this->_callbacks.empty();
			}
		public:
			/**
			 * dump Метод получения дампа функций обратного вызова
			 * @return выводим созданный блок дампа контейнера
			 */
			const std::map <uint64_t, fn_t> & dump() const noexcept {
				// Выводим дамп функций обратного вызова
				return this->_callbacks;
			}
			/**
			 * dump Метод установки дампа функций обратного вызова
			 * @param callbacks дамп данных функций обратного вызова
			 */
			void dump(const std::map <uint64_t, fn_t> & callbacks) noexcept {
				// Если данные функций обратного вызова переданы
				if(!callbacks.empty()){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Устанавливаем новые данные функциий обратного вызова
						this->_callbacks = callbacks;
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
			}
		public:
			/**
			 * clear Метод очистки контейнера
			 */
			void clear() noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем очистку списка функций обратного вызова
					this->_callbacks.clear();
					// Выполняем очистку выделенной памяти для списка функций обратного вызова
					std::map <uint64_t, fn_t> ().swap(this->_callbacks);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
		private:
			/**
			 * _is Метод проверки наличия функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    результат проверки
			 */
			bool _is(const uint64_t fid) const noexcept {
				// Выводим результат проверки
				return ((fid > 0) && !this->_callbacks.empty() && (this->_callbacks.find(fid) != this->_callbacks.end()));
			}
		public:
			/**
			 * is Метод проверки наличия функции обратного вызова
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 */
			bool is(const char * name) const noexcept {
				// Если название передано
				if(name != nullptr)
					// Выполняем првоерку существования функции обратного вызова
					return this->_is(this->fid(name));
				// Выводим значение по умолчанию
				return false;
			}
			/**
			 * is Метод проверки наличия функции обратного вызова
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 */
			bool is(const string & name) const noexcept {
				// Выполняем првоерку существования функции обратного вызова
				return this->_is(this->fid(name));
			}
			/**
			 * @tparam Шаблон метода проверки наличия функции обратного вызова
			 * @param T тип идентификатора функции
			 */
			template <typename T>
			/**
			 * is Метод проверки наличия функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    результат проверки
			 */
			bool is(const T fid) const noexcept {
				// Если мы получили на вход число
				if(is_integral_v <T> || is_enum_v <T> || is_floating_point_v <T>)
					// Выполняем првоерку существования функции обратного вызова
					return this->_is(static_cast <uint64_t> (fid));
				// Выводим результат по умолчанию
				return false;
			}
		private:
			/**
			 * _erase Метод удаления функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 */
			void _erase(const uint64_t fid) noexcept {
				// Если название функции обратного вызова передано
				if(fid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск существующей функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция существует
						if(i != this->_callbacks.end()){
							// Выполняем блокировку потока
							const lock_guard <std::mutex> lock(this->_mtx);
							// Удаляем функцию обратного вызова
							this->_callbacks.erase(i);
						}
						// Если функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем функцию обратного вызова
							apply(this->_callback, make_tuple(event_t::DEL, fid, nullptr));
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
			}
		public:
			/**
			 * erase Метод удаления функции обратного вызова
			 * @param name функция обратного вызова для удаления
			 */
			void erase(const char * name) noexcept {
				// Если название функции обратного вызова передано
				if(name != nullptr)
					// Выполняем удаление функции обратного вызова
					this->_erase(this->fid(name));
			}
			/**
			 * erase Метод удаления функции обратного вызова
			 * @param name функция обратного вызова для удаления
			 */
			void erase(const string & name) noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty())
					// Выполняем удаление функции обратного вызова
					this->_erase(this->fid(name));
			}
			/**
			 * @tparam Шаблон метода удаления функции обратного вызова
			 * @param T тип идентификатора функции
			 */
			template <typename T>
			/**
			 * erase Метод удаления функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 */
			void erase(const T fid) noexcept {
				// Если мы получили на вход число
				if(is_integral_v <T> || is_enum_v <T> || is_floating_point_v <T>)
					// Выполняем удаление функции обратного вызова
					this->_erase(static_cast <uint64_t> (fid));
			}
		public:
			/**
			 * swap Метод обмена функциями
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void swap(Callback & storage) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока основным мютексом
					const lock_guard <std::mutex> lock1(this->_mtx);
					// Выполняем блокировку потока сторонним мютексом
					const lock_guard <std::mutex> lock2(storage._mtx);
					// Выполняем обмен функций названий
					this->_callbacks.swap(storage._callbacks);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
		private:
			/**
			 * _swap Метод обмена функциями
			 * @param fid1 идентификатор первой функции
			 * @param fid2 идентификатор второй функции
			 */
			void _swap(const uint64_t fid1, const uint64_t fid2) noexcept {
				// Если идентификаторы переданы
				if((fid1 > 0) && (fid2 > 0)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск первой функции
						auto i = this->_callbacks.find(fid1);
						// Если функция получена
						if(i != this->_callbacks.end()){
							// Выполняем блокировку потока
							const lock_guard <std::mutex> lock(this->_mtx);
							// Получаем первую функцию обратного вызова
							auto callback = std::move(i->second);
							// Выполняем поиск второй функции
							auto j = this->_callbacks.find(fid2);
							// Если функция получена
							if(j != this->_callbacks.end()){
								// Выполняем замену первой функции
								i->second = std::move(j->second);
								// Выполняем замену второй функции
								j->second = std::move(callback);
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid1, fid2), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
			}
		public:
			/**
			 * swap Метод обмена функциями
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 */
			void swap(const char * name1, const char * name2) noexcept {
				// Если названия переданы
				if((name1 != nullptr) && (name2 != nullptr))
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->fid(name1), this->fid(name2));
			}
			/**
			 * swap Метод обмена функциями
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 */
			void swap(const string & name1, const string & name2) noexcept {
				// Если названия переданы
				if(!name1.empty() && !name2.empty())
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->fid(name1), this->fid(name2));
			}
			/**
			 * @tparam Шаблон метода обмена функциями
			 * @param T тип идентификатора функции
			 */
			template <typename T>
			/**
			 * swap Метод обмена функциями
			 * @param fid1 идентификатор первой функции
			 * @param fid2 идентификатор второй функции
			 */
			void swap(const T fid1, const T fid2) noexcept {
				// Если мы получили на вход число
				if(is_integral_v <T> || is_enum_v <T> || is_floating_point_v <T>)
					// Выполняем обмен функциями обратного вызова
					this->_swap(static_cast <uint64_t> (fid1), static_cast <uint64_t> (fid2));
			}
		private:
			/**
			 * _swap Метод обмена функциями
			 * @param fid1    идентификатор первой функции
			 * @param fid2    идентификатор второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void _swap(const uint64_t fid1, const uint64_t fid2, Callback & storage) noexcept {
				// Если идентификаторы переданы
				if((fid1 > 0) && (fid2 > 0) && !storage.empty()){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск первой функции
						auto i = this->_callbacks.find(fid1);
						// Если функция получена
						if(i != this->_callbacks.end()){
							// Выполняем блокировку потока
							const lock_guard <std::mutex> lock(this->_mtx);
							// Получаем первую функцию обратного вызова
							auto callback = std::move(i->second);
							// Выполняем поиск второй функции
							auto j = storage._callbacks.find(fid2);
							// Если функция получена
							if(j != storage._callbacks.end()){
								// Выполняем замену первой функции
								i->second = std::move(j->second);
								// Выполняем замену второй функции
								j->second = std::move(callback);
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid1, fid2), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
			}
		public:
			/**
			 * swap Метод обмена функциями
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void swap(const char * name1, const char * name2, Callback & storage) noexcept {
				// Если названия переданы
				if((name1 != nullptr) && (name2 != nullptr) && !storage.empty())
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->fid(name1), this->fid(name2), storage);
			}
			/**
			 * swap Метод обмена функциями
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void swap(const string & name1, const string & name2, Callback & storage) noexcept {
				// Если названия переданы
				if(!name1.empty() && !name2.empty() && !storage.empty())
					// Выполняем обмен функциями обратного вызова
					this->_swap(this->fid(name1), this->fid(name2), storage);
			}
			/**
			 * @tparam Шаблон метода обмена функциями
			 * @param T тип идентификатора функции
			 */
			template <typename T>
			/**
			 * swap Метод обмена функциями
			 * @param fid1    идентификатор первой функции
			 * @param fid2    идентификатор второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void swap(const T fid1, const T fid2, Callback & storage) noexcept {
				// Если мы получили на вход число
				if(!storage.empty() && (is_integral_v <T> || is_enum_v <T> || is_floating_point_v <T>))
					// Выполняем обмен функциями обратного вызова
					this->_swap(static_cast <uint64_t> (fid1), static_cast <uint64_t> (fid2), storage);
			}
		private:
			/**
			 * _set Метод установки функции из одного хранилища в текущее
			 * @param fid     идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto _set(const uint64_t fid, const Callback & storage) noexcept -> uint64_t {
				// Если указанная функция существует
				if(!storage._callbacks.empty() && storage.is(fid)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск указанной функции в переданном хранилище
						auto i = storage._callbacks.find(fid);
						// Если функция в хранилище получена
						if(i != storage._callbacks.end()){
							// Выполняем поиск существующей функции обратного вызова
							auto j = this->_callbacks.find(fid);
							// Если функция такая уже существует
							if(j != this->_callbacks.end()){
								// Выполняем блокировку потока
								this->_mtx.lock();
								// Устанавливаем новую функцию обратного вызова
								j->second = i->second;
								// Выполняем блокировку потока
								this->_mtx.unlock();
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем функцию обратного вызова
									apply(this->_callback, make_tuple(event_t::SET, fid, j->second));
								// Выводим идентификатор обратной функции
								return i->first;
							// Если функция ещё не существует
							} else {
								// Выполняем блокировку потока
								this->_mtx.lock();
								// Создаём новую функцию
								auto ret = this->_callbacks.emplace(fid, i->second);
								// Выполняем блокировку потока
								this->_mtx.unlock();
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем функцию обратного вызова
									apply(this->_callback, make_tuple(event_t::SET, fid, ret.first->second));
								// Выводим идентификатор обратной функции
								return ret.first->first;
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto set(const char * name, const Callback & storage) noexcept -> uint64_t {
				// Если название функции обратного вызова передано
				if((name != nullptr) && !storage.empty())
					// Выполняем установку функции обратного вызова
					return this->_set(this->fid(name), storage);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param name    название первой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto set(const string & name, const Callback & storage) noexcept -> uint64_t {
				// Если название функции обратного вызова передано
				if(!name.empty() && !storage.empty())
					// Выполняем установку функции обратного вызова
					return this->_set(this->fid(name), storage);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода установки функции из одного хранилища в текущее
			 * @param T тип идентификатора функции
			 */
			template <typename T>
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param fid     идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto set(const T fid, const Callback & storage) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(!storage.empty() && (is_integral_v <T> || is_enum_v <T> || is_floating_point_v <T>))
					// Выполняем установку функции обратного вызова
					return this->_set(static_cast <uint64_t> (fid), storage);
				// Выводим результат по умолчанию
				return 0;
			}
		private:
			/**
			 * _set Метод установки функции из одного хранилища в текущее
			 * @param fid1    идентификатор копируемой функции
			 * @param fid2    новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto _set(const uint64_t fid1, const uint64_t fid2, const Callback & storage) noexcept -> uint64_t {
				// Если указанная функция существует
				if(!storage._callbacks.empty() && storage.is(fid1)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск указанной функции в переданном хранилище
						auto i = storage._callbacks.find(fid1);
						// Если функция в хранилище получена
						if(i != storage._callbacks.end()){
							// Выполняем поиск существующей функции обратного вызова
							auto j = this->_callbacks.find(fid2);
							// Если функция такая уже существует
							if(j != this->_callbacks.end()){
								// Выполняем блокировку потока
								this->_mtx.lock();
								// Устанавливаем новую функцию обратного вызова
								j->second = i->second;
								// Выполняем блокировку потока
								this->_mtx.unlock();
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем функцию обратного вызова
									apply(this->_callback, make_tuple(event_t::SET, fid2, j->second));
								// Выводим идентификатор обратной функции
								return i->first;
							// Если функция ещё не существует
							} else {
								// Выполняем блокировку потока
								this->_mtx.lock();
								// Создаём новую функцию
								auto ret = this->_callbacks.emplace(fid2, i->second);
								// Выполняем блокировку потока
								this->_mtx.unlock();
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr)
									// Выполняем функцию обратного вызова
									apply(this->_callback, make_tuple(event_t::SET, fid2, ret.first->second));
								// Выводим идентификатор обратной функции
								return ret.first->first;
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid1, fid2), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto set(const char * name1, const char * name2, Callback & storage) noexcept -> uint64_t {
				// Если названия переданы
				if((name1 != nullptr) && (name2 != nullptr) && !storage.empty())
					// Выполняем установку функции обратного вызова
					return this->_set(this->fid(name1), this->fid(name2), storage);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto set(const string & name1, const string & name2, Callback & storage) noexcept -> uint64_t {
				// Если названия переданы
				if(!name1.empty() && !name2.empty() && !storage.empty())
					// Выполняем установку функции обратного вызова
					return this->_set(this->fid(name1), this->fid(name2), storage);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода установки функции из одного хранилища в текущее
			 * @param T тип идентификатора функции
			 */
			template <typename T>
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param fid1    идентификатор копируемой функции
			 * @param fid2    новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 * @return        идентификатор добавленной функции обратного вызова
			 */
			auto set(const T fid1, const T fid2, const Callback & storage) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(!storage.empty() && (is_integral_v <T> || is_enum_v <T> || is_floating_point_v <T>))
					// Выполняем установку функции обратного вызова
					return this->_set(static_cast <uint64_t> (fid1), static_cast <uint64_t> (fid2), storage);
				// Выводим результат по умолчанию
				return 0;
			}
		private:
			/**
			 * _set Метод установки функции обратного вызова в чистом виде
			 * @param fid      идентификатор устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 */
			auto _set(const uint64_t fid, const fn_t & callback) noexcept -> uint64_t {
				// Если параметры функции обратного вызова переданы
				if((fid > 0) && (callback != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция найдена в списке
						if(i != this->_callbacks.end()){
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем замену функции обратного вызова
							i->second = callback;
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, i->second));
							// Выводим идентификатор обратной функции
							return i->first;
						// Если функция не найдена
						} else {
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем установку функции обратного вызова
							auto ret = this->_callbacks.emplace(fid, callback);
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, ret.first->second));
							// Выводим идентификатор обратной функции
							return ret.first->first;
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * set Метод установки функции обратного вызова в чистом виде
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 */
			auto set(const char * name, const fn_t & callback) noexcept -> uint64_t {
				// Если название функции обратного вызова передано
				if((name != nullptr) && (callback != nullptr))
					// Выполняем установку функции обратного вызова
					return this->_set(this->fid(name), callback);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * set Метод установки функции обратного вызова в чистом виде
			 * @param name     название устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 */
			auto set(const string & name, const fn_t & callback) noexcept -> uint64_t {
				// Если название функции обратного вызова передано
				if(!name.empty() && (callback != nullptr))
					// Выполняем установку функции обратного вызова
					return this->_set(this->fid(name), callback);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода установки функции обратного вызова в чистом виде
			 * @param T тип идентификатора функции
			 */
			template <typename T>
			/**
			 * set Метод установки функции обратного вызова в чистом виде
			 * @param fid      идентификатор устанавливаемой функции
			 * @param callback устанавливаемая функция обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 */
			auto set(const T fid, const fn_t & callback) noexcept -> uint64_t {
				// Если мы получили на вход число
				if((callback != nullptr) && (is_integral_v <T> || is_enum_v <T> || is_floating_point_v <T>))
					// Выполняем установку функции обратного вызова
					return this->_set(static_cast <uint64_t> (fid), callback);
				// Выводим результат по умолчанию
				return 0;
			}
		private:
			/**
			 * _get Метод получения функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    функция обратного вызова если существует
			 */
			auto _get(const uint64_t fid) const noexcept -> const fn_t & {
				// Если идентификатор функции передан
				if(fid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция найдена в списке
						if(i != this->_callbacks.end())
							// Выводим найденную функцию обратного вызова
							return i->second;
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return nullptr;
			}
		public:
			/**
			 * get Метод извлечения функции обратного вызова
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 */
			auto get(const char * name) const noexcept -> const fn_t & {
				// Выполняем получение функции обратного вызова
				return this->_get(this->fid(name));
			}
			/**
			 * get Метод извлечения функции обратного вызова
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 */
			auto get(const string & name) const noexcept -> const fn_t & {
				// Выполняем получение функции обратного вызова
				return this->_get(this->fid(name));
			}
			/**
			 * get Метод извлечения функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 */
			auto get(const uint64_t fid) const noexcept -> const fn_t & {
				// Выполняем получение функции обратного вызова
				return this->_get(fid);
			}
			/**
			 * @tparam Шаблон метода извлечения функции обратного вызова
			 * @param T сигнатура функции
			 */
			template <typename T>
			/**
			 * get Метод извлечения функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 */
			auto get(const T fid) const noexcept -> const fn_t & {
				// Выполняем получение функции обратного вызова
				return this->_get(static_cast <uint64_t> (fid));
			}
		private:
			/**
			 * @tparam Шаблон метода получения функции обратного вызова
			 * @param T сигнатура функции
			 */
			template <typename T>
			/**
			 * _get Метод получения функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 */
			auto _get(const uint64_t fid) const noexcept -> function <T> {
				// Если идентификатор функции передан
				if(fid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция обратного вызова найдена
						if(i != this->_callbacks.end())
							// Получаем функцию обратного вызова в нужном нам виде
							return static_cast <const BasicFunction <T> &> (* i->second).fn;
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return nullptr;
			}
		public:
			/**
			 * @tparam Шаблон метода получения функции обратного вызова
			 * @param T сигнатура функции
			 */
			template <typename T>
			/**
			 * get Метод получения функции обратного вызова
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 */
			auto get(const char * name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (this->fid(name));
			}
			/**
			 * @tparam Шаблон метода получения функции обратного вызова
			 * @param T сигнатура функции
			 */
			template <typename T>
			/**
			 * get Метод получения функции обратного вызова
			 * @param name название функкции обратного вызова
			 * @return     запрашиваемая функция обратного вызова
			 */
			auto get(const string & name) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (this->fid(name));
			}
			/**
			 * @tparam Шаблон метода получения функции обратного вызова
			 * @param T сигнатура функции
			 */
			template <typename T>
			/**
			 * get Метод получения функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 */
			auto get(const uint64_t fid) const noexcept -> function <T> {
				// Выполняем получение функции обратного вызова
				return this->_get <T> (fid);
			}
			/**
			 * @tparam Шаблон метода получения функции обратного вызова
			 * @param A тип идентификатора функции
			 * @param B тип функции обратного вызова
			 */
			template <typename A, typename B>
			/**
			 * get Метод получения функции обратного вызова
			 * @param fid идентификатор функции обратного вызова
			 * @return    запрашиваемая функция обратного вызова
			 */
			auto get(const A fid) const noexcept -> function <B> {
				// Выполняем получение функции обратного вызова
				return this->_get <B> (static_cast <uint64_t> (fid));
			}
		private:
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T тип функции обратного вызова
			 */
			template <typename T>
			/**
			 * _on Метод подключения финкции обратного вызова
			 * @param fid идентификатор функкции обратного вызова
			 * @param fn  функция обратного вызова для добавления
			 * @return    идентификатор добавленной функции обратного вызова
			 */
			auto _on(const uint64_t fid, function <T> fn) noexcept -> uint64_t {
				// Если идентификатор функции передан
				if(fid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция найдена в списке
						if(i != this->_callbacks.end()){
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем замену функции обратного вызова
							i->second = std::unique_ptr <Function> (new BasicFunction <T> (fn));
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, i->second));
							// Выводим идентификатор обратной функции
							return i->first;
						// Если функция не найдена
						} else {
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем установку функции обратного вызова
							auto ret = this->_callbacks.emplace(fid, std::unique_ptr <Function> (new BasicFunction <T> (fn)));
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, ret.first->second));
							// Выводим идентификатор обратной функции
							return ret.first->first;
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T тип функции обратного вызова
			 */
			template <typename T>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, function <T> fn) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_on <T> (this->fid(name), fn);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T тип функции обратного вызова
			 */
			template <typename T>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, function <T> fn) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_on <T> (this->fid(name), fn);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T тип функции обратного вызова
			 */
			template <typename T>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid идентификатор функкции обратного вызова
			 * @param fn  функция обратного вызова для добавления
			 * @return    идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint64_t fid, function <T> fn) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_on <T> (fid, fn);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param A тип идентификатора функции
			 * @param B тип функции обратного вызова
			 */
			template <typename A, typename B>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid идентификатор функкции обратного вызова
			 * @param fn  функция обратного вызова для добавления
			 * @return    идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, function <B> fn) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_on <B> (static_cast <uint64_t> (fid), fn);
				// Выводим результат по умолчанию
				return 0;
			}
		private:
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * _on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto _on(const uint64_t fid, function <T> fn, Args... args) noexcept -> uint64_t {
				// Если идентификатор функции передан
				if(fid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция найдена в списке
						if(i != this->_callbacks.end()){
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем замену функции обратного вызова
							i->second = std::unique_ptr <Function> (new BasicFunction <T> (std::bind(fn, args...)));
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, i->second));
							// Выводим идентификатор обратной функции
							return i->first;
						// Если функция не найдена
						} else {
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем установку функции обратного вызова
							auto ret = this->_callbacks.emplace(fid, std::unique_ptr <Function> (new BasicFunction <T> (std::bind(fn, args...))));
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, ret.first->second));
							// Выводим идентификатор обратной функции
							return ret.first->first;
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, function <T> fn, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_on <T> (this->fid(name), fn, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name название функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, function <T> fn, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_on <T> (this->fid(name), fn, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint64_t fid, function <T> fn, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_on <T> (fid, fn, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param A    тип идентификатора функции
			 * @param B    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param fn   функция обратного вызова для добавления
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, function <B> fn, Args... args) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_on <B> (static_cast <uint64_t> (fid), fn, args...);
				// Выводим результат по умолчанию
				return 0;
			}
		private:
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * _on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto _on(const uint64_t fid, Args... args) noexcept -> uint64_t {
				// Если идентификатор функции передан
				if(fid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция найдена в списке
						if(i != this->_callbacks.end()){
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем замену функции обратного вызова
							i->second = std::unique_ptr <Function> (new BasicFunction <T> (std::bind(args...)));
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, i->second));
							// Выводим идентификатор обратной функции
							return i->first;
						// Если функция не найдена
						} else {
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем установку функции обратного вызова
							auto ret = this->_callbacks.emplace(fid, std::unique_ptr <Function> (new BasicFunction <T> (std::bind(args...))));
							// Выполняем блокировку потока
							this->_mtx.unlock();
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::SET, fid, ret.first->second));
							// Выводим идентификатор обратной функции
							return ret.first->first;
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc &) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_on <T> (this->fid(name), args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_on <T> (this->fid(name), args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_on <T> (fid, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @tparam Шаблон метода подключения финкции обратного вызова
			 * @param A    тип идентификатора функции
			 * @param B    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * on Метод подключения финкции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, Args... args) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_on <B> (static_cast <uint64_t> (fid), args...);
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * on Метод установки функции обратного события на получения событий модуля
			 * @param callback функция обратного вызова для установки
			 */
			void on(function <void (const event_t, const uint64_t, const fn_t &)> callback) noexcept {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем установку функции обратного вызова
					this->_callback = callback;
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			}
		public:
			/**
			 * call Метод вызова всех функций обратного вызова
			 */
			void call() const noexcept {
				// Если функции обратного вызова существуют
				if(!this->_callbacks.empty()){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем переход по всему списку обратных функций
						for(auto & callback : this->_callbacks){
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::RUN, callback.first, callback.second));
							// Выполняем функцию обратного вызова
							std::apply(static_cast <const BasicFunction <void (void)> &> (* callback.second.get()).fn, make_tuple());
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
			}
		private:
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * _call Метод выполнения функции обратного вызова
			 * @param callback функция обратного вызова
			 * @param fid      идентификатор функкции обратного вызова
			 * @param args     аргументы функции обратного вызова
			 * @return         результат выполнения функции
			 */
			auto _call(const fn_t & callback, const uint64_t fid, Args... args) const noexcept -> typename function <T>::result_type {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Если функция обратного вызова установлена
					if(this->_callback != nullptr)
						// Выполняем функцию обратного вызова
						apply(this->_callback, make_tuple(event_t::RUN, fid, callback));
					// Выполняем функцию обратного вызова
					return (typename function <T>::result_type) std::apply(static_cast <const BasicFunction <T> &> (* callback.get()).fn, make_tuple(args...));
				/**
				 * Если возникает ошибка
				 */
				} catch(const bad_function_call & error) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
				// Выводим результат по умолчанию
				return (typename function <T>::result_type) typename function <T>::result_type();
			}
		public:
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param callback функция обратного вызова
			 * @param args     аргументы функции обратного вызова
			 * @return         результат выполнения функции
			 */
			auto call(const fn_t & callback, Args... args) const noexcept -> typename function <T>::result_type {
				// Выполняем функцию обратного вызова
				return this->_call <T> (callback, 0, args...);
			}
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param callback функция обратного вызова
			 * @param name     название функкции обратного вызова
			 * @param args     аргументы функции обратного вызова
			 * @return         результат выполнения функции
			 */
			auto call(const fn_t & callback, const char * name, Args... args) const noexcept -> typename function <T>::result_type {
				// Выполняем функцию обратного вызова
				return this->_call <T> (callback, this->fid(name), args...);
			}
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param callback функция обратного вызова
			 * @param name     название функкции обратного вызова
			 * @param args     аргументы функции обратного вызова
			 * @return         результат выполнения функции
			 */
			auto call(const fn_t & callback, const string & name, Args... args) const noexcept -> typename function <T>::result_type {
				// Выполняем функцию обратного вызова
				return this->_call <T> (callback, this->fid(name), args...);
			}
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param callback функция обратного вызова
			 * @param fid      идентификатор функкции обратного вызова
			 * @param args     аргументы функции обратного вызова
			 * @return         результат выполнения функции
			 */
			auto call(const fn_t & callback, const uint64_t fid, Args... args) const noexcept -> typename function <T>::result_type {
				// Выполняем функцию обратного вызова
				return this->_call <T> (callback, fid, args...);
			}
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param A    тип идентификатора функции
			 * @param B    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param callback функция обратного вызова
			 * @param fid      идентификатор функкции обратного вызова
			 * @param args     аргументы функции обратного вызова
			 * @return         идентификатор добавленной функции обратного вызова
			 */
			auto call(const fn_t & callback, const A fid, Args... args) noexcept -> typename function <B>::result_type {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_call <B> (callback, static_cast <uint64_t> (fid), args...);
				// Выводим результат по умолчанию
				return (typename function <B>::result_type) typename function <B>::result_type();
			}
		private:
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * _call Метод выполнения функции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции
			 */
			auto _call(const uint64_t fid, Args... args) const noexcept -> typename function <T>::result_type {
				// Если идентификатор функции передан
				if(fid > 0){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск функции обратного вызова
						auto i = this->_callbacks.find(fid);
						// Если функция найдена в списке
						if(i != this->_callbacks.end()){
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr)
								// Выполняем функцию обратного вызова
								apply(this->_callback, make_tuple(event_t::RUN, fid, i->second));
							// Выполняем функцию обратного вызова
							return (typename function <T>::result_type) std::apply(static_cast <const BasicFunction <T> &> (* i->second.get()).fn, make_tuple(args...));
						}
						// Выводим результат по умолчанию
						return (typename function <T>::result_type) typename function <T>::result_type();
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_function_call & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fid), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выводим результат по умолчанию
				return (typename function <T>::result_type) typename function <T>::result_type();
			}
		public:
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param name название функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции
			 */
			auto call(const char * name, Args... args) const noexcept -> typename function <T>::result_type {
				// Выполняем функцию обратного вызова
				return this->_call <T> (this->fid(name), args...);
			}
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param name название функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции
			 */
			auto call(const string & name, Args... args) const noexcept -> typename function <T>::result_type {
				// Выполняем функцию обратного вызова
				return this->_call <T> (this->fid(name), args...);
			}
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param T    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     результат выполнения функции
			 */
			auto call(const uint64_t fid, Args... args) const noexcept -> typename function <T>::result_type {
				// Выполняем функцию обратного вызова
				return this->_call <T> (fid, args...);
			}
			/**
			 * @tparam Шаблон метода выполнения функции обратного вызова
			 * @param A    тип идентификатора функции
			 * @param B    тип функции обратного вызова
			 * @param Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * call Метод выполнения функции обратного вызова
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto call(const A fid, Args... args) noexcept -> typename function <B>::result_type {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_call <B> (static_cast <uint64_t> (fid), args...);
				// Выводим результат по умолчанию
				return (typename function <B>::result_type) typename function <B>::result_type();
			}
		public:
			/**
			 * Оператор [=] присвоения функций обратного вызова
			 * @param storage хранилище функций откуда нужно получить функции
			 * @return        текущий объект
			 */
			Callback & operator = (const Callback & storage) noexcept {
				// Если функции обратного вызова установлены
				if(!storage._callbacks.empty()){
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполням установку функций обратного вызова
					this->_callbacks = storage._callbacks;
				}
				// Выводим результат
				return (* this);
			}
		public:
			/**
			 * Callback Конструктор
			 * @param log объект для работы с логами
			 */
			Callback(const log_t * log) noexcept : _callback(nullptr), _log(log) {}
	} callback_t;
};

#endif // __AWH_CALLBACK__
