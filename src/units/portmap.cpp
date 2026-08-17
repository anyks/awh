/**
 * @file portmap.cpp
 * @date 2026-08-02
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
 * @brief Модуль перенаправления портов — класс unit::Portmap
 *
 * @copyright Copyright © 2026
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
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
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
	 * @brief Пауза перед повторной попыткой подключения к устройству UPnP (мсек)
	 *
	 * @note Неудачей здесь оборачивается потерянное рукопожатие, а не занятость
	 *       устройства: ждать долго незачем, новая попытка обходится дешевле
	 *
	 */
	constexpr uint32_t RECONNECT_DELAY = 100;

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
	 * @brief Срок жизни пробоя заслона IPv6 по умолчанию в секундах
	 *
	 * @note Бессрочных пробоев договор UPnP не заводит вовсе: срок обязателен, и при
	 *       незаданном берётся этот
	 *
	 */
	constexpr uint32_t DEFAULT_LIFETIME = 0xE10;

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
 proto(proto_t::NONE), internalPort(0), externalPort(0), lifeTime(0), description{""},
 pinhole(0), enabled(true), remoteHost{""}, internalClient{""} {}

/**
 * @brief Конструктор
 *
 */
awh::unit::Portmap::Exchange::Exchange() noexcept : waiting(false), attempt(0), eid(0) {}

/**
 * @brief Конструктор
 *
 */
awh::unit::Portmap::Epoch::Epoch() noexcept : filled(false), value(0), stamp(0) {}

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
				 * Выполняем сверку отсчёта времени работы маршрутизатора
				 *
				 * @note Сверяется отсчёт у каждого разобранного ответа, а не только у
				 *       ответа на свою просьбу: договор предписывает именно так, и ответ
				 *       на чужую просьбу приходит от того же маршрутизатора. Обмена сверка
				 *       не прерывает - об утрате состояния она лишь объявляет
				 */
				this->lost(type, answer.epoch);
				/**
				 * Если ответ отправленной просьбе не принадлежит
				 *
				 * @note Ответ приходит на общий порт договора, и прийти он может на чужую
				 *       просьбу. Сличает их кодек: он сверяет и действие, и отличительную
				 *       метку, и внутренний порт. Отказом это не считается - ждём своего
				 *       ответа
				 */
				if(!this->_pcp.belongs(answer, this->_inquiry)){
					/**
					 * Выполняем продолжение прерванного ожидания ответа
					 *
					 * @note Приходом чужой дейтаграммы движок ожидание снял, и не продолжи
					 *       мы его здесь, обмен остался бы без срока вовсе: отправлять
					 *       повторно на этом шаге нечего, а иного срабатывания не будет.
					 *       Продолжается ожидание остатком, а не заново - сколько бы чужих
					 *       ответов ни пришло, отказ наступит в свой черёд
					 */
					this->_io->rearmTimeout(eid, event::action_t::READ);
					// Ожидаем ответа на свою просьбу
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
				 * @note Назначенное маршрутизатором перенимается при заведении и продлении,
				 *       но не при удалении: ответ на удаление несёт по договору нулевой
				 *       внешний порт и нулевой срок, и перенять их значило бы выдать в итоге
				 *       не то перенаправление, которое убиралось. Продление же назначает
				 *       новый срок, и выдать вызывающему запрошенный вместо назначенного
				 *       значило бы соврать о том, до каких пор перенаправление живёт
				 */
				if((this->_action == action_t::OPEN) || (this->_action == action_t::RENEW)){
					// Запоминаем назначенный маршрутизатором внешний порт
					this->_mapping.externalPort = answer.externalPort;
					// Запоминаем назначенный маршрутизатором срок жизни перенаправления
					this->_mapping.lifeTime = answer.lifeTime;
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
				if(!this->_ssdp.parse(string_view(reinterpret_cast <const char *> (data), size), answer, error)){
					/**
					 * Выполняем продолжение прерванного ожидания ответа
					 *
					 * @note Рассылка ведётся на групповой адрес, и ответы посторонних
					 *       устройств приходят по ней всякий раз: не продолжи мы ожидание,
					 *       первое же чужое сообщение оставило бы отыскание без срока
					 */
					this->_io->rearmTimeout(eid, event::action_t::READ);
					// Ожидаем ответа пригодного устройства
					return;
				}
				/**
				 * Если ответ устройству доступа в сеть не отвечает
				 */
				if(!this->_ssdp.suitable(answer, proto::portmap::ssdp_t::TARGET_GATEWAY)){
					// Выполняем продолжение прерванного ожидания ответа
					this->_io->rearmTimeout(eid, event::action_t::READ);
					// Ожидаем ответа пригодного устройства
					return;
				}
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
				 * Выполняем сверку отсчёта времени работы маршрутизатора
				 *
				 * @note Сверяется отсчёт у каждого разобранного ответа, как и по договору
				 *       PCP: отсчёт несут оба вида ответа этого договора - и о внешнем
				 *       адресе, и о перенаправлении
				 */
				this->lost(type, answer.epoch);
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
				 * Если ответ отправленной просьбе не принадлежит
				 *
				 * @details Отличительной метки договор не имеет вовсе, и опознаётся ответ
				 *          по внутреннему порту и договору перенаправления: их маршрутизатор
				 *          повторяет в ответе именно затем. Ответ приходит на открытый порт,
				 *          и прийти он может на чужую просьбу
				 *
				 * @note Отказом это не считается - ждём своего ответа, как и по договору PCP
				 */
				if((answer.internalPort != this->_mapping.internalPort) ||
				   (answer.proto != ((this->_mapping.proto == proto_t::TCP) ? proto::portmap::natpmp_t::proto_t::TCP : proto::portmap::natpmp_t::proto_t::UDP))){
					// Выполняем продолжение прерванного ожидания ответа
					this->_io->rearmTimeout(eid, event::action_t::READ);
					// Ожидаем ответа на свою просьбу
					return;
				}
				/**
				 * Если заводилось перенаправление порта
				 *
				 * @note Назначенное маршрутизатором перенимается при заведении и продлении,
				 *       но не при удалении: ответ на удаление несёт по договору нулевой
				 *       внешний порт и нулевой срок, и перенять их значило бы выдать в итоге
				 *       не то перенаправление, которое убиралось. Продление же назначает
				 *       новый срок, и выдать вызывающему запрошенный вместо назначенного
				 *       значило бы соврать о том, до каких пор перенаправление живёт
				 */
				if((this->_action == action_t::OPEN) || (this->_action == action_t::RENEW)){
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
			// Если продлевался срок заведённого перенаправления
			case static_cast <uint8_t> (action_t::RENEW):
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
bool awh::unit::Portmap::timeout(const event::id_t eid, const event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept {
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
	 * Если срок ожидания истёк у потокового события обмена с устройством UPnP
	 *
	 */
	if((this->_stream > 0) && (eid == this->_stream)){
		/**
		 * Определяем действие, у которого истёк срок ожидания
		 */
		switch(static_cast <uint8_t> (action)){
			/**
			 * Если подошла пора повторить попытку подключения к устройству
			 *
			 * @warning Смысл ответа здесь обратный прочим действиям: положительный
			 *          означает «переподключаться», а отрицательным попытка
			 *          прерывается. Счёт попыток ведётся выше, и, исчерпав их, модуль
			 *          завершает обмен отказом - тем череда и прекращается
			 */
			case static_cast <uint8_t> (event::action_t::RECONNECT):
				// Разрешаем событию повторить попытку подключения к устройству
				return true;
			/**
			 * Если устройство не ответило на отправленный запрос
			 *
			 * @note Подключение при этом живо, и само по себе оно не оборвётся: движок
			 *       заводит новую попытку по обрыву, а не по молчанию. Не повторить
			 *       запрос здесь значило бы оставить обмен без итога вовсе - ни ответа,
			 *       ни отказа
			 */
			case static_cast <uint8_t> (event::action_t::READ): {
				// Если повторить текущий шаг обмена не удалось
				if(!this->repeat())
					// Выполняем завершение обмена отказом
					this->failure(type, error_t::NO_RESPONSE);
				// Запрещаем завершение потокового события после истечения срока ожидания
				return false;
			}
		}
		/**
		 * Запрещаем завершение потокового события после истечения срока ожидания
		 *
		 * @note Новую попытку подключения событие заводит само: неудавшееся подключение
		 *       движок прерывает и заводит срок ожидания повторной попытки, а по нему
		 *       приходит уже действие переподключения
		 */
		return false;
	}
	/**
	 * Если срок ожидания истёк у рассылки обнаружения устройства UPnP
	 *
	 * @note Повторяется здесь рассылка, а не просьба маршрутизатору: просьбы собирает
	 *       кодек договора, а у UPnP на этом шаге просьбы нет вовсе - есть рассылка,
	 *       и заводит её отдельный метод. Повтор ей нужен не меньше прочих: рассылка
	 *       эта дейтаграммная и групповая, а такую потерять проще всего
	 */
	if(type == type_t::UPNP){
		/**
		 * Если разослать просьбу обнаружения повторно не удалось
		 */
		if(!this->search())
			// Выполняем завершение обмена отказом
			this->failure(type, error_t::NO_RESPONSE);
		// Запрещаем завершение события после истечения срока ожидания
		return false;
	}
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
		// Если у маршрутизатора не осталось места под пробои заслона IPv6
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::PINHOLE_EXHAUSTED):
			// Выводим отказ по исчерпанию места под перенаправления
			return error_t::OUT_OF_RESOURCES;
		/**
		 * Если заслон IPv6 отключён либо пробои его запрещены
		 *
		 * @note Отказ этот означает не ошибку просьбы, а отключённое устройством
		 *       средство: просить его снова тем же способом бесполезно
		 */
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::FIREWALL_DISABLED):
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::PINHOLE_NOT_ALLOWED):
			// Выводим отказ по неподдерживаемому договору
			return error_t::NOT_SUPPORTED;
		// Если договор пробоя маршрутизатором не поддерживается
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::PROTO_NOT_SUPPORTED):
			// Выводим отказ по неподдерживаемому договору
			return error_t::NOT_SUPPORTED;
		/**
		 * Если пустые признаки пробоя заслона IPv6 не допускаются
		 */
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::INT_PORT_NOT_WILDCARD):
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::PROTO_NOT_WILDCARD):
		case static_cast <uint32_t> (proto::portmap::upnp_t::result_t::SRC_NOT_WILDCARD):
			// Выводим отказ по ошибочно построенному запросу
			return error_t::MALFORMED;
	}
	// Выводим отказ по иной причине
	return error_t::REFUSED;
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
	if(!this->_io->getAddress(eid, ((this->_family == family_t::IPV6) ? event::address_t::IPV6 : event::address_t::IPV4), value) || (value == nullptr))
		// Выводим отрицательный результат получения адреса
		return false;
	// Выполняем размещение полученного адреса
	this->_addr.source(value.get());
	/**
	 * Выполняем размещение внутреннего адреса машины записью IPv6, предписанной договором
	 *
	 * @note Объект работы с адресами переводит записи между разновидностями сам: адрес
	 *       IPv4 он отдаёт отображением на пространство IPv6, а настоящий адрес IPv6 -
	 *       как есть, и оба вида договор принимает без различия
	 */
	const auto & result = this->_addr.v6(net_addr_t::endian_t::LITTLE);
	// Выполняем копирование полученного адреса
	::memcpy(address, result.data(), result.size());
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
	// Сбрасываем адрес маршрутизатора, отысканный прежде
	this->_address = nullptr;
	/**
	 * Если адрес маршрутизатора задан настройкой
	 */
	if(!this->_router.empty()){
		/**
		 * Если заданный адрес маршрутизатора разобрать не удалось
		 */
		if(!this->_addr.parse(this->_router))
			// Выводим отрицательный результат отыскания маршрутизатора
			return false;
		/**
		 * Если заданный адрес маршрутизатора разновидности обмена не отвечает
		 *
		 * @note Обмен ведётся целиком одной разновидностью, и адрес другой к
		 *       маршрутизатору не приведёт: событие обмена заводится разновидностью
		 *       настройки, и отправка по чужому адресу отказала бы уже в системе
		 */
		if(this->_addr.type() != ((this->_family == family_t::IPV6) ? net_addr_t::type_t::IPV6 : net_addr_t::type_t::IPV4)){
			// Записываем в лог сообщение о непригодном адресе маршрутизатора
			this->_log->print("Router address \"%s\" does not match the network family in use", log_t::flag_t::WARNING, this->_router.c_str());
			// Выводим отрицательный результат отыскания маршрутизатора
			return false;
		}
		/**
		 * Запоминаем заданный настройкой адрес маршрутизатора
		 *
		 * @note Адрес снимается готовым объектом, а не собирается по числу: разбирать
		 *       его вид и раскладывать октеты объект работы с адресами умеет сам
		 */
		this->_address = this->_addr.source();
		// Выводим результат отыскания маршрутизатора
		return (this->_address != nullptr);
	}
	// Маршрут до внешней сети
	eth::gateway_t::route_t route{};
	/**
	 * Выполняем размещение адреса маршрутизатора в маршруте
	 *
	 * @note Разновидность отыскиваемого маршрута задаётся размещённым адресом: система
	 *       держит таблицы маршрутов раздельно, и маршрут разновидности обмена берётся
	 *       из своей
	 */
	if(this->_family == family_t::IPV6)
		// Выполняем размещение адреса маршрутизатора записью IPv6
		route.gateway = make_unique <net::addr_net_ipv6_t> ();
	// Выполняем размещение адреса маршрутизатора записью IPv4
	else route.gateway = make_unique <net::addr_net_ipv4_t> ();
	/**
	 * Если маршрут до внешней сети получить не удалось
	 */
	if(!this->_gateway.get(route))
		// Выводим отрицательный результат отыскания маршрутизатора
		return false;
	/**
	 * Запоминаем сетевое устройство, которым ведётся обмен
	 *
	 * @note Устройство нужно рассылке SSDP в сети IPv6, и берётся оно отсюда: маршрут
	 *       до внешней сети ведёт через маршрутизатор, а он и есть то устройство, с
	 *       которым ведётся обмен
	 */
	this->_link = route.ifname;
	/**
	 * Запоминаем адрес маршрутизатора, взятый из таблицы маршрутов
	 *
	 * @note Адрес маршрута перенимается целиком, а не по числу: разновидность его
	 *       задаёт сам маршрут, и разбирать её вызывающему незачем
	 */
	this->_address = ::move(route.gateway);
	// Выводим результат отыскания маршрутизатора
	return (this->_address != nullptr);
}
/**
 * @brief Метод получения сетевого устройства, которым ведётся обмен
 *
 * @return название сетевого устройства
 *
 */
