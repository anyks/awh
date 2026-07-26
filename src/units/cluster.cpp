/**
 * @file: cluster.cpp
 * @date: 2026-02-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля кластера — запуск и контроль дочерних воркеров, обмен сообщениями между процессами,
 *        перезапуск упавших воркеров и защита от цикла быстрых перезапусков
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <thread>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <csignal>

/**
 * Для операционных систем, отличных от MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Системные заголовочные файлы
	 */
	#include <dlfcn.h>
	#include <sys/wait.h>
	#include <execinfo.h>
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/cluster.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * @brief Инкапсулируем статические типы данных в пространство имён
	 *
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
 * Для операционных систем, отличных от MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		/**
		 * @brief Инкапсулируем статические параметры локального кэша в пространство имён
		 *
		 */
		namespace {
			/**
			 * @brief Функция выполнения внешней команды и получения её стандартного вывода
			 *
			 * @param cmd команда для выполнения
			 * @return    стандартный вывод выполненной команды
			 *
			 */
			string __awh_exec_command__(const string & cmd) noexcept {
				// Результат выполнения команды
				string result = "";
				// Открываем канал на чтение стандартного вывода команды
				FILE * pipe = ::popen(cmd.c_str(), "r");
				// Если канал открыт
				if(pipe != nullptr){
					// Буфер для чтения вывода команды
					char buffer[1024];
					/**
					 * Читаем вывод команды до конца
					 */
					while(::fgets(buffer, sizeof(buffer), pipe) != nullptr)
						// Добавляем прочитанные данные в результат
						result.append(buffer);
					// Закрываем канал
					::pclose(pipe);
				}
				// Возвращаем результат
				return result;
			}
			/**
			 * @brief Функция определения позиции в исходном коде по адресу инструкции
			 *
			 * @param pc адрес инструкции из бэктрейса
			 * @return   строка вида «функция файл:строка», либо пустая строка, если позицию определить не удалось
			 *
			 */
			string __awh_resolve_line__(void * pc) noexcept {
				// Результат определения позиции в исходном коде
				string result = "";
				// Объект информации о символе
				Dl_info info;
				// Зануляем объект информации о символе
				::memset(&info, 0, sizeof(info));
				// Если по адресу удалось определить модуль (исполняемый файл или разделяемую библиотеку)
				if((::dladdr(pc, &info) != 0) && (info.dli_fname != nullptr)){
					// Получаем абсолютный адрес инструкции
					const uint64_t addr = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (pc));
					// Получаем базовый адрес загрузки модуля
					const uint64_t base = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (info.dli_fbase));
					// Буфер для формирования команды символизации
					char command[2048];
					/**
					 * Для операционной системы macOS
					 */
					#if __APPLE__ || __MACH__
						// Формируем команду символизации через atos (смещение загрузки модуля задаётся параметром -l)
						::snprintf(command, sizeof(command), "atos -o '%s' -l 0x%llx 0x%llx 2>/dev/null", info.dli_fname, static_cast <uint64_t> (base), static_cast <uint64_t> (addr));
					/**
					 * Для операционных систем Linux, FreeBSD и OpenIndiana (illumos/Solaris)
					 */
					#else
						// Формируем команду символизации через addr2line по смещению инструкции внутри модуля
						::snprintf(command, sizeof(command), "addr2line -f -C -p -e '%s' 0x%lx 2>/dev/null", info.dli_fname, static_cast <uint64_t> (addr - base));
					#endif
					// Выполняем команду символизации и получаем её вывод
					result = __awh_exec_command__(command);
					/**
					 * Удаляем завершающие пробельные символы из результата
					 */
					while(!result.empty() && ((result.back() == '\n') || (result.back() == '\r') || (result.back() == ' ') || (result.back() == '\t')))
						// Удаляем последний пробельный символ
						result.pop_back();
					// Если символизатор не смог определить позицию (вернул нерасшифрованный результат) — очищаем результат
					if(result.empty() || (result.find("??") != string::npos))
						// Очищаем результат
						result.clear();
				}
				// Возвращаем результат
				return result;
			}
			/**
			 * @brief Функция выводи трейса ошибок дочернего потока
			 *
			 * @param sig номер сигнала вызвавшего краш
			 *
			 */
			void childCrashHandler(const int32_t sig) noexcept {
				// Буфер для формирования ошибки
				void * array[50];
				// Определяем размер бэктрейса
				const int32_t size = ::backtrace(array, 50);
				// Получаем текстовое представление символов бэктрейса (используется как запасной вариант вывода)
				char ** symbols = ::backtrace_symbols(array, size);
				// Записываем в лог информацию в консоль
				cerr << "Child PID " << ::getpid() << " crashed with signal " << sig << " (" << ::strsignal(sig) << ")\nBacktrace:" << endl;
				/**
				 * Переходим по всем кадрам бэктрейса
				 */
				for(int32_t i = 0; i < size; ++i){
					// Определяем позицию в исходном коде по адресу инструкции
					const string & location = __awh_resolve_line__(array[i]);
					// Если позицию в исходном коде удалось определить
					if(!location.empty())
						// Выводим номер кадра, адрес и расшифрованную позицию «функция файл:строка»
						cerr << "  #" << i << " " << array[i] << " " << location << endl;
					// Если позицию определить не удалось — выводим исходную (закодированную) информацию символа
					else cerr << "  #" << i << " " << ((symbols != nullptr) ? symbols[i] : "") << endl;
				}
				// Если символы бэктрейса были выделены — освобождаем память
				if(symbols != nullptr)
					// Освобождаем память символов бэктрейса
					::free(symbols);
				// Возвращаем стандартный обработчик, чтобы операционная система создала полноценный Crash Report
				::signal(sig, SIG_DFL);
				// Повторно вызываем сигнал
				::kill(::getpid(), sig);
			}
		}
	#endif
