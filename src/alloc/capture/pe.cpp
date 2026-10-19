/**
 * @file pe.cpp
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
 * @brief Реализация захвата выделения памяти переписыванием входа функций у MS Windows
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/pe.hpp>

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Системные заголовочные файлы
 */
#include <windows.h>

/**
 * @brief Пространство имён средств переписывания входа
 *
 */
namespace {
	/**
	 * Имена подменяемых функций в порядке полей набора functions_t
	 */
	static const char * const NAMES[] = {
		"malloc", "free", "calloc", "realloc", "_msize",
		"_aligned_malloc", "_aligned_free", "_aligned_realloc", "_aligned_msize"
	};
	// Прежние функции каждой библиотеки, заведённые постоянным выражением
	static awh::alloc::functions_t __awh_pe_originals__[2];
	// Отклики распределителя, общие для обеих библиотек
	static awh::alloc::functions_t __awh_pe_hooks__;
	// Владелец установленных подмен
	static const awh::alloc::PECapture * __awh_pe_owner__ = nullptr;
	/**
	 * @brief Функция передачи вызова с сохранением исходной CRT
	 *
	 * @tparam Index номер библиотеки времени исполнения
	 * @param ptr адрес освобождаемого блока
	 *
	 */
	template <size_t Index>
	static void __awh_pe_free__(void * ptr) {
		// Передаём прежние функции именно той библиотеки, через которую пришёл вызов
		(* __awh_pe_hooks__.freeFrom)(ptr, __awh_pe_originals__[Index]);
	}
	/**
	 * @brief Функция передачи вызова с сохранением исходной CRT
	 *
	 * @tparam Index номер библиотеки времени исполнения
	 * @param ptr адрес изменяемого блока
	 * @param size требуемый размер
	 * @return адрес выданной памяти либо nullptr
	 *
	 */
	template <size_t Index>
	static void * __awh_pe_realloc__(void * ptr, size_t size) {
		// Передаём прежние функции именно той библиотеки, через которую пришёл вызов
		return (* __awh_pe_hooks__.reallocFrom)(ptr, size, __awh_pe_originals__[Index]);
	}
	/**
	 * @brief Функция передачи вызова с сохранением исходной CRT
	 *
	 * @tparam Index номер библиотеки времени исполнения
	 * @param ptr разбираемый адрес
	 * @return размер блока в байтах
	 *
	 */
	template <size_t Index>
	static size_t __awh_pe_msize__(const void * ptr) {
		// Передаём прежние функции именно той библиотеки, через которую пришёл вызов
		return (* __awh_pe_hooks__.msizeFrom)(ptr, __awh_pe_originals__[Index]);
	}
	/**
	 * @brief Функция передачи вызова с сохранением исходной CRT
	 *
	 * @tparam Index номер библиотеки времени исполнения
	 * @param ptr адрес освобождаемого блока
	 *
	 */
	template <size_t Index>
	static void __awh_pe_alignedFree__(void * ptr) {
		// Передаём прежние функции именно той библиотеки, через которую пришёл вызов
		(* __awh_pe_hooks__.alignedFreeFrom)(ptr, __awh_pe_originals__[Index]);
	}
	/**
	 * @brief Функция передачи вызова с сохранением исходной CRT
	 *
	 * @tparam Index номер библиотеки времени исполнения
	 * @param ptr адрес изменяемого блока
	 * @param size требуемый размер
	 * @param alignment выравнивание блока
	 * @return адрес выданной памяти либо nullptr
	 *
	 */
	template <size_t Index>
	static void * __awh_pe_alignedRealloc__(void * ptr, size_t size, size_t alignment) {
		// Передаём прежние функции именно той библиотеки, через которую пришёл вызов
		return (* __awh_pe_hooks__.alignedReallocFrom)(ptr, size, alignment, __awh_pe_originals__[Index]);
	}
	/**
	 * @brief Функция передачи вызова с сохранением исходной CRT
	 *
	 * @tparam Index номер библиотеки времени исполнения
	 * @param ptr разбираемый адрес
	 * @param alignment выравнивание блока
	 * @param offset смещение блока
	 * @return размер блока в байтах
	 *
	 */
	template <size_t Index>
	static size_t __awh_pe_alignedMsize__(void * ptr, size_t alignment, size_t offset) {
		// Передаём прежние функции именно той библиотеки, через которую пришёл вызов
		return (* __awh_pe_hooks__.alignedMsizeFrom)(ptr, alignment, offset, __awh_pe_originals__[Index]);
	}
	/**
	 * @brief Функция построения набора подмен для одной CRT
	 *
	 * @tparam Index номер библиотеки времени исполнения
	 * @param hooks общие отклики распределителя
	 * @return      отклики с сохранением исходной библиотеки
	 *
	 */
	template <size_t Index>
	static awh::alloc::functions_t __awh_pe_targets__(const awh::alloc::functions_t & hooks) noexcept {
		// Сохраняем общие функции выдачи
		awh::alloc::functions_t result = hooks;
		// Назначаем отклик с номером исходной библиотеки
		result.free = &__awh_pe_free__ <Index>;
		// Назначаем отклик с номером исходной библиотеки
		result.realloc = &__awh_pe_realloc__ <Index>;
		// Назначаем отклик с номером исходной библиотеки
		result.msize = &__awh_pe_msize__ <Index>;
		// Назначаем отклик с номером исходной библиотеки
		result.alignedFree = &__awh_pe_alignedFree__ <Index>;
		// Назначаем отклик с номером исходной библиотеки
		result.alignedRealloc = &__awh_pe_alignedRealloc__ <Index>;
		// Назначаем отклик с номером исходной библиотеки
		result.alignedMsize = &__awh_pe_alignedMsize__ <Index>;
		// Выводим подготовленный набор
		return result;
	}
	/**
	 * @brief Функция извлечения адреса поля набора функций
	 *
	 * @param functions разбираемый набор функций
	 * @param index     номер требуемого поля
	 * @return          адрес функции из требуемого поля
	 *
	 */
	static const void * pick(const awh::alloc::functions_t & functions, const size_t index) noexcept {
		/**
		 * Определяем требуемое поле
		 */
		switch(index){
			// Выделение памяти
			case 0: return reinterpret_cast <const void *> (functions.malloc);
			// Освобождение памяти
			case 1: return reinterpret_cast <const void *> (functions.free);
			// Выделение обнулённой памяти
			case 2: return reinterpret_cast <const void *> (functions.calloc);
			// Изменение размера выделенной памяти
			case 3: return reinterpret_cast <const void *> (functions.realloc);
			// Определение размера выделенного блока
			case 4: return reinterpret_cast <const void *> (functions.msize);
			// Выделение памяти с требуемым выравниванием
			case 5: return reinterpret_cast <const void *> (functions.alignedAlloc);
			// Освобождение памяти, выданной с выравниванием
			case 6: return reinterpret_cast <const void *> (functions.alignedFree);
			// Изменение размера памяти, выданной с выравниванием
			case 7: return reinterpret_cast <const void *> (functions.alignedRealloc);
			// Определение размера памяти, выданной с выравниванием
			case 8: return reinterpret_cast <const void *> (functions.alignedMsize);
		}
		// Поля с таким номером нет
		return nullptr;
	}
	/**
	 * @brief Функция записи адреса в поле набора функций
	 *
	 * @param functions заполняемый набор функций
	 * @param index     номер требуемого поля
	 * @param addr      записываемый адрес
	 *
	 */
	static void put(awh::alloc::functions_t & functions, const size_t index, void * addr) noexcept {
		/**
		 * Определяем требуемое поле
		 */
		switch(index){
			// Выделение памяти
			case 0: functions.malloc = reinterpret_cast <void * (*)(size_t)> (addr); break;
			// Освобождение памяти
			case 1: functions.free = reinterpret_cast <void (*)(void *)> (addr); break;
			// Выделение обнулённой памяти
			case 2: functions.calloc = reinterpret_cast <void * (*)(size_t, size_t)> (addr); break;
			// Изменение размера выделенной памяти
			case 3: functions.realloc = reinterpret_cast <void * (*)(void *, size_t)> (addr); break;
			// Определение размера выделенного блока
			case 4: functions.msize = reinterpret_cast <size_t (*)(const void *)> (addr); break;
			// Выделение памяти с требуемым выравниванием
			case 5: functions.alignedAlloc = reinterpret_cast <void * (*)(size_t, size_t)> (addr); break;
			// Освобождение памяти, выданной с выравниванием
			case 6: functions.alignedFree = reinterpret_cast <void (*)(void *)> (addr); break;
			// Изменение размера памяти, выданной с выравниванием
			case 7: functions.alignedRealloc = reinterpret_cast <void * (*)(void *, size_t, size_t)> (addr); break;
			// Определение размера памяти, выданной с выравниванием
			case 8: functions.alignedMsize = reinterpret_cast <size_t (*)(void *, size_t, size_t)> (addr); break;
		}
	}
};

