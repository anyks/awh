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
#if _WIN32 || _WIN64

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
	static const char * const NAMES[] = {"malloc", "free", "calloc", "realloc", "_msize"};
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
			 * Пропускаем расширяющую приставку
			 *
			 * Иных приставок в начале функций не встречается, и знать их разбирателю
			 * незачем: на всякой незнакомой он отвечает отказом
			 */
			if((command[0] >= 0x40u) && (command[0] <= 0x4Fu))
				// Учитываем расширяющую приставку
				length++;
			// Запоминаем код операции
			const uint8_t opcode = command[length];
			// Учитываем код операции
			length++;
			/**
			 * Разбираем код операции
			 */
			switch(opcode){
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
				 */
				case 0x31: case 0x33: case 0x85: case 0x89:
				case 0x8B: case 0x8D: {
					// Запоминаем байт вида операнда
					const uint8_t modrm = command[length];
					// Учитываем байт вида операнда
					length++;
					// Извлекаем поле вида адресации
					const uint8_t mod = static_cast <uint8_t> ((modrm >> 6) & 0x03u);
					// Извлекаем поле регистра-основания
					const uint8_t rm = static_cast <uint8_t> (modrm & 0x07u);
					/**
					 * Отказываем адресации от счётчика команд
					 *
					 * Смещение её считается от адреса СЛЕДУЮЩЕЙ команды, и перенос такой
					 * команды в переходник увёл бы обращение неведомо куда. Правка её
					 * возможна, но в начале функций она не встречается, а писать правку
					 * без единого случая для проверки значило бы писать наугад
					 */
					if((mod == 0x00u) && (rm == 0x05u))
						// Отвечаем отказом
						return 0;
					// Если адресация идёт через байт масштаба
					if((mod != 0x03u) && (rm == 0x04u))
						// Учитываем байт масштаба
						length++;
					// Если у команды однобайтовое смещение
					if(mod == 0x01u)
						// Учитываем однобайтовое смещение
						length++;
					// Если у команды четырёхбайтовое смещение
					else if(mod == 0x02u)
						// Учитываем четырёхбайтовое смещение
						length += 4;
				} break;
				/**
				 * Команды с байтом вида операнда и однобайтовым непосредственным значением
				 */
				case 0x83: {
					// Запоминаем байт вида операнда
					const uint8_t modrm = command[length];
					// Учитываем байт вида операнда
					length++;
					// Извлекаем поле вида адресации
					const uint8_t mod = static_cast <uint8_t> ((modrm >> 6) & 0x03u);
					// Извлекаем поле регистра-основания
					const uint8_t rm = static_cast <uint8_t> (modrm & 0x07u);
					// Отказываем адресации от счётчика команд
					if((mod == 0x00u) && (rm == 0x05u))
						// Отвечаем отказом
						return 0;
					// Если адресация идёт через байт масштаба
					if((mod != 0x03u) && (rm == 0x04u))
						// Учитываем байт масштаба
						length++;
					// Если у команды однобайтовое смещение
					if(mod == 0x01u)
						// Учитываем однобайтовое смещение
						length++;
					// Если у команды четырёхбайтовое смещение
					else if(mod == 0x02u)
						// Учитываем четырёхбайтовое смещение
						length += 4;
					// Учитываем однобайтовое непосредственное значение
					length++;
				} break;
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
	if((this->_arena != nullptr) && ((this->_arenaUsed + size) <= this->_arenaSize)){
		// Запоминаем адрес выдачи
		void * result = (reinterpret_cast <uint8_t *> (this->_arena) + this->_arenaUsed);
		// Сдвигаем занятое в области
		this->_arenaUsed += size;
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
			this->_arena = block;
			// Запоминаем размер взятой области
			this->_arenaSize = static_cast <size_t> (step);
			// Запоминаем занятое в области
			this->_arenaUsed = size;
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
 * @param patch  сведения о подмене
 * @param entry  адрес входа подменяемой функции
 * @param target адрес подставляемой функции
 * @return       признак успеха
 *
 */
bool awh::alloc::PECapture::apply(patch_t & patch, void * entry, const void * target) noexcept {
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
			if((command[0] >= 0x70u) && (command[0] <= 0x7Fu)){
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
		// Переносим собранное на вход функции
		::memcpy(entry, buffer, moved);
		// Возвращаем прежние права доступа
		::VirtualProtect(entry, moved, previous, &previous);
		// Сбрасываем кэш команд
		::FlushInstructionCache(::GetCurrentProcess(), entry, moved);
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
	// Если захват уже состоялся
	if(this->_acquired)
		// Повторять его незачем
		return true;
	/**
	 * Ищем библиотеку времени исполнения среди известных
	 *
	 * Их две, и какая из них загружена - решает окружение сборки: у MSYS2 CLANGARM64
	 * это `ucrtbase.dll`, а у MSYS2 MINGW64 - `msvcrt.dll`. Искали мы прежде только
	 * первую, и на x86-64 захват отвечал отказом, не дойдя до подмены вовсе
	 */
	// Названия известных библиотек времени исполнения
	static const char * const LIBRARIES[] = {"ucrtbase.dll", "msvcrt.dll"};
	// Описатель библиотеки времени исполнения
	HMODULE module = nullptr;
	/**
	 * Перебираем известные библиотеки времени исполнения
	 */
	for(size_t i = 0; (i < (sizeof(LIBRARIES) / sizeof(LIBRARIES[0]))) && (module == nullptr); i++)
		// Получаем описатель очередной библиотеки
		module = ::GetModuleHandleA(LIBRARIES[i]);
	// Если ни одна из библиотек не загружена
	if(module == nullptr)
		// Отвечаем отказом: подменять нечего
		return false;
	/**
	 * Разбираем входы всех подменяемых функций прежде, чем править хоть один
	 *
	 * Порядок этот выбран намеренно: если хоть один вход окажется без горячей заплатки,
	 * отказать надо не тронув ничего. Правка половины функций оставила бы процесс в
	 * состоянии, где часть памяти идёт к нам, а часть мимо, и распутать это нечем
	 */
	void * entries[PATCH_COUNT] = {nullptr};
	/**
	 * Перебираем подменяемые функции
	 */
	for(size_t i = 0; i < PATCH_COUNT; i++){
		// Получаем адрес входа очередной функции
		entries[i] = reinterpret_cast <void *> (::GetProcAddress(module, NAMES[i]));
		// Если функция не найдена либо вход её заплаткой не начинается
		if((entries[i] == nullptr) || !this->suits(entries[i]))
			// Отвечаем отказом, не тронув ничего
			return false;
		// Если для функции не задана наша замена
		if(pick(hooks, i) == nullptr)
			// Отвечаем отказом: подменять не на что
			return false;
	}
	/**
	 * Накладываем подмены
	 */
	for(size_t i = 0; i < PATCH_COUNT; i++){
		// Накладываем очередную подмену
		if(!this->apply(this->_patches[i], entries[i], pick(hooks, i))){
			/**
			 * Откатываем уже наложенное
			 *
			 * Оставить половину наложенной нельзя: часть выделений шла бы к нам, а
			 * освобождение их - к прежнему распределителю
			 */
			for(size_t j = 0; j < i; j++)
				// Снимаем ранее наложенную подмену
				this->revert(this->_patches[j]);
			// Отвечаем отказом
			return false;
		}
		// Отдаём прежнюю функцию вызывающей стороне
		put(originals, i, this->_patches[i].body);
	}
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
	 * Снимаем наложенные подмены
	 */
	for(size_t i = 0; i < PATCH_COUNT; i++)
		// Снимаем очередную подмену
		this->revert(this->_patches[i]);
	// Отмечаем захват снятым
	this->_acquired = false;
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
 * @brief Деструктор
 *
 */
awh::alloc::PECapture::~PECapture() noexcept {
	// Снимаем захват, если он состоялся
	this->release();
}

#endif // _WIN32 || _WIN64
