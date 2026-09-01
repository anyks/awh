/**
 * @file schedule.cpp
 * @date 2026-08-19
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
 * @brief Файл реализации отбоя срока бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the beating out of the deadline of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/schedule.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Функция получения нынешнего штампа времени
	 *
	 * @return нынешний штамп времени в миллисекундах
	 *
	 */
	uint64_t stamp() noexcept {
		// Выводим нынешний штамп времени от начала отсчёта устойчивых часов
		return static_cast <uint64_t> (chrono::duration_cast <chrono::milliseconds> (
		 chrono::steady_clock::now().time_since_epoch()).count());
	}
};

/**
 * @brief Метод отбоя срока своим потоком
 *
 */
void awh::codec::abc::Schedule::run() noexcept {
	/**
	 * Выполняем отбой срока, покуда работа отбоя не остановлена
	 */
	while(true){
		// Выполняем захват замка состояния отбоя срока
		unique_lock <mutex> lock(this->_mtx);
		/**
		 * Выполняем ожидание наступления срока на условной переменной.
		 *
		 * Ожидание идёт на переменной, а не выдержкой: выдержка обязала бы остановку
		 * дожидаться конца срока, а это зависание, а не остановка
		 */
		this->_cond.wait_for(lock, chrono::milliseconds(this->_delay), [this]() noexcept -> bool {
			// Выводим признак остановки работы отбоя срока
			return !this->_working;
		});
		/**
		 * Если работа отбоя срока остановлена, выходим из потока
		 */
		if(!this->_working)
			// Прекращаем отбой срока
			return;
		// Выполняем получение работы, зовомой по наступлении срока
		function <void (void)> callback = this->_callback;
		// Выполняем установку штампа времени наступившего срока
		this->_stamp = stamp();
		// Выполняем освобождение замка состояния отбоя срока
		lock.unlock();
		/**
		 * Если работа по наступлении срока установлена, выполняем её вне замка:
		 * работа эта ходит в правщика со своим замком, и держать при том свой
		 * значило бы сводить два замка в один порядок захвата
		 */
		if(callback != nullptr)
			// Выполняем работу, зовомую по наступлении срока
			callback();
	}
}
/**
 * @brief Метод установки работы, зовомой по наступлении срока
 *
 * @param callback устанавливаемая работа
 *
 */
void awh::codec::abc::Schedule::callback(function <void (void)> callback) noexcept {
	// Выполняем захват замка состояния отбоя срока
	lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку работы, зовомой по наступлении срока
	this->_callback = ::std::move(callback);
}
/**
 * @brief Метод запуска отбоя срока
 *
 * @param mode  способ отбоя срока
 * @param delay срок отбоя в миллисекундах
 * @return      признак успешного запуска
 *
 */
bool awh::codec::abc::Schedule::start(const mode_t mode, const uint32_t delay) noexcept {
	// Выполняем остановку прежнего отбоя срока
	this->stop();
	/**
	 * Если отбой срока не затребован вовсе
	 */
	if((mode == mode_t::NONE) || (delay == 0))
		// Выводим признак того, что отбой срока не запущен
		return false;
	// Выполняем захват замка состояния отбоя срока
	unique_lock <mutex> lock(this->_mtx);
	// Выполняем установку способа отбоя срока
	this->_mode = mode;
	// Выполняем установку срока отбоя
	this->_delay = delay;
	// Выполняем установку признака работы отбоя срока
	this->_working = true;
	// Выполняем установку штампа времени начала отсчёта срока
	this->_stamp = stamp();
	/**
	 * Если срок отбивается своим потоком
	 */
	if(mode == mode_t::THREAD){
		// Выполняем освобождение замка состояния отбоя срока
		lock.unlock();
		// Выполняем заведение потока отбоя срока
		this->_thread = thread(&Schedule::run, this);
	}
	// Выводим признак успешного запуска отбоя срока
	return true;
}
/**
 * @brief Метод остановки отбоя срока
 *
 */
