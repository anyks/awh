/**
 * @file codegen.cpp
 * @date 2026-08-02
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
 * @brief Реализация преобразования программы регулярного выражения в машинный код —
 *        обход инструкций программы, порождение прохода рядов повторения
 *        и сборка обстановки исполнения порождённого кода
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <algorithm>
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/text.hpp>
#include <regex/codegen.hpp>
#include <encoding/unicode/unicode.hpp>

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
	 * @details Ряд занимает три восьмибайтовых значения: положение отступления,
	 *          положение начала ряда, ниже какого отступление невозможно,
	 *          и отказ, действовавший до прохода ряда.
	 *
	 */
	constexpr size_t SLOTS = 3;

	/**
	 * @brief Размер кадра вызова, приходящийся на одну цепочку ветвей выбора
	 *
	 * @details Цепочка занимает два восьмибайтовых значения: позицию начала выбора
	 *          и отказ, действовавший до выбора.
	 *
	 */
	constexpr size_t PICKS = 2;

	/**
	 * @brief Количество мест кадра вызова, сохраняющих регистры на время вызова
	 *
	 * @details Соглашение о вызове подпрограмм сохранности младших регистров
	 *          не обещает, поэтому в кадре вызова сохраняется всё, чем сопоставление
	 *          живёт: адрес текста, размер текста, адрес набора границ, адрес
	 *          обстановки исполнения, позиция начала попытки, позиция сопоставления
	 *          и адрес возврата. Позиция сопоставления попала сюда не сразу:
	 *          пока подпрограммы вызывались лишь до начала попытки, она была мертва,
	 *          а проверка привязки к границе слова вызывается посреди сопоставления.
	 *
	 */
	constexpr size_t SPILLS = 7;

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
	 * @brief Номер места обстановки, несущего адрес подпрограммы проверки привязки
	 *
	 */
	constexpr size_t SLOT_ASSERTING = 3;

	/**
	 * @brief Номер места обстановки, несущего адрес подпрограммы прохода ряда
	 *
	 */
	constexpr size_t SLOT_SCANNING = 4;

	/**
	 * @brief Количество мест обстановки, отведённых прежде значений хранилища
	 *
	 */
	constexpr size_t SLOT_TABLES = 5;

	/**
	 * @brief Количество границ совпадения, отводимых на кадре вызова
	 *
	 * @details Значение покрывает совпадение целиком и пятнадцать захватывающих
	 *          групп, чего достаёт подавляющему большинству выражений, а занимает
	 *          при этом двести пятьдесят шесть байтов кадра.
	 *
	 */
	constexpr size_t BOUNDS = 32;

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
	 * @brief Функция прохода ряда повторения поиском предела его
	 *
	 * @details Ряд, набору какого принадлежат все значения байта кроме одного,
	 *          проходится поиском этого одного значения: поиск байта в тексте
	 *          выполняется набором команд процессора над несколькими байтами
	 *          сразу, тогда как порождённый разбор таблицы принадлежности читает
	 *          из памяти дважды на каждый байт. Ряды прочие проходятся
	 *          порождённым кодом: у них предел один поиском не описывается.
	 *
	 * @param text  адрес начала текста сопоставления
	 * @param size  размер текста сопоставления в байтах
	 * @param pos   позиция начала прохода ряда
	 * @param limit адрес значения байта, ряд ограничивающего
	 * @return      позиция завершения прохода ряда
	 *
	 */
	size_t scanning(const char * text, const size_t size, const size_t pos, const void * limit) noexcept {
		// Получаем значение байта, ряд ограничивающее
		const uint8_t letter = (* reinterpret_cast <const uint8_t *> (limit));
		/**
		 * Если позиция начала прохода за пределы текста выходит
		 */
		if(pos >= size)
			// Выводим позицию завершения прохода ряда
			return size;
		// Выполняем поиск значения байта, ряд ограничивающего
		const void * found = ::memchr((text + pos), static_cast <int> (letter), (size - pos));
		// Выводим позицию завершения прохода ряда
		return ((found != nullptr) ? static_cast <size_t> (reinterpret_cast <const char *> (found) - text) : size);
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
	 * @brief Функция проверки привязки к позиции в тексте
	 *
	 * @details Проверка выполняется той же подпрограммой, что и при исполнении
	 *          программы, отчего смысл привязки в порождённом коде и в толкователе
	 *          совпадает по построению, а не по сличению. Позиция начала попытки
	 *          подпрограмме не передаётся: вызовом проверяются лишь привязки
	 *          к границе слова, от начала попытки не зависящие.
	 *
	 * @param text  адрес начала текста сопоставления
	 * @param size  размер текста сопоставления в байтах
	 * @param pos   проверяемая позиция в тексте
	 * @param guard адрес приметы проверяемой привязки
	 * @return      результат проверки привязки к позиции в тексте
	 *
	 */
	size_t asserting(const char * text, const size_t size, const size_t pos, const void * guard) noexcept {
		// Получаем примету проверяемой привязки к позиции в тексте
		const uint64_t packed = (* reinterpret_cast <const uint64_t *> (guard));
		// Выводим результат проверки привязки к позиции в тексте
		return (awh::regex::assertion(std::string_view(text, size), 0,
		 static_cast <awh::regex::anchor_t> (packed & 0xFF), static_cast <uint32_t> (packed >> 8), pos) ? 1 : 0);
	}
	/**
	 * @brief Функция проверки порождения привязки в самом машинном коде
	 *
	 * @details Привязки к границам текста и строки порождаются несколькими
	 *          командами, тогда как привязка к границе слова требует разбора
	 *          символа, ей предшествующего, - в режиме разбора UTF-8 он занимает
	 *          несколько байтов, - и потому выполняется вызовом подпрограммы.
	 *
	 * @param type тип проверяемой привязки к позиции в тексте
	 * @return     результат проверки порождения привязки в машинном коде
	 *
	 */
	inline bool inlined(const awh::regex::anchor_t type) noexcept {
		// Выводим результат проверки порождения привязки в машинном коде
		return ((type != awh::regex::anchor_t::WORD_EDGE) && (type != awh::regex::anchor_t::WORD_INNER));
	}
	/**
	 * @brief Функция порождения вызова подпрограммы обстановки исполнения
	 *
	 * @details Порождается сохранение затираемых вызовом регистров в кадре,
	 *          передача доводов, вызов и восстановление сохранённых регистров.
	 *          Итог вызова остаётся в регистре промежуточного значения,
	 *          восстановлением не затрагиваемом.
	 *
	 * @param emitter  объект порождения машинного кода
	 * @param slot     номер места обстановки, несущего адрес подпрограммы
	 * @param spill    номер первого места кадра, сохраняющего регистры
	 * @param pos      регистр позиции, передаваемой подпрограмме доводом
	 * @param argument номер места обстановки, передаваемого подпрограмме доводом
	 *
	 */
	void invoke(awh::regex::Emitter & emitter, const size_t slot, const size_t spill, const awh::regex::Emitter::reg_t pos, const size_t argument) noexcept {
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
		emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (spill + 6));
		// Выполняем чтение адреса вызываемой подпрограммы из обстановки исполнения
		emitter.context(reg_t::SCRATCH, static_cast <uint32_t> (slot));
		// Выполняем передачу адреса значения обстановки четвёртым доводом
		emitter.context(reg_t::BOUNDS, static_cast <uint32_t> (argument));
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
		emitter.fetch(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (spill + 6));
	}

	/**
	 * @brief Разбор цепочки ветвей выбора одной из них
	 *
	 */
	typedef struct Chain {
		// Набор ветвей выбора, задаваемых началом и концом области инструкций
		std::vector <std::pair <awh::regex::address_t, awh::regex::address_t>> branches;
		// Адрес инструкции, следующей за выбором одной из ветвей
		awh::regex::address_t join;
		/**
		 * @brief Конструктор
		 *
		 */
		Chain() noexcept : join(awh::regex::INVALID_ADDRESS) {}
	} chain_t;

	/**
	 * @brief Функция разбора цепочки ветвей выбора одной из них
	 *
	 * @details Выбор одной из ветвей компилируется цепочкой переходов по двум ветвям:
	 *          первая ветвь каждого перехода несёт очередной выбор, вторая - остаток
	 *          цепочки, а завершается всякая ветвь, кроме последней, переходом
	 *          к общему продолжению. Необязательный элемент выражения устроен так же,
	 *          но последняя ветвь его пуста, отчего разбор для них един.
	 *
	 * @param program программа регулярного выражения
	 * @param pc      адрес перехода по двум ветвям, цепочку возглавляющего
	 * @param result  разобранная цепочка ветвей выбора
	 * @return        результат разбора цепочки ветвей выбора
	 *
	 */
	bool chaining(const awh::regex::program_t & program, const awh::regex::address_t pc, chain_t & result) noexcept {
		// Получаем адрес разбираемого перехода по двум ветвям
		awh::regex::address_t current = pc;
		/**
		 * Выполняем разбор цепочки переходов по двум ветвям
		 */
		while(static_cast <size_t> (current) < program.instructions.size()) {
			// Получаем очередной разбираемый переход по двум ветвям
			const awh::regex::instruction_t & instruction = program.instructions.at(static_cast <size_t> (current));
			/**
			 * Если очередная инструкция переходом по двум ветвям не является
			 */
			if(instruction.type != awh::regex::opcode_t::SPLIT)
				// Выходим из разбора цепочки ветвей выбора
				break;
			/**
			 * Если очередной переход возглавляет повторение одиночного символа
			 */
			if(instruction.split.run != awh::regex::INVALID_ADDRESS)
				// Выходим из разбора цепочки ветвей выбора
				break;
			// Получаем адрес остатка цепочки ветвей выбора
			const awh::regex::address_t rest = instruction.split.second;
			/**
			 * Если адреса ветвей выбора порядка не соблюдают
			 *
			 * @details Порождение обходит инструкции в порядке возрастания адресов,
			 *          поэтому область ветви обязана лежать между началом её
			 *          и остатком цепочки.
			 *
			 */
			if((instruction.split.first <= current) || (rest <= instruction.split.first) ||
			 (static_cast <size_t> (rest) > program.instructions.size()))
				// Выводим отказ разбора цепочки ветвей выбора
				return false;
			// Получаем конец области очередной ветви выбора
			awh::regex::address_t finish = rest;
			// Получаем инструкцию, областью ветви завершающую
			const awh::regex::instruction_t & last = program.instructions.at(static_cast <size_t> (rest - 1));
			/**
			 * Если область ветви завершается переходом к общему продолжению
			 */
			if(last.type == awh::regex::opcode_t::JUMP) {
				// Выполняем установку конца области очередной ветви выбора
				finish = (rest - 1);
				/**
				 * Если общее продолжение цепочки ветвей ещё не установлено
				 */
				if(result.join == awh::regex::INVALID_ADDRESS)
					// Выполняем установку общего продолжения цепочки ветвей
					result.join = last.jump.target;
				/**
				 * Если общее продолжение цепочки ветвей установлено иначе
				 */
				else if(result.join != last.jump.target)
					// Выводим отказ разбора цепочки ветвей выбора
					return false;
			}
			// Выполняем добавление разобранной ветви выбора
			result.branches.emplace_back(instruction.split.first, finish);
			// Переходим к остатку цепочки ветвей выбора
			current = rest;
			/**
			 * Если область ветви переходом к общему продолжению не завершается
			 *
			 * @details Так устроен необязательный элемент выражения: ветвь его
			 *          единственна, а остаток цепочки и есть общее продолжение.
			 *          Продолжать разбор за него нельзя - следующий переход
			 *          по двум ветвям принадлежит уже иному выбору, и включение
			 *          его в цепочку обратило бы «x?y?» в «(x|y|)».
			 *
			 */
			if(finish == rest) {
				/**
				 * Если общее продолжение цепочки ветвей ещё не установлено
				 */
				if(result.join == awh::regex::INVALID_ADDRESS)
					// Выполняем установку общего продолжения цепочки ветвей
					result.join = rest;
				/**
				 * Если общее продолжение цепочки ветвей установлено иначе
				 */
				else if(result.join != rest)
					// Выводим отказ разбора цепочки ветвей выбора
					return false;
				// Выходим из разбора цепочки ветвей выбора
				break;
			}
		}
		/**
		 * Если цепочка ветвей выбора не разобрана
		 */
		if(result.branches.empty())
			// Выводим отказ разбора цепочки ветвей выбора
			return false;
		/**
		 * Если общее продолжение цепочки ветвей не установлено
		 *
		 * @details Переходом к общему продолжению не завершается лишь ветвь
		 *          необязательного элемента выражения: остаток цепочки при этом
		 *          и есть общее продолжение.
		 *
		 */
		if(result.join == awh::regex::INVALID_ADDRESS)
			// Выполняем установку общего продолжения цепочки ветвей
			result.join = current;
		/**
		 * Если остаток цепочки за общее продолжение выходит
		 */
		if(current > result.join)
			// Выводим отказ разбора цепочки ветвей выбора
			return false;
		// Выполняем добавление последней ветви выбора
		result.branches.emplace_back(current, result.join);
		// Выводим результат разбора цепочки ветвей выбора
		return true;
	}
	/**
	 * @brief Функция сбора ячеек захвата, записываемых внутри цепочки ветвей
	 *
	 * @details Ветвь, сопоставление какой прервано, оставила бы записанные ею
	 *          границы групп установленными, а ветвь следующая их не перезапишет.
	 *          Поэтому границы эти запоминаются при входе в цепочку и
	 *          восстанавливаются при переходе к ветви следующей. Сбор ведётся
	 *          обходом области цепочки по адресам, а не по путям исполнения:
	 *          лишняя ячейка в наборе стоит двух обращений к памяти, тогда как
	 *          недостающая дала бы неверные границы.
	 *
	 * @param program программа регулярного выражения
	 * @param from    начало области цепочки ветвей выбора
	 * @param to      конец области цепочки ветвей выбора
	 * @param result  набор собираемых ячеек захвата
	 *
	 */
	void journal(const awh::regex::program_t & program, const awh::regex::address_t from, const awh::regex::address_t to, std::vector <uint32_t> & result) noexcept {
		// Выполняем очистку набора собираемых ячеек захвата
		result.clear();
		/**
		 * Выполняем обход области цепочки ветвей выбора
		 */
		for(awh::regex::address_t pc = from; (pc < to) && (static_cast <size_t> (pc) < program.instructions.size()); pc++) {
			// Получаем очередную инструкцию области цепочки ветвей
			const awh::regex::instruction_t & instruction = program.instructions.at(static_cast <size_t> (pc));
			/**
			 * Если инструкция границы захватывающей группы не записывает
			 */
			if((instruction.type != awh::regex::opcode_t::SAVE) || (instruction.save.slot <= 1))
				// Переходим к следующей инструкции области
				continue;
			/**
			 * Если ячейка захвата в наборе уже присутствует
			 */
			if(std::find(result.begin(), result.end(), instruction.save.slot) != result.end())
				// Переходим к следующей инструкции области
				continue;
			// Выполняем добавление ячейки захвата в набор
			result.push_back(instruction.save.slot);
		}
	}
	/**
	 * @brief Функция проверки принадлежности инструкции сопоставляющим одиночный символ
	 *
	 * @param type код операции проверяемой инструкции
	 * @return     результат проверки принадлежности инструкции
	 *
	 */
	inline bool singular(const awh::regex::opcode_t type) noexcept {
		// Выводим результат проверки принадлежности инструкции
		return ((type == awh::regex::opcode_t::CHAR) || (type == awh::regex::opcode_t::CLASS) || (type == awh::regex::opcode_t::ANY));
	}
	/**
	 * @brief Функция проверки принадлежности значения байта сопоставляемым символам
	 *
	 * @details Расчёт этот ведёт и заведение таблицы принадлежности байтов, и
	 *          проверка применимости кодогенерации к программе в режиме разбора
	 *          UTF-8: оба обязаны сходиться в точности, иначе проверка отвечала
	 *          бы за один набор сопоставляемых значений, а порождение
	 *          выполнялось бы для другого.
	 *
	 * @param instruction сопоставляющая инструкция программы
	 * @param program     программа регулярного выражения
	 * @param letter      проверяемое значение байта
	 * @return            результат проверки принадлежности значения байта
	 *
	 */
	bool belonging(const awh::regex::instruction_t & instruction, const awh::regex::program_t & program, const uint32_t letter) noexcept {
		/**
		 * Определяем код операции сопоставляющей инструкции
		 */
		switch(static_cast <uint8_t> (instruction.type)) {
			/**
			 * Если инструкция сопоставляет одиночный символ
			 */
			case static_cast <uint8_t> (awh::regex::opcode_t::CHAR): {
				/**
				 * Если установлен режим сопоставления без учёта регистра
				 */
				if(awh::regex::hasFlag(instruction.flags, awh::regex::flag_t::CASELESS))
					// Выводим результат сопоставления символов без учёта регистра
					return (awh::regex::fold(letter, instruction.flags) == awh::regex::fold(instruction.letter.code, instruction.flags));
				// Выводим результат сопоставления символов с учётом регистра
				return (letter == instruction.letter.code);
			}
			/**
			 * Если инструкция сопоставляет символ из класса символов
			 */
			case static_cast <uint8_t> (awh::regex::opcode_t::CLASS):
				// Выводим результат проверки принадлежности символа классу символов
				return awh::regex::belongs(program.charclass(instruction.charclass.index), letter, instruction.flags);
			/**
			 * Если инструкция сопоставляет любой символ
			 */
			case static_cast <uint8_t> (awh::regex::opcode_t::ANY):
				// Выводим результат проверки соответствия символа переводу строки
				return (awh::regex::hasFlag(instruction.flags, awh::regex::flag_t::DOTALL) || (letter != 0x0A));
		}
		// Выводим отсутствие принадлежности значения байта
		return false;
	}
	/**
	 * @brief Функция проверки сопоставления инструкцией одних лишь символов ASCII
	 *
	 * @details Проверка эта потребна режиму разбора UTF-8: порождаемый код
	 *          сопоставляет байты, тогда как программа сопоставляет символы
	 *          целиком, и сходятся два эти способа лишь на символах ASCII.
	 *          Кодирование UTF-8 самосинхронизируется - байт ASCII внутри
	 *          последовательности многобайтовой не встречается вовсе, - отчего
	 *          инструкция, сопоставляющая одни лишь символы ASCII, сопоставлением
	 *          байтов выражается точно: байты последовательности многобайтовой
	 *          все до единого лежат вне ASCII и получают отказ, каковой выдало
	 *          бы и сопоставление символа целиком.
	 *
	 * @param instruction сопоставляющая инструкция программы
	 * @param program     программа регулярного выражения
	 * @return            результат проверки сопоставления одних лишь символов ASCII
	 *
	 */
	bool restricted(const awh::regex::instruction_t & instruction, const awh::regex::program_t & program) noexcept {
		// Получаем признак сопоставления символов без учёта регистра
		const bool caseless = awh::regex::hasFlag(instruction.flags, awh::regex::flag_t::CASELESS);
		/**
		 * @brief Проверка приведения к символу ASCII символов вне ASCII
		 *
		 * @details Приведение регистра по таблицам стандарта Юникода сводит
		 *          к символу ASCII и символы вне ASCII: знак Кельвина «K»
		 *          приводится к букве «k», а долгое «ſ» - к букве «s». Символ,
		 *          такой набор образующий, сопоставляется без учёта регистра
		 *          наравне с символом ASCII, отчего сопоставлением байтов
		 *          не выражается.
		 *
		 * @param code кодовое значение проверяемого символа
		 * @return     результат проверки приведения символов вне ASCII
		 *
		 */
		auto solitary = [](const uint32_t code) noexcept -> bool {
			// Создаём набор символов, приводимых к одному значению
			std::vector <uint32_t> members;
			/**
			 * Если символ набора приведения регистра не образует
			 */
			if(!awh::unicode::variants(code, members))
				// Выводим отсутствие символов вне ASCII в наборе приведения
				return true;
			/**
			 * Выполняем обход набора символов приведения регистра
			 */
			for(auto & member : members) {
				/**
				 * Если символ набора пределы ASCII превышает
				 */
				if(member >= 0x80)
					// Выводим наличие символов вне ASCII в наборе приведения
					return false;
			}
			// Выводим отсутствие символов вне ASCII в наборе приведения
			return true;
		};
		/**
		 * Определяем код операции сопоставляющей инструкции
		 */
		switch(static_cast <uint8_t> (instruction.type)) {
			/**
			 * Если инструкция сопоставляет одиночный символ
			 */
			case static_cast <uint8_t> (awh::regex::opcode_t::CHAR): {
				/**
				 * Если сопоставляемый символ пределы ASCII превышает
				 */
				if(instruction.letter.code >= 0x80)
					// Выводим сопоставление инструкцией символов вне ASCII
					return false;
				// Выводим результат проверки набора приведения регистра символа
				return (!caseless || solitary(instruction.letter.code));
			}
			/**
			 * Если инструкция сопоставляет символ из класса символов
			 */
			case static_cast <uint8_t> (awh::regex::opcode_t::CLASS): {
				// Получаем обзор класса символов сопоставляющей инструкции
				const awh::regex::classview_t & value = program.charclass(instruction.charclass.index);
				/**
				 * Если класс символов отрицается
				 *
				 * @details Класс отрицаемый сопоставляет всякий символ, в него
				 *          не входящий, а значит и символы вне ASCII.
				 *
				 */
				if(value.negative)
					// Выводим сопоставление инструкцией символов вне ASCII
					return false;
				/**
				 * Если класс символов ссылается на свойства Юникода
				 *
				 * @details Свойства стандарта Юникода описаны символами вне ASCII
				 *          в подавляющем большинстве, отчего проверка их
				 *          не ведётся вовсе.
				 *
				 */
				if(!value.properties.empty())
					// Выводим сопоставление инструкцией символов вне ASCII
					return false;
				/**
				 * Выполняем обход набора диапазонов класса символов
				 */
				for(auto & range : value.ranges) {
					/**
					 * Если диапазон класса пределы ASCII превышает
					 */
					if(range.end >= 0x80)
						// Выводим сопоставление инструкцией символов вне ASCII
						return false;
					/**
					 * Если сопоставление ведётся без учёта регистра
					 */
					if(caseless) {
						/**
						 * Выполняем обход символов диапазона класса
						 */
						for(uint32_t code = range.begin; code <= range.end; code++) {
							/**
							 * Если набор приведения регистра символа содержит символы вне ASCII
							 */
							if(!solitary(code))
								// Выводим сопоставление инструкцией символов вне ASCII
								return false;
						}
					}
				}
				// Выводим сопоставление инструкцией одних лишь символов ASCII
				return true;
			}
		}
		/**
		 * Выводим сопоставление инструкцией символов вне ASCII
		 *
		 * @details Сопоставление любого символа сопоставляет и символы вне ASCII,
		 *          а инструкция иная сопоставляющей не является вовсе.
		 *
		 */
		return false;
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
	bool walk(const awh::regex::program_t & program, size_t & runs, size_t & chains, size_t & recorded) noexcept {
		// Выполняем сброс количества рядов повторения
		runs = 0;
		// Выполняем сброс количества цепочек ветвей выбора
		chains = 0;
		// Выполняем сброс количества мест запоминания границ групп
		recorded = 0;
		/**
		 * Получаем признак сборки в режиме разбора UTF-8
		 *
		 * @details Порождаемый код сопоставляет байты, тогда как в режиме разбора
		 *          UTF-8 программа сопоставляет символы целиком, отчего
		 *          кодогенерацию получает не всякая программа этого режима,
		 *          а лишь такая, где сопоставляются одни символы ASCII: на них
		 *          два способа сопоставления сходятся в точности. Позиции же
		 *          начала попытки порождаемый код перебирает по границам символов,
		 *          а не по байтам, чем сходится с программой и на совпадении
		 *          пустом, какое сопоставления байтов не несёт вовсе.
		 *
		 */
		const bool utf = awh::regex::hasFlag(program.flags, awh::regex::flag_t::UTF);
		/**
		 * Если набор инструкций программы пуст
		 */
		if(program.instructions.empty())
			// Выводим неприменимость кодогенерации к программе
			return false;
		// Создаём набор посещённых адресов инструкций программы
		vector <bool> visited(program.instructions.size(), false);
		/**
		 * @brief Обход области инструкций программы
		 *
		 * @details Обход повторяет порядок порождения в точности: иначе проверка
		 *          применимости отвечала бы за одно устройство программы,
		 *          а порождение выполнялось бы для другого.
		 *
		 * @param from   начало обходимой области инструкций
		 * @param to     конец обходимой области инструкций
		 * @param inside признак обхода области ветви выбора
		 * @return       результат обхода области инструкций
		 *
		 */
		std::function <bool (const awh::regex::address_t, const awh::regex::address_t, const bool)> region;
		// Выполняем установку обхода области инструкций программы
		region = [&](const awh::regex::address_t from, const awh::regex::address_t to, const bool inside) noexcept -> bool {
		// Получаем адрес исполняемой инструкции программы
		awh::regex::address_t pc = from;
		/**
		 * Выполняем обход инструкций области программы
		 */
		while(true) {
			/**
			 * Если область инструкций исчерпана
			 */
			if((to != awh::regex::INVALID_ADDRESS) && (pc >= to))
				// Выводим применимость кодогенерации к области
				return true;
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
			if(singular(instruction.type)) {
				/**
				 * Если инструкция в режиме разбора UTF-8 сопоставляет символы вне ASCII
				 */
				if(utf && !restricted(instruction, program))
					// Выводим неприменимость кодогенерации к программе
					return false;
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
				 *          сохранение их пропускается, а сохранение границ
				 *          захватывающих групп размещается записью позиции
				 *          в набор границ вызывающей стороны.
				 *
				 */
				case static_cast <uint8_t> (awh::regex::opcode_t::SAVE): {
					/**
					 * Если ячейка захвата за пределы набора границ выходит
					 */
					if(instruction.save.slot >= ((program.captures + 1) * 2))
						// Выводим неприменимость кодогенерации к программе
						return false;

					// Переходим к следующей инструкции программы
					pc++;
				} break;
				/**
				 * Если инструкция проверяет привязку к позиции в тексте
				 *
				 * @details Сброс начала совпадения смещает границу совпадения,
				 *          какую порождённый код ведёт сам, и потому отвергается.
				 *
				 */
				case static_cast <uint8_t> (awh::regex::opcode_t::ANCHOR): {
					/**
					 * Если привязка сбрасывает начало совпадения
					 */
					if(instruction.assertion.type == awh::regex::anchor_t::KEEP_OUT)
						// Выводим неприменимость кодогенерации к программе
						return false;
					// Переходим к следующей инструкции программы
					pc++;
				} break;
				/**
				 * Если инструкция выполняет переход по двум ветвям
				 */
				case static_cast <uint8_t> (awh::regex::opcode_t::SPLIT): {
					// Получаем признак ленивого повторения одиночного символа
					const bool lazily = (instruction.split.lazy != awh::regex::INVALID_ADDRESS);
					// Получаем адрес тела повторения одиночного символа
					const awh::regex::address_t body = (lazily ? instruction.split.lazy : instruction.split.run);
					/**
					 * Если переход возглавляет цепочку ветвей выбора одной из них
					 */
					if(body == awh::regex::INVALID_ADDRESS) {
						// Создаём разбираемую цепочку ветвей выбора одной из них
						chain_t branching;
						/**
						 * Если разбор цепочки ветвей выбора не выполнен
						 */
						if(!chaining(program, pc, branching))
							// Выводим неприменимость кодогенерации к программе
							return false;
						/**
						 * Если цепочка ветвей за пределы обходимой области выходит
						 */
						if((to != awh::regex::INVALID_ADDRESS) && (branching.join > to))
							// Выводим неприменимость кодогенерации к программе
							return false;
						/**
						 * Если допустимое количество цепочек ветвей исчерпано
						 */
						if(++chains > awh::regex::MAX_CHAINS)
							// Выводим неприменимость кодогенерации к программе
							return false;
						// Создаём набор ячеек захвата, цепочкой записываемых
						std::vector <uint32_t> records;
						// Выполняем сбор ячеек захвата, цепочкой записываемых
						journal(program, pc, branching.join, records);
						// Увеличиваем количество мест запоминания границ групп
						recorded += records.size();
						/**
						 * Выполняем обход областей ветвей выбора одной из них
						 */
						for(auto & item : branching.branches) {
							/**
							 * Если обход области очередной ветви выбора не выполнен
							 */
							if(!region(item.first, item.second, true))
								// Выводим неприменимость кодогенерации к программе
								return false;
						}
						// Переходим к общему продолжению цепочки ветвей выбора
						pc = branching.join;
						// Выходим из определения кода операции инструкции
						break;
					}
					/**
					 * Если тело повторения одиночного символа не сопоставляет
					 */
					if((static_cast <size_t> (body) >= program.instructions.size()) || !singular(program.instructions.at(static_cast <size_t> (body)).type))
						// Выводим неприменимость кодогенерации к программе
						return false;
					/**
					 * Если тело повторения в режиме разбора UTF-8 сопоставляет символы вне ASCII
					 *
					 * @details Тело повторения обходом не посещается: обход
					 *          проходит к ветви завершения повторения, минуя его, -
					 *          отчего проверка тела ведётся здесь.
					 *
					 */
					if(utf && !restricted(program.instructions.at(static_cast <size_t> (body)), program))
						// Выводим неприменимость кодогенерации к программе
						return false;
					/**
					 * Если допустимое количество рядов повторения исчерпано
					 */
					if(++runs > awh::regex::MAX_RUNS)
						// Выводим неприменимость кодогенерации к программе
						return false;
					/**
					 * Переходим к ветви завершения повторения
					 *
					 * @details Завершение ленивого повторения ведает ветвь первая:
					 *          ветви его переставлены.
					 *
					 */
					pc = (lazily ? instruction.split.first : instruction.split.second);
				} break;
				/**
				 * Если инструкция завершает сопоставление с успехом
				 */
				case static_cast <uint8_t> (awh::regex::opcode_t::MATCH):
					// Выводим применимость кодогенерации к области инструкций
					return (to == awh::regex::INVALID_ADDRESS);
				/**
				 * Если инструкция кодогенерации не получает
				 */
				default:
					// Выводим неприменимость кодогенерации к программе
					return false;
			}
		}
		};
		// Выводим результат обхода программы целиком
		return region(0, awh::regex::INVALID_ADDRESS, false);
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
	// Получаем смещение размещаемой таблицы в хранилище значений
	const size_t offset = this->_members.size();
	// Выполняем заведение места под адрес таблицы в обстановке исполнения
	this->_context.push_back(nullptr);
	// Выполняем запоминание смещения таблицы в хранилище значений
	this->_offsets.push_back(offset);
	// Выполняем размещение таблицы принадлежности значений байта
	this->_members.resize(this->_members.size() + TABLE, 0);
	// Получаем адрес размещённой таблицы принадлежности байтов
	uint8_t * members = (this->_members.data() + offset);
	/**
	 * Выполняем обход пространства значений байта
	 */
	for(uint32_t letter = 0; letter < TABLE; letter++)
		// Выполняем установку принадлежности значения байта
		members[letter] = (belonging(instruction, program, letter) ? 1 : 0);
	// Выводим номер заведённой таблицы принадлежности байтов
	return result;
}
/**
 * @brief Метод заведения таблицы допустимых начальных байтов совпадения
 *
 * @details Таблица повторяет набор допустимых байтов предварительного отбора,
 *          но в виде, порождённому коду доступном: обращение по значению байта
 *          взамен обращения к полю набора.
 *
 * @return номер заведённой таблицы в обстановке исполнения
 *
 */
size_t awh::regex::Codegen::sieve() noexcept {
	// Получаем номер заводимой таблицы допустимых начальных байтов
	const size_t result = this->_context.size();
	// Получаем смещение размещаемой таблицы в хранилище значений
	const size_t offset = this->_members.size();
	// Выполняем заведение места под адрес таблицы в обстановке исполнения
	this->_context.push_back(nullptr);
	// Выполняем запоминание смещения таблицы в хранилище значений
	this->_offsets.push_back(offset);
	// Выполняем размещение таблицы допустимых начальных байтов
	this->_members.resize((this->_members.size() + TABLE), 0);
	/**
	 * Выполняем обход пространства значений байта
	 */
	for(size_t letter = 0; letter < TABLE; letter++)
		// Выполняем установку допустимости значения байта в начале совпадения
		this->_members.at(offset + letter) = (this->_prefilter.bytes[letter] ? 1 : 0);
	// Выводим номер заведённой таблицы допустимых начальных байтов
	return result;
}
/**
 * @brief Метод заведения значения байта, ряд повторения ограничивающего
 *
 * @param letter значение байта, ряд повторения ограничивающее
 * @return       номер заведённого значения в обстановке исполнения
 *
 */
size_t awh::regex::Codegen::limiter(const uint8_t letter) noexcept {
	// Получаем номер заводимого значения байта
	const size_t result = this->_context.size();
	// Получаем смещение размещаемого значения в хранилище значений
	const size_t offset = this->_members.size();
	// Выполняем заведение места под адрес значения в обстановке исполнения
	this->_context.push_back(nullptr);
	// Выполняем запоминание смещения значения в хранилище значений
	this->_offsets.push_back(offset);
	// Выполняем размещение значения байта, ряд ограничивающего
	this->_members.resize((this->_members.size() + sizeof(uint64_t)), 0);
	// Выполняем запись значения байта в хранилище значений
	this->_members.at(offset) = letter;
	// Выводим номер заведённого значения байта
	return result;
}
/**
 * @brief Метод заведения приметы привязки к позиции в тексте
 *
 * @param instruction инструкция привязки к позиции в тексте
 * @return            номер заведённой приметы в обстановке исполнения
 *
 */
size_t awh::regex::Codegen::guard(const instruction_t & instruction) noexcept {
	// Получаем номер заводимой приметы привязки к позиции в тексте
	const size_t result = this->_context.size();
	// Получаем смещение размещаемой приметы в хранилище значений
	const size_t offset = this->_members.size();
	// Выполняем заведение места под адрес приметы в обстановке исполнения
	this->_context.push_back(nullptr);
	// Выполняем запоминание смещения приметы в хранилище значений
	this->_offsets.push_back(offset);
	// Выполняем размещение приметы привязки к позиции в тексте
	this->_members.resize(this->_members.size() + sizeof(uint64_t), 0);
	/**
	 * Собираем примету привязки из типа её и набора режимов компиляции
	 *
	 * @details Тип привязки занимает младшие восемь разрядов, набор режимов -
	 *          старшие: подпрограмма проверки разбирает примету обратно.
	 *
	 */
	const uint64_t packed = (static_cast <uint64_t> (instruction.assertion.type) |
	 (static_cast <uint64_t> (instruction.flags) << 8));
	// Выполняем запись собранной приметы в хранилище значений
	::memcpy((this->_members.data() + offset), &packed, sizeof(packed));
	// Выводим номер заведённой приметы привязки
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
	// Количество цепочек ветвей выбора одной из них
	size_t chains = 0;
	// Количество мест запоминания границ групп
	size_t recorded = 0;
	// Выводим результат проверки применимости кодогенерации
	return walk(program, runs, chains, recorded);
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
	// Количество цепочек ветвей выбора одной из них
	size_t chains = 0;
	// Количество мест запоминания границ групп
	size_t recorded = 0;
	/**
	 * Если кодогенерация сборкой не поддерживается
	 */
	if(!Emitter::available() || !Assembly::available())
		// Выводим результат порождения сопоставителя
		return false;
	/**
	 * Если кодогенерация к программе неприменима
	 */
	if(!walk(program, runs, chains, recorded))
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
	const bool seek = (!program.anchored && this->_prefilter.active &&
	 (this->_prefilter.unique || (this->_prefilter.leading.size() > 1)));
	/**
	 * Получаем признак порождения проверки возможности совпадения
	 *
	 * @details Проверка ищет в тексте обязательный литерал совпадения, а отбор
	 *          позиции начала попытки ищет ведущий: поиск обязательного литерала
	 *          проходом текста повторял бы уже выполненное, если обязательный
	 *          литерал ведущим и содержится.
	 *
	 */
	const bool possible = (!this->_prefilter.literal.empty() &&
	 !(seek && (this->_prefilter.leading.find(this->_prefilter.literal) != string::npos)));
	/**
	 * Получаем признак порождения просеивания позиций начала попытки
	 *
	 * @details Просеивание пропускает позиции, байт в каких совпадению начинать
	 *          не дан. Отбором позиций оно не заменяется: отбор ищет ведущий
	 *          литерал либо единственный байт, а набор допустимых байтов ни тем,
	 *          ни другим не описывается - таков всякий выбор ветвей, литералы
	 *          каких начинаются по-разному.
	 *
	 *          Порождается просеивание в самом коде, а не вызовом подпрограммы:
	 *          проход текста побайтно оно ускоряет не пропуском участков, а
	 *          дешевизной хода - два обращения к памяти взамен попытки
	 *          сопоставления целиком. Вызов подпрограммы на такой ход был бы
	 *          расходом чистым.
	 *
	 */
	size_t allowed = 0;
	/**
	 * Выполняем обход набора допустимых начальных байтов совпадения
	 */
	for(size_t letter = 0; letter < TABLE; letter++)
		// Выполняем подсчёт байтов, совпадение начинать способных
		allowed += (this->_prefilter.bytes[letter] ? 1 : 0);
	// Получаем признак порождения просеивания позиций начала попытки
	const bool sifting = (!program.anchored && this->_prefilter.active && !seek && (allowed > 0) && (allowed < TABLE));
	/**
	 * Получаем номер места кадра, действующий отказ сопоставления несущего
	 *
	 * @details Отказ, действующий в очередной миг сопоставления, известен лишь
	 *          при исполнении: вложенные выборы ветвей откладывают его один
	 *          за другим и снимают в обратном порядке. Ведётся он единственной
	 *          ячейкой кадра, а цепочки ветвей сохраняют в своих местах прежнее
	 *          её значение.
	 *
	 */
	/**
	 * Получаем признак ведения действующего отказа ячейкой кадра
	 *
	 * @details Выбор одной из ветвей откладывает отказ и снимает его при исполнении,
	 *          отчего вести отказ меткой при выборах нельзя. Без выборов же связка
	 *          отказов известна при порождении вся: ряды повторения вложены друг
	 *          в друга по построению. Ведение отказа ячейкой стоит двух обращений
	 *          к памяти на проход ряда и перехода по вычисленному адресу на каждое
	 *          отступление, поэтому выражения без выборов его не получают.
	 *
	 */
	const bool cellular = (chains > 0);
	/**
	 * Получаем номер первого места кадра, границы групп запоминающего
	 *
	 * @details Места отводятся цепочкам ветвей одно за другим в том порядке,
	 *          в каком они порождаются, а порождение обходит программу в том же
	 *          порядке, что и проверка применимости, отчего количество мест,
	 *          проверкой посчитанное, порождению и отвечает.
	 *
	 */
	const size_t records = ((runs * SLOTS) + (chains * PICKS));
	// Получаем номер места кадра, действующий отказ сопоставления несущего
	const size_t cell = (records + recorded);
	// Получаем номер места кадра, несущего позицию начала поиска совпадения
	const size_t origin = (cell + 1);
	// Получаем номер места кадра, несущего положение конца первого ряда повторения
	const size_t span = (origin + 1);
	// Получаем номер первого места кадра, сохраняющего регистры на время вызова
	const size_t spill = (span + 1);
	/**
	 * Получаем признак пропуска пройденного участка при отказе попытки
	 *
	 * @details Отказ попытки, ряд повторения исчерпавшей, означает, что хвост
	 *          выражения не сошёлся ни в одной позиции отступления ряда. Начало
	 *          попытки более позднее сдвигает начало ряда вперёд, а конец его
	 *          оставляет прежним - байт, ряду не принадлежащий, стоит на месте, -
	 *          отчего набор проверяемых позиций окажется подмножеством уже
	 *          отвергнутого. Такая попытка отказна заведомо, и перебор её
	 *          проходит участок повторно, давая квадрат от длины ряда.
	 *
	 *          Правомерен пропуск лишь при постоянной длине участка, ряду
	 *          предшествующего: ветви выбора разной длины сдвинули бы начало
	 *          ряда назад, открыв позиции, ещё не проверенные. Потому выражения
	 *          с цепочками ветвей пропуска не получают.
	 *
	 *          Хвост выражения сходится или нет по одной лишь позиции: привязки,
	 *          от начала попытки зависящие, принимаемое подмножество не содержит.
	 *
	 */
	const bool skipping = (!program.anchored && (chains == 0) && (runs > 0));
	/**
	 * Получаем размер кадра вызова порождаемого сопоставителя
	 *
	 * @details Кадр отводится под положения отступления рядов повторения
	 *          и места сохранения регистров, вызовом затираемых, а выравнивается
	 *          по шестнадцати байтам, как того требует соглашение о вызове ARM64.
	 *
	 */
	const uint32_t frame = static_cast <uint32_t> (((((spill + SPILLS) * sizeof(size_t)) + 15) / 16) * 16);
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
	// Заводим метку просеивания позиции начала очередной попытки сопоставления
	const size_t sifter = emitter.label();
	// Заводим метку выхода позиции начала попытки на границу символа
	const size_t border = emitter.label();
	/**
	 * Получаем признак сборки в режиме разбора UTF-8
	 *
	 * @details В режиме этом позиции начала попытки перебираются по границам
	 *          символов, а не по байтам: программа продвигает начало поиска
	 *          на символ целиком, и перебор побайтовый выдавал бы совпадение
	 *          пустое посреди символа многобайтового, какого программа
	 *          не выдала бы вовсе.
	 *
	 */
	const bool utf = hasFlag(program.flags, flag_t::UTF);
	// Получаем метку входа в очередную попытку сопоставления
	const size_t entry = (utf ? border : attempt);
	// Создаём набор меток отступления рядов повторения
	vector <size_t> retries;
	/**
	 * Выполняем заведение меток отступления рядов повторения
	 */
	for(size_t i = 0; i < runs; i++)
		// Выполняем заведение метки отступления очередного ряда
		retries.push_back(emitter.label());
	// Выполняем размещение входа в порождаемый сопоставитель
	emitter.prologue(frame);
	// Выполняем установку позиции начала попытки сопоставления
	emitter.move(reg_t::KEEPER, reg_t::START);
	// Выполняем сохранение позиции начала поиска совпадения в кадре вызова
	emitter.store(reg_t::START, reg_t::STACK, static_cast <uint32_t> (origin));
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
		invoke(emitter, SLOT_FEASIBLE, spill, reg_t::KEEPER, SLOT_PREFILTER);
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
		invoke(emitter, SLOT_SEEKING, spill, reg_t::KEEPER, SLOT_PREFILTER);
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
	/**
	 * Если просеивание позиций начала попытки порождается
	 */
	if(sifting) {
		// Выполняем заведение таблицы допустимых начальных байтов совпадения
		const size_t number = this->sieve();
		// Выполняем расстановку метки просеивания позиций начала попытки
		emitter.place(sifter);
		// Выполняем сравнение позиции начала попытки с размером текста
		emitter.compare(reg_t::KEEPER, reg_t::SIZE);
		/**
		 * Выполняем переход к отсутствию совпадения по исчерпании текста
		 *
		 * @details Совпадение в позиции конца текста при действующем просеивании
		 *          невозможно: просеивание ведётся по байту, совпадением
		 *          сопоставляемому, отчего пустое совпадение его не имеет.
		 *
		 */
		emitter.branch(cond_t::ABOVE, none);
		// Выполняем чтение байта текста в позиции начала попытки
		emitter.load(reg_t::LETTER, reg_t::TEXT, reg_t::KEEPER);
		// Выполняем чтение адреса таблицы допустимых начальных байтов
		emitter.context(reg_t::SCRATCH, static_cast <uint32_t> (number));
		// Выполняем чтение допустимости байта в начале совпадения
		emitter.load(reg_t::SPARE, reg_t::SCRATCH, reg_t::LETTER);
		// Выполняем сравнение допустимости байта с нулём
		emitter.compare(reg_t::SPARE, static_cast <uint32_t> (0));
		// Выполняем переход к попытке сопоставления при допустимости байта
		emitter.branch(cond_t::NOTEQUAL, entry);
		// Переходим к следующей позиции начала попытки сопоставления
		emitter.add(reg_t::KEEPER, reg_t::KEEPER, 1);
		// Выполняем переход к просеиванию позиции следующей
		emitter.jump(sifter);
	}
	/**
	 * Если выход позиции начала попытки на границу символа порождается
	 *
	 * @details Байтом продолжающим кодирование UTF-8 полагает всякий байт
	 *          в пределах от 0x80 до 0xBF, а граница символа стоит перед
	 *          всяким байтом иным. Проверка ведётся вычитанием нижнего предела
	 *          со сравнением без учёта знака: значение, предела не достигшее,
	 *          переполняется вниз и сравнение это не проходит.
	 *
	 */
	if(utf) {
		// Выполняем расстановку метки выхода позиции на границу символа
		emitter.place(border);
		// Выполняем сравнение позиции начала попытки с размером текста
		emitter.compare(reg_t::KEEPER, reg_t::SIZE);
		/**
		 * Выполняем переход к попытке сопоставления по исчерпании текста
		 *
		 * @details Позиция конца текста границею символа является всегда,
		 *          а чтение байта в ней вышло бы за пределы текста.
		 *
		 */
		emitter.branch(cond_t::ABOVE, attempt);
		// Выполняем чтение байта текста в позиции начала попытки
		emitter.load(reg_t::LETTER, reg_t::TEXT, reg_t::KEEPER);
		// Выполняем вычитание нижнего предела байтов продолжающих
		emitter.sub(reg_t::LETTER, reg_t::LETTER, static_cast <uint32_t> (0x80));
		// Выполняем сравнение байта с шириною набора байтов продолжающих
		emitter.compare(reg_t::LETTER, static_cast <uint32_t> (0x40));
		// Выполняем переход к попытке сопоставления на границе символа
		emitter.branch(cond_t::ABOVE, attempt);
		// Переходим к следующей позиции начала попытки сопоставления
		emitter.add(reg_t::KEEPER, reg_t::KEEPER, 1);
		// Выполняем переход к выходу позиции следующей на границу символа
		emitter.jump(border);
	}
	// Выполняем расстановку метки начала очередной попытки сопоставления
	emitter.place(attempt);
	// Выполняем установку позиции сопоставления в позицию начала попытки
	emitter.move(reg_t::CURSOR, reg_t::KEEPER);
	/**
	 * Если пропуск пройденного участка при отказе попытки порождается
	 *
	 * @details Положение конца ряда полагается равным началу попытки: попытка,
	 *          отказавшая прежде ряда, пропускать ничего не вправе, а положение,
	 *          попыткой прежней записанное, пропустило бы участок непройденный.
	 *
	 */
	if(skipping)
		// Выполняем установку положения конца первого ряда в кадре вызова
		emitter.store(reg_t::KEEPER, reg_t::STACK, static_cast <uint32_t> (span));
	/**
	 * Выполняем установку действующего отказа очередной попытки сопоставления
	 *
	 * @details Попытка начинается без отложенных выборов, поэтому отказ ведёт
	 *          к следующей позиции её начала.
	 *
	 */
	if(cellular) {
		// Выполняем получение адреса перехода к следующей позиции начала попытки
		emitter.address(reg_t::SCRATCH, following);
		// Выполняем сохранение действующего отказа сопоставления в кадре вызова
		emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
	}
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
	const size_t verified = (seek ? this->_prefilter.leading.size() : 0);
	// Количество символов ведущего литерала, сопоставление каких уже заменено
	size_t consumed = 0;
	// Количество символов, продвижение позиции на какие ещё не размещено
	size_t elided = 0;
	/**
	 * Длина участка, первому ряду повторения предшествующего
	 *
	 * @details Длина эта постоянна и известна при порождении: до ряда первого
	 *          размещаются лишь сопоставления одиночных символов. Пропуск
	 *          пройденного участка её вычитает: ряд при начале попытки более
	 *          позднем начинается на столько же позиций правее, и попытка,
	 *          начатая за концом ряда без учёта длины участка, прошла бы мимо
	 *          позиций, ряду ещё не открытых.
	 *
	 */
	size_t prefix = 0;
	// Признак продолжения замены сопоставления продвижением позиции
	bool replacing = (verified > 0);
	/**
	 * Получаем метку отказа сопоставления, действующую в начале программы
	 *
	 * @details Отказ до первого ряда повторения отступать некуда, поэтому
	 *          он переходит к следующей позиции начала попытки.
	 *
	 */
	size_t failure = following;
	// Номер порождаемой цепочки ветвей выбора одной из них
	size_t chain = 0;
	// Смещение первого свободного места запоминания границ групп
	size_t recording = 0;
	// Заводим метку отказа сопоставления, выбором ветви отложенного
	const size_t miss = emitter.label();
	/**
	 * @brief Порождение области инструкций программы
	 *
	 * @details Порождение обходит область в порядке возрастания адресов и вызывает
	 *          себя для каждой ветви выбора: область ветви лежит внутри области,
	 *          её содержащей. Область, концом не ограниченная, есть программа целиком
	 *          и завершается инструкцией успеха.
	 *
	 * @param from начало порождаемой области инструкций
	 * @param to   конец порождаемой области инструкций
	 * @return     результат порождения области инструкций
	 *
	 */
	function <bool (const address_t, const address_t)> region;
	// Выполняем установку порождения области инструкций программы
	region = [&](const address_t from, const address_t to) noexcept -> bool {
	// Получаем адрес исполняемой инструкции программы
	address_t pc = from;
	/**
	 * Выполняем обход инструкций области программы
	 */
	while(true) {
		/**
		 * Если область инструкций исчерпана
		 */
		if((to != INVALID_ADDRESS) && (pc >= to))
			// Выводим результат порождения области инструкций
			return true;
		/**
		 * Если адрес инструкции находится за пределами программы
		 */
		if(static_cast <size_t> (pc) >= program.instructions.size())
			// Выводим отказ порождения области инструкций
			return false;
		// Получаем исполняемую инструкцию программы
		const instruction_t & instruction = program.instructions.at(static_cast <size_t> (pc));
		/**
		 * Если инструкция сохраняет позицию в ячейке захвата
		 *
		 * @details Границы совпадения целиком порождённый код ведёт сам, поэтому
		 *          сохранение их кода не порождает. Границы захватывающей группы
		 *          записываются в набор границ вызывающей стороны прямо, без
		 *          запоминания и восстановления при отступлении: отступление
		 *          возвращает исполнение к сопоставлению вслед за рядом,
		 *          отчего сохранения, размещённые после него, выполняются
		 *          заново, а размещённые прежде него отступлением не затронуты.
		 *
		 */
		if(instruction.type == opcode_t::SAVE) {
			/**
			 * Если сохранение выполняется в ячейку границ захватывающей группы
			 */
			if(instruction.save.slot > 1) {
				/**
				 * Если сопоставление символов заменено продвижением позиции
				 *
				 * @details Позиция сопоставления замену эту отстаёт, поэтому
				 *          запись её в набор границ требует продвижения прежде.
				 *
				 */
				if(elided > 0) {
					// Выполняем продвижение позиции сопоставления на заменённые символы
					emitter.add(reg_t::CURSOR, reg_t::CURSOR, static_cast <uint32_t> (elided));
					// Выполняем сброс количества заменённых символов
					elided = 0;
				}
				// Выполняем запись позиции сопоставления в набор границ
				emitter.store(reg_t::CURSOR, reg_t::BOUNDS, instruction.save.slot);
			}
			// Переходим к следующей инструкции программы
			pc++;
			// Продолжаем обход инструкций программы
			continue;
		}
		/**
		 * Если инструкция проверяет привязку к позиции в тексте
		 *
		 * @details Привязка длины не имеет, отчего замену сопоставления
		 *          продвижением позиции она не прерывает, но требует размещения
		 *          отложенного продвижения прежде себя: проверка ведётся
		 *          по положению сопоставления, а замена его отстаёт.
		 *
		 */
		if(instruction.type == opcode_t::ANCHOR) {
			/**
			 * Если сопоставление символов заменено продвижением позиции
			 */
			if(elided > 0) {
				// Выполняем продвижение позиции сопоставления на заменённые символы
				emitter.add(reg_t::CURSOR, reg_t::CURSOR, static_cast <uint32_t> (elided));
				// Выполняем сброс количества заменённых символов
				elided = 0;
			}
			// Получаем признак соответствия привязок границам строк
			const bool lines = hasFlag(instruction.flags, flag_t::MULTILINE);
			/**
			 * Определяем тип проверяемой привязки к позиции в тексте
			 */
			switch(static_cast <uint8_t> (instruction.assertion.type)) {
				/**
				 * Если проверяется привязка к началу текста
				 */
				case static_cast <uint8_t> (anchor_t::TEXT_BEGIN): {
					// Выполняем сравнение позиции сопоставления с началом текста
					emitter.compare(reg_t::CURSOR, static_cast <uint32_t> (0));
					// Выполняем переход к отказу вне начала текста
					emitter.branch(cond_t::NOTEQUAL, failure);
				} break;
				/**
				 * Если проверяется привязка к началу текущей попытки поиска
				 */
				case static_cast <uint8_t> (anchor_t::SEARCH_HEAD): {
					// Выполняем чтение позиции начала поиска совпадения из кадра
					emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (origin));
					// Выполняем сравнение позиции сопоставления с началом поиска
					emitter.compare(reg_t::CURSOR, reg_t::SCRATCH);
					// Выполняем переход к отказу вне начала поиска совпадения
					emitter.branch(cond_t::NOTEQUAL, failure);
				} break;
				/**
				 * Если проверяется привязка к концу текста
				 */
				case static_cast <uint8_t> (anchor_t::TEXT_END): {
					// Выполняем сравнение позиции сопоставления с размером текста
					emitter.compare(reg_t::CURSOR, reg_t::SIZE);
					// Выполняем переход к отказу вне конца текста
					emitter.branch(cond_t::NOTEQUAL, failure);
				} break;
				/**
				 * Если проверяется привязка к началу текста либо строки
				 */
				case static_cast <uint8_t> (anchor_t::LINE_BEGIN): {
					/**
					 * Если режим соответствия привязок границам строк не установлен
					 */
					if(!lines) {
						// Выполняем сравнение позиции сопоставления с началом текста
						emitter.compare(reg_t::CURSOR, static_cast <uint32_t> (0));
						// Выполняем переход к отказу вне начала текста
						emitter.branch(cond_t::NOTEQUAL, failure);
						// Выходим из определения типа привязки
						break;
					}
					// Заводим метку выполненной проверки привязки
					const size_t passed = emitter.label();
					// Выполняем сравнение позиции сопоставления с началом текста
					emitter.compare(reg_t::CURSOR, static_cast <uint32_t> (0));
					// Выполняем переход к выполненной проверке в начале текста
					emitter.branch(cond_t::EQUAL, passed);
					// Выполняем сравнение позиции сопоставления с размером текста
					emitter.compare(reg_t::CURSOR, reg_t::SIZE);
					/**
					 * Выполняем переход к отказу в конце текста
					 *
					 * @details Позиция за переводом строки, завершающим текст,
					 *          началом строки не является: новая строка при этом
					 *          не начинается.
					 *
					 */
					emitter.branch(cond_t::ABOVE, failure);
					// Выполняем получение положения предшествующего байта текста
					emitter.sub(reg_t::SCRATCH, reg_t::CURSOR, 1);
					// Выполняем чтение предшествующего байта текста
					emitter.load(reg_t::LETTER, reg_t::TEXT, reg_t::SCRATCH);
					// Выполняем сравнение предшествующего байта с переводом строки
					emitter.compare(reg_t::LETTER, static_cast <uint32_t> (0x0A));
					// Выполняем переход к отказу вне начала строки
					emitter.branch(cond_t::NOTEQUAL, failure);
					// Выполняем расстановку метки выполненной проверки привязки
					emitter.place(passed);
				} break;
				/**
				 * Если проверяется привязка к концу текста с переводом строки
				 */
				case static_cast <uint8_t> (anchor_t::TEXT_FINISH):
				/**
				 * Если проверяется привязка к концу текста либо строки
				 */
				case static_cast <uint8_t> (anchor_t::LINE_END): {
					// Получаем признак проверки привязки к концу строки
					const bool ending = (lines && (instruction.assertion.type == anchor_t::LINE_END));
					// Заводим метку выполненной проверки привязки
					const size_t passed = emitter.label();
					// Выполняем сравнение позиции сопоставления с размером текста
					emitter.compare(reg_t::CURSOR, reg_t::SIZE);
					// Выполняем переход к выполненной проверке в конце текста
					emitter.branch(cond_t::EQUAL, passed);
					/**
					 * Если привязка проверяется к концу текста, а не строки
					 *
					 * @details Вне режима соответствия границам строк привязке
					 *          отвечает лишь перевод строки, текст завершающий.
					 *
					 */
					if(!ending) {
						// Выполняем получение положения последнего байта текста
						emitter.sub(reg_t::SCRATCH, reg_t::SIZE, 1);
						// Выполняем сравнение позиции сопоставления с последним байтом
						emitter.compare(reg_t::CURSOR, reg_t::SCRATCH);
						// Выполняем переход к отказу вне последнего байта текста
						emitter.branch(cond_t::NOTEQUAL, failure);
					}
					// Выполняем чтение байта текста в позиции сопоставления
					emitter.load(reg_t::LETTER, reg_t::TEXT, reg_t::CURSOR);
					// Выполняем сравнение байта текста с переводом строки
					emitter.compare(reg_t::LETTER, static_cast <uint32_t> (0x0A));
					// Выполняем переход к отказу вне конца строки
					emitter.branch(cond_t::NOTEQUAL, failure);
					// Выполняем расстановку метки выполненной проверки привязки
					emitter.place(passed);
				} break;
				/**
				 * Если проверяется привязка, порождению в коде не поддающаяся
				 */
				default: {
					// Выполняем вызов подпрограммы проверки привязки к позиции
					invoke(emitter, SLOT_ASSERTING, spill, reg_t::CURSOR, this->guard(instruction));
					// Выполняем сравнение итога проверки привязки с нулём
					emitter.compare(reg_t::SCRATCH, static_cast <uint32_t> (0));
					// Выполняем переход к отказу при невыполнении привязки
					emitter.branch(cond_t::EQUAL, failure);
				}
			}
			// Переходим к следующей инструкции программы
			pc++;
			// Продолжаем обход инструкций программы
			continue;
		}
		/**
		 * Если сопоставление символа выполнено отбором позиции начала попытки
		 */
		if(replacing && (consumed < verified) && (elided < MAX_ELISION) && (instruction.type == opcode_t::CHAR) &&
		 !hasFlag(instruction.flags, flag_t::CASELESS) &&
		 (instruction.letter.code == static_cast <uint32_t> (static_cast <uint8_t> (this->_prefilter.leading.at(consumed))))) {
			// Увеличиваем количество символов ведущего литерала, замену получивших
			consumed++;
			// Увеличиваем количество символов, продвижение на какие не размещено
			elided++;
			// Увеличиваем длину участка, ряду первому предшествующего
			prefix++;
			// Переходим к следующей инструкции программы
			pc++;
			// Продолжаем обход инструкций программы
			continue;
		}
		// Выполняем прекращение замены сопоставления продвижением позиции
		replacing = false;
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
		if(singular(instruction.type)) {
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
			// Увеличиваем длину участка, ряду первому предшествующего
			prefix++;
			// Переходим к следующей инструкции программы
			pc++;
			// Продолжаем обход инструкций программы
			continue;
		}
		/**
		 * Если инструкция завершает сопоставление с успехом
		 */
		if(instruction.type == opcode_t::MATCH) {
			/**
			 * Если инструкция успеха встречена внутри ветви выбора
			 *
			 * @details Ветвь выбора завершается общим продолжением, а не успехом:
			 *          успех внутри неё означал бы устройство, разбору не поддавшееся.
			 *
			 */
			if(to != INVALID_ADDRESS)
				// Выводим отказ порождения области инструкций
				return false;
			// Выполняем переход к обнаружению совпадения в тексте
			emitter.jump(found);
			// Выводим результат порождения области инструкций
			return true;
		}
		/**
		 * Если инструкция выбирает одну из ветвей выражения
		 */
		if((instruction.type == opcode_t::SPLIT) && (instruction.split.run == INVALID_ADDRESS) &&
		 (instruction.split.lazy == INVALID_ADDRESS)) {
			// Создаём разбираемую цепочку ветвей выбора одной из них
			chain_t branching;
			/**
			 * Если разбор цепочки ветвей выбора не выполнен
			 */
			if(!chaining(program, pc, branching))
				// Выводим отказ порождения области инструкций
				return false;
			/**
			 * Если цепочка ветвей за пределы порождаемой области выходит
			 */
			if((to != INVALID_ADDRESS) && (branching.join > to))
				// Выводим отказ порождения области инструкций
				return false;
			/**
			 * Если допустимое количество цепочек ветвей исчерпано
			 */
			if(chain >= MAX_CHAINS)
				// Выводим отказ порождения области инструкций
				return false;
			// Получаем номер первого места кадра, цепочке ветвей отведённого
			const size_t slot = (runs * SLOTS) + (chain++ * PICKS);
			// Заводим метку общего продолжения цепочки ветвей выбора
			const size_t join = emitter.label();
			// Заводим метку исчерпания ветвей цепочки выбора
			const size_t drained = emitter.label();
			// Создаём набор ячеек захвата, цепочкой записываемых
			vector <uint32_t> written;
			// Выполняем сбор ячеек захвата, цепочкой записываемых
			journal(program, pc, branching.join, written);
			// Получаем смещение мест запоминания границ групп цепочки
			const size_t keeping = (records + recording);
			// Увеличиваем смещение первого свободного места запоминания
			recording += written.size();
			// Создаём набор меток входа в ветви выбора одной из них
			vector <size_t> entries;
			/**
			 * Выполняем заведение меток входа в ветви выбора
			 */
			for(size_t i = 0; i < branching.branches.size(); i++)
				// Выполняем заведение метки входа в очередную ветвь выбора
				entries.push_back(emitter.label());
			/**
			 * Выполняем сохранение позиции начала выбора в кадре вызова
			 *
			 * @details Ветви выбора сопоставляются с одной и той же позиции текста,
			 *          поэтому переход к следующей ветви её восстанавливает.
			 *
			 */
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (slot));
			/**
			 * Выполняем сохранение прежнего отказа в кадре вызова
			 *
			 * @details Отказ, действовавший до выбора, восстанавливается последней
			 *          ветвью цепочки: исчерпав ветви, сопоставление обязано
			 *          вернуться туда, куда оно вернулось бы и без выбора.
			 *
			 */
			emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
			// Выполняем сохранение прежнего отказа в месте цепочки ветвей
			emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (slot + 1));
			/**
			 * Выполняем запоминание границ групп, цепочкой записываемых
			 *
			 * @details Границы запоминаются при входе в цепочку и восстанавливаются
			 *          при переходе к ветви следующей, отчего ветвь, сопоставление
			 *          какой прервано, установленных ею границ по себе не оставляет.
			 *
			 */
			for(size_t i = 0; i < written.size(); i++) {
				// Выполняем чтение границы группы из набора границ
				emitter.fetch(reg_t::SCRATCH, reg_t::BOUNDS, written.at(i));
				// Выполняем запоминание границы группы в кадре вызова
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (keeping + i));
			}
			/**
			 * Выполняем порождение ветвей выбора одной из них
			 */
			for(size_t i = 0; i < branching.branches.size(); i++) {
				// Получаем признак порождения последней ветви выбора
				const bool last = ((i + 1) >= branching.branches.size());
				/**
				 * Если порождается ветвь, первой не являющаяся
				 */
				if(i > 0) {
					// Выполняем расстановку метки входа в очередную ветвь выбора
					emitter.place(entries.at(i));
					// Выполняем размещение метки цели перехода по адресу в регистре
					emitter.landing();
					// Выполняем восстановление позиции начала выбора из кадра вызова
					emitter.fetch(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (slot));
					/**
					 * Выполняем восстановление границ групп, ветвью прежней записанных
					 */
					for(size_t k = 0; k < written.size(); k++) {
						// Выполняем чтение запомненной границы группы из кадра вызова
						emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (keeping + k));
						// Выполняем восстановление границы группы в наборе границ
						emitter.store(reg_t::SCRATCH, reg_t::BOUNDS, written.at(k));
					}
				}
				/**
				 * Выполняем установку отказа, вслед за ветвью действующего
				 *
				 * @details Отказ, случившийся после выбора ветви, обязан вернуться
				 *          к ветви следующей, а после последней - туда, куда вёл
				 *          отказ до выбора. Какая ветвь выбрана и какие выборы
				 *          вложены в неё, известно лишь при исполнении, поэтому
				 *          действующий отказ ведётся ячейкой кадра, а не меткой.
				 *
				 */
				if(last)
					// Выполняем получение адреса исчерпания ветвей цепочки
					emitter.address(reg_t::SCRATCH, drained);
				// Выполняем получение адреса входа в ветвь следующую
				else emitter.address(reg_t::SCRATCH, entries.at(i + 1));
				// Выполняем установку действующего отказа сопоставления
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
				// Выполняем установку метки отказа сопоставления ветви выбора
				failure = miss;
				/**
				 * Если порождение области очередной ветви выбора не выполнено
				 */
				if(!region(branching.branches.at(i).first, branching.branches.at(i).second))
					// Выводим отказ порождения области инструкций
					return false;
				/**
				 * Если порождается ветвь, последней не являющаяся
				 */
				if(!last)
					// Выполняем переход к общему продолжению цепочки ветвей
					emitter.jump(join);
			}
			// Выполняем переход к общему продолжению цепочки ветвей
			emitter.jump(join);
			/**
			 * Выполняем расстановку метки исчерпания ветвей цепочки выбора
			 *
			 * @details Исчерпав ветви, цепочка восстанавливает границы групп,
			 *          последней ветвью записанные, снимает свой отказ и передаёт
			 *          сопоставление отказу, действовавшему до неё.
			 *
			 */
			emitter.place(drained);
			// Выполняем размещение метки цели перехода по адресу в регистре
			emitter.landing();
			/**
			 * Выполняем восстановление границ групп, последней ветвью записанных
			 */
			for(size_t i = 0; i < written.size(); i++) {
				// Выполняем чтение запомненной границы группы из кадра вызова
				emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (keeping + i));
				// Выполняем восстановление границы группы в наборе границ
				emitter.store(reg_t::SCRATCH, reg_t::BOUNDS, written.at(i));
			}
			// Выполняем чтение прежнего отказа из места цепочки ветвей
			emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (slot + 1));
			// Выполняем восстановление действующего отказа сопоставления
			emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
			// Выполняем переход по восстановленному отказу сопоставления
			emitter.proceed(reg_t::SCRATCH);
			// Выполняем расстановку метки общего продолжения цепочки ветвей
			emitter.place(join);
			// Выполняем установку метки отказа сопоставления вслед за выбором
			failure = miss;
			// Переходим к общему продолжению цепочки ветвей выбора
			pc = branching.join;
			// Продолжаем обход инструкций области программы
			continue;
		}
		/**
		 * Порождаем проход ленивого ряда повторения одиночного символа
		 *
		 * @details Ленивый ряд ходу навстречу жадному: сопоставление продолжается
		 *          сразу, а тело повторения поглощает по символу лишь по отказу
		 *          продолжения. Оттого прохода и отступления ленивый ряд не имеет
		 *          вовсе - имеет продвижение по отказу, устроенное тем же местом
		 *          кадра, что и отступление жадного.
		 *
		 */
		if(instruction.split.lazy != INVALID_ADDRESS) {
			// Получаем инструкцию тела ленивого повторения одиночного символа
			const instruction_t & repeated = program.instructions.at(static_cast <size_t> (instruction.split.lazy));
			// Выполняем заведение таблицы принадлежности байтов тела повторения
			const size_t number = this->table(repeated, program);
			// Заводим метку продвижения ленивого ряда по отказу продолжения
			const size_t advance = retries.at(index);
			// Заводим метку продолжения сопоставления вслед за ленивым рядом
			const size_t resume = emitter.label();
			// Выполняем сохранение положения ленивого ряда в кадре вызова
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			/**
			 * Если действующий отказ ведётся ячейкой кадра
			 *
			 * @details Исчерпав продвижение, ряд возвращает сопоставление туда,
			 *          куда оно вернулось бы и без него.
			 *
			 */
			if(cellular) {
				// Выполняем чтение прежнего отказа из кадра вызова
				emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
				// Выполняем сохранение прежнего отказа в месте ряда повторения
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> ((index * SLOTS) + 2));
				// Выполняем получение адреса продвижения ленивого ряда
				emitter.address(reg_t::SCRATCH, advance);
				// Выполняем установку действующего отказа сопоставления
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
			}
			// Выполняем переход к продолжению сопоставления вслед за рядом
			emitter.jump(resume);
			/**
			 * Выполняем расстановку метки продвижения ленивого ряда
			 *
			 * @details Продвижение поглощает один символ тела повторения и повторяет
			 *          сопоставление вслед за рядом. Символ, телу не принадлежащий,
			 *          и конец текста продвижение исчерпывают.
			 *
			 */
			emitter.place(advance);
			// Выполняем размещение метки цели перехода по адресу в регистре
			emitter.landing();
			// Выполняем чтение положения ленивого ряда из кадра вызова
			emitter.fetch(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			// Заводим метку исчерпания продвижения ленивого ряда
			const size_t drained = (cellular ? emitter.label() : failure);
			// Выполняем сравнение позиции сопоставления с размером текста
			emitter.compare(reg_t::CURSOR, reg_t::SIZE);
			// Выполняем переход к исчерпанию при достижении конца текста
			emitter.branch(cond_t::ABOVE, drained);
			// Выполняем чтение байта текста в позиции сопоставления
			emitter.load(reg_t::LETTER, reg_t::TEXT, reg_t::CURSOR);
			// Выполняем чтение адреса таблицы принадлежности байтов
			emitter.context(reg_t::SCRATCH, static_cast <uint32_t> (number));
			// Выполняем чтение принадлежности байта таблице сопоставления
			emitter.load(reg_t::SPARE, reg_t::SCRATCH, reg_t::LETTER);
			// Выполняем сравнение принадлежности байта с нулём
			emitter.compare(reg_t::SPARE, static_cast <uint32_t> (0));
			// Выполняем переход к исчерпанию при непринадлежности байта
			emitter.branch(cond_t::EQUAL, drained);
			// Переходим к следующей позиции текста сопоставления
			emitter.add(reg_t::CURSOR, reg_t::CURSOR, 1);
			// Выполняем сохранение положения ленивого ряда в кадре вызова
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			/**
			 * Если действующий отказ ведётся ячейкой кадра
			 *
			 * @details Исчерпанный ряд снимает свой отказ и передаёт сопоставление
			 *          отказу, действовавшему до него.
			 *
			 */
			if(cellular) {
				// Выполняем переход к продолжению сопоставления вслед за рядом
				emitter.jump(resume);
				// Выполняем расстановку метки исчерпания продвижения ряда
				emitter.place(drained);
				// Выполняем размещение метки цели перехода по адресу в регистре
				emitter.landing();
				// Выполняем чтение прежнего отказа из места ряда повторения
				emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> ((index * SLOTS) + 2));
				// Выполняем восстановление действующего отказа сопоставления
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
				// Выполняем переход по восстановленному отказу сопоставления
				emitter.proceed(reg_t::SCRATCH);
			}
			// Выполняем расстановку метки продолжения сопоставления вслед за рядом
			emitter.place(resume);
			// Выполняем установку метки отказа сопоставления вслед за рядом
			failure = (cellular ? miss : advance);
			// Переходим к следующему ряду повторения одиночного символа
			index++;
			// Переходим к ветви завершения ленивого повторения
			pc = instruction.split.first;
			// Продолжаем обход инструкций области программы
			continue;
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
			const instruction_t & repeated = program.instructions.at(static_cast <size_t> (instruction.split.run));
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
			/**
			 * Если действующий отказ ведётся ячейкой кадра
			 *
			 * @details Исчерпав отступление, ряд возвращает сопоставление туда,
			 *          куда оно вернулось бы и без него.
			 *
			 */
			if(cellular) {
				// Выполняем чтение прежнего отказа из кадра вызова
				emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
				// Выполняем сохранение прежнего отказа в месте ряда повторения
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> ((index * SLOTS) + 2));
			}
			/**
			 * Получаем количество значений байта, ряду не принадлежащих
			 *
			 * @details Проход ряда порождается по пределу его: ряд, всякий байт
			 *          принимающий, доходит до конца текста одним ходом, а ряд
			 *          с пределом единственным - поиском этого предела. Ряды
			 *          прочие проходятся порождённым разбором таблицы.
			 *
			 */
			size_t limits = 0;
			// Значение байта, ряд повторения ограничивающее
			uint8_t limit = 0;
			{
				// Получаем адрес таблицы принадлежности байтов тела повторения
				const uint8_t * members = (this->_members.data() + this->_offsets.at(number - SLOT_TABLES));
				/**
				 * Выполняем обход пространства значений байта
				 */
				for(size_t letter = 0; letter < TABLE; letter++) {
					/**
					 * Если значение байта ряду не принадлежит
					 */
					if(members[letter] == 0) {
						// Запоминаем значение байта, ряд ограничивающее
						limit = static_cast <uint8_t> (letter);
						// Увеличиваем количество значений байта, ряду не принадлежащих
						limits++;
					}
				}
			}
			/**
			 * Если ряд повторения принимает всякое значение байта
			 *
			 * @details Проход такого ряда упирается лишь в конец текста, отчего
			 *          и порождается установкой позиции в размер его.
			 *
			 */
			if(limits == 0) {
				// Выполняем установку позиции сопоставления в размер текста
				emitter.move(reg_t::CURSOR, reg_t::SIZE);
				// Выполняем расстановку метки завершения прохода ряда
				emitter.place(scan);
			/**
			 * Если ряд повторения ограничен единственным значением байта
			 */
			} else if(limits == 1) {
				// Выполняем заведение значения байта, ряд ограничивающего
				const size_t number = this->limiter(limit);
				// Выполняем вызов подпрограммы прохода ряда поиском предела
				invoke(emitter, SLOT_SCANNING, spill, reg_t::CURSOR, number);
				// Выполняем установку позиции завершения прохода ряда
				emitter.move(reg_t::CURSOR, reg_t::SCRATCH);
				// Выполняем расстановку метки завершения прохода ряда
				emitter.place(scan);
			/**
			 * Если ряд повторения проходится разбором таблицы принадлежности
			 */
			} else {
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
			}
			// Выполняем расстановку метки завершения прохода ряда
			emitter.place(complete);
			// Выполняем сохранение положения отступления ряда в кадре вызова
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			/**
			 * Если пропуск пройденного участка порождается первым рядом повторения
			 *
			 * @details Положение конца запоминает ряд первый: он единственный,
			 *          чей конец при начале попытки более позднем остаётся прежним.
			 *          Ряды, размещённые за ним, начинаются с положения, отступлением
			 *          ряда первого изменяемого.
			 *
			 */
			if(skipping && (index == 0)) {
				/**
				 * Выполняем получение положения пропуска пройденного участка
				 *
				 * @details Из положения конца ряда вычитается длина участка, ряду
				 *          предшествующего: попытка следующая должна начаться там,
				 *          где ряд начнётся за концом нынешнего, а не за концом его
				 *          самого.
				 *
				 */
				emitter.sub(reg_t::SPARE, reg_t::CURSOR, static_cast <uint32_t> (prefix));
				// Выполняем сохранение положения пропуска в кадре вызова
				emitter.store(reg_t::SPARE, reg_t::STACK, static_cast <uint32_t> (span));
			}
			/**
			 * Если действующий отказ ведётся ячейкой кадра
			 */
			if(cellular) {
				// Выполняем получение адреса отступления ряда повторения
				emitter.address(reg_t::SCRATCH, retries.at(index));
				// Выполняем установку действующего отказа сопоставления
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
			}
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
			// Выполняем размещение метки цели перехода по адресу в регистре
			emitter.landing();
			// Выполняем чтение положения отступления ряда из кадра вызова
			emitter.fetch(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			// Выполняем чтение положения начала ряда из кадра вызова
			emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> ((index * SLOTS) + 1));
			// Заводим метку исчерпания отступления ряда повторения
			const size_t drained = (cellular ? emitter.label() : failure);
			// Выполняем сравнение положения отступления с положением начала ряда
			emitter.compare(reg_t::CURSOR, reg_t::SCRATCH);
			// Выполняем переход к исчерпанию при достижении начала ряда
			emitter.branch(cond_t::LESS, drained);
			// Выполняем отступление на одну позицию текста
			emitter.sub(reg_t::CURSOR, reg_t::CURSOR, 1);
			// Выполняем сохранение положения отступления ряда в кадре вызова
			emitter.store(reg_t::CURSOR, reg_t::STACK, static_cast <uint32_t> (index * SLOTS));
			/**
			 * Если действующий отказ ведётся ячейкой кадра
			 *
			 * @details Исчерпанный ряд снимает свой отказ и передаёт сопоставление
			 *          отказу, действовавшему до него.
			 *
			 */
			if(cellular) {
				// Выполняем переход к продолжению сопоставления вслед за рядом
				emitter.jump(resume);
				// Выполняем расстановку метки исчерпания отступления ряда
				emitter.place(drained);
				// Выполняем размещение метки цели перехода по адресу в регистре
				emitter.landing();
				// Выполняем чтение прежнего отказа из места ряда повторения
				emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> ((index * SLOTS) + 2));
				// Выполняем восстановление действующего отказа сопоставления
				emitter.store(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
				// Выполняем переход по восстановленному отказу сопоставления
				emitter.proceed(reg_t::SCRATCH);
			}
			// Выполняем расстановку метки продолжения сопоставления вслед за рядом
			emitter.place(resume);
			// Выполняем установку метки отказа сопоставления вслед за рядом
			failure = (cellular ? miss : retries.at(index));
			// Переходим к следующему ряду повторения одиночного символа
			index++;
			// Переходим к ветви завершения повторения
			pc = instruction.split.second;
		}
	}
	};
	/**
	 * Если порождение программы целиком не выполнено
	 */
	if(!region(0, INVALID_ADDRESS)) {
		// Выполняем очистку порождённого сопоставителя
		this->clear();
		// Выводим результат порождения сопоставителя
		return false;
	}
	/**
	 * Если цепочки ветвей выбора порождены
	 */
	if(cellular) {
		// Выполняем расстановку метки отказа, выбором ветви отложенного
		emitter.place(miss);
		// Выполняем чтение действующего отказа сопоставления из кадра вызова
		emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (cell));
		// Выполняем переход по действующему отказу сопоставления
		emitter.proceed(reg_t::SCRATCH);
	}
	// Выполняем расстановку метки перехода к следующей позиции начала попытки
	emitter.place(following);
	// Выполняем размещение метки цели перехода по адресу в регистре
	emitter.landing();
	/**
	 * Если выражение привязано к позиции начала поиска совпадения
	 *
	 * @details Совпадение, начинающееся правее позиции начала поиска, привязкой
	 *          запрещено, поэтому прерванная попытка означает отсутствие совпадения
	 *          вовсе: перебор позиций начала попытки проходил бы текст впустую.
	 *
	 */
	if(program.anchored)
		// Выполняем переход к отсутствию совпадения в тексте
		emitter.jump(none);
	/**
	 * Если выражение к позиции начала поиска не привязано
	 */
	else {
		// Выполняем сравнение позиции начала попытки с размером текста
		emitter.compare(reg_t::KEEPER, reg_t::SIZE);
		// Выполняем переход к отсутствию совпадения при достижении конца текста
		emitter.branch(cond_t::ABOVE, none);
		/**
		 * Если пропуск пройденного участка при отказе попытки порождается
		 *
		 * @details Попытка следующая начинается за концом первого ряда, а не
		 *          в позиции, начало попытки нынешней сменяющей: участок,
		 *          рядом пройденный, отвергнут целиком.
		 *
		 */
		if(skipping) {
			// Выполняем чтение положения конца первого ряда из кадра вызова
			emitter.fetch(reg_t::SCRATCH, reg_t::STACK, static_cast <uint32_t> (span));
			// Переходим к позиции, за концом первого ряда стоящей
			emitter.add(reg_t::KEEPER, reg_t::SCRATCH, 1);
			// Выполняем сравнение позиции начала попытки с размером текста
			emitter.compare(reg_t::KEEPER, reg_t::SIZE);
			/**
			 * Выполняем переход к отсутствию совпадения за пределами текста
			 *
			 * @details Проверка предела, попытке предшествующая, ведётся по
			 *          позиции прежней, тогда как пропуск участка пройденного
			 *          сменяет её положением конца ряда, каковой доходит и до
			 *          конца текста. Позиция конца текста началом попытки быть
			 *          вправе - в ней возможно совпадение пустое, - а позиция,
			 *          её превысившая, выводит чтение текста за его пределы.
			 *
			 */
			emitter.branch(cond_t::GREATER, none);
		// Переходим к следующей позиции начала попытки сопоставления
		} else emitter.add(reg_t::KEEPER, reg_t::KEEPER, 1);
		// Выполняем переход к отбору позиции начала очередной попытки сопоставления
		emitter.jump(seek ? seeker : (sifting ? sifter : entry));
	}
	// Выполняем расстановку метки обнаружения совпадения в тексте
	emitter.place(found);
	// Выполняем запись начальной границы обнаруженного совпадения
	emitter.store(reg_t::KEEPER, reg_t::BOUNDS, 0);
	// Выполняем запись конечной границы обнаруженного совпадения
	emitter.store(reg_t::CURSOR, reg_t::BOUNDS, 1);
	// Выполняем размещение выхода из порождаемого сопоставителя
	emitter.epilogue(frame);
	// Выполняем установку результата обнаружения совпадения
	emitter.move(reg_t::RESULT, static_cast <uint64_t> (1));
	// Выполняем размещение завершения вызова
	emitter.ret();
	// Выполняем расстановку метки отсутствия совпадения в тексте
	emitter.place(none);
	// Выполняем размещение выхода из порождаемого сопоставителя
	emitter.epilogue(frame);
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
		// Выполняем установку адреса значения хранилища
		this->_context.at(i) = (this->_members.data() + this->_offsets.at(i - SLOT_TABLES));
	// Выполняем установку адреса предварительного отбора позиций
	this->_context.at(SLOT_PREFILTER) = &this->_prefilter;
	// Выполняем установку адреса подпрограммы отбора позиций
	this->_context.at(SLOT_SEEKING) = reinterpret_cast <const void *> (&seeking);
	// Выполняем установку адреса подпрограммы проверки возможности совпадения
	this->_context.at(SLOT_FEASIBLE) = reinterpret_cast <const void *> (&feasible);
	// Выполняем установку адреса подпрограммы проверки привязки к позиции
	this->_context.at(SLOT_ASSERTING) = reinterpret_cast <const void *> (&asserting);
	// Выполняем установку адреса подпрограммы прохода ряда повторения
	this->_context.at(SLOT_SCANNING) = reinterpret_cast <const void *> (&scanning);
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
	// Выполняем установку количества захватывающих групп выражения
	this->_captures = program.captures;
	// Выполняем установку опознания программы порождённого сопоставителя
	this->_identity = program.id;
	// Выводим результат порождения сопоставителя
	return true;
}
/**
 * @brief Опознание набора команд порождённого машинного кода
 *
 * @details Порождённый код годен лишь набору команд, для какого порождён,
 *          поэтому запись несёт его опознание. Восстановление на машине
 *          набора иного отвечает отказом, а не исполнением чужих команд.
 *
 */
