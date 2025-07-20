/**
 * @file: threadpool.hpp
 * @date: 2023-12-22
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

#ifndef __AWH_THREAD_POOL__
#define __AWH_THREAD_POOL__

/**
 * Стандартные модули
 */
#include <queue>
#include <mutex>
#include <vector>
#include <memory>
#include <thread>
#include <future>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <condition_variable>

/**
 * Разрешаем сборку под Windows
 */
#include <sys/global.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Класс пула потоков
	 */
	typedef class ThreadPool {
		private:
			// Тип очереди задач
			typedef std::queue <function <void()>> task_t;
		private:
			// Флаг завершения работы пула потоков
			bool _stop;
			// Флаг ожидания завершения работы всех задачь
			bool _wait;
		private:
			// Количество потоков
			uint16_t _threads;
		private:
			// Очередь задач на исполнение
			task_t _tasks;
		private:
			// Мьютекс для разграничения доступа к очереди задач
			mutable std::mutex _locker;
			// Условная переменная, контролирующая исполнение задачи
			std::condition_variable _cv;
		private:
			// Рабочие потоки для обработки задач
			vector <thread> _workers;
		private:
			/**
			 * check Метод проверки завершения заморозки потока
			 * @return результат проверки
			 */
			bool check() const noexcept {
				// Если данные получены или произошла остановка
				return (this->_stop || !this->_tasks.empty());
			}
		private:
			/**
			 * work Метод обработки очереди задач в одном потоке
			 */
			void work() noexcept {
				// Запускаем бесконечный цикл
				while(!this->_stop){
					// Создаём текущее задание
					function <void (void)> task;
					// Ожидаем своей задачи в очереди потоков
					{
						// Выполняем блокировку уникальным мютексом
						unique_lock <std::mutex> lock(this->_locker);
						// Если это не остановка приложения и список задач пустой, ожидаем добавления нового задания
						this->_cv.wait_for(lock, 100ms, std::bind(&ThreadPool::check, this));
						// Если это остановка приложения и список задач пустой, выходим
						if(this->_stop && this->_tasks.empty())
							// Выходим из функции
							return;
						// Если данные в очереди существуют
						if(!this->_tasks.empty()){
							// Получаем текущее задание
							task = this->_tasks.front();
							// Удаляем текущее задание
							this->_tasks.pop();
						// Иначе выполняем пропуск
						} else continue;
					}
					// Задача появилась, исполняем её и сообщаем о том, что задача выбрана из очереди
					task();
					// Если мы ожидаем завершения работы всех задач
					if(this->_wait)
						// Выполняем остановку работы цикла задач
						this->_stop = this->_tasks.empty();
				}
			}
		public:
			/**
			 * is Метод проверки на инициализацию тредпула
			 * @return результат проверки
			 */
			bool is() const noexcept {
				// Выводим результат проверки
				return !this->_workers.empty();
			}
		public:
			/**
			 * wait Метод ожидания выполнения задач
			 */
			void wait() noexcept {
				// Устанавливаем флаг ожидания выполнения всех зада
				this->_wait = true;
				// Ожидаем завершение работы каждого воркера
				for(auto & worker: this->_workers)
					// Выполняем ожидание завершения работы потоков
					worker.join();
				// Создаем пустой список задач
				task_t empty;
				// Сбрасываем флаг завершения работы пула потоков по умолчанию
				this->_stop = false;
				// Сбрасываем флаг ожидания выполнения всех зада
				this->_wait = false;
				// Очищаем список потоков
				this->_workers.clear();
				// Очищаем список задач
				swap(this->_tasks, empty);
			}
			/**
			 * stop Метод завершения выполнения задач
			 */
			void stop() noexcept {
				{
					// Останавливаем работу потоков
					this->_stop = true;
					// Создаем уникальный мютекс
					unique_lock <std::mutex> lock(this->_locker);
				}
				// Сообщаем всем что мы завершаем работу
				this->_cv.notify_all();
				// Ожидаем завершение работы каждого воркера
				for(auto & worker: this->_workers)
					// Выполняем ожидание завершения работы потоков
					worker.join();
				// Создаем пустой список задач
				task_t empty;
				// Восстанавливаем работу потоков
				this->_stop = false;
				// Очищаем список потоков
				this->_workers.clear();
				// Очищаем список задач
				swap(this->_tasks, empty);
			}
			/**
			 * clean Метод очистки списка потоков
			 */
			void clean() noexcept {
				// Очищаем список потоков
				this->_workers.clear();
			}
			/**
			 * init Метод инициализации работы тредпула
			 * @param count количество потоков
			 */
			void init(const uint16_t count = 0) noexcept {
				// Если количество потоков передано
				if(count > 0)
					// Устанавливаем количество потоков
					this->_threads = count;
				// Ели количество потоков передано
				if(this->_threads > 0){
					// Добавляем в список воркеров, новую задачу
					for(uint16_t i = 0; i < this->_threads; ++i)
						// Добавляем новую задачу
						this->_workers.emplace_back(std::bind(&ThreadPool::work, this));
				}
			}
		public:
			/**
			 * getTaskQueueSize Метод возврата количества сообщений в очереди задач на исполнение
			 * @return результат работы функции
			 */
			const size_t getTaskQueueSize() const noexcept {
				// Выполняем блокировку уникальным мютексом
				unique_lock <std::mutex> lock(this->_locker);
				// Выводим количество заданий
				return this->_tasks.size();
			}
		public:
			/**
			 * ThreadPool Конструктор
			 * @param count количество потоков
			 */
			explicit ThreadPool(const uint16_t count = 0) noexcept : _stop(false), _wait(false), _threads(0) {
				// Ели количество потоков передано
				if(count > 0)
					// Устанавливаем количество потоков
					this->_threads = count;
				// Если количество потоков не установлено
				else this->_threads = static_cast <uint16_t> (std::thread::hardware_concurrency());
			}
			/**
			 * ~ThreadPool Деструктор
			 */
			~ThreadPool() noexcept {
				// Выполняем ожидание завершения работы пула потоков
				this->stop();
			}
		public:
			/**
			 * Шаблон метода добавления задач в пул
			 */
			template <class Func, class ... Args>
			/**
			 * push Метод добавления задач в пул
			 * @param func функция для обработки
			 * @param args аргументы для передачи в функцию
			 */
			auto push(Func && func, Args && ... args) noexcept -> future <typename invoke_result <Func, Args...>::type> {
				// Устанавливаем тип возвращаемого значения
				using result_t = typename invoke_result <Func, Args...>::type;
				// Добавляем задачу в очередь для последующего исполнения
				auto task = make_shared <packaged_task <result_t()>> (std::bind(std::forward <Func> (func), std::forward <Args> (args)...));
				// Создаем шаблон асинхронных операций
				future <result_t> res = task->get_future();
				{
					// Выполняем блокировку уникальным мютексом
					unique_lock <std::mutex> lock(this->_locker);
					// Если это не остановка работы
					if(!this->_stop)
						// Выполняем добавление задания в список заданий
						this->_tasks.emplace([task](){(* task)();});
				}
				// Сообщаем потокам, что появилась новая задача
				this->_cv.notify_one();
				// Выводим результат
				return res;
			}
	} thr_t;
};

#endif // __AWH_THREAD_POOL__
