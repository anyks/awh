/**
 * @file: portmap.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Модуль перенаправления портов — класс unit::Portmap
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <random>

/**
 * Подключаем заголовочный файл модуля
 */
#include <units/portmap.hpp>

/**
 * Подключаем пространства имён
 */
using namespace awh;
using namespace std;
using namespace placeholders;

/**
 * @brief Инкапсулируем работу модуля перенаправления портов в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Источник случайных чисел для отличительных меток перенаправлений
	 *
	 */
	random_device __awh_portmap_randev__;

	/**
	 * @brief Срок ожидания ответа маршрутизатора по умолчанию в миллисекундах
	 *
	 */
	constexpr uint32_t DEFAULT_DELAY = 0xBB8;

	/**
	 * @brief Количество попыток обращения к маршрутизатору по умолчанию
	 *
	 * @note Договоры предписывают куда больше попыток с удвоением срока ожидания, но
	 *       столько уместно вечно работающей службе, а не приложению, которому нужен
	 *       ответ сейчас: договор, которого маршрутизатор не знает, молчит все попытки
	 *
	 */
	constexpr uint8_t DEFAULT_ATTEMPTS = 0x03;

	/**
	 * @brief Метод заполнения отличительной метки перенаправления случайными байтами
	 *
	 * @param nonce место под отличительную метку перенаправления
	 * @param size  размер отведённого места
	 *
	 */
	static void nonce(uint8_t * nonce, const size_t size) noexcept {
		// Источник случайных чисел
		static thread_local mt19937 generator(::__awh_portmap_randev__());
		// Распределение случайных чисел по всему набору значений байта
		static thread_local uniform_int_distribution <uint32_t> dist(0, 0xFF);
		/**
		 * Выполняем перебор всех байтов отличительной метки
		 */
		for(size_t i = 0; i < size; i++)
			// Выполняем заполнение очередного байта отличительной метки
			nonce[i] = static_cast <uint8_t> (dist(generator));
	}
};

/**
 * @brief Конструктор
 *
 */
awh::unit::Portmap::Mapping::Mapping() noexcept :
 proto(proto_t::NONE), internalPort(0), externalPort(0), lifeTime(0), description{""} {}

/**
 * @brief Конструктор
 *
 */
awh::unit::Portmap::Exchange::Exchange() noexcept : waiting(false), attempt(0), eid(0) {}

/**
 * @brief Метод обработки ошибок событий обмена с маршрутизатором
 *
 * @param eid         идентификатор события обмена
 * @param error       код ошибки события обмена
 * @param description описание ошибки события обмена
 *
 */
void awh::unit::Portmap::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки ответов маршрутизатора
 *
 * @param eid  идентификатор события чтения
 * @param data данные, полученные от маршрутизатора
 * @param size размер полученных данных
 *
 */
