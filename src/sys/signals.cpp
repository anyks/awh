/**
 * @file: signals.cpp
 * @date: 2024-07-06
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

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные модули
	 */
	#include <pwd.h>
	#include <ctime>
	#include <mutex>
	#include <atomic>
	#include <cerrno>
	#include <fcntl.h>
	#include <unistd.h>
	#include <pthread.h>
	/**
	 * Подключаем наши модули
	 */
	#include <sys/investigator.hpp>
#endif

/**
 * Подключаем заголовочный файл
 */
#include <sys/signals.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Структура глобального объекта
 *
 */
static struct Self {
	// Объект фреймворка
	const awh::fmk_t * fmk;
	// Объект для работы с логами
	const awh::log_t * log;
	// Функция обратного вызова при получении сигнала
	function <void (const int32_t)> callback;
	/**
	 * @brief Конструктор
	 *
	 */
	Self() noexcept : fmk(nullptr), log(nullptr), callback(nullptr) {}
} self;

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Намеренное решение: обработчик сигнала выполняет только async-signal-safe действия.
	 *
	 * Раньше прямо в обработчике вызывались getpwuid, чтение /proc через дознавателя,
	 * std::string, вывод в лог и std::thread(...).detach(). Все они берут блокировку
	 * распределителя памяти, и SIGTERM, пришедший пока другой поток держит эту блокировку,
	 * навсегда подвешивал процесс. Теперь обработчик только записывает номер сигнала в
	 * канал (write() безопасен в обработчике), а всю работу делает заранее запущенный
	 * поток-диспетчер. Договор с потребителями сохранён: функция обратного вызова, как и
	 * раньше, выполняется в отдельном обычном потоке (не в контексте сигнала) и получает
	 * номер сигнала.
	 */
	/**
	 * Время ожидания обработки аварийного сигнала: 500 шагов по 10мс (5 секунд)
	 */
	static constexpr uint32_t CRASH_WAIT_STEPS = 500;
	/**
	 * @brief Структура сообщения о сигнале, передаваемого через канал
	 *
	 */
	typedef struct Payload {
		int32_t sig;  // Номер сигнала
		pid_t pid;    // Идентификатор процесса отправителя
		uid_t uid;    // Идентификатор пользователя отправителя
		uint8_t info; // Флаг наличия сведений об отправителе
	} payload_t;
	/**
	 * Сокеты канала передачи сигналов (изменяются только вне обработчика сигнала)
	 */
	static int32_t channel[2] = {-1, -1};
	/**
	 * Сокет канала на запись, доступный обработчику сигнала
	 */
	static std::atomic <int32_t> pipefd{-1};
	/**
	 * Идентификатор процесса, в котором работает поток-диспетчер
	 */
	static std::atomic <pid_t> owner{0};
	/**
	 * Флаг получения аварийного сигнала
	 */
	static std::atomic_bool crashing{false};
	/**
	 * Флаг завершения обработки аварийного сигнала
	 */
	static std::atomic_bool handled{false};
	/**
	 * Мютекс запуска потока-диспетчера
	 */
	static std::mutex launcher;
	/**
	 * @brief Функция проверки является ли сигнал аварийным
	 *
	 * @param signal номер сигнала
	 * @return       результат проверки
	 */
	static bool fatal(const int32_t signal) noexcept {
		// Выводим результат проверки
		return (
			(signal == SIGILL) ||
			(signal == SIGFPE) ||
			(signal == SIGBUS) ||
			(signal == SIGABRT) ||
			(signal == SIGSEGV)
		);
	}
	/**
	 * @brief Функция восстановления стандартного обработчика сигнала (async-signal-safe)
	 *
	 * @param signal номер сигнала
	 */
	static void restore(const int32_t signal) noexcept {
		// Создаём объект перехватчика сигнала
		struct sigaction action;
		// Выполняем зануление структуры перехватчика
		::memset(&action, 0, sizeof(action));
		// Устанавливаем стандартный обработчик сигнала
		action.sa_handler = SIG_DFL;
		// Устанавливаем пустую маску перехвата
		sigemptyset(&action.sa_mask);
		// Активируем стандартный обработчик сигнала
		::sigaction(signal, &action, nullptr);
	}
	/**
	 * @brief Функция вывода в лог сведений об убийце процесса
	 *
	 * @param pid идентификатор процесса отправителя
	 * @param uid идентификатор пользователя отправителя
	 */
	static void killer(const pid_t pid, const uid_t uid) noexcept {
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если объект логирования не установлен
			if(self.log == nullptr)
				// Выходим из функции
				return;
			// Создаём объект дознавателя
			awh::igtr_t igtr;
			// Название пользователя
			const char * user = nullptr;
			// Определяем название пользователя
			const auto * pwd = ::getpwuid(uid);
			// Если название пользователя определено
			if(pwd != nullptr)
				// Устанавливаем название пользователя
				user = pwd->pw_name;
			// Выполняем получение названия процесса
			const string & app = igtr.inquiry(pid);
			// Если название приложения получено
			if(!app.empty()){
				// Если название пользователя получено
				if(user != nullptr)
					// Выводим сообщение в лог
					self.log->print("Killer detected APP=%s, USER=%s", awh::log_t::flag_t::WARNING, app.c_str(), user);
				// Если имя пользователя не получено
				else self.log->print("Killer detected APP=%s, UID=%u", awh::log_t::flag_t::WARNING, app.c_str(), uid);
			// Если название приложения не получено
			} else {
				// Если название пользователя получено
				if(user != nullptr)
					// Выводим сообщение в лог
					self.log->print("Killer detected PID=%u, USER=%s", awh::log_t::flag_t::WARNING, pid, user);
				// Если имя пользователя не получено
				else self.log->print("Killer detected PID=%u, UID=%u", awh::log_t::flag_t::WARNING, pid, uid);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Сведения об убийце не обязательны, продолжаем работу
		}
	}
	/**
	 * @brief Функция потока-диспетчера сигналов
	 *
	 * @param fd сокет канала на чтение
	 */
	static void dispatcher(const int32_t fd) noexcept {
		// Создаём объект сообщения о сигнале
		payload_t payload;
		/**
		 * Выполняем чтение сообщений пока канал открыт
		 */
		for(;;){
			// Выполняем зануление сообщения
			::memset(&payload, 0, sizeof(payload));
			// Количество прочитанных байт
			size_t size = 0;
			// Количество байт полученных при чтении
			ssize_t bytes = 0;
			/**
			 * Выполняем чтение сообщения целиком
			 */
			while(size < sizeof(payload)){
				// Выполняем чтение данных из канала
				bytes = ::read(fd, reinterpret_cast <char *> (&payload) + size, sizeof(payload) - size);
				// Если данные прочитаны
				if(bytes > 0)
					// Увеличиваем количество прочитанных байт
					size += static_cast <size_t> (bytes);
				// Если чтение прервано сигналом, повторяем попытку
				else if((bytes < 0) && (errno == EINTR))
					// Продолжаем чтение
					continue;
				// Канал закрыт или произошла ошибка, завершаем работу потока
				else return;
			}
			// Если произошло убийство приложения и сведения об отправителе есть
			if((payload.sig == SIGTERM) && (payload.info > 0))
				// Выводим в лог сведения об убийце
				killer(payload.pid, payload.uid);
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				// Получаем копию функции обратного вызова
				function <void (const int32_t)> callback = self.callback;
				// Если функция обратного вызова установлена
				if(callback != nullptr){
					// Получаем номер сигнала
					const int32_t signal = payload.sig;
					/**
					 * Выполняем функцию обратного вызова в отдельном потоке, как и раньше:
					 * потребители (ACU, сайты) останавливают в ней сервер под мютексом
					 */
					std::thread([callback, signal]() noexcept -> void {
						// Выполняем функцию обратного вызова
						callback(signal);
						// Если сигнал аварийный
						if(fatal(signal))
							// Сообщаем обработчику, что аварийный сигнал обработан
							handled.store(true);
					}).detach();
				// Если сигнал аварийный
				} else if(fatal(payload.sig))
					// Сообщаем обработчику, что аварийный сигнал обработан
					handled.store(true);
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception &) {
				// Если сигнал аварийный
				if(fatal(payload.sig))
					// Сообщаем обработчику, что ждать больше нечего
					handled.store(true);
			}
		}
	}
	/**
	 * @brief Функция запуска потока-диспетчера в текущем процессе
	 *
	 * @return результат запуска
	 */
	static bool launch() noexcept {
		// Если поток-диспетчер уже работает в текущем процессе
		if(owner.load() == ::getpid())
			// Сообщаем, что диспетчер запущен
			return true;
		// Если канал уже был создан (достался от родительского процесса)
		if(channel[0] != -1){
			// Выполняем закрытие канала на чтение
			::close(channel[0]);
			// Выполняем закрытие канала на запись
			::close(channel[1]);
			// Сбрасываем сокеты канала
			channel[0] = channel[1] = -1;
		}
		// Сбрасываем сокет канала для обработчика
		pipefd.store(-1);
		// Сбрасываем флаг получения аварийного сигнала
		crashing.store(false);
		// Сбрасываем флаг завершения обработки аварийного сигнала
		handled.store(false);
		// Выполняем создание канала
		if(::pipe(channel) != 0){
			// Сбрасываем сокеты канала
			channel[0] = channel[1] = -1;
			// Сообщаем, что диспетчер не запущен
			return false;
		}
		// Делаем канал на запись неблокирующим (обработчик сигнала не должен зависать на записи)
		::fcntl(channel[1], F_SETFL, ::fcntl(channel[1], F_GETFL, 0) | O_NONBLOCK);
		// Запрещаем наследование канала на чтение при запуске других программ
		::fcntl(channel[0], F_SETFD, ::fcntl(channel[0], F_GETFD, 0) | FD_CLOEXEC);
		// Запрещаем наследование канала на запись при запуске других программ
		::fcntl(channel[1], F_SETFD, ::fcntl(channel[1], F_GETFD, 0) | FD_CLOEXEC);
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем запуск потока-диспетчера
			std::thread(&dispatcher, channel[0]).detach();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем закрытие канала на чтение
			::close(channel[0]);
			// Выполняем закрытие канала на запись
			::close(channel[1]);
			// Сбрасываем сокеты канала
			channel[0] = channel[1] = -1;
			// Сообщаем, что диспетчер не запущен
			return false;
		}
		// Устанавливаем сокет канала для обработчика
		pipefd.store(channel[1]);
		// Запоминаем процесс, в котором работает диспетчер
		owner.store(::getpid());
		// Сообщаем, что диспетчер запущен
		return true;
	}
	/**
	 * @brief Функция перезапуска потока-диспетчера в дочернем процессе после fork()
	 *
	 * После fork() в дочернем процессе остаётся только вызвавший поток, диспетчера там нет,
	 * а канал общий с родителем: без перезапуска сигналы дочернего процесса читал бы
	 * диспетчер родителя. Дочерние процессы кластера сигналы не перезапускают, поэтому
	 * новый канал и новый диспетчер создаются здесь
	 */
	static void reborn() noexcept {
		// Если диспетчер был запущен в родительском процессе
		if(channel[0] != -1)
			// Выполняем запуск нового диспетчера
			launch();
	}
	/**
	 * @brief Функция фильтр перехватчика сигналов
	 *
	 * @param signal номер сигнала полученного системой
	 * @param info   объект информации полученный системой
	 * @param ctx    передаваемый внутренний контекст
	 */
	static void signalHandler(int32_t signal, siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
		// Запоминаем код ошибки прерванного потока
		const int32_t error = errno;
		// Получаем сокет канала на запись
		const int32_t fd = pipefd.load();
		// Если в текущем процессе работает диспетчер сигналов
		if((fd >= 0) && (owner.load() == ::getpid())){
			// Создаём объект сообщения о сигнале
			payload_t payload;
			// Выполняем зануление сообщения
			::memset(&payload, 0, sizeof(payload));
			// Устанавливаем номер сигнала
			payload.sig = signal;
			// Если сведения об отправителе получены
			if(info != nullptr){
				// Устанавливаем флаг наличия сведений
				payload.info = 1;
				// Устанавливаем идентификатор процесса отправителя
				payload.pid = info->si_pid;
				// Устанавливаем идентификатор пользователя отправителя
				payload.uid = info->si_uid;
			}
			// Если сигнал аварийный
			if(fatal(signal)){
				// Если это первый аварийный сигнал, отправляем его диспетчеру
				if(!crashing.exchange(true)){
					// Выполняем запись сообщения в канал
					while((::write(fd, &payload, sizeof(payload)) < 0) && (errno == EINTR));
				}
				/**
				 * Намеренное решение: аварийный поток ждёт обработки (не дольше 5 секунд).
				 * Раньше обработчик сразу возвращался, сбойная инструкция повторялась,
				 * и процесс умирал раньше, чем функция обратного вызова успевала сработать
				 */
				for(uint32_t i = 0; (i < CRASH_WAIT_STEPS) && !handled.load(); i++){
					// Время ожидания одного шага 10мс
					struct timespec delay = {0, 10000000L};
					// Выполняем ожидание
					::nanosleep(&delay, nullptr);
				}
				/**
				 * Восстанавливаем стандартный обработчик: при повторе сбойной инструкции процесс
				 * завершится с дампом ядра, даже если диспетчер не смог отработать
				 */
				restore(signal);
			// Выполняем запись сообщения в канал
			} else while((::write(fd, &payload, sizeof(payload)) < 0) && (errno == EINTR));
		// Если диспетчера сигналов нет
		} else {
			// Восстанавливаем стандартный обработчик сигнала
			restore(signal);
			// Выполняем стандартное действие для сигнала
			::raise(signal);
		}
		// Восстанавливаем код ошибки прерванного потока
		errno = error;
	}
