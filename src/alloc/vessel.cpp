/**
 * @file vessel.cpp
 * @date 2026-09-15
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
 * @brief Файл приёмника тайн — побайтного приёма в защищённую память
 *
 * \~english
 * @brief Secret vessel file — byte-by-byte intake into protected memory
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/vessel.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>

/**
 * @brief Безымянное пространство имён
 *
 */
namespace {
	/**
	 * @brief Метод затирания памяти, неустранимого собирателем
	 *
	 * @note Обычный `memset` собиратель выбрасывает как запись, какую никто не читает,
	 *       - и затирание тайны исчезает вместе с ним. Запись через изменчивый
	 *       указатель выбросить он не вправе
	 *
	 * @param addr адрес затираемой области
	 * @param size размер затираемой области в байтах
	 *
	 */
	static void erase(void * addr, const size_t size) noexcept {
		// Если затирать нечего
		if((addr == nullptr) || (size == 0))
			// Выходим из функции
			return;
		// Получаем изменчивый указатель на область
		volatile uint8_t * target = reinterpret_cast <volatile uint8_t *> (addr);
		/**
		 * Выполняем перебор всех байтов области
		 */
		for(size_t i = 0; i < size; i++)
			// Затираем очередной байт области
			target[i] = 0;
	}
};

/**
 * @brief Метод получения ёмкости приёмника
 *
 * @return ёмкость приёмника в байтах
 *
 */
size_t awh::alloc::Vessel::capacity() const noexcept {
	// Выводим ёмкость приёмника
	return this->_capacity;
}
/**
 * @brief Метод получения занятого в приёмнике
 *
 * @return занятое в приёмнике в байтах
 *
 */
size_t awh::alloc::Vessel::size() const noexcept {
	// Выводим занятое в приёмнике
	return this->_length;
}
/**
 * @brief Метод получения сведений о состоявшейся защите
 *
 * @return сведения о состоявшейся защите
 *
 */
const awh::alloc::shelter_t & awh::alloc::Vessel::shelter() const noexcept {
	// Выводим сведения о состоявшейся защите
	return this->_shelter;
}
/**
 * @brief Метод проверки заведённости приёмника
 *
 * @return признак заведённого приёмника
 *
 */
bool awh::alloc::Vessel::exists() const noexcept {
	// Выводим признак заведённого приёмника
	return ((this->_area != nullptr) && (this->_capacity > 0));
}
/**
 * @brief Метод заведения приёмника заданной ёмкости
 *
 * @param capacity ёмкость приёмника в байтах
 * @return         признак выполнения операции
 *
 */
bool awh::alloc::Vessel::reserve(const size_t capacity) noexcept {
	// Затираем и освобождаем прежнее содержимое приёмника
	this->wipe();
	// Если заводить нечего
	if(capacity == 0)
		// Отвечаем отказом
		return false;
	// Выдаём защищённую область под приёмник
	this->_area = awh::alloc::Allocator::secure(capacity, &this->_shelter);
	// Если область не выдана
	if(this->_area == nullptr){
		// Записываем отказ выдачи в журнал
		awh::log::print("Secret vessel: unable to allocate protected memory of %zu bytes",
		 awh::log::flag_t::CRITICAL, capacity);
		// Отвечаем отказом
		return false;
	}
	/**
	 * Строгий приёмник отвечает отказом, не получив обещанной защиты
	 *
	 * Молчаливое понижение защиты хуже честного отказа: заведший приёмник вправе знать,
	 * что содержимое его уйдёт в подкачку, ПРЕЖДЕ чем он туда что-то положит
	 */
	if(!this->_shelter.wired){
		// Если приёмник заведён строгим
		if(this->_secrecy == secrecy_t::STRICT){
			// Освобождаем заведённое
			this->wipe();
			// Записываем отказ заведения в журнал
			awh::log::print("Secret vessel: memory locking is unavailable, strict vessel refuses to hold the secret",
			 awh::log::flag_t::CRITICAL);
			// Отвечаем отказом
			return false;
		}
		// Записываем понижение защиты в журнал
		awh::log::print("Secret vessel: memory locking is unavailable, the secret may reach the swap",
		 awh::log::flag_t::WARNING);
	}
	// Запоминаем ёмкость приёмника
	this->_capacity = capacity;
	// Занятого в приёмнике пока нет
	this->_length = 0;
	// Затираем содержимое приёмника
	::erase(this->_area, capacity);
	// Закрываем область приёмника
	static_cast <void> (awh::alloc::Allocator::shield(this->_area, false));
	// Выводим результат выполнения операции
	return true;
}
/**
 * @brief Метод приёма очередного байта содержимого
 *
 * @param byte принимаемый байт
 * @return     признак выполнения операции
 *
 */