const string & awh::unit::Portmap::iface() const noexcept {
	// Выводим устройство, заданное настройкой, либо взятое из маршрута до внешней сети
	return (!this->_iface.empty() ? this->_iface : this->_link);
}
/**
 * @brief Метод вступления события рассылки в группу обнаружения устройств
 *
 * @param group групповой адрес обнаружения устройств
 * @param six   признак того, что обмен ведётся сетью IPv6
 * @return      результат вступления в группу обнаружения
 *
 */
bool awh::unit::Portmap::membership(const string & group, const bool six) noexcept {
	/**
	 * Если обмен ведётся сетью IPv4
	 *
	 * @note Устройство подписки в сети IPv4 задаётся неопределённым адресом: система
	 *       выбирает его сама по таблице маршрутов
	 */
	if(!six)
		// Выполняем вступление в группу обнаружения устройств
		return this->_io->membership(this->_exchangeUPNP.eid, event::mode_t::ENABLED, group, "0.0.0.0");
	// Получаем сетевое устройство, которым ведётся обмен
	const string & iface = this->iface();
	/**
	 * Если сетевое устройство обмена неизвестно
	 *
	 * @note Выбор устройства подписки оставляется системе: с исправно заведённой сетью
	 *       IPv6 она справляется с этим сама
	 */
	if(iface.empty())
		// Выполняем вступление в группу обнаружения устройств
		return this->_io->membership(this->_exchangeUPNP.eid, event::mode_t::ENABLED, group, "::");
	/**
	 * Получаем адрес сетевого устройства, которым ведётся обмен
	 *
	 * @note Устройство подписки задаётся тем же, которым уходит рассылка, а не
	 *       выбирается системой: групповой адрес в сети IPv6 принадлежит связи, и
	 *       подписка на чужой связи оставила бы рассылку без ответа. Неопределённый
	 *       адрес система принимает и сама, когда путь IPv6 у неё есть, но
	 *       полагаться на её выбор здесь незачем - устройство уже назначено
	 */
	const unique_ptr <net::addr_t> address = this->_ifaces.getAddress(iface, event::family_t::IPV6);
	// Если адрес сетевого устройства получить не удалось
	if(address == nullptr)
		// Выводим отрицательный результат вступления в группу обнаружения
		return false;
	// Выполняем разбор группового адреса обнаружения устройств
	if(!this->_addr.parse(group))
		// Выводим отрицательный результат вступления в группу обнаружения
		return false;
	// Получаем групповой адрес обнаружения устройств
	const unique_ptr <net::addr_t> value = this->_addr.source();
	// Выполняем вступление в группу обнаружения устройств
	return this->_io->membership(this->_exchangeUPNP.eid, event::mode_t::ENABLED, value.get(), address.get());
}
/**
 * @brief Метод проверки принадлежности адреса локальной сети
 *
 * @param host проверяемый адрес устройства
 * @return     признак принадлежности адреса локальной сети
 *
 */
bool awh::unit::Portmap::local(const string & host) noexcept {
	/**
	 * Если адрес локальной сети не принадлежит
	 */
	if(!this->_addr.parse(host) || (this->_addr.own() != net_addr_t::own_t::LAN)){
		// Записываем в лог сообщение о непригодном адресе устройства
		this->_log->print("Device address \"%s\" does not belong to the local network", log_t::flag_t::WARNING, host.c_str());
		// Выводим признак того, что адрес локальной сети не принадлежит
		return false;
	}
	// Выводим признак того, что адрес локальной сети принадлежит
	return true;
}
/**
 * @brief Метод сборки узла для заголовка запроса к устройству UPnP
 *
 * @param host адрес устройства, с которым ведётся обмен
 * @param port порт устройства, с которым ведётся обмен
 * @return     собранный узел для заголовка запроса
 *
 */
string awh::unit::Portmap::authority(const string & host, const uint16_t port) const noexcept {
	/**
	 * Если адрес устройства записан видом IPv6
	 *
	 * @note Проверка ведётся по самой записи, а не по разновидности обмена: узел этот
	 *       назван устройством, и записан он так, как записан
	 */
	if(host.find(':') != string::npos)
		// Выводим узел записью IPv6, взятой в квадратные скобки
		return this->_fmk->format("[%s]:%u", host.c_str(), port);
	// Выводим узел обычной записью
	return this->_fmk->format("%s:%u", host.c_str(), port);
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
	 * Если обмен ведётся сетью IPv4
	 *
	 * @note Группа обнаружения в сети IPv4 одна, и перебирать здесь нечего
	 */
	if(this->_family != family_t::IPV6)
		// Выполняем отыскание устройства рассылкой на группу обнаружения
		return this->search(proto::portmap::ssdp_t::MULTICAST_ADDRESS);
	/**
	 * Группы обнаружения устройств сети IPv6
	 *
	 * @details Групп в сети IPv6 две: одна принадлежит связи, другая - площадке.
	 *          Устройство доступа в сеть лежит на той же связи и обычно отвечает в
	 *          первую, но объявлять себя лишь во второй ему не запрещено
	 */
	static constexpr const char * GROUPS[] = {
		proto::portmap::ssdp_t::MULTICAST_ADDRESS6,
		proto::portmap::ssdp_t::MULTICAST_ADDRESS6_SITE
	};
	// Получаем количество групп обнаружения устройств
	constexpr size_t count = (sizeof(GROUPS) / sizeof(GROUPS[0]));
	/**
	 * Получаем группу, отвечающую порядковому номеру попытки
	 *
	 * @details Группа берётся по номеру попытки, а не перебором до первой удавшейся:
	 *          удаётся рассылка почти всегда - отказывает она разве что при неисправной
	 *          сети, - и перебор обрывался бы на ближней группе, не дойдя до дальней
	 *          ни разу. Пробовать дальнюю следует, когда ближняя не ответила, а узнаётся
	 *          это лишь истечением срока, то есть на следующей попытке
	 *
	 * @note Первая попытка достаётся ближней группе: устройство доступа в сеть лежит на
	 *       той же связи, и отвечает оно обычно в неё
	 */
	const size_t offset = static_cast <size_t> (this->_exchangeUPNP.attempt);
	/**
	 * Выполняем перебор групп обнаружения устройств, начиная с группы этой попытки
	 */
	for(size_t i = 0; i < count; i++){
		// Если отыскание устройства рассылкой начать удалось
		if(this->search(GROUPS[(offset + i) % count]))
			// Выводим положительный результат начала отыскания устройства
			return true;
	}
	// Выводим отрицательный результат начала отыскания устройства
	return false;
}
/**
 * @brief Метод отыскания устройства UPnP рассылкой SSDP на заданную группу
 *
 * @param group групповой адрес обнаружения устройств
 * @return      результат начала отыскания устройства
 *
 */