static uint8_t instructionSet() noexcept {
	/**
	 * Если сборка выполняется для процессора с набором команд ARM64
	 */
	#if defined(__aarch64__) || defined(_M_ARM64)
		// Выводим опознание набора команд ARM64
		return 0x01;
	/**
	 * Если сборка выполняется для процессора с набором команд x86-64
	 */
	#elif defined(__x86_64__) || defined(_M_X64)
		// Выводим опознание набора команд x86-64
		return 0x02;
	/**
	 * Если сборка выполняется для прочих наборов команд
	 */
	#else
		// Выводим отсутствие опознания набора команд
		return 0x00;
	#endif
}
/**
 * @brief Функция записи числа переменной длины
 *
 * @param value  записываемое число
 * @param result запись порождённого сопоставителя
 *
 */
static void writeSize(uint64_t value, std::string & result) noexcept {
	/**
	 * Выполняем запись числа долями по семь разрядов
	 */
	while(value >= 0x80) {
		// Выполняем запись очередной доли числа с признаком продолжения
		result.push_back(static_cast <char> ((value & 0x7F) | 0x80));
		// Переходим к следующей доле числа
		value >>= 7;
	}
	// Выполняем запись последней доли числа
	result.push_back(static_cast <char> (value & 0x7F));
}
/**
 * @brief Функция чтения числа переменной длины
 *
 * @param data   запись порождённого сопоставителя
 * @param offset позиция чтения записи
 * @param value  прочитанное число
 * @return       результат чтения числа
 *
 */
