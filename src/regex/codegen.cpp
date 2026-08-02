/**
 * @file: codegen.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация преобразования программы регулярного выражения в машинный код —
 *        обход инструкций программы, порождение прохода рядов повторения
 *        и сборка обстановки исполнения порождённого кода
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/text.hpp>
#include <regex/codegen.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён вспомогательных значений кодогенерации
 *
 */
namespace {
	/**
	 * @brief Размер таблицы принадлежности значений байта в байтах
	 *
	 */
	constexpr size_t TABLE = 0x100;

	/**
	 * @brief Размер кадра вызова, приходящийся на один ряд повторения
	 *
	 * @details Ряд занимает два восьмибайтовых значения: положение отступления
	 *          и положение начала ряда, ниже какого отступление невозможно.
	 *
	 */
	constexpr size_t SLOTS = 2;

	/**
	 * @brief Количество мест кадра вызова, сохраняющих регистры на время вызова
	 *
	 * @details Соглашение о вызове подпрограмм сохранности младших регистров
	 *          не обещает, поэтому в кадре вызова сохраняется всё, чем сопоставление
	 *          живёт: адрес текста, размер текста, адрес набора границ, адрес
	 *          обстановки исполнения, позиция начала попытки и адрес возврата.
	 *          Позиция сопоставления сохранения не требует - она устанавливается
	 *          позицией начала попытки заново.
	 *
	 */
	constexpr size_t SPILLS = 6;

	/**
	 * @brief Наибольшее количество символов, заменяемых продвижением позиции
	 *
	 * @details Продвижение позиции размещается одной командой сложения с числом,
	 *          а число это несёт двенадцать разрядов.
	 *
	 */
	constexpr size_t MAX_ELISION = 0xFFF;

	/**
	 * @brief Номер места обстановки, несущего адрес предварительного отбора позиций
	 *
	 */
	constexpr size_t SLOT_PREFILTER = 0;

	/**
	 * @brief Номер места обстановки, несущего адрес подпрограммы отбора позиций
	 *
	 */
	constexpr size_t SLOT_SEEKING = 1;

	/**
	 * @brief Номер места обстановки, несущего адрес подпрограммы проверки возможности
	 *
	 */
	constexpr size_t SLOT_FEASIBLE = 2;

	/**
	 * @brief Количество мест обстановки, отведённых прежде таблиц принадлежности
	 *
	 */
	constexpr size_t SLOT_TABLES = 3;

