/**
 * @file: scenarios.hpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общие данные стендов сравнения контейнера HTTP-заголовков
 *
 * @details Образцы полей и количества повторений вынесены сюда, чтобы у всех
 *          сравниваемых реализаций они были буквально одни и те же. Стенд,
 *          заводящий их у себя, рано или поздно разойдётся с соседним - и
 *          сравнение начнёт мерить разницу образцов, а не реализаций
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_HEADERS_SCENARIOS__
#define __AWH_BENCHMARK_HEADERS_SCENARIOS__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>

/**
 * @brief Инкапсулируем общие данные стендов в пространство имён
 *
 */
namespace scenarios {
	/**
	 * @brief Количество повторений сценариев наполнения и сборки сообщения
	 *
	 * @details Одна операция обходится в сотни наносекунд, и на меньшем количестве
	 *          повторений замер измерял бы разрешение часов
	 *
	 */
	static constexpr size_t ROUNDS = 200000;
	/**
	 * @brief Количество повторений сценариев поиска и замены
	 *
	 * @details Обе операции на порядок дешевле наполнения: на общем количестве
	 *          повторений показатель определялся бы накладными расходами самого
	 *          цикла замера, а не измеряемой операцией
	 *
	 */
	static constexpr size_t LIGHT_ROUNDS = 1000000;
	/**
	 * @brief Количество повторений прогрева сценариев
	 *
	 * @details Первый прогон после сборки систематически ниже остальных:
	 *          распределитель памяти выходит на рабочий объём, а предсказатель
	 *          переходов - на установившийся режим
	 *
	 */
	static constexpr size_t WARMUP = 10000;
	/**
	 * @brief Количество полей образца
	 *
	 */
	static constexpr size_t FIELDS = 10;

	/**
	 * @brief Названия полей образца обычного запроса клиента
	 *
	 * @details Набор снят с обычного запроса обозревателя: десять полей, два из них
	 *          одноимённых. Кратность в образце намеренная - контейнер обязан её
	 *          сохранять, и реализация, хранящая поля картой, теряет на ней либо
	 *          значение, либо порядок
	 *
	 */
	static constexpr const char * NAMES[FIELDS] = {
		"Host", "User-Agent", "Accept", "Accept-Encoding", "Accept-Language",
		"Connection", "Cache-Control", "Set-Cookie", "Set-Cookie", "Content-Length"
	};
	/**
	 * @brief Значения полей образца обычного запроса клиента
	 *
	 */
	static constexpr const char * VALUES[FIELDS] = {
		"example.com",
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36",
		"text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8",
		"gzip, deflate, br",
		"ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7",
		"keep-alive",
		"no-cache",
		"session=abcdef0123456789; Path=/; HttpOnly",
		"theme=dark; Path=/; Max-Age=31536000",
		"1024"
	};
	/**
	 * @brief Название поля, отыскиваемого сценарием поиска
	 *
	 * @details Поля выбраны из середины и конца набора: поиск по ним проходит почти
	 *          весь набор, и реализация с линейным перебором показывает здесь свой
	 *          худший случай, а не удачное попадание в первое же поле.
	 *
	 *          Отыскиваемое название чередуется по кругу, а не задано одно: с одним
	 *          названием оптимизатор вправе вычислить поиск однажды и умножить итог
	 *          на число повторений, и замер мерил бы пустой цикл. Регистр названий
	 *          разный - поиск обязан быть регистронезависимым у всех сравниваемых
	 *
	 */
	static constexpr const char * LOOKUPS[2] = {"cache-control", "ACCEPT-LANGUAGE"};
	/**
	 * @brief Название поля, заменяемого сценарием замены
	 *
	 */
	static constexpr const char * REPLACE = "Content-Length";
	/**
	 * @brief Значения, подставляемые сценарием замены по кругу
	 *
	 * @details Значения разной длины: замена на строку той же длины измеряла бы
	 *          запись в готовый буфер, а не работу контейнера
	 *
	 */
	static constexpr const char * REPLACEMENTS[2] = {"4096", "131072"};
};

#endif // __AWH_BENCHMARK_HEADERS_SCENARIOS__
