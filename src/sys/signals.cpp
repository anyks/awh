/**
 * @file: signals.cpp
 * @date: 2026-01-26
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
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <thread>
#include <cstring>
#include <iostream>

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартный заголовочный файл
	 */
	#include <vector>

	/**
	 * Системные заголовочные файлы
	 */
	#include <pwd.h>
	#include <fcntl.h>
	#include <unistd.h>

	/**
	 * Подключаем заголовочный файл для получения названия процесса
	 */
	#include <sys/procre.hpp>
/**
 * Для операционной системы MS Windows
 */
#else
	/**
	 * Стандартные заголовочные файлы для очереди и условной переменной
	 */
	#include <queue>
	#include <mutex>
	#include <condition_variable>
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/signals.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем функции обработки сигналов в пространство имён
 *
 */
namespace signals {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		/**
		 * @brief Полезная нагрузка, передаваемая из обработчика сигнала в рабочий поток
		 *
		 * @details Структура имеет фиксированный размер меньше PIPE_BUF, поэтому её запись
		 *          в самопайп атомарна и безопасна внутри обработчика сигнала.
		 */
		typedef struct Payload {
			// Идентификатор процесса-отправителя
			pid_t pid;
			// Идентификатор пользователя-отправителя
			uid_t uid;
			// Номер полученного сигнала
			int32_t sig;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Payload() noexcept :
			 pid(0), uid(0), sig(0) {}
		} __attribute__((packed)) payload_t;

		/**
		 * Дескриптор записи самопайпа, доступный обработчику сигнала.
		 * Используется только атомарный обмен целым числом и системный вызов write,
		 * что является async-signal-safe операцией.
		 */
		static atomic_int32_t pipefd{-1};

		/**
		 * В режиме релиза требуется определять фатальные синхронные сигналы для их
		 * корректной обработки внутри обработчика (приостановка сбойного потока).
		 */
		#if !DEBUG_MODE
			/**
			 * @brief Функция проверки сигнала на принадлежность к синхронным фатальным
			 *
			 * @note К фатальным относятся сигналы ошибок выполнения, которые при возврате из
			 *       обработчика приводят к повторному исполнению сбойной инструкции.
			 *
			 * @param signal номер проверяемого сигнала
			 * @return        признак фатального синхронного сигнала
			 */
			static bool fatal(const int32_t signal) noexcept {
				// Определяем принадлежность сигнала к фатальным синхронным
				return (
					(signal == SIGILL) ||
					(signal == SIGFPE) ||
					(signal == SIGBUS) ||
					(signal == SIGABRT) ||
					(signal == SIGSEGV)
				);
			}
		#endif