void awh::unit::Portmap::response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем договор перенаправления, по которому получен ответ
		const type_t type = this->belongs(eid);
		/**
		 * Если событие ни одному из ведущихся обменов не принадлежит
		 *
		 * @note Событие обмена удаляется по завершении обращения, но выданное ему
		 *       чтение способно прийти уже после удаления: сличать следует, а не
		 *       принимать всякое событие за оставшийся договор
		 */
		if(type == type_t::NONE)
			// Завершаем обработку ответа
			return;
		// Получаем обмен, по которому получен ответ
		exchange_t & exchange = ((type == type_t::PCP) ? this->_exchangePCP : this->_exchangeNATPMP);
		/**
		 * Если ответ по этому договору не ожидается
		 *
		 * @note Под видом опроса AUTO договоров ведётся несколько разом, и ответ
		 *       опоздавшего договора приходит уже после принятого: его следует отбросить
		 */
		if(!exchange.waiting)
			// Завершаем обработку ответа
			return;
		/**
		 * Если данные ответа не получены
		 */
		if((data == nullptr) || (size == 0)){
			// Выполняем завершение обмена отказом
			this->failure(type, error_t::MALFORMED);
			// Завершаем обработку ответа
			return;
		}
		/**
		 * Определяем договор перенаправления, по которому получен ответ
		 */
		switch(static_cast <uint8_t> (type)){
			/**
			 * Если ответ получен по договору PCP
			 */
			case static_cast <uint8_t> (type_t::PCP): {
				// Код причины отказа кодека
				proto::portmap::pcp_t::error_t error = proto::portmap::pcp_t::error_t::NONE;
				// Разобранный ответ маршрутизатора
				proto::portmap::pcp_t::answer_t answer;
				/**
				 * Если ответ маршрутизатора разобрать не удалось
				 */
				if(!this->_pcp.parse(data, size, answer, error)){
					// Выполняем завершение обмена отказом
					this->failure(type, error_t::MALFORMED);
					// Завершаем обработку ответа
					return;
				}
				/**
				 * Если маршрутизатор ответил отказом
				 */
				if(answer.result != proto::portmap::pcp_t::result_t::SUCCESS){
					// Выполняем завершение обмена отказом
					this->failure(type, this->reason(answer.result));
					// Завершаем обработку ответа
					return;
				}
				/**
				 * Если заводилось перенаправление порта
				 *
				 * @note Назначенное маршрутизатором перенимается лишь при заведении: ответ
				 *       на удаление несёт по договору нулевой внешний порт и нулевой срок,
				 *       и перенять их значило бы выдать в итоге не то перенаправление,
				 *       которое убиралось
				 */
				if(this->_action == action_t::OPEN){
					// Запоминаем назначенный маршрутизатором внешний порт
					this->_mapping.externalPort = answer.externalPort;
					// Запоминаем назначенный маршрутизатором срок жизни перенаправления
					this->_mapping.lifeTime = answer.lifeTime;
				}
				/**
				 * Если запрашивался внешний адрес маршрутизатора
				 */
				if(this->_action == action_t::EXTERNAL){
					// Собираемый внешний адрес маршрутизатора
					string address;
					/**
					 * Если внешний адрес маршрутизатора извлечь удалось
					 */
					if(this->convert(answer.external, address)){
						// Выполняем завершение обмена по этому договору
						this->complete(type);
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const string &, const type_t)> ("external", address, type);
					// Если внешний адрес маршрутизатора извлечь не удалось
					} else this->failure(type, error_t::MALFORMED);
					// Завершаем обработку ответа
					return;
				}
			} break;
			/**
			 * Если ответ получен по договору NAT-PMP
			 */
			case static_cast <uint8_t> (type_t::NAT_PMP): {
				// Код причины отказа кодека
				proto::portmap::natpmp_t::error_t error = proto::portmap::natpmp_t::error_t::NONE;
				// Разобранный ответ маршрутизатора
				proto::portmap::natpmp_t::answer_t answer;
				/**
				 * Если ответ маршрутизатора разобрать не удалось
				 */
				if(!this->_natpmp.parse(data, size, answer, error)){
					// Выполняем завершение обмена отказом
					this->failure(type, error_t::MALFORMED);
					// Завершаем обработку ответа
					return;
				}
				/**
				 * Если маршрутизатор ответил отказом
				 */
				if(answer.result != proto::portmap::natpmp_t::result_t::SUCCESS){
					// Выполняем завершение обмена отказом
					this->failure(type, this->reason(answer.result));
					// Завершаем обработку ответа
					return;
				}
				/**
				 * Если запрашивался внешний адрес маршрутизатора
				 */
				if(this->_action == action_t::EXTERNAL){
					/**
					 * Если вид полученного сообщения запросу не отвечает
					 */
					if(answer.kind != proto::portmap::natpmp_t::kind_t::ADDRESS){
						// Выполняем завершение обмена отказом
						this->failure(type, error_t::MALFORMED);
						// Завершаем обработку ответа
						return;
					}
					/**
					 * Выполняем размещение внешнего адреса маршрутизатора
					 *
					 * @note Кодек выдаёт адрес числом, старший октет которого является первым
					 *       октетом адреса, и порядок размещения здесь именно такой
					 */
					this->_addr.v4(answer.address, net_addr_t::endian_t::BIG);
					// Выполняем завершение обмена по этому договору
					this->complete(type);
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const string &, const type_t)> ("external", static_cast <string> (this->_addr), type);
					// Завершаем обработку ответа
					return;
				}
				/**
				 * Если вид полученного сообщения запросу не отвечает
				 */
				if(answer.kind != proto::portmap::natpmp_t::kind_t::MAPPING){
					// Выполняем завершение обмена отказом
					this->failure(type, error_t::MALFORMED);
					// Завершаем обработку ответа
					return;
				}
				/**
				 * Если заводилось перенаправление порта
				 *
				 * @note Назначенное маршрутизатором перенимается лишь при заведении: ответ
				 *       на удаление несёт по договору нулевой внешний порт и нулевой срок,
				 *       и перенять их значило бы выдать в итоге не то перенаправление,
				 *       которое убиралось
				 */
				if(this->_action == action_t::OPEN){
					// Запоминаем назначенный маршрутизатором внешний порт
					this->_mapping.externalPort = answer.externalPort;
					// Запоминаем назначенный маршрутизатором срок жизни перенаправления
					this->_mapping.lifeTime = answer.lifeTime;
				}
			} break;
		}
		/**
		 * Сохраняем просьбу, с которой велось обращение
		 *
		 * @note Завершение обмена сбрасывает просьбу, а разбирать, какой обратный вызов
		 *       звать, следует по той просьбе, что подавалась
		 */
		const action_t action = this->_action;
		// Выполняем завершение обмена по этому договору
		this->complete(type);
		/**
		 * Определяем просьбу, с которой велось обращение
		 */
		switch(static_cast <uint8_t> (action)){
			// Если заводилось перенаправление порта
			case static_cast <uint8_t> (action_t::OPEN):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const mapping_t &, const type_t)> ("mapped", this->_mapping, type);
			break;
			// Если убиралось заведённое перенаправление порта
			case static_cast <uint8_t> (action_t::CLOSE):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const mapping_t &, const type_t)> ("unmapped", this->_mapping, type);
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обработки истечения срока ожидания ответа маршрутизатора
 *
 * @param eid    идентификатор события обмена
 * @param action тип действия для истекшего срока ожидания
 * @param delay  длительность срока ожидания в миллисекундах
 * @return       нужно ли завершить обработчик после истечения срока
 *
 */
