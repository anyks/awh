/**
 * @file obsd.cpp
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
 * @brief Реализация внутренних имён распределителя библиотеки времени исполнения OpenBSD
 *
 * @details У OpenBSD библиотека времени исполнения зовёт выдачу памяти НАПРЯМУЮ, по внутреннему
 *          имени `_libc_malloc`, а `malloc` у неё лишь слабый псевдоним поверх: во всей libc
 *          таких прямых вызовов 159. Подмена именами до них не достаёт по устройству, и всякий
 *          блок, выданный нами и попавший во внутреннее перевыделение libc, валил процесс -
 *          `recallocarray(): bogus pointer`
 *
 *          Определив внутренние имена сами, мы добиваемся того, что `malloc.o` не вытягивается
 *          из архива вовсе, и внутренние вызовы приходят к нам. Работает это лишь при
 *          СТАТИЧЕСКОМ связывании: у разделяемой libc вызовы связаны внутри неё самой ещё при её
 *          сборке, и снаружи их не перенаправить ничем
 *
 *          Файл собирается только под OpenBSD. У динамической сборки определения эти безвредны:
 *          их просто никто не зовёт.
 *
 * @copyright Copyright © 2026
 *
 */
#include <cstddef>
#include <cstring>
#include <cerrno>
#include <unistd.h>
/**
 * Признак того, что libc идёт через наши внутренние имена
 *
 * Ставится первым же таким вызовом. У динамически связанной программы имена эти не
 * зовёт никто: там libc связана внутри себя ещё при своей сборке. Признак этот и
 * отличает полный перехват от обычного - без единого вызова и без опроса связывателя
 */
bool __awh_alloc_libc_seen__ = false;
// Выдача блока, укрытого от снимков памяти: определена фасадом распределителя
extern void * __awh_alloc_conceal__(const size_t size, bool * hidden, const bool wire, bool * wired) noexcept;
/**
 * Область связывания C для внутренних имён распределителя
 */