bool awh::unit::Portmap::search(string_view group) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Признак того, что обмен ведётся сетью IPv6
		const bool six = (this->_family == family_t::IPV6);
		/**
		 * Собираемый адрес группы обнаружения устройств
		 *
		 * @note Групповой адрес сети IPv6 принадлежит связи, и без указания устройства
		 *       он неоднозначен: система такую рассылку отвергает, не имея, каким
		 *       устройством её вести
		 */
		string target(group.begin(), group.end());
		/**
		 * Если обмен ведётся сетью IPv6 и сетевое устройство обмена известно
		 *
		 * @note Неизвестное устройство рассылку не отменяет: систему с исправно
		 *       заведённой сетью IPv6 зона не заботит - она выбирает устройство сама
		 *       по таблице маршрутов. Отменять рассылку значило бы отказывать там, где
		 *       она прошла бы
		 */
		if(six && !this->iface().empty())
			// Выполняем дописывание зоны группового адреса
			target.append(1, '%').append(this->iface());
		// Если событие обмена уже заведено
		if(this->_exchangeUPNP.eid > 0)
			// Выполняем удаление события обмена
			this->_io->destroy(this->_exchangeUPNP.eid);
		// Выполняем заведение события рассылки
		this->_exchangeUPNP.eid = this->_io->event(event::node_t::CLIENT, (six ? event::family_t::IPV6 : event::family_t::IPV4), event::type_t::DATAGRAM, event::protocol_t::UDP);
		/**
		 * Если сетевое устройство обмена задано
		 *
		 * @note Устройство задаётся и рассылке IPv4: сеть бывает не одна, и рассылать
		 *       просьбу следует той, где лежит маршрутизатор
		 */
		if(!this->iface().empty()){
			/**
			 * Если установить сетевое устройство события рассылки не удалось
			 *
			 * @details Отказ разбирается по тому, откуда устройство взялось. Названное
			 *          настройкой - указание вызывающего: он выбрал сеть, в которой
			 *          маршрутизатор ищется, и разослать просьбу в другую значило бы
			 *          подменить его выбор молча. Взятое же из маршрута до внешней сети -
			 *          не указание, а подсказка: рассылка и без неё уходит тем же
			 *          маршрутом, и отказывать по ней значило бы закрывать дорогу там,
			 *          где она открыта
			 *
			 * @note Отказ этот прежде не разбирался вовсе, и рассылка при нём уходила
			 *       устройством, выбранным таблицей маршрутов, - о чём вызывающий не
			 *       узнавал никак
			 */
			if(!this->_io->setIface(this->_exchangeUPNP.eid, this->iface()) && !this->_iface.empty()){
				// Выполняем удаление события рассылки
				this->_io->destroy(this->_exchangeUPNP.eid);
				// Сбрасываем идентификатор события рассылки
				this->_exchangeUPNP.eid = 0;
				// Выводим отрицательный результат начала отыскания устройства
				return false;
			}
		}
		/**
		 * Если групповой режим события установить не удалось
		 */
		if(!this->_io->setDelivery(this->_exchangeUPNP.eid, event::delivery_mode_t::MULTICAST)){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Сбрасываем идентификатор события рассылки
			this->_exchangeUPNP.eid = 0;
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		/**
		 * Если предел числа переходов рассылки установить не удалось
		 *
		 * @note Рассылка обнаружения дальше своей сети не уходит: устройство доступа в
		 *       сеть лежит на ней же, а разослать просьбу шире значило бы беспокоить чужие.
		 *       Предел этот берётся настройкой (setHops), потому что держащему устройство
		 *       доступа на самой машине нужен предел иной - LOOPBACK, не выпускающий
		 *       просьбу наружу вовсе
		 */
		if(!this->_io->setHops(this->_exchangeUPNP.eid, this->_hops)){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Сбрасываем идентификатор события рассылки
			this->_exchangeUPNP.eid = 0;
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		/**
		 * Если порт обнаружения устройств установить не удалось
		 */
		if(!this->_io->setTargetPort(this->_exchangeUPNP.eid, proto::portmap::ssdp_t::PORT)){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Сбрасываем идентификатор события рассылки
			this->_exchangeUPNP.eid = 0;
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		/**
		 * Если групповой адрес обнаружения устройств установить не удалось
		 */
		if(!this->_io->setTarget(this->_exchangeUPNP.eid, target)){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Сбрасываем идентификатор события рассылки
			this->_exchangeUPNP.eid = 0;
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
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
			// Сбрасываем идентификатор события рассылки
			this->_exchangeUPNP.eid = 0;
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		/**
		 * Если вступление в группу обнаружения устройств выполнить не удалось
		 *
		 * @note Отказ обмен не прерывает: ответ на просьбу обнаружения устройство шлёт
		 *       одному спросившему, а не в группу, и приходит он обычным чтением
		 *       события. Членство нужно лишь для объявлений, которых модуль не ждёт
		 */
		if(!this->membership(target, six))
			// Записываем в лог сообщение о неудавшемся вступлении в группу обнаружения
			this->_log->print("Joining the discovery group \"%s\" failed, awaiting the unicast answer", log_t::flag_t::WARNING, target.c_str());
		/**
		 * Если запустить событие рассылки не удалось
		 */
		if(!(this->_io->commit(this->_exchangeUPNP.eid) && this->_io->launch(this->_exchangeUPNP.eid))){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Сбрасываем идентификатор события рассылки
			this->_exchangeUPNP.eid = 0;
			// Выводим отрицательный результат начала отыскания устройства
			return false;
		}
		// Выполняем сборку просьбы обнаружения устройства доступа в сеть
		const string & message = this->_ssdp.search(proto::portmap::ssdp_t::TARGET_GATEWAY, proto::portmap::ssdp_t::DEFAULT_DELAY, group);
		/**
		 * Если просьбу разослать не удалось
		 */
		if(this->_io->send(this->_exchangeUPNP.eid, message.data(), message.length()) == 0){
			// Выполняем удаление события рассылки
			this->_io->destroy(this->_exchangeUPNP.eid);
			// Сбрасываем идентификатор события рассылки
			this->_exchangeUPNP.eid = 0;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(group), log_t::flag_t::CRITICAL, error.what());
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
		// Выполняем подготовку объекта разбора к чтению ответа устройства
		this->prepare();
		// Снимаем признак того, что потоковое событие подключено
		this->_connected = false;
		// Выполняем заведение потокового события обмена
		/**
		 * Определяем разновидность сети по адресу устройства
		 *
		 * @note Разновидность берётся у самого адреса, а не у настройки: адрес этот
		 *       назван устройством в ответе на рассылку, и подключаться к нему следует
		 *       той разновидностью, какой он записан
		 */
		const bool six = (this->_addr.parse(address) && (this->_addr.type() == net_addr_t::type_t::IPV6));
		// Выполняем заведение потокового события обмена
		this->_stream = this->_io->event(event::node_t::CLIENT, (six ? event::family_t::IPV6 : event::family_t::IPV4), event::type_t::STREAM, event::protocol_t::TCP);
		/**
		 * Если порт устройства установить не удалось
		 */
		if(!this->_io->setTargetPort(this->_stream, port)){
			// Выполняем удаление потокового события обмена
			this->_io->destroy(this->_stream);
			// Сбрасываем идентификатор потокового события обмена
			this->_stream = 0;
			// Выводим отрицательный результат заведения потокового события обмена
			return false;
		}
		/**
		 * Если адрес устройства установить не удалось
		 */
		if(!this->_io->setTarget(this->_stream, address)){
			// Выполняем удаление потокового события обмена
			this->_io->destroy(this->_stream);
			// Сбрасываем идентификатор потокового события обмена
			this->_stream = 0;
			// Выводим отрицательный результат заведения потокового события обмена
			return false;
		}
		// Устанавливаем срок ожидания ответа устройства
		this->_io->setTimeout(this->_stream, event::action_t::READ, this->_delay);
		/**
		 * Устанавливаем срок ожидания подключения к устройству
		 *
		 * @note Без него неудачная попытка подключения висит столько, сколько отведёт
		 *       система, и обмен молчит вместо того, чтобы завершиться отказом
		 */
		this->_io->setTimeout(this->_stream, event::action_t::CONNECT, this->_delay);
		/**
		 * Разрешаем событию самостоятельное переподключение
		 *
		 * @note Опции `AUTO_RECONNECT` мало: движок ведёт переподключение, только
		 *       когда оно событию ещё и разрешено действием. Мало и этого - согласие
		 *       спрашивается ещё и у подписки на истечение срока, и там смысл ответа
		 *       обратный прочим действиям: отрицательным попытка прерывается
		 */
		this->_io->setAction(this->_stream, event::action_t::RECONNECT, event::mode_t::ENABLED);
		/**
		 * Устанавливаем паузу перед повторной попыткой подключения
		 *
		 * @note Без неё берётся пять секунд, а ждать столько незачем: неудачей здесь
		 *       оборачивается потерянное рукопожатие, и лечится она новой попыткой, а
		 *       не терпением
		 */
		this->_io->setTimeout(this->_stream, event::action_t::RECONNECT, RECONNECT_DELAY);
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
		if(!this->_io->setOptions(this->_stream, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY | event::options::AUTO_RECONNECT)){
			// Выполняем удаление потокового события обмена
			this->_io->destroy(this->_stream);
			// Сбрасываем идентификатор потокового события обмена
			this->_stream = 0;
			// Выводим отрицательный результат заведения потокового события обмена
			return false;
		}
		/**
		 * Если запустить потоковое событие обмена не удалось
		 */
		if(!(this->_io->commit(this->_stream) && this->_io->connect(this->_stream) && this->_io->launch(this->_stream))){
			// Выполняем удаление потокового события обмена
			this->_io->destroy(this->_stream);
			// Сбрасываем идентификатор потокового события обмена
			this->_stream = 0;
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
 * @brief Метод подготовки объекта разбора к чтению ответа устройства UPnP
 *
 * @details Объект разбора живёт весь обмен и переиспользуется: сбрасывать его перед
 *          каждым ответом обязательно, иначе следующий ответ разбирался бы поверх
 *          состояния предыдущего
 *
 */
void awh::unit::Portmap::prepare() noexcept {
	// Выполняем очистку собираемого тела ответа устройства
	this->_payload.clear();
	// Снимаем признак того, что ответ устройства прочитан до конца
	this->_complete = false;
	// Выполняем сброс объекта разбора ответа устройства
	this->_parser.clear();
	/**
	 * Устанавливаем функцию обратного вызова на получение тела ответа
	 *
	 * @note Тело копится подпиской, а не берётся у объекта разбора по завершении: ответ
	 *       приходит кусками, и собрать его иначе неоткуда
	 */
	this->_parser.on(http::parser_http_t::data_callback_t([this](const uint32_t, const void * data, const size_t size, const bool) noexcept -> bool {
		// Выполняем накопление полученного тела ответа
		this->_payload.append(reinterpret_cast <const char *> (data), size);
		// Разрешаем продолжение разбора
		return true;
	}));
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
	 *
	 * @note Обмен здесь не завершается: об этом же извещает и срок ожидания
	 *       подключения, приходящий следом, и повтор ведётся там - по общему счёту
	 *       попыток, как и для прочих договоров
	 */
	if(!ok)
		// Завершаем обработку подключения
		return;
	// Запоминаем, что потоковое событие к устройству подключено
	this->_connected = true;
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
		/**
		 * Выполняем разбор полученных данных
		 *
		 * @note Парсеру подаётся лишь пришедший кусок: своё состояние он держит между
		 *       чтениями сам, а тело ответа копится подпиской, выставленной при заведении
		 *       потокового события обмена. Подавать ему заново весь накопленный ответ на
		 *       каждый пришедший кусок значило бы разбирать ответ столько раз, сколько
		 *       кусков пришло
		 */
		this->_parser.parse(data, size);
		/**
		 * Если ответ устройства прочитан не до конца
		 *
		 * @note Завершённость определяется итоговым состоянием разбора, а не стадией
		 *       END: стадия объявляется и на конце блока заголовков, и лишь состояние
		 *       COMPLETE говорит о том, что сообщение прочитано целиком
		 */
		if(this->_parser.status() != http::parser_http_t::status_t::COMPLETE){
			/**
			 * Выполняем продолжение прерванного ожидания ответа
			 *
			 * @note Ответ устройства приходит кусками, и каждый из них ожидание снимает.
			 *       Не продолжи мы его здесь, весь остаток ответа шёл бы без срока вовсе:
			 *       устройство, оборвавшее передачу посередине, держало бы обмен молча и
			 *       без конца. Продолжается ожидание остатком - предел кладётся на ответ
			 *       целиком, а не на каждый его кусок порознь
			 */
			this->_io->rearmTimeout(eid, event::action_t::READ);
			// Ожидаем поступления остатка ответа
			return;
		}
		// Запоминаем, что ответ устройства прочитан до конца
		this->_complete = true;
		/**
		 * Запоминаем, держит ли устройство соединение открытым
		 *
		 * @note Решает это устройство, а не мы: просьбу `keep-alive` оно вправе не
		 *       принять, и часть устройств соединение закрывает всегда. Дальше идти по
		 *       живому соединению можно лишь с его согласия, а без него событие
		 *       заводится заново
		 */
		this->_connected = this->_parser.message().flags.keepAlive;
		/**
		 * Сбрасываем счёт выполненных попыток обращения к устройству
		 *
		 * @note Счёт ведётся на попытки подряд, а не на весь обмен: чтение перечня
		 *       требует обращения на каждое перенаправление, и накопленные за весь
		 *       обход неудачи обрывали бы его тем скорее, чем перечень длиннее
		 */
		this->exchange(type_t::UPNP).attempt = 0;
		/**
		 * Если разбор ответа устройства завершился отказом
		 */
		if(this->_parser.error() != http::parser_http_t::error_t::NONE){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем обработку полученных данных
			return;
		}
		/**
		 * Если ответ устройства заголовка не содержит
		 */
		if(this->_parser.message().provider == nullptr){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем обработку полученных данных
			return;
		}
		// Получаем код ответа устройства
		const uint32_t code = static_cast <const http::response_t *> (this->_parser.message().provider.get())->code;
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
	/**
	 * Если адрес описания устройства локальной сети не принадлежит
	 *
	 * @details Адрес этот назван тем, кто ответил на рассылку, а ответить в неё вправе
	 *          кто угодно в сети: устройство доступа в сеть лежит на ней же, и увести
	 *          обращение за её пределы такой ответ не должен. Проверка отсекает увод
	 *          наружу - на узел, которому предназначалось бы описание вместе со всеми
	 *          последующими просьбами
	 *
	 * @note Полная проверка требовала бы сличения с адресом отправителя ответа, но
	 *       чтение датаграммного события его не выдаёт: договор `read_t` несёт лишь
	 *       данные. Оттого проверяется происхождение адреса, а не его совпадение
	 */
	if(!this->local(host))
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
		"Host: %s\r\n"
		"Connection: keep-alive\r\n"
		"User-Agent: %s\r\n"
		"Accept: text/xml\r\n"
		"\r\n",
		path.c_str(), this->authority(host, port).c_str(), AWH_NAME
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
		/**
		 * Выполняем завершение обмена отказом
		 *
		 * @details Отсутствие пригодной службы кодек выдаёт наравне с отказом разбора, а
		 *          поводы это разные: описание при нём построено верно и прочитано целиком -
		 *          устройство попросту не умеет того, о чём его просят. Объявить это
		 *          неразобранным ответом значило бы указать вызывающему не на ту причину:
		 *          он взялся бы искать порчу разметки там, где её нет
		 *
		 * @note Тот же повод, обнаруженный по службе заслона IPv6, ниже уже объявляется
		 *       неподдерживаемым договором: разница выходила лишь из того, каким путём
		 *       отсутствие службы обнаружено
		 */
		this->failure(type_t::UPNP, ((error == proto::portmap::device_t::error_t::EMPTY_SERVICE) ? error_t::NOT_SUPPORTED : error_t::MALFORMED));
		// Завершаем разбор описания устройства
		return;
	}
	/**
	 * Определяем, проделывается ли или заделывается пробой заслона IPv6
	 *
	 * @note Перенаправлений в сети IPv6 служба соединения не заводит: преобразования
	 *       адресов там нет, и подключения сквозь заслон разрешает отдельная служба.
	 *       Внешний адрес и перечень служба соединения выдаёт и по связи IPv6, и они
	 *       ведутся ею же
	 */
	const bool pinhole = ((this->_family == family_t::IPV6) && ((this->_action == action_t::OPEN) || (this->_action == action_t::CLOSE) || (this->_action == action_t::RENEW)));
	/**
	 * Если проделывается либо заделывается пробой заслона IPv6
	 */
	if(pinhole){
		// Выполняем отыскание службы заслона IPv6
		const proto::portmap::device_t::service_t * service = this->_device.service(description, proto::portmap::device_t::SERVICE_WAN_IPV6);
		/**
		 * Если служба заслона IPv6 устройством не выдаётся
		 */
		if(service == nullptr){
			// Записываем в лог сообщение об отсутствии службы заслона IPv6
			this->_log->print("Device does not provide the IPv6 firewall service", log_t::flag_t::WARNING);
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::NOT_SUPPORTED);
			// Завершаем разбор описания устройства
			return;
		}
		// Запоминаем обозначение вида отысканной службы
		this->_service = service->type;
		// Выполняем сборку полного адреса управления отысканной службой
		this->_control = this->_device.address(description, this->_location, service->control);
		/**
		 * Запоминаем, что состояние заслона IPv6 спрашивается прежде просьбы о пробое
		 *
		 * @note Спрашивается оно лишь перед проделыванием: заделывание пробоя от
		 *       состояния заслона не зависит, а лишний шаг обмена его только затянет
		 */
		this->_probe = (this->_action == action_t::OPEN);
		/**
		 * Если вызов действия службы начать не удалось
		 */
		if(!this->control())
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::NO_RESPONSE);
		// Завершаем разбор описания устройства
		return;
	}
	/**
	 * Выполняем перебор всех видов службы соединения
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
		/**
		 * Если читается перечень заведённых перенаправлений
		 *
		 * @note Перечня служба разом не выдаёт: перенаправления читаются по одному,
		 *       наращивая порядковый номер, пока служба не ответит отказом о выходе за
		 *       перечень
		 */
		case static_cast <uint8_t> (action_t::LIST):
			// Выполняем сборку вызова чтения перенаправления по порядковому номеру
			request = this->_upnp.entry(this->_service, this->_index);
		break;
		/**
		 * Если заводится перенаправление порта либо продлевается срок заведённого
		 *
		 * @note Продление у службы соединения отдельного действия не имеет: договор
		 *       продлевает перенаправление повторной просьбой о его заведении, и
		 *       собирается она той же сборкой. Отдельное действие есть лишь у службы
		 *       заслона IPv6, где заведение заново выдало бы новый опознаватель
		 */
		case static_cast <uint8_t> (action_t::OPEN):
		case static_cast <uint8_t> (action_t::RENEW): {
			/**
			 * Если продлевается срок пробоя заслона IPv6
			 */
			if((this->_family == family_t::IPV6) && (this->_action == action_t::RENEW)){
				// Выполняем сборку вызова продления срока пробоя заслона IPv6
				request = this->_upnp.repinhole(
					this->_service, this->_mapping.pinhole,
					(this->_mapping.lifeTime > 0 ? this->_mapping.lifeTime : ::DEFAULT_LIFETIME)
				);
				// Выходим из определения просьбы
				break;
			}
			/**
			 * Если проделывается пробой заслона IPv6
			 */
			if(this->_family == family_t::IPV6){
				/**
				 * Если состояние заслона IPv6 ещё не спрошено
				 *
				 * @note Спрашивается оно тем же ходом обмена, что и просьба о пробое:
				 *       ответ на вопрос уводит сюда же, снимая признак, и следующая
				 *       сборка выдаёт уже саму просьбу
				 */
				if(this->_probe){
					// Выполняем сборку вызова запроса состояния заслона IPv6
					request = this->_upnp.firewall(this->_service);
					// Выходим из определения просьбы
					break;
				}
				// Собираемый пробой заслона IPv6
				proto::portmap::upnp_t::pinhole_t pinhole;
				// Запоминаем договор пробоя заслона
				pinhole.proto = ((this->_mapping.proto == proto_t::TCP) ? proto::portmap::upnp_t::proto_t::TCP : proto::portmap::upnp_t::proto_t::UDP);
				// Запоминаем внутренний порт машины, которой отдаются подключения
				pinhole.internalPort = this->_mapping.internalPort;
				/**
				 * Запоминаем запрашиваемый срок жизни пробоя заслона
				 *
				 * @note Бессрочных пробоев договор не заводит, и нулевой срок здесь
				 *       заменяется отведённым по умолчанию
				 */
				pinhole.lifeTime = (this->_mapping.lifeTime > 0 ? this->_mapping.lifeTime : ::DEFAULT_LIFETIME);
				// Запоминаем внешний узел, подключения с которого пропускаются
				pinhole.remoteHost = this->_mapping.remoteHost;
				/**
				 * Получаем внутренний адрес машины, которой отдаются подключения
				 *
				 * @note Преобразования адресов в сети IPv6 нет, и подставить адрес
				 *       маршрутизатор не может: указывать его обязан просящий
				 */
				pinhole.internalClient = this->_io->getAddress(this->_stream, event::address_t::IPV6);
				// Если внутренний адрес машины получить не удалось
				if(pinhole.internalClient.empty())
					// Выводим отрицательный результат начала вызова действия службы
					return false;
				// Выполняем сборку вызова проделывания пробоя заслона IPv6
				request = this->_upnp.pinhole(this->_service, pinhole);
				// Выходим из определения просьбы
				break;
			}
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
			mapping.internalClient = this->_io->getAddress(this->_stream, ((this->_family == family_t::IPV6) ? event::address_t::IPV6 : event::address_t::IPV4));
			// Если внутренний адрес машины получить не удалось
			if(mapping.internalClient.empty())
				// Выводим отрицательный результат начала вызова действия службы
				return false;
			// Выполняем сборку вызова заведения перенаправления порта
			request = this->_upnp.add(this->_service, mapping);
		} break;
		// Если убирается заведённое перенаправление порта
		case static_cast <uint8_t> (action_t::CLOSE):
			/**
			 * Если заделывается пробой заслона IPv6
			 *
			 * @note Пробой заделывается по опознавателю, выданному при его проделывании:
			 *       одним и тем же признакам отвечает несколько пробоев, и снимать их
			 *       следует поимённо
			 */
			if(this->_family == family_t::IPV6){
				/**
				 * Если опознаватель заделываемого пробоя не задан
				 */
				if(this->_mapping.pinhole == 0){
					// Записываем в лог сообщение о незаданном опознавателе пробоя
					this->_log->print("Firewall pinhole identifier is missing", log_t::flag_t::WARNING);
					// Выводим отрицательный результат начала вызова действия службы
					return false;
				}
				// Выполняем сборку вызова заделывания пробоя заслона IPv6
				request = this->_upnp.unpinhole(this->_service, this->_mapping.pinhole);
				// Выходим из определения просьбы
				break;
			}
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
	/**
	 * Если адрес управления службой локальной сети не принадлежит
	 *
	 * @details Адрес этот собран по описанию, взятому у устройства, и устройство
	 *          вправе назвать в нём и основание записи, и полный адрес управления.
	 *          Проверка описания их не покрывает: она сличает адрес самого описания,
	 *          а увести обращение за пределы сети описание способно и своим
	 *          содержимым. Здесь то же требование прилагается к тому адресу, по
	 *          которому обращение и пойдёт
	 *
	 * @note Просьба несёт внутренний адрес машины и её порты, и отправлять её
	 *       узлу, названному кем угодно ответившим на рассылку, недопустимо
	 */
	if(!this->local(host))
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
		"Host: %s\r\n"
		"Connection: keep-alive\r\n"
		"User-Agent: %s\r\n"
		"Content-Type: text/xml; charset=\"utf-8\"\r\n"
		"SOAPAction: %s\r\n"
		"Content-Length: %zu\r\n"
		"\r\n%s",
		path.c_str(), this->authority(host, port).c_str(), AWH_NAME,
		request.header.c_str(), request.body.length(), request.body.c_str()
	);
	// Запоминаем шаг обмена по договору UPnP
	this->_stage = stage_t::CONTROL;
	/**
	 * Если потоковое событие обмена уже подключено
	 *
	 * @details Соединение переиспользуется, а не заводится заново: обращения к службе
	 *          идут к тому же устройству, что и чтение описания, и рукопожатие ради
	 *          каждого из них - лишняя трата и лишний случай его потерять. При чтении
	 *          перечня обращений столько же, сколько перенаправлений, и разница
	 *          выходит уже не в накладных расходах, а в надёжности
	 *
	 * @note Запрос отправляется с полем `Connection: keep-alive`, и устройство
	 *       соединение держит. Закрыть его оно всё же вправе, и тогда отправка
	 *       отказывает - событие в этом случае заводится заново
	 */
	if(this->_connected && (this->_stream > 0)){
		// Выполняем подготовку объекта разбора к чтению ответа устройства
		this->prepare();
		// Если запрос устройству отправить удалось
		if(this->_io->send(this->_stream, this->_request.data(), this->_request.length()) > 0)
			// Выводим положительный результат начала вызова действия службы
			return true;
		// Снимаем признак того, что потоковое событие подключено
		this->_connected = false;
	}
	// Выполняем заведение потокового события обмена с устройством
	return this->stream(host, port);
}
/**
 * @brief Метод повторения текущего шага обмена по договору UPnP
 *
 * @return результат повторения текущего шага обмена
 *
 */
bool awh::unit::Portmap::repeat() noexcept {
	/**
	 * Снимаем признак того, что потоковое событие подключено
	 *
	 * @note Тем обращение и уводится на заведение нового события: подключение, с
	 *       которого ответа не дождались, к делу больше не годится, а уничтожает
	 *       старое событие само заведение нового
	 */
	this->_connected = false;
	/**
	 * Определяем шаг обмена, на котором стоит обращение
	 */
	switch(static_cast <uint8_t> (this->_stage)){
		// Если читалось описание устройства
		case static_cast <uint8_t> (stage_t::DESCRIPTION):
			// Выполняем повторное чтение описания устройства
			return this->describe();
		// Если вызывалось действие службы устройства
		case static_cast <uint8_t> (stage_t::CONTROL):
			// Выполняем повторный вызов действия службы устройства
			return this->control();
	}
	// Выводим отрицательный результат повторения текущего шага обмена
	return false;
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
		// Получаем код итога, выданный службой устройства
		const proto::portmap::upnp_t::result_t result = this->_upnp.result(answer);
		/**
		 * Если перечень заведённых перенаправлений прочитан до конца
		 *
		 * @note Конца перечня служба иначе не объявляет: отказ о выходе за перечень и
		 *       есть признак того, что перенаправления кончились. Отказом обмена он
		 *       поэтому не считается - перечень прочитан, пусть даже пустой
		 */
		if((this->_action == action_t::LIST) && (result == proto::portmap::upnp_t::result_t::INDEX_INVALID)){
			// Выполняем завершение обмена по этому договору
			this->complete(type_t::UPNP);
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const vector <mapping_t> &, const type_t)> ("mappings", this->_mappings, type_t::UPNP);
			// Завершаем разбор ответа службы устройства
			return;
		}
		// Выполняем завершение обмена отказом
		this->failure(type_t::UPNP, this->reason(result));
		// Завершаем разбор ответа службы устройства
		return;
	}
	/**
	 * Если читается перечень заведённых перенаправлений
	 */
	if(this->_action == action_t::LIST){
		// Извлекаемое перенаправление порта
		proto::portmap::upnp_t::mapping_t entry;
		/**
		 * Если перенаправление порта извлечь не удалось
		 */
		if(!this->_upnp.mapping(answer, entry)){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем разбор ответа службы устройства
			return;
		}
		// Собираемое перенаправление порта для перечня
		mapping_t mapping;
		// Запоминаем договор перенаправляемого порта
		mapping.proto = ((entry.proto == proto::portmap::upnp_t::proto_t::TCP) ? proto_t::TCP : proto_t::UDP);
		// Запоминаем внешний порт перенаправления
		mapping.externalPort = entry.externalPort;
		// Запоминаем внутренний порт перенаправления
		mapping.internalPort = entry.internalPort;
		// Запоминаем срок жизни перенаправления
		mapping.lifeTime = entry.lifeTime;
		// Запоминаем описание перенаправления
		mapping.description = ::move(entry.description);
		// Запоминаем признак того, что перенаправление подключения пропускает
		mapping.enabled = entry.enabled;
		// Запоминаем внешний узел, подключения с которого пропускаются
		mapping.remoteHost = ::move(entry.remoteHost);
		// Запоминаем внутренний адрес машины, которой отдаются подключения
		mapping.internalClient = ::move(entry.internalClient);
		// Выполняем добавление прочитанного перенаправления в перечень
		this->_mappings.push_back(::move(mapping));
		// Выполняем переход к следующему перенаправлению перечня
		this->_index++;
		/**
		 * Если перечень достиг предельной длины
		 *
		 * @note Конец перечня служба объявляет отказом о выходе за него, и устройство,
		 *       отвечающее успехом без конца, обход тем самым не завершает. Прочитанное
		 *       при этом не пропадает: перечень выдаётся тем, что успели собрать
		 */
		if(this->_index >= MAX_MAPPINGS){
			// Записываем в лог сообщение о превышении предельной длины перечня
			this->_log->print("Port mappings list exceeded the limit of %u entries", log_t::flag_t::WARNING, MAX_MAPPINGS);
			// Выполняем завершение обмена по этому договору
			this->complete(type_t::UPNP);
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const vector <mapping_t> &, const type_t)> ("mappings", this->_mappings, type_t::UPNP);
			// Завершаем разбор ответа службы устройства
			return;
		}
		/**
		 * Если чтение следующего перенаправления начать не удалось
		 */
		if(!this->control())
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::NO_RESPONSE);
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
	/**
	 * Если спрашивалось состояние заслона IPv6
	 *
	 * @details Ответ этот просьбой не является: он лишь отвечает, есть ли смысл
	 *          просить о пробое. Отключённый заслон и запрет пробоев объявляются
	 *          здесь прямо, а на саму просьбу устройство ответило бы отказом, по
	 *          которому причина уже не видна
	 */
	if(this->_probe){
		// Признак того, что заслон IPv6 у маршрутизатора включён
		bool enabled = false;
		// Признак того, что пробои заслона IPv6 маршрутизатором разрешены
		bool allowed = false;
		// Снимаем признак того, что спрашивается состояние заслона IPv6
		this->_probe = false;
		/**
		 * Если состояние заслона IPv6 извлечь не удалось
		 */
		if(!this->_upnp.firewall(answer, enabled, allowed)){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем разбор ответа службы устройства
			return;
		}
		/**
		 * Если заслон IPv6 отключён либо пробои его запрещены
		 *
		 * @note Отказ этот означает не ошибку просьбы, а отключённое устройством
		 *       средство: просить его снова тем же способом бесполезно
		 */
		if(!enabled || !allowed){
			// Записываем в лог сообщение о недоступности пробоев заслона IPv6
			this->_log->print(
				"IPv6 firewall is %s", log_t::flag_t::WARNING,
				(enabled ? "enabled but inbound pinholes are not allowed" : "disabled")
			);
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::NOT_SUPPORTED);
			// Завершаем разбор ответа службы устройства
			return;
		}
		/**
		 * Если просьбу о пробое заслона начать не удалось
		 */
		if(!this->control())
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::NO_RESPONSE);
		// Завершаем разбор ответа службы устройства
		return;
	}
	/**
	 * Если проделывался пробой заслона IPv6
	 *
	 * @note Опознаватель проделанного пробоя запоминается в перенаправлении и выдаётся
	 *       вместе с ним: заделать пробой без опознавателя нечем
	 */
	if((this->_family == family_t::IPV6) && (this->_action == action_t::OPEN)){
		// Извлекаемый опознаватель пробоя заслона IPv6
		uint16_t unique = 0;
		/**
		 * Если опознаватель пробоя заслона извлечь не удалось
		 */
		if(!this->_upnp.unique(answer, unique)){
			// Выполняем завершение обмена отказом
			this->failure(type_t::UPNP, error_t::MALFORMED);
			// Завершаем разбор ответа службы устройства
			return;
		}
		// Запоминаем опознаватель проделанного пробоя заслона
		this->_mapping.pinhole = unique;
		/**
		 * Запоминаем назначенный срок жизни пробоя заслона
		 *
		 * @note Назначенного срока служба в ответе не выдаёт, и остаётся запрошенный:
		 *       устройство вправе отвести и меньший, но узнать его негде
		 */
		if(this->_mapping.lifeTime == 0)
			// Запоминаем срок жизни пробоя, отведённый по умолчанию
			this->_mapping.lifeTime = ::DEFAULT_LIFETIME;
	}
	/**
	 * Если продлевался срок пробоя заслона IPv6
	 *
	 * @note Срок доводится до отведённого по умолчанию тем же порядком, что и при
	 *       проделывании пробоя: нулевой срок просьбе передан не был - его заменила
	 *       сборка вызова, - и выдать вызывающему нуль значило бы соврать о том, до
	 *       каких пор пробой живёт
	 */
	if((this->_family == family_t::IPV6) && (this->_action == action_t::RENEW) && (this->_mapping.lifeTime == 0))
		// Запоминаем срок жизни пробоя, отведённый по умолчанию
		this->_mapping.lifeTime = ::DEFAULT_LIFETIME;
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
		// Если продлевался срок заведённого перенаправления
		case static_cast <uint8_t> (action_t::RENEW):
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
		exchange.eid = this->_io->event(event::node_t::CLIENT, ((this->_family == family_t::IPV6) ? event::family_t::IPV6 : event::family_t::IPV4), event::type_t::DATAGRAM, event::protocol_t::UDP);
		/**
		 * Если порт маршрутизатора установить не удалось
		 */
		if(!this->_io->setTargetPort(exchange.eid, ((type == type_t::PCP) ? proto::portmap::pcp_t::PORT : proto::portmap::natpmp_t::PORT))){
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
			// Сбрасываем идентификатор события обмена
			exchange.eid = 0;
			// Выводим отрицательный результат заведения события обмена
			return false;
		}
		/**
		 * Если адрес маршрутизатора установить не удалось
		 */
		if(!this->_io->setTarget(exchange.eid, this->_address.get())){
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
			// Сбрасываем идентификатор события обмена
			exchange.eid = 0;
			// Выводим отрицательный результат заведения события обмена
			return false;
		}
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
			// Сбрасываем идентификатор события обмена
			exchange.eid = 0;
			// Выводим отрицательный результат заведения события обмена
			return false;
		}
		/**
		 * Если запустить событие обмена не удалось
		 */
		if(!(result = (this->_io->commit(exchange.eid) && this->_io->launch(exchange.eid)))){
			// Выполняем удаление события обмена
			this->_io->destroy(exchange.eid);
			// Сбрасываем идентификатор события обмена
			exchange.eid = 0;
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
 * @brief Метод прекращения заведённого события обмена
 *
 * @param type договор перенаправления, по которому ведётся обмен
 *
 */
void awh::unit::Portmap::discard(const type_t type) noexcept {
	// Получаем обмен, событие которого прекращается
	exchange_t & exchange = this->exchange(type);
	/**
	 * Если событие обмена заведено
	 */
	if(exchange.eid > 0){
		// Выполняем удаление события обмена
		this->_io->destroy(exchange.eid);
		// Сбрасываем идентификатор события обмена
		exchange.eid = 0;
	}
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
				/**
				 * Собираемая просьба маршрутизатору
				 *
				 * @note Просьба держится обменом, а не остаётся здесь: ответ сличается с
				 *       ней средствами кодека, и сличать его иначе не с чем
				 */
				proto::portmap::pcp_t::request_t & request = this->_inquiry;
				// Выполняем очистку собираемой просьбы маршрутизатору
				request = proto::portmap::pcp_t::request_t();
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
				/**
				 * Выполняем размещение отличительной метки перенаправления
				 *
				 * @note Метка берётся у обмена, а не собирается здесь: повтор просьбы с
				 *       другой меткой договор счёл бы просьбой о другом перенаправлении
				 */
				::memcpy(&request.nonce[0], this->_nonce.data(), proto::portmap::pcp_t::NONCE_SIZE);
				/**
				 * Если внутренний адрес машины получить не удалось
				 *
				 * @note Договор предписывает указывать в просьбе адрес обращающейся машины,
				 *       и маршрутизатор сличает его с адресом отправителя дейтаграммы
				 */
				if(!this->client(exchange.eid, &request.client[0])){
					// Выполняем прекращение заведённого события обмена
					this->discard(type);
					// Выводим отрицательный результат отправки просьбы
					return false;
				}
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
		/**
		 * Если просьбу собрать не удалось
		 */
		if(size == 0){
			// Выполняем прекращение заведённого события обмена
			this->discard(type);
			// Выводим отрицательный результат отправки просьбы
			return false;
		}
		/**
		 * Если просьбу отправить не удалось
		 */
		if(this->_io->send(exchange.eid, &buffer[0], size) == 0){
			// Выполняем прекращение заведённого события обмена
			this->discard(type);
			// Выводим отрицательный результат отправки просьбы
			return false;
		}
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
	 * Выполняем отыскание маршрутизатора
	 *
	 * @note Отказ отыскания обращение не прерывает: адрес маршрутизатора нужен лишь
	 *       дейтаграммным договорам, а UPnP отыскивает устройство рассылкой SSDP и
	 *       обходится без него. Сеть, где таблица маршрутов шлюза не выдаёт, для UPnP
	 *       поэтому не помеха, и закрывать ему дорогу отказом прочих неверно
	 */
	const bool routed = this->discover();
	/**
	 * Если маршрутизатор отыскать не удалось и обмен ведётся лишь дейтаграммным договором
	 */
	if(!routed && (this->_type != type_t::UPNP) && (this->_type != type_t::AUTO)){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::NO_GATEWAY);
		// Выводим отрицательный результат начала обращения
		return false;
	}
	/**
	 * Готовим отличительную метку перенаправления для договора PCP
	 *
	 * @note Метка собирается один раз на обращение, а не на каждую попытку: договор
	 *       предписывает одну и ту же метку для просьбы и всех её повторов. Метка,
	 *       переданная вызывающим, имеет старшинство - ею снимается перенаправление,
	 *       заведённое в прошлый запуск
	 */
	if(mapping.nonce.size() == proto::portmap::pcp_t::NONCE_SIZE)
		// Запоминаем метку, переданную вызывающим
		this->_nonce = mapping.nonce;
	/**
	 * Если метка вызывающим не передана
	 */
	else {
		// Выделяем место под отличительную метку перенаправления
		this->_nonce.resize(proto::portmap::pcp_t::NONCE_SIZE, 0);
		// Выполняем заполнение отличительной метки перенаправления
		::nonce(this->_nonce.data(), this->_nonce.size());
	}
	// Признак того, что обмен начат хотя бы по одному договору
	bool result = false;
	// Сбрасываем порядковый номер читаемого перенаправления
	this->_index = 0;
	// Сбрасываем признак того, что спрашивается состояние заслона IPv6
	this->_probe = false;
	// Выполняем очистку прочитанного перечня перенаправлений
	this->_mappings.clear();
	/**
	 * Определяем, читается ли перечень заведённых перенаправлений
	 *
	 * @note Перечень выдаёт лишь договор UPnP, и опрашивать прочие бессмысленно: их
	 *       молчание обмен только затянет, а ответить им нечего
	 */
	const bool listing = (action == action_t::LIST);
	/**
	 * Если обмен ведётся договором PCP
	 *
	 * @note Действия запроса внешнего адреса договор не имеет: адрес выдаётся в ответе
	 *       на просьбу о перенаправлении, и спрашивать его отдельно негде. Заводить
	 *       перенаправление ради одного лишь адреса неверно, и просьба эта договору не
	 *       передаётся - её выполняют NAT-PMP и UPnP
	 */
	if(routed && !listing && (action != action_t::EXTERNAL) && ((this->_type == type_t::PCP) || (this->_type == type_t::AUTO))){
		// Выполняем отправку просьбы по договору PCP
		result = (this->submit(type_t::PCP) || result);
		/**
		 * Если просьба заведения отправлена по договору PCP
		 *
		 * @details Метка выдаётся вызывающему сразу, а не по ответу, и вот почему. Под
		 *          видом опроса AUTO договоров ведётся несколько разом, и обращение
		 *          завершается по первому ответу - но просьба, отправленная договором
		 *          PCP, к этому времени уже ушла, и маршрутизатор вправе завести по ней
		 *          перенаправление, чей ответ мы отбросили как опоздавший. Снять такое
		 *          перенаправление можно лишь по метке, и не выдать её значило бы
		 *          оставить его жить до истечения срока
		 *
		 * @note Метка выдаётся и тогда, когда ответил другой договор: лишней она не
		 *       будет - прочие договоры её не имеют и просто не смотрят
		 */
		if(this->_exchangePCP.waiting && (action == action_t::OPEN))
			// Запоминаем отличительную метку отправленной просьбы
			this->_mapping.nonce = this->_nonce;
	}
	/**
	 * Определяем, заделывается ли пробой заслона IPv6 без опознавателя
	 *
	 * @note Пробой заделывается по опознавателю, выданному при его проделывании, и без
	 *       него служба заслона отказала бы наверняка. Опрашивать её впустую незачем:
	 *       под видом опроса AUTO пробой в такой сети заделывает договор PCP, и её
	 *       отказ лишь затянул бы обмен и запутал бы вызывающего
	 */
	const bool unnamed = ((this->_family == family_t::IPV6) && ((action == action_t::CLOSE) || (action == action_t::RENEW)) && (mapping.pinhole == 0));
	/**
	 * Если обмен ведётся договором UPnP
	 *
	 * @note Отыскание маршрутизатора договору UPnP не нужно: устройство отыскивается
	 *       рассылкой SSDP, и заданный настройкой адрес маршрутизатора здесь ни при чём
	 */
	if(!unnamed && ((this->_type == type_t::UPNP) || (this->_type == type_t::AUTO)))
		// Выполняем отыскание устройства рассылкой SSDP
		result = (this->search() || result);
	/**
	 * Если обмен ведётся договором NAT-PMP
	 *
	 * @note Разновидности IPv6 договор не имеет вовсе: RFC 6886 описан только для
	 *       IPv4, и опрашивать им маршрутизатор в сети IPv6 незачем - ответить ему
	 *       нечем, а общий срок опроса это только затянет
	 */
	if(routed && !listing && (this->_family != family_t::IPV6) && ((this->_type == type_t::NAT_PMP) || (this->_type == type_t::AUTO)))
		// Выполняем отправку просьбы по договору NAT-PMP
		result = (this->submit(type_t::NAT_PMP) || result);
	/**
	 * Если обмен не начат ни по одному договору
	 */
	if(!result)
		// Выполняем завершение обмена отказом
		this->failure(this->_type, (routed ? error_t::NO_RESPONSE : error_t::NO_GATEWAY));
	// Выводим результат начала обращения
	return result;
}
/**
 * @brief Метод сверки отсчёта времени работы маршрутизатора
 *
 * @param type  договор перенаправления, по которому получен ответ
 * @param epoch отсчёт времени работы маршрутизатора из ответа
 * @return      признак того, что маршрутизатор утратил своё состояние
 *
 */