bool awh::unit::Portmap::timeout(const event::id_t eid, [[maybe_unused]] const event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept {
	// Получаем договор перенаправления, по которому истёк срок ожидания
	const type_t type = this->belongs(eid);
	// Если событие ни одному из ведущихся обменов не принадлежит
	if(type == type_t::NONE)
		// Запрещаем завершение события после истечения срока ожидания
		return false;
	// Получаем обмен, по которому истёк срок ожидания
	exchange_t & exchange = ((type == type_t::PCP) ? this->_exchangePCP : this->_exchangeNATPMP);
	// Если ответ по этому договору не ожидается
	if(!exchange.waiting)
		// Запрещаем завершение события после истечения срока ожидания
		return false;
	/**
	 * Если попытки обращения к маршрутизатору исчерпаны
	 */
	if((exchange.attempt + 1) >= this->_attempts){
		// Выполняем завершение обмена отказом
		this->failure(type, error_t::NO_RESPONSE);
		// Запрещаем завершение события после истечения срока ожидания
		return false;
	}
	// Выполняем подсчёт выполненных попыток обращения к маршрутизатору
	exchange.attempt++;
	/**
	 * Если отправить просьбу повторно не удалось
	 */
	if(!this->submit(type))
		// Выполняем завершение обмена отказом
		this->failure(type, error_t::NO_RESPONSE);
	// Запрещаем завершение события после истечения срока ожидания
	return false;
}
/**
 * @brief Метод определения договора, которому принадлежит событие обмена
 *
 * @note Событие обмена удаляется по завершении обращения, но выданное ему
 * чтение способно прийти уже после удаления: сличать следует, а не
 * принимать всякое событие за оставшийся договор
 *
 * @param eid идентификатор события обмена
 * @return    договор перенаправления, которому принадлежит событие
 *
 */
awh::unit::Portmap::type_t awh::unit::Portmap::belongs(const event::id_t eid) const noexcept {
	// Если событие обмена не заведено
	if(eid == 0)
		// Выводим неопределённый договор перенаправления
		return type_t::NONE;
	// Если событие принадлежит обмену по договору PCP
	if(eid == this->_exchangePCP.eid)
		// Выводим договор PCP
		return type_t::PCP;
	// Если событие принадлежит обмену по договору NAT-PMP
	if(eid == this->_exchangeNATPMP.eid)
		// Выводим договор NAT-PMP
		return type_t::NAT_PMP;
	// Выводим неопределённый договор перенаправления
	return type_t::NONE;
}
/**
 * @brief Метод приведения кода итога договора PCP к коду причины отказа
 *
 * @param result код итога, выданный маршрутизатором
 * @return       код причины отказа перенаправления
 *
 */
awh::unit::Portmap::error_t awh::unit::Portmap::reason(const proto::portmap::pcp_t::result_t result) const noexcept {
	/**
	 * Определяем код итога, выданный маршрутизатором
	 */
	switch(static_cast <uint8_t> (result)){
		// Если просьба выполнена
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::SUCCESS):
			// Выводим отсутствие ошибки
			return error_t::NONE;
		// Если издание договора не поддерживается
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::UNSUPP_VERSION):
		// Если действие не поддерживается
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::UNSUPP_OPCODE):
		// Если дополнение запроса не поддерживается
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::UNSUPP_OPTION):
		// Если договор перенаправления не поддерживается
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::UNSUPP_PROTOCOL):
			// Выводим отказ по неподдерживаемому договору
			return error_t::NOT_SUPPORTED;
		// Если просьба отвергнута настройкой маршрутизатора
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::NOT_AUTHORIZED):
			// Выводим отказ по настройке маршрутизатора
			return error_t::NOT_AUTHORIZED;
		// Если маршрутизатор не имеет связи с внешней сетью
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::NETWORK_FAILURE):
			// Выводим отказ по отсутствию связи с внешней сетью
			return error_t::NETWORK_FAILURE;
		// Если у маршрутизатора не осталось места под перенаправления
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::NO_RESOURCES):
		// Если машина исчерпала отведённую ей долю перенаправлений
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::USER_EX_QUOTA):
			// Выводим отказ по исчерпанию места под перенаправления
			return error_t::OUT_OF_RESOURCES;
		// Если запрос либо дополнение запроса построены ошибочно
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::MALFORMED_REQUEST):
		// Если дополнение запроса построено ошибочно
		case static_cast <uint8_t> (proto::portmap::pcp_t::result_t::MALFORMED_OPTION):
			// Выводим отказ по ошибочно построенному запросу
			return error_t::MALFORMED;
	}
	// Выводим отказ по иной причине
	return error_t::REFUSED;
}
/**
 * @brief Метод приведения кода итога договора NAT-PMP к коду причины отказа
 *
 * @param result код итога, выданный маршрутизатором
 * @return       код причины отказа перенаправления
 *
 */
