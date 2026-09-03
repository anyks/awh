/**
 * @file schedule.hpp
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
 * @brief Заголовочный файл отбоя срока бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the beating out of the deadline of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_SCHEDULE__
#define __AWH_CODEC_ABC_SCHEDULE__

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <thread>
#include <vector>
#include <cstdint>
#include <functional>
#include <condition_variable>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
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
	 * @brief Пространство имён контейнеров данных
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён бинарного контейнера ABC
		 *
		 * \~english
		 * @brief ABC binary container namespace
		 *
		 * \~
		 */
		namespace abc {
			/**
			 * \~russian
			 * @brief Класс отбоя срока
			 *
			 * @details Кодек сетевого движка не тянет и тянуть не станет: зависимость эта
			 * обратила бы всякую работу с файлом в работу с сетью. Оттого срок отбивается
			 * одним из трёх путей, и выбор между ними за потребителем
			 *
			 * @details **Путь первый - юнит таймера сетевого движка.** Кодеку для того не
			 * нужно ничего: потребитель подписывает свой таймер на фиксацию сам. Путь этот
			 * и надлежит там, где движок уже поднят - свой поток был бы вторым сроком при
			 * одном уже отбиваемом
			 *
			 * @details **Путь второй - штамп времени, поверяемый при обращении.** Ни потока,
			 * ни замка: срок поверяется тем же вызовом, каким вносится запись. Тем он и
			 * ограничен - без обращений срок не наступит вовсе, и последняя пачка будет
			 * ждать следующей записи
			 *
			 * @details **Путь третий - свой поток.** Срок отбивается и в тишине, но работа
			 * зовётся чужим потоком, и состояние приходится сторожить замком
			 *
			 * @note Ожидание идёт на условной переменной, а не выдержкой: выдержка обязала
			 * бы остановку дожидаться конца срока, а отсрочка выхода на минуту при остановке
			 * на минутном сроке - это зависание, а не остановка
			 *
			 * \~english
			 * @brief Class of the beating out of a deadline
			 * @details The codec does not pull the network engine and will not: this dependency
			 * would turn every work with a file into a work with the network. Therefore the deadline is beaten out
			 * by one of three ways, and the choice among them is the consumer's
			 * @details **The first way is the unit of the timer of the network engine.** The codec needs
			 * nothing for that: the consumer subscribes its own timer to the commit itself. This way
			 * is proper where the engine is already raised — an own thread would be a second deadline while one is
			 * already being beaten out
			 * @details **The second way is a timestamp checked at an appeal.** Neither a thread
			 * nor a lock: the deadline is checked by the same call by which a record is added. By that it is
			 * limited as well — without appeals the deadline will not come at all, and the last batch will
			 * wait for the next record
			 * @details **The third way is an own thread.** The deadline is beaten out in the silence as well, but the work
			 * is called by a foreign thread, and the state has to be guarded by a lock
			 * @note The waiting goes on a conditional variable rather than by a sleep: a sleep would oblige
			 * the stopping to wait for the end of the deadline, and a postponement of the exit by a minute at a stopping
			 * at a minute deadline is a hanging rather than a stopping
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Schedule {
				public:
					/**
					 * \~russian
					 * @brief Способы отбоя срока
					 *
					 * \~english
					 * @brief Ways of the beating out of a deadline
					 *
					 * \~
					 */
					enum class mode_t : uint8_t {
						NONE     = 0x00, // Срок не отбивается вовсе
						DEADLINE = 0x01, // Срок поверяется штампом времени при обращении
						THREAD   = 0x02  // Срок отбивается своим потоком
					};
				private:
					// Способ отбоя срока
					mode_t _mode;
				private:
					// Срок отбоя в миллисекундах
					uint32_t _delay;
				private:
					// Признак работы отбоя срока
					bool _working;
				private:
					// Штамп времени последнего отбоя срока в миллисекундах
					uint64_t _stamp;
				private:
					// Замок состояния отбоя срока
					mutable mutex _mtx;
				private:
					// Условная переменная ожидания срока
					condition_variable _cond;
				private:
					// Поток отбоя срока
					thread _thread;
					/**
					 * \~russian
					 * Потоки отбоя, ушедшие в отставку и ждущие ожидания
					 *
					 * @details Поток, остановленный ИЗ САМОГО СЕБЯ, ожидать нельзя - стандарт зовёт
					 * это тупиком, - но и оставить его в поле нельзя тоже: следующий запуск положил
					 * бы новый поток поверх ожидаемого, а это `std::terminate`. Отставка разводит
					 * два эти требования: поле освобождается тут же, а ожидание берёт на себя
					 * следующая остановка либо разрушитель, когда звать их будет уже иной поток
					 *
					 * @note Отвязать такой поток нельзя: отвязанный, он трогал бы замок объекта уже
					 * разрушенного
					 *
					 * \~english
					 * Threads of the deadline retired and awaiting the joining
					 *
					 * \~
					 */
					vector <thread> _retired;
				private:
					// Работа, зовомая по наступлении срока
					function <void (void)> _callback;
				private:
					/**
					 * \~russian
					 * @brief Метод отбоя срока своим потоком
					 *
					 *
					 * \~english
					 * @brief Method of the beating out of a deadline by an own thread
					 *
					 * \~
					 */
					void run() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки работы, зовомой по наступлении срока
					 *
					 * @param callback устанавливаемая работа
					 *
					 * \~english
					 * @brief Method of the setting of the work called upon the coming of the deadline
					 * @param callback work being set
					 *
					 * \~
					 */
					void callback(function <void (void)> callback) noexcept;
					/**
					 * \~russian
					 * @brief Метод запуска отбоя срока
					 *
					 * @param mode  способ отбоя срока
					 * @param delay срок отбоя в миллисекундах
					 * @return      признак успешного запуска
					 *
					 * \~english
					 * @brief Method of the start of the beating out of a deadline
					 * @param mode way of the beating out of the deadline
					 * @param delay deadline in milliseconds
					 * @return sign of a successful start
					 *
					 * \~
					 */
					[[nodiscard]] bool start(const mode_t mode, const uint32_t delay) noexcept;
					/**
					 * \~russian
					 * @brief Метод остановки отбоя срока
					 *
					 *
					 * @note **Остановку ВОЛЬНО звать из самого отклика наступившего срока.**
					 * Ход этот потребителю естественен - «отбей однажды и остановись», - и
					 * иного мига остановить отбой ему неоткуда. Ожидание конца потока при том
					 * не делается (ожидать себя самого стандарт запрещает), а перекладывается
					 * на следующую остановку либо на разрушитель: признак работы снят, и поток
					 * выйдет сам, едва отклик вернётся
					 *
					 * @note Остановка, застав отбой уже остановленным, работы не бросает:
					 * ожидание конца потока делается и в этом случае. Прежде тут стоял выход
					 * вперёд, и он терял ожидание после остановки из отклика - поток оставался
					 * ожидаемым, а разрушитель его либо новый запуск поверх него дают
					 * `std::terminate` по стандарту. Закреплено проверками
					 * `CodecAbcSchedule.StopFromCallbackDoesNotAbort` и
					 * `CodecAbcSchedule.RestartAfterStopFromCallbackSurvives`
					 *
					 * @warning Запуск и остановка от РАЗНЫХ потоков между собою не разведены:
					 * поле потока читается вне замка, ибо под замком его ожидать нельзя -
					 * поток берёт тот же замок. Договор таков: заводит и останавливает отбой
					 * тот, кто им владеет, а отклику вольна лишь остановка
					 *
					 * \~english
					 * @brief Method of the stop of the beating out of a deadline
					 * @note **It is allowed to call the stop from the callback of the come deadline itself.**
					 * @warning The start and the stop from DIFFERENT threads are not separated between themselves
					 *
					 * \~
					 */
					void stop() noexcept;
					/**
					 * \~russian
					 * @brief Метод поверки наступления срока при обращении
					 *
					 * @details Поверка эта надлежит способу штампа времени: при прочих
					 * способах она отвечает отказом, ибо срок отбивается не ею
					 *
					 * @return признак наступления срока
					 *
					 * \~english
					 * @brief Method of the checking of the coming of the deadline at an appeal
					 * @details This checking is proper to the way of the timestamp: at the other
					 * ways it answers by a refusal, for the deadline is beaten out not by it
					 * @return sign of the coming of the deadline
					 *
					 * \~
					 */
					[[nodiscard]] bool touch() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки работы отбоя срока
					 *
					 * @return признак работы отбоя срока
					 *
					 * \~english
					 * @brief Method of the checking of the work of the beating out of a deadline
					 * @return sign of the work of the beating out of the deadline
					 *
					 * \~
					 */
					[[nodiscard]] bool working() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения способа отбоя срока
					 *
					 * @return способ отбоя срока
					 *
					 * \~english
					 * @brief Method of the extraction of the way of the beating out of a deadline
					 * @return way of the beating out of the deadline
					 *
					 * \~
					 */
					mode_t mode() const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Конструктор копирования (запрещаем)
					 *
					 *
					 * \~english
					 * @brief Copy constructor (prohibited)
					 *
					 * \~
					 */
					Schedule(const Schedule &) = delete;
					/**
					 * \~russian
					 * @brief Оператор копирования (запрещаем)
					 *
					 * @return текущее значение объекта
					 *
					 * \~english
					 * @brief Copy assignment operator (prohibited)
					 * @return current value of the object
					 *
					 * \~
					 */
					Schedule & operator = (const Schedule &) = delete;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @details Журнала расписание не берёт намеренно: доносить ему не о чем -
					 * «ложь» его вызовов означает «отбой не затребован» либо «срок не наступил»,
					 * а это исходы работы, а не отказы. Пара берётся там, где она нужна
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Schedule() noexcept;
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
					~Schedule() noexcept;
			} schedule_t;
		};
	};
};

#endif // __AWH_CODEC_ABC_SCHEDULE__
