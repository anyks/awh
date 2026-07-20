/**
 * @file: fds.cpp
 * @date: 2025-10-27
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
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <string>
#include <cstring>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <cstdio>
	#include <vector>
#endif

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Системные заголовочные файлы
	 */
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "ws2_32.lib")
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <sys/types.h>
	#include <sys/resource.h>
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/fds.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод вывода в лог справочной помощи
 *
 * @param actual  текущее значение установленных файловых дескрипторов
 * @param desired желаемое значение для установки файловых дескрипторов
 */
void awh::Files_Descriptors::help(const uint32_t actual, const uint32_t desired) const noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Выполняем формирование лога
		this->_log->print(
			"\nMaximum sockets requested: %u, but current system limit is: %u.\n"
			"On Windows, the default handle limit is ~16K. If you need more, increase it programmatically.\n\n"
			"🔧 How to increase the limit on Windows:\n\n"
			"1. Programmatically — call early in your application (before creating sockets!):\n"
			"   SetHandleCount(%u);\n\n"
			"   C++ Example:\n"
			"      #include <windows.h>\n"
			"      int main() {\n"
			"          SetHandleCount(%u); // Call as early as possible!\n"
			"          // ... rest of your code ...\n"
			"      }\n\n"
			"2. Via Registry (affects GUI handles, may not affect sockets):\n"
			"   Open regedit → navigate to:\n"
			"      HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\SubSystems\n"
			"   Find \"Windows\" value, a string like:\n"
			"      %%SystemRoot%%\\system32\\csrss.exe ... SharedSection=1024,20480,768\n"
			"   Increase the THIRD number (e.g., to %u).\n"
			"   Reboot system.\n\n"
			"3. For server applications — prefer SetHandleCount().\n\n"
			"💡 Important: SetHandleCount() is a hint to the system — actual limit depends on Windows version and available memory.\n"
			"💡 Tip: Check current handle usage in Task Manager → Details tab → \"Handles\" column.",
			log_t::flag_t::WARNING, desired, actual, desired, desired, desired
		);
	/**
	 * Для операционной системы Linux
	 */
	#elif __linux__
		// Выполняем формирование лога
		this->_log->print(
			"\nMaximum sockets requested: %u, but current system limit is: %u.\n"
			"This may cause failures under high load.\n\n"
			"🔧 How to increase the limit on Linux:\n\n"
			"1. Temporary increase (for current session):\n"
			"   ulimit -n %u\n\n"
			"2. Permanent increase:\n"
			"   Edit: /etc/security/limits.conf\n"
			"   Add lines:\n"
			"      * soft nofile %u\n"
			"      * hard nofile %u\n\n"
			"   (Replace '*' with username if setting for a specific user)\n\n"
			"3. For systemd services:\n"
			"   Edit: /etc/systemd/system.conf\n"
			"   Add:\n"
			"      DefaultLimitNOFILE=%u\n\n"
			"   Then run:\n"
			"      sudo systemctl daemon-reload\n\n"
			"4. Restart your application or re-login to apply.\n\n"
			"💡 Tip: You can also increase system-wide limit:\n"
			"   echo 'fs.file-max = 2000000' | sudo tee -a /etc/sysctl.conf\n"
			"   sudo sysctl -p",
			log_t::flag_t::WARNING, desired, actual, desired, desired, desired, desired
		);
	/**
	 * Для операционной системы OpenBSD
	 */
	#elif __OpenBSD__
		// Выполняем формирование лога
		this->_log->print(
			"\nMaximum sockets requested: %u, but current system limit is: %u.\n"
			"OpenBSD has strict defaults for security.\n\n"
			"🔧 How to increase the limit on OpenBSD:\n\n"
			"1. Temporary increase:\n"
			"   ulimit -n %u\n\n"
			"2. Permanent increase:\n"
			"   Edit: /etc/login.conf\n"
			"   In target class (e.g., \"default:\"), add:\n"
			"      :openfiles-cur=%u:\n"
			"      :openfiles-max=%u:\n\n"
			"3. Rebuild database:\n"
			"   cap_mkdb /etc/login.conf\n\n"
			"4. Increase system limits (if needed):\n"
			"   sysctl kern.maxfiles=%u\n"
			"   sysctl kern.maxfilesperproc=%u\n\n"
			"   For permanent change, add to /etc/sysctl.conf:\n"
			"      kern.maxfiles=%u\n"
			"      kern.maxfilesperproc=%u\n\n"
			"5. Reboot or re-login to apply.\n\n"
			"💡 Note: OpenBSD may require reboot for some changes to take effect.",
			log_t::flag_t::WARNING, desired, actual, desired, desired, desired, desired, desired, desired, desired
		);
	/**
	 * Для операционной системы Sun Solaris
	 */
	#elif __sun__
		// Выполняем формирование лога
		this->_log->print(
			"\nMaximum sockets requested: %u, but current system limit is: %u.\n"
			"Solaris requires configuration via projects and system parameters.\n\n"
			"🔧 How to increase the limit on Solaris:\n\n"
			"1. Check current limits:\n"
			"   ulimit -n\n\n"
			"2. Set temporary limit:\n"
			"   ulimit -n %u\n\n"
			"3. For permanent limit — use projects:\n"
			"   Create project (if not exists):\n"
			"      projadd -U $USER network\n\n"
			"   Set limit for project:\n"
			"      projmod -s -K \"process.max-file-descriptor=(priv,%u,deny)\" network\n\n"
			"   Assign project to user:\n"
			"      usermod -K project=network $USER\n\n"
			"4. Re-login or start app within project:\n"
			"   newtask -p network ./your_app\n\n"
			"5. Optionally, increase system-wide limit:\n"
			"   echo \"rlim_fd_max=%u\" >> /etc/system\n"
			"   echo \"rlim_fd_cur=%u\" >> /etc/system\n"
			"   Reboot system.\n\n"
			"💡 Tip: Use `prctl -n process.max-file-descriptor -i process $$` to check current process limit.",
			log_t::flag_t::WARNING, desired, actual, desired, desired, desired, desired
		);
	/**
	 * Для операционной системы FreeBSD
	 */
	#elif __FreeBSD__
		// Выполняем формирование лога
		this->_log->print(
			"\nMaximum sockets requested: %u, but current system limit is: %u.\n"
			"FreeBSD allows tuning limits via login.conf and sysctl.\n\n"
			"🔧 How to increase the limit on FreeBSD:\n\n"
			"1. Temporary increase:\n"
			"   ulimit -n %u\n\n"
			"2. Permanent increase via login.conf:\n"
			"   Edit: /etc/login.conf\n"
			"   Find class (e.g., \"default:\") and add/modify:\n"
			"      :openfiles=%u:\n\n"
			"3. Rebuild database:\n"
			"   cap_mkdb /etc/login.conf\n\n"
			"4. Assign class to user (if needed):\n"
			"   pw usermod $USER -L <class_name>\n\n"
			"5. Increase system limits (optional):\n"
			"   sysctl kern.maxfiles=%u\n"
			"   sysctl kern.maxfilesperproc=%u\n\n"
			"   For permanent change, add to /etc/sysctl.conf:\n"
			"      kern.maxfiles=%u\n"
			"      kern.maxfilesperproc=%u\n\n"
			"6. Restart application or re-login.",
			log_t::flag_t::WARNING, desired, actual, desired, desired, desired, desired, desired, desired
		);
	/**
	 * Для операционной системы NetBSD
	 */
	#elif __NetBSD__
		// Выполняем формирование лога
		this->_log->print(
			"\nMaximum sockets requested: %u, but current system limit is: %u.\n"
			"NetBSD uses login.conf and sysctl for tuning limits.\n\n"
			"🔧 How to increase the limit on NetBSD:\n\n"
			"1. Temporary increase:\n"
			"   ulimit -n %u\n\n"
			"2. Permanent increase:\n"
			"   Edit: /etc/login.conf\n"
			"   In target class (e.g., \"default:\"), add:\n"
			"      :openfiles=%u:\n\n"
			"3. Rebuild database:\n"
			"   cap_mkdb /etc/login.conf\n\n"
			"4. Increase system limits:\n"
			"   sysctl kern.maxfiles=%u\n"
			"   sysctl kern.maxfilesperproc=%u\n\n"
			"   For permanent change, add to /etc/sysctl.conf:\n"
			"      kern.maxfiles=%u\n"
			"      kern.maxfilesperproc=%u\n\n"
			"5. Restart application or re-login.\n\n"
			"💡 Tip: Use `sysctl -a | grep maxfiles` to view current system limits.",
			log_t::flag_t::WARNING, desired, actual, desired, desired, desired, desired, desired, desired
		);
	/**
	 * Для операционной системы macOS
	 */
	#elif __APPLE__ || __MACH__
		// Выполняем формирование лога
		this->_log->print(
			"\nMaximum sockets requested: %u, but current system limit is: %u.\n"
			"macOS default limits are often too low for server applications.\n\n"
			"🔧 How to increase the limit on macOS:\n\n"
			"1. Temporary increase (in current terminal):\n"
			"   ulimit -n %u\n\n"
			"2. Permanent increase:\n"
			"   Create file: /Library/LaunchDaemons/limit.maxfiles.plist\n"
			"   Paste content:\n\n"
			"   <?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"   <!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
			"   <plist version=\"1.0\">\n"
			"     <dict>\n"
			"       <key>Label</key>\n"
			"       <string>limit.maxfiles</string>\n"
			"       <key>ProgramArguments</key>\n"
			"       <array>\n"
			"         <string>launchctl</string>\n"
			"         <string>limit</string>\n"
			"         <string>maxfiles</string>\n"
			"         <string>%u</string>\n"
			"         <string>%u</string>\n"
			"       </array>\n"
			"       <key>RunAtLoad</key>\n"
			"       <true/>\n"
			"       <key>ServiceIPC</key>\n"
			"       <false/>\n"
			"     </dict>\n"
			"   </plist>\n\n"
			"3. Set permissions and load:\n"
			"   sudo chown root:wheel /Library/LaunchDaemons/limit.maxfiles.plist\n"
			"   sudo chmod 644 /Library/LaunchDaemons/limit.maxfiles.plist\n"
			"   sudo launchctl load -w /Library/LaunchDaemons/limit.maxfiles.plist\n\n"
			"4. Reboot or restart your application.\n\n"
			"💡 Note: On some macOS versions, disabling SIP (System Integrity Protection) may be required — proceed with caution.",
			log_t::flag_t::WARNING, desired, actual, desired, desired, desired
		);
	#endif
}
/**
 * @brief Метод установки нужного количества файловых дескрипторов
 *
 * @param limit желаемое количество файловых дескрипторов
 * @return      результат установки
 */