	/**
	 * @brief Функция поиска ближайшей позиции возможного начала совпадения
	 *
	 * @details Функция вызывается порождённым кодом по адресу, прочитанному
	 *          из обстановки исполнения, и потому несёт соглашение о вызове,
	 *          порождённым кодом соблюдаемое.
	 *
	 * @param text      адрес начала текста сопоставления
	 * @param size      размер текста сопоставления в байтах
	 * @param pos       позиция начала поиска
	 * @param prefilter адрес предварительного отбора позиций
	 * @return          позиция возможного начала совпадения
	 *
	 */
	size_t seeking(const char * text, const size_t size, const size_t pos, const void * prefilter) noexcept {
		// Выводим позицию возможного начала совпадения
		return reinterpret_cast <const awh::regex::prefilter_t *> (prefilter)->search(std::string_view(text, size), pos);
	}
	/**
	 * @brief Функция проверки возможности совпадения в оставшемся тексте
	 *
	 * @param text      адрес начала текста сопоставления
	 * @param size      размер текста сопоставления в байтах
	 * @param pos       позиция начала проверяемого участка текста
	 * @param prefilter адрес предварительного отбора позиций
	 * @return          результат проверки возможности совпадения
	 *
	 */
	size_t feasible(const char * text, const size_t size, const size_t pos, const void * prefilter) noexcept {
		// Выводим результат проверки возможности совпадения
		return (reinterpret_cast <const awh::regex::prefilter_t *> (prefilter)->possible(std::string_view(text, size), pos) ? 1 : 0);
	}
	/**
	 * @brief Функция порождения вызова подпрограммы обстановки исполнения
	 *
	 * @details Порождается сохранение затираемых вызовом регистров в кадре,
	 *          передача доводов, вызов и восстановление сохранённых регистров.
	 *          Итог вызова остаётся в регистре промежуточного значения,
	 *          восстановлением не затрагиваемом.
	 *
	 * @param emitter объект порождения машинного кода
	 * @param slot    номер места обстановки, несущего адрес подпрограммы
	 * @param spill   номер первого места кадра, сохраняющего регистры
	 * @param pos     регистр позиции, передаваемой подпрограмме доводом
	 *
	 */
	void invoke(awh::regex::Emitter & emitter, const size_t slot, const size_t spill, const awh::regex::Emitter::reg_t pos) noexcept {
		// Подписываемся на перечисление регистров соглашения о вызове
		using reg_t = awh::regex::Emitter::reg_t;
		/**
		 * Выполняем сохранение затираемых вызовом регистров в кадре вызова
		 */
		emitter.store(reg_t::TEXT, reg_t::STACK, static_cast <uint32_t> (spill + 0));
		emitter.store(reg_t::SIZE, reg_t::STACK, static_cast <uint32_t> (spill + 1));
		emitter.store(reg_t::BOUNDS, reg_t::STACK, static_cast <uint32_t> (spill + 2));
		emitter.store(reg_t::CONTEXT, reg_t::STACK, static_cast <uint32_t> (spill + 3));
		emitter.store(reg_t::LINK, reg_t::STACK, static_cast <uint32_t> (spill + 4));
		emitter.store(reg_t::KEEPER, reg_t::STACK, static_cast <uint32_t> (spill + 5));
		// Выполняем чтение адреса вызываемой подпрограммы из обстановки исполнения
		emitter.context(reg_t::SCRATCH, static_cast <uint32_t> (slot));
		// Выполняем передачу адреса предварительного отбора четвёртым доводом
		emitter.context(reg_t::BOUNDS, static_cast <uint32_t> (SLOT_PREFILTER));
		// Выполняем передачу позиции третьим доводом вызова
		emitter.move(reg_t::START, pos);
		// Выполняем размещение вызова подпрограммы обстановки исполнения
		emitter.call(reg_t::SCRATCH);
		// Выполняем перенос итога вызова в регистр промежуточного значения
		emitter.move(reg_t::SCRATCH, reg_t::RESULT);
		/**
		 * Выполняем восстановление затёртых вызовом регистров из кадра вызова
		 */
		emitter.fetch(reg_t::TEXT, reg_t::STACK, static_cast <uint32_t> (spill + 0));
		emitter.fetch(reg_t::SIZE, reg_t::STACK, static_cast <uint32_t> (spill + 1));
		emitter.fetch(reg_t::BOUNDS, reg_t::STACK, static_cast <uint32_t> (spill + 2));
		emitter.fetch(reg_t::CONTEXT, reg_t::STACK, static_cast <uint32_t> (spill + 3));
		emitter.fetch(reg_t::LINK, reg_t::STACK, static_cast <uint32_t> (spill + 4));
		emitter.fetch(reg_t::KEEPER, reg_t::STACK, static_cast <uint32_t> (spill + 5));
	}

