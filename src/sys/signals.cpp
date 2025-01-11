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
 * @copyright: Copyright © 2024
 */

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Стандартные модули
	 */
	#include <pwd.h>
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
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Объект фреймворка
	 */
	const static awh::fmk_t * awhFmk = nullptr;
	/**
	 * Объект для работы с логами
	 */
	const static awh::log_t * awhLog = nullptr;
#endif

/**
 * Функция обратного вызова при получении сигнала
 */
static function <void (const int32_t)> callbackFn = nullptr;

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * signalHandler Функция фильтр перехватчика сигналов
	 * @param signal номер сигнала полученного системой
	 * @param info   объект информации полученный системой
	 * @param ctx    передаваемый внутренний контекст
	 */
	static void signalHandler(int32_t signal, siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
		// Если функция обратного вызова установлена, выводим её
		if(callbackFn != nullptr){
			// Если произошло убийство приложения
			if((info != nullptr) && (signal == SIGTERM)){
				// Создаём объект дознавателя
				awh::igtr_t igtr;
				// Название пользователя
				const char * user = nullptr;
				// Определяем название пользователя
				const auto * pwd = ::getpwuid(info->si_uid);
				// Если название пользователя определено
				if(pwd != nullptr)
					// Устанавливаем название пользователя
					user = pwd->pw_name;
				// Выполняем получение названия процесса
				const string & app = igtr.inquiry(info->si_pid);
				// Если название приложения получено
				if(!app.empty()){
					// Если название пользователя получено
					if(user != nullptr)
						// Выводим сообщение в лог
						awhLog->print("Killer detected APP=%s, USER=%s", awh::log_t::flag_t::WARNING, app.c_str(), user);
					// Если имя пользователя не получено
					else awhLog->print("Killer detected APP=%s, UID=%u", awh::log_t::flag_t::WARNING, app.c_str(), info->si_uid);
				// Если название приложения не получено
				} else {
					// Если название пользователя получено
					if(user != nullptr)
						// Выводим сообщение в лог
						awhLog->print("Killer detected PID=%u, USER=%s", awh::log_t::flag_t::WARNING, info->si_pid, user);
					// Если имя пользователя не получено
					else awhLog->print("Killer detected PID=%u, UID=%u", awh::log_t::flag_t::WARNING, info->si_pid, info->si_uid);
				}
			}
			// Выполняем функцию обратного вызова
			std::thread(callbackFn, signal).detach();
		}
	}
/**
 * Если операционной системой является MS Windows
 */
#else
	/**
	 * signalHandler Функция фильтр перехватчика сигналов
	 * @param signal номер сигнала полученного системой
	 */
	static void signalHandler(int32_t signal) noexcept {
		// Если функция обратного вызова установлена, выводим её
		if(callbackFn != nullptr)
			// Выполняем функцию обратного вызова
			std::thread(callbackFn, signal).detach();
	}
#endif

/**
 * callback Функция обратного вызова
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
 * stop Метод остановки обработки сигналов
 */
void awh::Signals::stop() noexcept {
	// Если отслеживание сигналов уже запущено
	if(this->_mode){
		// Снимаем флаг запуска отслеживания сигналов
		this->_mode = !this->_mode;
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
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
		 * Если операционной системой является MS Windows
		 */
		#else
			// Создаём обработчик сигнала для SIGINT
			this->_ev.sigInt = signal(SIGINT, nullptr);
			// Создаём обработчик сигнала для SIGFPE
			this->_ev.sigFpe = signal(SIGFPE, nullptr);
			// Создаём обработчик сигнала для SIGILL
			this->_ev.sigIll = signal(SIGILL, nullptr);
			// Создаём обработчик сигнала для SIGABRT
			this->_ev.sigAbrt = signal(SIGABRT, nullptr);
			// Создаём обработчик сигнала для SIGTERM
			this->_ev.sigTerm = signal(SIGTERM, nullptr);
			// Создаём обработчик сигнала для SIGSEGV
			this->_ev.sigSegv = signal(SIGSEGV, nullptr);
		#endif
	}
}
/**
 * start Метод запуска обработки сигналов
 */
void awh::Signals::start() noexcept {
	// Если отслеживание сигналов ещё не запущено
	if(!this->_mode){
		// Устанавливаем флаг запуска отслеживания сигналов
		this->_mode = !this->_mode;
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
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
		 * Если операционной системой является MS Windows
		 */
		#else
			// Создаём обработчик сигнала для SIGINT
			this->_ev.sigInt = signal(SIGINT, signalHandler);
			// Создаём обработчик сигнала для SIGFPE
			this->_ev.sigFpe = signal(SIGFPE, signalHandler);
			// Создаём обработчик сигнала для SIGILL
			this->_ev.sigIll = signal(SIGILL, signalHandler);
			// Создаём обработчик сигнала для SIGABRT
			this->_ev.sigAbrt = signal(SIGABRT, signalHandler);
			// Создаём обработчик сигнала для SIGTERM
			this->_ev.sigTerm = signal(SIGTERM, signalHandler);
			// Создаём обработчик сигнала для SIGSEGV
			this->_ev.sigSegv = signal(SIGSEGV, signalHandler);
		#endif
	}
}
/**
 * on Метод установки функции обратного вызова, которая должна сработать при получении сигнала
 * @param callback функция обратного вызова
 */
void awh::Signals::on(function <void (const int32_t)> callback) noexcept {
	// Выполняем установку функцию обратного вызова
	this->_callback = callback;
	// Выполняем установки функции обратного вызова
	callbackFn = bind(&sig_t::callback, this, _1);
}
/**
 * Signals Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Signals::Signals([[maybe_unused]] const fmk_t * fmk, [[maybe_unused]] const log_t * log) noexcept : _mode(false), _callback(nullptr) {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Запоминаем объект фреймворка
		awhFmk = fmk;
		// Запоминаем объект для работы с логами
		awhLog = log;
	#endif
}
/**
 * ~Signals Деструктор
 */
awh::Signals::~Signals() noexcept {
	// Останавливаем работу отслеживания событий
	this->stop();
}
