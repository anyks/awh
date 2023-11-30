/**
 * @file: fn.hpp
 * @date: 2023-11-30
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_FUNCTION__
#define __AWH_FUNCTION__

/**
 * Стандартная библиотека
 */
#include <map>
#include <tuple>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <variant>
#include <iostream>
#include <functional>

/**
 * Наши модули
 */
#include <sys/idw.hpp>
#include <sys/log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * FN Прототип класса работы с функциями
	 */
	class FN;
	/**
	 * FN Класс работы с функциями
	 */
	typedef class FN {
		private:
			// Объект генерации идентификаторов
			idw_t _idw;
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
			 * Шаблон базовой функции
			 * @tparam A сигнатура функции
			 */
			template <typename A>
			/**
			 * BasicFunction Структура базовой функции
			 */
			struct BasicFunction : Function {
				// Функция обратного вызова
				std::function <A> fn;
				/**
				 * BasicFunction Конструктор
				 * @param fn функция обратного вызова для установки
				 */
				BasicFunction(std::function <A> fn) noexcept : fn(fn) {}
			};
		public:
			// Создаём тип данных функций обратного вызова
			typedef map <uint64_t, std::shared_ptr <Function>> fns_t;
		private:
			// Список функций обратного вызова
			fns_t _functions;
		private:
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * dump Метод получения дампа функций обратного вызова
			 * @return std::pair <const fns_t *, const sigs_t *> 
			 */
			const fns_t * dump() const noexcept {
				// Выводим дамп функций обратного вызова
				return &this->_functions;
			}
			/**
			 * dump Метод установки дампа функций обратного вызова
			 * @param data данные функций обратного вызова
			 */
			void dump(const fns_t * & data) noexcept {
				// Если данные функций обратного вызова переданы
				if((data != nullptr) && !data->empty()){
					// Выполняем очистку текущего списка функций обратного вызова
					this->clear();
					// Выполняем перебор всех функций обратного вызова в хранилище
					for(auto & item : * data)
						// Устанавливаем новую функцию обратного вызова
						this->_functions.emplace(item.first, item.second);
				}
			}
		public:
			/**
			 * dump Метод извлечения данных функции обратного вызова по её идентификатору
			 * @param idw идентификатор функции обратного вызова
			 * @return    дамп функции обратного вызова
			 */
			std::shared_ptr <Function> dump(const uint64_t idw) const noexcept {
				// Если название функции обратного вызова передано
				if(idw > 0){
					// Выполняем поиск существующей функции обратного вызова
					auto it = this->_functions.find(idw);
					// Если функция существует
					if(it != this->_functions.end())
						// Устанавливаем функцию обратного вызова
						return it->second;
				}
				// Выводим результат работы функции
				return nullptr;
			}
			/**
			 * dump Метод извлечения данных функции обратного вызова по её имени
			 * @param name название функции обратного вызова
			 * @return     дамп функции обратного вызова
			 */
			std::shared_ptr <Function> dump(const string & name) const noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty())
					// Выводим получение дампа функции обратного вызова
					return this->dump(this->_idw.id(name));
				// Выводим результат работы функции
				return nullptr;
			}
		public:
			/**
			 * dump Метод установки данных функции обратного вызова по её идентификатору
			 * @param idw  идентификатор функции обратного вызова
			 * @param data дамп функции обратного вызова
			 */
			void dump(const uint64_t idw, const std::shared_ptr <Function> & data) noexcept {
				// Если название функции обратного вызова передано
				if((idw > 0) && (data != nullptr)){
					// Выполняем поиск существующей функции обратного вызова
					auto it = this->_functions.find(idw);
					// Если функция существует
					if(it != this->_functions.end())
						// Устанавливаем функцию обратного вызова
						it->second = data;
					// Устанавливаем новую функцию обратного вызова
					else this->_functions.emplace(idw, data);
				}
			}
			/**
			 * dump Метод установки данных функции обратного вызова по её идентификатору
			 * @param name название функции обратного вызова
			 * @param data дамп функции обратного вызова
			 */
			void dump(const string & name, const std::shared_ptr <Function> & data) noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty())
					// Выполняем установку данных функции обратного вызова
					this->dump(this->_idw.id(name), data);
			}
		public:
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param idw     идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void set(const uint64_t idw, const FN & storage) noexcept {
				// Если указанная функция существует
				if(!storage._functions.empty() && storage.is(idw)){
					// Выполняем поиск указанной функции в переданном хранилище
					auto i = storage._functions.find(idw);
					// Если функция в хранилище получена
					if(i != storage._functions.end()){
						// Выполняем поиск существующей функции обратного вызова
						auto j = this->_functions.find(idw);
						// Если функция такая уже существует
						if(j != this->_functions.end())
							// Устанавливаем новую функцию обратного вызова
							j->second = i->second;
						// Если функция ещё не существует, создаём новую функцию
						else this->_functions.emplace(idw, i->second);
					}
				}
			}
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param name    название копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void set(const string & name, const FN & storage) noexcept {
				// Если данные переданы правильно
				if(!name.empty() && !storage._functions.empty())
					// Выполняем установку функции обратного вызова
					this->set(this->_idw.id(name), storage);
			}
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param idw     идентификатор копируемой функции
			 * @param dest    новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void set(const uint64_t idw, const uint64_t dest, const FN & storage) noexcept {
				// Если указанная функция существует
				if(!storage._functions.empty() && storage.is(idw)){
					// Выполняем поиск указанной функции в переданном хранилище
					auto i = storage._functions.find(idw);
					// Если функция в хранилище получена
					if(i != storage._functions.end()){
						// Выполняем поиск существующей функции обратного вызова
						auto j = this->_functions.find(dest);
						// Если функция такая уже существует
						if(j != this->_functions.end())
							// Устанавливаем новую функцию обратного вызова
							j->second = i->second;
						// Если функция ещё не существует, создаём новую функцию
						else this->_functions.emplace(dest, i->second);
					}
				}
			}
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param name    название копируемой функции
			 * @param dest    новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void set(const string & name, const string & dest, const FN & storage) noexcept {
				// Если данные переданы правильно
				if(!name.empty() && !dest.empty() && !storage._functions.empty())
					// Выполняем установку функции обратного вызова
					this->set(this->_idw.id(name), this->_idw.id(dest), storage);
			}
		public:
			/**
			 * empty Метод проверки на пустоту контейнера
			 * @return результат проверки
			 */
			bool empty() const noexcept {
				// Выводим результат проверки
				return this->_functions.empty();
			}
		public:
			/**
			 * is Метод проверки наличия функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 * @return    результат проверки
			 */
			bool is(const uint64_t idw) const noexcept {
				// Выводим результат проверки
				return ((idw > 0) && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end()));
			}
			/**
			 * is Метод проверки наличия функции обратного вызова
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 */
			bool is(const string & name) const noexcept {
				// Выводим результат проверки
				return this->is(this->_idw.id(name));
			}
		public:
			/**
			 * clear Метод очистки параметров модуля
			 */
			void clear() noexcept {
				// Выполняем очистку функций обратного вызова
				this->_functions.clear();
				// Выполняем очистку выделенной памяти
				fns_t().swap(this->_functions);
			}
		public:
			/**
			 * erase Метод удаления функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 */
			void erase(const uint64_t idw) noexcept {
				// Если название функции обратного вызова передано
				if(idw > 0){
					// Выполняем поиск существующей функции обратного вызова
					auto it = this->_functions.find(idw);
					// Если функция существует
					if(it != this->_functions.end())
						// Удаляем функцию обратного вызова
						this->_functions.erase(it);
				}
			}
			/**
			 * erase Метод удаления функции обратного вызова
			 * @param name функция обратного вызова для удаления
			 */
			void erase(const string & name) noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty())
					// Выполняем удаление функции обратного вызова
					this->erase(this->_idw.id(name));
			}
		public:
			/**
			 * Шаблон метода установки функции обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename A>
			/**
			 * set Метод установки функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 * @param fn  функция обратного вызова
			 */
			void set(const uint64_t idw, std::function <A> fn) noexcept {
				// Если данные переданы
				if((idw > 0) && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск существующей функции обратного вызова
						auto it = this->_functions.find(idw);
						// Если функция такая уже существует
						if(it != this->_functions.end())
							// Устанавливаем новую функцию обратного вызова
							it->second = std::shared_ptr <Function> (new BasicFunction <A> (fn));
						// Если функция ещё не существует, создаём новую функцию
						else this->_functions.emplace(idw, std::shared_ptr <Function> (new BasicFunction <A> (fn)));
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc & error) {
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					}
				}
			}
			/**
			 * Шаблон метода установки функции обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename A>
			/**
			 * set Метод установки функции обратного вызова
			 * @param name название функции обратного вызова
			 * @param fn   функция обратного вызова
			 */
			void set(const string & name, std::function <A> fn) noexcept {
				// Если данные переданы
				if(!name.empty() && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Получаем идентификатор обратного вызова
						const uint64_t idw = this->_idw.id(name);
						// Выполняем поиск существующей функции обратного вызова
						auto it = this->_functions.find(idw);
						// Если функция такая уже существует
						if(it != this->_functions.end())
							// Устанавливаем новую функцию обратного вызова
							it->second = std::shared_ptr <Function> (new BasicFunction <A> (fn));
						// Если функция ещё не существует, создаём новую функцию
						else this->_functions.emplace(idw, std::shared_ptr <Function> (new BasicFunction <A> (fn)));
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc & error) {
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					}
				}
			}
			/**
			 * Шаблон метода установки функции обратного вызова
			 * @tparam A    сигнатура функции
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, class... Args>
			/**
			 * set Метод установки функции обратного вызова с актуальными аргументами
			 * @param idw  идентификатор функции обратного вызова
			 * @param fn   функция обратного вызова
			 * @param args список актуальных аргументов
			 */
			void set(const uint64_t idw, std::function <A> fn, Args... args) noexcept {
				// Если данные переданы
				if((idw > 0) && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск существующей функции обратного вызова
						auto it = this->_functions.find(idw);
						// Если функция такая уже существует
						if(it != this->_functions.end())
							// Устанавливаем новую функцию обратного вызова
							it->second = std::shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...)));
						// Если функция ещё не существует, создаём новую функцию
						else this->_functions.emplace(idw, std::shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...))));
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc & error) {
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					}
				}
			}
			/**
			 * Шаблон метода установки функции обратного вызова
			 * @tparam A    сигнатура функции
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, class... Args>
			/**
			 * set Метод установки функции обратного вызова с актуальными аргументами
			 * @param name название функции обратного вызова
			 * @param fn   функция обратного вызова
			 * @param args список актуальных аргументов
			 */
			void set(const string & name, std::function <A> fn, Args... args) noexcept {
				// Если данные переданы
				if(!name.empty() && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Получаем идентификатор обратного вызова
						const uint64_t idw = this->_idw.id(name);
						// Выполняем поиск существующей функции обратного вызова
						auto it = this->_functions.find(idw);
						// Если функция такая уже существует
						if(it != this->_functions.end())
							// Устанавливаем новую функцию обратного вызова
							it->second = std::shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...)));
						// Если функция ещё не существует, создаём новую функцию
						else this->_functions.emplace(idw, std::shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...))));
					/**
					 * Если возникает ошибка
					 */
					} catch(const bad_alloc & error) {
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					}
				}
			}
		public:
			/**
			 * Шаблон метода получения функции обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename A>
			/**
			 * get Метод получения функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 * @return    функция обратного вызова если существует
			 */
			auto get(const uint64_t idw) const noexcept -> std::function <A> {
				// Результат работы функции
				std::function <A> result = nullptr;
				// Если название функции передано
				if(idw > 0){
					// Выполняем поиск функции обратного вызова
					auto it = this->_functions.find(idw);
					// Если функция обратного вызова найдена
					if(it != this->_functions.end()){
						// Получаем функцию обратного вызова
						const Function & fn = (* it->second);
						// Получаем функцию обратного вызова в нужном нам виде
						result = static_cast <const BasicFunction <A> &> (fn).fn;
					}
				}
				// Выводим результат
				return result;
			}
			/**
			 * Шаблон метода получения функции обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename A>
			/**
			 * get Метод получения функции обратного вызова
			 * @param name название функции обратного вызова
			 * @return     функция обратного вызова если существует
			 */
			auto get(const string & name) const noexcept -> std::function <A> {
				// Результат работы функции
				std::function <A> result = nullptr;
				// Если название функции передано
				if(!name.empty()){
					// Получаем идентификатор обратного вызова
					const uint64_t idw = this->_idw.id(name);
					// Выполняем поиск функции обратного вызова
					auto it = this->_functions.find(idw);
					// Если функция обратного вызова найдена
					if(it != this->_functions.end()){
						// Получаем функцию обратного вызова
						const Function & fn = (* it->second);
						// Получаем функцию обратного вызова в нужном нам виде
						result = static_cast <const BasicFunction <A> &> (fn).fn;
					}
				}
				// Выводим результат
				return result;
			}
		public:
			/**
			 * Шаблон метода вызова функции обратного вызова
			 * @tparam A параметры функции обратного вызова
			 */
			template <typename A>
			/**
			 * call Метод вызова функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 */
			void call(const uint64_t idw) const noexcept {
				// Если название функции передано
				if((idw > 0) && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <A> (idw);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) std::apply(fn, std::make_tuple());
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
				// Выводим полученный результат
				return (typename function <A>::result_type) typename function <A>::result_type();
			}
			/**
			 * Шаблон метода вызова функции обратного вызова
			 * @tparam A параметры функции обратного вызова
			 */
			template <typename A>
			/**
			 * call Метод вызова функции обратного вызова
			 * @param name название функции обратного вызова
			 */
			auto call(const string & name) const noexcept -> typename function <A>::result_type {
				// Получаем идентификатор обратного вызова
				const uint64_t idw = this->_idw.id(name);
				// Если название функции передано
				if(!name.empty() && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <A> (idw);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) std::apply(fn, std::make_tuple());
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
				// Выводим полученный результат
				return (typename function <A>::result_type) typename function <A>::result_type();
			}
			/**
			 * Шаблон метода вызова функции обратного вызова
			 * @tparam A    параметры функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, class... Args>
			/**
			 * call Метод вызова функции обратного вызова
			 * @param idw  идентификатор функции обратного вызова
			 * @param args аргументы передаваемые в функцию обратного вызова
			 */
			auto call(const uint64_t idw, Args... args) const noexcept -> typename function <A>::result_type {
				// Если название функции передано
				if((idw > 0) && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <A> (idw);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) std::apply(fn, std::make_tuple(args...));
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
				// Выводим полученный результат
				return (typename function <A>::result_type) typename function <A>::result_type();
			}
			/**
			 * Шаблон метода вызова функции обратного вызова
			 * @tparam A    параметры функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, class... Args>
			/**
			 * call Метод вызова функции обратного вызова
			 * @param name название функции обратного вызова
			 * @param args аргументы передаваемые в функцию обратного вызова
			 */
			auto call(const string & name, Args... args) const noexcept -> typename function <A>::result_type {
				// Получаем идентификатор обратного вызова
				const uint64_t idw = this->_idw.id(name);
				// Если название функции передано
				if(!name.empty() && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <A> (idw);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) std::apply(fn, std::make_tuple(args...));
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
				// Выводим полученный результат
				return (typename function <A>::result_type) typename function <A>::result_type();
			}
		public:
			/**
			 * bind Метод выполнения всех функций обратного вызова с сохранёнными параметрами
			 */
			void bind() const noexcept {
				// Если функции обратного вызова существуют
				if(!this->_functions.empty()){
					// Выполняем переход по всему списку обратных функций
					for(auto & item : this->_functions){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							static_cast <const BasicFunction <void (void)> &> (* item.second).fn();
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
			}
			/**
			 * Шаблон метода выполнения всех функций обратного вызова с сохранёнными параметрами
			 * @tparam A тип возвращаемого значения
			 */
			template <typename A>
			/**
			 * bind Метод выполнения всех функций обратного вызова с сохранёнными параметрами
			 */
			auto bind() const noexcept -> vector <A> {
				// Результат работы функции
				vector <A> result;
				// Если функции обратного вызова существуют
				if(!this->_functions.empty()){
					// Выполняем переход по всему списку обратных функций
					for(auto & item : this->_functions){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							result.push_back(static_cast <const BasicFunction <A (void)> &> (* item.second).fn());
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
				// Выводим результат
				return result;
			}
			/**
			 * bind Метод вызова функции обратного вызова с сохранёнными параметрами
			 * @param idw идентификатор функции обратного вызова
			 */
			void bind(const uint64_t idw) const noexcept {
				// Если название функции передано
				if((idw > 0) && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Выполняем поиск запрашиваемой функции
					auto it = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(it != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							static_cast <const BasicFunction <void (void)> &> (* it->second).fn();
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
			}
			/**
			 * bind Метод вызова функции обратного вызова с сохранёнными параметрами
			 * @param name название функции обратного вызова
			 */
			void bind(const string & name) const noexcept {
				// Получаем идентификатор обратного вызова
				const uint64_t idw = this->_idw.id(name);
				// Если название функции передано
				if(!name.empty() && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Выполняем поиск запрашиваемой функции
					auto it = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(it != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							static_cast <const BasicFunction <void (void)> &> (* it->second).fn();
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
			}
			/**
			 * Шаблон метода вызова функции обратного вызова с сохранёнными параметрами 
			 * @tparam A тип возвращаемого значения
			 */
			template <typename A>
			/**
			 * bind Метод вызова функции обратного вызова с сохранёнными параметрами
			 * @param idw идентификатор функции обратного вызова
			 */
			auto bind(const uint64_t idw) const noexcept -> A {
				// Если название функции передано
				if((idw > 0) && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Выполняем поиск запрашиваемой функции
					auto it = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(it != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							return static_cast <const BasicFunction <A (void)> &> (* it->second).fn();
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
				// Выводим значение по умолчанию
				return A();
			}
			/**
			 * Шаблон метода вызова функции обратного вызова с сохранёнными параметрами 
			 * @tparam A тип возвращаемого значения
			 */
			template <typename A>
			/**
			 * bind Метод вызова функции обратного вызова с сохранёнными параметрами
			 * @param name название функции обратного вызова
			 */
			auto bind(const string & name) const noexcept -> A {
				// Получаем идентификатор обратного вызова
				const uint64_t idw = this->_idw.id(name);
				// Если название функции передано
				if(!name.empty() && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Выполняем поиск запрашиваемой функции
					auto it = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(it != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							return static_cast <const BasicFunction <A (void)> &> (* it->second).fn();
						/**
						 * Если возникает ошибка
						 */
						} catch(const std::bad_function_call & error) {
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				}
				// Выводим значение по умолчанию
				return A();
			}
		public:
			/**
			 * Оператор [=] присвоения функций обратного вызова
			 * @param storage хранилище функций откуда нужно получить функции
			 * @return        текущий объект
			 */
			FN & operator = (const FN & storage) noexcept {
				// Если функции обратного вызова установлены
				if(!storage._functions.empty()){
					// Выполняем очистку списка функций обратного вызова
					this->clear();
					// Выполняем перебор всех функций обратного вызова в хранилище
					for(auto & item : storage._functions)
						// Устанавливаем новую функцию обратного вызова
						this->_functions.emplace(item.first, item.second);
				}
				// Выводим результат
				return (* this);
			}
		public:
			/**
			 * FN Конструктор
			 * @param log объект для работы с логами
			 */
			FN(const log_t * log) noexcept : _log(log) {}
	} fn_t;
};

#endif // __AWH_FUNCTION__