awh::unit::Portmap::error_t awh::unit::Portmap::reason(const proto::portmap::natpmp_t::result_t result) const noexcept {
	/**
	 * Определяем код итога, выданный маршрутизатором
	 */
	switch(static_cast <uint16_t> (result)){
		// Если просьба выполнена
		case static_cast <uint16_t> (proto::portmap::natpmp_t::result_t::SUCCESS):
			// Выводим отсутствие ошибки
			return error_t::NONE;
		// Если издание договора не поддерживается
		case static_cast <uint16_t> (proto::portmap::natpmp_t::result_t::UNSUPPORTED_VERSION):
		// Если действие не поддерживается
		case static_cast <uint16_t> (proto::portmap::natpmp_t::result_t::UNSUPPORTED_OPCODE):
			// Выводим отказ по неподдерживаемому договору
			return error_t::NOT_SUPPORTED;
		// Если просьба отвергнута настройкой маршрутизатора
		case static_cast <uint16_t> (proto::portmap::natpmp_t::result_t::NOT_AUTHORIZED):
			// Выводим отказ по настройке маршрутизатора
			return error_t::NOT_AUTHORIZED;
		// Если маршрутизатор не имеет связи с внешней сетью
		case static_cast <uint16_t> (proto::portmap::natpmp_t::result_t::NETWORK_FAILURE):
			// Выводим отказ по отсутствию связи с внешней сетью
			return error_t::NETWORK_FAILURE;
		// Если у маршрутизатора не осталось места под перенаправления
		case static_cast <uint16_t> (proto::portmap::natpmp_t::result_t::OUT_OF_RESOURCES):
			// Выводим отказ по исчерпанию места под перенаправления
			return error_t::OUT_OF_RESOURCES;
	}
	// Выводим отказ по иной причине
	return error_t::REFUSED;
}
/**
 * @brief Метод извлечения внешнего адреса маршрутизатора из записи договора PCP
 *
 * @details Договор PCP хранит адреса шестнадцатью октетами: адрес IPv4 лежит
 * в них отображением на пространство IPv6, и извлекается из последних четырёх
 *
 * @param address адрес в записи договора PCP
 * @param result  извлечённый адрес в виде текста
 * @return        результат извлечения адреса
 *
 */
bool awh::unit::Portmap::convert(const uint8_t * address, string & result) noexcept {
	// Извлекаемый адрес в порядке октетов машины
	uint32_t value = 0;
	/**
	 * Если адрес отображением IPv4 на пространство IPv6 не является
	 */
	if(!proto::portmap::pcp_t::decode(address, value))
		// Выводим отрицательный результат извлечения адреса
		return false;
	/**
	 * Выполняем размещение извлечённого адреса
	 *
	 * @note Кодек выдаёт адрес числом, старший октет которого является первым октетом
	 *       адреса, и порядок размещения здесь именно такой
	 */
	this->_addr.v4(value, net_addr_t::endian_t::BIG);
	// Запоминаем извлечённый адрес в виде текста
	result = static_cast <string> (this->_addr);
	// Выводим положительный результат извлечения адреса
	return true;
}
/**
 * @brief Метод получения внутреннего адреса машины для просьбы договора PCP
 *
 * @note Договор предписывает указывать в просьбе адрес обращающейся машины,
 * и маршрутизатор сличает его с адресом отправителя дейтаграммы
 *
 * @param eid     идентификатор события обмена
 * @param address место под адрес в записи договора PCP
 * @return        результат получения адреса
 *
 */
bool awh::unit::Portmap::client(const event::id_t eid, uint8_t * address) noexcept {
	// Внутренний адрес машины
	unique_ptr <net::addr_t> value = nullptr;
	/**
	 * Если внутренний адрес машины получить не удалось
	 */
	if(!this->_io->getAddress(eid, event::address_t::IPV4, value) || (value == nullptr))
		// Выводим отрицательный результат получения адреса
		return false;
	/**
	 * Если полученный адрес адресом IPv4 не является
	 */
	if(value->size != 4)
		// Выводим отрицательный результат получения адреса
		return false;
	// Выполняем размещение внутреннего адреса машины отображением на пространство IPv6
	proto::portmap::pcp_t::encode(address, awh_cast <net::addr_net_ipv4_t *> (value.get())->address);
	// Выводим положительный результат получения адреса
	return true;
}
/**
 * @brief Метод отыскания маршрутизатора
 *
 * @details Заданный настройкой адрес имеет старшинство над отысканным: сеть
 * с несколькими маршрутизаторами таблицей маршрутов однозначно не описывается,
 * и выбор там за приложением
 *
 * @return результат отыскания маршрутизатора
 *
 */
