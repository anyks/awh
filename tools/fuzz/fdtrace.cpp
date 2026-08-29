/**
 * @file fdtrace.cpp
 * @date 2026-08-29
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп учёта заведений и закрытий описателей у систем с ELF
 *
 * @details Розыск утечки описателя чтением исходных текстов уже дважды дал ложный
 *          ответ: путь заведения в движке не один, и закрытий по нему тоже не одно.
 *          Щуп снимает вопрос замером - он запоминает, ОТКУДА заведён каждый живой
 *          описатель, и по требованию выдаёт стек заведения тех, что остались
 *
 * @warning Перехват работает СВЯЗЫВАНИЕМ: наш объектный файл определяет socket(),
 *          accept(), close() и прочие, и связыватель ELF отдаёт им предпочтение перед
 *          телами из libc для ВСЕХ объектных файлов сборки. Отсюда два следствия:
 *          щуп непереносим на macOS (там своё устройство подмены) и он обязан звать
 *          настоящие тела через dlsym(RTLD_NEXT), иначе уходит в бесконечность
 *
 * @note Щуп собирается ТОЛЬКО в сборку ворошителя и в тело библиотеки не входит
 *
 * @copyright Copyright © 2026
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <map>
#include <vector>

#include <dlfcn.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/socket.h>

/**
 * Пространство имён щупа учёта описателей
 */
namespace {
	/**
	 * @brief Запись о заведённом описателе
	 */
	typedef struct Trace {
		// Глубина снятого стека заведения
		int32_t depth;
		// Снятый стек заведения описателя
		void * frames[24];
		/**
		 * @brief Конструктор
		 */
		Trace() noexcept : depth(0) {
			// Обнуляем снятый стек заведения
			::memset(this->frames, 0, sizeof(this->frames));
		}
	} trace_t;
	// Учёт живых описателей и мест их заведения
	static std::map <int32_t, trace_t> & registry() noexcept {
		// Учёт заводится при первом обращении: порядок заведения переменных процесса не определён
		static std::map <int32_t, trace_t> storage;
		// Выводим учёт живых описателей
		return storage;
	}
	// Замок учёта живых описателей
	static std::recursive_mutex & guard() noexcept {
		// Замок заводится при первом обращении
		static std::recursive_mutex mutex;
		// Выводим замок учёта
		return mutex;
	}
	// Признак работы внутри самого щупа: снятие стека само зовёт перехваченные тела
	static thread_local bool inside = false;
	/**
	 * @brief Функция запоминания места заведения описателя
	 *
	 * @param fd заведённый описатель
	 */
	static void remember(const int32_t fd) noexcept {
		// Если описатель негоден либо мы уже внутри щупа, учёт не ведём
		if((fd < 0) || inside)
			// Выходим из функции
			return;
		// Запоминаем, что работаем внутри щупа
		inside = true;
		{
			// Выполняем блокировку учёта
			const std::lock_guard <std::recursive_mutex> lock(guard());
			// Заводим запись о описателе
			trace_t & trace = registry()[fd];
			// Снимаем стек заведения описателя
			trace.depth = ::backtrace(trace.frames, 24);
		}
		// Снимаем признак работы внутри щупа
		inside = false;
	}
	/**
	 * @brief Функция снятия записи о закрытом описателе
	 *
	 * @param fd закрытый описатель
	 */
	static void forget(const int32_t fd) noexcept {
		// Если описатель негоден либо мы уже внутри щупа, учёт не ведём
		if((fd < 0) || inside)
			// Выходим из функции
			return;
		// Запоминаем, что работаем внутри щупа
		inside = true;
		{
			// Выполняем блокировку учёта
			const std::lock_guard <std::recursive_mutex> lock(guard());
			// Снимаем запись о описателе
			registry().erase(fd);
		}
		// Снимаем признак работы внутри щупа
		inside = false;
	}
}

/**
 * Настоящие тела перехваченных функций
 */
