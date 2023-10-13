/**
 * @file: threadpool.hpp
 * @date: 2022-02-06
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_THREAD_POOL__
#define __AWH_THREAD_POOL__

/**
 * Стандартная библиотека
 */
#include <queue>
#include <mutex>
#include <vector>
#include <memory>
#include <thread>
#include <future>
#include <stdexcept>
#include <functional>
#include <condition_variable>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Класс пула потоков
	 */
	typedef class ThreadPool {
		private:
			// Сингнал остановки работы пула потоков
			bool _stop;
		private:
			// Количество потоков
			uint16_t _threads;
		private:
			// Тип очереди задач
			typedef queue <function <void()>> task_t;
		private:
			// Очередь задач на исполнение
			task_t _tasks;
		private:
			// Условная переменная, контролирующая исполнение задачи
			condition_variable _cv;
			// Мьютекс для разграничения доступа к очереди задач
			mutable mutex _queueMutex;
		private:
			// Рабочие потоки для обработки задач
			vector <std::thread> _workers;
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
				for(;;){
					// Создаём текущее задание
					function <void (void)> task;
					// Ожидаем своей задачи в очереди потоков
					{
						// Выполняем блокировку уникальным мютексом
						unique_lock <mutex> lock(this->_queueMutex);
						// Если это не остановка приложения и список задач пустой, ожидаем добавления нового задания
						this->_cv.wait_for(lock, 100ms, std::bind(&ThreadPool::check, this));
						// Если это остановка приложения и список задач пустой, выходим
						if(this->_stop && this->_tasks.empty())
							// Выходим из функции
							return;
						// Если данные в очереди существуют
						if(!this->_tasks.empty()){
							// Получаем текущее задание
							task = std::move(this->_tasks.front());
							// Удаляем текущее задание
							this->_tasks.pop();
						// Иначе выполняем пропуск
						} else continue;
					}
					// Задача появилась, исполняем ее и сообщаем о том, что задача выбрана из очереди
					task();
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
			 * @param stop флаг остановки пула потоков
			 */
			void wait(const bool stop = true) noexcept {
				{
					// Останавливаем работу потоков
					this->_stop = true;
					// Создаем уникальный мютекс
					unique_lock <mutex> lock(this->_queueMutex);
				}
				// Создаем пустой список задач
				task_t empty;
				// Сообщаем всем что мы завершаем работу
				this->_cv.notify_all();
				// Ожидаем завершение работы каждого воркера
				for(auto & worker: this->_workers)
					// Выполняем ожидание завершения работы потоков
					worker.join();
				// Если нужно завершить работу всех потоков
				if(stop){
					// Очищаем список потоков
					this->_workers.clear();
					// Очищаем список задач
					std::swap(this->_tasks, empty);
					// Восстанавливаем работу потоков
					this->_stop = false;
				}
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
				unique_lock <mutex> lock(this->_queueMutex);
				// Выводим количество заданий
				return this->_tasks.size();
			}
		public:
			/**
			 * ThreadPool Конструктор
			 * @param count количество потоков
			 */
			explicit ThreadPool(const uint16_t count = static_cast <uint16_t> (std::thread::hardware_concurrency())) noexcept : _stop(false), _threads(0) {
				// Ели количество потоков передано
				if(count > 0)
					// Устанавливаем количество потоков
					this->_threads = count;
				// Иначе устанавливаем один поток
				else this->_threads = 1;
			}
			/**
			 * ~ThreadPool Деструктор
			 */
			~ThreadPool() noexcept {
				// Выполняем ожидание выполнения задач
				this->wait();
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
			auto push(Func && func, Args && ... args) noexcept -> future <typename result_of <Func (Args...)>::type> {
				// Устанавливаем тип возвращаемого значения
				using return_type = typename result_of <Func(Args...)>::type;
				// Добавляем задачу в очередь для последующего исполнения
				auto task = make_shared <packaged_task <return_type()>> (std::bind(std::forward <Func> (func), std::forward <Args> (args)...));
				// Создаем шаблон асинхронных операций
				future <return_type> res = task->get_future();
				{
					// Выполняем блокировку уникальным мютексом
					unique_lock <mutex> lock(this->_queueMutex);
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
