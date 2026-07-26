/**
 * @file: threadpool.hpp
 * @date: 2023-12-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля пула потоков — класс Threadpool, распределяющий пользовательские задачи по
 *        фиксированному набору рабочих потоков с общей очередью и корректным завершением работы
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_THREAD_POOL__
#define __AWH_THREAD_POOL__

/**
 * Стандартные заголовочные файлы
 */
#include <queue>
#include <mutex>
#include <tuple>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>
#include <future>
#include <cstdint>
#include <utility>
#include <functional>
#include <condition_variable>

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
	 * @brief Класс пула потоков
	 *
	 */
	typedef class Threadpool {
		private:
			/**
			 * @brief Тип очереди задач
			 *
			 * @details Очередь задач хранит объекты типа std::function<void()>, представляющие задачи для выполнения в пуле потоков.
			 *
			 */
			typedef std::queue <function <void()>> task_t;
		private:
			// Очередь задач на исполнение
			task_t _tasks;
		private:
			// Количество потоков
			uint16_t _threads;
		private:
			// Флаг завершения работы пула потоков
			std::atomic_bool _stop;
			// Флаг ожидания завершения работы всех задач
			std::atomic_bool _wait;
		private:
			// Мьютекс для разграничения доступа к очереди задач
			mutable std::mutex _locker;
			// Условная переменная, контролирующая исполнение задачи
			std::condition_variable _cv;
		private:
			// Рабочие потоки для обработки задач
			vector <std::thread> _workers;
		private:
			/**
			 * @brief Метод проверки завершения заморозки потока
			 *
			 * @return результат проверки
			 *
			 */
			bool check() const noexcept {
				/**
				 * Поток должен проснуться при: принудительной остановке, появлении задачи
				 * или при запросе ожидания завершения работы (для дренажа и корректного выхода).
				 */
				return (
					this->_stop.load(std::memory_order_acquire) ||
					!this->_tasks.empty() ||
					this->_wait.load(std::memory_order_acquire)
				);
			}
		private:
			/**
			 * @brief Метод обработки очереди задач в одном потоке
			 *
			 */
			void work() noexcept {
				/**
				 * Запускаем бесконечный цикл обработки задач
				 */
				for(;;){
					/**
					 * Создаём текущее задание
					 */
					function <void ()> task = nullptr;
					// Ожидаем своей задачи в очереди потоков
					{
						// Выполняем блокировку уникальным мютексом
						unique_lock <std::mutex> lock(this->_locker);
						// Ожидаем сигнала: остановка, появление задачи или запрос ожидания завершения
						this->_cv.wait(lock, [this]() noexcept -> bool {
							// Возвращаем результат проверки условия пробуждения
							return this->check();
						});
						// Если запрошена принудительная остановка, выходим сразу, отбрасывая невыполненные задачи
						if(this->_stop.load(std::memory_order_acquire))
							// Выходим из функции
							return;
						// Если данные в очереди существуют
						if(!this->_tasks.empty()){
							// Получаем текущее задание
							task = std::move(this->_tasks.front());
							// Удаляем текущее задание
							this->_tasks.pop();
						/**
						 * Очередь пуста: сюда попадаем только при запросе ожидания (_wait).
						 * Задачи, добавленные из других задач, выполняются синхронно до возврата
						 * из task(), поэтому обслуживающий их поток гарантированно заберёт их при
						 * следующей итерации - потери вложенных задач не происходит.
						 */
						} else return;
					}
					// Задача появилась, исполняем её вне блокировки
					task();
				}
			}
		public:
			/**
			 * @brief Метод проверки на инициализацию тредпула
			 *
			 * @return результат проверки
			 *
			 */
			bool initialized() const noexcept {
				// Возвращаем результат проверки
				return !this->_workers.empty();
			}
		public:
			/**
			 * @brief Метод ожидания выполнения задач
			 *
			 */
			void wait() noexcept {
				{
					/**
					 * Меняем флаг под мьютексом условной переменной: иначе возможна потеря пробуждения,
					 * когда поток уже проверил предикат, но ещё не успел уснуть на условной переменной
					 */
					unique_lock <std::mutex> lock(this->_locker);
					// Устанавливаем флаг ожидания выполнения всех задач
					this->_wait.store(true, std::memory_order_release);
				}
				// Будим все потоки, чтобы они начали дренаж очереди и корректно завершились
				this->_cv.notify_all();
				/**
				 * Ожидаем завершение работы каждого воркера
				 */
				for(auto & worker: this->_workers)
					// Выполняем ожидание завершения работы потоков
					worker.join();
				// Сбрасываем флаг завершения работы пула потоков по умолчанию
				this->_stop.store(false, std::memory_order_release);
				// Сбрасываем флаг ожидания выполнения всех задач
				this->_wait.store(false, std::memory_order_release);
				// Очищаем список потоков
				this->_workers.clear();
				// Очищаем список задач
				std::queue <decltype(this->_tasks)::value_type> ().swap(this->_tasks);
			}
			/**
			 * @brief Метод завершения выполнения задач
			 *
			 */
			void stop() noexcept {
				{
					/**
					 * Меняем флаг под мьютексом условной переменной: иначе возможна потеря пробуждения,
					 * когда поток уже проверил предикат, но ещё не успел уснуть на условной переменной
					 */
					unique_lock <std::mutex> lock(this->_locker);
					// Останавливаем работу потоков
					this->_stop.store(true, std::memory_order_release);
				}
				// Сообщаем всем что мы завершаем работу
				this->_cv.notify_all();
				/**
				 * Ожидаем завершение работы каждого воркера
				 */
				for(auto & worker: this->_workers)
					// Выполняем ожидание завершения работы потоков
					worker.join();
				// Восстанавливаем работу потоков
				this->_stop.store(false, std::memory_order_release);
				// Сбрасываем флаг ожидания выполнения всех задач
				this->_wait.store(false, std::memory_order_release);
				// Очищаем список потоков
				this->_workers.clear();
				// Очищаем список задач (невыполненные задачи отбрасываются)
				std::queue <decltype(this->_tasks)::value_type> ().swap(this->_tasks);
			}
			/**
			 * @brief Метод очистки списка потоков
			 *
			 */
			void clean() noexcept {
				// Если в пуле остались рабочие потоки, выполняем их корректную остановку
				if(!this->_workers.empty())
					/**
					 * Полная остановка пула: сигнал завершения, ожидание потоков и очистка.
					 * Это исключает уничтожение ещё работающих потоков и вызов std::terminate
					 */
					this->stop();
			}
			/**
			 * @brief Метод инициализации работы тредпула
			 *
			 * @param count количество потоков
			 *
			 */
			void init(const uint16_t count = 0) noexcept {
				// Если пул уже инициализирован, повторная инициализация не требуется
				if(!this->_workers.empty())
					// Выходим, чтобы не плодить дубликаты рабочих потоков
					return;
				// Если количество потоков передано
				if(count > 0)
					// Устанавливаем количество потоков
					this->_threads = count;
				// Если количество потоков всё ещё не задано
				if(this->_threads == 0)
					// Используем безопасное значение по умолчанию
					this->_threads = 1;
				// Сбрасываем флаги остановки и ожидания перед запуском рабочих потоков
				this->_stop.store(false, std::memory_order_release);
				// Сбрасываем флаг ожидания выполнения всех задач
				this->_wait.store(false, std::memory_order_release);
				/**
				 * Добавляем в список воркеров новые рабочие потоки
				 */
				for(uint16_t i = 0; i < this->_threads; ++i)
					// Добавляем новый рабочий поток
					this->_workers.emplace_back(std::bind(&Threadpool::work, this));
			}
		public:
			/**
			 * @brief Метод возврата количества сообщений в очереди задач на исполнение
			 *
			 * @return результат работы функции
			 *
			 */
			size_t getTaskQueueSize() const noexcept {
				// Выполняем блокировку уникальным мютексом
				unique_lock <std::mutex> lock(this->_locker);
				// Возвращаем количество заданий
				return this->_tasks.size();
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param count количество потоков
			 *
			 */
			explicit Threadpool(const uint16_t count = 0) noexcept : _threads(0), _stop(false), _wait(false) {
				// Ели количество потоков передано
				if(count > 0)
					// Устанавливаем количество потоков
					this->_threads = count;
				// Если количество потоков не установлено
				else this->_threads = static_cast <uint16_t> (std::thread::hardware_concurrency());
				// Если определить число аппаратных потоков не удалось
				if(this->_threads == 0)
					// Используем минимум один поток
					this->_threads = 1;
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Threadpool() noexcept {
				// Выполняем ожидание завершения работы пула потоков
				this->stop();
			}
		public:
			/**
			 * @brief Шаблон метода добавления задач в пул
			 *
			 * @tparam Func тип данных функции обратного вызова
			 * @tparam Args аргумента функции обратного вызова
			 *
			 */
			template <class Func, class ... Args>
			/**
			 * @brief Метод добавления задач в пул
			 *
			 * @param func функция для обработки
			 * @param args аргументы для передачи в функцию
			 *
			 */
			auto push(Func && func, Args && ... args) noexcept -> std::future <typename std::invoke_result <Func, Args...>::type> {
				// Устанавливаем тип возвращаемого значения
				using result_t = typename std::invoke_result <Func, Args...>::type;
				// Формируем задачу с захватом функции и аргументов через perfect-forwarding (поддержка move-only типов)
				auto task = std::make_shared <std::packaged_task <result_t ()>> (
					[func = std::forward <Func> (func), args = std::make_tuple(std::forward <Args> (args)...)]() mutable -> result_t {
						// Выполняем вызов функции с распаковкой аргументов
						return std::apply(std::move(func), std::move(args));
					}
				);
				// Создаем шаблон асинхронных операций
				std::future <result_t> result = task->get_future();
				// Признак того, что задание было добавлено в очередь
				bool added = false;
				{
					// Выполняем блокировку уникальным мютексом
					unique_lock <std::mutex> lock(this->_locker);
					// Если это не остановка работы
					if(!this->_stop.load(std::memory_order_acquire)){
						// Выполняем добавление задания в список заданий
						this->_tasks.emplace([task](){ (* task)(); });
						// Запоминаем, что задание добавлено
						added = true;
					}
				}
				// Если задание добавлено в очередь
				if(added)
					// Сообщаем потокам, что появилась новая задача
					this->_cv.notify_one();
				/**
				 * Если пул остановлен, задание не добавляется, а связанный packaged_task будет уничтожен.
				 * В этом случае вызов result.get() выбросит std::future_error(broken_promise),
				 * что является штатным способом сигнализировать вызывающему об отклонении задачи.
				 */
				// Возвращаем результат
				return result;
			}
	} thr_t;
};

#endif // __AWH_THREAD_POOL__