extern "C" {
	// Тип настоящего тела заведения сокета
	typedef int32_t (* socket_fn_t) (int32_t, int32_t, int32_t);
	// Тип настоящего тела приёма подключения
	typedef int32_t (* accept_fn_t) (int32_t, struct sockaddr *, socklen_t *);
	// Тип настоящего тела приёма подключения с признаками
	typedef int32_t (* accept4_fn_t) (int32_t, struct sockaddr *, socklen_t *, int32_t);
	// Тип настоящего тела заведения пары сокетов
	typedef int32_t (* socketpair_fn_t) (int32_t, int32_t, int32_t, int32_t *);
	// Тип настоящего тела закрытия описателя
	typedef int32_t (* close_fn_t) (int32_t);
	// Тип настоящего тела удвоения описателя
	typedef int32_t (* dup_fn_t) (int32_t);
	/**
	 * @brief Метод заведения сокета
	 */
	int32_t socket(int32_t domain, int32_t type, int32_t protocol){
		// Настоящее тело заведения сокета
		static socket_fn_t original = reinterpret_cast <socket_fn_t> (::dlsym(RTLD_NEXT, "socket"));
		// Выполняем заведение сокета
		const int32_t result = original(domain, type, protocol);
		// Запоминаем место заведения описателя
		remember(result);
		// Выводим заведённый описатель
		return result;
	}
	/**
	 * @brief Метод приёма подключения
	 */
	int32_t accept(int32_t fd, struct sockaddr * addr, socklen_t * length){
		// Настоящее тело приёма подключения
		static accept_fn_t original = reinterpret_cast <accept_fn_t> (::dlsym(RTLD_NEXT, "accept"));
		// Выполняем приём подключения
		const int32_t result = original(fd, addr, length);
		// Запоминаем место заведения описателя
		remember(result);
		// Выводим принятый описатель
		return result;
	}
	/**
	 * @brief Метод приёма подключения с признаками
	 */
	int32_t accept4(int32_t fd, struct sockaddr * addr, socklen_t * length, int32_t flags){
		// Настоящее тело приёма подключения с признаками
		static accept4_fn_t original = reinterpret_cast <accept4_fn_t> (::dlsym(RTLD_NEXT, "accept4"));
		// Выполняем приём подключения
		const int32_t result = original(fd, addr, length, flags);
		// Запоминаем место заведения описателя
		remember(result);
		// Выводим принятый описатель
		return result;
	}
	/**
	 * @brief Метод заведения пары связанных сокетов
	 */
	int32_t socketpair(int32_t domain, int32_t type, int32_t protocol, int32_t fds[2]){
		// Настоящее тело заведения пары сокетов
		static socketpair_fn_t original = reinterpret_cast <socketpair_fn_t> (::dlsym(RTLD_NEXT, "socketpair"));
		// Выполняем заведение пары сокетов
		const int32_t result = original(domain, type, protocol, fds);
		// Если пару завести удалось
		if(result == 0){
			// Запоминаем место заведения первого описателя
			remember(fds[0]);
			// Запоминаем место заведения второго описателя
			remember(fds[1]);
		}
		// Выводим итог заведения пары
		return result;
	}
	/**
	 * @brief Метод удвоения описателя
	 */
	int32_t dup(int32_t fd){
		// Настоящее тело удвоения описателя
		static dup_fn_t original = reinterpret_cast <dup_fn_t> (::dlsym(RTLD_NEXT, "dup"));
		// Выполняем удвоение описателя
		const int32_t result = original(fd);
		// Запоминаем место заведения описателя
		remember(result);
		// Выводим удвоенный описатель
		return result;
	}
	/**
	 * @brief Метод закрытия описателя
	 */
	int32_t close(int32_t fd){
		// Настоящее тело закрытия описателя
		static close_fn_t original = reinterpret_cast <close_fn_t> (::dlsym(RTLD_NEXT, "close"));
		// Снимаем запись о описателе
		forget(fd);
		// Выполняем закрытие описателя
		return original(fd);
	}
	/**
	 * @brief Метод вывода мест заведения живых описателей
	 *
	 * @details Зовётся ворошителем в тот же миг, в какой снимается перечень утёкших
	 *          описателей, - до снятия движка. Порядок обязан совпадать с порядком
	 *          самой находки, иначе щуп меряет не то состояние, что находка
	 *
	 * @param fds   перечень описателей, признанных утёкшими
	 * @param count число описателей в перечне
	 */
	void __awh_fdtrace_report__(const int32_t * fds, const size_t count){
		// Выполняем блокировку учёта
		const std::lock_guard <std::recursive_mutex> lock(guard());
		/**
		 * Перебираем перечень утёкших описателей
		 */
		for(size_t i = 0; i < count; i++){
			// Выполняем поиск записи о описателе
			auto it = registry().find(fds[i]);
			// Если запись о описателе не найдена
			if(it == registry().end()){
				// Сообщаем, что место заведения описателя щупу неизвестно
				::fprintf(stderr, "  описатель %d: место заведения щупу неизвестно\n", fds[i]);
				// Переходим к следующему описателю
				continue;
			}
			// Выводим заголовок стека заведения описателя
			::fprintf(stderr, "  описатель %d заведён здесь:\n", fds[i]);
			// Снимаем имена по стеку заведения
			char ** names = ::backtrace_symbols(it->second.frames, it->second.depth);
			// Если имена по стеку снять удалось
			if(names != nullptr){
				// Перебираем весь снятый стек
				for(int32_t j = 0; j < it->second.depth; j++)
					// Выводим очередной кадр стека заведения
					::fprintf(stderr, "    %s\n", names[j]);
				// Освобождаем память имён стека
				::free(names);
			}
		}
	}
}