bool awh::unit::Portmap::discover() noexcept {
	// Маршрут до внешней сети
	eth::gateway_t::route_t route{};
	// Выполняем размещение адреса маршрутизатора в маршруте
	route.gateway = make_unique <net::addr_net_ipv4_t> ();
	// Получаем признак того, что маршрут до внешней сети получен
	const bool routed = this->_gateway.get(route);
	// Выполняем размещение адреса маршрутизатора
	this->_address = make_unique <net::addr_net_ipv4_t> ();
	/**
	 * Если адрес маршрутизатора задан настройкой
	 */
	if(!this->_router.empty()){
		/**
		 * Если заданный адрес маршрутизатора разобрать не удалось
		 */
		if(!this->_addr.parse(this->_router, net_addr_t::type_t::IPV4))
			// Выводим отрицательный результат отыскания маршрутизатора
			return false;
		// Запоминаем заданный настройкой адрес маршрутизатора
		awh_cast <net::addr_net_ipv4_t *> (this->_address.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
	/**
	 * Если адрес маршрутизатора берётся из таблицы маршрутов
	 */
	} else {
		/**
		 * Если маршрут до внешней сети получить не удалось
		 */
		if(!routed)
			// Выводим отрицательный результат отыскания маршрутизатора
			return false;
		// Запоминаем адрес маршрутизатора, взятый из таблицы маршрутов
		awh_cast <net::addr_net_ipv4_t *> (this->_address.get())->address = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
	}
	/**
	 * Выполняем отыскание устройства, которым машина достаёт маршрутизатор
	 *
	 * @note Устройство отыскивается принадлежностью адреса маршрутизатора его сети, а
	 *       не запросом маршрута до него: маршрутизатор почти всегда лежит на одной сети
	 *       с машиной, а на такую сеть отдельного маршрута со шлюзом нет вовсе, и запрос
	 *       маршрута до него отвечает отказом
	 */
	this->_source = this->locate(awh_cast <net::addr_net_ipv4_t *> (this->_address.get())->address);
	/**
	 * Если устройство отыскать не удалось, а маршрут до внешней сети получен
	 *
	 * @note Маршрутизатор вправе лежать и за пределами своей сети: тогда достаётся он
	 *       тем же устройством, что и вся внешняя сеть
	 */
	if((this->_source == nullptr) && routed && !route.ifname.empty())
		// Выполняем получение адреса устройства, ведущего во внешнюю сеть
		this->_source = this->_iface.getAddress(route.ifname, event::family_t::IPV4);
	// Выводим результат отыскания маршрутизатора
	return (this->_source != nullptr);
}
/**
 * @brief Метод отыскания устройства, на чьей сети лежит указанный адрес
 *
 * @param address отыскиваемый адрес в порядке октетов, принятом хранилищем адресов
 * @return        адрес отысканного устройства
 *
 */
unique_ptr <net::addr_t> awh::unit::Portmap::locate(const uint32_t address) noexcept {
	/**
	 * Выполняем перебор всех доступных сетевых устройств
	 */
	for(const string & name : this->_iface.available()){
		// Адрес сетевого устройства
		unique_ptr <net::addr_t> ip = nullptr;
		// Адрес узла на другом конце устройства точка-точка
		unique_ptr <net::addr_t> peer = nullptr;
		// Длина приставки сети устройства
		uint8_t prefix = 0;
		// Если параметры сетевого устройства получить не удалось
		if(!this->_iface.getAddress(name, ip, peer, prefix) || (ip == nullptr))
			// Выполняем переход к следующему сетевому устройству
			continue;
		// Если адрес сетевого устройства адресом IPv4 не является
		if((ip->size != 4) || (prefix == 0) || (prefix > 32))
			// Выполняем переход к следующему сетевому устройству
			continue;
		/**
		 * Получаем разряды сети устройства
		 *
		 * @note Приставка считается от старшего октета адреса, а хранится адрес числом,
		 *       младший октет которого является первым октетом адреса: разряды приставки
		 *       потому берутся с младшего конца числа
		 */
		const uint32_t mask = ((prefix == 32) ? 0xFFFFFFFFu : ((1u << prefix) - 1u));
		// Получаем адрес сетевого устройства
		const uint32_t value = awh_cast <net::addr_net_ipv4_t *> (ip.get())->address;
		/**
		 * Если отыскиваемый адрес лежит на сети устройства
		 */
		if((value & mask) == (address & mask))
			// Выводим адрес отысканного устройства
			return ip;
	}
	// Выводим отсутствие отысканного устройства
	return nullptr;
}
/**
 * @brief Метод заведения события обмена по дейтаграммному договору
 *
 * @note Событие пересоздаётся на каждую попытку: назначение получателя на
 * запущенном событии не действует
 *
 * @param type договор перенаправления, по которому ведётся обмен
 * @return     результат заведения события обмена
 *
 */
bool awh::unit::Portmap::datagram(const type_t type) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем обмен, по которому заводится событие
		exchange_t & exchange = ((type == type_t::PCP) ? this->_exchangePCP : this->_exchangeNATPMP);
		// Если событие обмена уже заведено
		if(exchange.eid > 0)
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
		// Выполняем заведение события обмена
		exchange.eid = this->_io->event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
		/**
		 * Если порт маршрутизатора установить не удалось
		 */
		if(!this->_io->setTargetPort(exchange.eid, ((type == type_t::PCP) ? proto::portmap::pcp_t::PORT : proto::portmap::natpmp_t::PORT)))
			// Выводим отрицательный результат заведения события обмена
			return false;
		/**
		 * Если адрес устройства, которым машина достаёт маршрутизатор, установить не удалось
		 *
		 * @note Привязка ведётся именно к этому адресу, а не ко всем сразу: машина с
		 *       несколькими путями во внешнюю сеть - обычное дело, и привязка ко всем
		 *       адресам уводит дейтаграмму не тем устройством, откуда маршрутизатор
		 *       недостижим
		 */
		if(!this->_io->setAddress(exchange.eid, event::address_t::IPV4, this->_source.get()))
			// Выводим отрицательный результат заведения события обмена
			return false;
		/**
		 * Если адрес маршрутизатора установить не удалось
		 */
		if(!this->_io->setTarget(exchange.eid, this->_address.get()))
			// Выводим отрицательный результат заведения события обмена
			return false;
		// Устанавливаем срок ожидания ответа маршрутизатора
		this->_io->setTimeout(exchange.eid, event::action_t::READ, this->_delay);
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(exchange.eid, static_cast <engine::callback::error_t> (std::bind(&portmap_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(exchange.eid, static_cast <engine::callback::read_t> (std::bind(&portmap_t::response, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие истечения срока ожидания
		this->_io->on(exchange.eid, static_cast <engine::callback::timeout_t> (std::bind(static_cast <bool (portmap_t::*)(const event::id_t, const event::action_t, const uint32_t)> (&portmap_t::timeout), this, _1, _2, _3)));
		/**
		 * Если опции события обмена установить не удалось
		 */
		if(!this->_io->setOptions(exchange.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
			// Выводим отрицательный результат заведения события обмена
			return false;
		}
		/**
		 * Если запустить событие обмена не удалось
		 */
		if(!(result = (this->_io->commit(exchange.eid) && this->_io->launch(exchange.eid))))
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат заведения события обмена
	return result;
}
/**
 * @brief Метод отправки просьбы маршрутизатору по дейтаграммному договору
 *
 * @param type договор перенаправления, по которому ведётся обмен
 * @return     результат отправки просьбы
 *
 */
bool awh::unit::Portmap::submit(const type_t type) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем обмен, по которому отправляется просьба
		exchange_t & exchange = ((type == type_t::PCP) ? this->_exchangePCP : this->_exchangeNATPMP);
		/**
		 * Если событие обмена завести не удалось
		 *
		 * @note Событие пересоздаётся на каждую попытку: назначение получателя на
		 *       запущенном событии не действует
		 */
		if(!this->datagram(type))
			// Выводим отрицательный результат отправки просьбы
			return false;
		// Место под собираемое сообщение
		uint8_t buffer[proto::portmap::pcp_t::MAX_MESSAGE_SIZE] = {0};
		// Размер собранного сообщения
		size_t size = 0;
		/**
		 * Определяем договор перенаправления, по которому отправляется просьба
		 */
		switch(static_cast <uint8_t> (type)){
			/**
			 * Если просьба отправляется по договору PCP
			 */
			case static_cast <uint8_t> (type_t::PCP): {
				// Код причины отказа кодека
				proto::portmap::pcp_t::error_t error = proto::portmap::pcp_t::error_t::NONE;
				// Собираемая просьба маршрутизатору
				proto::portmap::pcp_t::request_t request;
				// Запоминаем действие договора
				request.opcode = proto::portmap::pcp_t::opcode_t::MAP;
				// Запоминаем договор перенаправляемого порта
				request.proto = ((this->_mapping.proto == proto_t::TCP) ? proto::portmap::pcp_t::proto_t::TCP : proto::portmap::pcp_t::proto_t::UDP);
				// Запоминаем внутренний порт перенаправления
				request.internalPort = this->_mapping.internalPort;
				// Запоминаем желаемый внешний порт перенаправления
				request.externalPort = this->_mapping.externalPort;
				// Запоминаем запрашиваемый срок жизни перенаправления
				request.lifeTime = ((this->_action == action_t::CLOSE) ? 0 : this->_mapping.lifeTime);
				// Выполняем заполнение отличительной метки перенаправления
				::nonce(&request.nonce[0], proto::portmap::pcp_t::NONCE_SIZE);
				/**
				 * Если внутренний адрес машины получить не удалось
				 *
				 * @note Договор предписывает указывать в просьбе адрес обращающейся машины,
				 *       и маршрутизатор сличает его с адресом отправителя дейтаграммы
				 */
				if(!this->client(exchange.eid, &request.client[0]))
					// Выводим отрицательный результат отправки просьбы
					return false;
				// Выполняем сборку просьбы маршрутизатору
				size = this->_pcp.request(&buffer[0], sizeof(buffer), request, error);
			} break;
			/**
			 * Если просьба отправляется по договору NAT-PMP
			 */
			case static_cast <uint8_t> (type_t::NAT_PMP): {
				// Код причины отказа кодека
				proto::portmap::natpmp_t::error_t error = proto::portmap::natpmp_t::error_t::NONE;
				/**
				 * Если запрашивается внешний адрес маршрутизатора
				 */
				if(this->_action == action_t::EXTERNAL)
					// Выполняем сборку просьбы о внешнем адресе маршрутизатора
					size = this->_natpmp.address(&buffer[0], sizeof(buffer), error);
				/**
				 * Если заводится либо убирается перенаправление порта
				 */
				else {
					// Собираемая просьба маршрутизатору
					proto::portmap::natpmp_t::request_t request;
					// Запоминаем договор перенаправляемого порта
					request.proto = ((this->_mapping.proto == proto_t::TCP) ? proto::portmap::natpmp_t::proto_t::TCP : proto::portmap::natpmp_t::proto_t::UDP);
					// Запоминаем внутренний порт перенаправления
					request.internalPort = this->_mapping.internalPort;
					/**
					 * Запоминаем желаемый внешний порт перенаправления
					 *
					 * @note Убирается перенаправление той же просьбой с нулевым сроком жизни
					 *       и нулевым внешним портом: отдельного действия договор не имеет
					 */
					request.externalPort = ((this->_action == action_t::CLOSE) ? 0 : this->_mapping.externalPort);
					// Запоминаем запрашиваемый срок жизни перенаправления
					request.lifeTime = ((this->_action == action_t::CLOSE) ? 0 : this->_mapping.lifeTime);
					// Выполняем сборку просьбы маршрутизатору
					size = this->_natpmp.mapping(&buffer[0], sizeof(buffer), request, error);
				}
			} break;
		}
		// Если просьбу собрать не удалось
		if(size == 0)
			// Выводим отрицательный результат отправки просьбы
			return false;
		// Если просьбу отправить не удалось
		if(this->_io->send(exchange.eid, &buffer[0], size) == 0)
			// Выводим отрицательный результат отправки просьбы
			return false;
		// Запоминаем, что ответ маршрутизатора ожидается
		exchange.waiting = true;
		// Выводим положительный результат отправки просьбы
		return true;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим отрицательный результат отправки просьбы
	return false;
}
/**
 * @brief Метод начала обращения к маршрутизатору
 *
 * @details Начатое ранее обращение прекращается: одновременно ведётся лишь
 * одно обращение, и просьба, поданная поверх незавершённой, отменяет её
 *
 * @param action  просьба, с которой ведётся обращение
 * @param mapping перенаправление, о котором ведётся обращение
 * @return        результат начала обращения
 *
 */
bool awh::unit::Portmap::perform(const action_t action, const mapping_t & mapping) noexcept {
	// Выполняем прекращение всех ведущихся обменов
	this->cancel();
	// Запоминаем просьбу, с которой ведётся обращение
	this->_action = action;
	// Запоминаем перенаправление, о котором ведётся обращение
	this->_mapping = mapping;
	/**
	 * Если маршрутизатор отыскать не удалось
	 */
	if(!this->discover()){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::NO_GATEWAY);
		// Выводим отрицательный результат начала обращения
		return false;
	}
	// Признак того, что обмен начат хотя бы по одному договору
	bool result = false;
	/**
	 * Если обмен ведётся договором PCP
	 */
	if((this->_type == type_t::PCP) || (this->_type == type_t::AUTO))
		// Выполняем отправку просьбы по договору PCP
		result = (this->submit(type_t::PCP) || result);
	/**
	 * Если обмен ведётся договором NAT-PMP
	 */
	if((this->_type == type_t::NAT_PMP) || (this->_type == type_t::AUTO))
		// Выполняем отправку просьбы по договору NAT-PMP
		result = (this->submit(type_t::NAT_PMP) || result);
	/**
	 * Если обмен не начат ни по одному договору
	 */
	if(!result)
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::NO_RESPONSE);
	// Выводим результат начала обращения
	return result;
}
/**
 * @brief Метод завершения обмена отказом
 *
 * @param type  договор перенаправления, по которому вёлся обмен
 * @param error код причины отказа
 *
 */
void awh::unit::Portmap::failure(const type_t type, const error_t error) noexcept {
	/**
	 * Если обмен вёлся по одному из дейтаграммных договоров
	 */
	if((type == type_t::PCP) || (type == type_t::NAT_PMP)){
		// Получаем обмен, по которому вёлся обмен
		exchange_t & exchange = ((type == type_t::PCP) ? this->_exchangePCP : this->_exchangeNATPMP);
		// Снимаем признак ожидания ответа маршрутизатора
		exchange.waiting = false;
		// Если событие обмена заведено
		if(exchange.eid > 0)
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
		// Сбрасываем идентификатор события обмена
		exchange.eid = 0;
	}
	/**
	 * Если обмен ведётся хотя бы по одному договору
	 *
	 * @note Под видом опроса AUTO договоров ведётся несколько разом, и отказ одного из
	 *       них ещё не означает отказа обращения: оно завершается, когда откажут все
	 */
	if(this->_exchangePCP.waiting || this->_exchangeNATPMP.waiting)
		// Завершаем обработку отказа
		return;
	// Сбрасываем просьбу, с которой велось обращение
	this->_action = action_t::NONE;
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("failure");
	/**
	 * Если функция обратного вызова установлена
	 */
	if(this->_callback.is(fid))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const error_t, const type_t)> (fid, error, type);
	/**
	 * Если функция обратного вызова не установлена
	 */
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"Port mapping failed (protocol: %u, reason: %u)",
				__PRETTY_FUNCTION__,
				make_tuple(static_cast <uint16_t> (type), static_cast <uint16_t> (error)),
				log_t::flag_t::WARNING,
				static_cast <uint16_t> (type), static_cast <uint16_t> (error)
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Port mapping failed (protocol: %u, reason: %u)", log_t::flag_t::WARNING, static_cast <uint16_t> (type), static_cast <uint16_t> (error));
		#endif
	}
}
/**
 * @brief Метод завершения обмена по одному договору
 *
 * @details Обмены прочих договоров прекращаются: под видом опроса AUTO их
 * ведётся несколько разом, и первый пришедший ответ делает остальные ненужными
 *
 * @param type договор перенаправления, по которому получен ответ
 *
 */
void awh::unit::Portmap::complete([[maybe_unused]] const type_t type) noexcept {
	// Выполняем прекращение всех ведущихся обменов
	this->cancel();
	// Сбрасываем просьбу, с которой велось обращение
	this->_action = action_t::NONE;
}
/**
 * @brief Метод прекращения всех ведущихся обменов
 *
 */
void awh::unit::Portmap::cancel() noexcept {
	/**
	 * Выполняем перебор всех дейтаграммных обменов
	 */
	for(exchange_t * exchange : {&this->_exchangePCP, &this->_exchangeNATPMP}){
		// Снимаем признак ожидания ответа маршрутизатора
		exchange->waiting = false;
		// Сбрасываем отсчёт попыток обращения к маршрутизатору
		exchange->attempt = 0;
		/**
		 * Если событие обмена заведено
		 */
		if(exchange->eid > 0){
			// Выполняем удаление события обмена
			this->_io->destroy(exchange->eid);
			// Сбрасываем идентификатор события обмена
			exchange->eid = 0;
		}
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::unit::Portmap::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова для заведённого перенаправления
	this->_callback.set("mapped", callback);
	// Выполняем установку функции обратного вызова для убранного перенаправления
	this->_callback.set("unmapped", callback);
	// Выполняем установку функции обратного вызова для внешнего адреса маршрутизатора
	this->_callback.set("external", callback);
	// Выполняем установку функции обратного вызова для отказа перенаправления
	this->_callback.set("failure", callback);
}
/**
 * @brief Метод получения вида опроса маршрутизатора
 *
 * @return вид опроса маршрутизатора
 *
 */
awh::unit::Portmap::type_t awh::unit::Portmap::getType() const noexcept {
	// Выводим вид опроса маршрутизатора
	return this->_type;
}
/**
 * @brief Метод установки вида опроса маршрутизатора
 *
 * @param type вид опроса маршрутизатора для установки
 *
 */
void awh::unit::Portmap::setType(const type_t type) noexcept {
	// Устанавливаем вид опроса маршрутизатора
	this->_type = type;
}
/**
 * @brief Метод установки срока ожидания ответа маршрутизатора
 *
 * @param delay срок ожидания ответа маршрутизатора в миллисекундах
 *
 */
void awh::unit::Portmap::setTimeout(const uint32_t delay) noexcept {
	// Устанавливаем срок ожидания ответа маршрутизатора
	this->_delay = delay;
}
/**
 * @brief Метод установки количества попыток обращения к маршрутизатору
 *
 * @param attempts количество попыток обращения к маршрутизатору
 *
 */
void awh::unit::Portmap::setAttempts(const uint8_t attempts) noexcept {
	// Если количество попыток обращения к маршрутизатору передано
	if(attempts > 0)
		// Устанавливаем количество попыток обращения к маршрутизатору
		this->_attempts = attempts;
}
/**
 * @brief Метод установки адреса маршрутизатора
 *
 * @details Заданный адрес отменяет отыскание маршрутизатора по таблице
 * маршрутов. Пустой адрес возвращает отыскание
 *
 * @param router адрес маршрутизатора для установки
 *
 */
void awh::unit::Portmap::setRouter(string_view router) noexcept {
	// Устанавливаем адрес маршрутизатора
	this->_router.assign(router.begin(), router.end());
}
/**
 * @brief Метод получения внешнего адреса маршрутизатора
 *
 * @return результат отправки просьбы
 *
 */
bool awh::unit::Portmap::external() noexcept {
	// Выполняем начало обращения к маршрутизатору
	return this->perform(action_t::EXTERNAL, mapping_t());
}
/**
 * @brief Метод заведения перенаправления порта
 *
 * @param mapping перенаправление порта для заведения
 * @return        результат отправки просьбы
 *
 */
bool awh::unit::Portmap::open(const mapping_t & mapping) noexcept {
	/**
	 * Если перенаправление задано неполно
	 */
	if((mapping.proto == proto_t::NONE) || (mapping.internalPort == 0)){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::MALFORMED);
		// Выводим отрицательный результат отправки просьбы
		return false;
	}
	// Выполняем начало обращения к маршрутизатору
	return this->perform(action_t::OPEN, mapping);
}
/**
 * @brief Метод удаления заведённого перенаправления порта
 *
 * @param mapping перенаправление порта для удаления
 * @return        результат отправки просьбы
 *
 */
bool awh::unit::Portmap::close(const mapping_t & mapping) noexcept {
	/**
	 * Если перенаправление задано неполно
	 */
	if((mapping.proto == proto_t::NONE) || (mapping.internalPort == 0)){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::MALFORMED);
		// Выводим отрицательный результат отправки просьбы
		return false;
	}
	// Выполняем начало обращения к маршрутизатору
	return this->perform(action_t::CLOSE, mapping);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::Portmap::Portmap(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _type(type_t::AUTO), _action(action_t::NONE),
 _attempts(::DEFAULT_ATTEMPTS), _delay(::DEFAULT_DELAY), _router{""},
 _address(nullptr), _source(nullptr), _addr(fmk, log), _iface(fmk, log), _gateway(fmk, log), _pcp(fmk, log),
 _ssdp(fmk, log), _soap(fmk, log), _upnp(fmk, log), _device(fmk, log), _natpmp(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Portmap::~Portmap() noexcept {
	// Выполняем прекращение всех ведущихся обменов
	this->cancel();
}