	/**
	 * @brief Функция проверки принадлежности инструкции сопоставляющим одиночный символ
	 *
	 * @param type код операции проверяемой инструкции
	 * @return     результат проверки принадлежности инструкции
	 *
	 */
	inline bool single(const awh::regex::opcode_t type) noexcept {
		// Выводим результат проверки принадлежности инструкции
		return ((type == awh::regex::opcode_t::CHAR) || (type == awh::regex::opcode_t::CLASS) || (type == awh::regex::opcode_t::ANY));
	}
	/**
	 * @brief Функция обхода программы по пути, получающему кодогенерацию
	 *
	 * @details Обход повторяет порядок порождения кода, но кода не порождает:
	 *          им проверяется применимость кодогенерации к программе и им же
	 *          подсчитывается количество рядов повторения. Разделение проверки
	 *          и порождения намеренное - иначе отказ порождения оставлял бы
	 *          после себя наполовину порождённый код.
	 *
	 * @param program проверяемая программа регулярного выражения
	 * @param runs    количество рядов повторения одиночного символа
	 * @return        результат проверки применимости кодогенерации
	 *
	 */
	bool walk(const awh::regex::program_t & program, size_t & runs) noexcept {
		// Выполняем сброс количества рядов повторения
		runs = 0;
		/**
		 * Если сборка выполняется в режиме разбора UTF-8
		 *
		 * @details Порождаемый код сопоставляет байты, тогда как в режиме разбора
		 *          UTF-8 программа сопоставляет символы целиком.
		 *
		 */
		if(awh::regex::hasFlag(program.flags, awh::regex::flag_t::UTF))
			// Выводим неприменимость кодогенерации к программе
			return false;
		/**
		 * Если выражение выполняет захват групп
		 *
		 * @details Порождаемый код устанавливает границы совпадения целиком,
		 *          а границ захватывающих групп не устанавливает.
		 *
		 */
		if(program.captures > 0)
			// Выводим неприменимость кодогенерации к программе
			return false;
		/**
		 * Если набор инструкций программы пуст
		 */
		if(program.instructions.empty())
			// Выводим неприменимость кодогенерации к программе
			return false;
		// Создаём набор посещённых адресов инструкций программы
		vector <bool> visited(program.instructions.size(), false);
		// Получаем адрес исполняемой инструкции программы
		awh::regex::address_t pc = 0;
		/**
		 * Выполняем обход инструкций программы регулярного выражения
		 */
		while(true) {
			/**
			 * Если адрес инструкции находится за пределами программы
			 */
			if(static_cast <size_t> (pc) >= program.instructions.size())
				// Выводим неприменимость кодогенерации к программе
				return false;
			/**
			 * Если инструкция по адресу уже посещалась
			 *
			 * @details Повторное посещение означает переход назад, то есть путь,
			 *          проходом рядов не описываемый.
			 *
			 */
			if(visited.at(static_cast <size_t> (pc)))
				// Выводим неприменимость кодогенерации к программе
				return false;
			// Выполняем пометку посещения инструкции по адресу
			visited.at(static_cast <size_t> (pc)) = true;
			// Получаем исполняемую инструкцию программы
			const awh::regex::instruction_t & instruction = program.instructions.at(static_cast <size_t> (pc));
			/**
			 * Если инструкция сопоставляет одиночный символ
			 */
			if(single(instruction.type)) {
				// Переходим к следующей инструкции программы
				pc++;
				// Продолжаем обход инструкций программы
				continue;
			}
			/**
			 * Определяем код операции исполняемой инструкции
			 */
			switch(static_cast <uint8_t> (instruction.type)) {
				/**
				 * Если инструкция сохраняет позицию в ячейке захвата
				 *
				 * @details Границы совпадения порождаемый код ведёт сам, поэтому
				 *          сохранение их пропускается. Прочие ячейки означают
				 *          захват групп и отвергаются проверкой выше.
				 *
				 */
				case static_cast <uint8_t> (awh::regex::opcode_t::SAVE): {
					/**
					 * Если сохранение выполняется не в ячейку границ совпадения
					 */
					if(instruction.save.slot > 1)
						// Выводим неприменимость кодогенерации к программе
						return false;
					// Переходим к следующей инструкции программы
					pc++;
				} break;
				/**
				 * Если инструкция выполняет переход по двум ветвям
				 */
				case static_cast <uint8_t> (awh::regex::opcode_t::SPLIT): {
					// Получаем адрес тела повторения одиночного символа
					const awh::regex::address_t body = program.runs.at(static_cast <size_t> (pc));
					/**
					 * Если переход повторение одиночного символа не возглавляет
					 */
					if(body == awh::regex::INVALID_ADDRESS)
						// Выводим неприменимость кодогенерации к программе
						return false;
					/**
					 * Если тело повторения одиночного символа не сопоставляет
					 */
					if((static_cast <size_t> (body) >= program.instructions.size()) || !single(program.instructions.at(static_cast <size_t> (body)).type))
						// Выводим неприменимость кодогенерации к программе
						return false;
					/**
					 * Если допустимое количество рядов повторения исчерпано
					 */
					if(++runs > awh::regex::MAX_RUNS)
						// Выводим неприменимость кодогенерации к программе
						return false;
					// Переходим к ветви завершения повторения
					pc = instruction.split.second;
				} break;
				/**
				 * Если инструкция завершает сопоставление с успехом
				 */
				case static_cast <uint8_t> (awh::regex::opcode_t::MATCH):
					// Выводим применимость кодогенерации к программе
					return true;
				/**
				 * Если инструкция кодогенерации не получает
				 */
				default:
					// Выводим неприменимость кодогенерации к программе
					return false;
			}
		}
	}
};

/**
 * @brief Метод заведения таблицы принадлежности байтов сопоставления
 *
 * @param instruction сопоставляющая инструкция программы
 * @param program     программа регулярного выражения
 * @return            номер заведённой таблицы в обстановке исполнения
 *
 */
