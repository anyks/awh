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
 * @details <b>Захват идёт зоной, а не подменой имён.</b> macOS связывает имена
 *          двухуровнево: обращение из библиотеки времени исполнения помечено её же
 *          именем и нашим определением не заслоняется вовсе. Подмена именами
 *          заслонила бы там лишь обращения самой программы, оставив половину процесса
 *          прежнему распределителю, - а разделённый надвое распределитель означает
 *          выдачу одним и освобождение другим.
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
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_ZONE__
#define __AWH_ALLOC_ZONE__

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
#include "../sys/global.hpp"

/**
 * @brief Пространство имён фреймворка
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён распределителя памяти
	 *
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс захвата выделения памяти зоной macOS
		 *
		 * \~english
		 * @brief macOS malloc zone capture class
		 *
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
				 * @brief Метод захвата выделения памяти процесса
				 *
				 * @param hooks     наши функции, ставимые на место прежних
				 * @param originals прежние функции, отдаваемые захватом
				 * @return          признак состоявшегося захвата
				 *
				 */
				bool acquire(const functions_t & hooks, functions_t & originals) noexcept override;
				/**
				 * @brief Метод снятия захвата
				 *
				 */
				void release() noexcept override;
				/**
				 * @brief Метод определения состоявшегося захвата
				 *
				 * @return признак захвата
				 *
				 */
				bool acquired() const noexcept override;
			public:
				/**
				 * @brief Метод опознания указателя, выданного прежним распределителем
				 *
				 * @param ptr разбираемый указатель
				 * @return    признак принадлежности прежнему распределителю
				 *
				 */
				bool foreign(const void * ptr) const noexcept override;
			public:
				/**
				 * @brief Метод получения названия способа захвата
				 *
				 * @return название способа захвата
				 *
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
				 */
				const functions_t & originals() const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				ZoneCapture() noexcept : _originals(), _acquired(false) {}
				/**
				 * @brief Деструктор
				 *
				 */
				~ZoneCapture() noexcept override;
		} zone_capture_t;
	};
};

#endif // __APPLE__

#endif // __AWH_ALLOC_ZONE__