bool awh::unit::Portmap::lost(const type_t type, const uint32_t epoch) noexcept {
	// Отсчёт времени работы маршрутизатора по нужному договору
	epoch_t & record = ((type == type_t::PCP) ? this->_epochPCP : this->_epochNATPMP);
	// Получаем время получения ответа по часам этой машины
	const uint64_t stamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
	/**
	 * Если отсчёт получен впервые
	 *
	 * @note Сверять первый отсчёт не с чем: он лишь запоминается, и утратой это не
	 *       считается. Тем же ходом обходится и запуск после перезагрузки маршрутизатора -
	 *       заводить нам к этому времени ещё нечего
	 */
	if(!record.filled){
		// Запоминаем полученный отсчёт
		record.filled = true;
		// Запоминаем значение отсчёта
		record.value = epoch;
		// Запоминаем время получения отсчёта
		record.stamp = stamp;
		// Состояние маршрутизатором не утрачено
		return false;
	}
	/**
	 * Считаем, на сколько отсчёт вырос и сколько времени прошло по часам этой машины
	 *
	 * @note Считается в секундах и с защитой от хода часов вспять: перевод времени
	 *       машины назад дал бы отрицательную разницу, а она здесь беззнаковая
	 */
	const uint64_t elapsed = ((stamp > record.stamp) ? ((stamp - record.stamp) / 1000) : 0);
	// Определяем прирост отсчёта маршрутизатора
	const uint64_t growth = ((epoch >= record.value) ? static_cast <uint64_t> (epoch - record.value) : 0);
	/**
	 * Определяем, утратил ли маршрутизатор своё состояние
	 *
	 * @details Отсчёт, пошедший вспять, означает утрату сам по себе. Отставший же
	 *          сверяется с допуском RFC 6886: прощается одна шестнадцатая прошедшего
	 *          времени и ещё две секунды сверху - столько расходятся часы двух машин,
	 *          но не столько теряет перезагрузившийся маршрутизатор
	 */
	const bool result = ((epoch < record.value) || ((growth + 2) < (elapsed - (elapsed / 16))));
	// Запоминаем значение отсчёта
	record.value = epoch;
	// Запоминаем время получения отсчёта
	record.stamp = stamp;
	/**
	 * Если маршрутизатор своё состояние утратил
	 */
	if(result){
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("reset");
		/**
		 * Если функция обратного вызова установлена
		 */
		if(this->_callback.is(fid))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const type_t)> (fid, type);
		/**
		 * Если функция обратного вызова не установлена
		 */
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем сообщение в лог
				this->_log->debug(
					"Router has lost its port mapping state (protocol: %u)",
					__PRETTY_FUNCTION__,
					make_tuple(static_cast <uint16_t> (type)),
					log_t::flag_t::WARNING,
					static_cast <uint16_t> (type)
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем сообщение в лог
				this->_log->print("Router has lost its port mapping state (protocol: %u)", log_t::flag_t::WARNING, static_cast <uint16_t> (type));
			#endif
		}
	}
	// Выводим признак утраты состояния маршрутизатором
	return result;
}
/**
 * @brief Метод приёма отправителя объявлений маршрутизатора
 *
 * @param eid идентификатор события приёма объявлений
 * @param oid идентификатор события отправителя объявлений
 *
 */