bool awh::Files_Descriptors::limit(const uint32_t limit) const noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// SetHandleCount — рекомендация системе, не гарантирует лимит
		if(::SetHandleCount(limit)){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Печатаем информационное сообщение
				this->_log->print("Called SetHandleCount(%u) successfully", log_t::flag_t::INFO, limit);
			#endif
			// Возвращаем true
			return true;
		// Если ничего не получилось
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("SetHandleCount(%u) failed", __PRETTY_FUNCTION__, make_tuple(limit), log_t::flag_t::WARNING, limit);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("SetHandleCount(%u) failed", log_t::flag_t::WARNING, limit);
			#endif
		}
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		/**
		 * Структура заполнения доступных лимитов
		 */
		struct rlimit rl;
		// Получаем текущие лимиты
		if(::getrlimit(RLIMIT_NOFILE, &rl) != 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(limit), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
			// Выходим из функции
			return false;
		}
		// Получаем мягкое значение (с защитой от усечения RLIM_INFINITY)
		const uint32_t currentSoft = ((rl.rlim_cur == RLIM_INFINITY) || (rl.rlim_cur > static_cast <rlim_t> (UINT32_MAX)) ? UINT32_MAX : static_cast <uint32_t> (rl.rlim_cur));
		// Получаем жёсткое значение (с защитой от усечения RLIM_INFINITY)
		const uint32_t currentHard = ((rl.rlim_max == RLIM_INFINITY) || (rl.rlim_max > static_cast <rlim_t> (UINT32_MAX)) ? UINT32_MAX : static_cast <uint32_t> (rl.rlim_max));
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Печатаем информационное сообщение
			this->_log->print("Current FD limits: soft=%u, hard=%u", log_t::flag_t::INFO, currentSoft, currentHard);
		#endif
		// Если soft лимит уже >= target — ничего не делаем
		if(currentSoft >= limit)
			// Возвращаем true
			return true;
		/**
		 * Если жёсткого лимита достаточно — поднимаем только мягкий лимит до желаемого значения
		 */
		if(currentHard >= limit){
			// Устанавливаем новое значение мягкого лимита
			rl.rlim_cur = static_cast <rlim_t> (limit);
			// Устанавливаем новое значение файловых дескрипторов
			if(::setrlimit(RLIMIT_NOFILE, &rl) == 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Печатаем информационное сообщение
					this->_log->print("Successfully raised soft FD limit to %u", log_t::flag_t::INFO, limit);
				#endif
				// Возвращаем true
				return true;
			}
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Failed to raise soft FD limit to %u: %s", __PRETTY_FUNCTION__, make_tuple(limit), log_t::flag_t::WARNING, limit, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Failed to raise soft FD limit to %u: %s", log_t::flag_t::WARNING, limit, ::strerror(errno));
			#endif
			// Возвращаем результат неудачи
			return false;
		}
		/**
		 * Жёсткого лимита недостаточно — пытаемся поднять и мягкий, и жёсткий лимит до желаемого (требуются права root)
		 */
		// Устанавливаем новое значение мягкого лимита
		rl.rlim_cur = static_cast <rlim_t> (limit);
		// Устанавливаем новое значение жёсткого лимита
		rl.rlim_max = static_cast <rlim_t> (limit);
		// Устанавливаем новое значение файловых дескрипторов
		if(::setrlimit(RLIMIT_NOFILE, &rl) == 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Печатаем информационное сообщение
				this->_log->print("Successfully raised soft and hard FD limit to %u", log_t::flag_t::INFO, limit);
			#endif
			// Возвращаем true
			return true;
		}
		/**
		 * Поднять жёсткий лимит не удалось — откатываемся к максимально возможному мягкому лимиту (до текущего жёсткого)
		 */
		// Устанавливаем мягкий лимит на максимум доступного жёсткого
		rl.rlim_cur = static_cast <rlim_t> (currentHard);
		// Восстанавливаем прежнее значение жёсткого лимита
		rl.rlim_max = static_cast <rlim_t> (currentHard);
		// Пытаемся поднять мягкий лимит хотя бы до жёсткого
		if(::setrlimit(RLIMIT_NOFILE, &rl) == 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Печатаем информационное сообщение
				this->_log->print("Raised soft FD limit to hard maximum %u (requested %u)", log_t::flag_t::INFO, currentHard, limit);
			#endif
		// Если даже это не удалось
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Failed to raise soft FD limit to %u: %s", __PRETTY_FUNCTION__, make_tuple(limit), log_t::flag_t::WARNING, currentHard, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Failed to raise soft FD limit to %u: %s", log_t::flag_t::WARNING, currentHard, ::strerror(errno));
			#endif
		}
	#endif
	// Возвращаем результат неудачи: желаемый лимит не достигнут полностью
	return false;
}
/**
 * @brief Метод получения лимита файловых дескрипторов установленных в операционной системе
 *
 * @return количество файловых дескрипторов установленных в файловой системе
 */
