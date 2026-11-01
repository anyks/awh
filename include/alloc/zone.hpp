/**
 * @file zone.hpp
 * @date 2026-08-20
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
 * @brief Заголовочный файл захвата выделения памяти зоной macOS
 *
 * @section zone_decisions Намеренные решения
 *
 * @details <b>Захват ДВОЙНОЙ: зона и подмена имён разом.</b> macOS связывает имена
 *          двухуровнево: обращение из библиотеки времени исполнения помечено её же
 *          именем и нашим определением не заслоняется вовсе. Оттого одной подмены имён
 *          мало - она оставила бы половину процесса прежнему распределителю, а
 *          разделённый надвое распределитель означает выдачу одним и освобождение
 *          другим. Полноту даёт только зона, и она здесь незаменима.
 *
 *          Но за полноту зона платит раздачей: всякий `malloc` идёт сперва в libsystem,
 *          та ищет основную зону и лишь затем зовёт нас. Замерено - подмена имён,
 *          поставленная РЯДОМ с зоной, ускорила выдачу вдвое с половиной (8.26 → 3.27
 *          нс/оп на сценарии одного разряда). Оттого взято и то и другое: имена уводят к
 *          нам обращения своего образа коротким путём, зона ловит остальное. Указатель,
 *          выданный одним путём и освобождённый другим, приходит к нам обоими: путь у них
 *          общий - `reserve` и `discard`.
 *
 *          <b>Под санитайзером имена не подменяются.</b> Санитайзер заслоняет выдачу
 *          собою, и захват наш там отвечает отказом - это его устройство. Подмена же имён
 *          отказа не спрашивает: она работает связыванием. Останься она, выдача
 *          разделилась бы натрое, и не увидел бы её ни один из троих.
 *
 *          <b>Своя зона ставится первой перезаписью прежней, а не признаком.</b>
 *          Признака «эта зона теперь основная» система не предоставляет: основной
 *          зовётся первая в перечне. Оттого приём один - внести свою, а прежнюю
 *          основную изъять и внести заново, отчего она уходит в конец перечня.
 *
 *          <b>Прежняя зона не сносится, а отодвигается.</b> Память, выданная ею до
 *          захвата, остаётся живой, и освобождать её обязана она же. Снос основной
 *          зоны означал бы порчу всей памяти, выданной до входа в программу.
 *
 *          <b>Опознание чужого указателя ведётся перебором зон.</b> Каждая зона
 *          отвечает размером своего блока и нулём у чужого - оттого принадлежность
 *          указателя прежнему распределителю устанавливается прямо, а не от обратного,
 *          как у систем ELF.
 *
 * \~english
 * @brief Header file of memory allocation capture by the macOS malloc zone
 *
 * @section zone_decisions Intentional decisions
 *
 * @details <b>Capture is DUAL: zone and symbol interposition at once.</b> macOS links
 *          names two-level: a reference from the runtime library is tagged with its own
 *          name and is not shadowed by our definition at all. Hence symbol interposition
 *          alone is not enough - it would leave half the process to the previous
 *          allocator, and a split allocator means allocation by one and free by another.
 *          Only the zone gives completeness, and it is indispensable here.
 *
 *          But the zone pays for completeness with dispatch: every `malloc` goes first
 *          to libsystem, which looks up the main zone and only then calls us. Measured -
 *          symbol interposition placed ALONGSIDE the zone sped up allocation two and a
 *          half times (8.26 → 3.27 ns/op on the single-class scenario). Hence both are
 *          taken: names route our image's references the short way, the zone catches the
 *          rest. A pointer issued by one path and freed by the other reaches us by both:
 *          they share `reserve` and `discard`.
 *
 *          <b>Under a sanitizer, names are not interposed.</b> The sanitizer shadows
 *          allocation itself, and our capture answers with refusal there - that is its
 *          design. Symbol interposition does not ask for permission: it works by linking.
 *          Were it to stay, allocation would split three ways, and none of the three
 *          would see it.
 *
 *          <b>Our zone is installed by first overwriting the previous one, not by a
 *          flag.</b> The system provides no "this zone is now main" flag: the main one is
 *          the first in the list. Hence the only technique is to insert ours, then remove
 *          and re-insert the previous main zone, sending it to the end of the list.
 *
 *          <b>The previous zone is not destroyed but pushed aside.</b> Memory issued by it
 *          before capture remains valid, and it must free it itself. Destroying the main
 *          zone would mean corrupting all memory issued before entering the program.
 *
 *          <b>Identifying a foreign pointer is done by scanning zones.</b> Each zone
 *          answers with its own block size and zero for a foreign one - hence pointer
 *          ownership by the previous allocator is established directly, not by
 *          elimination as on ELF systems.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Если операционной системой является macOS
 */
