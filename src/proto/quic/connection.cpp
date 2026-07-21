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
#include <openssl/rand.h>

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
	static constexpr size_t CID_SIZE = 8;
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
	 * @brief Запас октетов на заголовок пакета и тег AEAD при планировании нагрузки
	 *
	 */
	static constexpr size_t OVERHEAD_RESERVE = 64;
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
	 * @brief Максимальная задержка подтверждения удалённого эндпоинта в миллисекундах (RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t MAX_ACK_DELAY = 25;
	/**
	 * @brief Предельный показатель экспоненциальной выдержки таймера PTO
	 *
	 */
	static constexpr uint8_t MAX_PTO_COUNT = 10;
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
awh::quic::Connection::Sent::Sent() noexcept : pn(0), time(0), handshakeDone(false) {}

/**
 * @brief Конструктор состояния потока приложения
 *
 */
awh::quic::Connection::Stream::Stream() noexcept :
 txOffset(0), txBuffer{""}, txMax(0), txFin(false), txFinSent(false),
 txReset(false), txResetSent(false), txResetCode(0), txBlocked(false),
 rxOffset(0), rxHigh(0), rxReady{""}, rxMax(0), rxMaxQueued(false),
 rxFin(false), rxFinal(0), rxFinDelivered(false), rxReset(false),
 rxResetCode(0), stopQueued(false), stopSent(false), stopCode(0),
 credited(false) {}

/**
 * @brief Конструктор состояния пространства номеров пакетов
 *
 */
awh::quic::Connection::Space::Space() noexcept :
 txPn(0), largestAcked(0), hasAcked(false), largestRx(0), hasRx(false),
 ackElicited(false), txOffset(0), txBuffer{""}, rxOffset(0), lossTime(0),
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
	// Если принят наибольший номер пакета
	if(!item.hasRx || (pn > item.largestRx)){
		// Обновляем наибольший принятый номер пакета
		item.largestRx = pn;
		// Устанавливаем флаг приёма хотя бы одного пакета
		item.hasRx = true;
	}
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
		// Если номер пакета примыкает к диапазону сверху
		if(pn == (i->high + 1)){
			// Расширяем диапазон вверх
			i->high = pn;
			// Если предыдущий диапазон примыкает к расширенному
			if((i != item.ranges.begin()) && ((i - 1)->low == (pn + 1))){
				// Расширяем текущий диапазон до предыдущего
				i->high = (i - 1)->high;
				// Удаляем предыдущий диапазон
				item.ranges.erase(i - 1);
			}
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
	/**
	 * Перебираем диапазоны принятых номеров пакетов
	 */
	for(auto & range : item.ranges){
		// Если номер пакета входит в диапазон
		if((pn >= range.low) && (pn <= range.high))
			// Выводим положительный результат - дубликат
			return true;
	}
	// Выводим отрицательный результат
	return false;
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
		if(this->_handshake.pending(level))
			// Перекладываем данные в буфер пространства номеров пакетов
			this->_spaces[static_cast <size_t> (this->space(level))].txBuffer.append(this->_handshake.data(level));
	}
}
/**
 * @brief Метод постановки завершения соединения с ошибкой транспорта в очередь
 *
 * @param error код ошибки транспорта
 */
void awh::quic::Connection::fail(const error_t error) noexcept {
	// Если завершение соединения ещё не поставлено в очередь
	if(!this->_closeQueued){
		// Устанавливаем код ошибки транспорта соединения
		this->_error = error;
		// Устанавливаем код ошибки завершения соединения
		this->_closeCode = static_cast <uint64_t> (error);
		// Сбрасываем флаг ошибки приложения
		this->_closeApp = false;
		// Устанавливаем флаг постановки завершения соединения в очередь
		this->_closeQueued = true;
		// Устанавливаем состояние завершения соединения
		this->_state = state_t::CLOSING;
	}
}
/**
 * @brief Метод сброса ключей уровня вместе с состоянием восстановления потерь (RFC 9001 §4.9)
 *
 * @param level уровень шифрования
 */
