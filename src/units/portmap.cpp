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
		exchange_t & exchange = this->exchange(type);
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
			 * Если ответ получен по договору UPnP
			 */
			case static_cast <uint8_t> (type_t::UPNP): {
				// Код причины отказа кодека
				proto::portmap::ssdp_t::error_t error = proto::portmap::ssdp_t::error_t::NONE;
				// Разобранное сообщение договора
				proto::portmap::ssdp_t::answer_t answer;
				/**
				 * Если сообщение договора разобрать не удалось
				 *
				 * @note Отказ здесь обмена не прерывает: на групповой адрес приходят и
				 *       сообщения посторонних устройств, и негодное следует пропускать,
				 *       продолжая ждать пригодного до истечения срока
				 */
				if(!this->_ssdp.parse(string_view(reinterpret_cast <const char *> (data), size), answer, error))
					// Ожидаем ответа пригодного устройства
					return;
				/**
				 * Если ответ устройству доступа в сеть не отвечает
				 */
				if(!this->_ssdp.suitable(answer, proto::portmap::ssdp_t::TARGET_GATEWAY))
					// Ожидаем ответа пригодного устройства
					return;
				// Запоминаем адрес описания отысканного устройства
				this->_location = answer.location;
				/**
				 * Сбрасываем счёт выполненных попыток обращения
				 *
				 * @note Признак ожидания при этом не снимается: устройство лишь отыскано, а
				 *       обмен по договору продолжается потоковым событием, и срок ожидания
				 *       нужен и ему
				 */
				exchange.attempt = 0;
				/**
				 * Если событие рассылки заведено
				 *
				 * @note Устройство отыскано, и рассылка больше не нужна: ответы прочих
				 *       устройств сети к делу не относятся
				 */
				if(exchange.eid > 0){
					// Выполняем удаление события рассылки
					this->_io->destroy(exchange.eid);
					// Сбрасываем идентификатор события рассылки
					exchange.eid = 0;
				}
				/**
				 * Если чтение описания устройства начать не удалось
				 */
				if(!this->describe())
					// Выполняем завершение обмена отказом
					this->failure(type, error_t::NO_RESPONSE);
				// Завершаем обработку ответа
				return;
			}
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
	exchange_t & exchange = this->exchange(type);
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
	/**
	 * Если событие принадлежит обмену по договору UPnP
	 *
	 * @note Обмен по этому договору ведётся двумя событиями: рассылка отыскивает
	 *       устройство, а потоковое событие читает описание и вызывает действие
	 *       службы - и оба принадлежат одному договору
	 */
	if((eid == this->_exchangeUPNP.eid) || ((this->_stream > 0) && (eid == this->_stream)))
		// Выводим договор UPnP
		return type_t::UPNP;
	// Если событие принадлежит обмену по договору NAT-PMP
	if(eid == this->_exchangeNATPMP.eid)
		// Выводим договор NAT-PMP
		return type_t::NAT_PMP;
	// Выводим неопределённый договор перенаправления
	return type_t::NONE;
}
/**
 * @brief Метод получения записи обмена по договору перенаправления
 *
 * @param type договор перенаправления, по которому ведётся обмен
 * @return     запись обмена по указанному договору
 *
 */
