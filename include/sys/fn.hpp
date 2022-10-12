/**
 * @file: fn.hpp
 * @date: 2022-10-10
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __GLB_FUNCTION__
#define __GLB_FUNCTION__

/**
 * Стандартная библиотека
 */
#include <tuple>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <variant>
#include <iostream>
#include <functional>
#include <unordered_map>

/**
 * Наши модули
 */
#include <sys/log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * FN Класс работы с функциями
	 */
	typedef class FN {
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
			 * Parameters Структура параметров функцкии
			 */
			struct Parameters {
				/**
				 * ~Parameters Деструктор
				 */
				virtual ~Parameters() noexcept {}
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
			/**
			 * Шаблон базовых параметров
			 * @tparam A сигнатура параметров
			 */
			template <typename... A>
			/**
			 * BasicParameters Структура базовых параметров
			 */
			struct BasicParameters : Parameters {
				// Параметры базовой функции
				std::tuple <A...> pm;
				/**
				 * BasicParameters Конструктор
				 * @param pm параметры базовой функции для установки
				 */
				BasicParameters(std::tuple <A...> pm) noexcept : pm(pm) {}
			};
		private:
			// Список параметров базовых функций
			unordered_map <string, std::unique_ptr <Parameters>> _params;
			// Список функций обратного вызова
			unordered_map <string, std::unique_ptr <Function>> _callbacks;
		private:
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * is Метод проверки наличия функции обратного вызова
			 * @param name название функции обратного вызова
			 * @return     результат проверки
			 */
			bool is(const string & name) const noexcept {
				// Выводим результат проверки
				return (!name.empty() && !this->_callbacks.empty() && (this->_callbacks.find(name) != this->_callbacks.end()));
			}
			/**
			 * clear Метод очистки параметров модуля
			 */
			void clear() noexcept {
				// Выполняем очистку параметров функций обратного вызова
				this->_params.clear();
				// Выполняем очистку функций обратного вызова
				this->_callbacks.clear();
			}
			/**
			 * rm Метод удаления функции обратного вызова
			 * @param name функция обратного вызова для удаления
			 */
			void rm(const string & name) noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty()){
					{
						// Выполняем поиск существующей функции обратного вызова
						auto it = this->_callbacks.find(name);
						// Если функция существует
						if(it != this->_callbacks.end())
							// Удаляем функцию обратного вызова
							this->_callbacks.erase(it);
					}{
						// Выполняем поиск существующих параметров функции обратного вызова
						auto it = this->_params.find(name);
						// Если параметры функции обратного вызова существуют
						if(it != this->_params.end())
							// Удаляем параметры функции обратного вызова
							this->_params.erase(it);
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
					// Выполняем поиск существующей функции обратного вызова
					auto it = this->_callbacks.find(name);
					// Если функция такая уже существует
					if(it != this->_callbacks.end())
						// Устанавливаем новую функцию обратного вызова
						it->second = std::unique_ptr <Function> (new BasicFunction <A> (fn));
					// Если функция ещё не существует, создаём новую функцию
					else this->_callbacks.emplace(name, std::unique_ptr <Function> (new BasicFunction <A> (fn)));
				}
			}
			/**
			 * Шаблон метода установки функции обратного вызова
			 * @tparam A    сигнатура функции
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, class... Args>
			/**
			 * set Метод добавления функции обратного вызова с актуальными аргументами
			 * @param name название функции обратного вызова
			 * @param fn   функция обратного вызова
			 * @param args список актуальных аргументов
			 */
			void set(const string & name, std::function <A> fn, Args... args) noexcept {
				// Если данные переданы
				if(!name.empty() && (fn != nullptr)){
					// Создаём объект параметров функции
					std::tuple <Args...> pm = std::make_tuple(args...);
					// Выполняем поиск существующей функции обратного вызова
					auto it = this->_callbacks.find(name);
					// Если функция такая уже существует
					if(it != this->_callbacks.end())
						// Устанавливаем новую функцию обратного вызова
						it->second = std::unique_ptr <Function> (new BasicFunction <A> (fn));
					// Если функция ещё не существует, создаём новую функцию
					else this->_callbacks.emplace(name, std::unique_ptr <Function> (new BasicFunction <A> (fn)));
					// Выполняем поиск существующих параметров функции обратного вызова
					auto jt = this->_params.find(name);
					// Если параметры уже существуют
					if(jt != this->_params.end())
						// Устанавливаем новый список параметров
						jt->second = std::unique_ptr <Parameters> (new BasicParameters <Args...> (std::forward <std::tuple <Args...>> (pm)));
					// Если параметры ещё не существуют, создаём новую функцию
					else this->_params.emplace(name, std::unique_ptr <Parameters> (new BasicParameters <Args...> (std::forward <std::tuple <Args...>> (pm))));
				}
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
			std::function <A> get(const string & name) const noexcept {
				// Результат работы функции
				std::function <A> result = nullptr;
				// Если название функции передано
				if(!name.empty()){
					// Выполняем поиск функции обратного вызова
					auto it = this->_callbacks.find(name);
					// Если функция обратного вызова найдена
					if(it != this->_callbacks.end()){
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
			 * Шаблон метода выполнения всех функций обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename... A>
			/**
			 * call Метод выполнения всех функций обратного вызова
			 */
			void call() const noexcept {
				// Если функции обратного вызова существуют
				if(!this->_callbacks.empty()){
					// Выполняем переход по всему списку обратных функций
					for(auto & item : this->_callbacks){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Получаем функцию обратного вызова
							const Function & fn = (* item.second);
							// Выполняем функцию обратного вызова
							static_cast <const BasicFunction <void (A...)> &> (fn).fn();
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
			 * Шаблон метода вызова функции обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename... A>
			/**
			 * call Метод вызова функции обратного вызова
			 * @param name название функции обратного вызова
			 */
			void call(const string & name) const noexcept {
				// Если название функции передано
				if(!name.empty() && !this->_callbacks.empty() && (this->_callbacks.find(name) != this->_callbacks.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <void (A...)> (name);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							std::apply(fn);
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
			 * Шаблон метода вызова функции обратного вызова
			 * @tparam A    сигнатура функции
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename... A, class... Args>
			/**
			 * call Метод вызова функции обратного вызова
			 * @param name название функции обратного вызова
			 * @param args аргументы передаваемые в функцию обратного вызова
			 */
			void call(const string & name, Args... args) const noexcept {
				// Если название функции передано
				if(!name.empty() && !this->_callbacks.empty() && (this->_callbacks.find(name) != this->_callbacks.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <void (A...)> (name);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							std::apply(fn, std::make_tuple(args...));
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
			 * Шаблон метода выполнения всех функций обратного вызова с возвратам списка результатов
			 * @tparam A возвращаемое значение функции
			 * @tparam B сигнатура функции
			 */
			template <typename A, typename... B>
			/**
			 * apply Метод выполнения всех функций обратного вызова с возвратам списка результатов
			 */
			vector <A> apply() const noexcept {
				// Результат работы функции
				vector <A> result;
				// Если функции обратного вызова существуют
				if(!this->_callbacks.empty()){
					// Выполняем переход по всему списку обратных функций
					for(auto & item : this->_callbacks){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Получаем функцию обратного вызова
							const Function & fn = (* item.second);
							// Выполняем функцию обратного вызова
							result.push_back(static_cast <const BasicFunction <A (B...)> &> (fn).fn());
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
			 * Шаблон метода вызова функции обратного вызова с возвратам результата
			 * @tparam A возвращаемое значение функции
			 * @tparam B сигнатура функции
			 */
			template <typename A, typename... B>
			/**
			 * apply Метод вызова функции обратного вызова с возвратам результата
			 * @param name название функции обратного вызова
			 */
			A apply(const string & name) const noexcept {
				// Результат работы функции
				A result = A();
				// Если название функции передано
				if(!name.empty() && !this->_callbacks.empty() && (this->_callbacks.find(name) != this->_callbacks.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <A (B...)> (name);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							result = std::apply(fn);
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
			 * Шаблон метода вызова функции обратного вызова с возвратам результата
			 * @tparam A    возвращаемое значение функции
			 * @tparam B    сигнатура функции
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, typename... B, class... Args>
			/**
			 * apply Метод вызова функции обратного вызова с возвратам результата
			 * @param name название функции обратного вызова
			 * @param args аргументы передаваемые в функцию обратного вызова
			 */
			A apply(const string & name, Args... args) const noexcept {
				// Результат работы функции
				A result = A();
				// Если название функции передано
				if(!name.empty() && !this->_callbacks.empty() && (this->_callbacks.find(name) != this->_callbacks.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <A (B...)> (name);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем функцию обратного вызова
							result = std::apply(fn, std::make_tuple(args...));
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
			 * Шаблон метода выполнения всех функций обратного вызова с сохранёнными параметрами
			 * @tparam A сигнатура функции
			 */
			template <typename... A>
			/**
			 * bind Метод выполнения всех функций обратного вызова с сохранёнными параметрами
			 */
			void bind() const noexcept {
				// Если функции обратного вызова существуют
				if(!this->_callbacks.empty()){
					// Выполняем переход по всему списку обратных функций
					for(auto & item : this->_callbacks){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем поиск параметров функции обратного вызова
							auto it = this->_params.find(item.first);
							// Если параметры функции обратного вызова найдены
							if(it != this->_params.end()){
								// Получаем функцию обратного вызова
								const Function & fn = (* item.second);
								// Получаем параметры функции обратного вызова
								const Parameters & pm = (* it->second);
								// Выполняем функцию обратного вызова
								std::apply(
									static_cast <const BasicFunction <void (A...)> &> (fn).fn,
									static_cast <const BasicParameters <A...> &> (pm).pm
								);
							}
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
			 * @tparam A сигнатура функции
			 */
			template <typename... A>
			/**
			 * bind Метод вызова функции обратного вызова с сохранёнными параметрами
			 * @param name название функции обратного вызова
			 */
			void bind(const string & name) const noexcept {
				// Если название функции передано
				if(!name.empty() && !this->_callbacks.empty() && (this->_callbacks.find(name) != this->_callbacks.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <void (A...)> (name);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем поиск параметров функции обратного вызова
							auto it = this->_params.find(name);
							// Если параметры функции обратного вызова найдены
							if(it != this->_params.end()){
								// Получаем параметры функции обратного вызова
								const Parameters & pm = (* it->second);
								// Выполняем функцию обратного вызова
								std::apply(fn, static_cast <const BasicParameters <A...> &> (pm).pm);
							}
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
			 * Шаблон метода получения вызова функции обратного вызова
			 * @tparam A возвращаемое значение функции
			 * @tparam B сигнатура функции
			 */
			template <typename A, typename... B>
			/**
			 * retrieve Метод выполнения всех функций обратного вызова с сохранёнными параметрами и возвратам списка результатов
			 */
			vector <A> retrieve() const noexcept {
				// Результат работы функции
				vector <A> result;
				// Если функции обратного вызова существуют
				if(!this->_callbacks.empty()){
					// Выполняем переход по всему списку обратных функций
					for(auto & item : this->_callbacks){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем поиск параметров функции обратного вызова
							auto it = this->_params.find(item.first);
							// Если параметры функции обратного вызова найдены
							if(it != this->_params.end()){
								// Получаем функцию обратного вызова
								const Function & fn = (* item.second);
								// Получаем параметры функции обратного вызова
								const Parameters & pm = (* it->second);
								// Добавляем полученный результат в массив
								result.push_back(
									// Выполняем функцию обратного вызова
									std::apply(
										static_cast <const BasicFunction <A (B...)> &> (fn).fn,
										static_cast <const BasicParameters <B...> &> (pm).pm
									)
								);
							}
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
			 * Шаблон метода получения вызова функции обратного вызова 
			 * @tparam A возвращаемое значение функции
			 * @tparam B сигнатура функции
			 */
			template <typename A, typename... B>
			/**
			 * retrieve Метод вызова функции обратного вызова с сохранёнными параметрами и возвратам результата
			 * @param name название функции обратного вызова
			 */
			A retrieve(const string & name) const noexcept {
				// Результат работы функции
				A result = A();
				// Если название функции передано
				if(!name.empty() && !this->_callbacks.empty() && (this->_callbacks.find(name) != this->_callbacks.end())){
					// Получаем функцию обратного вызова
					auto fn = this->get <A (B...)> (name);
					// Если функция получена, выполняем её
					if(fn != nullptr){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем поиск параметров функции обратного вызова
							auto it = this->_params.find(name);
							// Если параметры функции обратного вызова найдены
							if(it != this->_params.end()){
								// Получаем параметры функции обратного вызова
								const Parameters & pm = (* it->second);
								// Выполняем функцию обратного вызова
								result = std::apply(fn, static_cast <const BasicParameters <B...> &> (pm).pm);
							}
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
		public:
			/**
			 * FN Конструктор
			 * @param log объект для работы с логами
			 */
			FN(const log_t * log) noexcept : _log(log) {}
	} fn_t;
};

#endif // __GLB_FUNCTION__