#endif

/**
 * @brief Конструктор
 *
 */
awh::unit::Cluster::Worker::Worker() noexcept :
 pid(0), life(0), eid(0) {}

/**
 * @brief Конструктор
 *
 */
awh::unit::Cluster::Rebirth::Rebirth() noexcept :
 mode(false), limit(10),
 window(30000), restarts(0) {}

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
			/**
			 * Создаём дочерние процессы по количеству установленных воркеров
			 */
			for(uint16_t index = 0; index < this->_count; ++index){
				// Создаём очередной воркер с отложенным запуском события
				const family_t family = this->spawn(0, true);
				// Если мы оказались в дочернем процессе — прекращаем создание и выходим в цикл событий
				if(family == family_t::CHILDREN)
					// Выходим из функции
					return;
				// Если воркер создать не удалось — откатываем уже созданные процессы
				else if(family == family_t::NONE) {
					// Освобождаем ресурсы и принудительно завершаем уже созданные процессы
					this->clear(shutdown_t::FORCEFUL);
					// Выходим из функции
					return;
				}
			}
			// Записываем в лог информацию о запущенном кластере
			this->_log->print("Cluster [%s] has been started successfully", log_t::flag_t::INFO, this->_name.c_str());
			/**
			 * Переходим по всему списку активных воркеров
			 */
			for(auto & [pid, worker] : this->_workers){
				// Выполняем фиксацию и запуск работы события
				if(!(this->_io->commit(worker->eid) && this->_io->launch(worker->eid))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог запуска события
						this->_log->debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, pid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог запуска события
						this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, pid);
					#endif
					// Выходим из приложения
					::_exit(EXIT_FAILURE);
				}
			}
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const pid_t, const event_t)> ("events", this->_pid, event_t::START);
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод размещения нового дочернего процесса
 *
 * @param pid идентификатор убитого процесса
 *
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
			// Создаём новый дочерний процесс с немедленным запуском события взамен завершившегося
			this->spawn(pid, false);
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(pid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод освобождения ресурсов воркера
 *
 * @param eid идентификатор события воркера
 *
 */
void awh::unit::Cluster::release([[maybe_unused]] const event::id_t eid) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Сбрасываем функцию обратного вызова на изменение статуса, чтобы не реагировать на DESTROYED при ручном закрытии события
		this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
		// Удаляем соответствие идентификатора события и идентификатора процесса
		this->_matching.erase(eid);
		// Уничтожаем событие (закрывает сокет, что уведомляет дочерний процесс о завершении работы)
		this->_io->destroy(eid);
	#endif
}
/**
 * @brief Метод запуска/остановки работы кластера
 *
 * @param status статус запуска/остановки кластера
 *
 */
