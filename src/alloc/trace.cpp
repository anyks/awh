/**
 * @file trace.cpp
 * @date 2026-08-21
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
 * \~russian
 * @brief Файл съёма стека вызовов
 *
 * \~english
 * @brief Call stack capture file
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/trace.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
	/**
	 * Раскрутка нужна и здесь
	 *
	 * Родной `RtlCaptureStackBackTrace` у MS Windows на ARM64 отдаёт ОДИН уровень
	 * вместо всего стека - и прямо, и с глубины в двадцать четыре вызова (сличено на
	 * стенде: родной 1 против 29 у раскрутки, тогда как на x86-64 оба дают 29). Оттого
	 * съём везде один и тот же, а родной путь не годится вовсе
	 *
	 * Заголовок раскрутки даёт компилятор GCC/Clang, у нативного MSVC его нет вовсе:
	 * там съём идёт родным `RtlCaptureStackBackTrace` (на ARM64 с известной просадкой
	 * до одного уровня), а не раскруткой
	 */
	#if !defined(_MSC_VER)
		#include <unwind.h>
	#endif
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	#include <pthread.h>
	#include <unwind.h>
	/**
	 * Обращение адреса в имя есть не всюду
	 *
	 * У систем ELF и у macOS его даёт `dladdr` из средств связывания. Заголовок этот
	 * тянет за собою лишь объявления и памяти не просит
	 */
	#include <dlfcn.h>
#endif

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * Признак нахождения потока внутри съёма
	 *
	 * Хранится он по-разному, и выбор этот везде подчинён одному правилу: отметка,
	 * ставимая изнутри выдачи памяти, САМА просить памяти не вправе. Ни один из двух
	 * способов не безопасен всюду, оттого их два
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Ключ признака нахождения потока внутри съёма
		static DWORD __awh_trace_key__ = FLS_OUT_OF_INDEXES;
	/**
	 * Системам ELF признак хранится СРЕДСТВАМИ СОБИРАТЕЛЯ, а не системы
	 *
	 * Ключ системы здесь не годится: у glibc `pthread_setspecific` заводит второй
	 * уровень места под значения ВЫЗОВОМ `calloc`, а номер нашего ключа заранее не
	 * известен - съём заводится по требованию, и до него ключей заводят сколько угодно.
	 * Приди `calloc` из отметки обратно в нашу выдачу - та пошла бы снимать стек, не
	 * нашла бы себя внутри съёма (отметка-то ещё не легла) и отметилась бы снова, и так
	 * до срыва стека. Проверено разбором машинного кода libc 2.36 и щупом с заслонённым
	 * `calloc`: одно выделение на первую отметку ключа с номером от тридцати двух
	 *
	 * Модель `initial-exec` обязательна по той же причине, что и у зеркала кэша:
	 * ленивая ходила бы за местом через `__tls_get_addr`, а тот САМ просит памяти.
	 * Перечень систем здесь тот же, каким заводится зеркало, и по тем же доводам -
	 * записаны они в `cache.cpp`
	 */
	#elif !defined(__APPLE__) && !defined(__MACH__) && !defined(__OpenBSD__)
		#define AWH_TRACE_TLS 1
		// Признак нахождения потока внутри съёма
		static __thread bool __awh_trace_busy__ __attribute__((tls_model("initial-exec"))) = false;
	#else
		/**
		 * macOS и OpenBSD хранят признак ключом системы
		 *
		 * Место потока средствами собирателя просит там памяти само - у macOS через
		 * `_tlv_get_addr`, у OpenBSD через связывателя, - и сторож, поставленный им,
		 * уходил бы в ту самую возвратность, какую призван разорвать. Ключ же там
		 * безопасен: щуп отмерил ноль выделений на первую отметку при любом его номере
		 */
		// Ключ признака нахождения потока внутри съёма
		static ::pthread_key_t __awh_trace_key__;
	#endif
	/**
	 * @brief Метод определения нахождения потока внутри съёма
	 *
	 * @return признак нахождения потока внутри съёма
	 *
	 */
	static bool inside() noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Если ключ не заведён
			if(__awh_trace_key__ == FLS_OUT_OF_INDEXES)
				// Внутри съёма мы не находимся
				return false;
			// Выводим признак нахождения потока внутри съёма
			return (::FlsGetValue(__awh_trace_key__) != nullptr);
		/**
		 * Для систем ELF
		 */
		#elif defined(AWH_TRACE_TLS)
			// Выводим признак нахождения потока внутри съёма
			return __awh_trace_busy__;
		/**
		 * Для macOS и OpenBSD
		 */
		#else
			// Выводим признак нахождения потока внутри съёма
			return (::pthread_getspecific(__awh_trace_key__) != nullptr);
		#endif
	}
	/**
	 * @brief Метод отметки нахождения потока внутри съёма
	 *
	 * @param value признак нахождения потока внутри съёма
	 *
	 */
	static void mark(const bool value) noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Если ключ не заведён
			if(__awh_trace_key__ == FLS_OUT_OF_INDEXES)
				// Отмечать нечего
				return;
			// Отмечаем нахождение потока внутри съёма
			::FlsSetValue(__awh_trace_key__, (value ? reinterpret_cast <void *> (static_cast <uintptr_t> (1)) : nullptr));
		/**
		 * Для систем ELF
		 */
		#elif defined(AWH_TRACE_TLS)
			// Отмечаем нахождение потока внутри съёма
			__awh_trace_busy__ = value;
		/**
		 * Для macOS и OpenBSD
		 */
		#else
			// Отмечаем нахождение потока внутри съёма
			::pthread_setspecific(__awh_trace_key__, (value ? reinterpret_cast <void *> (static_cast <uintptr_t> (1)) : nullptr));
		#endif
	}
		/**
		 * @brief Ход съёма стека вызовов раскруткой
		 *
		 */
		typedef struct Walk {
			// Массив под адреса стека вызовов
			const void ** frames;
			// Длина массива в местах
			size_t depth;
			// Остаток пропускаемых ближних уровней
			size_t skip;
			// Число снятых адресов
			size_t count;
		} walk_t;
		/**
		 * @brief Метод обхода уровня стека вызовов
		 *
		 * @note Обход раскруткой заведён лишь там, где есть её заголовок: у нативного
		 *       MSVC съём идёт родным `RtlCaptureStackBackTrace`, а не обходом уровней
		 *
		 * @param context уровень стека вызовов
		 * @param arg     ход съёма стека вызовов
		 * @return        признак продолжения обхода
		 *
		 */
		#if !defined(_MSC_VER)
		static ::_Unwind_Reason_Code walker(struct ::_Unwind_Context * context, void * arg) noexcept {
			// Получаем ход съёма стека вызовов
			walk_t * walk = reinterpret_cast <walk_t *> (arg);
			// Определяем адрес возврата уровня
			const uintptr_t address = static_cast <uintptr_t> (::_Unwind_GetIP(context));
			// Если адрес возврата не определён
			if(address == 0)
				// Стек кончился
				return ::_URC_END_OF_STACK;
			// Если ближние уровни ещё пропускаются
			if(walk->skip > 0){
				// Уменьшаем остаток пропускаемых уровней
				walk->skip--;
				// Переходим к следующему уровню
				return ::_URC_NO_REASON;
			}
			// Если массив заполнен целиком
			if(walk->count >= walk->depth)
				// Снимать больше некуда
				return ::_URC_END_OF_STACK;
			// Записываем адрес возврата уровня
			walk->frames[walk->count++] = reinterpret_cast <const void *> (address);
			// Переходим к следующему уровню
			return ::_URC_NO_REASON;
		}
		#endif
};

