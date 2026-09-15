/**
 * @file vault.hpp
 * @date 2026-08-22
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
 * @brief Заголовочный файл склада тайн — хранения ключей и паролей в шифрованном виде
 *
 * @section vault_decisions Намеренные решения
 *
 * @details <b>Шифруется ХОЛОДНОЕ, а не всякая выдача.</b> Живую память шифровать нельзя
 *          вовсе: процессор читает её открытой, а ключ лежит в том же адресном
 *          пространстве. Шифрование защищает не от чтения памяти работающей программы,
 *          а от СНИМКОВ - подкачки, снимка при падении, спячки. Оттого шифруется то,
 *          что лежит без дела: ключи, пароли, опознаватели.
 *
 *          <b>Договор - рукоять: взять на время и вернуть.</b> Открытый текст живёт
 *          лишь пока рукоять цела, и лежит он в укрытой памяти - не уходящей в
 *          подкачку и затираемой при возврате. Возврат рукояти затирает открытый текст
 *          немедля, а не когда-нибудь: держать рукоять дольше работы с тайной - значит
 *          отдать всё, ради чего склад заведён.
 *
 *          <b>Цена известна заранее.</b> Выдача памяти стоит около пяти наносекунд, а
 *          AES с поддержкой процессора идёт около гигабайта в секунду на ядро - это
 *          примерно микросекунда на четыре килобайта, то есть в двести раз дороже.
 *          Свойством всякой выдачи такое быть не может, и склад заводится по просьбе.
 *
 *          <b>Ключ склада наружу не выдаётся.</b> Он берётся случайным при заведении
 *          склада, живёт в укрытой памяти и умирает вместе со складом. Пароля,
 *          заданного человеком, здесь нет намеренно: склад переживает лишь работу
 *          программы, и хранить его между запусками нечем.
 *
 *          <b>Шифротекст лежит в обычной памяти.</b> Укрывать его незачем - он и есть
 *          то, что можно показать. Укрытая память дорога страницами, и тратить её на
 *          то, что защищено само собой, расточительно.
 *
 * \~english
 * @brief Header file of the secret vault — storing keys and passwords encrypted
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <mutex>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include "crypto.hpp"
#include "../alloc/keeper.hpp"
#include "../alloc/vessel.hpp"

/**
 * @brief Пространство имён фреймворка
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён стандартной библиотеки
	 *
	 */
	using namespace std;
	/**
	 * \~russian
	 * @brief Класс склада тайн
	 *
	 * \~english
	 * @brief Secret vault class
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Vault {
		public:
			// Тип укрытого буфера: содержимое его не уходит в подкачку и затирается при возврате
			typedef std::vector <char, awh::alloc::Keeper <char>> buffer_t;
		public:
			/**
			 * \~russian
			 * @brief Вид многопоточности склада
			 *
			 * @details Замок на горячем пути стоит дорого, и выбор потому отдан задаче:
			 *          склад, поделённый между потоками, ведёт замок, а свой у каждого
			 *          потока - не ведёт, потому что делить ему не с кем
			 *
			 * \~english
			 * @brief Threading kind of the vault
			 *
			 * \~
			 */
			enum class threading_t : uint8_t {
				SHARED = 0x00, // Склад делится между потоками: раскрытие ведётся под замком
				LOCAL  = 0x01  // Склад принадлежит одному потоку: замок не ведётся
			};
		public:
			/**
			 * \~russian
			 * @brief Класс рукояти тайны
			 *
			 * @note Открытый текст живёт лишь пока рукоять цела: возврат её затирает
			 *       содержимое немедля
			 *
			 * \~english
			 * @brief Secret handle class
			 *
			 */
			class __AWH_SHARED_EXPORT__ Handle {
				private:
					// Открытый текст тайны
					buffer_t _plain;
					// Признак удавшегося взятия
					bool _valid;
				public:
					/**
					 * \~russian
					 * @brief Метод обращения к содержимому тайны
					 *
					 * @note Содержимое выдаётся ЛИШЬ на время вызова обработчика и наружу
					 *       не выносится. Так граница договора перестаёт быть местом, где
					 *       открытый текст сам себя отдаёт: указатель, пересекающий её,
					 *       берётся точкой останова на входе, и никакая защита хранения
					 *       того не отменяет
					 *
					 * @note Обработчик НЕ вправе запоминать поданный адрес: жив он лишь
					 *       пока цела рукоять, а с заведением закрытия страниц обращение
					 *       по нему за пределами вызова повалит программу
					 *
					 * @note Уход из обработчика исключением ЗАКОНЕН: рукоять остаётся
					 *       целой, а само исключение уходит наружу нетронутым
					 *
					 * @param callback обработчик содержимого, зовомый парой «адрес, длина»
					 * @return         признак состоявшегося обращения
					 *
					 * \~english
					 * @brief Method of accessing the secret content
					 *
					 * \~
					 */
					/**
					 * Пометки `noexcept` здесь НЕТ намеренно
					 *
					 * Обработчик волен уйти исключением, и пометка обращала бы такой уход
					 * в аварийный останов процесса ПРЕЖДЕ раскрутки - то есть отменяла бы
					 * ровно то, ради чего затирание и запечатывание ведутся деструктором.
					 * В `awh::alloc::vessel_t` разряд этот пойман набором: с пометкой
					 * проверка валилась с `terminating due to uncaught exception`.
					 *
					 * Уклад AWH ставит `noexcept` почти везде, и оттого разряд опасен:
					 * здесь пометка выглядела бы соблюдением стиля, а была бы обещанием
					 * за чужой код, какого дающий дать не может
					 */
					template <typename T>
					bool apply(T callback) const {
						// Если взятие тайны не удалось
						if(!this->_valid)
							// Выводим признак несостоявшегося обращения
							return false;
						/**
						 * Обработчику подаётся пара «адрес, длина»
						 *
						 * Пустая тайна - законное содержимое, и адрес при ней негоден:
						 * судить о взятии по длине нельзя, на то заведён `valid()`
						 */
						callback(
							(this->_plain.empty() ? nullptr : this->_plain.data()),
							this->_plain.size()
						);
						// Выводим признак состоявшегося обращения
						return true;
					}
					/**
					 * \~russian
					 * @brief Метод получения содержимого тайны
					 *
					 * @deprecated Ход этот УСТАРЕЛ и оставлен ради потребителей, написанных
					 * прежде заведения обращения через обработчик. Зови `apply()`: указатель,
					 * вынесенный за границу договора, отменяет защиту хранения, а с заведением
					 * закрытия страниц (`awh::alloc::Allocator::shield`) обращение по нему за
					 * пределами вызова повалит программу
					 *
					 * @return содержимое тайны
					 *
					 * \~english
					 * @brief Method of getting the secret content
					 * @deprecated This access is DEPRECATED and is left for the sake of the consumers written
					 * before the introduction of the access through a handler. Call `apply()`
					 *
					 * \~
					 */
					const char * data() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения размера тайны
					 *
					 * @return размер тайны в байтах
					 *
					 * \~english
					 * @brief Method of getting the secret size
					 *
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения признака удавшегося взятия
					 *
					 * @note Пустая тайна - законное содержимое, и судить о взятии по
					 *       размеру нельзя
					 *
					 * @return признак удавшегося взятия
					 *
					 * \~english
					 * @brief Method of getting the sign of the successful borrowing
					 *
					 */
					bool valid() const noexcept;
				public:
					/**
					 * @brief Оператор копирования
					 *
					 */
					Handle & operator = (const Handle &) = delete;
					/**
					 * @brief Оператор переноса
					 *
					 * @param handle переносимая рукоять
					 * @return       текущая рукоять
					 *
					 */
					Handle & operator = (Handle && handle) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					Handle() noexcept;
					/**
					 * @brief Конструктор копирования
					 *
					 */
					Handle(const Handle &) = delete;
					/**
					 * @brief Конструктор переноса
					 *
					 * @param handle переносимая рукоять
					 *
					 */
					Handle(Handle && handle) noexcept;
					/**
					 * @brief Конструктор
					 *
					 * @param plain открытый текст тайны
					 *
					 */
					Handle(buffer_t && plain) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Handle() noexcept;
			};
		private:
			// Шифротекст тайн склада
			std::map <std::string, std::vector <char>> _secrets;
		private:
			// Средство шифрования
			awh::Crypto _crypto;
		private:
			// Признак заведённого склада
			bool _ready;
			// Вид многопоточности склада
			threading_t _threading;
			// Строгость склада
			awh::alloc::secrecy_t _secrecy;
			/**
			 * Замок раскрытия тайн склада
			 *
			 * @note Ведётся он лишь у склада, поделённого между потоками, и стережёт
			 *       СОГЛАСОВАННОСТЬ СОДЕРЖИМОГО, а не страницы: доступность области
			 *       считает сам распределитель (`Allocator::shield` считает держателей
			 *       и запирает область на нуле), и гонка «закрыл под ногами второго»
			 *       снимается там, а не здесь
			 */
			mutable std::mutex _mutex;
			// Сведения о защите, состоявшейся у укрытой памяти склада
			awh::alloc::shelter_t _shelter;
		private:
			// Объект фреймворка
		public:
			/**
			 * \~russian
			 * @brief Метод получения признака заведённого склада
			 *
			 * @note Склад не заводится, когда случайного ключа взять неоткуда: работать
			 *       он тогда отказывается целиком, а не шифрует чем придётся. Готовность
			 *       самого средства шифрования спрашивать неоткуда - метод его закрыт, -
			 *       и признак здесь говорит лишь о том, что ключ склада взят
			 *
			 * @return признак заведённого склада
			 *
			 * \~english
			 * @brief Method of getting the sign of the prepared vault
			 *
			 */
			bool ready() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения сведений о состоявшейся защите памяти склада
			 *
			 * @note Обещания у систем разные, и спрашивать положено тому, кто склад
			 *       заводит: укрытия от снимка памяти нет у Linux, macOS и NetBSD, а
			 *       право на запрет подкачки ограничено пределом и у illumos требует
			 *       прав. Ответ говорит о том, что состоялось НА ДЕЛЕ, а не о том, о
			 *       чём просили, - молчаливого понижения защиты здесь нет, но узнать
			 *       о нём звавший может лишь спросив
			 *
			 * @warning Шифрование тайн состоится и без всякой защиты памяти: признаки
			 *          эти о `ready` не говорят ничего, и склад с пустой защитой
			 *          работает так же, лишь обещая меньше
			 *
			 * @return сведения о состоявшейся защите
			 *
			 * \~english
			 * @brief Method of getting the information about the achieved protection of the vault memory
			 *
			 * @return information about the achieved protection
			 *
			 */
			const awh::alloc::shelter_t & shelter() const noexcept;
			/**
			 * \~russian
			 * @brief Метод укладки тайны на склад
			 *
			 * @param name название тайны
			 * @param data содержимое тайны
			 * @param size размер содержимого
			 * @return     признак удавшейся укладки
			 *
			 * \~english
			 * @brief Method of storing a secret in the vault
			 *
			 */
			bool store(const std::string & name, const void * data, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод укладки тайны на склад из приёмника тайн
			 *
			 * @note Ход этот заведён ради того, чтобы содержимое НЕ проходило через
			 *       промежуточное вместилище языка: всякое перевыделение `std::string`
			 *       оставляет в куче копию прежнего содержимого, какую никто не затирает.
			 *       Ввод пароля с терминала и сборка ключа из рассеянного материала идут
			 *       побайтно прямо в приёмник, а склад берёт содержимое из его защищённой
			 *       области
			 *
			 * @note Приёмник после укладки НЕ затирается сам: распоряжается им заведший,
			 *       и держать тайну дольше надобности либо отдать её вновь - его решение
			 *
			 * @param name   название тайны
			 * @param vessel приёмник с содержимым тайны
			 * @return       признак удавшейся укладки
			 *
			 * \~english
			 * @brief Method of storing a secret in the vault out of a secret vessel
			 *
			 * \~
			 */
			bool store(const std::string & name, awh::alloc::vessel_t & vessel) noexcept;
			/**
			 * \~russian
			 * @brief Метод взятия тайны со склада
			 *
			 * @note Открытый текст живёт лишь пока рукоять цела
			 *
			 * @param name название тайны
			 * @return     рукоять тайны
			 *
			 * \~english
			 * @brief Method of borrowing a secret from the vault
			 *
			 */
			Handle borrow(const std::string & name) noexcept;
			/**
			 * \~russian
			 * @brief Метод снятия тайны со склада
			 *
			 * @param name название тайны
			 * @return     признак снятой тайны
			 *
			 * \~english
			 * @brief Method of removing a secret from the vault
			 *
			 */
			bool erase(const std::string & name) noexcept;
			/**
			 * \~russian
			 * @brief Метод снятия шифротекста тайны
			 *
			 * @note Показывать шифротекст безопасно по определению - ровно ради этого он
			 *       и заведён. Метод нужен, чтобы обещание склада ПРОВЕРЯЛОСЬ, а не
			 *       принималось на веру: открытого текста в шифротексте быть не должно
			 *
			 * @param name   название тайны
			 * @param cipher буфер, куда ложится шифротекст
			 * @return       признак снятого шифротекста
			 *
			 * \~english
			 * @brief Method of getting the ciphertext of a secret
			 *
			 */
			bool sealed(const std::string & name, std::vector <char> & cipher) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки наличия тайны на складе
			 *
			 * @param name название тайны
			 * @return     признак наличия тайны
			 *
			 * \~english
			 * @brief Method of checking the presence of a secret
			 *
			 */
			bool has(const std::string & name) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения числа тайн на складе
			 *
			 * @return число тайн на складе
			 *
			 * \~english
			 * @brief Method of getting the number of the stored secrets
			 *
			 */
			size_t count() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения склада, общего на процесс
			 *
			 * @note Склад этот ведёт замок и делится между потоками. Заводится он при
			 *       первом обращении и живёт до конца работы программы: ключ его
			 *       случаен при заведении, и пережить заведение заново тайны не могут
			 *
			 * @warning Строгость общего склада - строгая, и на системах, где запрет
			 *          подкачки не даётся, `ready()` его отвечает ложью. Спрашивать
			 *          признак этот обязан тот, кто складом пользуется
			 *
			 * @return склад, общий на процесс
			 *
			 * \~english
			 * @brief Method of getting the vault shared across the process
			 *
			 * \~
			 */
			static Vault & shared() noexcept;
			/**
			 * \~russian
			 * @brief Метод получения склада, своего у каждого потока
			 *
			 * @note Склад этот замка не ведёт: делить его не с кем. Память тем взята по
			 *       числу потоков, обратившихся к нему, - плата за снятие замка с
			 *       горячего пути
			 *
			 * @note Тайны, уложенные одним потоком, другому НЕ видны вовсе: это разные
			 *       склады с разными ключами, а не один склад под разными замками
			 *
			 * @return склад, свой у каждого потока
			 *
			 * \~english
			 * @brief Method of getting the vault local to each thread
			 *
			 * \~
			 */
			static Vault & local() noexcept;
		public:
			/**
			 * @brief Оператор копирования
			 *
			 */
			Vault & operator = (const Vault &) = delete;
		public:
			/**
			 * @brief Конструктор копирования
			 *
			 */
			Vault(const Vault &) = delete;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @note Склад заводится СТРОГИМ умолчанием: не сумев запретить подкачку своей
			 *       памяти, он отвечает отказом, а не работает молча слабее обещанного.
			 *       Послабление просится явно - настройка, снижающая защиту, включаться
			 *       сама не должна, иначе однажды соберут без неё и не заметят
			 *
			 * @note Решающий признак берётся из того, что состоялось НА ДЕЛЕ (`shelter`),
			 *       а не из того, о чём просили. Строгость судится по запрету подкачки:
			 *       укрытие от снимка памяти даётся не всюду, и требовать его значило бы
			 *       закрыть склад на системах, где защита лишь ниже, а не отсутствует
			 *
			 * @note Замок склад ведёт УМОЛЧАНИЕМ, по тому же доводу, что и строгость:
			 *       настройка, снижающая надёжность, включаться сама не должна. Отказ
			 *       от замка просится явно - `threading_t::LOCAL`, - и просит его тот,
			 *       кто знает, что склад его одним потоком и ограничен
			 *
			 * @param secrecy   строгость склада
			 * @param threading вид многопоточности склада
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Vault(const awh::alloc::secrecy_t secrecy = awh::alloc::secrecy_t::STRICT, const threading_t threading = threading_t::SHARED) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Vault() noexcept;
	} vault_t;
};