awh::unit::Portmap::exchange_t & awh::unit::Portmap::exchange(const type_t type) noexcept {
	/**
	 * Определяем договор перенаправления, по которому ведётся обмен
	 */
	switch(static_cast <uint8_t> (type)){
		// Если обмен ведётся договором PCP
		case static_cast <uint8_t> (type_t::PCP):
			// Выводим запись обмена по договору PCP
			return this->_exchangePCP;
		// Если обмен ведётся договором UPnP
		case static_cast <uint8_t> (type_t::UPNP):
			// Выводим запись обмена по договору UPnP
			return this->_exchangeUPNP;
	}
	// Выводим запись обмена по договору NAT-PMP
	return this->_exchangeNATPMP;
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
 * @brief Метод приведения кода итога службы UPnP к коду причины отказа
 *
 * @param result код итога, выданный службой устройства
 * @return       код причины отказа перенаправления
 *
 */
awh::unit::Portmap::error_t awh::unit::Portmap::reason(const proto::portmap::upnp_t::result_t result) const noexcept {
	/**
	 * Определяем код итога, выданный службой устройства
	 */
	switch(static_cast <uint32_t> (result)){
		// Если просьба выполнена
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::SUCCESS):
			// Выводим отсутствие ошибки
			return error_t::NONE;
		// Если действие службе неизвестно
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::INVALID_ACTION):
			// Выводим отказ по неподдерживаемому договору
			return error_t::NOT_SUPPORTED;
		// Если действие отвергнуто настройкой маршрутизатора
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::NOT_AUTHORIZED):
			// Выводим отказ по настройке маршрутизатора
			return error_t::NOT_AUTHORIZED;
		// Если у маршрутизатора не осталось места под перенаправления
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::NO_PORT_MAPS):
			// Выводим отказ по исчерпанию места под перенаправления
			return error_t::OUT_OF_RESOURCES;
		// Если доводы вызова построены ошибочно
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::INVALID_ARGS):
			// Выводим отказ по ошибочно построенному запросу
			return error_t::MALFORMED;
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
		// Выводим положительный результат отыскания маршрутизатора
		return true;
	}
	// Маршрут до внешней сети
	eth::gateway_t::route_t route{};
	// Выполняем размещение адреса маршрутизатора в маршруте
	route.gateway = make_unique <net::addr_net_ipv4_t> ();
	/**
	 * Если маршрут до внешней сети получить не удалось
	 */
	if(!this->_gateway.get(route))
		// Выводим отрицательный результат отыскания маршрутизатора
		return false;
	// Запоминаем адрес маршрутизатора, взятый из таблицы маршрутов
	awh_cast <net::addr_net_ipv4_t *> (this->_address.get())->address = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
	// Выводим положительный результат отыскания маршрутизатора
	return true;
}
/**
 * @brief Метод отыскания устройства UPnP рассылкой SSDP
 *
 * @details Просьба рассылается на групповой адрес: устройство отвечает не
 * сразу, а спустя случайное время в пределах отведённого срока, поэтому
 * ответ приходит обычным чтением события, а не следом за отправкой
 *
 * @return результат начала отыскания устройства
 *
 */