static bool readSize(std::string_view data, size_t & offset, uint64_t & value) noexcept {
	// Выполняем сброс прочитанного числа
	value = 0;
	/**
	 * Выполняем чтение числа долями по семь разрядов
	 */
	for(uint8_t shift = 0; shift < 64; shift += 7) {
		/**
		 * Если запись оборвана до завершения числа
		 */
		if(offset >= data.size())
			// Выводим результат чтения числа
			return false;
		// Получаем очередную долю числа
		const uint8_t part = static_cast <uint8_t> (data[offset++]);
		// Выполняем добавление доли числа
		value |= (static_cast <uint64_t> (part & 0x7F) << shift);
		/**
		 * Если признак продолжения числа не установлен
		 */
		if((part & 0x80) == 0)
			// Выводим результат чтения числа
			return true;
	}
	// Выводим результат чтения числа
	return false;
}
/**
 * @brief Метод записи порождённого сопоставителя
 *
 * @param result запись порождённого сопоставителя
 * @return       результат записи порождённого сопоставителя
 *
 */
bool awh::regex::Codegen::save(string & result) const noexcept {
	/**
	 * Если порождённый сопоставитель не готов
	 */
	if(!this->ready())
		// Выводим результат записи порождённого сопоставителя
		return false;
	// Выполняем запись опознания набора команд порождённого кода
	result.push_back(static_cast <char> (instructionSet()));
	// Выполняем запись размера порождённого машинного кода
	writeSize(static_cast <uint64_t> (this->_assembly.length()), result);
	// Выполняем запись порождённого машинного кода
	result.append(reinterpret_cast <const char *> (this->_assembly.entry()), this->_assembly.length());
	// Выполняем запись размера хранилища значений обстановки
	writeSize(static_cast <uint64_t> (this->_members.size()), result);
	/**
	 * Если хранилище значений обстановки не пусто
	 */
	if(!this->_members.empty())
		// Выполняем запись хранилища значений обстановки
		result.append(reinterpret_cast <const char *> (this->_members.data()), this->_members.size());
	// Выполняем запись количества смещений значений хранилища
	writeSize(static_cast <uint64_t> (this->_offsets.size()), result);
	/**
	 * Выполняем перебор смещений значений хранилища
	 */
	for(const auto & offset : this->_offsets)
		// Выполняем запись очередного смещения значения хранилища
		writeSize(static_cast <uint64_t> (offset), result);
	// Выводим результат записи порождённого сопоставителя
	return true;
}
/**
 * @brief Метод восстановления порождённого сопоставителя
 *
 * @param data    запись порождённого сопоставителя
 * @param offset  позиция чтения записи
 * @param program программа порождённого сопоставителя
 * @return        результат восстановления сопоставителя
 *
 */
