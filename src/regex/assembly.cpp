/**
 * @file assembly.cpp
 * @date 2026-08-02
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
 * @brief Реализация исполняемой памяти кодогенерации — размещение участка памяти,
 *        допускающего исполнение, наполнение его порождённым машинным кодом
 *        и переключение прав доступа между записью и исполнением
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <tuple>
#include <cerrno>
#include <string>
#include <cstring>

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/log.hpp>

/**
 * Подключаем заголовочный файл модуля
 */
#include <regex/assembly.hpp>

/**
 * Если сборка выполняется для операционной системы Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/macro/win32.hpp>
/**
 * Если сборка выполняется для операционной системы семейства UNIX
 */
#else
	/**
	 * Стандартные заголовочные файлы UNIX
	 */
	#include <unistd.h>
	#include <sys/mman.h>

	/**
	 * Если сборка выполняется для операционной системы семейства Apple
	 */
	#if defined(__APPLE__) || defined(__MACH__)
		/**
		 * Стандартные заголовочные файлы Apple
		 */
		#include <pthread.h>
		#include <libkern/OSCacheControl.h>
	#endif
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён вспомогательных значений исполняемой памяти
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Наибольший допустимый размер участка исполняемой памяти
	 *
	 * @details Порождённый код выражения соразмерен количеству его инструкций,
	 *          а количество это ограничено самим разбором выражения. Предел
	 *          заведён затем, чтобы ошибка расчёта размера не приводила
	 *          к запросу памяти, соразмерному адресному пространству.
	 *
	 */
	constexpr size_t MAX_ASSEMBLY = 0x4000000;

	/**
	 * @brief Функция извлечения размера страницы памяти
	 *
	 * @return размер страницы памяти в байтах
	 *
	 */
	static size_t granularity() noexcept {
		/**
		 * Если сборка выполняется для операционной системы Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Создаём сведения о системе
			SYSTEM_INFO info;
			// Извлекаем сведений о системе
			::GetSystemInfo(&info);
			// Выводим размер страницы памяти
			return static_cast <size_t> (info.dwPageSize);
		/**
		 * Если сборка выполняется для операционной системы семейства UNIX
		 */
		#else
			// Получаем размер страницы памяти
			const long result = ::sysconf(_SC_PAGESIZE);
			// Выводим размер страницы памяти
			return ((result > 0) ? static_cast <size_t> (result) : 0x1000);
		#endif
	}
};

/**
 * @brief Метод проверки поддержки исполняемой памяти сборкой
 *
 * @return результат проверки поддержки исполняемой памяти сборкой
 *
 */
bool awh::regex::Assembly::available() noexcept {
	/**
	 * Если сборка выполняется для поддерживаемой операционной системы
	 *
	 * @details Размещение памяти, допускающей исполнение, выполнено средствами
	 *          Windows и средствами систем семейства UNIX. Прочие системы
	 *          исполняемой памяти не получают и сопоставляются исполнением
	 *          программы, как и прежде.
	 *
	 */
	#if defined(_WIN32) || defined(_WIN64) || defined(__unix__) || defined(__unix) || defined(__APPLE__) || defined(__MACH__)
		// Выводим поддержку исполняемой памяти сборкой
		return true;
	/**
	 * Если сборка выполняется для прочих операционных систем
	 */
	#else
		// Выводим отсутствие поддержки исполняемой памяти сборкой
		return false;
	#endif
}
/**
 * @brief Метод размещения участка исполняемой памяти
 *
 * @param size требуемый размер участка памяти в байтах
 * @return     результат размещения участка исполняемой памяти
 *
 */