void awh::unit::Cluster::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы сервера
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа кластера запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED): {
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("cluster_status", status);
			// Сбрасываем счётчик подряд идущих быстрых падений при запуске кластера
			this->_rebirth.restarts = 0;
			// Создаём событие пробуждения до запуска дочерних процессов (отложенная обработка сигнала SIGCHLD)
			this->_wakeup = this->_io->event(event::node_t::NOTIFY, event::family_t::USER);
			// Если событие пробуждения создано и его настройки зафиксированы
			if((this->_wakeup != 0) && this->_io->commit(this->_wakeup)){
				// Устанавливаем функцию обратного вызова на чтение для отложенной обработки завершившихся процессов
				this->_io->on(this->_wakeup, static_cast <engine::callback::read_t> (std::bind(&cluster_t::reap, this, _1, _2, _3)));
				// Запускаем работу события пробуждения
				if(!this->_io->launch(this->_wakeup)){
					// Уничтожаем событие пробуждения
					this->_io->destroy(this->_wakeup);
					// Обнуляем идентификатор события пробуждения
					this->_wakeup = 0;
				}
			// Если событие пробуждения создать не удалось
			} else if(this->_wakeup != 0) {
				// Уничтожаем событие пробуждения
				this->_io->destroy(this->_wakeup);
				// Обнуляем идентификатор события пробуждения
				this->_wakeup = 0;
			}
			// Если количество создаваемых процессов установлено
			if(this->_count > 0)
				// Выполняем создание дочерних процессов
				this->create();
			// Если количество создаваемых процессов не установлено
			else {
				// Записываем в лог информацию о запущенном сервере на PIPE
				this->_log->print("Cluster [%s] has been started successfully", log_t::flag_t::INFO, this->_name.c_str());
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event_t)> ("events", this->_pid, event_t::START);
			}
		} break;
		// Если работа кластера подлежит уничтожение
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Если список активных воркеров не пустой
			if(!this->_workers.empty()){
				/**
				 * Переходим по всему списку активных воркеров
				 */
				for(auto & [pid, worker] : this->_workers){
					// Запрещаем анализ остановленного процесса
					worker->pid = 0;
					// Освобождаем ресурсы воркера (закрываем сокет, что завершает дочерний процесс)
					this->release(worker->eid);
				}
				// Уничтожаем события всех активных воркеров
				this->_workers.clear();
			}
			// Если событие пробуждения создано — уничтожаем его
			if(this->_wakeup != 0){
				// Уничтожаем событие пробуждения
				this->_io->destroy(this->_wakeup);
				// Обнуляем идентификатор события пробуждения
				this->_wakeup = 0;
			}
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("cluster_status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid)){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> (fid, status);
				// Выполняем получение функции обратного вызова
				this->_callback.set(fid, this->_callback.id("status"), this->_callback);
			}
		} break;
	}
}
/**
 * @brief Метод создания одного дочернего процесса (воркера)
 *
 * @param replaced идентификатор замещаемого (упавшего) процесса, либо 0 при первичном создании
 * @param deferred флаг отложенного запуска события (true — фиксация/запуск выполняются позже пакетно)
 * @return         семейство процесса: MASTER — родитель, CHILDREN — дочерний, NONE — ошибка создания
 *
 */