/**
 * @brief Метод заведения съёма стека вызовов
 *
 * @return признак заведения съёма
 *
 */
bool awh::alloc::Trace::init() noexcept {
	// Если съём уже заведён
	if(this->_warmed.load(std::memory_order_acquire))
		// Заводить нечего
		return true;
	/**
	 * Заводим ключ признака нахождения потока внутри съёма
	 */
	/**
	 * Заводит ключ ровно ОДИН поток, и выбирается он неделимой подменой
	 *
	 * Настройки приложение вправе менять из любого потока, и два потока, вошедшие в
	 * заведение разом, оба видели признак снятым и оба заводили ключ. Второй затирал
	 * первый: тот утекал, а поток, отметившийся внутри съёма ПЕРВЫМ ключом, читался
	 * вторым как не отметившийся - и вложенный съём становился возможен, ровно тот,
	 * ради запрета которого отметка и заведена
	 *
	 * Проигравший подмену не ждёт победителя, а отвечает готовностью съёма как она есть:
	 * ожидание здесь платилось бы кружением на пути настроек, а звавший к отказу готов -
	 * без прогрева съём просто не снимает ничего
	 */
	// Признак, ожидаемый неделимой подменой
	bool expected = false;
	// Если ключ заводить выпало не нам
	if(!this->_keyed.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
		// Выводим готовность съёма как она есть
		return this->_warmed.load(std::memory_order_acquire);
	#if defined(_WIN32) || defined(_WIN64)
		// Заводим ключ признака нахождения потока внутри съёма
		::__awh_trace_key__ = ::FlsAlloc(nullptr);
		// Если ключ не заведён
		if(::__awh_trace_key__ == FLS_OUT_OF_INDEXES){
			// Возвращаем признак на место: ключа нет, и заведение вправе повториться
			this->_keyed.store(false, std::memory_order_release);
			// Отвечаем отказом
			return false;
		}
	#elif defined(AWH_TRACE_TLS)
		/**
		 * Заводить нечего: место под признак отведено в блоке TLS при запуске программы
		 */
	#else
		// Заводим ключ признака нахождения потока внутри съёма
		if(::pthread_key_create(&::__awh_trace_key__, nullptr) != 0){
			// Возвращаем признак на место: ключа нет, и заведение вправе повториться
			this->_keyed.store(false, std::memory_order_release);
			// Отвечаем отказом
			return false;
		}
	#endif
	/**
	 * Прогреваем съём
	 *
	 * Первое обращение к раскрутке строит указатели разделов раскрутки, а у части
	 * систем ещё и выделяет память. Делаем это здесь, вне всякой выдачи: дальше съём
	 * памяти не просит вовсе
	 */
	// Массив под адреса прогревочного съёма
	const void * frames[8];
	// Отмечаем съём прогретым прежде прогрева: иначе прогрев ушёл бы в отказ готовности
	this->_warmed.store(true, std::memory_order_release);
	// Прогреваем съём
	static_cast <void> (this->capture(frames, 8, 0));
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия съёма стека вызовов
 *
 */
void awh::alloc::Trace::reset() noexcept {
	// Если ключ заведён
	if(this->_keyed.load(std::memory_order_acquire)){
		/**
		 * Снимаем ключ признака нахождения потока внутри съёма
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Снимаем ключ признака нахождения потока внутри съёма
			::FlsFree(::__awh_trace_key__);
			// Отмечаем ключ снятым
			::__awh_trace_key__ = FLS_OUT_OF_INDEXES;
		#elif defined(AWH_TRACE_TLS)
			// Снимаем признак нахождения потока внутри съёма
			__awh_trace_busy__ = false;
		#else
			// Снимаем ключ признака нахождения потока внутри съёма
			::pthread_key_delete(::__awh_trace_key__);
		#endif
		// Отмечаем ключ снятым
		this->_keyed.store(false, std::memory_order_release);
	}
	// Отмечаем съём непрогретым
	this->_warmed.store(false, std::memory_order_release);
}
/**
 * @brief Метод съёма стека вызовов
 *
 * @param frames массив под адреса стека вызовов
 * @param depth  длина массива в местах
 * @param skip   число ближних уровней, какие пропустить
 * @return       число снятых адресов
 *
 */
size_t awh::alloc::Trace::capture(const void ** frames, const size_t depth, const size_t skip) noexcept {
	// Если снимать некуда либо съём не заведён
	if((frames == nullptr) || (depth == 0) || !this->_warmed.load(std::memory_order_acquire))
		// Снимать нечего
		return 0;
	/**
	 * Если поток уже находится внутри съёма
	 *
	 * Раскрутка вольна обратиться за памятью, а выдача памяти - за съёмом: обращение
	 * это вернулось бы сюда и не вернулось бы никогда обратно. Разрываем возвратность
	 * отказом от вложенного съёма
	 */
	if(::inside())
		// Снимать нечего
		return 0;
	// Отмечаем поток находящимся внутри съёма
	::mark(true);
	// Число снятых адресов
	size_t result = 0;
	{
		// Наибольшее число снимаемых уровней
		const size_t cap = ((depth > DEPTH) ? DEPTH : depth);
		#if defined(_MSC_VER)
			/**
			 * Съём родным средством системы
			 *
			 * У нативного MSVC заголовка раскрутки нет, и съём идёт родным
			 * `RtlCaptureStackBackTrace`. На ARM64 он даёт известную просадку до одного
			 * уровня (см. врезку у подключения раскрутки), на x86-64 - весь стек
			 */
			result = static_cast <size_t> (::RtlCaptureStackBackTrace(
				static_cast <ULONG> (skip + 1), static_cast <ULONG> (cap),
				reinterpret_cast <PVOID *> (const_cast <void **> (frames)), nullptr));
		#else
			// Ход съёма стека вызовов
			walk_t walk;
			// Записываем массив под адреса стека вызовов
			walk.frames = frames;
			// Записываем длину массива
			walk.depth = cap;
			// Записываем остаток пропускаемых ближних уровней
			walk.skip = (skip + 1);
			// Снятых адресов пока нет
			walk.count = 0;
			// Снимаем стек раскруткой
			static_cast <void> (::_Unwind_Backtrace(&::walker, &walk));
			// Запоминаем число снятых адресов
			result = walk.count;
		#endif
	}
	// Отмечаем поток вышедшим из съёма
	::mark(false);
	// Выводим число снятых адресов
	return result;
}
/**
 * @brief Метод обращения адреса стека в имя
 *
 * @param frame  разбираемый адрес
 * @param symbol сведения о разобранном адресе
 * @return       признак разбора адреса
 *
 */
bool awh::alloc::Trace::resolve(const void * frame, symbol_t & symbol) noexcept {
	// Если разбирать нечего
	if(frame == nullptr)
		// Разбирать нечего
		return false;
	/**
	 * Для операционной системы MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		/**
		 * Разбираем адрес образом, а не именем функции
		 *
		 * Имена функций у MS Windows добываются средствами разбора (dbghelp), а те
		 * тянут за собою отдельную библиотеку и замок процесса. Образ же со смещением
		 * называет место однозначно и добывается средствами самой системы
		 */
		// Описатель образа, которому принадлежит адрес
		HMODULE image = nullptr;
		// Добываем описатель образа по адресу
		if(::GetModuleHandleExA((GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT), reinterpret_cast <LPCSTR> (frame), &image) == FALSE)
			// Разобрать не удалось
			return false;
		/**
		 * Название образа хранится за нами, и хранится У КАЖДОГО ПОТОКА СВОЁ
		 *
		 * Средства системы кладут его в переданный буфер, а договор обещает строку,
		 * годную до следующего разбора: буфер этот и есть «до следующего разбора».
		 * Прежде буфер был ОДИН на процесс, и договор этот в многопоточной работе не
		 * исполнялся вовсе - разбор в чужом потоке затирал строку под руками звавшего,
		 * а тот об этом узнать не мог никак. У прочих систем такого нет: там строка
		 * принадлежит средствам связывания и живёт своей жизнью
		 *
		 * Место потока средствами собирателя здесь безопасно, в отличие от кэша и от
		 * признака нахождения внутри съёма: разбор адреса в имя зовётся ПРИКЛАДНЫМ
		 * КОДОМ, а не изнутри выдачи памяти, и ленивое заведение места ему не страшно
		 */
		// Буфер названия образа, свой у каждого потока
		static thread_local char storage[MAX_PATH];
		// Добываем название образа
		const DWORD length = ::GetModuleFileNameA(image, storage, sizeof(storage));
		// Если название образа добыть не вышло
		if(length == 0)
			// Разобрать не удалось
			return false;
		/**
		 * Дописываем завершающий нуль при усечении
		 *
		 * Средства системы у ранних выпусков MS Windows усекают название по длине
		 * буфера и нуля НЕ дописывают: строка вышла бы незавершённой, а читает её
		 * прикладной код
		 */
		if(length >= static_cast <DWORD> (sizeof(storage)))
			// Завершаем строку названия образа
			storage[sizeof(storage) - 1] = '\0';
		// Записываем название образа
		symbol.image = storage;
		// Имени функции у нас нет
		symbol.name = nullptr;
		// Записываем адрес начала образа
		symbol.begin = reinterpret_cast <const void *> (image);
		// Записываем смещение разбираемого адреса от начала образа
		symbol.offset = (reinterpret_cast <const uint8_t *> (frame) - reinterpret_cast <const uint8_t *> (image));
		// Отвечаем успехом
		return true;
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Сведения об адресе, добытые средствами связывания
		::Dl_info info;
		// Обнуляем сведения об адресе
		::memset(&info, 0, sizeof(info));
		// Добываем сведения об адресе
		/**
		 * Приведение обязательно
		 *
		 * У Solaris и OpenIndiana `dladdr` принимает изменяемый указатель, а не
		 * постоянный: сборка там валится без приведения, у прочих же оно безвредно
		 */
		if(::dladdr(const_cast <void *> (frame), &info) == 0)
			// Разобрать не удалось
			return false;
		// Записываем название образа
		symbol.image = info.dli_fname;
		// Записываем название функции
		symbol.name = info.dli_sname;
		/**
		 * Записываем начало функции, а при отсутствии её - начало образа
		 *
		 * Имени у статической функции может не быть вовсе: она не лежит в таблице
		 * имён образа. Смещение от начала образа тогда единственное, что называет место
		 */
		// Записываем адрес начала функции
		symbol.begin = ((info.dli_saddr != nullptr) ? info.dli_saddr : info.dli_fbase);
		// Записываем смещение разбираемого адреса от начала функции
		symbol.offset = (reinterpret_cast <const uint8_t *> (frame) - reinterpret_cast <const uint8_t *> (symbol.begin));
		// Отвечаем успехом
		return true;
	#endif
}
/**
 * @brief Метод определения готовности съёма
 *
 * @return признак готовности съёма
 *
 */
bool awh::alloc::Trace::ready() const noexcept {
	// Выводим признак готовности съёма
	return this->_warmed.load(std::memory_order_acquire);
}
/**
 * @brief Метод получения названия способа съёма
 *
 * @return название способа съёма
 *
 */
const char * awh::alloc::Trace::name() const noexcept {
	// Выводим название способа съёма
	#if defined(_MSC_VER)
		return "RtlCaptureStackBackTrace";
	#else
		return "_Unwind_Backtrace";
	#endif
}
