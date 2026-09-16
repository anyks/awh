/**
 * @file cluster.cpp
 * @date 2026-02-21
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
 * @brief Реализация модуля кластера — запуск и контроль дочерних воркеров, обмен сообщениями между процессами,
 *        перезапуск упавших воркеров и защита от цикла быстрых перезапусков
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
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
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/macro/win32.hpp>
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
	#if !defined(__linux__) || defined(__GLIBC__)
		#define AWH_BACKTRACE_SUPPORTED 1
		#include <execinfo.h>
	#endif
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/macro/lib.hpp>
#include <unit/cluster.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

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
	#if defined(_WIN32) || defined(_WIN64)
		constexpr int32_t AWH_CLUSTER_STOPPED = static_cast <int32_t> (0xE0000001u);
	#else
		constexpr int32_t AWH_CLUSTER_STOPPED = SIGSTOP;
	#endif
};

/**
 * @brief Инкапсулируем разметку служебного канала кластера в пространство имён
 *
 */
namespace {
	/**
	 * @brief Заголовок служебного сообщения кластера
	 *
	 * @details Длины в заголовке нет намеренно: служебный канал заводится SEQPACKET
	 *          безусловно, границы сообщений держит он сам, и сообщение приходит
	 *          целым. Оттого и разбирать здесь нечего - заголовок снимается одним
	 *          копированием
	 *
	 */
	__AWH_PACK_BEGIN__
	typedef struct Envelope {
		// Вид служебного сообщения (значение cluster_t::control_t)
		uint8_t type;
		// Узел, которого сообщение касается (0 - никого)
		int32_t pid;
	} __AWH_PACKED__ envelope_t;
	__AWH_PACK_END__
	/**
	 * @brief Предельный размер служебного сообщения кластера
	 *
	 * @details Предел объявлен и проверяется НА ОТПРАВКЕ. Обнаруживать его отказом
	 *          записи в канал нельзя: работник узнал бы о нём случайно и с чужой
	 *          причиной - сообщение, не вместившееся в дейтаграмму, система отвергает
	 *          так же, как и всякое иное
	 *
	 * @note Число взято с запасом вниз от предела дейтаграммы домена UNIX у самой
	 *       скупой из поддерживаемых систем: пересылка заведена под короткие
	 *       сообщения, и упираться в него ей не предназначено
	 *
	 */
	constexpr size_t AWH_CLUSTER_CONTROL_LIMIT = 8192;
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
		#if defined(_WIN32) || defined(_WIN64)
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
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * @brief Инкапсулируем статические типы данных в пространство имён
	 *
	 */
	namespace {
		/**
		 * @brief Объект перехвата сигнала
		 *
		 */
		struct sigaction __awh_action__{};
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
#if defined(_WIN32) || defined(_WIN64)
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
		/**
		 * @brief Замок доступа к спискам дочерних процессов
		 *
		 * @warning Замок этот гасить НЕЛЬЗЯ, и переключателя `threadSafety` у него нет
		 *          намеренно: отклик `Cluster::child` система зовёт из своего пула
		 *          потоков, и он кладёт завершившийся процесс в очередь ровно тогда,
		 *          когда цикл событий её разбирает. Это не опережающая осторожность,
		 *          а два настоящих потока, и работа тут никогда не бывает однопоточной
		 *
		 * @note Оттого замок и оставлен ВКЛЮЧЁННЫМ - состояние блокировок заводится
		 *       включённым, и здесь это то, что нужно
		 *
		 */
		static awh::lock_state_t <> __awh_children_mutex__;
	};
#endif

/**
 * Для операционных систем, отличных от MS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Если включён режим отладки
	 */
	#if defined(DEBUG_MODE)
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
					#if defined(__APPLE__) || defined(__MACH__)
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
 pid(0), life(0), eid(0), cid(0) {}

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
	#if defined(_WIN32) || defined(_WIN64)
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
			awh::log::print("Cluster [%s] has been started successfully", awh::log::flag_t::INFO, this->_name.c_str());
			/**
			 * Переходим по всему списку активных воркеров
			 *
			 * @note На MS Windows события уже зафиксированы и запущены - прежде порождения
			 *       процессов: работник выходит на канал по имени сразу, и конец мастера
			 *       обязан ждать подключения к тому времени
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
			for(auto & [pid, worker] : this->_workers){
				// Если служебный канал заведён, фиксируем и запускаем его
				if((worker->cid != 0) && !(this->_io->commit(worker->cid) && this->_io->launch(worker->cid)))
					// Записываем ошибку в лог
					awh::log::print("Cluster control channel of the worker process [%d] could not be launched", awh::log::flag_t::CRITICAL, pid);
				// Выполняем фиксацию и запуск работы события
				if(!(this->_io->commit(worker->eid) && this->_io->launch(worker->eid))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог запуска события
						awh::log::debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {}, awh::log::flag_t::CRITICAL, pid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог запуска события
						awh::log::print("Cluster worker process [%d] event could not be launched", awh::log::flag_t::CRITICAL, pid);
					#endif
					// Выходим из приложения
					::_exit(EXIT_FAILURE);
				}
			}
			#endif
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			awh::log::debug("%s", __PRETTY_FUNCTION__, {}, awh::log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			awh::log::print("%s", awh::log::flag_t::CRITICAL, error.what());
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			awh::log::debug("%s", __PRETTY_FUNCTION__, {pid}, awh::log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			awh::log::print("%s", awh::log::flag_t::CRITICAL, error.what());
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
	 * Служебный канал освобождается вместе с пользовательским
	 *
	 * @note Ищется он по тому же соответствию, что и сам воркер: снимать его отдельным
	 *       обращением значило бы завести второе место, где о нём надо помнить
	 */
	{
		// Выполняем поиск процесса по идентификатору пользовательского события
		auto i = this->_matching.find(eid);
		// Если процесс найден
		if(i != this->_matching.end()){
			// Выполняем поиск воркера по идентификатору процесса
			auto j = this->_workers.find(i->second);
			// Если воркер найден и служебный канал ему заведён
			if((j != this->_workers.end()) && (j->second->cid != 0)){
				// Сбрасываем функцию обратного вызова на чтение служебного канала
				this->_io->on(j->second->cid, static_cast <engine::callback::read_t> (nullptr));
				// Удаляем соответствие идентификатора служебного события и идентификатора процесса
				this->_controls.erase(j->second->cid);
				// Уничтожаем служебное событие
				this->_io->destroy(j->second->cid);
				// Обнуляем идентификатор служебного события
				j->second->cid = 0;
			}
		}
	}
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
			#if defined(_WIN32) || defined(_WIN64)
				/**
				 * Распознаём роль процесса по метке в окружении
				 *
				 * Дочерний процесс запускается тем же образом и с той же строкой доводов,
				 * проходит main заново и доходит сюда точно так же, как мастер. Отличает
				 * его лишь метка, выставленная мастером перед запуском
				 */
				if(this->adopt()){
					// Записываем в лог сообщение об успешном запуске воркера
					awh::log::print("Cluster worker process [%d] has been started successfully", awh::log::flag_t::INFO, ::getpid());
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
				awh::log::print("Cluster [%s] has been started successfully", awh::log::flag_t::INFO, this->_name.c_str());
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
			// Очищаем список поднявшихся узлов
			this->_online.clear();
			// Очищаем список уходящих намеренно узлов
			this->_leaving.clear();
			// Очищаем список связей между работниками
			this->_links.clear();
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
	#if !defined(_WIN32) && !defined(_WIN64)
		// Создаём новый вокрер дочернего процесса
		unique_ptr <worker_t> worker = make_unique <worker_t> ();
		// Устанавливаем время создания процесса
		worker->life = awh::fmk::timestamp <uint64_t> (awh::fmk::chrono_t::MILLISECONDS);
		// Добавляем новые события для обмена сообщениями между процессами
		const auto & events = this->_io->events(event::family_t::UDS, this->_type);
		// Если события не созданы
		if((events[0] == 0) || (events[1] == 0)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				awh::log::debug("Child process worker could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, awh::log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				awh::log::print("Child process worker could not be created", awh::log::flag_t::CRITICAL);
			#endif
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		/**
		 * Заводим служебный канал обмена, отдельный от пользовательского
		 *
		 * @details Канал этот заводится SEQPACKET БЕЗУСЛОВНО, каким бы ни был
		 *          пользовательский тип обмена: границы сообщений держит он сам, и
		 *          служебное сообщение приходит целым. Пойди служебный обмен по
		 *          пользовательскому каналу - у потокового типа границ не стало бы вовсе,
		 *          и разметка обернулась бы своим протоколом передачи внутри кластера,
		 *          платой на каждом пользовательском сообщении. Разбор - в CLUSTER-LINK.md
		 */
		const auto & controls = this->_io->events(event::family_t::UDS, event::type_t::SEQPACKET);
		// Если служебный канал не заведён
		if((controls[0] == 0) || (controls[1] == 0)){
			// Записываем ошибку в лог
			awh::log::print("Cluster control channel could not be created", awh::log::flag_t::CRITICAL);
			// Если первый конец служебного канала заведён
			if(controls[0] != 0)
				// Уничтожаем первый конец служебного канала
				this->_io->destroy(controls[0]);
			// Если второй конец служебного канала заведён
			if(controls[1] != 0)
				// Уничтожаем второй конец служебного канала
				this->_io->destroy(controls[1]);
			// Уничтожаем оба конца пользовательского канала
			this->_io->destroy(events[0]);
			// Уничтожаем второй конец пользовательского канала
			this->_io->destroy(events[1]);
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
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					awh::log::debug("Child process could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, awh::log::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					awh::log::print("Child process could not be created", awh::log::flag_t::CRITICAL);
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
			} break;
			// Если процесс является дочерним
			case 0: {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
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
							// Если служебный канал другого дочернего процесса заведён
							if(i->second->cid != 0)
								// Уничтожаем служебное событие другого дочернего процесса
								this->_io->destroy(i->second->cid);
							// Удаляем воркера из списка активных воркеров
							i = this->_workers.erase(i);
						}
					}
					// Если список соответствия не пустой
					if(!this->_matching.empty())
						// Очищаем список соответствия идентификаторов событий и идентификатора процесса
						this->_matching.clear();
					// Если список соответствия служебных событий не пустой
					if(!this->_controls.empty())
						// Очищаем список соответствия идентификаторов служебных событий и идентификатора процесса
						this->_controls.clear();
					/**
					 * Забываем связи, унаследованные от мастера
					 *
					 * @note Список этот ведёт мастер, и дочернему процессу он достался по
					 *       наследству чужим: связи в нём чужие, и разрывать их он не вправе
					 */
					if(!this->_links.empty())
						// Очищаем список связей между работниками
						this->_links.clear();
					// Если список уходящих намеренно узлов не пустой
					if(!this->_leaving.empty())
						// Очищаем список уходящих намеренно узлов
						this->_leaving.clear();
					// Если список поднявшихся узлов не пустой
					if(!this->_online.empty())
						// Очищаем список поднявшихся узлов
						this->_online.clear();
					// Уничтожаем унаследованное событие пробуждения (дочерний процесс не пожинает завершившиеся процессы)
					if(this->_wakeup != 0){
						// Уничтожаем событие пробуждения
						this->_io->destroy(this->_wakeup);
						// Обнуляем идентификатор события пробуждения
						this->_wakeup = 0;
					}
					// Уничтожаем событие родительского процесса
					this->_io->destroy(events[0]);
					// Уничтожаем служебное событие родительского процесса
					this->_io->destroy(controls[0]);
					// Выполняем переинициализацию асинхронного движка ввода-вывода
					this->_io->reinitialize();
					// Устанавливаем опции события
					if(!this->_io->setOptions(events[1], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							awh::log::debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, {replaced, deferred}, awh::log::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							awh::log::print("Error setting cluster worker event options", awh::log::flag_t::WARNING);
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
					/**
					 * Заводим свой конец служебного канала
					 *
					 * @note Отклики его - одно лишь чтение: пользовательские отклики
					 *       служебного канала не касаются вовсе, и разметка его наружу не
					 *       выходит
					 */
					this->_io->on(controls[1], static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
					// Устанавливаем идентификатор события для обмена сообщениями между процессами
					worker->eid = events[1];
					// Устанавливаем идентификатор служебного события
					worker->cid = controls[1];
					// Запоминаем свой служебный канал
					this->_control = controls[1];
					// Устанавливаем идентификатор процесса воркера
					worker->pid = ::getpid();
					// Добавляем нового воркера в список активных воркеров
					auto ret = this->_workers.emplace(static_cast <pid_t> (worker->pid), ::move(worker));
					// Выполняем фиксацию и запуск работы события
					if(this->_io->commit(ret.first->second->eid) && this->_io->launch(ret.first->second->eid)){
						/**
						 * Заводим служебный канал вслед за пользовательским
						 *
						 * @warning Отказ здесь работника НЕ валит: без служебного канала он
						 *          лишается связи с соседями и приказов мастера, но свою
						 *          пользовательскую работу ведёт по-прежнему. Валить его
						 *          отказом второстепенного канала значило бы менять
						 *          поведение кластера там, где связи никто и не просил
						 */
						if(!(this->_io->commit(this->_control) && this->_io->launch(this->_control))){
							// Записываем ошибку в лог
							awh::log::print("Cluster control channel could not be launched, the worker is deaf to the master", awh::log::flag_t::CRITICAL);
							// Уничтожаем свой конец служебного канала
							this->_io->destroy(this->_control);
							// Обнуляем идентификатор служебного канала
							this->_control = 0;
							// Обнуляем идентификатор служебного события воркера
							ret.first->second->cid = 0;
						/**
						 * Извещаем мастера о том, что работник поднялся
						 *
						 * @details Извещение это - не вежливость: у MS Windows оно
						 *          единственный способ узнать правду. Мастер знает лишь,
						 *          что процесс породился, а дошёл ли тот до цикла событий,
						 *          ему неизвестно вовсе
						 */
						} else this->dispatch(this->_control, control_t::ONLINE, ret.first->first);
						// Записываем в лог сообщение об успешном запуске события
						awh::log::print("Cluster worker process [%d] has been started successfully", awh::log::flag_t::INFO, ret.first->first);
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const event_t)> ("events", ret.first->first, event_t::START);
					// Если событие не может быть запущено
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог запуска события
							awh::log::debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {replaced, deferred}, awh::log::flag_t::CRITICAL, ret.first->first);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог запуска события
							awh::log::print("Cluster worker process [%d] event could not be launched", awh::log::flag_t::CRITICAL, ret.first->first);
						#endif
						// Выходим из приложения
						::_exit(EXIT_FAILURE);
					}
				// Если родительский процесс умер
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {replaced, deferred}, awh::log::flag_t::CRITICAL, ::getpid());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Процесс превратился в зомби, самоликвидируем его
						awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
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
				// Уничтожаем служебное событие дочернего процесса
				this->_io->destroy(controls[1]);
				// Устанавливаем функцию обратного вызова на чтение служебного канала
				this->_io->on(controls[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
				// Устанавливаем опции события
				if(!this->_io->setOptions(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						awh::log::debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, {replaced, deferred}, awh::log::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						awh::log::print("Error setting cluster worker event options", awh::log::flag_t::WARNING);
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
				// Устанавливаем идентификатор служебного события
				worker->cid = controls[0];
				// Добавляем нового воркера в список активных воркеров
				auto ret = this->_workers.emplace(static_cast <pid_t> (worker->pid), ::move(worker));
				// Добавляем соответствие идентификаторов событий и идентификатора процесса в список соответствия
				this->_matching.emplace(static_cast <event::id_t> (ret.first->second->eid), ret.first->first);
				// Добавляем соответствие идентификаторов служебных событий и идентификатора процесса
				this->_controls.emplace(static_cast <event::id_t> (ret.first->second->cid), ret.first->first);
				// Если запуск события не отложен (одиночное размещение воркера во время работы кластера)
				if(!deferred){
					// Выполняем фиксацию и запуск работы служебного канала
					if(!(this->_io->commit(ret.first->second->cid) && this->_io->launch(ret.first->second->cid)))
						// Записываем ошибку в лог
						awh::log::print("Cluster control channel of the worker process [%d] could not be launched", awh::log::flag_t::CRITICAL, ret.first->first);
					// Выполняем фиксацию и запуск работы события
					if(!(this->_io->commit(ret.first->second->eid) && this->_io->launch(ret.first->second->eid))){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог запуска события
							awh::log::debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {replaced, deferred}, awh::log::flag_t::CRITICAL, replaced);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог запуска события
							awh::log::print("Cluster worker process [%d] event could not be launched", awh::log::flag_t::CRITICAL, replaced);
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
		worker->life = awh::fmk::timestamp <uint64_t> (awh::fmk::chrono_t::MILLISECONDS);
		// Заводим свой конец пользовательского канала обмена сообщениями
		const event::id_t eid = this->provision(L"AWH_CLUSTER_PIPE", this->_type, false);
		// Если пользовательский канал обмена сообщениями не заведён
		if(eid == 0)
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		/**
		 * Заводим свой конец служебного канала обмена
		 *
		 * @details Канал этот заводится SEQPACKET БЕЗУСЛОВНО, каким бы ни был
		 *          пользовательский тип обмена: границы сообщений держит он сам, и
		 *          служебное сообщение приходит целым. Пойди служебный обмен по
		 *          пользовательскому каналу - у потокового типа границ не стало бы вовсе,
		 *          и разметка обернулась бы своим протоколом передачи внутри кластера,
		 *          платой на каждом пользовательском сообщении. Разбор - в CLUSTER-LINK.md
		 */
		const event::id_t cid = this->provision(L"AWH_CLUSTER_CONTROL", event::type_t::SEQPACKET, true);
		// Если служебный канал обмена не заведён
		if(cid == 0){
			// Снимаем имя пользовательского канала из окружения
			::SetEnvironmentVariableW(L"AWH_CLUSTER_PIPE", nullptr);
			// Уничтожаем событие пользовательского канала
			this->_io->destroy(eid);
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		// Выполняем порождение дочернего процесса
		const pid_t pid = this->execute();
		// Снимаем имя канала из окружения: своим процессам работник его не передаёт
		::SetEnvironmentVariableW(L"AWH_CLUSTER_PIPE", nullptr);
		// Снимаем имя служебного канала из окружения по тому же доводу
		::SetEnvironmentVariableW(L"AWH_CLUSTER_CONTROL", nullptr);
		// Если порождение процесса не удалось
		if(pid == 0){
			// Уничтожаем событие родительского процесса
			this->_io->destroy(eid);
			// Уничтожаем служебное событие родительского процесса
			this->_io->destroy(cid);
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		// Устанавливаем идентификатор процесса воркера
		worker->pid = pid;
		// Устанавливаем идентификатор события для обмена сообщениями между процессами
		worker->eid = eid;
		// Устанавливаем идентификатор служебного события
		worker->cid = cid;
		// Добавляем нового воркера в список активных воркеров
		auto ret = this->_workers.emplace(pid, ::move(worker));
		// Добавляем соответствие идентификаторов событий и идентификатора процесса в список соответствия
		this->_matching.emplace(static_cast <event::id_t> (ret.first->second->eid), ret.first->first);
		// Добавляем соответствие идентификаторов служебных событий и идентификатора процесса
		this->_controls.emplace(static_cast <event::id_t> (ret.first->second->cid), ret.first->first);
		/**
		 * Фиксация и запуск события здесь уже произошли - прежде порождения процесса,
		 * и признак отложенного запуска на MS Windows означает лишь то, что оповещать
		 * о замене упавшего процесса рано
		 */
		// Если запуск события не отложен (одиночное размещение воркера во время работы кластера)
		if(!deferred){
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
#if defined(_WIN32) || defined(_WIN64)
/**
 * @brief Метод заведения своего конца канала обмена для порождаемого процесса
 *
 * @param variable имя переменной окружения, какой имя канала уходит порождаемому процессу
 * @param type     устройство обмена канала
 * @param service  признак служебного канала
 * @return         идентификатор заведённого события, либо 0 при отказе
 *
 */
awh::event::id_t awh::unit::Cluster::provision(const wchar_t * variable, const event::type_t type, const bool service) noexcept {
	// Добавляем новые события для обмена сообщениями между процессами
	const auto & events = this->_io->events(event::family_t::PIPE, type);
	// Если события не созданы
	if((events[0] == 0) || (events[1] == 0)){
		// Записываем ошибку в лог
		awh::log::print("Cluster %s channel could not be created", awh::log::flag_t::CRITICAL, (service ? "control" : "worker"));
		// Выводим отсутствие заведённого события
		return 0;
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
	/**
	 * Имя канала обязано дойти до порождаемого процесса
	 *
	 * @warning Прежде оба отказа лишь заносились в журнал, а порождение шло дальше:
	 *          работник поднимался, не находил имени канала в своём окружении и
	 *          оставался ЖИВЫМ, но глухим - обмен сообщениями с мастером у него не
	 *          заводился вовсе. Мастер же считал его исправным работником и слал
	 *          ему задания в никуда
	 *
	 */
	if(pipe.empty() || !::SetEnvironmentVariableW(variable, awh::fmk::convert(pipe).c_str())){
		// Записываем ошибку в лог
		awh::log::print("Cluster %s pipe name %s, the worker is not spawned", awh::log::flag_t::CRITICAL, (service ? "control" : "worker"),
		 (pipe.empty() ? "could not be obtained" : "could not be passed to the child process"));
		// Уничтожаем оба конца канала обмена сообщениями
		this->_io->destroy(events[1]);
		// Уничтожаем событие родительского процесса
		this->_io->destroy(events[0]);
		// Выводим отсутствие заведённого события
		return 0;
	}
	// Уничтожаем событие дочернего процесса: унаследовать его порождённый процесс не может
	this->_io->destroy(events[1]);
	/**
	 * Свой конец канала доводится до ожидания подключения ПРЕЖДЕ порождения процесса
	 *
	 * @details Порождённый процесс выходит на канал по имени сразу, едва запустившись,
	 *          и застаёт единственный экземпляр канала занятым: мастер держал его сам,
	 *          покуда не закрыл свой конец пары, а к ожиданию нового подключения конец
	 *          возвращает лишь подача приёма. Пока приём не подан, обращение работника
	 *          отвечает `ERROR_PIPE_BUSY` (231), и работник сдаётся по сроку ожидания -
	 *          кластер под MS Windows не обменивался тогда ни одним сообщением.
	 *          Проверено щупом на обоих стендах, x86-64 и ARM64
	 *
	 * @note На системах POSIX порядок этот безразличен: там пара достаётся дочернему
	 *       процессу по наследству вызовом fork, уже связанной с обеих сторон
	 */
	// Устанавливаем опции события
	if(!this->_io->setOptions(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
		// Записываем ошибку в лог
		awh::log::print("Error setting cluster worker event options", awh::log::flag_t::WARNING);
	/**
	 * Служебному каналу подписывается одно лишь чтение
	 *
	 * @note Наружу он не выходит вовсе: пользовательские отклики разбирают соответствие
	 *       идентификаторов, а служебные события лежат в своём соответствии, отдельном
	 */
	if(service)
		// Устанавливаем функцию обратного вызова на чтение служебного канала
		this->_io->on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
	// Если канал является пользовательским
	else {
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
	}
	// Выполняем фиксацию и запуск работы события: конец канала переходит к ожиданию подключения
	if(!(this->_io->commit(events[0]) && this->_io->launch(events[0]))){
		// Записываем ошибку в лог запуска события
		awh::log::print("Cluster %s event could not be launched", awh::log::flag_t::CRITICAL, (service ? "control" : "worker"));
		/**
		 * Снимаем имя канала из окружения: канал уничтожается, и имя его лживо
		 *
		 * @details Сторож этот стоит УСТРОЙСТВОМ: кто переменную поставил, тот её и
		 *          снимает, и вызывающей стороне о ней помнить не надо
		 *
		 * @warning Прежде отказ этот уходил наружу, оставляя имя уничтоженного канала
		 *          висеть в окружении мастера, - и всякий последующий запуск приложения
		 *          получал его по наследству. То самое, от чего бережёт снятие имени
		 *          после порождения работника, только с другого конца
		 */
		::SetEnvironmentVariableW(variable, nullptr);
		// Уничтожаем событие родительского процесса
		this->_io->destroy(events[0]);
		// Выводим отсутствие заведённого события
		return 0;
	}
	/**
	 * Даём движку оборот прежде порождения процесса
	 *
	 * @details Фиксация и запуск события конец канала к ожиданию подключения ещё не
	 *          приводят: ожидание подаётся оборотом цикла, а до цикла хозяин доходит,
	 *          породив ВСЕ свои процессы. Работник же выходит на канал по имени сразу,
	 *          застаёт экземпляр незанятым, но и не ждущим подключения, и получает
	 *          `ERROR_PIPE_BUSY` - обмена в кластере не выходило вовсе
	 *
	 * @note Оборот берётся холостой, без ожидания: подать ожидание подключения ему
	 *       достаточно, а ждать здесь нечего - встречная сторона ещё даже не запущена
	 *
	 * @warning Разрыв подключения при фиксации тут не годится: конец канала после
	 *          закрытия встречного конца уже разорван - `DisconnectNamedPipe` отвечает
	 *          `ERROR_PIPE_NOT_CONNECTED` (233), - а обращение работника всё равно
	 *          отвергается, покуда сторона не ЖДЁТ подключения. Установлено щупом
	 */
	/**
	 * Дожидаемся, пока конец канала встанет в ожидание подключения
	 *
	 * @details Экземпляр канала, доставшийся от пары, остаётся за прежним подключением:
	 *          пара заводится связанной, встречный конец её открыт тут же, и закрытие
	 *          его экземпляра не освобождает. Система пускает работника лишь к стороне,
	 *          которая ЖДЁТ подключения, а ожидание подаётся согласованием подписок в
	 *          цикле опроса. Готовность спрашивается поэтому у самой системы:
	 *          `WaitNamedPipeW` с нулевым сроком отвечает истиной ровно тогда, когда
	 *          экземпляр готов принять подключение
	 *
	 * @warning Ожидание это не запас времени «на всякий случай», а условие работы:
	 *          прежде порождение шло сразу за запуском события, работник получал
	 *          `ERROR_PIPE_BUSY` и сдавался по сроку - обмена в кластере не выходило
	 *          вовсе, ни на x86-64, ни на ARM64. Причём под печатью щупа обмен ШЁЛ:
	 *          печать вносила задержку, и согласование успевало пройти
	 *
	 * @note Отвергнутое устройство (проверено прогоном): перезаведение стороны канала
	 *       при фиксации события. Подключались тогда ВСЕ работники, но обмен отвечал
	 *       отказом ввода-вывода - состояние движка, живущее на прежнем описателе, к
	 *       заведённому заново не переходит, и одного забывания прежнего мало
	 *
	 * @note Срок ограничен: не дождавшись, порождаем всё равно - у работника свой срок
	 */
	{
		// Имя канала в понимании системы
		const std::wstring channel = awh::fmk::convert(pipe);
		// Число оборотов ожидания готовности канала
		uint16_t rounds = 0;
		/**
		 * Выполняем обороты цикла, покуда канал не станет ждать подключения
		 */
		while((rounds++ < 200) && !::WaitNamedPipeW(channel.c_str(), 0))
			// Выполняем холостой оборот цикла событий
			this->_io->poll(0);
		/**
		 * Исчерпание оборотов ожидания обязано быть слышно
		 *
		 * @warning Прежде цикл этот исчерпывался МОЛЧА, и порождение шло дальше по
		 *          неготовому каналу: работник поднимался, подключиться не мог, а в
		 *          журнале не оставалось ничего. Отличить это от исправной работы
		 *          было нечем
		 *
		 * @note Порождение при этом не обрывается: канал вправе поспеть и позже,
		 *       а обрыв стоил бы работника там, где всё обошлось бы. Запись же
		 *       даёт разбирающему зацепку
		 *
		 */
		if(rounds > 200)
			// Записываем предупреждение в лог
			awh::log::print("Cluster %s pipe was not ready after %u rounds, the worker is spawned anyway", awh::log::flag_t::WARNING, (service ? "control" : "worker"), static_cast <uint32_t> (rounds - 1));
	}
	// Выводим идентификатор заведённого события
	return events[0];
}
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
				awh::log::print("Cluster job object limits could not be set", awh::log::flag_t::WARNING);
		// Если объект задания создать не удалось
		} else awh::log::print("Cluster job object could not be created, orphaned workers are possible", awh::log::flag_t::WARNING);
	}
	// Буфер под путь к образу приложения
	wchar_t image[MAX_PATH]{0};
	// Получаем длину пути к образу приложения
	const DWORD length = ::GetModuleFileNameW(nullptr, image, MAX_PATH);
	/**
	 * Путь обязан уместиться в буфер целиком
	 *
	 * @warning Проверялся прежде лишь нуль, а усечение НУЛЁМ НЕ ОТВЕЧАЕТ: обращение
	 *          отдаёт при нём размер буфера и оставляет путь обрезанным. Дальше
	 *          порождение шло по обрезанному пути и отказывало «файл не найден» - то
	 *          есть отказ называл следствие и о длине пути не намекал ничем
	 *
	 * @note Предел этот достижим: у MS Windows `MAX_PATH` равен 260, а образ набора
	 *       проверок живёт под каталогом сборки, вложенным на пять-шесть уровней
	 *
	 */
	if((length == 0) || (length >= MAX_PATH)){
		// Записываем ошибку в лог
		awh::log::print("Cluster could not determine its own executable path%s", awh::log::flag_t::CRITICAL, ((length >= MAX_PATH) ? ": the path is longer than MAX_PATH" : ""));
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
		awh::log::print("Cluster role marker could not be set in the environment", awh::log::flag_t::CRITICAL);
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
	/**
	 * Описатели мастера дочернему процессу НЕ наследуются
	 *
	 * @warning Наследование было включено, а нужды в нём нет ни одной: канал воркер
	 *          открывает ПО ИМЕНИ, роль узнаёт из окружения, а описатель мастера
	 *          открывает по его номеру. Меж тем сокеты у MS Windows наследуются по
	 *          умолчанию - заводятся они без `WSA_FLAG_NO_HANDLE_INHERIT`, - и
	 *          дочернему доставались ВСЕ сокеты мастера: и слушающий, и всякий
	 *          принятый. Мастер, закрывший свой сокет, порта не освобождал, а
	 *          закрытое им подключение не разрывалось: встречная сторона не получала
	 *          `FIN`, покуда жив хоть один воркер, державший тот же сокет
	 *
	 * @note Доказано щупом на стенде одним прогоном о двух ходах, разница одна:
	 *       с наследованием повторная привязка порта после закрытия сокета мастером
	 *       отвечает отказом 10048 (`WSAEADDRINUSE`), без наследования - удаётся
	 *
	 */
	const BOOL created = ::CreateProcessW(image, command.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startup, &info);
	// Снимаем метку роли из своего окружения, чтобы та не досталась мастеру
	::SetEnvironmentVariableW(L"AWH_CLUSTER_MASTER", nullptr);
	// Если процесс породить не удалось
	if(!created){
		// Записываем ошибку в лог
		awh::log::print("Child process could not be created", awh::log::flag_t::CRITICAL);
		// Возвращаем признак отсутствия порождённого процесса
		return 0;
	}
	// Если объект задания заведён — вносим в него порождённый процесс
	if((::__awh_job__ != nullptr) && !::AssignProcessToJobObject(::__awh_job__, info.hProcess))
		// Записываем ошибку в лог
		awh::log::print("Child process [%d] could not be assigned to the cluster job object", awh::log::flag_t::WARNING, static_cast <int32_t> (info.dwProcessId));
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
		/**
		 * Наблюдение за завершением обязано быть заведено
		 *
		 * @warning Прежде отказ лишь заносился в журнал, а процесс всё равно попадал в
		 *          список наблюдаемых с ПУСТЫМ описателем ожидания и возобновлялся.
		 *          Узнать о его гибели было тогда нечем: отклик `child` не звался
		 *          никогда, разбор завершения не шёл, замена упавшего работника не
		 *          выполнялась. Мастер продолжал считать работника живым, а кластер
		 *          молча редел
		 *
		 * @note Порождённый процесс здесь снимается принудительно: он остановлен
		 *       (`CREATE_SUSPENDED`) и работать ещё не начинал, а оставлять его без
		 *       надзора хуже, чем не порождать вовсе
		 *
		 */
		if(!::RegisterWaitForSingleObject(&child.wait, info.hProcess, &cluster_t::child, reinterpret_cast <PVOID> (static_cast <uintptr_t> (pid)), INFINITE, WT_EXECUTEONLYONCE)){
			// Записываем ошибку в лог
			awh::log::print("Child process [%d] termination watch could not be registered, the process is terminated", awh::log::flag_t::CRITICAL, pid);
			// Снимаем порождённый процесс, работать он ещё не начинал
			::TerminateProcess(info.hProcess, static_cast <UINT> (EXIT_FAILURE));
			// Закрываем дескриптор объекта процесса
			::CloseHandle(info.hProcess);
			// Закрываем дескриптор основного потока порождённого процесса
			::CloseHandle(info.hThread);
			// Возвращаем признак отсутствия порождённого процесса
			return 0;
		}
		// Выполняем блокировку замка доступа к спискам дочерних процессов
		const awh::locker_t <> lock(::__awh_children_mutex__);
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
		awh::log::print("Cluster role marker is malformed, the process is treated as master", awh::log::flag_t::CRITICAL);
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
	else awh::log::print("Cluster master process [%d] could not be opened, orphan detection is disabled", awh::log::flag_t::CRITICAL, pid);
	/**
	 * Открываем свой конец канала обмена сообщениями с мастером
	 *
	 * @warning Итог этот прежде ОТБРАСЫВАЛСЯ: работник, не открывший канала, оставался
	 *          живым и глухим, а запуск его тут же объявлялся удачным записью
	 *          «worker process has been started successfully». Мастер считал его
	 *          исправным и слал задания в никуда - ровно тот изъян, от какого
	 *          стережётся встречная сторона у spawn, отказываясь порождать работника
	 *          без имени канала
	 *
	 * @note Отказ здесь для работника смертелен: обмена с мастером у него нет, а
	 *       иного дела у работника не бывает. Возврат же признака мастера отсюда
	 *       НЕДОПУСТИМ - работник ушёл бы тогда в ветвь хозяина и принялся порождать
	 *       собственных работников
	 *
	 * @note Круговой перезапуск здесь не грозит: мастер видит гибель работника и
	 *       считает быстрые падения, а превысив предел, останавливает кластер целиком
	 *
	 */
	if(!this->attach()){
		// Записываем ошибку в лог
		awh::log::print("Cluster worker process [%d] has no messaging channel with the master, the worker is terminated", awh::log::flag_t::CRITICAL, ::getpid());
		// Завершаем работу работника: работать ему нечем
		::_exit(EXIT_FAILURE);
	}
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
		awh::log::print("Cluster worker pipe name is not set, messaging with the master is disabled", awh::log::flag_t::CRITICAL);
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
		awh::log::print("Cluster worker event could not be created", awh::log::flag_t::CRITICAL);
		// Сообщаем, что канал обмена сообщениями не открыт
		return false;
	}
	// Устанавливаем имя канала обмена сообщениями событию
	this->_io->setTarget(eid, awh::fmk::convert(wstring(buffer)));
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
	/**
	 * Открываем свой конец служебного канала
	 *
	 * @details Имя его приходит той же дорогой, что и имя пользовательского канала, -
	 *          окружением, - и снимается из окружения по тому же доводу: порождай
	 *          работник свои процессы, имя досталось бы им по наследству
	 *
	 * @warning Отказ здесь работника НЕ валит: без служебного канала он лишается связи
	 *          с соседями и приказов мастера, но свою пользовательскую работу ведёт
	 *          по-прежнему
	 */
	event::id_t cid = 0;
	{
		// Буфер под имя служебного канала
		wchar_t control[256]{0};
		// Получаем имя служебного канала из окружения
		const DWORD length = ::GetEnvironmentVariableW(L"AWH_CLUSTER_CONTROL", control, static_cast <DWORD> (sizeof(control) / sizeof(control[0])));
		// Если имя служебного канала в окружении есть
		if((length > 0) && (length < (sizeof(control) / sizeof(control[0])))){
			// Снимаем имя служебного канала из окружения
			::SetEnvironmentVariableW(L"AWH_CLUSTER_CONTROL", nullptr);
			// Заводим событие служебного канала
			cid = this->_io->event(event::node_t::IPC, event::family_t::PIPE, event::type_t::SEQPACKET);
			// Если событие служебного канала заведено
			if(cid != 0){
				// Устанавливаем имя служебного канала событию
				this->_io->setTarget(cid, awh::fmk::convert(wstring(control)));
				// Устанавливаем функцию обратного вызова на чтение служебного канала
				this->_io->on(cid, static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
			}
		// Если имени служебного канала в окружении нет
		} else awh::log::print("Cluster control channel name is not set, the worker is deaf to the master", awh::log::flag_t::CRITICAL);
	}
	// Создаём воркера для самого себя
	unique_ptr <worker_t> worker = make_unique <worker_t> ();
	// Устанавливаем время создания процесса
	worker->life = awh::fmk::timestamp <uint64_t> (awh::fmk::chrono_t::MILLISECONDS);
	// Устанавливаем идентификатор события обмена сообщениями
	worker->eid = eid;
	// Устанавливаем идентификатор служебного события
	worker->cid = cid;
	// Запоминаем свой служебный канал
	this->_control = cid;
	// Устанавливаем собственный идентификатор процесса
	worker->pid = static_cast <pid_t> (::getpid());
	// Добавляем воркера в список активных воркеров
	auto ret = this->_workers.emplace(static_cast <pid_t> (worker->pid), ::move(worker));
	// Добавляем соответствие идентификатора события идентификатору процесса
	this->_matching.emplace(eid, ret.first->first);
	/**
	 * Выполняем фиксацию, подключение и запуск работы события
	 *
	 * @warning Отказ здесь прежде уходил наружу БЕЗ отката: событие оставалось
	 *          заведённым, а записи о воркере и о соответствии - в списках. Работник
	 *          после того числил сам себя в живых воркерах, держал описатель канала до
	 *          конца работы и отвечал на channel по своему номеру процесса событием,
	 *          какое не запущено
	 *
	 * @note Порядок отката обратен заведению: сперва снимаются записи списков, затем
	 *       уничтожается само событие - соответствие ищется по его номеру
	 *
	 */
	if(!(this->_io->commit(eid) && this->_io->connect({eid}) && this->_io->launch(eid))){
		// Записываем ошибку в лог
		awh::log::print("Cluster worker event could not be launched", awh::log::flag_t::CRITICAL);
		// Удаляем соответствие идентификатора события идентификатору процесса
		this->_matching.erase(eid);
		// Удаляем воркера из списка активных воркеров
		this->_workers.erase(ret.first);
		// Уничтожаем событие обмена сообщениями с мастером
		this->_io->destroy(eid);
		// Если служебное событие заведено
		if(cid != 0){
			// Уничтожаем служебное событие
			this->_io->destroy(cid);
			// Обнуляем идентификатор служебного канала
			this->_control = 0;
		}
		// Сообщаем, что канал обмена сообщениями не открыт
		return false;
	}
	// Если служебное событие заведено
	if(cid != 0){
		// Выполняем фиксацию, подключение и запуск работы служебного события
		if(!(this->_io->commit(cid) && this->_io->connect({cid}) && this->_io->launch(cid))){
			// Записываем ошибку в лог
			awh::log::print("Cluster control channel could not be launched, the worker is deaf to the master", awh::log::flag_t::CRITICAL);
			// Уничтожаем служебное событие
			this->_io->destroy(cid);
			// Обнуляем идентификатор служебного канала
			this->_control = 0;
			// Обнуляем идентификатор служебного события воркера
			ret.first->second->cid = 0;
		/**
		 * Извещаем мастера о том, что работник поднялся
		 *
		 * @details У этой системы извещение - единственный способ узнать правду: мастер
		 *          знает лишь, что процесс породился, а дошёл ли тот до цикла событий и
		 *          открыл ли свои концы каналов, ему неизвестно вовсе
		 */
		} else this->dispatch(cid, control_t::ONLINE, ret.first->first);
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
			awh::log::print("Child process stopped, PID=%d, STATUS=%d", awh::log::flag_t::WARNING, pid, status);
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
			const bool rapid = ((awh::fmk::timestamp <uint64_t> (awh::fmk::chrono_t::MILLISECONDS) - i->second->life) <= this->_rebirth.window);
			// Освобождаем ресурсы завершившегося воркера
			this->release(i->second->eid);
			// Удаляем завершившийся процесс из списка активных воркеров
			this->_workers.erase(i);
			/**
			 * Определяем, ушёл ли работник намеренно
			 *
			 * @details Признак этот ставится приказом мастера либо извещением самого
			 *          работника об уходе, и снимается он здесь - разбором завершившегося
			 *
			 * @warning Ушедший намеренно работник возрождению НЕ подлежит, и счётчик
			 *          быстрых падений его уход не задевает: иначе мастер поднимал бы
			 *          работника, которому сам же велел уйти, а череда правильных уходов
			 *          останавливала бы кластер защитой от цикла перезапусков
			 */
			const bool leaving = (this->_leaving.erase(pid) > 0);
			// Снимаем выбывший узел с учёта поднявшихся
			this->_online.erase(pid);
			// Разрываем все связи выбывшего узла и извещаем его соседей
			this->dissolve(pid);
			// Извещаем оставшихся работников о выбытии узла
			this->announce(control_t::LEAVE, pid);
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const pid_t)> ("leave", pid);
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const pid_t, const int32_t)> ("exit", pid, status);
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const pid_t, const event_t)> ("events", pid, event_t::STOP);
			// Если разрешён автоматический перезапуск процесса и работник ушёл не по своей воле
			if(this->_rebirth.mode && !leaving){
				// Если процесс упал слишком быстро
				if(rapid)
					// Увеличиваем счётчик подряд идущих быстрых падений
					++this->_rebirth.restarts;
				// Если процесс прожил достаточно долго — сбрасываем счётчик быстрых падений
				else this->_rebirth.restarts = 0;
				// Если защита включена и число подряд идущих быстрых падений превысило порог — прекращаем перезапуск и останавливаем кластер
				if((this->_rebirth.limit > 0) && (this->_rebirth.restarts >= this->_rebirth.limit)){
					// Записываем в лог сообщение об обнаружении цикла перезапусков
					awh::log::print("Cluster [%s] worker keeps crashing on startup, aborting after %u rapid restarts", awh::log::flag_t::CRITICAL, this->_name.c_str(), this->_rebirth.restarts);
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
			// Снимаем признак намеренного ухода выбывшего узла
			this->_leaving.erase(pid);
			// Снимаем выбывший узел с учёта поднявшихся
			this->_online.erase(pid);
			// Разрываем все связи выбывшего узла и извещаем его соседей
			this->dissolve(pid);
			// Извещаем оставшихся работников о выбытии узла
			this->announce(control_t::LEAVE, pid);
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
	#if !defined(_WIN32) && !defined(_WIN64)
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
			const awh::locker_t <> lock(::__awh_children_mutex__);
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
			// Описатель снимаемого ожидания завершения процесса
			HANDLE wait = nullptr;
			// Дескриптор объекта завершившегося процесса
			HANDLE process = nullptr;
			{
				// Выполняем блокировку замка доступа к спискам дочерних процессов
				const awh::locker_t <> lock(::__awh_children_mutex__);
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
					// Снимаем описатель ожидания завершения процесса
					wait = i->second.wait;
					// Снимаем дескриптор объекта завершившегося процесса
					process = i->second.process;
					// Удаляем процесс из списка наблюдаемых
					::__awh_children__.erase(i);
				}
			}
			/**
			 * Ожидание снимается ВНЕ замка
			 *
			 * @warning Снятие с доводом `INVALID_HANDLE_VALUE` не возвращается, покуда не
			 *          отработают уже начатые извещения, а извещает система откликом
			 *          `child`, который берёт ЭТОТ ЖЕ замок. Снимай ожидание под замком -
			 *          и отклик, поднятый пулом потоков по другому процессу, встанет на
			 *          замке, а снятие встанет на отклике: заклинивание намертво. Тем
			 *          вероятнее оно, чем больше процессов кластера завершается разом
			 *
			 * @note Запись из списка к этому времени уже изъята, и описатели эти
			 *       принадлежат нам одним: тронуть их больше некому
			 *
			 */
			if(wait != nullptr)
				// Снимаем ожидание завершения процесса
				::UnregisterWaitEx(wait, INVALID_HANDLE_VALUE);
			// Если дескриптор объекта процесса получен
			if(process != nullptr)
				// Закрываем дескриптор объекта процесса
				::CloseHandle(process);
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
#if !defined(_WIN32) && !defined(_WIN64)
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
#if defined(_WIN32) || defined(_WIN64)
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
		const awh::locker_t <> lock(::__awh_children_mutex__);
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
 * @brief Метод отправки служебного сообщения кластера
 *
 * @param eid    идентификатор служебного события
 * @param type   вид служебного сообщения
 * @param pid    узел, которого сообщение касается
 * @param buffer тело служебного сообщения
 * @param size   размер тела служебного сообщения
 * @return       количество отправленных байт
 *
 */
size_t awh::unit::Cluster::dispatch(const event::id_t eid, const control_t type, const pid_t pid, const void * buffer, const size_t size) noexcept {
	// Если служебного события нет вовсе, отправлять некуда
	if(eid == 0)
		// Выводим отсутствие отправленных байт
		return 0;
	/**
	 * Если тело сообщения не вмещается в служебный канал
	 *
	 * @note Проверка эта - единственное место, где предел объявлен вызывающей стороне
	 *       честно: отказ записи в канал назвал бы ей чужую причину
	 */
	if(size > (::AWH_CLUSTER_CONTROL_LIMIT - sizeof(::envelope_t))){
		// Записываем ошибку в лог
		awh::log::print("Cluster control message of %zu bytes exceeds the limit of %zu bytes", awh::log::flag_t::WARNING, size, (::AWH_CLUSTER_CONTROL_LIMIT - sizeof(::envelope_t)));
		// Выводим отсутствие отправленных байт
		return 0;
	}
	// Создаём заголовок служебного сообщения
	::envelope_t envelope{};
	// Устанавливаем вид служебного сообщения
	envelope.type = static_cast <uint8_t> (type);
	// Устанавливаем узел, которого сообщение касается
	envelope.pid = static_cast <int32_t> (pid);
	// Создаём буфер служебного сообщения
	vector <uint8_t> message(sizeof(envelope) + size);
	// Копируем заголовок служебного сообщения в буфер
	::memcpy(message.data(), &envelope, sizeof(envelope));
	// Если тело служебного сообщения передано
	if((buffer != nullptr) && (size > 0))
		// Копируем тело служебного сообщения в буфер
		::memcpy(message.data() + sizeof(envelope), buffer, size);
	// Отправляем служебное сообщение целиком одним обращением
	return this->_io->send(eid, message.data(), message.size());
}
/**
 * @brief Метод извещения работников о входе либо выбытии узла
 *
 * @param type вид служебного сообщения
 * @param pid  узел, о котором идёт речь
 *
 */
void awh::unit::Cluster::announce(const control_t type, const pid_t pid) noexcept {
	/**
	 * Переходим по всему списку активных воркеров
	 */
	for(const auto & [node, worker] : this->_workers){
		/**
		 * Извещение уходит всем ПОДНЯВШИМСЯ, КРОМЕ названного узла
		 *
		 * @note Себе о своём входе работник вести счёт не обязан, а о своём выбытии
		 *       извещать его уже некуда
		 *
		 * @warning Не дошедшему до цикла узлу извещение слать НЕЛЬЗЯ: свой список
		 *          соседей он получит подъёмом, и посланное раньше пришло бы вторым
		 */
		if((node != pid) && (this->_online.find(node) != this->_online.end()))
			// Отправляем извещение работнику
			this->dispatch(worker->cid, type, pid);
	}
}
/**
 * @brief Метод заведения прямой связи между двумя работниками
 *
 * @param initiator узел, заказавший связь
 * @param peer      узел, с которым связь заказана
 * @param data      тело заказа: имя канала у MS Windows, пустое у систем POSIX
 * @param size      размер тела заказа
 *
 */
void awh::unit::Cluster::establish(const pid_t initiator, const pid_t peer, [[maybe_unused]] const uint8_t * data, [[maybe_unused]] const size_t size) noexcept {
	// Выполняем поиск заказавшего связь узла
	auto i = this->_workers.find(initiator);
	// Если заказавший связь узел кластеру не принадлежит, отвечать некому
	if(i == this->_workers.end())
		// Выходим из функции
		return;
	/**
	 * Функция отказа в заведении связи
	 *
	 * @note Причина уходит заказавшему связь узлу телом сообщения: молчаливый отказ
	 *       оставил бы его гадать, повторять ли заказ
	 */
	auto deny = [this, &i, peer](const reason_t reason) noexcept -> void {
		// Получаем причину отказа в заведении связи
		const uint8_t value = static_cast <uint8_t> (reason);
		// Отправляем отказ в заведении связи заказавшему её узлу
		this->dispatch(i->second->cid, control_t::DENY, peer, &value, sizeof(value));
	};
	// Если узел заказал связь с самим собой
	if(initiator == peer)
		// Отказываем в заведении связи
		return deny(reason_t::ITSELF);
	// Выполняем поиск узла, с которым связь заказана
	auto j = this->_workers.find(peer);
	// Если узла, с которым связь заказана, в кластере нет
	if(j == this->_workers.end())
		// Отказываем в заведении связи
		return deny(reason_t::UNKNOWN);
	{
		// Выполняем поиск связей заказавшего узла
		auto k = this->_links.find(initiator);
		// Если связь между этими узлами уже заведена
		if((k != this->_links.end()) && (k->second.find(peer) != k->second.end()))
			// Отказываем в заведении связи
			return deny(reason_t::EXISTS);
	}
	{
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("linking");
		// Если мастер связь запретил
		if(this->_callback.is(fid) && !this->_callback.call <bool (const pid_t, const pid_t)> (fid, initiator, peer))
			// Отказываем в заведении связи
			return deny(reason_t::REFUSED);
	}
	/**
	 * Для операционной системы MS Windows
	 *
	 * @details Пару заводит сам заказавший узел, а мастеру достаётся одно лишь имя
	 *          канала: передать описатель канала нельзя - перенос устройства события
	 *          ведётся там описанием сокета, а канал сокетом не является. Оттого
	 *          встреча идёт по имени, тем же путём, каким работник выходит на канал
	 *          мастера
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Если имени канала в заказе нет вовсе
		if((data == nullptr) || (size == 0))
			// Отказываем в заведении связи
			return deny(reason_t::FAILED);
		// Передаём имя канала узлу, с которым связь заказана
		if(this->dispatch(j->second->cid, control_t::GRANT, initiator, data, size) == 0)
			// Отказываем в заведении связи
			return deny(reason_t::FAILED);
		/**
		 * Извещаем заказавшего узла о заведённой связи пустым телом
		 *
		 * @note Свой конец пары у него уже есть - он же её и завёл, - и поднимать ему
		 *       нечего: сообщение это лишь подтверждает согласие мастера
		 */
		this->dispatch(i->second->cid, control_t::GRANT, peer);
	/**
	 * Для операционных систем, отличных от MS Windows
	 *
	 * @details Пару заводит мастер и раздаёт её концы обоим узлам снимками: событием-
	 *          переносчиком годен узел межпроцессного обмена, и служебный канал к
	 *          получателю им и является
	 */
	#else
		// Заводим пару обмена для прямой связи работников
		const auto & events = this->_io->events(event::family_t::UDS, this->_type);
		// Если пара обмена не заведена
		if((events[0] == 0) || (events[1] == 0)){
			// Уничтожаем заведённые концы пары
			if(events[0] != 0)
				// Уничтожаем первый конец пары
				this->_io->destroy(events[0]);
			// Если второй конец пары заведён
			if(events[1] != 0)
				// Уничтожаем второй конец пары
				this->_io->destroy(events[1]);
			// Отказываем в заведении связи
			return deny(reason_t::FAILED);
		}
		// Буфер снимка конца пары, предназначенного заказавшему связь узлу
		vector <uint8_t> snapshot;
		/**
		 * Снимаем снимок и отправляем его тем же обращением
		 *
		 * @warning Порядок здесь значим: снятие снимка лишь ОТМЕЧАЕТ передачу описателя
		 *          у переносчика, а уносит описатель ближайшее исходящее сообщение.
		 *          Снять оба снимка подряд и лишь затем отправить два сообщения нельзя -
		 *          метки разошлись бы с сообщениями
		 */
		if(!(this->_io->commit(events[0]) && this->_io->snapshot(events[0], i->second->cid, snapshot) &&
		 (this->dispatch(i->second->cid, control_t::GRANT, peer, snapshot.data(), snapshot.size()) > 0))){
			// Уничтожаем оба конца пары: раздать их не удалось
			this->_io->destroy(events[0]);
			// Уничтожаем второй конец пары
			this->_io->destroy(events[1]);
			// Отказываем в заведении связи
			return deny(reason_t::FAILED);
		}
		/**
		 * Отправляем второй конец пары узлу, с которым связь заказана
		 *
		 * @warning Отказ здесь оставляет заказавшего узла со своим концом на руках, и
		 *          известить его о разрыве обязаны мы: сам он о нём не узнает, покуда
		 *          не попробует что-нибудь отправить
		 */
		if(!(this->_io->commit(events[1]) && this->_io->snapshot(events[1], j->second->cid, snapshot) &&
		 (this->dispatch(j->second->cid, control_t::GRANT, initiator, snapshot.data(), snapshot.size()) > 0))){
			/**
			 * Извещаем заказавшего узла РАЗРЫВОМ, а не отказом
			 *
			 * @note Свой конец пары он уже получил первым снимком, и отказ сказал бы ему
			 *       неправду: связь завелась и тут же распалась, а не не завелась вовсе.
			 *       Разрыв же он разберёт как обычно - снесёт своё событие
			 */
			this->dispatch(i->second->cid, control_t::DROP, peer);
			// Уничтожаем оба конца пары
			this->_io->destroy(events[0]);
			// Уничтожаем второй конец пары
			this->_io->destroy(events[1]);
			// Записываем ошибку в лог
			awh::log::print("Cluster link between workers [%d] and [%d] has fallen apart on the second handover", awh::log::flag_t::CRITICAL, initiator, peer);
			// Выходим из функции
			return;
		}
		/**
		 * Уничтожаем свои узлы пары: описатели ушли работникам
		 *
		 * @note Держать их у себя значило бы перехватывать обмен, предназначенный не нам
		 */
		this->_io->destroy(events[0]);
		// Уничтожаем второй конец пары
		this->_io->destroy(events[1]);
	#endif
	// Запоминаем заведённую связь у заказавшего узла
	this->_links[initiator].emplace(peer);
	// Запоминаем заведённую связь у узла, с которым связь заказана
	this->_links[peer].emplace(initiator);
	// Записываем в лог сообщение о заведённой связи
	awh::log::print("Cluster workers [%d] and [%d] have been linked", awh::log::flag_t::INFO, initiator, peer);
}
/**
 * @brief Метод разрыва всех связей выбывающего узла
 *
 * @param pid выбывающий узел
 *
 */
void awh::unit::Cluster::dissolve(const pid_t pid) noexcept {
	// Выполняем поиск связей выбывающего узла
	auto i = this->_links.find(pid);
	// Если связей у выбывающего узла нет вовсе, разрывать нечего
	if(i == this->_links.end())
		// Выходим из функции
		return;
	/**
	 * Переходим по всем связям выбывающего узла
	 */
	for(const pid_t peer : i->second){
		// Выполняем поиск связей встречного узла
		auto j = this->_links.find(peer);
		// Если связи встречного узла найдены
		if(j != this->_links.end()){
			// Забываем связь с выбывающим узлом
			j->second.erase(pid);
			// Если связей у встречного узла больше не осталось
			if(j->second.empty())
				// Удаляем запись о связях встречного узла
				this->_links.erase(j);
		}
		// Выполняем поиск встречного узла среди активных воркеров
		auto k = this->_workers.find(peer);
		// Если встречный узел ещё жив
		if(k != this->_workers.end())
			// Извещаем встречный узел о разрыве связи
			this->dispatch(k->second->cid, control_t::DROP, pid);
	}
	// Удаляем запись о связях выбывающего узла
	this->_links.erase(i);
}
/**
 * @brief Метод разбора служебного сообщения кластера
 *
 * @param eid  идентификатор служебного события
 * @param data данные служебного сообщения
 * @param size размер служебного сообщения
 *
 */
void awh::unit::Cluster::control(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Если сообщение короче заголовка - разбирать нечего
	 *
	 * @note Границы сообщений держит сам канал, и усечённым сообщение прийти не может:
	 *       короткое сообщение означает чужую разметку, а не полученную наполовину
	 */
	if((data == nullptr) || (size < sizeof(::envelope_t)))
		// Выходим из функции
		return;
	// Заголовок разбираемого служебного сообщения
	::envelope_t envelope{};
	// Снимаем заголовок служебного сообщения
	::memcpy(&envelope, data, sizeof(envelope));
	// Получаем тело служебного сообщения
	const uint8_t * body = (data + sizeof(envelope));
	// Получаем размер тела служебного сообщения
	const size_t length = (size - sizeof(envelope));
	// Получаем узел, которого служебное сообщение касается
	const pid_t node = static_cast <pid_t> (envelope.pid);
	// Если процесс является родительским
	if(this->master()){
		// Выполняем поиск узла, приславшего служебное сообщение
		auto i = this->_controls.find(eid);
		// Если приславший сообщение узел кластеру не принадлежит
		if(i == this->_controls.end())
			// Выходим из функции
			return;
		// Получаем узел, приславший служебное сообщение
		const pid_t pid = i->second;
		/**
		 * Определяем вид служебного сообщения
		 */
		switch(envelope.type){
			// Если работник поднялся и дошёл до цикла событий
			case static_cast <uint8_t> (control_t::ONLINE): {
				// Выполняем поиск поднявшегося работника
				auto j = this->_workers.find(pid);
				// Если поднявшийся работник найден
				if(j != this->_workers.end()){
					/**
					 * Извещаем новичка обо всех уже поднятых узлах
					 *
					 * @note Своего способа узнать соседей у работника нет вовсе: он знает
					 *       свой номер и номер мастера, и список этот - единственный
					 *       путь к связи
					 */
					for(const pid_t other : this->_online){
						// Если узел новичком не является
						if(other != pid)
							// Извещаем новичка об уже поднятом узле
							this->dispatch(j->second->cid, control_t::JOIN, other);
					}
				}
				// Извещаем прочих работников о новичке
				this->announce(control_t::JOIN, pid);
				// Отмечаем новичка поднявшимся
				this->_online.emplace(pid);
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t)> ("join", pid);
			} break;
			/**
			 * Если работник уходит сам
			 *
			 * @warning Признак намеренного ухода обязан быть выставлен ДО того, как
			 *          система известит о завершении процесса: разбор завершившегося по
			 *          нему и решает, возрождать ли работника
			 */
			case static_cast <uint8_t> (control_t::OFFLINE): {
				// Помечаем узел уходящим намеренно
				this->_leaving.emplace(pid);
				// Записываем в лог сообщение об уходе работника
				awh::log::print("Cluster worker process [%d] is leaving on its own", awh::log::flag_t::INFO, pid);
			} break;
			// Если работник заказал прямую связь
			case static_cast <uint8_t> (control_t::LINK):
				// Выполняем заведение прямой связи между работниками
				this->establish(pid, node, body, length);
			break;
			// Если работник разрывает прямую связь
			case static_cast <uint8_t> (control_t::UNLINK): {
				// Выполняем поиск связей разрывающего узла
				auto j = this->_links.find(pid);
				// Если связь между этими узлами заведена
				if((j != this->_links.end()) && (j->second.erase(node) > 0)){
					// Если связей у разрывающего узла больше не осталось
					if(j->second.empty())
						// Удаляем запись о связях разрывающего узла
						this->_links.erase(j);
					// Выполняем поиск связей встречного узла
					auto k = this->_links.find(node);
					// Если связи встречного узла найдены
					if(k != this->_links.end()){
						// Забываем связь с разрывающим узлом
						k->second.erase(pid);
						// Если связей у встречного узла больше не осталось
						if(k->second.empty())
							// Удаляем запись о связях встречного узла
							this->_links.erase(k);
					}
					// Выполняем поиск встречного узла среди активных воркеров
					auto m = this->_workers.find(node);
					// Если встречный узел ещё жив
					if(m != this->_workers.end())
						// Извещаем встречный узел о разрыве связи
						this->dispatch(m->second->cid, control_t::DROP, pid);
				}
			} break;
			// Если работник пересылает сообщение соседу
			case static_cast <uint8_t> (control_t::RELAY): {
				// Выполняем поиск узла, которому предназначена пересылка
				auto j = this->_workers.find(node);
				// Если узел, которому предназначена пересылка, найден
				if(j != this->_workers.end())
					// Пересылаем сообщение названному узлу от имени отправителя
					this->dispatch(j->second->cid, control_t::RELAY, pid, body, length);
			} break;
		}
	// Если процесс является дочерним
	} else {
		/**
		 * Определяем вид служебного сообщения
		 */
		switch(envelope.type){
			// Если в кластер вошёл узел
			case static_cast <uint8_t> (control_t::JOIN): {
				/**
				 * Извещаем потребителя лишь о НОВОМ узле
				 *
				 * @note Повторное извещение об уже известном узле увело бы потребителя к
				 *       повторному заказу связи, а тот получил бы отказ `EXISTS`
				 */
				if(this->_nodes.emplace(node).second)
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t)> ("join", node);
			} break;
			// Если узел из кластера выбыл
			case static_cast <uint8_t> (control_t::LEAVE): {
				// Забываем выбывший узел
				this->_nodes.erase(node);
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t)> ("leave", node);
			} break;
			// Если связь заведена
			case static_cast <uint8_t> (control_t::GRANT): {
				// Идентификатор события заведённой связи
				event::id_t link = 0;
				/**
				 * Для операционной системы MS Windows
				 *
				 * @details Пустое тело означает согласие мастера на связь, которую узел
				 *          завёл сам: свой конец пары у него уже есть, и поднимать ему
				 *          нечего. Имя же канала означает связь, заведённую соседом, - к
				 *          ней выходят по имени
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Если тело сообщения пустое - связь завёл сам этот узел
					if(length == 0){
						// Выполняем поиск заведённой узлом связи
						auto i = this->_peers.find(node);
						// Если связь узлом заведена, о ней и извещаем
						if(i != this->_peers.end())
							// Запоминаем идентификатор события заведённой связи
							link = i->second;
					// Если в теле сообщения приехало имя канала
					} else {
						// Заводим событие своего конца канала связи
						link = this->_io->event(event::node_t::IPC, event::family_t::PIPE, this->_type);
						// Если событие связи заведено
						if(link != 0){
							// Устанавливаем имя канала связи событию
							this->_io->setTarget(link, string(reinterpret_cast <const char *> (body), length));
							// Устанавливаем функцию обратного вызова на событие чтения сообщений
							this->_io->on(link, static_cast <engine::callback::read_t> (std::bind(&cluster_t::incoming, this, _1, _2, _3)));
							// Устанавливаем функцию обратного вызова на событие изменения состояния
							this->_io->on(link, static_cast <engine::callback::status_t> (std::bind(&cluster_t::broken, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на закрытие связи
							this->_io->on(link, static_cast <engine::callback::event_t> (std::bind(&cluster_t::closed, this, _1, _2)));
							// Выполняем фиксацию, подключение и запуск работы события связи
							if(!(this->_io->commit(link) && this->_io->connect({link}) && this->_io->launch(link))){
								// Уничтожаем событие связи
								this->_io->destroy(link);
								// Сбрасываем идентификатор события связи
								link = 0;
							}
						}
					}
				/**
				 * Для операционных систем, отличных от MS Windows
				 *
				 * @details Конец пары приезжает описателем по служебному каналу, и узлу
				 *          остаётся поднять его у себя. Фиксация не зовётся: описатель
				 *          заведён чужим процессом, а подъём зовёт её сам
				 */
				#else
					// Заводим пустое событие под подъём приехавшего конца пары
					link = this->_io->event(event::node_t::IPC, event::family_t::UDS, this->_type);
					// Если событие связи заведено
					if(link != 0){
						// Поднимаем приехавший конец пары
						if(this->_io->restore(link, body, length)){
							// Устанавливаем функцию обратного вызова на событие чтения сообщений
							this->_io->on(link, static_cast <engine::callback::read_t> (std::bind(&cluster_t::incoming, this, _1, _2, _3)));
							// Устанавливаем функцию обратного вызова на событие изменения состояния
							this->_io->on(link, static_cast <engine::callback::status_t> (std::bind(&cluster_t::broken, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на закрытие связи
							this->_io->on(link, static_cast <engine::callback::event_t> (std::bind(&cluster_t::closed, this, _1, _2)));
							// Запускаем работу события связи
							if(!this->_io->launch(link)){
								// Уничтожаем событие связи
								this->_io->destroy(link);
								// Сбрасываем идентификатор события связи
								link = 0;
							}
						// Если поднять приехавший конец пары не удалось
						} else {
							// Уничтожаем событие связи
							this->_io->destroy(link);
							// Сбрасываем идентификатор события связи
							link = 0;
						}
					}
				#endif
				// Если событие связи заведено
				if(link != 0){
					// Запоминаем заведённую связь
					this->_peers.emplace(node, link);
					// Запоминаем соответствие события связи и узла
					this->_matchingPeers.emplace(link, node);
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const event::id_t)> ("linked", node, link);
				// Если событие связи завести не удалось
				} else {
					// Извещаем мастера о разрыве не заведённой связи
					this->dispatch(this->_control, control_t::UNLINK, node);
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const pid_t, const reason_t)> ("unlinked", node, reason_t::FAILED);
				}
			} break;
			// Если в заведении связи отказано
			case static_cast <uint8_t> (control_t::DENY): {
				// Причина отказа в заведении связи
				reason_t reason = reason_t::NONE;
				// Если причина отказа приехала телом сообщения
				if(length >= sizeof(uint8_t))
					// Снимаем причину отказа в заведении связи
					reason = static_cast <reason_t> (body[0]);
				/**
				 * Уничтожаем свой конец пары, заведённый под связь
				 *
				 * @note Заводит пару сам узел лишь у MS Windows; у систем POSIX её заводит
				 *       мастер, и уничтожать тут нечего
				 */
				auto i = this->_peers.find(node);
				// Если свой конец пары заведён
				if(i != this->_peers.end()){
					// Забываем соответствие события связи и узла
					this->_matchingPeers.erase(i->second);
					// Уничтожаем событие связи
					this->_io->destroy(i->second);
					// Забываем заведённую было связь
					this->_peers.erase(i);
				}
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const reason_t)> ("unlinked", node, reason);
			} break;
			// Если связь разорвана
			case static_cast <uint8_t> (control_t::DROP): {
				// Выполняем поиск разорванной связи
				auto i = this->_peers.find(node);
				// Если разорванная связь найдена
				if(i != this->_peers.end()){
					// Забываем соответствие события связи и узла
					this->_matchingPeers.erase(i->second);
					// Уничтожаем событие связи
					this->_io->destroy(i->second);
					// Забываем разорванную связь
					this->_peers.erase(i);
				}
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const reason_t)> ("unlinked", node, reason_t::NONE);
			} break;
			// Если пришла пересылка от соседа
			case static_cast <uint8_t> (control_t::RELAY):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("relay", node, body, length);
			break;
			// Если мастер велел завершить работу
			case static_cast <uint8_t> (control_t::SHUTDOWN): {
				// Код завершения работы по умолчанию
				int32_t code = cluster_t::SHUTDOWN_CODE;
				// Если код завершения работы приехал телом сообщения
				if(length >= sizeof(code))
					// Снимаем код завершения работы
					::memcpy(&code, body, sizeof(code));
				// Выполняем получение идентификатора функции обратного вызова
				const callback_t::id_t fid = this->_callback.id("shutdown");
				/**
				 * Если потребитель приказ перехватывает - уходить ему самому
				 *
				 * @note Довести своё дело до конца работник вправе: приказ этот не сигнал,
				 *       и мгновенного ухода не требует
				 */
				if(this->_callback.is(fid))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const int32_t)> (fid, code);
				// Если приказ перехватывать некому - уходим немедленно
				else this->leave(code);
			} break;
		}
	}
}
/**
 * @brief Метод обработки событий чтения по прямой связи с соседом
 *
 * @param eid  идентификатор события связи
 * @param data данные сообщения
 * @param size размер сообщения
 *
 */
void awh::unit::Cluster::incoming(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем поиск узла по идентификатору события связи
	auto i = this->_matchingPeers.find(eid);
	// Если узел найден
	if(i != this->_matchingPeers.end())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("peer", i->second, data, size);
}
/**
 * @brief Метод обработки состояния прямой связи с соседом
 *
 * @param eid    идентификатор события связи
 * @param status статус события
 *
 */
void awh::unit::Cluster::broken(const event::id_t eid, const event::status_t status) noexcept {
	// Если связь уничтожена
	if(status == event::status_t::DESTROYED)
		// Выполняем разбор разорванной связи
		this->sever(eid);
}
/**
 * @brief Метод обработки закрытия прямой связи с соседом
 *
 * @param eid    идентификатор события связи
 * @param action действие, случившееся с событием
 *
 */
void awh::unit::Cluster::closed(const event::id_t eid, const event::action_t action) noexcept {
	// Если связь закрыта
	if(action == event::action_t::CLOSE)
		// Выполняем разбор разорванной связи
		this->sever(eid);
}
/**
 * @brief Метод разбора разорванной прямой связи
 *
 * @param eid идентификатор события связи
 *
 */
void awh::unit::Cluster::sever(const event::id_t eid) noexcept {
	// Выполняем поиск узла по идентификатору события связи
	auto i = this->_matchingPeers.find(eid);
	/**
	 * Если узел найден
	 *
	 * @note Не найденный узел означает, что разрыв уже разобран другим признаком, а не
	 *       ошибку: признаков разрыва два, и приходят они оба
	 */
	if(i != this->_matchingPeers.end()){
		// Получаем узел, связь с которым разорвана
		const pid_t node = i->second;
		// Забываем соответствие события связи и узла
		this->_matchingPeers.erase(i);
		// Забываем разорванную связь
		this->_peers.erase(node);
		/**
		 * Событие связи здесь НЕ уничтожается намеренно
		 *
		 * @note Оба признака приходят от самого движка, изымающего узел: уничтожать его
		 *       из отклика значило бы сносить событие посреди его собственного разбора
		 */
		this->_callback.call <void (const pid_t, const reason_t)> ("unlinked", node, reason_t::NONE);
	}
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
			if(this->parent())
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const pid_t, const size_t)> (fid, this->_pid, size);
			// Если родительский процесс умер
			else {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, size}, awh::log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
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
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, data, size}, awh::log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
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
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (status)}, awh::log::flag_t::CRITICAL, ::getpid());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Процесс превратился в зомби, самоликвидируем его
						awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
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
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (status)}, awh::log::flag_t::CRITICAL, ::getpid());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Процесс превратился в зомби, самоликвидируем его
							awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
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
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (error), message}, awh::log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
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
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (status), size}, awh::log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
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
	#if defined(_WIN32) || defined(_WIN64)
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
	#if defined(_WIN32) || defined(_WIN64)
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
	#if defined(_WIN32) || defined(_WIN64)
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
	#if defined(_WIN32) || defined(_WIN64)
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
	#if defined(_WIN32) || defined(_WIN64)
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
	#if defined(_WIN32) || defined(_WIN64)
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			awh::log::debug("Only the master process can stop the cluster", __PRETTY_FUNCTION__, {}, awh::log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			awh::log::print("Only the master process can stop the cluster", awh::log::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод запуска кластера
 *
 */
void awh::unit::Cluster::start() noexcept {
	/**
	 * Если кластер отвергнут как второй в процессе, запускать его нельзя
	 *
	 * @note Отказ этот - единственное место, где о нём узнают: конструктор сообщить о
	 *       нём не может, а исключения в движке не применяются
	 */
	if(this->_rejected){
		// Записываем ошибку в лог
		awh::log::print("Cluster is rejected as the second one in this process and cannot be started", awh::log::flag_t::CRITICAL);
		// Выходим из функции
		return;
	}
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			awh::log::debug("Only the master process can start the cluster", __PRETTY_FUNCTION__, {}, awh::log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			awh::log::print("Only the master process can start the cluster", awh::log::flag_t::WARNING);
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
		/**
		 * Забываем учёт узлов целиком
		 *
		 * @note Извещать о выбытии здесь некого: работников не осталось ни одного, а
		 *       записи об их связях и подъёме без них не значат ничего
		 */
		{
			// Очищаем список поднявшихся узлов
			this->_online.clear();
			// Очищаем список уходящих намеренно узлов
			this->_leaving.clear();
			// Очищаем список связей между работниками
			this->_links.clear();
		}
	// Если процесс является дочерним то выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			awh::log::debug("Only the master process can clear the cluster", __PRETTY_FUNCTION__, {}, awh::log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			awh::log::print("Only the master process can clear the cluster", awh::log::flag_t::WARNING);
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
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				awh::log::debug("Only the master process can create child processes", __PRETTY_FUNCTION__, {}, awh::log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				awh::log::print("Only the master process can create child processes", awh::log::flag_t::WARNING);
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
				/**
				 * Снимаем узел с учёта и разрываем его связи
				 *
				 * @warning Разбор завершившегося процесса сюда НЕ придёт: воркера в списке
				 *          активных уже нет, и известие системы о его смерти пройдёт мимо.
				 *          Не разорви мы связи здесь - соседи держали бы события связи с
				 *          покойником до конца своей работы
				 */
				this->_leaving.erase(pid);
				// Снимаем узел с учёта поднявшихся
				this->_online.erase(pid);
				// Разрываем все связи снятого узла и извещаем его соседей
				this->dissolve(pid);
				// Извещаем оставшихся работников о выбытии узла
				this->announce(control_t::LEAVE, pid);
			}
		}
	// Если процесс является дочерним то выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			awh::log::debug("Only the master process can remove child processes", __PRETTY_FUNCTION__, {pid}, awh::log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			awh::log::print("Only the master process can remove child processes", awh::log::flag_t::WARNING);
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
	// Выполняем установку функции обратного вызова при входе узла в кластер
	this->_callback.set("join", callback);
	// Выполняем установку функции обратного вызова при выбытии узла из кластера
	this->_callback.set("leave", callback);
	// Выполняем установку функции обратного вызова при заведении прямой связи
	this->_callback.set("linked", callback);
	// Выполняем установку функции обратного вызова при разрыве прямой связи
	this->_callback.set("unlinked", callback);
	// Выполняем установку функции обратного вызова при запросе разрешения на связь
	this->_callback.set("linking", callback);
	// Выполняем установку функции обратного вызова при получении сообщения по прямой связи
	this->_callback.set("peer", callback);
	// Выполняем установку функции обратного вызова при получении пересылки через мастера
	this->_callback.set("relay", callback);
	// Выполняем установку функции обратного вызова при получении приказа завершить работу
	this->_callback.set("shutdown", callback);
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
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					awh::log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {buffer, size}, awh::log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					awh::log::print("Process [%d] has turned into a zombie, we perform self-destruction", awh::log::flag_t::CRITICAL, ::getpid());
				#endif
				// Выходим из приложения
				::_exit(EXIT_FAILURE);
			}
		// Если процесс является родительским
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				awh::log::debug("A message addressed to a parent process can only be sent from child processes", __PRETTY_FUNCTION__, {buffer, size}, awh::log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Процесс превратился в зомби, самоликвидируем его
				awh::log::print("A message addressed to a parent process can only be sent from child processes", awh::log::flag_t::WARNING);
			#endif
		}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод определения роли процесса до запуска кластера
 *
 * @details Отвечает на вопрос «запущен ли этот процесс работником» ещё до того, как
 *          кластер начал работу: порядок подготовки у мастера и у работника
 *          расходится, и знать роль нужно раньше запуска
 *
 * @return признак того, что процесс запущен работником кластера
 *
 */
bool awh::unit::Cluster::worker() const noexcept {
	/**
	 * Для операционной системы MS Windows
	 *
	 * @details Ветвления у этой системы нет: работник запускается заново тем же
	 *          образом и той же строкой доводов, и отличает его лишь метка роли,
	 *          выставленная мастером в окружении
	 *
	 * @note Метка здесь только спрашивается: снимает её перенятие роли при запуске
	 *       кластера, и снять её раньше значило бы отнять у него признак
	 */
	#if defined(_WIN32) || defined(_WIN64)
		/**
		 * Роль спрашивается двумя путями, и оба нужны
		 *
		 * @note До перенятия роли работника выдаёт метка в окружении: номер процесса
		 *       мастера ещё свой, и метод master ответил бы утвердительно. После
		 *       перенятия метки уже нет, зато номер процесса мастера чужой, и ответ
		 *       даёт сам master
		 */
		return ((::GetEnvironmentVariableW(L"AWH_CLUSTER_MASTER", nullptr, 0) != 0) || !this->master());
	/**
	 * Для операционных систем POSIX
	 *
	 * @note Работник у них получается ветвлением и роль свою знает с первого мига:
	 *       спрашивать окружение незачем
	 */
	#else
		// Выводим признак того, что процесс родительским не является
		return !this->master();
	#endif
}
/**
 * @brief Метод получения события обмена с процессом кластера
 *
 * @param pid идентификатор процесса кластера
 * @return    идентификатор события обмена с процессом кластера
 *
 */
awh::event::id_t awh::unit::Cluster::channel(const pid_t pid) const noexcept {
	// Выполняем поиск указанного процесса по идентификатору
	auto i = this->_workers.find(pid);
	// Если указанный процесс найден
	if(i != this->_workers.end())
		// Выводим идентификатор события обмена с процессом
		return i->second->eid;
	// Выводим отсутствие события обмена
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
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				awh::log::debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, {pid, buffer, size}, awh::log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Процесс превратился в зомби, самоликвидируем его
				awh::log::print("A message addressed to a child process can only be sent from the parent process", awh::log::flag_t::WARNING);
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
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				awh::log::debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, {buffer, size}, awh::log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Процесс превратился в зомби, самоликвидируем его
				awh::log::print("A message addressed to a child process can only be sent from the parent process", awh::log::flag_t::WARNING);
			#endif
		}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод заказа прямой связи с соседним работником
 *
 * @param pid узел, с которым заказывается связь
 * @return    признак того, что заказ отправлен мастеру
 *
 */
bool awh::unit::Cluster::link(const pid_t pid) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Записываем ошибку в лог
		awh::log::print("Only a worker process can order a direct link", awh::log::flag_t::WARNING);
		// Выводим отрицательный результат
		return false;
	}
	// Если служебный канал не заведён, заказывать связь нечем
	if(this->_control == 0){
		// Записываем ошибку в лог
		awh::log::print("Cluster control channel is not established, a direct link cannot be ordered", awh::log::flag_t::WARNING);
		// Выводим отрицательный результат
		return false;
	}
	// Если узел заказал связь с самим собой
	if(pid == static_cast <pid_t> (::getpid())){
		// Записываем ошибку в лог
		awh::log::print("Cluster worker process [%d] cannot link to itself", awh::log::flag_t::WARNING, pid);
		// Выводим отрицательный результат
		return false;
	}
	// Если связь с названным узлом уже заведена
	if(this->_peers.find(pid) != this->_peers.end()){
		// Записываем ошибку в лог
		awh::log::print("Cluster worker process [%d] is already linked", awh::log::flag_t::WARNING, pid);
		// Выводим отрицательный результат
		return false;
	}
	/**
	 * Для операционной системы MS Windows
	 *
	 * @details Пару заводит сам заказавший узел: описатель канала передать нельзя -
	 *          перенос устройства события ведётся описанием сокета, а канал сокетом не
	 *          является, - и встреча идёт по имени. Мастеру уходит одно лишь имя, а тот
	 *          доводит его до соседа, распорядившись правом на связь
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Заводим пару обмена для прямой связи
		const auto & events = this->_io->events(event::family_t::PIPE, this->_type);
		// Если пара обмена не заведена
		if((events[0] == 0) || (events[1] == 0)){
			// Записываем ошибку в лог
			awh::log::print("Cluster link channel could not be created", awh::log::flag_t::CRITICAL);
			// Если первый конец пары заведён
			if(events[0] != 0)
				// Уничтожаем первый конец пары
				this->_io->destroy(events[0]);
			// Если второй конец пары заведён
			if(events[1] != 0)
				// Уничтожаем второй конец пары
				this->_io->destroy(events[1]);
			// Выводим отрицательный результат
			return false;
		}
		/**
		 * Снимаем имя канала ПРЕЖДЕ уничтожения второго конца
		 *
		 * @note Уничтоженное событие имени уже не отдаст, а имя это - единственная точка
		 *       встречи для соседа
		 */
		const string & name = this->_io->getTarget(events[1]);
		// Если имя канала связи получить не удалось
		if(name.empty()){
			// Записываем ошибку в лог
			awh::log::print("Cluster link channel name could not be obtained", awh::log::flag_t::CRITICAL);
			// Уничтожаем первый конец пары
			this->_io->destroy(events[0]);
			// Уничтожаем второй конец пары
			this->_io->destroy(events[1]);
			// Выводим отрицательный результат
			return false;
		}
		// Уничтожаем второй конец пары: к нему сосед выйдет по имени
		this->_io->destroy(events[1]);
		// Устанавливаем функцию обратного вызова на событие чтения сообщений
		this->_io->on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::incoming, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие изменения состояния
		this->_io->on(events[0], static_cast <engine::callback::status_t> (std::bind(&cluster_t::broken, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на закрытие связи
		this->_io->on(events[0], static_cast <engine::callback::event_t> (std::bind(&cluster_t::closed, this, _1, _2)));
		/**
		 * Выполняем фиксацию и запуск своего конца канала связи
		 *
		 * @note Ожидания подключения тут не подаётся отдельно, в отличие от порождения
		 *       работника: сосед выйдет на канал не раньше, чем заказ обойдёт мастера и
		 *       дойдёт до него, - а это не один оборот цикла событий, и конец успеет
		 *       встать в ожидание сам
		 */
		if(!(this->_io->commit(events[0]) && this->_io->launch(events[0]))){
			// Записываем ошибку в лог
			awh::log::print("Cluster link channel could not be launched", awh::log::flag_t::CRITICAL);
			// Уничтожаем свой конец пары
			this->_io->destroy(events[0]);
			// Выводим отрицательный результат
			return false;
		}
		// Запоминаем заведённую связь
		this->_peers.emplace(pid, events[0]);
		// Запоминаем соответствие события связи и узла
		this->_matchingPeers.emplace(events[0], pid);
		// Отправляем мастеру заказ связи вместе с именем канала
		if(this->dispatch(this->_control, control_t::LINK, pid, name.c_str(), name.size()) > 0)
			// Выводим положительный результат
			return true;
		// Забываем соответствие события связи и узла
		this->_matchingPeers.erase(events[0]);
		// Забываем заведённую было связь
		this->_peers.erase(pid);
		// Уничтожаем свой конец пары: заказ до мастера не дошёл
		this->_io->destroy(events[0]);
		// Выводим отрицательный результат
		return false;
	/**
	 * Для операционных систем, отличных от MS Windows
	 *
	 * @details Пару заводит мастер и раздаёт её концы описателями: заказ несёт один
	 *          лишь номер узла
	 */
	#else
		// Отправляем мастеру заказ связи
		return (this->dispatch(this->_control, control_t::LINK, pid) > 0);
	#endif
}
/**
 * @brief Метод разрыва прямой связи с соседним работником
 *
 * @param pid узел, связь с которым разрывается
 * @return    признак того, что разрыв отправлен мастеру
 *
 */
bool awh::unit::Cluster::unlink(const pid_t pid) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Записываем ошибку в лог
		awh::log::print("Only a worker process can break a direct link", awh::log::flag_t::WARNING);
		// Выводим отрицательный результат
		return false;
	}
	// Выполняем поиск разрываемой связи
	auto i = this->_peers.find(pid);
	// Если связи с названным узлом нет вовсе
	if(i == this->_peers.end())
		// Выводим отрицательный результат
		return false;
	// Забываем соответствие события связи и узла
	this->_matchingPeers.erase(i->second);
	// Уничтожаем событие связи
	this->_io->destroy(i->second);
	// Забываем разорванную связь
	this->_peers.erase(i);
	// Извещаем мастера о разрыве связи
	return (this->dispatch(this->_control, control_t::UNLINK, pid) > 0);
}
/**
 * @brief Метод отправки сообщения соседу по прямой связи
 *
 * @param pid    узел, которому предназначено сообщение
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::Cluster::transmit(const pid_t pid, const void * buffer, const size_t size) noexcept {
	// Выполняем поиск связи с названным узлом
	auto i = this->_peers.find(pid);
	// Если связь с названным узлом заведена
	if(i != this->_peers.end())
		// Отправляем сообщение соседу напрямую
		return this->_io->send(i->second, buffer, size);
	/**
	 * Если включён режим отладки
	 */
	#if defined(DEBUG_MODE)
		// Записываем ошибку в лог
		awh::log::debug("Cluster worker process [%d] is not linked, a message cannot be transmitted", __PRETTY_FUNCTION__, {pid, buffer, size}, awh::log::flag_t::WARNING, pid);
	/**
	 * Если режим отладки не включён
	 */
	#else
		// Записываем ошибку в лог
		awh::log::print("Cluster worker process [%d] is not linked, a message cannot be transmitted", awh::log::flag_t::WARNING, pid);
	#endif
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод пересылки сообщения соседу через мастера
 *
 * @param pid    узел, которому предназначено сообщение
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::Cluster::relay(const pid_t pid, const void * buffer, const size_t size) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Записываем ошибку в лог
		awh::log::print("Only a worker process can relay a message through the master", awh::log::flag_t::WARNING);
		// Возвращаем значение по умолчанию
		return 0;
	}
	// Если служебный канал не заведён, пересылать нечем
	if(this->_control == 0){
		// Записываем ошибку в лог
		awh::log::print("Cluster control channel is not established, a message cannot be relayed", awh::log::flag_t::WARNING);
		// Возвращаем значение по умолчанию
		return 0;
	}
	// Пересылаем сообщение названному узлу через мастера
	if(this->dispatch(this->_control, control_t::RELAY, pid, buffer, size) > 0)
		// Выводим размер пересланного тела сообщения
		return size;
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения предельного размера пересылаемого сообщения
 *
 * @return предельный размер тела пересылаемого сообщения в байтах
 *
 */
size_t awh::unit::Cluster::maximumRelay() const noexcept {
	// Выводим предельный размер тела пересылаемого сообщения
	return (::AWH_CLUSTER_CONTROL_LIMIT - sizeof(::envelope_t));
}
/**
 * @brief Метод получения списка известных работнику узлов кластера
 *
 * @return список известных узлов кластера
 *
 */
unordered_set <pid_t> awh::unit::Cluster::nodes() const noexcept {
	// Выводим список известных узлов кластера
	return this->_nodes;
}
/**
 * @brief Метод приказа работнику завершить работу
 *
 * @param pid  узел, которому велено завершить работу
 * @param code код завершения работы
 * @return     признак того, что приказ отправлен работнику
 *
 */
bool awh::unit::Cluster::shutdown(const pid_t pid, const int32_t code) noexcept {
	// Если процесс является дочерним
	if(!this->master()){
		// Записываем ошибку в лог
		awh::log::print("Only the master process can order a worker to terminate", awh::log::flag_t::WARNING);
		// Выводим отрицательный результат
		return false;
	}
	// Выполняем поиск узла, которому велено завершить работу
	auto i = this->_workers.find(pid);
	// Если узла в кластере нет вовсе
	if(i == this->_workers.end())
		// Выводим отрицательный результат
		return false;
	/**
	 * Помечаем узел уходящим намеренно ПРЕЖДЕ отправки приказа
	 *
	 * @warning Порядок здесь значим: работник вправе уйти немедленно, и известие
	 *          системы о завершившемся процессе способно опередить возврат отсюда.
	 *          Признак, выставленный после, застал бы разбор завершившегося уже
	 *          прошедшим - и тот возродил бы ушедшего по приказу работника
	 */
	this->_leaving.emplace(pid);
	// Отправляем работнику приказ завершить работу
	if(this->dispatch(i->second->cid, control_t::SHUTDOWN, pid, &code, sizeof(code)) > 0)
		// Выводим положительный результат
		return true;
	// Снимаем признак намеренного ухода: приказ до работника не дошёл
	this->_leaving.erase(pid);
	// Выводим отрицательный результат
	return false;
}
/**
 * @brief Метод ухода работника из кластера по своей воле
 *
 * @param code код завершения работы
 *
 */
void awh::unit::Cluster::leave(const int32_t code) noexcept {
	// Если процесс является родительским
	if(this->master()){
		// Записываем ошибку в лог
		awh::log::print("Only a worker process can leave the cluster", awh::log::flag_t::WARNING);
		// Выходим из функции
		return;
	}
	/**
	 * Извещаем мастера об уходе прежде выхода
	 *
	 * @details Отправка отдаёт сообщение каналу тут же, покуда очередь его пуста, и
	 *          возврата отсюда довольно: отданное в буфер ядра переживает смерть
	 *          отправителя - приёмный конец принадлежит мастеру, и тот дочитает
	 *          известие уже после того, как работника не станет
	 *
	 * @note Извещение уходит даже тогда, когда служебного канала нет вовсе: выход в
	 *       таком случае идёт молча, и мастер узнает об уходе одним лишь завершением
	 *       процесса - возродив работника, если возрождение включено
	 */
	if(this->_control != 0)
		// Извещаем мастера о своём уходе
		this->dispatch(this->_control, control_t::OFFLINE, static_cast <pid_t> (::getpid()), &code, sizeof(code));
	// Записываем в лог сообщение об уходе работника
	awh::log::print("Cluster worker process [%d] is leaving with code %d", awh::log::flag_t::INFO, ::getpid(), code);
	// Завершаем работу процесса названным кодом
	::_exit(code);
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
 */
awh::unit::Cluster::Cluster() noexcept :
 unit_t(), _name{AWH_SHORT_NAME}, _rejected(false),
 _count(0), _wakeup(0), _type(event::type_t::SEQPACKET), _control(0) {
	/**
	 * Для операционной системы MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Обнуляем дескриптор объекта родительского процесса (захватывается дочерним процессом при запуске)
		this->_master = 0;
		/**
		 * Если кластер уже был создан ранее
		 *
		 * @warning Отвергнутый кластер объекта процесса НЕ перехватывает: перехват
		 *          увёл бы к нему дочерние процессы первого кластера
		 */
		if(::__awh_cluster__ != nullptr){
			// Помечаем кластер отвергнутым
			this->_rejected = true;
			// Записываем ошибку в лог
			awh::log::print("A cluster already exists in this process: the second one is rejected and will not be started", awh::log::flag_t::CRITICAL);
			// Выходим из конструктора
			return;
		}
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
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#else
		/**
		 * Если кластер уже был создан ранее
		 *
		 * @warning Отвергнутый кластер НЕ трогает ни объект процесса, ни перехватчик
		 *          сигнала SIGCHLD: перехват увёл бы к нему дочерние процессы первого
		 *          кластера, а снятие оставило бы первый без оповещений вовсе
		 */
		if(::__awh_cluster__ != nullptr){
			// Помечаем кластер отвергнутым
			this->_rejected = true;
			// Записываем ошибку в лог
			awh::log::print("A cluster already exists in this process: the second one is rejected and will not be started", awh::log::flag_t::CRITICAL);
		// Если кластер ещё не создан
		} else {
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
	#if defined(_WIN32) || defined(_WIN64)
		// Если дескриптор объекта родительского процесса был получен
		if(this->_master != 0){
			// Закрываем дескриптор объекта родительского процесса
			::CloseHandle(reinterpret_cast <HANDLE> (this->_master));
			// Обнуляем дескриптор объекта родительского процесса
			this->_master = 0;
		}
		// Если разрушаемый объект является текущим зарегистрированным кластером
		if(::__awh_cluster__ == this){
			// Список наблюдаемых процессов, снятый для устранения
			std::unordered_map <pid_t, Child> children;
			{
				// Выполняем блокировку замка доступа к спискам дочерних процессов
				const awh::locker_t <> lock(::__awh_children_mutex__);
				// Снимаем список наблюдаемых процессов целиком
				children.swap(::__awh_children__);
				// Очищаем очередь завершившихся процессов
				::__awh_finished__.clear();
			}
			/**
			 * Ожидания снимаются ВНЕ замка
			 *
			 * @warning Довод тот же, что и у разбора завершившегося процесса: снятие с
			 *          `INVALID_HANDLE_VALUE` не возвращается, покуда не отработают уже
			 *          начатые извещения, а отклик `child` берёт этот же замок. Здесь
			 *          заклинивание вероятнее всего: разрушение кластера снимает ожидания
			 *          РАЗОМ по всем процессам, и любой отклик, поднятый в это время,
			 *          останавливает всё
			 *
			 * @note Список к этому времени снят целиком и принадлежит нам одним
			 *
			 */
			for(auto & [pid, child] : children){
				// Снимаем ожидание завершения процесса, дождавшись начатых извещений
				if(child.wait != nullptr)
					// Снимаем ожидание завершения процесса
					::UnregisterWaitEx(child.wait, INVALID_HANDLE_VALUE);
				// Если дескриптор объекта процесса получен
				if(child.process != nullptr)
					// Закрываем дескриптор объекта процесса
					::CloseHandle(child.process);
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
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#else
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