awh::unit::cluster_t::family_t awh::unit::Cluster::spawn([[maybe_unused]] const pid_t replaced, [[maybe_unused]] const bool deferred) noexcept {
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
				// Записываем ошибку в лог
				this->_log->debug("Child process worker could not be created", __PRETTY_FUNCTION__, make_tuple(replaced, deferred), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Child process worker could not be created", log_t::flag_t::CRITICAL);
			#endif
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
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
					// Записываем ошибку в лог
					this->_log->debug("Child process could not be created", __PRETTY_FUNCTION__, make_tuple(replaced, deferred), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Child process could not be created", log_t::flag_t::CRITICAL);
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
			} break;
			// Если процесс является дочерним
			case 0: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					{
						// Создаём объект перехвата сигнала
						struct sigaction sa{};
						// Устанавливаем обрабочик перехвата сигнала
						sa.sa_handler = ::childCrashHandler;
						// Зануляем маску объекта перехватчика
						sigemptyset(&sa.sa_mask);
						// Сбрасываем флаги перехватчика
						sa.sa_flags = 0;
						// Устанавливаем перехват сигнала SIGSEGV
						::sigaction(SIGSEGV, &sa, nullptr);
						// Устанавливаем перехват сигнала SIGBUS
						::sigaction(SIGBUS, &sa, nullptr);
						// Устанавливаем перехват сигнала SIGILL
						::sigaction(SIGILL, &sa, nullptr);
						// Устанавливаем перехват сигнала SIGABRT
						::sigaction(SIGABRT, &sa, nullptr);
					}
				#endif
				// Если родительский процесс живой
				if(this->_pid == ::getppid()){
					// Если список активных воркеров не пустой
					if(!this->_workers.empty()){
						/**
						 * Перебираем всех активных воркеров
						 */
						for(auto i = this->_workers.begin(); i != this->_workers.end();){
							// Уничтожаем событие других дочерних процессов
							this->_io->destroy(i->second->eid);
							// Удаляем воркера из списка активных воркеров
							i = this->_workers.erase(i);
						}
					}
					// Если список соответствия не пустой
					if(!this->_matching.empty())
						// Очищаем список соответствия идентификаторов событий и идентификатора процесса
						this->_matching.clear();
					// Уничтожаем унаследованное событие пробуждения (дочерний процесс не пожинает завершившиеся процессы)
					if(this->_wakeup != 0){
						// Уничтожаем событие пробуждения
						this->_io->destroy(this->_wakeup);
						// Обнуляем идентификатор события пробуждения
						this->_wakeup = 0;
					}
					// Уничтожаем событие родительского процесса
					this->_io->destroy(events[0]);
					// Выполняем переинициализацию асинхронного движка ввода-вывода
					this->_io->reinitialize();
					// Устанавливаем опции события
					if(!this->_io->setOptions(events[1], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, make_tuple(replaced, deferred), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
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
						// Записываем в лог сообщение об успешном запуске события
						this->_log->print("Cluster worker process [%d] has been started successfully", log_t::flag_t::INFO, ret.first->first);
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const event_t)> ("events", ret.first->first, event_t::START);
					// Если событие не может быть запущено
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог запуска события
							this->_log->debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, make_tuple(replaced, deferred), log_t::flag_t::CRITICAL, ret.first->first);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог запуска события
							this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, ret.first->first);
						#endif
						// Выходим из приложения
						::_exit(EXIT_FAILURE);
					}
				// Если родительский процесс умер
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(replaced, deferred), log_t::flag_t::CRITICAL, ::getpid());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Процесс превратился в зомби, самоликвидируем его
						this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
					#endif
					// Выходим из приложения
					::_exit(EXIT_FAILURE);
				}
				// Возвращаем признак дочернего процесса (создание следующих воркеров должно быть прекращено)
				return family_t::CHILDREN;
			}
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
						// Записываем ошибку в лог
						this->_log->debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, make_tuple(replaced, deferred), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
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
				this->_matching.emplace(ret.first->second->eid, ret.first->first);
				// Если запуск события не отложен (одиночное размещение воркера во время работы кластера)
				if(!deferred){
					// Выполняем фиксацию и запуск работы события
					if(!(this->_io->commit(ret.first->second->eid) && this->_io->launch(ret.first->second->eid))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог запуска события
							this->_log->debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, make_tuple(replaced, deferred), log_t::flag_t::CRITICAL, replaced);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог запуска события
							this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, replaced);
						#endif
						// Выходим из приложения
						::_exit(EXIT_FAILURE);
					}
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const pid_t)> ("rebase", replaced, ret.first->first);
				}
			} break;
		}
		// Возвращаем признак родительского процесса
		return family_t::MASTER;
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Возвращаем результат отсутствия созданного воркера
		return family_t::NONE;
	#endif
}
/**
 * @brief Метод перезапуска упавшего процесса
 *
 * @param pid    идентификатор упавшего процесса
 * @param status статус остановившегося процесса
 *
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
			// Если завершившийся процесс требуется анализировать дальше
			if(i->second->pid == pid){
				// Записываем в лог сообщение об остановке дочернего процесса
				this->_log->print("Child process stopped, PID=%d, STATUS=%d", log_t::flag_t::WARNING, pid, status);
				// Определяем, является ли завершение ручной остановкой процесса (сигнал SIGINT)
				const bool manual = (WIFSIGNALED(status) && (WTERMSIG(status) == SIGINT));
				// Если это ручная остановка процесса — останавливаем весь кластер
				if(manual){
					// Освобождаем ресурсы всех воркеров и очищаем список активных воркеров
					this->clear(shutdown_t::NONE);
					// Выходим из приложения с кодом сигнала ручной остановки
					::_exit(SIGINT);
				}
				// Определяем, упал ли процесс в пределах временного окна жизни (признак быстрого/раннего падения)
				const bool rapid = ((this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) - i->second->life) <= this->_rebirth.window);
				// Освобождаем ресурсы завершившегося воркера
				this->release(i->second->eid);
				// Удаляем завершившийся процесс из списка активных воркеров
				this->_workers.erase(i);
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const int32_t)> ("exit", pid, status);
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event_t)> ("events", pid, event_t::STOP);
				// Если разрешён автоматический перезапуск процесса
				if(this->_rebirth.mode){
					// Если процесс упал слишком быстро
					if(rapid)
						// Увеличиваем счётчик подряд идущих быстрых падений
						++this->_rebirth.restarts;
					// Если процесс прожил достаточно долго — сбрасываем счётчик быстрых падений
					else this->_rebirth.restarts = 0;
					// Если защита включена и число подряд идущих быстрых падений превысило порог — прекращаем перезапуск и останавливаем кластер
					if((this->_rebirth.limit > 0) && (this->_rebirth.restarts >= this->_rebirth.limit)){
						// Записываем в лог сообщение об обнаружении цикла перезапусков
						this->_log->print("Cluster [%s] worker keeps crashing on startup, aborting after %u rapid restarts", log_t::flag_t::CRITICAL, this->_name.c_str(), this->_rebirth.restarts);
						// Освобождаем ресурсы оставшихся воркеров и очищаем список активных воркеров
						this->clear(shutdown_t::NONE);
						// Выходим из приложения с кодом завершения дочернего процесса
						::_exit(WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE);
					}
					// Выполняем создание нового процесса взамен упавшего
					this->emplace(pid);
				}
			// Если завершившийся процесс анализировать не нужно
			} else {
				// Освобождаем ресурсы воркера
				this->release(i->second->eid);
				// Удаляем завершившийся процесс из списка активных воркеров
				this->_workers.erase(i);
			}
		}
	#endif
}
/**
 * @brief Метод отложенной обработки завершившихся процессов (выполняется в цикле событий)
 *
 * @param eid  идентификатор события пробуждения
 * @param data данные события пробуждения
 * @param size размер данных события пробуждения
 *
 */