#if defined(__APPLE__)

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "capture.hpp"
#include "../sys/macro/global.hpp"

/**
 * \~russian
 * @brief Пространство имён фреймворка
 *
 * \~english
 * @brief Framework namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Memory allocator namespace
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс захвата выделения памяти зоной macOS
		 *
		 * \~english
		 * @brief macOS malloc zone capture class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ ZoneCapture : public Capture {
			public:
				// Название нашей зоны, видимое средствами разбора памяти системы
				static constexpr const char * TITLE = "awh";
			private:
				// Прежние функции выделения памяти
				functions_t _originals;
				// Признак состоявшегося захвата
				bool _acquired;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата выделения памяти процесса
				 *
				 * @param hooks     наши функции, ставимые на место прежних
				 * @param originals прежние функции, отдаваемые захватом
				 * @return          признак состоявшегося захвата
				 *
				 * \~english
				 * @brief Method of capturing process memory allocation
				 *
				 * @param hooks     our functions installed in place of the previous ones
				 * @param originals previous functions returned by the capture
				 * @return          flag of the capture having taken place
				 *
				 * \~
				 */
				bool acquire(const functions_t & hooks, functions_t & originals) noexcept override;
				/**
				 * \~russian
				 * @brief Метод снятия захвата
				 *
				 * \~english
				 * @brief Method of releasing the capture
				 *
				 * \~
				 */
				void release() noexcept override;
				/**
				 * \~russian
				 * @brief Метод определения состоявшегося захвата
				 *
				 * @return признак захвата
				 *
				 * \~english
				 * @brief Method of determining whether the capture has taken place
				 *
				 * @return capture flag
				 *
				 * \~
				 */
				bool acquired() const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод опознания указателя, выданного прежним распределителем
				 *
				 * @param ptr разбираемый указатель
				 * @return    признак принадлежности прежнему распределителю
				 *
				 * \~english
				 * @brief Method of identifying a pointer issued by the previous allocator
				 *
				 * @param ptr pointer being resolved
				 * @return    flag of belonging to the previous allocator
				 *
				 * \~
				 */
				bool foreign(const void * ptr) const noexcept override;
				/**
				 * \~russian
				 * @brief Метод определения владельца чужого блока
				 *
				 * @param ptr разбираемый указатель
				 * @return    nullptr: владельцев несколько не бывает
				 *
				 * \~english
				 * @brief Method of determining the owner of a foreign block
				 *
				 * @param ptr pointer being resolved
				 * @return    nullptr: there are never multiple owners
				 *
				 * \~
				 */
				const functions_t * owner(const void * ptr) const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод получения названия способа захвата
				 *
				 * @return название способа захвата
				 *
				 * \~english
				 * @brief Method of getting the capture method name
				 *
				 * @return capture method name
				 *
				 * \~
				 */
				const char * name() const noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод задания откликов ветвления процесса
				 *
				 * @note Система сообщает зоне о ветвлении САМА, своими откликами
				 *       `force_lock`, `force_unlock` и `reinit_lock`, - и зовёт их
				 *       раньше всяких `pthread_atfork`. Заведи распределитель отклики
				 *       через `pthread_atfork`, они оказались бы позже откликов самой
				 *       библиотеки времени исполнения: та обратилась бы у потомка за
				 *       памятью, наткнулась на замок, захваченный перед ветвлением, и
				 *       встала бы навсегда. Проверено опытом на macOS
				 *
				 * @param before отклик перед ветвлением
				 * @param after  отклик у родителя после ветвления
				 * @param child  отклик у потомка после ветвления
				 *
				 * \~english
				 * @brief Method of setting the fork callbacks
				 *
				 * @note The system notifies the zone of forking ITSELF, with its own
				 *       callbacks `force_lock`, `force_unlock`, and `reinit_lock`, - and
				 *       calls them before any `pthread_atfork`. Were the allocator to
				 *       register callbacks via `pthread_atfork`, they would come after the
				 *       runtime library's own callbacks: the library would request memory
				 *       in the child, hit a lock acquired before the fork, and hang forever.
				 *       Verified by experiment on macOS
				 *
				 * @param before callback before forking
				 * @param after  callback in the parent after forking
				 * @param child  callback in the child after forking
				 *
				 * \~
				 */
				static void fork(void (* before)(), void (* after)(), void (* child)()) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения прежних функций выделения памяти
				 *
				 * @return прежние функции
				 *
				 * \~english
				 * @brief Method of getting the previous allocation functions
				 *
				 * @return previous functions
				 *
				 * \~
				 */
				const functions_t & originals() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				ZoneCapture() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~ZoneCapture() noexcept override;
		} zone_capture_t;
	};
};

#endif