void awh::unit::Portmap::accepted([[maybe_unused]] const event::id_t eid, const event::id_t oid) noexcept {
	// Если событие отправителя объявлений не заведено, выходим
	if(oid == 0) return;
	/**
	 * Если предел числа отправителей объявлений исчерпан
	 *
	 * @note Закрывается самое давнее событие, а не отвергается новое: объявление,
	 *       пришедшее только что, ближе к делу, чем то, что молчит дольше всех
	 */
	if(this->_announcers.size() >= MAX_ANNOUNCERS){
		// Выполняем удаление самого давнего события отправителя объявлений
		this->_io->destroy(this->_announcers.front());
		// Выполняем удаление самого давнего события из списка отправителей
		this->_announcers.erase(this->_announcers.begin());
	}
	// Запоминаем событие отправителя объявлений
	this->_announcers.push_back(oid);
	// Устанавливаем функцию обратного вызова на событие получения ошибок
	this->_io->on(oid, static_cast <engine::callback::error_t> (std::bind(&portmap_t::error, this, _1, _2, _3)));
	// Устанавливаем функцию обратного вызова на событие чтения объявления
	this->_io->on(oid, static_cast <engine::callback::read_t> (std::bind(&portmap_t::announced, this, _1, _2, _3)));
}
/**
 * @brief Метод приёма объявления, разосланного маршрутизатором
 *
 * @param eid  идентификатор события приёма объявления
 * @param data полученное объявление маршрутизатора
 * @param size размер полученного объявления
 *
 */