void awh::unit::Cluster::reap([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const uint8_t * data, [[maybe_unused]] const size_t size) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Идентификатор завершившегося процесса
		pid_t pid = 0;
		// Статус завершившегося процесса
		int32_t status = 0;
		/**
		 * Пожинаем все завершившиеся дочерние процессы. Метод выполняется в потоке цикла событий,
		 * поэтому мутации контейнеров, перезапуск воркеров и логирование здесь безопасны.
		 */
		while((pid = ::waitpid(-1, &status, WNOHANG)) > 0)
			// Выполняем обработку завершившегося процесса
			this->process(pid, status);
	#endif
}
/**
 * @brief Функция фильтр перехватчика сигналов
 *
 * @param signal номер сигнала полученного системой
 * @param info   объект информации полученный системой
 * @param ctx    передаваемый внутренний контекст
 *
 */
void awh::unit::Cluster::child([[maybe_unused]] int32_t signal, [[maybe_unused]] siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если объект кластера ещё существует
		if(::__awh_cluster__ != nullptr){
			// Получаем указатель на объект кластера
			cluster_t * self = ::__awh_cluster__;
			// Если событие пробуждения активно
			if(self->_wakeup != 0){
				// Байт-маркер для триггера события пробуждения
				const uint8_t marker = 0x01;
				/**
				 * Не выполняем здесь waitpid/fork/логирование и мутацию контейнеров: контекст обработчика
				 * сигнала не является асинхронно-сигнал-безопасным. Безопасно триггерим событие пробуждения
				 * (один системный вызов), чтобы фактическая обработка завершившихся процессов выполнилась
				 * в потоке цикла событий (см. метод reap).
				 */
				self->_io->send(self->_wakeup, &marker, sizeof(marker));
			}
		}
	#endif
}
/**
 * @brief Метод обработки событий записи сообщений кластера
 *
 * @param eid  идентификатор события
 * @param size размер сообщения
 *
 */
