/**
 * @file status.cpp
 * @date 2026-08-06
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
 * @brief Тесты разбора состояния завершения процесса кластера —
 *        проверка того, что переносимые вопросы к состоянию дают одинаковые ответы
 *        на разном представлении состояния у разных операционных систем
 *
 * @details Состояние завершения приходит в обработчик события "exit" в том виде, в
 *          каком его отдаёт система: у POSIX это упакованное состояние ожидания от
 *          waitpid, у MS Windows - значение GetExitCodeProcess. К общему виду они
 *          намеренно не приводятся, поскольку каждое несёт и признак того, как процесс
 *          кончился, и то, с чем именно. Общими сделаны не значения, а вопросы к ним -
 *          вот их тесты и закрепляют
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/unit/cluster.hpp"

/**
 * Для операционных систем, отличных от MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Системные заголовочные файлы
	 */
	#include <csignal>
	#include <sys/wait.h>
#endif

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Функция построения состояния обычного завершения процесса
 *
 * @param code код возврата процесса
 * @return     состояние завершения в виде, отдаваемом системой
 *
 */
static int32_t exitedStatus(const int32_t code) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Код завершения отдаётся системой как есть
		return code;
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Код возврата занимает старший октет упакованного состояния
		return (code << 8);
	#endif
}

/**
 * @brief Функция построения состояния снятия процесса сигналом
 *
 * @param signal номер сигнала, снявшего процесс
 * @return       состояние завершения в виде, отдаваемом системой
 *
 * @note Соответствия сигналам у MS Windows нет: падения приходят туда значениями
 *       NTSTATUS. Подбор их и ведётся здесь по смыслу сигнала, а не по номеру
 *
 */
static int32_t signaledStatus(const int32_t signal) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Подбираем ближайшее по смыслу значение NTSTATUS
		 */
		switch(signal){
			// Обращению по недопустимому адресу отвечает STATUS_ACCESS_VIOLATION
			case 11: return static_cast <int32_t> (0xC0000005u);
			// Прерыванию с клавиатуры отвечает STATUS_CONTROL_C_EXIT
			case 2:  return static_cast <int32_t> (0xC000013Au);
		}
		// Для прочих случаев берём общий признак неудачи
		return static_cast <int32_t> (0xC0000001u);
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Номер сигнала занимает младшие разряды упакованного состояния
		return signal;
	#endif
}

/**
 * @brief Тест разбора обычного завершения процесса
 *
 */
TEST(ClusterStatusFixture, ExitedStatusTest){
	// Строим состояние обычного завершения с кодом возврата 42
	const int32_t status = ::exitedStatus(42);
	// Процесс завершился сам
	ASSERT_TRUE(unit::cluster_t::exited(status));
	// Код возврата извлекается неискажённым
	ASSERT_EQ(42, unit::cluster_t::exitcode(status));
	// Сигналом процесс снят не был
	ASSERT_FALSE(unit::cluster_t::signaled(status));
	// Номера сигнала у обычного завершения нет
	ASSERT_EQ(0, unit::cluster_t::termsig(status));
	// Завершение ненормальным не является
	ASSERT_FALSE(unit::cluster_t::crashed(status));
	// Ручной остановкой завершение не является
	ASSERT_FALSE(unit::cluster_t::manual(status));
}

/**
 * @brief Тест разбора нулевого кода возврата
 *
 */
TEST(ClusterStatusFixture, ExitedZeroStatusTest){
	// Строим состояние обычного завершения с нулевым кодом возврата
	const int32_t status = ::exitedStatus(0);
	// Процесс завершился сам
	ASSERT_TRUE(unit::cluster_t::exited(status));
	// Код возврата извлекается неискажённым
	ASSERT_EQ(0, unit::cluster_t::exitcode(status));
	// Завершение ненормальным не является
	ASSERT_FALSE(unit::cluster_t::crashed(status));
}

/**
 * @brief Тест разбора падения процесса
 *
 */
TEST(ClusterStatusFixture, CrashedStatusTest){
	// Строим состояние падения с обращением по недопустимому адресу
	const int32_t status = ::signaledStatus(11);
	// Завершение признаётся ненормальным на всех системах
	ASSERT_TRUE(unit::cluster_t::crashed(status));
	// Сам процесс не завершался
	ASSERT_FALSE(unit::cluster_t::exited(status));
	// Кодом возврата у падения считается признак неудачи
	ASSERT_EQ(EXIT_FAILURE, unit::cluster_t::exitcode(status));
	// Ручной остановкой падение не является
	ASSERT_FALSE(unit::cluster_t::manual(status));
	/**
	 * Для операционных систем, отличных от MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Процесс снят сигналом
		ASSERT_TRUE(unit::cluster_t::signaled(status));
		// Номер снявшего сигнала извлекается неискажённым
		ASSERT_EQ(SIGSEGV, unit::cluster_t::termsig(status));
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Сигналов у MS Windows нет вовсе
		ASSERT_FALSE(unit::cluster_t::signaled(status));
		// Номера сигнала у MS Windows нет вовсе
		ASSERT_EQ(0, unit::cluster_t::termsig(status));
	#endif
}

/**
 * @brief Тест разбора ручной остановки процесса
 *
 */
TEST(ClusterStatusFixture, ManualStatusTest){
	// Строим состояние прерывания с клавиатуры
	const int32_t status = ::signaledStatus(2);
	// Остановка признаётся ручной на всех системах
	ASSERT_TRUE(unit::cluster_t::manual(status));
	/**
	 * Падением ручная остановка не считается: процесс сняли намеренно, и отличать
	 * такое от падения обязаны обе системы, хотя помечено оно у MS Windows тем же
	 * признаком важности, что и падение
	 */
	ASSERT_FALSE(unit::cluster_t::crashed(status));
	// Сам процесс не завершался
	ASSERT_FALSE(unit::cluster_t::exited(status));
}

/**
 * @brief Тест разбора остановки воркера мастером
 *
 * @details Состояние это кластер выставляет сам, когда мастер останавливает воркера
 *          закрытием своего конца канала. Ни падением, ни ручной остановкой, ни
 *          обычным завершением оно не является - и одинаково не является на всех
 *          системах, хотя значение у каждой своё
 *
 */
TEST(ClusterStatusFixture, StoppedByMasterStatusTest){
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Состояние остановки воркера мастером у MS Windows
		const int32_t status = static_cast <int32_t> (0xE0000001u);
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Состояние остановки воркера мастером у POSIX
		const int32_t status = SIGSTOP;
	#endif
	// Падением остановка не является
	ASSERT_FALSE(unit::cluster_t::crashed(status));
	// Ручной остановкой она тоже не является
	ASSERT_FALSE(unit::cluster_t::manual(status));
	// Сам процесс не завершался
	ASSERT_FALSE(unit::cluster_t::exited(status));
	// Кодом возврата считается признак неудачи
	ASSERT_EQ(EXIT_FAILURE, unit::cluster_t::exitcode(status));
}