void awh::unit::Portmap::announced(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Если объявление получено пустым, выходим
	if((data == nullptr) || (size == 0)) return;
	/**
	 * Получаем адрес отправителя объявления
	 *
	 * @note Объявление приходит на групповой адрес, и разослать его вправе любой узел
	 *       сети, а не один лишь маршрутизатор. Отбрасывается пришедшее из внешней
	 *       сети: подделать отправителя нападающий сумел бы и так, но принимать
	 *       заведомо чужое незачем
	 *
	 * @note Отбор ведётся по внешнему адресу, а не по принадлежности местной сети:
	 *       петля местной сетью не считается, а объявление по ней приходит от службы,
	 *       работающей на этой же машине, - отбрасывать его значило бы отвергать своё
	 */
	const string & host = this->_io->getTarget(eid);
	// Если отправитель объявления принадлежит внешней сети, выходим
	if(host.empty() || !this->_addr.parse(host) || (this->_addr.own() == net_addr_t::own_t::WAN)) return;
	/**
	 * Определяем договор, по которому разослано объявление
	 *
	 * @note Объявления обоих договоров приходят на один и тот же порт, и разделяются
	 *       они изданием в первом октете сообщения - как и обычные ответы
	 */
	switch(data[0]){
		/**
		 * Если объявление разослано по договору NAT-PMP
		 */
		case proto::portmap::natpmp_t::VERSION: {
			// Код причины отказа кодека
			proto::portmap::natpmp_t::error_t error = proto::portmap::natpmp_t::error_t::NONE;
			// Разобранное объявление маршрутизатора
			proto::portmap::natpmp_t::answer_t answer;
			// Если объявление разобрать не удалось, выходим
			if(!this->_natpmp.parse(data, size, answer, error)) return;
			/**
			 * Если объявление удачным не является, выходим
			 *
			 * @note Объявление о неудаче договор не рассылает вовсе, и пришедшее с
			 *       кодом отказа объявлением не является
			 */
			if(answer.result != proto::portmap::natpmp_t::result_t::SUCCESS) return;
			// Если объявлен не внешний адрес маршрутизатора, выходим
			if(answer.kind != proto::portmap::natpmp_t::kind_t::ADDRESS) return;
			// Выполняем сверку отсчёта времени работы маршрутизатора
			this->lost(type_t::NAT_PMP, answer.epoch);
			/**
			 * Выполняем размещение объявленного внешнего адреса маршрутизатора
			 *
			 * @note Кодек выдаёт адрес числом, старший октет которого является первым
			 *       октетом адреса, - как и у ответа на запрос внешнего адреса
			 */
			this->_addr.v4(answer.address, net_addr_t::endian_t::BIG);
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("external");
			/**
			 * Если функция обратного вызова установлена
			 *
			 * @note Объявленный адрес выдаётся тем же обратным вызовом, что и
			 *       спрошенный: адрес это один и тот же, и заводить под объявленный
			 *       отдельный вызов значило бы обязать вызывающего разбирать одно и то
			 *       же дважды
			 */
			if(this->_callback.is(fid))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const string &, const type_t)> (fid, static_cast <string> (this->_addr), type_t::NAT_PMP);
		} break;
		/**
		 * Если объявление разослано по договору PCP
		 */
		case proto::portmap::pcp_t::VERSION: {
			// Код причины отказа кодека
			proto::portmap::pcp_t::error_t error = proto::portmap::pcp_t::error_t::NONE;
			// Разобранное объявление маршрутизатора
			proto::portmap::pcp_t::answer_t answer;
			// Если объявление разобрать не удалось, выходим
			if(!this->_pcp.parse(data, size, answer, error)) return;
			/**
			 * Если полученное действие объявлением не является, выходим
			 *
			 * @note На этот порт приходят лишь объявления, но действие проверяется
			 *       всё равно: порт открыт всей сети, и прислать туда вправе что угодно
			 */
			if(answer.opcode != proto::portmap::pcp_t::opcode_t::ANNOUNCE) return;
			// Выполняем сверку отсчёта времени работы маршрутизатора
			this->lost(type_t::PCP, answer.epoch);
		} break;
	}
}
/**
 * @brief Метод включения приёма объявлений маршрутизатора
 *
 * @param mode режим приёма объявлений маршрутизатора
 * @return     результат включения приёма объявлений
 *
 */
