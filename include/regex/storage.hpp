/**
 * @file storage.hpp
 * @date 2026-08-04
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
 * @brief Заголовочный файл хранилища собранных регулярных выражений —
 *        запись собранных выражений последовательностью байтов и восстановление
 *        их из записи без повторного разбора и компиляции
 *
 * @section storage_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Наборы программы пишутся образом памяти.</b> Инструкции, классы
 *          символов, диапазоны и свойства переносятся в запись как есть, а
 *          восстановление их сводится к установке обзора на участок записи:
 *          размещения и переноса не выполняется вовсе. Ценою тому - привязка
 *          записи к устройству машины, поэтому заголовок несёт опознание
 *          порядка байтов и размеров структур, а запись чужая отвергается
 *          ошибкой «BAD_PLATFORM». Так же поступает и эталон: запись,
 *          порождённая «pcre2_serialize_encode», переносу между машинами
 *          не подлежит по его же описанию. Прочие поля - счётчики, признаки
 *          и последовательности символов - пишутся по-прежнему полями в
 *          порядке байтов от младшего.
 *
 *          <b>Запись живёт столько же, сколько восстановленные выражения.</b>
 *          Наборы программы обозревают участки записи, собственного содержимого
 *          не имея, поэтому запись берётся хранилищем во владение и держится
 *          разделяемым владением при каждой программе. Метод «load» копию
 *          записи заводит сам, метод «adopt» принимает её переносом.
 *
 *          <b>Сжатие записи выполняется обработчиками потребителя.</b> Метод
 *          сжатия хранится в заголовке, отчего запись самоописательна и
 *          восстановление разжимает её без напоминания. Само же сжатие
 *          хранилище не выполняет: оно вызывает обработчики, потребителем
 *          установленные. Решение это выбрано затем, чтобы модуль регулярных
 *          выражений остался самостоятельным - собираемым из своих исходных
 *          текстов на голой машине, где посторонних библиотек не установлено
 *          вовсе. Именно так его собирает переносимый стенд на девяти
 *          системах. Подключение «compressor» напрямую привязало бы модуль к
 *          дюжине сторонних библиотек сжатия ради возможности, потребной не
 *          всякому потребителю. Из «compressor» взят лишь перечень методов -
 *          заголовок его посторонних библиотек не подключает.
 *
 *          <b>Восстановление проверяет каждое поле записи.</b> Запись приходит
 *          из источника, доверия не заслуживающего, а разворачивается она в
 *          структуры, по каким исполнение ходит адресами и указателями. Поэтому
 *          проверяются все адреса переходов, все указания на классы символов, на
 *          последовательности символов, на ячейки состояния и на номера групп:
 *          запись испорченная обязана обернуться отказом, а не блужданием по
 *          памяти.
 *
 *          <b>Порождённый машинный код хранится вместе с выражением.</b>
 *          Порождение его заново обходится почти двенадцатью микросекундами на
 *          выражение против двух с половиною на всё прочее восстановление, то
 *          есть впятеро дороже. На наборе в миллион выражений это двенадцать
 *          секунд ожидания при запуске - ровно то, ради чего запись и заведена.
 *
 *          Хранение возможно потому, что порождённый код перемещаем: обращения
 *          за его пределы выполняются смещением от регистра обстановки либо от
 *          счётчика команд, а сама обстановка собирается восстановлением
 *          заново - четыре адреса подпрограмм и адрес отбора позиций
 *          принадлежат исполняемому образу, адреса же таблиц выводятся из
 *          смещений, записью хранимых.
 *
 *          <b>Код годен лишь набору команд, для какого порождён.</b> Запись
 *          несёт опознание набора команд, и восстановление на машине набора
 *          иного код отвергает, а сопоставитель порождает заново. Отказ этот
 *          изъяном не является и восстановления не срывает: выражение
 *          сопоставляется исполнением программы, покуда порождение не
 *          выполнено. Благодаря этому запись остаётся годной и там, где
 *          порождена машиной набора команд другого.
 *
 * \~english
 * @brief Header file of the storage of built regular expressions —
 *        writing built expressions as a sequence of bytes and restoring
 *        them from the record without repeated parsing and compilation
 * @section storage_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>The sequences of the program are written as a memory image.</b> The instructions, character
 *          classes, ranges and properties are transferred into the record as they are, and
 *          restoring them amounts to setting a view onto a span of the record:
 *          no allocation and no copying is performed at all. The price for that is the binding of
 *          the record to the arrangement of the machine, therefore the header carries the identification of
 *          the byte order and the sizes of the structures, and a foreign record is rejected
 *          with the «BAD_PLATFORM» error. The reference does the same: a record
 *          produced by «pcre2_serialize_encode» is not subject to transfer between machines
 *          by its own description. The other fields — counters, indications
 *          and character sequences — are written as before, as fields in
 *          little-endian byte order.
 *          <b>The record lives as long as the restored expressions.</b>
 *          The sequences of the program view spans of the record, having no content
 *          of their own, therefore the record is taken into ownership by the storage and is held
 *          by shared ownership at every program. The «load» method makes a copy of
 *          the record itself, the «adopt» method takes it over by moving.
 *          <b>Compression of the record is performed by the handlers of the consumer.</b> The compression
 *          method is kept in the header, which makes the record self-describing and lets
 *          restoration decompress it without a reminder. The compression itself is not performed by
 *          the storage: it calls the handlers set by the consumer.
 *          This decision was chosen so that the regular expression module
 *          remains self-sufficient — buildable from its own source
 *          texts on a bare machine where no foreign libraries are installed
 *          at all. That is exactly how the portable stand builds it on nine
 *          systems. Including «compressor» directly would bind the module to
 *          a dozen third-party compression libraries for the sake of a capability that not
 *          every consumer needs. Only the list of methods is taken from «compressor» —
 *          its header includes no foreign libraries.
 *          <b>Restoration checks every field of the record.</b> The record comes
 *          from a source that does not deserve trust, and it is unfolded into
 *          structures over which execution walks by addresses and pointers. Therefore
 *          all the jump addresses, all the references to character classes, to
 *          character sequences, to state cells and to group numbers are checked:
 *          a corrupted record must turn into a failure rather than into wandering over
 *          memory.
 *          <b>The generated machine code is kept together with the expression.</b>
 *          Generating it anew costs almost twelve microseconds per
 *          expression against two and a half for all the rest of the restoration, that
 *          is five times more. On a set of a million expressions that is twelve
 *          seconds of waiting at startup — exactly what the record was introduced for.
 *          Keeping it is possible because the generated code is relocatable: references
 *          beyond its bounds are performed by an offset from the context register or from
 *          the instruction counter, and the context itself is assembled by restoration
 *          anew — four subroutine addresses and the address of the position selection
 *          belong to the executable image, while the addresses of the tables are derived from
 *          the offsets kept by the record.
 *          <b>The code is fit only for the instruction set it was generated for.</b> The record
 *          carries the identification of the instruction set, and restoration on a machine with a different
 *          instruction set rejects the code and generates the matcher anew. That rejection
 *          is not a defect and does not break the restoration: the expression
 *          is matched by executing the program until the generation has been
 *          performed. Thanks to that the record remains fit even where it was
 *          produced by a machine with a different instruction set.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_STORAGE__
#define __AWH_REGEX_STORAGE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "engine.hpp"
#include "../compressor/types.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;


	/**
	 * \~russian
	 * @brief Пространство имён модуля регулярных выражений
	 *
	 * \~english
	 * @brief Namespace of the regular expression module
	 *
	 * \~
	 */
	namespace regex {
		/**
		 * \~russian
		 * @brief Опознание записи хранилища собранных выражений
		 *
		 * \~english
		 * @brief Identification of the record of the storage of built expressions
		 *
		 * \~
		 */
		constexpr uint64_t STORAGE_MAGIC = 0x5847455241485741ull;

		/**
		 * \~russian
		 * @brief Версия устройства записи хранилища собранных выражений
		 *
		 * @details Версия увеличивается при всякой правке устройства записи:
		 *          набора полей, порядка их следования, значения кодов операций
		 *          либо устройства структур, образом памяти записываемых. Запись версии иной восстановлению не подлежит,
		 *          поскольку прочитана будет неверно.
		 *
		 * \~english
		 * @brief Layout version of the record of the storage of built expressions
		 * @details The version is increased on every change of the layout of the record:
		 *          the set of fields, the order in which they follow, the values of the operation codes
		 *          or the layout of the structures written as a memory image. A record of a different version is not subject to restoration,
		 *          since it would be read incorrectly.
		 *
		 * \~
		 */
		constexpr uint16_t STORAGE_VERSION = 0x000C;

		/**
		 * \~russian
		 * @brief Коды ошибок хранилища собранных выражений
		 *
		 * \~english
		 * @brief Error codes of the storage of built expressions
		 *
		 * \~
		 */
		enum class storage_error_t : uint8_t {
			NONE          = 0x00, // Ошибок не обнаружено
			EMPTY         = 0x01, // Запись пуста
			TRUNCATED     = 0x02, // Запись оборвана до завершения
			BAD_MAGIC     = 0x03, // Опознание записи не совпадает
			BAD_VERSION   = 0x04, // Версия устройства записи не поддерживается
			BAD_CHECKSUM  = 0x05, // Контрольная сумма записи не совпадает
			BAD_CONTENT   = 0x06, // Содержимое записи не отвечает устройству
			TOO_LARGE     = 0x07, // Размер записи превышает допустимый
			BAD_PLATFORM  = 0x08, // Запись порождена машиной устройства иного
			BAD_METHOD    = 0x09, // Обработчик метода сжатия записи не установлен
			BAD_PACKING   = 0x0A  // Сжатие либо разжатие записи не выполнено
		};

		/**
		 * \~russian
		 * @brief Класс хранилища собранных регулярных выражений
		 *
		 * @details Класс записывает собранные выражения последовательностью
		 *          байтов и восстанавливает их из записи, минуя разбор текста
		 *          выражения и компиляцию программы. Восстановление обходится
		 *          на порядок дешевле сборки из текста, поскольку наборы
		 *          программы обозревают участки записи, а не размещаются заново.
		 *
		 * \~english
		 * @brief Class of the storage of built regular expressions
		 * @details The class writes built expressions as a sequence of
		 *          bytes and restores them from the record, bypassing parsing of the text
		 *          of the expression and compilation of the program. Restoration costs
		 *          an order of magnitude less than building from the text, since the sequences of
		 *          the program view spans of the record rather than being allocated anew.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Storage {
			public:
				/**
				 * \~russian
				 * @brief Собранное регулярное выражение
				 *
				 * \~english
				 * @brief Built regular expression
				 *
				 * \~
				 */
				using exp_t = shared_ptr <const expression_t>;
				/**
				 * \~russian
				 * @brief Обработчик сжатия либо разжатия записи хранилища
				 *
				 * @details Обработчик получает исходное содержимое и заполняет
				 *          выводимое, выдавая признак выполнения. Установка его
				 *          возложена на потребителя: хранилище знает метод сжатия
				 *          лишь по имени и сжатия не выполняет.
				 *
				 * \~english
				 * @brief Handler of compression or decompression of the storage record
				 * @details The handler receives the source content and fills in
				 *          the output one, yielding an indication of success. Setting it
				 *          is laid on the consumer: the storage knows the compression method
				 *          only by name and performs no compression.
				 *
				 * \~
				 */
				using packer_t = function <bool (string_view, string &)>;
			private:
				// Код ошибки хранилища собранных выражений
				mutable storage_error_t _error;
			private:
				// Объект журнала событий
				const log_t * _log;
			private:
				/**
				 * \~russian
				 * Признак доверия порождённому коду, записью несомому
				 *
				 * @details Порождённый машинный код записи исполняется, а не
				 *          разбирается, отчего проверкой он не оберегаем вовсе:
				 *          проверить можно данные, но не команды. Признак снят
				 *          по умолчанию, и восстановление код записи не берёт,
				 *          а порождает заново.
				 *
				 * \~english
				 * Indication of trust in the generated code carried by the record
				 * @details The generated machine code of the record is executed rather than
				 *          parsed, which is why it is not guarded by any check at all:
				 *          data can be checked, but not instructions. The indication is cleared
				 *          by default, and restoration does not take the code of the record
				 *          but generates it anew.
				 *
				 * \~
				 */
				bool _trusted;
			private:
				// Метод сжатия записи хранилища
				compressor::method_t _method;
			private:
				// Обработчик сжатия записи хранилища
				packer_t _pack;
				// Обработчик разжатия записи хранилища
				packer_t _unpack;
			private:
				/**
				 * \~russian
				 * @brief Метод записи программы регулярного выражения
				 *
				 * @param program записываемая программа
				 * @param result  запись хранилища
				 *
				 * \~english
				 * @brief Method of writing the program of a regular expression
				 * @param program program to write
				 * @param result  storage record
				 *
				 * \~
				 */
				void save(const program_t & program, string & result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод восстановления программы регулярного выражения
				 *
				 * @param data    запись хранилища
				 * @param offset  позиция чтения записи
				 * @param program восстанавливаемая программа
				 * @return        результат восстановления программы
				 *
				 * \~english
				 * @brief Method of restoring the program of a regular expression
				 * @param data    storage record
				 * @param offset  reading position in the record
				 * @param program program to restore
				 * @return        result of restoring the program
				 *
				 * \~
				 */
				bool load(string_view data, size_t & offset, program_t & program) const noexcept;
				/**
				 * \~russian
				 * @brief Метод восстановления собранных выражений из записи
				 *
				 * @param blob   запись хранилища, взятая во владение
				 * @param result набор восстановленных выражений
				 * @return       результат восстановления собранных выражений
				 *
				 * \~english
				 * @brief Method of restoring built expressions from a record
				 * @param blob   storage record taken into ownership
				 * @param result set of restored expressions
				 * @return       result of restoring the built expressions
				 *
				 * \~
				 */
				bool restore(const shared_ptr <const string> & blob, vector <exp_t> & result) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод восстановления собранных выражений телом своим
				 *
				 * @param blob   запись хранилища, взятая во владение
				 * @param result набор восстановленных выражений
				 * @return       результат восстановления собранных выражений
				 *
				 * @details Тело вынесено из метода публичного, дабы отказ его
				 *          сообщался журналом единожды: точек выхода отказных
				 *          у восстановления три десятка, и запись в журнал
				 *          при каждой обратила бы сообщение в бюрократию.
				 *
				 * \~english
				 * @brief Method of restoring built expressions by its own body
				 * @param blob   storage record taken into ownership
				 * @param result set of restored expressions
				 * @return       result of restoring the built expressions
				 *
				 * \~
				 */
				bool restoring(const shared_ptr <const string> & blob, vector <exp_t> & result) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки правильности программы регулярного выражения
				 *
				 * @param program проверяемая программа
				 * @return        результат проверки правильности программы
				 *
				 * @details Проверка выполняется над записью восстановленной и
				 *          удостоверяет, что адреса переходов, указания на классы
				 *          символов и прочие ссылки программы указывают внутрь
				 *          самой программы.
				 *
				 * \~english
				 * @brief Method of checking the validity of the program of a regular expression
				 * @param program program to check
				 * @return        result of checking the validity of the program
				 * @details The check is performed over a restored record and
				 *          certifies that the jump addresses, the references to character
				 *          classes and the other references of the program point inside
				 *          the program itself.
				 *
				 * \~
				 */
				bool verify(const program_t & program) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод записи собранных выражений
				 *
				 * @param expressions набор собранных выражений
				 * @param result      запись хранилища
				 * @return            результат записи собранных выражений
				 *
				 * \~english
				 * @brief Method of writing built expressions
				 * @param expressions set of built expressions
				 * @param result      storage record
				 * @return            result of writing the built expressions
				 *
				 * \~
				 */
				bool save(const vector <exp_t> & expressions, string & result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод восстановления собранных выражений
				 *
				 * @param data   запись хранилища
				 * @param result набор восстановленных выражений
				 * @return       результат восстановления собранных выражений
				 *
				 * @details Порождённый машинный код в записи не хранится и при
				 *          восстановлении порождается заново для выражений,
				 *          собранных с режимом «JIT».
				 *
				 * \~english
				 * @brief Method of restoring built expressions
				 * @param data   storage record
				 * @param result set of restored expressions
				 * @return       result of restoring the built expressions
				 * @details The generated machine code is not kept in the record and on
				 *          restoration is generated anew for the expressions
				 *          built with the «JIT» mode.
				 *
				 * \~
				 */
				bool load(string_view data, vector <exp_t> & result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод восстановления собранных выражений
				 *
				 * @param data   запись хранилища
				 * @param result набор восстановленных выражений
				 * @return       результат восстановления собранных выражений
				 *
				 * @details Запись передаётся хранилищу во владение, отчего
				 *          копирование её не выполняется вовсе: наборы программ
				 *          обозревают участки переданной записи, а она живёт
				 *          столько же, сколько восстановленные выражения.
				 *
				 * \~english
				 * @brief Method of restoring built expressions
				 * @param data   storage record
				 * @param result set of restored expressions
				 * @return       result of restoring the built expressions
				 * @details The record is handed to the storage into ownership, which is why
				 *          it is not copied at all: the sequences of the programs
				 *          view spans of the handed record, and it lives
				 *          as long as the restored expressions.
				 *
				 * \~
				 */
				bool adopt(string && data, vector <exp_t> & result) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки сжатия записи хранилища
				 *
				 * @param method метод сжатия записи хранилища
				 * @param pack   обработчик сжатия записи хранилища
				 * @param unpack обработчик разжатия записи хранилища
				 *
				 * @details Метод сжатия записывается в заголовок записи, поэтому
				 *          восстановление разжимает её тем же методом само.
				 *          Установка метода «NONE» сжатие отключает, и запись
				 *          восстанавливается обзорами прямо в ней, без разжатия.
				 *          Восстановление записи сжатой требует установленного
				 *          обработчика разжатия и без него отвечает ошибкой
				 *          «BAD_METHOD».
				 *
				 * \~english
				 * @brief Method of setting the compression of the storage record
				 * @param method compression method of the storage record
				 * @param pack   compression handler of the storage record
				 * @param unpack decompression handler of the storage record
				 * @details The compression method is written into the header of the record, therefore
				 *          restoration decompresses it by the same method on its own.
				 *          Setting the «NONE» method disables compression, and the record
				 *          is restored by views right inside it, without decompression.
				 *          Restoring a compressed record requires a set
				 *          decompression handler and without it answers with the
				 *          «BAD_METHOD» error.
				 *
				 * \~
				 */
				void packer(const compressor::method_t method, packer_t pack, packer_t unpack) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки доверия порождённому коду записи
				 *
				 * @param mode признак доверия порождённому коду записи
				 *
				 * @details Признак снят по умолчанию: восстановление порождённый
				 *          код записи не берёт, а порождает заново. Установка его
				 *          означает, что запись изготовлена самим потребителем
				 *          и подмене не подвергалась, - код её при этом берётся
				 *          как есть и размещается в исполняемой памяти.
				 *
				 * \~english
				 * @brief Method of setting the trust in the generated code of the record
				 * @param mode indication of trust in the generated code of the record
				 * @details The indication is cleared by default: restoration does not take the generated
				 *          code of the record but generates it anew. Setting it
				 *          means that the record was produced by the consumer itself
				 *          and was not subjected to substitution — its code is then taken
				 *          as it is and placed in executable memory.
				 *
				 * \~
				 */
				void trusted(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода ошибки хранилища
				 *
				 * @return код ошибки хранилища собранных выражений
				 *
				 * \~english
				 * @brief Method of getting the error code of the storage
				 * @return error code of the storage of built expressions
				 *
				 * \~
				 */
				storage_error_t error() const noexcept;
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
				explicit Storage(const log_t * log) noexcept :
				 _error(storage_error_t::NONE), _log(log), _trusted(false), _method(compressor::method_t::NONE) {}
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
				~Storage() noexcept {}
		} storage_t;
	}
}

#endif // __AWH_REGEX_STORAGE__