bool awh::regex::Codegen::restore(string_view data, size_t & offset, const program_t & program) noexcept {
	// Выполняем очистку порождённого сопоставителя
	this->clear();
	/**
	 * Если запись оборвана до опознания набора команд
	 */
	if(offset >= data.size())
		// Выводим результат восстановления сопоставителя
		return false;
	// Получаем опознание набора команд порождённого кода
	const uint8_t machine = static_cast <uint8_t> (data[offset++]);
	// Размер порождённого машинного кода
	uint64_t length = 0;
	/**
	 * Если чтение размера порождённого кода не выполнено
	 */
	if(!readSize(data, offset, length))
		// Выводим результат восстановления сопоставителя
		return false;
	/**
	 * Если порождённый код за пределы записи выходит
	 */
	if(length > static_cast <uint64_t> (data.size() - offset))
		// Выводим результат восстановления сопоставителя
		return false;
	// Получаем указание на начало порождённого машинного кода
	const char * code = (data.data() + offset);
	// Переходим за порождённый машинный код
	offset += static_cast <size_t> (length);
	// Размер хранилища значений обстановки исполнения
	uint64_t members = 0;
	/**
	 * Если чтение размера хранилища значений не выполнено
	 */
	if(!readSize(data, offset, members))
		// Выводим результат восстановления сопоставителя
		return false;
	/**
	 * Если хранилище значений за пределы записи выходит
	 */
	if(members > static_cast <uint64_t> (data.size() - offset))
		// Выводим результат восстановления сопоставителя
		return false;
	// Получаем указание на начало хранилища значений обстановки
	const char * storage = (data.data() + offset);
	// Переходим за хранилище значений обстановки
	offset += static_cast <size_t> (members);
	// Количество смещений значений хранилища
	uint64_t count = 0;
	/**
	 * Если чтение количества смещений не выполнено
	 */
	if(!readSize(data, offset, count))
		// Выводим результат восстановления сопоставителя
		return false;
	/**
	 * Если количество смещений превышает размер оставшейся записи
	 */
	if(count > static_cast <uint64_t> (data.size() - offset))
		// Выводим результат восстановления сопоставителя
		return false;
	// Набор смещений значений хранилища
	vector <size_t> offsets;
	// Выполняем размещение набора смещений значений хранилища
	offsets.reserve(static_cast <size_t> (count));
	/**
	 * Выполняем чтение набора смещений значений хранилища
	 */
	for(uint64_t i = 0; i < count; i++) {
		// Очередное смещение значения хранилища
		uint64_t value = 0;
		/**
		 * Если чтение смещения значения хранилища не выполнено
		 */
		if(!readSize(data, offset, value))
			// Выводим результат восстановления сопоставителя
			return false;
		/**
		 * Если смещение хранилищу значений не принадлежит
		 *
		 * @details Смещение обращается адресом, по какому порождённый код
		 *          читает таблицы и приметы, поэтому принадлежность его
		 *          хранилищу удостоверяется до всякого исполнения.
		 *
		 */
		if(value >= members)
			// Выводим результат восстановления сопоставителя
			return false;
		// Выполняем добавление смещения в набор
		offsets.push_back(static_cast <size_t> (value));
	}
	/**
	 * Если запись порождена для набора команд иного
	 *
	 * @details Отказ этот изъяном не является: вызывающая сторона порождает
	 *          сопоставитель заново, а выражение сопоставляется исполнением
	 *          программы, покуда порождение не выполнено.
	 *
	 */
	if((machine == 0x00) || (machine != instructionSet()))
		// Выводим результат восстановления сопоставителя
		return false;
	/**
	 * Если порождённый машинный код пуст
	 */
	if(length == 0)
		// Выводим результат восстановления сопоставителя
		return false;
	// Выполняем установку хранилища значений обстановки исполнения
	this->_members.assign(storage, (storage + members));
	// Выполняем установку смещений значений хранилища
	this->_offsets = ::move(offsets);
	// Выполняем установку предварительного отбора позиций
	this->_prefilter = program.prefilter;
	// Выполняем размещение набора адресов обстановки исполнения
	this->_context.assign((SLOT_TABLES + this->_offsets.size()), nullptr);
	/**
	 * Выполняем сборку набора адресов обстановки исполнения
	 */
	for(size_t i = SLOT_TABLES; i < this->_context.size(); i++)
		// Выполняем установку адреса значения хранилища
		this->_context.at(i) = (this->_members.data() + this->_offsets.at(i - SLOT_TABLES));
	// Выполняем установку адреса предварительного отбора позиций
	this->_context.at(SLOT_PREFILTER) = &this->_prefilter;
	// Выполняем установку адреса подпрограммы отбора позиций
	this->_context.at(SLOT_SEEKING) = reinterpret_cast <const void *> (&seeking);
	// Выполняем установку адреса подпрограммы проверки возможности совпадения
	this->_context.at(SLOT_FEASIBLE) = reinterpret_cast <const void *> (&feasible);
	// Выполняем установку адреса подпрограммы проверки привязки к позиции
	this->_context.at(SLOT_ASSERTING) = reinterpret_cast <const void *> (&asserting);
	// Выполняем установку адреса подпрограммы прохода ряда повторения
	this->_context.at(SLOT_SCANNING) = reinterpret_cast <const void *> (&scanning);
	/**
	 * Если размещение порождённого машинного кода не выполнено
	 */
	if(!this->_assembly.allocate(static_cast <size_t> (length)) ||
	 !this->_assembly.fill(code, static_cast <size_t> (length)) || !this->_assembly.commit()) {
		// Выполняем очистку порождённого сопоставителя
		this->clear();
		// Выводим результат восстановления сопоставителя
		return false;
	}
	// Выполняем установку вызова порождённого сопоставителя
	this->_matcher = reinterpret_cast <matcher_t> (const_cast <void *> (this->_assembly.entry()));
	// Выполняем установку количества захватывающих групп выражения
	this->_captures = program.captures;
	// Выполняем установку опознания программы порождённого сопоставителя
	this->_identity = program.id;
	// Выводим результат восстановления сопоставителя
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
	// Выполняем очистку смещений значений хранилища
	this->_offsets.clear();
	// Выполняем очистку набора адресов обстановки исполнения
	this->_context.clear();
	// Выполняем сброс количества захватывающих групп выражения
	this->_captures = 0;
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
	// Получаем требуемое количество границ обнаруженного совпадения
	const size_t count = ((static_cast <size_t> (this->_captures) + 1) * 2);
	/**
	 * Создаём набор границ обнаруженного совпадения на кадре вызова
	 *
	 * @details Набор несёт по две границы на совпадение целиком и на каждую
	 *          захватывающую группу выражения. Порождённый код записывает
	 *          границы групп прямо в набор, отчего переноса их он не требует.
	 *          Набор заполняется признаком отсутствия границы: группа, ветвью
	 *          выбора не пройденная, границ не получает вовсе, а ветвь, сопоставление
	 *          какой прервано, границы, ею записанные, восстанавливает сама.
	 *
	 *          Набор отводится на кадре вызова, а не размещением в куче:
	 *          сопоставление вызывается на каждой позиции текста, и размещение
	 *          набора обходилось дороже самого сопоставления. Замер на
	 *          девяноста тысячах вызовов давал сто сорок наносекунд на вызов
	 *          у Linux и четыреста шестьдесят у OpenBSD, распределитель какого
	 *          укреплён заполнением мусором и сторожевыми страницами, - и
	 *          выигрыш порождённого кода тонул в этом расходе целиком.
	 *
	 */
	size_t frame[BOUNDS];
	// Набор границ, размещаемый в куче при недостатке кадра вызова
	vector <size_t> spare;
	// Указание на начало набора границ обнаруженного совпадения
	size_t * bounds = frame;
	/**
	 * Если кадра вызова набору границ не достаёт
	 *
	 * @details Выражения с числом захватывающих групп свыше пятнадцати редки,
	 *          и им размещение в куче остаётся: расход его на таком выражении
	 *          теряется в самом сопоставлении.
	 *
	 */
	if(count > BOUNDS) {
		// Выполняем размещение набора границ в куче
		spare.assign(count, string_view::npos);
		// Выполняем установку указания на начало набора границ
		bounds = spare.data();
	/**
	 * Если набор границ отводится на кадре вызова
	 */
	} else {
		/**
		 * Выполняем заполнение набора признаком отсутствия границы
		 */
		for(size_t i = 0; i < count; i++)
			// Выполняем установку признака отсутствия очередной границы
			bounds[i] = string_view::npos;
	}
	// Получаем позицию начала поиска совпадения
	const size_t position = ((start > text.size()) ? text.size() : start);
	/**
	 * Если совпадение в тексте не обнаружено
	 */
	if(!this->_matcher(text.data(), text.size(), position, bounds, this->_context.data()))
		// Выводим результат поиска совпадения
		return false;
	// Выполняем размещение набора границ совпадения и захваченных групп
	captures.reserve(count / 2);
	/**
	 * Выполняем размещение границ совпадения и захваченных групп
	 */
	for(size_t i = 0; i < count; i += 2)
		// Выполняем размещение очередной пары границ
		captures.emplace_back(bounds[i], bounds[i + 1]);
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
awh::regex::Codegen::Codegen() noexcept : _captures(0), _identity(0), _matcher(nullptr) {}