/**
 * @brief Метод разбора цели перехода горячей заплатки
 *
 * @param entry адрес входа функции
 * @return      адрес настоящего тела либо nullptr
 *
 */
void * awh::alloc::PECapture::body(void * entry) const noexcept {
	// Если вход не задан
	if(entry == nullptr)
		// Разбирать нечего
		return nullptr;
	/**
	 * Для набора команд ARM64
	 *
	 * Вход функций выделения памяти у ucrtbase начинается горячей заплаткой: переход
	 * через набивку, две пустые команды и четыре байта набивки, итого шестнадцать байт,
	 * отведённых Microsoft под подмену. Настоящее тело начинается за ними, и цель
	 * перехода на него и указывает. Команды у ARM64 по четыре байта, границу искать
	 * не требуется, оттого разбирателя набора команд здесь нет
	 */
	#if defined(_M_ARM64) || defined(__aarch64__)
		// Читаем первое слово входа
		const uint32_t word = *reinterpret_cast <const uint32_t *> (entry);
		// Если это не безусловный переход, вход заплаткой не начинается
		if((word & 0xFC000000u) != 0x14000000u)
			// Отвечаем отказом: переписывать вслепую нельзя
			return nullptr;
		// Извлекаем смещение перехода в словах
		int32_t offset = static_cast <int32_t> (word & 0x03FFFFFFu);
		/**
		 * Расширяем знак двадцатишестиразрядного смещения
		 *
		 * Расширение обязательно: у _msize и _recalloc переход отрицательный, тело их
		 * лежит раньше входа, и без расширения знака адрес вышел бы диким
		 */
		if(offset & 0x02000000)
			// Доводим до отрицательного числа
			offset |= static_cast <int32_t> (0xFC000000u);
		// Возвращаем адрес настоящего тела
		return (reinterpret_cast <uint8_t *> (entry) + (static_cast <intptr_t> (offset) * 4));
	/**
	 * Для прочих наборов команд
	 *
	 * У x86-64 команды переменной длины, и границу их без разбирателя не найти.
	 * Разбиратель этот здесь не написан намеренно: Windows x86-64 покрыта иными
	 * средствами, а ARM64 - тот случай, ради какого модуль и заведён
	 */
	#else
		// Отвечаем отказом: способ разбора для этого набора команд не написан
		return nullptr;
	#endif
}
/**
 * Для набора команд x86-64
 */
#if defined(_M_X64) || defined(__x86_64__)
	/**
	 * @brief Измерение длины поля адресации команды x86-64 (ModRM + SIB + смещение)
	 *
	 * @param field адрес байта ModRM
	 * @return      число байт поля адресации, либо нуль при адресации от счётчика команд
	 *
	 * @note Собрано из трёх повторявшихся разборов в одно: там терялось четырёхоктетное
	 *       смещение у формы `mod=00` с основанием 101 в байте масштаба (`[index*scale +
	 *       disp32]`). Команда мерилась на четыре октета короче, и мостик вставал в
	 *       середину команды - молчаливая порча кода библиотеки, а не безопасный отказ
	 *
	 */
	static size_t __awh_pe_operand__(const uint8_t * field) noexcept {
		// Извлекаем поле вида адресации
		const uint8_t mod = static_cast <uint8_t> ((field[0] >> 6) & 0x03u);
		// Извлекаем поле регистра-основания
		const uint8_t rm = static_cast <uint8_t> (field[0] & 0x07u);
		// Длина поля начинается с самого байта ModRM
		size_t length = 1;
		// Регистровая адресация: ни байта масштаба, ни смещения
		if(mod == 0x03u)
			// Поле измерено
			return length;
		// Адресация от счётчика команд: перенос в мостик увёл бы обращение мимо
		if((mod == 0x00u) && (rm == 0x05u))
			// Отвечаем отказом
			return 0;
		// Основание адресации: у формы с байтом масштаба берётся из него
		uint8_t base = rm;
		// Если адресация идёт через байт масштаба
		if(rm == 0x04u){
			// Основание берём из байта масштаба
			base = static_cast <uint8_t> (field[1] & 0x07u);
			// Учитываем байт масштаба
			length++;
		}
		// Если у команды однобайтовое смещение
		if(mod == 0x01u)
			// Учитываем однобайтовое смещение
			length++;
		// Если у команды четырёхбайтовое смещение
		else if(mod == 0x02u)
			// Учитываем четырёхбайтовое смещение
			length += 4;
		// Форма mod=00 с основанием 101 несёт четырёхбайтовое смещение без основания
		else if((mod == 0x00u) && (base == 0x05u))
			// Учитываем четырёхбайтовое смещение
			length += 4;
		// Выводим длину поля адресации
		return length;
	}
