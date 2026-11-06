/**
 * @file smallvec.hpp
 * @date 2026-11-05
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
 * @brief Заголовочный файл вектора с малым инлайн-хранилищем (small buffer optimization) —
 *        шаблон quic::Small_Vector, размещающий первые N элементов внутри самого объекта
 *        и переходящий в кучу только при росте сверх этой ёмкости
 *
 * \~english
 * @brief Header file of the vector with small-buffer optimization — the template quic::Small_Vector
 *        placing the first N elements inside the object itself and moving to the heap
 *        only when the growth exceeds this capacity
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <new>
#include <utility>
#include <cstddef>

/**
 * \~russian
 * @brief основное пространство имён
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 * \~english
	 * @brief QUIC transport protocol namespace
	 *
	 * \~
	 */
	namespace quic {
		/**
		 * \~russian
		 * @brief Шаблон класса вектора с малым инлайн-хранилищем (small buffer optimization)
		 *
		 * @tparam T тип элемента
		 * @tparam N число элементов инлайн-хранилища
		 *
		 * \~english
		 * @brief Vector class template with small-buffer optimization
		 *
		 * @tparam T type of an element
		 * @tparam N number of the elements of the inline storage
		 *
		 * \~
		 */
		template <class T, size_t N>
		/**
		 * \~russian
		 * @brief Класс вектора с малым инлайн-хранилищем (small buffer optimization)
		 *
		 * @details Первые N элементов размещаются в самом объекте без обращения к куче;
		 *          свыше N хранилище переносится в кучу с удвоением ёмкости. Применяется
		 *          для учётных записей отправленных блоков потоков в пакете: типовой
		 *          пакет несёт один STREAM-фрейм, поэтому инлайн-хранилища на N элементов
		 *          хватает и выделение на каждый пакет не выполняется. При переносе в
		 *          кучу ёмкость удерживается методом clear() (по образцу пула отправки).
		 *          Реализован минимальный набор операций, требуемый учётной записью пакета.
		 *
		 * @tparam T тип элемента
		 * @tparam N число элементов инлайн-хранилища
		 *
		 * \~english
		 * @brief Vector class with small-buffer optimization
		 *
		 * @details The first N elements are placed in the object itself without an address to the heap;
		 *          above N the storage is carried over into the heap with a doubling of the capacity. It is applied
		 *          for the account records of the sent blocks of the streams in a packet: a typical
		 *          packet carries one STREAM frame, therefore an inline storage for N elements
		 *          suffices and an allocation per every packet is not performed. At a carrying over into
		 *          the heap the capacity is held by the method clear() (by the model of the pool of the sending).
		 *          A minimal collection of the operations required by an account record of a packet is implemented.
		 *
		 * @tparam T type of an element
		 * @tparam N number of the elements of the inline storage
		 *
		 * \~
		 */
		class Small_Vector {
			private:
				/**
				 * Кучевое хранилище выделяется базовым ::operator new, гарантирующим
				 * выравнивание лишь до __STDCPP_DEFAULT_NEW_ALIGNMENT__. Тип с большим
				 * требованием выравнивания был бы недовыровнен на кучевом пути, поэтому
				 * запрещаем его на этапе компиляции (для chunk_t выравнивание равно 8)
				 */
				static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__, "Small_Vector: over-aligned element type is unsupported on the heap path");
			private:
				// Число хранимых элементов
				size_t _size;
				// Ёмкость текущего хранилища в элементах
				size_t _cap;
				// Указатель на активное хранилище (инлайн либо куча)
				T * _data;
			private:
				/**
				 * \~russian
				 * @brief Инлайн-хранилище первых N элементов
				 *
				 * @note Поле звалось прежде _inline, и под MinGW имя это непригодно:
				 *       заголовок _mingw.h заводит "#define _inline __inline" ради
				 *       совместимости с MSVC, и объявление превращалось в
				 *       "unsigned char __inline[...]". Переименовано, а не закрыто снятием
				 *       макроса: поле частное и внутреннее, открытого API не касается,
				 *       а имя, совпадающее с ключевым словом целевой системы, ненадёжно само
				 *       по себе
				 *
				 * \~english
				 * @brief Inline storage of the first N elements
				 *
				 * @note The field was called _inline before, and under MinGW this name is unsuitable:
				 *       the header _mingw.h starts a "#define _inline __inline" for the sake of
				 *       a compatibility with MSVC, and the declaration turned into
				 *       "unsigned char __inline[...]". It is renamed rather than closed by a removal
				 *       of the macro: the field is a private and an internal one, it does not touch the open API,
				 *       while a name coinciding with a keyword of the target system is unreliable by
				 *       itself
				 *
				 * \~
				 */
				alignas(T) unsigned char _storage[N * sizeof(T)];
			private:
				/**
				 * \~russian
				 * @brief Метод получения указателя на инлайн-хранилище
				 *
				 * @return указатель на инлайн-хранилище
				 *
				 * \~english
				 * @brief Method of getting the pointer to the inline storage
				 *
				 * @return pointer to the inline storage
				 *
				 * \~
				 */
				T * inlined() noexcept {
					// Выводим указатель на инлайн-хранилище
					return reinterpret_cast <T *> (this->_storage);
				}
				/**
				 * \~russian
				 * @brief Метод проверки размещения хранилища в инлайн-буфере
				 *
				 * @return признак размещения в инлайн-буфере
				 *
				 * \~english
				 * @brief Method of checking the placement of the storage in the inline buffer
				 *
				 * @return flag of the placement in the inline buffer
				 *
				 * \~
				 */
				bool isInlined() const noexcept {
					// Выводим признак размещения хранилища в инлайн-буфере
					return (this->_data == reinterpret_cast <const T *> (this->_storage));
				}
				/**
				 * \~russian
				 * @brief Метод расширения хранилища переносом в кучу
				 *
				 * @param need требуемое минимальное число элементов
				 *
				 * \~english
				 * @brief Method of the extension of the storage by a carrying over into the heap
				 *
				 * @param need required smallest number of the elements
				 *
				 * \~
				 */
				void grow(const size_t need) noexcept {
					// Вычисляем новую ёмкость удвоением
					size_t capacity = (this->_cap ? (this->_cap * 2) : N);
					// Если удвоенной ёмкости не хватает - берём требуемую
					if(capacity < need)
						capacity = need;
					// Выделяем новое хранилище в куче
					T * data = static_cast <T *> (operator new(capacity * sizeof(T)));
					/**
					 * Переносим элементы в новое хранилище
					 */
					for(size_t i = 0; i < this->_size; i++){
						// Перемещаем элемент в новое хранилище
						new (data + i) T(std::move(this->_data[i]));
						// Уничтожаем исходный элемент
						this->_data[i].~T();
					}
					// Если прежнее хранилище было в куче - освобождаем его
					if(!this->isInlined())
						// Освобождаем прежнее хранилище кучи
						operator delete(this->_data);
					// Переключаемся на новое хранилище
					this->_data = data;
					// Запоминаем новую ёмкость
					this->_cap = capacity;
				}
				/**
				 * \~russian
				 * @brief Метод переноса содержимого из другого вектора
				 *
				 * @param other вектор-источник (опустошается)
				 *
				 * \~english
				 * @brief Method of the carrying over of the content from another vector
				 *
				 * @param other vector-source (is emptied)
				 *
				 * \~
				 */
				void adopt(Small_Vector && other) noexcept {
					// Если источник размещён в куче - забираем его хранилище
					if(!other.isInlined()){
						// Забираем указатель на хранилище кучи
						this->_data = other._data;
						// Забираем ёмкость хранилища
						this->_cap = other._cap;
						// Забираем число элементов
						this->_size = other._size;
						// Переводим источник на пустое инлайн-хранилище
						other._data = other.inlined();
						// Восстанавливаем инлайн-ёмкость источника
						other._cap = N;
						// Обнуляем число элементов источника
						other._size = 0;
					// Если источник размещён инлайн - перемещаем его элементы поэлементно
					} else {
						/**
						 * Перемещаем элементы источника в собственное инлайн-хранилище
						 */
						for(size_t i = 0; i < other._size; i++){
							// Перемещаем элемент источника
							::new (this->_data + i) T(std::move(other._data[i]));
							// Уничтожаем исходный элемент
							other._data[i].~T();
						}
						// Забираем число элементов источника
						this->_size = other._size;
						// Обнуляем число элементов источника
						other._size = 0;
					}
				}
			public:
				/**
				 * \~russian
				 * @brief Метод проверки пустоты вектора
				 *
				 * @return признак отсутствия элементов
				 *
				 * \~english
				 * @brief Method of checking the emptiness of the vector
				 *
				 * @return flag of the absence of the elements
				 *
				 * \~
				 */
				bool empty() const noexcept {
					// Выводим признак отсутствия элементов
					return (this->_size == 0);
				}
				/**
				 * \~russian
				 * @brief Метод получения числа элементов
				 *
				 * @return число хранимых элементов
				 *
				 * \~english
				 * @brief Method of getting the number of the elements
				 *
				 * @return number of the stored elements
				 *
				 * \~
				 */
				size_t size() const noexcept {
					// Выводим число хранимых элементов
					return this->_size;
				}
				/**
				 * \~russian
				 * @brief Метод получения указателя на начало
				 *
				 * @return указатель на первый элемент
				 *
				 * \~english
				 * @brief Method of getting the pointer to the beginning
				 *
				 * @return pointer to the first element
				 *
				 * \~
				 */
				T * begin() noexcept { return this->_data; }
				/**
				 * \~russian
				 * @brief Метод получения указателя на конец
				 *
				 * @return указатель за последним элементом
				 *
				 * \~english
				 * @brief Method of getting the pointer to the end
				 *
				 * @return pointer behind the last element
				 *
				 * \~
				 */
				T * end() noexcept { return (this->_data + this->_size); }
				/**
				 * \~russian
				 * @brief Метод получения константного указателя на начало
				 *
				 * @return константный указатель на первый элемент
				 *
				 * \~english
				 * @brief Method of getting the constant pointer to the beginning
				 *
				 * @return constant pointer to the first element
				 *
				 * \~
				 */
				const T * begin() const noexcept { return this->_data; }
				/**
				 * \~russian
				 * @brief Метод получения константного указателя на конец
				 *
				 * @return константный указатель за последним элементом
				 *
				 * \~english
				 * @brief Method of getting the constant pointer to the end
				 *
				 * @return constant pointer behind the last element
				 *
				 * \~
				 */
				const T * end() const noexcept { return (this->_data + this->_size); }
				/**
				 * \~russian
				 * @brief Метод добавления элемента переносом в конец
				 *
				 * @param value добавляемый элемент
				 *
				 * \~english
				 * @brief Method of adding an element by a moving into the end
				 *
				 * @param value element being added
				 *
				 * \~
				 */
				void push(T && value) noexcept {
					// Если хранилище исчерпано - расширяем его
					if(this->_size == this->_cap)
						// Расширяем хранилище под ещё один элемент
						this->grow(this->_size + 1);
					// Размещаем элемент в конце хранилища
					::new (this->_data + this->_size) T(std::move(value));
					// Увеличиваем число элементов
					this->_size++;
				}
				/**
				 * \~russian
				 * @brief Метод очистки с сохранением ёмкости хранилища
				 *
				 * \~english
				 * @brief Method of the clearing with the preservation of the capacity of the storage
				 *
				 * \~
				 */
				void clear() noexcept {
					/**
					 * Уничтожаем хранимые элементы
					 */
					for(size_t i = 0; i < this->_size; i++)
						// Уничтожаем элемент
						this->_data[i].~T();
					// Обнуляем число элементов, сохраняя выделенную ёмкость
					this->_size = 0;
				}
				/**
				 * \~russian
				 * @brief Метод обмена содержимым с другим вектором
				 *
				 * @param other вектор для обмена
				 *
				 * \~english
				 * @brief Method of the exchange of the content with another vector
				 *
				 * @param other vector for the exchange
				 *
				 * \~
				 */
				void swap(Small_Vector & other) noexcept {
					// Выполняем обмен через перемещения (инлайн-хранилище исключает обмен указателями)
					Small_Vector temp(std::move(other));
					// Переносим собственное содержимое в источник
					other = std::move(* this);
					// Переносим временное содержимое в себя
					* this = std::move(temp);
				}
			public:
				/**
				 * \~russian
				 * @brief Запрещаем оператор копирования - учётная запись пакета перемещается, но не копируется
				 *
				 * \~english
				 * @brief We disable the copy operator — a package account is moved, not copied.
				 *
				 * \~
				 */
				Small_Vector & operator = (const Small_Vector &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Оператор перемещения
				 *
				 * @param other вектор-источник
				 * @return      ссылка на текущий вектор
				 *
				 * \~english
				 * @brief Operator of the moving
				 *
				 * @param other vector-source
				 * @return      reference to the current vector
				 *
				 * \~
				 */
				Small_Vector & operator = (Small_Vector && other) noexcept {
					// Если присваивается не сам себе
					if(this != & other){
						/**
						 * Уничтожаем собственные элементы
						 */
						for(size_t i = 0; i < this->_size; i++)
							// Уничтожаем элемент
							this->_data[i].~T();
						// Если собственное хранилище в куче - освобождаем и возвращаемся в инлайн
						if(!this->isInlined()){
							// Освобождаем хранилище кучи
							::operator delete(this->_data);
							// Возвращаемся на инлайн-хранилище
							this->_data = this->inlined();
							// Восстанавливаем инлайн-ёмкость
							this->_cap = N;
						}
						// Обнуляем число элементов
						this->_size = 0;
						// Переносим содержимое источника
						this->adopt(std::move(other));
					}
					// Выводим ссылку на текущий вектор
					return * this;
				}
			public:
				/**
				 * \~russian
				 * @brief Запрещаем оператор копирования - учётная запись пакета перемещается, но не копируется
				 *
				 * \~english
				 * @brief We disable the copy operator — a package account is moved, not copied.
				 *
				 * \~
				 */
				explicit Small_Vector(const Small_Vector &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор перемещения
				 *
				 * @param other вектор-источник
				 *
				 * \~english
				 * @brief Constructor of the moving
				 *
				 * @param other vector-source
				 *
				 * \~
				 */
				explicit Small_Vector(Small_Vector && other) noexcept :
				 _size(0), _cap(N), _data(reinterpret_cast <T *> (this->_storage)) {
					// Переносим содержимое источника
					this->adopt(std::move(other));
				}
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Small_Vector() noexcept :
				 _size(0), _cap(N), _data(reinterpret_cast <T *> (this->_storage)) {}
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Small_Vector() noexcept {
					/**
					 * Уничтожаем хранимые элементы
					 */
					for(size_t i = 0; i < this->_size; i++)
						// Уничтожаем элемент
						this->_data[i].~T();
					// Если хранилище в куче - освобождаем его
					if(!this->isInlined())
						// Освобождаем хранилище кучи
						::operator delete(this->_data);
				}
		};
		/**
		 * \~russian
		 * @brief Шаблон класса вектора с малым инлайн-хранилищем (small buffer optimization)
		 *
		 * @tparam T тип элемента
		 * @tparam N число элементов инлайн-хранилища
		 *
		 * \~english
		 * @brief Vector class template with small-buffer optimization
		 *
		 * @tparam T type of an element
		 * @tparam N number of the elements of the inline storage
		 *
		 * \~
		 */
		template <class T, size_t N>
		/**
		 * \~russian
		 * @brief Создаём тип данных класса вектора с малым инлайн-хранилищем (small buffer optimization)
		 *
		 * \~english
		 * @brief Creating a vector-like data type with small-buffer optimization (inline storage)
		 *
		 * \~
		 */
		using small_vector_t = Small_Vector <T, N>;
	};
};