bool awh::regex::Assembly::allocate(const size_t size) noexcept {
	// Освобождаем участка, размещённого прежде
	this->release();
	/**
	 * Если требуемый размер участка недопустим
	 */
	if((size == 0) || (size > MAX_ASSEMBLY))
		// Размещение участка исполняемой памяти: отказ
		return false;
	// Получаем размер страницы памяти
	const size_t page = ::granularity();
	/**
	 * Получаем размер участка, кратный размеру страницы памяти
	 *
	 * @details Права доступа устанавливаются страницами целиком, поэтому участок
	 *          округляется до страницы: иначе изменение прав затронуло бы память,
	 *          участку не принадлежащую.
	 *
	 */
	const size_t length = (((size + page) - 1) / page) * page;
	/**
	 * Если сборка выполняется для операционной системы Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Размещаем участок памяти, допускающего запись
		this->_address = ::VirtualAlloc(nullptr, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	/**
	 * Если сборка выполняется для операционной системы семейства Apple
	 */
	#elif defined(__APPLE__) || defined(__MACH__)
		/**
		 * Размещаем участок памяти, допускающего порождение кода
		 *
		 * @details Признак «MAP_JIT» обязателен: на процессорах ARM64 участок,
		 *          допускающий исполнение, размещается лишь с ним. Права участка
		 *          при этом запрашиваются разом на запись и исполнение, однако
		 *          одновременно они не действуют: какое из двух доступно потоку,
		 *          определяется отдельным вызовом, и состояние это принадлежит
		 *          потоку, а не участку.
		 *
		 */
		this->_address = ::mmap(nullptr, length, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON | MAP_JIT, -1, 0);
		/**
		 * Если размещение участка памяти не выполнено
		 */
		if(this->_address == MAP_FAILED)
			// Сбрасываем адрес размещённого участка памяти
			this->_address = nullptr;
	/**
	 * Если сборка выполняется для прочих операционных систем семейства UNIX
	 */
	#else
		/**
		 * Получаем набор прав размещаемого участка памяти
		 *
		 * @details Права, какие участку понадобятся впредь, объявляются при
		 *          размещении его: система NetBSD запрещает выдавать право
		 *          исполнения участку, о таком праве не заявившему, и отказывает
		 *          изменению прав, а не размещению. Признак «PROT_MPROTECT»
		 *          объявляет право исполнения заранее, оставляя действующими
		 *          лишь чтение и запись.
		 *
		 *          Системы, признака такого не имеющие, размещают участок как
		 *          прежде: объявление прав будущих им незнакомо, а запрета
		 *          на выдачу права исполнения они не ставят.
		 *
		 */
		#if defined(PROT_MPROTECT)
			// Заводим права памяти на чтение, запись и исполнение кода
			const int32_t rights = (PROT_READ | PROT_WRITE | PROT_MPROTECT(PROT_EXEC));
		/**
		 * Если PROT_MPROTECT не доступен
		 */
		#else
			// Заводим права памяти на чтение и запись
			const int32_t rights = (PROT_READ | PROT_WRITE);
		#endif
		// Размещаем участок памяти, допускающего запись
		this->_address = ::mmap(nullptr, length, rights, MAP_PRIVATE | MAP_ANON, -1, 0);
		/**
		 * Если размещение участка памяти не выполнено
		 */
		if(this->_address == MAP_FAILED)
			// Сбрасываем адрес размещённого участка памяти
			this->_address = nullptr;
	#endif
	/**
	 * Если размещение участка памяти не выполнено
	 */
	if(this->_address == nullptr){
		/**
		 * Если сборка выполняется для операционной системы Windows
		 *
		 * @details Система MS Windows причину системного вызова кладёт
		 *          не в «errno», а в собственное место, и `strerror` о ней
		 *          не знает вовсе: сообщение вышло бы «Unknown error»
		 *
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Получаем описание отказа системы по номеру её ошибки
			const string & reason = ("code " + ::to_string(::GetLastError()));
		/**
		 * Если сборка выполняется для операционной системы семейства UNIX
		 */
		#else
			// Получаем описание отказа системы по коду ошибки
			const string & reason = ::strerror(errno);
		#endif
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("Executable memory of %zu bytes could not be allocated: %s", __PRETTY_FUNCTION__, {length}, log::flag_t::CRITICAL, reason.c_str());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("Executable memory of %zu bytes could not be allocated: %s", log::flag_t::CRITICAL, length, reason.c_str());
		#endif
		// Размещение участка исполняемой памяти: отказ
		return false;
	}
	// Сбрасываем количество байт, занятых порождённым кодом
	this->_length = 0;
	// Устанавливаем размер размещённого участка памяти
	this->_size = length;
	// Сбрасываем флаг разрешения исполнения участка памяти
	this->_executable = false;
	// Размещение участка исполняемой памяти: успех
	return true;
}
/**
 * @brief Метод освобождения участка исполняемой памяти
 *
 */