void awh::unit::Cluster::write(const event::id_t eid, const size_t size) noexcept {
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("sending");
	// Если функция обратного вызова установлена
	if(this->_callback.is(fid)){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_matching.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_matching.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const size_t)> (fid, i->second, size);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const size_t)> (fid, this->_pid, size);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(eid, size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
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
 *
 */
void awh::unit::Cluster::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("message");
	// Если функция обратного вызова установлена
	if(this->_callback.is(fid)){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_matching.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_matching.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> (fid, i->second, data, size);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> (fid, this->_pid, data, size);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(eid, data, size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
			}
		}
	}
}
/**
 * @brief Метод обработки состояния кластера
 *
 * @param eid    идентификатор события
 * @param status статус события
 *
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
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const pid_t, const int32_t)> ("exit", i->first, SIGSTOP);
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const pid_t, const event_t)> ("events", i->first, event_t::STOP);
							// Удаляем завершившийся процесс из списка активных воркеров
							this->_workers.erase(i);
							// Завершаем работу процесса
							::_exit(EXIT_SUCCESS);
						}
					}
				// Если родительский процесс умер
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, ::getpid());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Процесс превратился в зомби, самоликвидируем его
						this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
					#endif
					// Выходим из приложения
					::_exit(EXIT_FAILURE);
				}
			}
		} break;
		// Если мы получили любой другой статус
		default: {
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("state");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid)){
				// Если процесс является родительским
				if(this->master()){
					// Выполняем поиск идентификатора процесса по идентификатору события
					auto i = this->_matching.find(eid);
					// Если идентификатор процесса найден
					if(i != this->_matching.end())
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const event::status_t)> (fid, i->second, status);
				// Если процесс является дочерним
				} else {
					// Если родительский процесс живой
					if(this->_pid == ::getppid())
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const event::status_t)> (fid, this->_pid, status);
					// Если родительский процесс умер
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (status)), log_t::flag_t::CRITICAL, ::getpid());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Процесс превратился в зомби, самоликвидируем его
							this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
						#endif
						// Выходим из приложения
						::_exit(EXIT_FAILURE);
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
 *
 */
void awh::unit::Cluster::error(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("error");
	// Если функция обратного вызова установлена
	if(this->_callback.is(fid)){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_matching.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_matching.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::error_t, const string &)> (fid, i->second, error, message);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::error_t, const string &)> (fid, this->_pid, error, message);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (error), message), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
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
 *
 */
