/**
 * @file: stream.hpp
 * @date: 2026-07-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля потоковой компрессии — публичный API класса compressor::Stream, выполняющего
 *        инкрементальное сжатие и распаковку данных по мере их поступления с управлением временем жизни тяжёлого
 *        контекста движка и финализацией потока
 *
 * \~english
 * @brief Header file of the streaming compression module — the public API of the compressor::Stream class performing
 *        incremental compression and decompression of data as it arrives, managing the lifetime of the heavy engine
 *        context and the stream finalization
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_COMPRESSOR_STREAM__
#define __AWH_COMPRESSOR_STREAM__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <memory>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "types.hpp"
#include "../sys/log.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён компрессора
	 *
	 * \~english
	 * @brief Compressor namespace
	 *
	 * \~
	 */
	namespace compressor {
		/**
		 * \~russian
		 * @brief Внутренний бэкенд потокового движка (объявление, реализация скрыта)
		 *
		 * \~english
		 * @brief Internal backend of the streaming engine (declaration, implementation hidden)
		 *
		 * \~
		 */
		class coder_t;
		/**
		 * \~russian
		 * @brief Класс потоковой (streaming) компрессии/декомпрессии данных
		 *
		 * @details Один объект обслуживает один поток/соединение и не является
		 *          потокобезопасным. Тяжёлый контекст движка жив только от создания
		 *          объекта до финализации (finish) или разрушения.
		 *
		 *          Сжатие сообщений WebSocket (RFC 7692) ведётся не здесь, а классом
		 *          Block: там сообщение завершается Z_SYNC_FLUSH и контекст живёт между
		 *          сообщениями (takeoverDeflate). Потоковая сессия при финализации
		 *          закрывает поток Z_FINISH — это другая модель обмена, и метод DEFLATE
		 *          сам по себе к permessage-deflate её не приравнивает.
		 *
		 * \~english
		 * @brief Data streaming compression/decompression class
		 *
		 * @details A single object serves a single stream/connection and is not thread-safe.
		 *          The heavy engine context lives only from the creation of the object until
		 *          finalization (finish) or destruction.
		 *
		 *          WebSocket message compression (RFC 7692) is carried out not here but by the
		 *          Block class: there a message is terminated with Z_SYNC_FLUSH and the context
		 *          lives between messages (takeoverDeflate). Upon finalization a streaming session
		 *          closes the stream with Z_FINISH — this is a different exchange model, and the
		 *          DEFLATE method by itself does not equate it to permessage-deflate.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Stream {
			private:
				// Направление операции (компрессия/декомпрессия)
				event_t _event;
				// Метод компрессии
				method_t _method;
			private:
				// Живой контекст движка (nullptr => поток невалиден)
				unique_ptr <coder_t> _coder;
			private:
				/**
				 * Переиспользуемый буфер готового выхода.
				 *
				 * Заведён затем, чтобы подача порции не выделяла память заново: кодер пишет
				 * сюда, а наружу выход переносится либо копируется по типу контейнера
				 */
				mutable vector <char> _out;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки завершённости потока
				 *
				 * @details Для невалидного потока метод возвращает true: обрабатывать больше нечего.
				 *          Чтобы отличить корректно финализированный поток от сломанного (после
				 *          ошибки в push контекст освобождается), следует дополнительно проверить valid().
				 *
				 * @return true, если поток финализирован (после finish) либо невалиден
				 *
				 * \~english
				 * @brief Stream completeness check method
				 *
				 * @details For an invalid stream the method returns true: there is nothing left to process.
				 *          To tell a properly finalized stream from a broken one (after an error in push the
				 *          context is released), valid() should additionally be checked.
				 *
				 * @return true if the stream is finalized (after finish) or invalid
				 *
				 * \~
				 */
				bool done() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки валидности потока
				 *
				 * @return true, если движок поддержал потоковый режим и контекст создан
				 *
				 * \~english
				 * @brief Stream validity check method
				 *
				 * @return true if the engine supported the streaming mode and the context was created
				 *
				 * \~
				 */
				bool valid() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения направления операции потока
				 *
				 * @return направление операции
				 *
				 * \~english
				 * @brief Method of obtaining the stream operation direction
				 *
				 * @return operation direction
				 *
				 * \~
				 */
				event_t event() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения метода компрессии потока
				 *
				 * @return метод компрессии
				 *
				 * \~english
				 * @brief Method of obtaining the stream compression method
				 *
				 * @return compression method
				 *
				 * \~
				 */
				method_t method() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода принудительного выдавливания накопленного
				 *
				 * @tparam T тип контейнера результата
				 *
				 * \~english
				 * @brief Template of the method forcing out the accumulated data
				 *
				 * @tparam T result container type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод принудительного выдавливания накопленного (SYNC-flush)
				 *
				 * @details Выдавливание затем и заведено, чтобы поданное дошло до принимающей
				 *          стороны не дожидаясь конца потока. Восемь движков из девяти так и
				 *          работают, но BZip2 стоит особняком: сжатие выдавленное отдаёт, а
				 *          распаковка его целиком принимает и наружу не выдаёт ни октета,
				 *          дожидаясь конца потока. Свойство это принадлежит самой библиотеке
				 *          и обойти его силами модуля нечем, поэтому обмен сообщениями,
				 *          опирающийся на выдавливание, BZip2 вести не может — для такого
				 *          обмена берётся любой из восьми остальных движков
				 *
				 * @param result контейнер, куда помещается готовый выход
				 *
				 * \~english
				 * @brief Method forcing out the accumulated data (SYNC flush)
				 *
				 * @details Forcing out exists precisely so that what has been fed reaches the receiving
				 *          side without waiting for the end of the stream. Eight engines out of nine work
				 *          that way, but BZip2 stands apart: compression does give out what was forced out,
				 *          while decompression takes it in whole and yields not a single octet outward,
				 *          waiting for the end of the stream. This property belongs to the library itself
				 *          and there is no way for the module to work around it, therefore a message exchange
				 *          relying on forcing out cannot be conducted by BZip2 — for such an exchange any of
				 *          the eight remaining engines is taken
				 *
				 * @param result container the ready output is placed into
				 *
				 * \~
				 */
				void flush(T & result) noexcept;
				/**
				 * \~russian
				 * @brief Шаблон метода финализации потока
				 *
				 * @tparam T тип контейнера результата
				 *
				 * \~english
				 * @brief Template of the stream finalization method
				 *
				 * @tparam T result container type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод финализации потока (дожать хвост, завершить)
				 *
				 * @details На сжатии метод доводит кадр до конца, и done() после него истинен.
				 *          На распаковке done() отражает не вызов метода, а то, дошёл ли движок
				 *          до конца потока: на усечённом входе он останется ложным при валидном
				 *          потоке — это и есть признак недополученных данных, а не отказ модуля.
				 *
				 * @param result контейнер, куда помещается остаток данных
				 *
				 * \~english
				 * @brief Stream finalization method (squeeze out the tail, terminate)
				 *
				 * @details On compression the method brings the frame to its end, and done() is true
				 *          afterwards. On decompression done() reflects not the call of the method but
				 *          whether the engine reached the end of the stream: on truncated input it stays
				 *          false while the stream is valid — that is precisely the sign of data not fully
				 *          received rather than a failure of the module.
				 *
				 * @param result container the remaining data is placed into
				 *
				 * \~
				 */
				void finish(T & result) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода подачи порции данных в поток
				 *
				 * @tparam T тип контейнера результата
				 *
				 * \~english
				 * @brief Template of the method feeding a portion of data into the stream
				 *
				 * @tparam T result container type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод подачи порции данных в поток
				 *
				 * @param buffer буфер данных для обработки
				 * @param result контейнер, куда помещается готовый выход этой порции
				 * @param flush  режим сброса данных
				 *
				 * \~english
				 * @brief Method feeding a portion of data into the stream
				 *
				 * @param buffer data buffer to process
				 * @param result container the ready output of this portion is placed into
				 * @param flush  data flush mode
				 *
				 * \~
				 */
				void push(string_view buffer, T & result, const flush_t flush = flush_t::NONE) noexcept;
				/**
				 * \~russian
				 * @brief Шаблон метода подачи порции данных в поток
				 *
				 * @tparam T тип контейнера результата
				 *
				 * \~english
				 * @brief Template of the method feeding a portion of data into the stream
				 *
				 * @tparam T result container type
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод подачи порции данных в поток
				 *
				 * @details Единой политики обращения с хвостом за концом кадра нет: движок либо
				 *          отбрасывает посторонние октеты, либо считает их порчей и рвёт сессию.
				 *          Границы кадра задаёт вызывающая сторона, и разбор нескольких кадров
				 *          подряд одной сессией не предполагается — сессия обслуживает один поток.
				 *
				 *          Отвергнутая порция — пустой буфер при ненулевом размере либо размер,
				 *          не умещающийся в разрядность движка — сессию не рвёт: это ошибка
				 *          вызывающей стороны, а не порча контекста, и поток остаётся валидным.
				 *          Отказ движка, напротив, освобождает контекст, и valid() становится ложным.
				 *
				 * @param buffer буфер данных для обработки
				 * @param size   размер данных для обработки
				 * @param result контейнер, куда помещается готовый выход этой порции
				 * @param flush  режим сброса данных
				 *
				 * \~english
				 * @brief Method feeding a portion of data into the stream
				 *
				 * @details There is no single policy for handling the tail beyond the end of the frame: an
				 *          engine either discards the extraneous octets or treats them as corruption and tears
				 *          the session down. Frame boundaries are set by the calling side, and parsing several
				 *          frames in a row within one session is not intended — a session serves a single stream.
				 *
				 *          A rejected portion — an empty buffer with a non-zero size, or a size not fitting the
				 *          width of the engine — does not tear the session down: this is an error of the calling
				 *          side rather than corruption of the context, and the stream remains valid. A failure of
				 *          the engine, on the contrary, releases the context, and valid() becomes false.
				 *
				 * @param buffer data buffer to process
				 * @param size   size of the data to process
				 * @param result container the ready output of this portion is placed into
				 * @param flush  data flush mode
				 *
				 * \~
				 */
				void push(const void * buffer, const size_t size, T & result, const flush_t flush = flush_t::NONE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор перемещения
				 *
				 * \~english
				 * @brief Move assignment operator
				 *
				 * \~
				 */
				Stream & operator = (Stream &&) noexcept;
				/**
				 * \~russian
				 * @brief Запрещаем копирование
				 *
				 * @return результат операции
				 *
				 * \~english
				 * @brief Copying is forbidden
				 *
				 * @return operation result
				 *
				 * \~
				 */
				Stream & operator = (const Stream &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор перемещения
				 *
				 * \~english
				 * @brief Move constructor
				 *
				 * \~
				 */
				explicit Stream(Stream &&) noexcept;
				/**
				 * \~russian
				 * @brief Запрещаем копирование
				 *
				 * \~english
				 * @brief Copying is forbidden
				 *
				 * \~
				 */
				explicit Stream(const Stream &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор пустого (невалидного) потока
				 *
				 * \~english
				 * @brief Constructor of an empty (invalid) stream
				 *
				 * \~
				 */
				explicit Stream() noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @details Параметры проверяются здесь же, а не одними лишь установщиками
				 *          класса Block: конструктор открыт наружу. Размер скользящего окна вне
				 *          промежутка 9…15 оставляет сессию невалидной — у части значений он не
				 *          отказ даёт, а молча меняет формат: у GZip окно складывается с
				 *          шестнадцатью, у Deflate знак переворачивается.
				 *
				 * @param method метод компрессии
				 * @param event  направление операции
				 * @param params параметры инициализации
				 * @param log    объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @details The parameters are checked right here rather than by the setters of the Block
				 *          class alone: the constructor is open outward. A sliding window size outside the
				 *          9…15 range leaves the session invalid — for some values it does not give a failure
				 *          but silently changes the format: for GZip the window is added to sixteen, for
				 *          Deflate the sign is flipped.
				 *
				 * @param method compression method
				 * @param event  operation direction
				 * @param params initialization parameters
				 * @param log    object for working with logs
				 *
				 * \~
				 */
				explicit Stream(const method_t method, const event_t event, const params_t & params, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Stream() noexcept;
		} stream_t;
	};
};

#endif // __AWH_COMPRESSOR_STREAM__
