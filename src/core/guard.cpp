/**
 * @file: guard.cpp
 * @date: 2025-10-13
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

/**
 * Подключаем заголовочный файл
 */
#include <core/guard.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Оператор проверки доступа
 * 
 * @return результат проверки
 */
awh::Guard::Locker::operator bool() const noexcept {
	// Выполняем проверку доступности охранника
	return (this->_guard != nullptr);
}
/**
 * @brief Конструктор перемещения
 * 
 * @param other объект другого локера
 */
awh::Guard::Locker::Locker(locker_t && other) noexcept : _id(other._id), _guard(other._guard) {
	// Зануляем объект охранника у объекта другого локера
	other._guard = nullptr;
}
/**
 * @brief Конструктор
 * 
 * @param guard основной объект сторожа
 */
awh::Guard::Locker::Locker(guard_t & guard) noexcept {
	// Если функция не заблокированна для всех
	if(guard._ok){
		// Выполняем блокировку функции для всех
		guard._ok = !guard._ok;
		// Выполняем установку объекта охранника
		this->_guard = &guard;
	}
}
/**
 * @brief Конструктор
 * 
 * @param id    изентификатор желающий захватить функцию
 * @param guard основной объект сторожа
 */
awh::Guard::Locker::Locker(const uint32_t id, guard_t & guard) noexcept : _id(id), _guard(nullptr) {
	// Если функция не заблокированна для всех
	if(guard._ok){
		// Выполняем блокировку потока
		lock_guard lock(guard._mtx);
		// Если идентификатор успешно добавлен
		if(guard._ids.insert(id).second)
			// Выполняем установку объекта охранника
			this->_guard = &guard;
	}
}
/**
 * @brief Деструктор
 * 
 */
awh::Guard::Locker::~Locker() noexcept {
	// Если объект охранника установлен
	if(this->_guard != nullptr){
		// Если функция заблокированна для всех
		if(!this->_guard->_ok)
			// Выполняем разблокировку функции
			this->_guard->_ok = !this->_guard->_ok;
		// Если функция заблокированна не для всех
		else {
			// Выполняем блокировку потока
			lock_guard lock(this->_guard->_mtx);
			// Выполняем удаление идентификатора
			this->_guard->_ids.erase(this->_id);
		}
	}
}
/**
 * @brief Метод проверки на доступ к функции
 * 
 * @return результат проверки
 */
bool awh::Guard::locked() noexcept {
	// Выполняем проверку доступа
	return !this->_ok;
}
/**
 * @brief Метод проверки на доступ к функции
 * 
 * @param id идентификатор проверяющий доступ
 * @return   результат проверки
 */
bool awh::Guard::locked(const uint32_t id) noexcept {
	// Выполняем проверку доступа
	return (!this->_ok || (this->_ids.find(id) != this->_ids.end()));
}
/**
 * @brief Метод выполнения блокировки доступа к функции
 * 
 * @return результат блокировки
 */
awh::Guard::locker_t awh::Guard::lock() noexcept {
	// Выполняем создание локера
	return locker_t(* this);
}
/**
 * @brief Метод выполнения блокировки доступа к функции
 * 
 * @param id идентификатор желающий захватить доступ
 * @return   результат блокировки
 */
awh::Guard::locker_t awh::Guard::lock(const uint32_t id) noexcept {
	// Выполняем создание локера
	return locker_t(id, * this);
}
/**
 * @brief Конструктор
 * 
 */
awh::Guard::Guard() noexcept : _ok(true) {}