void awh::unit::Cluster::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("available");
	// Если функция обратного вызова установлена
	if(this->_callback.is(fid)){
		// Если процесс является родительским
		if(this->master()){
			// Выполняем поиск идентификатора процесса по идентификатору события
			auto i = this->_matching.find(eid);
			// Если идентификатор процесса найден
			if(i != this->_matching.end())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::status_t, const size_t)> (fid, i->second, status, size);
		// Если процесс является дочерним
		} else {
			// Если родительский процесс живой
			if(this->_pid == ::getppid())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const event::status_t, const size_t)> (fid, this->_pid, status, size);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (status), size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
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
				// Записываем ошибку в лог
				this->_log->debug("Only the master process can stop the cluster", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
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
			// Записываем ошибку в лог запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог запуска события
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
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("status");
				// Если функция обратного вызова установлена
				if(this->_callback.is(fid))
					// Выполняем получение функции обратного вызова
					this->_callback.set(fid, this->_callback.id("cluster_status"), this->_callback);
				// Устанавливаем функцию обратного вызова на запуск системы
				this->_callback.on <void (const event::status_t)> (fid, &cluster_t::launch, this, _1);
				// Выполняем запуск работы основного юнита
				unit_t::start();
			}
		// Если процесс является дочерним то выводим сообщение об ошибке
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Only the master process can start the cluster", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
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
			// Записываем ошибку в лог запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог запуска события
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
}
/**
 * @brief Метод очистки всех выделенных ресурсов
 *
 * @param shutdown тип завершения работы кластера
 *
 */
