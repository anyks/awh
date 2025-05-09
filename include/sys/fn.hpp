/**
 * @file: fn.hpp
 * @date: 2023-11-30
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

#ifndef __AWH_FUNCTION__
#define __AWH_FUNCTION__

/**
 * Стандартные модули
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
#include <sys/log.hpp>
#include <cityhash/city.h>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * FN Прототип класса работы с функциями
	 */
	class FN;
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * FN Класс работы с функциями
	 */
	typedef class FN {
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
			 * Основные типы данных возвращаемые функциями
			 */
			enum class type_t : uint8_t {
				NONE          = 0x00, // Тип данных не установлен
				TYPE_ENUM     = 0x01, // Тип данных принадлежит к перечислениям
				TYPE_VOID     = 0x02, // Тип данных не возвращает значения
				TYPE_UNION    = 0x03, // Тип данных принадлежит к объединениям
				TYPE_ARRAY    = 0x04, // Тип данных принадлежит к массивам
				TYPE_CLASS    = 0x05, // Тип данных принадлежит к классам и структурам
				TYPE_POINTER  = 0x06, // Тип данных принадлежит к указателям и ссылкам
				TYPE_FUNCTION = 0x07  // Тип данных принадлежит к функциям
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
			 * Шаблон базовой функции
			 * @tparam A сигнатура функции
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
			// Создаём тип данных возвращаемых значений функций обратного вызова
			typedef map <uint64_t, type_t> types_t;
			// Создаём тип данных функций обратного вызова
			typedef map <uint64_t, shared_ptr <Function>> fns_t;
			// Структура дампа данных конкретной функции
			typedef pair <shared_ptr <Function>, type_t> dump_t;
		private:
			// Поддерживаемые типы данных
			types_t _types;
			// Список функций обратного вызова
			fns_t _functions;
		private:
			// Функция обратного вызова при получении события установки или удаления функции
			function <void (const event_t, const uint64_t, const string &, const dump_t *)> _callback;
		private:
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * Шаблон метода установки функции обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename A>
			/**
			 * type Метод определения типа значения
			 * @return тип которому соответствует переменная
			 */
			auto type() const noexcept -> type_t {
				// Если тип данных принадлежит к VOID
				if(is_void <A>::value)
					// Выводим результат
					return type_t::TYPE_VOID;
				// Если тип данных принадлежит к ENUM
				else if(is_enum <A>::value)
					// Выводим результат
					return type_t::TYPE_ENUM;
				// Если тип данных принадлежит к UNION
				else if(is_union <A>::value)
					// Выводим результат
					return type_t::TYPE_UNION;
				// Если тип данных принадлежит к ARRAY
				else if(is_array <A>::value)
					// Выводим результат
					return type_t::TYPE_ARRAY;
				// Если тип данных принадлежит к CLASS
				else if(is_class <A>::value)
					// Выводим результат
					return type_t::TYPE_CLASS;
				// Если тип данных принадлежит к POINTER
				else if(is_pointer <A>::value)
					// Выводим результат
					return type_t::TYPE_POINTER;
				// Если тип данных принадлежит к FUNCTION
				else if(is_function <A>::value)
					// Выводим результат
					return type_t::TYPE_FUNCTION;
				// Выводим результат
				return type_t::NONE;
			}
		public:
			/**
			 * dump Метод получения дампа функций обратного вызова
			 * @return выводим созданный блок дампа контейнера
			 */
			const pair <const fns_t *, const types_t *> dump() const noexcept {
				// Выводим дамп функций обратного вызова
				return make_pair(&this->_functions, &this->_types);
			}
			/**
			 * dump Метод установки дампа функций обратного вызова
			 * @param data данные функций обратного вызова
			 */
			void dump(const pair <const fns_t *, const types_t *> & data) noexcept {
				// Если данные функций обратного вызова переданы
				if(((data.first != nullptr) && !data.first->empty()) && ((data.second != nullptr) && !data.second->empty())){
					// Выполняем очистку текущего списка функций обратного вызова
					this->clear();
					// Выполняем перебор всех параметров возвращаемых функциями
					for(auto & item : * data.second)
						// Устанавливаем возвращаемое значение
						this->_types.emplace(item.first, item.second);
					// Выполняем перебор всех функций обратного вызова в хранилище
					for(auto & item : * data.first)
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
			dump_t dump(const uint64_t idw) const noexcept {
				// Результат работы функции
				dump_t result = make_pair(nullptr, type_t::NONE);
				// Если название функции обратного вызова передано
				if(idw > 0){
					{
						// Выполняем поиск типа возвращаемого значения
						auto i = this->_types.find(idw);
						// Если возвращаемое значение получено
						if(i != this->_types.end())
							// Устанавливаем тип возвращаемого значения
							result.second = i->second;
					}{
						// Выполняем поиск существующей функции обратного вызова
						auto i = this->_functions.find(idw);
						// Если функция существует
						if(i != this->_functions.end())
							// Устанавливаем функцию обратного вызова
							result.first = i->second;
					}
				}
				// Выводим результат работы функции
				return result;
			}
			/**
			 * dump Метод извлечения данных функции обратного вызова по её имени
			 * @param name название функции обратного вызова
			 * @return     дамп функции обратного вызова
			 */
			dump_t dump(const string & name) const noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty())
					// Выводим получение дампа функции обратного вызова
					return this->dump(this->idw(name));
				// Выводим результат работы функции
				return make_pair(nullptr, type_t::NONE);
			}
		public:
			/**
			 * dump Метод установки данных функции обратного вызова по её идентификатору
			 * @param idw  идентификатор функции обратного вызова
			 * @param data дамп функции обратного вызова
			 */
			void dump(const uint64_t idw, const dump_t & data) noexcept {
				// Если название функции обратного вызова передано
				if((idw > 0) && (data.first != nullptr)){
					{
						// Выполняем поиск типа возвращаемого значения
						auto i = this->_types.find(idw);
						// Если возвращаемое значение получено
						if(i != this->_types.end())
							// Устанавливаем тип возвращаемого значения
							i->second = data.second;
						// Устанавливаем новую функцию обратного вызова
						else this->_types.emplace(idw, data.second);
					}{
						// Выполняем поиск существующей функции обратного вызова
						auto i = this->_functions.find(idw);
						// Если функция существует
						if(i != this->_functions.end())
							// Устанавливаем функцию обратного вызова
							i->second = data.first;
						// Устанавливаем новую функцию обратного вызова
						else this->_functions.emplace(idw, data.first);
					}
				}
			}
			/**
			 * dump Метод установки данных функции обратного вызова по её идентификатору
			 * @param name название функции обратного вызова
			 * @param data дамп функции обратного вызова
			 */
			void dump(const string & name, const dump_t & data) noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty())
					// Выполняем установку данных функции обратного вызова
					this->dump(this->idw(name), data);
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
				return this->is(this->idw(name));
			}
		public:
			/**
			 * idw Метод генерации идентификатора функции
			 * @param name название функции для генерации идентификатора
			 * @return     сгенерированный идентификатор функции
			 */
			uint64_t idw(const string & name) const noexcept {
				// Получаем идентификатор обратного вызова
				return CityHash64(name.c_str(), name.size());
			}
		public:
			/**
			 * clear Метод очистки параметров модуля
			 */
			void clear() noexcept {
				// Выполняем очистку списка возвращаемых значений функций
				this->_types.clear();
				// Выполняем очистку функций обратного вызова
				this->_functions.clear();
				// Выполняем очистку выделенной памяти для возвращаемых значений
				types_t().swap(this->_types);
				// Выполняем очистку выделенной памяти для функций
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
					{
						// Выполняем поиск типа возвращаемого значения
						auto i = this->_types.find(idw);
						// Если возвращаемое значение получено
						if(i != this->_types.end())
							// Удаляем возвращаемое значение функцией
							this->_types.erase(i);
					}{
						// Выполняем поиск существующей функции обратного вызова
						auto i = this->_functions.find(idw);
						// Если функция существует
						if(i != this->_functions.end())
							// Удаляем функцию обратного вызова
							this->_functions.erase(i);
					}
					// Если функция обратного вызова установлена
					if(this->_callback != nullptr)
						// Выполняем функцию обратного вызова
						this->_callback(event_t::DEL, idw, "", nullptr);
				}
			}
			/**
			 * erase Метод удаления функции обратного вызова
			 * @param name функция обратного вызова для удаления
			 */
			void erase(const string & name) noexcept {
				// Если название функции обратного вызова передано
				if(!name.empty()){
					// Получаем идентификатор обратного вызова
					const uint64_t idw = this->idw(name);
					// Если название функции обратного вызова передано
					if(idw > 0){
						{
							// Выполняем поиск типа возвращаемого значения
							auto i = this->_types.find(idw);
							// Если возвращаемое значение получено
							if(i != this->_types.end())
								// Удаляем возвращаемое значение функцией
								this->_types.erase(i);
						}{
							// Выполняем поиск существующей функции обратного вызова
							auto i = this->_functions.find(idw);
							// Если функция существует
							if(i != this->_functions.end())
								// Удаляем функцию обратного вызова
								this->_functions.erase(i);
						}
						// Если функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем функцию обратного вызова
							this->_callback(event_t::DEL, idw, name, nullptr);
					}
				}
			}
		public:
			/**
			 * swap Метод обмена функциями
			 * @param idw1 идентификатор первой функции
			 * @param idw2 идентификатор второй функции
			 */
			void swap(const uint64_t idw1, const uint64_t idw2) noexcept {
				// Если идентификаторы переданы
				if((idw1 > 0) && (idw2 > 0)){
					{
						// Выполняем поиск первого возвращаемого значения
						auto i = this->_types.find(idw1);
						// Если возвращаемое значение получено
						if(i != this->_types.end()){
							// Получаем значение первого возвращаемого значения
							const type_t type = i->second;
							// Выполняем поиск первого возвращаемого значения
							auto j = this->_types.find(idw2);
							// Если возвращаемое значение получено
							if(j != this->_types.end()){
								// Выполняем замену первого возвращаемого значения
								i->second = j->second;
								// Выполняем замену второго возвращаемого значения
								j->second = type;
							}
						}
					}{
						// Выполняем поиск первой функции
						auto i = this->_functions.find(idw1);
						// Если функция получена
						if(i != this->_functions.end()){
							// Получаем первую функцию обратного вызова
							auto fn = i->second;
							// Выполняем поиск второй функции
							auto j = this->_functions.find(idw2);
							// Если функция получена
							if(j != this->_functions.end()){
								// Выполняем замену первой функции
								i->second = j->second;
								// Выполняем замену второй функции
								j->second = fn;
							}
						}
					}
				}
			}
			/**
			 * swap Метод обмена функциями
			 * @param name1 название первой функции
			 * @param name2 название второй функции
			 */
			void swap(const string & name1, const string & name2) noexcept {
				// Если названия переданы
				if(!name1.empty() && !name2.empty()){
					// Получаем идентификатор названия первой функции
					const uint64_t idw1 = this->idw(name1);
					// Получаем идентификатор названия второй функции
					const uint64_t idw2 = this->idw(name2);
					// Если идентификаторы переданы
					if((idw1 > 0) && (idw2 > 0))
						// Выполняем функцию обратного вызова
						this->swap(idw1, idw2);
				}
			}
			/**
			 * swap Метод обмена функциями
			 * @param idw1    идентификатор первой функции
			 * @param idw2    идентификатор второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void swap(const uint64_t idw1, const uint64_t idw2, FN & storage) noexcept {
				// Если идентификаторы переданы
				if((idw1 > 0) && (idw2 > 0) && !storage.empty()){
					{
						// Выполняем поиск первого возвращаемого значения
						auto i = this->_types.find(idw1);
						// Если возвращаемое значение получено
						if(i != this->_types.end()){
							// Получаем значение первого возвращаемого значения
							const type_t type = i->second;
							// Выполняем поиск первого возвращаемого значения
							auto j = storage._types.find(idw2);
							// Если возвращаемое значение получено
							if(j != storage._types.end()){
								// Выполняем замену первого возвращаемого значения
								i->second = j->second;
								// Выполняем замену второго возвращаемого значения
								j->second = type;
							}
						}
					}{
						// Выполняем поиск первой функции
						auto i = this->_functions.find(idw1);
						// Если функция получена
						if(i != this->_functions.end()){
							// Получаем первую функцию обратного вызова
							auto fn = i->second;
							// Выполняем поиск второй функции
							auto j = storage._functions.find(idw2);
							// Если функция получена
							if(j != storage._functions.end()){
								// Выполняем замену первой функции
								i->second = j->second;
								// Выполняем замену второй функции
								j->second = fn;
							}
						}
					}
				}
			}
			/**
			 * swap Метод обмена функциями
			 * @param name1   название первой функции
			 * @param name2   название второй функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void swap(const string & name1, const string & name2, FN & storage) noexcept {
				// Если названия переданы
				if(!name1.empty() && !name2.empty() && !storage.empty()){
					// Получаем идентификатор названия первой функции
					const uint64_t idw1 = this->idw(name1);
					// Получаем идентификатор названия второй функции
					const uint64_t idw2 = this->idw(name2);
					// Если идентификаторы переданы
					if((idw1 > 0) && (idw2 > 0))
						// Выполняем функцию обратного вызова
						this->swap(idw1, idw2, storage);
				}
			}
		public:
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param idw     идентификатор копируемой функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void set(const uint64_t idw, const FN & storage) noexcept {
				// Если указанная функция существует
				if((idw > 0) && !storage._functions.empty() && storage.is(idw)){
					{
						// Выполняем поиск указанного возвращаемого значения в переданном хранилище
						auto i = storage._types.find(idw);
						// Если возвращаемое значение в хранилище получено
						if(i != storage._types.end()){
							// Выполняем поиск типа возвращаемого значения
							auto j = this->_types.find(idw);
							// Если возвращаемое значение получено
							if(j != this->_types.end())
								// Устанавливаем новое значение типа возвращаемого значения
								j->second = i->second;
							// Если возращаемое значение ещё не установлено
							else this->_types.emplace(idw, i->second);
						}
					}{
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
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::SET, idw, "", &result);
							}
						}
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
				if(!name.empty() && !storage._functions.empty()){
					// Получаем идентификатор обратного вызова
					const uint64_t idw = this->idw(name);
					// Если указанная функция существует
					if((idw > 0) && storage.is(idw)){
						{
							// Выполняем поиск указанного возвращаемого значения в переданном хранилище
							auto i = storage._types.find(idw);
							// Если возвращаемое значение в хранилище получено
							if(i != storage._types.end()){
								// Выполняем поиск типа возвращаемого значения
								auto j = this->_types.find(idw);
								// Если возвращаемое значение получено
								if(j != this->_types.end())
									// Устанавливаем новое значение типа возвращаемого значения
									j->second = i->second;
								// Если возращаемое значение ещё не установлено
								else this->_types.emplace(idw, i->second);
							}
						}{
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
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr){
									// Результат работы функции
									const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
									// Выполняем функцию обратного вызова
									this->_callback(event_t::SET, idw, name, &result);
								}
							}
						}
					}
				}
			}
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param idw1    идентификатор копируемой функции
			 * @param idw2    новый идентификатор полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void set(const uint64_t idw1, const uint64_t idw2, const FN & storage) noexcept {
				// Если указанная функция существует
				if((idw1 > 0) && !storage._functions.empty() && storage.is(idw1)){
					{
						// Выполняем поиск указанного возвращаемого значения в переданном хранилище
						auto i = storage._types.find(idw1);
						// Если возвращаемое значение в хранилище получено
						if(i != storage._types.end()){
							// Выполняем поиск типа возвращаемого значения
							auto j = this->_types.find(idw2);
							// Если возвращаемое значение получено
							if(j != this->_types.end())
								// Устанавливаем новое значение типа возвращаемого значения
								j->second = i->second;
							// Если возращаемое значение ещё не установлено
							else this->_types.emplace(idw2, i->second);
						}
					}{
						// Выполняем поиск указанной функции в переданном хранилище
						auto i = storage._functions.find(idw1);
						// Если функция в хранилище получена
						if(i != storage._functions.end()){
							// Выполняем поиск существующей функции обратного вызова
							auto j = this->_functions.find(idw2);
							// Если функция такая уже существует
							if(j != this->_functions.end())
								// Устанавливаем новую функцию обратного вызова
								j->second = i->second;
							// Если функция ещё не существует, создаём новую функцию
							else this->_functions.emplace(idw2, i->second);
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw2), this->_types.at(idw2));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::SET, idw2, "", &result);
							}
						}
					}
				}
			}
			/**
			 * set Метод установки функции из одного хранилища в текущее
			 * @param name1   название копируемой функции
			 * @param name2   новое название полученной функции
			 * @param storage хранилище функций откуда нужно получить функцию
			 */
			void set(const string & name1, const string & name2, const FN & storage) noexcept {
				// Если данные переданы правильно
				if(!name1.empty() && !name2.empty() && !storage._functions.empty()){
					// Получаем идентификатор названия первой функции
					const uint64_t idw1 = this->idw(name1);
					// Получаем идентификатор названия второй функции
					const uint64_t idw2 = this->idw(name2);
					// Если указанная функция существует
					if((idw1 > 0) && (idw2 > 0) && storage.is(idw1)){
						{
							// Выполняем поиск указанного возвращаемого значения в переданном хранилище
							auto i = storage._types.find(idw1);
							// Если возвращаемое значение в хранилище получено
							if(i != storage._types.end()){
								// Выполняем поиск типа возвращаемого значения
								auto j = this->_types.find(idw2);
								// Если возвращаемое значение получено
								if(j != this->_types.end())
									// Устанавливаем новое значение типа возвращаемого значения
									j->second = i->second;
								// Если возращаемое значение ещё не установлено
								else this->_types.emplace(idw2, i->second);
							}
						}{
							// Выполняем поиск указанной функции в переданном хранилище
							auto i = storage._functions.find(idw1);
							// Если функция в хранилище получена
							if(i != storage._functions.end()){
								// Выполняем поиск существующей функции обратного вызова
								auto j = this->_functions.find(idw2);
								// Если функция такая уже существует
								if(j != this->_functions.end())
									// Устанавливаем новую функцию обратного вызова
									j->second = i->second;
								// Если функция ещё не существует, создаём новую функцию
								else this->_functions.emplace(idw2, i->second);
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr){
									// Результат работы функции
									const dump_t result = make_pair(this->_functions.at(idw2), this->_types.at(idw2));
									// Выполняем функцию обратного вызова
									this->_callback(event_t::SET, idw2, name2, &result);
								}
							}
						}
					}
				}
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
			void set(const uint64_t idw, function <A> fn) noexcept {
				// Если данные переданы
				if((idw > 0) && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						{
							// Выполняем поиск типа данных функции
							auto i = this->_types.find(idw);
							// Если тип данных получен
							if(i != this->_types.end())
								// Выполняем установку типа возвращаемого значения
								i->second = this->type <typename function <A>::result_type> ();
							// Выполняем добавление полученного типа возвращаемого значения
							else this->_types.emplace(idw, this->type <typename function <A>::result_type> ());
						}{
							// Выполняем поиск существующей функции обратного вызова
							auto i = this->_functions.find(idw);
							// Если функция такая уже существует
							if(i != this->_functions.end())
								// Устанавливаем новую функцию обратного вызова
								i->second = shared_ptr <Function> (new BasicFunction <A> (fn));
							// Если функция ещё не существует, создаём новую функцию
							else this->_functions.emplace(idw, shared_ptr <Function> (new BasicFunction <A> (fn)));
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::SET, idw, "", &result);
							}
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
			void set(const string & name, function <A> fn) noexcept {
				// Если данные переданы
				if(!name.empty() && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Получаем идентификатор обратного вызова
						const uint64_t idw = this->idw(name);
						// Если идентификатор получен
						if(idw > 0){
							{
								// Выполняем поиск типа данных функции
								auto i = this->_types.find(idw);
								// Если тип данных получен
								if(i != this->_types.end())
									// Выполняем установку типа возвращаемого значения
									i->second = this->type <typename function <A>::result_type> ();
								// Выполняем добавление полученного типа возвращаемого значения
								else this->_types.emplace(idw, this->type <typename function <A>::result_type> ());
							}{
								// Выполняем поиск существующей функции обратного вызова
								auto i = this->_functions.find(idw);
								// Если функция такая уже существует
								if(i != this->_functions.end())
									// Устанавливаем новую функцию обратного вызова
									i->second = shared_ptr <Function> (new BasicFunction <A> (fn));
								// Если функция ещё не существует, создаём новую функцию
								else this->_functions.emplace(idw, shared_ptr <Function> (new BasicFunction <A> (fn)));
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr){
									// Результат работы функции
									const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
									// Выполняем функцию обратного вызова
									this->_callback(event_t::SET, idw, name, &result);
								}
							}
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
			void set(const uint64_t idw, function <A> fn, Args... args) noexcept {
				// Если данные переданы
				if((idw > 0) && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						{
							// Выполняем поиск типа данных функции
							auto i = this->_types.find(idw);
							// Если тип данных получен
							if(i != this->_types.end())
								// Выполняем установку типа возвращаемого значения
								i->second = this->type <typename function <A>::result_type> ();
							// Выполняем добавление полученного типа возвращаемого значения
							else this->_types.emplace(idw, this->type <typename function <A>::result_type> ());
						}{
							// Выполняем поиск существующей функции обратного вызова
							auto i = this->_functions.find(idw);
							// Если функция такая уже существует
							if(i != this->_functions.end())
								// Устанавливаем новую функцию обратного вызова
								i->second = shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...)));
							// Если функция ещё не существует, создаём новую функцию
							else this->_functions.emplace(idw, shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...))));
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::SET, idw, "", &result);
							}
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
			void set(const string & name, function <A> fn, Args... args) noexcept {
				// Если данные переданы
				if(!name.empty() && (fn != nullptr)){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Получаем идентификатор обратного вызова
						const uint64_t idw = this->idw(name);
						// Если идентификатор получен
						if(idw > 0){
							{
								// Выполняем поиск типа данных функции
								auto i = this->_types.find(idw);
								// Если тип данных получен
								if(i != this->_types.end())
									// Выполняем установку типа возвращаемого значения
									i->second = this->type <typename function <A>::result_type> ();
								// Выполняем добавление полученного типа возвращаемого значения
								else this->_types.emplace(idw, this->type <typename function <A>::result_type> ());
							}{
								// Выполняем поиск существующей функции обратного вызова
								auto i = this->_functions.find(idw);
								// Если функция такая уже существует
								if(i != this->_functions.end())
									// Устанавливаем новую функцию обратного вызова
									i->second = shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...)));
								// Если функция ещё не существует, создаём новую функцию
								else this->_functions.emplace(idw, shared_ptr <Function> (new BasicFunction <A> (std::bind(fn, args...))));
								// Если функция обратного вызова установлена
								if(this->_callback != nullptr){
									// Результат работы функции
									const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
									// Выполняем функцию обратного вызова
									this->_callback(event_t::SET, idw, name, &result);
								}
							}
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, "Memory allocation error");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
			 * Шаблон метода получения функции обратного вызова
			 * @tparam A сигнатура функции
			 */
			template <typename A>
			/**
			 * get Метод получения функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 * @return    функция обратного вызова если существует
			 */
			auto get(const uint64_t idw) const noexcept -> function <A> {
				// Результат работы функции
				function <A> result = nullptr;
				// Если название функции передано
				if(idw > 0){
					// Выполняем поиск функции обратного вызова
					auto i = this->_functions.find(idw);
					// Если функция обратного вызова найдена
					if(i != this->_functions.end()){
						// Получаем функцию обратного вызова
						const Function & fn = (* i->second);
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
			auto get(const string & name) const noexcept -> function <A> {
				// Результат работы функции
				function <A> result = nullptr;
				// Если название функции передано
				if(!name.empty()){
					// Получаем идентификатор обратного вызова
					const uint64_t idw = this->idw(name);
					// Если идентификатор получен
					if(idw > 0){
						// Выполняем поиск функции обратного вызова
						auto i = this->_functions.find(idw);
						// Если функция обратного вызова найдена
						if(i != this->_functions.end()){
							// Получаем функцию обратного вызова
							const Function & fn = (* i->second);
							// Получаем функцию обратного вызова в нужном нам виде
							result = static_cast <const BasicFunction <A> &> (fn).fn;
						}
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
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::RUN, idw, "", &result);
							}
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) apply(fn, make_tuple());
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
				const uint64_t idw = this->idw(name);
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
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::RUN, idw, name, &result);
							}
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) apply(fn, make_tuple());
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
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
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::RUN, idw, "", &result);
							}
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) apply(fn, make_tuple(args...));
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
				const uint64_t idw = this->idw(name);
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
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::RUN, idw, name, &result);
							}
							// Выполняем функцию обратного вызова
							return (typename function <A>::result_type) apply(fn, make_tuple(args...));
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
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
							// Выполняем поиск типа данных функции
							auto i = this->_types.find(item.first);
							// Если тип данных получен
							if(i != this->_types.end()){
								// Если функция не возвращает значение
								if(i->second == type_t::TYPE_VOID){
									// Если функция обратного вызова установлена
									if(this->_callback != nullptr){
										// Результат работы функции
										const dump_t result = make_pair(this->_functions.at(item.first), this->_types.at(item.first));
										// Выполняем функцию обратного вызова
										this->_callback(event_t::RUN, item.first, "", &result);
									}
									// Выполняем функцию обратного вызова
									static_cast <const BasicFunction <void (void)> &> (* item.second).fn();
								}
							// Выводим сообщение, что функция не найдена в хранилище
							} else this->_log->print("Requested function was not found in the storage", log_t::flag_t::CRITICAL);
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
							// Выполняем поиск типа данных функции
							auto i = this->_types.find(item.first);
							// Если тип данных получен
							if(i != this->_types.end()){
								// Если возвращаемое значение совпадает с хранимым или функция возвращает значение
								if((i->second == this->type <A> ()) && (i->second != type_t::TYPE_VOID)){
									// Если функция обратного вызова установлена
									if(this->_callback != nullptr){
										// Результат работы функции
										const dump_t result = make_pair(this->_functions.at(item.first), this->_types.at(item.first));
										// Выполняем функцию обратного вызова
										this->_callback(event_t::RUN, item.first, "", &result);
									}
									// Выполняем функцию обратного вызова
									result.push_back(static_cast <const BasicFunction <A (void)> &> (* item.second).fn());
								}
							// Выводим сообщение, что функция не найдена в хранилище
							} else this->_log->print("Requested function was not found in the storage", log_t::flag_t::CRITICAL);
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
					auto i = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(i != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::RUN, idw, "", &result);
							}
							// Выполняем функцию обратного вызова
							static_cast <const BasicFunction <void (void)> &> (* i->second).fn();
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
			}
			/**
			 * bind Метод вызова функции обратного вызова с сохранёнными параметрами
			 * @param name название функции обратного вызова
			 */
			void bind(const string & name) const noexcept {
				// Получаем идентификатор обратного вызова
				const uint64_t idw = this->idw(name);
				// Если название функции передано
				if((idw > 0) && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Выполняем поиск запрашиваемой функции
					auto i = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(i != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Результат работы функции
								const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
								// Выполняем функцию обратного вызова
								this->_callback(event_t::RUN, idw, name, &result);
							}
							// Выполняем функцию обратного вызова
							static_cast <const BasicFunction <void (void)> &> (* i->second).fn();
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
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
					auto i = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(i != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем поиск типа данных функции
							auto j = this->_types.find(i->first);
							// Если тип данных получен
							if(j != this->_types.end()){
								// Если возвращаемое значение совпадает с хранимым или функция возвращает значение
								if((j->second == this->type <A> ()) && (j->second != type_t::TYPE_VOID)){
									// Если функция обратного вызова установлена
									if(this->_callback != nullptr){
										// Результат работы функции
										const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
										// Выполняем функцию обратного вызова
										this->_callback(event_t::RUN, idw, "", &result);
									}
									// Выполняем функцию обратного вызова
									return static_cast <const BasicFunction <A (void)> &> (* i->second).fn();
								// Выводим сообщение об ошибке
								} else this->_log->print("Function does not match the specified call parameters", log_t::flag_t::WARNING);
							// Выводим сообщение, что функция не найдена в хранилище
							} else this->_log->print("Requested function was not found in the storage", log_t::flag_t::CRITICAL);
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
				const uint64_t idw = this->idw(name);
				// Если название функции передано
				if((idw > 0) && !this->_functions.empty() && (this->_functions.find(idw) != this->_functions.end())){
					// Выполняем поиск запрашиваемой функции
					auto i = this->_functions.find(idw);
					// Если запрашиваемая функция найдена
					if(i != this->_functions.end()){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Выполняем поиск типа данных функции
							auto j = this->_types.find(i->first);
							// Если тип данных получен
							if(j != this->_types.end()){
								// Если возвращаемое значение совпадает с хранимым или функция возвращает значение
								if((j->second == this->type <A> ()) && (j->second != type_t::TYPE_VOID)){
									// Если функция обратного вызова установлена
									if(this->_callback != nullptr){
										// Результат работы функции
										const dump_t result = make_pair(this->_functions.at(idw), this->_types.at(idw));
										// Выполняем функцию обратного вызова
										this->_callback(event_t::RUN, idw, name, &result);
									}
									// Выполняем функцию обратного вызова
									return static_cast <const BasicFunction <A (void)> &> (* i->second).fn();
								// Выводим сообщение об ошибке
								} else this->_log->print("\"%s\" function does not match the specified call parameters", log_t::flag_t::WARNING, name.c_str());
							// Выводим сообщение, что функция не найдена в хранилище
							} else this->_log->print("Requested \"%s\" function was not found in the storage", log_t::flag_t::CRITICAL, name.c_str());
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_function_call & error) {
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
					}
				}
				// Выводим значение по умолчанию
				return A();
			}
		public:
			/**
			 * callback Метод установки функции обратного события на получения событий модуля
			 * @param callback 
			 */
			void callback(function <void (const event_t, const uint64_t, const string &, const dump_t *)> callback) noexcept {
				// Выполняем установку функции обратного вызова
				this->_callback = callback;
			}
		public:
			/**
			 * Оператор [=] присвоения функций обратного вызова
			 * @param storage хранилище функций откуда нужно получить функции
			 * @return        текущий объект
			 */
			FN & operator = (const FN & storage) noexcept {
				// Если функции обратного вызова установлены
				if(!storage._functions.empty() && !storage._types.empty()){
					// Выполняем очистку списка функций обратного вызова
					this->clear();
					// Выполняем перебор всех параметров возвращаемых функциями
					for(auto & item : storage._types)
						// Устанавливаем возвращаемое значение
						this->_types.emplace(item.first, item.second);
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
			FN(const log_t * log) noexcept : _callback(nullptr), _log(log) {}
	} fn_t;
};

#endif // __AWH_FUNCTION__