bool awh::unit::Portmap::announce(const bool mode) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Выполняем удаление событий отправителей объявлений
		 *
		 * @note События эти заведены движком, а закрывать их обязан модуль: время их
		 *       жизни движку не принадлежит
		 */
		for(const event::id_t oid : this->_announcers)
			// Выполняем удаление события отправителя объявлений
			this->_io->destroy(oid);
		// Выполняем очистку списка событий отправителей объявлений
		this->_announcers.clear();
		/**
		 * Если событие приёма объявлений заведено
		 */
		if(this->_announcer > 0){
			// Выполняем удаление события приёма объявлений
			this->_io->destroy(this->_announcer);
			// Сбрасываем идентификатор события приёма объявлений
			this->_announcer = 0;
		}
		// Если приём объявлений отключается, выводим положительный результат
		if(!mode) return true;
		// Признак того, что приём ведётся сетью IPv6
		const bool six = (this->_family == family_t::IPV6);
		/**
		 * Если приём объявлений ведётся сетью IPv6 по договору NAT-PMP
		 *
		 * @note Договор описан только для IPv4, и объявлений в сети IPv6 он не
		 *       рассылает вовсе. Приём при этом не отменяется: тем же гнездом приходят
		 *       объявления договора PCP, у которого разновидность IPv6 есть
		 */
		// Выполняем заведение события приёма объявлений
		this->_announcer = this->_io->event(event::node_t::SERVER, (six ? event::family_t::IPV6 : event::family_t::IPV4), event::type_t::DATAGRAM, event::protocol_t::UDP);
		// Если событие приёма объявлений завести не удалось, выводим отрицательный результат
		if(this->_announcer == 0) return false;
		/**
		 * Если адрес привязки приёма объявлений установить не удалось
		 *
		 * @note Привязка ведётся неопределённым адресом намеренно: объявление
		 *       рассылается на групповой адрес, и принять его гнездо, привязанное к
		 *       адресу самой машины, не сумело бы
		 */
		if(!this->_io->setAddress(this->_announcer, (six ? event::address_t::IPV6 : event::address_t::IPV4), (six ? "::" : "0.0.0.0"))){
			// Выполняем удаление события приёма объявлений
			this->_io->destroy(this->_announcer);
			// Сбрасываем идентификатор события приёма объявлений
			this->_announcer = 0;
			// Выводим отрицательный результат включения приёма объявлений
			return false;
		}
		/**
		 * Если порт приёма объявлений установить не удалось
		 */
		if(!this->_io->setSourcePort(this->_announcer, proto::portmap::natpmp_t::ANNOUNCE_PORT)){
			// Выполняем удаление события приёма объявлений
			this->_io->destroy(this->_announcer);
			// Сбрасываем идентификатор события приёма объявлений
			this->_announcer = 0;
			// Выводим отрицательный результат включения приёма объявлений
			return false;
		}
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_announcer, static_cast <engine::callback::error_t> (std::bind(&portmap_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на приём отправителя объявлений
		this->_io->on(this->_announcer, static_cast <engine::callback::accept_t> (std::bind(&portmap_t::accepted, this, _1, _2)));
		/**
		 * Если опции события приёма объявлений установить не удалось
		 *
		 * @note Переиспользование адреса и порта здесь обязательно: порт этот
		 *       договорный, и занимает его всякая работающая на машине служба
		 *       перенаправления - без переиспользования приём не завёлся бы рядом с ней
		 */
		if(!this->_io->setOptions(this->_announcer, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::MULTICAST_LOOPBACK)){
			// Выполняем удаление события приёма объявлений
			this->_io->destroy(this->_announcer);
			// Сбрасываем идентификатор события приёма объявлений
			this->_announcer = 0;
			// Выводим отрицательный результат включения приёма объявлений
			return false;
		}
		/**
		 * Если запустить событие приёма объявлений не удалось
		 */
		if(!(this->_io->commit(this->_announcer) && this->_io->launch(this->_announcer))){
			// Выполняем удаление события приёма объявлений
			this->_io->destroy(this->_announcer);
			// Сбрасываем идентификатор события приёма объявлений
			this->_announcer = 0;
			// Выводим отрицательный результат включения приёма объявлений
			return false;
		}
		/**
		 * Вступление в группу объявлений намеренно не выполняется
		 *
		 * @details Объявления рассылаются на всеобщие групповые адреса - 224.0.0.1 в сети
		 *          IPv4 и FF02::1 в сети IPv6. Членство в них принадлежит всякому узлу по
		 *          устройству самой сети, и рассылку система доставляет без подписки - тем
		 *          же порядком, что и обращённую прямо к машине. Подписка прибавила бы к
		 *          этому ровно ничего
		 *
		 * @note Отказом система на такую просьбу не отвечает: проверено опытом на macOS,
		 *       Linux и FreeBSD - вступление в 224.0.0.1 принимают все три, и FF02::1 тоже
		 *       принимают Linux с FreeBSD. Отказывает лишь macOS и лишь на FF02::1 с
		 *       неназванным устройством: групповой адрес там принадлежит связи, и
		 *       устройство приходится называть. Довод отказаться от подписки не в этом -
		 *       она попросту не нужна
		 *
		 * @note Отступление это от рассылки обнаружения SSDP, где членство заводится: там
		 *       групповой адрес свой, договорный, и без подписки на него не придёт ничего
		 */
		// Выводим положительный результат включения приёма объявлений
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(mode), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим отрицательный результат включения приёма объявлений
	return false;
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
	 * Если отказ обмену одного договора не принадлежит
	 *
	 * @note Отказом под видом опроса AUTO завершается всё обращение: договор здесь не
	 *       назван, а значит не начат ни один из них - прекращать следует все разом
	 */
	if((type == type_t::AUTO) || (type == type_t::NONE))
		// Выполняем прекращение всех ведущихся обменов
		this->cancel();
	/**
	 * Если обмен вёлся по одному из договоров перенаправления
	 */
	else if((type == type_t::PCP) || (type == type_t::UPNP) || (type == type_t::NAT_PMP)){
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
	// Снимаем признак того, что потоковое событие подключено
	this->_connected = false;
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
	// Выполняем установку функции обратного вызова для перечня заведённых перенаправлений
	this->_callback.set("mappings", callback);
	// Выполняем установку функции обратного вызова для отказа перенаправления
	this->_callback.set("failure", callback);
	// Выполняем установку функции обратного вызова для утраты состояния маршрутизатором
	this->_callback.set("reset", callback);
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
 * @brief Метод получения разновидности сети, в которой ведётся обмен
 *
 * @return установленная разновидность сети
 *
 */
awh::unit::Portmap::family_t awh::unit::Portmap::getFamily() const noexcept {
	// Выводим разновидность сети, в которой ведётся обмен
	return this->_family;
}
/**
 * @brief Метод установки разновидности сети, в которой ведётся обмен
 *
 * @param family разновидность сети для установки
 *
 */
void awh::unit::Portmap::setFamily(const family_t family) noexcept {
	// Устанавливаем разновидность сети, в которой ведётся обмен
	this->_family = family;
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
 * @brief Метод установки сетевого устройства, которым ведётся обмен
 *
 * @param iface название сетевого устройства для установки
 *
 */
void awh::unit::Portmap::setIface(string_view iface) noexcept {
	// Устанавливаем сетевое устройство, которым ведётся обмен
	this->_iface.assign(iface.begin(), iface.end());
}
/**
 * @brief Метод установки предела числа переходов рассылки обнаружения
 *
 * @param hops предел числа переходов для установки
 *
 */
void awh::unit::Portmap::setHops(const event::hops_t hops) noexcept {
	// Устанавливаем предел числа переходов рассылки обнаружения
	this->_hops = hops;
}
/**
 * @brief Метод получения внешнего адреса маршрутизатора
 *
 * @return результат отправки просьбы
 *
 */
bool awh::unit::Portmap::external() noexcept {
	/**
	 * Если опрос ведётся договором PCP
	 *
	 * @note Действия запроса внешнего адреса договор не имеет: адрес выдаётся в ответе
	 *       на просьбу о перенаправлении, и спрашивать его отдельно негде. Заводить
	 *       перенаправление ради одного лишь адреса неверно
	 */
	if(this->_type == type_t::PCP){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::NOT_SUPPORTED);
		// Выводим отрицательный результат отправки просьбы
		return false;
	}
	/**
	 * Если опрос ведётся договором NAT-PMP в сети IPv6
	 *
	 * @note Разновидности IPv6 договор не имеет, и внешний адрес в такой сети выдаёт
	 *       лишь UPnP
	 */
	if((this->_family == family_t::IPV6) && (this->_type == type_t::NAT_PMP)){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::NOT_SUPPORTED);
		// Выводим отрицательный результат отправки просьбы
		return false;
	}
	// Выполняем начало обращения к маршрутизатору
	return this->perform(action_t::EXTERNAL, mapping_t());
}
/**
 * @brief Метод получения перечня заведённых перенаправлений портов
 *
 * @return результат отправки просьбы
 *
 */
bool awh::unit::Portmap::list() noexcept {
	/**
	 * Если опрос ведётся не договором UPnP
	 *
	 * @note Перечень заведённых перенаправлений выдаёт лишь этот договор: у NAT-PMP и
	 *       PCP чтения заведённого нет вовсе, и опрос AUTO тут ничего не спасает -
	 *       спрашивать нечего у двух договоров из трёх
	 */
	if((this->_type != type_t::UPNP) && (this->_type != type_t::AUTO)){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::NOT_SUPPORTED);
		// Выводим отрицательный результат отправки просьбы
		return false;
	}
	// Выполняем начало обращения к маршрутизатору
	return this->perform(action_t::LIST, mapping_t());
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
 * @brief Метод продления срока заведённого перенаправления порта
 *
 * @param mapping перенаправление порта для продления
 * @return        результат отправки просьбы
 *
 */
bool awh::unit::Portmap::renew(const mapping_t & mapping) noexcept {
	/**
	 * Если перенаправление задано неполно
	 */
	if((mapping.proto == proto_t::NONE) || (mapping.internalPort == 0)){
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::MALFORMED);
		// Выводим отрицательный результат отправки просьбы
		return false;
	}
	/**
	 * Если продлевается пробой заслона IPv6 без опознавателя
	 *
	 * @note Продлевается пробой по опознавателю, выданному при его проделывании, и
	 *       без него служба заслона отказала бы наверняка
	 */
	if((this->_family == family_t::IPV6) && (this->_type == type_t::UPNP) && (mapping.pinhole == 0)){
		// Записываем в лог сообщение о незаданном опознавателе пробоя
		this->_log->print("Firewall pinhole identifier is missing", log_t::flag_t::WARNING);
		// Выполняем завершение обмена отказом
		this->failure(this->_type, error_t::MALFORMED);
		// Выводим отрицательный результат отправки просьбы
		return false;
	}
	// Выполняем начало обращения к маршрутизатору
	return this->perform(action_t::RENEW, mapping);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::Portmap::Portmap(const fmk_t * fmk, const log_t * log) noexcept :
 unit_t(fmk, log), _type(type_t::AUTO), _family(family_t::IPV4), _action(action_t::NONE),
 _attempts(::DEFAULT_ATTEMPTS), _delay(::DEFAULT_DELAY), _announcer(0),
 _index(0), _probe(false), _stage(stage_t::NONE), _stream(0), _location{""}, _control{""}, _service{""},
 _parser(http::direct_t::RESPONSE, fmk, log), _request{""}, _payload{""}, _complete(false), _connected(false), _uri(fmk, log), _router{""},
 _iface{""}, _hops(event::hops_t::NETWORK), _address(nullptr), _addr(fmk, log), _ifaces(fmk, log), _gateway(fmk, log), _pcp(fmk, log),
 _ssdp(fmk, log), _soap(fmk, log), _upnp(fmk, log), _device(fmk, log), _natpmp(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Portmap::~Portmap() noexcept {
	// Выполняем прекращение всех ведущихся обменов
	this->cancel();
	/**
	 * Выполняем отключение приёма объявлений маршрутизатора
	 *
	 * @note Приём обменами не заводится и ими не прекращается: держится он всё время
	 *       работы модуля, и закрыть его больше негде
	 */
	this->announce(false);
}