void awh::unit::Cluster::clear(const shutdown_t shutdown) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Если список активных воркеров не пустой
		if(!this->_workers.empty()){
			/**
			 * Переходим по всему списку активных воркеров
			 */
			for(auto & [pid, worker] : this->_workers){
				// Запрещаем анализ остановленного процесса
				worker->pid = 0;
				// Освобождаем ресурсы воркера (сброс колбэка статуса, очистка соответствия, уничтожение события)
				this->release(worker->eid);
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
			// Записываем ошибку в лог
			this->_log->debug("Only the master process can clear the cluster", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
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
				// Записываем ошибку в лог
				this->_log->debug("Only the master process can create child processes", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
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
			// Записываем ошибку в лог запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог запуска события
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
}
/**
 * @brief Метод удаления активного процесса
 *
 * @param pid       идентификатор процесса
 * @param shutdown тип завершения работы кластера
 *
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
				// Освобождаем ресурсы воркера (сброс колбэка статуса, очистка соответствия, уничтожение события)
				this->release(i->second->eid);
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
			// Записываем ошибку в лог
			this->_log->debug("Only the master process can remove child processes", __PRETTY_FUNCTION__, make_tuple(pid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Only the master process can remove child processes", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод получения типа протокола передачи данных между воркерами
 *
 * @return тип протокола передачи данных между воркерами
 *
 */
awh::event::type_t awh::unit::Cluster::getTypeEventMessage() const noexcept {
	// Получаем тип протокола передачи данных между воркерами
	return this->_type;
}
/**
 * @brief Метод установки типа протокола передачи данных между воркерами
 *
 * @param type тип протокола передачи данных между воркерами для установки
 *
 */
void awh::unit::Cluster::setTypeEventMessage(const event::type_t type) noexcept {
	// Устанавливаем тип протокола передачи данных между воркерами
	this->_type = type;
}
/**
 * @brief Метод установки флага автоматического возрождения процессов
 *
 * @param mode флаг возрождения процессов
 *
 */
void awh::unit::Cluster::rebirth(const bool mode) noexcept {
	// Устанавливаем флаг автоматического возрождения процессов
	this->_rebirth.mode = mode;
}
/**
 * @brief Метод установки параметров защиты от цикла перезапусков воркеров
 *
 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
 * @param window временное окно «быстрого» (раннего) падения воркера в миллисекундах
 *
 */
void awh::unit::Cluster::rebirthLimit(const uint16_t limit, const uint64_t window) noexcept {
	// Устанавливаем максимальное число подряд идущих быстрых падений воркеров
	this->_rebirth.limit = limit;
	// Устанавливаем временное окно «быстрого» падения воркера
	this->_rebirth.window = window;
}
/**
 * @brief Метод установки названия кластера
 *
 * @param name название кластера для установки
 *
 */
void awh::unit::Cluster::name(string_view name) noexcept {
	// Устанавливаем название кластера
	this->_name = name;
}
/**
 * @brief Метод получения максимального количества процессов
 *
 * @return максимальное количество процессов
 *
 */
uint16_t awh::unit::Cluster::count() const noexcept {
	// Получаем максимальное количество процессов
	return this->_count;
}
/**
 * @brief Метод установки максимального количества процессов
 *
 * @param count максимальное количество процессов
 *
 */
void awh::unit::Cluster::count(const uint16_t count) noexcept {
	// Устанавливаем максимальное количество процессов
	this->_count = count;
}
/**
 * @brief Метод получения списка дочерних процессов
 *
 * @return список дочерних процессов
 *
 */
unordered_set <pid_t> awh::unit::Cluster::workers() const noexcept {
	// Переменная результата
	unordered_set <pid_t> result;
	// Если список активных воркеров не пустой
	if(!this->_workers.empty()){
		/**
		 * Переходим по всему списку активных воркеров
		 */
		for(const auto & [pid, worker] : this->_workers)
			// Добавляем идентификатор процесса в результат
			result.emplace(pid);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
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
 *
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
					// Записываем ошибку в лог
					this->_log->debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					this->_log->print("Process [%d] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
			}
		// Если процесс является родительским
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("A message addressed to a parent process can only be sent from child processes", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
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
			// Записываем ошибку в лог запуска события
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог запуска события
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения дочернему процессу
 *
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
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
				// Записываем ошибку в лог
				this->_log->debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, make_tuple(pid, buffer, size), log_t::flag_t::WARNING);
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
			// Записываем ошибку в лог
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, make_tuple(pid, buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения всем дочерним процессам
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
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
				// Переменная результата
				size_t result = 0;
				/**
				 * Переходим по всему списку активных воркеров
				 */
				for(const auto & [pid, worker] : this->_workers)
					// Отправляем сообщение дочернему процессу
					result += this->_io->send(worker->eid, buffer, size);
				// Возвращаем результат
				return result;
			}
		// Если процесс является дочерним
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
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
			// Записываем ошибку в лог
			this->_log->debug("MS Windows OS, does not support cluster mode", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
		#endif
	#endif
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @return       размер буфера события
 *
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
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 *
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
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::Cluster::Cluster(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _name{AWH_SHORT_NAME},
 _count(0), _wakeup(0), _type(event::type_t::SEQPACKET) {
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
			// Если количество доступных ядер определить не удалось
			if(this->_count == 0)
				// Используем один воркер по умолчанию
				this->_count = 1;
			// Если количество доступных воркеров больше 1-х, уменьшаем пополам
			else if(this->_count > 1)
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
	// Если событие пробуждения создано — уничтожаем его
	if(this->_wakeup != 0){
		// Уничтожаем событие пробуждения
		this->_io->destroy(this->_wakeup);
		// Обнуляем идентификатор события пробуждения
		this->_wakeup = 0;
	}
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Если разрушаемый объект является текущим зарегистрированным кластером
		if(::__awh_cluster__ == this){
			// Создаём объект восстановления стандартного обработчика сигнала
			struct sigaction sa{};
			// Устанавливаем стандартный обработчик сигнала
			sa.sa_handler = SIG_DFL;
			// Зануляем маску перехватчика
			sigemptyset(&sa.sa_mask);
			// Сбрасываем флаги перехватчика
			sa.sa_flags = 0;
			// Восстанавливаем стандартный обработчик сигнала завершения дочерних процессов
			::sigaction(SIGCHLD, &sa, nullptr);
			// Сбрасываем глобальный указатель на объект кластера
			::__awh_cluster__ = nullptr;
		}
	#endif
}
