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
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <dlfcn.h>
	#include <sys/wait.h>
	/**
	 * Разбор стека вызовов доступен не везде
	 *
	 * @note Заголовок execinfo.h вместе с backtrace() - принадлежность glibc и систем
	 *       BSD, а НЕ стандарта. У musl его нет вовсе: обращение к нему валит сборку
	 *       ещё на подключении - проверено на стенде Alpine 3.24 (12.08.2026).
	 *       Признак __GLIBC__ musl не объявляет, им и различаются
	 */
	#if !__linux__ || defined(__GLIBC__)
		#define AWH_BACKTRACE_SUPPORTED 1
		#include <execinfo.h>
	#endif
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
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * @brief Состояние завершения воркера, остановленного мастером
	 *
	 * @details Мастер останавливает воркера закрытием своего конца канала, и тот
	 *          завершает работу сам. Наружу об этом извещает событие "exit",
	 *          принимающее состояние завершения в том виде, в каком его отдаёт система,
	 *          поэтому и значение это у каждой системы своё
	 *
	 *          У POSIX берётся SIGSTOP: число это - правильно сложенное состояние
	 *          ожидания, читаемое разборными макросами как «снят сигналом SIGSTOP».
	 *          Подделывать тот же номер у MS Windows нельзя - там на его месте стоит
	 *          код завершения, и число 17 или 19 прочиталось бы как обычный код
	 *          возврата приложения
	 *
	 *          Взамен у MS Windows берётся значение по правилам NTSTATUS: старшие
	 *          разряды несут признак важности «ошибка» вместе с разрядом, отведённым
	 *          значениям прикладным. Разряд этот затем и заведён - чтобы значения
	 *          приложений не путались с системными, и по нему же метод `crashed`
	 *          отличает падение от остановки
	 *
	 */
	#if _WIN32 || _WIN64
		constexpr int32_t AWH_CLUSTER_STOPPED = static_cast <int32_t> (0xE0000001u);
	#else
		constexpr int32_t AWH_CLUSTER_STOPPED = SIGSTOP;
	#endif
};

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция принудительного завершения процесса
	 *
	 * @param pid идентификатор завершаемого процесса
	 *
	 * @details Соответствие между системами прямое: `kill(pid, SIGKILL)` у POSIX и
	 *          `TerminateProcess` у MS Windows. Ни то, ни другое процесс перехватить
	 *          не может, и ни то, ни другое не даёт ему довести работу до конца
	 *
	 * @note Дескриптор процесса у MS Windows приходится открывать заново по номеру:
	 *       список активных воркеров хранит именно номера процессов. Номер система
	 *       переиспользует, поэтому вызывать функцию допустимо лишь для процессов,
	 *       которые кластер считает живыми
	 *
	 */
	void __awh_terminate__([[maybe_unused]] const pid_t pid) noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем открытие дескриптора завершаемого процесса
			HANDLE handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, static_cast <DWORD> (pid));
			// Если дескриптор процесса получен
			if(handle != nullptr){
				// Выполняем принудительное завершение процесса
				::TerminateProcess(handle, static_cast <UINT> (EXIT_FAILURE));
				// Закрываем дескриптор процесса
				::CloseHandle(handle);
			}
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Выполняем принудительное завершение процесса
			::kill(pid, SIGKILL);
		#endif
	}
};

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
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * @brief Инкапсулируем состояние управления процессами в пространство имён
	 *
	 * @details Ничего этого нет у POSIX: там дочерний процесс достаётся вызовом fork,
	 *          о завершении его извещает сигнал SIGCHLD, а пожинает его waitpid. У
	 *          MS Windows каждое из трёх заменяется своим средством, и всем трём нужно
	 *          где-то держать состояние
	 *
	 */
	namespace {
		/**
		 * @brief Дочерний процесс кластера
		 *
		 */
		struct Child {
			// Дескриптор объекта процесса
			HANDLE process;
			// Дескриптор ожидания завершения процесса из системного пула потоков
			HANDLE wait;
			/**
			 * @brief Конструктор
			 *
			 */
			Child() noexcept :
			 process(nullptr), wait(nullptr) {}
		};

		/**
		 * @brief Объект кластера для работы статических функций
		 *
		 */
		static awh::unit::cluster_t * __awh_cluster__ = nullptr;
		/**
		 * @brief Объект задания, удерживающий дочерние процессы
		 *
		 * @details Задание заводится с пределом JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE:
		 *          закрытие последнего дескриптора задания снимает все входящие в него
		 *          процессы. Так замещается проверка на осиротевание, какую у POSIX даёт
		 *          сравнение с getppid: погибни мастер любым образом, включая падение,
		 *          система снимет воркеров сама, и осиротевших процессов не остаётся
		 *
		 */
		static HANDLE __awh_job__ = nullptr;
		// Список дочерних процессов, за которыми ведётся наблюдение
		static std::unordered_map <pid_t, Child> __awh_children__;
		// Очередь завершившихся процессов, ожидающих разбора в петле событий
		static std::deque <pid_t> __awh_finished__;
		// Замок доступа к спискам дочерних процессов
		static std::mutex __awh_children_mutex__;
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
				/**
				 * Если разбор стека вызовов системой не поддерживается
				 *
				 * @note Сообщение о падении печатается и здесь: без стека оно менее
				 *       подробно, но молчать о падении дочернего процесса нельзя
				 */
				#if !defined(AWH_BACKTRACE_SUPPORTED)
					// Записываем в лог сообщение о падении дочернего процесса
					cerr << "Child PID " << ::getpid() << " crashed with signal " << sig << " (" << ::strsignal(sig) << ")" << endl;
				/**
				 * Если разбор стека вызовов системой поддерживается
				 */
				#else
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
				#endif
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
 * @brief Метод проверки, что родительский процесс жив
 *
 * @return признак того, что родительский процесс жив
 *
 */
bool awh::unit::Cluster::parent() const noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Если дескриптор объекта родительского процесса не получен - процесс родителя не отслеживается
		if(this->_master == 0)
			// Сообщаем, что родительского процесса нет
			return false;
		/**
		 * Ожидание объекта процесса с нулевой выдержкой отвечает WAIT_TIMEOUT, пока
		 * процесс работает, и WAIT_OBJECT_0, как только тот завершился. Дескриптор
		 * удерживает запись о процессе в системе, поэтому номер его в этот промежуток
		 * не может достаться другому процессу, и подмены здесь не происходит
		 */
		return (::WaitForSingleObject(reinterpret_cast <HANDLE> (this->_master), 0) == WAIT_TIMEOUT);
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Сообщаем, что родителем процесса по прежнему является мастер кластера
		return (this->_pid == ::getppid());
	#endif
}
/**
 * @brief Метод создания дочерних процессов при запуске кластера
 *
 */
