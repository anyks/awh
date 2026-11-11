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
 * @section vault_decisions Deliberate decisions
 *
 * @details <b>The COLD is encrypted, and not every issue.</b> To encrypt a live memory is impossible
 *         at all: a processor reads it open, and a key lies in the same address
 *         space. An encryption protects not from a reading of the memory of a working program,
 *         but from the SNAPSHOTS - a swap, a snapshot at a fall, a sleep. Therefore that is encrypted,
 *         what lies idle: the keys, the passwords, the recognisers.
 *         
 *         <b>A contract is a handle: to take for a time and to return.</b> An open text lives
 *         only while a handle is whole, and it lies in a sheltered memory - not going into
 *         a swap and wiped at a return. A return of a handle wipes an open text
 *         immediately, and not someday: to hold a handle longer than a work with a secret - means
 *         to give away all, for the sake of which a vault is started.
 *         
 *         <b>A price is known beforehand.</b> An issue of a memory costs about five nanoseconds, and
 *         an AES with a processor support goes about a gigabyte per second per core - this is
 *         about a microsecond per four kilobytes, that is a two hundred times dearer.
 *         To be such a property of every issue it can not, and a vault is started by a request.
 *         
 *         <b>A key of a vault is not issued outward.</b> It is taken random at a starting
 *         of a vault, lives in a sheltered memory and dies together with a vault. A password,
 *         set by a person, is here none on purpose: a vault outlives only a work of a program, and to keep it
 *         between the startings is by nothing.
 *         
 *         <b>A ciphertext lies in an ordinary memory.</b> To shelter it is no need - it is
 *         that very thing, which can be shown. A sheltered memory is dear by pages, and to spend it on
 *         what is protected by itself is wasteful.
 * \~
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
 * \~russian
 * @brief Пространство имён фреймворка
 *
 * \~english
 * @brief Namespace of the framework
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён стандартной библиотеки
	 *
	 * \~english
	 * @brief Namespace of the standard library
	 *
	 * \~
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс склада тайн
	 *
	 * \~english
	 * @brief Secret vault class
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Vault {
		public:
			// Тип укрытого буфера: содержимое его не уходит в подкачку и затирается при возврате
			typedef vector <char, alloc::Keeper <char>> buffer_t;
		public:
			/**
			 * \~russian
			 * @brief Вид многопоточности склада
			 *
			 * @details Замок на горячем пути стоит дорого, и выбор потому отдан задаче:
			 *          склад, поделённый между потоками, ведёт замок, а свой у каждого
			 *          потока - не ведёт, потому что делить ему не с кем.
			 *
			 * \~english
			 * @brief Threading kind of the vault
			 *
			 * @details A lock on a hot path costs dear, and a choice is therefore given to a task: a vault,
			 *         shared between the threads, leads a lock, and a local to each thread - does not lead,
			 *         because it has nobody to share with.
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
			 * @note An open text lives only while a handle is whole: a return of it wipes
			 *      a content immediately
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ Handle {
				private:
					// Признак удавшегося взятия
					bool _valid;
				private:
					// Открытый текст тайны
					buffer_t _plain;
				public:
					/**
					 * \~russian
					 * @brief Шаблон метода обращения к содержимому тайны
					 *
					 * @tparam T тип обработчика содержимого тайны
					 *
					 * \~english
					 * @brief Template of a method of accessing the secret content
					 *
					 * @tparam T type of a handler of the secret content
					 *
					 * \~
					 */
					template <typename T>
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
					 * @warning Пометки `noexcept` здесь НЕТ намеренно
					 *
					 *          Обработчик волен уйти исключением, и пометка обращала бы такой уход
					 *          в аварийный останов процесса ПРЕЖДЕ раскрутки - то есть отменяла бы
					 *          ровно то, ради чего затирание и запечатывание ведутся деструктором.
					 *          В `alloc::vessel_t` разряд этот пойман набором: с пометкой
					 *          проверка валилась с `terminating due to uncaught exception`.
					 *
					 *          Уклад AWH ставит `noexcept` почти везде, и оттого разряд опасен:
					 *          здесь пометка выглядела бы соблюдением стиля, а была бы обещанием
					 *          за чужой код, какого дающий дать не может
					 *
					 * @param callback обработчик содержимого, зовомый парой «адрес, длина»
					 * @return         признак состоявшегося обращения
					 *
					 * \~english
					 * @brief Method of accessing the secret content
					 *
					 * @note A content is issued ONLY for a time of a call of a handler and is not carried
					 *      outward. So a border of a contract stops being a place, where
					 *      an open text gives itself away: a pointer crossing it
					 *      is taken by a stop point at an entrance, and no protection of a storage
					 *      of it does not cancel
					 *
					 * @note A handler has NO right to remember a submitted address: it is live only
					 *      while a handle is whole, and at a starting of a closing of the pages an address
					 *      by it beyond a call would crash a program
					 *
					 * @note A leaving from a handler by an exception is LAWFUL: a handle stays
					 *      whole, and that very exception goes outward untouched
					 *
					 * @warning There is NO `noexcept` mark here on purpose
					 *         
					 *         A handler is free to leave by an exception, and a mark would turn such a leaving
					 *         into an emergency stop of a process BEFORE a unwinding - that is would cancel
					 *         that very thing, for the sake of which a wiping and a sealing are led by a destructor.
					 *         In `alloc::vessel_t` this kind was caught by a suite: with a mark
					 *         a check fell down with `terminating due to uncaught exception`.
					 *         
					 *         A layout of AWH puts `noexcept` almost everywhere, and therefore this kind is dangerous:
					 *         here a mark would look as an observance of a style, while it would be a promise
					 *         for an alien code, which the one giving is not able to give
					 *
					 * @param callback a handler of a content, called by a pair `address, length`
					 * @return         a sign of a taken place accessing
					 *
					 * \~
					 */
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
						callback((this->_plain.empty() ? nullptr : this->_plain.data()), this->_plain.size());
						// Выводим признак состоявшегося обращения
						return true;
					}
					/**
					 * \~russian
					 * @brief Метод получения размера тайны
					 *
					 * @return размер тайны в байтах
					 *
					 * \~english
					 * @brief Method of getting the secret size
					 *
					 * @return a size of a secret in bytes
					 *
					 * \~
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
					 * @note An empty secret is a lawful content, and to judge on a taking by a
					 *      size is impossible
					 *
					 * @return a sign of a successful taking
					 *
					 * \~
					 */
					bool valid() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор копирования
					 *
					 * \~english
					 * @brief Operator of a copying
					 *
					 * \~
					 */
					Handle & operator = (const Handle &) = delete;
					/**
					 * \~russian
					 * @brief Оператор переноса
					 *
					 * @param handle переносимая рукоять
					 * @return       текущая рукоять
					 *
					 * \~english
					 * @brief Operator of a moving
					 *
					 * @param handle moved handle
					 * @return       current handle
					 *
					 * \~
					 */
					Handle & operator = (Handle && handle) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Handle() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор копирования
					 *
					 * \~english
					 * @brief Constructor of a copying
					 *
					 * \~
					 */
					Handle(const Handle &) = delete;
					/**
					 * \~russian
					 * @brief Конструктор переноса
					 *
					 * @param handle переносимая рукоять
					 *
					 * \~english
					 * @brief Constructor of a moving
					 *
					 * @param handle moved handle
					 *
					 * \~
					 */
					Handle(Handle && handle) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param plain открытый текст тайны
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param plain an open text of a secret
					 *
					 * \~
					 */
					explicit Handle(buffer_t && plain) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Handle() noexcept;
			};
		private:
			// Шифротекст тайн склада
			map <string, vector <char>> _secrets;
		private:
			// Средство шифрования
			Crypto _crypto;
		private:
			// Признак заведённого склада
			bool _ready;
			// Вид многопоточности склада
			threading_t _threading;
			// Строгость склада
			alloc::secrecy_t _secrecy;
			/**
			 * \~russian
			 * @brief Замок раскрытия тайн склада
			 *
			 * @note Ведётся он лишь у склада, поделённого между потоками, и стережёт
			 *       СОГЛАСОВАННОСТЬ СОДЕРЖИМОГО, а не страницы: доступность области
			 *       считает сам распределитель (`allocator::shield` считает держателей
			 *       и запирает область на нуле), и гонка «закрыл под ногами второго»
			 *       снимается там, а не здесь
			 *
			 * \~english
			 * @brief A lock of an opening of the secrets of a vault
			 *
			 * @note It is led only by a vault shared between the threads, and it guards
			 *      A CONSISTENCE OF A CONTENT, and not the pages: an availability of a region
			 *      counts a allocator itself (`allocator::shield` counts the holders
			 *      and locks a region at a zero), and a race `closed under the feet of the second`
			 *      is removed there, and not here
			 *
			 * \~
			 */
			mutable std::mutex _mutex;
			// Сведения о защите, состоявшейся у укрытой памяти склада
			alloc::shelter_t _shelter;
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
			 * @note A vault is not started, when there is nowhere to take a random key: to work
			 *      it then refuses wholly, and not encrypts by whatever it has. A readiness
			 *      of an encryption means itself is nowhere to ask - a method of it is closed, -
			 *      and a sign here speaks only about that a key of a vault was taken
			 *
			 * @return a sign of a prepared vault
			 *
			 * \~
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
			 * @note A promises of the systems are different, and to ask is due to the one who starts a vault:
			 *      a sheltering from a snapshot of a memory is none at Linux, macOS and NetBSD, and a
			 *      right to a forbid of a swap is limited by a bound and at illumos requires
			 *      the rights. An answer speaks about that what took place IN DEED, and not about that, of what
			 *      they were asked - a silent lowering of a protection is here none, but to learn
			 *      about it the one calling can only by asking
			 *
			 * @warning An encryption of the secrets will take place and without any protection of a memory: these signs
			 *         speak nothing about `ready`, and a vault with an empty protection
			 *         works the same, only promising less
			 *
			 * @return information about the achieved protection
			 *
			 * \~
			 */
			const alloc::shelter_t & shelter() const noexcept;
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
			 * @param name a name of a secret
			 * @param data a content of a secret
			 * @param size a size of a content
			 * @return     a sign of a successful laying
			 *
			 * \~
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
			 * @note This move is started for the sake of that a content did NOT pass through
			 *      an intermediate vessel of a language: every reallocation of `std::string`
			 *      leaves in a heap a copy of a former content, which nobody wipes.
			 *      An input of a password from a terminal and an assembling of a key from a scattered material go
			 *      byte by byte right into a vessel, and a vault takes a content from its protected
			 *      area
			 *
			 * @note A vessel after a laying is NOT wiped by itself: the one who started it disposes of it,
			 *      and to hold a secret longer than a need or to give it away again is his decision
			 *
			 * @param name   a name of a secret
			 * @param vessel a vessel with a content of a secret
			 * @return       a sign of a successful laying
			 *
			 * \~
			 */
			bool store(const std::string & name, alloc::vessel_t & vessel) noexcept;
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
			 * @note An open text lives only while a handle is whole
			 *
			 * @param name a name of a secret
			 * @return     a handle of a secret
			 *
			 * \~
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
			 * @param name a name of a secret
			 * @return     a sign of a removed secret
			 *
			 * \~
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
			 * @note To show a ciphertext is safe by a definition - exactly for this sake it is
			 *      started. A method is needed so that a promise of a vault WAS CHECKED, and not
			 *      taken on faith: an open text in a ciphertext must be none
			 *
			 * @param name   a name of a secret
			 * @param cipher a buffer, where a ciphertext lays
			 * @return       a sign of a taken ciphertext
			 *
			 * \~
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
			 * @param name a name of a secret
			 * @return     a sign of a presence of a secret
			 *
			 * \~
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
			 * @return a number of the secrets on a vault
			 *
			 * \~
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
			 * @note This vault leads a lock and is shared between the threads. It is started at a
			 *      first accessing and lives up to an end of a work of a program: its key is
			 *      random at a starting, and the secrets can not outlive a starting anew
			 *
			 * @warning A strictness of a common vault is a strict, and on the systems, where a forbid of a swap
			 *         is not given, its `ready()` answers a lie. To ask
			 *         this sign is due to the one who uses a vault
			 *
			 * @return a vault shared across a process
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
			 * @note This vault does not lead a lock: it has nobody to share with. A memory is taken by this
			 *      by a number of the threads, having addressed to it, - a price for a removal of a lock from a
			 *      hot path
			 *
			 * @note The secrets, laid by one thread, to another are NOT seen at all: these are different
			 *      vaults with different keys, and not one vault under different locks
			 *
			 * @return a vault local to each thread
			 *
			 * \~
			 */
			static Vault & local() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 * \~english
			 * @brief Operator of a copying
			 *
			 * \~
			 */
			Vault & operator = (const Vault &) = delete;
		public:
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 * \~english
			 * @brief Constructor of a copying
			 *
			 * \~
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
			 * @note A vault is started by a STRICT default: not having managed to forbid a swap of its
			 *      memory, it answers by a refusal, and not works silently weaker than promised.
			 *      A relaxation is asked explicitly - a setting, lowering a protection, must not
			 *      be turned on by itself, otherwise once they will build without it and will not notice
			 *
			 * @note A decisive sign is taken from that what took place IN DEED (`shelter`),
			 *      and not from that, of what they were asked. A strictness is judged by a forbid of a swap:
			 *      a sheltering from a snapshot of a memory is given not everywhere, and to require it would mean
			 *      to close a vault on the systems, where a protection is only lower, and not absent
			 *
			 * @note A lock a vault leads BY A DEFAULT, by that same argument, that and a strictness: a setting,
			 *      lowering a reliability, must not be turned on by itself. A refusal
			 *      of a lock is asked explicitly - `threading_t::LOCAL`, - and asks it the one,
			 *      who knows, that his vault is limited by one thread
			 *
			 * @param secrecy   a strictness of a vault
			 * @param threading a threading kind of a vault
			 *
			 * \~
			 */
			explicit Vault(const alloc::secrecy_t secrecy = alloc::secrecy_t::STRICT, const threading_t threading = threading_t::SHARED) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Vault() noexcept;
	} vault_t;
};
