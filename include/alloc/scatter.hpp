/**
 * @file scatter.hpp
 * @date 2026-09-15
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
 * \~russian
 * @brief Заголовочный файл рассеяния ключевого материала по образу
 *
 * @section scatter_decisions Намеренные решения
 *
 * @details <b>Задача.</b> Зашитый в образ ключ находят НЕ разбором кода, а поиском
 *          участка высокой неопределённости: тридцать два случайных байта видны в
 *          образе пятном и находятся сканером за секунды, без понимания логики. Оттого
 *          материал в образе не лежит готовым и не лежит кучкой - он рассеян по носителю
 *          и выводится из него на запуске.
 *
 *          <b>Рамка, которую нельзя терять.</b> Приложение выводит ключ САМО, без
 *          человека, - значит всё нужное лежит внутри, и у хозяина машины оно есть.
 *          Покупается стоимость поиска, а не невозможность: рассеяние закрывает дорогу
 *          сканеру пятна и разбору по частям, но не закрывает дорогу тому, кто снимет
 *          собранный материал точкой останова на входе в вывод ключа. Против последнего
 *          средства здесь нет и не обещано.
 *
 *          <b>Две стороны одного договора считают ОДНУ раскладку из ОДНОГО зерна.</b>
 *          Раскладчик (`lay`) работает при сборке на закрытой машине: секрет и зерно на
 *          входе, носитель на выходе. Сборщик (`gather`) работает на запуске в образе:
 *          носитель и то же зерно на входе, собранный материал в приёмнике тайн на
 *          выходе. Обе стороны выводят перестановку мест и гамму из зерна одним и тем же
 *          воспроизводимым потоком, оттого поток обязан давать один ряд на всякой
 *          системе - он и даёт: `splitmix64` есть чистая арифметика над
 *          шестидесятичетырёхразрядным словом, без обращения к платформе.
 *
 *          <b>Гамма наложением обязательна, а не украшение.</b> Ляг в носитель открытый
 *          байт секрета, пусть и рассеянный, - места секрета выдали бы себя разбором по
 *          частям: тридцать два особых байта среди рабочих данных. Оттого в носитель
 *          ложится `секрет XOR гамма`, а гамма выводится из зерна, какого в образе нет
 *          вовсе. Это и значит «ключ не хранится, а выводится»: в образе лежит наложение,
 *          снять его нечем, не имея зерна.
 *
 *          <b>Носитель равномерен по всей длине.</b> Места, не занятые секретом,
 *          заполняются байтами того же потока, а не нулём и не случаем со стороны. Так
 *          места секрета неотличимы от заполняющих по распределению, и пятна в образе
 *          нет вовсе. Заполни мы прочее нулём - секрет проступил бы островками среди
 *          пустоты; возьми случай со стороны - раскладка перестала бы быть
 *          воспроизводимой, а воспроизводимость нужна: одно зерно даёт одну сборку.
 *
 *          <b>Ёмкость носителя объявляет раскладчик, и она НЕ меньше длины секрета.</b>
 *          Запас сверх длины - место, куда лечь заполняющим байтам и куда рассеять
 *          секрет пошире. Чем шире носитель, тем реже места секрета, тем дороже их
 *          собрать, не зная зерна, - но тем толще образ. Мера отдана зовущему.
 *
 *          <b>Собранное ложится в приёмник тайн ПОБАЙТНО, минуя кучу.</b> Один временный
 *          сбор на стеке или в куче обнулил бы весь труд: собранный материал лежал бы
 *          открытым в незащищённой памяти. Сборщик льёт байт за байтом прямо в
 *          `vessel_t`, и промежуточного сбора не возникает вовсе.
 *
 *          <b>Привязка к контрольной сумме кода ведётся якорем.</b> `anchor` подмешивает
 *          в зерно контрольную сумму собственного кода и версию ключа: патч образа
 *          меняет сумму, а с ней зерно и весь поток, и секрет собирается неверным -
 *          защита неотделима от расшифровки. Механизм здесь; саму сумму и участок образа,
 *          по какому она берётся, подаёт зовущий - участок у macOS и MS Windows подпись
 *          правит ПОСЛЕ сборки, и выбирать его надо со стендами каждой системы.
 *
 *          <b>Чего рассеяние НЕ делает.</b> Оно не выводит ключ из собранного материала -
 *          вывод (KDF, растяжение пароля) есть договор криптографической библиотеки, и
 *          рассеяние оставляет ему собранный материал в приёмнике тайн, не более.
 *
 * \~english
 * @brief Header file of scattering the key material across the image
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "vessel.hpp"
#include "../sys/macro/global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Namespace of the memory allocator
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс рассеяния ключевого материала по образу
		 *
		 * \~english
		 * @brief Class of scattering the key material across the image
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Scatter {
			public:
				/**
				 * \~russian
				 * @brief Метод подмешивания якоря в зерно раскладки
				 *
				 * @note Привязывает раскладку к якорю - контрольной сумме собственного кода
				 *       и версии ключа. Обе стороны договора зовут его ОДИНАКОВО до `lay` и
				 *       `gather`, подавая одно зерно, один якорь, одну версию: иначе поток
				 *       разойдётся и секрет не соберётся
				 *
				 * @note Патч образа - снятие проверки, вставка точки останова - меняет
				 *       контрольную сумму, а с ней зерно и весь поток: секрет собирается
				 *       неверным. Защита становится неотделима от расшифровки - не
				 *       «приложение обнаружило взлом», а «контейнер не открылся»
				 *
				 * @note Саму контрольную сумму и участок образа, по какому она берётся,
				 *       подаёт зовущий: участок у macOS и MS Windows подпись правит ПОСЛЕ
				 *       сборки, и выбирать его надо со стендами каждой системы
				 *
				 * @param seed     зерно раскладки из закрытого контура
				 * @param checksum контрольная сумма кода либо иной якорь, либо нуль
				 * @param version  версия ключа, разводящая контуры сборок
				 * @return          зерно, привязанное к якорю и версии
				 *
				 * \~english
				 * @brief Method of blending an anchor into the layout seed
				 *
				 * @param seed     layout seed from the closed loop
				 * @param checksum code checksum or another anchor, or zero
				 * @param version  key version separating build loops
				 * @return          seed bound to the anchor and the version
				 *
				 * \~
				 */
				static uint64_t anchor(const uint64_t seed, const uint64_t checksum, const uint64_t version = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод раскладки секрета по носителю (время сборки)
				 *
				 * @note Работает при сборке на закрытой машине: секрет наружу образа не
				 *       уходит, в образ уходит лишь носитель. В приложении не зовётся
				 *
				 * @note Ёмкость носителя обязана быть НЕ меньше длины секрета: иначе
				 *       части секрета некуда лечь, и метод отвечает пустым носителем
				 *
				 * @param secret   адрес секрета
				 * @param length   длина секрета в байтах
				 * @param seed      зерно раскладки из закрытого контура
				 * @param capacity  ёмкость носителя в байтах, не меньше длины секрета
				 * @return           носитель с рассеянным секретом либо пустой при отказе
				 *
				 * \~english
				 * @brief Method of laying the secret out across the carrier (build time)
				 *
				 * @param secret   address of the secret
				 * @param length   length of the secret in bytes
				 * @param seed      layout seed from the closed loop
				 * @param capacity  capacity of the carrier in bytes, not less than the length
				 * @return           carrier with the scattered secret, or empty on refusal
				 *
				 * \~
				 */
				static std::vector <uint8_t> lay(const uint8_t * secret, const size_t length, const uint64_t seed, const size_t capacity) noexcept;
				/**
				 * \~russian
				 * @brief Метод вплетения секрета в предоставленную рабочую таблицу (время сборки)
				 *
				 * @note В отличие от `lay`, носитель СВОЙ не заводит и прочих мест не
				 *       заполняет: на местах секрета правит поданную таблицу, прочие её места
				 *       - настоящие рабочие данные - оставляет нетронутыми. Так секрет
				 *       сливается с осмысленными данными, а не образует свой участок высокой
				 *       неопределённости, какой сканер нашёл бы целиком
				 *
				 * @note Таблица обязана годиться носителем, и это ответственность зовущего:
				 *       её правка на местах секрета допустима (места отданы под секрет и
				 *       рабочего значения там больше нет), а собственная неопределённость её
				 *       достаточна, чтобы вкрапления секрета в ней не выделялись. Таблица из
				 *       нулей либо связного текста носителем не годится
				 *
				 * @param table  адрес рабочей таблицы, правимой на местах секрета
				 * @param size   размер рабочей таблицы в байтах, не меньше длины секрета
				 * @param secret адрес секрета
				 * @param length длина секрета в байтах
				 * @param seed   зерно раскладки из закрытого контура
				 * @return        признак выполнения операции
				 *
				 * \~english
				 * @brief Method of weaving the secret into a provided working table (build time)
				 *
				 * @param table  address of the working table edited at the secret places
				 * @param size   size of the working table in bytes, not less than the length
				 * @param secret address of the secret
				 * @param length length of the secret in bytes
				 * @param seed   layout seed from the closed loop
				 * @return        flag of the operation performed
				 *
				 * \~
				 */
				static bool weave(uint8_t * table, const size_t size, const uint8_t * secret, const size_t length, const uint64_t seed) noexcept;
				/**
				 * \~russian
				 * @brief Метод порождения исходного текста с носителем (время сборки)
				 *
				 * @note Работает при сборке: отдаёт готовый к включению текст на C++ с
				 *       носителем в виде массива. Ни секрета, ни зерна текст не несёт -
				 *       только носитель, длину секрета и ёмкость
				 *
				 * @param carrier носитель, полученный от `lay`
				 * @param length  длина секрета в байтах
				 * @param name    имя порождаемого массива носителя
				 * @return         исходный текст на C++ либо пустая строка при отказе
				 *
				 * \~english
				 * @brief Method of emitting the source text with the carrier (build time)
				 *
				 * @param carrier carrier obtained from `lay`
				 * @param length  length of the secret in bytes
				 * @param name    name of the emitted carrier array
				 * @return         C++ source text, or an empty string on refusal
				 *
				 * \~
				 */
				static std::string emit(const std::vector <uint8_t> & carrier, const size_t length, const std::string & name) noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки секрета из носителя в приёмник тайн (запуск)
				 *
				 * @note Работает на запуске в образе: снимает наложение и льёт секрет
				 *       ПОБАЙТНО прямо в приёмник, минуя кучу. Промежуточного сбора нет
				 *
				 * @note Зерно обязано совпадать с тем, каким велась раскладка: чужое
				 *       зерно соберёт из носителя мусор, а не секрет
				 *
				 * @note Приёмник заводится ЗДЕСЬ на длину секрета: прежнее его содержимое
				 *       снимается
				 *
				 * @param carrier  адрес носителя в образе
				 * @param capacity ёмкость носителя в байтах
				 * @param seed     то же зерно, каким велась раскладка
				 * @param length   длина секрета в байтах
				 * @param vessel   приёмник, куда ложится собранный секрет
				 * @return          признак выполнения операции
				 *
				 * \~english
				 * @brief Method of gathering the secret from the carrier into the vessel (run time)
				 *
				 * @param carrier  address of the carrier in the image
				 * @param capacity capacity of the carrier in bytes
				 * @param seed     the same seed the layout was made with
				 * @param length   length of the secret in bytes
				 * @param vessel   vessel the gathered secret is put into
				 * @return          flag of the operation performed
				 *
				 * \~
				 */
				static bool gather(const uint8_t * carrier, const size_t capacity, const uint64_t seed, const size_t length, vessel_t & vessel) noexcept;
		} scatter_t;
	};
};