		/**
		 * @brief Функция фильтр перехватчика сигналов
		 *
		 * @details Выполняет только async-signal-safe операции: формирует полезную нагрузку
		 *          и неблокирующе записывает её в самопайп.
		 *          Вся остальная обработка выполняется в рабочем потоке.
		 *
		 * @param sig  номер сигнала полученного системой
		 * @param info объект информации полученный системой
		 * @param ctx  передаваемый внутренний контекст
		 */
		static void handler(const int32_t sig, siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
			// Запоминаем текущее значение errno, чтобы не повредить его в прерванном коде
			const int32_t error = errno;
			// Получаем дескриптор записи самопайпа
			const int32_t fd = pipefd.load(std::memory_order_acquire);
			// Если самопайп активен
			if(fd >= 0){
				// Формируем полезную нагрузку
				payload_t payload{};
				// Устанавливаем номер сигнала
				payload.sig = sig;
				// Если информация о сигнале получена
				if(info != nullptr){
					// Запоминаем идентификатор процесса-отправителя
					payload.pid = info->si_pid;
					// Запоминаем идентификатор пользователя-отправителя
					payload.uid = info->si_uid;
				}
				// Выполняем неблокирующую запись полезной нагрузки в самопайп
				ssize_t bytes = 0;
				/**
				 * Повторяем запись при прерывании системным вызовом
				 */
				do {
					// Записываем полезную нагрузку
					bytes = ::write(fd, &payload, sizeof(payload));
				/**
				 * Если запись прервана сигналом, повторяем попытку
				 */
				} while((bytes < 0) && (errno == EINTR));
			}
			/**
			 * В режиме отладки фатальные сигналы регистрируются с флагом SA_RESETHAND,
			 * поэтому после возврата из обработчика повторное исполнение сбойной инструкции
			 * приводит к созданию core dump для последующего анализа.
			 *
			 * В режиме релиза core dump не создаётся, поэтому фатальный сигнал обрабатывается
			 * как обычный: рабочий поток выполнит функцию обратного вызова, что позволит
			 * приложению корректно завершить работу и сообщить об ошибке пользователю.
			 * Чтобы исключить бесконечное повторное срабатывание сбойной инструкции, сбойный
			 * поток приостанавливается до завершения процесса (pause является async-signal-safe).
			 */
			#if !DEBUG_MODE
				// Если получен фатальный синхронный сигнал
				if(fatal(sig)){
					// Восстанавливаем значение errno
					errno = error;
					/**
					 * Приостанавливаем сбойный поток до завершения процесса рабочим потоком.
					 * Без приостановки сбойная инструкция исполнялась бы повторно бесконечно.
					 */
					for(;;)
						// Ожидаем доставки сигнала (поток будет снят при завершении процесса)
						::pause();
				}
			#endif
			// Восстанавливаем значение errno
			errno = error;
		}
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		/**
		 * @brief Структура глобального контекста синхронизации рабочего потока
		 *
		 * @details На MS Windows обработчик сигнала имеет сигнатуру void(int) и не позволяет
		 *          передать контекст, поэтому синхронизация выносится в область модуля.
		 */
		static struct Self {
			// Мьютекс защиты очереди сигналов
			std::mutex mtx;
			// Условная переменная пробуждения рабочего потока
			std::condition_variable cv;
			// Очередь полученных сигналов
			std::queue <int32_t> queue;
			// Флаг активности перехвата сигналов
			std::atomic_bool active{false};
		} self;

		/**
		 * @brief Функция фильтр перехватчика сигналов
		 *
		 * @param sig номер сигнала полученного системой
		 */
		static void handler(const int32_t sig) noexcept {
			// Если перехват сигналов активен
			if(self.active.load(std::memory_order_acquire)){
				{
					// Блокируем доступ к очереди сигналов
					std::lock_guard <std::mutex> lock(self.mtx);
					// Добавляем сигнал в очередь
					self.queue.push(sig);
				}
				// Пробуждаем рабочий поток
				self.cv.notify_one();
			}
		}
	#endif
};

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * @brief Конструктор
	 *
	 */
	awh::Signals::Events::Events() noexcept :
	 sigint(nullptr), sigfpe(nullptr),
	 sigill(nullptr), sigabrt(nullptr),
	 sigterm(nullptr), sigsegv(nullptr) {}
#endif

/**
 * @brief Метод восстановления обработчиков сигналов по умолчанию
 *
 */