void awh::quic::Connection::discard(const level_t level) noexcept {
	// Сбрасываем ключи уровня шифрования
	this->_handshake.discard(level);
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
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
	this->_latestRtt = sample;
	// Если это первое измерение задержки приёма-передачи
	if(!this->_rttSampled){
		// Устанавливаем минимальную задержку приёма-передачи
		this->_minRtt = sample;
		// Устанавливаем сглаженную задержку приёма-передачи
		this->_smoothedRtt = sample;
		// Устанавливаем вариативность задержки приёма-передачи
		this->_rttVar = (sample / 2);
		// Устанавливаем флаг наличия первого измерения
		this->_rttSampled = true;
	// Если измерения уже выполнялись
	} else {
		// Обновляем минимальную задержку приёма-передачи
		this->_minRtt = ::min(this->_minRtt, sample);
		// Скорректированная задержка приёма-передачи
		uint64_t adjusted = sample;
		// Если вычитание задержки подтверждения не опускает оценку ниже минимальной (RFC 9002 §5.3)
		if((adjusted >= delay) && ((adjusted - delay) >= this->_minRtt))
			// Вычитаем задержку подтверждения удалённого эндпоинта
			adjusted -= delay;
		// Вычисляем отклонение от сглаженной задержки
		const uint64_t deviation = ((this->_smoothedRtt > adjusted) ? (this->_smoothedRtt - adjusted) : (adjusted - this->_smoothedRtt));
		// Обновляем вариативность задержки приёма-передачи
		this->_rttVar = (((3 * this->_rttVar) + deviation) / 4);
		// Обновляем сглаженную задержку приёма-передачи
		this->_smoothedRtt = (((7 * this->_smoothedRtt) + adjusted) / 8);
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
	// Перебираем отправленные CRYPTO-данные пакета
	for(auto & chunk : packet.crypto)
		// Ставим CRYPTO-данные в очередь ретрансмиссии
		item.rtxQueue.push_back(chunk);
	// Если пакет содержал фрейм HANDSHAKE_DONE
	if(packet.handshakeDone)
		// Восстанавливаем флаг необходимости отправки фрейма HANDSHAKE_DONE
		this->_handshakeDone = true;
	// Перебираем отправленные блоки данных потоков приложения
	for(auto & chunk : packet.stream)
		// Ставим блок данных потока в очередь ретрансмиссии
		this->_streamRtx.push_back(chunk);
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
				this->_rxMaxDataQueued = true;
			break;
			// Фрейм лимита двунаправленных потоков MAX_STREAMS
			case frame_t::MAX_STREAMS_BIDI:
				// Восстанавливаем флаг необходимости отправки обновлённого лимита
				this->_maxBidiQueued = true;
			break;
			// Фрейм лимита однонаправленных потоков MAX_STREAMS
			case frame_t::MAX_STREAMS_UNI:
				// Восстанавливаем флаг необходимости отправки обновлённого лимита
				this->_maxUniQueued = true;
			break;
			// Фреймы состояния потока
			case frame_t::MAX_STREAM_DATA:
			case frame_t::RESET_STREAM:
			case frame_t::STOP_SENDING: {
				// Ищем поток по идентификатору
				auto i = this->_streams.find(control.second);
				// Если поток найден
				if(i != this->_streams.end()){
					// Если потерян фрейм лимита данных потока MAX_STREAM_DATA
					if(control.first == frame_t::MAX_STREAM_DATA)
						// Восстанавливаем флаг необходимости отправки обновлённого лимита
						i->second.rxMaxQueued = true;
					// Если потерян фрейм аварийного завершения потока RESET_STREAM
					else if(control.first == frame_t::RESET_STREAM)
						// Восстанавливаем флаг необходимости отправки фрейма RESET_STREAM
						i->second.txReset = true;
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
	const uint64_t base = (this->_rttSampled ? ::max(this->_latestRtt, this->_smoothedRtt) : INITIAL_RTT);
	// Вычисляем задержку детекта потерь с коэффициентом 9/8
	const uint64_t lossDelay = ::max((9 * base) / 8, GRANULARITY);
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
		if(((i->pn + PACKET_THRESHOLD) <= item.largestAcked) || ((i->time + lossDelay) <= this->_now)){
			// Ставим содержимое потерянного пакета в очереди отправки
			this->requeue(space, * i);
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
	uint64_t result = (this->_rttSampled ? (this->_smoothedRtt + ::max(4 * this->_rttVar, GRANULARITY)) : (3 * INITIAL_RTT));
	// Если пространство пакетов приложения и хендшейк подтверждён (RFC 9002 §6.2.1)
	if((space == space_t::APPLICATION) && this->_confirmed)
		// Дописываем максимальную задержку подтверждения удалённого эндпоинта
		result += MAX_ACK_DELAY;
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
	if(item.sent.empty())
		// Выводим нулевой дедлайн - таймер не взведён
		return 0;
	// Выводим дедлайн: время последней ack-eliciting отправки + интервал с экспоненциальной выдержкой
	return (item.lastElicited + (this->interval(space) << this->_ptoCount));
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
	return ((sid & 0x01) == ((this->_endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
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
	return ((sid & 0x01) != ((this->_endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
}
/**
 * @brief Метод получения начального лимита приёма потока из локальных параметров
 *
 * @param sid идентификатор потока
 * @return    начальный лимит приёма потока в октетах
 */
uint64_t awh::quic::Connection::rxWindow(const uint64_t sid) const noexcept {
	// Определяем инициатора потока
	const bool local = ((sid & 0x01) == ((this->_endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
	// Если поток однонаправленный
	if((sid & 0x02) != 0)
		// Выводим лимит однонаправленных потоков (приём только от удалённого инициатора)
		return (local ? 0 : this->_params.initialMaxStreamDataUni);
	// Двунаправленный поток: лимит зависит от инициатора (RFC 9000 §18.2)
	return (local ? this->_params.initialMaxStreamDataBidiLocal : this->_params.initialMaxStreamDataBidiRemote);
}
/**
 * @brief Метод получения начального лимита отправки потока из параметров удалённого эндпоинта
 *
 * @param sid идентификатор потока
 * @return    начальный лимит отправки потока в октетах
 */
uint64_t awh::quic::Connection::txWindow(const uint64_t sid) const noexcept {
	// Определяем инициатора потока
	const bool local = ((sid & 0x01) == ((this->_endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
	// Если поток однонаправленный
	if((sid & 0x02) != 0)
		// Выводим лимит однонаправленных потоков (отправка только локальным инициатором)
		return (local ? this->_remote.initialMaxStreamDataUni : 0);
	// Двунаправленный поток: лимит зависит от инициатора (RFC 9000 §18.2)
	return (local ? this->_remote.initialMaxStreamDataBidiRemote : this->_remote.initialMaxStreamDataBidiLocal);
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
	auto i = this->_streams.find(sid);
	// Если поток найден
	if(i != this->_streams.end())
		// Выводим состояние потока
		return &i->second;
	// Определяем инициатора потока
	const bool local = ((sid & 0x01) == ((this->_endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
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
	const uint64_t limit = (unidirectional ? this->_maxUniLocal : this->_maxBidiLocal);
	// Если порядковый номер превышает анонсированный лимит (RFC 9000 §4.6)
	if(index >= limit){
		// Устанавливаем код ошибки превышения лимита потоков
		error = error_t::STREAM_LIMIT_ERROR;
		// Выводим пустой результат
		return nullptr;
	}
	// Получаем счётчик принятых потоков удалённого эндпоинта
	uint64_t & accepted = (unidirectional ? this->_acceptedUni : this->_acceptedBidi);
	// Если открыт поток с наибольшим порядковым номером
	if((index + 1) > accepted)
		// Обновляем счётчик принятых потоков (потоки с меньшими номерами открываются неявно)
		accepted = (index + 1);
	// Создаём состояние нового потока удалённого эндпоинта
	auto ret = this->_streams.emplace(sid, stream_data_t());
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
		this->_rxData += delta;
		// Обновляем наибольшее принятое смещение данных потока
		stream->rxHigh = end;
		// Если данные превышают анонсированный лимит приёма соединения (RFC 9000 §4.1)
		if(this->_rxData > this->_rxMaxData){
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
		this->_rxConsumed += delta;
		// Выводим положительный результат
		return status_t::OK;
	}
	// Если данные фрейма уже полностью собраны
	if(!frame.data.empty() && (end > stream->rxOffset)){
		// Если данные фрейма опережают непрерывно собранное смещение
		if(frame.offset > stream->rxOffset){
			// Ищем буферизированный фрагмент с тем же смещением
			auto i = stream->rxBuffer.find(frame.offset);
			// Если фрагмент с тем же смещением не буферизирован либо новый фрагмент длиннее
			if((i == stream->rxBuffer.end()) || (i->second.size() < frame.data.size()))
				// Буферизируем фрагмент до заполнения разрыва
				stream->rxBuffer[frame.offset] = string(frame.data);
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
	if(this->_handshake.peer(this->_remote, error) != status_t::OK){
		// Ставим завершение соединения с ошибкой транспортных параметров в очередь
		this->fail((error != error_t::NO_ERROR) ? error : error_t::TRANSPORT_PARAMETER_ERROR);
		// Выводим отрицательный результат
		return false;
	}
	// Устанавливаем лимит отправки данных соединения от удалённого эндпоинта
	this->_txMaxData = this->_remote.initialMaxData;
	// Устанавливаем лимит на локально открываемые двунаправленные потоки
	this->_maxBidiRemote = this->_remote.initialMaxStreamsBidi;
	// Устанавливаем лимит на локально открываемые однонаправленные потоки
	this->_maxUniRemote = this->_remote.initialMaxStreamsUni;
	// Устанавливаем анонсированный лимит приёма данных соединения
	this->_rxMaxData = this->_params.initialMaxData;
	// Устанавливаем анонсированный лимит на двунаправленные потоки удалённого эндпоинта
	this->_maxBidiLocal = this->_params.initialMaxStreamsBidi;
	// Устанавливаем анонсированный лимит на однонаправленные потоки удалённого эндпоинта
	this->_maxUniLocal = this->_params.initialMaxStreamsUni;
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
	if((sid & 0x01) == ((this->_endpoint == endpoint_t::SERVER) ? 0x01 : 0x00))
		// Выходим из метода
		return;
	// Устанавливаем флаг учтённого завершения потока
	stream.credited = true;
	// Если поток однонаправленный
	if((sid & 0x02) != 0){
		// Увеличиваем анонсированный лимит однонаправленных потоков
		this->_maxUniLocal++;
		// Устанавливаем флаг необходимости отправки обновлённого лимита MAX_STREAMS
		this->_maxUniQueued = true;
	// Если поток двунаправленный
	} else {
		// Увеличиваем анонсированный лимит двунаправленных потоков
		this->_maxBidiLocal++;
		// Устанавливаем флаг необходимости отправки обновлённого лимита MAX_STREAMS
		this->_maxBidiQueued = true;
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
	if(this->_handshake.crypto(level, reinterpret_cast <const uint8_t *> (data.data()), data.size()) != status_t::OK){
		// Ставим завершение соединения с ошибкой хендшейка в очередь
		this->fail(this->_handshake.error());
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
			if(this->_handshake.crypto(level, reinterpret_cast <const uint8_t *> (chunk.data()), chunk.size()) != status_t::OK){
				// Ставим завершение соединения с ошибкой хендшейка в очередь
				this->fail(this->_handshake.error());
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
awh::quic::status_t awh::quic::Connection::frames(const level_t level, const uint8_t * data, const size_t size) noexcept {
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
		// Если пакет уровня Initial или Handshake
		if((level == level_t::INITIAL) || (level == level_t::HANDSHAKE)){
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
					// Время отправки наибольшего впервые подтверждённого пакета
					uint64_t sentTime = 0;
					// Флаг подтверждения наибольшего номера пакета фрейма
					bool largest = false;
					/**
					 * Перебираем список отправленных пакетов
					 */
					for(auto i = item.sent.begin(); i != item.sent.end();){
						// Флаг подтверждения приёма пакета
						bool found = false;
						/**
						 * Перебираем диапазоны подтверждённых номеров пакетов
						 */
						for(auto & range : frame.ranges){
							// Если номер пакета входит в диапазон подтверждения
							if((i->pn >= range.low) && (i->pn <= range.high)){
								// Устанавливаем флаг подтверждения приёма пакета
								found = true;
								// Выходим из перебора диапазонов
								break;
							}
						}
						// Если приём пакета подтверждён впервые
						if(found){
							// Устанавливаем флаг наличия впервые подтверждённых пакетов
							acked = true;
							// Если подтверждён наибольший номер пакета фрейма
							if(!frame.ranges.empty() && (i->pn == frame.ranges.front().high)){
								// Запоминаем время отправки пакета
								sentTime = i->time;
								// Устанавливаем флаг подтверждения наибольшего номера
								largest = true;
							}
							// Удаляем подтверждённый пакет из списка отправленных
							i = item.sent.erase(i);
						// Если пакет ещё не подтверждён
						} else ++i;
					}
					// Если впервые подтверждены отправленные пакеты
					if(acked){
						// Сбрасываем счётчик срабатываний таймера PTO (RFC 9002 §6.2.1)
						this->_ptoCount = 0;
						// Если подтверждён наибольший номер и время отправки известно
						if(largest && (this->_now >= sentTime)){
							// Задержка подтверждения удалённого эндпоинта в миллисекундах
							uint64_t delay = 0;
							// Если пространство пакетов приложения (RFC 9002 §5.3)
							if(space == space_t::APPLICATION)
								// Вычисляем задержку подтверждения с показателем по умолчанию (RFC 9000 §18.2)
								delay = ((frame.delay << 3) / 1000);
							// Выполняем обновление оценки задержки приёма-передачи
							this->rtt(this->_now - sentTime, delay);
						}
						// Выполняем детект потерянных пакетов пространства
						this->detect(space);
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
				// Если фрейм прислал клиент (RFC 9000 §19.7)
				if((status == status_t::OK) && (this->_endpoint == endpoint_t::SERVER)){
					// Ставим завершение соединения с нарушением протокола в очередь
					this->fail(error_t::PROTOCOL_VIOLATION);
					// Выводим отрицательный результат
					return status_t::ERROR;
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
						this->_rxData += (frame.finalSize - stream->rxHigh);
						// Обновляем наибольшее принятое смещение данных потока
						stream->rxHigh = frame.finalSize;
						// Если данные превышают анонсированный лимит приёма соединения
						if(this->_rxData > this->_rxMaxData){
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
					this->_rxConsumed += (frame.finalSize - (stream->rxOffset - stream->rxReady.size()));
					// Отбрасываем несобранные фрагменты данных потока
					stream->rxBuffer.clear();
					// Отбрасываем собранные данные потока (RFC 9000 §3.2)
					stream->rxReady.clear();
					// Учитываем завершение потока удалённого эндпоинта в лимите MAX_STREAMS
					this->credit(frame.streamId, * stream);
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
					 * Определяем тип фрейма лимита
					 */
					switch(type){
						// Фрейм лимита данных соединения MAX_DATA (RFC 9000 §19.9)
						case frame_t::MAX_DATA:
							// Обновляем лимит отправки данных соединения (лимиты только растут)
							this->_txMaxData = ::max(this->_txMaxData, value);
						break;
						// Фрейм лимита двунаправленных потоков MAX_STREAMS (RFC 9000 §19.11)
						case frame_t::MAX_STREAMS_BIDI:
							// Обновляем лимит на локально открываемые двунаправленные потоки
							this->_maxBidiRemote = ::max(this->_maxBidiRemote, value);
						break;
						// Фрейм лимита однонаправленных потоков MAX_STREAMS (RFC 9000 §19.11)
						case frame_t::MAX_STREAMS_UNI:
							// Обновляем лимит на локально открываемые однонаправленные потоки
							this->_maxUniRemote = ::max(this->_maxUniRemote, value);
						break;
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
				// Если разобран фрейм лимита данных потока MAX_STREAM_DATA (RFC 9000 §19.10)
				if((status == status_t::OK) && (type == frame_t::MAX_STREAM_DATA)){
					// Ищем поток по идентификатору
					auto i = this->_streams.find(streamId);
					// Если поток найден
					if(i != this->_streams.end())
						// Обновляем лимит отправки потока (лимиты только растут)
						i->second.txMax = ::max(i->second.txMax, value);
				}
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм анонса нового идентификатора соединения NEW_CONNECTION_ID
			case frame_t::NEW_CONNECTION_ID: {
				// Разобранный фрейм анонса идентификатора
				frame::new_connection_id_t frame;
				// Выполняем разбор фрейма NEW_CONNECTION_ID (ротация идентификаторов - следующий этап)
				status = frame::parser::newConnectionId(data + offset, size - offset, frame, consumed, error);
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм проверки достижимости пути PATH_CHALLENGE
			case frame_t::PATH_CHALLENGE: {
				// Выполняем разбор фрейма PATH_CHALLENGE
				status = frame::parser::path(data + offset, size - offset, type, this->_pathData, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK)
					// Устанавливаем флаг необходимости отправки фрейма PATH_RESPONSE
					this->_pathResponse = true;
				// Устанавливаем флаг приёма ack-eliciting фрейма
				elicit = true;
			} break;
			// Фрейм ответа на проверку достижимости пути PATH_RESPONSE
			case frame_t::PATH_RESPONSE: {
				// Данные проверки пути
				uint8_t buffer[proto::PATH_DATA_SIZE];
				// Выполняем разбор фрейма PATH_RESPONSE (проверка пути - следующий этап)
				status = frame::parser::path(data + offset, size - offset, type, buffer, consumed, error);
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
						this->_error = error_t::APPLICATION_ERROR;
					// Если удалённый эндпоинт сообщил об ошибке транспорта
					else this->_error = static_cast <error_t> (frame.code);
					// Устанавливаем состояние завершения соединения удалённым эндпоинтом
					this->_state = state_t::DRAINING;
				}
			} break;
			// Фрейм подтверждения завершения хендшейка HANDSHAKE_DONE
			case frame_t::HANDSHAKE_DONE: {
				// Выполняем разбор фрейма HANDSHAKE_DONE
				status = frame::parser::handshakeDone(data + offset, size - offset, consumed, error);
				// Если фрейм разобран успешно
				if(status == status_t::OK){
					// Если фрейм прислал клиент (RFC 9000 §19.20)
					if(this->_endpoint == endpoint_t::SERVER){
						// Ставим завершение соединения с нарушением протокола в очередь
						this->fail(error_t::PROTOCOL_VIOLATION);
						// Выводим отрицательный результат
						return status_t::ERROR;
					}
					// Устанавливаем флаг подтверждения хендшейка (RFC 9001 §4.1.2)
					this->_confirmed = true;
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
	if(elicit)
		// Устанавливаем флаг необходимости отправки подтверждения
		this->_spaces[static_cast <size_t> (this->space(level))].ackElicited = true;
	// Выводим положительный результат
	return status_t::OK;
}
/**
 * @brief Метод сборки нагрузки очередного пакета уровня шифрования
 *
 * @param level  уровень шифрования пакета
 * @param budget доступно октетов в датаграмме для нагрузки
 * @param output собранная нагрузка пакета (фреймы)
 * @param meta   учётная запись пакета для восстановления потерь
 * @param elicit флаг наличия ack-eliciting фреймов в нагрузке
 * @return       результат сборки (true - нагрузка не пустая)
 */
bool awh::quic::Connection::payload(const level_t level, const size_t budget, string & output, sent_t & meta, bool & elicit) noexcept {
	// Получаем состояние пространства номеров пакетов
	auto & item = this->_spaces[static_cast <size_t> (this->space(level))];
	// Если требуется отправка подтверждения и есть принятые пакеты
	if(item.ackElicited && !item.ranges.empty()){
		// Формируем фрейм подтверждения приёма пакетов
		frame::ack_t frame;
		// Устанавливаем нулевую задержку подтверждения (подтверждения отправляются немедленно)
		frame.delay = 0;
		// Устанавливаем диапазоны принятых номеров пакетов
		frame.ranges = item.ranges;
		// Выполняем сборку фрейма ACK
		frame::serialize::ack(output, frame);
		// Сбрасываем флаг необходимости отправки подтверждения
		item.ackElicited = false;
	}
	/**
	 *  Пока есть CRYPTO-данные для ретрансмиссии и в датаграмме осталось место
	 */
	while(!item.rtxQueue.empty() && (budget > (output.size() + 16))){
		// Получаем первый блок данных очереди ретрансмиссии
		auto & front = item.rtxQueue.front();
		// Вычисляем доступный размер данных CRYPTO-фрейма
		const size_t chunk = ::min(front.second.size(), budget - output.size() - 16);
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
	if(!item.txBuffer.empty() && (budget > (output.size() + 16))){
		// Вычисляем доступный размер данных CRYPTO-фрейма
		const size_t chunk = ::min(item.txBuffer.size(), budget - output.size() - 16);
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
	// Если собирается нагрузка уровня приложения
	if(level == level_t::APPLICATION){
		// Если требуется отправка фрейма HANDSHAKE_DONE (только сервер)
		if(this->_handshakeDone){
			// Выполняем сборку фрейма HANDSHAKE_DONE
			frame::serialize::handshakeDone(output);
			// Запоминаем фрейм HANDSHAKE_DONE в учётной записи пакета
			meta.handshakeDone = true;
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки фрейма HANDSHAKE_DONE
			this->_handshakeDone = false;
		}
		// Если требуется отправка фрейма PATH_RESPONSE
		if(this->_pathResponse){
			// Выполняем сборку фрейма PATH_RESPONSE
			frame::serialize::path(output, frame_t::PATH_RESPONSE, this->_pathData);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки фрейма PATH_RESPONSE
			this->_pathResponse = false;
		}
		// Если требуется отправка обновлённого лимита данных соединения MAX_DATA
		if(this->_rxMaxDataQueued && (budget > (output.size() + 16))){
			// Выполняем сборку фрейма MAX_DATA (RFC 9000 §19.9)
			frame::serialize::single(output, frame_t::MAX_DATA, this->_rxMaxData);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::MAX_DATA, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки обновлённого лимита
			this->_rxMaxDataQueued = false;
		}
		// Если требуется отправка обновлённого лимита двунаправленных потоков MAX_STREAMS
		if(this->_maxBidiQueued && (budget > (output.size() + 16))){
			// Выполняем сборку фрейма MAX_STREAMS (RFC 9000 §19.11)
			frame::serialize::single(output, frame_t::MAX_STREAMS_BIDI, this->_maxBidiLocal);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::MAX_STREAMS_BIDI, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки обновлённого лимита
			this->_maxBidiQueued = false;
		}
		// Если требуется отправка обновлённого лимита однонаправленных потоков MAX_STREAMS
		if(this->_maxUniQueued && (budget > (output.size() + 16))){
			// Выполняем сборку фрейма MAX_STREAMS (RFC 9000 §19.11)
			frame::serialize::single(output, frame_t::MAX_STREAMS_UNI, this->_maxUniLocal);
			// Запоминаем управляющий фрейм в учётной записи пакета
			meta.control.emplace_back(frame_t::MAX_STREAMS_UNI, 0);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг необходимости отправки обновлённого лимита
			this->_maxUniQueued = false;
		}
		// Если отправка данных соединения заблокирована лимитом удалённого эндпоинта
		if(this->_txDataBlocked && (budget > (output.size() + 16))){
			// Выполняем сборку фрейма DATA_BLOCKED (RFC 9000 §19.12)
			frame::serialize::single(output, frame_t::DATA_BLOCKED, this->_txMaxData);
			// Устанавливаем флаг наличия ack-eliciting фреймов
			elicit = true;
			// Сбрасываем флаг заблокированной отправки данных соединения
			this->_txDataBlocked = false;
		}
		/**
		 * Перебираем список потоков приложения (управляющие фреймы потоков)
		 */
		for(auto & entry : this->_streams){
			// Если в датаграмме не осталось места на управляющий фрейм
			if(budget <= (output.size() + 24))
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
			if(stream.txReset && (budget > (output.size() + 24))){
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
			if(stream.stopQueued && (budget > (output.size() + 24))){
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
			if(stream.txBlocked && (budget > (output.size() + 24))){
				// Выполняем сборку фрейма STREAM_DATA_BLOCKED (RFC 9000 §19.13)
				frame::serialize::pair(output, frame_t::STREAM_DATA_BLOCKED, entry.first, stream.txMax);
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Сбрасываем флаг заблокированной отправки данных потока
				stream.txBlocked = false;
			}
		}
		/**
		 * Пока есть блоки данных потоков для ретрансмиссии и в датаграмме осталось место
		 */
		while(!this->_streamRtx.empty() && (budget > (output.size() + 24))){
			// Получаем первый блок данных очереди ретрансмиссии
			auto & front = this->_streamRtx.front();
			// Ищем поток по идентификатору
			auto i = this->_streams.find(front.sid);
			// Если поток сброшен - ретрансмиссия данных не требуется (RFC 9000 §3.1)
			if((i == this->_streams.end()) || i->second.txReset || i->second.txResetSent){
				// Удаляем блок данных из очереди ретрансмиссии
				this->_streamRtx.erase(this->_streamRtx.begin());
				// Продолжаем обработку очереди
				continue;
			}
			// Вычисляем доступный размер данных фрейма STREAM
			const size_t chunk = ::min(front.data.size(), budget - output.size() - 24);
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
				this->_streamRtx.erase(this->_streamRtx.begin());
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
		 * Перебираем список потоков приложения (данные потоков)
		 */
		for(auto & entry : this->_streams){
			// Если в датаграмме не осталось места на фрейм STREAM
			if(budget <= (output.size() + 24))
				// Прекращаем упаковку данных потоков
				break;
			// Получаем состояние потока
			auto & stream = entry.second;
			// Если отправка потока аварийно завершена - данные не отправляются
			if(stream.txReset || stream.txResetSent)
				// Переходим к следующему потоку
				continue;
			// Если есть данные для отправки
			if(!stream.txBuffer.empty()){
				// Вычисляем доступное окно flow control потока
				const uint64_t streamWindow = ((stream.txMax > stream.txOffset) ? (stream.txMax - stream.txOffset) : 0);
				// Вычисляем доступное окно flow control соединения
				const uint64_t connWindow = ((this->_txMaxData > this->_txData) ? (this->_txMaxData - this->_txData) : 0);
				// Вычисляем доступный размер данных фрейма STREAM
				const size_t chunk = static_cast <size_t> (::min(::min(static_cast <uint64_t> (stream.txBuffer.size()), ::min(streamWindow, connWindow)), static_cast <uint64_t> (budget - output.size() - 24)));
				// Если данные заблокированы лимитом потока (RFC 9000 §19.13)
				if((chunk == 0) && (streamWindow == 0)){
					// Устанавливаем флаг заблокированной отправки данных потока
					stream.txBlocked = true;
					// Переходим к следующему потоку
					continue;
				}
				// Если данные заблокированы лимитом соединения (RFC 9000 §19.12)
				if((chunk == 0) && (connWindow == 0)){
					// Устанавливаем флаг заблокированной отправки данных соединения
					this->_txDataBlocked = true;
					// Прекращаем упаковку данных потоков
					break;
				}
				// Если место в датаграмме исчерпано
				if(chunk == 0)
					// Прекращаем упаковку данных потоков
					break;
				// Определяем флаг завершения потока для фрагмента
				const bool fin = (stream.txFin && (chunk == stream.txBuffer.size()));
				// Выполняем сборку фрейма STREAM (RFC 9000 §19.8)
				frame::serialize::stream(output, entry.first, stream.txOffset, string_view(stream.txBuffer.data(), chunk), fin);
				// Формируем блок данных для учётной записи пакета
				chunk_t sent;
				// Устанавливаем идентификатор потока
				sent.sid = entry.first;
				// Устанавливаем смещение данных в потоке
				sent.offset = stream.txOffset;
				// Устанавливаем флаг завершения потока
				sent.fin = fin;
				// Устанавливаем данные блока
				sent.data = stream.txBuffer.substr(0, chunk);
				// Запоминаем отправленный блок данных в учётной записи пакета
				meta.stream.push_back(::move(sent));
				// Устанавливаем флаг наличия ack-eliciting фреймов
				elicit = true;
				// Продвигаем смещение отправленных данных потока
				stream.txOffset += chunk;
				// Учитываем отправленные данные в flow control соединения
				this->_txData += chunk;
				// Удаляем упакованные данные из буфера
				stream.txBuffer.erase(0, chunk);
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
	if(item.pingQueued){
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
 * @return       размер заголовка пакета в октетах
 */
size_t awh::quic::Connection::headerSize(const level_t level, const uint64_t length, const size_t pnSize) const noexcept {
	// Если пакет уровня приложения (короткий заголовок)
	if(level == level_t::APPLICATION)
		// Выводим размер короткого заголовка: первый октет + DCID + номер пакета
		return (1 + this->_dcid.size + pnSize);
	// Размер длинного заголовка: первый октет + версия + длины и данные идентификаторов
	size_t result = (1 + 4 + 1 + this->_dcid.size + 1 + this->_scid.size);
	// Если пакет уровня Initial
	if(level == level_t::INITIAL)
		// Дописываем размер поля длины токена (токен пустой - Retry следующий этап)
		result += varint::size(0);
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
 * @return        результат сборки (false - ошибка криптографической библиотеки)
 */
bool awh::quic::Connection::seal(string & output, const level_t level, string_view payload) noexcept {
	// Получаем ключи защиты исходящих пакетов уровня
	const crypto::keys_t * keys = this->_handshake.encryption(level);
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
	// Собираемый заголовок пакета
	string header = "";
	/**
	 * Определяем уровень шифрования пакета
	 */
	switch(level){
		// Пакет уровня Initial
		case level_t::INITIAL: {
			// Выполняем сборку длинного заголовка пакета Initial
			if(!packet::serialize::longHeader(header, packet_t::INITIAL, proto::VERSION_1, this->_dcid, this->_scid, "", length, pn, pnSize))
				// Выводим отрицательный результат
				return false;
		} break;
		// Пакет уровня Handshake
		case level_t::HANDSHAKE: {
			// Выполняем сборку длинного заголовка пакета Handshake
			if(!packet::serialize::longHeader(header, packet_t::HANDSHAKE, proto::VERSION_1, this->_dcid, this->_scid, "", length, pn, pnSize))
				// Выводим отрицательный результат
				return false;
		} break;
		// Пакет уровня приложения (1-RTT)
		case level_t::APPLICATION: {
			// Выполняем сборку короткого заголовка пакета 1-RTT
			if(!packet::serialize::shortHeader(header, this->_dcid, pn, pnSize, false, false))
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
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод установки списка поддерживаемых ALPN-протоколов
 *
 * @param protocols список поддерживаемых ALPN-протоколов (например "h3")
 */
void awh::quic::Connection::alpn(const vector <string> & protocols) noexcept {
	// Устанавливаем список поддерживаемых ALPN-протоколов хендшейк-машине
	this->_handshake.alpn(protocols);
}
/**
 * @brief Метод извлечения согласованного ALPN-протокола
 *
 * @return согласованный ALPN-протокол (пусто - согласование не выполнено)
 */
string awh::quic::Connection::alpn() const noexcept {
	// Выводим согласованный ALPN-протокол хендшейк-машины
	return this->_handshake.alpn();
}
/**
 * @brief Метод установки доменного имени удалённого сервера (SNI)
 *
 * @param sni доменное имя удалённого сервера
 */
void awh::quic::Connection::serverNameIndication(string_view sni) noexcept {
	// Устанавливаем доменное имя удалённого сервера хендшейк-машине
	this->_handshake.serverNameIndication(sni);
}
/**
 * @brief Метод установки сертификата и приватного ключа локального узла
 *
 * @param certificate сертификат в формате PEM
 * @param privateKey  приватный ключ в формате PEM
 */
void awh::quic::Connection::certificate(string_view certificate, string_view privateKey) noexcept {
	// Устанавливаем сертификат и приватный ключ хендшейк-машине
	this->_handshake.certificate(certificate, privateKey);
}
/**
 * @brief Метод установки проверки сертификата удалённого узла
 *
 * @param mode режим проверки сертификата удалённого узла
 */
void awh::quic::Connection::verify(const bool mode) noexcept {
	// Устанавливаем флаг проверки сертификата хендшейк-машине
	this->_handshake.verify(mode);
}
/**
 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 */
void awh::quic::Connection::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры
	this->_params = params;
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
	return this->_handshake.peer(params, error);
}
/**
 * @brief Метод начала соединения клиентом
 *
 * @return результат начала соединения (OK/ERROR)
 */
awh::quic::status_t awh::quic::Connection::connect() noexcept {
	// Если эндпоинт не является клиентом либо соединение уже начато
	if((this->_endpoint != endpoint_t::CLIENT) || (this->_state != state_t::NONE))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Выполняем генерацию идентификатора соединения локального эндпоинта
	if(!::makeCid(this->_scid))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Выполняем генерацию идентификатора соединения удалённого эндпоинта
	if(!::makeCid(this->_dcid))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Выполняем вывод ключей уровня Initial из DCID (RFC 9001 §5.2)
	if(!this->_handshake.initial(this->_dcid))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Устанавливаем флаг наличия SCID первого пакета эндпоинта
	this->_params.hasInitialScid = true;
	// Устанавливаем SCID первого пакета эндпоинта (RFC 9000 §7.3)
	this->_params.initialScid = this->_scid;
	// Устанавливаем локальные транспортные параметры хендшейк-машине
	if(!this->_handshake.params(this->_params))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Выполняем начало хендшейка (формируется ClientHello)
	if(this->_handshake.start() != status_t::OK)
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Перекладываем исходящие CRYPTO-данные в буферы пространств
	this->pull();
	// Устанавливаем состояние выполнения хендшейка
	this->_state = state_t::HANDSHAKING;
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
awh::quic::status_t awh::quic::Connection::read(const uint8_t * data, const size_t size, const uint64_t now) noexcept {
	// Если данные датаграммы отсутствуют
	if((data == nullptr) || (size == 0))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Обновляем текущее время последнего вызова
	this->_now = now;
	// Если клиент не начал соединение
	if((this->_endpoint == endpoint_t::CLIENT) && (this->_state == state_t::NONE))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Если удалённый эндпоинт завершил соединение
	if(this->_state == state_t::DRAINING)
		// Выводим положительный результат - датаграммы игнорируются
		return status_t::OK;
	// Копируем датаграмму в изменяемый буфер (снятие защиты выполняется на месте)
	string buffer(reinterpret_cast <const char *> (data), size);
	// Смещение очередного пакета в датаграмме
	size_t offset = 0;
	/**
	 *  Перебираем коалесцированные пакеты датаграммы (RFC 9000 §12.2)
	 */
	while(offset < buffer.size()){
		// Разобранный заголовок пакета
		packet::header_t header;
		// Код ошибки транспорта разбора заголовка
		error_t error = error_t::NO_ERROR;
		// Выполняем разбор заголовка очередного пакета
		if(packet::parser::header(reinterpret_cast <const uint8_t *> (buffer.data()) + offset, buffer.size() - offset, this->_scid.size, header, error) != status_t::OK)
			// Прекращаем разбор датаграммы - остаток отбрасывается (RFC 9000 §12.2)
			break;
		// Если размер пакета не определён либо выходит за пределы датаграммы
		if((header.size == 0) || (header.size > (buffer.size() - offset)))
			// Прекращаем разбор датаграммы
			break;
		// Если принят пакет Version Negotiation либо Retry (обработка - следующий этап)
		if((header.type == packet_t::VERSION_NEGOTIATION) || (header.type == packet_t::RETRY))
			// Прекращаем разбор датаграммы
			break;
		// Если версия пакета с длинным заголовком не поддерживается
		if((header.type != packet_t::ONE_RTT) && (header.version != proto::VERSION_1)){
			// Пропускаем пакет неподдерживаемой версии
			offset += header.size;
			// Продолжаем разбор датаграммы
			continue;
		}
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
		// Если сервер принимает первый пакет соединения
		if((this->_endpoint == endpoint_t::SERVER) && (this->_state == state_t::NONE)){
			// Если первый пакет не является пакетом Initial либо DCID короче минимума (RFC 9000 §7.2)
			if((header.type != packet_t::INITIAL) || (header.dcid.size < MIN_INITIAL_DCID))
				// Прекращаем разбор датаграммы
				break;
			// Выполняем генерацию идентификатора соединения локального эндпоинта
			if(!::makeCid(this->_scid))
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Устанавливаем идентификатор соединения удалённого эндпоинта
			this->_dcid = header.scid;
			// Выполняем вывод ключей уровня Initial из DCID клиента (RFC 9001 §5.2)
			if(!this->_handshake.initial(header.dcid))
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Устанавливаем флаг наличия SCID первого пакета эндпоинта
			this->_params.hasInitialScid = true;
			// Устанавливаем SCID первого пакета эндпоинта (RFC 9000 §7.3)
			this->_params.initialScid = this->_scid;
			// Устанавливаем флаг наличия исходного DCID первого пакета Initial клиента
			this->_params.hasOdcid = true;
			// Устанавливаем исходный DCID первого пакета Initial клиента (RFC 9000 §7.3)
			this->_params.odcid = header.dcid;
			// Устанавливаем локальные транспортные параметры хендшейк-машине
			if(!this->_handshake.params(this->_params))
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Выполняем начало хендшейка (сервер ожидает ClientHello)
			if(this->_handshake.start() != status_t::OK)
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Устанавливаем состояние выполнения хендшейка
			this->_state = state_t::HANDSHAKING;
		}
		// Получаем ключи снятия защиты входящих пакетов уровня
		const crypto::keys_t * keys = this->_handshake.decryption(level);
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
		// Расшифрованная нагрузка пакета
		string plain = "";
		// Код ошибки транспорта снятия защиты
		error_t oerror = error_t::NO_ERROR;
		// Выполняем снятие защиты пакета: защита заголовка и AEAD-расшифровка
		if(crypto::open(reinterpret_cast <uint8_t *> (buffer.data()) + offset, header.size, header.pnOffset, (item.hasRx ? item.largestRx : 0), * keys, pn, plain, oerror) != status_t::OK){
			// Пропускаем повреждённый либо чужой пакет (RFC 9000 §12.2)
			offset += header.size;
			// Продолжаем разбор датаграммы
			continue;
		}
		// Если пакет с таким номером уже был принят
		if(this->duplicate(this->space(level), pn)){
			// Пропускаем дубликат пакета
			offset += header.size;
			// Продолжаем разбор датаграммы
			continue;
		}
		// Если клиент принял первый ответ сервера с длинным заголовком
		if((this->_endpoint == endpoint_t::CLIENT) && !this->_dcidUpdated && (header.type != packet_t::ONE_RTT)){
			// Обновляем идентификатор соединения удалённого эндпоинта (RFC 9000 §7.2)
			this->_dcid = header.scid;
			// Устанавливаем флаг обновления DCID по первому ответу сервера
			this->_dcidUpdated = true;
		}
		// Регистрируем принятый номер пакета в диапазонах пространства
		this->record(this->space(level), pn);
		// Выполняем разбор и диспетчеризацию фреймов нагрузки пакета
		if(this->frames(level, reinterpret_cast <const uint8_t *> (plain.data()), plain.size()) != status_t::OK)
			// Выводим отрицательный результат
			return status_t::ERROR;
		// Если сервер успешно обработал первый пакет Handshake
		if((this->_endpoint == endpoint_t::SERVER) && (level == level_t::HANDSHAKE) && (this->_handshake.decryption(level_t::INITIAL) != nullptr))
			// Сбрасываем ключи уровня Initial (RFC 9001 §4.9.1)
			this->discard(level_t::INITIAL);
		/**
		 * Если хендшейк успешно завершён - применяем параметры сразу,
		 * не дожидаясь конца датаграммы: следующие коалесцированные
		 * пакеты 1-RTT могут содержать фреймы STREAM (RFC 9000 §12.2)
		 */
		if((this->_state == state_t::HANDSHAKING) && (this->_handshake.state() == handshake_t::state_t::COMPLETED)){
			// Выполняем применение транспортных параметров удалённого эндпоинта
			if(!this->established())
				// Выводим отрицательный результат
				return status_t::ERROR;
			// Устанавливаем состояние установленного соединения
			this->_state = state_t::CONNECTED;
			// Если локальный эндпоинт является сервером
			if(this->_endpoint == endpoint_t::SERVER){
				// Устанавливаем флаг необходимости отправки фрейма HANDSHAKE_DONE (RFC 9000 §19.20)
				this->_handshakeDone = true;
				// Устанавливаем флаг подтверждения хендшейка (RFC 9001 §4.1.2)
				this->_confirmed = true;
				// Сбрасываем ключи уровня Handshake (RFC 9001 §4.9.2)
				this->discard(level_t::HANDSHAKE);
			}
		}
		// Переходим к следующему пакету датаграммы
		offset += header.size;
	}
	// Если хендшейк завершился ошибкой и завершение соединения ещё не в очереди
	if((this->_handshake.state() == handshake_t::state_t::FAILED) && !this->_closeQueued)
		// Ставим завершение соединения с ошибкой хендшейка в очередь
		this->fail(this->_handshake.error());
	// Перекладываем исходящие CRYPTO-данные в буферы пространств
	this->pull();
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
	// Если соединение не начато
	if(this->_state == state_t::NONE)
		// Выводим отрицательный результат
		return false;
	// Обновляем текущее время последнего вызова
	this->_now = now;
	// Перекладываем исходящие CRYPTO-данные в буферы пространств
	this->pull();
	// Если завершение соединения поставлено в очередь и ещё не отправлено
	if(this->_closeQueued && !this->_closeSent){
		// Список уровней шифрования в порядке убывания предпочтения
		static const level_t levels[] = {level_t::APPLICATION, level_t::HANDSHAKE, level_t::INITIAL};
		/**
		 * Перебираем список уровней шифрования
		 */
		for(auto & level : levels){
			// Если ключи защиты исходящих пакетов уровня выведены
			if(this->_handshake.encryption(level) != nullptr){
				// Нагрузка пакета завершения соединения
				string payload = "";
				// Если ошибка приложения отправляется в пакете Initial или Handshake (RFC 9000 §10.2.3)
				if(this->_closeApp && (level != level_t::APPLICATION))
					// Выполняем сборку фрейма CONNECTION_CLOSE с кодом APPLICATION_ERROR без причины
					frame::serialize::connectionClose(payload, static_cast <uint64_t> (error_t::APPLICATION_ERROR), 0, "", false);
				// Если завершение отправляется штатно
				else
					// Выполняем сборку фрейма CONNECTION_CLOSE (RFC 9000 §10.2.1)
					frame::serialize::connectionClose(payload, this->_closeCode, 0, this->_closeReason, this->_closeApp);
				// Если нагрузка меньше минимума для выборки защиты заголовка
				if(payload.size() < MIN_PAYLOAD_SIZE)
					// Дополняем нагрузку фреймами PADDING
					frame::serialize::padding(payload, MIN_PAYLOAD_SIZE - payload.size());
				// Выполняем сборку и защиту пакета завершения соединения
				if(!this->seal(output, level, payload))
					// Выводим отрицательный результат
					return false;
				// Устанавливаем флаг выполненной отправки фрейма CONNECTION_CLOSE
				this->_closeSent = true;
				// Выводим положительный результат
				return true;
			}
		}
		// Выводим отрицательный результат - ключи недоступны
		return false;
	}
	// Если соединение завершается либо завершено удалённым эндпоинтом
	if((this->_state == state_t::CLOSING) || (this->_state == state_t::DRAINING))
		// Выводим отрицательный результат
		return false;
	// Список уровней шифрования в порядке возрастания (RFC 9000 §12.2)
	static const level_t levels[] = {level_t::INITIAL, level_t::HANDSHAKE, level_t::APPLICATION};
	/**
	 * @brief Структура собранного пакета датаграммы
	 *
	 */
	typedef struct Spec {
		// Уровень шифрования пакета
		level_t level;
		// Нагрузка пакета (фреймы)
		string payload;
		// Учётная запись пакета для восстановления потерь
		sent_t meta;
		// Флаг наличия ack-eliciting фреймов в нагрузке
		bool elicit;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Spec() noexcept : level(level_t::INITIAL), payload{""}, elicit(false) {}
	} spec_t;
	// Список собранных нагрузок пакетов датаграммы
	vector <spec_t> specs;
	// Оценка занятого размера датаграммы
	size_t used = 0;
	/**
	 * Перебираем список уровней шифрования
	 */
	for(auto & level : levels){
		// Если ключи защиты исходящих пакетов уровня не выведены
		if(this->_handshake.encryption(level) == nullptr)
			// Переходим к следующему уровню
			continue;
		// Если в датаграмме не осталось места на пакет уровня
		if((used + OVERHEAD_RESERVE) >= MAX_DATAGRAM_SIZE)
			// Прекращаем сборку датаграммы
			break;
		// Собираемый пакет уровня
		spec_t spec;
		// Устанавливаем уровень шифрования пакета
		spec.level = level;
		// Выполняем сборку нагрузки пакета уровня
		if(!this->payload(level, MAX_DATAGRAM_SIZE - used - OVERHEAD_RESERVE, spec.payload, spec.meta, spec.elicit))
			// Переходим к следующему уровню
			continue;
		// Учитываем оценку размера пакета: заголовок + нагрузка + тег AEAD
		used += (this->headerSize(level, spec.payload.size() + 4 + crypto::AEAD_TAG_SIZE, 4) + spec.payload.size() + crypto::AEAD_TAG_SIZE);
		// Добавляем нагрузку пакета в список сборки
		specs.push_back(::move(spec));
	}
	// Если нагрузок для отправки нет
	if(specs.empty())
		// Выводим отрицательный результат
		return false;
	// Флаг наличия пакета Initial в датаграмме
	bool initial = false;
	/**
	 * Перебираем список собранных нагрузок
	 */
	for(auto & spec : specs)
		// Определяем наличие пакета Initial в датаграмме
		initial = (initial || (spec.level == level_t::INITIAL));
	// Если датаграмма содержит пакет Initial (RFC 9000 §14.1)
	if(initial){
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
				// Вычисляем размер кодирования номера пакета
				const size_t pnSize = packet::packetNumberSize(item.txPn, (item.hasAcked ? item.largestAcked : item.txPn));
				// Суммируем точный размер пакета: заголовок + номер + нагрузка + тег AEAD
				total += (this->headerSize(spec.level, pnSize + spec.payload.size() + crypto::AEAD_TAG_SIZE, pnSize) + spec.payload.size() + crypto::AEAD_TAG_SIZE);
			}
			// Если датаграмма меньше минимального размера
			if(total < proto::MIN_INITIAL_SIZE){
				// Учитываем количество добавленных октетов PADDING
				added += (proto::MIN_INITIAL_SIZE - total);
				// Дополняем нагрузку последнего пакета фреймами PADDING
				frame::serialize::padding(specs.back().payload, proto::MIN_INITIAL_SIZE - total);
			// Если датаграмма превысила минимум из-за роста поля Length (varint)
			} else if((total > proto::MIN_INITIAL_SIZE) && (added >= (total - proto::MIN_INITIAL_SIZE))){
				// Учитываем количество удаляемых октетов PADDING
				added -= (total - proto::MIN_INITIAL_SIZE);
				// Удаляем излишек фреймов PADDING из конца нагрузки
				specs.back().payload.erase(specs.back().payload.size() - (total - proto::MIN_INITIAL_SIZE));
			// Если датаграмма достигла минимального размера
			} else
				// Прекращаем дополнение
				break;
		}
	}
	// Флаг наличия пакета Handshake в датаграмме
	bool handshake = false;
	/**
	 * Перебираем список собранных нагрузок
	 */
	for(auto & spec : specs){
		// Определяем наличие пакета Handshake в датаграмме
		handshake = (handshake || (spec.level == level_t::HANDSHAKE));
		// Получаем состояние пространства номеров пакетов
		auto & item = this->_spaces[static_cast <size_t> (this->space(spec.level))];
		// Запоминаем номер отправляемого пакета
		spec.meta.pn = item.txPn;
		// Выполняем сборку и защиту пакета уровня
		if(!this->seal(output, spec.level, spec.payload))
			// Выводим отрицательный результат
			return false;
		// Если пакет содержит ack-eliciting фреймы (RFC 9002 §A.1)
		if(spec.elicit){
			// Устанавливаем время отправки пакета
			spec.meta.time = now;
			// Добавляем учётную запись в список отправленных пакетов
			item.sent.push_back(::move(spec.meta));
			// Устанавливаем время отправки последнего ack-eliciting пакета
			item.lastElicited = now;
			// Устанавливаем флаг наличия отправленных ack-eliciting пакетов
			item.hasElicited = true;
		}
	}
	// Если клиент отправил первый пакет Handshake
	if((this->_endpoint == endpoint_t::CLIENT) && handshake && (this->_handshake.encryption(level_t::INITIAL) != nullptr))
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
	// Если соединение не активно
	if((this->_state != state_t::HANDSHAKING) && (this->_state != state_t::CONNECTED))
		// Выводим нулевой дедлайн - таймер не требуется
		return 0;
	// Дедлайн ближайшего события таймера
	uint64_t result = 0;
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
		const uint64_t pto = this->deadline(static_cast <space_t> (i));
		// Если таймер PTO пространства взведён
		if((pto > 0) && ((result == 0) || (pto < result)))
			// Устанавливаем дедлайн таймера PTO
			result = pto;
	}
	// Выводим дедлайн ближайшего события таймера
	return result;
}
/**
 * @brief Метод обработки просроченных таймеров (RFC 9002 §6.2)
 *
 * @param now текущее время в миллисекундах
 */
void awh::quic::Connection::tick(const uint64_t now) noexcept {
	// Если соединение не активно
	if((this->_state != state_t::HANDSHAKING) && (this->_state != state_t::CONNECTED))
		// Выходим из метода
		return;
	// Обновляем текущее время последнего вызова
	this->_now = now;
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
		if(this->_ptoCount < MAX_PTO_COUNT)
			// Увеличиваем счётчик срабатываний таймера PTO (RFC 9002 §6.2.1)
			this->_ptoCount++;
		// Ставим зондирующие данные пространства в очередь отправки
		this->probe(expired);
	}
}
/**
 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
 *
 * @param code   код ошибки приложения
 * @param reason человекочитаемая причина завершения
 */
void awh::quic::Connection::close(const uint64_t code, string_view reason) noexcept {
	// Если завершение соединения ещё не поставлено в очередь
	if(!this->_closeQueued && (this->_state != state_t::NONE) && (this->_state != state_t::DRAINING)){
		// Устанавливаем код ошибки завершения соединения
		this->_closeCode = code;
		// Устанавливаем флаг ошибки приложения
		this->_closeApp = true;
		// Устанавливаем причину завершения соединения
		this->_closeReason.assign(reason);
		// Устанавливаем флаг постановки завершения соединения в очередь
		this->_closeQueued = true;
		// Устанавливаем состояние завершения соединения
		this->_state = state_t::CLOSING;
	}
}
/**
 * @brief Метод открытия нового потока приложения (RFC 9000 §2.1)
 *
 * @param unidirectional флаг однонаправленного потока
 * @return               идентификатор потока (INVALID_STREAM - открытие невозможно)
 */
uint64_t awh::quic::Connection::open(const bool unidirectional) noexcept {
	// Если соединение не установлено
	if(this->_state != state_t::CONNECTED)
		// Выводим недопустимый идентификатор потока
		return INVALID_STREAM;
	// Получаем счётчик открытых локально потоков
	uint64_t & opened = (unidirectional ? this->_openedUni : this->_openedBidi);
	// Получаем лимит удалённого эндпоинта на локально открываемые потоки
	const uint64_t limit = (unidirectional ? this->_maxUniRemote : this->_maxBidiRemote);
	// Если лимит потоков удалённого эндпоинта исчерпан (RFC 9000 §4.6)
	if(opened >= limit)
		// Выводим недопустимый идентификатор потока
		return INVALID_STREAM;
	// Вычисляем идентификатор нового потока (RFC 9000 §2.1)
	const uint64_t sid = ((opened << 2) | (unidirectional ? 0x02 : 0x00) | ((this->_endpoint == endpoint_t::SERVER) ? 0x01 : 0x00));
	// Увеличиваем счётчик открытых локально потоков
	opened++;
	// Создаём состояние нового потока
	auto ret = this->_streams.emplace(sid, stream_data_t());
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
	// Если соединение не установлено
	if(this->_state != state_t::CONNECTED)
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Если отправка данных в поток локальным эндпоинтом недопустима
	if(!this->sendable(sid))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Ищем поток по идентификатору
	auto i = this->_streams.find(sid);
	// Если поток не найден
	if(i == this->_streams.end())
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
 * @brief Метод получения списка потоков с данными для приложения
 *
 * @return список идентификаторов потоков с собранными данными либо завершением
 */
vector <uint64_t> awh::quic::Connection::readable() const noexcept {
	// Результирующий список идентификаторов потоков
	vector <uint64_t> result;
	/**
	 * Перебираем список потоков приложения
	 */
	for(auto & entry : this->_streams){
		// Если поток сброшен удалённым эндпоинтом - данные выдаче не подлежат
		if(entry.second.rxReset)
			// Переходим к следующему потоку
			continue;
		// Если есть собранные данные либо невыданное завершение потока
		if(!entry.second.rxReady.empty() ||
		   (entry.second.rxFin && !entry.second.rxFinDelivered && (entry.second.rxOffset == entry.second.rxFinal)))
			// Добавляем идентификатор потока в список
			result.push_back(entry.first);
	}
	// Выводим список идентификаторов потоков
	return result;
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
	auto i = this->_streams.find(sid);
	// Если поток не найден либо приём данных недопустим
	if((i == this->_streams.end()) || !this->receivable(sid))
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
		this->_rxConsumed += stream.rxReady.size();
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
	if((this->_rxMaxData - this->_rxConsumed) < (this->_params.initialMaxData / 2)){
		// Продвигаем анонсированный лимит приёма данных соединения
		this->_rxMaxData = (this->_rxConsumed + this->_params.initialMaxData);
		// Устанавливаем флаг необходимости отправки обновлённого лимита MAX_DATA
		this->_rxMaxDataQueued = true;
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
	auto i = this->_streams.find(sid);
	// Если поток не найден либо аварийное завершение уже выполнялось
	if((i == this->_streams.end()) || i->second.txReset || i->second.txResetSent)
		// Выходим из метода
		return;
	// Ставим отправку фрейма RESET_STREAM в очередь
	i->second.txReset = true;
	// Устанавливаем код ошибки приложения фрейма RESET_STREAM
	i->second.txResetCode = code;
	// Отбрасываем неотправленные данные потока
	i->second.txBuffer.clear();
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
	auto i = this->_streams.find(sid);
	// Если поток не найден либо запрос прекращения уже выполнялся
	if((i == this->_streams.end()) || i->second.stopQueued || i->second.stopSent)
		// Выходим из метода
		return;
	// Получаем состояние потока
	auto & stream = i->second;
	// Ставим отправку фрейма STOP_SENDING в очередь
	stream.stopQueued = true;
	// Устанавливаем код ошибки приложения фрейма STOP_SENDING
	stream.stopCode = code;
	// Учитываем отброшенные данные как потреблённые в flow control соединения
	this->_rxConsumed += (stream.rxHigh - (stream.rxOffset - stream.rxReady.size()));
	// Отбрасываем несобранные фрагменты данных потока
	stream.rxBuffer.clear();
	// Отбрасываем собранные данные потока
	stream.rxReady.clear();
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
	auto i = this->_streams.find(sid);
	// Если поток найден и сброшен удалённым эндпоинтом
	if((i != this->_streams.end()) && i->second.rxReset){
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
	return this->_state;
}
/**
 * @brief Метод получения кода ошибки транспорта соединения
 *
 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
 */
awh::quic::error_t awh::quic::Connection::error() const noexcept {
	// Выводим код ошибки транспорта соединения
	return this->_error;
}
/**
 * @brief Метод получения идентификатора соединения локального эндпоинта
 *
 * @return идентификатор соединения локального эндпоинта
 */
const awh::quic::cid_t & awh::quic::Connection::scid() const noexcept {
	// Выводим идентификатор соединения локального эндпоинта
	return this->_scid;
}
/**
 * @brief Метод получения идентификатора соединения удалённого эндпоинта
 *
 * @return идентификатор соединения удалённого эндпоинта
 */
const awh::quic::cid_t & awh::quic::Connection::dcid() const noexcept {
	// Выводим идентификатор соединения удалённого эндпоинта
	return this->_dcid;
}
/**
 * @brief Метод доступа к машине криптографического хендшейка
 *
 * @return машина криптографического хендшейка
 */
const awh::quic::handshake_t & awh::quic::Connection::handshake() const noexcept {
	// Выводим машину криптографического хендшейка
	return this->_handshake;
}
/**
 * @brief Конструктор
 *
 * @param endpoint роль локального эндпоинта на соединении
 */
awh::quic::Connection::Connection(const endpoint_t endpoint) noexcept :
 _endpoint(endpoint), _state(state_t::NONE), _dcidUpdated(false),
 _confirmed(false), _handshakeDone(false), _pathResponse(false), _pathData{0},
 _closeCode(0), _closeApp(false), _closeQueued(false), _closeSent(false),
 _closeReason{""}, _error(error_t::NO_ERROR), _now(0), _latestRtt(0),
 _minRtt(0), _smoothedRtt(0), _rttVar(0), _rttSampled(false), _ptoCount(0),
 _handshake(endpoint), _openedBidi(0), _openedUni(0), _maxBidiRemote(0),
 _maxUniRemote(0), _acceptedBidi(0), _acceptedUni(0), _maxBidiLocal(0),
 _maxUniLocal(0), _maxBidiQueued(false), _maxUniQueued(false), _txData(0),
 _txMaxData(0), _txDataBlocked(false), _rxData(0), _rxConsumed(0),
 _rxMaxData(0), _rxMaxDataQueued(false) {}
