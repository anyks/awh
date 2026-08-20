/**
 * @file writer.cpp
 * @date 2026-08-12
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
 * @brief Бенчмарки записи текста настроек TOML — сборка текста из пар со значениями
 *        всех простых типов, запись строк, требующих ограждения, и расход выделений
 *        памяти на собранный файл настроек
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include "toml.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков контейнера TOML
 */
using namespace awh::benchmark::settings;

/**
 * @brief Внутренние параметры и сценарии бенчмарков записи текста настроек
 *
 */
namespace {
	/**
	 * @brief Количество собираемых файлов настроек
	 *
	 */
	static constexpr size_t ROUNDS = 4000;
	/**
	 * @brief Количество таблиц в собираемом файле настроек
	 *
	 */
	static constexpr size_t TABLES = 16;
	/**
	 * @brief Количество пар в одной таблице собираемого файла настроек
	 *
	 */
	static constexpr size_t KEYS = 16;

	/**
	 * @brief Порог пропускной способности записи текста настроек
	 *
	 * @details Порог назначен по замеру на рабочей машине 12.08.2026 с запасом
	 *          вчетверо: отладочные стенды отстают от неё вчетверо-впятеро
	 *
	 */
	static constexpr double WRITE_THRESHOLD = 23.0;
	/**
	 * @brief Порог пропускной способности записи строк, требующих ограждения
	 *
	 * @details Значение, которое разбор прочитал бы иначе, запись защищает оградой
	 *          либо управляющими последовательностями: путь этот проходится лишь
	 *          такими строками и стоит заметно дороже записи как есть
	 *
	 */
	static constexpr double WRITE_GUARDED_THRESHOLD = 17.0;
	/**
	 * @brief Порог расхода выделений памяти на запись
	 *
	 * @details Величина назначена по съёму 20.08.2026 на пяти машинах: libc++ даёт 9,
	 *          libstdc++ даёт 10. Порог взят вдвое выше худшего из снятых
	 *
	 * @warning Прежний порог был 2000 при десяти замеренных - двухсоткратный запас. Такой
	 *          порог не сторожит ничего: он пропустил бы и стократный рост расхода,
	 *          отчитываясь при том зелёной строкой. Порог, запас какого назначен догадкой
	 *          вместо замера, ошибается в обе стороны разом - и тесно, и вольно
	 *
	 */
	static constexpr double WRITE_ALLOCATIONS_THRESHOLD = 20.0;

	/**
	 * @brief Функция получения имён ключей собираемого файла настроек
	 *
	 * @note Имена собираются однократно до замера: сборка их внутри измеряемого
	 *       цикла вносила бы в замер стоимость работы со строкой
	 *
	 * @return перечень имён ключей собираемого файла настроек
	 *
	 */
	static const vector <string> & names() noexcept {
		// Перечень имён ключей собираемого файла настроек
		static const vector <string> result = []() noexcept -> vector <string> {
			// Собираемый перечень имён ключей
			vector <string> names;
			// Выполняем резервирование памяти под перечень имён
			names.reserve(KEYS > TABLES ? KEYS : TABLES);
			/**
			 * Выполняем сборку всех имён ключей
			 */
			for(size_t i = 0; i < (KEYS > TABLES ? KEYS : TABLES); i++)
				// Выполняем добавление очередного имени ключа
				names.push_back(string("key").append(to_string(i)));
			// Выводим собранный перечень имён ключей
			return names;
		}();
		// Выводим перечень имён ключей собираемого файла настроек
		return result;
	}
	/**
	 * @brief Функция сборки текста настроек
	 *
	 * @param guarded признак записи значений, требующих ограждения
	 * @return        длина собранного текста настроек
	 *
	 */
	static uint64_t write(const bool guarded) noexcept {
		// Объект записи текста настроек
		awh::codec::toml::writer_t writer;
		// Получаем перечень имён ключей собираемого файла настроек
		const vector <string> & keys = names();
		/**
		 * Выполняем сборку всех таблиц файла настроек
		 */
		for(size_t i = 0; i < TABLES; i++){
			// Выполняем запись объявления очередной таблицы
			writer.table(keys.at(i));
			/**
			 * Выполняем сборку всех пар очередной таблицы
			 */
			for(size_t j = 0; j < KEYS; j++){
				// Выполняем запись имени ключа очередной пары
				writer.key(keys.at(j));
				/**
				 * Если записываются значения, требующие ограждения
				 */
				if(guarded)
					// Выполняем запись строкового значения со знаками, требующими ограждения
					writer.text("путь\\к\"файлу\"\tи перевод\nстроки");
				/**
				 * Если записываются значения всех простых типов
				 */
				else {
					/**
					 * Выполняем выбор типа записываемого значения
					 */
					switch(j % 4){
						// Если записывается строковое значение
						case 0: writer.text("значение"); break;
						// Если записывается целое число
						case 1: writer.integer(static_cast <int64_t> (j * 1000)); break;
						// Если записывается логическое значение
						case 2: writer.boolean((j % 8) == 2); break;
						// Если записывается число с плавающей точкой
						case 3: writer.real(3.14159265358979); break;
					}
				}
			}
		}
		// Выводим длину собранного текста настроек
		return static_cast <uint64_t> (writer.text().size());
	}
	/**
	 * @brief Функция получения длины собираемого текста настроек
	 *
	 * @param guarded признак записи значений, требующих ограждения
	 * @return        длина собираемого текста настроек
	 *
	 */
	static size_t sized(const bool guarded) noexcept {
		// Выводим длину собираемого текста настроек
		return static_cast <size_t> (::write(guarded));
	}
	/**
	 * @brief Функция прогона сценария записи текста настроек
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeValues() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::sized(false), ROUNDS, []() noexcept {
			// Выполняем сборку текста настроек
			return ::write(false);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария записи значений, требующих ограждения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeGuarded() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::sized(true), ROUNDS, []() noexcept {
			// Выполняем сборку текста настроек
			return ::write(true);
		});
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария расхода выделений памяти на запись
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(::sized(false), ROUNDS, []() noexcept {
			// Выполняем сборку текста настроек
			return ::write(false);
		});
		/**
		 * Если ни одной операции не выполнено
		 *
		 * @note Показатель «на одну операцию» при нуле операций выдал бы ноль, а ноль
		 *       укладывается в любой порог с верхней границей
		 */
		if(outcome.operations == 0){
			// Запоминаем признак недействительности измерения
			result.invalid = true;
			// Запоминаем причину недействительности измерения
			result.reason = "запись не выполнила ни одной операции";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		/**
		 * Если учёт выделений памяти не работает
		 */
		if(!counted(outcome, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренное значение
		result.value = perDocument(outcome);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария записи текста настроек
	 */
	static const bool VALUES_REGISTERED = awh::benchmark::add(
		"codec/toml: запись значений", "МБ/с", WRITE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeValues
	);
	/**
	 * Выполняем регистрацию сценария записи значений, требующих ограждения
	 */
	static const bool GUARDED_REGISTERED = awh::benchmark::add(
		"codec/toml: запись ограждаемых значений", "МБ/с", WRITE_GUARDED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeGuarded
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на запись
	 */
	static const bool ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/toml: выделения на запись", "выд./файл", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
};
