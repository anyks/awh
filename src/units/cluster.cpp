/**
 * @file: cluster.cpp
 * @date: 2026-02-21
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <thread>
#include <cerrno>
#include <cstring>
#include <csignal>

/**
 * Для операционных систем, отличных от MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные библиотеки
	 */
	#include <sys/wait.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <units/cluster.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Инкапсулируем статические типы данных в пространство имён
	 */
	namespace {
		/**
		 * @brief Объект перехвата сигнала
		 *
		 */
		struct sigaction __awh_action__{0};
		/**
		 * @brief Объект кластера для работы статических методов
		 *
		 */
		static awh::unit::cluster_t * __awh_cluster__ = nullptr;
	};
#endif

/**
 * @brief Метод создания дочерних процессов при запуске кластера
 *
 */
void awh::unit::Cluster::create() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционных систем, отличных от MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Индекс инициализированного процесса
			uint16_t index = 0;
			// Устанавливаем метку начала создания процессов
			NewProcess:
			// Если не все форки созданы
			if(index < this->_count){
				// Создаём новый вокрер дочернего процесса
				unique_ptr <worker_t> worker = make_unique <worker_t> ();
				// Устанавливаем время создания процесса
				worker->life = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				// Добавляем новые события для обмена сообщениями между процессами
				const auto & events = this->_io->events(event::family_t::UDS, this->_type);
				// Если события не созданы
				if((events[0] == 0) || (events[1] == 0)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Child process worker could not be created", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Child process worker could not be created", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из функции
					return;
				}
				/**
				 * Определяем тип потока
				 */
				switch((worker->pid = ::fork())){
					// Если поток не создан
					case -1: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Child process could not be created", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Child process could not be created", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					} break;
					// Если процесс является дочерним
					case 0: {
						// Если родительский процесс живой
						if(this->_pid == ::getppid()){
							// Выполняем переинициализацию асинхронного движка ввода-вывода
							this->_io->reinitialize();
							// Уничтожаем событие родительского процесса
							this->_io->destroy(events[0]);
							// Если список соответствия не пустой
							if(!this->_accord.empty()){
								// Переходим по всему списку соответствий
								for(auto & [eid, pid] : this->_accord)
									// Уничтожаем событие других дочерних процессов
									this->_io->destroy(eid);
							}
							// Уничтожаем события всех активных воркеров
							this->_workers.clear();
							// Устанавливаем опции события
							if(!this->_io->setOptions(events[1], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, std::make_tuple(index), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Error setting cluster worker event options", log_t::flag_t::WARNING);
								#endif
							}
							// Устанавливаем функцию обратного вызова на событие записи сообщений
							this->_io->on(events[1], static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на событие чтения сообщений
							this->_io->on(events[1], static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
							// Устанавливаем функцию обратного вызова на событие изменения состояния
							this->_io->on(events[1], static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на событие получения ошибок
							this->_io->on(events[1], static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
							// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
							this->_io->on(events[1], static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
							// Устанавливаем идентификатор события для обмена сообщениями между процессами
							worker->eid = events[1];
							// Устанавливаем идентификатор процесса воркера
							worker->pid = ::getpid();
							// Добавляем нового воркера в список активных воркеров
							auto ret = this->_workers.emplace(worker->pid, ::move(worker));
							// Выполняем фиксацию и запуск работы события
							if(this->_io->commit(ret.first->second->eid) && this->_io->launch(ret.first->second->eid)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об успешном запуске события
									this->_log->debug("Cluster worker process [%d] has been started successfully", __PRETTY_FUNCTION__, {}, log_t::flag_t::INFO, ret.first->first);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об успешном запуске события
									this->_log->print("Cluster worker process [%d] has been started successfully", log_t::flag_t::INFO, ret.first->first);
								#endif
								// Если функция обратного вызова установлена
								if(this->_callback.is("events"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const pid_t, const event_t)> ("events", ret.first->first, event_t::START);
							// Если событие не может быть запущено
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке запуска события
									this->_log->debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ret.first->first);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке запуска события
									this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, ret.first->first);
								#endif
								// Выходим из приложения
								::exit(EXIT_FAILURE);
							}
						// Если родительский процесс умер
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::getpid());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Процесс превратился в зомби, самоликвидируем его
								this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
							#endif
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						}
					} break;
					// Если процесс является родительским
					default: {
						// Уничтожаем событие дочернего процесса
						this->_io->destroy(events[1]);
						// Устанавливаем опции события
						if(!this->_io->setOptions(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Error setting cluster worker event options", log_t::flag_t::WARNING);
							#endif
						}
						// Устанавливаем функцию обратного вызова на событие записи сообщений
						this->_io->on(events[0], static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
						// Устанавливаем функцию обратного вызова на событие чтения сообщений
						this->_io->on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
						// Устанавливаем функцию обратного вызова на событие изменения состояния
						this->_io->on(events[0], static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
						// Устанавливаем функцию обратного вызова на событие получения ошибок
						this->_io->on(events[0], static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
						// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
						this->_io->on(events[0], static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
						// Устанавливаем идентификатор события для обмена сообщениями между процессами
						worker->eid = events[0];
						// Добавляем нового воркера в список активных воркеров
						auto ret = this->_workers.emplace(worker->pid, ::move(worker));
						// Добавляем соответствие идентификаторов событий и идентификатора процесса в список соответствия
						this->_accord.emplace(ret.first->second->eid, ret.first->first);
						// Увеличиваем индекс созданного процесса
						++index;
						// Переходим к метке создания следующего процесса
						goto NewProcess;
					}
				}
			// Если все процессы удачно созданы
			} else {
				// Выводим информацию о запущенном сервере на PIPE
				this->_log->print("Cluster [%s] has been started successfully", log_t::flag_t::INFO, this->_name.c_str());
				// Переходим по всему списку активных воркеров
				for(auto & [pid, worker] : this->_workers){
					// Выполняем фиксацию и запуск работы события
					if(!(this->_io->commit(worker->eid) && this->_io->launch(worker->eid))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке запуска события
							this->_log->debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, pid);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке запуска события
							this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, pid);
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				}
				// Если функция обратного вызова установлена
				if(this->_callback.is("events"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const event_t)> ("events", this->_pid, event_t::START);
			}
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
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
/**
 * @brief Метод размещения нового дочернего процесса
 *
 * @param pid идентификатор убитого процесса
 */
void awh::unit::Cluster::emplace([[maybe_unused]] const pid_t pid) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционных систем, отличных от MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Создаём новый вокрер дочернего процесса
			unique_ptr <worker_t> worker = make_unique <worker_t> ();
			// Устанавливаем время создания процесса
			worker->life = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
			// Добавляем новые события для обмена сообщениями между процессами
			const auto & events = this->_io->events(event::family_t::UDS, this->_type);
			// Если события не созданы
			if((events[0] == 0) || (events[1] == 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Child process worker could not be created", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Child process worker could not be created", log_t::flag_t::CRITICAL);
				#endif
				// Выходим из функции
				return;
			}
			/**
			 * Определяем тип потока
			 */
			switch((worker->pid = ::fork())){
				// Если поток не создан
				case -1: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Child process could not be created", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Child process could not be created", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				} break;
				// Если процесс является дочерним
				case 0: {
					// Если родительский процесс живой
					if(this->_pid == ::getppid()){
						// Выполняем переинициализацию асинхронного движка ввода-вывода
						this->_io->reinitialize();
						// Уничтожаем событие родительского процесса
						this->_io->destroy(events[0]);
						// Если список соответствия не пустой
						if(!this->_accord.empty()){
							// Переходим по всему списку соответствий
							for(auto & [eid, pid] : this->_accord)
								// Уничтожаем событие других дочерних процессов
								this->_io->destroy(eid);
						}
						// Уничтожаем события всех активных воркеров
						this->_workers.clear();
						// Устанавливаем опции события
						if(!this->_io->setOptions(events[1], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Error setting cluster worker event options", log_t::flag_t::WARNING);
							#endif
						}
						// Устанавливаем функцию обратного вызова на событие записи сообщений
						this->_io->on(events[1], static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
						// Устанавливаем функцию обратного вызова на событие чтения сообщений
						this->_io->on(events[1], static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
						// Устанавливаем функцию обратного вызова на событие изменения состояния
						this->_io->on(events[1], static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
						// Устанавливаем функцию обратного вызова на событие получения ошибок
						this->_io->on(events[1], static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
						// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
						this->_io->on(events[1], static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
						// Устанавливаем идентификатор события для обмена сообщениями между процессами
						worker->eid = events[1];
						// Устанавливаем идентификатор процесса воркера
						worker->pid = ::getpid();
						// Добавляем нового воркера в список активных воркеров
						auto ret = this->_workers.emplace(worker->pid, ::move(worker));
						// Выполняем фиксацию и запуск работы события
						if(this->_io->commit(ret.first->second->eid) && this->_io->launch(ret.first->second->eid)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об успешном запуске события
								this->_log->debug("Cluster worker process [%d] has been started successfully", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::INFO, ret.first->first);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об успешном запуске события
								this->_log->print("Cluster worker process [%d] has been started successfully", log_t::flag_t::INFO, ret.first->first);
							#endif
							// Если функция обратного вызова установлена
							if(this->_callback.is("events"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const pid_t, const event_t)> ("events", ret.first->first, event_t::START);
						// Если событие не может быть запущено
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке запуска события
								this->_log->debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::CRITICAL, ret.first->first);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке запуска события
								this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, ret.first->first);
							#endif
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						}
					// Если родительский процесс умер
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::CRITICAL, ::getpid());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Процесс превратился в зомби, самоликвидируем его
							this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				} break;
				// Если процесс является родительским
				default: {
					// Уничтожаем событие дочернего процесса
					this->_io->destroy(events[1]);
					// Устанавливаем опции события
					if(!this->_io->setOptions(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Error setting cluster worker event options", log_t::flag_t::WARNING);
						#endif
					}
					// Устанавливаем функцию обратного вызова на событие записи сообщений
					this->_io->on(events[0], static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
					// Устанавливаем функцию обратного вызова на событие чтения сообщений
					this->_io->on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие изменения состояния
					this->_io->on(events[0], static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(events[0], static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
					this->_io->on(events[0], static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
					// Устанавливаем идентификатор события для обмена сообщениями между процессами
					worker->eid = events[0];
					// Добавляем нового воркера в список активных воркеров
					auto ret = this->_workers.emplace(worker->pid, ::move(worker));
					// Добавляем соответствие идентификаторов событий и идентификатора процесса в список соответствия
					this->_accord.emplace(ret.first->second->eid, ret.first->first);
					// Выполняем фиксацию и запуск работы события
					if(!(this->_io->commit(ret.first->second->eid) && this->_io->launch(ret.first->second->eid))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке запуска события
							this->_log->debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, std::make_tuple(pid, ret.first->second->pid), log_t::flag_t::CRITICAL, pid);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке запуска события
							this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, pid);
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Если функция обратного вызова установлена
					if(this->_callback.is("rebase") && (pid > 0))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const pid_t)> ("rebase", ret.first->first, pid);
				}
			}
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод запуска/остановки работы кластера
 *
 * @param status статус запуска/остановки кластера
 */
void awh::unit::Cluster::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы сервера
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа кластера запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED): {
			// Если функция обратного вызова установлена
			if(this->_callback.is("clusterStatus"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("clusterStatus", status);
			// Если количество создаваемых процессов установлено
			if(this->_count > 0)
				// Выполняем создание дочерних процессов
				this->create();
			// Если количество создаваемых процессов не установлено
			else {
				// Выводим информацию о запущенном сервере на PIPE
				this->_log->print("Cluster [%s] has been started successfully", log_t::flag_t::INFO, this->_name.c_str());
				// Если функция обратного вызова установлена
				if(this->_callback.is("events"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const event_t)> ("events", this->_pid, event_t::START);
			}
		} break;
		// Если работа кластера подлежит уничтожение
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Если список активных воркеров не пустой
			if(!this->_workers.empty()){
				// Переходим по всему списку активных воркеров
				for(auto & [pid, worker] : this->_workers){
					// Запрещаем анализ остановленного процесса
					worker->pid = 0;
					// Устанавливаем функцию обратного вызова на событие изменения статуса
					this->_io->on(worker->eid, static_cast <engine::callback::status_t> (nullptr));
				}
				// Уничтожаем события всех активных воркеров
				this->_workers.clear();
			}
			// Если функция обратного вызова установлена
			if(this->_callback.is("clusterStatus")){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("clusterStatus", status);
				// Выполняем получение функции обратного вызова
				this->_callback.set("clusterStatus", "status", this->_callback);
			}
		} break;
	}
}
/**
 * @brief Метод перезапуска упавшего процесса
 *
 * @param pid    идентификатор упавшего процесса
 * @param status статус остановившегося процесса
 */
void awh::unit::Cluster::process([[maybe_unused]] const pid_t pid, [[maybe_unused]] const int32_t status) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Выполняем поиск завершившегося процесса
		auto i = this->_workers.find(pid);
		// Если указанный воркер найден
		if(i != this->_workers.end()){
			// Уничтожаем событие завершившегося процесса
			this->_io->destroy(i->second->eid);
			// Если завершившийся процесс требуется анализировать дальше
			if(i->second->pid == pid){
				// Выводим сообщение об ошибке, о невозможности отправить сообщение
				this->_log->print("Child process stopped, PID=%d, STATUS=%d", log_t::flag_t::WARNING, pid, status);
				// Если статус сигнала — ручная остановка процесса
				if(status == SIGINT){
					// Если список активных воркеров не пустой
					if(!this->_workers.empty()){
						// Переходим по всему списку активных воркеров
						for(auto & [pid, worker] : this->_workers){
							// Запрещаем анализ остановленного процесса
							worker->pid = 0;
							// Устанавливаем функцию обратного вызова на событие изменения статуса
							this->_io->on(worker->eid, static_cast <engine::callback::status_t> (nullptr));
						}
						// Уничтожаем события всех активных воркеров
						this->_workers.clear();
					}
					// Выходим из приложения
					::exit(SIGINT);
				// Если время жизни процесса составляет меньше 30 секунд
				} else if((this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) - i->second->life) <= 30000ULL) {
					// Если список активных воркеров не пустой
					if(!this->_workers.empty()){
						// Переходим по всему списку активных воркеров
						for(auto & [pid, worker] : this->_workers){
							// Запрещаем анализ остановленного процесса
							worker->pid = 0;
							// Устанавливаем функцию обратного вызова на событие изменения статуса
							this->_io->on(worker->eid, static_cast <engine::callback::status_t> (nullptr));
						}
						// Уничтожаем события всех активных воркеров
						this->_workers.clear();
					}
					// Выходим из приложения
					::exit(status);
				}
				// Удаляем завершившийся процесс из списка активных воркеров
				this->_workers.erase(i);
				// Если функция обратного вызова установлена
				if(this->_callback.is("exit"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const int32_t)> ("exit", pid, status);
				// Если функция обратного вызова установлена
				if(this->_callback.is("events"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const event_t)> ("events", pid, event_t::STOP);
				// Если разрешен автоматический перезапуск процесса
				if(this->_rebirth)
					// Выполняем создание нового процесса
					this->emplace(pid);
			// Удаляем завершившийся процесс из списка активных воркеров
			} else this->_workers.erase(i);
		}
	#endif
}
/**
 * @brief Функция фильтр перехватчика сигналов
 *
 * @param signal номер сигнала полученного системой
 * @param info   объект информации полученный системой
 * @param ctx    передаваемый внутренний контекст
 */
void awh::unit::Cluster::child([[maybe_unused]] int32_t signal, [[maybe_unused]] siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Идентификатор упавшего процесса
		pid_t pid = 0;
		// Статус упавшего процесса
		int32_t status = 0;
		/**
		 * Выполняем получение идентификатора упавшего процесса
		 */
		while((pid = ::waitpid(-1, &status, WNOHANG)) > 0)
			// Выполняем создание дочернего потока
			const_cast <awh::unit::cluster_t *> (::__awh_cluster__)->process(pid, status);
	#endif
}
/**
 * @brief Метод обработки событий записи сообщений кластера
 *
 * @param eid  идентификатор события
 * @param size размер сообщения
 */
void awh::unit::Cluster::write(const event::id_t eid, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("sending")){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_accord.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_accord.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const size_t)> ("sending", i->second, size);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const size_t)> ("sending", this->_pid, size);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(eid, size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		}
	}
}
/**
 * @brief Метод обработки событий чтения сообщений кластера
 *
 * @param eid  идентификатор события
 * @param data данные сообщения
 * @param size размер сообщения
 */
void awh::unit::Cluster::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("message")){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_accord.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_accord.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("message", i->second, data, size);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("message", this->_pid, data, size);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(eid, data, size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		}
	}
}
/**
 * @brief Метод обработки состояния кластера
 *
 * @param eid    идентификатор события
 * @param status статус события
 */
void awh::unit::Cluster::state(const event::id_t eid, const event::status_t status) noexcept {
	/**
	 * Обрабатываем статус события
	 */
	switch(static_cast <uint8_t> (status)){
		// Если статус уничтожения
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Если процесс является дочерним
			if(!this->master()){
				// Если родительский процесс живой
				if(this->_pid == ::getppid()){
					// Выполняем поиск процесса по идентификатору
					auto i = this->_workers.find(::getpid());
					// Если указанный процесс найден
					if(i != this->_workers.end()){
						// Если уничтоженное событие соответствует событию процесса
						if(i->second->eid == eid){
							// Если функция обратного вызова установлена
							if(this->_callback.is("exit"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const pid_t, const int32_t)> ("exit", i->first, SIGSTOP);
							// Если функция обратного вызова установлена
							if(this->_callback.is("events"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const pid_t, const event_t)> ("events", i->first, event_t::STOP);
							// Удаляем завершившийся процесс из списка активных воркеров
							this->_workers.erase(i);
							// Завершаем работу процесса
							::exit(EXIT_SUCCESS);
						}
					}
				// Если родительский процесс умер
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, ::getpid());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Процесс превратился в зомби, самоликвидируем его
						this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
					#endif
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
			}
		} break;
		// Если мы получили любой другой статус
		default: {
			// Если функция обратного вызова установлена
			if(this->_callback.is("state")){
				// Если процесс является родительским
				if(this->master()){
					// Выполняем поиск идентификатора процесса по идентификатору события
					auto i = this->_accord.find(eid);
					// Если идентификатор процесса найден
					if(i != this->_accord.end())
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const event::status_t)> ("state", i->second, status);
				// Если процесс является дочерним
				} else {
					// Если родительский процесс живой
					if(this->_pid == ::getppid())
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const event::status_t)> ("state", this->_pid, status);
					// Если родительский процесс умер
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, ::getpid());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Процесс превратился в зомби, самоликвидируем его
							this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				}
			}
		}
	}
}
/**
 * @brief Метод обработки исключений событий кластера
 *
 * @param eid     идентификатор события
 * @param error   тип ошибки
 * @param message сообщение об ошибке
 */
void awh::unit::Cluster::error(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("error")){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_accord.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_accord.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("error", i->second, error, message);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("error", this->_pid, error, message);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (error), message), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		}
	}
}
/**
 * @brief Метод обработки событий доступного размера очереди события кластера
 *
 * @param eid    идентификатор события
 * @param status статус события
 * @param size   доступный размер очереди в байтах
 */
void awh::unit::Cluster::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("available")){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_accord.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_accord.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("available", i->second, status, size);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("available", this->_pid, status, size);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (status), size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		}
	}
}
/**
 * @brief Метод остановки кластера
 *
 */
void awh::unit::Cluster::stop() noexcept {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если процесс является родительским
		if(this->master()){
			// Если работа юнита запущена
			if(this->working())
				// Останавливаем работу основного юнита
				unit_t::stop();
		// Если процесс является дочерним то выводим сообщение об ошибке
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Only the master process can stop the cluster", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Only the master process can stop the cluster", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке запуска события
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
}
/**
 * @brief Метод запуска кластера
 *
 */
void awh::unit::Cluster::start() noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если процесс является родительским
		if(this->master()){
			// Если работа юнита ещё не запущена
			if(!this->working()){
				// Если функция обратного вызова установлена
				if(this->_callback.is("status"))
					// Выполняем получение функции обратного вызова
					this->_callback.set("status", "clusterStatus", this->_callback);
				// Устанавливаем функцию обратного вызова на запуск системы
				this->_callback.on <void (const event::status_t)> ("status", &cluster_t::launch, this, _1);
				// Выполняем запуск работы основного юнита
				unit_t::start();
			}
		// Если процесс является дочерним то выводим сообщение об ошибке
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Only the master process can start the cluster", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Only the master process can start the cluster", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке запуска события
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
}
/**
 * @brief Метод очистки всех выделенных ресурсов
 *
 * @param shutdown тип завершения работы кластера
 */
void awh::unit::Cluster::clear(const shutdown_t shutdown) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Если список активных воркеров не пустой
		if(!this->_workers.empty()){
			// Переходим по всему списку активных воркеров
			for(auto & [pid, worker] : this->_workers){
				// Запрещаем анализ остановленного процесса
				worker->pid = 0;
				// Устанавливаем функцию обратного вызова на событие изменения статуса
				this->_io->on(worker->eid, static_cast <engine::callback::status_t> (nullptr));
				// Уничтожаем событие процесса
				this->_io->destroy(worker->eid);
				// Если требуется принудительное завершение работы процесса
				if(shutdown == shutdown_t::FORCEFUL)
					// Убиваем дочерний процесс
					::kill(pid, SIGKILL);
			}
			// Очищаем список активных воркеров
			this->_workers.clear();
		}
	// Если процесс является дочерним то выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Only the master process can clear the cluster", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Only the master process can clear the cluster", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод размещения нового дочернего процесса
 *
 */
void awh::unit::Cluster::emplace() noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если процесс является родительским
		if(this->master()){
			// Если работа юнита запущена
			if(this->working())
				// Выполняем создание нового дочернего процесса
				this->emplace(0);
		// Если процесс является дочерним то выводим сообщение об ошибке
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Only the master process can create child processes", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Only the master process can create child processes", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке запуска события
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
}
/**
 * @brief Метод удаления активного процесса
 *
 * @param pid       идентификатор процесса
 * @param shutdown тип завершения работы кластера
 */
void awh::unit::Cluster::erase(const pid_t pid, const shutdown_t shutdown) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Если список активных воркеров не пустой
		if(!this->_workers.empty()){
			// Выполняем поиск процесса по идентификатору
			auto i = this->_workers.find(pid);
			// Если указанный процесс найден
			if(i != this->_workers.end()){
				// Запрещаем анализ остановленного процесса
				i->second->pid = 0;
				// Устанавливаем функцию обратного вызова на событие изменения статуса
				this->_io->on(i->second->eid, static_cast <engine::callback::status_t> (nullptr));
				// Уничтожаем событие процесса
				this->_io->destroy(i->second->eid);
				// Если требуется принудительное завершение работы процесса
				if(shutdown == shutdown_t::FORCEFUL)
					// Убиваем дочерний процесс
					::kill(i->first, SIGKILL);
				// Удаляем процесс из списка активных воркеров
				this->_workers.erase(i);
			}
		}
	// Если процесс является дочерним то выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Only the master process can remove child processes", __PRETTY_FUNCTION__, std::make_tuple(pid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Only the master process can remove child processes", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки флага автоматического возрождения процессов
 *
 * @param mode флаг возрождения процессов
 */
void awh::unit::Cluster::rebirth(const bool mode) noexcept {
	// Устанавливаем флаг автоматического возрождения процессов
	this->_rebirth = mode;
}
/**
 * @brief Метод установки названия кластера
 *
 * @param name название кластера для установки
 */
void awh::unit::Cluster::name(string_view name) noexcept {
	// Устанавливаем название кластера
	this->_name = name;
}
/**
 * @brief Метод получения максимального количества процессов
 *
 * @return максимальное количество процессов
 */
uint16_t awh::unit::Cluster::count() const noexcept {
	// Получаем максимальное количество процессов
	return this->_count;
}
/**
 * @brief Метод установки максимального количества процессов
 *
 * @param count максимальное количество процессов
 */
void awh::unit::Cluster::count(const uint16_t count) noexcept {
	// Устанавливаем максимальное количество процессов
	this->_count = count;
}
/**
 * @brief Метод получения списка дочерних процессов
 *
 * @return список дочерних процессов
 */
unordered_set <pid_t> awh::unit::Cluster::workers() const noexcept {
	// Результат работы функции
	unordered_set <pid_t> result;
	// Если список активных воркеров не пустой
	if(!this->_workers.empty()){
		// Переходим по всему списку активных воркеров
		for(const auto & [pid, worker] : this->_workers)
			// Добавляем идентификатор процесса в результат
			result.emplace(pid);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::Cluster::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса
	this->_callback.set("exit", callback);
	// Выполняем установку функции обратного вызова при получении состояния процесса
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса
	this->_callback.set("rebase", callback);
	// Выполняем установку функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
	this->_callback.set("events", callback);
	// Выполняем установку функции обратного вызова при отправке сообщения
	this->_callback.set("sending", callback);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callback.set("message", callback);
	// Выполняем установку функции обратного вызова при получении доступности размера очереди сообщений
	this->_callback.set("available", callback);
}
/**
 * @brief Метод отправки сообщения родительскому процессу
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 */
size_t awh::unit::Cluster::send(const void * buffer, const size_t size) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если процесс является дочерним
		if(!this->master()){
			// Если родительский процесс живой
			if(this->_pid == ::getppid()){
				// Выполняем поиск текущего процесса по идентификатору
				auto i = this->_workers.find(::getpid());
				// Если указанный процесс найден
				if(i != this->_workers.end())
					// Отправляем сообщение родительскому процессу
					return this->_io->send(i->second->eid, buffer, size);
			// Если родительский процесс умер
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		// Если процесс является родительским
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("A message addressed to a parent process can only be sent from child processes", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Процесс превратился в зомби, самоликвидируем его
				this->_log->print("A message addressed to a parent process can only be sent from child processes", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке запуска события
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения дочернему процессу
 *
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 */
size_t awh::unit::Cluster::send(const pid_t pid, const void * buffer, const size_t size) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск указанного процесса по идентификатору
			auto i = this->_workers.find(pid);
			// Если указанный процесс найден
			if(i != this->_workers.end())
				// Отправляем сообщение дочернему процессу
				return this->_io->send(i->second->eid, buffer, size);
		// Если процесс является дочерним
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, std::make_tuple(pid, buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Процесс превратился в зомби, самоликвидируем его
				this->_log->print("A message addressed to a child process can only be sent from the parent process", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, std::make_tuple(pid, buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения всем дочерним процессам
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 */
size_t awh::unit::Cluster::broadcast(const void * buffer, const size_t size) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если процесс является родительским
		if(this->master()){
			// Если список активных воркеров не пустой
			if(!this->_workers.empty()){
				// Результат работы функции
				size_t result = 0;
				// Переходим по всему списку активных воркеров
				for(const auto & [pid, worker] : this->_workers)
					// Отправляем сообщение дочернему процессу
					result += this->_io->send(worker->eid, buffer, size);
				// Выводим результат
				return result;
			}
		// Если процесс является дочерним
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Процесс превратился в зомби, самоликвидируем его
				this->_log->print("A message addressed to a child process can only be sent from the parent process", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод получения размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @return       размер буфера события
 */
size_t awh::unit::Cluster::getBufferSize(const pid_t pid, const event::action_t action) const noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Выполняем поиск процесса по идентификатору
		auto i = this->_workers.find(pid);
		// Если указанный процесс найден
		if(i != this->_workers.end())
			// Извлекаем размер буфера события
			return this->_io->getBufferSize(i->second->eid, action);
	// Если процесс является дочерним
	} else {
		// Выполняем поиск процесса по идентификатору
		auto i = this->_workers.find(::getpid());
		// Если указанный процесс найден
		if(i != this->_workers.end())
			// Извлекаем размер буфера события
			return this->_io->getBufferSize(i->second->eid, action);
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 */
bool awh::unit::Cluster::setBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Выполняем поиск процесса по идентификатору
		auto i = this->_workers.find(pid);
		// Если указанный процесс найден
		if(i != this->_workers.end())
			// Устанавливаем размер буфера события
			return this->_io->setBufferSize(i->second->eid, action, size);
	// Если процесс является дочерним
	} else {
		// Выполняем поиск процесса по идентификатору
		auto i = this->_workers.find(::getpid());
		// Если указанный процесс найден
		if(i != this->_workers.end())
			// Устанавливаем размер буфера события
			return this->_io->setBufferSize(i->second->eid, action, size);
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::Cluster::Cluster(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _name{AWH_SHORT_NAME},
 _rebirth(false), _count(0), _type(event::type_t::SEQPACKET) {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если кластер уже был создан ранее
		if(::__awh_cluster__ != nullptr)
			// Выполняем генерацию исключения
			throw std::logic_error("A cluster cannot exist twice");
		// Если кластер ещё не создан
		else {
			// Выполняем установку объекта кластера
			::__awh_cluster__ = this;
			// Устанавливаем функцию перехватчика событий
			::__awh_action__.sa_sigaction = &cluster_t::child;
			// Устанавливаем флаги перехвата сигналов
			::__awh_action__.sa_flags = (SA_RESTART | SA_SIGINFO);
			// Устанавливаем маску перехвата
			sigemptyset(&::__awh_action__.sa_mask);
			// Активируем перехватчик событий
			::sigaction(SIGCHLD, &::__awh_action__, nullptr);
			// Устанавливаем количество доступных ядер в системе
			this->_count = static_cast <uint16_t> (thread::hardware_concurrency());
			// Если количество доступных воркеров больше 1-х, уменьшаем пополам
			if(this->_count > 1)
				// Уменьшаем количество воркеров в два раза
				this->_count /= 2;
		}
	#endif
}
/**
 * @brief Деструктор
 *
 */
awh::unit::Cluster::~Cluster() noexcept {
	// Если процесс является родительским
	if(this->master())
		// Выполняем очистку всех выделенных ресурсов
		this->clear(shutdown_t::FORCEFUL);
}