#endif
/**
 * @brief Метод измерения длины команд входа функции
 *
 * @param entry адрес входа функции
 * @param need  требуемое число байт
 * @return      длина целого числа команд не менее требуемой, либо нуль
 *
 */
size_t awh::alloc::PECapture::prologue(const void * entry, const size_t need, size_t * offsets, size_t * count) const noexcept {
	// Если разбирать нечего
	if((entry == nullptr) || (need == 0))
		// Разбирать нечего
		return 0;
	/**
	 * Для набора команд x86-64
	 */
	#if defined(_M_X64) || defined(__x86_64__)
		// Разбираемый код
		const uint8_t * code = reinterpret_cast <const uint8_t *> (entry);
		// Пройденная длина в байтах
		size_t passed = 0;
		// Число разобранных команд
		size_t parsed = 0;
		/**
		 * Разбираем команды, пока не наберём требуемой длины
		 */
		while(passed < need){
			// Если требуются смещения начал команд
			if(offsets != nullptr)
				// Записываем смещение начала очередной команды
				offsets[parsed] = passed;
			// Увеличиваем число разобранных команд
			parsed++;
			// Начало очередной команды
			const uint8_t * command = (code + passed);
			// Длина очередной команды
			size_t length = 0;
			/**
			 * Метка защиты потока управления endbr64 (F3 0F 1E FA)
			 *
			 * Ею начинается вход функций у сборок ucrtbase с защитой CET (Windows 11).
			 * Четыре октета, от места не зависит, в мостик переносится дословно. Не
			 * распознай мы её - чтение кода операции упёрлось бы в приставку F3, и захват
			 * отменялся бы целиком на всякой такой сборке
			 */
			if((command[0] == 0xF3u) && (command[1] == 0x0Fu) && (command[2] == 0x1Eu) && (command[3] == 0xFAu)){
				// Учитываем метку целиком
				passed += 4;
				// Если метка вышла за область подмены
				if(passed > PATCH_SIZE)
					// Отвечаем отказом: сохранять столько некуда
					return 0;
				// Метка разобрана, переходим к следующей команде
				continue;
			}
			// Признак расширяющей приставки с разрядом W (задаёт разрядность непосредственного)
			bool wide = false;
			// Признак наличия расширяющей приставки перед кодом операции
			bool rex = false;
			/**
			 * Пропускаем расширяющую приставку
			 *
			 * Иных приставок в начале функций не встречается, и знать их разбирателю
			 * незачем: на всякой незнакомой он отвечает отказом
			 */
			if((command[0] >= 0x40u) && (command[0] <= 0x4Fu)){
				// Запоминаем разряд W приставки
				wide = ((command[0] & 0x08u) != 0u);
				// Отмечаем наличие расширяющей приставки
				rex = true;
				// Учитываем расширяющую приставку
				length++;
			}
			// Запоминаем код операции
			const uint8_t opcode = command[length];
			// Учитываем код операции
			length++;
			/**
			 * Разбираем код операции
			 */
			switch(opcode){
				/**
				 * Безусловный переход по относительному смещению
				 *
				 * Встречается прямо на входе: `_aligned_malloc` у `msvcrt.dll` - это
				 * `xor r8d, r8d` да переход на общее тело, три октета и пять. Не знай
				 * разбиратель этой команды, он останавливался бы на трёх октетах,
				 * пятерых для подмены не набирал и отвечал отказом - выровненная семья
				 * оттого и не перехватывалась вовсе
				 *
				 * Переносить такую команду ДОСЛОВНО нельзя: смещение у неё считается от
				 * своего же места, и в мостике оно указало бы мимо. Перенос пересчитывает
				 * смещение - тем же порядком, каким уже переносятся короткие условные
				 * переходы
				 */
				case 0xE9: {
					/**
					 * Отказываем относительному переходу под расширяющей приставкой
					 *
					 * Разбиратель длин снимает приставку прежде кода операции, а
					 * пересборщик смещений в переходнике различает команды по ПЕРВОМУ
					 * октету входа: голый E9 он пересчитывает, а `REX+E9` попал бы в
					 * ветвь дословного переноса - смещение считается от своего места, и
					 * в переходнике указало бы мимо, порча управления. Реальные CRT
					 * приставки перед `JMP rel` не ставят (разряды её тут бессмысленны),
					 * так что отказ лишь смыкает разбор с переносом, ничего годного не
					 * теряя
					 */
					if(rex)
						// Отвечаем отказом: такой переход перенести нечем
						return 0;
					// Учитываем четырёхоктетное смещение перехода
					length += 4;
					// Разбор команды окончен
					break;
				}
				/**
				 * Команды без дополнительных байт: занесение и снятие со стека
				 */
				case 0x50: case 0x51: case 0x52: case 0x53:
				case 0x54: case 0x55: case 0x56: case 0x57:
				case 0x58: case 0x59: case 0x5A: case 0x5B:
				case 0x5C: case 0x5D: case 0x5E: case 0x5F:
					// Дополнительных байт у команды нет
				break;
				/**
				 * Команды с байтом вида операнда
				 *
				 * Длина поля адресации (ModRM + байт масштаба + смещение) измеряется одним
				 * помощником: разбор был трижды повторён, и в каждом терялось смещение у
				 * формы mod=00 с основанием 101 в байте масштаба
				 */
				case 0x31: case 0x33: case 0x85: case 0x89:
				case 0x8B: case 0x8D: {
					// Измеряем поле адресации
					const size_t field = __awh_pe_operand__(command + length);
					// Отказываем адресации от счётчика команд: она неперемещаема
					if(field == 0)
						// Отвечаем отказом
						return 0;
					// Учитываем поле адресации
					length += field;
				} break;
				/**
				 * Команды с байтом вида операнда и однобайтовым непосредственным значением
				 */
				case 0x83: {
					// Измеряем поле адресации
					const size_t field = __awh_pe_operand__(command + length);
					// Отказываем адресации от счётчика команд
					if(field == 0)
						// Отвечаем отказом
						return 0;
					// Учитываем поле адресации и однооктетное непосредственное значение
					length += (field + 1);
				} break;
				/**
				 * Команды слоя ALU с байтом вида операнда и четырёхоктетным непосредственным
				 *
				 * Сюда входит `sub rsp, imm32` (48 81 EC ...) - пролог с кадром стека
				 * больше 0x7F октетов, обычный у `realloc`/`calloc` в части сборок CRT.
				 * Однооктетный собрат (0x83) разбирался, а этот - нет, и захват отменялся
				 * целиком той же ценой, что и на неизвестном `free`
				 */
				case 0x81: {
					// Измеряем поле адресации
					const size_t field = __awh_pe_operand__(command + length);
					// Отказываем адресации от счётчика команд
					if(field == 0)
						// Отвечаем отказом
						return 0;
					// Учитываем поле адресации и четырёхоктетное непосредственное значение
					length += (field + 4);
				} break;
				/**
				 * Команда с байтом вида операнда и однооктетным непосредственным (mov r/m8, imm8)
				 */
				case 0xC6: {
					// Измеряем поле адресации
					const size_t field = __awh_pe_operand__(command + length);
					// Отказываем адресации от счётчика команд
					if(field == 0)
						// Отвечаем отказом
						return 0;
					// Учитываем поле адресации и однооктетное непосредственное значение
					length += (field + 1);
				} break;
				/**
				 * Команда с байтом вида операнда и четырёхоктетным непосредственным значением
				 *
				 * Встречается прямо на входе `free` у `ucrtbase.dll`: обнуление слота на
				 * стеке `mov dword ptr [rsp+0x10], 0` (C7 44 24 10 00 00 00 00). Не знай
				 * её разбиратель, вход `free` признавался бы непригодным, и захват
				 * отменялся ЦЕЛИКОМ - обязательная пятёрка берётся всё или ничего. На
				 * x86-64 это всплывает лишь у ucrtbase: у msvcrt (сборка MinGW) вход free
				 * иной. Тело её от места не зависит, и перенос в мостик идёт дословно
				 */
				case 0xC7: {
					// Измеряем поле адресации
					const size_t field = __awh_pe_operand__(command + length);
					// Отказываем адресации от счётчика команд
					if(field == 0)
						// Отвечаем отказом
						return 0;
					// Учитываем поле адресации и четырёхоктетное непосредственное значение
					length += (field + 4);
				} break;
				/**
				 * Занесение непосредственного значения в регистр
				 *
				 * Непосредственное восьмиоктетно при разряде W расширяющей приставки,
				 * иначе четырёхоктетно. Значение от места не зависит, перенос дословный
				 */
				case 0xB8: case 0xB9: case 0xBA: case 0xBB:
				case 0xBC: case 0xBD: case 0xBE: case 0xBF:
					// Учитываем непосредственное значение по разрядности приставки
					length += (wide ? 8u : 4u);
				break;
				/**
				 * Условный переход по однобайтовому смещению
				 *
				 * Смещение его считается от адреса следующей команды, и перенос требует
				 * пересчёта. Пересчёт этот делается при сборке переходника, а здесь
				 * команда лишь измеряется
				 */
				case 0x70: case 0x71: case 0x72: case 0x73:
				case 0x74: case 0x75: case 0x76: case 0x77:
				case 0x78: case 0x79: case 0x7A: case 0x7B:
				case 0x7C: case 0x7D: case 0x7E: case 0x7F:
					/**
					 * Отказываем условному переходу под расширяющей приставкой
					 *
					 * Довод тот же, что у безусловного перехода: пересборщик смещений
					 * различает команды по первому октету входа и пересчитывает лишь
					 * голый переход, а `REX+Jcc` унёс бы однооктетное смещение дословно
					 * мимо цели. Приставки перед `Jcc rel` реальные CRT не ставят
					 */
					if(rex)
						// Отвечаем отказом: такой переход перенести нечем
						return 0;
					// Учитываем однобайтовое смещение перехода
					length++;
				break;
				/**
				 * Прочие команды разбирателю незнакомы
				 */
				default:
					// Отвечаем отказом: переписывать вслепую нельзя
					return 0;
			}
			// Увеличиваем пройденную длину
			passed += length;
			// Если пройденная длина вышла за область подмены
			if(passed > PATCH_SIZE)
				// Отвечаем отказом: сохранять столько некуда
				return 0;
		}
		// Если требуется число разобранных команд
		if(count != nullptr)
			// Записываем число разобранных команд
			(* count) = parsed;
		// Выводим пройденную длину
		return passed;
	/**
	 * Для прочих наборов команд
	 */
	#else
		// Отмечаем неиспользуемые параметры
		static_cast <void> (entry);
		// Отмечаем неиспользуемые параметры
		static_cast <void> (need);
		// Отмечаем неиспользуемые параметры
		static_cast <void> (offsets);
		// Отмечаем неиспользуемые параметры
		static_cast <void> (count);
		// Разбиратель здесь не нужен: вход разбирается заплаткой
		return 0;
	#endif
}
/**
 * @brief Метод определения пригодности входа функции к подмене
 *
 * @param entry адрес входа функции
 * @return      признак пригодности входа
 *
 */