void awh::Signals::disarm() noexcept {
	// Если отслеживание сигналов запущено, снимаем флаг
	if(this->_mode.exchange(false, std::memory_order_acq_rel)){
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Формируем структуру восстановления обработчика по умолчанию
			struct sigaction sa{};
			// Заполняем структуру нулями
			::memset(&sa, 0, sizeof(sa));
			// Устанавливаем обработчик по умолчанию
			sa.sa_handler = SIG_DFL;
			// Очищаем маску перехвата
			sigemptyset(&sa.sa_mask);
			// Список перехватываемых сигналов
			const int32_t list[] = {
				SIGINT,  // Сигнал прерывания (interrupt)
				SIGFPE,  // Сигнал ошибки арифметической операции (floating point exception)
				SIGILL,  // Сигнал недопустимой инструкции (illegal instruction)
				SIGBUS,  // Сигнал ошибки шины (bus error)
				SIGABRT, // Сигнал аварийного завершения (abort)
				SIGTERM, // Сигнал убийства приложения (kill)
				SIGSEGV  // Сигнал ошибки сегментации (segmentation fault)
			};
			/**
			 * Восстанавливаем обработчик по умолчанию для каждого сигнала
			 */
			for(auto sig : list)
				// Активируем обработчик по умолчанию
				::sigaction(sig, &sa, nullptr);
		/**
		 * Для операционной системы MS Windows
		 */
		#else
			// Список перехватываемых сигналов
			const int32_t list[] = {
				SIGINT,  // Сигнал прерывания (interrupt)
				SIGFPE,  // Сигнал ошибки арифметической операции (floating point exception)
				SIGILL,  // Сигнал недопустимой инструкции (illegal instruction)
				SIGABRT, // Сигнал аварийного завершения (abort)
				SIGTERM, // Сигнал убийства приложения (kill)
				SIGSEGV  // Сигнал ошибки сегментации (segmentation fault)
			};
			/**
			 * Восстанавливаем обработчик по умолчанию для каждого сигнала
			 */
			for(auto sig : list)
				// Активируем обработчик по умолчанию
				::signal(sig, SIG_DFL);
		#endif
	}
}
/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * @brief Метод рабочего потока асинхронной обработки сигналов
	 *
	 */
	void awh::Signals::worker() noexcept {
		// Полезная нагрузка очередного сигнала
		::signals::payload_t payload{};
		/**
		 * Выполняем чтение сигналов до получения запроса на остановку
		 */
		while(!this->_exit.load(std::memory_order_acquire)){
			// Указатель на текущую позицию записи в полезную нагрузку
			char * ptr = reinterpret_cast <char *> (&payload);
			// Количество байт, которое осталось прочитать
			size_t need = sizeof(payload);
			// Флаг успешного чтения полезной нагрузки
			bool success = true;
			/**
			 * Читаем ровно одну полезную нагрузку фиксированного размера
			 */
			while(need > 0){
				// Выполняем чтение данных из самопайпа
				const ssize_t bytes = ::read(this->_pipe[0], ptr, need);
				// Если данные прочитаны
				if(bytes > 0){
					// Смещаем указатель записи
					ptr += bytes;
					// Уменьшаем количество оставшихся байт
					need -= static_cast <size_t> (bytes);
				// Если самопайп закрыт
				} else if(bytes == 0) {
					// Помечаем чтение неуспешным
					success = false;
					// Выходим из цикла чтения
					break;
				// Если произошла ошибка
				} else {
					// Если чтение прервано сигналом, повторяем попытку
					if(errno == EINTR)
						// Продолжаем чтение
						continue;
					// Помечаем чтение неуспешным
					success = false;
					// Выходим из цикла чтения
					break;
				}
			}
			// Если получен запрос на остановку, выходим из цикла
			if(this->_exit.load(std::memory_order_acquire))
				// Завершаем работу рабочего потока
				break;
			// Если чтение завершилось неуспешно, выходим из цикла
			if(!success)
				// Завершаем работу рабочего потока
				break;
			// Если получен служебный сигнал пробуждения, пропускаем его
			if(payload.sig == 0)
				// Переходим к следующей итерации
				continue;
			// Выполняем обработку полученного сигнала
			this->process(payload.sig, payload.pid, payload.uid);
		}
	}
	/**
	 * @brief Метод обработки полученного сигнала вне контекста обработчика
	 *
	 * @param sig номер полученного сигнала
	 * @param pid идентификатор процесса-отправителя
	 * @param uid идентификатор пользователя-отправителя
	 */
	void awh::Signals::process(const int32_t sig, const pid_t pid, const uid_t uid) noexcept {
		// Если произошло убийство приложения и установлен объект логирования
		if((sig == SIGTERM) && (this->_log != nullptr)){
			// Буфер для данных
			long size = ::sysconf(_SC_GETPW_R_SIZE_MAX);
			// Если размер буфера не определён
			if(size == -1)
				// Устанавливаем размер буфера по умолчанию
				size = 0x4000;
			// Структуры для получения результата
			struct passwd pwd{};
			// Результат получения названия пользователя
			struct passwd * result = nullptr;
			// Создаём буфер
			vector <char> buffer(size, 0);
			// Определяем название пользователя
			::getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result);
			// Название пользователя
			const char * user = nullptr;
			// Если название пользователя определено
			if(result != nullptr)
				// Устанавливаем название пользователя
				user = result->pw_name;
			// Создаём объект дознавателя
			awh::procre_t procre(this->_log);
			// Выполняем получение названия процесса
			const string & name = procre.name(pid);
			// Если название приложения получено
			if(!name.empty()){
				// Если название пользователя получено
				if(user != nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем в лог сообщение в лог
						this->_log->debug("Killer detected APP=%s, USER=%s", __PRETTY_FUNCTION__, make_tuple(sig, pid, uid), awh::log_t::flag_t::WARNING, name.c_str(), user);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем в лог сообщение в лог
						this->_log->print("Killer detected APP=%s, USER=%s", awh::log_t::flag_t::WARNING, name.c_str(), user);
					#endif
				// Если имя пользователя не получено
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем в лог сообщение в лог
						this->_log->debug("Killer detected APP=%s, UID=%u", __PRETTY_FUNCTION__, make_tuple(sig, pid, uid), awh::log_t::flag_t::WARNING, name.c_str(), uid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем в лог сообщение в лог
						this->_log->print("Killer detected APP=%s, UID=%u", awh::log_t::flag_t::WARNING, name.c_str(), uid);
					#endif
				}
			// Если название приложения не получено
			} else {
				// Если название пользователя получено
				if(user != nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем в лог сообщение в лог
						this->_log->debug("Killer detected PID=%u, USER=%s", __PRETTY_FUNCTION__, make_tuple(sig, pid, uid), awh::log_t::flag_t::WARNING, pid, user);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем в лог сообщение в лог
						this->_log->print("Killer detected PID=%u, USER=%s", awh::log_t::flag_t::WARNING, pid, user);
					#endif
				// Если имя пользователя не получено
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем в лог сообщение в лог
						this->_log->debug("Killer detected PID=%u, UID=%u", __PRETTY_FUNCTION__, make_tuple(sig, pid, uid), awh::log_t::flag_t::WARNING, pid, uid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем в лог сообщение в лог
						this->_log->print("Killer detected PID=%u, UID=%u", awh::log_t::flag_t::WARNING, pid, uid);
					#endif
				}
			}
		}
		// Снимаем перехватчики, чтобы повторный сигнал обрабатывался системой по умолчанию
		this->disarm();
		// Если функция обратного вызова установлена, выполняем её
		if(this->_callback != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback(sig);
	}
/**
 * Для операционной системы MS Windows
 */
#else
	/**
	 * @brief Метод рабочего потока асинхронной обработки сигналов
	 *
	 */
	void awh::Signals::worker() noexcept {
		/**
		 * Выполняем обработку сигналов до получения запроса на остановку
		 */
		for(;;){
			// Полученный сигнал
			int32_t signal = 0;
			{
				// Формируем блокировку очереди сигналов
				std::unique_lock <std::mutex> lock(::signals::self.mtx);
				// Ожидаем поступления сигнала либо запроса на остановку
				::signals::self.cv.wait(lock, [this]{
					// Пробуждаемся при запросе остановки или наличии сигнала в очереди
					return (this->_exit.load(std::memory_order_acquire) || !::signals::self.queue.empty());
				});
				// Если получен запрос на остановку и очередь пуста, выходим
				if(this->_exit.load(std::memory_order_acquire) && ::signals::self.queue.empty())
					// Завершаем работу рабочего потока
					break;
				// Извлекаем сигнал из очереди
				signal = ::signals::self.queue.front();
				// Удаляем сигнал из очереди
				::signals::self.queue.pop();
			}
			// Если получен запрос на остановку, выходим
			if(this->_exit.load(std::memory_order_acquire))
				// Завершаем работу рабочего потока
				break;
			// Выполняем обработку полученного сигнала
			this->process(signal);
		}
	}
	/**
	 * @brief Метод обработки полученного сигнала вне контекста обработчика
	 *
	 * @param sig номер полученного сигнала
	 */
	void awh::Signals::process(const int32_t sig) noexcept {
		// Снимаем перехватчики, чтобы повторный сигнал обрабатывался системой по умолчанию
		this->disarm();
		// Если функция обратного вызова установлена, выполняем её
		if(this->_callback != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback(sig);
	}
#endif
/**
 * @brief Метод остановки обработки сигналов
 *
 */
void awh::Signals::stop() noexcept {
	// Формируем блокировку операций управления
	std::lock_guard <std::mutex> lock(this->_mtx);
	// Восстанавливаем обработчики сигналов по умолчанию
	this->disarm();
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Деактивируем самопайп для обработчика сигналов
		::signals::pipefd.store(-1, std::memory_order_release);
		// Если рабочий поток запущен
		if(this->_worker.joinable()){
			// Устанавливаем флаг запроса остановки рабочего потока
			this->_exit.store(true, std::memory_order_release);
			// Если дескриптор записи самопайпа открыт
			if(this->_pipe[1] >= 0){
				// Формируем служебную полезную нагрузку пробуждения
				::signals::payload_t payload{};
				// Пробуждаем рабочий поток служебной записью (неблокирующей)
				const ssize_t bytes = ::write(this->_pipe[1], &payload, sizeof(payload));
				// Подавляем предупреждение о неиспользуемом результате
				(void) bytes;
			}
			// Если остановка выполняется не из рабочего потока
			if(this->_worker.get_id() != std::this_thread::get_id()){
				// Дожидаемся завершения рабочего потока
				this->_worker.join();
				// Закрываем дескриптор чтения самопайпа
				if(this->_pipe[0] >= 0){
					// Закрываем дескриптор
					::close(this->_pipe[0]);
					// Сбрасываем дескриптор
					this->_pipe[0] = -1;
				}
				// Закрываем дескриптор записи самопайпа
				if(this->_pipe[1] >= 0){
					// Закрываем дескриптор
					::close(this->_pipe[1]);
					// Сбрасываем дескриптор
					this->_pipe[1] = -1;
				}
			}
			/**
			 * Если остановка выполняется из рабочего потока, завершение и закрытие
			 * дескрипторов будет выполнено деструктором либо повторным вызовом stop()
			 * из другого потока (рабочий поток выйдет самостоятельно по флагу _exit).
			 */
		}
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		// Деактивируем перехват сигналов для обработчика
		::signals::self.active.store(false, std::memory_order_release);
		// Если рабочий поток запущен
		if(this->_worker.joinable()){
			{
				// Формируем блокировку очереди сигналов
				std::lock_guard <std::mutex> lk(::signals::self.mtx);
				// Устанавливаем флаг запроса остановки рабочего потока
				this->_exit.store(true, std::memory_order_release);
			}
			// Пробуждаем рабочий поток
			::signals::self.cv.notify_all();
			// Если остановка выполняется не из рабочего потока
			if(this->_worker.get_id() != std::this_thread::get_id())
				// Дожидаемся завершения рабочего потока
				this->_worker.join();
		}
	#endif
}
/**
 * @brief Метод запуска обработки сигналов
 *
 */
void awh::Signals::start() noexcept {
	// Формируем блокировку операций управления
	std::lock_guard <std::mutex> lock(this->_mtx);
	// Если отслеживание сигналов уже запущено, выходим
	if(this->_mode.load(std::memory_order_acquire))
		// Прекращаем дальнейшую работу
		return;
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Присоединяем рабочий поток, оставшийся от остановки из колбэка
		if(this->_worker.joinable()){
			// Устанавливаем флаг запроса остановки рабочего потока
			this->_exit.store(true, std::memory_order_release);
			// Если дескриптор записи самопайпа открыт, пробуждаем рабочий поток
			if(this->_pipe[1] >= 0){
				// Формируем служебную полезную нагрузку пробуждения
				::signals::payload_t payload{};
				// Пробуждаем рабочий поток служебной записью
				const ssize_t bytes = ::write(this->_pipe[1], &payload, sizeof(payload));
				// Подавляем предупреждение о неиспользуемом результате
				(void) bytes;
			}
			// Дожидаемся завершения рабочего потока
			this->_worker.join();
			// Закрываем дескриптор чтения самопайпа
			if(this->_pipe[0] >= 0){
				// Закрываем дескриптор
				::close(this->_pipe[0]);
				// Сбрасываем дескриптор
				this->_pipe[0] = -1;
			}
			// Закрываем дескриптор записи самопайпа
			if(this->_pipe[1] >= 0){
				// Закрываем дескриптор
				::close(this->_pipe[1]);
				// Сбрасываем дескриптор
				this->_pipe[1] = -1;
			}
		}
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		// Присоединяем рабочий поток, оставшийся от остановки из колбэка
		if(this->_worker.joinable()){
			{
				// Формируем блокировку очереди сигналов
				std::lock_guard <std::mutex> lk(::signals::self.mtx);
				// Устанавливаем флаг запроса остановки рабочего потока
				this->_exit.store(true, std::memory_order_release);
			}
			// Пробуждаем рабочий поток
			::signals::self.cv.notify_all();
			// Дожидаемся завершения рабочего потока
			this->_worker.join();
		}
	#endif
	// Сбрасываем флаг запроса остановки рабочего потока
	this->_exit.store(false, std::memory_order_release);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Создаём самопайп для передачи сигналов рабочему потоку
		if(::pipe(this->_pipe) != 0){
			// Если создать самопайп не удалось, выводим сообщение об ошибке
			if(this->_log != nullptr)
				// Записываем в лог сообщение об ошибке
				this->_log->print("Signal pipe creation failed: %s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
			// Прекращаем дальнейшую работу
			return;
		}
		// Устанавливаем неблокирующий режим записи, чтобы обработчик не блокировался
		::fcntl(this->_pipe[1], F_SETFL, ::fcntl(this->_pipe[1], F_GETFL, 0) | O_NONBLOCK);
		// Устанавливаем флаг закрытия дескрипторов при exec
		::fcntl(this->_pipe[0], F_SETFD, ::fcntl(this->_pipe[0], F_GETFD, 0) | FD_CLOEXEC);
		::fcntl(this->_pipe[1], F_SETFD, ::fcntl(this->_pipe[1], F_GETFD, 0) | FD_CLOEXEC);
		// Публикуем дескриптор записи для обработчика сигналов
		::signals::pipefd.store(this->_pipe[1], std::memory_order_release);
		// Запускаем рабочий поток обработки сигналов
		this->_worker = std::thread(&awh::Signals::worker, this);
		// Выполняем игнорирование сигнала SIGPIPE
		::signal(SIGPIPE, SIG_IGN);
		/**
		 * Заполняем структуры перехватчиков нулями.
		 *
		 * Это гарантирует, что все поля, кроме явно устанавливаемых, будут инициализированы нулями,
		 * что предотвращает неопределённое поведение при использовании структуры в системных вызовах.
		 */
		::memset(&this->_events.sigint, 0, sizeof(this->_events.sigint));
		::memset(&this->_events.sigfpe, 0, sizeof(this->_events.sigfpe));
		::memset(&this->_events.sigill, 0, sizeof(this->_events.sigill));
		::memset(&this->_events.sigbus, 0, sizeof(this->_events.sigbus));
		::memset(&this->_events.sigabrt, 0, sizeof(this->_events.sigabrt));
		::memset(&this->_events.sigterm, 0, sizeof(this->_events.sigterm));
		::memset(&this->_events.sigsegv, 0, sizeof(this->_events.sigsegv));
		/**
		 * Устанавливаем функцию перехватчика событий для каждого сигнала
		 */
		this->_events.sigint.sa_sigaction  = ::signals::handler;
		this->_events.sigfpe.sa_sigaction  = ::signals::handler;
		this->_events.sigill.sa_sigaction  = ::signals::handler;
		this->_events.sigbus.sa_sigaction  = ::signals::handler;
		this->_events.sigabrt.sa_sigaction = ::signals::handler;
		this->_events.sigterm.sa_sigaction = ::signals::handler;
		this->_events.sigsegv.sa_sigaction = ::signals::handler;
		/**
		 * Для асинхронных сигналов используем SA_RESTART для перезапуска прерванных вызовов
		 */
		this->_events.sigint.sa_flags  = (SA_RESTART | SA_SIGINFO);
		this->_events.sigterm.sa_flags = (SA_RESTART | SA_SIGINFO);
		/**
		 * Флаги для синхронных фатальных сигналов (SIGFPE, SIGILL, SIGBUS, SIGSEGV, SIGABRT).
		 *
		 * В режиме отладки используем SA_RESETHAND: после первой обработки восстанавливается
		 * обработчик по умолчанию, и повторное исполнение сбойной инструкции создаёт core dump.
		 *
		 * В режиме релиза core dump не создаётся, поэтому обработчик оставляем установленным,
		 * а сбойный поток приостанавливается внутри обработчика — это позволяет рабочему потоку
		 * корректно завершить работу приложения через функцию обратного вызова.
		 */
		#if DEBUG_MODE
			// Флаги фатальных сигналов в режиме отладки
			const int32_t flags = (SA_SIGINFO | SA_RESETHAND);
		#else
			// Флаги фатальных сигналов в режиме релиза
			const int32_t flags = SA_SIGINFO;
		#endif
		/**
		 * Устанавливаем флаги перехвата для фатальных сигналов
		 */
		this->_events.sigfpe.sa_flags  = flags;
		this->_events.sigill.sa_flags  = flags;
		this->_events.sigbus.sa_flags  = flags;
		this->_events.sigabrt.sa_flags = flags;
		this->_events.sigsegv.sa_flags = flags;
		/**
		 * Очищаем маски перехвата сигналов, так как обработчик должен быть прерван только при получении соответствующего сигнала,
		 * и не должен быть прерван другими сигналами, которые могут возникать в процессе обработки (например, при выполнении функции обратного вызова).
		 */
		sigemptyset(&this->_events.sigint.sa_mask);
		sigemptyset(&this->_events.sigfpe.sa_mask);
		sigemptyset(&this->_events.sigill.sa_mask);
		sigemptyset(&this->_events.sigbus.sa_mask);
		sigemptyset(&this->_events.sigabrt.sa_mask);
		sigemptyset(&this->_events.sigterm.sa_mask);
		sigemptyset(&this->_events.sigsegv.sa_mask);
		// Устанавливаем флаг запуска отслеживания сигналов
		this->_mode.store(true, std::memory_order_release);
		/**
		 * Активируем перехватчики событий для каждого сигнала.
		 *
		 * В случае ошибки активации обработчика для сигнала, система будет использовать обработчик по умолчанию,
		 * что может привести к завершению процесса при получении этого сигнала.
		 * Поэтому важно, чтобы функция обработки сигналов была максимально надёжной и не вызывала ошибок,
		 * так как это может повлиять на стабильность приложения.
		 */
		::sigaction(SIGINT, &this->_events.sigint, nullptr);
		::sigaction(SIGFPE, &this->_events.sigfpe, nullptr);
		::sigaction(SIGILL, &this->_events.sigill, nullptr);
		::sigaction(SIGBUS, &this->_events.sigbus, nullptr);
		::sigaction(SIGABRT, &this->_events.sigabrt, nullptr);
		::sigaction(SIGTERM, &this->_events.sigterm, nullptr);
		::sigaction(SIGSEGV, &this->_events.sigsegv, nullptr);
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		// Активируем перехват сигналов для обработчика
		::signals::self.active.store(true, std::memory_order_release);
		// Запускаем рабочий поток обработки сигналов
		this->_worker = std::thread(&awh::Signals::worker, this);
		// Устанавливаем флаг запуска отслеживания сигналов
		this->_mode.store(true, std::memory_order_release);
		// Создаём обработчик сигнала для SIGINT
		this->_events.sigint = ::signal(SIGINT, ::signals::handler);
		// Создаём обработчик сигнала для SIGFPE
		this->_events.sigfpe = ::signal(SIGFPE, ::signals::handler);
		// Создаём обработчик сигнала для SIGILL
		this->_events.sigill = ::signal(SIGILL, ::signals::handler);
		// Создаём обработчик сигнала для SIGABRT
		this->_events.sigabrt = ::signal(SIGABRT, ::signals::handler);
		// Создаём обработчик сигнала для SIGTERM
		this->_events.sigterm = ::signal(SIGTERM, ::signals::handler);
		// Создаём обработчик сигнала для SIGSEGV
		this->_events.sigsegv = ::signal(SIGSEGV, ::signals::handler);
	#endif
}
/**
 * @brief Метод установки функции обратного вызова, которая должна сработать при получении сигнала
 *
 * @param callback функция обратного вызова
 */
void awh::Signals::on(function <void (const int32_t)> callback) noexcept {
	// Формируем блокировку операций управления
	std::lock_guard <std::mutex> lock(this->_mtx);
	// Выполняем установку функции обратного вызова
	this->_callback = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Signals::Signals(const fmk_t * fmk, const log_t * log) noexcept :
 _mode(false), _exit(false), _fmk(fmk), _log(log), _callback(nullptr) {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Инициализируем дескрипторы самопайпа закрытыми значениями
		this->_pipe[0] = -1;
		// Инициализируем дескриптор записи самопайпа закрытым значением
		this->_pipe[1] = -1;
	#endif
}
/**
 * @brief Деструктор
 *
 */
awh::Signals::~Signals() noexcept {
	// Останавливаем работу отслеживания событий
	this->stop();
}