void awh::unit::Cluster::create() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
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
		}
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
void awh::unit::Cluster::emplace(const pid_t pid) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Заслона по системам здесь нет намеренно: порождение работника разведено по
		 * системам внутри самого spawn - у POSIX это fork, у MS Windows повторный запуск
		 * себя, - а размещение взамен выбывшего у них общее
		 *
		 * @note Прежде тело это было заключено в заслон «кроме MS Windows», отчего
		 *       перезапуск упавшего работника там не делал ровно ничего: кластер
		 *       распознавал падение, рассылал события завершения и остановки, а нового
		 *       работника взамен не поднимал. Найдено пробой на стенде
		 */
		// Создаём новый дочерний процесс с немедленным запуском события взамен завершившегося
		this->spawn(pid, false);
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
void awh::unit::Cluster::release(const event::id_t eid) noexcept {
	/**
	 * Заслона по системам здесь нет намеренно: все три действия существуют у обеих
	 * систем, а событие обмена сообщениями заводится и там, и там
	 *
	 * @note Прежде тело это было заключено в заслон «кроме MS Windows», отчего события
	 *       выбывших работников там не уничтожались вовсе - ни при их падении, ни при
	 *       снятии мастером. Соответствие идентификаторов росло без конца
	 */
	// Сбрасываем функцию обратного вызова на изменение статуса, чтобы не реагировать на DESTROYED при ручном закрытии события
	this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
	// Удаляем соответствие идентификатора события и идентификатора процесса
	this->_matching.erase(eid);
	// Уничтожаем событие (закрывает сокет, что уведомляет дочерний процесс о завершении работы)
	this->_io->destroy(eid);
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
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				/**
				 * Распознаём роль процесса по метке в окружении
				 *
				 * Дочерний процесс запускается тем же образом и с той же строкой доводов,
				 * проходит main заново и доходит сюда точно так же, как мастер. Отличает
				 * его лишь метка, выставленная мастером перед запуском
				 */
				if(this->adopt()){
					// Записываем в лог сообщение об успешном запуске воркера
					this->_log->print("Cluster worker process [%d] has been started successfully", log_t::flag_t::INFO, ::getpid());
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("cluster_status", status);
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const event_t)> ("events", static_cast <pid_t> (::getpid()), event_t::START);
					// Дочерний процесс воркеров не создаёт и завершившихся не пожинает
					return;
				}
			#endif
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
				if(this->parent()){
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
					auto ret = this->_workers.emplace(static_cast <pid_t> (worker->pid), ::move(worker));
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
				auto ret = this->_workers.emplace(static_cast <pid_t> (worker->pid), ::move(worker));
				// Добавляем соответствие идентификаторов событий и идентификатора процесса в список соответствия
				this->_matching.emplace(static_cast <event::id_t> (ret.first->second->eid), ret.first->first);
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
		// Создаём новый вокрер дочернего процесса
		unique_ptr <worker_t> worker = make_unique <worker_t> ();
		// Устанавливаем время создания процесса
		worker->life = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
		// Добавляем новые события для обмена сообщениями между процессами
		const auto & events = this->_io->events(event::family_t::PIPE, this->_type);
		// Если события не созданы
		if((events[0] == 0) || (events[1] == 0)){
			// Записываем ошибку в лог
			this->_log->print("Child process worker could not be created", log_t::flag_t::CRITICAL);
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		/**
		 * Передаём порождаемому процессу имя канала обмена сообщениями
		 *
		 * @details Дескриптора по наследству порождённый процесс не получает - он
		 *          проходит main заново, - зато имя именованного канала переносимо и
		 *          доходит до него окружением. Свой конец канала работник открывает по
		 *          этому имени сам
		 *
		 * @note Имя снимается прежде уничтожения события: уничтоженное события имени
		 *       уже не отдаст
		 *
		 */
		const string & pipe = this->_io->getTarget(events[1]);
		// Если имя канала обмена сообщениями получено
		if(!pipe.empty()){
			// Передаём имя канала порождаемому процессу через окружение
			if(!::SetEnvironmentVariableW(L"AWH_CLUSTER_PIPE", this->_fmk->convert(pipe).c_str()))
				// Записываем ошибку в лог
				this->_log->print("Cluster worker pipe name could not be passed to the child process", log_t::flag_t::CRITICAL);
		// Если имя канала обмена сообщениями получить не удалось
		} else this->_log->print("Cluster worker pipe name could not be obtained", log_t::flag_t::CRITICAL);
		// Уничтожаем событие дочернего процесса: унаследовать его порождённый процесс не может
		this->_io->destroy(events[1]);
		// Выполняем порождение дочернего процесса
		const pid_t pid = this->execute();
		// Снимаем имя канала из окружения: своим процессам работник его не передаёт
		::SetEnvironmentVariableW(L"AWH_CLUSTER_PIPE", nullptr);
		// Если порождение процесса не удалось
		if(pid == 0){
			// Уничтожаем событие родительского процесса
			this->_io->destroy(events[0]);
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		// Устанавливаем идентификатор процесса воркера
		worker->pid = pid;
		// Устанавливаем опции события
		if(!this->_io->setOptions(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
			// Записываем ошибку в лог
			this->_log->print("Error setting cluster worker event options", log_t::flag_t::WARNING);
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
		auto ret = this->_workers.emplace(pid, ::move(worker));
		// Добавляем соответствие идентификаторов событий и идентификатора процесса в список соответствия
		this->_matching.emplace(static_cast <event::id_t> (ret.first->second->eid), ret.first->first);
		// Если запуск события не отложен (одиночное размещение воркера во время работы кластера)
		if(!deferred){
			// Выполняем фиксацию и запуск работы события
			if(!(this->_io->commit(ret.first->second->eid) && this->_io->launch(ret.first->second->eid))){
				// Записываем ошибку в лог запуска события
				this->_log->print("Cluster worker process [%d] event could not be launched", log_t::flag_t::CRITICAL, pid);
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
			}
			// Если процесс размещается взамен упавшего
			if(replaced > 0)
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const pid_t)> ("rebase", replaced, ret.first->first);
		}
		// Возвращаем признак родительского процесса
		return family_t::MASTER;
	#endif
}
/**
 * Для операционной системы MS Windows
 *
 * @note Метода этого на системах POSIX нет вовсе: дочерний процесс достаётся там
 *       вызовом fork, продолжающим работу с того же места и с тем же состоянием.
 *       У MS Windows соответствия fork нет, и дочерний процесс приходится
 *       запускать заново - собственным образом приложения
 *
 */
#if _WIN32 || _WIN64
/**
 * @brief Метод порождения дочернего процесса повторным запуском образа приложения
 *
 * @return идентификатор порождённого процесса, либо 0 при отказе
 *
 */
pid_t awh::unit::Cluster::execute() noexcept {
	/**
	 * Заводим объект задания, если тот ещё не заведён
	 *
	 * Задание держит все порождённые процессы и снимает их при закрытии последнего
	 * своего дескриптора - то есть при завершении мастера любым образом, включая
	 * падение. Так замещается проверка на осиротевание, какую у POSIX даёт сравнение
	 * с getppid, и осиротевших воркеров не остаётся
	 */
	if(::__awh_job__ == nullptr){
		// Создаём объект задания
		::__awh_job__ = ::CreateJobObjectW(nullptr, nullptr);
		// Если объект задания создан
		if(::__awh_job__ != nullptr){
			// Создаём объект пределов задания
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
			// Устанавливаем предел снятия процессов при закрытии задания
			limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			// Устанавливаем пределы объекта задания
			if(!::SetInformationJobObject(::__awh_job__, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
				// Записываем ошибку в лог
				this->_log->print("Cluster job object limits could not be set", log_t::flag_t::WARNING);
		// Если объект задания создать не удалось
		} else this->_log->print("Cluster job object could not be created, orphaned workers are possible", log_t::flag_t::WARNING);
	}
	// Буфер под путь к образу приложения
	wchar_t image[MAX_PATH]{0};
	// Получаем путь к образу приложения
	if(::GetModuleFileNameW(nullptr, image, MAX_PATH) == 0){
		// Записываем ошибку в лог
		this->_log->print("Cluster could not determine its own executable path", log_t::flag_t::CRITICAL);
		// Возвращаем признак отсутствия порождённого процесса
		return 0;
	}
	/**
	 * Помечаем роль дочернего процесса в его окружении
	 *
	 * Окружение достаётся порождённому процессу целиком, и метка эта - единственное,
	 * чем тот отличает себя от мастера: запускается он тем же образом и с той же
	 * строкой доводов. Номер мастера в метке нужен ещё и затем, чтобы дочерний процесс
	 * мог открыть дескриптор его объекта и следить, жив ли тот
	 */
	if(!::SetEnvironmentVariableW(L"AWH_CLUSTER_MASTER", std::to_wstring(static_cast <uint32_t> (this->_pid)).c_str())){
		// Записываем ошибку в лог
		this->_log->print("Cluster role marker could not be set in the environment", log_t::flag_t::CRITICAL);
		// Возвращаем признак отсутствия порождённого процесса
		return 0;
	}
	// Копия строки доводов запуска: CreateProcessW вправе менять её на месте
	std::wstring command = ::GetCommandLineW();
	// Создаём объект сведений о запуске процесса
	STARTUPINFOW startup{};
	// Устанавливаем размер объекта сведений о запуске
	startup.cb = sizeof(startup);
	// Создаём объект сведений о порождённом процессе
	PROCESS_INFORMATION info{};
	/**
	 * Порождаем процесс приостановленным: до внесения его в задание он работать не
	 * должен. Успей он завершиться раньше, в задание попасть было бы уже некому, и
	 * снятие по закрытию задания его не коснулось бы
	 */
	const BOOL created = ::CreateProcessW(image, command.data(), nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, nullptr, &startup, &info);
	// Снимаем метку роли из своего окружения, чтобы та не досталась мастеру
	::SetEnvironmentVariableW(L"AWH_CLUSTER_MASTER", nullptr);
	// Если процесс породить не удалось
	if(!created){
		// Записываем ошибку в лог
		this->_log->print("Child process could not be created", log_t::flag_t::CRITICAL);
		// Возвращаем признак отсутствия порождённого процесса
		return 0;
	}
	// Если объект задания заведён — вносим в него порождённый процесс
	if((::__awh_job__ != nullptr) && !::AssignProcessToJobObject(::__awh_job__, info.hProcess))
		// Записываем ошибку в лог
		this->_log->print("Child process [%d] could not be assigned to the cluster job object", log_t::flag_t::WARNING, static_cast <int32_t> (info.dwProcessId));
	// Получаем идентификатор порождённого процесса
	const pid_t pid = static_cast <pid_t> (info.dwProcessId);
	{
		// Создаём запись о наблюдаемом дочернем процессе
		::Child child;
		// Запоминаем дескриптор объекта процесса
		child.process = info.hProcess;
		/**
		 * Подписываемся на завершение процесса
		 *
		 * Извещение однократное: WT_EXECUTEONLYONCE снимает подписку после первого
		 * срабатывания, а завершиться процесс может лишь однажды
		 */
		if(!::RegisterWaitForSingleObject(&child.wait, info.hProcess, &cluster_t::child, reinterpret_cast <PVOID> (static_cast <uintptr_t> (pid)), INFINITE, WT_EXECUTEONLYONCE))
			// Записываем ошибку в лог
			this->_log->print("Child process [%d] termination watch could not be registered", log_t::flag_t::CRITICAL, pid);
		// Выполняем блокировку замка доступа к спискам дочерних процессов
		const std::lock_guard <std::mutex> lock(::__awh_children_mutex__);
		// Добавляем процесс в список наблюдаемых
		::__awh_children__.emplace(pid, child);
	}
	// Возобновляем работу порождённого процесса
	::ResumeThread(info.hThread);
	// Закрываем дескриптор основного потока порождённого процесса: тот больше не нужен
	::CloseHandle(info.hThread);
	// Возвращаем идентификатор порождённого процесса
	return pid;
}
/**
 * @brief Метод распознавания роли дочернего процесса и захвата мастера
 *
 * @return признак того, что процесс является дочерним
 *
 */
bool awh::unit::Cluster::adopt() noexcept {
	// Буфер под метку роли
	wchar_t marker[32]{0};
	// Получаем метку роли из окружения
	const DWORD size = ::GetEnvironmentVariableW(L"AWH_CLUSTER_MASTER", marker, static_cast <DWORD> (sizeof(marker) / sizeof(marker[0])));
	// Если метки роли в окружении нет — процесс является мастером
	if((size == 0) || (size >= (sizeof(marker) / sizeof(marker[0]))))
		// Сообщаем, что процесс дочерним не является
		return false;
	/**
	 * Снимаем метку роли из окружения
	 *
	 * Порождай воркер собственные процессы, метка досталась бы тем по наследству, и
	 * те сочли бы себя воркерами несуществующего мастера
	 */
	::SetEnvironmentVariableW(L"AWH_CLUSTER_MASTER", nullptr);
	// Идентификатор процесса мастера
	pid_t pid = 0;
	/**
	 * Выполняем перехват ошибок разбора
	 */
	try {
		// Разбираем идентификатор процесса мастера
		pid = static_cast <pid_t> (std::stoul(marker));
	/**
	 * Если разобрать метку не удалось
	 */
	} catch(const exception &) {
		// Записываем ошибку в лог
		this->_log->print("Cluster role marker is malformed, the process is treated as master", log_t::flag_t::CRITICAL);
		// Сообщаем, что процесс дочерним не является
		return false;
	}
	/**
	 * Перенимаем номер процесса мастера
	 *
	 * Поле это заполняется в конструкторе основания собственным номером процесса, и
	 * на нём держится метод master. У дочернего процесса, запущенного заново, номер
	 * этот свой, и без подмены тот счёл бы себя мастером
	 */
	this->_pid = pid;
	/**
	 * Открываем дескриптор объекта процесса мастера
	 *
	 * По дескриптору этому метод parent и отвечает, жив ли мастер. Права запрашиваются
	 * наименьшие из достаточных: SYNCHRONIZE довольно, чтобы ожидать объект
	 */
	HANDLE handle = ::OpenProcess(SYNCHRONIZE, FALSE, static_cast <DWORD> (pid));
	// Если дескриптор объекта мастера получен
	if(handle != nullptr)
		// Запоминаем дескриптор объекта родительского процесса
		this->_master = reinterpret_cast <uintptr_t> (handle);
	// Если дескриптор объекта мастера получить не удалось
	else this->_log->print("Cluster master process [%d] could not be opened, orphan detection is disabled", log_t::flag_t::CRITICAL, pid);
	// Открываем свой конец канала обмена сообщениями с мастером
	this->attach();
	// Сообщаем, что процесс является дочерним
	return true;
}
/**
 * @brief Метод открытия своего конца канала обмена сообщениями с мастером
 *
 * @details Соответствия socketpair у MS Windows нет, и пара обмена строится
 *          именованным каналом. Сторону ожидания заводит мастер, а имя её передаёт
 *          порождаемому процессу окружением - дескриптора тот по наследству не
 *          получает, проходя main заново. Здесь имя это снимается, и по нему
 *          открывается свой конец
 *
 * @note Работник заводит себе воркера с собственным номером процесса - ровно так же,
 *       как это делает дочерний процесс на системах POSIX после fork. На нём и
 *       держится отправка сообщений мастеру
 *
 * @return признак того, что канал обмена сообщениями открыт
 *
 */
bool awh::unit::Cluster::attach() noexcept {
	// Буфер под имя канала обмена сообщениями
	wchar_t buffer[256]{0};
	// Получаем имя канала обмена сообщениями из окружения
	const DWORD size = ::GetEnvironmentVariableW(L"AWH_CLUSTER_PIPE", buffer, static_cast <DWORD> (sizeof(buffer) / sizeof(buffer[0])));
	// Если имени канала в окружении нет
	if((size == 0) || (size >= (sizeof(buffer) / sizeof(buffer[0])))){
		// Записываем ошибку в лог
		this->_log->print("Cluster worker pipe name is not set, messaging with the master is disabled", log_t::flag_t::CRITICAL);
		// Сообщаем, что канал обмена сообщениями не открыт
		return false;
	}
	/**
	 * Снимаем имя канала из окружения
	 *
	 * Порождай работник собственные процессы, имя досталось бы тем по наследству, и
	 * те подключились бы к чужому каналу
	 */
	::SetEnvironmentVariableW(L"AWH_CLUSTER_PIPE", nullptr);
	// Заводим событие обмена сообщениями с мастером
	const event::id_t eid = this->_io->event(event::node_t::IPC, event::family_t::PIPE, this->_type);
	// Если событие завести не удалось
	if(eid == 0){
		// Записываем ошибку в лог
		this->_log->print("Cluster worker event could not be created", log_t::flag_t::CRITICAL);
		// Сообщаем, что канал обмена сообщениями не открыт
		return false;
	}
	// Устанавливаем имя канала обмена сообщениями событию
	this->_io->setTarget(eid, this->_fmk->convert(wstring(buffer)));
	// Устанавливаем функцию обратного вызова на событие записи сообщений
	this->_io->on(eid, static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
	// Устанавливаем функцию обратного вызова на событие чтения сообщений
	this->_io->on(eid, static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
	// Устанавливаем функцию обратного вызова на событие изменения состояния
	this->_io->on(eid, static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
	// Устанавливаем функцию обратного вызова на событие получения ошибок
	this->_io->on(eid, static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
	// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
	this->_io->on(eid, static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
	// Создаём воркера для самого себя
	unique_ptr <worker_t> worker = make_unique <worker_t> ();
	// Устанавливаем время создания процесса
	worker->life = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
	// Устанавливаем идентификатор события обмена сообщениями
	worker->eid = eid;
	// Устанавливаем собственный идентификатор процесса
	worker->pid = static_cast <pid_t> (::getpid());
	// Добавляем воркера в список активных воркеров
	auto ret = this->_workers.emplace(static_cast <pid_t> (worker->pid), ::move(worker));
	// Добавляем соответствие идентификатора события идентификатору процесса
	this->_matching.emplace(eid, ret.first->first);
	// Выполняем фиксацию, подключение и запуск работы события
	if(!(this->_io->commit(eid) && this->_io->connect({eid}) && this->_io->launch(eid))){
		// Записываем ошибку в лог
		this->_log->print("Cluster worker event could not be launched", log_t::flag_t::CRITICAL);
		// Сообщаем, что канал обмена сообщениями не открыт
		return false;
	}
	// Сообщаем, что канал обмена сообщениями открыт
	return true;
}
#endif
/**
 * @brief Метод перезапуска упавшего процесса
 *
 * @param pid    идентификатор упавшего процесса
 * @param status статус остановившегося процесса
 *
 */
void awh::unit::Cluster::process(const pid_t pid, const int32_t status) noexcept {
	// Выполняем поиск завершившегося процесса
	auto i = this->_workers.find(pid);
	// Если указанный воркер найден
	if(i != this->_workers.end()){
		// Если завершившийся процесс требуется анализировать дальше
		if(i->second->pid == pid){
			// Записываем в лог сообщение об остановке дочернего процесса
			this->_log->print("Child process stopped, PID=%d, STATUS=%d", log_t::flag_t::WARNING, pid, status);
			// Определяем, является ли завершение ручной остановкой процесса
			const bool manual = cluster_t::manual(status);
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
					::_exit(cluster_t::exitcode(status));
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
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Список завершившихся процессов, снятый с очереди на разбор
		std::deque <pid_t> finished;
		{
			// Выполняем блокировку замка доступа к спискам дочерних процессов
			const std::lock_guard <std::mutex> lock(::__awh_children_mutex__);
			// Забираем всю очередь завершившихся процессов
			finished.swap(::__awh_finished__);
		}
		/**
		 * Пожинаем все завершившиеся дочерние процессы. Метод выполняется в потоке цикла
		 * событий, поэтому мутации контейнеров, перезапуск воркеров и логирование здесь
		 * безопасны
		 */
		for(const pid_t pid : finished){
			// Код завершения процесса
			DWORD status = static_cast <DWORD> (EXIT_FAILURE);
			{
				// Выполняем блокировку замка доступа к спискам дочерних процессов
				const std::lock_guard <std::mutex> lock(::__awh_children_mutex__);
				// Выполняем поиск завершившегося процесса среди наблюдаемых
				auto i = ::__awh_children__.find(pid);
				// Если наблюдаемый процесс найден
				if(i != ::__awh_children__.end()){
					// Получаем код завершения процесса
					if(!::GetExitCodeProcess(i->second.process, &status))
						// Считаем завершение ненормальным, если код получить не удалось
						status = static_cast <DWORD> (EXIT_FAILURE);
					/**
					 * Снимаем наблюдение за процессом. Ожидание снимается доводом
					 * INVALID_HANDLE_VALUE - тот велит системе дождаться завершения уже
					 * начатых извещений, и после возврата обратный вызов не работает ни в
					 * одном потоке. Без этого дескрипторы закрывались бы под работающим
					 * извещением
					 */
					if(i->second.wait != nullptr)
						// Снимаем ожидание завершения процесса
						::UnregisterWaitEx(i->second.wait, INVALID_HANDLE_VALUE);
					// Если дескриптор объекта процесса получен
					if(i->second.process != nullptr)
						// Закрываем дескриптор объекта процесса
						::CloseHandle(i->second.process);
					// Удаляем процесс из списка наблюдаемых
					::__awh_children__.erase(i);
				}
			}
			// Выполняем обработку завершившегося процесса
			this->process(pid, static_cast <int32_t> (status));
		}
	#endif
}
/**
 * Для операционных систем, отличных от MS Windows
 *
 * @note Метода этого под MS Windows нет вовсе - ни объявления в заголовке, ни тела
 *       здесь: сигнала SIGCHLD там не существует, как и типа siginfo_t. О завершении
 *       дочернего процесса извещает ожидание объекта процесса из системного пула
 *       потоков, а пробуждение цикла и разбор завершившихся - общие, через событие
 *       `_wakeup` и метод `reap`
 *
 */
#if !_WIN32 && !_WIN64
/**
 * @brief Функция фильтр перехватчика сигналов
 *
 * @param signal номер сигнала полученного системой
 * @param info   объект информации полученный системой
 * @param ctx    передаваемый внутренний контекст
 *
 */
void awh::unit::Cluster::child([[maybe_unused]] int32_t signal, [[maybe_unused]] siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
	{
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
	}
}
#endif
/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
/**
 * @brief Функция извещения о завершении дочернего процесса
 *
 * @param ctx     идентификатор завершившегося процесса
 * @param timeout признак срабатывания по истечении срока ожидания
 *
 */
void __stdcall awh::unit::Cluster::child(void * ctx, [[maybe_unused]] uint8_t timeout) noexcept {
	{
		// Выполняем блокировку замка доступа к спискам дочерних процессов
		const std::lock_guard <std::mutex> lock(::__awh_children_mutex__);
		// Добавляем завершившийся процесс в очередь ожидающих разбора
		::__awh_finished__.push_back(static_cast <pid_t> (reinterpret_cast <uintptr_t> (ctx)));
	}
	// Если объект кластера ещё существует
	if(::__awh_cluster__ != nullptr){
		// Получаем указатель на объект кластера
		cluster_t * self = ::__awh_cluster__;
		// Если событие пробуждения активно
		if(self->_wakeup != 0){
			// Байт-маркер для триггера события пробуждения
			const uint8_t marker = 0x01;
			/**
			 * Не выполняем здесь разбора завершившихся процессов и мутации контейнеров:
			 * вызов идёт из системного пула потоков, а списки кластера принадлежат потоку
			 * петли событий. Безопасно триггерим событие пробуждения, чтобы разбор
			 * выполнился в потоке петли (см. метод reap)
			 */
			self->_io->send(self->_wakeup, &marker, sizeof(marker));
		}
	}
}
#endif
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
			if(this->parent())
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
			if(this->parent())
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
				if(this->parent()){
					// Выполняем поиск процесса по идентификатору
					auto i = this->_workers.find(::getpid());
					// Если указанный процесс найден
					if(i != this->_workers.end()){
						// Если уничтоженное событие соответствует событию процесса
						if(i->second->eid == eid){
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const pid_t, const int32_t)> ("exit", i->first, AWH_CLUSTER_STOPPED);
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
					if(this->parent())
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
			if(this->parent())
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
			if(this->parent())
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
 * @brief Метод проверки, завершился ли процесс сам
 *
 * @param status состояние завершения процесса
 * @return       признак того, что процесс завершился сам
 *
 */
bool awh::unit::Cluster::exited(const int32_t status) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Различить возврат из main и снятие через TerminateProcess у MS Windows нельзя:
		 * код завершения выставляется и в том, и в другом случае. Отделяется потому лишь
		 * то, что кодом возврата не является вовсе - падение, снятие с клавиатуры и
		 * остановка воркера мастером
		 */
		return (!cluster_t::crashed(status) && !cluster_t::manual(status) && (status != ::AWH_CLUSTER_STOPPED));
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Сообщаем, завершился ли процесс сам
		return WIFEXITED(status);
	#endif
}
/**
 * @brief Метод получения кода возврата завершившегося процесса
 *
 * @param status состояние завершения процесса
 * @return       код возврата процесса, либо EXIT_FAILURE при завершении ненормальном
 *
 */
int32_t awh::unit::Cluster::exitcode(const int32_t status) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Значение GetExitCodeProcess и есть код возврата, разбирать нечего. Ненормальное
		 * же завершение кодом возврата не является вовсе: система кладёт туда NTSTATUS
		 * прервавшего исключения, и выдавать его за код возврата было бы обманом
		 */
		return (cluster_t::exited(status) ? status : EXIT_FAILURE);
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Возвращаем код возврата процесса
		return (WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE);
	#endif
}
/**
 * @brief Метод проверки, снят ли процесс сигналом
 *
 * @param status состояние завершения процесса
 * @return       признак того, что процесс снят сигналом
 *
 */
bool awh::unit::Cluster::signaled([[maybe_unused]] const int32_t status) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Сигналов у MS Windows нет вовсе, снятым сигналом процесс быть не может
		return false;
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Сообщаем, снят ли процесс сигналом
		return WIFSIGNALED(status);
	#endif
}
/**
 * @brief Метод получения номера сигнала, снявшего процесс
 *
 * @param status состояние завершения процесса
 * @return       номер сигнала, либо 0 если процесс снят не сигналом
 *
 */
int32_t awh::unit::Cluster::termsig([[maybe_unused]] const int32_t status) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Сигналов у MS Windows нет вовсе, отдавать нечего
		return 0;
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Возвращаем номер снявшего процесс сигнала
		return (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
	#endif
}
/**
 * @brief Метод проверки, завершился ли процесс ненормально
 *
 * @param status состояние завершения процесса
 * @return       признак ненормального завершения процесса
 *
 */
bool awh::unit::Cluster::crashed(const int32_t status) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Необработанное исключение система кладёт в код завершения значением NTSTATUS,
		 * у которого два старших разряда - признак важности - выставлены в единицы
		 * (`0xC0000005` - обращение по недопустимому адресу, `0xC00000FD` - переполнение
		 * стека). Обычный код возврата приложения в такой диапазон не попадает
		 *
		 * Значения с выставленным прикладным разрядом (`0x20000000`) падением не
		 * считаются: разряд этот затем и отведён, чтобы отличать значения приложений от
		 * системных, и им же помечено собственное состояние остановки воркера
		 *
		 * Снятие с клавиатуры сюда тоже не относится: помечено оно тем же признаком
		 * важности, но падением не является - процесс сняли намеренно
		 */
		const uint32_t code = static_cast <uint32_t> (status);
		// Сообщаем, завершился ли процесс ненормально
		return (((code & 0xC0000000u) == 0xC0000000u) && ((code & 0x20000000u) == 0) && !cluster_t::manual(status));
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Если процесс снят не сигналом - ненормальным завершение не является
		if(!WIFSIGNALED(status))
			// Сообщаем, что завершение ненормальным не является
			return false;
		/**
		 * Определяем сигнал, снявший процесс
		 */
		switch(WTERMSIG(status)){
			// Сигналы, снятие которыми считается падением процесса
			case SIGILL:
			case SIGFPE:
			case SIGBUS:
			case SIGSEGV:
			case SIGABRT:
				// Сообщаем, что процесс завершился ненормально
				return true;
		}
		// Сообщаем, что завершение ненормальным не является
		return false;
	#endif
}
/**
 * @brief Метод проверки, снят ли процесс с клавиатуры
 *
 * @param status состояние завершения процесса
 * @return       признак ручной остановки процесса
 *
 */
bool awh::unit::Cluster::manual(const int32_t status) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Сообщаем, признана ли остановка ручной
		return (static_cast <uint32_t> (status) == static_cast <uint32_t> (STATUS_CONTROL_C_EXIT));
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Сообщаем, признана ли остановка ручной
		return (WIFSIGNALED(status) && (WTERMSIG(status) == SIGINT));
	#endif
}
/**
 * @brief Метод остановки кластера
 *
 */
void awh::unit::Cluster::stop() noexcept {
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
}
/**
 * @brief Метод запуска кластера
 *
 */
void awh::unit::Cluster::start() noexcept {
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
					__awh_terminate__(pid);
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
	 * Заслона по системам здесь нет намеренно - пояснение смотрите у emplace(pid)
	 */
	{
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
	}
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
					__awh_terminate__(i->first);
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
		// Если процесс является дочерним
		if(!this->master()){
			// Если родительский процесс живой
			if(this->parent()){
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
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Обнуляем дескриптор объекта родительского процесса (захватывается дочерним процессом при запуске)
		this->_master = 0;
		// Если кластер уже был создан ранее
		if(::__awh_cluster__ != nullptr)
			// Выполняем генерацию исключения
			throw std::logic_error("A cluster cannot exist twice");
		// Выполняем установку объекта кластера
		::__awh_cluster__ = this;
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
	#endif
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
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Если дескриптор объекта родительского процесса был получен
		if(this->_master != 0){
			// Закрываем дескриптор объекта родительского процесса
			::CloseHandle(reinterpret_cast <HANDLE> (this->_master));
			// Обнуляем дескриптор объекта родительского процесса
			this->_master = 0;
		}
		// Если разрушаемый объект является текущим зарегистрированным кластером
		if(::__awh_cluster__ == this){
			{
				// Выполняем блокировку замка доступа к спискам дочерних процессов
				const std::lock_guard <std::mutex> lock(::__awh_children_mutex__);
				/**
				 * Снимаем наблюдение за всеми оставшимися дочерними процессами
				 */
				for(auto & [pid, child] : ::__awh_children__){
					// Снимаем ожидание завершения процесса, дождавшись начатых извещений
					if(child.wait != nullptr)
						// Снимаем ожидание завершения процесса
						::UnregisterWaitEx(child.wait, INVALID_HANDLE_VALUE);
					// Если дескриптор объекта процесса получен
					if(child.process != nullptr)
						// Закрываем дескриптор объекта процесса
						::CloseHandle(child.process);
				}
				// Очищаем список наблюдаемых процессов
				::__awh_children__.clear();
				// Очищаем очередь завершившихся процессов
				::__awh_finished__.clear();
			}
			/**
			 * Закрываем объект задания
			 *
			 * Закрытие последнего дескриптора задания снимает все входящие в него
			 * процессы - тем и завершается работа воркеров, переживших мастера
			 */
			if(::__awh_job__ != nullptr){
				// Закрываем объект задания
				::CloseHandle(::__awh_job__);
				// Обнуляем объект задания
				::__awh_job__ = nullptr;
			}
			// Сбрасываем глобальный указатель на объект кластера
			::__awh_cluster__ = nullptr;
		}
	#endif
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