void awh::regex::Assembly::release() noexcept {
	/**
	 * Если участок исполняемой памяти размещён
	 */
	if(this->_address != nullptr){
		/**
		 * Если сборка выполняется для операционной системы Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Освобождаем участок памяти
			::VirtualFree(this->_address, 0, MEM_RELEASE);
		/**
		 * Если сборка выполняется для операционной системы семейства UNIX
		 */
		#else
			// Освобождаем участок памяти
			::munmap(this->_address, this->_size);
		#endif
		// Сбрасываем адрес размещённого участка памяти
		this->_address = nullptr;
	}
	// Сбрасываем размер размещённого участка памяти
	this->_size = 0;
	// Сбрасываем количество байт, занятых порождённым кодом
	this->_length = 0;
	// Сбрасываем флаг разрешения исполнения участка памяти
	this->_executable = false;
}
/**
 * @brief Метод наполнения участка памяти порождённым машинным кодом
 *
 * @param code   адрес порождённого машинного кода
 * @param length размер порождённого машинного кода в байтах
 * @return       результат наполнения участка памяти
 *
 */
bool awh::regex::Assembly::fill(const void * code, const size_t length) noexcept {
	/**
	 * Если участок памяти не размещён либо наполнение недопустимо
	 *
	 * @details Наполнение участка, исполнение какого разрешено, привело бы
	 *          к записи в память, доступной для записи не будучи.
	 *
	 */
	if((this->_address == nullptr) || this->_executable || (code == nullptr) || (length == 0) || (length > this->_size))
		// Наполнения участка памяти: отказ
		return false;
	/**
	 * Если сборка выполняется для операционной системы семейства Apple
	 */
	#if defined(__APPLE__) || defined(__MACH__)
		/**
		 * Разрешаем записю в участок памяти
		 *
		 * @details Участок, размещённый с признаком «MAP_JIT», доступен потоку
		 *          либо для записи, либо для исполнения, и состояние это
		 *          принадлежит потоку, а не участку.
		 *
		 */
		::pthread_jit_write_protect_np(0);
	#endif
	// Наполняем участок памяти порождённым машинным кодом
	::memcpy(this->_address, code, length);
	// Устанавливаем количество байт, занятых порождённым кодом
	this->_length = length;
	// Наполнения участка памяти: успех
	return true;
}
/**
 * @brief Метод разрешения исполнения участка памяти
 *
 * @return результат разрешения исполнения участка памяти
 *
 */
bool awh::regex::Assembly::commit() noexcept {
	/**
	 * Если участок памяти не размещён либо порождённым кодом не наполнен
	 */
	if((this->_address == nullptr) || (this->_length == 0))
		// Разрешение исполнения участка памяти: отказ
		return false;
	/**
	 * Если исполнение участка памяти уже разрешено
	 */
	if(this->_executable)
		// Разрешение исполнения участка памяти: успех
		return true;
	/**
	 * Если сборка выполняется для операционной системы Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Прежние права доступа к участку памяти
		DWORD previous = 0;
		/**
		 * Если установка прав доступа к участку памяти не выполнена
		 */
		if(::VirtualProtect(this->_address, this->_size, PAGE_EXECUTE_READ, &previous) == 0)
			// Разрешение исполнения участка памяти: отказ
			return false;
		// Сбрасываем кэш команд процессора
		::FlushInstructionCache(::GetCurrentProcess(), this->_address, this->_length);
	/**
	 * Если сборка выполняется для операционной системы семейства Apple
	 */
	#elif defined(__APPLE__) || defined(__MACH__)
		/**
		 * Запрещаем записю в участок памяти
		 *
		 * @details Запрет записи разрешает исполнение: участок, размещённый
		 *          с признаком «MAP_JIT», допускает потоку одно из двух.
		 *
		 */
		::pthread_jit_write_protect_np(1);
		/**
		 * Сбрасываем кэш команд процессора
		 *
		 * @details Порождённый код записан через кэш данных, тогда как выборка
		 *          команд идёт через кэш команд, и согласованность их процессорами
		 *          ARM64 не обеспечивается. Пропуск сброса даёт исполнение
		 *          случайного содержимого памяти.
		 *
		 */
		::sys_icache_invalidate(this->_address, this->_length);
	/**
	 * Если сборка выполняется для прочих операционных систем семейства UNIX
	 */
	#else
		/**
		 * Если установка прав доступа к участку памяти не выполнена
		 */
		if(::mprotect(this->_address, this->_size, PROT_READ | PROT_EXEC) != 0){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Execution of the allocated memory could not be permitted: %s", __PRETTY_FUNCTION__, {this->_size}, log::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Execution of the allocated memory could not be permitted: %s", log::flag_t::CRITICAL, ::strerror(errno));
			#endif
			// Разрешение исполнения участка памяти: отказ
			return false;
		}
		/**
		 * Если сборка выполняется компилятором, порождающим сброс кэша команд
		 */
		#if defined(__GNUC__) || defined(__clang__)
			// Сбрасываем кэш команд процессора
			__builtin___clear_cache(reinterpret_cast <char *> (this->_address), (reinterpret_cast <char *> (this->_address) + this->_length));
		#endif
	#endif
	// Устанавливаем флаг разрешения исполнения участка памяти
	this->_executable = true;
	// Разрешение исполнения участка памяти: успех
	return true;
}
/**
 * @brief Метод извлечения адреса порождённого машинного кода
 *
 * @return адрес порождённого машинного кода либо нулевой указатель
 *
 */