bool awh::unit::Portmap::search() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие обмена уже заведено
		if(this->_exchangeUPNP.eid > 0)
			// Выполняем удаление события обмена
			this->_io->destroy(this->_exchangeUPNP.eid);
		// Выполняем заведение события рассылки
		this->_exchangeUPNP.eid = this->_io->event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
		/**
		 * Если групповой режим события установить не удалось
		 */
		if(!this->_io->setDelivery(this->_exchangeUPNP.eid, event::delivery_mode_t::MULTICAST))
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		/**
		 * Если предел числа переходов рассылки установить не удалось
		 *
		 * @note Рассылка обнаружения дальше своей сети не уходит: устройство доступа в
		 *       сеть лежит на ней же, а разослать просьбу шире значило бы беспокоить чужие
		 */
		if(!this->_io->setHops(this->_exchangeUPNP.eid, event::hops_t::NETWORK))
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		/**
		 * Если порт обнаружения устройств установить не удалось
		 */
		if(!this->_io->setTargetPort(this->_exchangeUPNP.eid, proto::portmap::ssdp_t::PORT))
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		/**
		 * Если групповой адрес обнаружения устройств установить не удалось
		 */
		if(!this->_io->setTarget(this->_exchangeUPNP.eid, proto::portmap::ssdp_t::MULTICAST_ADDRESS))
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		// Устанавливаем срок ожидания ответа устройства
		this->_io->setTimeout(this->_exchangeUPNP.eid, event::action_t::READ, this->_delay);
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_exchangeUPNP.eid, static_cast <engine::callback::error_t> (std::bind(&portmap_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(this->_exchangeUPNP.eid, static_cast <engine::callback::read_t> (std::bind(&portmap_t::response, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие истечения срока ожидания
		this->_io->on(this->_exchangeUPNP.eid, static_cast <engine::callback::timeout_t> (std::bind(static_cast <bool (portmap_t::*)(const event::id_t, const event::action_t, const uint32_t)> (&portmap_t::timeout), this, _1, _2, _3)));
		/**
		 * Если опции события рассылки установить не удалось
		 */
		if(!this->_io->setOptions(this->_exchangeUPNP.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::MULTICAST_LOOPBACK)){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		/**
		 * Если вступление в группу обнаружения устройств выполнить не удалось
		 */
		if(!this->_io->membership(this->_exchangeUPNP.eid, event::mode_t::ENABLED, proto::portmap::ssdp_t::MULTICAST_ADDRESS, "0.0.0.0")){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		/**
		 * Если запустить событие рассылки не удалось
		 */
		if(!(this->_io->commit(this->_exchangeUPNP.eid) && this->_io->launch(this->_exchangeUPNP.eid))){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		// Выполняем сборку просьбы обнаружения устройства доступа в сеть
		const string & message = this->_ssdp.search(proto::portmap::ssdp_t::TARGET_GATEWAY);
		/**
		 * Если просьбу разослать не удалось
		 */
		if(this->_io->send(this->_exchangeUPNP.eid, message.data(), message.length()) == 0){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		// Запоминаем шаг обмена по договору UPnP
		this->_stage = stage_t::SEARCH;
		// Запоминаем, что ответ устройства ожидается
		this->_exchangeUPNP.waiting = true;
		// Выводим положительный результат начала отыскания устройства
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим отрицательный результат начала отыскания устройства
	return false;
}
/**
 * @brief Метод заведения потокового события обмена с устройством UPnP
 *
 * @param address адрес устройства, с которым ведётся обмен
 * @param port    порт устройства, с которым ведётся обмен
 * @return        результат заведения потокового события обмена
 *
 */
bool awh::unit::Portmap::stream(string_view address, const uint16_t port) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если потоковое событие обмена уже заведено
		if(this->_stream > 0)
			// Выполняем удаление потокового события обмена
			this->_io->destroy(this->_stream);
		// Запоминаем адрес устройства, с которым ведётся обмен
		this->_host = address;
		// Запоминаем порт устройства, с которым ведётся обмен
		this->_port = port;
		// Выполняем очистку собираемого тела ответа устройства
		this->_payload.clear();
		// Снимаем признак того, что ответ устройства прочитан до конца
		this->_complete = false;
		// Выполняем заведение потокового события обмена
		this->_stream = this->_io->event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
		/**
		 * Если порт устройства установить не удалось
		 */
		if(!this->_io->setTargetPort(this->_stream, port))
			// Выводим отрицательный результат заведения потокового события обмена
			return false;
		/**
		 * Если адрес устройства установить не удалось
		 */
		if(!this->_io->setTarget(this->_stream, address))
			// Выводим отрицательный результат заведения потокового события обмена
			return false;
		// Устанавливаем срок ожидания ответа устройства
		this->_io->setTimeout(this->_stream, event::action_t::READ, this->_delay);
		/**
		 * Устанавливаем срок ожидания подключения к устройству
		 *
		 * @note Без него неудачная попытка подключения висит столько, сколько отведёт
		 *       система, и обмен молчит вместо того, чтобы завершиться отказом
		 */
		this->_io->setTimeout(this->_stream, event::action_t::CONNECT, this->_delay);
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_stream, static_cast <engine::callback::error_t> (std::bind(&portmap_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие подключения к устройству
		this->_io->on(this->_stream, static_cast <engine::callback::connect_t> (std::bind(&portmap_t::connected, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(this->_stream, static_cast <engine::callback::read_t> (std::bind(&portmap_t::incoming, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие истечения срока ожидания
		this->_io->on(this->_stream, static_cast <engine::callback::timeout_t> (std::bind(static_cast <bool (portmap_t::*)(const event::id_t, const event::action_t, const uint32_t)> (&portmap_t::timeout), this, _1, _2, _3)));
		/**
		 * Если опции потокового события обмена установить не удалось
		 */
		if(!this->_io->setOptions(this->_stream, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
			// Выполняем удаление потокового события обмена
			this->_io->destroy(this->_stream);
			// Выводим отрицательный результат заведения потокового события обмена
			return false;
		}
		/**
		 * Если запустить потоковое событие обмена не удалось
		 */
		if(!(this->_io->commit(this->_stream) && this->_io->connect(this->_stream) && this->_io->launch(this->_stream))){
			// Выполняем удаление потокового события обмена
			this->_io->destroy(this->_stream);
			// Выводим отрицательный результат заведения потокового события обмена
			return false;
		}
		// Выводим положительный результат заведения потокового события обмена
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(address, port), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим отрицательный результат заведения потокового события обмена
	return false;
}
/**
 * @brief Метод обработки подключения к устройству UPnP
 *
 * @note Запрос отправляется по подключении, а не раньше: до него отправлять
 * потоковому событию нечего
 *
 * @param eid идентификатор потокового события обмена
 * @param ok  результат подключения к устройству
 *
 */
void awh::unit::Portmap::connected(const event::id_t eid, const bool ok) noexcept {
	// Если событие потоковому обмену не принадлежит
	if(eid != this->_stream)
		// Завершаем обработку подключения
		return;
	/**
	 * Если подключиться к устройству не удалось
	 */
	if(!ok){
		// Получаем обмен по договору UPnP
		exchange_t & exchange = this->exchange(type_t::UPNP);
		/**
		 * Если попытки обращения к устройству не исчерпаны
		 *
		 * @note Неудача подключения расходует попытку, а не завершает обмен: устройство
		 *       домашней сети отвечает не всегда с первого раза, и повтор обходится
		 *       дешевле отказа
		 */
		if((exchange.attempt + 1) < this->_attempts){
			// Выполняем подсчёт выполненных попыток обращения к устройству
			exchange.attempt++;
			// Если повторное подключение к устройству начать удалось
			if(this->stream(this->_host, this->_port))
				// Завершаем обработку подключения
				return;
		}
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, error_t::NO_RESPONSE);
		// Завершаем обработку подключения
		return;
	}
	/**
	 * Если запрос отправить не удалось
	 */
	if(this->_io->send(this->_stream, this->_request.data(), this->_request.length()) == 0)
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, error_t::NO_RESPONSE);
}
/**
 * @brief Метод обработки данных, полученных от устройства UPnP
 *
 * @param eid  идентификатор потокового события обмена
 * @param data данные, полученные от устройства
 * @param size размер полученных данных
 *
 */
void awh::unit::Portmap::incoming(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если событие потоковому обмену не принадлежит
		if(eid != this->_stream)
			// Завершаем обработку полученных данных
			return;
		// Если ответ устройства уже прочитан до конца
		if(this->_complete)
			// Завершаем обработку полученных данных
			return;
		/**
		 * Если данные не получены
		 */
		if((data == nullptr) || (size == 0)){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем обработку полученных данных
			return;
		}
		/**
		 * Если ответ устройства превысил допустимый размер
		 *
		 * @note Предел обязателен: ответ берётся у устройства, о котором заранее ничего
		 *       не известно, и доверять его размеру нельзя
		 */
		if((this->_payload.length() + size) > proto::portmap::device_t::MAX_DESCRIPTION_SIZE){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем обработку полученных данных
			return;
		}
		// Выполняем накопление полученных данных
		this->_payload.append(reinterpret_cast <const char *> (data), size);
		// Объект разбора ответа устройства
		http::parser_http_t parser(http::direct_t::RESPONSE, this->_fmk, this->_log);
		// Собираемое тело ответа устройства
		string body;
		// Устанавливаем функцию обратного вызова на получение тела ответа
		parser.on(http::parser_http_t::data_callback_t([&body](const uint32_t, const void * data, const size_t size, const bool) noexcept -> bool {
			// Выполняем накопление полученного тела ответа
			body.append(reinterpret_cast <const char *> (data), size);
			// Разрешаем продолжение разбора
			return true;
		}));
		/**
		 * Выполняем разбор накопленного ответа устройства
		 *
		 * @note Разбор ведётся по накопленному целиком, а не по мере поступления: объект
		 *       разбора заводится на каждое чтение заново, и своего состояния между
		 *       чтениями не держит
		 */
		parser.parse(this->_payload.data(), this->_payload.length());
		/**
		 * Если ответ устройства прочитан не до конца
		 *
		 * @note Завершённость определяется итоговым состоянием разбора, а не стадией
		 *       END: стадия объявляется и на конце блока заголовков, и лишь состояние
		 *       COMPLETE говорит о том, что сообщение прочитано целиком
		 */
		if(parser.status() != http::parser_http_t::status_t::COMPLETE)
			// Ожидаем поступления остатка ответа
			return;
		// Запоминаем, что ответ устройства прочитан до конца
		this->_complete = true;
		/**
		 * Если разбор ответа устройства завершился отказом
		 */
		if(parser.error() != http::parser_http_t::error_t::NONE){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем обработку полученных данных
			return;
		}
		/**
		 * Если устройство ответило отказом
		 */
		/**
		 * Если ответ устройства заголовка не содержит
		 */
		if(parser.message().provider == nullptr){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем обработку полученных данных
			return;
		}
		// Получаем код ответа устройства
		const uint32_t code = static_cast <const http::response_t *> (parser.message().provider.get())->code;
		/**
		 * Если устройство ответило отказом
		 *
		 * @note Договор UPnP предписывает отдавать отказ службы кодом 500 с конвертом
		 *       SOAP Fault в теле: причину отказа несёт тело, а не код ответа, и
		 *       читать его следует так же, как удачный ответ
		 */
		if((code != 200) && !((code == 500) && (this->_stage == stage_t::CONTROL))){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::REFUSED);
			// Завершаем обработку полученных данных
			return;
		}
		// Запоминаем тело ответа устройства
		this->_payload = ::move(body);
		/**
		 * Определяем шаг обмена по договору UPnP
		 */
		switch(static_cast <uint8_t> (this->_stage)){
			// Если читалось описание устройства
			case static_cast <uint8_t> (stage_t::DESCRIPTION):
				// Выполняем разбор прочитанного описания устройства
				this->described();
			break;
			// Если вызывалось действие службы устройства
			case static_cast <uint8_t> (stage_t::CONTROL):
				// Выполняем разбор ответа службы устройства
				this->controlled();
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
 * @brief Метод чтения описания отысканного устройства UPnP
 *
 * @return результат начала чтения описания устройства
 *
 */
bool awh::unit::Portmap::describe() noexcept {
	// Выполняем очистку объекта работы с адресами ресурсов
	this->_uri.clear();
	/**
	 * Если адрес описания устройства разобрать не удалось
	 */
	if(this->_uri.parse(this->_location) == uri_t::type_t::NONE)
		// Выводим отрицательный результат начала чтения описания устройства
		return false;
	// Получаем адрес устройства, у которого читается описание
	const string & host = this->_uri.host();
	/**
	 * Если адрес устройства получить не удалось
	 */
	if(host.empty())
		// Выводим отрицательный результат начала чтения описания устройства
		return false;
	// Получаем порт устройства, у которого читается описание
	const uint16_t port = (this->_uri.port() > 0 ? this->_uri.port() : 80);
	// Выполняем сборку пути запроса описания устройства
	string path = this->_uri.print(uri_t::item_t::REQUEST);
	// Если путь запроса описания устройства пуст
	if(path.empty())
		// Устанавливаем путь запроса описания устройства по умолчанию
		path.assign(1, '/');
	// Выполняем сборку запроса описания устройства
	this->_request = this->_fmk->format(
		"GET %s HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Connection: close\r\n"
		"User-Agent: %s\r\n"
		"Accept: text/xml\r\n"
		"\r\n",
		path.c_str(), host.c_str(), port, AWH_NAME
	);
	// Запоминаем шаг обмена по договору UPnP
	this->_stage = stage_t::DESCRIPTION;
	// Выполняем заведение потокового события обмена с устройством
	return this->stream(host, port);
}
/**
 * @brief Метод разбора прочитанного описания устройства UPnP
 *
 */
void awh::unit::Portmap::described() noexcept {
	// Код причины отказа кодека
	proto::portmap::device_t::error_t error = proto::portmap::device_t::error_t::NONE;
	// Разобранное описание устройства
	proto::portmap::device_t::description_t description;
	/**
	 * Если описание устройства разобрать не удалось
	 */
	if(!this->_device.parse(this->_payload, description, error)){
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, error_t::MALFORMED);
		// Завершаем разбор описания устройства
		return;
	}
	/**
	 * Выполняем перебор всех видов службы перенаправления портов
	 *
	 * @note Служба соединения бывает двух видов, и какой из них несёт устройство,
	 *       заранее неизвестно: соединение по адресу IP встречается чаще, но на
	 *       подключении по договору PPP устройство несёт лишь второй
	 */
	for(const char * type : {
		proto::portmap::device_t::SERVICE_WAN_IP,
		proto::portmap::device_t::SERVICE_WAN_IP2,
		proto::portmap::device_t::SERVICE_WAN_PPP
	}){
		// Выполняем отыскание службы перенаправления портов
		const proto::portmap::device_t::service_t * service = this->_device.service(description, type);
		// Если служба перенаправления портов не отыскана
		if(service == nullptr)
			// Выполняем переход к следующему виду службы
			continue;
		// Запоминаем обозначение вида отысканной службы
		this->_service = service->type;
		// Выполняем сборку полного адреса управления отысканной службой
		this->_control = this->_device.address(description, this->_location, service->control);
		// Выходим из перебора видов службы
		break;
	}
	/**
	 * Если служба перенаправления портов устройством не выдаётся
	 */
	if(this->_control.empty()){
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, error_t::NOT_SUPPORTED);
		// Завершаем разбор описания устройства
		return;
	}
	/**
	 * Если вызов действия службы начать не удалось
	 */
	if(!this->control())
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, error_t::NO_RESPONSE);
}
/**
 * @brief Метод вызова действия службы отысканного устройства UPnP
 *
 * @return результат начала вызова действия службы
 *
 */
bool awh::unit::Portmap::control() noexcept {
	// Собираемый вызов действия службы
	proto::portmap::upnp_t::request_t request;
	/**
	 * Определяем просьбу, с которой ведётся обращение
	 */
	switch(static_cast <uint8_t> (this->_action)){
		// Если запрашивается внешний адрес маршрутизатора
		case static_cast <uint8_t> (action_t::EXTERNAL):
			// Выполняем сборку вызова запроса внешнего адреса
			request = this->_upnp.external(this->_service);
		break;
		// Если заводится перенаправление порта
		case static_cast <uint8_t> (action_t::OPEN): {
			// Собираемое перенаправление порта
			proto::portmap::upnp_t::mapping_t mapping;
			// Запоминаем договор перенаправляемого порта
			mapping.proto = ((this->_mapping.proto == proto_t::TCP) ? proto::portmap::upnp_t::proto_t::TCP : proto::portmap::upnp_t::proto_t::UDP);
			// Запоминаем внутренний порт перенаправления
			mapping.internalPort = this->_mapping.internalPort;
			// Запоминаем желаемый внешний порт перенаправления
			mapping.externalPort = (this->_mapping.externalPort > 0 ? this->_mapping.externalPort : this->_mapping.internalPort);
			// Запоминаем запрашиваемый срок жизни перенаправления
			mapping.lifeTime = this->_mapping.lifeTime;
			// Запоминаем описание перенаправления
			mapping.description = this->_mapping.description;
			/**
			 * Получаем внутренний адрес машины, которой отдаются подключения
			 *
			 * @note Договор предписывает указывать в просьбе адрес машины, которой
			 *       отдаются подключения: сам маршрутизатор его не подставляет. Адрес
			 *       берётся у подключённого события обмена: оно уже установило связь с
			 *       устройством, и его адрес - это ровно тот, по которому устройство
			 *       машину и видит
			 */
			mapping.internalClient = this->_io->getAddress(this->_stream, event::address_t::IPV4);
			// Если внутренний адрес машины получить не удалось
			if(mapping.internalClient.empty())
				// Выводим отрицательный результат начала вызова действия службы
				return false;
			// Выполняем сборку вызова заведения перенаправления порта
			request = this->_upnp.add(this->_service, mapping);
		} break;
		// Если убирается заведённое перенаправление порта
		case static_cast <uint8_t> (action_t::CLOSE):
			// Выполняем сборку вызова удаления перенаправления порта
			request = this->_upnp.remove(
				this->_service,
				((this->_mapping.proto == proto_t::TCP) ? proto::portmap::upnp_t::proto_t::TCP : proto::portmap::upnp_t::proto_t::UDP),
				(this->_mapping.externalPort > 0 ? this->_mapping.externalPort : this->_mapping.internalPort)
			);
		break;
	}
	if(!request.valid())
		// Выводим отрицательный результат начала вызова действия службы
		return false;
	// Выполняем очистку объекта работы с адресами ресурсов
	this->_uri.clear();
	/**
	 * Если адрес управления службой разобрать не удалось
	 */
	if(this->_uri.parse(this->_control) == uri_t::type_t::NONE)
		// Выводим отрицательный результат начала вызова действия службы
		return false;
	// Получаем адрес устройства, у которого вызывается действие службы
	const string & host = this->_uri.host();
	// Если адрес устройства получить не удалось
	if(host.empty())
		// Выводим отрицательный результат начала вызова действия службы
		return false;
	// Получаем порт устройства, у которого вызывается действие службы
	const uint16_t port = (this->_uri.port() > 0 ? this->_uri.port() : 80);
	// Выполняем сборку пути вызова действия службы
	string path = this->_uri.print(uri_t::item_t::REQUEST);
	// Если путь вызова действия службы пуст
	if(path.empty())
		// Устанавливаем путь вызова действия службы по умолчанию
		path.assign(1, '/');
	// Выполняем сборку вызова действия службы
	this->_request = this->_fmk->format(
		"POST %s HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Connection: close\r\n"
		"User-Agent: %s\r\n"
		"Content-Type: text/xml; charset=\"utf-8\"\r\n"
		"SOAPAction: %s\r\n"
		"Content-Length: %zu\r\n"
		"\r\n%s",
		path.c_str(), host.c_str(), port, AWH_NAME,
		request.header.c_str(), request.body.length(), request.body.c_str()
	);
	// Запоминаем шаг обмена по договору UPnP
	this->_stage = stage_t::CONTROL;
	// Выполняем заведение потокового события обмена с устройством
	return this->stream(host, port);
}
/**
 * @brief Метод разбора ответа службы устройства UPnP
 *
 */
void awh::unit::Portmap::controlled() noexcept {
	// Код причины отказа кодека
	proto::portmap::soap_t::error_t error = proto::portmap::soap_t::error_t::NONE;
	// Разобранный ответ службы устройства
	proto::portmap::soap_t::answer_t answer;
	/**
	 * Если ответ службы устройства разобрать не удалось
	 */
	if(!this->_soap.parse(this->_payload, answer, error)){
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, error_t::MALFORMED);
		// Завершаем разбор ответа службы устройства
		return;
	}
	/**
	 * Если служба устройства ответила отказом
	 */
	if(answer.fault){
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, this->reason(this->_upnp.result(answer)));
		// Завершаем разбор ответа службы устройства
		return;
	}
	/**
	 * Если запрашивался внешний адрес маршрутизатора
	 */
	if(this->_action == action_t::EXTERNAL){
		// Извлекаемый внешний адрес маршрутизатора
		string address;
		/**
		 * Если внешний адрес маршрутизатора извлечь не удалось
		 */
		if(!this->_upnp.address(answer, address)){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем разбор ответа службы устройства
			return;
		}
		// Выполняем завершение обмена по этому договору
		this->complete(type_t::UPNP);
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const string &, const type_t)> ("external", address, type_t::UPNP);
		// Завершаем разбор ответа службы устройства
		return;
	}
	// Сохраняем просьбу, с которой велось обращение
	const action_t action = this->_action;
	// Выполняем завершение обмена по этому договору
	this->complete(type_t::UPNP);
	/**
	 * Определяем просьбу, с которой велось обращение
	 */
	switch(static_cast <uint8_t> (action)){
		// Если заводилось перенаправление порта
		case static_cast <uint8_t> (action_t::OPEN):
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const mapping_t &, const type_t)> ("mapped", this->_mapping, type_t::UPNP);
		break;
		// Если убиралось заведённое перенаправление порта
		case static_cast <uint8_t> (action_t::CLOSE):
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const mapping_t &, const type_t)> ("unmapped", this->_mapping, type_t::UPNP);
		break;
	}
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
		exchange_t & exchange = this->exchange(type);
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
		exchange_t & exchange = this->exchange(type);
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
	 * Если обмен ведётся договором UPnP
	 *
	 * @note Отыскание маршрутизатора договору UPnP не нужно: устройство отыскивается
	 *       рассылкой SSDP, и заданный настройкой адрес маршрутизатора здесь ни при чём
	 */
	if((this->_type == type_t::UPNP) || (this->_type == type_t::AUTO))
		// Выполняем отыскание устройства рассылкой SSDP
		result = (this->search() || result);
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
	 * Если обмен вёлся по одному из договоров перенаправления
	 */
	if((type == type_t::PCP) || (type == type_t::UPNP) || (type == type_t::NAT_PMP)){
		// Получаем обмен, по которому вёлся обмен
		exchange_t & exchange = this->exchange(type);
		// Снимаем признак ожидания ответа маршрутизатора
		exchange.waiting = false;
		// Если событие обмена заведено
		if(exchange.eid > 0)
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
		// Сбрасываем идентификатор события обмена
		exchange.eid = 0;
		/**
		 * Если обмен вёлся по договору UPnP
		 */
		if(type == type_t::UPNP){
			// Если потоковое событие обмена заведено
			if(this->_stream > 0)
				// Выполняем удаление потокового события обмена
				this->_io->destroy(this->_stream);
			// Сбрасываем идентификатор потокового события обмена
			this->_stream = 0;
			// Сбрасываем шаг обмена по договору UPnP
			this->_stage = stage_t::NONE;
		}
	}
	/**
	 * Если обмен ведётся хотя бы по одному договору
	 *
	 * @note Под видом опроса AUTO договоров ведётся несколько разом, и отказ одного из
	 *       них ещё не означает отказа обращения: оно завершается, когда откажут все
	 */
	if(this->_exchangePCP.waiting || this->_exchangeUPNP.waiting || this->_exchangeNATPMP.waiting)
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
	for(exchange_t * exchange : {&this->_exchangePCP, &this->_exchangeUPNP, &this->_exchangeNATPMP}){
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
	/**
	 * Если потоковое событие обмена с устройством UPnP заведено
	 */
	if(this->_stream > 0){
		// Выполняем удаление потокового события обмена
		this->_io->destroy(this->_stream);
		// Сбрасываем идентификатор потокового события обмена
		this->_stream = 0;
	}
	// Сбрасываем шаг обмена по договору UPnP
	this->_stage = stage_t::NONE;
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
 _attempts(::DEFAULT_ATTEMPTS), _delay(::DEFAULT_DELAY),
 _stage(stage_t::NONE), _stream(0), _port(0), _host{""}, _location{""}, _control{""}, _service{""},
 _request{""}, _payload{""}, _complete(false), _uri(fmk, log), _router{""},
 _address(nullptr), _addr(fmk, log), _gateway(fmk, log), _pcp(fmk, log),
 _ssdp(fmk, log), _soap(fmk, log), _upnp(fmk, log), _device(fmk, log), _natpmp(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Portmap::~Portmap() noexcept {
	// Выполняем прекращение всех ведущихся обменов
	this->cancel();
}
