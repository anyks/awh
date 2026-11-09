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
#include <limits>
#include <thread>
#include <cerrno>
#include <cstdio>
#include <cwchar>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <algorithm>

/**
 * Подключаем заголовочный файл модуля разбора чисел
 *
 * @note Разбор ведётся им, а не средствами рамки языка: те судят запись по локали
 *       и кидают исключения, а метод, зовущий разбор, объявлен `noexcept`
 */
#include <num/lexical/lexical.hpp>

/**
 * Для операционной системы Microsoft Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем единую точку подключения системных заголовков Microsoft Windows
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
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/macro/lib.hpp>
#include <unit/cluster.hpp>

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
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * Для операционной системы Microsoft Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		/**
		 * @brief Функция подготовки отдельного окружения работника
		 *
		 * @details Окружение мастера не меняется. Сохраняются обычные переменные и
		 *          записи текущих каталогов дисков, а кластерные ключи заменяются
		 *          без учёта регистра только в копии для порождаемого процесса.
		 *
		 * @param pid     идентификатор мастера
		 * @param pipe    имя пользовательского канала
		 * @param control имя служебного канала
		 * @return        блок Unicode с двойным нулём в конце, либо пустой блок при отказе
		 *
		 */
		static vector <wchar_t> __awh_cluster_environment__(const pid_t pid, const string & pipe, const string & control) noexcept {
			// Результат работы функции
			vector <wchar_t> result;
			// Если имена каналов отсутствуют
			if(pipe.empty() || control.empty())
				// Выводим отсутствие подготовленного окружения
				return result;
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				// Получаем снимок окружения с автоматическим освобождением памяти Windows
				unique_ptr <wchar_t, decltype(&::FreeEnvironmentStringsW)> source(::GetEnvironmentStringsW(), &::FreeEnvironmentStringsW);
				// Если снимок окружения получить не удалось
				if(source == nullptr)
					// Выводим отсутствие подготовленного окружения
					return result;
				// Список переменных окружения работника
				vector <wstring> entries;
				/**
				 * Копируем переменные мастера, включая служебные записи дисков вида =C:
				 */
				for(const wchar_t * item = source.get(); *item != L'\0'; item += (::wcslen(item) + 1)){
					// Копируем текущую запись
					const wstring entry(item);
					// Определяем конец имени, пропуская начальный знак служебной записи
					const size_t length = entry.find(L'=', 1);
					// Если запись окружения повреждена
					if(length == wstring::npos)
						// Выводим отсутствие подготовленного окружения
						return result;
					// Если запись не является кластерным ключом
					if((::CompareStringOrdinal(item, static_cast <int> (length), L"AWH_CLUSTER_MASTER", -1, TRUE) != CSTR_EQUAL) &&
					   (::CompareStringOrdinal(item, static_cast <int> (length), L"AWH_CLUSTER_PIPE", -1, TRUE) != CSTR_EQUAL) &&
					   (::CompareStringOrdinal(item, static_cast <int> (length), L"AWH_CLUSTER_CONTROL", -1, TRUE) != CSTR_EQUAL))
						// Сохраняем исходную запись без изменения её значения
						entries.push_back(entry);
				}
				// Добавляем роль и идентификатор мастера
				entries.push_back(L"AWH_CLUSTER_MASTER=" + ::to_wstring(static_cast <uint32_t> (pid)));
				// Добавляем имя пользовательского канала
				entries.push_back(L"AWH_CLUSTER_PIPE=" + fmk::convert(pipe));
				// Добавляем имя служебного канала
				entries.push_back(L"AWH_CLUSTER_CONTROL=" + fmk::convert(control));
				// Сортируем имена без учёта регистра и локали по правилам Windows
				::sort(entries.begin(), entries.end(), [](const wstring & first, const wstring & second) noexcept -> bool {
					// Сравниваем только имена переменных, сохраняя начальный знак служебных записей
					return (::CompareStringOrdinal(first.c_str(), static_cast <int> (first.find(L'=', 1)), second.c_str(), static_cast <int> (second.find(L'=', 1)), TRUE) == CSTR_LESS_THAN);
				});
				/**
				 * Собираем последовательность строк, разделённых нулевыми символами
				 */
				for(const auto & entry : entries){
					// Переносим запись в блок окружения
					result.insert(result.end(), entry.begin(), entry.end());
					// Завершаем строку записи
					result.push_back(L'\0');
				}
				// Завершаем весь блок вторым нулевым символом
				result.push_back(L'\0');
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Удаляем незавершённый блок окружения
				result.clear();
				// Записываем ошибку в лог
				log::print("Cluster worker environment could not be prepared: %s", log::flag_t::CRITICAL, error.what());
			}
			// Выводим результат
			return result;
		}
	#endif
	/**
	 * @brief Состояние завершения воркера, остановленного мастером
	 *
	 * @details Мастер останавливает воркера закрытием своего конца канала, и тот
	 *          завершает работу сам. Наружу об этом извещает событие "exit",
	 *          принимающее состояние завершения в том виде, в каком его отдаёт система,
	 *          поэтому и значение это у каждой системы своё.
	 *
	 *          У POSIX берётся SIGSTOP: число это - правильно сложенное состояние
	 *          ожидания, читаемое разборными макросами как «снят сигналом SIGSTOP».
	 *          Подделывать тот же номер у Microsoft Windows нельзя - там на его месте стоит
	 *          код завершения, и число 17 или 19 прочиталось бы как обычный код
	 *          возврата приложения.
	 *
	 *          Взамен у Microsoft Windows берётся значение по правилам NTSTATUS: старшие
	 *          разряды несут признак важности «ошибка» вместе с разрядом, отведённым
	 *          значениям прикладным. Разряд этот затем и заведён - чтобы значения
	 *          приложений не путались с системными, и по нему же метод `crashed`
	 *          отличает падение от остановки.
	 *
	 */
	#if defined(_WIN32) || defined(_WIN64)
		/**
		 * @brief Устанавливаем значение состояния завершения воркера, остановленного мастером
		 *
		 */
		constexpr int32_t __AWH_CLUSTER_STOPPED__ = static_cast <int32_t> (0xE0000001u);
	/**
	 * @brief Если операционная система принадлежит к Unix-подобной (POSIX)
	 *
	 */
	#else
		/**
		 * @brief Устанавливаем значение состояния завершения воркера, остановленного мастером
		 *
		 */
		constexpr int32_t __AWH_CLUSTER_STOPPED__ = SIGSTOP;
	#endif
};

/**
 * @brief Инкапсулируем разметку служебного канала кластера в пространство имён
 *
 */
namespace {
	/**
	 * @brief Упаковка структуры заголовка служебного сообщения кластера
	 *
	 */
	__AWH_PACK_BEGIN__
	/**
	 * @brief Структура заголовка служебного сообщения кластера
	 *
	 * @details Длины в заголовке нет намеренно: служебный канал заводится SEQPACKET
	 *          безусловно, границы сообщений держит он сам, и сообщение приходит
	 *          целым. Оттого и разбирать здесь нечего - заголовок снимается одним
	 *          копированием.
	 *
	 */
	typedef struct Envelope {
		// Вид служебного сообщения (значение cluster_t::control_t)
		uint8_t type = 0;
		// Узел, которого сообщение касается (0 - никого)
		int32_t pid = 0;
	} __AWH_PACKED__ envelope_t;
	__AWH_PACK_END__