size_t awh::regex::Codegen::table(const instruction_t & instruction, const program_t & program) noexcept {
	// Получаем номер заводимой таблицы принадлежности байтов
	const size_t result = this->_context.size();
	// Выполняем заведение места под адрес таблицы в обстановке исполнения
	this->_context.push_back(nullptr);
	// Выполняем размещение таблицы принадлежности значений байта
	this->_members.resize(this->_members.size() + TABLE, 0);
	// Получаем адрес размещённой таблицы принадлежности байтов
	uint8_t * members = (this->_members.data() + ((result - SLOT_TABLES) * TABLE));
	/**
	 * Выполняем обход пространства значений байта
	 */
	for(uint32_t letter = 0; letter < TABLE; letter++) {
		// Флаг принадлежности значения байта сопоставляемым символам
		bool belongs = false;
		/**
		 * Определяем код операции сопоставляющей инструкции
		 */
		switch(static_cast <uint8_t> (instruction.type)) {
			/**
			 * Если инструкция сопоставляет одиночный символ
			 */
			case static_cast <uint8_t> (opcode_t::CHAR): {
				/**
				 * Если установлен режим сопоставления без учёта регистра
				 */
				if(hasFlag(instruction.flags, flag_t::CASELESS))
					// Выполняем сопоставление символов без учёта регистра
					belongs = (fold(letter, instruction.flags) == fold(instruction.letter.code, instruction.flags));
				// Выполняем сопоставление символов с учётом регистра
				else belongs = (letter == instruction.letter.code);
			} break;
			/**
			 * Если инструкция сопоставляет символ из класса символов
			 */
			case static_cast <uint8_t> (opcode_t::CLASS):
				// Выполняем проверку принадлежности символа классу символов
				belongs = regex::belongs(program.classes.at(instruction.charclass.index), letter, instruction.flags);
			break;
			/**
			 * Если инструкция сопоставляет любой символ
			 */
			case static_cast <uint8_t> (opcode_t::ANY):
				// Выполняем проверку соответствия символа переводу строки
				belongs = (hasFlag(instruction.flags, flag_t::DOTALL) || (letter != 0x0A));
			break;
		}
		// Выполняем установку принадлежности значения байта
		members[letter] = (belongs ? 1 : 0);
	}
	// Выводим номер заведённой таблицы принадлежности байтов
	return result;
}
/**
 * @brief Метод проверки применимости кодогенерации к программе
 *
 * @param program проверяемая программа регулярного выражения
 * @return        результат проверки применимости кодогенерации
 *
 */
bool awh::regex::Codegen::applicable(const program_t & program) noexcept {
	/**
	 * Если кодогенерация сборкой не поддерживается
	 */
	if(!Emitter::available() || !Assembly::available())
		// Выводим неприменимость кодогенерации к программе
		return false;
	// Количество рядов повторения одиночного символа
	size_t runs = 0;
	// Выводим результат проверки применимости кодогенерации
	return walk(program, runs);
}
/**
 * @brief Метод порождения сопоставителя программы
 *
 * @param program программа регулярного выражения
 * @return        результат порождения сопоставителя
 *
 */
