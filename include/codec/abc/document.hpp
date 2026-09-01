/**
 * @file document.hpp
 * @date 2026-08-18
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
 * @brief Заголовочный файл дерева документа бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the tree of a document of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_DOCUMENT__
#define __AWH_CODEC_ABC_DOCUMENT__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"
#include "reader.hpp"
#include "writer.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 */
#include "../../sys/macro_push.hpp"

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
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён контейнеров данных
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён бинарного контейнера ABC
		 *
		 * \~english
		 * @brief ABC binary container namespace
		 *
		 * \~
		 */
		namespace abc {
			/**
			 * \~russian
			 * @brief Класс дерева документа
			 *
			 * @details Дерево лежит одним вместилищем узлов подряд: дети стоят сразу за
			 * родителем, указаний на них нет вовсе. Переход к следующему соседу стоит
			 * сложения, и пропуск вложенного построения целиком - тоже, ибо узел-вместимое
			 * несёт размах своего поддерева
			 *
			 * @details Содержимое всех строк, двоичных значений и имён полей лежит в одном
			 * хранилище: имя не выделяет памяти вовсе, и весь документ обходится двумя
			 * выделениями вместо выделения на всякий узел
			 *
			 * @details **Намеренные решения.** Перечисленное ниже выбрано осознанно, и
			 * возвращаться к этим вопросам при разборе кода не следует:
			 *
			 * @li **Дерево документа правки не имеет.** Разбор укладывает дерево, сборка
			 * выводит из него запись, а правится содержимое на стороне владеющего значения.
			 * Правка дерева завела бы второй путь укладки записи - мимо строгого вида, порога
			 * укладки содержимого ссылкой, порога объявления размаха вместимого и подбора
			 * метода сжатия: две дороги укладывать одно и то же, и честна перед настройками
			 * сборки лишь одна. Тем ABC и расходится с текстовыми кодеками, где дерево есть
			 * то, что правят, а текст - то, что выводят: у бинарного контейнера первична
			 * запись, а дерево из неё выведено
			 *
			 * @note Перенос владеющего значения в дерево этим не отменён: `Value::graft` его
			 * ведёт, но стоит он полного круга через октеты и правкою на месте не является.
			 * Прямая же дорога такова: собрать значение вызовами `builder_t`, уложить его в
			 * запись через `compose` либо `dump`, внести запись в контейнер через
			 * `editor_t::append` с закреплением `commit`, а разобранное поднять обратно в
			 * значение через `Value(const Document::value_t &)`
			 *
			 * @warning **Замком работа НЕ защищена: один объект — один поток.** Замок держит
			 * лишь `Editor` — ему он нужен ради фиксации по сроку своим потоком, — и
			 * равняться по нему нельзя. Замер 25.08.2026, один `Fetcher` на четыре потока:
			 * тринадцать донесений TSan и девятнадцать неверно прочитанных записей из
			 * четырёхсот, молча. Свой объект у всякого потока над ОБЩИМ источником чтения:
			 * ноль донесений, ноль расхождений — источник читается, а не правится, и делится
			 * свободно
			 *
			 * \~english
			 * @brief Class of the tree of a document
			 * @details The tree lies as one container of the nodes in a row: the children stand right after
			 * the parent, there are no pointers to them at all. The transition to the next neighbour costs
			 * an addition, and the skipping of a nested construction as a whole costs it too, for a node-container
			 * carries the extent of its own subtree
			 * @details The content of all the strings, binary values and names of the fields lies in one
			 * storage: a name does not allocate any memory at all, and the whole document makes do with two
			 * allocations instead of an allocation for every node
			 *
			 * \~
			 * @warning **The work is NOT protected by a lock: one object — one thread.** Only `Editor`
			 * holds a lock, and one must not judge the others by it. A measurement of 25.08.2026, one `Fetcher`
			 * on four threads: thirteen reports of TSan and nineteen records of four hundred read wrongly,
			 * silently. An own object per thread over a SHARED source of the reading: zero reports,
			 * zero divergences
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					/**
					 * \~russian
					 * @brief Узел дерева документа
					 *
					 * @details Узел занимает двадцать октетов. Число кладётся в поле
					 * содержимого переносом октетов, а не отдельным полем шириною в восемь
					 * октетов: поле такое потребовало бы выравнивания, и узел вырос бы ещё
					 *
					 * @details **Имя поля отображения есть отдельный узел, стоящий перед
					 * значением.** У текстовых кодеков имя лежит полем узла, ибо именем стоит
					 * лишь строка; здесь же именем вправе стоять значение любого вида, кроме
					 * вместимого, и полем узла его было бы не уложить. Оттого отображение из
					 * `N` пар несёт `2N` детей, а признак `keyed` отличает имя от значения
					 *
					 * \~english
					 * @brief Node of the tree of a document
					 * @details A node occupies twenty octets. A number is placed into the field
					 * of the content by a transfer of the octets rather than by a separate field eight
					 * octets wide: such a field would demand an alignment, and the node would grow further
					 *
					 * \~
					 */
					typedef struct Node {
						// Вид значения узла документа
						type_t type;
						/**
						 * \~russian
						 * Признак того, что узел является именем поля отображения
						 *
						 * @note Признак этот необходим: имя стоит тем же узлом, что и значение,
						 * и без него первое значение отображения было бы неотличимо от имени
						 *
						 * \~english
						 * Flag that the node is the name of a field of a mapping
						 * @note This flag is necessary: a name stands as the same node as a value,
						 * and without it the first value of a mapping would be indistinguishable from a name
						 *
						 * \~
						 */
						bool keyed;
						// Признак того, что величина числа меньше нуля
						bool negative;
						/**
						 * \~russian
						 * Признак того, что поддерево узла ещё не развёрнуто
						 *
						 * @note Задел под отложенный разбор: узел-вместимое вправе хранить
						 * лишь отрезок записи своего поддерева, а дети его заводятся при
						 * первом обращении. Обход дерева обязан спрашивать признак этот
						 * прежде обращения к детям
						 *
						 * \~english
						 * Flag that the subtree of the node is not yet expanded
						 * @note Groundwork for the deferred parsing: a node-container has the right to hold
						 * only the segment of the record of its subtree, while its children are created at the
						 * first access. The traversal of the tree must ask this flag before an access to the children
						 *
						 * \~
						 */
						bool pending;
						// Смещение содержимого значения в хранилище октетов
						uint32_t offset;
						/**
						 * \~russian
						 * Содержимое узла шириною в восемь октетов
						 *
						 * @details Восемь этих октетов служат узлу по-разному, смотря по виду
						 * значения его. У вместимого это пара чисел: количество детей и размах
						 * поддерева. У строки и у двоичных данных - длина содержимого. У числа
						 * родного вида - само число, готовое к выдаче. У числа неограниченной
						 * ширины - длина октетов величины его
						 *
						 * \~english
						 * Content of the node eight octets wide
						 * @details These eight octets serve the node differently, depending on the kind
						 * of its value. For a container it is a pair of numbers: the number of the children and the extent
						 * of the subtree. For a string and for binary data it is the length of the content. For a number
						 * of a native kind it is the number itself ready for the issuance. For a number of an unlimited
						 * width it is the length of the octets of its magnitude
						 *
						 * \~
						 */
						uint32_t content[2];
						/**
						 * \~russian
						 * @brief Метод получения количества детей вместимого либо длины содержимого
						 *
						 * @return количество детей вместимого либо длина содержимого в октетах
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the children of a container or of the length of the content
						 * @return number of the children of a container or the length of the content in octets
						 *
						 * \~
						 */
						AWH_ABC_INLINE bool container() const noexcept {
							// Выводим признак того, что узел является вместимым
							return ((static_cast <uint32_t> (this->type) & static_cast <uint32_t> (type_t::CONTAINER)) != 0);
						}
						/**
						 * \~russian
						 * @brief Метод получения количества детей вместимого либо длины содержимого
						 *
						 * @return количество детей вместимого либо длина содержимого в октетах
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the children of a container or of the length of the content
						 * @return number of the children of a container or the length of the content in octets
						 *
						 * \~
						 */
						AWH_ABC_INLINE uint32_t length() const noexcept {
							// Выводим количество детей вместимого либо длину содержимого
							return this->content[0];
						}
						/**
						 * \~russian
						 * @brief Метод получения размаха поддерева узла
						 *
						 * @return размах поддерева узла в узлах, считая сам узел
						 *
						 * \~english
						 * @brief Method of the obtaining of the extent of the subtree of a node
						 * @return extent of the subtree of the node in the nodes counting the node itself
						 *
						 * \~
						 */
						AWH_ABC_INLINE uint32_t extent() const noexcept {
							/**
							 * Если узел вместимым не является, размах его равен одному узлу.
							 *
							 * Хранить размах у одиночного узла нельзя: поле содержимого занято
							 * у него самим числом, и запись размаха затёрла бы старшую половину
							 * его разрядов. Оттого размах одиночного узла подразумевается, а
							 * не хранится
							 */
							if(!this->container())
								// Выводим размах поддерева одиночного узла
								return 1;
							// Выводим размах поддерева вместимого
							return this->content[1];
						}
						/**
						 * \~russian
						 * @brief Метод установки количества детей вместимого либо длины содержимого
						 *
						 * @param value устанавливаемое значение
						 *
						 * \~english
						 * @brief Method of the setting of the number of the children of a container or of the length of the content
						 * @param value value being set
						 *
						 * \~
						 */
						AWH_ABC_INLINE void length(const uint32_t value) noexcept {
							// Выполняем установку количества детей вместимого либо длины содержимого
							this->content[0] = value;
						}
						/**
						 * \~russian
						 * @brief Метод установки размаха поддерева узла
						 *
						 * @param value устанавливаемое значение
						 *
						 * \~english
						 * @brief Method of the setting of the extent of the subtree of a node
						 * @param value value being set
						 *
						 * \~
						 */
						AWH_ABC_INLINE void extent(const uint32_t value) noexcept {
							// Выполняем установку размаха поддерева узла
							this->content[1] = value;
						}
						/**
						 * \~russian
						 * @brief Метод проверки того, что узел является вместимым
						 *
						 * @return признак того, что узел является вместимым
						 *
						 * \~english
						 * @brief Method of the checking that a node is a container
						 * @return sign that the node is a container
						 *
						 * \~
						 */
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						Node() noexcept :
						 type(type_t::UNDEFINED), keyed(false), negative(false),
						 pending(false), offset(0), content{0, 0} {}
					} node_t;
					/**
					 * \~russian
					 * @brief Сторож ширины узла дерева документа
					 *
					 * @details Ширина эта объявлена договором класса числом («узел занимает
					 * двадцать октетов»), а поверить её проверкой НЕЛЬЗЯ: тип узла закрыт, и
					 * снаружи он не виден. Число же при доводе о плотном дереве держится ничем -
					 * довод («дети лежат сразу за родителем, переход к соседу стоит сложения»)
					 * от точной ширины не зависит вовсе, и поле, добавленное по надобности,
					 * растило бы узел МОЛЧА, оставляя договор устаревшим
					 *
					 * @note Сторож стоит внутри работы класса нарочно: там закрытый тип виден.
					 * Поле, прибавленное к узлу, не даст собрать кодек - рост становится
					 * решением осознанным, а не случившимся по дороге. Ширина эта переносима:
					 * поля узла суть перечень, три признака и два числа по четыре октета, а
					 * указателей и восьмиоктетных полей у него нет вовсе
					 *
					 * \~english
					 * @brief Guard of the width of a node of the tree of a document
					 * @details This width is declared by the contract of the class as a number, and it cannot
					 * be checked by a test: the type of a node is closed and is not visible from the outside
					 *
					 * \~
					 */
					static_assert(sizeof(Node) == 20, "ABC: ширина узла дерева документа переменилась, договор класса устарел");
				public:
					/**
					 * \~russian
					 * @brief Класс ссылки на значение документа
					 *
					 * @details Ссылка не владеет ничем и живёт не дольше документа. Извлечение
					 * сличает само значение с пределами затребованного вида, а не вид хранения
					 * с видом затребованным
					 *
					 * \~english
					 * @brief Class of a reference to a value of a document
					 * @details A reference owns nothing and lives no longer than the document. The extraction
					 * compares the value itself with the limits of the demanded kind rather than the kind of the storage
					 * with the demanded kind
					 *
					 * \~
					 */
					typedef class __AWH_SHARED_EXPORT__ Value {
						private:
							// Документ, которому значение принадлежит
							const Document * _doc;
						private:
							// Номер узла значения в дереве документа
							uint32_t _index;
						private:
							/**
							 * \~russian
							 * Номер узла за последним узлом вместимого, какому значение принадлежит
							 *
							 * @details Указания на родителя узел не несёт, а переход к соседу обязан
							 * останавливаться на границе своего вместимого: без границы обход массива
							 * продолжился бы соседом родителя его
							 *
							 * \~english
							 * Index of the node past the last node of the container to which the value belongs
							 * @details A node does not carry a pointer to its parent, while the transition to a sibling is obliged
							 * to stop at the boundary of its own container: without the boundary the traversal of an array
							 * would continue with the sibling of its parent
							 *
							 * \~
							 */
							uint32_t _bound;
						public:
							/**
							 * \~russian
							 * @brief Метод проверки действительности ссылки
							 *
							 * @return признак действительности ссылки
							 *
							 * \~english
							 * @brief Method of the checking of the validity of a reference
							 * @return sign of the validity of the reference
							 *
							 * \~
							 */
							[[nodiscard]] bool valid() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения вида значения
							 *
							 * @return вид значения документа
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of a value
							 * @return kind of the value of the document
							 *
							 * \~
							 */
							type_t type() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения вида узла
							 *
							 * @return вид узла документа
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of a node
							 * @return kind of the node of the document
							 *
							 * \~
							 */
							kind_t kind() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки принадлежности значения к виду
							 *
							 * @param type вид значения, сборный либо точный
							 * @return     признак принадлежности значения к виду
							 *
							 * \~english
							 * @brief Method of the checking of the belonging of a value to a kind
							 * @param type kind of the value, composite or exact
							 * @return sign of the belonging of the value to the kind
							 *
							 * \~
							 */
							[[nodiscard]] bool is(const type_t type) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения количества значений вместимого
							 *
							 * @return количество значений вместимого
							 *
							 * \~english
							 * @brief Method of the extraction of the number of the values of a container
							 * @return number of the values of the container
							 *
							 * \~
							 */
							size_t size() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод извлечения значения вместимого по его номеру
							 *
							 * @param index номер значения вместимого
							 * @return      ссылка на значение вместимого
							 *
							 * \~english
							 * @brief Method of the extraction of a value of a container by its number
							 * @param index number of the value of the container
							 * @return reference to the value of the container
							 *
							 * \~
							 */
							Value at(const size_t index) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения имени поля отображения по его номеру
							 *
							 * @details Имя поля выдаётся ссылкою на значение: именем вправе стоять
							 * не только строка, но и число, и отметка времени, и опознаватель
							 *
							 * @param index номер пары отображения
							 * @return      ссылка на имя поля отображения
							 *
							 * \~english
							 * @brief Method of the extraction of the name of a field of a mapping by its number
							 * @details The name of a field is issued by a reference to a value: not only a string has the right
							 * to stand as a name, but also a number, and a time stamp, and an identifier
							 * @param index number of the pair of the mapping
							 * @return reference to the name of the field of the mapping
							 *
							 * \~
							 */
							Value key(const size_t index) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения первого значения вместимого
							 *
							 * @details Обход вместимого ведётся первым значением вместе с переходом к
							 * соседу: обращение по номеру пропускает узлы, стоящие до затребованного,
							 * и обход им обошёлся бы дороже квадрата
							 *
							 * @note У отображения первым значением стоит имя первого поля: имя есть
							 * такой же узел, и признак `keyed` отличает его от значения
							 *
							 * @return ссылка на первое значение вместимого
							 *
							 * \~english
							 * @brief Method of the extraction of the first value of a container
							 * @details The traversal of a container is conducted by the first value together with the transition to
							 * a sibling: an access by a number skips the nodes standing before the demanded one,
							 * and a traversal by it would cost more than a square
							 * @note For a mapping the first value is the name of the first field: a name is
							 * the same kind of node, and the flag `keyed` distinguishes it from a value
							 * @return reference to the first value of the container
							 *
							 * \~
							 */
							Value begin() const noexcept;
							/**
							 * \~russian
							 * @brief Метод перехода к следующему значению вместимого
							 *
							 * @details Переход стоит одного сложения: вложенное вместимое пропускается
							 * целиком по размаху своего поддерева
							 *
							 * @return ссылка на следующее значение вместимого
							 *
							 * \~english
							 * @brief Method of the transition to the next value of a container
							 * @details The transition costs one addition: a nested container is skipped
							 * as a whole by the extent of its subtree
							 * @return reference to the next value of the container
							 *
							 * \~
							 */
							Value next() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки того, что значение является именем поля отображения
							 *
							 * @return признак того, что значение является именем поля отображения
							 *
							 * \~english
							 * @brief Method of the checking that a value is the name of a field of a mapping
							 * @return sign that the value is the name of a field of a mapping
							 *
							 * \~
							 */
							[[nodiscard]] bool keyed() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения значения поля отображения по имени
							 *
							 * @param name имя поля отображения
							 * @return     ссылка на значение поля отображения
							 *
							 * \~english
							 * @brief Method of the extraction of a value of a field of a mapping by a name
							 * @param name name of the field of the mapping
							 * @return reference to the value of the field of the mapping
							 *
							 * \~
							 */
							Value get(const string_view name) const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки наличия поля отображения по имени
							 *
							 * @param name имя поля отображения
							 * @return     признак наличия поля отображения
							 *
							 * \~english
							 * @brief Method of the checking of the presence of a field of a mapping by a name
							 * @param name name of the field of the mapping
							 * @return sign of the presence of the field of the mapping
							 *
							 * \~
							 */
							[[nodiscard]] bool has(const string_view name) const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод извлечения логического значения
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a logical value
							 * @param result value being extracted
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							[[nodiscard]] bool value(bool & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом целого без знака
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind of an integer without a sign
							 * @param result value being extracted
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							[[nodiscard]] bool value(uint64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом целого со знаком
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind of an integer with a sign
							 * @param result value being extracted
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							[[nodiscard]] bool value(int64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом дробного
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind of a fractional one
							 * @param result value being extracted
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							[[nodiscard]] bool value(double & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения содержимого значения
							 *
							 * @details Выдаётся содержимое строки, двоичных данных, опознавателя
							 * либо октеты величины числа неограниченной ширины
							 *
							 * @return содержимое значения
							 *
							 * \~english
							 * @brief Method of the extraction of the content of a value
							 * @details The content of a string, of binary data, of an identifier or
							 * the octets of the magnitude of a number of an unlimited width is issued
							 * @return content of the value
							 *
							 * \~
							 */
							string_view data() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения десятичного порядка величины
							 *
							 * @return десятичный порядок величины
							 *
							 * \~english
							 * @brief Method of the extraction of the decimal exponent of a magnitude
							 * @return decimal exponent of the magnitude
							 *
							 * \~
							 */
							int64_t exponent() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения номера подвида открытого расширения
							 *
							 * @note Номер этот принадлежит потребителю целиком: модуль его не толкует
							 *
							 * @return номер подвида открытого расширения
							 *
							 * \~english
							 * @brief Method of the extraction of the number of the subtype of an open extension
							 * @note This number belongs to the consumer entirely: the module does not interpret it
							 * @return number of the subtype of the open extension
							 *
							 * \~
							 */
							uint64_t subtype() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки того, что величина меньше нуля
							 *
							 * @return признак того, что величина меньше нуля
							 *
							 * \~english
							 * @brief Method of the checking that a magnitude is less than zero
							 * @return sign that the magnitude is less than zero
							 *
							 * \~
							 */
							[[nodiscard]] bool negative() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Конструктор
							 *
							 *
							 * \~english
							 * @brief Constructor
							 *
							 * \~
							 */
							Value() noexcept : _doc(nullptr), _index(NO_INDEX), _bound(0) {}
							/**
							 * \~russian
							 * @brief Конструктор
							 *
							 * @param doc   документ, которому значение принадлежит
							 * @param index номер узла значения в дереве документа
							 * @param bound номер узла за последним узлом вместимого, какому значение принадлежит
							 *
							 * \~english
							 * @brief Constructor
							 * @param doc document the value belongs to
							 * @param index number of the node of the value in the tree of the document
							 * @param bound index of the node past the last node of the container to which the value belongs
							 *
							 * \~
							 */
							Value(const Document * doc, const uint32_t index, const uint32_t bound) noexcept :
							 _doc(doc), _index(index), _bound(bound) {}
					} value_t;
				private:
					// Вместилище узлов дерева документа
					vector <node_t> _nodes;
				private:
					// Хранилище содержимого значений и имён полей
					string _storage;
				private:
					// Код отказа разбора записи
					mutable error_t _error;
				private:
					// Указатели имён полей отображений, заводимые по требованию
					mutable unordered_map <uint32_t, unordered_map <string_view, uint32_t>> _index;
				private:
					/**
					 * \~russian
					 * @brief Метод заведения указателя имён полей отображения
					 *
					 * @param index номер узла отображения в дереве документа
					 * @return      заведённый указатель имён полей
					 *
					 * \~english
					 * @brief Method of the creation of an index of the names of the fields of a mapping
					 * @param index number of the node of the mapping in the tree of the document
					 * @return created index of the names of the fields
					 *
					 * \~
					 */
					const unordered_map <string_view, uint32_t> & naming(const uint32_t index) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод сличения двух поддеревьев дерева документа
					 *
					 * @details Сличение ведётся по значению узлов, а не по записи, их
					 * породившей: дерево записи уже не помнит. Оттого два имени, писанные
					 * разной шириною метки, но несущие одно число, сличаются РАВНЫМИ, тогда
					 * как отказ `duplicate_t::REFUSE` у разбирателя сличает полную запись и
					 * счёл бы их различными. Расхождение это намеренно: правила `FIRST` и
					 * `LAST` выбирают между ЗНАЧЕНИЯМИ, и ширина метки к выбору отношения
					 * не имеет. Расхождение это закреплено проверкой
					 * `CodecAbcDocument.KeyIdentityDivergesByLayer`: она утверждает ОБЕ
					 * стороны его разом - согласие разбирателя и потерю пары деревом на
					 * одной и той же записи
					 *
					 * @param left  номер узла первого сличаемого поддерева
					 * @param right номер узла второго сличаемого поддерева
					 * @return      признак совпадения поддеревьев
					 *
					 * \~english
					 * @brief Method of the comparison of two subtrees of the tree of a document
					 * @details The comparison is conducted by the value of the nodes rather than by the record
					 * that produced them: the tree no longer remembers the record
					 * @param left number of the node of the first subtree being compared
					 * @param right number of the node of the second subtree being compared
					 * @return sign of the coincidence of the subtrees
					 *
					 * \~
					 */
					[[nodiscard]] bool identical(const uint32_t left, const uint32_t right) const noexcept;
					/**
					 * \~russian
					 * @brief Метод применения правила повтора имени к закрытому отображению
					 *
					 * @details Правило применяется в миг закрытия отображения оттого, что
					 * узлы его о ту пору суть ХВОСТ перечня: ничего построенного позади них
					 * ещё нет, и изъятие пары не двигает ни одного стороннего узла. Размах
					 * поддерева хранится длиною, а не указателем, оттого уцелевшие пары
					 * переносятся на новое место как есть
					 *
					 * @note Потоковый разбор правил `FIRST` и `LAST` осуществить не может:
					 * выбор между первым и последним требует видеть отображение целиком, а
					 * события выдаются по одному и назад разбор не ходит. Оттого правила эти
					 * живут деревом, а разбиратель их пропускает наравне с `KEEP`
					 *
					 * @param index номер узла закрываемого отображения
					 * @param rule  правило обращения с повторяющимся именем поля
					 *
					 * \~english
					 * @brief Method of the application of the rule of a repeating name to a closed mapping
					 * @details The rule is applied at the moment of the closing of a mapping because its nodes
					 * are the TAIL of the list by that time
					 * @param index number of the node of the mapping being closed
					 * @param rule rule of the handling of a repeating name of a field
					 *
					 * \~
					 */
					void resolve(const uint32_t index, const duplicate_t rule) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния документа
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of a document
					 *
					 * \~
					 */
					void clear() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора записи в дерево документа
					 *
					 * @param buffer   буфер разбираемой записи
					 * @param size     размер разбираемой записи в октетах
					 * @param settings настройки разбора записи
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a record into a tree of a document
					 * @param buffer buffer of the record being parsed
					 * @param size size of the record being parsed in octets
					 * @param settings settings of the parsing of the record
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool parse(const void * buffer, const size_t size, const reader_t::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора записи в дерево документа
					 *
					 * @details Разбор опустошает прежнее дерево сам, СОХРАНЯЯ запас его
					 * вместилищ: разбирающему многие записи подряд надлежит держать ОДНО
					 * дерево и звать разбор по кругу, а не заводить дерево на всякую запись
					 *
					 * @note Замер 23.08.2026, малая запись, 200 000 кругов: своё дерево на
					 * всякую запись идёт 0.27 мкс, одно дерево по кругу - 0.20 мкс, то есть
					 * на треть быстрее. У сборки записи рычаг этот вчетверо крупнее -
					 * см. `Writer::reset`
					 *
					 * \~english
					 * @brief Method of the parsing of a record into a tree of a document
					 * @details The parsing empties the former tree itself, KEEPING the reserve of its
					 * containers: one parsing many records in a row ought to keep a SINGLE tree
					 * @note Measurement of 23.08.2026, a small record, 200 000 rounds: an own tree per
					 * every record runs at 0.27 us, a single tree in a circle — at 0.20 us
					 * @param buffer buffer of the record being parsed
					 * @param size size of the record being parsed in octets
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool parse(const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки записи из дерева документа
					 *
					 * @warning Собранная запись равна разобранной по СМЫСЛУ, но не по октетам:
					 * дерево хранит состав вместимого, а не вид записи его, и вместимое
					 * неопределённой длины укладывается обратно определённым. Так, массив
					 * `9F 01 02 03 DF` (5 окт.) выходит записью `83 01 02 03` (4 окт.)
					 *
					 * @warning Подпись контейнера считается по ОКТЕТАМ: запись, прочитанная в
					 * дерево и уложенная обратно, подписи своей более не отвечает. Кому нужна
					 * дословность, тот держит исходные октеты сам, а не пересобирает их деревом
					 *
					 * @param writer сборщик бинарной записи
					 * @return       признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the assembling of a record from a tree of a document
					 * @warning The assembled record equals the parsed one by the MEANING but not by the octets:
					 * a container of an indefinite length is laid back as a definite one
					 * @warning The signature of a container is computed over the OCTETS: a record read into
					 * a tree and laid back no longer agrees with its signature
					 * @param writer assembler of a binary record
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool build(writer_t & writer) const noexcept;
				protected:
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа разбора документа
					 *
					 * @details Донесение идёт отсюда, из единственного места объявления отказа:
					 * работа отвечает отказом множеством путей, и запись в каждом из них
					 * разошлась бы с прочими. Отказ, ПРИНЯТЫЙ от нижнего слоя, сюда не идёт -
					 * тот слой донёс о нём сам, и второе донесение лишь двоило бы записи
					 *
					 * @param error объявляемый код отказа
					 * @return      признак успешности, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a failure
					 *
					 * @param error code of the failure being declared
					 * @return      flag of the success, always false
					 *
					 * \~
					 */
					bool fail(const error_t error) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод приёма события разбора, выданного прямо из чтения
					 *
					 * @param context указание на состояние сборки дерева документа
					 * @param reader  разбиратель бинарной записи
					 * @param event   вид принимаемого события разбора
					 *
					 * \~english
					 * @brief Method of the reception of an event of the parsing issued directly from the reading
					 * @param context pointer to the state of the assembling of the tree of the document
					 * @param reader parser of a binary record
					 * @param event kind of the received event of the parsing
					 *
					 * \~
					 */
					static void assemble(void * context, reader_t & reader, const event_t event) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки дерева документа по событию разбора
					 *
					 * @param context указание на состояние сборки дерева документа
					 * @param self    документ, чьё дерево собирается
					 * @param reader  разбиратель бинарной записи
					 * @param event   вид принимаемого события разбора
					 * @return        признак успешности сборки дерева
					 *
					 * \~english
					 * @brief Method of the assembling of the tree of a document by an event of the parsing
					 * @param context pointer to the state of the assembling of the tree of the document
					 * @param self document whose tree is being assembled
					 * @param reader parser of a binary record
					 * @param event kind of the received event of the parsing
					 * @return sign of the success of the assembling of the tree
					 *
					 * \~
					 */
					[[nodiscard]] static bool digest(void * context, Document * self, const reader_t & reader, const event_t event) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения корня дерева документа
					 *
					 * @return ссылка на корень дерева документа
					 *
					 * \~english
					 * @brief Method of the extraction of the root of the tree of a document
					 * @return reference to the root of the tree of the document
					 *
					 * \~
					 */
					value_t root() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества узлов дерева документа
					 *
					 * @return количество узлов дерева документа
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the nodes of the tree of a document
					 * @return number of the nodes of the tree of the document
					 *
					 * \~
					 */
					size_t nodes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа разбора записи
					 *
					 * @return код отказа разбора записи
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the parsing of a record
					 * @return error code of the parsing of the record
					 *
					 * \~
					 */
					error_t error() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log object for working with logs
					 *
					 * \~
					 */
					explicit Document(const log_t * log) noexcept :
					 _error(error_t::NONE), _log(log) {}
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Document() noexcept {}
				public:
					/**
					 * \~russian
					 * Ссылка на значение обходит дерево документа напрямую
					 *
					 * \~english
					 * A reference to a value traverses the tree of the document directly
					 *
					 * \~
					 */
					friend class Value;
			} document_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_DOCUMENT__
