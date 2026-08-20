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
	// Разбираем цель перехода горячей заплатки
	void * body = this->body(entry);
	// Если вход заплаткой не начинается
	if(body == nullptr)
		// Отвечаем отказом: переписывать вслепую нельзя
		return false;
	/**
	 * Для набора команд ARM64
	 */
	#if defined(_M_ARM64) || defined(__aarch64__)
		// Запоминаем адрес входа
		patch.entry = entry;
		// Запоминаем адрес настоящего тела
		patch.body = body;
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
	// Открываем страницу на запись
	if(::VirtualProtect(patch.entry, PATCH_SIZE, PAGE_EXECUTE_READWRITE, &previous)){
		// Возвращаем прежнее содержимое
		::memcpy(patch.entry, patch.saved, PATCH_SIZE);
		// Возвращаем прежние права доступа
		::VirtualProtect(patch.entry, PATCH_SIZE, previous, &previous);
		// Сбрасываем кэш команд
		::FlushInstructionCache(::GetCurrentProcess(), patch.entry, PATCH_SIZE);
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
	// Получаем описатель библиотеки времени исполнения
	HMODULE module = ::GetModuleHandleA("ucrtbase.dll");
	// Если библиотека не загружена
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
		if((entries[i] == nullptr) || (this->body(entries[i]) == nullptr))
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