const void * awh::regex::Assembly::entry() const noexcept {
	// Выводим адрес порождённого машинного кода
	return (this->_executable ? this->_address : nullptr);
}
/**
 * @brief Метод извлечения адреса наполненного участка памяти
 *
 * @return адрес наполненного участка памяти либо нулевой указатель
 *
 */
const void * awh::regex::Assembly::image() const noexcept {
	// Выводим адрес участка памяти, порождённым кодом наполненного
	return ((this->_length > 0) ? this->_address : nullptr);
}
/**
 * @brief Метод извлечения размера порождённого машинного кода
 *
 * @return размер порождённого машинного кода в байтах
 *
 */
size_t awh::regex::Assembly::length() const noexcept {
	// Выводим размер порождённого машинного кода
	return this->_length;
}
/**
 * @brief Метод проверки разрешения исполнения участка памяти
 *
 * @return результат проверки разрешения исполнения участка памяти
 *
 */
bool awh::regex::Assembly::executable() const noexcept {
	// Проверка разрешения исполнения участка памяти
	return this->_executable;
}
/**
 * @brief Оператор перемещения
 *
 * @param assembly перемещаемый объект исполняемой памяти
 * @return         текущий объект исполняемой памяти
 *
 */
awh::regex::Assembly & awh::regex::Assembly::operator = (Assembly && assembly) noexcept {
	/**
	 * Если перемещение выполняется в тот же объект
	 */
	if(this == &assembly)
		// Выводим текущий объект исполняемой памяти
		return (* this);
	// Освобождаем участка, размещённого прежде
	this->release();
	// Перенимаем адрес размещённого участка памяти
	this->_address = assembly._address;
	// Перенимаем размер размещённого участка памяти
	this->_size = assembly._size;
	// Перенимаем количество байт, занятых порождённым кодом
	this->_length = assembly._length;
	// Перенимаем флаг разрешения исполнения участка памяти
	this->_executable = assembly._executable;
	// Сбрасываем адрес участка памяти перемещаемого объекта
	assembly._address = nullptr;
	// Сбрасываем размер участка памяти перемещаемого объекта
	assembly._size = 0;
	// Сбрасываем количество байт перемещаемого объекта
	assembly._length = 0;
	// Сбрасываем флаг разрешения исполнения перемещаемого объекта
	assembly._executable = false;
	// Выводим текущий объект исполняемой памяти
	return (* this);
}
/**
 * @brief Конструктор перемещения
 *
 * @param assembly перемещаемый объект исполняемой памяти
 *
 */
awh::regex::Assembly::Assembly(Assembly && assembly) noexcept :
 _executable(assembly._executable), _size(assembly._size),
 _length(assembly._length), _address(assembly._address) {
	// Сбрасываем адрес участка памяти перемещаемого объекта
	assembly._address = nullptr;
	// Сбрасываем размер участка памяти перемещаемого объекта
	assembly._size = 0;
	// Сбрасываем количество байт перемещаемого объекта
	assembly._length = 0;
	// Сбрасываем флаг разрешения исполнения перемещаемого объекта
	assembly._executable = false;
}
/**
 * @brief Конструктор
 *
 */
awh::regex::Assembly::Assembly() noexcept :
 _executable(false), _size(0), _length(0), _address(nullptr) {}
/**
 * @brief Деструктор
 *
 */
awh::regex::Assembly::~Assembly() noexcept {
	// Освобождаем участок исполняемой памяти
	this->release();
}