bool awh::regex::Codegen::compile(const program_t & program) noexcept {
	// Выполняем очистку порождённого прежде сопоставителя
	this->clear();
	// Количество рядов повторения одиночного символа
	size_t runs = 0;
	/**
	 * Если кодогенерация сборкой не поддерживается
	 */
	if(!Emitter::available() || !Assembly::available())
		// Выводим результат порождения сопоставителя
		return false;
	/**
	 * Если кодогенерация к программе неприменима
	 */
	if(!walk(program, runs))
		// Выводим результат порождения сопоставителя
		return false;
	// Подписываемся на перечисление регистров соглашения о вызове
	using reg_t = Emitter::reg_t;
	// Подписываемся на перечисление условий выполнения перехода
	using cond_t = Emitter::cond_t;
	// Создаём объект порождения машинного кода
	Emitter emitter;
	// Выполняем заведение мест обстановки, отведённых прежде таблиц
	this->_context.assign(SLOT_TABLES, nullptr);
	// Выполняем перенос предварительного отбора позиций из программы
	this->_prefilter = program.prefilter;
	/**
	 * Получаем признак порождения отбора позиций начала попытки
	 *
	 * @details Отбор порождается лишь там, где он пропускает участки текста
	 *          целиком, - при ведущем литерале совпадения либо единственном
	 *          допустимом начальном байте. Перебор набора допустимых байтов
	 *          проходит текст побайтно, как и сама попытка сопоставления,
	 *          отчего вызов подпрограммы взамен попытки был бы чистым расходом.
	 *
	 */
	const bool seek = (this->_prefilter.active && (this->_prefilter.unique || (this->_prefilter.leading.size() > 1)));
	// Получаем признак порождения проверки возможности совпадения
	const bool possible = !this->_prefilter.literal.empty();
	// Получаем номер первого места кадра, сохраняющего регистры на время вызова
	const size_t spill = (runs * SLOTS);
	/**
	 * Получаем размер кадра вызова порождаемого сопоставителя
	 *
	 * @details Кадр отводится под положения отступления рядов повторения
	 *          и места сохранения регистров, вызовом затираемых, а выравнивается
	 *          по шестнадцати байтам, как того требует соглашение о вызове ARM64.
	 *
	 */
	const uint32_t frame = static_cast <uint32_t> (((((spill + ((seek || possible) ? SPILLS : 0)) * sizeof(size_t)) + 15) / 16) * 16);
	// Заводим метку начала очередной попытки сопоставления
	const size_t attempt = emitter.label();
	// Заводим метку перехода к следующей позиции начала попытки
	const size_t following = emitter.label();
	// Заводим метку обнаружения совпадения в тексте
	const size_t found = emitter.label();
	// Заводим метку отсутствия совпадения в тексте
	const size_t none = emitter.label();
	// Заводим метку отбора позиции начала очередной попытки сопоставления
	const size_t seeker = emitter.label();
	// Создаём набор меток отступления рядов повторения
	vector <size_t> retries;
	/**
	 * Выполняем заведение меток отступления рядов повторения
	 */
	for(size_t i = 0; i < runs; i++)
		// Выполняем заведение метки отступления очередного ряда
		retries.push_back(emitter.label());
	/**
	 * Если кадр вызова порождаемому сопоставителю требуется
	 */
	if(frame > 0)
		// Выполняем отведение кадра вызова порождаемого сопоставителя
		emitter.sub(reg_t::STACK, reg_t::STACK, frame);
	// Выполняем установку позиции начала попытки сопоставления
	emitter.move(reg_t::KEEPER, reg_t::START);
	/**
	 * Если проверка возможности совпадения в тексте порождается
	 *
	 * @details Проверка выполняется единожды: обязательный литерал совпадения
	 *          отсутствует в оставшемся тексте целиком, а не в отдельной
	 *          позиции начала попытки.
	 *
	 */
	if(possible) {
		// Выполняем вызов подпрограммы проверки возможности совпадения
		invoke(emitter, SLOT_FEASIBLE, spill, reg_t::KEEPER);
		// Выполняем сравнение итога проверки возможности совпадения с нулём
		emitter.compare(reg_t::SCRATCH, static_cast <uint32_t> (0));
		// Выполняем переход к отсутствию совпадения при невозможности его
		emitter.branch(cond_t::EQUAL, none);
	}
	/**
	 * Если отбор позиции начала попытки сопоставления порождается
	 */
	if(seek) {
		// Выполняем расстановку метки отбора позиции начала попытки
		emitter.place(seeker);
		// Выполняем вызов подпрограммы отбора позиции начала попытки
		invoke(emitter, SLOT_SEEKING, spill, reg_t::KEEPER);
		// Выполняем установку отобранной позиции начала попытки
		emitter.move(reg_t::KEEPER, reg_t::SCRATCH);
		// Выполняем сравнение позиции начала попытки с размером текста
		emitter.compare(reg_t::KEEPER, reg_t::SIZE);
		/**
		 * Выполняем переход к отсутствию совпадения по исчерпании текста
		 *
		 * @details Отбор выдаёт размер текста, позиции возможного начала совпадения
		 *          не обнаружив, а совпадение в позиции конца текста при действующем
		 *          отборе невозможно: отбор ведётся по байту, совпадением
		 *          сопоставляемому, отчего пустое совпадение его не имеет.
		 *
		 */
		emitter.branch(cond_t::ABOVE, none);
	}
	// Выполняем расстановку метки начала очередной попытки сопоставления
	emitter.place(attempt);
	// Выполняем установку позиции сопоставления в позицию начала попытки
	emitter.move(reg_t::CURSOR, reg_t::KEEPER);
	// Номер порождаемого ряда повторения одиночного символа
	size_t index = 0;
	/**
	 * Получаем количество символов, сопоставление каких отбором уже выполнено
	 *
	 * @details Отбор позиции начала попытки ищет в тексте ведущий литерал
	 *          совпадения, отчего сопоставление символов этого литерала
	 *          порождённым кодом повторяло бы уже выполненное. Сопоставление
	 *          их заменяется продвижением позиции, а совпадение символов
	 *          литерала с инструкциями программы проверяется порождением:
	 *          несовпадение прекращает замену, а не порождает неверный код.
	 *
	 */
	size_t verified = (seek ? this->_prefilter.leading.size() : 0);
	// Количество символов, сопоставление каких заменено продвижением позиции
	size_t elided = 0;
	/**
	 * Получаем метку отказа сопоставления, действующую в начале программы
	 *
	 * @details Отказ до первого ряда повторения отступать некуда, поэтому
	 *          он переходит к следующей позиции начала попытки.
	 *
	 */
	size_t failure = following;
	// Получаем адрес исполняемой инструкции программы
	address_t pc = 0;
	/**
	 * Выполняем обход инструкций программы регулярного выражения
	 */
	while(true) {
		// Получаем исполняемую инструкцию программы
		const instruction_t & instruction = program.instructions.at(static_cast <size_t> (pc));
		/**
		 * Если инструкция сохраняет позицию в ячейке захвата
		 *
		 * @details Сохранение кода не порождает и замену сопоставления
		 *          продвижением позиции не прерывает.
		 *
		 */
		if(instruction.type == opcode_t::SAVE) {
			// Переходим к следующей инструкции программы
			pc++;
			// Продолжаем обход инструкций программы
			continue;
		}
		/**
		 * Если сопоставление символа выполнено отбором позиции начала попытки
		 */
		if((verified > 0) && (elided < MAX_ELISION) && (instruction.type == opcode_t::CHAR) &&
		 !hasFlag(instruction.flags, flag_t::CASELESS) &&
		 (instruction.letter.code == static_cast <uint32_t> (static_cast <uint8_t> (this->_prefilter.leading.at(elided))))) {
			// Увеличиваем количество символов, заменённых продвижением позиции
			elided++;
			// Уменьшаем количество символов, сопоставление каких выполнено отбором
			verified--;
			// Переходим к следующей инструкции программы
			pc++;
			// Продолжаем обход инструкций программы
			continue;
		}
		// Выполняем прекращение замены сопоставления продвижением позиции
		verified = 0;
		/**
		 * Если сопоставление символов заменено продвижением позиции
		 */
		if(elided > 0) {
			// Выполняем продвижение позиции сопоставления на заменённые символы
			emitter.add(reg_t::CURSOR, reg_t::CURSOR, static_cast <uint32_t> (elided));
			// Выполняем сброс количества заменённых символов
			elided = 0;
		}
		/**
		 * Если инструкция сопоставляет одиночный символ
		 */
		if(single(instruction.type)) {
			// Выполняем заведение таблицы принадлежности байтов сопоставления
			const size_t number = this->table(instruction, program);
			// Выполняем сравнение позиции сопоставления с размером текста
			emitter.compare(reg_t::CURSOR, reg_t::SIZE);
			// Выполняем переход к отказу при достижении конца текста
			emitter.branch(cond_t::ABOVE, failure);
			// Выполняем чтение байта текста в позиции сопоставления
			emitter.load(reg_t::LETTER, reg_t::TEXT, reg_t::CURSOR);
			// Выполняем чтение адреса таблицы принадлежности байтов
			emitter.context(reg_t::SCRATCH, static_cast <uint32_t> (number));
			// Выполняем чтение принадлежности байта таблице сопоставления
			emitter.load(reg_t::SPARE, reg_t::SCRATCH, reg_t::LETTER);
			// Выполняем сравнение принадлежности байта с нулём
			emitter.compare(reg_t::SPARE, static_cast <uint32_t> (0));
			// Выполняем переход к отказу при непринадлежности байта таблице
			emitter.branch(cond_t::EQUAL, failure);
			// Переходим к следующей позиции текста сопоставления
			emitter.add(reg_t::CURSOR, reg_t::CURSOR, 1);
			// Переходим к следующей инструкции программы
			pc++;
			// Продолжаем обход инструкций программы
			continue;
		}
		/**
		 * Если инструкция завершает сопоставление с успехом
		 */
		if(instruction.type == opcode_t::MATCH) {
			// Выполняем переход к обнаружению совпадения в тексте
			emitter.jump(found);
			// Выходим из обхода инструкций программы
			break;
		}
		/**
		 * Порождаем проход ряда повторения одиночного символа
		 *
		 * @details Ряд проходится целиком, после чего положение его завершения
		 *          и положение начала сохраняются в кадре вызова: отступление
		 *          выполняется по ним, не требуя набора точек возврата.
		 *
		 */
		{
			// Получаем инструкцию тела повторения одиночного символа
			const instruction_t & repeated = program.instructions.at(static_cast <size_t> (program.runs.at(static_cast <size_t> (pc))));
			// Выполняем заведение таблицы принадлежности байтов тела повторения
			const size_t number = this->table(repeated, program);
			// Заводим метку прохода ряда подходящих символов
			const size_t scan = emitter.label();
			// Заводим метку завершения прохода ряда подходящих символов
			const size_t complete = emitter.label();
			// Заводим метку продолжения сопоставления вслед за рядом
			const size_t resume = emitter.label();
			// Выполняем сохранение положения начала ряда в кадре вызова
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> ((index * SLOTS) + 1));
			// Выполняем расстановку метки прохода ряда подходящих символов
			emitter.place(scan);
			// Выполняем сравнение позиции сопоставления с размером текста
			emitter.compare(reg_t::CURSOR, reg_t::SIZE);
			// Выполняем переход к завершению прохода при достижении конца текста
			emitter.branch(cond_t::ABOVE, complete);
			// Выполняем чтение байта текста в позиции сопоставления
			emitter.load(reg_t::LETTER, reg_t::TEXT, reg_t::CURSOR);
			// Выполняем чтение адреса таблицы принадлежности байтов
			emitter.context(reg_t::SCRATCH, static_cast <uint32_t> (number));
			// Выполняем чтение принадлежности байта таблице сопоставления
			emitter.load(reg_t::SPARE, reg_t::SCRATCH, reg_t::LETTER);
			// Выполняем сравнение принадлежности байта с нулём
			emitter.compare(reg_t::SPARE, static_cast <uint32_t> (0));
			// Выполняем переход к завершению прохода при непринадлежности байта
			emitter.branch(cond_t::EQUAL, complete);
			// Переходим к следующей позиции текста сопоставления
			emitter.add(reg_t::CURSOR, reg_t::CURSOR, 1);
			// Выполняем переход к продолжению прохода ряда
			emitter.jump(scan);
			// Выполняем расстановку метки завершения прохода ряда
			emitter.place(complete);
			// Выполняем сохранение положения отступления ряда в кадре вызова
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			// Выполняем переход к продолжению сопоставления вслед за рядом
			emitter.jump(resume);
			/**
			 * Выполняем расстановку метки отступления ряда повторения
			 *
			 * @details Отступление уменьшает положение ряда на единицу и повторяет
			 *          сопоставление вслед за рядом. Исчерпание ряда передаёт отказ
			 *          ряду, размещённому прежде, а при его отсутствии - следующей
			 *          позиции начала попытки.
			 *
			 */
			emitter.place(retries.at(index));
			// Выполняем чтение положения отступления ряда из кадра вызова
			emitter.fetch(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			// Выполняем чтение положения начала ряда из кадра вызова
			emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> ((index * SLOTS) + 1));
			// Выполняем сравнение положения отступления с положением начала ряда
			emitter.compare(reg_t::CURSOR, reg_t::SCRATCH);
			// Выполняем переход к отказу при исчерпании ряда повторения
			emitter.branch(cond_t::LESS, failure);
			// Выполняем отступление на одну позицию текста
			emitter.sub(reg_t::CURSOR, reg_t::CURSOR, 1);
			// Выполняем сохранение положения отступления ряда в кадре вызова
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			// Выполняем расстановку метки продолжения сопоставления вслед за рядом
			emitter.place(resume);
			// Выполняем установку метки отказа сопоставления вслед за рядом
			failure = retries.at(index);
			// Переходим к следующему ряду повторения одиночного символа
			index++;
			// Переходим к ветви завершения повторения
			pc = instruction.split.second;
		}
	}
	// Выполняем расстановку метки перехода к следующей позиции начала попытки
	emitter.place(following);
	// Выполняем сравнение позиции начала попытки с размером текста
	emitter.compare(reg_t::KEEPER, reg_t::SIZE);
	// Выполняем переход к отсутствию совпадения при достижении конца текста
	emitter.branch(cond_t::ABOVE, none);
	// Переходим к следующей позиции начала попытки сопоставления
	emitter.add(reg_t::KEEPER, reg_t::KEEPER, 1);
	// Выполняем переход к отбору позиции начала очередной попытки сопоставления
	emitter.jump(seek ? seeker : attempt);
	// Выполняем расстановку метки обнаружения совпадения в тексте
	emitter.place(found);
	// Выполняем запись начальной границы обнаруженного совпадения
	emitter.store(reg_t::KEEPER, reg_t::BOUNDS, 0);
	// Выполняем запись конечной границы обнаруженного совпадения
	emitter.store(reg_t::CURSOR, reg_t::BOUNDS, 1);
	/**
	 * Если кадр вызова порождаемому сопоставителю отводился
	 */
	if(frame > 0)
		// Выполняем освобождение кадра вызова порождаемого сопоставителя
		emitter.add(reg_t::STACK, reg_t::STACK, frame);
	// Выполняем установку результата обнаружения совпадения
	emitter.move(reg_t::RESULT, static_cast <uint64_t> (1));
	// Выполняем размещение завершения вызова
	emitter.ret();
	// Выполняем расстановку метки отсутствия совпадения в тексте
	emitter.place(none);
	/**
	 * Если кадр вызова порождаемому сопоставителю отводился
	 */
	if(frame > 0)
		// Выполняем освобождение кадра вызова порождаемого сопоставителя
		emitter.add(reg_t::STACK, reg_t::STACK, frame);
	// Выполняем установку результата отсутствия совпадения
	emitter.move(reg_t::RESULT, static_cast <uint64_t> (0));
	// Выполняем размещение завершения вызова
	emitter.ret();
	/**
	 * Если разрешение отложенных переходов не выполнено
	 */
	if(!emitter.resolve()) {
		// Выполняем очистку порождённого сопоставителя
		this->clear();
		// Выводим результат порождения сопоставителя
		return false;
	}
	/**
	 * Выполняем сборку набора адресов обстановки исполнения
	 *
	 * @details Адреса собираются по завершении порождения: размещение таблиц
	 *          перемещает их в памяти, отчего адрес, взятый прежде, обесценивается.
	 *
	 */
	for(size_t i = SLOT_TABLES; i < this->_context.size(); i++)
		// Выполняем установку адреса таблицы принадлежности байтов
		this->_context.at(i) = (this->_members.data() + ((i - SLOT_TABLES) * TABLE));
	// Выполняем установку адреса предварительного отбора позиций
	this->_context.at(SLOT_PREFILTER) = &this->_prefilter;
	// Выполняем установку адреса подпрограммы отбора позиций
	this->_context.at(SLOT_SEEKING) = reinterpret_cast <const void *> (&seeking);
	// Выполняем установку адреса подпрограммы проверки возможности совпадения
	this->_context.at(SLOT_FEASIBLE) = reinterpret_cast <const void *> (&feasible);
	/**
	 * Если размещение порождённого машинного кода не выполнено
	 */
	if(!this->_assembly.allocate(emitter.length()) || !this->_assembly.fill(emitter.code().data(), emitter.length()) || !this->_assembly.commit()) {
		// Выполняем очистку порождённого сопоставителя
		this->clear();
		// Выводим результат порождения сопоставителя
		return false;
	}
	// Выполняем установку вызова порождённого сопоставителя
	this->_matcher = reinterpret_cast <matcher_t> (const_cast <void *> (this->_assembly.entry()));
	// Выполняем установку опознания программы порождённого сопоставителя
	this->_identity = program.id;
	// Выводим результат порождения сопоставителя
	return true;
}
/**
 * @brief Метод очистки порождённого сопоставителя
 *
 */
