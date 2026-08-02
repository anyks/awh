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
	uint8_t * members = (this->_members.data() + (result * TABLE));
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
	/**
	 * Получаем размер кадра вызова порождаемого сопоставителя
	 *
	 * @details Кадр отводится под положения отступления рядов повторения
	 *          и выравнивается по шестнадцати байтам, как того требует
	 *          соглашение о вызове ARM64.
	 *
	 */
	const uint32_t frame = static_cast <uint32_t> ((((runs * SLOTS * sizeof(size_t)) + 15) / 16) * 16);
	// Заводим метку начала очередной попытки сопоставления
	const size_t attempt = emitter.label();
	// Заводим метку перехода к следующей позиции начала попытки
	const size_t following = emitter.label();
	// Заводим метку обнаружения совпадения в тексте
	const size_t found = emitter.label();
	// Заводим метку отсутствия совпадения в тексте
	const size_t none = emitter.label();
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
	// Выполняем расстановку метки начала очередной попытки сопоставления
	emitter.place(attempt);
	// Выполняем установку позиции сопоставления в позицию начала попытки
	emitter.move(reg_t::CURSOR, reg_t::KEEPER);
	// Номер порождаемого ряда повторения одиночного символа
	size_t index = 0;
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
		 * Если инструкция сохраняет позицию в ячейке захвата
		 */
		if(instruction.type == opcode_t::SAVE) {
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
	// Выполняем переход к началу очередной попытки сопоставления
	emitter.jump(attempt);
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
	for(size_t i = 0; i < this->_context.size(); i++)
		// Выполняем установку адреса таблицы принадлежности байтов
		this->_context.at(i) = (this->_members.data() + (i * TABLE));
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
