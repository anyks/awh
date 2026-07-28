/**
 * @file: fuzz.cpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Фаззинг-тесты парсеров QUIC — прогон случайно сгенерированных буферов через разбор пакетов,
 *        фреймов и транспортных параметров для поиска аварийных завершений и выходов за границы буфера
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "quic.hpp"
#include "../../../include/proto/quic2/frame.hpp"
#include "../../../include/proto/quic2/params.hpp"
#include "../../../include/proto/quic2/packet.hpp"

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic2;

/**
 * @brief Внутренние вспомогательные средства фаззинга парсеров
 *
 */
namespace {
	/**
	 * @brief Количество итераций фаззинга на один парсер
	 *
	 */
	static constexpr size_t ITERATIONS = 20000;
	/**
	 * @brief Предельный размер генерируемого буфера в октетах
	 *
	 */
	static constexpr size_t MAX_BUFFER = 512;
	/**
	 * @brief Класс детерминированного генератора псевдослучайных чисел
	 *
	 * @details Используется линейный конгруэнтный генератор с фиксированным зерном:
	 *          отказ фаззинга обязан воспроизводиться дословно, поэтому источник
	 *          случайности не должен зависеть от платформы и запуска
	 *
	 */
	class Quic2Random {
		private:
			// Текущее состояние генератора
			uint64_t _state;
		public:
			/**
			 * @brief Метод получения очередного псевдослучайного числа
			 *
			 * @return псевдослучайное число
			 *
			 */
			uint64_t next() noexcept {
				// Продвигаем состояние генератора (константы Кнута для MMIX)
				this->_state = ((this->_state * 6364136223846793005ULL) + 1442695040888963407ULL);
				// Выводим старшие биты состояния как наиболее качественные
				return (this->_state >> 17);
			}
			/**
			 * @brief Метод получения псевдослучайного числа в диапазоне
			 *
			 * @param bound верхняя граница диапазона (не включается)
			 * @return      псевдослучайное число в диапазоне
			 *
			 */
			size_t range(const size_t bound) noexcept {
				// Выводим псевдослучайное число в заданном диапазоне
				return ((bound == 0) ? 0 : static_cast <size_t> (this->next() % bound));
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param seed зерно генератора
			 *
			 */
			explicit Quic2Random(const uint64_t seed) noexcept : _state(seed) {}
	};
	/**
	 * @brief Функция генерации псевдослучайного буфера
	 *
	 * @param random генератор псевдослучайных чисел
	 * @param prefix префикс буфера (октет типа фрейма либо пусто)
	 * @return       сгенерированный буфер
	 *
	 */
	static std::string generate(Quic2Random & random, const std::string & prefix = "") noexcept {
		// Результирующий буфер
		std::string result = prefix;
		// Определяем размер случайной части буфера
		const size_t length = random.range(MAX_BUFFER);
		// Резервируем память под буфер
		result.reserve(result.size() + length);
		/**
		 * Заполняем буфер псевдослучайными октетами
		 */
		for(size_t i = 0; i < length; i++)
			// Дописываем очередной октет буфера
			result.push_back(static_cast <char> (random.next() & 0xFF));
		// Выводим сгенерированный буфер
		return result;
	}
};

/**
 * @brief Тест устойчивости парсеров фреймов к произвольным данным (RFC 9000 §19)
 *
 * @details Парсер обязан либо отвергнуть буфер, либо сообщить количество
 *          потреблённых октетов в пределах буфера. Превышение размера буфера
 *          увело бы разбор нагрузки пакета за её границу
 *
 */
TEST_F(Quic2Fixture, FuzzFrameParsersTest){
	// Генератор псевдослучайных чисел с фиксированным зерном
	Quic2Random random(0x5157080000449eULL);
	/**
	 * Перебираем итерации фаззинга
	 */
	for(size_t i = 0; i < ITERATIONS; i++){
		// Генерируем псевдослучайный буфер
		const std::string buffer = ::generate(random);
		// Буфер разбираемых данных
		const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer.data());
		// Размер разбираемых данных
		const size_t size = buffer.size();
		// Тип очередного фрейма
		frame_t type = frame_t::UNKNOWN;
		// Выполняем определение типа фрейма
		frame::parser::type(data, size, type);
		// Количество потреблённых октетов
		size_t consumed = 0;
		// Код ошибки транспорта
		error_t error = error_t::NO_ERROR;
		// Разобранный фрейм подтверждения приёма пакетов
		frame::ack_t ack;
		// Выполняем разбор фрейма ACK
		if(frame::parser::ack(data, size, ack, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
			// Проверяем что количество диапазонов ограничено
			ASSERT_LE(ack.ranges.size(), frame::MAX_ACK_RANGES);
			// Проверяем что список диапазонов не пуст при успешном разборе
			ASSERT_FALSE(ack.ranges.empty());
			/**
			 * Перебираем диапазоны подтверждённых номеров пакетов
			 */
			for(auto & range : ack.ranges)
				// Проверяем корректность границ диапазона
				ASSERT_LE(range.low, range.high);
		}
		// Разобранный фрейм данных потока приложения
		frame::stream_t stream;
		// Выполняем разбор фрейма STREAM
		if(frame::parser::stream(data, size, stream, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
			// Проверяем что данные потока не выходят за границу буфера
			ASSERT_LE(stream.data.size(), size);
		}
		// Разобранный фрейм криптографического хендшейка
		frame::crypto_t crypto;
		// Выполняем разбор фрейма CRYPTO
		if(frame::parser::crypto(data, size, crypto, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
			// Проверяем что данные хендшейка не выходят за границу буфера
			ASSERT_LE(crypto.data.size(), size);
		}
		// Разобранный фрейм анонса идентификатора соединения
		frame::new_connection_id_t cid;
		// Выполняем разбор фрейма NEW_CONNECTION_ID
		if(frame::parser::newConnectionId(data, size, cid, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
			// Проверяем что длина идентификатора соединения в допустимом диапазоне
			ASSERT_GE(cid.cid.size, 1u);
			ASSERT_LE(cid.cid.size, proto::MAX_CID_SIZE);
		}
		// Разобранный фрейм завершения соединения
		frame::connection_close_t close;
		// Выполняем разбор фрейма CONNECTION_CLOSE
		if(frame::parser::connectionClose(data, size, close, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
			// Проверяем что причина завершения не выходит за границу буфера
			ASSERT_LE(close.reason.size(), size);
		}
		// Разобранный фрейм аварийного завершения потока
		frame::reset_stream_t reset;
		// Выполняем разбор фрейма RESET_STREAM
		if(frame::parser::resetStream(data, size, reset, consumed, error) == status_t::OK)
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
		// Разобранный фрейм запроса прекращения передачи
		frame::stop_sending_t stop;
		// Выполняем разбор фрейма STOP_SENDING
		if(frame::parser::stopSending(data, size, stop, consumed, error) == status_t::OK)
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
		// Токен для будущих соединений
		std::string_view token;
		// Выполняем разбор фрейма NEW_TOKEN
		if(frame::parser::newToken(data, size, token, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
			// Проверяем что токен не пуст и не выходит за границу буфера
			ASSERT_FALSE(token.empty());
			ASSERT_LE(token.size(), size);
		}
		// Данные проверки достижимости пути
		uint8_t path[proto::PATH_DATA_SIZE];
		// Выполняем разбор фрейма PATH_CHALLENGE
		if(frame::parser::path(data, size, frame_t::PATH_CHALLENGE, path, consumed, error) == status_t::OK)
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
		// Значение целочисленного поля фрейма
		uint64_t value = 0;
		// Выполняем разбор фрейма с одним целочисленным полем
		if(frame::parser::single(data, size, frame_t::MAX_DATA, value, consumed, error) == status_t::OK)
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
		// Идентификатор потока фрейма
		uint64_t sid = 0;
		// Выполняем разбор фрейма с идентификатором потока и лимитом
		if(frame::parser::pair(data, size, frame_t::MAX_STREAM_DATA, sid, value, consumed, error) == status_t::OK)
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
		// Выполняем разбор серии фреймов PADDING
		if(frame::parser::padding(data, size, consumed, error) == status_t::OK)
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, size);
	}
}

/**
 * @brief Тест устойчивости парсеров фреймов к данным с корректным типом (RFC 9000 §19)
 *
 * @details Случайный буфер отвергается уже на октете типа фрейма, поэтому тело
 *          фрейма остаётся непройденным. Подстановка корректного типа заводит
 *          разбор вглубь фрейма, где и проверяются границы полей
 *
 */
TEST_F(Quic2Fixture, FuzzFrameBodiesTest){
	// Генератор псевдослучайных чисел с фиксированным зерном
	Quic2Random random(0x8394c8f03e5157ULL);
	// Количество успешных разборов фреймов
	size_t accepted = 0;
	/**
	 * Перебираем итерации фаззинга
	 */
	for(size_t i = 0; i < ITERATIONS; i++){
		// Количество потреблённых октетов
		size_t consumed = 0;
		// Код ошибки транспорта
		error_t error = error_t::NO_ERROR;
		// Генерируем буфер фрейма ACK с корректным октетом типа
		const std::string ackBuffer = ::generate(random, std::string(1, static_cast <char> (frame_t::ACK)));
		// Разобранный фрейм подтверждения приёма пакетов
		frame::ack_t ack;
		// Выполняем разбор фрейма ACK
		if(frame::parser::ack(reinterpret_cast <const uint8_t *> (ackBuffer.data()), ackBuffer.size(), ack, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, ackBuffer.size());
			// Проверяем что количество диапазонов ограничено
			ASSERT_LE(ack.ranges.size(), frame::MAX_ACK_RANGES);
			/**
			 * Перебираем диапазоны подтверждённых номеров пакетов
			 */
			for(auto & range : ack.ranges)
				// Проверяем корректность границ диапазона
				ASSERT_LE(range.low, range.high);
		}
		// Генерируем буфер фрейма STREAM с корректным октетом типа и всеми полями
		const std::string streamBuffer = ::generate(random, std::string(1, static_cast <char> (0x0F)));
		// Разобранный фрейм данных потока приложения
		frame::stream_t stream;
		// Выполняем разбор фрейма STREAM
		if(frame::parser::stream(reinterpret_cast <const uint8_t *> (streamBuffer.data()), streamBuffer.size(), stream, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, streamBuffer.size());
			// Проверяем что данные потока лежат в пределах буфера
			ASSERT_LE(stream.data.size(), streamBuffer.size());
			// Проверяем что сумма смещения и длины данных представима целым числом переменной длины
			ASSERT_LE(stream.offset + stream.data.size(), proto::VARINT_MAX);
		}
		// Генерируем буфер фрейма CRYPTO с корректным октетом типа
		const std::string cryptoBuffer = ::generate(random, std::string(1, static_cast <char> (frame_t::CRYPTO)));
		// Разобранный фрейм криптографического хендшейка
		frame::crypto_t crypto;
		// Выполняем разбор фрейма CRYPTO
		if(frame::parser::crypto(reinterpret_cast <const uint8_t *> (cryptoBuffer.data()), cryptoBuffer.size(), crypto, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, cryptoBuffer.size());
			// Проверяем что данные хендшейка лежат в пределах буфера
			ASSERT_LE(crypto.data.size(), cryptoBuffer.size());
		}
		// Генерируем буфер фрейма NEW_CONNECTION_ID с корректным октетом типа
		const std::string cidBuffer = ::generate(random, std::string(1, static_cast <char> (frame_t::NEW_CONNECTION_ID)));
		// Разобранный фрейм анонса идентификатора соединения
		frame::new_connection_id_t cid;
		// Выполняем разбор фрейма NEW_CONNECTION_ID
		if(frame::parser::newConnectionId(reinterpret_cast <const uint8_t *> (cidBuffer.data()), cidBuffer.size(), cid, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, cidBuffer.size());
			// Проверяем что длина идентификатора соединения в допустимом диапазоне
			ASSERT_GE(cid.cid.size, 1u);
			ASSERT_LE(cid.cid.size, proto::MAX_CID_SIZE);
			// Проверяем соотношение порядковых номеров идентификатора
			ASSERT_LE(cid.retirePriorTo, cid.seq);
		}
		// Генерируем буфер фрейма CONNECTION_CLOSE с корректным октетом типа
		const std::string closeBuffer = ::generate(random, std::string(1, static_cast <char> (frame_t::CONNECTION_CLOSE)));
		// Разобранный фрейм завершения соединения
		frame::connection_close_t close;
		// Выполняем разбор фрейма CONNECTION_CLOSE
		if(frame::parser::connectionClose(reinterpret_cast <const uint8_t *> (closeBuffer.data()), closeBuffer.size(), close, consumed, error) == status_t::OK){
			// Проверяем что разбор не вышел за границу буфера
			ASSERT_LE(consumed, closeBuffer.size());
			// Проверяем что причина завершения лежит в пределах буфера
			ASSERT_LE(close.reason.size(), closeBuffer.size());
			// Считаем успешный разбор фрейма
			accepted++;
		}
	}
	/**
	 * Фаззинг обязан доходить до тел фреймов: разбор, отвергающий все входные
	 * данные без исключения, никаких границ полей не проверяет
	 */
	ASSERT_GT(accepted, 0u);
}

/**
 * @brief Тест устойчивости парсера транспортных параметров к произвольным данным (RFC 9000 §18)
 *
 * @details Разбор обязан либо отвергнуть буфер, либо выдать параметры в границах,
 *          заданных протоколом. Проверяются обе роли отправителя: ролевые
 *          ограничения на параметры различаются
 *
 */
TEST_F(Quic2Fixture, FuzzTransportParamsTest){
	// Генератор псевдослучайных чисел с фиксированным зерном
	Quic2Random random(0xf067a5502a4262ULL);
	// Список проверяемых ролей отправителя параметров
	const endpoint_t senders[] = {endpoint_t::CLIENT, endpoint_t::SERVER};
	// Количество успешных разборов транспортных параметров
	size_t accepted = 0;
	/**
	 * Перебираем итерации фаззинга
	 */
	for(size_t i = 0; i < ITERATIONS; i++){
		// Генерируем псевдослучайный буфер
		const std::string buffer = ::generate(random);
		/**
		 * Перебираем список ролей отправителя параметров
		 */
		for(auto & sender : senders){
			// Разобранные транспортные параметры
			params::params_t output;
			// Код ошибки транспорта
			error_t error = error_t::NO_ERROR;
			// Выполняем разбор транспортных параметров
			if(params::parser::decode(reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), sender, output, error) != status_t::OK)
				// Переходим к следующей роли отправителя
				continue;
			// Проверяем нижнюю границу максимального размера UDP-нагрузки (RFC 9000 §18.2)
			ASSERT_GE(output.maxUdpPayloadSize, 1200u);
			// Проверяем верхнюю границу показателя степени задержки подтверждения
			ASSERT_LE(output.ackDelayExponent, 20u);
			// Проверяем верхнюю границу максимальной задержки подтверждения
			ASSERT_LT(output.maxAckDelay, 16384u);
			// Проверяем нижнюю границу лимита активных идентификаторов соединения
			ASSERT_GE(output.activeConnectionIdLimit, 2u);
			// Проверяем верхнюю границу лимитов числа потоков (RFC 9000 §4.6)
			ASSERT_LE(output.initialMaxStreamsBidi, (static_cast <uint64_t> (1) << 60));
			ASSERT_LE(output.initialMaxStreamsUni, (static_cast <uint64_t> (1) << 60));
			// Проверяем длину идентификаторов соединения по лимиту QUIC v1
			ASSERT_LE(output.odcid.size, proto::MAX_CID_SIZE);
			ASSERT_LE(output.initialScid.size, proto::MAX_CID_SIZE);
			ASSERT_LE(output.retryScid.size, proto::MAX_CID_SIZE);
			// Если параметры закодированы клиентом
			if(sender == endpoint_t::CLIENT){
				// Проверяем отсутствие параметров, допустимых только для сервера (RFC 9000 §18.2)
				ASSERT_FALSE(output.hasOdcid);
				ASSERT_FALSE(output.hasRetryScid);
				ASSERT_FALSE(output.hasResetToken);
				ASSERT_FALSE(output.hasPreferredAddress);
			}
			// Если разобран предпочтительный адрес сервера
			if(output.hasPreferredAddress){
				// Проверяем длину идентификатора соединения предпочтительного адреса
				ASSERT_GE(output.preferredAddress.cid.size, 1u);
				ASSERT_LE(output.preferredAddress.cid.size, proto::MAX_CID_SIZE);
			}
			// Считаем успешный разбор транспортных параметров
			accepted++;
		}
	}
	// Проверяем что фаззинг доходит до успешного разбора параметров
	ASSERT_GT(accepted, 0u);
}

/**
 * @brief Тест устойчивости парсера заголовков пакетов к произвольным данным (RFC 9000 §17)
 *
 * @details Разбор обязан либо отвергнуть датаграмму, либо выдать заголовок,
 *          границы которого лежат в пределах принятой датаграммы
 *
 */
TEST_F(Quic2Fixture, FuzzPacketHeaderTest){
	// Генератор псевдослучайных чисел с фиксированным зерном
	Quic2Random random(0x2a4262b500407ULL);
	// Количество успешных разборов заголовков пакетов
	size_t accepted = 0;
	/**
	 * Перебираем итерации фаззинга
	 */
	for(size_t i = 0; i < ITERATIONS; i++){
		// Генерируем псевдослучайный буфер
		const std::string buffer = ::generate(random);
		// Разобранный заголовок пакета
		packet::header_t header;
		// Код ошибки транспорта
		error_t error = error_t::NO_ERROR;
		// Выполняем разбор заголовка пакета с типовой длиной идентификатора соединения
		if(packet::parser::header(reinterpret_cast <const uint8_t *> (buffer.data()), buffer.size(), 8, header, error) != status_t::OK)
			// Переходим к следующей итерации
			continue;
		// Проверяем что размер пакета не выходит за границу датаграммы
		ASSERT_LE(header.size, buffer.size());
		// Проверяем что смещение поля Packet Number не выходит за границу датаграммы
		ASSERT_LE(header.pnOffset, buffer.size());
		// Проверяем длину идентификаторов соединения по лимиту QUIC v1
		ASSERT_LE(header.dcid.size, proto::MAX_CID_SIZE);
		ASSERT_LE(header.scid.size, proto::MAX_CID_SIZE);
		// Проверяем что токен пакета Initial лежит в пределах датаграммы
		ASSERT_LE(header.token.size(), buffer.size());
		// Проверяем что нагрузка пакета лежит в пределах датаграммы
		ASSERT_LE(header.payload.size(), buffer.size());
		// Считаем успешный разбор заголовка пакета
		accepted++;
	}
	// Проверяем что фаззинг доходит до успешного разбора заголовков
	ASSERT_GT(accepted, 0u);
}
