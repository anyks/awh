/**
 * @file: connection.cpp
 * @date: 2026-07-21
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Заголовочные файлы BoringSSL
 */
#include <openssl/mem.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/digest.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/quic/connection.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутреннее пространство имён вспомогательных констант
 *
 */
namespace {
	/**
	 * @brief Длина генерируемых идентификаторов соединения (RFC 9000 §7.2 требует не менее 8 для DCID клиента)
	 *
	 */
	static constexpr size_t CID_SIZE = awh::quic::Connection::LOCAL_CID_SIZE;
	/**
	 * @brief Битовые флаги состояния соединения, упакованные в поле _core.flags
	 *
	 */
	namespace flags {
		// Флаги состояния не установлены
		static constexpr uint8_t NONE = 0x00;
		// Хендшейк подтверждён (RFC 9001 §4.1.2)
		static constexpr uint8_t CONFIRMED = 0x01;
		// Требуется отправка фрейма HANDSHAKE_DONE (только сервер)
		static constexpr uint8_t HANDSHAKE_DONE = 0x02;
		// Возврат отправленных ранних данных при отказе выполнен однократно (RFC 9001 §4.6.2)
		static constexpr uint8_t RESTORED = 0x04;
		// Отправлен ack-eliciting пакет после последнего принятого (RFC 9000 §10.1)
		static constexpr uint8_t IDLE_ELICITED = 0x08;
	}
	/**
	 * @brief Битовые флаги настраиваемых опций соединения, упакованные в поле _core.options
	 *
	 */
	namespace options {
		// Опции не установлены
		static constexpr uint8_t NONE = 0x00;
		// Следование за миграцией удалённого эндпоинта (RFC 9000 §9)
		static constexpr uint8_t ROAMING = 0x01;
	}
	/**
	 * @brief Минимальный размер нагрузки пакета для выборки защиты заголовка (RFC 9001 §5.4.2)
	 *
	 */
	static constexpr size_t MIN_PAYLOAD_SIZE = 4;
	/**
	 * @brief Максимальное количество хранимых диапазонов принятых номеров пакетов
	 *
	 */
	static constexpr size_t MAX_ACK_RANGES = 32;
	/**
	 * @brief Предельный лимит числа потоков фреймов MAX_STREAMS и STREAMS_BLOCKED
	 *
	 * @note Идентификаторы потоков кодируются varint, поэтому лимит сверх 2^60
	 *       разрешал бы открытие потока с некодируемым идентификатором
	 *       (RFC 9000 §19.11/§19.14)
	 */
	static constexpr uint64_t MAX_STREAMS_LIMIT = (static_cast <uint64_t> (1) << 60);
	/**
	 * @brief Объём анонсированного лимита приёма потока на один фрагмент сборки
	 *
	 * @note Количество фрагментов, буферизируемых до заполнения разрывов, ограничено
	 *       долей от анонсированного лимита приёма потока. Сами данные лимитом
	 *       уже ограничены, а вот их дробление - нет: тысячи однооктетных фрагментов
	 *       занимают на порядки больше памяти, чем несут данных (RFC 9000 §4.1)
	 */
	static constexpr uint64_t STREAM_CHUNK_SHARE = 512;
	/**
	 * @brief Минимальное количество фрагментов сборки потока
	 *
	 * @note Предел не опускается ниже: перестановка пакетов в сети образует разрывы
	 *       и на потоках с малым лимитом приёма
	 */
	static constexpr size_t MIN_STREAM_CHUNKS = 64;
	/**
	 * @brief Минимальная длина DCID первого пакета Initial клиента (RFC 9000 §7.2)
	 *
	 */
	static constexpr size_t MIN_INITIAL_DCID = 8;
	/**
	 * @brief Порог детекта потери по номеру пакета (RFC 9002 §6.1.1)
	 *
	 */
	static constexpr uint64_t PACKET_THRESHOLD = 3;
	/**
	 * @brief Гранулярность таймеров в миллисекундах (RFC 9002 §6.1.2)
	 *
	 */
	static constexpr uint64_t GRANULARITY = 1;
	/**
	 * @brief Начальная оценка задержки приёма-передачи в миллисекундах (RFC 9002 §6.2.2)
	 *
	 */
	static constexpr uint64_t INITIAL_RTT = 333;
	/**
	 * @brief Предельный показатель экспоненциальной выдержки таймера PTO
	 *
	 */
	static constexpr uint8_t MAX_PTO_COUNT = 10;
	/**
	 * @brief Срок годности токена проверки адреса в миллисекундах (RFC 9000 §8.1.3)
	 *
	 */
	static constexpr uint64_t RETRY_TOKEN_LIFETIME = 10000;
	/**
	 * @brief Метка формата токена проверки адреса, выданного пакетом Retry
	 *
	 */
	static constexpr uint8_t RETRY_TOKEN_MARK = 0x01;
	/**
	 * @brief Метка формата токена проверки адреса, выданного фреймом NEW_TOKEN
	 *
	 * @note Формат общий с токеном пакета Retry, различается меткой: обработка
	 *       у них разная, а отличить один от другого сервер обязан (RFC 9000 §8.1.4)
	 */
	static constexpr uint8_t ADDRESS_TOKEN_MARK = 0x02;
	/**
	 * @brief Срок годности токена проверки адреса фрейма NEW_TOKEN в миллисекундах
	 *
	 * @note Токен предъявляется в первом пакете следующего соединения, между
	 *       которыми проходит сколько угодно времени, поэтому срок годности
	 *       несопоставим со сроком годности токена пакета Retry (RFC 9000 §8.1.3)
	 */
	static constexpr uint64_t ADDRESS_TOKEN_LIFETIME = 86400000;
	/**
	 * @brief Размер фиксированной части токена: метка, отметка времени и длина ODCID
	 *
	 */
	static constexpr size_t RETRY_TOKEN_PREFIX = (1 + 8 + 1);
	/**
	 * @brief Длительность периода завершения соединения в интервалах таймера PTO
	 *
	 * @note Период выдерживается локальным эндпоинтом после отправки фрейма
	 *       CONNECTION_CLOSE: за него отставшие пакеты удалённого узла получают
	 *       повторный фрейм завершения, после чего состояние соединения
	 *       подлежит освобождению (RFC 9000 §10.2)
	 */
	static constexpr uint64_t CLOSING_PERIOD = 3;
	/**
	 * @brief Функция получения общего ключа подписи токенов проверки адреса
	 *
	 * @note Ключ создаётся один раз на процесс: токен обязан проверяться без
	 *       сохранения состояния, в том числе объектом соединения, отличным
	 *       от выдавшего его (RFC 9000 §8.1.4)
	 *
	 * @param output ключ подписи токенов проверки адреса
	 * @return       результат получения (false - ошибка генератора случайных чисел)
	 */
	static bool tokenKey(std::string & output) noexcept {
		/**
		 * @brief Структура владения общим ключом подписи токенов
		 *
		 */
		typedef struct Key {
			// Флаг успешной генерации ключа
			bool ready;
			// Данные ключа подписи токенов
			std::string data;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Key() noexcept : ready(false), data(32, '\0') {
				// Выполняем генерацию ключа подписи токенов
				this->ready = (::RAND_bytes(reinterpret_cast <uint8_t *> (&this->data[0]), this->data.size()) == 1);
			}
		} key_t;
		// Общий ключ подписи токенов проверки адреса
		static const key_t key;
		// Если ключ не сгенерирован
		if(!key.ready)
			// Выводим отрицательный результат
			return false;
		// Устанавливаем ключ подписи токенов
		output = key.data;
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Предельное количество выдаваемых идентификаторов соединения
	 *
	 */
	static constexpr uint64_t MAX_ISSUED_CIDS = 4;
	/**
	 * @brief Предельная длина очередей датаграмм приложения (RFC 9221 §5.3)
	 *
	 * @note Датаграммы flow control не подчиняются, поэтому очереди ограничены
	 *       сверху: без предела не читающее их приложение исчерпало бы память,
	 *       а ненадёжность доставки позволяет отбрасывать лишние
	 */
	static constexpr size_t MAX_QUEUED_DATAGRAMS = 64;
	/**
	 * @brief Предельное количество данных принятых проверок пути в очереди ответов
	 *
	 * @note Ответы ретрансмиссии не подлежат, поэтому неограниченная очередь
	 *       позволяла бы удалённому эндпоинту наращивать её потоком проверок
	 *       (RFC 9000 §8.2.2)
	 */
	static constexpr size_t MAX_QUEUED_RESPONSES = 8;
	/**
	 * @brief Начальное окно перегрузки в октетах (RFC 9002 §7.2)
	 *
	 */
	static constexpr uint64_t INITIAL_WINDOW = 12000;
	/**
	 * @brief Минимальное окно перегрузки в октетах (RFC 9002 §7.2)
	 *
	 */
	static constexpr uint64_t MINIMUM_WINDOW = 2400;
	/**
	 * @brief Количество зондирующих пакетов сверх окна перегрузки по таймеру PTO (RFC 9002 §7.5)
	 *
	 */
	static constexpr uint8_t PTO_PROBES = 2;
	/**
	 * @brief Множитель длительности периода устойчивой перегрузки (RFC 9002 §7.6.1)
	 *
	 */
	static constexpr uint64_t PERSISTENT_THRESHOLD = 3;
	/**
	 * @brief Минимальное количество потерянных пакетов для устойчивой перегрузки (RFC 9002 §7.6)
	 *
	 */
	static constexpr size_t PERSISTENT_PACKETS = 2;
	/**
	 * @brief Лимит целостности AEAD - предельное число неудачных снятий защиты (RFC 9001 §6.6)
	 *
	 * @details Взят наименьший лимит среди применяемых наборов (ChaCha20-Poly1305, 2^36):
	 *          завершение соединения не позже требуемого спецификацией допустимо
	 *          для любого набора
	 */
	static constexpr uint64_t AEAD_INTEGRITY_LIMIT = (static_cast <uint64_t> (1) << 36);
	/**
	 * @brief Лимит конфиденциальности AEAD - предельное число пакетов на одном ключе (RFC 9001 §6.6)
	 *
	 * @details Взят наименьший лимит среди применяемых наборов (AES-GCM, 2^23):
	 *          обновление ключей не позже требуемого спецификацией допустимо
	 *          для любого набора
	 */
	static constexpr uint64_t AEAD_CONFIDENTIALITY_LIMIT = (static_cast <uint64_t> (1) << 23);
	/**
	 * @brief Порог количества потоков для запуска сборки завершённых
	 *
	 * @details Сборка требует обхода очередей отправки, поэтому запускается только
	 *          когда список потоков вырос настолько, что обход себя окупает
	 */
	static constexpr size_t COLLECT_THRESHOLD = 64;
	/**
	 * @brief Порог уплотнения буфера исходящих данных потока в октетах
	 *
	 */
	static constexpr size_t COMPACT_THRESHOLD = 16384;
	/**
	 * @brief Запас октетов на заголовок фрейма CRYPTO (тип, смещение и длина)
	 *
	 */
	static constexpr size_t CRYPTO_OVERHEAD = 17;
	/**
	 * @brief Запас октетов на заголовок фрейма STREAM (тип, поток, смещение и длина)
	 *
	 */
	static constexpr size_t STREAM_OVERHEAD = 25;
	/**
	 * @brief Запас октетов на управляющий фрейм с двумя целочисленными полями
	 *
	 */
	static constexpr size_t CONTROL_OVERHEAD = 25;
	/**
	 * @brief Предельные накладные расходы фрейма DATAGRAM: тип и поле длины
	 *
	 */
	static constexpr size_t DATAGRAM_OVERHEAD = 9;
	/**
	 * @brief Точность поиска размера пути в октетах (RFC 8899 §5.1)
	 *
	 * @note Поиск прекращается, когда интервал сузился до этой величины:
	 *       дальнейшее уточнение зондами себя не окупает
	 */
	static constexpr size_t PMTU_GRANULARITY = 16;
	/**
	 * @brief Количество попыток отправки зонда одного размера (RFC 8899 §5.1.2)
	 *
	 */
	static constexpr uint8_t PMTU_PROBES = 3;
	/**
	 * @brief Запас октетов на фрейм NEW_CONNECTION_ID (RFC 9000 §19.15)
	 *
	 * @details Тип, порядковый номер, номер вывода из обращения, октет длины,
	 *          идентификатор соединения предельной длины и токен сброса
	 */
	static constexpr size_t NEW_CID_OVERHEAD = (1 + 8 + 8 + 1 + awh::quic::proto::MAX_CID_SIZE + awh::quic::proto::RESET_TOKEN_SIZE);
	/**
	 * @brief Функция генерации случайного идентификатора соединения
	 *
	 * @param cid генерируемый идентификатор соединения
	 * @return    результат генерации (false - ошибка генератора случайных чисел)
	 */
	static bool makeCid(awh::quic::cid_t & cid) noexcept {
		// Устанавливаем длину идентификатора соединения
		cid.size = CID_SIZE;
		// Выполняем генерацию случайных данных идентификатора
		return (::RAND_bytes(cid.data, CID_SIZE) == 1);
	}
};

/**
 * @brief Конструктор отправленного блока данных потока приложения
 *
 */
awh::quic::Connection::Chunk::Chunk() noexcept : sid(0), offset(0), fin(false), data{""} {}

/**
 * @brief Конструктор учётной записи отправленного пакета
 *
 */
awh::quic::Connection::Sent::Sent() noexcept :
 pn(0), time(0), size(0), handshakeDone(false), early(false),
 ecn(false), pmtu(false), stale(false) {}

/**
 * @brief Конструктор идентификатора соединения удалённого эндпоинта
 *
 */
/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Congestion::Congestion() noexcept :
 window(INITIAL_WINDOW), threshold(proto::VARINT_MAX), inflight(0), recovery(0),
 inRecovery(false), probes(0), acked(0), hasAcked(false) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Rtt::Rtt() noexcept :
 latest(0), minimum(0), smoothed(0), variation(0), sampled(false), ptoCount(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Path::Path() noexcept :
 pending(false), queued(false), validated(false), probe{0},
 relocating(false), alternate(false), deadline(0), address{""},
 previous{""}, migrations(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Close::Close() noexcept :
 code(0), app(false), queued(false), sent(false), received(0),
 threshold(1), deadline(0), reason{""} {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Token::Token() noexcept :
 retry(false), queued(false), initial{""}, retried{""}, address{""}, reset{""} {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Phase::Phase() noexcept :
 current(false), ready(false), hasPrevious(false), sent(0), packets(0), discard(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Routing::Routing() noexcept :
 sequence(0), retirePrior(0), issuedSeq(1), retired(false) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Marking::Marking() noexcept :
 received(event::ecn_t::NOT_ECT), enabled(false), failed(false) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Limits::Limits() noexcept :
 openedBidi(0), openedUni(0), maxBidiRemote(0), maxUniRemote(0), acceptedBidi(0),
 acceptedUni(0), maxBidiLocal(0), maxUniLocal(0), bidiQueued(false), uniQueued(false),
 bidiBlocked(false), uniBlocked(false), bidiBlockedAt(numeric_limits <uint64_t>::max()),
 uniBlockedAt(numeric_limits <uint64_t>::max()) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Flow::Flow() noexcept :
 txData(0), txMax(0), txBlocked(false), txBlockedAt(numeric_limits <uint64_t>::max()),
 rxData(0), rxConsumed(0), rxMax(0), rxQueued(false) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Buffers::Buffers() noexcept :
 datagram{""}, plain{""}, header{""} {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Dgram::Dgram() noexcept {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Identity::Identity() noexcept :
 updated(false), retried(false), relocated(false) {}

/**
 * @brief Конструктор
 *
 * @param endpoint роль локального эндпоинта на соединении
 */
awh::quic::Connection::Amplify::Amplify(const endpoint_t endpoint) noexcept :
 received(0), sent(0), validated(endpoint == endpoint_t::CLIENT) {}
/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::AEAD::AEAD() noexcept :
 failures(0), confidentiality(AEAD_CONFIDENTIALITY_LIMIT), integrity(AEAD_INTEGRITY_LIMIT) {}
/**
 * @brief Конструктор
 *
 * @param endpoint роль локального эндпоинта на соединении
 */
awh::quic::Connection::Core::Core(const endpoint_t endpoint) noexcept :
 endpoint(endpoint), state(state_t::NONE), error(error_t::NO_ERROR),
 flags(flags::NONE), options(options::ROAMING), address{""} {}
/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Times::Times() noexcept :
 now(0), idle(0) {}
/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Transport::Transport() noexcept {}
/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Cids::Cids() noexcept {}
/**
 * @brief Конструктор
 *
 * @param endpoint роль локального эндпоинта на соединении
 * @param ctx      идентификатор шаблона контекста безопасности
 * @param coder    объект кодера транспортной безопасности
 * @param log      объект для работы с логами
 */
awh::quic::Connection::Crypto::Crypto(const endpoint_t endpoint, const tls::coder_t::id_t ctx, const tls::coder_t & coder, const log_t * log) noexcept :
 handshake(endpoint, ctx, coder, log) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Streams::Streams() noexcept : cursor(0) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::Pmtu::Pmtu() noexcept :
 size(MAX_DATAGRAM_SIZE), high(MAX_PROBE_SIZE), limit(MAX_PROBE_SIZE),
 probe(0), count(0), queued(false) {}

/**
 * @brief Конструктор
 *
 */
awh::quic::Connection::RemoteCid::RemoteCid() noexcept : seq(0), used(false), hasToken(false), resetToken{0} {}

/**
 * @brief Конструктор состояния потока приложения
 *
 */
awh::quic::Connection::Stream::Stream() noexcept :
 txOffset(0), txBuffer{""}, txCursor(0), txMax(0), txFin(false), txFinSent(false),
 txReset(false), txResetSent(false), txResetCode(0), txBlocked(false),
 txBlockedAt(numeric_limits <uint64_t>::max()),
 rxOffset(0), rxHigh(0), rxReady{""}, rxMax(0), rxMaxQueued(false),
 rxFin(false), rxFinal(0), rxFinDelivered(false), rxCounted(0),
 rxReset(false), rxResetCode(0), stopQueued(false), stopSent(false),
 stopCode(0), credited(false), queued(false) {}

/**
 * @brief Конструктор состояния пространства номеров пакетов
 *
 */
awh::quic::Connection::Space::Space() noexcept :
 txPn(0), largestAcked(0), hasAcked(false), largestRx(0), hasRx(false), dedup(0),
 ackElicited(false), ackTime(0), ect0(0), ect1(0), ce(0), peerCe(0), peerEct0(0), peerEct1(0), hasPeerEcn(false), ecnSent(0),
 txOffset(0), txBuffer{""}, rxOffset(0), lossTime(0),
 hasLossTime(false), lastElicited(0), hasElicited(false), pingQueued(false) {}

/**
 * @brief Метод определения пространства номеров пакетов по уровню шифрования
 *
 * @param level уровень шифрования
 * @return      пространство номеров пакетов
 */
awh::quic::Connection::space_t awh::quic::Connection::space(const level_t level) const noexcept {
	/**
	 * Определяем уровень шифрования
	 */
	switch(level){
		// Уровень Initial
		case level_t::INITIAL:
			// Выводим пространство пакетов Initial
			return space_t::INITIAL;
		// Уровень хендшейка
		case level_t::HANDSHAKE:
			// Выводим пространство пакетов Handshake
			return space_t::HANDSHAKE;
		// Уровни ранних данных и приложения используют общее пространство (RFC 9000 §12.3)
		case level_t::EARLY_DATA:
		case level_t::APPLICATION:
			// Выводим пространство пакетов приложения
			return space_t::APPLICATION;
	}
	// Выводим пространство пакетов Initial по умолчанию
	return space_t::INITIAL;
}
/**
 * @brief Метод регистрации принятого номера пакета в диапазонах пространства
 *
 * @param space пространство номеров пакетов
 * @param pn    принятый номер пакета
 */
void awh::quic::Connection::record(const space_t space, const uint64_t pn) noexcept {
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (space)];
	/**
	 * Учитываем маркировку ECN принятого пакета: счётчики возвращаются пиру эхом
	 * во фрейме ACK_ECN, и по их приросту он судит о перегрузке пути. Счёт ведётся
	 * по пакетам, а не по датаграммам: коалесцированные пакеты одной датаграммы
	 * несут одну маркировку и учитываются каждый (RFC 9000 §13.4)
	 */
	switch(static_cast <uint8_t> (this->_marking.received)){
		// Если пакет несёт маркировку поддержки ECN
		case static_cast <uint8_t> (event::ecn_t::ECT0): item.ect0++; break;
		// Если пакет несёт альтернативную маркировку поддержки ECN
		case static_cast <uint8_t> (event::ecn_t::ECT1): item.ect1++; break;
		// Если пакет отмечен маршрутизатором как испытавший перегрузку
		case static_cast <uint8_t> (event::ecn_t::CE): item.ce++; break;
	}
	// Если пакет в пространстве принимается впервые
	if(!item.hasRx){
		// Устанавливаем наибольший принятый номер пакета
		item.largestRx = pn;
		// Устанавливаем флаг приёма хотя бы одного пакета
		item.hasRx = true;
	// Если принят номер пакета выше наибольшего принятого
	} else if(pn > item.largestRx) {
		// Вычисляем сдвиг окна защиты от повторов
		const uint64_t shift = (pn - item.largestRx);
		/**
		 * Сдвигаем окно и отмечаем в нём прежний наибольший номер пакета. При сдвиге
		 * ровно на 64 позиции сдвиг содержимого окна недопустим (неопределённое
		 * поведение), но прежний наибольший номер приходится на крайнюю позицию окна
		 * и отмечается старшим битом; при сдвиге свыше 64 окно вытесняется целиком
		 */
		item.dedup = ((shift < 64) ? ((item.dedup << shift) | (static_cast <uint64_t> (1) << (shift - 1))) :
		              ((shift == 64) ? (static_cast <uint64_t> (1) << 63) : 0));
		// Обновляем наибольший принятый номер пакета
		item.largestRx = pn;
	// Если принят номер пакета в пределах окна защиты от повторов
	} else if((item.largestRx - pn) <= 64)
		// Отмечаем номер пакета принятым в окне
		item.dedup |= (static_cast <uint64_t> (1) << ((item.largestRx - pn) - 1));
	/**
	 * Перебираем диапазоны принятых номеров пакетов (в порядке убывания)
	 */
	for(auto i = item.ranges.begin();; ++i){
		// Если диапазоны закончились
		if(i == item.ranges.end()){
			// Формируем новый диапазон из одного номера пакета
			frame::range_t range;
			// Устанавливаем наименьший номер диапазона
			range.low = pn;
			// Устанавливаем наибольший номер диапазона
			range.high = pn;
			// Добавляем диапазон в конец списка
			item.ranges.push_back(range);
			// Выходим из цикла
			break;
		}
		// Если номер пакета выше текущего диапазона с разрывом
		if(pn > (i->high + 1)){
			// Формируем новый диапазон из одного номера пакета
			frame::range_t range;
			// Устанавливаем наименьший номер диапазона
			range.low = pn;
			// Устанавливаем наибольший номер диапазона
			range.high = pn;
			// Вставляем диапазон перед текущим
			item.ranges.insert(i, range);
			// Выходим из цикла
			break;
		}
		/**
		 * Если номер пакета примыкает к диапазону сверху: смыкание с предыдущим
		 * диапазоном здесь невозможно и не проверяется. Диапазоны хранятся по
		 * убыванию, и чтобы перебор дошёл до непервого диапазона, номер обязан
		 * отстоять от предыдущего не менее чем на два - иначе он был бы поглощён
		 * им самим. Смыкание же требует ровно единицы, поэтому разрыв сверху
		 * закрывается ветвью примыкания снизу к предыдущему диапазону
		 */
		if(pn == (i->high + 1)){
			// Расширяем диапазон вверх
			i->high = pn;
			// Выходим из цикла
			break;
		}
		// Если номер пакета уже входит в диапазон
		if(pn >= i->low)
			// Выходим из цикла - дубликат
			break;
		// Если номер пакета примыкает к диапазону снизу
		if(pn == (i->low - 1)){
			// Расширяем диапазон вниз
			i->low = pn;
			// Если следующий диапазон примыкает к расширенному
			if(((i + 1) != item.ranges.end()) && ((i + 1)->high == (pn - 1))){
				// Расширяем текущий диапазон до следующего
				i->low = (i + 1)->low;
				// Удаляем следующий диапазон
				item.ranges.erase(i + 1);
			}
			// Выходим из цикла
			break;
		}
	}
	// Если количество диапазонов превысило лимит
	if(item.ranges.size() > MAX_ACK_RANGES)
		// Удаляем диапазон с наименьшими номерами пакетов
		item.ranges.pop_back();
}
/**
 * @brief Метод проверки повторного приёма номера пакета
 *
 * @param space пространство номеров пакетов
 * @param pn    принятый номер пакета
 * @return      результат проверки (true - пакет уже был принят)
 */
bool awh::quic::Connection::duplicate(const space_t space, const uint64_t pn) const noexcept {
	// Получаем состояние пространства номеров пакетов
	const auto & item = this->_spaces[static_cast <size_t> (space)];
	// Если пакетов в пространстве ещё не принималось
	if(!item.hasRx)
		// Выводим отрицательный результат
		return false;
	// Если номер пакета выше наибольшего принятого
	if(pn > item.largestRx)
		// Выводим отрицательный результат - пакет принимается впервые
		return false;
	// Если номер пакета совпадает с наибольшим принятым
	if(pn == item.largestRx)
		// Выводим положительный результат - дубликат
		return true;
	// Вычисляем удаление номера пакета от наибольшего принятого
	const uint64_t delta = (item.largestRx - pn);
	/**
	 * Если номер пакета вышел за пределы окна защиты от повторов - принять его
	 * безопасно нельзя, состояние приёма для него уже не хранится (RFC 9000 §12.3)
	 */
	if(delta > 64)
		// Выводим положительный результат - пакет отбрасывается как устаревший
		return true;
	// Выводим результат проверки номера пакета в окне
	return ((item.dedup >> (delta - 1)) & 1) != 0;
}
/**
 * @brief Метод перекладывания исходящих CRYPTO-данных хендшейка в буферы пространств
 *
 */
void awh::quic::Connection::pull() noexcept {
	// Список уровней шифрования с собственными CRYPTO-потоками
	static const level_t levels[] = {level_t::INITIAL, level_t::HANDSHAKE, level_t::APPLICATION};
	/**
	 * Перебираем список уровней шифрования
	 */
	for(auto & level : levels){
		// Если у хендшейк-машины есть исходящие CRYPTO-данные уровня
		if(this->_crypto.handshake.pending(level))
			// Перекладываем данные в буфер пространства номеров пакетов
			this->_spaces[static_cast <size_t> (this->space(level))].txBuffer.append(this->_crypto.handshake.data(level));
	}
}
/**
 * @brief Метод постановки завершения соединения с ошибкой транспорта в очередь
 *
 * @param error код ошибки транспорта
 */
void awh::quic::Connection::fail(const error_t error) noexcept {
	/**
	 * Если завершение соединения ещё не поставлено в очередь и не выполнено удалённым
	 * эндпоинтом: завершённое им соединение своим завершением не перекрывается -
	 * отправка пакетов в этом состоянии запрещена, а причина завершения уже
	 * получена от него самого (RFC 9000 §10.2.2)
	 */
	if(!this->_close.queued && (this->_core.state != state_t::DRAINING)){
		/**
		 * Записываем причину в лог: завершение с ошибкой транспорта - это единственная
		 * точка, где соединение рвётся по нарушению протокола, и без диагностики
		 * причина разрыва снаружи неразличима
		 */
		const string_view name = errorName(error);
		// Записываем ошибку в лог
		this->_log->print(
			"QUIC connection closed by transport error: %s (0x%llx)", log_t::flag_t::CRITICAL,
			string(name).c_str(), static_cast <unsigned long long> (error)
		);
		// Устанавливаем код ошибки транспорта соединения
		this->_core.error = error;
		// Устанавливаем код ошибки завершения соединения
		this->_close.code = static_cast <uint64_t> (error);
		// Сбрасываем флаг ошибки приложения
		this->_close.app = false;
		// Устанавливаем флаг постановки завершения соединения в очередь
		this->_close.queued = true;
		// Устанавливаем состояние завершения соединения
		this->_core.state = state_t::CLOSING;
	}
}
/**
 * @brief Метод сброса ключей уровня вместе с состоянием восстановления потерь (RFC 9001 §4.9)
 *
 * @param level уровень шифрования
 */
void awh::quic::Connection::discard(const level_t level) noexcept {
	// Сбрасываем ключи уровня шифрования
	this->_crypto.handshake.discard(level);
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
	/**
	 * Перебираем список отправленных пакетов пространства
	 */
	for(auto & packet : item.sent)
		// Списываем пакет из октетов в полёте (RFC 9002 §B.8)
		this->_congestion.inflight -= ::min(this->_congestion.inflight, static_cast <uint64_t> (packet.size));
	// Очищаем список отправленных пакетов
	item.sent.clear();
	// Очищаем очередь ретрансмиссии CRYPTO-данных
	item.rtxQueue.clear();
	// Очищаем буфер исходящих CRYPTO-данных
	item.txBuffer.clear();
	// Сбрасываем флаг взведённого таймера детекта потерь
	item.hasLossTime = false;
	// Сбрасываем флаг наличия отправленных ack-eliciting пакетов
	item.hasElicited = false;
	// Сбрасываем флаг необходимости отправки зондирующего фрейма PING
	item.pingQueued = false;
	// Сбрасываем флаг необходимости отправки подтверждения
	item.ackElicited = false;
}
/**
 * @brief Метод обновления оценки задержки приёма-передачи (RFC 9002 §5.3)
 *
 * @param sample измеренная задержка приёма-передачи
 * @param delay  задержка подтверждения удалённого эндпоинта
 */
void awh::quic::Connection::rtt(const uint64_t sample, const uint64_t delay) noexcept {
	// Устанавливаем последнюю измеренную задержку приёма-передачи
	this->_rtt.latest = sample;
	// Если это первое измерение задержки приёма-передачи
	if(!this->_rtt.sampled){
		// Устанавливаем минимальную задержку приёма-передачи
		this->_rtt.minimum = sample;
		// Устанавливаем сглаженную задержку приёма-передачи
		this->_rtt.smoothed = sample;
		// Устанавливаем вариативность задержки приёма-передачи
		this->_rtt.variation = (sample / 2);
		// Устанавливаем флаг наличия первого измерения
		this->_rtt.sampled = true;
	// Если измерения уже выполнялись
	} else {
		// Обновляем минимальную задержку приёма-передачи
		this->_rtt.minimum = ::min(this->_rtt.minimum, sample);
		// Скорректированная задержка приёма-передачи
		uint64_t adjusted = sample;
		// Если вычитание задержки подтверждения не опускает оценку ниже минимальной (RFC 9002 §5.3)
		if((adjusted >= delay) && ((adjusted - delay) >= this->_rtt.minimum))
			// Вычитаем задержку подтверждения удалённого эндпоинта
			adjusted -= delay;
		// Вычисляем отклонение от сглаженной задержки
		const uint64_t deviation = ((this->_rtt.smoothed > adjusted) ? (this->_rtt.smoothed - adjusted) : (adjusted - this->_rtt.smoothed));
		// Обновляем вариативность задержки приёма-передачи
		this->_rtt.variation = (((3 * this->_rtt.variation) + deviation) / 4);
		// Обновляем сглаженную задержку приёма-передачи
		this->_rtt.smoothed = (((7 * this->_rtt.smoothed) + adjusted) / 8);
	}
}
/**
 * @brief Метод повторной постановки содержимого пакета в очереди отправки (RFC 9002 §6.3)
 *
 * @param space  пространство номеров пакетов
 * @param packet учётная запись потерянного либо зондируемого пакета
 */
void awh::quic::Connection::requeue(const space_t space, const sent_t & packet) noexcept {
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (space)];
	/**
	 * Перебираем отправленные CRYPTO-данные пакета
	 */
	for(auto & chunk : packet.crypto)
		// Ставим CRYPTO-данные в очередь ретрансмиссии
		item.rtxQueue.push_back(chunk);
	// Если пакет содержал фрейм HANDSHAKE_DONE
	if(packet.handshakeDone)
		// Восстанавливаем флаг необходимости отправки фрейма HANDSHAKE_DONE
		this->_core.flags |= flags::HANDSHAKE_DONE;
	/**
	 * Перебираем отправленные блоки данных потоков приложения
	 */
	for(auto & chunk : packet.stream)
		// Ставим блок данных потока в очередь ретрансмиссии
		this->_stream.retransmit.push_back(chunk);
	/**
	 * Перебираем отправленные управляющие фреймы пакета
	 */
	for(auto & control : packet.control){
		/**
		 * Определяем тип управляющего фрейма
		 */
		switch(control.first){
			// Фрейм лимита данных соединения MAX_DATA
			case frame_t::MAX_DATA:
				// Восстанавливаем флаг необходимости отправки обновлённого лимита
				this->_flow.rxQueued = true;
			break;
			// Фрейм токена проверки адреса для будущих соединений NEW_TOKEN
			case frame_t::NEW_TOKEN:
				// Восстанавливаем флаг необходимости отправки токена проверки адреса
				this->_token.queued = true;
			break;
			// Фрейм лимита двунаправленных потоков MAX_STREAMS
			case frame_t::MAX_STREAMS_BIDI:
				// Восстанавливаем флаг необходимости отправки обновлённого лимита
				this->_limits.bidiQueued = true;
			break;
			// Фрейм лимита однонаправленных потоков MAX_STREAMS
			case frame_t::MAX_STREAMS_UNI:
				// Восстанавливаем флаг необходимости отправки обновлённого лимита
				this->_limits.uniQueued = true;
			break;
			// Фрейм анонса нового идентификатора соединения NEW_CONNECTION_ID
			case frame_t::NEW_CONNECTION_ID: {
				// Если идентификатор ещё выдан (не выведен из обращения)
				if(this->_cids.routing.issued.find(control.second) != this->_cids.routing.issued.end())
					// Восстанавливаем порядковый номер в очереди отправки анонсов
					this->_cids.routing.issueQueue.push_back(control.second);
			} break;
			// Фрейм вывода идентификатора соединения из обращения RETIRE_CONNECTION_ID
			case frame_t::RETIRE_CONNECTION_ID:
				// Восстанавливаем порядковый номер в очереди вывода из обращения
				this->_cids.routing.retireQueue.push_back(control.second);
			break;
			/**
			 * Фрейм ответа на проверку достижимости пути PATH_RESPONSE: ретрансмиссии
			 * не подлежит - при его потере удалённый эндпоинт присылает новую проверку,
			 * на которую отправляется новый ответ (RFC 9000 §13.3)
			 */
			case frame_t::PATH_RESPONSE: break;
			// Фрейм проверки достижимости пути PATH_CHALLENGE
			case frame_t::PATH_CHALLENGE: {
				// Если ответ на проверку пути ещё не получен
				if(this->_path.pending)
					// Восстанавливаем флаг необходимости отправки фрейма PATH_CHALLENGE
					this->_path.queued = true;
			} break;
			// Фрейм блокировки лимитом данных соединения DATA_BLOCKED
			case frame_t::DATA_BLOCKED:
				// Восстанавливаем флаг заблокированной отправки данных соединения
				this->_flow.txBlocked = true;
			break;
			// Фрейм блокировки лимитом двунаправленных потоков STREAMS_BLOCKED
			case frame_t::STREAMS_BLOCKED_BIDI:
				// Восстанавливаем флаг блокировки открытия двунаправленных потоков
				this->_limits.bidiBlocked = true;
			break;
			// Фрейм блокировки лимитом однонаправленных потоков STREAMS_BLOCKED
			case frame_t::STREAMS_BLOCKED_UNI:
				// Восстанавливаем флаг блокировки открытия однонаправленных потоков
				this->_limits.uniBlocked = true;
			break;
			// Фреймы состояния потока
			case frame_t::MAX_STREAM_DATA:
			case frame_t::RESET_STREAM:
			case frame_t::STOP_SENDING:
			case frame_t::STREAM_DATA_BLOCKED: {
				// Ищем поток по идентификатору
				auto i = this->_stream.list.find(control.second);
				// Если поток найден
				if(i != this->_stream.list.end()){
					// Если потерян фрейм лимита данных потока MAX_STREAM_DATA
					if(control.first == frame_t::MAX_STREAM_DATA)
						// Восстанавливаем флаг необходимости отправки обновлённого лимита
						i->second.rxMaxQueued = true;
					// Если потерян фрейм аварийного завершения потока RESET_STREAM
					else if(control.first == frame_t::RESET_STREAM)
						// Восстанавливаем флаг необходимости отправки фрейма RESET_STREAM
						i->second.txReset = true;
					// Если потерян фрейм блокировки лимитом данных потока STREAM_DATA_BLOCKED
					else if(control.first == frame_t::STREAM_DATA_BLOCKED)
						// Восстанавливаем флаг заблокированной отправки данных потока
						i->second.txBlocked = true;
					// Если потерян фрейм запроса прекращения передачи STOP_SENDING
					else i->second.stopQueued = true;
				}
			} break;
			// Остальные управляющие фреймы не ретранслируются
			default: break;
		}
	}
}
/**
 * @brief Метод возврата содержимого отправленных ранних данных в очереди отправки (RFC 9001 §4.6.2)
 *
 */
void awh::quic::Connection::restore() noexcept {
	/**
	 * Сбрасываем ключи уровня ранних данных: отправка ранних пакетов после отказа
	 * запрещена, а их содержимое уходит защитой уровня приложения. Общий метод
	 * сброса ключей здесь неприменим - уровни ранних данных и приложения делят
	 * пространство номеров пакетов, и он снял бы с учёта пакеты обоих
	 */
	this->_crypto.handshake.discard(level_t::EARLY_DATA);
	// Получаем состояние пространства номеров пакетов приложения
	auto & item = this->_spaces[static_cast <size_t> (space_t::APPLICATION)];
	// Количество возвращённых в очереди отправки ранних пакетов
	size_t count = 0;
	/**
	 * Перебираем список отправленных и ещё не подтверждённых пакетов пространства
	 */
	for(auto i = item.sent.begin(); i != item.sent.end();){
		// Если пакет отправлен не на уровне ранних данных
		if(!i->early){
			// Переходим к следующему пакету
			++i;
			// Продолжаем перебор
			continue;
		}
		// Ставим содержимое раннего пакета в очереди отправки
		this->requeue(space_t::APPLICATION, * i);
		// Списываем ранний пакет из октетов в полёте (RFC 9002 §B.8)
		this->_congestion.inflight -= ::min(this->_congestion.inflight, static_cast <uint64_t> (i->size));
		// Считаем возвращённый в очереди отправки ранний пакет
		count++;
		// Удаляем ранний пакет из списка отправленных
		i = item.sent.erase(i);
	}
	// Если неподтверждённых пакетов пространства не осталось
	if(item.sent.empty())
		// Сбрасываем флаг взведённого таймера детекта потерь
		item.hasLossTime = false;
	// Записываем в лог сообщение об отказе удалённого узла в ранних данных
	this->_log->print("QUIC early data rejected by peer, %zu packet(s) requeued", log_t::flag_t::INFO, count);
}
/**
 * @brief Метод проверки пути на поддержку ECN по счётчикам подтверждения (RFC 9000 §13.4.2.1)
 *
 * @param space  пространство номеров пакетов
 * @param frame  разобранный фрейм подтверждения со счётчиками маркировок
 * @param marked количество впервые подтверждённых помеченных пакетов
 * @return       результат обнаружения прироста счётчика перегрузки
 */
bool awh::quic::Connection::validate(const space_t space, const frame::ack_t & frame, const uint64_t marked) noexcept {
	/**
	 * Если маркировка исходящих датаграмм не ведётся либо проверка пути уже не
	 * пройдена: счётчики маркировок удалённого узла в обоих случаях недостоверны,
	 * и сокращать по ним окно перегрузки нельзя - иначе удалённый узел получил бы
	 * возможность произвольно давить нашу скорость отправки (RFC 9000 §13.4.2.2)
	 */
	if(!this->_marking.enabled || this->_marking.failed)
		// Выводим отрицательный результат - счётчики маркировок не обрабатываются
		return false;
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (space)];
	// Флаг обнаруженного прироста счётчика перегрузки
	bool congested = false;
	// Флаг непройденной проверки пути
	bool failed = false;
	// Если фрейм подтверждения несёт счётчики маркировок
	if(frame.hasEcn){
		/**
		 * Проверяем счётчики на убывание: счётчики маркировок удалённого узла
		 * только растут, и убывание любого из них означает их недостоверность
		 */
		failed = (item.hasPeerEcn && ((frame.ect0 < item.peerEct0) || (frame.ect1 < item.peerEct1) || (frame.ce < item.peerCe)));
		// Если счётчики маркировок достоверны
		if(!failed)
			/**
			 * Проверяем суммарный счётчик: удалённый узел не вправе учесть больше
			 * помеченных пакетов, чем было отправлено локальным эндпоинтом
			 */
			failed = ((frame.ect0 + frame.ect1 + frame.ce) > item.ecnSent);
		// Если счётчики маркировок достоверны и подтверждены помеченные пакеты
		if(!failed && (marked > 0)){
			// Вычисляем прирост счётчика пакетов с маркировкой поддержки ECN
			const uint64_t ect0 = (frame.ect0 - (item.hasPeerEcn ? item.peerEct0 : 0));
			// Вычисляем прирост счётчика пакетов с маркировкой перегрузки
			const uint64_t ce = (frame.ce - (item.hasPeerEcn ? item.peerCe : 0));
			/**
			 * Маркировка накладывается кодом ECT(0), поэтому каждый подтверждённый
			 * помеченный пакет обязан отразиться приростом счётчика поддержки либо
			 * счётчика перегрузки. Меньший прирост означает, что маркировку стёр
			 * либо подменил промежуточный узел пути
			 */
			failed = ((ect0 + ce) < marked);
		}
		// Если проверка пути пройдена
		if(!failed){
			// Определяем прирост счётчика пакетов с маркировкой перегрузки
			congested = (item.hasPeerEcn && (frame.ce > item.peerCe));
			// Запоминаем счётчик пакетов с маркировкой поддержки ECN
			item.peerEct0 = frame.ect0;
			// Запоминаем счётчик пакетов с альтернативной маркировкой поддержки ECN
			item.peerEct1 = frame.ect1;
			// Запоминаем счётчик пакетов с маркировкой перегрузки
			item.peerCe = frame.ce;
			// Устанавливаем флаг приёма счётчиков маркировок от пира
			item.hasPeerEcn = true;
		}
	/**
	 * Если счётчиков маркировок нет, а помеченные пакеты подтверждены: маркировку
	 * стёр промежуточный узел пути либо удалённый узел её не возвращает
	 */
	} else failed = (marked > 0);
	// Если проверка пути не пройдена
	if(failed){
		// Устанавливаем флаг непройденной проверки пути
		this->_marking.failed = true;
		// Записываем в лог сообщение о непройденной проверке пути
		this->_log->print("QUIC path does not support ECN, outgoing marking disabled", log_t::flag_t::WARNING);
		// Выводим отрицательный результат - счётчикам перегрузки доверять нельзя
		return false;
	}
	// Выводим результат обнаружения прироста счётчика перегрузки
	return congested;
}
/**
 * @brief Метод продвижения поиска размера пути (RFC 8899 §5.3)
 *
 */
void awh::quic::Connection::discover() noexcept {
	/**
	 * Если хендшейк не подтверждён либо адрес удалённого узла ещё не проверен:
	 * зондирование до завершения хендшейка только мешает - пакеты хендшейка и без
	 * того ограничены размером, который обязан пропускать любой путь (RFC 9000 §14.1).
	 * До подтверждения адреса зонды тратили бы лимит анти-амплификации, отнимая
	 * его у проверки достижимости пути (RFC 9000 §8.1)
	 */
	if(!(this->_core.flags & flags::CONFIRMED) || (this->_core.state != state_t::CONNECTED) ||
	   !this->_amplify.validated){
		// Сбрасываем размер собираемого зонда
		this->_pmtu.probe = 0;
		// Сбрасываем флаг необходимости отправки зонда
		this->_pmtu.queued = false;
		// Выходим из метода
		return;
	}
	/**
	 * Ограничиваем верхнюю границу поиска анонсированным удалённым узлом пределом
	 * приёма: датаграмму сверх него он не разберёт (RFC 9000 §18.2)
	 */
	if(this->_transport.remote.maxUdpPayloadSize > 0)
		// Опускаем верхнюю границу до анонсированного предела приёма
		this->_pmtu.high = ::min(this->_pmtu.high, static_cast <size_t> (this->_transport.remote.maxUdpPayloadSize));
	/**
	 * Если интервал поиска сузился до точности: продолжать зондирование незачем -
	 * выигрыш от уточнения не окупает отправки зондов (RFC 8899 §5.1)
	 */
	if((this->_pmtu.high <= this->_pmtu.size) || ((this->_pmtu.high - this->_pmtu.size) < PMTU_GRANULARITY)){
		// Сбрасываем размер собираемого зонда
		this->_pmtu.probe = 0;
		// Сбрасываем флаг необходимости отправки зонда
		this->_pmtu.queued = false;
		// Выходим из метода - поиск размера пути завершён
		return;
	}
	// Вычисляем размер очередного зонда делением интервала поиска пополам
	this->_pmtu.probe = (this->_pmtu.size + ((this->_pmtu.high - this->_pmtu.size + 1) / 2));
	// Обнуляем количество отправленных попыток зонда
	this->_pmtu.count = 0;
	// Устанавливаем флаг необходимости отправки зонда размера пути
	this->_pmtu.queued = true;
}
/**
 * @brief Метод детекта потерянных пакетов пространства (RFC 9002 §6.1)
 *
 * @param space пространство номеров пакетов
 */
void awh::quic::Connection::detect(const space_t space) noexcept {
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (space)];
	// Сбрасываем флаг взведённого таймера детекта потерь
	item.hasLossTime = false;
	// Если подтверждений от пира ещё не было
	if(!item.hasAcked)
		// Выходим из метода - детект потерь невозможен
		return;
	// Вычисляем базовую задержку детекта потерь (RFC 9002 §6.1.2)
	const uint64_t base = (this->_rtt.sampled ? ::max(this->_rtt.latest, this->_rtt.smoothed) : INITIAL_RTT);
	// Вычисляем задержку детекта потерь с коэффициентом 9/8
	const uint64_t lossDelay = ::max((9 * base) / 8, GRANULARITY);
	// Время отправки наиболее позднего потерянного пакета
	uint64_t lostTime = 0;
	// Флаг наличия потерянных пакетов
	bool lost = false;
	/**
	 * Границы периода потерь, пригодного для оценки устойчивой перегрузки: учитываются
	 * только пакеты, отправленные после наиболее позднего подтверждённого. Всё
	 * отправленное до него подтверждением закрыто, а период устойчивой перегрузки
	 * требует отсутствия подтверждений внутри него (RFC 9002 §7.6)
	 */
	uint64_t periodStart = 0;
	// Время отправки последнего потерянного пакета периода
	uint64_t periodEnd = 0;
	// Количество потерянных пакетов периода
	size_t periodCount = 0;
	/**
	 * Перебираем список отправленных пакетов
	 */
	for(auto i = item.sent.begin(); i != item.sent.end();){
		// Если пакет отправлен после наибольшего подтверждённого - потеря не детектируется
		if(i->pn > item.largestAcked){
			// Переходим к следующему пакету
			++i;
			// Продолжаем перебор
			continue;
		}
		// Если пакет потерян по порогу номера либо по порогу времени (RFC 9002 §6.1.1/§6.1.2)
		if(((i->pn + PACKET_THRESHOLD) <= item.largestAcked) || ((i->time + lossDelay) <= this->_times.now)){
			/**
			 * Если потерян зонд размера пути: пакет отброшен как не помещающийся
			 * в путь, а не из-за затора, поэтому событием перегрузки его потеря
			 * не является и в оценку устойчивой перегрузки не входит (RFC 9000 §14.4)
			 */
			if(i->pmtu){
				// Списываем потерянный зонд из октетов в полёте (RFC 9002 §B.8)
				this->_congestion.inflight -= ::min(this->_congestion.inflight, static_cast <uint64_t> (i->size));
				// Если попытки отправки зонда текущего размера не исчерпаны
				if((++this->_pmtu.count) < PMTU_PROBES)
					// Повторяем отправку зонда того же размера
					this->_pmtu.queued = true;
				// Если попытки исчерпаны - путь размера зонда не пропускает
				else {
					// Опускаем верхнюю границу поиска под размер непрошедшего зонда
					this->_pmtu.high = (this->_pmtu.probe - 1);
					// Продвигаем поиск размера пути
					this->discover();
				}
				// Удаляем потерянный зонд из списка отправленных
				i = item.sent.erase(i);
				// Продолжаем перебор
				continue;
			}
			// Ставим содержимое потерянного пакета в очереди отправки
			this->requeue(space, * i);
			/**
			 * Если потерян пакет прежнего пути: его содержимое переотправляется, но
			 * событием перегрузки нового пути его потеря не является - потерян он был
			 * на пути, которого соединение уже не использует (RFC 9000 §9.4)
			 */
			if(i->stale){
				// Удаляем потерянный пакет из списка отправленных
				i = item.sent.erase(i);
				// Продолжаем перебор
				continue;
			}
			// Списываем потерянный пакет из октетов в полёте (RFC 9002 §B.8)
			this->_congestion.inflight -= ::min(this->_congestion.inflight, static_cast <uint64_t> (i->size));
			// Запоминаем время отправки наиболее позднего потерянного пакета
			lostTime = ::max(lostTime, i->time);
			// Устанавливаем флаг наличия потерянных пакетов
			lost = true;
			// Если пакет отправлен после наиболее позднего подтверждённого
			if(!this->_congestion.hasAcked || (i->time > this->_congestion.acked)){
				// Если период потерь ещё не начат
				if(periodCount == 0)
					// Запоминаем время отправки первого потерянного пакета периода
					periodStart = i->time;
				// Продвигаем время отправки последнего потерянного пакета периода
				periodEnd = i->time;
				// Считаем потерянный пакет периода
				periodCount++;
			}
			// Удаляем потерянный пакет из списка отправленных
			i = item.sent.erase(i);
		// Если пакет ещё не потерян - взводим таймер детекта потерь
		} else {
			// Вычисляем время детекта потери пакета
			const uint64_t when = (i->time + lossDelay);
			// Если таймер ещё не взведён либо время детекта раньше
			if(!item.hasLossTime || (when < item.lossTime)){
				// Устанавливаем время детекта потерь пространства
				item.lossTime = when;
				// Устанавливаем флаг взведённого таймера детекта потерь
				item.hasLossTime = true;
			}
			// Переходим к следующему пакету
			++i;
		}
	}
	// Если детектированы потерянные пакеты
	if(lost)
		// Выполняем обработку события перегрузки (RFC 9002 §7.3.2)
		this->congestion(lostTime);
	/**
	 * Если потери образуют период устойчивой перегрузки: не менее двух потерянных
	 * ack-eliciting пакетов, ни один пакет между ними не подтверждён, а длительность
	 * периода превышает порог (RFC 9002 §7.6)
	 */
	if((periodCount >= PERSISTENT_PACKETS) && ((periodEnd - periodStart) > this->persistence())){
		// Схлопываем окно перегрузки до минимального (RFC 9002 §7.6.2)
		this->_congestion.window = MINIMUM_WINDOW;
		// Завершаем период восстановления - соединение возвращается в замедленный старт
		this->_congestion.inRecovery = false;
		// Сбрасываем время начала периода восстановления
		this->_congestion.recovery = 0;
	}
}
/**
 * @brief Метод постановки зондирующих данных пространства в очередь (RFC 9002 §6.2.4)
 *
 * @param space пространство номеров пакетов
 */
void awh::quic::Connection::probe(const space_t space) noexcept {
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (space)];
	// Если есть неподтверждённые отправленные пакеты
	if(!item.sent.empty()){
		// Получаем самый старый неподтверждённый пакет
		const auto & oldest = item.sent.front();
		// Ставим содержимое зондируемого пакета в очереди отправки
		this->requeue(space, oldest);
		// Если зондирующих данных в пакете не было
		if(oldest.crypto.empty() && !oldest.handshakeDone && oldest.stream.empty() && oldest.control.empty())
			// Устанавливаем флаг необходимости отправки зондирующего фрейма PING
			item.pingQueued = true;
	// Если неподтверждённых пакетов нет, а данных для отправки не осталось
	} else if(item.txBuffer.empty() && item.rtxQueue.empty())
		// Устанавливаем флаг необходимости отправки зондирующего фрейма PING
		item.pingQueued = true;
}
/**
 * @brief Метод вычисления интервала таймера PTO (RFC 9002 §6.2.1)
 *
 * @param space пространство номеров пакетов
 * @return     интервал таймера PTO в миллисекундах
 */
uint64_t awh::quic::Connection::interval(const space_t space) const noexcept {
	// Базовый интервал таймера PTO: сглаженная задержка + максимум из учетверённой вариативности и гранулярности
	uint64_t result = (this->_rtt.sampled ? (this->_rtt.smoothed + ::max(4 * this->_rtt.variation, GRANULARITY)) : (3 * INITIAL_RTT));
	// Если пространство пакетов приложения и хендшейк подтверждён (RFC 9002 §6.2.1)
	if((space == space_t::APPLICATION) && (this->_core.flags & flags::CONFIRMED))
		// Дописываем максимальную задержку подтверждения удалённого эндпоинта (RFC 9000 §18.2)
		result += this->_transport.remote.maxAckDelay;
	// Выводим интервал таймера PTO
	return result;
}
/**
 * @brief Метод вычисления дедлайна таймера PTO пространства (RFC 9002 §6.2.1)
 *
 * @param space пространство номеров пакетов
 * @return      дедлайн таймера PTO в миллисекундах (0 - таймер не взведён)
 */
uint64_t awh::quic::Connection::deadline(const space_t space) const noexcept {
	// Получаем состояние пространства номеров пакетов
	const auto & item = this->_spaces[static_cast <size_t> (space)];
	// Если неподтверждённых ack-eliciting пакетов нет
	if(item.sent.empty()){
		/**
		 * До подтверждения хендшейка клиент обязан держать таймер PTO взведённым даже
		 * при пустом окне отправки: иначе потеря флайта сервера оставит хендшейк
		 * без зондирования до самого таймаута простоя (RFC 9002 §6.2.2.1)
		 */
		if((this->_core.flags & flags::CONFIRMED) || (this->_core.endpoint != endpoint_t::CLIENT) || (space == space_t::APPLICATION))
			// Выводим нулевой дедлайн - таймер не взведён
			return 0;
		// Определяем уровень шифрования пространства номеров пакетов
		const level_t level = ((space == space_t::INITIAL) ? level_t::INITIAL : level_t::HANDSHAKE);
		// Если ключи защиты исходящих пакетов уровня сброшены - зондировать нечем
		if(this->_crypto.handshake.encryption(level) == nullptr)
			// Выводим нулевой дедлайн - таймер не взведён
			return 0;
		// Отсчитываем дедлайн от последней отправки уровня либо от начала активности
		const uint64_t base = (item.hasElicited ? item.lastElicited : this->_times.idle);
		// Если отправок на соединении ещё не было
		if(base == 0)
			// Выводим нулевой дедлайн - таймер не взведён
			return 0;
		// Выводим дедлайн зондирования с экспоненциальной выдержкой
		return (base + (this->interval(space) << this->_rtt.ptoCount));
	}
	// Выводим дедлайн: время последней ack-eliciting отправки + интервал с экспоненциальной выдержкой
	return (item.lastElicited + (this->interval(space) << this->_rtt.ptoCount));
}
/**
 * @brief Метод вычисления длительности периода устойчивой перегрузки (RFC 9002 §7.6.1)
 *
 * @return длительность периода устойчивой перегрузки в миллисекундах
 */
uint64_t awh::quic::Connection::persistence() const noexcept {
	// Базовый интервал таймера PTO без экспоненциальной выдержки
	const uint64_t base = (this->_rtt.sampled ? (this->_rtt.smoothed + ::max(4 * this->_rtt.variation, GRANULARITY)) : (3 * INITIAL_RTT));
	// Выводим длительность периода с учётом максимальной задержки подтверждения пира
	return ((base + this->_transport.remote.maxAckDelay) * PERSISTENT_THRESHOLD);
}
/**
 * @brief Метод вычисления дедлайна таймаута простоя соединения (RFC 9000 §10.1)
 *
 * @return дедлайн таймаута простоя в миллисекундах (0 - таймаут не согласован)
 */
uint64_t awh::quic::Connection::idle() const noexcept {
	// Таймаут простоя локального эндпоинта
	uint64_t result = this->_transport.local.maxIdleTimeout;
	// Таймаут простоя удалённого эндпоинта (известен после завершения хендшейка)
	const uint64_t remote = this->_transport.remote.maxIdleTimeout;
	// Если таймаут удалённого эндпоинта задан и строже локального (RFC 9000 §10.1)
	if((remote > 0) && ((result == 0) || (remote < result)))
		// Устанавливаем таймаут удалённого эндпоинта
		result = remote;
	// Если таймаут простоя не согласован либо активности ещё не было
	if((result == 0) || (this->_times.idle == 0))
		// Выводим нулевой дедлайн - таймаут отключён
		return 0;
	// Выводим дедлайн: таймаут не короче трёх интервалов PTO (RFC 9000 §10.1)
	return (this->_times.idle + ::max(result, 3 * this->interval(space_t::APPLICATION)));
}
/**
 * @brief Метод учёта данных потока потреблёнными в flow control соединения
 *
 * @param stream состояние потока
 * @param target учтённое смещение данных потока в октетах
 */
void awh::quic::Connection::consume(stream_data_t & stream, const uint64_t target) noexcept {
	// Если смещение ещё не учтено потреблённым (защита от повторных фреймов)
	if(target > stream.rxCounted){
		// Учитываем неучтённую часть данных потока в flow control соединения
		this->_flow.rxConsumed += (target - stream.rxCounted);
		// Продвигаем учтённое смещение данных потока
		stream.rxCounted = target;
	}
}
/**
 * @brief Метод обнаружения сброса без сохранения состояния (RFC 9000 §10.3.1)
 *
 * @param data буфер принятой датаграммы
 * @param size размер принятой датаграммы
 * @return     результат обнаружения (true - датаграмма является сбросом)
 */
bool awh::quic::Connection::stateless(const uint8_t * data, const size_t size) const noexcept {
	/**
	 * Сброс имитирует пакет с коротким заголовком и не может быть короче суммы
	 * минимального заголовка и токена: более короткие датаграммы сбросом не являются
	 */
	if(size < (5 + proto::RESET_TOKEN_SIZE))
		// Выводим отрицательный результат
		return false;
	// Если датаграмма начинается с длинного заголовка - это не сброс (RFC 9000 §10.3)
	if((data[0] & 0x80) != 0)
		// Выводим отрицательный результат
		return false;
	// Буфер предполагаемого токена сброса в хвосте датаграммы
	const uint8_t * candidate = (data + (size - proto::RESET_TOKEN_SIZE));
	/**
	 * Перебираем список идентификаторов соединения удалённого эндпоинта
	 */
	for(auto & item : this->_cids.routing.remote){
		/**
		 * Сверяем токены только использованных идентификаторов, для которых токен
		 * действительно получен: токены неиспользованных и выведенных из обращения
		 * идентификаторов в опознании не участвуют (RFC 9000 §10.3.1)
		 */
		if(item.used && item.hasToken && (CRYPTO_memcmp(candidate, item.resetToken, proto::RESET_TOKEN_SIZE) == 0))
			// Выводим положительный результат
			return true;
	}
	// Выводим отрицательный результат
	return false;
}
/**
 * @brief Метод формирования токена проверки адреса клиента (RFC 9000 §8.1.4)
 *
 * @param odcid  исходный DCID первого пакета Initial клиента
 * @param output сформированный токен проверки адреса
 * @return       результат формирования (false - ошибка генератора либо кода аутентичности)
 */
bool awh::quic::Connection::token(const uint8_t mark, const cid_t & odcid, string & output) const noexcept {
	// Ключ подписи токенов проверки адреса
	string key = "";
	// Получаем общий ключ подписи токенов
	if(!::tokenKey(key))
		// Формирование невозможно
		return false;
	// Очищаем выходной буфер токена
	output.clear();
	// Дописываем метку формата токена
	output.push_back(static_cast <char> (mark));
	/**
	 * Дописываем отметку времени выдачи в сетевом порядке байт
	 */
	for(size_t i = 8; i > 0; i--)
		// Дописываем очередной октет отметки времени
		output.push_back(static_cast <char> ((this->_times.now >> ((i - 1) * 8)) & 0xFF));
	// Дописываем длину исходного DCID
	output.push_back(static_cast <char> (odcid.size));
	// Если исходный DCID не пустой
	if(odcid.size > 0)
		// Дописываем данные исходного DCID
		output.append(reinterpret_cast <const char *> (odcid.data), odcid.size);
	/**
	 * Заверяем содержимое токена вместе с адресом клиента: адрес в токен не пишется,
	 * но входит в код аутентичности, поэтому токен непригоден с чужого адреса
	 */
	string signed_ = output;
	// Дописываем адрес клиента в заверяемые данные
	signed_.append(this->_core.address);
	// Буфер кода аутентичности
	uint8_t mac[EVP_MAX_MD_SIZE];
	// Длина кода аутентичности
	uint32_t length = 0;
	// Вычисляем код аутентичности содержимого токена
	if(::HMAC(::EVP_sha256(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (signed_.data()), signed_.size(), mac, &length) == nullptr)
		// Формирование невозможно
		return false;
	// Если длина кода аутентичности недостаточна
	if(length < proto::RESET_TOKEN_SIZE)
		// Формирование невозможно
		return false;
	// Дописываем усечённый код аутентичности в токен
	output.append(reinterpret_cast <const char *> (mac), proto::RESET_TOKEN_SIZE);
	// Формирование выполнено успешно
	return true;
}
/**
 * @brief Метод проверки токена проверки адреса клиента (RFC 9000 §8.1.4)
 *
 * @param token принятый токен проверки адреса
 * @param odcid восстановленный исходный DCID первого пакета Initial клиента
 * @return      результат проверки (true - токен выдан этому адресу и не истёк)
 */
bool awh::quic::Connection::validate(string_view token, cid_t & odcid, bool & retried) const noexcept {
	// Если токен короче суммы фиксированной части и кода аутентичности
	if(token.size() < (RETRY_TOKEN_PREFIX + proto::RESET_TOKEN_SIZE))
		// Проверка не пройдена
		return false;
	// Извлекаем метку формата токена
	const uint8_t mark = static_cast <uint8_t> (token[0]);
	// Если метка формата токена не соответствует ни одному из известных
	if((mark != RETRY_TOKEN_MARK) && (mark != ADDRESS_TOKEN_MARK))
		// Проверка не пройдена
		return false;
	// Устанавливаем признак выдачи токена пакетом Retry
	retried = (mark == RETRY_TOKEN_MARK);
	// Извлекаем длину исходного DCID
	const size_t length = static_cast <size_t> (static_cast <uint8_t> (token[9]));
	// Если длина исходного DCID превышает лимит QUIC v1
	if(length > proto::MAX_CID_SIZE)
		// Проверка не пройдена
		return false;
	/**
	 * Если токен выдан фреймом NEW_TOKEN и несёт идентификатор соединения: связать
	 * такой токен с соединением, на котором он выдан, наблюдателю не должно быть
	 * возможно, поэтому идентификатора в нём не бывает (RFC 9000 §8.1.3)
	 */
	if(!retried && (length > 0))
		// Проверка не пройдена
		return false;
	// Если объявленная длина не соответствует размеру токена
	if(token.size() != (RETRY_TOKEN_PREFIX + length + proto::RESET_TOKEN_SIZE))
		// Проверка не пройдена
		return false;
	// Ключ подписи токенов проверки адреса
	string key = "";
	// Получаем общий ключ подписи токенов
	if(!::tokenKey(key))
		// Проверка не пройдена
		return false;
	// Формируем заверяемые данные из содержимого токена без кода аутентичности
	string signed_(token.substr(0, RETRY_TOKEN_PREFIX + length));
	// Дописываем адрес клиента в заверяемые данные
	signed_.append(this->_core.address);
	// Буфер ожидаемого кода аутентичности
	uint8_t mac[EVP_MAX_MD_SIZE];
	// Длина кода аутентичности
	uint32_t size = 0;
	// Вычисляем ожидаемый код аутентичности содержимого токена
	if(::HMAC(::EVP_sha256(), key.data(), key.size(), reinterpret_cast <const uint8_t *> (signed_.data()), signed_.size(), mac, &size) == nullptr)
		// Проверка не пройдена
		return false;
	// Если длина кода аутентичности недостаточна
	if(size < proto::RESET_TOKEN_SIZE)
		// Проверка не пройдена
		return false;
	// Сравниваем коды аутентичности за постоянное время
	if(CRYPTO_memcmp(mac, token.data() + (token.size() - proto::RESET_TOKEN_SIZE), proto::RESET_TOKEN_SIZE) != 0)
		// Проверка не пройдена - токен выдан не этому адресу либо подделан
		return false;
	// Отметка времени выдачи токена
	uint64_t issued = 0;
	/**
	 * Собираем отметку времени выдачи из сетевого порядка байт
	 */
	for(size_t i = 1; i < 9; i++)
		// Дописываем очередной октет отметки времени
		issued = ((issued << 8) | static_cast <uint8_t> (token[i]));
	// Определяем срок годности токена по метке его формата
	const uint64_t lifetime = (retried ? RETRY_TOKEN_LIFETIME : ADDRESS_TOKEN_LIFETIME);
	// Если токен выдан в будущем либо срок его годности истёк (RFC 9000 §8.1.3)
	if((issued > this->_times.now) || ((this->_times.now - issued) > lifetime))
		// Проверка не пройдена
		return false;
	// Устанавливаем длину восстановленного исходного DCID
	odcid.size = length;
	// Если исходный DCID не пустой
	if(length > 0)
		// Копируем данные восстановленного исходного DCID
		::memcpy(odcid.data, token.data() + RETRY_TOKEN_PREFIX, length);
	// Проверка пройдена
	return true;
}
/**
 * @brief Метод вычисления доступного к отправке объёма данных (RFC 9000 §8.1)
 *
 * @return доступный к отправке объём данных в октетах
 */
size_t awh::quic::Connection::allowance() const noexcept {
	/**
	 * Если адрес удалённого эндпоинта подтверждён: лимит относится к адресу, а не
	 * к роли эндпоинта - усилителем трафика на чужой адрес соединение становится
	 * с любой стороны (RFC 9000 §8.1/§9.3.1)
	 */
	if(this->_amplify.validated)
		// Выводим подтверждённый размер датаграммы целиком
		return this->_pmtu.size;
	// Вычисляем трёхкратный объём принятых от удалённого эндпоинта октетов
	const uint64_t limit = (this->_amplify.received * 3);
	// Если лимит анти-амплификации исчерпан
	if(this->_amplify.sent >= limit)
		// Отправка запрещена до подтверждения адреса
		return 0;
	// Выводим остаток лимита, ограниченный размером датаграммы
	return static_cast <size_t> (::min(limit - this->_amplify.sent, static_cast <uint64_t> (this->_pmtu.size)));
}
/**
 * @brief Метод проверки невозможности отправки под лимитом анти-амплификации (RFC 9002 §6.2.2.1)
 *
 * @return результат проверки
 */
bool awh::quic::Connection::stalled() const noexcept {
	// Если адрес удалённого эндпоинта подтверждён - лимит отправку не ограничивает
	if(this->_amplify.validated)
		// Выводим отрицательный результат
		return false;
	/**
	 * Сравниваем остаток лимита с минимальным размером защищённого пакета: остаток
	 * меньше него не вмещает ни одного пакета, то есть отправка запрещена не менее
	 * надёжно, чем при исчерпанном нацело лимите
	 */
	return (this->allowance() < (CONTROL_OVERHEAD + MIN_PAYLOAD_SIZE + crypto::AEAD_TAG_SIZE));
}
/**
 * @brief Метод удаления завершённых потоков приложения
 *
 */
void awh::quic::Connection::collect() noexcept {
	// Если количество потоков не достигло порога сборки
	if(this->_stream.list.size() <= COLLECT_THRESHOLD)
		// Выходим из метода - обход списка не окупается
		return;
	// Получаем состояние пространства пакетов приложения
	const auto & item = this->_spaces[static_cast <size_t> (space_t::APPLICATION)];
	/**
	 * Перебираем список потоков приложения
	 */
	for(auto i = this->_stream.list.begin(); i != this->_stream.list.end();){
		// Получаем состояние потока
		const auto & stream = i->second;
		/**
		 * Определяем завершённость отправки: передан FIN либо аварийное завершение.
		 * Сторона, которой у потока нет, завершена всегда: в однонаправленный поток
		 * отправляет только его инициатор, и ждать от прочих завершения отправки
		 * значило бы не освободить такой поток никогда (RFC 9000 §2.1)
		 */
		const bool sent = (!this->sendable(i->first) || ((stream.txFinSent || stream.txResetSent) && (stream.txBuffer.size() == stream.txCursor)));
		/**
		 * Определяем завершённость приёма: поток сброшен, прекращён либо выдан
		 * приложению. Сторона, которой у потока нет, завершена всегда - из
		 * однонаправленного потока принимает кто угодно, кроме его инициатора
		 */
		const bool received = (!this->receivable(i->first) || stream.rxReset || stream.stopSent || (stream.rxFin && stream.rxFinDelivered));
		// Если хотя бы одна сторона потока не завершена
		if(!sent || !received){
			// Переходим к следующему потоку
			++i;
			// Продолжаем перебор
			continue;
		}
		// Флаг наличия ссылок на поток в очередях отправки
		bool referenced = false;
		/**
		 * Перебираем очередь ретрансмиссии блоков данных потоков
		 */
		for(auto & chunk : this->_stream.retransmit)
			// Определяем наличие ссылки на поток
			referenced = (referenced || (chunk.sid == i->first));
		/**
		 * Перебираем учётные записи неподтверждённых пакетов
		 */
		for(auto j = item.sent.begin(); (j != item.sent.end()) && !referenced; ++j){
			/**
			 * Перебираем отправленные блоки данных потоков пакета
			 */
			for(auto & chunk : j->stream)
				// Определяем наличие ссылки на поток
				referenced = (referenced || (chunk.sid == i->first));
			/**
			 * Перебираем отправленные управляющие фреймы пакета
			 */
			for(auto & control : j->control)
				// Определяем наличие ссылки на поток в управляющих фреймах потока
				referenced = (referenced || ((control.second == i->first) &&
				 ((control.first == frame_t::MAX_STREAM_DATA) || (control.first == frame_t::RESET_STREAM) ||
				  (control.first == frame_t::STOP_SENDING) || (control.first == frame_t::STREAM_DATA_BLOCKED))));
		}
		// Если на поток ещё ссылаются очереди отправки
		if(referenced)
			// Переходим к следующему потоку
			++i;
		// Удаляем завершённый поток из списка
		else i = this->_stream.list.erase(i);
	}
}
/**
 * @brief Метод вывода ключей следующей фазы уровня приложения (RFC 9001 §6)
 *
 */
void awh::quic::Connection::prepare() noexcept {
	// Если хендшейк не подтверждён либо ключи следующей фазы уже выведены
	if(!(this->_core.flags & flags::CONFIRMED) || this->_crypto.phase.ready)
		// Выходим из метода
		return;
	// Получаем текущие ключи снятия защиты пакетов уровня приложения
	const crypto::keys_t * read = this->_crypto.handshake.decryption(level_t::APPLICATION);
	// Получаем текущие ключи защиты пакетов уровня приложения
	const crypto::keys_t * write = this->_crypto.handshake.encryption(level_t::APPLICATION);
	// Если ключи уровня приложения ещё не выведены
	if((read == nullptr) || (write == nullptr))
		// Выходим из метода
		return;
	// Выводим ключи следующей фазы обоих направлений меткой "quic ku"
	this->_crypto.phase.ready = (crypto::update(* read, this->_crypto.phase.nextRead) && crypto::update(* write, this->_crypto.phase.nextWrite));
}
/**
 * @brief Метод переключения на следующую фазу ключей уровня приложения (RFC 9001 §6)
 *
 */
void awh::quic::Connection::promote() noexcept {
	// Получаем текущие ключи снятия защиты пакетов уровня приложения
	const crypto::keys_t * read = this->_crypto.handshake.decryption(level_t::APPLICATION);
	// Если ключи уровня приложения недоступны либо ключи следующей фазы не выведены
	if((read == nullptr) || !this->_crypto.phase.ready)
		// Выходим из метода - переключение фазы невозможно
		return;
	// Записываем в лог сообщение о переключении фазы ключей защиты пакетов
	this->_log->print(
		"QUIC key phase switched to %u", log_t::flag_t::INFO,
		static_cast <uint32_t> (!this->_crypto.phase.current)
	);
	// Сохраняем текущие ключи чтения для отставших пакетов предыдущей фазы
	this->_crypto.phase.prevRead = (* read);
	// Устанавливаем флаг наличия ключей чтения предыдущей фазы
	this->_crypto.phase.hasPrevious = true;
	/**
	 * Планируем сброс ключей предыдущей фазы через трёхкратный интервал PTO: за это
	 * время отставшие пакеты прежней фазы уже придут либо будут признаны потерянными,
	 * а дальнейшее удержание ключей лишь расширяло бы окно их возможного применения
	 * (RFC 9001 §6.3)
	 */
	this->_crypto.phase.discard = (this->_times.now + (3 * this->interval(space_t::APPLICATION)));
	// Устанавливаем ключи следующей фазы текущими
	this->_crypto.handshake.install(level_t::APPLICATION, this->_crypto.phase.nextRead, this->_crypto.phase.nextWrite);
	// Переключаем бит фазы ключей
	this->_crypto.phase.current = !this->_crypto.phase.current;
	// Запоминаем номер первого пакета текущей фазы
	this->_crypto.phase.sent = this->_spaces[static_cast <size_t> (space_t::APPLICATION)].txPn;
	// Сбрасываем счётчик пакетов, защищённых ключами фазы (RFC 9001 §6.6)
	this->_crypto.phase.packets = 0;
	// Сбрасываем флаг наличия выведенных ключей следующей фазы
	this->_crypto.phase.ready = false;
	// Выводим ключи новой следующей фазы
	this->prepare();
}
/**
 * @brief Метод учёта подтверждённого пакета в congestion control (RFC 9002 §7.3.1)
 *
 * @param packet учётная запись подтверждённого пакета
 */
void awh::quic::Connection::acked(const sent_t & packet) noexcept {
	/**
	 * Если подтверждён пакет прежнего пути: октеты в полёте по нему уже списаны
	 * сменой пути, а ёмкость нового пути его подтверждение не характеризует -
	 * окно перегрузки нового пути такой пакет наращивать не вправе (RFC 9000 §9.4)
	 */
	if(packet.stale)
		// Выходим из метода
		return;
	// Списываем подтверждённый пакет из октетов в полёте
	this->_congestion.inflight -= ::min(this->_congestion.inflight, static_cast <uint64_t> (packet.size));
	/**
	 * Если подтверждён зонд размера пути: путь пропускает датаграммы такого размера,
	 * поэтому подтверждённый размер поднимается до размера зонда, а поиск
	 * продолжается в оставшемся интервале (RFC 8899 §5.3)
	 */
	if(packet.pmtu && (packet.size > this->_pmtu.size)){
		// Поднимаем подтверждённый размер исходящей датаграммы до размера зонда
		this->_pmtu.size = packet.size;
		// Записываем в лог сообщение о подтверждённом размере пути
		this->_log->print("QUIC path maximum transmission unit raised to %zu bytes", log_t::flag_t::INFO, this->_pmtu.size);
		// Продвигаем поиск размера пути
		this->discover();
	}
	// Если период восстановления активен
	if(this->_congestion.inRecovery){
		// Если подтверждён пакет, отправленный после начала восстановления
		if(packet.time > this->_congestion.recovery)
			// Завершаем период восстановления (RFC 9002 §7.3.2)
			this->_congestion.inRecovery = false;
		// Пакеты периода восстановления окно перегрузки не увеличивают
		else return;
	}
	// Если окно перегрузки меньше порога замедленного старта
	if(this->_congestion.window < this->_congestion.threshold)
		// Замедленный старт: окно растёт на размер подтверждённого пакета (RFC 9002 §7.3.1)
		this->_congestion.window += packet.size;
	// Предотвращение перегрузки: окно растёт на долю датаграммы (RFC 9002 §7.3.3)
	else this->_congestion.window += ::max(static_cast <uint64_t> (1), (MAX_DATAGRAM_SIZE * static_cast <uint64_t> (packet.size)) / this->_congestion.window);
}
/**
 * @brief Метод обработки события перегрузки при детекте потерь (RFC 9002 §7.3.2)
 *
 * @param time время отправки наиболее позднего потерянного пакета
 */
void awh::quic::Connection::congestion(const uint64_t time) noexcept {
	// Если потерянный пакет отправлен в текущем периоде восстановления
	if(this->_congestion.inRecovery && (time <= this->_congestion.recovery))
		// Выходим из метода - окно уже уменьшено (RFC 9002 §7.3.2)
		return;
	// Начинаем период восстановления
	this->_congestion.inRecovery = true;
	// Устанавливаем время начала периода восстановления
	this->_congestion.recovery = this->_times.now;
	// Уменьшаем порог замедленного старта вдвое
	this->_congestion.threshold = (this->_congestion.window / 2);
	// Уменьшаем окно перегрузки не ниже минимального (RFC 9002 §7.2)
	this->_congestion.window = ::max(this->_congestion.threshold, MINIMUM_WINDOW);
}
/**
 * @brief Метод выдачи дополнительных идентификаторов соединения (RFC 9000 §5.1.1)
 *
 */
void awh::quic::Connection::issue() noexcept {
	// Вычисляем целевое количество активных идентификаторов (включая идентификатор хендшейка)
	const uint64_t target = ::min(this->_transport.remote.activeConnectionIdLimit, MAX_ISSUED_CIDS);
	/**
	 * Выдаём идентификаторы до достижения целевого количества
	 */
	while((this->_cids.routing.issued.size() + (this->_cids.routing.retired ? 0 : 1)) < target){
		// Формируем фрейм анонса нового идентификатора соединения
		frame::new_connection_id_t frame;
		// Устанавливаем порядковый номер идентификатора
		frame.seq = this->_cids.routing.issuedSeq;
		// Идентификаторы из обращения не выводятся
		frame.retirePriorTo = 0;
		// Выполняем генерацию нового идентификатора соединения
		if(!::makeCid(frame.cid))
			// Выходим из метода - ошибка генератора случайных чисел
			return;
		/**
		 * Выводим токен сброса из идентификатора соединения на общем ключе: только
		 * так токен воспроизводится после утраты состояния соединения. Без заданного
		 * ключа токен генерируется случайно - соединение работает обычным порядком,
		 * но сброс без сохранения состояния для него невозможен (RFC 9000 §10.3.2)
		 */
		if(!quic::resetToken(this->_token.reset, frame.cid, frame.resetToken)){
			// Выполняем генерацию случайного токена сброса без сохранения состояния
			if(::RAND_bytes(frame.resetToken, proto::RESET_TOKEN_SIZE) != 1)
				// Выходим из метода - ошибка генератора случайных чисел
				return;
		}
		// Сохраняем выданный идентификатор соединения
		this->_cids.routing.issued.emplace(frame.seq, frame);
		// Отмечаем идентификатор введённым в обращение для маршрутизации
		this->_cids.routing.added.push_back(frame.cid);
		// Ставим порядковый номер в очередь отправки фрейма NEW_CONNECTION_ID
		this->_cids.routing.issueQueue.push_back(frame.seq);
		// Продвигаем порядковый номер следующего выдаваемого идентификатора
		this->_cids.routing.issuedSeq++;
	}
}
/**
 * @brief Метод проверки закреплённости идентификатора за предпочтительным адресом (RFC 9000 §5.1.1)
 *
 * @param seq порядковый номер идентификатора соединения
 * @return    результат проверки
 */
bool awh::quic::Connection::reserved(const uint64_t seq) const noexcept {
	// Выводим результат проверки закреплённости идентификатора за предпочтительным адресом
	return ((seq == 1) && this->_transport.remote.hasPreferredAddress && !this->_cids.identity.relocated);
}
/**
 * @brief Метод проверки возможности отправки данных в поток локальным эндпоинтом
 *
 * @param sid идентификатор потока
 * @return    результат проверки (true - отправка допустима)
 */
bool awh::quic::Connection::sendable(const uint64_t sid) const noexcept {
	// Если поток двунаправленный - отправка допустима всегда
	if((sid & 0x02) == 0)
		// Выводим положительный результат
		return true;
	// Однонаправленный поток: отправка допустима только инициатору (RFC 9000 §2.1)
	return ((sid & 0x01) == ((this->_core.endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
}
/**
 * @brief Метод проверки возможности приёма данных потока локальным эндпоинтом
 *
 * @param sid идентификатор потока
 * @return    результат проверки (true - приём допустим)
 */
bool awh::quic::Connection::receivable(const uint64_t sid) const noexcept {
	// Если поток двунаправленный - приём допустим всегда
	if((sid & 0x02) == 0)
		// Выводим положительный результат
		return true;
	// Однонаправленный поток: приём допустим только не инициатору (RFC 9000 §2.1)
	return ((sid & 0x01) != ((this->_core.endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
}
/**
 * @brief Метод получения начального лимита приёма потока из локальных параметров
 *
 * @param sid идентификатор потока
 * @return    начальный лимит приёма потока в октетах
 */
uint64_t awh::quic::Connection::rxWindow(const uint64_t sid) const noexcept {
	// Определяем инициатора потока
	const bool local = ((sid & 0x01) == ((this->_core.endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
	// Если поток однонаправленный
	if((sid & 0x02) != 0)
		// Выводим лимит однонаправленных потоков (приём только от удалённого инициатора)
		return (local ? 0 : this->_transport.local.initialMaxStreamDataUni);
	// Двунаправленный поток: лимит зависит от инициатора (RFC 9000 §18.2)
	return (local ? this->_transport.local.initialMaxStreamDataBidiLocal : this->_transport.local.initialMaxStreamDataBidiRemote);
}
/**
 * @brief Метод получения начального лимита отправки потока из параметров удалённого эндпоинта
 *
 * @param sid идентификатор потока
 * @return    начальный лимит отправки потока в октетах
 */
uint64_t awh::quic::Connection::txWindow(const uint64_t sid) const noexcept {
	// Определяем инициатора потока
	const bool local = ((sid & 0x01) == ((this->_core.endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
	// Если поток однонаправленный
	if((sid & 0x02) != 0)
		// Выводим лимит однонаправленных потоков (отправка только локальным инициатором)
		return (local ? this->_transport.remote.initialMaxStreamDataUni : 0);
	// Двунаправленный поток: лимит зависит от инициатора (RFC 9000 §18.2)
	return (local ? this->_transport.remote.initialMaxStreamDataBidiRemote : this->_transport.remote.initialMaxStreamDataBidiLocal);
}
/**
 * @brief Метод поиска либо создания потока по принятому фрейму (RFC 9000 §3.2)
 *
 * @param sid   идентификатор потока
 * @param error код ошибки транспорта
 * @return      состояние потока (nullptr - нарушение протокола)
 */
awh::quic::Connection::stream_data_t * awh::quic::Connection::accept(const uint64_t sid, error_t & error) noexcept {
	// Ищем поток по идентификатору
	auto i = this->_stream.list.find(sid);
	// Если поток найден
	if(i != this->_stream.list.end())
		// Выводим состояние потока
		return &i->second;
	// Определяем инициатора потока
	const bool local = ((sid & 0x01) == ((this->_core.endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
	// Если фрейм пришёл для неоткрытого локального потока (RFC 9000 §19.8)
	if(local){
		// Устанавливаем код ошибки состояния потока
		error = error_t::STREAM_STATE_ERROR;
		// Выводим пустой результат
		return nullptr;
	}
	// Определяем направленность потока
	const bool unidirectional = ((sid & 0x02) != 0);
	// Вычисляем порядковый номер потока
	const uint64_t index = (sid >> 2);
	// Получаем анонсированный лимит потоков удалённого эндпоинта
	const uint64_t limit = (unidirectional ? this->_limits.maxUniLocal : this->_limits.maxBidiLocal);
	// Если порядковый номер превышает анонсированный лимит (RFC 9000 §4.6)
	if(index >= limit){
		// Устанавливаем код ошибки превышения лимита потоков
		error = error_t::STREAM_LIMIT_ERROR;
		// Выводим пустой результат
		return nullptr;
	}
	// Получаем счётчик принятых потоков удалённого эндпоинта
	uint64_t & accepted = (unidirectional ? this->_limits.acceptedUni : this->_limits.acceptedBidi);
	// Если открыт поток с наибольшим порядковым номером
	if((index + 1) > accepted)
		// Обновляем счётчик принятых потоков (потоки с меньшими номерами открываются неявно)
		accepted = (index + 1);
	// Создаём состояние нового потока удалённого эндпоинта
	auto ret = this->_stream.list.emplace(sid, stream_data_t());
	// Получаем состояние созданного потока
	stream_data_t * stream = &ret.first->second;
	// Устанавливаем начальный лимит приёма потока
	stream->rxMax = this->rxWindow(sid);
	// Устанавливаем начальный лимит отправки потока
	stream->txMax = this->txWindow(sid);
	// Выводим состояние потока
	return stream;
}
/**
 * @brief Метод обработки принятого фрейма STREAM (RFC 9000 §19.8)
 *
 * @param frame принятый фрейм данных потока приложения
 * @return      результат обработки (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::inputStream(const frame::stream_t & frame) noexcept {
	// Если приём данных потока локальным эндпоинтом недопустим (RFC 9000 §4.6)
	if(!this->receivable(frame.streamId)){
		// Ставим завершение соединения с ошибкой состояния потока в очередь
		this->fail(error_t::STREAM_STATE_ERROR);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Получаем состояние потока (с неявным созданием)
	stream_data_t * stream = this->accept(frame.streamId, error);
	// Если идентификатор потока нарушает протокол
	if(stream == nullptr){
		// Ставим завершение соединения с кодом ошибки в очередь
		this->fail(error);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Вычисляем конечное смещение данных фрейма
	const uint64_t end = (frame.offset + frame.data.size());
	// Если поток уже завершён и данные выходят за финальный размер (RFC 9000 §4.5)
	if(stream->rxFin && ((end > stream->rxFinal) || (frame.fin && (end != stream->rxFinal)))){
		// Ставим завершение соединения с ошибкой финального размера в очередь
		this->fail(error_t::FINAL_SIZE_ERROR);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Если завершение потока указывает финальный размер меньше принятых данных
	if(frame.fin && (end < stream->rxHigh)){
		// Ставим завершение соединения с ошибкой финального размера в очередь
		this->fail(error_t::FINAL_SIZE_ERROR);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Если данные превышают анонсированный лимит приёма потока (RFC 9000 §4.1)
	if(end > stream->rxMax){
		// Ставим завершение соединения с ошибкой flow control в очередь
		this->fail(error_t::FLOW_CONTROL_ERROR);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Прирост наибольшего принятого смещения данных потока
	uint64_t delta = 0;
	// Если принято новое наибольшее смещение данных потока
	if(end > stream->rxHigh){
		// Вычисляем прирост данных потока
		delta = (end - stream->rxHigh);
		// Учитываем прирост данных в flow control соединения
		this->_flow.rxData += delta;
		// Обновляем наибольшее принятое смещение данных потока
		stream->rxHigh = end;
		// Если данные превышают анонсированный лимит приёма соединения (RFC 9000 §4.1)
		if(this->_flow.rxData > this->_flow.rxMax){
			// Ставим завершение соединения с ошибкой flow control в очередь
			this->fail(error_t::FLOW_CONTROL_ERROR);
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
	}
	// Если принято завершение потока (FIN)
	if(frame.fin){
		// Устанавливаем флаг наличия финального размера потока
		stream->rxFin = true;
		// Устанавливаем финальный размер потока
		stream->rxFinal = end;
	}
	// Если поток сброшен удалённым эндпоинтом либо прекращён локально - данные отбрасываются
	if(stream->rxReset || stream->stopQueued || stream->stopSent){
		// Учитываем отброшенные данные как потреблённые в flow control соединения
		this->consume(* stream, stream->rxHigh);
		/**
		 * Если принят финальный размер потока - приём завершён окончательно. Данные
		 * приложению не выдаются, поэтому лимит потоков нужно вернуть здесь: иначе
		 * кредит MAX_STREAMS не возвращается никогда (RFC 9000 §4.6)
		 */
		if(stream->rxFin)
			// Учитываем завершение потока удалённого эндпоинта в лимите MAX_STREAMS
			this->credit(frame.streamId, * stream);
		// Обновляем готовность потока к выдаче данных приложению
		this->notify(frame.streamId, * stream);
		// Выводим положительный результат
		return status_t::OK;
	}
	// Если данные фрейма уже полностью собраны
	if(!frame.data.empty() && (end > stream->rxOffset)){
		// Если данные фрейма опережают непрерывно собранное смещение
		if(frame.offset > stream->rxOffset){
			// Ищем буферизированный фрагмент с тем же смещением
			auto i = stream->rxBuffer.find(frame.offset);
			/**
			 * Если фрагмент с тем же смещением уже буферизирован - сверяем перекрытие:
			 * октет потока по своему смещению один и тот же сколько бы раз он ни был
			 * прислан, и расхождение означает неисправный либо злонамеренный удалённый
			 * эндпоинт. Без сверки собранные данные зависели бы от порядка приёма
			 * фрагментов, что для потока недопустимо (RFC 9000 §2.2)
			 */
			if((i != stream->rxBuffer.end()) &&
			   (::memcmp(i->second.data(), frame.data.data(), ::min(i->second.size(), frame.data.size())) != 0)){
				// Ставим завершение соединения с нарушением протокола в очередь
				this->fail(error_t::PROTOCOL_VIOLATION);
				// Выводим отрицательный результат
				return status_t::ERROR;
			}
			// Если фрагмент с тем же смещением не буферизирован либо новый фрагмент длиннее
			if((i == stream->rxBuffer.end()) || (i->second.size() < frame.data.size())){
				/**
				 * Если новый фрагмент образует разрыв сверх предела: удалённый эндпоинт
				 * дробит поток мельче, чем локальный готов собирать. Данные лимитом приёма
				 * ограничены, но накладные расходы на хранение каждого фрагмента их
				 * многократно превышают, поэтому предел нужен и на дробление
				 */
				if((i == stream->rxBuffer.end()) && (stream->rxBuffer.size() >= ::max(MIN_STREAM_CHUNKS, static_cast <size_t> (this->rxWindow(frame.streamId) / STREAM_CHUNK_SHARE)))){
					// Ставим завершение соединения с ошибкой flow control в очередь
					this->fail(error_t::FLOW_CONTROL_ERROR);
					// Выводим отрицательный результат
					return status_t::ERROR;
				}
				// Буферизируем фрагмент до заполнения разрыва
				stream->rxBuffer[frame.offset] = string(frame.data);
			}
		// Если данные продолжают непрерывно собранное смещение
		} else {
			// Определяем несобранную часть данных фрейма
			string_view data = frame.data;
			// Отбрасываем уже собранную часть данных фрейма
			data.remove_prefix(static_cast <size_t> (stream->rxOffset - frame.offset));
			// Дописываем данные в буфер выдачи приложению
			stream->rxReady.append(data);
			// Продвигаем непрерывно собранное смещение
			stream->rxOffset = end;
			/**
			 *  Выполняем сборку буферизированных фрагментов
			 */
			while(!stream->rxBuffer.empty()){
				// Получаем фрагмент с наименьшим смещением
				auto i = stream->rxBuffer.begin();
				// Если фрагмент опережает собранное смещение - разрыв не заполнен
				if(i->first > stream->rxOffset)
					// Выходим из цикла сборки
					break;
				// Вычисляем конечное смещение фрагмента
				const uint64_t chunkEnd = (i->first + i->second.size());
				// Если фрагмент содержит ещё не собранные данные
				if(chunkEnd > stream->rxOffset){
					// Определяем несобранную часть фрагмента
					string_view chunk(i->second);
					// Отбрасываем уже собранную часть фрагмента
					chunk.remove_prefix(static_cast <size_t> (stream->rxOffset - i->first));
					// Дописываем данные в буфер выдачи приложению
					stream->rxReady.append(chunk);
					// Продвигаем непрерывно собранное смещение
					stream->rxOffset = chunkEnd;
				}
				// Удаляем собранный фрагмент из буфера
				stream->rxBuffer.erase(i);
			}
		}
	}
	// Обновляем готовность потока к выдаче данных приложению
	this->notify(frame.streamId, * stream);
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод применения транспортных параметров удалённого эндпоинта после хендшейка
 *
 * @return результат применения (true - параметры применены)
 */
bool awh::quic::Connection::established() noexcept {
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Извлекаем транспортные параметры удалённого эндпоинта
	if(this->_crypto.handshake.peer(this->_transport.remote, error) != status_t::OK){
		// Ставим завершение соединения с ошибкой транспортных параметров в очередь
		this->fail((error != error_t::NO_ERROR) ? error : error_t::TRANSPORT_PARAMETER_ERROR);
		// Выводим отрицательный результат
		return false;
	}
	// Если SCID первого пакета удалённого эндпоинта отсутствует или не совпадает (RFC 9000 §7.3)
	if(!this->_transport.remote.hasInitialScid || !(this->_transport.remote.initialScid == this->_cids.identity.destination)){
		// Ставим завершение соединения с ошибкой транспортных параметров в очередь
		this->fail(error_t::TRANSPORT_PARAMETER_ERROR);
		// Выводим отрицательный результат
		return false;
	}
	// Если локальный эндпоинт является клиентом
	if(this->_core.endpoint == endpoint_t::CLIENT){
		// Если исходный DCID отсутствует или не совпадает с отправленным (RFC 9000 §7.3)
		if(!this->_transport.remote.hasOdcid || !(this->_transport.remote.odcid == this->_cids.identity.original)){
			// Ставим завершение соединения с ошибкой транспортных параметров в очередь
			this->fail(error_t::TRANSPORT_PARAMETER_ERROR);
			// Выводим отрицательный результат
			return false;
		}
		// Если наличие SCID пакета Retry не соответствует обработанному Retry (RFC 9000 §7.3)
		if(this->_cids.identity.retried ? (!this->_transport.remote.hasRetryScid || !(this->_transport.remote.retryScid == this->_cids.identity.retry)) : this->_transport.remote.hasRetryScid){
			// Ставим завершение соединения с ошибкой транспортных параметров в очередь
			this->fail(error_t::TRANSPORT_PARAMETER_ERROR);
			// Выводим отрицательный результат
			return false;
		}
	}
	// Формируем запись идентификатора хендшейка удалённого эндпоинта (RFC 9000 §5.1.1)
	remote_cid_t cid;
	// Устанавливаем нулевой порядковый номер идентификатора хендшейка
	cid.seq = 0;
	// Устанавливаем идентификатор соединения удалённого эндпоинта
	cid.cid = this->_cids.identity.destination;
	// Устанавливаем флаг использования идентификатора в качестве DCID
	cid.used = true;
	/**
	 * Токен сброса идентификатора хендшейка приходит транспортным параметром
	 * сервера: у клиента он есть, у сервера токена клиента не бывает (RFC 9000 §18.2)
	 */
	if(this->_transport.remote.hasResetToken){
		// Копируем токен сброса без сохранения состояния
		::memcpy(cid.resetToken, this->_transport.remote.resetToken, proto::RESET_TOKEN_SIZE);
		// Устанавливаем флаг наличия токена сброса
		cid.hasToken = true;
	}
	// Добавляем идентификатор хендшейка в список
	this->_cids.routing.remote.push_back(cid);
	/**
	 * Если сервер анонсировал предпочтительный адрес: идентификатор из него
	 * вводится в обращение с порядковым номером 1 - именно на него клиент
	 * переключается при переезде на предпочтительный адрес (RFC 9000 §5.1.1)
	 */
	if((this->_core.endpoint == endpoint_t::CLIENT) && this->_transport.remote.hasPreferredAddress){
		// Формируем запись идентификатора предпочтительного адреса
		remote_cid_t preferred;
		// Устанавливаем порядковый номер идентификатора предпочтительного адреса
		preferred.seq = 1;
		// Устанавливаем идентификатор соединения предпочтительного адреса
		preferred.cid = this->_transport.remote.preferredAddress.cid;
		// Копируем токен сброса без сохранения состояния идентификатора
		::memcpy(preferred.resetToken, this->_transport.remote.preferredAddress.resetToken, proto::RESET_TOKEN_SIZE);
		// Устанавливаем флаг наличия токена сброса
		preferred.hasToken = true;
		// Добавляем идентификатор предпочтительного адреса в список
		this->_cids.routing.remote.push_back(preferred);
	}
	/**
	 * Если локальный эндпоинт анонсировал предпочтительный адрес: выданный вместе
	 * с адресом идентификатор занимает порядковый номер 1, поэтому он вводится
	 * в обращение наравне с остальными, а выдача следующих начинается со второго.
	 * Отправлять его фреймом NEW_CONNECTION_ID не нужно - удалённый узел получил
	 * его транспортным параметром (RFC 9000 §5.1.1)
	 */
	if((this->_core.endpoint == endpoint_t::SERVER) && this->_transport.local.hasPreferredAddress){
		// Формируем запись выданного идентификатора предпочтительного адреса
		frame::new_connection_id_t preferred;
		// Устанавливаем порядковый номер идентификатора предпочтительного адреса
		preferred.seq = 1;
		// Идентификаторы из обращения не выводятся
		preferred.retirePriorTo = 0;
		// Устанавливаем идентификатор соединения предпочтительного адреса
		preferred.cid = this->_transport.local.preferredAddress.cid;
		// Копируем токен сброса без сохранения состояния идентификатора
		::memcpy(preferred.resetToken, this->_transport.local.preferredAddress.resetToken, proto::RESET_TOKEN_SIZE);
		// Сохраняем выданный идентификатор предпочтительного адреса
		this->_cids.routing.issued.emplace(preferred.seq, preferred);
		// Отмечаем идентификатор введённым в обращение для маршрутизации
		this->_cids.routing.added.push_back(preferred.cid);
		// Если порядковый номер следующего выдаваемого идентификатора не продвинут
		if(this->_cids.routing.issuedSeq < 2)
			// Продвигаем порядковый номер за номером предпочтительного адреса
			this->_cids.routing.issuedSeq = 2;
	}
	// Выдаём дополнительные идентификаторы соединения (RFC 9000 §5.1.1)
	this->issue();
	// Устанавливаем лимит отправки данных соединения от удалённого эндпоинта
	this->_flow.txMax = this->_transport.remote.initialMaxData;
	// Устанавливаем лимит на локально открываемые двунаправленные потоки
	this->_limits.maxBidiRemote = this->_transport.remote.initialMaxStreamsBidi;
	// Устанавливаем лимит на локально открываемые однонаправленные потоки
	this->_limits.maxUniRemote = this->_transport.remote.initialMaxStreamsUni;
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод учёта завершения потока удалённого эндпоинта в лимите MAX_STREAMS
 *
 * @param sid    идентификатор потока
 * @param stream состояние потока
 */
void awh::quic::Connection::credit(const uint64_t sid, stream_data_t & stream) noexcept {
	// Если завершение потока уже учтено
	if(stream.credited)
		// Выходим из метода
		return;
	// Если поток открыт локальным эндпоинтом - лимит не расходовался
	if((sid & 0x01) == ((this->_core.endpoint == endpoint_t::SERVER) ? 0x01 : 0x00))
		// Выходим из метода
		return;
	// Устанавливаем флаг учтённого завершения потока
	stream.credited = true;
	// Если поток однонаправленный
	if((sid & 0x02) != 0){
		// Увеличиваем анонсированный лимит однонаправленных потоков
		this->_limits.maxUniLocal++;
		// Устанавливаем флаг необходимости отправки обновлённого лимита MAX_STREAMS
		this->_limits.uniQueued = true;
	// Если поток двунаправленный
	} else {
		// Увеличиваем анонсированный лимит двунаправленных потоков
		this->_limits.maxBidiLocal++;
		// Устанавливаем флаг необходимости отправки обновлённого лимита MAX_STREAMS
		this->_limits.bidiQueued = true;
	}
}
/**
 * @brief Метод проверки готовности потока к выдаче данных приложению
 *
 * @param stream состояние потока
 * @return       результат проверки (true - есть что выдать приложению)
 */
bool awh::quic::Connection::ready(const stream_data_t & stream) const noexcept {
	/**
	 * Поток готов к выдаче, когда он не сброшен удалённым эндпоинтом и содержит
	 * собранные данные либо ещё не выданное приложению завершение
	 */
	return (!stream.rxReset && (!stream.rxReady.empty() ||
	 (stream.rxFin && !stream.rxFinDelivered && (stream.rxOffset == stream.rxFinal))));
}
/**
 * @brief Метод постановки потока в список готовых к выдаче
 *
 * @param sid    идентификатор потока
 * @param stream состояние потока
 */
void awh::quic::Connection::notify(const uint64_t sid, stream_data_t & stream) noexcept {
	// Если поток готов к выдаче и в списке готовых ещё не числится
	if(this->ready(stream) && !stream.queued){
		// Отмечаем поток числящимся в списке готовых
		stream.queued = true;
		// Добавляем идентификатор потока в список готовых
		this->_stream.readable.push_back(sid);
	}
}
/**
 * @brief Метод обработки входящих CRYPTO-данных со сборкой по смещениям
 *
 * @param level  уровень шифрования пакета с CRYPTO-фреймом
 * @param offset смещение данных в потоке криптографического хендшейка
 * @param data   данные CRYPTO-фрейма
 * @return       результат обработки (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::input(const level_t level, const uint64_t offset, string_view data) noexcept {
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
	// Вычисляем конечное смещение данных фрейма
	const uint64_t end = (offset + data.size());
	// Если данные фрейма уже полностью собраны
	if(end <= item.rxOffset)
		// Выводим положительный результат - полный дубликат
		return status_t::OK;
	// Если данные фрейма опережают непрерывно собранное смещение
	if(offset > item.rxOffset){
		// Суммарный размер буферизированных данных
		size_t buffered = 0;
		/**
		 * Перебираем буфер сборки входящих CRYPTO-данных
		 */
		for(auto & chunk : item.rxBuffer)
			// Суммируем размер буферизированных данных
			buffered += chunk.second.size();
		// Если лимит буфера сборки превышен (RFC 9000 §7.5)
		if((buffered + data.size()) > MAX_CRYPTO_BUFFER){
			// Ставим завершение соединения с ошибкой переполнения буфера в очередь
			this->fail(error_t::CRYPTO_BUFFER_EXCEEDED);
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
		// Ищем буферизированный фрагмент с тем же смещением
		auto i = item.rxBuffer.find(offset);
		/**
		 * Если фрагмент с тем же смещением уже буферизирован - сверяем перекрытие:
		 * поток криптографического хендшейка переписыванию не подлежит, и расхождение
		 * данных по одному смещению является нарушением протокола (RFC 9000 §2.2)
		 */
		if((i != item.rxBuffer.end()) &&
		   (::memcmp(i->second.data(), data.data(), ::min(i->second.size(), data.size())) != 0)){
			// Ставим завершение соединения с нарушением протокола в очередь
			this->fail(error_t::PROTOCOL_VIOLATION);
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
		// Если фрагмент с тем же смещением не буферизирован либо новый фрагмент длиннее
		if((i == item.rxBuffer.end()) || (i->second.size() < data.size()))
			// Буферизируем фрагмент до заполнения разрыва
			item.rxBuffer[offset] = string(data);
		// Выводим положительный результат - данные буферизированы
		return status_t::OK;
	}
	// Отбрасываем уже собранную часть данных фрейма
	data.remove_prefix(static_cast <size_t> (item.rxOffset - offset));
	// Передаём данные хендшейк-машине
	if(this->_crypto.handshake.crypto(level, reinterpret_cast <const uint8_t *> (data.data()), data.size()) != status_t::OK){
		// Ставим завершение соединения с ошибкой хендшейка в очередь
		this->fail(this->_crypto.handshake.error());
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Продвигаем непрерывно собранное смещение
	item.rxOffset = end;
	/**
	 *  Выполняем сборку буферизированных фрагментов
	 */
	while(!item.rxBuffer.empty()){
		// Получаем фрагмент с наименьшим смещением
		auto i = item.rxBuffer.begin();
		// Если фрагмент опережает собранное смещение - разрыв не заполнен
		if(i->first > item.rxOffset)
			// Выходим из цикла сборки
			break;
		// Вычисляем конечное смещение фрагмента
		const uint64_t chunkEnd = (i->first + i->second.size());
		// Если фрагмент содержит ещё не собранные данные
		if(chunkEnd > item.rxOffset){
			// Определяем несобранную часть фрагмента
			string_view chunk(i->second);
			// Отбрасываем уже собранную часть фрагмента
			chunk.remove_prefix(static_cast <size_t> (item.rxOffset - i->first));
			// Передаём данные хендшейк-машине
			if(this->_crypto.handshake.crypto(level, reinterpret_cast <const uint8_t *> (chunk.data()), chunk.size()) != status_t::OK){
				// Ставим завершение соединения с ошибкой хендшейка в очередь
				this->fail(this->_crypto.handshake.error());
				// Выводим отрицательный результат
				return status_t::ERROR;
			}
			// Продвигаем непрерывно собранное смещение
			item.rxOffset = chunkEnd;
		}
		// Удаляем собранный фрагмент из буфера
		item.rxBuffer.erase(i);
	}
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод разбора и диспетчеризации фреймов расшифрованной нагрузки пакета
 *
 * @param level уровень шифрования пакета
 * @param data  буфер расшифрованной нагрузки
 * @param size  размер расшифрованной нагрузки
 * @return      результат разбора (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::frames(const level_t level, const uint8_t * data, const size_t size, bool & nonProbing) noexcept {
	// Смещение в буфере расшифрованной нагрузки
	size_t offset = 0;
	// Флаг приёма ack-eliciting фрейма (RFC 9000 §13.2.1)
	bool elicit = false;
	/**
	 *  Перебираем фреймы расшифрованной нагрузки
	 */
	while(offset < size){
		// Тип очередного фрейма
		frame_t type = frame_t::UNKNOWN;
		// Определяем тип очередного фрейма
		if(!frame::parser::type(data + offset, size - offset, type)){
			// Ставим завершение соединения с ошибкой кодирования фрейма в очередь
			this->fail(error_t::FRAME_ENCODING_ERROR);
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
		/**
		 * Определяем непробирующий характер пакета: пакет считается непробирующим,
		 * если несёт хотя бы один фрейм помимо PADDING, PATH_CHALLENGE, PATH_RESPONSE
		 * и NEW_CONNECTION_ID. Только такой пакет с наибольшим номером вправе вызвать
		 * миграцию соединения на новый путь (RFC 9000 §9.1)
		 */
		switch(type){
			// Пробирующие фреймы миграцию не инициируют
			case frame_t::PADDING:
			case frame_t::PATH_CHALLENGE:
			case frame_t::PATH_RESPONSE:
			case frame_t::NEW_CONNECTION_ID:
			break;
			// Любой прочий фрейм делает пакет непробирующим
			default: nonProbing = true;
		}
		// Если пакет уровня ранних данных
		if(level == level_t::EARLY_DATA){
			/**
			 * Определяем допустимость фрейма на уровне (RFC 9000 §12.4 таблица 3)
			 */
			switch(type){
				/**
				 * Недопустимые фреймы уровня ранних данных: подтверждения, данные
				 * хендшейка и всё, что относится к состоянию, согласуемому хендшейком.
				 * Ранние данные отправляются до его завершения, поэтому нести такое
				 * они не могут
				 */
				case frame_t::ACK:
				case frame_t::ACK_ECN:
				case frame_t::CRYPTO:
				case frame_t::NEW_TOKEN:
				case frame_t::PATH_RESPONSE:
				case frame_t::HANDSHAKE_DONE: {
					// Ставим завершение соединения с нарушением протокола в очередь
					this->fail(error_t::PROTOCOL_VIOLATION);
					// Выводим отрицательный результат
					return status_t::ERROR;
				}
				// Все остальные фреймы допустимы
				default: break;
			}
		// Если пакет уровня Initial или Handshake
		} else if((level == level_t::INITIAL) || (level == level_t::HANDSHAKE)) {
			/**
			 * Определяем допустимость фрейма на уровне (RFC 9000 §12.4 таблица 3)
			 */
			switch(type){
				// Допустимые фреймы уровней Initial и Handshake
				case frame_t::PADDING:
				case frame_t::PING:
				case frame_t::ACK:
				case frame_t::ACK_ECN:
				case frame_t::CRYPTO:
				case frame_t::CONNECTION_CLOSE:
					// Продолжаем обработку
					break;
				// Все остальные фреймы недопустимы
				default: {
					// Ставим завершение соединения с нарушением протокола в очередь
					this->fail(error_t::PROTOCOL_VIOLATION);
					// Выводим отрицательный результат
					return status_t::ERROR;
				}
			}
		}
		// Количество потреблённых октетов фрейма
		size_t consumed = 0;
		// Код ошибки транспорта разбора фрейма
		error_t error = error_t::NO_ERROR;
		// Результат разбора фрейма
		status_t status = status_t::OK;
		/**
		 * Определяем тип фрейма
		 */
		switch(type){
			// Фрейм заполнения PADDING
			case frame_t::PADDING:
				// Выполняем разбор серии фреймов PADDING
				status = frame::parser::padding(data + offset, size - offset, consumed, error);
			break;
			// Фрейм проверки живости PING
			case frame_t::PING: {
				// Выполняем разбор фрейма PING
				status = frame::parser::ping(data + offset, size - offset, consumed, error);
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фреймы подтверждения приёма пакетов ACK/ACK_ECN
			case frame_t::ACK:
			case frame_t::ACK_ECN: {
				// Разобранный фрейм подтверждения
				frame::ack_t frame;
				// Выполняем разбор фрейма ACK
				status = frame::parser::ack(data + offset, size - offset, frame, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Получаем пространство номеров пакетов уровня
					const space_t space = this->space(level);
					// Получаем состояние пространства номеров пакетов
					auto & item = this->_spaces[static_cast <size_t> (space)];
					// Если подтверждён номер пакета, который не отправлялся (RFC 9000 §13.1)
					if(!frame.ranges.empty() && (frame.ranges.front().high >= item.txPn)){
						// Ставим завершение соединения с нарушением протокола в очередь
						this->fail(error_t::PROTOCOL_VIOLATION);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Если подтверждён наибольший номер пакета
					if(!frame.ranges.empty() && (!item.hasAcked || (frame.ranges.front().high > item.largestAcked))){
						// Обновляем наибольший подтверждённый номер пакета
						item.largestAcked = frame.ranges.front().high;
						// Устанавливаем флаг наличия подтверждений от пира
						item.hasAcked = true;
					}
					// Флаг наличия впервые подтверждённых пакетов
					bool acked = false;
					// Количество впервые подтверждённых пакетов с маркировкой поддержки ECN
					uint64_t marked = 0;
					/**
					 * Время отправки наиболее позднего подтверждённого пакета до обработки
					 * текущего фрейма: именно оно ограничивает период устойчивой перегрузки
					 * снизу. Пакеты, подтверждаемые этим фреймом, отправлены уже после
					 * потерянных и границей периода служить не могут (RFC 9002 §7.6)
					 */
					const uint64_t priorAcked = this->_congestion.acked;
					// Флаг наличия подтверждённых пакетов до обработки текущего фрейма
					const bool hasPriorAcked = this->_congestion.hasAcked;
					// Время отправки наиболее позднего пакета, подтверждаемого текущим фреймом
					uint64_t newestAcked = 0;
					// Время отправки наибольшего впервые подтверждённого пакета
					uint64_t sentTime = 0;
					// Флаг подтверждения наибольшего номера пакета фрейма
					bool largest = false;
					/**
					 * Сопоставляем отправленные пакеты с диапазонами подтверждения одним
					 * проходом: список отправленных упорядочен по возрастанию номера,
					 * диапазоны фрейма - по убыванию, поэтому индекс диапазона движется
					 * от последнего к первому синхронно с перебором пакетов
					 */
					size_t index = frame.ranges.size();
					// Позиция записи неподтверждённых пакетов в списке отправленных
					auto position = item.sent.begin();
					/**
					 * Перебираем список отправленных пакетов
					 */
					for(auto i = item.sent.begin(); i != item.sent.end(); ++i){
						// Пропускаем диапазоны, целиком лежащие ниже номера пакета
						while((index > 0) && (frame.ranges[index - 1].high < i->pn))
							// Переходим к следующему диапазону в порядке возрастания
							index--;
						// Определяем вхождение номера пакета в текущий диапазон подтверждения
						const bool found = ((index > 0) && (i->pn >= frame.ranges[index - 1].low) && (i->pn <= frame.ranges[index - 1].high));
						// Если приём пакета не подтверждён
						if(!found){
							// Если позиция записи отстала от позиции чтения
							if(position != i)
								// Сдвигаем пакет к позиции записи
								(* position) = ::move(* i);
							// Продвигаем позицию записи
							++position;
							// Переходим к следующему пакету
							continue;
						}
						// Устанавливаем флаг наличия впервые подтверждённых пакетов
						acked = true;
						// Если подтверждённый пакет был отправлен с маркировкой поддержки ECN
						if(i->ecn)
							// Считаем впервые подтверждённый помеченный пакет
							marked++;
						// Если подтверждён наибольший номер пакета фрейма
						if(i->pn == frame.ranges.front().high){
							// Запоминаем время отправки пакета
							sentTime = i->time;
							/**
							 * Устанавливаем флаг подтверждения наибольшего номера: подтверждение
							 * пакета прежнего пути измеряет задержку не того пути, по которому
							 * соединение работает теперь, и образцом для оценки не является
							 * (RFC 9000 §9.4)
							 */
							largest = !i->stale;
						}
						// Запоминаем время отправки наиболее позднего подтверждаемого пакета
						newestAcked = ::max(newestAcked, i->time);
						// Учитываем подтверждённый пакет в congestion control (RFC 9002 §7.3.1)
						this->acked(* i);
					}
					// Удаляем подтверждённые пакеты из списка отправленных
					item.sent.erase(position, item.sent.end());
					// Если впервые подтверждены отправленные пакеты
					if(acked){
						// Сбрасываем счётчик срабатываний таймера PTO (RFC 9002 §6.2.1)
						this->_rtt.ptoCount = 0;
						// Если подтверждён наибольший номер и время отправки известно
						if(largest && (this->_times.now >= sentTime)){
							// Задержка подтверждения удалённого эндпоинта в миллисекундах
							uint64_t delay = 0;
							// Если пространство пакетов приложения (RFC 9002 §5.3)
							if(space == space_t::APPLICATION){
								// Получаем показатель степени задержки удалённого эндпоинта (RFC 9000 §18.2)
								const uint64_t exponent = this->_transport.remote.ackDelayExponent;
								// Если сдвиг не переполняет разрядность
								if(frame.delay <= (proto::VARINT_MAX >> exponent))
									// Вычисляем задержку подтверждения в миллисекундах
									delay = ((frame.delay << exponent) / 1000);
								// Задержка закодирована некорректно - используем максимальную
								else delay = this->_transport.remote.maxAckDelay;
								// Если хендшейк подтверждён - ограничиваем задержку максимумом пира (RFC 9002 §5.3)
								if((this->_core.flags & flags::CONFIRMED) && (delay > this->_transport.remote.maxAckDelay))
									// Ограничиваем задержку анонсированным максимумом
									delay = this->_transport.remote.maxAckDelay;
							}
							// Выполняем обновление оценки задержки приёма-передачи
							this->rtt(this->_times.now - sentTime, delay);
						}
						/**
						 * Если подтверждён наибольший номер пакета фрейма, выполняем проверку
						 * пути на поддержку ECN: подтверждение, не продвинувшее наибольший
						 * номер, счётчиков не обновляет и проверку провалить не вправе
						 * (RFC 9000 §13.4.2.1). Прирост счётчика перегрузки означает, что
						 * маршрутизатор на пути отметил наши пакеты, не отбросив их - сигнал
						 * сократить окно ровно как при потере, но раньше и без утраты данных
						 * (RFC 9002 §7.7). Обработка идёт по времени отправки наибольшего
						 * подтверждённого пакета, поэтому повторный сигнал в том же
						 * периоде восстановления окно не режет
						 */
						if(largest && this->validate(space, frame, marked))
							// Обрабатываем перегрузку пути по сигналу удалённого эндпоинта
							this->congestion(sentTime);
						// Выполняем детект потерянных пакетов пространства
						this->detect(space);
						// Обновляем время отправки наиболее позднего подтверждённого пакета
						if(!hasPriorAcked || (newestAcked > priorAcked)){
							// Устанавливаем время отправки подтверждённого пакета
							this->_congestion.acked = newestAcked;
							// Устанавливаем флаг наличия подтверждённых пакетов
							this->_congestion.hasAcked = true;
						}
					}
				}
			} break;
			// Фрейм данных криптографического хендшейка CRYPTO
			case frame_t::CRYPTO: {
				// Разобранный фрейм криптографического хендшейка
				frame::crypto_t frame;
				// Выполняем разбор фрейма CRYPTO
				status = frame::parser::crypto(data + offset, size - offset, frame, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Выполняем обработку данных криптографического хендшейка
					if(this->input(level, frame.offset, frame.data) != status_t::OK)
						// Выводим отрицательный результат
						return status_t::ERROR;
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм токена будущих соединений NEW_TOKEN
			case frame_t::NEW_TOKEN: {
				// Токен для будущих соединений
				string_view token;
				// Выполняем разбор фрейма NEW_TOKEN
				status = frame::parser::newToken(data + offset, size - offset, token, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Если фрейм прислал клиент (RFC 9000 §19.7)
					if(this->_core.endpoint == endpoint_t::SERVER){
						// Ставим завершение соединения с нарушением протокола в очередь
						this->fail(error_t::PROTOCOL_VIOLATION);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					/**
					 * Запоминаем токен проверки адреса для будущих соединений: сервер
					 * вправе прислать несколько токенов, и годен любой из них -
					 * оставляем последний присланный (RFC 9000 §8.1.3)
					 */
					this->_token.address.assign(token);
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм ненадёжно доставляемой датаграммы приложения DATAGRAM
			case frame_t::DATAGRAM: {
				// Данные принятой датаграммы приложения
				string_view payload;
				// Выполняем разбор фрейма DATAGRAM
				status = frame::parser::datagram(data + offset, size - offset, payload, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					/**
					 * Если приём датаграмм локальным эндпоинтом не анонсирован: отправка
					 * фрейма удалённым узлом без анонса является нарушением протокола
					 * (RFC 9221 §3)
					 */
					if(this->_transport.local.maxDatagramFrameSize == 0){
						// Ставим завершение соединения с нарушением протокола в очередь
						this->fail(error_t::PROTOCOL_VIOLATION);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Если размер принятого фрейма превышает анонсированный предел
					if(consumed > static_cast <size_t> (this->_transport.local.maxDatagramFrameSize)){
						// Ставим завершение соединения с нарушением протокола в очередь
						this->fail(error_t::PROTOCOL_VIOLATION);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					/**
					 * Буферизируем принятую датаграмму до выдачи приложению: очередь
					 * ограничена сверху - датаграммы flow control не подчиняются,
					 * и без предела не читающее их приложение исчерпало бы память
					 * (RFC 9221 §5.3)
					 */
					if(this->_dgram.rx.size() >= MAX_QUEUED_DATAGRAMS)
						// Отбрасываем наиболее старую принятую датаграмму
						this->_dgram.rx.pop_front();
					// Буферизируем принятую датаграмму приложения
					this->_dgram.rx.emplace_back(payload);
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм аварийного завершения потока RESET_STREAM
			case frame_t::RESET_STREAM: {
				// Разобранный фрейм аварийного завершения потока
				frame::reset_stream_t frame;
				// Выполняем разбор фрейма RESET_STREAM
				status = frame::parser::resetStream(data + offset, size - offset, frame, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Если приём данных потока локальным эндпоинтом недопустим (RFC 9000 §19.4)
					if(!this->receivable(frame.streamId)){
						// Ставим завершение соединения с ошибкой состояния потока в очередь
						this->fail(error_t::STREAM_STATE_ERROR);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Получаем состояние потока (с неявным созданием)
					stream_data_t * stream = this->accept(frame.streamId, error);
					// Если идентификатор потока нарушает протокол
					if(stream == nullptr){
						// Ставим завершение соединения с кодом ошибки в очередь
						this->fail(error);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Если финальный размер противоречит принятым данным (RFC 9000 §4.5)
					if((frame.finalSize < stream->rxHigh) || (stream->rxFin && (frame.finalSize != stream->rxFinal))){
						// Ставим завершение соединения с ошибкой финального размера в очередь
						this->fail(error_t::FINAL_SIZE_ERROR);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Если финальный размер превышает анонсированный лимит приёма потока
					if(frame.finalSize > stream->rxMax){
						// Ставим завершение соединения с ошибкой flow control в очередь
						this->fail(error_t::FLOW_CONTROL_ERROR);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Если финальный размер превышает наибольшее принятое смещение
					if(frame.finalSize > stream->rxHigh){
						// Учитываем прирост данных в flow control соединения
						this->_flow.rxData += (frame.finalSize - stream->rxHigh);
						// Обновляем наибольшее принятое смещение данных потока
						stream->rxHigh = frame.finalSize;
						// Если данные превышают анонсированный лимит приёма соединения
						if(this->_flow.rxData > this->_flow.rxMax){
							// Ставим завершение соединения с ошибкой flow control в очередь
							this->fail(error_t::FLOW_CONTROL_ERROR);
							// Выводим отрицательный результат
							return status_t::ERROR;
						}
					}
					// Устанавливаем флаг аварийного завершения потока удалённым эндпоинтом
					stream->rxReset = true;
					// Устанавливаем код ошибки приложения принятого фрейма RESET_STREAM
					stream->rxResetCode = frame.code;
					// Устанавливаем флаг наличия финального размера потока
					stream->rxFin = true;
					// Устанавливаем финальный размер потока
					stream->rxFinal = frame.finalSize;
					// Учитываем отброшенные данные как потреблённые в flow control соединения
					this->consume(* stream, frame.finalSize);
					// Отбрасываем несобранные фрагменты данных потока
					stream->rxBuffer.clear();
					// Отбрасываем собранные данные потока (RFC 9000 §3.2)
					stream->rxReady.clear();
					// Учитываем завершение потока удалённого эндпоинта в лимите MAX_STREAMS
					this->credit(frame.streamId, * stream);
					// Обновляем готовность потока к выдаче данных приложению
					this->notify(frame.streamId, * stream);
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм запроса прекращения передачи STOP_SENDING
			case frame_t::STOP_SENDING: {
				// Разобранный фрейм запроса прекращения передачи
				frame::stop_sending_t frame;
				// Выполняем разбор фрейма STOP_SENDING
				status = frame::parser::stopSending(data + offset, size - offset, frame, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Если отправка данных в поток локальным эндпоинтом недопустима (RFC 9000 §19.5)
					if(!this->sendable(frame.streamId)){
						// Ставим завершение соединения с ошибкой состояния потока в очередь
						this->fail(error_t::STREAM_STATE_ERROR);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Получаем состояние потока (с неявным созданием)
					stream_data_t * stream = this->accept(frame.streamId, error);
					// Если идентификатор потока нарушает протокол
					if(stream == nullptr){
						// Ставим завершение соединения с кодом ошибки в очередь
						this->fail(error);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Если аварийное завершение отправки ещё не выполнялось
					if(!stream->txReset && !stream->txResetSent){
						// Ставим отправку фрейма RESET_STREAM в очередь (RFC 9000 §3.5)
						stream->txReset = true;
						// Устанавливаем код ошибки приложения фрейма RESET_STREAM
						stream->txResetCode = frame.code;
						// Отбрасываем неотправленные данные потока
						stream->txBuffer.clear();
						/**
						 * Сбрасываем курсор упакованных данных буфера: завершённость отправки
						 * определяется совпадением курсора с размером буфера, и оставленный
						 * ненулевым курсор при опустошённом буфере не совпал бы с ним никогда -
						 * поток остался бы в списке потоков до конца соединения
						 */
						stream->txCursor = 0;
						// Сбрасываем флаг постановки завершения потока
						stream->txFin = false;
					}
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм данных потока приложения STREAM
			case frame_t::STREAM: {
				// Разобранный фрейм данных потока приложения
				frame::stream_t frame;
				// Выполняем разбор фрейма STREAM
				status = frame::parser::stream(data + offset, size - offset, frame, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Выполняем обработку принятого фрейма STREAM
					if(this->inputStream(frame) != status_t::OK)
						// Выводим отрицательный результат
						return status_t::ERROR;
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фреймы с одним целочисленным полем
			case frame_t::MAX_DATA:
			case frame_t::MAX_STREAMS_BIDI:
			case frame_t::MAX_STREAMS_UNI:
			case frame_t::DATA_BLOCKED:
			case frame_t::STREAMS_BLOCKED_BIDI:
			case frame_t::STREAMS_BLOCKED_UNI:
			case frame_t::RETIRE_CONNECTION_ID: {
				// Значение целочисленного поля фрейма
				uint64_t value = 0;
				// Выполняем разбор фрейма
				status = frame::parser::single(data + offset, size - offset, type, value, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					/**
					 * Если фрейм лимита числа потоков разрешает идентификатор сверх
					 * кодируемого varint предела: открыть такой поток невозможно,
					 * и лимит сверх него является ошибкой кодирования
					 * (RFC 9000 §19.11/§19.14)
					 */
					if((value > MAX_STREAMS_LIMIT) &&
					   ((type == frame_t::MAX_STREAMS_BIDI) || (type == frame_t::MAX_STREAMS_UNI) ||
					    (type == frame_t::STREAMS_BLOCKED_BIDI) || (type == frame_t::STREAMS_BLOCKED_UNI))){
						// Ставим завершение соединения с ошибкой кодирования фрейма в очередь
						this->fail(error_t::FRAME_ENCODING_ERROR);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					/**
					 * Определяем тип фрейма лимита
					 */
					switch(type){
						// Фрейм лимита данных соединения MAX_DATA (RFC 9000 §19.9)
						case frame_t::MAX_DATA:
							// Обновляем лимит отправки данных соединения (лимиты только растут)
							this->_flow.txMax = ::max(this->_flow.txMax, value);
						break;
						// Фрейм лимита двунаправленных потоков MAX_STREAMS (RFC 9000 §19.11)
						case frame_t::MAX_STREAMS_BIDI:
							// Обновляем лимит на локально открываемые двунаправленные потоки
							this->_limits.maxBidiRemote = ::max(this->_limits.maxBidiRemote, value);
						break;
						// Фрейм лимита однонаправленных потоков MAX_STREAMS (RFC 9000 §19.11)
						case frame_t::MAX_STREAMS_UNI:
							// Обновляем лимит на локально открываемые однонаправленные потоки
							this->_limits.maxUniRemote = ::max(this->_limits.maxUniRemote, value);
						break;
						// Фрейм вывода идентификатора соединения из обращения (RFC 9000 §19.16)
						case frame_t::RETIRE_CONNECTION_ID: {
							// Если порядковый номер не выдавался локальным эндпоинтом
							if(value >= this->_cids.routing.issuedSeq){
								// Ставим завершение соединения с нарушением протокола в очередь
								this->fail(error_t::PROTOCOL_VIOLATION);
								// Выводим отрицательный результат
								return status_t::ERROR;
							}
							// Если выведен идентификатор соединения хендшейка
							if(value == 0){
								// Устанавливаем флаг вывода идентификатора хендшейка из обращения
								this->_cids.routing.retired = true;
								// Отмечаем идентификатор выведенным из обращения для маршрутизации
								this->_cids.routing.removed.push_back(this->_cids.identity.source);
							// Если выведен дополнительно выданный идентификатор
							} else {
								// Выполняем поиск выведенного из обращения идентификатора
								auto j = this->_cids.routing.issued.find(value);
								// Если выведенный из обращения идентификатор найден
								if(j != this->_cids.routing.issued.end())
									// Отмечаем идентификатор выведенным из обращения для маршрутизации
									this->_cids.routing.removed.push_back(j->second.cid);
								// Удаляем идентификатор из списка выданных
								this->_cids.routing.issued.erase(value);
								/**
								 * Перебираем очередь отправки фреймов NEW_CONNECTION_ID
								 */
								for(auto i = this->_cids.routing.issueQueue.begin(); i != this->_cids.routing.issueQueue.end();){
									// Если порядковый номер выведен из обращения
									if(* i == value)
										// Удаляем порядковый номер из очереди отправки
										i = this->_cids.routing.issueQueue.erase(i);
									// Переходим к следующему порядковому номеру
									else ++i;
								}
							}
							// Выдаём замещающий идентификатор соединения (RFC 9000 §5.1.1)
							this->issue();
						} break;
						// Остальные фреймы информационные - лимиты обновляются локальной логикой
						default: break;
					}
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фреймы с полями идентификатора потока и лимита
			case frame_t::MAX_STREAM_DATA:
			case frame_t::STREAM_DATA_BLOCKED: {
				// Идентификатор потока
				uint64_t streamId = 0;
				// Значение лимита
				uint64_t value = 0;
				// Выполняем разбор фрейма
				status = frame::parser::pair(data + offset, size - offset, type, streamId, value, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					/**
					 * Проверяем допустимость фрейма для направленности потока: лимит
					 * данных относится к направлению отправки, поэтому недопустим для
					 * потока, принимающего только на приём, а сообщение о блокировке -
					 * наоборот (RFC 9000 §19.10/§19.13)
					 */
					const bool allowed = ((type == frame_t::MAX_STREAM_DATA) ? this->sendable(streamId) : this->receivable(streamId));
					// Если фрейм недопустим для направленности потока
					if(!allowed){
						// Ставим завершение соединения с ошибкой состояния потока в очередь
						this->fail(error_t::STREAM_STATE_ERROR);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Код ошибки транспорта состояния потока
					error_t reason = error_t::NO_ERROR;
					/**
					 * Получаем состояние потока с неявным созданием: фрейм для локально
					 * инициируемого, но ещё не открытого потока нарушает протокол
					 * и отвергается методом поиска (RFC 9000 §19.10)
					 */
					stream_data_t * stream = this->accept(streamId, reason);
					// Если идентификатор потока нарушает протокол
					if(stream == nullptr){
						// Ставим завершение соединения с кодом ошибки в очередь
						this->fail(reason);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Если разобран фрейм лимита данных потока MAX_STREAM_DATA
					if(type == frame_t::MAX_STREAM_DATA)
						// Обновляем лимит отправки потока (лимиты только растут)
						stream->txMax = ::max(stream->txMax, value);
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм анонса нового идентификатора соединения NEW_CONNECTION_ID
			case frame_t::NEW_CONNECTION_ID: {
				// Разобранный фрейм анонса идентификатора
				frame::new_connection_id_t frame;
				// Выполняем разбор фрейма NEW_CONNECTION_ID
				status = frame::parser::newConnectionId(data + offset, size - offset, frame, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Если анонсирован рост порядкового номера вывода из обращения
					if(frame.retirePriorTo > this->_cids.routing.retirePrior){
						// Обновляем порядковый номер вывода идентификаторов из обращения
						this->_cids.routing.retirePrior = frame.retirePriorTo;
						/**
						 * Перебираем список идентификаторов удалённого эндпоинта
						 */
						for(auto i = this->_cids.routing.remote.begin(); i != this->_cids.routing.remote.end();){
							// Если идентификатор выводится из обращения (RFC 9000 §5.1.2)
							if(i->seq < this->_cids.routing.retirePrior){
								// Ставим порядковый номер в очередь отправки фрейма RETIRE_CONNECTION_ID
								this->_cids.routing.retireQueue.push_back(i->seq);
								// Удаляем идентификатор из списка
								i = this->_cids.routing.remote.erase(i);
							// Переходим к следующему идентификатору
							} else ++i;
						}
					}
					// Если порядковый номер идентификатора уже выведен из обращения
					if(frame.seq < this->_cids.routing.retirePrior){
						// Ставим порядковый номер в очередь отправки фрейма RETIRE_CONNECTION_ID (RFC 9000 §19.15)
						this->_cids.routing.retireQueue.push_back(frame.seq);
						// Устанавливаем флаг приёма ack-eliciting фрейма
						elicit = true;
						// Прекращаем обработку фрейма
						break;
					}
					// Флаг повторного анонса идентификатора
					bool known = false;
					/**
					 * Перебираем список идентификаторов удалённого эндпоинта
					 */
					for(auto & item : this->_cids.routing.remote){
						// Если порядковый номер уже анонсирован
						if(item.seq == frame.seq){
							// Если содержимое анонса отличается (RFC 9000 §19.15)
							if(!(item.cid == frame.cid)){
								// Ставим завершение соединения с нарушением протокола в очередь
								this->fail(error_t::PROTOCOL_VIOLATION);
								// Выводим отрицательный результат
								return status_t::ERROR;
							}
							// Устанавливаем флаг повторного анонса
							known = true;
							// Прекращаем перебор
							break;
						}
					}
					// Если идентификатор анонсирован впервые
					if(!known){
						// Формируем запись идентификатора удалённого эндпоинта
						remote_cid_t item;
						// Устанавливаем порядковый номер идентификатора
						item.seq = frame.seq;
						// Устанавливаем идентификатор соединения
						item.cid = frame.cid;
						// Копируем токен сброса без сохранения состояния
						::memcpy(item.resetToken, frame.resetToken, proto::RESET_TOKEN_SIZE);
						// Устанавливаем флаг наличия токена сброса
						item.hasToken = true;
						// Добавляем идентификатор в список
						this->_cids.routing.remote.push_back(item);
						// Если превышен анонсированный лимит активных идентификаторов (RFC 9000 §5.1.1)
						if(this->_cids.routing.remote.size() > this->_transport.local.activeConnectionIdLimit){
							// Ставим завершение соединения с превышением лимита идентификаторов в очередь
							this->fail(error_t::CONNECTION_ID_LIMIT_ERROR);
							// Выводим отрицательный результат
							return status_t::ERROR;
						}
					}
					// Если текущий идентификатор выведен из обращения
					if(this->_cids.routing.sequence < this->_cids.routing.retirePrior){
						// Флаг выполненного переключения идентификатора
						bool switched = false;
						/**
						 * Перебираем список идентификаторов удалённого эндпоинта дважды:
						 * сперва в обход закреплённого за предпочтительным адресом, а затем
						 * и с ним. Продолжать пользоваться выведенным из обращения
						 * идентификатором нельзя, поэтому при отсутствии прочих берётся
						 * и закреплённый - переезд без идентификатора хуже неотправляемых
						 * пакетов не бывает (RFC 9000 §5.1.1/§5.1.2)
						 */
						for(uint8_t pass = 0; (pass < 2) && !switched; pass++){
							/**
							 * Перебираем список идентификаторов удалённого эндпоинта
							 */
							for(auto & item : this->_cids.routing.remote){
								// Если идентификатор ещё не использовался и допустим на текущем проходе
								if(!item.used && ((pass > 0) || !this->reserved(item.seq))){
									// Переключаем идентификатор соединения удалённого эндпоинта
									this->_cids.identity.destination = item.cid;
									// Обновляем порядковый номер текущего идентификатора
									this->_cids.routing.sequence = item.seq;
									// Устанавливаем флаг использования идентификатора
									item.used = true;
									// Устанавливаем флаг выполненного переключения
									switched = true;
									// Прекращаем перебор
									break;
								}
							}
						}
					}
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм проверки достижимости пути PATH_CHALLENGE
			case frame_t::PATH_CHALLENGE: {
				// Данные принятой проверки достижимости пути
				uint8_t buffer[proto::PATH_DATA_SIZE];
				// Выполняем разбор фрейма PATH_CHALLENGE
				status = frame::parser::path(data + offset, size - offset, type, buffer, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					/**
					 * Если очередь ответов заполнена: ответы ретрансмиссии не подлежат
					 * (RFC 9000 §13.3), поэтому очередь ограничена сверху, а поток проверок
					 * сверх предела обслуживается вытеснением наиболее старой
					 */
					if(this->_path.responses.size() >= MAX_QUEUED_RESPONSES)
						// Отбрасываем данные наиболее старой принятой проверки
						this->_path.responses.pop_front();
					// Ставим данные принятой проверки в очередь ответов (RFC 9000 §8.2.2)
					this->_path.responses.emplace_back(reinterpret_cast <const char *> (buffer), proto::PATH_DATA_SIZE);
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм ответа на проверку достижимости пути PATH_RESPONSE
			case frame_t::PATH_RESPONSE: {
				// Данные проверки пути
				uint8_t buffer[proto::PATH_DATA_SIZE];
				// Выполняем разбор фрейма PATH_RESPONSE
				status = frame::parser::path(data + offset, size - offset, type, buffer, consumed, error);
				// Если фрейм разобран успешно и проверка пути выполняется
				if((status == status_t::OK) && this->_path.pending){
					/**
					 * Сверяем данные ответа с отправленными: ответ обязан содержать
					 * ровно те же данные, иначе он не подтверждает достижимость пути
					 * и является нарушением протокола (RFC 9000 §8.2.3)
					 */
					if(::memcmp(buffer, this->_path.probe, proto::PATH_DATA_SIZE) != 0){
						// Ставим завершение соединения с нарушением протокола в очередь
						this->fail(error_t::PROTOCOL_VIOLATION);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Сбрасываем флаг ожидания ответа на проверку пути
					this->_path.pending = false;
					// Сбрасываем дедлайн отказа от проверки достижимости пути
					this->_path.deadline = 0;
					/**
					 * Запоминаем адрес последним проверенным: на него возвращается соединение,
					 * если проверка следующего адреса не пройдёт (RFC 9000 §9.3.2)
					 */
					if(!this->_path.relocating)
						// Запоминаем адрес текущего пути последним проверенным
						this->_path.previous = this->_path.address;
					// Устанавливаем флаг подтверждённой достижимости пути
					this->_path.validated = true;
					/**
					 * Подтверждение достижимости пути снимает лимит анти-амплификации:
					 * адрес удалённого эндпоинта на этом пути проверен (RFC 9000 §9.3)
					 */
					this->_amplify.validated = true;
					/**
					 * Если подтверждена достижимость предпочтительного адреса: проверка
					 * пройдена, и переезд на него становится дозволенным (RFC 9000 §9.6.2)
					 */
					if(this->_path.relocating)
						// Выполняем завершение переезда на предпочтительный адрес
						this->settle();
					// Возобновляем поиск размера пути после подтверждения адреса (RFC 8899)
					this->discover();
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фреймы завершения соединения CONNECTION_CLOSE
			case frame_t::CONNECTION_CLOSE:
			case frame_t::CONNECTION_CLOSE_APP: {
				// Разобранный фрейм завершения соединения
				frame::connection_close_t frame;
				// Выполняем разбор фрейма CONNECTION_CLOSE
				status = frame::parser::connectionClose(data + offset, size - offset, frame, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Если удалённый эндпоинт сообщил об ошибке приложения
					if(frame.app)
						// Устанавливаем код ошибки приложения
						this->_core.error = error_t::APPLICATION_ERROR;
					// Если удалённый эндпоинт сообщил об ошибке транспорта
					else this->_core.error = static_cast <error_t> (frame.code);
					// Устанавливаем состояние завершения соединения удалённым эндпоинтом
					this->_core.state = state_t::DRAINING;
					/**
					 * Прекращаем разбор нагрузки: остаток фреймов относится к соединению,
					 * которое удалённый эндпоинт уже завершил, а ошибка разбора в этом
					 * остатке поставила бы в очередь собственное завершение - отправлять
					 * его в состоянии завершения удалённым узлом запрещено (RFC 9000 §10.2.2)
					 */
					return status_t::OK;
				}
			} break;
			// Фрейм подтверждения завершения хендшейка HANDSHAKE_DONE
			case frame_t::HANDSHAKE_DONE: {
				// Выполняем разбор фрейма HANDSHAKE_DONE
				status = frame::parser::handshakeDone(data + offset, size - offset, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Если фрейм прислал клиент (RFC 9000 §19.20)
					if(this->_core.endpoint == endpoint_t::SERVER){
						// Ставим завершение соединения с нарушением протокола в очередь
						this->fail(error_t::PROTOCOL_VIOLATION);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Устанавливаем флаг подтверждения хендшейка (RFC 9001 §4.1.2)
					this->_core.flags |= flags::CONFIRMED;
					// Начинаем поиск размера пути зондированием (RFC 8899)
					this->discover();
					// Сбрасываем ключи уровня Handshake (RFC 9001 §4.9.2)
					this->discard(level_t::HANDSHAKE);
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Неизвестный тип фрейма
			default: {
				// Ставим завершение соединения с ошибкой кодирования фрейма в очередь
				this->fail(error_t::FRAME_ENCODING_ERROR);
				// Выводим отрицательный результат
				return status_t::ERROR;
			}
		}
		// Если разбор фрейма завершился ошибкой
		if(status != status_t::OK){
			// Ставим завершение соединения с кодом ошибки разбора в очередь
			this->fail(error);
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
		// Переходим к следующему фрейму
		offset += consumed;
	}
	// Если принят ack-eliciting фрейм
	if(elicit){
		// Получаем состояние пространства номеров пакетов уровня
		auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
		/**
		 * Если подтверждение ещё не поставлено в очередь - запоминаем время приёма:
		 * задержка отсчитывается от первого неподтверждённого пакета (RFC 9000 §13.2.1)
		 */
		if(!item.ackElicited)
			// Устанавливаем время приёма ack-eliciting пакета
			item.ackTime = this->_times.now;
		// Устанавливаем флаг необходимости отправки подтверждения
		item.ackElicited = true;
	}
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод сборки нагрузки очередного пакета уровня шифрования
 *
 * @param level   уровень шифрования пакета
 * @param budget  доступно октетов в датаграмме для нагрузки
 * @param output  собранная нагрузка пакета (фреймы)
 * @param meta    учётная запись пакета для восстановления потерь
 * @param elicit  флаг наличия ack-eliciting фреймов в нагрузке
 * @param limited флаг исчерпанного окна перегрузки (только подтверждения)
 * @return        результат сборки (true - нагрузка не пустая)
 */
bool awh::quic::Connection::payload(const level_t level, const size_t budget, string & output, sent_t & meta, bool & elicit, const bool limited) noexcept {
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
	/**
	 * Если требуется отправка подтверждения, есть принятые пакеты и бюджета хватает
	 * хотя бы на фрейм с одним диапазоном. Уровень ранних данных подтверждений
	 * не несёт: они относятся к состоянию, согласуемому хендшейком (RFC 9000 §12.4)
	 */
	if((level != level_t::EARLY_DATA) && item.ackElicited && !item.ranges.empty() && (budget >= (CONTROL_OVERHEAD + 16))){
		// Формируем фрейм подтверждения приёма пакетов
		frame::ack_t frame;
		// Вычисляем задержку подтверждения в миллисекундах
		const uint64_t delay = ((this->_times.now > item.ackTime) ? (this->_times.now - item.ackTime) : 0);
		/**
		 * Кодируем задержку в микросекундах с показателем степени локального
		 * эндпоинта: пир декодирует её анонсированным нами значением (RFC 9000 §18.2)
		 */
		frame.delay = ((delay * 1000) >> this->_transport.local.ackDelayExponent);
		/**
		 * Оцениваем количество диапазонов, помещающихся в бюджет: заголовок фрейма
		 * занимает до 25 октетов, каждый дополнительный диапазон - до 16. Первый
		 * диапазон содержит наибольший принятый номер и передаётся всегда, остальные
		 * при нехватке места отбрасываются - фрейм ACK не обязан быть полным (RFC 9000 §13.2.3)
		 */
		const size_t count = ::min(((budget - CONTROL_OVERHEAD) / 16), item.ranges.size());
		// Копируем диапазоны, помещающиеся в бюджет датаграммы
		frame.ranges.assign(item.ranges.begin(), item.ranges.begin() + static_cast <ptrdiff_t> (count));
		/**
		 * Если в пространстве принимались маркированные пакеты, возвращаем счётчики
		 * маркировок эхом: пир судит по их приросту о перегрузке пути (RFC 9000 §13.4.1)
		 */
		if((frame.hasEcn = ((item.ect0 > 0) || (item.ect1 > 0) || (item.ce > 0)))){
			// Устанавливаем счётчик принятых пакетов с маркировкой ECT(0)
			frame.ect0 = item.ect0;
			// Устанавливаем счётчик принятых пакетов с маркировкой ECT(1)
			frame.ect1 = item.ect1;
			// Устанавливаем счётчик принятых пакетов с маркировкой CE
			frame.ce = item.ce;
		}
		// Выполняем сборку фрейма ACK
		frame::serialize::ack(output, frame);
		// Сбрасываем флаг необходимости отправки подтверждения
		item.ackElicited = false;
	}
	// Если окно перегрузки исчерпано - отправляются только подтверждения (RFC 9002 §7)
	if(limited){
		// Если нагрузка меньше минимума для выборки защиты заголовка
		if(!output.empty() && (output.size() < MIN_PAYLOAD_SIZE))
			// Дополняем нагрузку фреймами PADDING
			frame::serialize::padding(output, MIN_PAYLOAD_SIZE - output.size());
		// Выводим результат сборки
		return !output.empty();
	}
	/**
	 *  Пока есть CRYPTO-данные для ретрансмиссии и в датаграмме осталось место.
	 *  Данные хендшейка на уровне ранних данных недопустимы (RFC 9000 §12.4)
	 */
	while((level != level_t::EARLY_DATA) && !item.rtxQueue.empty() && (budget > (output.size() + CRYPTO_OVERHEAD))){
		// Получаем первый блок данных очереди ретрансмиссии
		auto & front = item.rtxQueue.front();
		// Вычисляем доступный размер данных CRYPTO-фрейма
		const size_t chunk = ::min(front.second.size(), budget - output.size() - CRYPTO_OVERHEAD);
		// Выполняем сборку фрейма CRYPTO с исходным смещением
		frame::serialize::crypto(output, front.first, string_view(front.second.data(), chunk));
		// Запоминаем отправленные CRYPTO-данные в учётной записи пакета
		meta.crypto.emplace_back(front.first, front.second.substr(0, chunk));
		// Устанавливаем флаг наличия ack-eliciting фреймов
		elicit = true;
		// Если блок данных упакован полностью
		if(chunk == front.second.size())
			// Удаляем блок данных из очереди ретрансмиссии
			item.rtxQueue.erase(item.rtxQueue.begin());
		// Если блок данных упакован частично
		else {
			// Продвигаем смещение оставшихся данных блока
			front.first += chunk;
			// Удаляем упакованные данные из блока
			front.second.erase(0, chunk);
		}
	}
	// Если есть исходящие CRYPTO-данные и в датаграмме осталось место
	if((level != level_t::EARLY_DATA) && !item.txBuffer.empty() && (budget > (output.size() + CRYPTO_OVERHEAD))){
		// Вычисляем доступный размер данных CRYPTO-фрейма
		const size_t chunk = ::min(item.txBuffer.size(), budget - output.size() - CRYPTO_OVERHEAD);
		// Выполняем сборку фрейма CRYPTO
		frame::serialize::crypto(output, item.txOffset, string_view(item.txBuffer.data(), chunk));
		// Запоминаем отправленные CRYPTO-данные в учётной записи пакета
		meta.crypto.emplace_back(item.txOffset, item.txBuffer.substr(0, chunk));
		// Устанавливаем флаг наличия ack-eliciting фреймов
		elicit = true;
		// Продвигаем смещение потока криптографического хендшейка
		item.txOffset += chunk;
		// Удаляем упакованные данные из буфера
		item.txBuffer.erase(0, chunk);
	}
	/**
	 * Если собирается нагрузка уровня приложения либо ранних данных: ранние данные
	 * несут те же фреймы прикладного уровня, кроме относящихся к состоянию,
	 * которое согласуется хендшейком (RFC 9000 §12.4 таблица 3)
	 */
	if((level == level_t::APPLICATION) || (level == level_t::EARLY_DATA)){
		// Если требуется отправка фрейма HANDSHAKE_DONE (только сервер, только уровень приложения)
		if((level == level_t::APPLICATION) && (this->_core.flags & flags::HANDSHAKE_DONE) && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Выполняем сборку фрейма HANDSHAKE_DONE
			frame::serialize::handshakeDone(output);
			// Запоминаем фрейм HANDSHAKE_DONE в учётной записи пакета
			meta.handshakeDone = true;
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки фрейма HANDSHAKE_DONE
			this->_core.flags &= ~flags::HANDSHAKE_DONE;
		}
		/**
		 * Если требуется отправка токена проверки адреса для будущих соединений:
		 * фрейм допустим только на уровне приложения - он относится к состоянию,
		 * согласуемому хендшейком (RFC 9000 §12.4)
		 */
		if((level == level_t::APPLICATION) && this->_token.queued && (budget > (output.size() + CONTROL_OVERHEAD + this->_token.address.size()))){
			// Выполняем сборку фрейма NEW_TOKEN (RFC 9000 §19.7)
			frame::serialize::newToken(output, this->_token.address);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::NEW_TOKEN, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки фрейма NEW_TOKEN
			this->_token.queued = false;
		}
		/**
		 * Если требуется отправка фрейма PATH_CHALLENGE: проверка предпочтительного
		 * адреса сюда не попадает - она уходит отдельной датаграммой на сам
		 * предпочтительный адрес, а не по текущему пути (RFC 9000 §9.6.2)
		 */
		if(this->_path.queued && !this->_path.relocating && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Выполняем сборку фрейма PATH_CHALLENGE (RFC 9000 §19.17)
			frame::serialize::path(output, frame_t::PATH_CHALLENGE, this->_path.probe);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::PATH_CHALLENGE, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки фрейма PATH_CHALLENGE
			this->_path.queued = false;
		}
		/**
		 * Пока есть принятые проверки пути и в пакете осталось место: на каждую
		 * принятую проверку отправляется свой ответ с её данными. Ответы допустимы
		 * только на уровне приложения и ретрансмиссии не подлежат - при потере
		 * удалённый эндпоинт присылает новую проверку (RFC 9000 §8.2.2/§13.3)
		 */
		while((level == level_t::APPLICATION) && !this->_path.responses.empty() && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Выполняем сборку фрейма PATH_RESPONSE с данными принятой проверки
			frame::serialize::path(output, frame_t::PATH_RESPONSE, reinterpret_cast <const uint8_t *> (this->_path.responses.front().data()));
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::PATH_RESPONSE, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Удаляем обслуженную проверку из очереди ответов
			this->_path.responses.pop_front();
		}
		// Если требуется отправка обновлённого лимита данных соединения MAX_DATA
		if(this->_flow.rxQueued && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Выполняем сборку фрейма MAX_DATA (RFC 9000 §19.9)
			frame::serialize::single(output, frame_t::MAX_DATA, this->_flow.rxMax);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::MAX_DATA, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки обновлённого лимита
			this->_flow.rxQueued = false;
		}
		// Если требуется отправка обновлённого лимита двунаправленных потоков MAX_STREAMS
		if(this->_limits.bidiQueued && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Выполняем сборку фрейма MAX_STREAMS (RFC 9000 §19.11)
			frame::serialize::single(output, frame_t::MAX_STREAMS_BIDI, this->_limits.maxBidiLocal);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::MAX_STREAMS_BIDI, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки обновлённого лимита
			this->_limits.bidiQueued = false;
		}
		// Если требуется отправка обновлённого лимита однонаправленных потоков MAX_STREAMS
		if(this->_limits.uniQueued && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Выполняем сборку фрейма MAX_STREAMS (RFC 9000 §19.11)
			frame::serialize::single(output, frame_t::MAX_STREAMS_UNI, this->_limits.maxUniLocal);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::MAX_STREAMS_UNI, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки обновлённого лимита
			this->_limits.uniQueued = false;
		}
		// Если открытие двунаправленных потоков заблокировано лимитом удалённого эндпоинта
		if(this->_limits.bidiBlocked && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Если лимит с момента блокировки поднят - блокировки больше нет
			if(this->_limits.openedBidi < this->_limits.maxBidiRemote)
				// Снимаем флаг без отправки: позднее исчерпание нового лимита переармируется в open()
				this->_limits.bidiBlocked = false;
			// Если открытие всё ещё упирается в текущий лимит удалённого эндпоинта
			else {
				// Выполняем сборку фрейма STREAMS_BLOCKED (RFC 9000 §19.14)
				frame::serialize::single(output, frame_t::STREAMS_BLOCKED_BIDI, this->_limits.maxBidiRemote);
				// Запоминаем управляющий фрейм в учётной записи пакета
				meta.control.emplace_back(frame_t::STREAMS_BLOCKED_BIDI, 0);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Сбрасываем флаг необходимости отправки блокировки
				this->_limits.bidiBlocked = false;
				// Запоминаем лимит, при котором блокировка сигнализирована
				this->_limits.bidiBlockedAt = this->_limits.maxBidiRemote;
			}
		}
		// Если открытие однонаправленных потоков заблокировано лимитом удалённого эндпоинта
		if(this->_limits.uniBlocked && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Если лимит с момента блокировки поднят - блокировки больше нет
			if(this->_limits.openedUni < this->_limits.maxUniRemote)
				// Снимаем флаг без отправки: позднее исчерпание нового лимита переармируется в open()
				this->_limits.uniBlocked = false;
			// Если открытие всё ещё упирается в текущий лимит удалённого эндпоинта
			else {
				// Выполняем сборку фрейма STREAMS_BLOCKED (RFC 9000 §19.14)
				frame::serialize::single(output, frame_t::STREAMS_BLOCKED_UNI, this->_limits.maxUniRemote);
				// Запоминаем управляющий фрейм в учётной записи пакета
				meta.control.emplace_back(frame_t::STREAMS_BLOCKED_UNI, 0);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Сбрасываем флаг необходимости отправки блокировки
				this->_limits.uniBlocked = false;
				// Запоминаем лимит, при котором блокировка сигнализирована
				this->_limits.uniBlockedAt = this->_limits.maxUniRemote;
			}
		}
		// Если отправка данных соединения заблокирована лимитом удалённого эндпоинта
		if(this->_flow.txBlocked && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Если лимит данных соединения с момента блокировки поднят - блокировки больше нет
			if(this->_flow.txData < this->_flow.txMax)
				// Снимаем флаг без отправки: позднее исчерпание нового лимита переармируется
				this->_flow.txBlocked = false;
			// Если отправка всё ещё упирается в текущий лимит данных соединения
			else {
				// Выполняем сборку фрейма DATA_BLOCKED (RFC 9000 §19.12)
				frame::serialize::single(output, frame_t::DATA_BLOCKED, this->_flow.txMax);
				// Запоминаем управляющий фрейм в учётной записи пакета
				meta.control.emplace_back(frame_t::DATA_BLOCKED, 0);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Сбрасываем флаг заблокированной отправки данных соединения
				this->_flow.txBlocked = false;
				// Запоминаем лимит, о блокировке которым уведомлён удалённый эндпоинт
				this->_flow.txBlockedAt = this->_flow.txMax;
			}
		}
		/**
		 * Пока есть анонсы новых идентификаторов соединения и в датаграмме осталось место
		 */
		while(!this->_cids.routing.issueQueue.empty() && (budget > (output.size() + NEW_CID_OVERHEAD))){
			// Получаем порядковый номер первого анонса очереди
			const uint64_t seq = this->_cids.routing.issueQueue.front();
			// Ищем выданный идентификатор по порядковому номеру
			auto i = this->_cids.routing.issued.find(seq);
			// Если идентификатор ещё выдан (не выведен из обращения)
			if(i != this->_cids.routing.issued.end()){
				// Выполняем сборку фрейма NEW_CONNECTION_ID (RFC 9000 §19.15)
				frame::serialize::newConnectionId(output, i->second);
				// Запоминаем управляющий фрейм в учётной записи пакета
				meta.control.emplace_back(frame_t::NEW_CONNECTION_ID, seq);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
			}
			// Удаляем порядковый номер из очереди отправки
			this->_cids.routing.issueQueue.erase(this->_cids.routing.issueQueue.begin());
		}
		/**
		 * Пока есть выводимые из обращения идентификаторы и в датаграмме осталось место
		 */
		while(!this->_cids.routing.retireQueue.empty() && (budget > (output.size() + CONTROL_OVERHEAD))){
			// Получаем порядковый номер первого вывода очереди
			const uint64_t seq = this->_cids.routing.retireQueue.front();
			// Выполняем сборку фрейма RETIRE_CONNECTION_ID (RFC 9000 §19.16)
			frame::serialize::single(output, frame_t::RETIRE_CONNECTION_ID, seq);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::RETIRE_CONNECTION_ID, seq);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Удаляем порядковый номер из очереди отправки
			this->_cids.routing.retireQueue.erase(this->_cids.routing.retireQueue.begin());
		}
		/**
		 * Перебираем список потоков приложения (управляющие фреймы потоков)
		 */
		for(auto & entry : this->_stream.list){
			// Если в датаграмме не осталось места на управляющий фрейм
			if(budget <= (output.size() + STREAM_OVERHEAD))
				// Прекращаем сборку управляющих фреймов
				break;
			// Получаем состояние потока
			auto & stream = entry.second;
			// Если требуется отправка обновлённого лимита данных потока MAX_STREAM_DATA
			if(stream.rxMaxQueued){
				// Выполняем сборку фрейма MAX_STREAM_DATA (RFC 9000 §19.10)
				frame::serialize::pair(output, frame_t::MAX_STREAM_DATA, entry.first, stream.rxMax);
				// Запоминаем управляющий фрейм в учётной записи пакета
				meta.control.emplace_back(frame_t::MAX_STREAM_DATA, entry.first);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Сбрасываем флаг необходимости отправки обновлённого лимита
				stream.rxMaxQueued = false;
			}
			// Если требуется отправка фрейма RESET_STREAM
			if(stream.txReset && (budget > (output.size() + CONTROL_OVERHEAD))){
				// Выполняем сборку фрейма RESET_STREAM с финальным размером отправленных данных (RFC 9000 §19.4)
				frame::serialize::resetStream(output, entry.first, stream.txResetCode, stream.txOffset);
				// Запоминаем управляющий фрейм в учётной записи пакета
				meta.control.emplace_back(frame_t::RESET_STREAM, entry.first);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Сбрасываем флаг необходимости отправки фрейма RESET_STREAM
				stream.txReset = false;
				// Устанавливаем флаг выполненной отправки фрейма RESET_STREAM
				stream.txResetSent = true;
			}
			// Если требуется отправка фрейма STOP_SENDING
			if(stream.stopQueued && (budget > (output.size() + CONTROL_OVERHEAD))){
				// Выполняем сборку фрейма STOP_SENDING (RFC 9000 §19.5)
				frame::serialize::stopSending(output, entry.first, stream.stopCode);
				// Запоминаем управляющий фрейм в учётной записи пакета
				meta.control.emplace_back(frame_t::STOP_SENDING, entry.first);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Сбрасываем флаг необходимости отправки фрейма STOP_SENDING
				stream.stopQueued = false;
				// Устанавливаем флаг выполненной отправки фрейма STOP_SENDING
				stream.stopSent = true;
			}
			// Если отправка данных потока заблокирована лимитом удалённого эндпоинта
			if(stream.txBlocked && (budget > (output.size() + CONTROL_OVERHEAD))){
				// Если лимит данных потока с момента блокировки поднят - блокировки больше нет
				if(stream.txOffset < stream.txMax)
					// Снимаем флаг без отправки: позднее исчерпание нового лимита переармируется
					stream.txBlocked = false;
				// Если отправка всё ещё упирается в текущий лимит данных потока
				else {
					// Выполняем сборку фрейма STREAM_DATA_BLOCKED (RFC 9000 §19.13)
					frame::serialize::pair(output, frame_t::STREAM_DATA_BLOCKED, entry.first, stream.txMax);
					// Запоминаем управляющий фрейм в учётной записи пакета
					meta.control.emplace_back(frame_t::STREAM_DATA_BLOCKED, entry.first);
					// Устанавливаем флаг наличия ack-eliciting фреймов
					elicit = true;
					// Сбрасываем флаг заблокированной отправки данных потока
					stream.txBlocked = false;
					// Запоминаем лимит, о блокировке которым уведомлён удалённый эндпоинт
					stream.txBlockedAt = stream.txMax;
				}
			}
		}
		/**
		 * Пока есть исходящие датаграммы приложения и в датаграмме осталось место.
		 * Датаграммы отправляются вперёд данных потоков: доставка их ненадёжна,
		 * повторной отправки не будет, а задержка для них обычно и есть смысл
		 * их применения (RFC 9221 §5.2)
		 */
		while(!this->_dgram.tx.empty()){
			// Получаем первую датаграмму очереди отправки
			const string & front = this->_dgram.tx.front();
			// Если датаграмма целиком не помещается в оставшееся место
			if(budget <= (output.size() + DATAGRAM_OVERHEAD + front.size()))
				// Прекращаем упаковку датаграмм - остаток уйдёт следующим пакетом
				break;
			// Выполняем сборку фрейма DATAGRAM (RFC 9221 §4)
			frame::serialize::datagram(output, front);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Удаляем отправленную датаграмму из очереди
			this->_dgram.tx.pop_front();
		}
		/**
		 * Пока есть блоки данных потоков для ретрансмиссии и в датаграмме осталось место
		 */
		while(!this->_stream.retransmit.empty() && (budget > (output.size() + STREAM_OVERHEAD))){
			// Получаем первый блок данных очереди ретрансмиссии
			auto & front = this->_stream.retransmit.front();
			// Ищем поток по идентификатору
			auto i = this->_stream.list.find(front.sid);
			// Если поток сброшен - ретрансмиссия данных не требуется (RFC 9000 §3.1)
			if((i == this->_stream.list.end()) || i->second.txReset || i->second.txResetSent){
				// Удаляем блок данных из очереди ретрансмиссии
				this->_stream.retransmit.erase(this->_stream.retransmit.begin());
				// Продолжаем обработку очереди
				continue;
			}
			// Вычисляем доступный размер данных фрейма STREAM
			const size_t chunk = ::min(front.data.size(), budget - output.size() - STREAM_OVERHEAD);
			// Определяем флаг завершения потока для фрагмента
			const bool fin = (front.fin && (chunk == front.data.size()));
			// Выполняем сборку фрейма STREAM с исходным смещением
			frame::serialize::stream(output, front.sid, front.offset, string_view(front.data.data(), chunk), fin);
			// Формируем блок данных для учётной записи пакета
			chunk_t sent;
			// Устанавливаем идентификатор потока
			sent.sid = front.sid;
			// Устанавливаем смещение данных в потоке
			sent.offset = front.offset;
			// Устанавливаем флаг завершения потока
			sent.fin = fin;
			// Устанавливаем данные блока
			sent.data = front.data.substr(0, chunk);
			// Запоминаем отправленный блок данных в учётной записи пакета
			meta.stream.push_back(::move(sent));
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Если блок данных упакован полностью
			if(chunk == front.data.size())
				// Удаляем блок данных из очереди ретрансмиссии
				this->_stream.retransmit.erase(this->_stream.retransmit.begin());
			// Если блок данных упакован частично
			else {
				// Продвигаем смещение оставшихся данных блока
				front.offset += chunk;
				// Удаляем упакованные данные из блока
				front.data.erase(0, chunk);
				// Прекращаем обработку очереди - датаграмма заполнена
				break;
			}
		}
		/**
		 * Перебираем список потоков приложения кругом начиная с курсора: обход всегда
		 * с начала списка отдавал бы датаграмму потокам с наименьшими идентификаторами,
		 * а остальные простаивали бы неограниченно долго
		 */
		for(size_t index = 0; index < this->_stream.list.size(); index++){
			// Если в датаграмме не осталось места на фрейм STREAM
			if(budget <= (output.size() + STREAM_OVERHEAD))
				// Прекращаем упаковку данных потоков
				break;
			// Ищем первый поток начиная с позиции курсора
			auto position = this->_stream.list.lower_bound(this->_stream.cursor);
			// Если потоки от позиции курсора закончились
			if(position == this->_stream.list.end())
				// Продолжаем обход с начала списка
				position = this->_stream.list.begin();
			// Продвигаем курсор на следующий поток обхода
			this->_stream.cursor = (position->first + 1);
			// Получаем ссылку на запись потока
			auto & entry = (* position);
			// Получаем состояние потока
			auto & stream = entry.second;
			// Если отправка потока аварийно завершена - данные не отправляются
			if(stream.txReset || stream.txResetSent)
				// Переходим к следующему потоку
				continue;
			// Вычисляем объём неупакованных данных потока
			const size_t pending = (stream.txBuffer.size() - stream.txCursor);
			// Если есть данные для отправки
			if(pending > 0){
				// Вычисляем доступное окно flow control потока
				const uint64_t streamWindow = ((stream.txMax > stream.txOffset) ? (stream.txMax - stream.txOffset) : 0);
				// Вычисляем доступное окно flow control соединения
				const uint64_t connWindow = ((this->_flow.txMax > this->_flow.txData) ? (this->_flow.txMax - this->_flow.txData) : 0);
				// Вычисляем доступный размер данных фрейма STREAM
				const size_t chunk = static_cast <size_t> (::min(::min(static_cast <uint64_t> (pending), ::min(streamWindow, connWindow)), static_cast <uint64_t> (budget - output.size() - STREAM_OVERHEAD)));
				// Если данные заблокированы лимитом потока (RFC 9000 §19.13)
				if((chunk == 0) && (streamWindow == 0)){
					/**
					 * Уведомление о блокировке отправляется однократно на каждое
					 * значение лимита: пока удалённый эндпоинт не поднял лимит,
					 * повторять сигнал бессмысленно (RFC 9000 §4.1)
					 */
					if(stream.txBlockedAt != stream.txMax)
						// Устанавливаем флаг заблокированной отправки данных потока
						stream.txBlocked = true;
					// Переходим к следующему потоку
					continue;
				}
				// Если данные заблокированы лимитом соединения (RFC 9000 §19.12)
				if((chunk == 0) && (connWindow == 0)){
					// Уведомление отправляется однократно на каждое значение лимита
					if(this->_flow.txBlockedAt != this->_flow.txMax)
						// Устанавливаем флаг заблокированной отправки данных соединения
						this->_flow.txBlocked = true;
					// Прекращаем упаковку данных потоков
					break;
				}
				// Если место в датаграмме исчерпано
				if(chunk == 0)
					// Прекращаем упаковку данных потоков
					break;
				// Определяем флаг завершения потока для фрагмента
				const bool fin = (stream.txFin && (chunk == pending));
				// Выполняем сборку фрейма STREAM (RFC 9000 §19.8)
				frame::serialize::stream(output, entry.first, stream.txOffset, string_view(stream.txBuffer.data() + stream.txCursor, chunk), fin);
				// Формируем блок данных для учётной записи пакета
				chunk_t sent;
				// Устанавливаем идентификатор потока
				sent.sid = entry.first;
				// Устанавливаем смещение данных в потоке
				sent.offset = stream.txOffset;
				// Устанавливаем флаг завершения потока
				sent.fin = fin;
				// Устанавливаем данные блока
				sent.data.assign(stream.txBuffer, stream.txCursor, chunk);
				// Запоминаем отправленный блок данных в учётной записи пакета
				meta.stream.push_back(::move(sent));
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Продвигаем смещение отправленных данных потока
				stream.txOffset += chunk;
				// Учитываем отправленные данные в flow control соединения
				this->_flow.txData += chunk;
				// Продвигаем курсор упакованных данных буфера
				stream.txCursor += chunk;
				/**
				 * Уплотняем буфер, когда потреблённая часть занимает не менее половины
				 * и превышает порог: вырезание на каждый пакет давало бы сдвиг всего
				 * остатка буфера и квадратичную стоимость на длинных передачах
				 */
				if((stream.txCursor >= COMPACT_THRESHOLD) && ((stream.txCursor * 2) >= stream.txBuffer.size())){
					// Вырезаем потреблённую часть буфера
					stream.txBuffer.erase(0, stream.txCursor);
					// Сбрасываем курсор упакованных данных
					stream.txCursor = 0;
				}
				// Если отправлено завершение потока
				if(fin)
					// Устанавливаем флаг выполненной отправки завершения потока
					stream.txFinSent = true;
			// Если данных нет, но требуется отправка завершения потока (пустой фрейм с FIN)
			} else if(stream.txFin && !stream.txFinSent){
				// Выполняем сборку пустого фрейма STREAM с флагом FIN
				frame::serialize::stream(output, entry.first, stream.txOffset, string_view(), true);
				// Формируем блок данных для учётной записи пакета
				chunk_t sent;
				// Устанавливаем идентификатор потока
				sent.sid = entry.first;
				// Устанавливаем смещение данных в потоке
				sent.offset = stream.txOffset;
				// Устанавливаем флаг завершения потока
				sent.fin = true;
				// Запоминаем отправленный блок данных в учётной записи пакета
				meta.stream.push_back(::move(sent));
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Устанавливаем флаг выполненной отправки завершения потока
				stream.txFinSent = true;
			}
		}
	}
	// Если требуется отправка зондирующего фрейма PING (RFC 9002 §6.2.4)
	if(item.pingQueued && (budget > (output.size() + CONTROL_OVERHEAD))){
		// Выполняем сборку фрейма PING
		frame::serialize::ping(output);
		// Устанавливаем флаг наличия ack-eliciting фреймов
		elicit = true;
		// Сбрасываем флаг необходимости отправки зондирующего фрейма PING
		item.pingQueued = false;
	}
	// Если нагрузка меньше минимума для выборки защиты заголовка (RFC 9001 §5.4.2)
	if(!output.empty() && (output.size() < MIN_PAYLOAD_SIZE))
		// Дополняем нагрузку фреймами PADDING
		frame::serialize::padding(output, MIN_PAYLOAD_SIZE - output.size());
	// Выводим результат сборки
	return !output.empty();
}
/**
 * @brief Метод вычисления размера заголовка пакета уровня шифрования
 *
 * @param level  уровень шифрования пакета
 * @param length значение поля Length (номер пакета + нагрузка + тег AEAD)
 * @param pnSize размер кодирования номера пакета в октетах
 * @param dcid   идентификатор соединения получателя пакета
 * @return       размер заголовка пакета в октетах
 */
size_t awh::quic::Connection::headerSize(const level_t level, const uint64_t length, const size_t pnSize, const cid_t & dcid) const noexcept {
	// Если пакет уровня приложения (короткий заголовок)
	if(level == level_t::APPLICATION)
		// Выводим размер короткого заголовка: первый октет + DCID + номер пакета
		return (1 + dcid.size + pnSize);
	// Размер длинного заголовка: первый октет + версия + длины и данные идентификаторов
	size_t result = (1 + 4 + 1 + dcid.size + 1 + this->_cids.identity.source.size);
	// Если пакет уровня Initial
	if(level == level_t::INITIAL)
		// Дописываем размер поля длины токена и токена проверки адреса (RFC 9000 §17.2.2)
		result += (varint::size(this->_token.initial.size()) + this->_token.initial.size());
	// Дописываем размер поля Length и номера пакета
	result += (varint::size(length) + pnSize);
	// Выводим размер заголовка пакета
	return result;
}
/**
 * @brief Метод сборки и защиты пакета уровня шифрования (заголовок + нагрузка)
 *
 * @param output  выходной буфер датаграммы (пакет дописывается)
 * @param level   уровень шифрования пакета
 * @param payload нагрузка пакета (фреймы)
 * @param dcid    идентификатор соединения получателя пакета
 * @return        результат сборки (false - ошибка криптографической библиотеки)
 */
bool awh::quic::Connection::seal(string & output, const level_t level, string_view payload, const cid_t & dcid) noexcept {
	// Получаем ключи защиты исходящих пакетов уровня
	const crypto::keys_t * keys = this->_crypto.handshake.encryption(level);
	// Если ключи защиты пакетов уровня не выведены
	if(keys == nullptr)
		// Выводим отрицательный результат
		return false;
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
	// Получаем номер отправляемого пакета
	const uint64_t pn = item.txPn;
	// Вычисляем размер кодирования номера пакета
	const size_t pnSize = packet::packetNumberSize(pn, (item.hasAcked ? item.largestAcked : pn));
	// Вычисляем значение поля Length: номер пакета + нагрузка + тег AEAD
	const uint64_t length = (pnSize + payload.size() + crypto::AEAD_TAG_SIZE);
	// Собираемый заголовок пакета в переиспользуемом буфере
	string & header = this->_buffer.header;
	// Очищаем буфер заголовка от результатов предыдущей сборки
	header.clear();
	/**
	 * Определяем уровень шифрования пакета
	 */
	switch(level){
		// Пакет уровня Initial
		case level_t::INITIAL: {
			// Выполняем сборку длинного заголовка пакета Initial с токеном проверки адреса
			if(!packet::serialize::longHeader(header, packet_t::INITIAL, proto::VERSION_1, dcid, this->_cids.identity.source, this->_token.initial, length, pn, pnSize))
				// Выводим отрицательный результат
				return false;
		} break;
		// Пакет уровня Handshake
		case level_t::HANDSHAKE: {
			// Выполняем сборку длинного заголовка пакета Handshake
			if(!packet::serialize::longHeader(header, packet_t::HANDSHAKE, proto::VERSION_1, dcid, this->_cids.identity.source, "", length, pn, pnSize))
				// Выводим отрицательный результат
				return false;
		} break;
		/**
		 * Пакет уровня ранних данных: токена проверки адреса не несёт, поскольку
		 * тот относится к пакетам Initial (RFC 9000 §17.2.3)
		 */
		case level_t::EARLY_DATA: {
			// Выполняем сборку длинного заголовка пакета ранних данных
			if(!packet::serialize::longHeader(header, packet_t::ZERO_RTT, proto::VERSION_1, dcid, this->_cids.identity.source, "", length, pn, pnSize))
				// Выводим отрицательный результат
				return false;
		} break;
		// Пакет уровня приложения (1-RTT)
		case level_t::APPLICATION: {
			// Выполняем сборку короткого заголовка пакета 1-RTT с битом фазы ключей (RFC 9001 §6)
			if(!packet::serialize::shortHeader(header, dcid, pn, pnSize, this->_crypto.phase.current, false))
				// Выводим отрицательный результат
				return false;
		} break;
		// Остальные уровни на данном этапе не собираются
		default:
			// Выводим отрицательный результат
			return false;
	}
	// Выполняем защиту пакета: AEAD-шифрование нагрузки и защита заголовка
	if(!crypto::seal(output, * keys, pn, header, payload))
		// Выводим отрицательный результат
		return false;
	// Продвигаем номер следующего отправляемого пакета
	item.txPn++;
	// Если собран пакет уровня приложения
	if(level == level_t::APPLICATION){
		// Учитываем пакет в лимите конфиденциальности ключей текущей фазы (RFC 9001 §6.6)
		this->_crypto.phase.packets++;
		// Если лимит конфиденциальности ключей исчерпан
		if(this->_crypto.phase.packets >= this->_crypto.aead.confidentiality){
			/**
			 * Ключи текущей фазы более непригодны: переключаемся на следующую фазу,
			 * если она выведена и пакет текущей фазы подтверждён, иначе завершаем
			 * соединение - отправка сверх лимита недопустима (RFC 9001 §6.6)
			 */
			if(this->_crypto.phase.ready && item.hasAcked && (item.largestAcked >= this->_crypto.phase.sent))
				// Выполняем переключение на следующую фазу ключей
				this->promote();
			// Ставим завершение соединения в очередь - обновление ключей невозможно
			else this->fail(error_t::AEAD_LIMIT_REACHED);
		}
	}
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод извлечения согласованного ALPN-протокола
 *
 * @return согласованный ALPN-протокол (пустое название - согласование не выполнено)
 */
awh::tls::coder_t::alpn_t awh::quic::Connection::alpn() const noexcept {
	// Выводим согласованный ALPN-протокол хендшейк-машины
	return this->_crypto.handshake.alpn();
}
/**
 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 */
void awh::quic::Connection::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры
	this->_transport.local = params;
	/**
	 * Применяем анонсируемые лимиты приёма сразу: они целиком определяются локальными
	 * параметрами и требуются ещё до завершения хендшейка - ранние данные удалённый
	 * узел отправляет под лимиты, анонсированные прошлым соединением, и разбираются
	 * они на общих основаниях (RFC 9001 §4.6.1)
	 */
	this->_flow.rxMax = params.initialMaxData;
	// Устанавливаем анонсированный лимит на двунаправленные потоки удалённого эндпоинта
	this->_limits.maxBidiLocal = params.initialMaxStreamsBidi;
	// Устанавливаем анонсированный лимит на однонаправленные потоки удалённого эндпоинта
	this->_limits.maxUniLocal = params.initialMaxStreamsUni;
}
/**
 * @brief Метод извлечения транспортных параметров удалённого узла (RFC 9000 §7.4)
 *
 * @param params транспортные параметры удалённого узла
 * @param error  код ошибки транспорта
 * @return       результат извлечения (OK/INCOMPLETE/ERROR)
 */
awh::quic::status_t awh::quic::Connection::peer(quic::params::params_t & params, error_t & error) const noexcept {
	// Извлекаем транспортные параметры удалённого узла у хендшейк-машины
	return this->_crypto.handshake.peer(params, error);
}
/**
 * @brief Метод установки адреса удалённого эндпоинта (RFC 9000 §8.1.4)
 *
 * @param address опаковое представление адреса удалённого эндпоинта
 */
void awh::quic::Connection::address(string_view address) noexcept {
	// Устанавливаем опаковое представление адреса удалённого эндпоинта
	this->_core.address.assign(address);
}
/**
 * @brief Метод установки проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
 *
 * @param mode режим проверки адреса клиента
 */
void awh::quic::Connection::retry(const bool mode) noexcept {
	// Если эндпоинт является сервером и соединение не начато
	if((this->_core.endpoint == endpoint_t::SERVER) && (this->_core.state == state_t::NONE))
		// Устанавливаем режим проверки адреса клиента
		this->_token.retry = mode;
}
/**
 * @brief Метод установки токена проверки адреса для первого пакета (RFC 9000 §8.1.3)
 *
 * @param token токен проверки адреса
 */
void awh::quic::Connection::token(string_view token) noexcept {
	// Если соединение уже начато
	if(this->_core.state != state_t::NONE)
		// Выходим из метода - токен помещается в первый пакет соединения
		return;
	// Устанавливаем токен пакетов Initial
	this->_token.initial.assign(token);
}
/**
 * @brief Метод установки общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
 *
 * @param key общий ключ вывода токенов сброса
 */
void awh::quic::Connection::resetKey(string_view key) noexcept {
	// Если соединение уже начато
	if(this->_core.state != state_t::NONE)
		// Выходим из метода - токены выдаваемых идентификаторов уже выведены
		return;
	// Устанавливаем общий ключ вывода токенов сброса
	this->_token.reset.assign(key);
}
/**
 * @brief Метод получения токена проверки адреса для будущих соединений (RFC 9000 §8.1.3)
 *
 * @return токен проверки адреса (пусто - токен не присылался)
 */
const string & awh::quic::Connection::token() const noexcept {
	// Выводим принятый от удалённого узла токен проверки адреса
	return this->_token.address;
}
/**
 * @brief Метод установки маркировки исходящих датаграмм поддержкой ECN (RFC 9000 §13.4)
 *
 * @param mode режим маркировки исходящих датаграмм
 */
void awh::quic::Connection::ecn(const bool mode) noexcept {
	// Устанавливаем режим маркировки исходящих датаграмм
	this->_marking.enabled = mode;
	// Сбрасываем флаг непройденной проверки пути - проверка начинается заново
	this->_marking.failed = false;
}
/**
 * @brief Метод установки режима следования за миграцией удалённого эндпоинта (RFC 9000 §9)
 *
 * @param mode режим следования за миграцией удалённого эндпоинта
 */
void awh::quic::Connection::roaming(const bool mode) noexcept {
	// Если следование за миграцией включается
	if(mode)
		// Устанавливаем флаг следования за миграцией удалённого эндпоинта
		this->_core.options |= options::ROAMING;
	// Снимаем флаг следования за миграцией удалённого эндпоинта
	else this->_core.options &= ~options::ROAMING;
}
/**
 * @brief Метод получения маркировки для исходящих датаграмм (RFC 9000 §13.4.2)
 *
 * @return маркировка ECN для исходящих датаграмм
 */
awh::event::ecn_t awh::quic::Connection::marking() const noexcept {
	/**
	 * Выводим маркировку поддержки ECN, пока проверка пути не провалена: код ECT(1)
	 * не применяется - он предназначен для схем с раздельным учётом, которых
	 * транспорт не ведёт (RFC 9000 §13.4.1)
	 */
	return (((this->_marking.enabled && !this->_marking.failed) ? event::ecn_t::ECT0 : event::ecn_t::NOT_ECT));
}
/**
 * @brief Метод начала соединения клиентом
 *
 * @return результат начала соединения (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::connect() noexcept {
	// Если эндпоинт не является клиентом либо соединение уже начато
	if((this->_core.endpoint != endpoint_t::CLIENT) || (this->_core.state != state_t::NONE))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Выполняем генерацию идентификатора соединения локального эндпоинта
	if(!::makeCid(this->_cids.identity.source))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Отмечаем идентификатор введённым в обращение для маршрутизации
	this->_cids.routing.added.push_back(this->_cids.identity.source);
	// Выполняем генерацию идентификатора соединения удалённого эндпоинта
	if(!::makeCid(this->_cids.identity.destination))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Запоминаем исходный DCID первого пакета Initial (RFC 9000 §7.3)
	this->_cids.identity.original = this->_cids.identity.destination;
	// Выполняем вывод ключей уровня Initial из DCID (RFC 9001 §5.2)
	if(!this->_crypto.handshake.initial(this->_cids.identity.destination))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Устанавливаем флаг наличия SCID первого пакета эндпоинта
	this->_transport.local.hasInitialScid = true;
	// Устанавливаем SCID первого пакета эндпоинта (RFC 9000 §7.3)
	this->_transport.local.initialScid = this->_cids.identity.source;
	// Устанавливаем локальные транспортные параметры хендшейк-машине
	if(!this->_crypto.handshake.params(this->_transport.local))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Выполняем начало хендшейка (формируется ClientHello)
	if(this->_crypto.handshake.start() != status_t::OK)
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Перекладываем исходящие CRYPTO-данные в буферы пространств
	this->pull();
	// Устанавливаем состояние выполнения хендшейка
	this->_core.state = state_t::HANDSHAKING;
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод обработки входящей UDP-датаграммы
 *
 * @param data буфер входящей UDP-датаграммы
 * @param size размер входящей UDP-датаграммы
 * @param now  текущее время в миллисекундах
 * @return     результат обработки (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::read(const uint8_t * data, const size_t size, const uint64_t now, const event::ecn_t ecn) noexcept {
	// Запоминаем маркировку ECN принимаемой датаграммы
	this->_marking.received = ecn;
	// Выполняем обработку входящей датаграммы
	const status_t result = this->read(data, size, now);
	// Сбрасываем маркировку ECN принятой датаграммы
	this->_marking.received = event::ecn_t::NOT_ECT;
	// Выводим результат обработки
	return result;
}
/**
 * @brief Метод обработки входящей датаграммы
 *
 * @param data данные датаграммы
 * @param size размер датаграммы
 * @param now  текущее время в миллисекундах
 * @return     результат обработки (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::read(const uint8_t * data, const size_t size, const uint64_t now) noexcept {
	// Если данные датаграммы отсутствуют
	if((data == nullptr) || (size == 0))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Обновляем текущее время последнего вызова
	this->_times.now = now;
	/**
	 * Фиксируем адрес первичного пути соединения при первой датаграмме после
	 * установления: достижимость этого пути подтверждена самим хендшейком. Смена
	 * адреса миграцией здесь не считается - она отслеживается только по успешно
	 * обработанному непробирующему пакету с наибольшим номером, а не по адресу,
	 * который сообщает вызывающий код и который подделывается (RFC 9000 §9.3)
	 */
	if((this->_core.state == state_t::CONNECTED) && !this->_core.address.empty() && this->_path.address.empty()){
		// Запоминаем адрес первичного пути соединения
		this->_path.address = this->_core.address;
		/**
		 * Запоминаем адрес последним проверенным: достижимость первичного пути
		 * подтверждена самим хендшейком, а после переезда - проверкой пути
		 */
		this->_path.previous = this->_core.address;
	}
	/**
	 * Учитываем принятые октеты для контроля анти-амплификации: считаются все октеты
	 * датаграмм, отнесённых к соединению, независимо от успеха их разбора (RFC 9000 §8.1)
	 */
	this->_amplify.received += size;
	// Если клиент не начал соединение
	if((this->_core.endpoint == endpoint_t::CLIENT) && (this->_core.state == state_t::NONE))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Если удалённый эндпоинт завершил соединение
	if(this->_core.state == state_t::DRAINING)
		// Выводим положительный результат - датаграммы игнорируются
		return status_t::OK;
	/**
	 * Копируем датаграмму в переиспользуемый буфер: снятие защиты выполняется
	 * на месте, а входной буфер вызывающего кода неизменяем. Буфер удерживает
	 * ёмкость между вызовами, поэтому выделения памяти на датаграмму не происходит
	 */
	string & buffer = this->_buffer.datagram;
	// Заполняем буфер содержимым принятой датаграммы
	buffer.assign(reinterpret_cast <const char *> (data), size);
	// Смещение очередного пакета в датаграмме
	size_t offset = 0;
	// Флаг обработки хотя бы одного пакета датаграммы
	bool processed = false;
	/**
	 *  Перебираем коалесцированные пакеты датаграммы (RFC 9000 §12.2)
	 */
	while(offset < buffer.size()){
		/**
		 * Если удалённый эндпоинт завершил соединение предыдущим пакетом датаграммы:
		 * остаток коалесцированных пакетов относится к соединению, которого больше
		 * нет. Разбор их нагрузки не только бессмыслен, но и опасен - ошибка в ней
		 * поставила бы в очередь собственное завершение, отправлять которое в
		 * завершённом состоянии запрещено (RFC 9000 §10.2.2)
		 */
		if(this->_core.state == state_t::DRAINING)
			// Прекращаем разбор датаграммы
			break;
		// Указатель на начало очередного пакета
		const uint8_t * bytes = (reinterpret_cast <const uint8_t *> (buffer.data()) + offset);
		// Количество доступных октетов пакета
		const size_t avail = (buffer.size() - offset);
		// Если пакет с длинным заголовком содержит поле версии (инварианты RFC 8999)
		if(((bytes[0] & 0x80) != 0) && (avail >= 7)){
			// Извлекаем версию протокола
			const uint32_t version = ((static_cast <uint32_t> (bytes[1]) << 24) | (static_cast <uint32_t> (bytes[2]) << 16) |
			                          (static_cast <uint32_t> (bytes[3]) << 8) | static_cast <uint32_t> (bytes[4]));
			// Если версия пакета не поддерживается
			if((version != proto::VERSION_NEGOTIATION) && (version != proto::VERSION_1)){
				// Если сервер принял первый пакет достаточного размера (RFC 9000 §5.2.2/§6.1)
				if((this->_core.endpoint == endpoint_t::SERVER) && (this->_core.state == state_t::NONE) && (size >= proto::MIN_INITIAL_SIZE)){
					// Извлекаем длину идентификатора соединения получателя (инварианты RFC 8999)
					const size_t dcidSize = static_cast <size_t> (bytes[5]);
					// Если идентификатор получателя с октетом длины отправителя помещаются в пакет
					if((dcidSize <= proto::MAX_CID_SIZE) && (avail >= (7 + dcidSize))){
						// Извлекаем длину идентификатора соединения отправителя
						const size_t scidSize = static_cast <size_t> (bytes[6 + dcidSize]);
						// Если идентификатор отправителя помещается в пакет
						if((scidSize <= proto::MAX_CID_SIZE) && (avail >= (7 + dcidSize + scidSize))){
							// Идентификатор соединения получателя пакета Version Negotiation
							cid_t dcid;
							// Идентификатор соединения отправителя пакета Version Negotiation
							cid_t scid;
							// Устанавливаем длину идентификатора получателя (SCID пакета клиента)
							dcid.size = scidSize;
							// Копируем данные идентификатора получателя
							::memcpy(dcid.data, bytes + 7 + dcidSize, scidSize);
							// Устанавливаем длину идентификатора отправителя (DCID пакета клиента)
							scid.size = dcidSize;
							// Копируем данные идентификатора отправителя
							::memcpy(scid.data, bytes + 6, dcidSize);
							// Поддерживаемые локальным эндпоинтом версии
							static const uint32_t versions[] = {proto::VERSION_1};
							// Очищаем буфер датаграммы без состояния
							this->_dgram.stateless.clear();
							// Выполняем сборку пакета Version Negotiation (RFC 9000 §6.1)
							packet::serialize::versionNegotiation(this->_dgram.stateless, dcid, scid, versions, 1);
						}
					}
				}
				// Прекращаем разбор датаграммы - границы пакета неизвестной версии ненадёжны
				break;
			}
		}
		// Разобранный заголовок пакета
		packet::header_t header;
		// Код ошибки транспорта разбора заголовка
		error_t error = error_t::NO_ERROR;
		// Выполняем разбор заголовка очередного пакета
		if(packet::parser::header(bytes, avail, this->_cids.identity.source.size, header, error) != status_t::OK){
			// Регистрируем отброшенный пакет
			this->drop("header is malformed");
			// Прекращаем разбор датаграммы - остаток отбрасывается (RFC 9000 §12.2)
			break;
		}
		// Если размер пакета не определён либо выходит за пределы датаграммы
		if((header.size == 0) || (header.size > (buffer.size() - offset))){
			// Регистрируем отброшенный пакет
			this->drop("packet size is out of datagram bounds");
			// Прекращаем разбор датаграммы
			break;
		}
		// Если принят пакет Version Negotiation либо Retry
		if((header.type == packet_t::VERSION_NEGOTIATION) || (header.type == packet_t::RETRY)){
			// Флаг приёма хотя бы одного пакета от удалённого эндпоинта
			bool received = false;
			/**
			 * Перебираем пространства номеров пакетов
			 */
			for(size_t i = 0; i < SPACES; i++)
				// Определяем наличие принятых пакетов
				received = (received || this->_spaces[i].hasRx);
			// Если эндпоинт не является клиентом в ожидании первого ответа (RFC 9000 §6.2/§17.2.5.2)
			if((this->_core.endpoint != endpoint_t::CLIENT) || (this->_core.state != state_t::HANDSHAKING) || received || this->_cids.identity.retried){
				// Регистрируем отброшенный пакет
				this->drop("version negotiation or retry is not expected");
				// Прекращаем разбор датаграммы - пакет игнорируется
				break;
			}
			// Если принят пакет Version Negotiation
			if(header.type == packet_t::VERSION_NEGOTIATION){
				// Если идентификаторы соединения не соответствуют отправленным (RFC 9000 §6.2)
				if(!(header.dcid == this->_cids.identity.source) || !(header.scid == this->_cids.identity.destination)){
					// Регистрируем отброшенный пакет
					this->drop("version negotiation identifiers mismatch");
					// Прекращаем разбор датаграммы - пакет игнорируется
					break;
				}
				// Список поддерживаемых удалённым эндпоинтом версий
				vector <uint32_t> versions;
				// Код ошибки транспорта разбора списка версий
				error_t verror = error_t::NO_ERROR;
				// Выполняем разбор списка версий пакета Version Negotiation
				if(packet::parser::versions(header, versions, verror) != status_t::OK){
					// Регистрируем отброшенный пакет
					this->drop("version negotiation list is malformed");
					// Прекращаем разбор датаграммы - пакет игнорируется
					break;
				}
				// Флаг наличия локальной версии в списке
				bool found = false;
				/**
				 * Перебираем список поддерживаемых версий
				 */
				for(auto & version : versions)
					// Определяем наличие локальной версии в списке
					found = (found || (version == proto::VERSION_1));
				// Если локальная версия присутствует в списке (RFC 9000 §6.2)
				if(found){
					// Регистрируем отброшенный пакет
					this->drop("version negotiation offers the current version");
					// Прекращаем разбор датаграммы - пакет игнорируется
					break;
				}
				// Устанавливаем код ошибки согласования версии (RFC 9368 §4)
				this->_core.error = error_t::VERSION_NEGOTIATION_ERROR;
				// Завершаем соединение молча - общей версии нет (RFC 9000 §6.2)
				this->_core.state = state_t::DRAINING;
				// Выводим положительный результат
				return status_t::OK;
			}
			// Токен пакета Retry
			string_view token;
			// Тег целостности пакета Retry
			uint8_t tag[proto::RETRY_TAG_SIZE];
			// Код ошибки транспорта разбора пакета Retry
			error_t rerror = error_t::NO_ERROR;
			// Выполняем разбор нагрузки пакета Retry
			if(packet::parser::retry(header, token, tag, rerror) != status_t::OK){
				// Регистрируем отброшенный пакет
				this->drop("retry packet is malformed");
				// Прекращаем разбор датаграммы - пакет игнорируется
				break;
			}
			// Если токен пуст либо SCID не изменился (RFC 9000 §17.2.5.2)
			if(token.empty() || (header.scid == this->_cids.identity.destination)){
				// Регистрируем отброшенный пакет
				this->drop("retry integrity tag mismatch");
				// Прекращаем разбор датаграммы - пакет игнорируется
				break;
			}
			// Выполняем проверку тега целостности пакета Retry (RFC 9001 §5.8)
			if(!crypto::retryVerify(this->_cids.identity.original, string_view(buffer.data() + offset, header.size))){
				// Регистрируем отброшенный пакет
				this->drop("retry token is empty");
				// Прекращаем разбор датаграммы - пакет игнорируется
				break;
			}
			// Устанавливаем флаг обработанного пакета Retry
			this->_cids.identity.retried = true;
			// Запоминаем SCID пакета Retry для проверки транспортных параметров (RFC 9000 §7.3)
			this->_cids.identity.retry = header.scid;
			// Устанавливаем токен последующих пакетов Initial
			this->_token.initial.assign(token);
			// Обновляем идентификатор соединения удалённого эндпоинта
			this->_cids.identity.destination = header.scid;
			// Выполняем повторный вывод ключей уровня Initial из нового DCID (RFC 9001 §5.2)
			if(!this->_crypto.handshake.initial(this->_cids.identity.destination))
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Получаем состояние пространства пакетов Initial
			auto & item = this->_spaces[static_cast <size_t> (space_t::INITIAL)];
			/**
			 * Перебираем список отправленных пакетов Initial
			 */
			for(auto & packet : item.sent){
				// Ставим содержимое пакета в очереди повторной отправки (RFC 9002 §6.3)
				this->requeue(space_t::INITIAL, packet);
				// Списываем пакет из октетов в полёте
				this->_congestion.inflight -= ::min(this->_congestion.inflight, static_cast <uint64_t> (packet.size));
			}
			// Очищаем список отправленных пакетов Initial
			item.sent.clear();
			// Сбрасываем флаг взведённого таймера детекта потерь
			item.hasLossTime = false;
			// Сбрасываем счётчик срабатываний таймера PTO
			this->_rtt.ptoCount = 0;
			// Прекращаем разбор датаграммы - пакет Retry занимает её целиком
			break;
		}
		// Если версия пакета с длинным заголовком не поддерживается
		if((header.type != packet_t::ONE_RTT) && (header.version != proto::VERSION_1))
			// Прекращаем разбор датаграммы - границы пакета ненадёжны
			break;
		// Уровень шифрования пакета
		level_t level = level_t::INITIAL;
		/**
		 * Определяем тип пакета
		 */
		switch(header.type){
			// Пакет Initial
			case packet_t::INITIAL:
				// Устанавливаем уровень шифрования Initial
				level = level_t::INITIAL;
			break;
			// Пакет Handshake
			case packet_t::HANDSHAKE:
				// Устанавливаем уровень шифрования хендшейка
				level = level_t::HANDSHAKE;
			break;
			// Пакет ранних данных
			case packet_t::ZERO_RTT: {
				/**
				 * Ранние данные отправляет только клиент: их приём клиентом означает
				 * нарушение протокола (RFC 9000 §17.2.3)
				 */
				if(this->_core.endpoint != endpoint_t::SERVER){
					// Ставим завершение соединения с нарушением протокола в очередь
					this->fail(error_t::PROTOCOL_VIOLATION);
					// Выводим отрицательный результат
					return status_t::ERROR;
				}
				// Устанавливаем уровень шифрования ранних данных
				level = level_t::EARLY_DATA;
			} break;
			// Пакет 1-RTT с коротким заголовком
			case packet_t::ONE_RTT:
				// Устанавливаем уровень шифрования приложения
				level = level_t::APPLICATION;
			break;
			// Остальные типы пакетов на данном этапе не обрабатываются
			default: {
				// Пропускаем пакет
				offset += header.size;
				// Продолжаем разбор датаграммы
				continue;
			}
		}
		// Если сервер принял пакет Initial в датаграмме меньше 1200 октетов (RFC 9000 §14.1)
		if((this->_core.endpoint == endpoint_t::SERVER) && (header.type == packet_t::INITIAL) && (size < proto::MIN_INITIAL_SIZE))
			// Отбрасываем датаграмму целиком - защита от амплификации
			break;
		// Если сервер принимает первый пакет соединения
		if((this->_core.endpoint == endpoint_t::SERVER) && (this->_core.state == state_t::NONE)){
			// Если первый пакет не является пакетом Initial либо DCID короче минимума (RFC 9000 §7.2)
			if((header.type != packet_t::INITIAL) || (header.dcid.size < MIN_INITIAL_DCID))
				// Прекращаем разбор датаграммы
				break;
			// Если включена проверка адреса клиента через пакет Retry (RFC 9000 §8.1.2)
			if(this->_token.retry){
				// Признак выдачи предъявленного токена пакетом Retry
				bool retried = false;
				// Восстановленный из предъявленного токена исходный DCID
				cid_t restored;
				/**
				 * Определяем предъявление токена, выданного фреймом NEW_TOKEN: такой
				 * токен выдан на прошлом соединении и пакета Retry за собой не имеет,
				 * поэтому не прошедший проверку заменяется выдачей Retry, а не
				 * отбрасыванием датаграммы (RFC 9000 §8.1.3)
				 */
				const bool addressed = (!header.token.empty() && (static_cast <uint8_t> (header.token[0]) == ADDRESS_TOKEN_MARK));
				// Если пакет Initial не содержит токена либо предъявленный токен адреса не прошёл проверку
				if(header.token.empty() || (addressed && !this->validate(header.token, restored, retried))){
					// Запоминаем исходный DCID первого пакета Initial клиента
					this->_cids.identity.original = header.dcid;
					// Выполняем генерацию нового идентификатора соединения пакета Retry
					if(!::makeCid(this->_cids.identity.retry))
						// Выводим отрицательный результат
						return status_t::ERROR;
					/**
					 * Отмечаем идентификатор введённым в обращение для маршрутизации:
					 * повторный пакет Initial клиент адресует именно ему, поэтому
					 * маршрут обязан существовать до отправки пакета Retry
					 */
					this->_cids.routing.added.push_back(this->_cids.identity.retry);
					/**
					 * Формируем токен проверки адреса, заверенный от адреса клиента:
					 * состояние выдачи не сохраняется, проверка выполняется пересчётом
					 * кода аутентичности (RFC 9000 §8.1.4)
					 */
					if(!this->token(RETRY_TOKEN_MARK, this->_cids.identity.original, this->_token.retried))
						// Выводим отрицательный результат
						return status_t::ERROR;
					// Нулевой тег целостности для сборки пакета
					uint8_t tag[proto::RETRY_TAG_SIZE] = {0};
					// Очищаем буфер датаграммы без состояния
					this->_dgram.stateless.clear();
					// Выполняем сборку пакета Retry с нулевым тегом (RFC 9000 §17.2.5)
					if(!packet::serialize::retry(this->_dgram.stateless, proto::VERSION_1, header.scid, this->_cids.identity.retry, this->_token.retried, tag))
						// Выводим отрицательный результат
						return status_t::ERROR;
					// Вычисляем тег целостности по пакету без тега (RFC 9001 §5.8)
					if(!crypto::retryTag(this->_cids.identity.original, string_view(this->_dgram.stateless.data(), this->_dgram.stateless.size() - proto::RETRY_TAG_SIZE), tag)){
						// Очищаем буфер датаграммы без состояния
						this->_dgram.stateless.clear();
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Заменяем нулевой тег вычисленным тегом целостности
					this->_dgram.stateless.replace(this->_dgram.stateless.size() - proto::RETRY_TAG_SIZE, proto::RETRY_TAG_SIZE, reinterpret_cast <const char *> (tag), proto::RETRY_TAG_SIZE);
					// Прекращаем разбор датаграммы - соединение продолжится по токену
					break;
				}
				/**
				 * Проверяем токен пересчётом кода аутентичности: исходный DCID
				 * восстанавливается из самого токена, поэтому проверить его вправе
				 * и объект соединения, не выдававший токен (RFC 9000 §8.1.4)
				 */
				if(!addressed && !this->validate(header.token, restored, retried))
					// Прекращаем разбор датаграммы - токен некорректен либо истёк
					break;
				// Если токен выдан пакетом Retry
				if(retried){
					// Запоминаем восстановленный из токена исходный DCID первого пакета клиента
					this->_cids.identity.original = restored;
					/**
					 * Идентификатор получателя повторного пакета Initial и есть SCID
					 * выданного пакета Retry: он же становится идентификатором
					 * локального эндпоинта (RFC 9000 §7.3)
					 */
					this->_cids.identity.retry = header.dcid;
					// Используем SCID пакета Retry идентификатором локального эндпоинта
					this->_cids.identity.source = this->_cids.identity.retry;
					// Устанавливаем флаг продолжения соединения после пакета Retry
					this->_cids.identity.retried = true;
				/**
				 * Если токен выдан фреймом NEW_TOKEN: пакета Retry не было, поэтому
				 * идентификатор локального эндпоинта генерируется обычным порядком,
				 * а исходным DCID остаётся идентификатор первого пакета клиента
				 */
				} else {
					// Выполняем генерацию идентификатора соединения локального эндпоинта
					if(!::makeCid(this->_cids.identity.source))
						// Выводим отрицательный результат
						return status_t::ERROR;
					// Запоминаем исходный DCID первого пакета Initial клиента
					this->_cids.identity.original = header.dcid;
				}
				// Отмечаем идентификатор введённым в обращение для маршрутизации
				this->_cids.routing.added.push_back(this->_cids.identity.source);
				/**
				 * Возврат корректного токена подтверждает адрес клиента: лимит
				 * анти-амплификации снимается (RFC 9000 §8.1.2)
				 */
				this->_amplify.validated = true;
				// Возобновляем поиск размера пути после подтверждения адреса (RFC 8899)
				this->discover();
			// Если проверка адреса клиента не выполняется
			} else {
				// Выполняем генерацию идентификатора соединения локального эндпоинта
				if(!::makeCid(this->_cids.identity.source))
					// Выводим отрицательный результат
					return status_t::ERROR;
				// Отмечаем идентификатор введённым в обращение для маршрутизации
				this->_cids.routing.added.push_back(this->_cids.identity.source);
				// Запоминаем исходный DCID первого пакета Initial клиента
				this->_cids.identity.original = header.dcid;
			}
			// Устанавливаем идентификатор соединения удалённого эндпоинта
			this->_cids.identity.destination = header.scid;
			/**
			 * Выполняем вывод ключей уровня Initial из идентификатора получателя
			 * принятого пакета: после пакета Retry секреты Initial пересчитываются
			 * от выбранного сервером идентификатора, а не от исходного DCID, который
			 * служит только транспортным параметром (RFC 9001 §5.2)
			 */
			if(!this->_crypto.handshake.initial(header.dcid))
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Устанавливаем флаг наличия SCID первого пакета эндпоинта
			this->_transport.local.hasInitialScid = true;
			// Устанавливаем SCID первого пакета эндпоинта (RFC 9000 §7.3)
			this->_transport.local.initialScid = this->_cids.identity.source;
			/**
			 * Анонсируем токен сброса идентификатора хендшейка: без него удалённый узел
			 * не распознает сброс, отправленный на этот идентификатор, и продолжит
			 * отправку до самого таймаута простоя. Заданный приложением токен
			 * приоритетнее выведенного (RFC 9000 §10.3)
			 */
			if(!this->_transport.local.hasResetToken)
				// Выводим токен сброса идентификатора хендшейка на общем ключе
				this->_transport.local.hasResetToken = quic::resetToken(this->_token.reset, this->_cids.identity.source, this->_transport.local.resetToken);
			// Устанавливаем флаг наличия исходного DCID первого пакета Initial клиента
			this->_transport.local.hasOdcid = true;
			// Устанавливаем исходный DCID первого пакета Initial клиента (RFC 9000 §7.3)
			this->_transport.local.odcid = this->_cids.identity.original;
			// Если соединение продолжается после пакета Retry
			if(this->_cids.identity.retried){
				// Устанавливаем флаг наличия SCID пакета Retry
				this->_transport.local.hasRetryScid = true;
				// Устанавливаем SCID пакета Retry (RFC 9000 §7.3)
				this->_transport.local.retryScid = this->_cids.identity.retry;
			}
			// Устанавливаем локальные транспортные параметры хендшейк-машине
			if(!this->_crypto.handshake.params(this->_transport.local))
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Выполняем начало хендшейка (сервер ожидает ClientHello)
			if(this->_crypto.handshake.start() != status_t::OK)
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Устанавливаем состояние выполнения хендшейка
			this->_core.state = state_t::HANDSHAKING;
		}
		/**
		 * Пакеты уровня приложения (1-RTT) до завершения хендшейка не обрабатываются:
		 * их разбор до подтверждения подлинности собеседника недопустим, даже когда
		 * ключи снятия защиты уже выведены. Криптографический бэкенд удерживает эти
		 * ключи до завершения хендшейка, но полагаться лишь на его тайминг нельзя -
		 * коалесцированный ранее пакет 1-RTT дождётся перехода в CONNECTED тем же
		 * разбором датаграммы, а пришедший отдельно будет переотправлен (RFC 9001 §5.7).
		 * Проверка ограничена состоянием хендшейка: в состояниях завершения пакеты
		 * уровня приложения обрабатываются особо для повтора CONNECTION_CLOSE
		 */
		if((level == level_t::APPLICATION) && (this->_core.state == state_t::HANDSHAKING)){
			// Пропускаем ранний пакет уровня приложения
			offset += header.size;
			// Продолжаем разбор датаграммы
			continue;
		}
		// Получаем ключи снятия защиты входящих пакетов уровня
		const crypto::keys_t * keys = this->_crypto.handshake.decryption(level);
		// Если ключи уровня не выведены либо уже сброшены
		if(keys == nullptr){
			// Пропускаем пакет - расшифровать невозможно
			offset += header.size;
			// Продолжаем разбор датаграммы
			continue;
		}
		// Получаем состояние пространства номеров пакетов
		auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
		// Восстановленный полный номер пакета
		uint64_t pn = 0;
		// Расшифрованная нагрузка пакета в переиспользуемом буфере
		string & plain = this->_buffer.plain;
		// Код ошибки транспорта снятия защиты
		error_t oerror = error_t::NO_ERROR;
		// Флаг возможности сохранения защищённых октетов заголовка
		const bool snapshot = ((header.pnOffset + proto::MAX_PKT_NUM_SIZE) <= header.size);
		// Сохраняем защищённые октеты заголовка для повторных попыток расшифровки
		uint8_t guarded[1 + proto::MAX_PKT_NUM_SIZE] = {0};
		// Если октеты номера пакета помещаются в датаграмму
		if(snapshot){
			// Копируем первый октет пакета
			guarded[0] = static_cast <uint8_t> (buffer[offset]);
			// Копируем октеты номера пакета
			::memcpy(guarded + 1, buffer.data() + offset + header.pnOffset, proto::MAX_PKT_NUM_SIZE);
		}
		// Выполняем снятие защиты пакета: защита заголовка и AEAD-расшифровка
		if(crypto::open(reinterpret_cast <uint8_t *> (buffer.data()) + offset, header.size, header.pnOffset, (item.hasRx ? item.largestRx : 0), * keys, pn, plain, oerror) != status_t::OK){
			// Флаг успешного снятия защиты ключами другой фазы
			bool unsealed = false;
			/**
			 * Бит фазы ключей снят вместе с защитой заголовка ещё до отказа AEAD:
			 * ключ защиты заголовка при смене фазы не меняется, поэтому бит достоверен
			 * даже при неверных ключах нагрузки (RFC 9001 §6). Ключами другой фазы
			 * имеет смысл пробовать только при несовпадении бита с текущей фазой -
			 * иначе отказ вызван повреждением пакета, а не сменой ключей
			 */
			const bool phase = ((static_cast <uint8_t> (buffer[offset]) & 0x04) != 0);
			// Если пакет уровня приложения, хендшейк подтверждён и фаза ключей не совпадает
			if((level == level_t::APPLICATION) && (this->_core.flags & flags::CONFIRMED) && snapshot && (phase != this->_crypto.phase.current)){
				// Если выведены ключи следующей фазы
				if(this->_crypto.phase.ready){
					// Восстанавливаем первый защищённый октет пакета
					buffer[offset] = static_cast <char> (guarded[0]);
					// Восстанавливаем защищённые октеты номера пакета
					::memcpy(&buffer[offset + header.pnOffset], guarded + 1, proto::MAX_PKT_NUM_SIZE);
					// Выполняем снятие защиты ключами следующей фазы (обновление ключей пиром)
					if(crypto::open(reinterpret_cast <uint8_t *> (buffer.data()) + offset, header.size, header.pnOffset, (item.hasRx ? item.largestRx : 0), this->_crypto.phase.nextRead, pn, plain, oerror) == status_t::OK){
						// Выполняем переключение на следующую фазу ключей (RFC 9001 §6.2)
						this->promote();
						// Устанавливаем флаг успешного снятия защиты
						unsealed = true;
					}
				}
				// Если снятие защиты не выполнено и есть ключи предыдущей фазы
				if(!unsealed && this->_crypto.phase.hasPrevious){
					// Восстанавливаем первый защищённый октет пакета
					buffer[offset] = static_cast <char> (guarded[0]);
					// Восстанавливаем защищённые октеты номера пакета
					::memcpy(&buffer[offset + header.pnOffset], guarded + 1, proto::MAX_PKT_NUM_SIZE);
					// Выполняем снятие защиты ключами предыдущей фазы (отставший пакет)
					unsealed = (crypto::open(reinterpret_cast <uint8_t *> (buffer.data()) + offset, header.size, header.pnOffset, (item.hasRx ? item.largestRx : 0), this->_crypto.phase.prevRead, pn, plain, oerror) == status_t::OK);
				}
			}
			// Если снятие защиты не выполнено ни одним набором ключей
			if(!unsealed){
				// Учитываем неудачное снятие защиты в лимите целостности AEAD (RFC 9001 §6.6)
				this->_crypto.aead.failures++;
				// Если лимит целостности AEAD исчерпан
				if(this->_crypto.aead.failures >= this->_crypto.aead.integrity){
					// Ставим завершение соединения в очередь - ключи более не пригодны
					this->fail(error_t::AEAD_LIMIT_REACHED);
					// Выводим отрицательный результат
					return status_t::ERROR;
				}
				// Пропускаем повреждённый либо чужой пакет (RFC 9000 §12.2)
				offset += header.size;
				// Продолжаем разбор датаграммы
				continue;
			}
		}
		// Если пакет с таким номером уже был принят
		if(this->duplicate(this->space(level), pn)){
			// Пропускаем дубликат пакета
			offset += header.size;
			// Продолжаем разбор датаграммы
			continue;
		}
		// Если клиент принял первый ответ сервера с длинным заголовком
		if((this->_core.endpoint == endpoint_t::CLIENT) && !this->_cids.identity.updated && (header.type != packet_t::ONE_RTT)){
			// Обновляем идентификатор соединения удалённого эндпоинта (RFC 9000 §7.2)
			this->_cids.identity.destination = header.scid;
			// Устанавливаем флаг обновления DCID по первому ответу сервера
			this->_cids.identity.updated = true;
		}
		// Если локальный эндпоинт завершил соединение
		if(this->_core.state == state_t::CLOSING){
			// Если фрейм CONNECTION_CLOSE уже отправлен
			if(this->_close.sent){
				// Учитываем очередной принятый пакет после отправки завершения
				this->_close.received++;
				// Если порог принятых пакетов достигнут (RFC 9000 §10.2.1)
				if(this->_close.received >= this->_close.threshold){
					// Сбрасываем счётчик принятых пакетов
					this->_close.received = 0;
					// Удваиваем порог повторной отправки (экспоненциальная выдержка)
					this->_close.threshold <<= 1;
					// Разрешаем повторную отправку фрейма CONNECTION_CLOSE
					this->_close.sent = false;
				}
			}
			/**
			 * Нагрузка принятых пакетов в состоянии завершения не разбирается:
			 * эндпоинт отвечает на них только повторным фреймом CONNECTION_CLOSE
			 * и никаких иных фреймов не обрабатывает (RFC 9000 §10.2.1). Разбор
			 * оставлял бы поверхность атаки на уже закрываемом соединении
			 */
			offset += header.size;
			// Устанавливаем флаг обработки пакета датаграммы
			processed = true;
			// Продолжаем разбор датаграммы
			continue;
		}
		// Устанавливаем флаг обработки пакета датаграммы
		processed = true;
		/**
		 * Определяем, станет ли пакет наибольшим по номеру в пространстве приложения:
		 * миграцию вправе инициировать лишь непробирующий пакет с наибольшим номером,
		 * поэтому признак снимается до регистрации его номера (RFC 9000 §9.3)
		 */
		const auto & appItem = this->_spaces[static_cast <size_t> (space_t::APPLICATION)];
		// Признак наибольшего номера пакета уровня приложения
		const bool largest = ((level == level_t::APPLICATION) && (!appItem.hasRx || (pn > appItem.largestRx)));
		// Регистрируем принятый номер пакета в диапазонах пространства
		this->record(this->space(level), pn);
		// Признак наличия непробирующего фрейма в пакете (RFC 9000 §9.1)
		bool nonProbing = false;
		// Выполняем разбор и диспетчеризацию фреймов нагрузки пакета
		if(this->frames(level, reinterpret_cast <const uint8_t *> (plain.data()), plain.size(), nonProbing) != status_t::OK)
			// Выводим отрицательный результат
			return status_t::ERROR;
		/**
		 * Отслеживаем миграцию соединения на новый путь: адрес отправки меняется
		 * только в ответ на успешно обработанный непробирующий пакет с наибольшим
		 * номером, пришедший с нового адреса. Реакция на неаутентифицированные байты
		 * позволяла бы off-path атакующему перенаправить путь и сбросить оценки
		 * перегрузки, поэтому проверка выполняется после снятия защиты (RFC 9000 §9.3)
		 */
		if((this->_core.options & options::ROAMING) && (level == level_t::APPLICATION) && (this->_core.state == state_t::CONNECTED) && nonProbing && largest &&
		   !this->_core.address.empty() && !this->_path.address.empty() && (this->_core.address != this->_path.address)){
			/**
			 * Если выполняется проверка достижимости другого адреса: отказываемся от неё
			 * до записи нового адреса и без возврата на проверенный. Сменив адрес отправки,
			 * эндпоинт вправе бросить проверки прочих адресов, а возврат затёр бы адрес,
			 * на который соединение как раз переходит (RFC 9000 §9.3.1)
			 */
			if(this->_path.pending)
				// Выполняем отказ от проверки достижимости прежнего адреса
				this->abandon(false);
			// Запоминаем адрес нового пути соединения
			this->_path.address = this->_core.address;
			// Записываем в лог сообщение о миграции соединения на новый путь
			this->_log->print(
				"QUIC connection migrated to a new path: %s", log_t::flag_t::INFO,
				this->_path.address.c_str()
			);
			// Выполняем сброс состояния пути соединения со сменой адреса удалённого эндпоинта
			this->repath(true);
			/**
			 * Возвращаем октеты вызвавшей миграцию датаграммы в лимит анти-амплификации:
			 * repath обнулил учёт нового пути, а принятая с него датаграмма - первый
			 * подтверждённый объём, дающий право на ответ проверкой пути (RFC 9000 §8.1)
			 */
			this->_amplify.received += size;
			// Начинаем проверку достижимости нового пути
			this->probe();
		}
		// Обновляем время последнего принятого и обработанного пакета (RFC 9000 §10.1)
		this->_times.idle = this->_times.now;
		/**
		 * Разрешаем однократный перезапуск отсчёта отправкой: приём пакета
		 * подтвердил, что удалённый эндпоинт жив, поэтому начатая после него
		 * активность вправе продлить соединение (RFC 9000 §10.1)
		 */
		this->_core.flags &= ~flags::IDLE_ELICITED;
		// Если сервер успешно обработал пакет уровня Handshake
		if((this->_core.endpoint == endpoint_t::SERVER) && (level == level_t::HANDSHAKE)){
			/**
			 * Успешно расшифрованный пакет Handshake подтверждает адрес клиента:
			 * лимит анти-амплификации снимается (RFC 9000 §8.1)
			 */
			this->_amplify.validated = true;
			// Возобновляем поиск размера пути после подтверждения адреса (RFC 8899)
			this->discover();
			// Если ключи уровня Initial ещё не сброшены
			if(this->_crypto.handshake.decryption(level_t::INITIAL) != nullptr)
				// Сбрасываем ключи уровня Initial (RFC 9001 §4.9.1)
				this->discard(level_t::INITIAL);
		}
		/**
		 * Если хендшейк успешно завершён - применяем параметры сразу,
		 * не дожидаясь конца датаграммы: следующие коалесцированные
		 * пакеты 1-RTT могут содержать фреймы STREAM (RFC 9000 §12.2)
		 */
		if((this->_core.state == state_t::HANDSHAKING) && (this->_crypto.handshake.state() == handshake_t::state_t::COMPLETED)){
			// Выполняем применение транспортных параметров удалённого эндпоинта
			if(!this->established())
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Устанавливаем состояние установленного соединения
			this->_core.state = state_t::CONNECTED;
			/**
			 * Сбрасываем ключи уровня ранних данных: с выводом ключей уровня приложения
			 * отправка ранних пакетов запрещена. Общий метод сброса ключей здесь
			 * неприменим - уровни делят пространство номеров пакетов, и он снял бы
			 * с учёта отправленные ранние пакеты, которые ещё будут подтверждены
			 * удалённым узлом. Сервер ключи ранних данных удерживает: они у него
			 * только на чтение и позволяют разобрать переупорядоченные ранние
			 * пакеты, пришедшие после хендшейка (RFC 9001 §4.9.3)
			 */
			if((this->_core.endpoint == endpoint_t::CLIENT) && (this->_crypto.handshake.encryption(level_t::EARLY_DATA) != nullptr))
				// Сбрасываем ключи защиты исходящих пакетов уровня ранних данных
				this->_crypto.handshake.discard(level_t::EARLY_DATA);
			// Если локальный эндпоинт является сервером
			if(this->_core.endpoint == endpoint_t::SERVER){
				// Устанавливаем флаг необходимости отправки фрейма HANDSHAKE_DONE (RFC 9000 §19.20)
				this->_core.flags |= flags::HANDSHAKE_DONE;
				/**
				 * Формируем токен проверки адреса для будущих соединений: он выдаётся
				 * только при включённой проверке адреса - без неё пакет Retry не
				 * отправляется, и пропускать клиенту нечего (RFC 9000 §8.1.3)
				 */
				if(this->_token.retry && this->_token.address.empty()){
					// Идентификатор соединения токена: токен фрейма NEW_TOKEN его не несёт
					const cid_t empty;
					// Если формирование токена проверки адреса выполнено
					if(this->token(ADDRESS_TOKEN_MARK, empty, this->_token.address))
						// Устанавливаем флаг необходимости отправки фрейма NEW_TOKEN
						this->_token.queued = true;
					// Записываем предупреждение в лог - будущие соединения пройдут через пакет Retry
					else this->_log->print("QUIC address validation token is not issued", log_t::flag_t::WARNING);
				}
				// Устанавливаем флаг подтверждения хендшейка (RFC 9001 §4.1.2)
				this->_core.flags |= flags::CONFIRMED;
				// Начинаем поиск размера пути зондированием (RFC 8899)
				this->discover();
				// Сбрасываем ключи уровня Handshake (RFC 9001 §4.9.2)
				this->discard(level_t::HANDSHAKE);
			}
		}
		// Переходим к следующему пакету датаграммы
		offset += header.size;
	}
	/**
	 * Если ни один пакет датаграммы обработать не удалось - датаграмма может
	 * оказаться сбросом без сохранения состояния. Проверка выполняется только
	 * здесь: для успешно обработанной датаграммы совпадение хвоста с токеном
	 * было бы случайным (RFC 9000 §10.3.1)
	 */
	if(!processed && this->stateless(data, size)){
		/**
		 * Удалённый эндпоинт утратил состояние соединения: переходим в состояние
		 * завершения молча, отправка любых пакетов далее запрещена (RFC 9000 §10.3)
		 */
		this->_core.state = state_t::DRAINING;
		// Записываем в лог сообщение о приёме сброса без сохранения состояния
		this->_log->print("QUIC connection terminated by stateless reset", log_t::flag_t::WARNING);
		// Выводим положительный результат
		return status_t::OK;
	}
	// Если хендшейк завершился ошибкой и завершение соединения ещё не в очереди
	if((this->_crypto.handshake.state() == handshake_t::state_t::FAILED) && !this->_close.queued)
		// Ставим завершение соединения с ошибкой хендшейка в очередь
		this->fail(this->_crypto.handshake.error());
	// Если удалённый узел отказал в ранних данных и отказ ещё не обработан (RFC 9001 §4.6.2)
	if(!(this->_core.flags & flags::RESTORED) && this->_crypto.handshake.rejected()){
		// Устанавливаем флаг обработанного отказа в ранних данных
		this->_core.flags |= flags::RESTORED;
		// Возвращаем содержимое отправленных ранних данных в очереди отправки
		this->restore();
	}
	// Перекладываем исходящие CRYPTO-данные в буферы пространств
	this->pull();
	// Выводим ключи следующей фазы после подтверждения хендшейка (RFC 9001 §6)
	this->prepare();
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод сборки исходящей UDP-датаграммы
 *
 * @param output буфер исходящей UDP-датаграммы (очищается)
 * @param now    текущее время в миллисекундах
 * @return       результат сборки (true - датаграмма готова к отправке)
 */
bool awh::quic::Connection::write(string & output, const uint64_t now) noexcept {
	// Очищаем буфер исходящей датаграммы
	output.clear();
	// Сбрасываем признак адресации датаграммы предпочтительному адресу
	this->_path.alternate = false;
	// Если подготовлена датаграмма без состояния (Version Negotiation либо Retry)
	if(!this->_dgram.stateless.empty()){
		// Передаём датаграмму без состояния вызывающему коду
		output.swap(this->_dgram.stateless);
		// Очищаем буфер датаграммы без состояния
		this->_dgram.stateless.clear();
		/**
		 * Учитываем отправленные октеты в контроле анти-амплификации: датаграммы
		 * без состояния всегда короче вызвавшей их датаграммы, но исключать их
		 * из учёта нельзя - иначе лимит считается по неполному объёму (RFC 9000 §8.1)
		 */
		this->_amplify.sent += output.size();
		// Выводим положительный результат
		return true;
	}
	// Если соединение не начато
	if(this->_core.state == state_t::NONE)
		// Выводим отрицательный результат
		return false;
	// Обновляем текущее время последнего вызова
	this->_times.now = now;
	// Вычисляем доступный к отправке объём данных (RFC 9000 §8.1)
	const size_t capacity = this->allowance();
	// Если лимит анти-амплификации исчерпан до подтверждения адреса
	if(capacity == 0)
		// Выводим отрицательный результат - отправка запрещена
		return false;
	// Если активности на соединении ещё не было
	if(this->_times.idle == 0)
		// Начинаем отсчёт таймаута простоя с первой отправки (RFC 9000 §10.1)
		this->_times.idle = now;
	// Перекладываем исходящие CRYPTO-данные в буферы пространств
	this->pull();
	// Выводим ключи следующей фазы после подтверждения хендшейка (RFC 9001 §6)
	this->prepare();
	// Удаляем завершённые потоки приложения
	this->collect();
	/**
	 * Если завершение соединения поставлено в очередь и ещё не отправлено. Опередивший
	 * отправку приём завершения от удалённого эндпоинта её отменяет: эндпоинт в
	 * состоянии завершения удалённым узлом не вправе отправлять пакеты (RFC 9000 §10.2.2)
	 */
	if(this->_close.queued && !this->_close.sent && (this->_core.state != state_t::DRAINING)){
		/**
		 * С выведенными ключами уровня приложения довольно одного пакета 1-RTT.
		 * До этого хендшейк ещё не подтверждён, и удалённый узел может не иметь
		 * ключей старших уровней, поэтому завершение дублируется в пакетах Initial
		 * и Handshake, коалесцированных в одну датаграмму в порядке возрастания
		 * уровня - так узел прочтёт его любыми доступными ключами (RFC 9000 §10.2.3/§12.2)
		 */
		const bool application = (this->_crypto.handshake.encryption(level_t::APPLICATION) != nullptr);
		// Уровень отправки завершения после вывода ключей приложения
		static const level_t single[] = {level_t::APPLICATION};
		// Уровни отправки завершения до подтверждения хендшейка (в порядке коалесценции)
		static const level_t multiple[] = {level_t::INITIAL, level_t::HANDSHAKE};
		// Выбираем список уровней отправки завершения
		const level_t * levels = (application ? single : multiple);
		// Количество уровней отправки завершения
		const size_t count = (application ? 1 : 2);
		// Готовые к отправке уровни шифрования (у которых выведены ключи защиты)
		level_t ready[2];
		// Количество готовых уровней шифрования
		size_t total = 0;
		// Флаг наличия пакета Initial в датаграмме завершения
		bool initial = false;
		/**
		 * Отбираем уровни шифрования с выведенными ключами защиты
		 */
		for(size_t i = 0; i < count; i++){
			// Если ключи защиты исходящих пакетов уровня выведены
			if(this->_crypto.handshake.encryption(levels[i]) != nullptr){
				// Запоминаем готовый уровень шифрования
				ready[total++] = levels[i];
				// Отмечаем наличие пакета Initial в датаграмме
				initial = (initial || (levels[i] == level_t::INITIAL));
			}
		}
		// Если ни один уровень не готов - ключи недоступны
		if(total == 0)
			// Выводим отрицательный результат
			return false;
		/**
		 * Собираем и защищаем пакеты завершения в порядке возрастания уровня
		 */
		for(size_t i = 0; i < total; i++){
			// Уровень шифрования очередного пакета завершения
			const level_t level = ready[i];
			// Нагрузка пакета завершения соединения
			string payload = "";
			// Если ошибка приложения отправляется в пакете Initial или Handshake (RFC 9000 §10.2.3)
			if(this->_close.app && (level != level_t::APPLICATION))
				// Выполняем сборку фрейма CONNECTION_CLOSE с кодом APPLICATION_ERROR без причины
				frame::serialize::connectionClose(payload, static_cast <uint64_t> (error_t::APPLICATION_ERROR), 0, "", false);
			// Если завершение отправляется штатно
			else
				// Выполняем сборку фрейма CONNECTION_CLOSE (RFC 9000 §10.2.1)
				frame::serialize::connectionClose(payload, this->_close.code, 0, this->_close.reason, this->_close.app);
			// Если нагрузка меньше минимума для выборки защиты заголовка
			if(payload.size() < MIN_PAYLOAD_SIZE)
				// Дополняем нагрузку фреймами PADDING
				frame::serialize::padding(payload, MIN_PAYLOAD_SIZE - payload.size());
			/**
			 * Последний пакет датаграммы с пакетом Initial дополняется PADDING до
			 * минимального размера датаграммы: короткую датаграмму с пакетом Initial
			 * соответствующий стандарту сервер отбрасывает (RFC 9000 §14.1). Под лимитом
			 * анти-амплификации дополнение не выполняется - лимит имеет приоритет
			 */
			if(((i + 1) == total) && initial && (capacity >= proto::MIN_INITIAL_SIZE)){
				// Получаем состояние пространства номеров пакетов уровня
				const auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
				// Вычисляем размер кодирования номера пакета
				const size_t pnSize = packet::packetNumberSize(item.txPn, (item.hasAcked ? item.largestAcked : item.txPn));
				// Вычисляем размер заголовка при двухоктетном поле Length (верно для датаграмм ~1200 октетов)
				const size_t header = this->headerSize(level, proto::MIN_INITIAL_SIZE, pnSize, this->_cids.identity.destination);
				// Вычисляем накладной размер датаграммы без нагрузки последнего пакета
				const size_t overhead = (output.size() + header + crypto::AEAD_TAG_SIZE);
				// Если нагрузку требуется нарастить до минимального размера датаграммы
				if((overhead < proto::MIN_INITIAL_SIZE) && ((proto::MIN_INITIAL_SIZE - overhead) > payload.size()))
					// Дополняем нагрузку фреймами PADDING до минимального размера датаграммы
					frame::serialize::padding(payload, (proto::MIN_INITIAL_SIZE - overhead) - payload.size());
			}
			// Выполняем сборку и защиту пакета завершения, дописывая его в датаграмму
			if(!this->seal(output, level, payload, this->_cids.identity.destination)){
				// Откатываем буфер датаграммы
				output.clear();
				// Выводим отрицательный результат
				return false;
			}
		}
		// Если датаграмма завершения не помещается в доступный объём отправки
		if(output.size() > capacity){
			// Откатываем буфер датаграммы
			output.clear();
			// Выводим отрицательный результат
			return false;
		}
		// Учитываем отправленные октеты в контроле анти-амплификации
		this->_amplify.sent += output.size();
		// Устанавливаем флаг выполненной отправки фрейма CONNECTION_CLOSE
		this->_close.sent = true;
		/**
		 * Если дедлайн периода завершения ещё не взведён: период отсчитывается
		 * от первой отправки фрейма завершения, а его повторы, отправляемые
		 * в ответ на приходящие пакеты, дедлайн не отодвигают (RFC 9000 §10.2)
		 */
		if(this->_close.deadline == 0)
			// Взводим дедлайн периода завершения соединения
			this->_close.deadline = (now + (CLOSING_PERIOD * this->interval(space_t::APPLICATION)));
		// Выводим положительный результат
		return true;
	}
	// Если соединение завершается либо завершено удалённым эндпоинтом
	if((this->_core.state == state_t::CLOSING) || (this->_core.state == state_t::DRAINING))
		// Выводим отрицательный результат
		return false;
	/**
	 * Если требуется проверка достижимости предпочтительного адреса: она уходит
	 * отдельной датаграммой на сам предпочтительный адрес, а не по текущему пути,
	 * и несёт одни пробирующие фреймы. Соединение при этом продолжает работать
	 * по текущему пути - переезжать дозволено лишь на проверенный адрес
	 * (RFC 9000 §9.1/§9.6.2)
	 */
	if(this->_path.relocating && this->_path.queued && (this->_crypto.handshake.encryption(level_t::APPLICATION) != nullptr)){
		// Сбрасываем флаг необходимости отправки фрейма PATH_CHALLENGE
		this->_path.queued = false;
		// Получаем состояние пространства номеров пакетов приложения
		auto & item = this->_spaces[static_cast <size_t> (space_t::APPLICATION)];
		// Получаем переиспользуемый буфер нагрузки уровня приложения
		string & buffer = this->_buffer.payload[static_cast <size_t> (level_t::APPLICATION)];
		// Очищаем буфер от результатов предыдущей сборки
		buffer.clear();
		// Выполняем сборку фрейма PATH_CHALLENGE (RFC 9000 §19.17)
		frame::serialize::path(buffer, frame_t::PATH_CHALLENGE, this->_path.probe);
		// Вычисляем размер кодирования номера отправляемого пакета
		const size_t pnSize = packet::packetNumberSize(item.txPn, (item.hasAcked ? item.largestAcked : item.txPn));
		/**
		 * Вычисляем точные накладные расходы пакета: заголовок с номером пакета
		 * и тег AEAD. Оценка с запасом здесь неприменима - датаграмма обязана
		 * достичь минимального размера ровно (RFC 9000 §8.2.1)
		 */
		const size_t reserve = (this->headerSize(level_t::APPLICATION, proto::MIN_INITIAL_SIZE, pnSize, this->_path.relocation) + crypto::AEAD_TAG_SIZE);
		/**
		 * Если датаграмма не достигает минимального размера: датаграмма с фреймом
		 * проверки дополняется до него - проверка подтверждает пригодность пути
		 * к переносу датаграмм именно такого размера (RFC 9000 §8.2.1)
		 */
		if((reserve + buffer.size()) < proto::MIN_INITIAL_SIZE)
			// Дополняем нагрузку фреймами PADDING до минимального размера датаграммы
			frame::serialize::padding(buffer, proto::MIN_INITIAL_SIZE - reserve - buffer.size());
		// Учётная запись пробирующего пакета
		sent_t meta;
		// Устанавливаем номер отправляемого пакета
		meta.pn = item.txPn;
		/**
		 * Устанавливаем флаг отправки пакета вне текущего пути: контроль перегрузки
		 * и оценка задержки текущего пути к пробирующему пакету на другой адрес
		 * отношения не имеют
		 */
		meta.stale = true;
		// Запоминаем управляющий фрейм в учётной записи пакета для повтора при потере
		meta.control.emplace_back(frame_t::PATH_CHALLENGE, 0);
		/**
		 * Выполняем сборку и защиту пакета идентификатором предпочтительного адреса:
		 * прежний идентификатор на новом пути позволил бы наблюдателю связать
		 * пути между собой (RFC 9000 §9.5)
		 */
		if(!this->seal(output, level_t::APPLICATION, buffer, this->_path.relocation)){
			// Очищаем буфер исходящей датаграммы
			output.clear();
			// Выводим отрицательный результат - собрать проверку не удалось
			return false;
		}
		// Устанавливаем время отправки пробирующего пакета
		meta.time = now;
		// Устанавливаем размер пробирующего пакета
		meta.size = output.size();
		// Устанавливаем время отправки последнего ack-eliciting пакета
		item.lastElicited = now;
		// Устанавливаем флаг наличия отправленных ack-eliciting пакетов
		item.hasElicited = true;
		// Добавляем учётную запись пробирующего пакета в список отправленных
		item.sent.push_back(::move(meta));
		/**
		 * Устанавливаем признак адресации датаграммы предпочтительному адресу: в учёт
		 * анти-амплификации текущего пути она не входит намеренно - уходит она не по
		 * нему. Лимит же самого предпочтительного адреса неприменим: адрес анонсирован
		 * удалённым эндпоинтом в заверенных хендшейком параметрах, подделать его
		 * посторонний не может, а поток проверок ограничен сроком отказа от них
		 * (RFC 9000 §8.1/§8.2.4/§9.6.2)
		 */
		this->_path.alternate = true;
		// Выводим положительный результат - проверка предпочтительного адреса собрана
		return true;
	}
	/**
	 * Если требуется отправка зонда размера пути: зонд собирается отдельной
	 * датаграммой увеличенного размера с фреймом PING и дополнением. Полезных
	 * данных он не несёт намеренно - путь может его не пропустить, а повторная
	 * отправка потерянных данных обошлась бы дороже (RFC 9000 §14.4)
	 */
	if(this->_pmtu.queued && (this->_pmtu.probe > this->_pmtu.size) && (this->_crypto.handshake.encryption(level_t::APPLICATION) != nullptr)){
		// Сбрасываем флаг необходимости отправки зонда размера пути
		this->_pmtu.queued = false;
		// Получаем состояние пространства номеров пакетов приложения
		auto & item = this->_spaces[static_cast <size_t> (space_t::APPLICATION)];
		// Получаем переиспользуемый буфер нагрузки уровня приложения
		string & buffer = this->_buffer.payload[static_cast <size_t> (level_t::APPLICATION)];
		// Очищаем буфер от результатов предыдущей сборки
		buffer.clear();
		// Выполняем сборку фрейма PING (RFC 9000 §19.2)
		frame::serialize::ping(buffer);
		/**
		 * Вычисляем накладные расходы пакета: заголовок с номером пакета
		 * максимальной длины и тег AEAD
		 */
		const size_t reserve = (this->headerSize(level_t::APPLICATION, this->_pmtu.probe, proto::MAX_PKT_NUM_SIZE, this->_cids.identity.destination) + crypto::AEAD_TAG_SIZE);
		// Если зонд вместе с накладными расходами в датаграмму не помещается
		if((reserve + buffer.size()) < this->_pmtu.probe)
			// Дополняем нагрузку фреймами PADDING до размера зонда
			frame::serialize::padding(buffer, this->_pmtu.probe - reserve - buffer.size());
		// Учётная запись зонда размера пути
		sent_t meta;
		// Устанавливаем номер отправляемого зонда
		meta.pn = item.txPn;
		// Устанавливаем флаг отправки пакета зондом размера пути
		meta.pmtu = true;
		// Устанавливаем флаг отправки зонда с маркировкой поддержки ECN
		meta.ecn = (this->marking() != event::ecn_t::NOT_ECT);
		// Очищаем буфер исходящей датаграммы
		output.clear();
		// Выполняем сборку и защиту пакета зонда
		if(!this->seal(output, level_t::APPLICATION, buffer, this->_cids.identity.destination)){
			// Очищаем буфер исходящей датаграммы
			output.clear();
			// Выводим отрицательный результат - собрать зонд не удалось
			return false;
		}
		// Устанавливаем время отправки зонда
		meta.time = now;
		// Устанавливаем размер зонда для congestion control
		meta.size = output.size();
		// Учитываем отправленный зонд в октетах в полёте (RFC 9002 §B.2)
		this->_congestion.inflight += meta.size;
		// Устанавливаем время отправки последнего ack-eliciting пакета
		item.lastElicited = now;
		// Устанавливаем флаг наличия отправленных ack-eliciting пакетов
		item.hasElicited = true;
		// Если исходящие датаграммы помечаются поддержкой ECN
		if(meta.ecn)
			// Считаем отправленный с маркировкой зонд пространства
			item.ecnSent++;
		// Добавляем учётную запись зонда в список отправленных пакетов
		item.sent.push_back(::move(meta));
		// Учитываем отправленные октеты в контроле анти-амплификации (RFC 9000 §8.1)
		this->_amplify.sent += output.size();
		// Если зонд отправлен впервые после последнего принятого пакета
		if(!(this->_core.flags & flags::IDLE_ELICITED)){
			// Перезапускаем отсчёт таймаута простоя от момента отправки (RFC 9000 §10.1)
			this->_times.idle = now;
			// Устанавливаем флаг выполненного перезапуска отсчёта отправкой
			this->_core.flags |= flags::IDLE_ELICITED;
		}
		// Выводим положительный результат - зонд размера пути собран
		return true;
	}
	// Список уровней шифрования в порядке возрастания (RFC 9000 §12.2)
	static const level_t levels[] = {level_t::INITIAL, level_t::EARLY_DATA, level_t::HANDSHAKE, level_t::APPLICATION};
	/**
	 * @brief Структура собранного пакета датаграммы
	 *
	 */
	typedef struct Spec {
		// Уровень шифрования пакета
		level_t level;
		// Учётная запись пакета для восстановления потерь
		sent_t meta;
		// Флаг наличия ack-eliciting фреймов в нагрузке
		bool elicit;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Spec() noexcept : level(level_t::INITIAL), elicit(false) {}
	} spec_t;
	// Список собранных нагрузок пакетов датаграммы
	vector <spec_t> specs;
	// Оценка занятого размера датаграммы
	size_t used = 0;
	// Флаг исчерпанного окна перегрузки (RFC 9002 §7)
	const bool limited = ((this->_congestion.inflight >= this->_congestion.window) && (this->_congestion.probes == 0));
	/**
	 * Перебираем список уровней шифрования
	 */
	for(auto & level : levels){
		// Если ключи защиты исходящих пакетов уровня не выведены
		if(this->_crypto.handshake.encryption(level) == nullptr)
			// Переходим к следующему уровню
			continue;
		/**
		 * Вычисляем предельные накладные расходы пакета уровня: заголовок с номером
		 * пакета максимальной длины и тег AEAD. Для пакетов Initial заголовок включает
		 * токен проверки адреса, который бывает длиннее сотни октетов, поэтому
		 * фиксированный запас здесь неприменим
		 */
		const size_t reserve = (this->headerSize(level, capacity, proto::MAX_PKT_NUM_SIZE, this->_cids.identity.destination) + crypto::AEAD_TAG_SIZE);
		// Если в датаграмме не осталось места на пакет уровня
		if((used + reserve) >= capacity)
			// Прекращаем сборку датаграммы
			break;
		// Собираемый пакет уровня
		spec_t spec;
		// Устанавливаем уровень шифрования пакета
		spec.level = level;
		// Устанавливаем флаг отправки пакета на уровне ранних данных (RFC 9001 §4.6.2)
		spec.meta.early = (level == level_t::EARLY_DATA);
		// Получаем переиспользуемый буфер нагрузки уровня
		string & buffer = this->_buffer.payload[static_cast <size_t> (level)];
		// Очищаем буфер от результатов предыдущей сборки
		buffer.clear();
		// Выполняем сборку нагрузки пакета уровня
		if(!this->payload(level, capacity - used - reserve, buffer, spec.meta, spec.elicit, limited))
			// Переходим к следующему уровню
			continue;
		// Учитываем предельную оценку размера пакета: накладные расходы и нагрузка
		used += (reserve + buffer.size());
		// Добавляем нагрузку пакета в список сборки
		specs.push_back(::move(spec));
	}
	// Если нагрузок для отправки нет
	if(specs.empty())
		// Выводим отрицательный результат
		return false;
	// Флаг наличия пакета Initial в датаграмме
	// Флаг датаграммы, подлежащей дополнению до минимального размера
	bool expand = false;
	/**
	 * Перебираем список собранных нагрузок
	 */
	for(auto & spec : specs){
		// Датаграмма с пакетом Initial дополняется до минимального размера (RFC 9000 §14.1)
		expand = (expand || (spec.level == level_t::INITIAL));
		/**
		 * Перебираем управляющие фреймы нагрузки пакета
		 */
		for(auto & control : spec.meta.control)
			/**
			 * Определяем наличие фреймов проверки достижимости пути: датаграммы с ними
			 * дополняются до того же минимального размера, что и датаграммы с пакетом
			 * Initial. Проверка подтверждает не сам факт достижимости, а пригодность
			 * пути к переносу датаграмм минимального размера, поэтому короткая
			 * датаграмма проверила бы не то (RFC 9000 §8.2.1/§8.2.2)
			 */
			expand = (expand || (control.first == frame_t::PATH_CHALLENGE) || (control.first == frame_t::PATH_RESPONSE));
	}
	/**
	 * Если датаграмма подлежит дополнению и доступного объёма отправки хватает
	 * на минимальный размер: под лимитом анти-амплификации дополнение не выполняется,
	 * лимит имеет приоритет над требованием минимального размера (RFC 9000 §14.1)
	 */
	if(expand && (capacity >= proto::MIN_INITIAL_SIZE)){
		// Количество добавленных октетов PADDING
		size_t added = 0;
		/**
		 * Выполняем итеративное дополнение датаграммы до минимального размера
		 */
		for(uint8_t i = 0; i < 4; i++){
			// Точный суммарный размер датаграммы
			size_t total = 0;
			/**
			 * Перебираем список собранных нагрузок
			 */
			for(auto & spec : specs){
				// Получаем состояние пространства номеров пакетов
				const auto & item = this->_spaces[static_cast <size_t> (this->space(spec.level))];
				// Получаем размер собранной нагрузки уровня
				const size_t length = this->_buffer.payload[static_cast <size_t> (spec.level)].size();
				// Вычисляем размер кодирования номера пакета
				const size_t pnSize = packet::packetNumberSize(item.txPn, (item.hasAcked ? item.largestAcked : item.txPn));
				// Суммируем точный размер пакета: заголовок + номер + нагрузка + тег AEAD
				total += (this->headerSize(spec.level, pnSize + length + crypto::AEAD_TAG_SIZE, pnSize, this->_cids.identity.destination) + length + crypto::AEAD_TAG_SIZE);
			}
			// Если датаграмма меньше минимального размера
			if(total < proto::MIN_INITIAL_SIZE){
				// Учитываем количество добавленных октетов PADDING
				added += (proto::MIN_INITIAL_SIZE - total);
				// Дополняем нагрузку последнего пакета фреймами PADDING
				frame::serialize::padding(this->_buffer.payload[static_cast <size_t> (specs.back().level)], proto::MIN_INITIAL_SIZE - total);
			// Если датаграмма превысила минимум из-за роста поля Length (varint)
			} else if((total > proto::MIN_INITIAL_SIZE) && (added >= (total - proto::MIN_INITIAL_SIZE))){
				// Учитываем количество удаляемых октетов PADDING
				added -= (total - proto::MIN_INITIAL_SIZE);
				// Удаляем излишек фреймов PADDING из конца нагрузки
				this->_buffer.payload[static_cast <size_t> (specs.back().level)].erase(this->_buffer.payload[static_cast <size_t> (specs.back().level)].size() - (total - proto::MIN_INITIAL_SIZE));
			// Если датаграмма достигла минимального размера
			} else
				// Прекращаем дополнение
				break;
		}
	}
	// Количество успешно собранных и защищённых пакетов датаграммы
	size_t assembled = 0;
	/**
	 * Первая фаза: собираем и защищаем пакеты, не трогая состояние восстановления
	 * потерь. Учётные записи фиксируются только после успешной сборки всей датаграммы
	 */
	for(; assembled < specs.size(); assembled++){
		// Получаем собираемую нагрузку
		auto & spec = specs[assembled];
		// Получаем состояние пространства номеров пакетов
		const auto & item = this->_spaces[static_cast <size_t> (this->space(spec.level))];
		// Запоминаем номер отправляемого пакета
		spec.meta.pn = item.txPn;
		// Смещение начала пакета в буфере датаграммы
		const size_t start = output.size();
		// Выполняем сборку и защиту пакета уровня
		if(!this->seal(output, spec.level, this->_buffer.payload[static_cast <size_t> (spec.level)], this->_cids.identity.destination))
			// Прекращаем сборку - содержимое оставшихся пакетов будет возвращено в очереди
			break;
		// Устанавливаем время отправки пакета
		spec.meta.time = now;
		// Устанавливаем размер пакета для congestion control
		spec.meta.size = (output.size() - start);
	}
	/**
	 * Если датаграмма не уместилась в доступный объём отправки - отменяем её целиком:
	 * отправка датаграммы сверх лимита анти-амплификации либо сверх размера пути
	 * недопустима, а частичная отправка нарушила бы границы пакетов
	 */
	if(output.size() > capacity){
		// Очищаем буфер датаграммы
		output.clear();
		// Отменяем все собранные пакеты
		assembled = 0;
	}
	/**
	 * Возвращаем содержимое несобранных пакетов в очереди отправки: без этого
	 * снятые с очередей CRYPTO-данные, блоки потоков и управляющие фреймы
	 * потерялись бы безвозвратно - в список отправленных они не попадают
	 */
	for(size_t i = assembled; i < specs.size(); i++){
		// Определяем пространство номеров пакетов несобранного пакета
		const space_t space = this->space(specs[i].level);
		// Возвращаем содержимое пакета в очереди отправки
		this->requeue(space, specs[i].meta);
		/**
		 * Восстанавливаем необходимость отправки подтверждения: сборка нагрузки
		 * сбрасывает флаг, а лишнее подтверждение безвредно в отличие от потерянного
		 */
		this->_spaces[static_cast <size_t> (space)].ackElicited = true;
	}
	// Если ни одного пакета собрать не удалось
	if(assembled == 0)
		// Выводим отрицательный результат
		return false;
	// Флаг наличия пакета Handshake в датаграмме
	bool handshake = false;
	// Флаг наличия ack-eliciting пакетов в датаграмме
	bool elicited = false;
	// Флаг маркировки исходящей датаграммы поддержкой ECN (RFC 9000 §13.4.1)
	const bool marked = (this->marking() != event::ecn_t::NOT_ECT);
	/**
	 * Вторая фаза: фиксируем учётные записи отправленных пакетов
	 */
	for(size_t i = 0; i < assembled; i++){
		// Получаем собранную нагрузку
		auto & spec = specs[i];
		// Определяем наличие пакета Handshake в датаграмме
		handshake = (handshake || (spec.level == level_t::HANDSHAKE));
		// Получаем состояние пространства номеров пакетов
		auto & item = this->_spaces[static_cast <size_t> (this->space(spec.level))];
		// Если исходящая датаграмма помечается поддержкой ECN
		if(marked){
			// Устанавливаем флаг отправки пакета с маркировкой поддержки ECN
			spec.meta.ecn = true;
			/**
			 * Считаем отправленный с маркировкой пакет пространства: счёт ведётся
			 * по всем пакетам, а не только по учитываемым в восстановлении потерь -
			 * удалённый узел считает маркировки всех принятых (RFC 9000 §13.4.2.1)
			 */
			item.ecnSent++;
		}
		// Если пакет не содержит ack-eliciting фреймов (RFC 9002 §A.1)
		if(!spec.elicit)
			// Переходим к следующему пакету - учёт потерь для него не ведётся
			continue;
		// Учитываем отправленный пакет в октетах в полёте (RFC 9002 §B.2)
		this->_congestion.inflight += spec.meta.size;
		// Добавляем учётную запись в список отправленных пакетов
		item.sent.push_back(::move(spec.meta));
		// Устанавливаем время отправки последнего ack-eliciting пакета
		item.lastElicited = now;
		// Устанавливаем флаг наличия отправленных ack-eliciting пакетов
		item.hasElicited = true;
		// Устанавливаем флаг наличия ack-eliciting пакетов в датаграмме
		elicited = true;
	}
	// Учитываем отправленные октеты в контроле анти-амплификации (RFC 9000 §8.1)
	this->_amplify.sent += output.size();
	// Если отправлен зондирующий пакет сверх окна перегрузки (RFC 9002 §7.5)
	/**
	 * Если ack-eliciting пакет отправлен впервые после последнего принятого:
	 * перезапускаем отсчёт таймаута простоя, иначе начатая после долгой паузы
	 * активность оборвалась бы, не дождавшись ответа удалённого эндпоинта.
	 * Повторные отправки отсчёт уже не сдвигают (RFC 9000 §10.1)
	 */
	if(elicited && !(this->_core.flags & flags::IDLE_ELICITED)){
		// Перезапускаем отсчёт таймаута простоя от момента отправки
		this->_times.idle = now;
		// Устанавливаем флаг выполненного перезапуска отсчёта отправкой
		this->_core.flags |= flags::IDLE_ELICITED;
	}
	if(elicited && (this->_congestion.probes > 0))
		// Списываем разрешение на зондирующий пакет
		this->_congestion.probes--;
	// Если клиент отправил первый пакет Handshake
	if((this->_core.endpoint == endpoint_t::CLIENT) && handshake && (this->_crypto.handshake.encryption(level_t::INITIAL) != nullptr))
		// Сбрасываем ключи уровня Initial (RFC 9001 §4.9.1)
		this->discard(level_t::INITIAL);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод получения дедлайна ближайшего события таймера (RFC 9002 §6)
 *
 * @return дедлайн ближайшего события в миллисекундах (0 - таймер не требуется)
 */
uint64_t awh::quic::Connection::timeout() const noexcept {
	/**
	 * Если локальный эндпоинт выдерживает период завершения соединения: до его
	 * истечения соединение живо ради повтора фрейма CONNECTION_CLOSE, поэтому
	 * дедлайном таймера является дедлайн периода (RFC 9000 §10.2)
	 */
	if(this->_core.state == state_t::CLOSING)
		// Выводим дедлайн периода завершения, а до его взведения - немедленный дедлайн
		return (this->_close.deadline > 0 ? this->_close.deadline : (this->_times.now + 1));
	// Если соединение не активно
	if((this->_core.state != state_t::HANDSHAKING) && (this->_core.state != state_t::CONNECTED))
		// Выводим нулевой дедлайн - таймер не требуется
		return 0;
	// Дедлайн ближайшего события таймера
	uint64_t result = 0;
	/**
	 * Флаг запрета отправки лимитом анти-амплификации: зондировать нечем, поэтому
	 * таймер PTO не взводится - его срабатывания лишь наращивали бы экспоненциальную
	 * выдержку вхолостую, а разблокирует отправку приём датаграммы, а не таймер
	 * (RFC 9002 §6.2.2.1)
	 */
	const bool blocked = this->stalled();
	/**
	 * Перебираем пространства номеров пакетов
	 */
	for(size_t i = 0; i < SPACES; i++){
		// Получаем состояние пространства номеров пакетов
		const auto & item = this->_spaces[i];
		// Если таймер детекта потерь пространства взведён
		if(item.hasLossTime && ((result == 0) || (item.lossTime < result)))
			// Устанавливаем дедлайн детекта потерь
			result = item.lossTime;
		// Получаем дедлайн таймера PTO пространства
		const uint64_t pto = (blocked ? 0 : this->deadline(static_cast <space_t> (i)));
		// Если таймер PTO пространства взведён
		if((pto > 0) && ((result == 0) || (pto < result)))
			// Устанавливаем дедлайн таймера PTO
			result = pto;
		// Если подтверждение приёма поставлено в очередь
		if(item.ackElicited){
			/**
			 * Вычисляем дедлайн отправки подтверждения: откладывать его дозволено не
			 * долее анонсированной локальным эндпоинтом задержки, а подтверждения
			 * пакетов хендшейка откладывать нельзя вовсе (RFC 9000 §13.2.1)
			 */
			const uint64_t ack = (item.ackTime + ((static_cast <space_t> (i) == space_t::APPLICATION) ? this->_transport.local.maxAckDelay : 0));
			// Если дедлайн подтверждения является ближайшим событием
			if((result == 0) || (ack < result))
				// Устанавливаем дедлайн отправки подтверждения
				result = ack;
		}
	}
	/**
	 * Если выполняется проверка достижимости пути: по истечении её дедлайна путь
	 * признаётся непригодным, поэтому дедлайн является событием таймера
	 * (RFC 9000 §8.2.4)
	 */
	if(this->_path.pending && (this->_path.deadline > 0) && ((result == 0) || (this->_path.deadline < result)))
		// Устанавливаем дедлайн отказа от проверки достижимости пути
		result = this->_path.deadline;
	/**
	 * Если запланирован сброс ключей предыдущей фазы: по истечении дедлайна ключи
	 * подлежат удалению, поэтому дедлайн является событием таймера (RFC 9001 §6.3)
	 */
	if(this->_crypto.phase.hasPrevious && (this->_crypto.phase.discard > 0) && ((result == 0) || (this->_crypto.phase.discard < result)))
		// Устанавливаем дедлайн сброса ключей предыдущей фазы
		result = this->_crypto.phase.discard;
	// Получаем дедлайн таймаута простоя соединения (RFC 9000 §10.1)
	const uint64_t idle = this->idle();
	// Если таймаут простоя согласован и является ближайшим событием
	if((idle > 0) && ((result == 0) || (idle < result)))
		// Устанавливаем дедлайн таймаута простоя
		result = idle;
	// Выводим дедлайн ближайшего события таймера
	return result;
}
/**
 * @brief Метод обработки просроченных таймеров (RFC 9002 §6.2)
 *
 * @param now текущее время в миллисекундах
 */
void awh::quic::Connection::tick(const uint64_t now) noexcept {
	/**
	 * Если локальный эндпоинт выдерживает период завершения соединения: по его
	 * истечении состояние соединения подлежит освобождению, о чём вызывающий код
	 * узнаёт по переходу в завершённое состояние (RFC 9000 §10.2)
	 */
	if(this->_core.state == state_t::CLOSING){
		// Обновляем текущее время последнего вызова
		this->_times.now = now;
		// Если дедлайн периода завершения ещё не взведён
		if(this->_close.deadline == 0)
			// Взводим дедлайн периода завершения соединения
			this->_close.deadline = (now + (CLOSING_PERIOD * this->interval(space_t::APPLICATION)));
		// Если период завершения соединения истёк
		else if(now >= this->_close.deadline)
			// Переводим соединение в завершённое состояние
			this->_core.state = state_t::DRAINING;
		// Выходим из метода
		return;
	}
	// Если соединение не активно
	if((this->_core.state != state_t::HANDSHAKING) && (this->_core.state != state_t::CONNECTED))
		// Выходим из метода
		return;
	// Обновляем текущее время последнего вызова
	this->_times.now = now;
	// Получаем дедлайн таймаута простоя соединения (RFC 9000 §10.1)
	const uint64_t idle = this->idle();
	// Если таймаут простоя соединения истёк
	if((idle > 0) && (idle <= now)){
		// Завершаем соединение молча без отправки фреймов (RFC 9000 §10.1)
		this->_core.state = state_t::DRAINING;
		// Выходим из метода
		return;
	}
	/**
	 * Если истёк дедлайн удержания ключей предыдущей фазы: отставшие пакеты прежней
	 * фазы уже пришли либо признаны потерянными, поэтому ключи сбрасываются, а их
	 * материал затирается деструктором временного при обмене (RFC 9001 §6.3)
	 */
	if(this->_crypto.phase.hasPrevious && (this->_crypto.phase.discard > 0) && (now >= this->_crypto.phase.discard)){
		// Временный набор ключей для затирания сбрасываемых через деструктор
		crypto::keys_t discarded;
		// Перемещаем ключи предыдущей фазы во временный набор для затирания
		std::swap(this->_crypto.phase.prevRead, discarded);
		// Сбрасываем флаг наличия ключей предыдущей фазы
		this->_crypto.phase.hasPrevious = false;
		// Снимаем дедлайн сброса ключей предыдущей фазы
		this->_crypto.phase.discard = 0;
	}
	/**
	 * Если дедлайн проверки достижимости пути истёк: ответа не дождались, и
	 * переотправлять проверку далее незачем - путь признаётся непригодным
	 * (RFC 9000 §8.2.4)
	 */
	if(this->_path.pending && (this->_path.deadline > 0) && (now >= this->_path.deadline))
		// Выполняем отказ от проверки достижимости пути с возвратом на проверенный адрес
		this->abandon(true);
	// Флаг выполненного детекта потерь
	bool detected = false;
	/**
	 * Перебираем пространства номеров пакетов
	 */
	for(size_t i = 0; i < SPACES; i++){
		// Если таймер детекта потерь пространства просрочен
		if(this->_spaces[i].hasLossTime && (this->_spaces[i].lossTime <= now)){
			// Выполняем детект потерянных пакетов пространства
			this->detect(static_cast <space_t> (i));
			// Устанавливаем флаг выполненного детекта потерь
			detected = true;
		}
	}
	// Если детект потерь выполнен - таймер PTO не обрабатывается (RFC 9002 §6.2)
	if(detected)
		// Выходим из метода
		return;
	/**
	 * Если отправка запрещена лимитом анти-амплификации: зондирование невозможно,
	 * а счётчик срабатываний таймера PTO наращивал бы выдержку вхолостую
	 * (RFC 9002 §6.2.2.1)
	 */
	if(this->stalled())
		// Выходим из метода
		return;
	// Пространство с просроченным таймером PTO
	space_t expired = space_t::INITIAL;
	// Дедлайн просроченного таймера PTO
	uint64_t earliest = 0;
	/**
	 * Перебираем пространства номеров пакетов
	 */
	for(size_t i = 0; i < SPACES; i++){
		// Получаем дедлайн таймера PTO пространства
		const uint64_t pto = this->deadline(static_cast <space_t> (i));
		// Если таймер PTO пространства просрочен и является самым ранним
		if((pto > 0) && (pto <= now) && ((earliest == 0) || (pto < earliest))){
			// Запоминаем пространство с просроченным таймером PTO
			expired = static_cast <space_t> (i);
			// Запоминаем дедлайн просроченного таймера PTO
			earliest = pto;
		}
	}
	// Если просроченный таймер PTO найден
	if(earliest > 0){
		// Если предел экспоненциальной выдержки не достигнут
		if(this->_rtt.ptoCount < MAX_PTO_COUNT)
			// Увеличиваем счётчик срабатываний таймера PTO (RFC 9002 §6.2.1)
			this->_rtt.ptoCount++;
		// Разрешаем отправку зондирующих пакетов сверх окна перегрузки (RFC 9002 §7.5)
		this->_congestion.probes = PTO_PROBES;
		// Ставим зондирующие данные пространства в очередь отправки
		this->probe(expired);
		/**
		 * Если выполняется проверка достижимости пути: повторяем её отправку. Ответа
		 * на неё нет, а ретрансмиссии фрейм проверки не подлежит - вместо потерянного
		 * отправляется новый, и так до истечения срока отказа (RFC 9000 §8.2/§13.3)
		 */
		if(this->_path.pending)
			// Ставим отправку фрейма PATH_CHALLENGE в очередь
			this->_path.queued = true;
	}
}
/**
 * @brief Метод инициирования обновления ключей уровня приложения (RFC 9001 §6)
 *
 * @return результат инициирования (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::rekey(const uint64_t now) noexcept {
	// Обновляем текущее время последнего вызова
	this->_times.now = now;
	// Если соединение не установлено, хендшейк не подтверждён либо ключи не выведены
	if((this->_core.state != state_t::CONNECTED) || !(this->_core.flags & flags::CONFIRMED) || !this->_crypto.phase.ready)
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Получаем состояние пространства пакетов приложения
	const auto & item = this->_spaces[static_cast <size_t> (space_t::APPLICATION)];
	// Если пакет текущей фазы ещё не подтверждён (RFC 9001 §6.1)
	if(!item.hasAcked || (item.largestAcked < this->_crypto.phase.sent))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Выполняем переключение на следующую фазу ключей
	this->promote();
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод получения бита фазы ключей уровня приложения (RFC 9001 §6)
 *
 * @return бит фазы ключей
 */
bool awh::quic::Connection::phase() const noexcept {
	// Выводим бит фазы ключей уровня приложения
	return this->_crypto.phase.current;
}
/**
 * @brief Метод установки лимитов AEAD текущего соединения (RFC 9001 §6.6)
 *
 * @param confidentiality предельное число пакетов на одном ключе
 * @param integrity       предельное число неудачных снятий защиты
 */
void awh::quic::Connection::aeadLimits(const uint64_t confidentiality, const uint64_t integrity) noexcept {
	/**
	 * Лимиты ограничиваются сверху пределами спецификации: метод способен только
	 * ужесточить проверку, но не ослабить её ниже гарантий набора (RFC 9001 §6.6)
	 */
	this->_crypto.aead.confidentiality = ((confidentiality < AEAD_CONFIDENTIALITY_LIMIT) ? confidentiality : AEAD_CONFIDENTIALITY_LIMIT);
	// Ограничиваем лимит целостности пределом спецификации
	this->_crypto.aead.integrity = ((integrity < AEAD_INTEGRITY_LIMIT) ? integrity : AEAD_INTEGRITY_LIMIT);
}
/**
 * @brief Метод сброса состояния пути соединения (RFC 9000 §9.4)
 *
 */
void awh::quic::Connection::repath(const bool remote) noexcept {
	/**
	 * Окно перегрузки и оценка задержки характеризуют конкретный сетевой путь:
	 * на новом пути прежние значения неприменимы и приводят к отправке заведомо
	 * избыточного объёма данных в неизвестный по ёмкости канал
	 */
	this->_congestion.window = INITIAL_WINDOW;
	/**
	 * Списываем октеты в полёте: отправленное прежним путём ёмкость нового не
	 * занимает, а оставленный учёт запер бы отправку по новому пути окном
	 * перегрузки - в том числе проверку его достижимости (RFC 9000 §9.4)
	 */
	this->_congestion.inflight = 0;
	/**
	 * Перебираем пространства номеров пакетов
	 */
	for(size_t i = 0; i < SPACES; i++){
		/**
		 * Помечаем отправленное прежним путём: содержимое таких пакетов при потере
		 * переотправляется обычным порядком, но контроль перегрузки и оценку задержки
		 * нового пути их подтверждения и потери не затрагивают
		 */
		for(auto & packet : this->_spaces[i].sent)
			// Устанавливаем флаг отправки пакета по прежнему пути соединения
			packet.stale = true;
	}
	// Восстанавливаем начальный порог замедленного старта
	this->_congestion.threshold = proto::VARINT_MAX;
	// Завершаем период восстановления
	this->_congestion.inRecovery = false;
	// Сбрасываем время начала периода восстановления
	this->_congestion.recovery = 0;
	// Сбрасываем флаг наличия измерения задержки приёма-передачи
	this->_rtt.sampled = false;
	// Сбрасываем последнюю измеренную задержку приёма-передачи
	this->_rtt.latest = 0;
	// Сбрасываем минимальную задержку приёма-передачи
	this->_rtt.minimum = 0;
	// Сбрасываем сглаженную задержку приёма-передачи
	this->_rtt.smoothed = 0;
	// Сбрасываем вариативность задержки приёма-передачи
	this->_rtt.variation = 0;
	// Сбрасываем счётчик срабатываний таймера PTO
	this->_rtt.ptoCount = 0;
	/**
	 * Если сменился адрес удалённого эндпоинта: достижимости нового адреса ничто
	 * не подтверждало, а сменить его мог и посторонний, подделавший адрес
	 * отправителя - до проверки пути объём отправки на него ограничен трёхкратным
	 * объёмом принятого, иначе соединение стало бы усилителем трафика на чужой
	 * адрес. Смена лишь собственного адреса лимита не возвращает: адрес удалённого
	 * эндпоинта остаётся прежним и проверенным (RFC 9000 §8.1/§9.3.1)
	 */
	if(remote){
		// Сбрасываем флаг подтверждённого адреса удалённого эндпоинта
		this->_amplify.validated = false;
		// Обнуляем счётчик принятых от удалённого эндпоинта октетов
		this->_amplify.received = 0;
		// Обнуляем счётчик отправленных удалённому эндпоинту октетов
		this->_amplify.sent = 0;
	}
	// Сбрасываем флаг подтверждённой достижимости пути
	this->_path.validated = false;
	/**
	 * Проверка поддержки ECN относится к конкретному пути: новый путь маркировку
	 * стирать не обязан, поэтому непройденная на прежнем пути проверка выполняется
	 * заново. Накопленные счётчики маркировок при этом сохраняются: удалённый
	 * эндпоинт ведёт их нарастающим итогом по пространству номеров за всё
	 * соединение, а не по пути, и обнуление локального учёта сделало бы первый же
	 * присланный им счётчик недостоверно большим - проверка провалилась бы сразу
	 * и навсегда отключила маркировку (RFC 9000 §13.4.2)
	 */
	this->_marking.failed = false;
	/**
	 * Размер пути характеризует конкретный путь: на новом пути прежний
	 * подтверждённый размер неприменим, и поиск начинается заново от размера,
	 * который обязан пропускать любой путь (RFC 8899 §5.4)
	 */
	this->_pmtu.size = MAX_DATAGRAM_SIZE;
	// Восстанавливаем верхнюю границу поиска до заданной вызывающим кодом
	this->_pmtu.high = this->_pmtu.limit;
	// Сбрасываем размер собираемого зонда
	this->_pmtu.probe = 0;
	// Обнуляем количество отправленных попыток зонда
	this->_pmtu.count = 0;
	// Сбрасываем флаг необходимости отправки зонда
	this->_pmtu.queued = false;
	// Учитываем выполненную смену пути соединения
	this->_path.migrations++;
	// Начинаем поиск размера нового пути зондированием (RFC 8899)
	this->discover();
}
/**
 * @brief Метод инициирования проверки достижимости пути (RFC 9000 §8.2)
 *
 * @return результат инициирования (false - проверка уже выполняется либо соединение не установлено)
 */
bool awh::quic::Connection::probe() noexcept {
	// Если соединение не установлено либо проверка пути уже выполняется
	if((this->_core.state != state_t::CONNECTED) || this->_path.pending)
		// Выводим отрицательный результат
		return false;
	// Выполняем генерацию случайных данных проверки достижимости пути
	if(::RAND_bytes(this->_path.probe, proto::PATH_DATA_SIZE) != 1)
		// Выводим отрицательный результат - ошибка генератора случайных чисел
		return false;
	// Устанавливаем флаг ожидания ответа на проверку пути
	this->_path.pending = true;
	// Сбрасываем флаг подтверждённой достижимости пути
	this->_path.validated = false;
	// Ставим отправку фрейма PATH_CHALLENGE в очередь
	this->_path.queued = true;
	/**
	 * Взводим дедлайн отказа от проверки: новый путь бывает длиннее прежнего,
	 * поэтому отводится троекратный интервал PTO, но не меньше троекратного
	 * интервала по начальной оценке задержки - иначе быстрый прежний путь
	 * задал бы заведомо недостижимый срок (RFC 9000 §8.2.4)
	 */
	this->_path.deadline = (this->_times.now + (3 * ::max(this->interval(space_t::APPLICATION), (3 * INITIAL_RTT))));
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод отказа от проверки достижимости пути (RFC 9000 §8.2.4)
 *
 * @param revert признак возврата на последний проверенный адрес
 */
void awh::quic::Connection::abandon(const bool revert) noexcept {
	// Сбрасываем флаг ожидания ответа на проверку пути
	this->_path.pending = false;
	// Сбрасываем флаг необходимости отправки фрейма PATH_CHALLENGE
	this->_path.queued = false;
	// Сбрасываем дедлайн отказа от проверки достижимости пути
	this->_path.deadline = 0;
	/**
	 * Если проверялась достижимость предпочтительного адреса: переезд на него
	 * отменяется, а соединение остаётся на текущем пути - переезжать дозволено
	 * лишь на проверенный адрес (RFC 9000 §9.6.2)
	 */
	if(this->_path.relocating){
		// Сбрасываем флаг выполняемой проверки предпочтительного адреса
		this->_path.relocating = false;
		// Записываем в лог сообщение об отмене переезда на предпочтительный адрес
		this->_log->print("QUIC preferred address is unreachable, connection stays on the current path", log_t::flag_t::WARNING);
	/**
	 * Если проверялась достижимость текущего пути: путь признаётся непригодным,
	 * но соединение не завершается - непригодность пути его разрывом не является.
	 * Адрес удалённого эндпоинта остаётся неподтверждённым, поэтому объём отправки
	 * на него ограничен лимитом анти-амплификации (RFC 9000 §8.2.4/§9.3.1)
	 */
	/**
	 * Если проверялась достижимость нового адреса удалённого эндпоинта, а прежний
	 * проверенный адрес известен: возвращаем соединение на него. Смену адреса
	 * способен подделать находящийся на пути посторонний, и без возврата одна
	 * поддельная датаграмма уводила бы соединение на недостижимый адрес насовсем
	 * (RFC 9000 §9.3.2)
	 */
	} else if(revert && !this->_path.previous.empty() && (this->_path.previous != this->_path.address)) {
		// Возвращаем соединение на последний проверенный адрес
		this->_path.address = this->_path.previous;
		// Восстанавливаем флаг подтверждённой достижимости пути
		this->_path.validated = true;
		/**
		 * Восстанавливаем флаг подтверждённого адреса удалённого эндпоинта: прежний
		 * адрес проверку уже проходил, повторять её незачем (RFC 9000 §9.3)
		 */
		this->_amplify.validated = true;
		// Записываем в лог сообщение о возврате на последний проверенный адрес
		this->_log->print(
			"QUIC path validation failed, connection reverted to the last validated address: %s",
			log_t::flag_t::WARNING, this->_path.address.c_str()
		);
	/**
	 * Если возвращаться некуда: путь признаётся непригодным, но соединение не
	 * завершается - непригодность пути его разрывом не является. Адрес удалённого
	 * эндпоинта остаётся неподтверждённым, поэтому объём отправки на него ограничен
	 * лимитом анти-амплификации (RFC 9000 §8.2.4/§9.3.1)
	 */
	} else this->_log->print("QUIC path validation failed, path is considered unusable", log_t::flag_t::WARNING);
}
/**
 * @brief Метод получения состояния проверки достижимости пути (RFC 9000 §8.2)
 *
 * @return состояние проверки (true - путь подтверждён ответом удалённого эндпоинта)
 */
bool awh::quic::Connection::validated() const noexcept {
	// Выводим состояние проверки достижимости пути
	return this->_path.validated;
}
/**
 * @brief Метод инициирования миграции соединения на новый путь (RFC 9000 §9)
 *
 * @return результат инициирования (false - соединение не установлено либо нет неиспользованных идентификаторов)
 */
bool awh::quic::Connection::migrate() noexcept {
	// Если соединение не установлено
	if(this->_core.state != state_t::CONNECTED)
		// Выводим отрицательный результат
		return false;
	/**
	 * Если проверка достижимости пути уже выполняется: выходим до поворота
	 * идентификатора и сброса состояния пути. Иначе неудача запуска новой проверки
	 * оставила бы соединение с повёрнутым идентификатором на сброшенном пути,
	 * достижимость которого никто не проверяет
	 */
	if(this->_path.pending)
		// Выводим отрицательный результат
		return false;
	/**
	 * Если удалённый узел запретил активную миграцию: отправлять ему с нового
	 * локального адреса запрещено. Запрет на переезд по анонсированному
	 * предпочтительному адресу не распространяется - он активной миграцией
	 * не является (RFC 9000 §9.6.3)
	 */
	if(this->_transport.remote.disableActiveMigration){
		// Записываем в лог сообщение о запрете активной миграции удалённым узлом
		this->_log->print("QUIC active migration is disabled by peer", log_t::flag_t::WARNING);
		// Выводим отрицательный результат
		return false;
	}
	/**
	 * Переключаемся на неиспользованный идентификатор удалённого эндпоинта:
	 * использование прежнего идентификатора на новом пути позволило бы связать
	 * пути между собой наблюдателю (RFC 9000 §9.5)
	 */
	if(!this->rotate())
		// Выводим отрицательный результат - неиспользованных идентификаторов нет
		return false;
	/**
	 * Выполняем сброс состояния пути соединения: сменился лишь собственный адрес,
	 * поэтому проверенный адрес удалённого эндпоинта лимита анти-амплификации
	 * не возвращает (RFC 9000 §9.2)
	 */
	this->repath(false);
	// Начинаем проверку достижимости нового пути
	return this->probe();
}
/**
 * @brief Метод проверки наличия анонсированного предпочтительного адреса сервера (RFC 9000 §9.6)
 *
 * @return результат проверки
 */
bool awh::quic::Connection::relocatable() const noexcept {
	/**
	 * Предпочтительный адрес анонсирует только сервер и применяет только клиент:
	 * переезд возможен единожды на установленном соединении, повторный переезд
	 * анонсированного адреса под собой не имеет (RFC 9000 §9.6)
	 */
	return ((this->_core.endpoint == endpoint_t::CLIENT) && (this->_core.state == state_t::CONNECTED) &&
	        this->_transport.remote.hasPreferredAddress && !this->_cids.identity.relocated);
}
/**
 * @brief Метод извлечения предпочтительного адреса сервера (RFC 9000 §9.6)
 *
 * @param ipv6 флаг извлечения адреса семейства IPv6
 * @param ip   адрес сервера в сетевом порядке октетов (4 октета IPv4 либо 16 октетов IPv6)
 * @param port порт сервера
 * @return     результат извлечения (false - адрес семейства не анонсирован)
 */
bool awh::quic::Connection::preferred(const bool ipv6, string & ip, uint16_t & port) const noexcept {
	// Если предпочтительный адрес сервера не анонсирован
	if(!this->_transport.remote.hasPreferredAddress)
		// Выводим отрицательный результат
		return false;
	// Получаем анонсированный предпочтительный адрес сервера
	const auto & address = this->_transport.remote.preferredAddress;
	// Если извлекается адрес семейства IPv6
	if(ipv6){
		// Флаг наличия анонсированного адреса семейства IPv6
		bool present = false;
		/**
		 * Определяем наличие адреса: нулевой адрес означает, что для этого семейства
		 * предпочтительный адрес не анонсирован (RFC 9000 §18.2)
		 */
		for(uint8_t i = 0; (i < 16) && !present; i++)
			// Определяем ненулевой октет адреса
			present = (address.ipv6[i] != 0);
		// Если адрес семейства IPv6 не анонсирован
		if(!present)
			// Выводим отрицательный результат
			return false;
		// Устанавливаем адрес сервера в сетевом порядке октетов
		ip.assign(reinterpret_cast <const char *> (address.ipv6), 16);
		// Устанавливаем порт сервера
		port = address.ipv6Port;
	// Если извлекается адрес семейства IPv4
	} else {
		// Определяем наличие адреса семейства IPv4 по ненулевым октетам
		const bool present = ((address.ipv4[0] != 0) || (address.ipv4[1] != 0) || (address.ipv4[2] != 0) || (address.ipv4[3] != 0));
		// Если адрес семейства IPv4 не анонсирован
		if(!present)
			// Выводим отрицательный результат
			return false;
		// Устанавливаем адрес сервера в сетевом порядке октетов
		ip.assign(reinterpret_cast <const char *> (address.ipv4), 4);
		// Устанавливаем порт сервера
		port = address.ipv4Port;
	}
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод переезда соединения на предпочтительный адрес сервера (RFC 9000 §9.6)
 *
 * @return результат инициирования переезда (false - переезд невозможен)
 */
bool awh::quic::Connection::relocate() noexcept {
	// Если переезд на предпочтительный адрес невозможен
	if(!this->relocatable())
		// Выводим отрицательный результат
		return false;
	// Если проверка достижимости пути уже выполняется
	if(this->_path.pending)
		// Выводим отрицательный результат
		return false;
	// Флаг наличия идентификатора предпочтительного адреса
	bool found = false;
	/**
	 * Ищем неиспользованный идентификатор предпочтительного адреса: он введён
	 * в обращение с порядковым номером 1 при применении транспортных параметров
	 * сервера (RFC 9000 §5.1.1)
	 */
	for(auto & item : this->_cids.routing.remote){
		// Если найден неиспользованный идентификатор предпочтительного адреса
		if(!item.used && (item.seq == 1)){
			// Запоминаем идентификатор соединения предпочтительного адреса
			this->_path.relocation = item.cid;
			// Устанавливаем флаг наличия идентификатора предпочтительного адреса
			found = true;
			// Прекращаем перебор
			break;
		}
	}
	// Если идентификатор предпочтительного адреса недоступен
	if(!found)
		// Выводим отрицательный результат
		return false;
	/**
	 * Начинаем проверку достижимости предпочтительного адреса, не переключая на него
	 * соединение: переезжать дозволено лишь на проверенный адрес, поэтому до ответа
	 * удалённого эндпоинта соединение продолжает работать по текущему пути, а на
	 * предпочтительный адрес уходят одни пробирующие датаграммы (RFC 9000 §9.6.2)
	 */
	// Запоминаем состояние подтверждения достижимости текущего пути
	const bool validated = this->_path.validated;
	if(!this->probe())
		// Выводим отрицательный результат
		return false;
	/**
	 * Восстанавливаем подтверждение достижимости текущего пути: проверяется другой
	 * адрес - предпочтительный, - а текущий путь остаётся прежним и проверку уже
	 * проходил. Снятое подтверждение осталось бы снятым и при отказе от переезда,
	 * выставляя исправный путь непроверенным
	 */
	this->_path.validated = validated;
	// Устанавливаем флаг выполняемой проверки предпочтительного адреса
	this->_path.relocating = true;
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод завершения переезда на предпочтительный адрес (RFC 9000 §9.6.2)
 *
 */
void awh::quic::Connection::settle() noexcept {
	// Сбрасываем флаг выполняемой проверки предпочтительного адреса
	this->_path.relocating = false;
	// Ставим прежний идентификатор в очередь вывода из обращения (RFC 9000 §5.1.2)
	this->_cids.routing.retireQueue.push_back(this->_cids.routing.sequence);
	/**
	 * Перебираем список идентификаторов удалённого эндпоинта
	 */
	for(auto i = this->_cids.routing.remote.begin(); i != this->_cids.routing.remote.end(); ++i){
		// Если найден прежний идентификатор
		if(i->seq == this->_cids.routing.sequence){
			// Удаляем прежний идентификатор из списка
			this->_cids.routing.remote.erase(i);
			// Прекращаем перебор
			break;
		}
	}
	/**
	 * Перебираем список идентификаторов после удаления прежнего
	 */
	for(auto & item : this->_cids.routing.remote){
		// Если найден идентификатор предпочтительного адреса
		if(item.seq == 1){
			// Переключаем идентификатор соединения удалённого эндпоинта
			this->_cids.identity.destination = item.cid;
			// Обновляем порядковый номер текущего идентификатора
			this->_cids.routing.sequence = item.seq;
			// Устанавливаем флаг использования идентификатора
			item.used = true;
			// Прекращаем перебор
			break;
		}
	}
	// Устанавливаем флаг выполненного переезда на предпочтительный адрес
	this->_cids.identity.relocated = true;
	/**
	 * Выполняем сброс состояния пути соединения: предпочтительный адрес анонсирован
	 * самим удалённым эндпоинтом в заверенных хендшейком параметрах, подделать его
	 * посторонний не может, поэтому лимит анти-амплификации к нему неприменим
	 */
	this->repath(false);
	/**
	 * Восстанавливаем флаг подтверждённой достижимости пути: сброс состояния снял
	 * его наравне с прочим состоянием прежнего пути, но достижимость нового адреса
	 * только что подтверждена ответом на проверку - повторять её незачем
	 */
	this->_path.validated = true;
	/**
	 * Освобождаем адрес пути: соединение переезжает на предпочтительный адрес, и
	 * первая пришедшая с него датаграмма станет первичным путём, а не поводом
	 * счесть переезд подделанной сменой адреса
	 */
	this->_path.address.clear();
	// Записываем в лог сообщение о переезде на предпочтительный адрес
	this->_log->print("QUIC connection relocated to the server preferred address", log_t::flag_t::INFO);
}
/**
 * @brief Метод получения адреса удалённого эндпоинта текущего пути (RFC 9000 §9.3)
 *
 * @return адрес удалённого эндпоинта в заданном вызывающим кодом представлении
 */
const awh::string & awh::quic::Connection::path() const noexcept {
	// Выводим адрес удалённого эндпоинта текущего пути
	return this->_path.address;
}
/**
 * @brief Метод получения адресата собранной датаграммы (RFC 9000 §9.6.2)
 *
 * @return результат проверки (true - датаграмма адресована предпочтительному адресу)
 */
bool awh::quic::Connection::alternate() const noexcept {
	// Выводим признак адресации собранной датаграммы предпочтительному адресу
	return this->_path.alternate;
}
/**
 * @brief Метод получения подтверждённого размера исходящей датаграммы (RFC 8899)
 *
 * @return подтверждённый размер исходящей датаграммы в октетах
 */
size_t awh::quic::Connection::pmtu() const noexcept {
	// Выводим подтверждённый размер исходящей датаграммы
	return this->_pmtu.size;
}
/**
 * @brief Метод установки верхней границы поиска размера пути (RFC 8899 §5.1)
 *
 * @param limit верхняя граница размера исходящей датаграммы в октетах
 */
void awh::quic::Connection::pmtu(const size_t limit) noexcept {
	/**
	 * Опускаем заданную вызывающим кодом границу поиска: размер, который обязан
	 * пропускать любой путь, границей снизу остаётся в любом случае (RFC 9000 §14.1)
	 */
	this->_pmtu.limit = ::max(::min(this->_pmtu.limit, limit), static_cast <size_t> (MAX_DATAGRAM_SIZE));
	// Опускаем рабочую границу поиска до заданной вызывающим кодом
	this->_pmtu.high = ::min(this->_pmtu.high, this->_pmtu.limit);
	// Продвигаем поиск размера пути с учётом новой границы
	this->discover();
}
/**
 * @brief Метод получения количества обслуживаемых потоков приложения
 *
 * @return количество обслуживаемых потоков приложения
 */
size_t awh::quic::Connection::streams() const noexcept {
	// Выводим количество обслуживаемых потоков приложения
	return this->_stream.list.size();
}
/**
 * @brief Метод получения количества выполненных смен пути соединения
 *
 * @return количество выполненных смен пути
 */
uint64_t awh::quic::Connection::migrations() const noexcept {
	// Выводим количество выполненных смен пути соединения
	return this->_path.migrations;
}
/**
 * @brief Метод ротации идентификатора соединения удалённого эндпоинта (RFC 9000 §5.1.1)
 *
 * @return результат ротации (false - неиспользованных идентификаторов нет)
 */
bool awh::quic::Connection::rotate() noexcept {
	// Если соединение не установлено
	if(this->_core.state != state_t::CONNECTED)
		// Выводим отрицательный результат
		return false;
	// Порядковый номер неиспользованного идентификатора удалённого эндпоинта
	uint64_t next = 0;
	// Флаг наличия неиспользованного идентификатора
	bool found = false;
	/**
	 * Перебираем список идентификаторов удалённого эндпоинта
	 */
	for(auto & item : this->_cids.routing.remote){
		/**
		 * Если найден первый неиспользованный идентификатор: идентификатор с порядковым
		 * номером 1 закреплён за предпочтительным адресом сервера и для обычной смены
		 * пути непригоден - забрав его, соединение сделало бы переезд невозможным
		 * (RFC 9000 §5.1.1/§9.6.1)
		 */
		if(!item.used && !found && !this->reserved(item.seq)){
			// Запоминаем порядковый номер неиспользованного идентификатора
			next = item.seq;
			// Устанавливаем флаг наличия неиспользованного идентификатора
			found = true;
		}
	}
	// Если неиспользованных идентификаторов нет
	if(!found)
		// Выводим отрицательный результат
		return false;
	// Ставим прежний идентификатор в очередь вывода из обращения (RFC 9000 §5.1.2)
	this->_cids.routing.retireQueue.push_back(this->_cids.routing.sequence);
	/**
	 * Перебираем список идентификаторов удалённого эндпоинта
	 */
	for(auto i = this->_cids.routing.remote.begin(); i != this->_cids.routing.remote.end(); ++i){
		// Если найден прежний идентификатор
		if(i->seq == this->_cids.routing.sequence){
			// Удаляем прежний идентификатор из списка
			this->_cids.routing.remote.erase(i);
			// Прекращаем перебор
			break;
		}
	}
	/**
	 * Перебираем список идентификаторов после удаления прежнего
	 */
	for(auto & item : this->_cids.routing.remote){
		// Если найден новый идентификатор
		if(item.seq == next){
			// Переключаем идентификатор соединения удалённого эндпоинта
			this->_cids.identity.destination = item.cid;
			// Обновляем порядковый номер текущего идентификатора
			this->_cids.routing.sequence = item.seq;
			// Устанавливаем флаг использования идентификатора
			item.used = true;
			// Прекращаем перебор
			break;
		}
	}
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод извлечения возобновляемой сессии соединения (RFC 9001 §4.6)
 *
 * @return сериализованная сессия (пусто - сессия недоступна)
 */
string awh::quic::Connection::session() const noexcept {
	// Извлекаем билет возобновления хендшейк-машины
	const string ticket = this->_crypto.handshake.session();
	// Если билет возобновления недоступен
	if(ticket.empty())
		// Выводим пустой результат
		return ticket;
	/**
	 * Прикладываем к билету транспортные параметры удалённого узла: ранние данные
	 * отправляются под лимитами прошлого соединения, поскольку новых до завершения
	 * хендшейка ещё нет, и запомнить их обязан клиент (RFC 9001 §4.6.1)
	 */
	string params = "";
	// Определяем роль удалённого узла (противоположна локальной)
	const endpoint_t sender = ((this->_core.endpoint == endpoint_t::CLIENT) ? endpoint_t::SERVER : endpoint_t::CLIENT);
	// Если сериализация транспортных параметров удалённого узла не выполнена
	if(!quic::params::serialize::encode(params, this->_transport.remote, sender))
		// Выводим пустой результат
		return string();
	// Результирующий билет возобновления с транспортными параметрами
	string result = "";
	// Резервируем память под билет возобновления
	result.reserve(4 + params.size() + ticket.size());
	// Дописываем размер транспортных параметров в сетевом порядке октетов
	for(uint8_t i = 0; i < 4; i++)
		// Дописываем очередной октет размера транспортных параметров
		result.push_back(static_cast <char> ((params.size() >> ((3 - i) * 8)) & 0xFF));
	// Дописываем транспортные параметры удалённого узла
	result.append(params);
	// Дописываем билет возобновления
	result.append(ticket);
	// Выводим билет возобновления с транспортными параметрами
	return result;
}
/**
 * @brief Метод установки возобновляемой сессии соединения (RFC 9001 §4.6)
 *
 * @param session сериализованная сессия
 * @return        результат установки
 */
bool awh::quic::Connection::session(string_view session) noexcept {
	// Если соединение уже начато
	if(this->_core.state != state_t::NONE)
		// Выводим отрицательный результат
		return false;
	// Если билет возобновления короче заголовка размера транспортных параметров
	if(session.size() < 4)
		// Выводим отрицательный результат
		return false;
	// Извлекаем размер транспортных параметров удалённого узла
	size_t size = 0;
	// Собираем размер транспортных параметров из сетевого порядка октетов
	for(uint8_t i = 0; i < 4; i++)
		// Дописываем очередной октет размера транспортных параметров
		size = ((size << 8) | static_cast <uint8_t> (session[i]));
	// Если размер транспортных параметров выходит за границы билета
	if((4 + size) > session.size())
		// Выводим отрицательный результат
		return false;
	// Код ошибки разбора транспортных параметров
	error_t error = error_t::NO_ERROR;
	// Транспортные параметры удалённого узла прошлого соединения
	quic::params::params_t params;
	// Определяем роль удалённого узла (противоположна локальной)
	const endpoint_t sender = ((this->_core.endpoint == endpoint_t::CLIENT) ? endpoint_t::SERVER : endpoint_t::CLIENT);
	// Если разбор транспортных параметров удалённого узла не выполнен
	if(quic::params::parser::decode(reinterpret_cast <const uint8_t *> (session.data() + 4), size, sender, params, error) != status_t::OK)
		// Выводим отрицательный результат
		return false;
	/**
	 * Применяем запомненные лимиты удалённого узла: под ними отправляются ранние
	 * данные, а по завершении хендшейка они заменяются анонсированными в этом
	 * соединении. Занижать анонсированные значения относительно запомненных
	 * удалённый узел не вправе (RFC 9001 §4.6.1)
	 */
	this->_flow.txMax = params.initialMaxData;
	// Устанавливаем запомненный лимит на локально открываемые двунаправленные потоки
	this->_limits.maxBidiRemote = params.initialMaxStreamsBidi;
	// Устанавливаем запомненный лимит на локально открываемые однонаправленные потоки
	this->_limits.maxUniRemote = params.initialMaxStreamsUni;
	// Запоминаем транспортные параметры удалённого узла прошлого соединения
	this->_transport.remote = params;
	// Устанавливаем возобновляемую сессию хендшейк-машине
	return this->_crypto.handshake.session(session.substr(4 + size));
}
/**
 * @brief Метод проверки принятия ранних данных удалённым узлом (RFC 9001 §4.6.2)
 *
 * @return результат проверки
 */
bool awh::quic::Connection::early() const noexcept {
	// Выводим результат принятия ранних данных хендшейк-машиной
	return this->_crypto.handshake.early();
}
/**
 * @brief Метод получения окна перегрузки congestion control (RFC 9002 §7)
 *
 * @return окно перегрузки в октетах
 */
uint64_t awh::quic::Connection::cwnd() const noexcept {
	// Выводим окно перегрузки
	return this->_congestion.window;
}
/**
 * @brief Метод получения количества неподтверждённых октетов в полёте
 *
 * @return количество неподтверждённых октетов в полёте
 */
uint64_t awh::quic::Connection::inflight() const noexcept {
	// Выводим количество неподтверждённых октетов в полёте
	return this->_congestion.inflight;
}
/**
 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
 *
 * @param code   код ошибки приложения
 * @param reason человекочитаемая причина завершения
 */
void awh::quic::Connection::close(const uint64_t code, string_view reason) noexcept {
	// Если завершение соединения ещё не поставлено в очередь
	if(!this->_close.queued && (this->_core.state != state_t::NONE) && (this->_core.state != state_t::DRAINING)){
		// Устанавливаем код ошибки завершения соединения
		this->_close.code = code;
		// Устанавливаем флаг ошибки приложения
		this->_close.app = true;
		// Устанавливаем причину завершения соединения
		this->_close.reason.assign(reason);
		// Устанавливаем флаг постановки завершения соединения в очередь
		this->_close.queued = true;
		// Устанавливаем состояние завершения соединения
		this->_core.state = state_t::CLOSING;
	}
}
/**
 * @brief Метод регистрации отброшенного пакета
 *
 * @param reason причина отбрасывания пакета
 */
void awh::quic::Connection::drop([[maybe_unused]] const char * reason) const noexcept {
	/**
	 * Если включён режим отладки
	 */
	#if DEBUG_MODE
		// Записываем причину отбрасывания пакета в лог
		this->_log->debug(
			"QUIC packet dropped: %s", __PRETTY_FUNCTION__,
			make_tuple(static_cast <uint16_t> (this->_core.endpoint), static_cast <uint16_t> (this->_core.state)),
			log_t::flag_t::WARNING, reason
		);
	#endif
}
/**
 * @brief Метод проверки готовности соединения к отправке данных приложения
 *
 * @details Установленное соединение отправляет данные обычным порядком. До его
 *          установления отправка возможна только клиентом на возобновлённой
 *          сессии: ключи защиты ранних данных выданы, лимиты взяты из прошлого
 *          соединения, и данные уходят, не дожидаясь хендшейка (RFC 9001 §4.6)
 *
 * @return результат проверки
 */
bool awh::quic::Connection::writable() const noexcept {
	// Если соединение установлено
	if(this->_core.state == state_t::CONNECTED)
		// Выводим положительный результат
		return true;
	// Если соединение не выполняет хендшейк либо эндпоинт не является клиентом
	if((this->_core.state != state_t::HANDSHAKING) || (this->_core.endpoint != endpoint_t::CLIENT))
		// Выводим отрицательный результат
		return false;
	// Выводим результат наличия ключей защиты ранних данных
	return (this->_crypto.handshake.encryption(level_t::EARLY_DATA) != nullptr);
}
/**
 * @brief Метод открытия нового потока приложения (RFC 9000 §2.1)
 *
 * @param unidirectional флаг однонаправленного потока
 * @return               идентификатор потока (INVALID_STREAM - открытие невозможно)
 */
uint64_t awh::quic::Connection::open(const bool unidirectional) noexcept {
	// Если соединение к отправке данных приложения не готово
	if(!this->writable())
		// Выводим недопустимый идентификатор потока
		return INVALID_STREAM;
	// Получаем счётчик открытых локально потоков
	uint64_t & opened = (unidirectional ? this->_limits.openedUni : this->_limits.openedBidi);
	// Получаем лимит удалённого эндпоинта на локально открываемые потоки
	const uint64_t limit = (unidirectional ? this->_limits.maxUniRemote : this->_limits.maxBidiRemote);
	// Если лимит потоков удалённого эндпоинта исчерпан (RFC 9000 §4.6)
	if(opened >= limit){
		/**
		 * Сигнализируем удалённому эндпоинту блокировку лимитом потоков фреймом
		 * STREAMS_BLOCKED - однократно на каждое значение лимита, чтобы не наводнять
		 * повторами при частых попытках открытия (RFC 9000 §19.14)
		 */
		if(unidirectional){
			// Если блокировка при текущем лимите ещё не сигнализирована
			if(this->_limits.uniBlockedAt != limit)
				// Устанавливаем флаг необходимости отправки STREAMS_BLOCKED
				this->_limits.uniBlocked = true;
		// Если блокировка двунаправленных потоков при текущем лимите ещё не сигнализирована
		} else if(this->_limits.bidiBlockedAt != limit)
			// Устанавливаем флаг необходимости отправки STREAMS_BLOCKED
			this->_limits.bidiBlocked = true;
		// Выводим недопустимый идентификатор потока
		return INVALID_STREAM;
	}
	// Вычисляем идентификатор нового потока (RFC 9000 §2.1)
	const uint64_t sid = ((opened << 2) | (unidirectional ? 0x02 : 0x00) | ((this->_core.endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
	// Увеличиваем счётчик открытых локально потоков
	opened++;
	// Создаём состояние нового потока
	auto ret = this->_stream.list.emplace(sid, stream_data_t());
	// Устанавливаем начальный лимит приёма потока
	ret.first->second.rxMax = this->rxWindow(sid);
	// Устанавливаем начальный лимит отправки потока
	ret.first->second.txMax = this->txWindow(sid);
	// Выводим идентификатор нового потока
	return sid;
}
/**
 * @brief Метод постановки данных потока в очередь отправки (RFC 9000 §2.2)
 *
 * @param sid  идентификатор потока
 * @param data данные потока приложения
 * @param fin  флаг завершения потока (FIN)
 * @return     результат постановки (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::send(const uint64_t sid, string_view data, const bool fin) noexcept {
	// Если соединение к отправке данных приложения не готово
	if(!this->writable())
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Если отправка данных в поток локальным эндпоинтом недопустима
	if(!this->sendable(sid))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Ищем поток по идентификатору
	auto i = this->_stream.list.find(sid);
	// Если поток не найден
	if(i == this->_stream.list.end())
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Если отправка потока завершена либо аварийно прекращена
	if(i->second.txFin || i->second.txReset || i->second.txResetSent)
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Дописываем данные в буфер исходящих данных потока
	i->second.txBuffer.append(data);
	// Если приложение завершает поток
	if(fin)
		// Устанавливаем флаг постановки завершения потока
		i->second.txFin = true;
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод постановки датаграммы приложения в очередь отправки (RFC 9221 §4)
 *
 * @param data данные датаграммы приложения
 * @return     результат постановки (ERROR - датаграммы не поддерживаются либо размер превышен)
 */
awh::quic::status_t awh::quic::Connection::datagram(string_view data) noexcept {
	// Если соединение к отправке данных приложения не готово
	if(!this->writable())
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Получаем предельный размер данных отправляемой датаграммы
	const size_t limit = this->datagrams();
	// Если удалённый узел датаграммы не принимает либо размер превышает предел
	if((limit == 0) || (data.size() > limit))
		// Выводим отрицательный результат
		return status_t::ERROR;
	/**
	 * Если очередь отправки заполнена: датаграммы flow control не подчиняются,
	 * поэтому очередь ограничена сверху, а ненадёжность доставки позволяет
	 * отбросить наиболее старую (RFC 9221 §5.3)
	 */
	if(this->_dgram.tx.size() >= MAX_QUEUED_DATAGRAMS)
		// Отбрасываем наиболее старую датаграмму очереди отправки
		this->_dgram.tx.pop_front();
	// Ставим датаграмму приложения в очередь отправки
	this->_dgram.tx.emplace_back(data);
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод извлечения принятой датаграммы приложения (RFC 9221 §4)
 *
 * @param output буфер принятой датаграммы приложения
 * @return       результат извлечения (false - принятых датаграмм нет)
 */
bool awh::quic::Connection::datagram(string & output) noexcept {
	// Если принятых датаграмм приложения нет
	if(this->_dgram.rx.empty())
		// Выводим отрицательный результат
		return false;
	// Извлекаем наиболее раннюю принятую датаграмму приложения
	output = ::move(this->_dgram.rx.front());
	// Удаляем выданную датаграмму из очереди
	this->_dgram.rx.pop_front();
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
 *
 * @return предельный размер данных отправляемой датаграммы в октетах
 */
size_t awh::quic::Connection::datagrams() const noexcept {
	// Получаем анонсированный удалённым узлом предел размера фрейма
	const uint64_t limit = this->_transport.remote.maxDatagramFrameSize;
	// Если удалённый узел приём датаграмм не анонсировал
	if(limit == 0)
		// Выводим нулевой предел - отправка датаграмм невозможна
		return 0;
	/**
	 * Ограничиваем предел размером, помещающимся в одну исходящую датаграмму:
	 * фрейм DATAGRAM не фрагментируется, и не помещающийся в пакет отправлен
	 * быть не может (RFC 9221 §5)
	 */
	const size_t bound = ::min(static_cast <size_t> (limit), this->_pmtu.size);
	// Предельные накладные расходы пакета уровня приложения с защитой
	const size_t reserve = (DATAGRAM_OVERHEAD + CONTROL_OVERHEAD);
	// Если предел не покрывает накладные расходы пакета и фрейма
	if(bound <= reserve)
		// Выводим нулевой предел - отправка датаграмм невозможна
		return 0;
	// Выводим предельный размер данных отправляемой датаграммы
	return (bound - reserve);
}
/**
 * @brief Метод получения списка потоков с данными для приложения
 *
 * @return список идентификаторов потоков с собранными данными либо завершением
 */
void awh::quic::Connection::readable(vector <uint64_t> & output) noexcept {
	// Очищаем список идентификаторов потоков
	output.clear();
	// Позиция записи сохраняемых записей списка готовых
	auto position = this->_stream.readable.begin();
	/**
	 * Перебираем список готовых потоков, отсеивая устаревшие записи: поток мог
	 * быть выдан приложению, сброшен либо удалён сборкой завершённых
	 */
	for(auto i = this->_stream.readable.begin(); i != this->_stream.readable.end(); ++i){
		// Ищем поток по идентификатору
		auto entry = this->_stream.list.find(* i);
		// Если поток удалён либо более не готов к выдаче
		if((entry == this->_stream.list.end()) || !this->ready(entry->second)){
			// Если поток ещё существует
			if(entry != this->_stream.list.end())
				// Снимаем отметку о присутствии в списке готовых
				entry->second.queued = false;
			// Переходим к следующей записи - устаревшая запись не сохраняется
			continue;
		}
		// Если позиция записи отстала от позиции чтения
		if(position != i)
			// Сдвигаем запись к позиции записи
			(* position) = (* i);
		// Продвигаем позицию записи
		++position;
		// Добавляем идентификатор потока в выходной список
		output.push_back(* i);
	}
	// Удаляем устаревшие записи из списка готовых
	this->_stream.readable.erase(position, this->_stream.readable.end());
}
/**
 * @brief Метод выдачи собранных данных потока приложению (RFC 9000 §2.2)
 *
 * @param sid    идентификатор потока
 * @param output собранные данные потока (дописываются)
 * @param fin    флаг завершения потока удалённым эндпоинтом (FIN)
 * @return       результат выдачи (OK/ERROR - поток неизвестен либо сброшен)
 */
awh::quic::status_t awh::quic::Connection::receive(const uint64_t sid, string & output, bool & fin) noexcept {
	// Сбрасываем флаг завершения потока
	fin = false;
	// Ищем поток по идентификатору
	auto i = this->_stream.list.find(sid);
	// Если поток не найден либо приём данных недопустим
	if((i == this->_stream.list.end()) || !this->receivable(sid))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Получаем состояние потока
	auto & stream = i->second;
	// Если поток сброшен удалённым эндпоинтом
	if(stream.rxReset)
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Если есть собранные данные потока
	if(!stream.rxReady.empty()){
		// Учитываем выданные данные в flow control соединения
		this->_flow.rxConsumed += stream.rxReady.size();
		// Продвигаем учтённое смещение данных потока
		stream.rxCounted += stream.rxReady.size();
		// Дописываем собранные данные в выходной буфер
		output.append(stream.rxReady);
		// Очищаем буфер выдачи приложению
		stream.rxReady.clear();
	}
	// Если поток ещё не завершён - обновляем окно приёма потока
	if(!stream.rxFin){
		// Получаем начальный лимит приёма потока (размер окна)
		const uint64_t window = this->rxWindow(sid);
		// Если потреблено больше половины окна приёма потока
		if((stream.rxMax - stream.rxOffset) < (window / 2)){
			// Продвигаем анонсированный лимит приёма потока
			stream.rxMax = (stream.rxOffset + window);
			// Устанавливаем флаг необходимости отправки обновлённого лимита MAX_STREAM_DATA
			stream.rxMaxQueued = true;
		}
	}
	// Если потреблено больше половины окна приёма соединения
	if((this->_flow.rxMax - this->_flow.rxConsumed) < (this->_transport.local.initialMaxData / 2)){
		// Продвигаем анонсированный лимит приёма данных соединения
		this->_flow.rxMax = (this->_flow.rxConsumed + this->_transport.local.initialMaxData);
		// Устанавливаем флаг необходимости отправки обновлённого лимита MAX_DATA
		this->_flow.rxQueued = true;
	}
	// Если поток завершён и все данные собраны и выданы
	if(stream.rxFin && (stream.rxOffset == stream.rxFinal)){
		// Устанавливаем флаг завершения потока
		fin = true;
		// Если завершение потока выдаётся впервые
		if(!stream.rxFinDelivered){
			// Устанавливаем флаг выданного завершения потока
			stream.rxFinDelivered = true;
			// Учитываем завершение потока удалённого эндпоинта в лимите MAX_STREAMS
			this->credit(sid, stream);
		}
	}
	// Обновляем готовность потока к выдаче данных приложению
	this->notify(sid, stream);
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод аварийного завершения отправки потока (RFC 9000 §2.4)
 *
 * @param sid  идентификатор потока
 * @param code код ошибки приложения
 */
void awh::quic::Connection::reset(const uint64_t sid, const uint64_t code) noexcept {
	// Если отправка данных в поток локальным эндпоинтом недопустима
	if(!this->sendable(sid))
		// Выходим из метода
		return;
	// Ищем поток по идентификатору
	auto i = this->_stream.list.find(sid);
	// Если поток не найден либо аварийное завершение уже выполнялось
	if((i == this->_stream.list.end()) || i->second.txReset || i->second.txResetSent)
		// Выходим из метода
		return;
	// Ставим отправку фрейма RESET_STREAM в очередь
	i->second.txReset = true;
	// Устанавливаем код ошибки приложения фрейма RESET_STREAM
	i->second.txResetCode = code;
	// Отбрасываем неотправленные данные потока
	i->second.txBuffer.clear();
	// Сбрасываем курсор упакованных данных буфера
	i->second.txCursor = 0;
	// Сбрасываем флаг постановки завершения потока
	i->second.txFin = false;
}
/**
 * @brief Метод запроса прекращения передачи удалённым эндпоинтом (RFC 9000 §3.5)
 *
 * @param sid  идентификатор потока
 * @param code код ошибки приложения
 */
void awh::quic::Connection::stop(const uint64_t sid, const uint64_t code) noexcept {
	// Если приём данных потока локальным эндпоинтом недопустим
	if(!this->receivable(sid))
		// Выходим из метода
		return;
	// Ищем поток по идентификатору
	auto i = this->_stream.list.find(sid);
	// Если поток не найден либо запрос прекращения уже выполнялся
	if((i == this->_stream.list.end()) || i->second.stopQueued || i->second.stopSent)
		// Выходим из метода
		return;
	// Получаем состояние потока
	auto & stream = i->second;
	// Ставим отправку фрейма STOP_SENDING в очередь
	stream.stopQueued = true;
	// Устанавливаем код ошибки приложения фрейма STOP_SENDING
	stream.stopCode = code;
	// Учитываем отброшенные данные как потреблённые в flow control соединения
	this->consume(stream, stream.rxHigh);
	// Отбрасываем несобранные фрагменты данных потока
	stream.rxBuffer.clear();
	// Отбрасываем собранные данные потока
	stream.rxReady.clear();
	/**
	 * Если финальный размер потока уже принят - приём завершён и данные приложению
	 * выданы не будут, поэтому лимит потоков возвращается здесь (RFC 9000 §4.6)
	 */
	if(stream.rxFin)
		// Учитываем завершение потока удалённого эндпоинта в лимите MAX_STREAMS
		this->credit(sid, stream);
	// Обновляем готовность потока к выдаче данных приложению
	this->notify(sid, stream);
}
/**
 * @brief Метод проверки аварийного завершения потока удалённым эндпоинтом
 *
 * @param sid  идентификатор потока
 * @param code код ошибки приложения принятого фрейма RESET_STREAM
 * @return     результат проверки (true - поток сброшен удалённым эндпоинтом)
 */
bool awh::quic::Connection::aborted(const uint64_t sid, uint64_t & code) const noexcept {
	// Ищем поток по идентификатору
	auto i = this->_stream.list.find(sid);
	// Если поток найден и сброшен удалённым эндпоинтом
	if((i != this->_stream.list.end()) && i->second.rxReset){
		// Устанавливаем код ошибки приложения принятого фрейма RESET_STREAM
		code = i->second.rxResetCode;
		// Выводим положительный результат
		return true;
	}
	// Выводим отрицательный результат
	return false;
}
/**
 * @brief Метод получения состояния соединения
 *
 * @return состояние соединения
 */
awh::quic::Connection::state_t awh::quic::Connection::state() const noexcept {
	// Выводим состояние соединения
	return this->_core.state;
}
/**
 * @brief Метод получения кода ошибки транспорта соединения
 *
 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
 */
awh::quic::error_t awh::quic::Connection::error() const noexcept {
	// Выводим код ошибки транспорта соединения
	return this->_core.error;
}
/**
 * @brief Метод получения идентификатора соединения локального эндпоинта
 *
 * @return идентификатор соединения локального эндпоинта
 */
const awh::quic::cid_t & awh::quic::Connection::scid() const noexcept {
	// Выводим идентификатор соединения локального эндпоинта
	return this->_cids.identity.source;
}
/**
 * @brief Метод извлечения изменений набора идентификаторов локального эндпоинта
 *
 * @param added   идентификаторы, введённые в обращение
 * @param removed идентификаторы, выведенные из обращения
 */
void awh::quic::Connection::issued(vector <cid_t> & added, vector <cid_t> & removed) noexcept {
	// Выполняем очистку списка введённых в обращение идентификаторов
	added.clear();
	// Выполняем очистку списка выведенных из обращения идентификаторов
	removed.clear();
	// Если накоплены введённые в обращение идентификаторы
	if(!this->_cids.routing.added.empty())
		// Выполняем выдачу введённых в обращение идентификаторов
		added.swap(this->_cids.routing.added);
	// Если накоплены выведенные из обращения идентификаторы
	if(!this->_cids.routing.removed.empty())
		// Выполняем выдачу выведенных из обращения идентификаторов
		removed.swap(this->_cids.routing.removed);
}
/**
 * @brief Метод получения идентификатора соединения удалённого эндпоинта
 *
 * @return идентификатор соединения удалённого эндпоинта
 */
const awh::quic::cid_t & awh::quic::Connection::dcid() const noexcept {
	// Выводим идентификатор соединения удалённого эндпоинта
	return this->_cids.identity.destination;
}
/**
 * @brief Метод доступа к машине криптографического хендшейка
 *
 * @return машина криптографического хендшейка
 */
const awh::quic::handshake_t & awh::quic::Connection::handshake() const noexcept {
	// Выводим машину криптографического хендшейка
	return this->_crypto.handshake;
}
/**
 * @brief Конструктор
 *
 * @param endpoint роль локального эндпоинта на соединении
 * @param ctx      идентификатор шаблона контекста безопасности
 * @param coder    объект кодера транспортной безопасности
 * @param log      объект для работы с логами
 */
awh::quic::Connection::Connection(const endpoint_t endpoint, const tls::coder_t::id_t ctx, const tls::coder_t & coder, const log_t * log) noexcept :
 _core(endpoint), _amplify(endpoint), _log(log), _crypto(endpoint, ctx, coder, log) {}
