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
 * Стандартные модули
 */
#include <thread>
#include <cstring>
#include <iostream>

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные модули
	 */
	#include <pwd.h>
	#include <vector>
	#include <unistd.h>
	/**
	 * Подключаем наши модули
	 */
	#include <sys/procre.hpp>
#endif

/**
 * Подключаем заголовочный файл
 */
#include <sys/signals.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
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
	explicit Self() noexcept :
	 fmk(nullptr), log(nullptr), callback(nullptr) {}
} self;

/**
 * Инкапсулируем функции обработки сигналов в пространство имён
 */
namespace signals {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		/**
		 * @brief Функция фильтр перехватчика сигналов
		 *
		 * @param signal номер сигнала полученного системой
		 * @param info   объект информации полученный системой
		 * @param ctx    передаваемый внутренний контекст
		 */
		static void handler(int32_t signal, siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
			// Если функция обратного вызова установлена, выводим её
			if(self.callback != nullptr){
				// Если произошло убийство приложения
				if((info != nullptr) && (signal == SIGTERM)){
					// Буфер для данных
					long bufsize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
					// Если размер буфера не определён
					if(bufsize == -1)
						// Устанавливаем размер буфера по умолчанию
						bufsize = 16384;
					// Создаём буфер
					vector <char> buffer(bufsize);
					// Структуры для получения результата
					struct passwd pwd{};
					// Результат получения названия пользователя
					struct passwd * result = nullptr;
					// Определяем название пользователя
					::getpwuid_r(info->si_uid, &pwd, buffer.data(), buffer.size(), &result);
					// Название пользователя
					const char * user = nullptr;
					// Если название пользователя определено
					if(result != nullptr)
						// Устанавливаем название пользователя
						user = result->pw_name;
					// Создаём объект дознавателя
					awh::procre_t procre(self.log);
					// Выполняем получение названия процесса
					const string & name = procre.name(info->si_pid);
					// Если название приложения получено
					if(!name.empty()){
						// Если название пользователя получено
						if(user != nullptr){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение в лог
								self.log->debug("Killer detected APP=%s, USER=%s", __PRETTY_FUNCTION__, make_tuple(signal), awh::log_t::flag_t::WARNING, name.c_str(), user);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение в лог
								self.log->print("Killer detected APP=%s, USER=%s", awh::log_t::flag_t::WARNING, name.c_str(), user);
							#endif
						// Если имя пользователя не получено
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение в лог
								self.log->debug("Killer detected APP=%s, UID=%u", __PRETTY_FUNCTION__, make_tuple(signal), awh::log_t::flag_t::WARNING, name.c_str(), info->si_uid);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение в лог
								self.log->print("Killer detected APP=%s, UID=%u", awh::log_t::flag_t::WARNING, name.c_str(), info->si_uid);
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
								// Выводим сообщение в лог
								self.log->debug("Killer detected PID=%u, USER=%s", __PRETTY_FUNCTION__, make_tuple(signal), awh::log_t::flag_t::WARNING, info->si_pid, user);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение в лог
								self.log->print("Killer detected PID=%u, USER=%s", awh::log_t::flag_t::WARNING, info->si_pid, user);
							#endif
						// Если имя пользователя не получено
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение в лог
								self.log->debug("Killer detected PID=%u, UID=%u", __PRETTY_FUNCTION__, make_tuple(signal), awh::log_t::flag_t::WARNING, info->si_pid, info->si_uid);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение в лог
								self.log->print("Killer detected PID=%u, UID=%u", awh::log_t::flag_t::WARNING, info->si_pid, info->si_uid);
							#endif
						}
					}
				}
				// Выполняем функцию обратного вызова
				std::thread(self.callback, signal).detach();
			}
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
		static void handler(int32_t signal) noexcept {
			// Если функция обратного вызова установлена, выводим её
			if(self.callback != nullptr)
				// Выполняем функцию обратного вызова
				std::thread(self.callback, signal).detach();
		}
	#endif
};

/**
 * @brief Функция обратного вызова
 *
 * @param sig идентификатор сигнала
 */
void awh::Signals::callback(const int32_t sig) noexcept {
	// Выполняем остановку всех остальных сигналов
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
	if(this->_mode.load(std::memory_order_acquire)){
		// Снимаем флаг запуска отслеживания сигналов
		this->_mode.store(false, std::memory_order_release);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Выполняем зануление структур перехватчиков событий
			this->_events.sigint.sa_flags  = 0;
			this->_events.sigfpe.sa_flags  = 0;
			this->_events.sigill.sa_flags  = 0;
			this->_events.sigbus.sa_flags  = 0;
			this->_events.sigabrt.sa_flags = 0;
			this->_events.sigterm.sa_flags = 0;
			this->_events.sigsegv.sa_flags = 0;
			// Устанавливаем функцию перехватчика событий
			this->_events.sigint.sa_handler  = SIG_DFL;
			this->_events.sigfpe.sa_handler  = SIG_DFL;
			this->_events.sigill.sa_handler  = SIG_DFL;
			this->_events.sigbus.sa_handler  = SIG_DFL;
			this->_events.sigabrt.sa_handler = SIG_DFL;
			this->_events.sigterm.sa_handler = SIG_DFL;
			this->_events.sigsegv.sa_handler = SIG_DFL;
			// Активируем перехватчик событий
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
			// Создаём обработчик сигнала для SIGINT
			this->_events.sigint = ::signal(SIGINT, nullptr);
			// Создаём обработчик сигнала для SIGFPE
			this->_events.sigfpe = ::signal(SIGFPE, nullptr);
			// Создаём обработчик сигнала для SIGILL
			this->_events.sigill = ::signal(SIGILL, nullptr);
			// Создаём обработчик сигнала для SIGABRT
			this->_events.sigabrt = ::signal(SIGABRT, nullptr);
			// Создаём обработчик сигнала для SIGTERM
			this->_events.sigterm = ::signal(SIGTERM, nullptr);
			// Создаём обработчик сигнала для SIGSEGV
			this->_events.sigsegv = ::signal(SIGSEGV, nullptr);
		#endif
	}
}
/**
 * @brief Метод запуска обработки сигналов
 *
 */
void awh::Signals::start() noexcept {
	// Если отслеживание сигналов ещё не запущено
	if(!this->_mode.load(std::memory_order_acquire)){
		// Устанавливаем флаг запуска отслеживания сигналов
		this->_mode.store(true, std::memory_order_release);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Выполняем игнорирование сигналов SIGPIPE
			::signal(SIGPIPE, SIG_IGN);
			// Выполняем зануление структур перехватчиков событий
			::memset(&this->_events.sigint, 0, sizeof(this->_events.sigint));
			::memset(&this->_events.sigfpe, 0, sizeof(this->_events.sigfpe));
			::memset(&this->_events.sigill, 0, sizeof(this->_events.sigill));
			::memset(&this->_events.sigbus, 0, sizeof(this->_events.sigbus));
			::memset(&this->_events.sigabrt, 0, sizeof(this->_events.sigabrt));
			::memset(&this->_events.sigterm, 0, sizeof(this->_events.sigterm));
			::memset(&this->_events.sigsegv, 0, sizeof(this->_events.sigsegv));
			// Устанавливаем функцию перехвадчика событий
			this->_events.sigint.sa_sigaction  = ::signals::handler;
			this->_events.sigfpe.sa_sigaction  = ::signals::handler;
			this->_events.sigill.sa_sigaction  = ::signals::handler;
			this->_events.sigbus.sa_sigaction  = ::signals::handler;
			this->_events.sigabrt.sa_sigaction = ::signals::handler;
			this->_events.sigterm.sa_sigaction = ::signals::handler;
			this->_events.sigsegv.sa_sigaction = ::signals::handler;
			// Устанавливаем флаги перехвата сигналов
			this->_events.sigint.sa_flags  = (SA_RESTART | SA_SIGINFO);
			this->_events.sigfpe.sa_flags  = (SA_RESTART | SA_SIGINFO);
			this->_events.sigill.sa_flags  = (SA_RESTART | SA_SIGINFO);
			this->_events.sigbus.sa_flags  = (SA_RESTART | SA_SIGINFO);
			this->_events.sigabrt.sa_flags = (SA_RESTART | SA_SIGINFO);
			this->_events.sigterm.sa_flags = (SA_RESTART | SA_SIGINFO);
			this->_events.sigsegv.sa_flags = (SA_RESTART | SA_SIGINFO);
			// Устанавливаем маску перехвата
			sigemptyset(&this->_events.sigint.sa_mask);
			sigemptyset(&this->_events.sigfpe.sa_mask);
			sigemptyset(&this->_events.sigill.sa_mask);
			sigemptyset(&this->_events.sigbus.sa_mask);
			sigemptyset(&this->_events.sigabrt.sa_mask);
			sigemptyset(&this->_events.sigterm.sa_mask);
			sigemptyset(&this->_events.sigsegv.sa_mask);
			// Активируем перехватчик событий
			::sigaction(SIGINT, &this->_events.sigint, nullptr);
			::sigaction(SIGFPE, &this->_events.sigfpe, nullptr);
			::sigaction(SIGILL, &this->_events.sigill, nullptr);
			::sigaction(SIGBUS, &this->_events.sigbus, nullptr);
			::sigaction(SIGABRT, &this->_events.sigabrt, nullptr);
			::sigaction(SIGTERM, &this->_events.sigterm, nullptr);
			::sigaction(SIGSEGV, &this->_events.sigsegv, nullptr);
			// Отправка сигнала для теста
			// ::raise(SIGABRT);
		/**
		 * Для операционной системы MS Windows
		 */
		#else
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
}
/**
 * @brief Метод установки функции обратного вызова, которая должна сработать при получении сигнала
 *
 * @param callback функция обратного вызова
 */
void awh::Signals::on(function <void (const int32_t)> callback) noexcept {
	// Выполняем установку функцию обратного вызова
	this->_callback = ::move(callback);
	// Выполняем установки функции обратного вызова
	self.callback = std::bind(&signals_t::callback, this, _1);
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