/**
 * Для операционной системы MS Windows
 */
#else
	/**
	 * @brief Функция фильтр перехватчика сигналов
	 *
	 * @param signal номер сигнала полученного системой
	 */
	static void signalHandler(int32_t signal) noexcept {
		// Если функция обратного вызова установлена, выводим её
		if(self.callback != nullptr)
			// Выполняем функцию обратного вызова
			std::thread(self.callback, signal).detach();
	}
#endif

/**
 * @brief Функция обратного вызова
 *
 * @param sig идентификатор сигнала
 */
void awh::Signals::callback(const int32_t sig) noexcept {
	// Выполняем остановку всех сотальных сигналов
	this->stop();
	// Если функция обратного вызова установлена, выводим её
	if(this->_callback != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback(sig);
}
/**
 * @brief Метод остановки обработки сигналов
 *
 */
void awh::Signals::stop() noexcept {
	// Если отслеживание сигналов уже запущено
	if(this->_mode){
		// Снимаем флаг запуска отслеживания сигналов
		this->_mode = !this->_mode;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Устанавливаем функцию перехвадчика событий
			this->_ev.sigInt.sa_handler  = SIG_DFL;
			this->_ev.sigFpe.sa_handler  = SIG_DFL;
			this->_ev.sigIll.sa_handler  = SIG_DFL;
			this->_ev.sigBus.sa_handler  = SIG_DFL;
			this->_ev.sigAbrt.sa_handler = SIG_DFL;
			this->_ev.sigTerm.sa_handler = SIG_DFL;
			this->_ev.sigSegv.sa_handler = SIG_DFL;
			// Активируем перехватчик событий
			::sigaction(SIGINT, &this->_ev.sigInt, nullptr);
			::sigaction(SIGFPE, &this->_ev.sigFpe, nullptr);
			::sigaction(SIGILL, &this->_ev.sigIll, nullptr);
			::sigaction(SIGBUS, &this->_ev.sigBus, nullptr);
			::sigaction(SIGABRT, &this->_ev.sigAbrt, nullptr);
			::sigaction(SIGTERM, &this->_ev.sigTerm, nullptr);
			::sigaction(SIGSEGV, &this->_ev.sigSegv, nullptr);
		/**
		 * Для операционной системы MS Windows
		 */
		#else
			// Создаём обработчик сигнала для SIGINT
			this->_ev.sigInt = ::signal(SIGINT, nullptr);
			// Создаём обработчик сигнала для SIGFPE
			this->_ev.sigFpe = ::signal(SIGFPE, nullptr);
			// Создаём обработчик сигнала для SIGILL
			this->_ev.sigIll = ::signal(SIGILL, nullptr);
			// Создаём обработчик сигнала для SIGABRT
			this->_ev.sigAbrt = ::signal(SIGABRT, nullptr);
			// Создаём обработчик сигнала для SIGTERM
			this->_ev.sigTerm = ::signal(SIGTERM, nullptr);
			// Создаём обработчик сигнала для SIGSEGV
			this->_ev.sigSegv = ::signal(SIGSEGV, nullptr);
		#endif
	}
}
/**
 * @brief Метод запуска обработки сигналов
 *
 */
void awh::Signals::start() noexcept {
	// Если отслеживание сигналов ещё не запущено
	if(!this->_mode){
		// Устанавливаем флаг запуска отслеживания сигналов
		this->_mode = !this->_mode;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			{
				// Выполняем блокировку потока
				const lock_guard <std::mutex> lock(launcher);
				// Флаг регистрации перезапуска диспетчера после fork()
				static bool registered = false;
				// Если перезапуск диспетчера после fork() ещё не зарегистрирован
				if(!registered)
					// Выполняем регистрацию перезапуска диспетчера в дочернем процессе
					registered = (::pthread_atfork(nullptr, nullptr, &reborn) == 0);
				// Если поток-диспетчер сигналов не запущен
				if(!launch())
					// Выводим сообщение об ошибке (сигналы будут обрабатываться системой по умолчанию)
					self.log->print("Signal dispatcher cannot be started: %s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
			}
			// Выполняем игнорирование сигналов SIGPIPE
			::signal(SIGPIPE, SIG_IGN);
			// Выполняем зануление структур перехватчиков событий
			::memset(&this->_ev.sigInt, 0, sizeof(this->_ev.sigInt));
			::memset(&this->_ev.sigFpe, 0, sizeof(this->_ev.sigFpe));
			::memset(&this->_ev.sigIll, 0, sizeof(this->_ev.sigIll));
			::memset(&this->_ev.sigBus, 0, sizeof(this->_ev.sigBus));
			::memset(&this->_ev.sigAbrt, 0, sizeof(this->_ev.sigAbrt));
			::memset(&this->_ev.sigTerm, 0, sizeof(this->_ev.sigTerm));
			::memset(&this->_ev.sigSegv, 0, sizeof(this->_ev.sigSegv));
			// Устанавливаем функцию перехвадчика событий
			this->_ev.sigInt.sa_sigaction = signalHandler;
			this->_ev.sigFpe.sa_sigaction = signalHandler;
			this->_ev.sigIll.sa_sigaction = signalHandler;
			this->_ev.sigBus.sa_sigaction = signalHandler;
			this->_ev.sigAbrt.sa_sigaction = signalHandler;
			this->_ev.sigTerm.sa_sigaction = signalHandler;
			this->_ev.sigSegv.sa_sigaction = signalHandler;
			// Устанавливаем флаги перехвата сигналов
			this->_ev.sigInt.sa_flags = SA_RESTART | SA_SIGINFO;
			this->_ev.sigFpe.sa_flags = SA_RESTART | SA_SIGINFO;
			this->_ev.sigIll.sa_flags = SA_RESTART | SA_SIGINFO;
			this->_ev.sigBus.sa_flags = SA_RESTART | SA_SIGINFO;
			this->_ev.sigAbrt.sa_flags = SA_RESTART | SA_SIGINFO;
			this->_ev.sigTerm.sa_flags = SA_RESTART | SA_SIGINFO;
			this->_ev.sigSegv.sa_flags = SA_RESTART | SA_SIGINFO;
			// Устанавливаем маску перехвата
			sigemptyset(&this->_ev.sigInt.sa_mask);
			sigemptyset(&this->_ev.sigFpe.sa_mask);
			sigemptyset(&this->_ev.sigIll.sa_mask);
			sigemptyset(&this->_ev.sigBus.sa_mask);
			sigemptyset(&this->_ev.sigAbrt.sa_mask);
			sigemptyset(&this->_ev.sigTerm.sa_mask);
			sigemptyset(&this->_ev.sigSegv.sa_mask);
			// Активируем перехватчик событий
			::sigaction(SIGINT, &this->_ev.sigInt, nullptr);
			::sigaction(SIGFPE, &this->_ev.sigFpe, nullptr);
			::sigaction(SIGILL, &this->_ev.sigIll, nullptr);
			::sigaction(SIGBUS, &this->_ev.sigBus, nullptr);
			::sigaction(SIGABRT, &this->_ev.sigAbrt, nullptr);
			::sigaction(SIGTERM, &this->_ev.sigTerm, nullptr);
			::sigaction(SIGSEGV, &this->_ev.sigSegv, nullptr);
			// Отправка сигнала для теста
			// ::raise(SIGABRT);
		/**
		 * Для операционной системы MS Windows
		 */
		#else
			// Создаём обработчик сигнала для SIGINT
			this->_ev.sigInt = ::signal(SIGINT, signalHandler);
			// Создаём обработчик сигнала для SIGFPE
			this->_ev.sigFpe = ::signal(SIGFPE, signalHandler);
			// Создаём обработчик сигнала для SIGILL
			this->_ev.sigIll = ::signal(SIGILL, signalHandler);
			// Создаём обработчик сигнала для SIGABRT
			this->_ev.sigAbrt = ::signal(SIGABRT, signalHandler);
			// Создаём обработчик сигнала для SIGTERM
			this->_ev.sigTerm = ::signal(SIGTERM, signalHandler);
			// Создаём обработчик сигнала для SIGSEGV
			this->_ev.sigSegv = ::signal(SIGSEGV, signalHandler);
		#endif
	}
}
/**
 * @brief Метод установки функции обратного вызова, которая должна сработать при получении сигнала
 *
 * @param callback функция обратного вызова
 */
void awh::Signals::on(function <void (const int32_t)> callback) noexcept {
	// Выполняем установку функцию обратного вызова
	this->_callback = callback;
	// Выполняем установки функции обратного вызова
	self.callback = std::bind(&sig_t::callback, this, _1);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Signals::Signals(const fmk_t * fmk, const log_t * log) noexcept : _mode(false), _callback(nullptr) {
	// Запоминаем объект фреймворка
	self.fmk = fmk;
	// Запоминаем объект для работы с логами
	self.log = log;
}
/**
 * @brief Деструктор
 *
 */
awh::Signals::~Signals() noexcept {
	// Останавливаем работу отслеживания событий
	this->stop();
}
