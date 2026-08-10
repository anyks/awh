/**
 * @file: ini.cpp
 * @date: 2026-08-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация общего окружения бенчмарков контейнера INI — сведения о прогоне,
 *        извлечение показателей и сборка эталонных текстов настроек всех путей разбора
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков контейнера INI
 */
#include "ini.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние параметры сборки эталонных текстов настроек
 *
 */
namespace {
	/**
	 * @brief Размер эталонного крупного файла настроек в октетах
	 *
	 * @details Размер выбран заведомо превосходящим кэш последнего уровня: разбор
	 *          текста, целиком укладывающегося в кэш, показывает скорость работы с
	 *          кэшем, а не установившуюся пропускную способность
	 *
	 */
	static constexpr size_t LARGE_SIZE = (16 * 1024 * 1024);
	/**
	 * @brief Размер эталонных текстов с преобладанием одного вида записи в октетах
	 *
	 */
	static constexpr size_t FOCUSED_SIZE = (4 * 1024 * 1024);
	/**
	 * @brief Количество свойств в одном разделе крупного файла настроек
	 *
	 */
	static constexpr size_t SECTION_KEYS = 32;

	/**
	 * @brief Функция получения десятичной записи числа
	 *
	 * @note Запись выполняется средствами стандартной библиотеки намеренно: эталонные
	 *       тексты собираются однократно до замера, и стоимость их сборки в замер не
	 *       входит
	 *
	 * @param value записываемое число
	 * @return      десятичная запись числа
	 *
	 */
	static string number(const uint32_t value) noexcept {
		// Выводим десятичную запись числа
		return to_string(value);
	}
};

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
string awh::benchmark::config::details(const outcome_t & output) noexcept {
	// Собираемые сведения о прогоне
	string result;
	// Выполняем добавление количества разобранных файлов настроек
	result.append(::number(static_cast <uint32_t> (output.operations))).append(" файл., ");
	// Выполняем добавление количества выделений памяти на один файл настроек
	result.append(::number(static_cast <uint32_t> (perDocument(output) + 0.5))).append(" выд./файл, ");
	// Выполняем добавление объёма выделенной памяти на один файл настроек
	result.append(::number(static_cast <uint32_t> (output.operations > 0 ? (output.allocated / output.operations) : 0))).append(" окт./файл");
	// Выводим собранные сведения о прогоне
	return result;
}
/**
 * @brief Функция извлечения пропускной способности разбора
 *
 * @param output итоги прогона сценария
 * @return       пропускная способность в мегабайтах в секунду
 *
 */
double awh::benchmark::config::perSecond(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.seconds <= 0.0)
		// Выводим нулевую пропускную способность
		return 0.0;
	// Выводим пропускную способность разбора
	return ((static_cast <double> (output.bytes) / (1024.0 * 1024.0)) / output.seconds);
}
/**
 * @brief Функция извлечения количества выделений памяти на один файл
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на один файл настроек
 *
 */
double awh::benchmark::config::perDocument(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевое количество выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на один файл настроек
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}
/**
 * @brief Функция извлечения задержки обработки одного файла настроек
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одного файла в микросекундах
 *
 */