std::pair <uint32_t, uint32_t> awh::Files_Descriptors::limit() const noexcept {
	// Переменная результата
	std::pair <uint32_t, uint32_t> result = {0, 0};
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * На Windows нет аналога getrlimit(RLIMIT_NOFILE). Лимит файловых дескрипторов
		 * уровня CRT управляется функциями _getmaxstdio()/_setmaxstdio() (по умолчанию 512,
		 * максимум 8192). Возвращаем эти значения без создания и закрытия сокетов,
		 * так как пробинг сокетами крайне дорог и не отражает реальный одновременный лимит.
		 */
		// Получаем текущее значение лимита файловых дескрипторов уровня CRT
		const int current = ::_getmaxstdio();
		// Устанавливаем текущее значение количества доступных файловых дескрипторов
		result.first = (current > 0 ? static_cast <uint32_t> (current) : 512);
		// Устанавливаем максимально возможное значение (потолок _setmaxstdio)
		result.second = 8192;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Печатаем информационное сообщение
			this->_log->print("Current FD limits: soft=%u, hard=%u", log_t::flag_t::INFO, result.first, result.second);
		#endif
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Структура заполнения доступных лимитов
			 */
			struct rlimit rl{0};
			// Выполняем извлечение информации об доступных файловых дескрипторах
			if(::getrlimit(RLIMIT_NOFILE, &rl) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Выполняем установку текущего значения количества доступных файловых дескрипторов (с защитой от усечения RLIM_INFINITY)
			result.first = ((rl.rlim_cur == RLIM_INFINITY) || (rl.rlim_cur > static_cast <rlim_t> (UINT32_MAX)) ? UINT32_MAX : static_cast <uint32_t> (rl.rlim_cur));
			// Выполняем установку максимального значения количества доступных файловых дескрипторов (с защитой от усечения RLIM_INFINITY)
			result.second = ((rl.rlim_max == RLIM_INFINITY) || (rl.rlim_max > static_cast <rlim_t> (UINT32_MAX)) ? UINT32_MAX : static_cast <uint32_t> (rl.rlim_max));
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
	#endif
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод оценки лимита одновременно открытых сокетов
 *
 * @param max верхний предел пробинга (0 - использовать значение по умолчанию)
 * @return    пара значений (оценка доступного количества сокетов, верхний предел пробинга)
 */
std::pair <uint32_t, uint32_t> awh::Files_Descriptors::sockets(const uint32_t max) const noexcept {
	// Переменная результата
	std::pair <uint32_t, uint32_t> result = {0, 0};
	// Определяем верхний предел пробинга (по умолчанию 65535)
	const uint32_t limit = (max > 0 ? max : 65535);
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Структура данных Winsock
			 */
			WSADATA wsa;
			// Выполняем инициализацию Winsock
			if(::WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("WSAStartup failed", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("WSAStartup failed", log_t::flag_t::CRITICAL);
				#endif
				// Возвращаем результат
				return result;
			}
			// Список открытых сокетов (держим их открытыми для корректного измерения)
			vector <SOCKET> socks;
			// Резервируем память, чтобы избежать частых аллокаций
			socks.reserve(limit < 1024 ? limit : 1024);
			// Сокет для инициализации
			SOCKET sock = INVALID_SOCKET;
			// Счётчик успешно открытых сокетов
			uint32_t count = 0;
			/**
			 * Выполняем короткий пробинг до первой ошибки или до достижения предела
			 */
			for(; count < limit; ++count){
				// Выполняем инициализацию сокета
				if((sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
					// Прерываем цикл, упёршись в лимит
					break;
				// Сохраняем открытый сокет
				socks.push_back(sock);
			}
			/**
			 * Выполняем закрытие всех открытых сокетов
			 */
			for(auto & sock : socks)
				// Закрываем открытый сокет
				::closesocket(sock);
			// Выполняем деинициализацию Winsock
			::WSACleanup();
			// Устанавливаем оценку доступного количества сокетов
			result.first = count;
			// Устанавливаем верхний предел пробинга
			result.second = limit;
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
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// На POSIX-системах лимит сокетов ограничен лимитом файловых дескрипторов
		const std::pair <uint32_t, uint32_t> fds = this->limit();
		// Оценка доступного количества сокетов (ограничена верхним пределом пробинга)
		result.first = (fds.first > limit ? limit : fds.first);
		// Устанавливаем верхний предел пробинга
		result.second = limit;
	#endif
	// Возвращаем результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 */
awh::Files_Descriptors::Files_Descriptors(const log_t * log) noexcept : _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Files_Descriptors::~Files_Descriptors() noexcept {}