void awh::codec::abc::Schedule::stop() noexcept {
	{
		// Выполняем захват замка состояния отбоя срока
		lock_guard <mutex> lock(this->_mtx);
		// Выполняем снятие признака работы отбоя срока
		this->_working = false;
	}
	// Выполняем пробуждение потока отбоя срока
	this->_cond.notify_all();
	/**
	 * Если поток отбоя срока заведён
	 *
	 * @details Выхода вперёд по снятому признаку работы здесь НЕТ намеренно: прежде он
	 *          стоял, и остановка, застав отбой уже остановленным, ожидания конца потока
	 *          не делала. Поток при том оставался ожидаемым, а следующий запуск клал
	 *          новый поток поверх ожидаемого - и это `std::terminate` по стандарту
	 */
	if(this->_thread.joinable()){
		/**
		 * Если остановка затребована ИЗ САМОГО потока отбоя срока
		 *
		 * @details Ожидание себя самого стандартом запрещено: `join` отвечает отказом
		 *          «Resource deadlock avoided», отказ этот есть исключение, а остановка
		 *          объявлена `noexcept` - и работа снималась целиком. Замер 01.09.2026:
		 *          остановка, зовомая из отклика наступившего срока, валила щуп с кодом
		 *          134. Ход же этот законен и потребителю естественен - «отбей однажды
		 *          и остановись»
		 *
		 * @note Ожидание не теряется, а ПЕРЕКЛАДЫВАЕТСЯ: признак работы снят, поток
		 *       выйдет сам, едва отклик вернётся, а ожидание конца его возьмёт на себя
		 *       следующая остановка либо разрушитель. Отвязывать поток здесь нельзя -
		 *       отвязанный, он трогал бы замок объекта уже разрушенного
		 */
		if(this->_thread.get_id() != this_thread::get_id())
			// Выполняем ожидание конца потока отбоя срока
			this->_thread.join();
	}
	// Выполняем захват замка состояния отбоя срока
	lock_guard <mutex> lock(this->_mtx);
	// Выполняем сброс способа отбоя срока
	this->_mode = mode_t::NONE;
}
/**
 * @brief Метод поверки наступления срока при обращении
 *
 * @return признак наступления срока
 *
 */
bool awh::codec::abc::Schedule::touch() noexcept {
	// Выполняем захват замка состояния отбоя срока
	lock_guard <mutex> lock(this->_mtx);
	/**
	 * Если срок отбивается не штампом времени, поверка эта не о нём
	 */
	if(!this->_working || (this->_mode != mode_t::DEADLINE))
		// Выводим признак того, что срок не наступил
		return false;
	// Выполняем получение нынешнего штампа времени
	const uint64_t current = stamp();
	/**
	 * Если срок ещё не вышел
	 */
	if((current - this->_stamp) < static_cast <uint64_t> (this->_delay))
		// Выводим признак того, что срок не наступил
		return false;
	// Выполняем установку штампа времени наступившего срока
	this->_stamp = current;
	// Выводим признак наступления срока
	return true;
}
/**
 * @brief Метод проверки работы отбоя срока
 *
 * @return признак работы отбоя срока
 *
 */
bool awh::codec::abc::Schedule::working() const noexcept {
	/**
	 * Выполняем захват замка состояния отбоя срока
	 *
	 * @note Признак этот правится потоком отбоя, и чтение его без замка есть гонка -
	 *       TSan её видит. Замок здесь ничего не стоит: спрашивают состояние редко
	 */
	lock_guard <mutex> lock(this->_mtx);
	// Выводим признак работы отбоя срока
	return this->_working;
}
/**
 * @brief Метод извлечения способа отбоя срока
 *
 * @return способ отбоя срока
 *
 */
awh::codec::abc::Schedule::mode_t awh::codec::abc::Schedule::mode() const noexcept {
	// Выполняем захват замка состояния отбоя срока
	lock_guard <mutex> lock(this->_mtx);
	// Выводим способ отбоя срока
	return this->_mode;
}
/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Schedule::Schedule() noexcept :
 _mode(mode_t::NONE), _delay(0), _working(false), _stamp(0), _callback(nullptr) {}
/**
 * @brief Деструктор
 *
 */
awh::codec::abc::Schedule::~Schedule() noexcept {
	// Выполняем остановку отбоя срока
	this->stop();
}