bool awh::alloc::Vessel::pour(const uint8_t byte) noexcept {
	// Если приёмник не заведён
	if(!this->exists())
		// Отвечаем отказом
		return false;
	/**
	 * Приём сверх ёмкости отвечает ОТКАЗОМ, а не ростом
	 *
	 * Рост означал бы перевыделение, перевыделение - копию прежнего содержимого в
	 * куче, какую никто не затирает, то есть ровно ту беду, от которой приёмник и
	 * заведён. Верхнюю границу объявляет зовущий: у ввода пароля она известна заранее
	 */
	if(this->_length >= this->_capacity)
		// Отвечаем отказом
		return false;
	// Открываем область приёмника
	static_cast <void> (awh::alloc::Allocator::shield(this->_area, true));
	// Записываем принимаемый байт в приёмник
	reinterpret_cast <uint8_t *> (this->_area)[this->_length] = byte;
	// Увеличиваем занятое в приёмнике
	this->_length++;
	// Закрываем область приёмника
	static_cast <void> (awh::alloc::Allocator::shield(this->_area, false));
	// Выводим результат выполнения операции
	return true;
}
/**
 * @brief Метод снятия последнего принятого байта
 *
 * @return признак выполнения операции
 *
 */
bool awh::alloc::Vessel::drop() noexcept {
	// Если приёмник не заведён либо пуст
	if(!this->exists() || (this->_length == 0))
		// Отвечаем отказом
		return false;
	// Открываем область приёмника
	static_cast <void> (awh::alloc::Allocator::shield(this->_area, true));
	// Уменьшаем занятое в приёмнике
	this->_length--;
	/**
	 * Снятый байт затираем НА МЕСТЕ
	 *
	 * Оставь мы его лежать за границей занятого - он остался бы в области до самого
	 * затирания приёмника, а приёмник ради того и заведён, чтобы лишнего в нём не
	 * лежало
	 */
	::erase((reinterpret_cast <uint8_t *> (this->_area) + this->_length), 1);
	// Закрываем область приёмника
	static_cast <void> (awh::alloc::Allocator::shield(this->_area, false));
	// Выводим результат выполнения операции
	return true;
}
/**
 * @brief Метод затирания и освобождения приёмника
 *
 */
void awh::alloc::Vessel::wipe() noexcept {
	// Если приёмник заведён
	if(this->_area != nullptr){
		/**
		 * Открываем область ПРЕЖДЕ затирания
		 *
		 * Затирание пишет в область, а запись в недоступную страницу валит программу.
		 * Открытие это к тому же снимает всякий незакрытый счёт раскрытий: считать их
		 * до нуля здесь некому, а область всё равно уходит
		 */
		static_cast <void> (awh::alloc::Allocator::shield(this->_area, true));
		// Затираем содержимое приёмника
		::erase(this->_area, this->_capacity);
		// Освобождаем область приёмника
		awh::alloc::Allocator::release(this->_area);
		// Сбрасываем адрес области приёмника
		this->_area = nullptr;
	}
	// Сбрасываем ёмкость приёмника
	this->_capacity = 0;
	// Сбрасываем занятое в приёмнике
	this->_length = 0;
	// Сбрасываем сведения о состоявшейся защите
	this->_shelter = shelter_t();
}
/**
 * @brief Конструктор
 *
 * @param secrecy строгость приёмника
 *
 */
awh::alloc::Vessel::Vessel(const secrecy_t secrecy) noexcept :
 _area(nullptr), _capacity(0), _length(0), _secrecy(secrecy), _shelter() {}
/**
 * @brief Деструктор
 *
 */
awh::alloc::Vessel::~Vessel() noexcept {
	// Затираем и освобождаем приёмник
	this->wipe();
}
/**
 * @brief Конструктор переноса
 *
 * @param vessel переносимый приёмник
 *
 */
awh::alloc::Vessel::Vessel(Vessel && vessel) noexcept :
 _area(vessel._area), _capacity(vessel._capacity), _length(vessel._length),
 _secrecy(vessel._secrecy), _shelter(vessel._shelter) {
	// Снимаем владение областью с переносимого приёмника
	vessel._area = nullptr;
	// Сбрасываем ёмкость переносимого приёмника
	vessel._capacity = 0;
	// Сбрасываем занятое в переносимом приёмнике
	vessel._length = 0;
}
/**
 * @brief Оператор переноса
 *
 * @param vessel переносимый приёмник
 * @return       ссылка на текущий объект
 *
 */
awh::alloc::Vessel & awh::alloc::Vessel::operator = (Vessel && vessel) noexcept {
	// Если переносится не сам объект
	if(this != &vessel){
		// Затираем и освобождаем своё содержимое
		this->wipe();
		// Забираем область переносимого приёмника
		this->_area = vessel._area;
		// Забираем ёмкость переносимого приёмника
		this->_capacity = vessel._capacity;
		// Забираем занятое в переносимом приёмнике
		this->_length = vessel._length;
		// Забираем строгость переносимого приёмника
		this->_secrecy = vessel._secrecy;
		// Забираем сведения о защите переносимого приёмника
		this->_shelter = vessel._shelter;
		// Снимаем владение областью с переносимого приёмника
		vessel._area = nullptr;
		// Сбрасываем ёмкость переносимого приёмника
		vessel._capacity = 0;
		// Сбрасываем занятое в переносимом приёмнике
		vessel._length = 0;
	}
	// Выводим ссылку на текущий объект
	return (* this);
}