double awh::benchmark::config::perLatency(const outcome_t & output) noexcept {
	/**
	 * Если замер не состоялся
	 */
	if(output.operations == 0)
		// Выводим нулевую задержку обработки файла настроек
		return 0.0;
	// Выводим задержку обработки одного файла настроек
	return ((output.seconds * 1000000.0) / static_cast <double> (output.operations));
}
/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::config::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим ссылку на контрольную сумму прогонов
	return result;
}
/**
 * @brief Функция получения эталонного файла настроек приложения
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::config::service() noexcept {
	// Эталонный текст настроек
	static const string result =
		"; настройки службы, правились вручную\n"
		"\n"
		"[server]\n"
		"host = 127.0.0.1\n"
		"port = 8080\n"
		"workers = 4\n"
		"backlog = 512\n"
		"timeout = 30\n"
		"\n"
		"; пути размещения\n"
		"[paths]\n"
		"root = /opt/awh\n"
		"logs = /var/log/awh\n"
		"cache = /var/cache/awh\n"
		"pidfile = /var/run/awh.pid\n"
		"\n"
		"[logging]\n"
		"level = debug\n"
		"rotate = daily\n"
		"keep = 14\n"
		"\n"
		"[security]\n"
		"tls = on\n"
		"certificate = /etc/awh/server.pem\n"
		"ciphers = ECDHE-ECDSA-AES256-GCM-SHA384\n";
	// Выводим эталонный текст настроек
	return result;
}
/**
 * @brief Функция получения эталонного файла настроек по образцу Git
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::config::repository() noexcept {
	// Эталонный текст настроек
	static const string result =
		"[core]\n"
		"\trepositoryformatversion = 0\n"
		"\tfilemode = true\n"
		"\tbare = false\n"
		"\tlogallrefupdates = true\n"
		"\teditor = \"vim -f\" ; правится вручную\n"
		"[remote \"origin\"]\n"
		"\turl = git@example.com:anyks/awh.git\n"
		"\tfetch = +refs/heads/*:refs/remotes/origin/*\n"
		"[branch \"main\"]\n"
		"\tremote = origin\n"
		"\tmerge = refs/heads/main\n"
		"[branch \"v5\"]\n"
		"\tremote = origin\n"
		"\tmerge = refs/heads/v5\n"
		"[alias]\n"
		"\tlg = log --graph --pretty=format:\"%h\\t%s\"\n"
		"[user]\n"
		"\tname = ANYKS\n"
		"\temail = info@anyks.com\n";
	// Выводим эталонный текст настроек
	return result;
}
/**
 * @brief Функция получения эталонного крупного файла настроек
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::config::large() noexcept {
	// Собираемый эталонный текст настроек
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string result;
		// Выполняем упреждающее выделение памяти под собираемый текст
		result.reserve(LARGE_SIZE + 1024);
		// Порядковый номер собираемого раздела
		uint32_t index = 0;
		/**
		 * Выполняем сборку разделов до достижения требуемого размера
		 */
		while(result.size() < LARGE_SIZE){
			// Выполняем запись объявления очередного раздела
			result.append("[section").append(::number(index++)).append("]\n");
			/**
			 * Выполняем запись свойств очередного раздела
			 */
			for(size_t i = 0; i < SECTION_KEYS; i++)
				// Выполняем запись очередного свойства раздела
				result.append("key").append(::number(static_cast <uint32_t> (i)))
				      .append(" = значение свойства номер ").append(::number(static_cast <uint32_t> (i))).append("\n");
			// Выполняем запись пустой строки за разделом
			result.append("\n");
		}
		// Выводим собранный текст настроек
		return result;
	}();
	// Выводим эталонный текст настроек
	return result;
}
/**
 * @brief Функция получения эталонного текста с преобладанием примечаний
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::config::annotated() noexcept {
	// Собираемый эталонный текст настроек
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string result("[annotated]\n");
		// Выполняем упреждающее выделение памяти под собираемый текст
		result.reserve(FOCUSED_SIZE + 1024);
		// Порядковый номер собираемой записи
		uint32_t index = 0;
		/**
		 * Выполняем сборку записей до достижения требуемого размера
		 */
		while(result.size() < FOCUSED_SIZE){
			// Выполняем запись примечания отдельной строкой
			result.append("; примечание к свойству номер ").append(::number(index)).append("\n");
			// Выполняем запись примечания отдельной строкой
			result.append("# и второе примечание к нему же\n");
			// Выполняем запись свойства со значением
			result.append("key").append(::number(index++)).append(" = значение\n");
		}
		// Выводим собранный текст настроек
		return result;
	}();
	// Выводим эталонный текст настроек
	return result;
}
/**
 * @brief Функция получения эталонного текста со множеством разделов
 *
 * @return эталонный текст настроек
 *
 */
const string & awh::benchmark::config::sections() noexcept {
	// Собираемый эталонный текст настроек
	static const string result = []() noexcept -> string {
		// Собираемый текст настроек
		string result;
		// Выполняем упреждающее выделение памяти под собираемый текст
		result.reserve(FOCUSED_SIZE + 1024);
		// Порядковый номер собираемого раздела
		uint32_t index = 0;
		/**
		 * Выполняем сборку разделов до достижения требуемого размера
		 *
		 * @note У каждого раздела единственное свойство намеренно: сценарий
		 *       измеряет стоимость заведения раздела и поиска его среди прочих,
		 *       а не стоимость разбора свойств
		 */
		while(result.size() < FOCUSED_SIZE){
			// Получаем десятичную запись порядкового номера раздела
			const string number = ::number(index++);
			// Выполняем запись объявления очередного раздела
			result.append("[раздел.").append(number).append("]\n");
			// Выполняем запись единственного свойства раздела
			result.append("key = ").append(number).append("\n");
		}
		// Выводим собранный текст настроек
		return result;
	}();
	// Выводим эталонный текст настроек
	return result;
}