extern "C" {
	// Объявляем системную выдачу памяти
	void * malloc(size_t);
	// Объявляем системное освобождение памяти
	void free(void *);
	// Объявляем системную обнулённую выдачу памяти
	void * calloc(size_t, size_t);
	// Объявляем системную перевыдачу памяти
	void * realloc(void *, size_t);
	// Объявляем системную выдачу с выравниванием
	int posix_memalign(void **, size_t, size_t);
	// Объявляем системную выдачу по адресу выравнивания
	void * aligned_alloc(size_t, size_t);
	// Внутренние имена, какими libc зовёт выдачу у себя внутри
	// Определяем внутреннюю выдачу памяти с отметкой признака
	void * _libc_malloc(size_t size) { ::__awh_alloc_libc_seen__ = true; return ::malloc(size); }
	// Определяем внутреннее освобождение памяти с отметкой признака
	void _libc_free(void * ptr) { ::__awh_alloc_libc_seen__ = true; ::free(ptr); }
	// Определяем внутреннюю обнулённую выдачу с отметкой признака
	void * _libc_calloc(size_t count, size_t size) { ::__awh_alloc_libc_seen__ = true; return ::calloc(count, size); }
	// Определяем внутреннюю перевыдачу с отметкой признака
	void * _libc_realloc(void * ptr, size_t size) { ::__awh_alloc_libc_seen__ = true; return ::realloc(ptr, size); }
	// Определяем внутреннюю выдачу с выравниванием
	int _libc_posix_memalign(void ** memptr, size_t alignment, size_t size) { return ::posix_memalign(memptr, alignment, size); }
	// Определяем внутреннюю выдачу по адресу выравнивания
	void * _libc_aligned_alloc(size_t alignment, size_t size) { return ::aligned_alloc(alignment, size); }
	/**
	 * Выдача, укрытая от снимков памяти
	 *
	 * Договор OpenBSD обещает по ней две вещи: область отводится с признаком
	 * `MAP_CONCEAL` - то есть не попадает в снимок памяти при падении, - и содержимое
	 * затирается при освобождении. Прежде здесь стояла обычная выдача, и оба обещания
	 * понижались МОЛЧА: тот, кто держал в такой памяти ключи, о том не узнавал
	 *
	 * Выдача идёт слоем крупных выдач: лишь он берёт память у источника напрямую, а
	 * укрытие задаётся при отведении и задним числом не навешивается
	 */
	void * malloc_conceal(size_t size) { return ::__awh_alloc_conceal__(size, nullptr, false, nullptr); }
	/**
	 * Укрытая выдача с обнулением
	 *
	 * Обнулять здесь нечего: область приходит от системы свежей, а та отдаёт лишь
	 * обнулённые страницы. Сверять переполнение произведения, однако, обязаны - иначе
	 * усечённый счёт дал бы блок меньше затребованного
	 */
	/**
	 * @brief Функция выдачи обнулённой укрытой памяти
	 *
	 * @note Обнуления здесь НЕТ намеренно, и держится это на двух свойствах слоя
	 *       крупных выдач: укрытая область берётся у системы напрямую, минуя список
	 *       придержанных, - а системная выдача приходит нулевой, - и при возврате
	 *       затирается сама. Заведись у укрытых областей придержка без затирания, и
	 *       обнулённой памятью здесь молча пошёл бы мусор
	 *
	 * @param count число значений
	 * @param size  размер значения
	 * @return      адрес выданной памяти либо nullptr
	 *
	 */
	void * calloc_conceal(size_t count, size_t size) {
		// Если произведение переполнится
		if((size != 0) && (count > (static_cast <size_t> (-1) / size))){
			// Отвечаем отказом
			errno = ENOMEM;
			// Выдавать нечего
			return nullptr;
		}
		// Выдаём укрытую область
		return ::__awh_alloc_conceal__((count * size), nullptr, false, nullptr);
	}
	// Определяем внутреннюю укрытую выдачу
	void * _libc_malloc_conceal(size_t size) { return malloc_conceal(size); }
	// Определяем внутреннюю обнулённую укрытую выдачу
	void * _libc_calloc_conceal(size_t count, size_t size) { return calloc_conceal(count, size); }
	/**
	 * Освобождение с обнулением содержимого
	 *
	 * Договор OpenBSD: `freezero` обнуляет ровно заявленный размер и лишь затем
	 * освобождает. Обнуление здесь обязательно - на нём держится вся суть вызова
	 */
	void _libc_freezero(void * ptr, size_t size) { if(ptr != nullptr){ ::memset(ptr, 0, size); ::free(ptr); } }
	// Определяем освобождение с обнулением через внутреннее
	void freezero(void * ptr, size_t size) { _libc_freezero(ptr, size); }
	/**
	 * Перевыдача с обнулением прироста
	 *
	 * Договор: старый размер обязан совпасть с заявленным, прирост обнуляется, а
	 * старое содержимое затирается. Мы соблюдаем видимую часть договора
	 */
	void * _libc_recallocarray(void * ptr, size_t oldnmemb, size_t newnmemb, size_t size) {
		// Если новое произведение переполнится
		if((size != 0) && (newnmemb > (static_cast <size_t> (-1) / size))){ errno = ENOMEM; return nullptr; }
		// Старое произведение проверяем тем же порядком: заворот дал бы КОРОТКИЙ `had`, а
		// на нём держатся и перенос содержимого, и затирание старой тайны перед `free`
		if((size != 0) && (oldnmemb > (static_cast <size_t> (-1) / size))){ errno = EINVAL; return nullptr; }
		// Определяем объёмы нового и старого произведений
		const size_t want = (newnmemb * size), had = (oldnmemb * size);
		// Выделяем новую область
		void * result = ::calloc(newnmemb, size);
		// Если область выдана и старый блок существует
		if((result != nullptr) && (ptr != nullptr)){
			// Переносим содержимое старого блока
			::memcpy(result, ptr, ((had < want) ? had : want));
			// Затираем старое содержимое
			::memset(ptr, 0, had);
			// Освобождаем старый блок
			::free(ptr);
		}
		// Выводим адрес новой области
		return result;
	}
	// Определяем перевыдачу через внутреннюю
	void * recallocarray(void * ptr, size_t o, size_t n, size_t s) { return _libc_recallocarray(ptr, o, n, s); }
	// Заведение распределителя libc: заводить нечего, наш заводится сам
	void _libc__malloc_init(int) {}
	// Определяем заведение распределителя libc пустым действием
	void _malloc_init(int) {}
	// Настройки распределителя libc: нашими не читаются
	char * malloc_options = nullptr;
	// Съём состояния распределителя libc: своего съёма у нас пока нет
	void _libc_malloc_dump(int) {}
	// Определяем съём состояния распределителя libc пустым действием
	void malloc_dump(int) {}
}