void awh::regex::Codegen::clear() noexcept {
	// Выполняем освобождение исполняемой памяти порождённого сопоставителя
	this->_assembly.release();
	// Выполняем очистку таблиц принадлежности значений байта
	this->_members.clear();
	// Выполняем очистку предварительного отбора позиций
	this->_prefilter.clear();
	// Выполняем очистку набора адресов обстановки исполнения
	this->_context.clear();
	// Выполняем сброс опознания программы порождённого сопоставителя
	this->_identity = 0;
	// Выполняем сброс вызова порождённого сопоставителя
	this->_matcher = nullptr;
}
/**
 * @brief Метод сопоставления регулярного выражения порождённым кодом
 *
 * @param text     текст для сопоставления
 * @param start    позиция начала поиска совпадения
 * @param captures набор границ обнаруженного совпадения
 * @return         результат поиска совпадения
 *
 */
bool awh::regex::Codegen::exec(string_view text, const size_t start, vector <pair <size_t, size_t>> & captures) const noexcept {
	// Выполняем очистку набора границ обнаруженного совпадения
	captures.clear();
	/**
	 * Если порождённый сопоставитель не готов
	 */
	if(this->_matcher == nullptr)
		// Выводим результат поиска совпадения
		return false;
	// Создаём набор границ обнаруженного совпадения
	size_t bounds[2] = {0, 0};
	// Получаем позицию начала поиска совпадения
	const size_t position = ((start > text.size()) ? text.size() : start);
	/**
	 * Если совпадение в тексте не обнаружено
	 */
	if(!this->_matcher(text.data(), text.size(), position, bounds, this->_context.data()))
		// Выводим результат поиска совпадения
		return false;
	// Выполняем размещение границ обнаруженного совпадения
	captures.emplace_back(bounds[0], bounds[1]);
	// Выводим результат поиска совпадения
	return true;
}
/**
 * @brief Метод проверки готовности порождённого сопоставителя
 *
 * @return результат проверки готовности порождённого сопоставителя
 *
 */
bool awh::regex::Codegen::ready() const noexcept {
	// Выводим результат проверки готовности порождённого сопоставителя
	return (this->_matcher != nullptr);
}
/**
 * @brief Метод извлечения опознания программы порождённого сопоставителя
 *
 * @return опознание программы порождённого сопоставителя
 *
 */
uint64_t awh::regex::Codegen::identity() const noexcept {
	// Выводим опознание программы порождённого сопоставителя
	return this->_identity;
}
/**
 * @brief Метод извлечения размера порождённого машинного кода
 *
 * @return размер порождённого машинного кода в байтах
 *
 */
size_t awh::regex::Codegen::length() const noexcept {
	// Выводим размер порождённого машинного кода
	return this->_assembly.length();
}
/**
 * @brief Конструктор
 *
 */
awh::regex::Codegen::Codegen() noexcept : _identity(0), _matcher(nullptr) {}
