/**
 * @file: obsd.cpp
 * @date: 2026-08-21
 *
 * @brief Внутренние имена распределителя библиотеки времени исполнения OpenBSD
 *
 * У OpenBSD библиотека времени исполнения зовёт выдачу памяти НАПРЯМУЮ, по внутреннему
 * имени `_libc_malloc`, а `malloc` у неё лишь слабый псевдоним поверх: во всей libc
 * таких прямых вызовов 159. Подмена именами до них не достаёт по устройству, и всякий
 * блок, выданный нами и попавший во внутреннее перевыделение libc, валил процесс -
 * `recallocarray(): bogus pointer`
 *
 * Определив внутренние имена сами, мы добиваемся того, что `malloc.o` не вытягивается
 * из архива вовсе, и внутренние вызовы приходят к нам. Работает это лишь при
 * СТАТИЧЕСКОМ связывании: у разделяемой libc вызовы связаны внутри неё самой ещё при её
 * сборке, и снаружи их не перенаправить ничем
 *
 * Файл собирается только под OpenBSD. У динамической сборки определения эти безвредны:
 * их просто никто не зовёт
 *
 * @author \ANYKS
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
extern "C" {
	void * malloc(size_t);
	void free(void *);
	void * calloc(size_t, size_t);
	void * realloc(void *, size_t);
	int posix_memalign(void **, size_t, size_t);
	void * aligned_alloc(size_t, size_t);
	// Внутренние имена, какими libc зовёт выдачу у себя внутри
	void * _libc_malloc(size_t size) { ::__awh_alloc_libc_seen__ = true; return ::malloc(size); }
	void _libc_free(void * ptr) { ::__awh_alloc_libc_seen__ = true; ::free(ptr); }
	void * _libc_calloc(size_t count, size_t size) { ::__awh_alloc_libc_seen__ = true; return ::calloc(count, size); }
	void * _libc_realloc(void * ptr, size_t size) { ::__awh_alloc_libc_seen__ = true; return ::realloc(ptr, size); }
	int _libc_posix_memalign(void ** memptr, size_t alignment, size_t size) { return ::posix_memalign(memptr, alignment, size); }
	void * _libc_aligned_alloc(size_t alignment, size_t size) { return ::aligned_alloc(alignment, size); }
	// Выдача, обнуляемая при освобождении: своего слоя у нас нет, обнуляем сами
	void * _libc_malloc_conceal(size_t size) { return ::malloc(size); }
	void * _libc_calloc_conceal(size_t count, size_t size) { return ::calloc(count, size); }
	void * malloc_conceal(size_t size) { return ::malloc(size); }
	void * calloc_conceal(size_t count, size_t size) { return ::calloc(count, size); }
	/**
	 * Освобождение с обнулением содержимого
	 *
	 * Договор OpenBSD: `freezero` обнуляет ровно заявленный размер и лишь затем
	 * освобождает. Обнуление здесь обязательно - на нём держится вся суть вызова
	 */
	void _libc_freezero(void * ptr, size_t size) { if(ptr != nullptr){ ::memset(ptr, 0, size); ::free(ptr); } }
	void freezero(void * ptr, size_t size) { _libc_freezero(ptr, size); }
	/**
	 * Перевыдача с обнулением прироста
	 *
	 * Договор: старый размер обязан совпасть с заявленным, прирост обнуляется, а
	 * старое содержимое затирается. Мы соблюдаем видимую часть договора
	 */
	void * _libc_recallocarray(void * ptr, size_t oldnmemb, size_t newnmemb, size_t size) {
		if((size != 0) && (newnmemb > (static_cast <size_t> (-1) / size))){ errno = ENOMEM; return nullptr; }
		const size_t want = (newnmemb * size), had = (oldnmemb * size);
		void * result = ::calloc(newnmemb, size);
		if((result != nullptr) && (ptr != nullptr)){
			::memcpy(result, ptr, ((had < want) ? had : want));
			::memset(ptr, 0, had);
			::free(ptr);
		}
		return result;
	}
	void * recallocarray(void * ptr, size_t o, size_t n, size_t s) { return _libc_recallocarray(ptr, o, n, s); }
	// Заведение распределителя libc: заводить нечего, наш заводится сам
	void _libc__malloc_init(int) {}
	void _malloc_init(int) {}
	// Настройки распределителя libc: нашими не читаются
	char * malloc_options = nullptr;
	// Съём состояния распределителя libc: своего съёма у нас пока нет
	void _libc_malloc_dump(int) {}
	void malloc_dump(int) {}
}
