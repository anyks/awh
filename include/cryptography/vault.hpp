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

#ifndef __AWH_VAULT__
#define __AWH_VAULT__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include "crypto.hpp"
#include "../alloc/keeper.hpp"

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
					 * @brief Метод получения содержимого тайны
					 *
					 * @return содержимое тайны
					 *
					 * \~english
					 * @brief Method of getting the secret content
					 *
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
			// Сведения о защите, состоявшейся у укрытой памяти склада
			awh::alloc::shelter_t _shelter;
		private:
			// Объект фреймворка
			[[maybe_unused]] const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
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
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Vault(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Vault() noexcept;
	} vault_t;
};

#endif // __AWH_VAULT__