bool awh::alloc::PECapture::suits(void * entry) const noexcept {
	/**
	 * Для набора команд ARM64
	 */
	#if defined(_M_ARM64) || defined(__aarch64__)
		// Вход пригоден, если он начинается горячей заплаткой
		return (this->body(entry) != nullptr);
	/**
	 * Для набора команд x86-64
	 */
	#elif defined(_M_X64) || defined(__x86_64__)
		// Вход пригоден, если начало его разбирается на целое число команд
		return (this->prologue(entry, 5, nullptr, nullptr) > 0);
	/**
	 * Для прочих наборов команд
	 */
	#else
		// Отмечаем неиспользуемый параметр
		static_cast <void> (entry);
		// Способ подмены для этого набора команд не написан
		return false;
	#endif
}
/**
 * @brief Метод выдачи памяти под переходник вблизи образа
 *
 * @param anchor адрес, вблизи которого нужна память
 * @param size требуемый размер в байтах
 * @return     адрес выданной памяти либо nullptr
 *
 */
/**
 * Имя `anchor`, а не `near`: у MS Windows `near` - слово, занятое заголовками системы
 * со времён сегментной памяти, и параметр с таким именем валит сборку
 */
void * awh::alloc::PECapture::nearby(const void * anchor, const size_t size) noexcept {
	// Если в уже взятой области место есть
	if((this->_arena[this->_library] != nullptr) && ((this->_arenaUsed[this->_library] + size) <= this->_arenaSize[this->_library])){
		// Запоминаем адрес выдачи
		void * result = (reinterpret_cast <uint8_t *> (this->_arena[this->_library]) + this->_arenaUsed[this->_library]);
		// Сдвигаем занятое в области
		this->_arenaUsed[this->_library] += size;
		// Выводим адрес выданной памяти
		return result;
	}
	// Сведения о разбираемом участке памяти
	::SYSTEM_INFO info;
	// Получаем сведения о системе
	::GetSystemInfo(&info);
	// Определяем шаг поиска: зернистость выдачи памяти системой
	const uintptr_t step = static_cast <uintptr_t> (info.dwAllocationGranularity);
	// Определяем начало поиска, приведённое к зернистости
	const uintptr_t origin = ((reinterpret_cast <uintptr_t> (anchor) / step) * step);
	/**
	 * Ищем свободное место в пределах досягаемости относительного перехода
	 *
	 * Переход по относительному смещению достаёт не далее двух гигабайт в обе стороны.
	 * Ищем в обе, начиная от самого образа: рядом с ним свободное место находится
	 * почти всегда
	 */
	for(uintptr_t shift = step; shift < (uintptr_t) 0x60000000ull; shift += step){
		/**
		 * Пробуем оба направления
		 */
		for(int32_t side = 0; side < 2; side++){
			// Определяем пробуемый адрес
			const uintptr_t probe = ((side == 0) ? (origin + shift) : (origin - shift));
			// Если адрес ушёл ниже начала памяти
			if((side != 0) && (origin < shift))
				// Пробовать нечего
				continue;
			// Пробуем взять память по пробуемому адресу
			void * block = ::VirtualAlloc(reinterpret_cast <void *> (probe), step, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
			// Если память не выдана
			if(block == nullptr)
				// Пробуем следующий адрес
				continue;
			// Запоминаем взятую область
			this->_arena[this->_library] = block;
			// Запоминаем размер взятой области
			this->_arenaSize[this->_library] = static_cast <size_t> (step);
			// Запоминаем занятое в области
			this->_arenaUsed[this->_library] = size;
			// Выводим адрес выданной памяти
			return block;
		}
	}
	// Свободного места вблизи образа не нашлось
	return nullptr;
}
/**
 * @brief Метод наложения подмены на вход функции
 *
 * @param patch     сведения о подмене
 * @param entry     адрес входа подменяемой функции
 * @param target    адрес подставляемой функции
 * @param originals прежние функции исходной библиотеки
 * @param index     номер подменяемой функции
 * @return          признак успеха
 *
 */
bool awh::alloc::PECapture::apply(patch_t & patch, void * entry, const void * target, functions_t & originals, const size_t index) noexcept {
	// Если вход или подставляемая функция не заданы
	if((entry == nullptr) || (target == nullptr))
		// Отвечаем отказом
		return false;
	/**
	 * Для набора команд ARM64
	 */
	#if defined(_M_ARM64) || defined(__aarch64__)
		/**
		 * Разбираем цель перехода горячей заплатки
		 *
		 * Разбор этот СВОЙ у ARM64 и негоден прочим: у x86-64 заплатки на входе нет, и
		 * стой он общей проверкой наверху - подмена отказывала бы там, не дойдя до
		 * своей ветви вовсе (так и было: щуп отчитывался отказом при вполне разобранном
		 * входе)
		 */
		void * body = this->body(entry);
		// Если вход заплаткой не начинается
		if(body == nullptr)
			// Отвечаем отказом: переписывать вслепую нельзя
			return false;
		// Запоминаем адрес входа
		patch.entry = entry;
		// Запоминаем адрес настоящего тела
		patch.body = body;
		// Запоминаем число вытесненных байт: у ARM64 это вся область подмены
		patch.moved = PATCH_SIZE;
		// Сохраняем прежнее содержимое области подмены
		::memcpy(patch.saved, entry, PATCH_SIZE);
		// Прежние права доступа к странице
		DWORD previous = 0;
		// Открываем страницу на запись
		if(!::VirtualProtect(entry, PATCH_SIZE, PAGE_EXECUTE_READWRITE, &previous))
			// Отвечаем отказом
			return false;
		// Собираемое содержимое области подмены
		uint8_t buffer[PATCH_SIZE];
		// Команда чтения адреса по метке: LDR X16, +8
		const uint32_t ldr = 0x58000050u;
		// Команда перехода по регистру: BR X16
		const uint32_t br = 0xD61F0200u;
		/**
		 * Регистр X16 (IP0) на входе функции затираем по соглашению о вызовах,
		 * оттого пользоваться им здесь безопасно
		 */
		// Записываем команду чтения адреса
		::memcpy(buffer, &ldr, sizeof(ldr));
		// Записываем команду перехода
		::memcpy(buffer + 4, &br, sizeof(br));
		// Записываем адрес подставляемой функции
		::memcpy(buffer + 8, &target, sizeof(target));
		// Публикуем прежнюю функцию до включения перехода на наш отклик
		put(originals, index, patch.body);
		// Переносим собранное на вход функции
		::memcpy(entry, buffer, PATCH_SIZE);
		// Возвращаем прежние права доступа
		::VirtualProtect(entry, PATCH_SIZE, previous, &previous);
		/**
		 * Сбрасываем кэш команд: без этого процессор исполняет прежний код, и подмена
		 * оказывается наложенной лишь по виду
		 */
		::FlushInstructionCache(::GetCurrentProcess(), entry, PATCH_SIZE);
		// Отмечаем подмену наложенной
		patch.applied = true;
		// Отвечаем успехом
		return true;
	/**
	 * Для набора команд x86-64
	 *
	 * Горячей заплатки на входе здесь нет: у функций библиотеки времени исполнения
	 * начало входа занято настоящими командами. Оттого подмена ВЫТЕСНЯЕТ их, а
	 * вытесненные переносятся в переходник, откуда исполнение возвращается за место
	 * подмены
	 */
	#elif defined(_M_X64) || defined(__x86_64__)
		// Смещения начал вытесняемых команд
		size_t offsets[PATCH_SIZE] = {0};
		// Число вытесняемых команд
		size_t count = 0;
		// Измеряем длину вытесняемых команд
		const size_t moved = this->prologue(entry, 5, offsets, &count);
		// Если начало входа разобрать не удалось
		if(moved == 0)
			// Отвечаем отказом: переписывать вслепую нельзя
			return false;
		/**
		 * Берём память под переходники ВБЛИЗИ подменяемой функции
		 *
		 * Относительный переход достаёт не далее двух гигабайт, а наши функции лежат в
		 * образе программы - от библиотеки времени исполнения это семь гигабайт и более
		 */
		// Берём память под мостик до нашей функции
		uint8_t * bridge = reinterpret_cast <uint8_t *> (this->nearby(entry, 14));
		// Если память под мостик не выдана
		if(bridge == nullptr)
			// Отвечаем отказом
			return false;
		// Берём память под переходник к прежнему телу
		uint8_t * relay = reinterpret_cast <uint8_t *> (this->nearby(entry, (moved + (count * 4u) + 14u)));
		// Если память под переходник не выдана
		if(relay == nullptr)
			// Отвечаем отказом
			return false;
		/**
		 * Собираем мостик: абсолютный переход на нашу функцию
		 *
		 * Команда FF 25 с нулевым смещением берёт адрес перехода из восьми байт,
		 * лежащих сразу за нею
		 */
		// Записываем команду абсолютного перехода
		bridge[0] = 0xFFu;
		// Записываем вид операнда команды перехода
		bridge[1] = 0x25u;
		// Обнуляем смещение до адреса перехода
		::memset(bridge + 2, 0, 4);
		// Записываем адрес нашей функции
		::memcpy(bridge + 6, &target, sizeof(target));
		/**
		 * Собираем переходник: вытесненные команды да возврат за место подмены
		 */
		// Занято в переходнике
		size_t used = 0;
		/**
		 * Переносим вытесненные команды по одной
		 */
		for(size_t i = 0; i < count; i++){
			// Определяем начало очередной команды
			const uint8_t * command = (reinterpret_cast <const uint8_t *> (entry) + offsets[i]);
			// Определяем длину очередной команды
			const size_t length = (((i + 1) < count) ? (offsets[i + 1] - offsets[i]) : (moved - offsets[i]));
			/**
			 * Если команда - условный переход по однобайтовому смещению
			 *
			 * Смещение его считается от адреса следующей команды, и перенос требует
			 * пересчёта. Пересчитанное смещение в один байт уже не уложится, оттого
			 * команда пересобирается в двухбайтовый вид с четырёхбайтовым смещением
			 */
			/**
			 * Переносим безусловный переход с пересчётом смещения
			 *
			 * Смещение считается от места СЛЕДУЮЩЕЙ команды, а в мостике место это
			 * иное: перенеси мы команду дословно - переход указал бы мимо тела
			 */
			if(command[0] == 0xE9u){
				// Прочитанное смещение перехода
				int32_t offset = 0;
				// Снимаем смещение перехода
				::memcpy(&offset, (command + 1), sizeof(offset));
				// Определяем настоящую цель перехода
				const uint8_t * aim = (command + 5 + static_cast <intptr_t> (offset));
				/**
				 * Если цель лежит внутри вытесняемой области
				 *
				 * Перенос её означал бы переход на команды, каких в мостике нет: место
				 * подмены занято нашим переходом
				 */
				if((aim >= reinterpret_cast <const uint8_t *> (entry)) && (aim < (reinterpret_cast <const uint8_t *> (entry) + moved)))
					// Отвечаем отказом
					return false;
				// Записываем код операции безусловного перехода
				relay[used] = 0xE9u;
				// Определяем пересчитанное смещение перехода
				const int64_t shift = (static_cast <int64_t> (aim - (relay + used + 5)));
				// Если пересчитанное смещение не укладывается в четыре байта
				if((shift > 0x7FFFFFFFll) || (shift < -0x80000000ll))
					// Отвечаем отказом
					return false;
				// Сужаем пересчитанное смещение перехода
				const int32_t narrow = static_cast <int32_t> (shift);
				// Переносим пересчитанное смещение
				::memcpy((relay + used + 1), &narrow, sizeof(narrow));
				// Учитываем занятое командой место
				used += 5;
			} else if((command[0] >= 0x70u) && (command[0] <= 0x7Fu)){
				// Определяем настоящую цель перехода
				const uint8_t * aim = (command + 2 + static_cast <intptr_t> (static_cast <int8_t> (command[1])));
				/**
				 * Если цель перехода лежит ВНУТРИ вытесняемого куска
				 *
				 * Такой переход после переноса указывал бы на прежнее место, а команды
				 * там уже иные. Пересчитать его можно, но случая проверить нет, а писать
				 * наугад мы не станем
				 */
				if((aim >= reinterpret_cast <const uint8_t *> (entry)) && (aim < (reinterpret_cast <const uint8_t *> (entry) + moved)))
					// Отвечаем отказом
					return false;
				// Записываем расширяющий код операции
				relay[used] = 0x0Fu;
				// Записываем код операции двухбайтового условного перехода
				relay[used + 1] = static_cast <uint8_t> (command[0] + 0x10u);
				// Определяем пересчитанное смещение перехода
				const int64_t shift = (static_cast <int64_t> (aim - (relay + used + 6)));
				// Если пересчитанное смещение не укладывается в четыре байта
				if((shift > 0x7FFFFFFFll) || (shift < -0x80000000ll))
					// Отвечаем отказом
					return false;
				// Записываем пересчитанное смещение перехода
				const int32_t narrow = static_cast <int32_t> (shift);
				// Переносим пересчитанное смещение
				::memcpy(relay + used + 2, &narrow, sizeof(narrow));
				// Учитываем занятое командой место
				used += 6;
			/**
			 * Прочие команды переносятся как есть
			 */
			} else {
				// Переносим команду в переходник
				::memcpy(relay + used, command, length);
				// Учитываем занятое командой место
				used += length;
			}
		}
		// Определяем адрес возврата за место подмены
		const void * back = (reinterpret_cast <const uint8_t *> (entry) + moved);
		// Записываем команду абсолютного перехода
		relay[used] = 0xFFu;
		// Записываем вид операнда команды перехода
		relay[used + 1] = 0x25u;
		// Обнуляем смещение до адреса перехода
		::memset(relay + used + 2, 0, 4);
		// Записываем адрес возврата за место подмены
		::memcpy(relay + used + 6, &back, sizeof(back));
		// Запоминаем адрес входа
		patch.entry = entry;
		// Запоминаем переходник как прежнее тело
		patch.body = relay;
		// Запоминаем число вытесненных байт
		patch.moved = moved;
		// Сохраняем прежнее содержимое области подмены
		::memcpy(patch.saved, entry, moved);
		// Прежние права доступа к странице
		DWORD previous = 0;
		// Открываем страницу на запись
		if(!::VirtualProtect(entry, moved, PAGE_EXECUTE_READWRITE, &previous))
			// Отвечаем отказом
			return false;
		// Определяем смещение перехода на мостик
		const int64_t reach = (static_cast <int64_t> (bridge - (reinterpret_cast <uint8_t *> (entry) + 5)));
		// Если мостик оказался вне досягаемости относительного перехода
		if((reach > 0x7FFFFFFFll) || (reach < -0x80000000ll)){
			// Возвращаем прежние права доступа
			::VirtualProtect(entry, moved, previous, &previous);
			// Отвечаем отказом
			return false;
		}
		// Собираемое содержимое области подмены
		uint8_t buffer[PATCH_SIZE];
		/**
		 * Заполняем область подмены пустыми командами
		 *
		 * Вытесненных байт может оказаться больше пяти: хвост их обязан остаться годным
		 * кодом на случай, если кто-то прыгнет в середину входа
		 */
		::memset(buffer, 0x90, sizeof(buffer));
		// Записываем команду относительного перехода
		buffer[0] = 0xE9u;
		// Определяем смещение перехода
		const int32_t narrow = static_cast <int32_t> (reach);
		// Записываем смещение перехода
		::memcpy(buffer + 1, &narrow, sizeof(narrow));
		// Публикуем прежнюю функцию до включения перехода на наш отклик
		put(originals, index, patch.body);
		// Переносим собранное на вход функции
		::memcpy(entry, buffer, moved);
		// Возвращаем прежние права доступа
		::VirtualProtect(entry, moved, previous, &previous);
		// Сбрасываем кэш команд у входа
		::FlushInstructionCache(::GetCurrentProcess(), entry, moved);
		/**
		 * Сбрасываем кэш команд у мостика и переходника
		 *
		 * Оба - свежесобранный исполняемый код в выданной под них памяти, и по договору
		 * системы перед исполнением записанного кода кэш команд обязан быть сброшен.
		 * Мостик - четырнадцать октетов, переходник - вытесненное тело плюс те же
		 * четырнадцать октетов обратного перехода
		 */
		::FlushInstructionCache(::GetCurrentProcess(), bridge, 14u);
		::FlushInstructionCache(::GetCurrentProcess(), relay, (used + 14u));
		// Отмечаем подмену наложенной
		patch.applied = true;
		// Отвечаем успехом
		return true;
	/**
	 * Для прочих наборов команд
	 */
	#else
		// Отмечаем неиспользуемые параметры
		static_cast <void> (patch);
		// Отвечаем отказом
		return false;
	#endif
}
/**
 * @brief Метод снятия подмены со входа функции
 *
 * @param patch сведения о подмене
 *
 */
void awh::alloc::PECapture::revert(patch_t & patch) noexcept {
	// Если подмена не накладывалась
	if(!patch.applied)
		// Снимать нечего
		return;
	// Прежние права доступа к странице
	DWORD previous = 0;
	// Определяем длину возвращаемого содержимого
	const size_t length = ((patch.moved > 0) ? patch.moved : PATCH_SIZE);
	// Открываем страницу на запись
	if(::VirtualProtect(patch.entry, length, PAGE_EXECUTE_READWRITE, &previous)){
		// Возвращаем прежнее содержимое
		::memcpy(patch.entry, patch.saved, length);
		// Возвращаем прежние права доступа
		::VirtualProtect(patch.entry, length, previous, &previous);
		// Сбрасываем кэш команд
		::FlushInstructionCache(::GetCurrentProcess(), patch.entry, length);
	}
	// Отмечаем подмену снятой
	patch.applied = false;
}
/**
 * @brief Метод захвата выделения памяти процесса
 *
 * @param hooks     наши функции, ставимые на место прежних
 * @param originals прежние функции, отдаваемые захватом
 * @return          признак состоявшегося захвата
 *
 */
bool awh::alloc::PECapture::acquire(const functions_t & hooks, functions_t & originals) noexcept {
	// Если этот экземпляр уже владеет захватом
	if(this->_acquired)
		// Повторно накладывать подмены не требуется
		return true;
	// Если подменами владеет другой экземпляр
	if(__awh_pe_owner__ != nullptr)
		// Менять наборы действующих откликов нельзя
		return false;
	// Если отклики не умеют сохранять исходную библиотеку
	if((hooks.freeFrom == nullptr) || (hooks.reallocFrom == nullptr) || (hooks.msizeFrom == nullptr))
		// Без прежних функций каждой CRT старые блоки освобождать небезопасно
		return false;
	// Имена поддерживаемых библиотек
	static const char * const LIBRARIES[LIBRARY_COUNT] = {"ucrtbase.dll", "msvcrt.dll"};
	// Описатели уже загруженных библиотек
	::HMODULE modules[LIBRARY_COUNT] = {nullptr};
	// Входы функций каждой библиотеки
	void * entries[LIBRARY_COUNT][PATCH_TOTAL] = {{nullptr}};
	// Подмены с сохранением исходной библиотеки
	const functions_t targets[LIBRARY_COUNT] = {__awh_pe_targets__ <0> (hooks), __awh_pe_targets__ <1> (hooks)};
	// Номер первой найденной библиотеки
	size_t first = LIBRARY_COUNT;
	/**
	 * Проверяем все обязательные группы до изменения первого входа
	 */
	for(size_t library = 0; library < LIBRARY_COUNT; library++){
		// Получаем описатель уже загруженной библиотеки
		modules[library] = ::GetModuleHandleA(LIBRARIES[library]);
		// Если библиотека не загружена
		if(modules[library] == nullptr)
			// Загружать новую CRT ради захвата не требуется
			continue;
		// Если это первая найденная библиотека
		if(first == LIBRARY_COUNT)
			// Запоминаем основной набор прежних функций
			first = library;
		/**
		 * Проверяем входы обязательной группы
		 */
		for(size_t i = 0; i < PATCH_COUNT; i++){
			// Получаем адрес входа
			entries[library][i] = reinterpret_cast <void *> (::GetProcAddress(modules[library], NAMES[i]));
			// Если вход непригоден либо замена не задана
			if(!this->suits(entries[library][i]) || (pick(hooks, i) == nullptr))
				// Отказываем до изменения кода
				return false;
		}
		// Признак наличия откликов выровненной семьи
		bool aligned = ((hooks.alignedFreeFrom != nullptr) && (hooks.alignedReallocFrom != nullptr));
		/**
		 * Проверяем всё ядро выровненной семьи
		 */
		for(size_t i = ALIGNED_FIRST; aligned && (i < PATCH_TOTAL); i++){
			// Получаем адрес очередного входа
			entries[library][i] = reinterpret_cast <void *> (::GetProcAddress(modules[library], NAMES[i]));
			// Если необязательное измерение не экспортируется
			if((i == (PATCH_TOTAL - 1)) && (entries[library][i] == nullptr))
				// Подменять отсутствующую функцию не требуется
				continue;
			// Проверяем пригодность входа и наличие замены
			aligned = (this->suits(entries[library][i]) && (pick(hooks, i) != nullptr));
			// Если измерение экспортируется, оно тоже должно сохранять исходную CRT
			if(i == (PATCH_TOTAL - 1))
				// Не оставляем системное измерение над нашими блоками
				aligned = (aligned && (hooks.alignedMsizeFrom != nullptr));
		}
		// Если семья не может быть установлена целиком
		if(!aligned){
			/**
			 * Исключаем всю выровненную группу
			 */
			for(size_t i = ALIGNED_FIRST; i < PATCH_TOTAL; i++)
				// Оставляем исходный вход нетронутым
				entries[library][i] = nullptr;
		}
	}
	// Если ни одна CRT не загружена
	if(first == LIBRARY_COUNT)
		// Подменять нечего
		return false;
	/**
	 * Общий адрес двух экспортов нельзя переписывать дважды
	 *
	 * Для перенаправленных экспортов отказываем до изменения кода: источник
	 * вызова у общего входа неразличим, а второй переходник захватил бы первый.
	 */
	for(size_t i = 0; i < PATCH_TOTAL; i++){
		/**
		 * Сопоставляем входы двух библиотек
		 */
		for(size_t j = 0; j < PATCH_TOTAL; j++){
			// Если обе библиотеки ведут в один вход
			if((entries[0][i] != nullptr) && (entries[0][i] == entries[1][j]))
				// Не накладываем неоднозначную подмену
				return false;
		}
	}
	// Сохраняем отклики до включения первого перехода
	__awh_pe_hooks__ = hooks;
	// Закрепляем владение общими таблицами
	__awh_pe_owner__ = this;
	/**
	 * Устанавливаем обязательные группы обеих библиотек
	 */
	for(size_t library = 0; library < LIBRARY_COUNT; library++){
		// Если библиотека не загружена
		if(modules[library] == nullptr)
			// Подменять нечего
			continue;
		// Выбираем отдельную область переходников этой библиотеки
		this->_library = library;
		/**
		 * Устанавливаем обязательные подмены
		 */
		for(size_t i = 0; i < PATCH_COUNT; i++){
			// Если очередная подмена не установлена
			if(!this->apply(this->_patches[library][i], entries[library][i], pick(targets[library], i), __awh_pe_originals__[library], i)){
				/**
				 * Возвращаем обе библиотеки в исходное состояние
				 */
				for(size_t crt = 0; crt < LIBRARY_COUNT; crt++){
					/**
					 * Снимаем уже установленные обязательные подмены
					 */
					for(size_t j = 0; j < PATCH_COUNT; j++)
						// Восстанавливаем прежний вход
						this->revert(this->_patches[crt][j]);
				}
				// Освобождаем владение таблицами
				__awh_pe_owner__ = nullptr;
				// Сообщаем об отказе всей операции
				return false;
			}
		}
	}
	/**
	 * Устанавливаем необязательные выровненные группы отдельно для каждой CRT
	 */
	for(size_t library = 0; library < LIBRARY_COUNT; library++){
		// Выбираем область переходников этой библиотеки
		this->_library = library;
		/**
		 * Устанавливаем подготовленные входы выровненной семьи
		 */
		for(size_t i = ALIGNED_FIRST; i < PATCH_TOTAL; i++){
			// Если вход отсутствует либо вся группа исключена
			if(entries[library][i] == nullptr)
				// Подменять нечего
				continue;
			// Если очередная подмена не установлена
			if(!this->apply(this->_patches[library][i], entries[library][i], pick(targets[library], i), __awh_pe_originals__[library], i)){
				/**
				 * Снимаем всю выровненную группу этой библиотеки
				 */
				for(size_t j = ALIGNED_FIRST; j < i; j++)
					// Восстанавливаем прежний вход
					this->revert(this->_patches[library][j]);
				// К следующей библиотеке переходим без частично установленной группы
				break;
			}
		}
	}
	// Возвращаем прежние функции основной библиотеки для прямых вызовов
	originals = __awh_pe_originals__[first];
	// Отмечаем захват состоявшимся
	this->_acquired = true;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия захвата
 *
 */
void awh::alloc::PECapture::release() noexcept {
	// Если захват не состоялся
	if(!this->_acquired)
		// Снимать нечего
		return;
	/**
	 * Восстанавливаем входы обеих библиотек
	 */
	for(size_t library = 0; library < LIBRARY_COUNT; library++){
		/**
		 * Снимаем обязательные и выровненные подмены
		 */
		for(size_t i = 0; i < PATCH_TOTAL; i++)
			// Восстанавливаем прежний вход
			this->revert(this->_patches[library][i]);
	}
	// Отмечаем захват снятым
	this->_acquired = false;
	// Освобождаем владение общими таблицами
	__awh_pe_owner__ = nullptr;
}
/**
 * @brief Метод определения состоявшегося захвата
 *
 * @return признак захвата
 *
 */
bool awh::alloc::PECapture::acquired() const noexcept {
	// Выводим признак захвата
	return this->_acquired;
}
/**
 * @brief Метод опознания указателя, выданного прежним распределителем
 *
 * @param ptr разбираемый указатель
 * @return    признак принадлежности прежнему распределителю
 *
 */
bool awh::alloc::PECapture::foreign(const void * ptr) const noexcept {
	// Если указатель не задан
	if(ptr == nullptr)
		// Чужим он не является
		return false;
	/**
	 * Опознание идёт перебором куч процесса
	 *
	 * Память, выданная до захвата, лежит в кучах, заведённых библиотекой времени
	 * исполнения. Иного способа отличить её от потерянного указателя система не даёт,
	 * и тем же приёмом пользуется перенаправитель mimalloc
	 */
	// Число куч процесса
	DWORD count = ::GetProcessHeaps(0, nullptr);
	// Если куч не оказалось
	if(count == 0)
		// Опознать не удалось
		return false;
	// Набор описателей куч процесса
	HANDLE heaps[64];
	// Если куч больше, чем помещается в набор, берём сколько влезло
	if(count > (sizeof(heaps) / sizeof(heaps[0])))
		// Ограничиваем число куч размером набора
		count = static_cast <DWORD> (sizeof(heaps) / sizeof(heaps[0]));
	// Заполняем набор описателями куч
	count = ::GetProcessHeaps(count, heaps);
	/**
	 * Перебираем кучи процесса
	 */
	for(DWORD i = 0; i < count; i++){
		// Если указатель принадлежит очередной куче
		if(::HeapValidate(heaps[i], 0, ptr))
			// Указатель выдан прежним распределителем
			return true;
	}
	// Указатель прежнему распределителю не принадлежит
	return false;
}
/**
 * @brief Метод получения названия способа захвата
 *
 * @return название способа захвата
 *
 */
const char * awh::alloc::PECapture::name() const noexcept {
	// Выводим название способа захвата
	return "PE entry rewrite";
}
/**
 * @brief Конструктор сведений о подмене
 *
 */
awh::alloc::PECapture::Patch::Patch() noexcept :
 entry(nullptr), body(nullptr), saved{0}, moved(0), applied(false) {}
/**
 * @brief Конструктор захвата обеих библиотек времени исполнения
 *
 */
awh::alloc::PECapture::PECapture() noexcept :
 _library(0), _arena{nullptr}, _arenaUsed{0}, _arenaSize{0}, _acquired(false) {}
/**
 * @brief Деструктор
 *
 * @details Захват здесь НЕ снимается намеренно. Память, выданная нами, живёт дольше
 *          самого захвата: разрушители статических объектов чужих единиц трансляции
 *          освобождают её уже после нас, и снятие захвата обращает такое освобождение
 *          в осиротевшее. У macOS это видно сразу - система отвечает «pointer being
 *          freed was not allocated» и валит процесс на выходе; у прочих систем беда
 *          тише, но того же рода. Порядок разрушения статических объектов задаётся
 *          порядком связывания, оттого беда эта то видна, то нет: отдельный стенд
 *          проверок валился, а тот же набор в дереве проходил.
 *
 *          Снять захват можно лишь ЯВНО, вызовом `Allocator::surrender`, - и звавший
 *          берёт на себя обязанность не освобождать выданное нами после этого.
 *          Распределители третьей стороны поступают так же: память при выходе процесса
 *          намеренно не возвращается вовсе.
 *
 */
awh::alloc::PECapture::~PECapture() noexcept {}

#endif