	/**
	 * @brief Предельный размер служебного сообщения кластера
	 *
	 * @details Предел объявлен и проверяется НА ОТПРАВКЕ. Обнаруживать его отказом
	 *          записи в канал нельзя: работник узнал бы о нём случайно и с чужой
	 *          причиной - сообщение, не вместившееся в дейтаграмму, система отвергает
	 *          так же, как и всякое иное.
	 *
	 * @note Число взято с запасом вниз от предела дейтаграммы домена UNIX у самой
	 *       скупой из поддерживаемых систем: пересылка заведена под короткие
	 *       сообщения, и упираться в него ей не предназначено
	 *
	 */
	constexpr size_t __AWH_CLUSTER_CONTROL_LIMIT__ = 8192;
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
	 *          `TerminateProcess` у Microsoft Windows. Ни то, ни другое процесс перехватить
	 *          не может, и ни то, ни другое не даёт ему довести работу до конца.
	 *
	 * @note Дескриптор процесса у Microsoft Windows приходится открывать заново по номеру:
	 *       список активных воркеров хранит именно номера процессов. Номер система
	 *       переиспользует, поэтому вызывать функцию допустимо лишь для процессов,
	 *       которые кластер считает живыми
	 *
	 */
	void __awh_terminate__([[maybe_unused]] const pid_t pid) noexcept {
		/**
		 * Для операционной системы Microsoft Windows
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
 * Для операционной системы не являющейся Microsoft Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * @brief Инкапсулируем статические типы данных в пространство имён
	 *
	 */
	namespace {
		/**
		 * Используем пространство имён AWH
		 */
		using namespace awh;

		/**
		 * @brief Объект перехвата сигнала
		 *
		 */
		struct sigaction __awh_action__{};
		/**
		 * @brief Объект кластера для работы статических методов
		 *
		 */
		static unit::cluster_t * __awh_cluster__ = nullptr;
	};
#endif

/**
 * Для операционной системы Microsoft Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * @brief Инкапсулируем состояние управления процессами в пространство имён
	 *
	 * @details Ничего этого нет у POSIX: там дочерний процесс достаётся вызовом fork,
	 *          о завершении его извещает сигнал SIGCHLD, а пожинает его waitpid. У
	 *          Microsoft Windows каждое из трёх заменяется своим средством, и всем трём нужно
	 *          где-то держать состояние.
	 *
	 */
	namespace {
		/**
		 * Используем пространство имён AWH
		 */
		using namespace awh;

		/**
		 * @brief Дочерний процесс кластера
		 *
		 */
		typedef struct Child {
			// Дескриптор ожидания завершения процесса из системного пула потоков
			HANDLE wait;
			// Дескриптор объекта процесса
			HANDLE process;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Child() noexcept :
			 wait(nullptr), process(nullptr) {}
		} child_t;

		/**
		 * @brief Очередь завершившихся процессов, ожидающих разбора в петле событий
		 *
		 */
		static deque <pid_t> __awh_finished__;
		/**
		 * @brief Список дочерних процессов, за которыми ведётся наблюдение
		 *
		 */
		static unordered_map <pid_t, child_t> __awh_children__;

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
		static lock_state_t <> __awh_mtx__;
		/**
		 * @brief Объект задания, удерживающий дочерние процессы
		 *
		 * @details Задание заводится с пределом JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE:
		 *          закрытие последнего дескриптора задания снимает все входящие в него
		 *          процессы. Так замещается проверка на осиротевание, какую у POSIX даёт
		 *          сравнение с getppid: погибни мастер любым образом, включая падение,
		 *          система снимет воркеров сама, и осиротевших процессов не остаётся.
		 *
		 */
		static HANDLE __awh_job__ = nullptr;
		/**
		 * @brief Объект кластера для работы статических функций
		 *
		 */
		static unit::cluster_t * __awh_cluster__ = nullptr;
	};
#endif

/**
 * Для операционных систем, отличных от Microsoft Windows
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
			 * Используем пространство имён AWH
			 */
			using namespace awh;

			/**
			 * @brief Функция выполнения внешней команды и получения её стандартного вывода
			 *
			 * @param cmd команда для выполнения
			 * @return    стандартный вывод выполненной команды
			 *
			 */
			static __AWH_INLINE__ string __awh_exec_command__(const string & cmd) noexcept {
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
			static string __awh_resolve_line__(void * pc) noexcept {
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
			static void childCrashHandler(const int32_t sig) noexcept {
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
		};
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
	 * Для операционной системы Microsoft Windows
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
		log::print("Cluster [%s] has been started successfully", log::flag_t::INFO, this->_name.c_str());
		/**
		 * Для операционных систем, отличных от Microsoft Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			/**
			 * Переходим по всему списку активных воркеров
			 *
			 * @note На Microsoft Windows события уже зафиксированы и запущены - прежде порождения
			 *       процессов: работник выходит на канал по имени сразу, и конец мастера
			 *       обязан ждать подключения к тому времени
			 */
			for(auto & [pid, worker] : this->_workers){
				// Если служебный канал заведён, фиксируем и запускаем его
				if((worker->cid != 0) && !(engine::io::commit(worker->cid) && engine::io::launch(worker->cid)))
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог запуска события
						log::debug("Cluster control channel of the worker process [%d] could not be launched", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, pid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("Cluster control channel of the worker process [%d] could not be launched", log::flag_t::CRITICAL, pid);
					#endif
				// Выполняем фиксацию и запуск работы события
				if(!(engine::io::commit(worker->eid) && engine::io::launch(worker->eid))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог запуска события
						log::debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, pid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог запуска события
						log::print("Cluster worker process [%d] event could not be launched", log::flag_t::CRITICAL, pid);
					#endif
					// Выходим из приложения
					::_exit(EXIT_FAILURE);
				}
			}
		#endif
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const event_t)> ("events", this->_pid, event_t::START);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
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
		 * @brief Создаём новый дочерний процесс с немедленным запуском события взамен завершившегося
		 *
		 * @details Заслона по системам здесь нет намеренно: порождение работника разведено по
		 *          системам внутри самого spawn - у POSIX это fork, у Microsoft Windows повторный запуск
		 *          себя, - а размещение взамен выбывшего у них общее.
		 *
		 * @note Прежде тело это было заключено в заслон «кроме MS Windows», отчего
		 *       перезапуск упавшего работника там не делал ровно ничего: кластер
		 *       распознавал падение, рассылал события завершения и остановки, а нового
		 *       работника взамен не поднимал. Найдено пробой на стенде
		 *
		 */
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
			log::debug("%s", __PRETTY_FUNCTION__, {pid}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
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
				engine::io::on(j->second->cid, static_cast <engine::callback::read_t> (nullptr));
				// Удаляем соответствие идентификатора служебного события и идентификатора процесса
				this->_controls.erase(j->second->cid);
				// Уничтожаем служебное событие
				engine::io::destroy(j->second->cid);
				// Обнуляем идентификатор служебного события
				j->second->cid = 0;
			}
		}
	}
	/**
	 * @brief Сбрасываем функцию обратного вызова на изменение статуса, чтобы не реагировать на DESTROYED при ручном закрытии события
	 *
	 * @details Заслона по системам здесь нет намеренно: все три действия существуют у обеих
	 *          систем, а событие обмена сообщениями заводится и там, и там.
	 *
	 * @note Прежде тело это было заключено в заслон «кроме MS Windows», отчего события
	 *       выбывших работников там не уничтожались вовсе - ни при их падении, ни при
	 *       снятии мастером. Соответствие идентификаторов росло без конца
	 *
	 */
	engine::io::on(eid, static_cast <engine::callback::status_t> (nullptr));
	// Удаляем соответствие идентификатора события и идентификатора процесса
	this->_matching.erase(eid);
	// Уничтожаем событие (закрывает сокет, что уведомляет дочерний процесс о завершении работы)
	engine::io::destroy(eid);
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
			 * Для операционной системы Microsoft Windows
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
					log::print("Cluster worker process [%d] has been started successfully", log::flag_t::INFO, ::getpid());
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
			this->_wakeup = engine::io::event(event::node_t::NOTIFY, event::family_t::USER);
			// Если событие пробуждения создано и его настройки зафиксированы
			if((this->_wakeup != 0) && engine::io::commit(this->_wakeup)){
				// Устанавливаем функцию обратного вызова на чтение для отложенной обработки завершившихся процессов
				engine::io::on(this->_wakeup, static_cast <engine::callback::read_t> (std::bind(&cluster_t::reap, this, _1, _2, _3)));
				// Запускаем работу события пробуждения
				if(!engine::io::launch(this->_wakeup)){
					// Уничтожаем событие пробуждения
					engine::io::destroy(this->_wakeup);
					// Обнуляем идентификатор события пробуждения
					this->_wakeup = 0;
				}
			// Если событие пробуждения создать не удалось
			} else if(this->_wakeup != 0) {
				// Уничтожаем событие пробуждения
				engine::io::destroy(this->_wakeup);
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
				log::print("Cluster [%s] has been started successfully", log::flag_t::INFO, this->_name.c_str());
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
				engine::io::destroy(this->_wakeup);
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
 * @brief Метод создания одного дочернего процесса (воркера)
 *
 * @param replaced идентификатор замещаемого (упавшего) процесса, либо 0 при первичном создании
 * @param deferred флаг отложенного запуска события (true — фиксация/запуск выполняются позже пакетно)
 * @return         семейство процесса: MASTER — родитель, CHILDREN — дочерний, NONE — ошибка создания
 *
 */
awh::unit::cluster_t::family_t awh::unit::Cluster::spawn([[maybe_unused]] const pid_t replaced, [[maybe_unused]] const bool deferred) noexcept {
	/**
	 * Для операционных систем, отличных от Microsoft Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Создаём новый вокрер дочернего процесса
		unique_ptr <worker_t> worker = make_unique <worker_t> ();
		// Устанавливаем время создания процесса
		worker->life = fmk::milliseconds();
		// Добавляем новые события для обмена сообщениями между процессами
		const auto & events = engine::io::events(event::family_t::UDS, this->_type);
		// Если события не созданы
		if((events[0] == 0) || (events[1] == 0)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Child process worker could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Child process worker could not be created", log::flag_t::CRITICAL);
			#endif
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		/**
		 * @brief Заводим служебный канал обмена, отдельный от пользовательского
		 *
		 * @details Канал этот заводится SEQPACKET БЕЗУСЛОВНО, каким бы ни был
		 *          пользовательский тип обмена: границы сообщений держит он сам, и
		 *          служебное сообщение приходит целым. Пойди служебный обмен по
		 *          пользовательскому каналу - у потокового типа границ не стало бы вовсе,
		 *          и разметка обернулась бы своим протоколом передачи внутри кластера,
		 *          платой на каждом пользовательском сообщении. Разбор - в CLUSTER-LINK.md.
		 *
		 */
		const auto & controls = engine::io::events(event::family_t::UDS, event::type_t::SEQPACKET);
		// Если служебный канал не заведён
		if((controls[0] == 0) || (controls[1] == 0)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster control channel could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster control channel could not be created", log::flag_t::CRITICAL);
			#endif
			// Если первый конец служебного канала заведён
			if(controls[0] != 0)
				// Уничтожаем первый конец служебного канала
				engine::io::destroy(controls[0]);
			// Если второй конец служебного канала заведён
			if(controls[1] != 0)
				// Уничтожаем второй конец служебного канала
				engine::io::destroy(controls[1]);
			// Уничтожаем оба конца пользовательского канала
			engine::io::destroy(events[0]);
			// Уничтожаем второй конец пользовательского канала
			engine::io::destroy(events[1]);
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
					log::debug("Child process could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Child process could not be created", log::flag_t::CRITICAL);
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
							engine::io::destroy(i->second->eid);
							// Если служебный канал другого дочернего процесса заведён
							if(i->second->cid != 0)
								// Уничтожаем служебное событие другого дочернего процесса
								engine::io::destroy(i->second->cid);
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
						engine::io::destroy(this->_wakeup);
						// Обнуляем идентификатор события пробуждения
						this->_wakeup = 0;
					}
					// Уничтожаем событие родительского процесса
					engine::io::destroy(events[0]);
					// Уничтожаем служебное событие родительского процесса
					engine::io::destroy(controls[0]);
					// Выполняем переинициализацию асинхронного движка ввода-вывода
					engine::io::reinitialize();
					// Устанавливаем опции события
					if(!engine::options::set(events[1], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("Error setting cluster worker event options", log::flag_t::WARNING);
						#endif
					}
					// Устанавливаем функцию обратного вызова на событие записи сообщений
					engine::io::on(events[1], static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
					// Устанавливаем функцию обратного вызова на событие изменения состояния
					engine::io::on(events[1], static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
					// Устанавливаем функцию обратного вызова на событие чтения сообщений
					engine::io::on(events[1], static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					engine::io::on(events[1], static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
					engine::io::on(events[1], static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
					/**
					 * @brief Заводим свой конец служебного канала
					 *
					 * @note Отклики его - одно лишь чтение: пользовательские отклики
					 *       служебного канала не касаются вовсе, и разметка его наружу не выходит
					 *
					 */
					engine::io::on(controls[1], static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
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
					if(engine::io::commit(ret.first->second->eid) && engine::io::launch(ret.first->second->eid)){
						/**
						 * @brief Заводим служебный канал вслед за пользовательским
						 *
						 * @warning Отказ здесь работника НЕ валит: без служебного канала он
						 *          лишается связи с соседями и приказов мастера, но свою
						 *          пользовательскую работу ведёт по-прежнему. Валить его
						 *          отказом второстепенного канала значило бы менять
						 *          поведение кластера там, где связи никто и не просил
						 *
						 */
						if(!(engine::io::commit(this->_control) && engine::io::launch(this->_control))){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("Cluster control channel could not be launched, the worker is deaf to the master", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("Cluster control channel could not be launched, the worker is deaf to the master", log::flag_t::CRITICAL);
							#endif
							// Уничтожаем свой конец служебного канала
							engine::io::destroy(this->_control);
							// Обнуляем идентификатор служебного канала
							this->_control = 0;
							// Обнуляем идентификатор служебного события воркера
							ret.first->second->cid = 0;
						/**
						 * @brief Извещаем мастера о том, что работник поднялся
						 *
						 * @details Извещение это - не вежливость: у Microsoft Windows оно
						 *          единственный способ узнать правду. Мастер знает лишь,
						 *          что процесс породился, а дошёл ли тот до цикла событий,
						 *          ему неизвестно вовсе.
						 *
						 */
						} else this->dispatch(this->_control, control_t::ONLINE, ret.first->first);
						// Записываем в лог сообщение об успешном запуске события
						log::print("Cluster worker process [%d] has been started successfully", log::flag_t::INFO, ret.first->first);
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const pid_t, const event_t)> ("events", ret.first->first, event_t::START);
					// Если событие не может быть запущено
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL, ret.first->first);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("Cluster worker process [%d] event could not be launched", log::flag_t::CRITICAL, ret.first->first);
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
						log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL, ::getpid());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
				engine::io::destroy(events[1]);
				// Уничтожаем служебное событие дочернего процесса
				engine::io::destroy(controls[1]);
				// Устанавливаем функцию обратного вызова на чтение служебного канала
				engine::io::on(controls[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
				// Устанавливаем опции события
				if(!engine::options::set(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						log::debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("Error setting cluster worker event options", log::flag_t::WARNING);
					#endif
				}
				// Устанавливаем функцию обратного вызова на событие записи сообщений
				engine::io::on(events[0], static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
				// Устанавливаем функцию обратного вызова на событие изменения состояния
				engine::io::on(events[0], static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
				// Устанавливаем функцию обратного вызова на событие чтения сообщений
				engine::io::on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
				// Устанавливаем функцию обратного вызова на событие получения ошибок
				engine::io::on(events[0], static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
				// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
				engine::io::on(events[0], static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
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
					if(!(engine::io::commit(ret.first->second->cid) && engine::io::launch(ret.first->second->cid)))
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("Cluster control channel of the worker process [%d] could not be launched", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL, ret.first->first);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("Cluster control channel of the worker process [%d] could not be launched", log::flag_t::CRITICAL, ret.first->first);
						#endif
					// Выполняем фиксацию и запуск работы события
					if(!(engine::io::commit(ret.first->second->eid) && engine::io::launch(ret.first->second->eid))){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("Cluster worker process [%d] event could not be launched", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL, replaced);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("Cluster worker process [%d] event could not be launched", log::flag_t::CRITICAL, replaced);
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
	 * Если операционной системой является Microsoft Windows
	 */
	#else
		// Создаём новый вокрер дочернего процесса
		unique_ptr <worker_t> worker = make_unique <worker_t> ();
		// Устанавливаем время создания процесса
		worker->life = fmk::milliseconds();
		// Заводим свой конец пользовательского канала обмена сообщениями
		const event::id_t eid = this->provision(this->_type, false);
		// Если пользовательский канал обмена сообщениями не заведён
		if(eid == 0){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster control channel could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster control channel could not be created", log::flag_t::CRITICAL);
			#endif
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		/**
		 * @brief Заводим свой конец служебного канала обмена
		 *
		 * @details Канал этот заводится SEQPACKET БЕЗУСЛОВНО, каким бы ни был
		 *          пользовательский тип обмена: границы сообщений держит он сам, и
		 *          служебное сообщение приходит целым. Пойди служебный обмен по
		 *          пользовательскому каналу - у потокового типа границ не стало бы вовсе,
		 *          и разметка обернулась бы своим протоколом передачи внутри кластера,
		 *          платой на каждом пользовательском сообщении. Разбор - в CLUSTER-LINK.md.
		 *
		 */
		const event::id_t cid = this->provision(event::type_t::SEQPACKET, true);
		// Если служебный канал обмена не заведён
		if(cid == 0){
			// Уничтожаем событие пользовательского канала
			engine::io::destroy(eid);
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster control channel could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster control channel could not be created", log::flag_t::CRITICAL);
			#endif
			// Возвращаем результат отсутствия созданного воркера
			return family_t::NONE;
		}
		// Выполняем порождение дочернего процесса
		const pid_t pid = this->execute(eid, cid);
		// Если порождение процесса не удалось
		if(pid == 0){
			// Уничтожаем событие родительского процесса
			engine::io::destroy(eid);
			// Уничтожаем служебное событие родительского процесса
			engine::io::destroy(cid);
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster control channel could not be created", __PRETTY_FUNCTION__, {replaced, deferred}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster control channel could not be created", log::flag_t::CRITICAL);
			#endif
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
		 * @brief Если запуск события не отложен (одиночное размещение воркера во время работы кластера)
		 *
		 * @details Фиксация и запуск события здесь уже произошли - прежде порождения процесса,
		 *          и признак отложенного запуска на Microsoft Windows означает лишь то, что оповещать
		 *          о замене упавшего процесса рано.
		 *
		 */ 
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
 * Для операционной системы Microsoft Windows
 *
 * @note Метода этого на системах POSIX нет вовсе: дочерний процесс достаётся там
 *       вызовом fork, продолжающим работу с того же места и с тем же состоянием.
 *       У Microsoft Windows соответствия fork нет, и дочерний процесс приходится
 *       запускать заново - собственным образом приложения
 *
 */
#if defined(_WIN32) || defined(_WIN64)
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
		 * @brief Выполняем разбор метки роли числом
		 *
		 * @warning Разбор ведётся `lexical`, а НЕ `std::stoul`: тот кидает исключение
		 *          и при переполнении, и при негодной записи, а метод объявлен
		 *          `noexcept` - перехват исключений в теле его есть скрытая роскошь,
		 *          которую рамка не обещает. Разбор локали не подвластен и исключений
		 *          не бросает вовсе
		 *
		 * @note Метка пишется самим мастером из четырёхбайтового номера процесса, и
		 *       запись, в четыре байта не влезающая либо разобранная не целиком, меткой
		 *       не является: поставить её мог лишь тот, кто правил окружение руками
		 *
		 */
		uint64_t number = 0;
		// Выполняем разбор идентификатора процесса мастера
		const auto & parsed = lexical::fromChars <uint64_t, wchar_t> (marker, (marker + size), number);
		/**
		 * Если метка роли числом не разобралась
		 */
		if(!static_cast <bool> (parsed) || (parsed.ptr != (marker + size)) ||
		  (number > static_cast <uint64_t> (numeric_limits <uint32_t>::max()))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster role marker is malformed, the process is treated as master", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster role marker is malformed, the process is treated as master", log::flag_t::CRITICAL);
			#endif
			// Сообщаем, что процесс дочерним не является
			return false;
		}
		// Запоминаем разобранный идентификатор процесса мастера
		pid = static_cast <pid_t> (number);
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
		else {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster master process [%d] could not be opened, orphan detection is disabled", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, pid);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster master process [%d] could not be opened, orphan detection is disabled", log::flag_t::CRITICAL, pid);
			#endif
		}
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
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster worker process [%d] has no messaging channel with the master, the worker is terminated", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, ::getpid());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster worker process [%d] has no messaging channel with the master, the worker is terminated", log::flag_t::CRITICAL, ::getpid());
			#endif
			// Завершаем работу работника: работать ему нечем
			::_exit(EXIT_FAILURE);
		}
		// Сообщаем, что процесс является дочерним
		return true;
	}
	/**
	 * @brief Метод открытия своего конца канала обмена сообщениями с мастером
	 *
	 * @details Соответствия socketpair у Microsoft Windows нет, и пара обмена строится
	 *          именованным каналом. Сторону ожидания заводит мастер, а имя её передаёт
	 *          порождаемому процессу окружением - дескриптора тот по наследству не
	 *          получает, проходя main заново. Здесь имя это снимается, и по нему
	 *          открывается свой конец.
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
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster worker pipe name is not set, messaging with the master is disabled", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster worker pipe name is not set, messaging with the master is disabled", log::flag_t::CRITICAL);
			#endif
			// Сообщаем, что канал обмена сообщениями не открыт
			return false;
		}
		/**
		 * @brief Снимаем имя канала из окружения
		 *
		 * @details Порождай работник собственные процессы, имя досталось бы тем по наследству, и
		 *          те подключились бы к чужому каналу.
		 *
		 */
		::SetEnvironmentVariableW(L"AWH_CLUSTER_PIPE", nullptr);
		// Заводим событие обмена сообщениями с мастером
		const event::id_t eid = engine::io::event(event::node_t::IPC, event::family_t::PIPE, this->_type);
		// Если событие завести не удалось
		if(eid == 0){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster worker event could not be created", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster worker event could not be created", log::flag_t::CRITICAL);
			#endif
			// Сообщаем, что канал обмена сообщениями не открыт
			return false;
		}
		// Устанавливаем имя канала обмена сообщениями событию
		engine::target::set(eid, fmk::convert(wstring(buffer)));
		// Устанавливаем функцию обратного вызова на событие записи сообщений
		engine::io::on(eid, static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие изменения состояния
		engine::io::on(eid, static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие чтения сообщений
		engine::io::on(eid, static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		engine::io::on(eid, static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
		engine::io::on(eid, static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
		/**
		 * Открываем свой конец служебного канала
		 *
		 * @details Имя его приходит той же дорогой, что и имя пользовательского канала, -
		 *          окружением, - и снимается из окружения по тому же доводу: порождай
		 *          работник свои процессы, имя досталось бы им по наследству.
		 *
		 * @warning Отказ здесь работника НЕ валит: без служебного канала он лишается связи
		 *          с соседями и приказов мастера, но свою пользовательскую работу ведёт
		 *          по-прежнему
		 *
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
				cid = engine::io::event(event::node_t::IPC, event::family_t::PIPE, event::type_t::SEQPACKET);
				// Если событие служебного канала заведено
				if(cid != 0){
					// Устанавливаем имя служебного канала событию
					engine::target::set(cid, fmk::convert(wstring(control)));
					// Устанавливаем функцию обратного вызова на чтение служебного канала
					engine::io::on(cid, static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
				}
			// Если имени служебного канала в окружении нет
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("Cluster control channel name is not set, the worker is deaf to the master", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Cluster control channel name is not set, the worker is deaf to the master", log::flag_t::CRITICAL);
				#endif
			}
		}
		// Создаём воркера для самого себя
		unique_ptr <worker_t> worker = make_unique <worker_t> ();
		// Устанавливаем время создания процесса
		worker->life = fmk::milliseconds();
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
		 * @brief Выполняем фиксацию, подключение и запуск работы события
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
		if(!(engine::io::commit(eid) && engine::io::connect({eid}) && engine::io::launch(eid))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster worker event could not be launched", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster worker event could not be launched", log::flag_t::CRITICAL);
			#endif
			// Удаляем соответствие идентификатора события идентификатору процесса
			this->_matching.erase(eid);
			// Удаляем воркера из списка активных воркеров
			this->_workers.erase(ret.first);
			// Уничтожаем событие обмена сообщениями с мастером
			engine::io::destroy(eid);
			// Если служебное событие заведено
			if(cid != 0){
				// Уничтожаем служебное событие
				engine::io::destroy(cid);
				// Обнуляем идентификатор служебного канала
				this->_control = 0;
			}
			// Сообщаем, что канал обмена сообщениями не открыт
			return false;
		}
		// Если служебное событие заведено
		if(cid != 0){
			// Выполняем фиксацию, подключение и запуск работы служебного события
			if(!(engine::io::commit(cid) && engine::io::connect({cid}) && engine::io::launch(cid))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("Cluster control channel could not be launched, the worker is deaf to the master", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Cluster control channel could not be launched, the worker is deaf to the master", log::flag_t::CRITICAL);
				#endif
				// Уничтожаем служебное событие
				engine::io::destroy(cid);
				// Обнуляем идентификатор служебного канала
				this->_control = 0;
				// Обнуляем идентификатор служебного события воркера
				ret.first->second->cid = 0;
			/**
			 * @brief Извещаем мастера о том, что работник поднялся
			 *
			 * @details У этой системы извещение - единственный способ узнать правду: мастер
			 *          знает лишь, что процесс породился, а дошёл ли тот до цикла событий и
			 *          открыл ли свои концы каналов, ему неизвестно вовсе.
			 *
			 */
			} else this->dispatch(cid, control_t::ONLINE, ret.first->first);
		}
		// Сообщаем, что канал обмена сообщениями открыт
		return true;
	}
	/**
	 * @brief Метод порождения дочернего процесса повторным запуском образа приложения
	 *
	 * @param eid идентификатор пользовательского канала
	 * @param cid идентификатор служебного канала
	 * @return    идентификатор порождённого процесса, либо 0 при отказе
	 *
	 */
	pid_t awh::unit::Cluster::execute(const event::id_t eid, const event::id_t cid) noexcept {
		/**
		 * @brief Заводим объект задания, если тот ещё не заведён
		 *
		 * @details Задание держит все порождённые процессы и снимает их при закрытии последнего
		 *          своего дескриптора - то есть при завершении мастера любым образом, включая
		 *          падение. Так замещается проверка на осиротевание, какую у POSIX даёт сравнение
		 *          с getppid, и осиротевших воркеров не остаётся.
		 *
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
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						log::debug("Cluster job object limits could not be set", __PRETTY_FUNCTION__, {eid, cid}, log::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("Cluster job object limits could not be set", log::flag_t::WARNING);
					#endif
			// Если объект задания создать не удалось
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("Cluster job object could not be created, orphaned workers are possible", __PRETTY_FUNCTION__, {eid, cid}, log::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Cluster job object could not be created, orphaned workers are possible", log::flag_t::WARNING);
				#endif
			}
		}
		// Буфер под путь к образу приложения
		wchar_t image[MAX_PATH]{0};
		// Получаем длину пути к образу приложения
		const DWORD length = ::GetModuleFileNameW(nullptr, image, MAX_PATH);
		/**
		 * @brief Путь обязан уместиться в буфер целиком
		 *
		 * @warning Проверялся прежде лишь нуль, а усечение НУЛЁМ НЕ ОТВЕЧАЕТ: обращение
		 *          отдаёт при нём размер буфера и оставляет путь обрезанным. Дальше
		 *          порождение шло по обрезанному пути и отказывало «файл не найден» - то
		 *          есть отказ называл следствие и о длине пути не намекал ничем
		 *
		 * @note Предел этот достижим: у Microsoft Windows `MAX_PATH` равен 260, а образ набора
		 *       проверок живёт под каталогом сборки, вложенным на пять-шесть уровней
		 *
		 */
		if((length == 0) || (length >= MAX_PATH)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster could not determine its own executable path%s", __PRETTY_FUNCTION__, {eid, cid}, log::flag_t::CRITICAL, ((length >= MAX_PATH) ? ": the path is longer than MAX_PATH" : ""));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster could not determine its own executable path%s", log::flag_t::CRITICAL, ((length >= MAX_PATH) ? ": the path is longer than MAX_PATH" : ""));
			#endif
			// Возвращаем признак отсутствия порождённого процесса
			return 0;
		}
		// Готовим отдельный блок окружения, не меняя переменные мастера
		vector <wchar_t> environment = ::__awh_cluster_environment__(this->_pid, engine::target::get(eid), engine::target::get(cid));
		// Если окружение работника подготовить не удалось
		if(environment.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster worker environment could not be prepared", __PRETTY_FUNCTION__, {eid, cid}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster worker environment could not be prepared", log::flag_t::CRITICAL);
			#endif
			// Возвращаем признак отсутствия порождённого процесса
			return 0;
		}
		// Копия строки доводов запуска: CreateProcessW вправе менять её на месте
		wstring command = ::GetCommandLineW();
		// Создаём объект сведений о запуске процесса
		STARTUPINFOW startup{};
		// Устанавливаем размер объекта сведений о запуске
		startup.cb = sizeof(startup);
		// Создаём объект сведений о порождённом процессе
		PROCESS_INFORMATION info{};
		/**
		 * @brief Описатели мастера дочернему процессу НЕ наследуются
		 *
		 * @details Порождаем процесс приостановленным: до внесения его в задание он работать не
		 *          должен. Успей он завершиться раньше, в задание попасть было бы уже некому, и
		 *          снятие по закрытию задания его не коснулось бы.
		 *
		 * @warning Наследование было включено, а нужды в нём нет ни одной: канал воркер
		 *          открывает ПО ИМЕНИ, роль узнаёт из окружения, а описатель мастера
		 *          открывает по его номеру. Меж тем сокеты у Microsoft Windows наследуются по
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
		const BOOL created = ::CreateProcessW(image, command.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT, environment.data(), nullptr, &startup, &info);
		// Если процесс породить не удалось
		if(!created){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Child process could not be created", __PRETTY_FUNCTION__, {eid, cid}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Child process could not be created", log::flag_t::CRITICAL);
			#endif
			// Возвращаем признак отсутствия порождённого процесса
			return 0;
		}
		// Если объект задания заведён — вносим в него порождённый процесс
		if((::__awh_job__ != nullptr) && !::AssignProcessToJobObject(::__awh_job__, info.hProcess))
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Child process [%d] could not be assigned to the cluster job object", __PRETTY_FUNCTION__, {eid, cid}, log::flag_t::WARNING, static_cast <int32_t> (info.dwProcessId));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Child process [%d] could not be assigned to the cluster job object", log::flag_t::WARNING, static_cast <int32_t> (info.dwProcessId));
			#endif
		// Получаем идентификатор порождённого процесса
		const pid_t pid = static_cast <pid_t> (info.dwProcessId);
		{
			// Создаём запись о наблюдаемом дочернем процессе
			::child_t child;
			// Запоминаем дескриптор объекта процесса
			child.process = info.hProcess;
			/**
			 * @brief Наблюдение за завершением обязано быть заведено
			 *
			 * @details Подписываемся на завершение процесса.
			 *
			 *          Извещение однократное: WT_EXECUTEONLYONCE снимает подписку после первого
			 *          срабатывания, а завершиться процесс может лишь однажды.
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
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("Child process [%d] termination watch could not be registered, the process is terminated", __PRETTY_FUNCTION__, {eid, cid}, log::flag_t::CRITICAL, pid);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Child process [%d] termination watch could not be registered, the process is terminated", log::flag_t::CRITICAL, pid);
				#endif
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
			const awh::locker_t <> lock(::__awh_mtx__);
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
	 * @brief Метод заведения своего конца канала обмена для порождаемого процесса
	 *
	 * @param type    устройство обмена канала
	 * @param service признак служебного канала
	 * @return        идентификатор заведённого события, либо 0 при отказе
	 *
	 */
	awh::event::id_t awh::unit::Cluster::provision(const event::type_t type, const bool service) noexcept {
		// Добавляем новые события для обмена сообщениями между процессами
		const auto & events = engine::io::events(event::family_t::PIPE, type);
		// Если события не созданы
		if((events[0] == 0) || (events[1] == 0)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster %s channel could not be created", __PRETTY_FUNCTION__, {static_cast <uint16_t> (type), service}, log::flag_t::CRITICAL, (service ? "control" : "worker"));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster %s channel could not be created", log::flag_t::CRITICAL, (service ? "control" : "worker"));
			#endif
			// Выводим отсутствие заведённого события
			return 0;
		}
		// Сохраняем имя канала для ожидания готовности после закрытия второго конца
		const string pipe = engine::target::get(events[0]);
		// Если имя созданного канала недоступно для передачи работнику
		if(pipe.empty()){
			// Уничтожаем конец канала работника
			engine::io::destroy(events[1]);
			// Уничтожаем конец канала мастера
			engine::io::destroy(events[0]);
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster %s pipe name could not be obtained", __PRETTY_FUNCTION__, {static_cast <uint16_t> (type), service}, log::flag_t::CRITICAL, (service ? "control" : "worker"));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster %s pipe name could not be obtained", log::flag_t::CRITICAL, (service ? "control" : "worker"));
			#endif
			// Выводим отсутствие заведённого события
			return 0;
		}
		// Уничтожаем событие дочернего процесса: унаследовать его порождённый процесс не может
		engine::io::destroy(events[1]);
		/**
		 * @brief Устанавливаем опции события
		 *
		 * @note Свой конец канала доводится до ожидания подключения ПРЕЖДЕ порождения процесса
		 *
		 * @details Порождённый процесс выходит на канал по имени сразу, едва запустившись,
		 *          и застаёт единственный экземпляр канала занятым: мастер держал его сам,
		 *          покуда не закрыл свой конец пары, а к ожиданию нового подключения конец
		 *          возвращает лишь подача приёма. Пока приём не подан, обращение работника
		 *          отвечает `ERROR_PIPE_BUSY` (231), и работник сдаётся по сроку ожидания -
		 *          кластер под Microsoft Windows не обменивался тогда ни одним сообщением.
		 *          Проверено щупом на обоих стендах, x86-64 и ARM64.
		 *
		 * @note На системах POSIX порядок этот безразличен: там пара достаётся дочернему
		 *       процессу по наследству вызовом fork, уже связанной с обеих сторон
		 *
		 */
		if(!engine::options::set(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Error setting cluster worker event options", __PRETTY_FUNCTION__, {static_cast <uint16_t> (type), service}, log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Error setting cluster worker event options", log::flag_t::WARNING);
			#endif
		/**
		 * @brief Служебному каналу подписывается одно лишь чтение
		 *
		 * @note Наружу он не выходит вовсе: пользовательские отклики разбирают соответствие
		 *       идентификаторов, а служебные события лежат в своём соответствии, отдельном
		 *
		 */
		if(service)
			// Устанавливаем функцию обратного вызова на чтение служебного канала
			engine::io::on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::control, this, _1, _2, _3)));
		// Если канал является пользовательским
		else {
			// Устанавливаем функцию обратного вызова на событие записи сообщений
			engine::io::on(events[0], static_cast <engine::callback::write_t> (std::bind(&cluster_t::write, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие изменения состояния
			engine::io::on(events[0], static_cast <engine::callback::status_t> (std::bind(&cluster_t::state, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие чтения сообщений
			engine::io::on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::read, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			engine::io::on(events[0], static_cast <engine::callback::error_t> (std::bind(&cluster_t::error, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
			engine::io::on(events[0], static_cast <engine::callback::available_t> (std::bind(&cluster_t::available, this, _1, _2, _3)));
		}
		// Выполняем фиксацию и запуск работы события: конец канала переходит к ожиданию подключения
		if(!(engine::io::commit(events[0]) && engine::io::launch(events[0]))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster %s event could not be launched", __PRETTY_FUNCTION__, {static_cast <uint16_t> (type), service}, log::flag_t::CRITICAL, (service ? "control" : "worker"));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster %s event could not be launched", log::flag_t::CRITICAL, (service ? "control" : "worker"));
			#endif
			// Уничтожаем событие родительского процесса
			engine::io::destroy(events[0]);
			// Выводим отсутствие заведённого события
			return 0;
		}
		/**
		 * @brief Дожидаемся, пока конец канала встанет в ожидание подключения
		 *
		 * @details Экземпляр канала, доставшийся от пары, остаётся за прежним подключением:
		 *          пара заводится связанной, встречный конец её открыт тут же, и закрытие
		 *          его экземпляра не освобождает. Система пускает работника лишь к стороне,
		 *          которая ЖДЁТ подключения, а ожидание подаётся согласованием подписок в
		 *          цикле опроса. Готовность спрашивается поэтому у самой системы:
		 *          `WaitNamedPipeW` с нулевым сроком отвечает истиной ровно тогда, когда
		 *          экземпляр готов принять подключение.
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
		 *
		 */
		{
			// Имя канала в понимании системы
			const wstring & channel = fmk::convert(pipe);
			// Число оборотов ожидания готовности канала
			uint16_t rounds = 0;
			/**
			 * Выполняем обороты цикла, покуда канал не станет ждать подключения
			 */
			while((rounds++ < 200) && !::WaitNamedPipeW(channel.c_str(), 0))
				// Выполняем холостой оборот цикла событий
				engine::io::poll(0);
			/**
			 * @brief Исчерпание оборотов ожидания обязано быть слышно
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
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем предупреждение в лог
					log::debug("Cluster %s pipe was not ready after %u rounds, the worker is spawned anyway", __PRETTY_FUNCTION__, {static_cast <uint16_t> (type), service}, log::flag_t::WARNING, (service ? "control" : "worker"), static_cast <uint32_t> (rounds - 1));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем предупреждение в лог
					log::print("Cluster %s pipe was not ready after %u rounds, the worker is spawned anyway", log::flag_t::WARNING, (service ? "control" : "worker"), static_cast <uint32_t> (rounds - 1));
				#endif
		}
		// Выводим идентификатор заведённого события
		return events[0];
	}
#endif
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
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем в лог сообщение об остановке дочернего процесса
				log::debug("Child process stopped, PID=%d, STATUS=%d", __PRETTY_FUNCTION__, {pid, status}, log::flag_t::WARNING, pid, status);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем в лог сообщение об остановке дочернего процесса
				log::print("Child process stopped, PID=%d, STATUS=%d", log::flag_t::WARNING, pid, status);
			#endif
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
			const bool rapid = ((fmk::milliseconds() - i->second->life) <= this->_rebirth.window);
			// Освобождаем ресурсы завершившегося воркера
			this->release(i->second->eid);
			// Удаляем завершившийся процесс из списка активных воркеров
			this->_workers.erase(i);
			/**
			 * @brief Определяем, ушёл ли работник намеренно
			 *
			 * @details Признак этот ставится приказом мастера либо извещением самого
			 *          работника об уходе, и снимается он здесь - разбором завершившегося.
			 *
			 * @warning Ушедший намеренно работник возрождению НЕ подлежит, и счётчик
			 *          быстрых падений его уход не задевает: иначе мастер поднимал бы
			 *          работника, которому сам же велел уйти, а череда правильных уходов
			 *          останавливала бы кластер защитой от цикла перезапусков
			 *
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
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем в лог сообщение об обнаружении цикла перезапусков
						log::debug("Cluster [%s] worker keeps crashing on startup, aborting after %u rapid restarts", __PRETTY_FUNCTION__, {pid, status}, log::flag_t::CRITICAL, this->_name.c_str(), this->_rebirth.restarts);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем в лог сообщение об обнаружении цикла перезапусков
						log::print("Cluster [%s] worker keeps crashing on startup, aborting after %u rapid restarts", log::flag_t::CRITICAL, this->_name.c_str(), this->_rebirth.restarts);
					#endif
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
	 * Для операционных систем, отличных от Microsoft Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Идентификатор завершившегося процесса
		pid_t pid = 0;
		// Статус завершившегося процесса
		int32_t status = 0;
		/**
		 * Пожинаем все завершившиеся дочерние процессы. Метод выполняется в потоке цикла событий,
		 * поэтому мутации контейнеров, перезапуск воркеров и логирование здесь безопасны
		 */
		while((pid = ::waitpid(-1, &status, WNOHANG)) > 0)
			// Выполняем обработку завершившегося процесса
			this->process(pid, status);
	/**
	 * Если операционной системой является Microsoft Windows
	 */
	#else
		// Список завершившихся процессов, снятый с очереди на разбор
		deque <pid_t> finished;
		{
			// Выполняем блокировку замка доступа к спискам дочерних процессов
			const awh::locker_t <> lock(::__awh_mtx__);
			// Забираем всю очередь завершившихся процессов
			finished.swap(::__awh_finished__);
		}
		/**
		 * Пожинаем все завершившиеся дочерние процессы. Метод выполняется в потоке цикла
		 * событий, поэтому мутации контейнеров, перезапуск воркеров и логирование здесь безопасны
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
				const awh::locker_t <> lock(::__awh_mtx__);
				// Выполняем поиск завершившегося процесса среди наблюдаемых
				auto i = ::__awh_children__.find(pid);
				// Если наблюдаемый процесс найден
				if(i != ::__awh_children__.end()){
					// Получаем код завершения процесса
					if(!::GetExitCodeProcess(i->second.process, &status))
						// Считаем завершение ненормальным, если код получить не удалось
						status = static_cast <DWORD> (EXIT_FAILURE);
					/**
					 * @brief Снимаем описатель ожидания завершения процесса
					 *
					 * @details Снимаем наблюдение за процессом. Ожидание снимается доводом
					 *          INVALID_HANDLE_VALUE - тот велит системе дождаться завершения уже
					 *          начатых извещений, и после возврата обратный вызов не работает ни в
					 *          одном потоке. Без этого дескрипторы закрывались бы под работающим извещением.
					 *
					 */
					wait = i->second.wait;
					// Снимаем дескриптор объекта завершившегося процесса
					process = i->second.process;
					// Удаляем процесс из списка наблюдаемых
					::__awh_children__.erase(i);
				}
			}
			/**
			 * @brief Ожидание снимается ВНЕ замка
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
 * Для операционных систем, отличных от Microsoft Windows
 *
 * @note Метода этого под Microsoft Windows нет вовсе - ни объявления в заголовке, ни тела
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
				engine::io::send(self->_wakeup, &marker, sizeof(marker));
			}
		}
	}
/**
 * Для операционной системы Microsoft Windows
 */
#elif defined(_WIN32) || defined(_WIN64)
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
			const awh::locker_t <> lock(::__awh_mtx__);
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
				engine::io::send(self->_wakeup, &marker, sizeof(marker));
			}
		}
	}
#endif
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
		 * @brief Извещение уходит всем ПОДНЯВШИМСЯ, КРОМЕ названного узла
		 *
		 * @note Себе о своём входе работник вести счёт не обязан, а о своём выбытии
		 *       извещать его уже некуда
		 *
		 * @warning Не дошедшему до цикла узлу извещение слать НЕЛЬЗЯ: свой список
		 *          соседей он получит подъёмом, и посланное раньше пришло бы вторым
		 *
		 */
		if((node != pid) && (this->_online.find(node) != this->_online.end()))
			// Отправляем извещение работнику
			this->dispatch(worker->cid, type, pid);
	}
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
	 * @brief Если сообщение короче заголовка - разбирать нечего
	 *
	 * @note Границы сообщений держит сам канал, и усечённым сообщение прийти не может:
	 *       короткое сообщение означает чужую разметку, а не полученную наполовину
	 *
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
					 * @brief Извещаем новичка обо всех уже поднятых узлах
					 *
					 * @note Своего способа узнать соседей у работника нет вовсе: он знает
					 *       свой номер и номер мастера, и список этот - единственный путь к связи
					 *
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
			 * @brief Если работник уходит сам
			 *
			 * @warning Признак намеренного ухода обязан быть выставлен ДО того, как
			 *          система известит о завершении процесса: разбор завершившегося по
			 *          нему и решает, возрождать ли работника
			 *
			 */
			case static_cast <uint8_t> (control_t::OFFLINE): {
				// Помечаем узел уходящим намеренно
				this->_leaving.emplace(pid);
				// Записываем в лог сообщение об уходе работника
				log::print("Cluster worker process [%d] is leaving on its own", log::flag_t::INFO, pid);
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
				 * @brief Извещаем потребителя лишь о НОВОМ узле
				 *
				 * @note Повторное извещение об уже известном узле увело бы потребителя к
				 *       повторному заказу связи, а тот получил бы отказ `EXISTS`
				 *
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
				 * @brief Для операционной системы Microsoft Windows
				 *
				 * @details Пустое тело означает согласие мастера на связь, которую узел
				 *          завёл сам: свой конец пары у него уже есть, и поднимать ему
				 *          нечего. Имя же канала означает связь, заведённую соседом, - к
				 *          ней выходят по имени.
				 *
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
						link = engine::io::event(event::node_t::IPC, event::family_t::PIPE, this->_type);
						// Если событие связи заведено
						if(link != 0){
							// Устанавливаем имя канала связи событию
							engine::target::set(link, string(reinterpret_cast <const char *> (body), length));
							// Устанавливаем функцию обратного вызова на закрытие связи
							engine::io::on(link, static_cast <engine::callback::event_t> (std::bind(&cluster_t::closed, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на событие изменения состояния
							engine::io::on(link, static_cast <engine::callback::status_t> (std::bind(&cluster_t::broken, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на событие чтения сообщений
							engine::io::on(link, static_cast <engine::callback::read_t> (std::bind(&cluster_t::incoming, this, _1, _2, _3)));
							// Открываем канал при фиксации, включаем очередь отправки и подключаем событие связи
							if(!(engine::io::commit(link) && engine::options::set(link, event::options::NO_IO_BLOCK) && engine::io::connect({link}) && engine::io::launch(link))){
								// Уничтожаем событие связи
								engine::io::destroy(link);
								// Сбрасываем идентификатор события связи
								link = 0;
							}
						}
					}
				/**
				 * @brief Для операционных систем, отличных от Microsoft Windows
				 *
				 * @details Конец пары приезжает описателем по служебному каналу, и узлу
				 *          остаётся поднять его у себя. Фиксация не зовётся: описатель
				 *          заведён чужим процессом, а подъём зовёт её сам.
				 *
				 */
				#else
					// Заводим пустое событие под подъём приехавшего конца пары
					link = engine::io::event(event::node_t::IPC, event::family_t::UDS, this->_type);
					// Если событие связи заведено
					if(link != 0){
						// Поднимаем приехавший конец пары
						if(engine::io::restore(link, body, length)){
							// Устанавливаем функцию обратного вызова на закрытие связи
							engine::io::on(link, static_cast <engine::callback::event_t> (std::bind(&cluster_t::closed, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на событие изменения состояния
							engine::io::on(link, static_cast <engine::callback::status_t> (std::bind(&cluster_t::broken, this, _1, _2)));
							// Устанавливаем функцию обратного вызова на событие чтения сообщений
							engine::io::on(link, static_cast <engine::callback::read_t> (std::bind(&cluster_t::incoming, this, _1, _2, _3)));
							// Запускаем работу события связи
							if(!engine::io::launch(link)){
								// Уничтожаем событие связи
								engine::io::destroy(link);
								// Сбрасываем идентификатор события связи
								link = 0;
							}
						// Если поднять приехавший конец пары не удалось
						} else {
							// Уничтожаем событие связи
							engine::io::destroy(link);
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
				 * @brief Уничтожаем свой конец пары, заведённый под связь
				 *
				 * @note Заводит пару сам узел лишь у Microsoft Windows; у систем POSIX её заводит
				 *       мастер, и уничтожать тут нечего
				 *
				 */
				auto i = this->_peers.find(node);
				// Если свой конец пары заведён
				if(i != this->_peers.end()){
					// Забываем соответствие события связи и узла
					this->_matchingPeers.erase(i->second);
					// Уничтожаем событие связи
					engine::io::destroy(i->second);
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
					engine::io::destroy(i->second);
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
				 * @brief Если потребитель приказ перехватывает - уходить ему самому
				 *
				 * @note Довести своё дело до конца работник вправе: приказ этот не сигнал,
				 *       и мгновенного ухода не требует
				 *
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
 * @brief Метод заведения прямой связи между двумя работниками
 *
 * @param initiator узел, заказавший связь
 * @param peer      узел, с которым связь заказана
 * @param data      тело заказа: имя канала у Microsoft Windows, пустое у систем POSIX
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
	 * @brief Функция отказа в заведении связи
	 *
	 * @note Причина уходит заказавшему связь узлу телом сообщения: молчаливый отказ
	 *       оставил бы его гадать, повторять ли заказ
	 *
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
	}{
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("linking");
		// Если мастер связь запретил
		if(this->_callback.is(fid) && !this->_callback.call <bool (const pid_t, const pid_t)> (fid, initiator, peer))
			// Отказываем в заведении связи
			return deny(reason_t::REFUSED);
	}
	/**
	 * @brief Для операционной системы Microsoft Windows
	 *
	 * @details Пару заводит сам заказавший узел, а мастеру достаётся одно лишь имя
	 *          канала: передать описатель канала нельзя - перенос устройства события
	 *          ведётся там описанием сокета, а канал сокетом не является. Оттого
	 *          встреча идёт по имени, тем же путём, каким работник выходит на канал мастера.
	 *
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
		 * @brief Извещаем заказавшего узла о заведённой связи пустым телом
		 *
		 * @note Свой конец пары у него уже есть - он же её и завёл, - и поднимать ему
		 *       нечего: сообщение это лишь подтверждает согласие мастера
		 *
		 */
		this->dispatch(i->second->cid, control_t::GRANT, peer);
	/**
	 * @brief Для операционных систем, отличных от Microsoft Windows
	 *
	 * @details Пару заводит мастер и раздаёт её концы обоим узлам снимками: событием-
	 *          переносчиком годен узел межпроцессного обмена, и служебный канал к
	 *          получателю им и является.
	 *
	 */
	#else
		// Заводим пару обмена для прямой связи работников
		const auto & events = engine::io::events(event::family_t::UDS, this->_type);
		// Если пара обмена не заведена
		if((events[0] == 0) || (events[1] == 0)){
			// Уничтожаем заведённые концы пары
			if(events[0] != 0)
				// Уничтожаем первый конец пары
				engine::io::destroy(events[0]);
			// Если второй конец пары заведён
			if(events[1] != 0)
				// Уничтожаем второй конец пары
				engine::io::destroy(events[1]);
			// Отказываем в заведении связи
			return deny(reason_t::FAILED);
		}
		// Буфер снимка конца пары, предназначенного заказавшему связь узлу
		vector <uint8_t> snapshot;
		/**
		 * @brief Снимаем снимок и отправляем его тем же обращением
		 *
		 * @warning Порядок здесь значим: снятие снимка лишь ОТМЕЧАЕТ передачу описателя
		 *          у переносчика, а уносит описатель ближайшее исходящее сообщение.
		 *          Снять оба снимка подряд и лишь затем отправить два сообщения нельзя -
		 *          метки разошлись бы с сообщениями
		 *
		 */
		if(!(engine::io::commit(events[0]) && engine::io::snapshot(events[0], i->second->cid, snapshot) &&
		  (this->dispatch(i->second->cid, control_t::GRANT, peer, snapshot.data(), snapshot.size()) > 0))){
			// Уничтожаем оба конца пары: раздать их не удалось
			engine::io::destroy(events[0]);
			// Уничтожаем второй конец пары
			engine::io::destroy(events[1]);
			// Отказываем в заведении связи
			return deny(reason_t::FAILED);
		}
		/**
		 * @brief Отправляем второй конец пары узлу, с которым связь заказана
		 *
		 * @warning Отказ здесь оставляет заказавшего узла со своим концом на руках, и
		 *          известить его о разрыве обязаны мы: сам он о нём не узнает, покуда
		 *          не попробует что-нибудь отправить
		 *
		 */
		if(!(engine::io::commit(events[1]) && engine::io::snapshot(events[1], j->second->cid, snapshot) &&
		  (this->dispatch(j->second->cid, control_t::GRANT, initiator, snapshot.data(), snapshot.size()) > 0))){
			/**
			 * @brief Извещаем заказавшего узла РАЗРЫВОМ, а не отказом
			 *
			 * @note Свой конец пары он уже получил первым снимком, и отказ сказал бы ему
			 *       неправду: связь завелась и тут же распалась, а не не завелась вовсе.
			 *       Разрыв же он разберёт как обычно - снесёт своё событие
			 *
			 */
			this->dispatch(i->second->cid, control_t::DROP, peer);
			// Уничтожаем оба конца пары
			engine::io::destroy(events[0]);
			// Уничтожаем второй конец пары
			engine::io::destroy(events[1]);
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster link between workers [%d] and [%d] has fallen apart on the second handover", __PRETTY_FUNCTION__, {initiator, peer, data, size}, log::flag_t::CRITICAL, initiator, peer);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster link between workers [%d] and [%d] has fallen apart on the second handover", log::flag_t::CRITICAL, initiator, peer);
			#endif
			// Выходим из функции
			return;
		}
		/**
		 * @brief Уничтожаем свои узлы пары: описатели ушли работникам
		 *
		 * @note Держать их у себя значило бы перехватывать обмен, предназначенный не нам
		 *
		 */
		engine::io::destroy(events[0]);
		// Уничтожаем второй конец пары
		engine::io::destroy(events[1]);
	#endif
	// Запоминаем заведённую связь у заказавшего узла
	this->_links[initiator].emplace(peer);
	// Запоминаем заведённую связь у узла, с которым связь заказана
	this->_links[peer].emplace(initiator);
	// Записываем в лог сообщение о заведённой связи
	log::print("Cluster workers [%d] and [%d] have been linked", log::flag_t::INFO, initiator, peer);
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
	 * @brief Если тело сообщения не вмещается в служебный канал
	 *
	 * @note Проверка эта - единственное место, где предел объявлен вызывающей стороне
	 *       честно: отказ записи в канал назвал бы ей чужую причину
	 *
	 */
	if(size > (::__AWH_CLUSTER_CONTROL_LIMIT__ - sizeof(::envelope_t))){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Cluster control message of %zu bytes exceeds the limit of %zu bytes", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (type), pid, buffer, size}, log::flag_t::WARNING, size, (::__AWH_CLUSTER_CONTROL_LIMIT__ - sizeof(::envelope_t)));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Cluster control message of %zu bytes exceeds the limit of %zu bytes", log::flag_t::WARNING, size, (::__AWH_CLUSTER_CONTROL_LIMIT__ - sizeof(::envelope_t)));
		#endif
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
	return engine::io::send(eid, message.data(), message.size());
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
					log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, size}, log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
					log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, data, size}, log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
							this->_callback.call <void (const pid_t, const int32_t)> ("exit", i->first, __AWH_CLUSTER_STOPPED__);
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
						log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (status)}, log::flag_t::CRITICAL, ::getpid());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Процесс превратился в зомби, самоликвидируем его
						log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
							log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (status)}, log::flag_t::CRITICAL, ::getpid());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Процесс превратился в зомби, самоликвидируем его
							log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
					log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (error), message}, log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
					log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {eid, static_cast <uint16_t> (status), size}, log::flag_t::CRITICAL, ::getpid());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Процесс превратился в зомби, самоликвидируем его
					log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
	 * Для операционной системы Microsoft Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		/**
		 * Различить возврат из main и снятие через TerminateProcess у Microsoft Windows нельзя:
		 * код завершения выставляется и в том, и в другом случае. Отделяется потому лишь
		 * то, что кодом возврата не является вовсе - падение, снятие с клавиатуры и
		 * остановка воркера мастером
		 */
		return (!cluster_t::crashed(status) && !cluster_t::manual(status) && (status != ::__AWH_CLUSTER_STOPPED__));
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Сообщаем, завершился ли процесс сам
		return WIFEXITED(status);
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
	 * Для операционной системы Microsoft Windows
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
 * @brief Метод проверки, завершился ли процесс ненормально
 *
 * @param status состояние завершения процесса
 * @return       признак ненормального завершения процесса
 *
 */
bool awh::unit::Cluster::crashed(const int32_t status) noexcept {
	/**
	 * Для операционной системы Microsoft Windows
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
 * @brief Метод проверки, снят ли процесс сигналом
 *
 * @param status состояние завершения процесса
 * @return       признак того, что процесс снят сигналом
 *
 */
bool awh::unit::Cluster::signaled([[maybe_unused]] const int32_t status) noexcept {
	/**
	 * Для операционной системы Microsoft Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Сигналов у Microsoft Windows нет вовсе, снятым сигналом процесс быть не может
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
 * @brief Метод получения кода возврата завершившегося процесса
 *
 * @param status состояние завершения процесса
 * @return       код возврата процесса, либо EXIT_FAILURE при завершении ненормальном
 *
 */
int32_t awh::unit::Cluster::exitcode(const int32_t status) noexcept {
	/**
	 * Для операционной системы Microsoft Windows
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
 * @brief Метод получения номера сигнала, снявшего процесс
 *
 * @param status состояние завершения процесса
 * @return       номер сигнала, либо 0 если процесс снят не сигналом
 *
 */
int32_t awh::unit::Cluster::termsig([[maybe_unused]] const int32_t status) noexcept {
	/**
	 * Для операционной системы Microsoft Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Сигналов у Microsoft Windows нет вовсе, отдавать нечего
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
			log::debug("Only the master process can stop the cluster", __PRETTY_FUNCTION__, {}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only the master process can stop the cluster", log::flag_t::WARNING);
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
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Cluster is rejected as the second one in this process and cannot be started", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Cluster is rejected as the second one in this process and cannot be started", log::flag_t::CRITICAL);
		#endif
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
			log::debug("Only the master process can start the cluster", __PRETTY_FUNCTION__, {}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only the master process can start the cluster", log::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод определения роли процесса до запуска кластера
 *
 * @details Отвечает на вопрос «запущен ли этот процесс работником» ещё до того, как
 *          кластер начал работу: порядок подготовки у мастера и у работника
 *          расходится, и знать роль нужно раньше запуска.
 *
 * @return признак того, что процесс запущен работником кластера
 *
 */
bool awh::unit::Cluster::worker() const noexcept {
	/**
	 * @brief Для операционной системы Microsoft Windows
	 *
	 * @details Ветвления у этой системы нет: работник запускается заново тем же
	 *          образом и той же строкой доводов, и отличает его лишь метка роли,
	 *          выставленная мастером в окружении.
	 *
	 * @note Метка здесь только спрашивается: снимает её перенятие роли при запуске
	 *       кластера, и снять её раньше значило бы отнять у него признак
	 *
	 */
	#if defined(_WIN32) || defined(_WIN64)
		/**
		 * @brief Роль спрашивается двумя путями, и оба нужны
		 *
		 * @note До перенятия роли работника выдаёт метка в окружении: номер процесса
		 *       мастера ещё свой, и метод master ответил бы утвердительно. После
		 *       перенятия метки уже нет, зато номер процесса мастера чужой, и ответ
		 *       даёт сам master
		 *
		 */
		return ((::GetEnvironmentVariableW(L"AWH_CLUSTER_MASTER", nullptr, 0) != 0) || !this->master());
	/**
	 * @brief Для операционных систем POSIX
	 *
	 * @note Работник у них получается ветвлением и роль свою знает с первого мига:
	 *       спрашивать окружение незачем
	 *
	 */
	#else
		// Выводим признак того, что процесс родительским не является
		return !this->master();
	#endif
}
/**
 * @brief Метод получения предельного размера пересылаемого сообщения
 *
 * @return предельный размер тела пересылаемого сообщения в байтах
 *
 */
size_t awh::unit::Cluster::maximumRelay() const noexcept {
	// Выводим предельный размер тела пересылаемого сообщения
	return (::__AWH_CLUSTER_CONTROL_LIMIT__ - sizeof(::envelope_t));
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
			// Очищаем список связей между работниками
			this->_links.clear();
			// Очищаем список поднявшихся узлов
			this->_online.clear();
			// Очищаем список уходящих намеренно узлов
			this->_leaving.clear();
		}
	// Если процесс является дочерним то выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Only the master process can clear the cluster", __PRETTY_FUNCTION__, {}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only the master process can clear the cluster", log::flag_t::WARNING);
		#endif
	}
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
				log::debug("Only the master process can create child processes", __PRETTY_FUNCTION__, {}, log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Only the master process can create child processes", log::flag_t::WARNING);
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
					::__awh_terminate__(i->first);
				// Удаляем процесс из списка активных воркеров
				this->_workers.erase(i);
				/**
				 * @brief Снимаем узел с учёта и разрываем его связи
				 *
				 * @warning Разбор завершившегося процесса сюда НЕ придёт: воркера в списке
				 *          активных уже нет, и известие системы о его смерти пройдёт мимо.
				 *          Не разорви мы связи здесь - соседи держали бы события связи с
				 *          покойником до конца своей работы
				 *
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
			log::debug("Only the master process can remove child processes", __PRETTY_FUNCTION__, {pid, static_cast <uint16_t> (shutdown)}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only the master process can remove child processes", log::flag_t::WARNING);
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
	// Выполняем установку функции обратного вызова при входе узла в кластер
	this->_callback.set("join", callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса
	this->_callback.set("exit", callback);
	// Выполняем установку функции обратного вызова при получении сообщения по прямой связи
	this->_callback.set("peer", callback);
	// Выполняем установку функции обратного вызова при получении пересылки через мастера
	this->_callback.set("relay", callback);
	// Выполняем установку функции обратного вызова при получении состояния процесса
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при выбытии узла из кластера
	this->_callback.set("leave", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса
	this->_callback.set("rebase", callback);
	// Выполняем установку функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
	this->_callback.set("events", callback);
	// Выполняем установку функции обратного вызова при заведении прямой связи
	this->_callback.set("linked", callback);
	// Выполняем установку функции обратного вызова при отправке сообщения
	this->_callback.set("sending", callback);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callback.set("message", callback);
	// Выполняем установку функции обратного вызова при запросе разрешения на связь
	this->_callback.set("linking", callback);
	// Выполняем установку функции обратного вызова при получении приказа завершить работу
	this->_callback.set("shutdown", callback);
	// Выполняем установку функции обратного вызова при разрыве прямой связи
	this->_callback.set("unlinked", callback);
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
				return engine::io::send(i->second->eid, buffer, size);
		// Если родительский процесс умер
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Process [%d] has turned into a zombie, we perform self-destruction", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::CRITICAL, ::getpid());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Процесс превратился в зомби, самоликвидируем его
				log::print("Process [%d] has turned into a zombie, we perform self-destruction", log::flag_t::CRITICAL, ::getpid());
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
			log::debug("A message addressed to a parent process can only be sent from child processes", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Процесс превратился в зомби, самоликвидируем его
			log::print("A message addressed to a parent process can only be sent from child processes", log::flag_t::WARNING);
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
			return engine::io::send(i->second->eid, buffer, size);
	// Если процесс является дочерним
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, {pid, buffer, size}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Процесс превратился в зомби, самоликвидируем его
			log::print("A message addressed to a child process can only be sent from the parent process", log::flag_t::WARNING);
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
				result += engine::io::send(worker->eid, buffer, size);
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
			log::debug("A message addressed to a child process can only be sent from the parent process", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Процесс превратился в зомби, самоликвидируем его
			log::print("A message addressed to a child process can only be sent from the parent process", log::flag_t::WARNING);
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
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Only a worker process can order a direct link", __PRETTY_FUNCTION__, {pid}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only a worker process can order a direct link", log::flag_t::WARNING);
		#endif
		// Выводим отрицательный результат
		return false;
	}
	// Если служебный канал не заведён, заказывать связь нечем
	if(this->_control == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Cluster control channel is not established, a direct link cannot be ordered", __PRETTY_FUNCTION__, {pid}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Cluster control channel is not established, a direct link cannot be ordered", log::flag_t::WARNING);
		#endif
		// Выводим отрицательный результат
		return false;
	}
	// Если узел заказал связь с самим собой
	if(pid == static_cast <pid_t> (::getpid())){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Cluster worker process [%d] cannot link to itself", __PRETTY_FUNCTION__, {pid}, log::flag_t::WARNING, pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Cluster worker process [%d] cannot link to itself", log::flag_t::WARNING, pid);
		#endif
		// Выводим отрицательный результат
		return false;
	}
	// Если связь с названным узлом уже заведена
	if(this->_peers.find(pid) != this->_peers.end()){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Cluster worker process [%d] is already linked", __PRETTY_FUNCTION__, {pid}, log::flag_t::WARNING, pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Cluster worker process [%d] is already linked", log::flag_t::WARNING, pid);
		#endif
		// Выводим отрицательный результат
		return false;
	}
	/**
	 * @brief Для операционной системы Microsoft Windows
	 *
	 * @details Пару заводит сам заказавший узел: описатель канала передать нельзя -
	 *          перенос устройства события ведётся описанием сокета, а канал сокетом не
	 *          является, - и встреча идёт по имени. Мастеру уходит одно лишь имя, а тот
	 *          доводит его до соседа, распорядившись правом на связь.
	 *
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Заводим пару обмена для прямой связи
		const auto & events = engine::io::events(event::family_t::PIPE, this->_type);
		// Если пара обмена не заведена
		if((events[0] == 0) || (events[1] == 0)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster link channel could not be created", __PRETTY_FUNCTION__, {pid}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster link channel could not be created", log::flag_t::CRITICAL);
			#endif
			// Если первый конец пары заведён
			if(events[0] != 0)
				// Уничтожаем первый конец пары
				engine::io::destroy(events[0]);
			// Если второй конец пары заведён
			if(events[1] != 0)
				// Уничтожаем второй конец пары
				engine::io::destroy(events[1]);
			// Выводим отрицательный результат
			return false;
		}
		/**
		 * @brief Снимаем имя канала ПРЕЖДЕ уничтожения второго конца
		 *
		 * @note Уничтоженное событие имени уже не отдаст, а имя это - единственная точка
		 *       встречи для соседа
		 *
		 */
		const string & name = engine::target::get(events[1]);
		// Если имя канала связи получить не удалось
		if(name.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster link channel name could not be obtained", __PRETTY_FUNCTION__, {pid}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster link channel name could not be obtained", log::flag_t::CRITICAL);
			#endif
			// Уничтожаем первый конец пары
			engine::io::destroy(events[0]);
			// Уничтожаем второй конец пары
			engine::io::destroy(events[1]);
			// Выводим отрицательный результат
			return false;
		}
		// Уничтожаем второй конец пары: к нему сосед выйдет по имени
		engine::io::destroy(events[1]);
		// Устанавливаем функцию обратного вызова на закрытие связи
		engine::io::on(events[0], static_cast <engine::callback::event_t> (std::bind(&cluster_t::closed, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие изменения состояния
		engine::io::on(events[0], static_cast <engine::callback::status_t> (std::bind(&cluster_t::broken, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие чтения сообщений
		engine::io::on(events[0], static_cast <engine::callback::read_t> (std::bind(&cluster_t::incoming, this, _1, _2, _3)));
		/**
		 * @brief Включаем неблокирующую отправку, фиксируем и запускаем свой конец канала связи
		 *
		 * @note Разрешение мастера может прийти раньше подключения соседа: первая
		 *       отправка тогда сохраняется в очереди до готовности канала
		 *
		 * @note Ожидания подключения тут не подаётся отдельно, в отличие от порождения
		 *       работника: сосед выйдет на канал не раньше, чем заказ обойдёт мастера и
		 *       дойдёт до него, - а это не один оборот цикла событий, и конец успеет
		 *       встать в ожидание сам
		 *
		 */
		if(!(engine::options::set(events[0], event::options::NO_IO_BLOCK) && engine::io::commit(events[0]) && engine::io::launch(events[0]))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Cluster link channel could not be launched", __PRETTY_FUNCTION__, {pid}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Cluster link channel could not be launched", log::flag_t::CRITICAL);
			#endif
			// Уничтожаем свой конец пары
			engine::io::destroy(events[0]);
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
		engine::io::destroy(events[0]);
		// Выводим отрицательный результат
		return false;
	/**
	 * @brief Для операционных систем, отличных от Microsoft Windows
	 *
	 * @details Пару заводит мастер и раздаёт её концы описателями: заказ несёт один
	 *          лишь номер узла.
	 *
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
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Only a worker process can break a direct link", __PRETTY_FUNCTION__, {pid}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only a worker process can break a direct link", log::flag_t::WARNING);
		#endif
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
	engine::io::destroy(i->second);
	// Забываем разорванную связь
	this->_peers.erase(i);
	// Извещаем мастера о разрыве связи
	return (this->dispatch(this->_control, control_t::UNLINK, pid) > 0);
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
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Only a worker process can leave the cluster", __PRETTY_FUNCTION__, {code}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only a worker process can leave the cluster", log::flag_t::WARNING);
		#endif
		// Выходим из функции
		return;
	}
	/**
	 * @brief Извещаем мастера об уходе прежде выхода
	 *
	 * @details Отправка отдаёт сообщение каналу тут же, покуда очередь его пуста, и
	 *          возврата отсюда довольно: отданное в буфер ядра переживает смерть
	 *          отправителя - приёмный конец принадлежит мастеру, и тот дочитает
	 *          известие уже после того, как работника не станет.
	 *
	 * @note Извещение уходит даже тогда, когда служебного канала нет вовсе: выход в
	 *       таком случае идёт молча, и мастер узнает об уходе одним лишь завершением
	 *       процесса - возродив работника, если возрождение включено
	 *
	 */
	if(this->_control != 0)
		// Извещаем мастера о своём уходе
		this->dispatch(this->_control, control_t::OFFLINE, static_cast <pid_t> (::getpid()), &code, sizeof(code));
	// Записываем в лог сообщение об уходе работника
	log::print("Cluster worker process [%d] is leaving with code %d", log::flag_t::INFO, ::getpid(), code);
	// Завершаем работу процесса названным кодом
	::_exit(code);
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
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Only the master process can order a worker to terminate", __PRETTY_FUNCTION__, {pid, code}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only the master process can order a worker to terminate", log::flag_t::WARNING);
		#endif
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
	 * @brief Помечаем узел уходящим намеренно ПРЕЖДЕ отправки приказа
	 *
	 * @warning Порядок здесь значим: работник вправе уйти немедленно, и известие
	 *          системы о завершившемся процессе способно опередить возврат отсюда.
	 *          Признак, выставленный после, застал бы разбор завершившегося уже
	 *          прошедшим - и тот возродил бы ушедшего по приказу работника
	 *
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
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Only a worker process can relay a message through the master", __PRETTY_FUNCTION__, {pid, buffer, size}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Only a worker process can relay a message through the master", log::flag_t::WARNING);
		#endif
		// Возвращаем значение по умолчанию
		return 0;
	}
	// Если служебный канал не заведён, пересылать нечем
	if(this->_control == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Cluster control channel is not established, a message cannot be relayed", __PRETTY_FUNCTION__, {pid, buffer, size}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Cluster control channel is not established, a message cannot be relayed", log::flag_t::WARNING);
		#endif
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
		return engine::io::send(i->second, buffer, size);
	/**
	 * Если включён режим отладки
	 */
	#if defined(DEBUG_MODE)
		// Записываем ошибку в лог
		log::debug("Cluster worker process [%d] is not linked, a message cannot be transmitted", __PRETTY_FUNCTION__, {pid, buffer, size}, log::flag_t::WARNING, pid);
	/**
	 * Если режим отладки не включён
	 */
	#else
		// Записываем ошибку в лог
		log::print("Cluster worker process [%d] is not linked, a message cannot be transmitted", log::flag_t::WARNING, pid);
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
			return engine::buffer::size::get(i->second->eid, action);
	// Если процесс является дочерним
	} else {
		// Выполняем поиск процесса по идентификатору
		auto i = this->_workers.find(::getpid());
		// Если указанный процесс найден
		if(i != this->_workers.end())
			// Извлекаем размер буфера события
			return engine::buffer::size::get(i->second->eid, action);
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
			return engine::buffer::size::set(i->second->eid, action, size);
	// Если процесс является дочерним
	} else {
		// Выполняем поиск процесса по идентификатору
		auto i = this->_workers.find(::getpid());
		// Если указанный процесс найден
		if(i != this->_workers.end())
			// Устанавливаем размер буфера события
			return engine::buffer::size::set(i->second->eid, action, size);
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
 _count(0), _wakeup(0), _control(0), _type(event::type_t::SEQPACKET) {
	/**
	 * Для операционной системы Microsoft Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Обнуляем дескриптор объекта родительского процесса (захватывается дочерним процессом при запуске)
		this->_master = 0;
		/**
		 * @brief Если кластер уже был создан ранее
		 *
		 * @warning Отвергнутый кластер объекта процесса НЕ перехватывает: перехват
		 *          увёл бы к нему дочерние процессы первого кластера
		 *
		 */
		if(::__awh_cluster__ != nullptr){
			// Помечаем кластер отвергнутым
			this->_rejected = true;
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("A cluster already exists in this process: the second one is rejected and will not be started", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("A cluster already exists in this process: the second one is rejected and will not be started", log::flag_t::CRITICAL);
			#endif
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
	 * Для операционных систем, отличных от Microsoft Windows
	 */
	#else
		/**
		 * @brief Если кластер уже был создан ранее
		 *
		 * @warning Отвергнутый кластер НЕ трогает ни объект процесса, ни перехватчик
		 *          сигнала SIGCHLD: перехват увёл бы к нему дочерние процессы первого
		 *          кластера, а снятие оставило бы первый без оповещений вовсе
		 *
		 */
		if(::__awh_cluster__ != nullptr){
			// Помечаем кластер отвергнутым
			this->_rejected = true;
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("A cluster already exists in this process: the second one is rejected and will not be started", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("A cluster already exists in this process: the second one is rejected and will not be started", log::flag_t::CRITICAL);
			#endif
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
		engine::io::destroy(this->_wakeup);
		// Обнуляем идентификатор события пробуждения
		this->_wakeup = 0;
	}
	/**
	 * Для операционной системы Microsoft Windows
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
			unordered_map <pid_t, ::child_t> children;
			{
				// Выполняем блокировку замка доступа к спискам дочерних процессов
				const awh::locker_t <> lock(::__awh_mtx__);
				// Снимаем список наблюдаемых процессов целиком
				children.swap(::__awh_children__);
				// Очищаем очередь завершившихся процессов
				::__awh_finished__.clear();
			}
			/**
			 * @brief Ожидания снимаются ВНЕ замка
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
			 * @brief Закрываем объект задания
			 *
			 * @details Закрытие последнего дескриптора задания снимает все входящие в него
			 *          процессы - тем и завершается работа воркеров, переживших мастера.
			 *
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
	 * Для операционных систем, отличных от Microsoft Windows
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
