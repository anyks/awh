/**
 * @file: upnp.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация кодека действий службы перенаправления UPnP — сборка вызовов
 *        заведения, снятия и перечисления перенаправлений и разбор ответов службы
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/ascii.hpp>
#include <proto/portmap/upnp.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Метод получения названия действия службы соединения
	 *
	 * @param action действие службы соединения
	 * @return       название действия, отведённое договором
	 *
	 */
	const char * name(const awh::proto::portmap::upnp_t::action_t action) noexcept {
		/**
		 * Определяем действие службы соединения
		 */
		switch(static_cast <uint8_t> (action)){
			// Если действием является заведение перенаправления порта
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::ADD):
				// Выводим название действия службы
				return "AddPortMapping";
			// Если действием является снятие перенаправления порта
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::DELETE):
				// Выводим название действия службы
				return "DeletePortMapping";
			// Если действием является чтение внешнего адреса маршрутизатора
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::EXTERNAL):
				// Выводим название действия службы
				return "GetExternalIPAddress";
			// Если действием является чтение перенаправления по порядковому номеру
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::ENTRY):
				// Выводим название действия службы
				return "GetGenericPortMappingEntry";
			// Если действием является чтение перенаправления по внешнему порту
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::SPECIFIC):
				// Выводим название действия службы
				return "GetSpecificPortMappingEntry";
			// Если действием является чтение состояния соединения маршрутизатора
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::STATUS):
				// Выводим название действия службы
				return "GetStatusInfo";
			// Если действием является проделывание пробоя заслона IPv6
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::PINHOLE):
				// Выводим название действия службы
				return "AddPinhole";
			// Если действием является заделывание пробоя заслона IPv6
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::UNPINHOLE):
				// Выводим название действия службы
				return "DeletePinhole";
			// Если действием является продление срока пробоя заслона IPv6
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::REPINHOLE):
				// Выводим название действия службы
				return "UpdatePinhole";
			// Если действием является чтение состояния заслона IPv6
			case static_cast <uint8_t> (awh::proto::portmap::upnp_t::action_t::FIREWALL):
				// Выводим название действия службы
				return "GetFirewallStatus";
		}
		// Выводим пустое название действия службы
		return "";
	}
};

/**
 * @brief Метод сборки вызова заведения перенаправления порта
 *
 * @param service обозначение вида службы соединения
 * @param mapping параметры заводимого перенаправления порта
 * @return        собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::add(const string_view service, const mapping_t & mapping) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	/**
	 * Если договор перенаправления не задан либо порты не заданы
	 *
	 * @note Нулевой порт договором отведён под особый смысл, и заводить с ним
	 *       перенаправление бессмысленно: подключений на него не бывает
	 */
	if((mapping.proto == proto_t::NONE) || (mapping.externalPort == 0) || (mapping.internalPort == 0)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(mapping.externalPort, mapping.internalPort),
				log_t::flag_t::WARNING, "port mapping parameters are incomplete"
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "port mapping parameters are incomplete");
		#endif
		// Выводим пустой вызов действия службы
		return result;
	}
	/**
	 * Если внутренний адрес машины не задан
	 *
	 * @note Без внутреннего адреса маршрутизатору некому отдавать подключения:
	 *       подставлять адрес отправителя запроса он не обязан
	 */
	if(mapping.internalClient.empty()){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(mapping.externalPort, mapping.internalPort),
				log_t::flag_t::WARNING, "internal client address is missing"
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "internal client address is missing");
		#endif
		// Выводим пустой вызов действия службы
		return result;
	}
	// Получаем описание заводимого перенаправления
	string description = mapping.description;
	/**
	 * Если описание перенаправления длиннее допустимого
	 *
	 * @note Описание обрезается, а не отвергает вызов: описание дело
	 *       предъявительское, и терять из-за него перенаправление незачем
	 */
	if(description.length() > MAX_DESCRIPTION)
		// Выполняем обрезку описания перенаправления
		description.resize(MAX_DESCRIPTION);
	// Собираемый перечень доводов вызова действия
	const vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("NewRemoteHost", mapping.remoteHost),
		soap_t::argument_t("NewExternalPort", ::std::to_string(mapping.externalPort)),
		soap_t::argument_t("NewProtocol", name(mapping.proto)),
		soap_t::argument_t("NewInternalPort", ::std::to_string(mapping.internalPort)),
		soap_t::argument_t("NewInternalClient", mapping.internalClient),
		soap_t::argument_t("NewEnabled", (mapping.enabled ? "1" : "0")),
		soap_t::argument_t("NewPortMappingDescription", description),
		soap_t::argument_t("NewLeaseDuration", ::std::to_string(mapping.lifeTime))
	};
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::ADD));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action, arguments);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова снятия перенаправления порта
 *
 * @param service      обозначение вида службы соединения
 * @param proto        договор снимаемого перенаправления порта
 * @param externalPort внешний порт снимаемого перенаправления
 * @param remoteHost   внешний узел снимаемого перенаправления
 * @return             собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::remove(const string_view service, const proto_t proto, const uint16_t externalPort, const string_view remoteHost) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	/**
	 * Если договор перенаправления не задан либо внешний порт не задан
	 */
	if((proto == proto_t::NONE) || (externalPort == 0)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(externalPort), log_t::flag_t::WARNING, "port mapping parameters are incomplete");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "port mapping parameters are incomplete");
		#endif
		// Выводим пустой вызов действия службы
		return result;
	}
	// Собираемый перечень доводов вызова действия
	const vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("NewRemoteHost", remoteHost),
		soap_t::argument_t("NewExternalPort", ::std::to_string(externalPort)),
		soap_t::argument_t("NewProtocol", name(proto))
	};
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::DELETE));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action, arguments);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова чтения внешнего адреса маршрутизатора
 *
 * @param service обозначение вида службы соединения
 * @return        собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::external(const string_view service) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::EXTERNAL));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова чтения перенаправления по порядковому номеру
 *
 * @param service обозначение вида службы соединения
 * @param index   порядковый номер читаемого перенаправления
 * @return        собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::entry(const string_view service, const uint32_t index) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	// Собираемый перечень доводов вызова действия
	const vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("NewPortMappingIndex", ::std::to_string(index))
	};
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::ENTRY));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action, arguments);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова чтения перенаправления по внешнему порту
 *
 * @param service      обозначение вида службы соединения
 * @param proto        договор читаемого перенаправления порта
 * @param externalPort внешний порт читаемого перенаправления
 * @param remoteHost   внешний узел читаемого перенаправления
 * @return             собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::specific(const string_view service, const proto_t proto, const uint16_t externalPort, const string_view remoteHost) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	/**
	 * Если договор перенаправления не задан либо внешний порт не задан
	 */
	if((proto == proto_t::NONE) || (externalPort == 0)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(externalPort), log_t::flag_t::WARNING, "port mapping parameters are incomplete");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "port mapping parameters are incomplete");
		#endif
		// Выводим пустой вызов действия службы
		return result;
	}
	// Собираемый перечень доводов вызова действия
	const vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("NewRemoteHost", remoteHost),
		soap_t::argument_t("NewExternalPort", ::std::to_string(externalPort)),
		soap_t::argument_t("NewProtocol", name(proto))
	};
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::SPECIFIC));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action, arguments);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова чтения состояния соединения маршрутизатора
 *
 * @param service обозначение вида службы соединения
 * @return        собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::status(const string_view service) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::STATUS));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова проделывания пробоя заслона IPv6
 *
 * @note Собрано по договору UPnP IGD:2 и проверено на живом устройстве: MiniUPnPd 2.3.7
 * с заслоном IPv6 на netfilter
 *
 * @param service обозначение вида службы заслона IPv6
 * @param pinhole параметры проделываемого пробоя заслона
 * @return        собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::pinhole(const string_view service, const pinhole_t & pinhole) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	/**
	 * Если договор пробоя не задан либо внутренний порт не задан
	 *
	 * @note Пустой договор и пустой внутренний порт договором отведены под особый
	 *       смысл, и устройство вправе отвергнуть такую просьбу целиком
	 */
	if((pinhole.proto == proto_t::NONE) || (pinhole.internalPort == 0)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(pinhole.remotePort, pinhole.internalPort),
				log_t::flag_t::WARNING, "firewall pinhole parameters are incomplete"
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "firewall pinhole parameters are incomplete");
		#endif
		// Выводим пустой вызов действия службы
		return result;
	}
	/**
	 * Если внутренний адрес машины не задан
	 *
	 * @note Без внутреннего адреса маршрутизатору некому пропускать подключения:
	 *       преобразования адресов в сети IPv6 нет, и подставить его он не может
	 */
	if(pinhole.internalClient.empty()){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(pinhole.remotePort, pinhole.internalPort),
				log_t::flag_t::WARNING, "internal client address is missing"
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "internal client address is missing");
		#endif
		// Выводим пустой вызов действия службы
		return result;
	}
	/**
	 * Если срок жизни пробоя не задан
	 *
	 * @note Бессрочных пробоев договор не заводит вовсе, и нулевой срок означал бы
	 *       просьбу, которую устройство отвергнет
	 */
	if(pinhole.lifeTime == 0){
		// Записываем ошибку в лог
		this->_log->print("%s", log_t::flag_t::WARNING, "firewall pinhole lease time is missing");
		// Выводим пустой вызов действия службы
		return result;
	}
	/**
	 * Собираемый перечень доводов вызова действия
	 *
	 * @note Договор пробоя передаётся числом договора по описи IANA, а не названием:
	 *       здесь это отличается от заведения перенаправления, где то же поле несёт
	 *       название договора
	 *
	 * @warning Порядок доводов задан описанием службы и произвольным не является:
	 *          договор UPnP предписывает передавать доводы вызова ровно в том порядке,
	 *          в каком они объявлены, и переставлять их местами нельзя
	 */
	const vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("RemoteHost", pinhole.remoteHost),
		soap_t::argument_t("RemotePort", ::std::to_string(pinhole.remotePort)),
		soap_t::argument_t("InternalClient", pinhole.internalClient),
		soap_t::argument_t("InternalPort", ::std::to_string(pinhole.internalPort)),
		soap_t::argument_t("Protocol", ::std::to_string((pinhole.proto == proto_t::TCP) ? 6 : 17)),
		soap_t::argument_t("LeaseTime", ::std::to_string(pinhole.lifeTime))
	};
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::PINHOLE));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action, arguments);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова заделывания пробоя заслона IPv6
 *
 * @note Вызов проверен на живом устройстве - см. замечание к сборке вызова
 * проделывания пробоя
 *
 * @param service обозначение вида службы заслона IPv6
 * @param unique  опознаватель заделываемого пробоя заслона
 * @return        собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::unpinhole(const string_view service, const uint16_t unique) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	// Собираемый перечень доводов вызова действия
	const vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("UniqueID", ::std::to_string(unique))
	};
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::UNPINHOLE));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action, arguments);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова продления срока пробоя заслона IPv6
 *
 * @note Вызов проверен на живом устройстве - см. замечание к сборке вызова
 * проделывания пробоя
 *
 * @param service  обозначение вида службы заслона IPv6
 * @param unique   опознаватель продлеваемого пробоя заслона
 * @param lifeTime запрашиваемый срок жизни пробоя заслона
 * @return         собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::repinhole(const string_view service, const uint16_t unique, const uint32_t lifeTime) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	/**
	 * Если запрашиваемый срок жизни пробоя договором не допускается
	 *
	 * @note Договор отводит сроку промежуток от одной секунды до суток, и просьба
	 *       за его пределами отвергается устройством. Отвергать её здесь дешевле:
	 *       иначе за отказом пришлось бы ходить к устройству
	 */
	if((lifeTime == 0) || (lifeTime > MAX_LIFETIME)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(unique, lifeTime),
				log_t::flag_t::WARNING, "firewall pinhole lease time is out of range"
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "firewall pinhole lease time is out of range");
		#endif
		// Выводим пустой вызов действия службы
		return result;
	}
	/**
	 * Собираемый перечень доводов вызова действия
	 *
	 * @warning Довод срока именуется здесь с приставкой - `NewLeaseTime`, а не
	 *          `LeaseTime`, как у проделывания пробоя. Приставку объявляет описание
	 *          службы, и без неё устройство срока попросту не увидит
	 */
	const vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("UniqueID", ::std::to_string(unique)),
		soap_t::argument_t("NewLeaseTime", ::std::to_string(lifeTime))
	};
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::REPINHOLE));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action, arguments);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод сборки вызова чтения состояния заслона IPv6
 *
 * @note Вызов проверен на живом устройстве - см. замечание к сборке вызова
 * проделывания пробоя
 *
 * @param service обозначение вида службы заслона IPv6
 * @return        собранный вызов действия службы
 *
 */
awh::proto::portmap::UPnP::request_t awh::proto::portmap::UPnP::firewall(const string_view service) const noexcept {
	// Собираемый вызов действия службы
	request_t result;
	// Запоминаем название вызываемого действия службы
	result.action.assign(::name(action_t::FIREWALL));
	// Выполняем сборку обозначения вызываемого действия службы
	result.header = this->_soap.action(service, result.action);
	// Выполняем сборку текста вызова действия службы
	result.body = this->_soap.request(service, result.action);
	// Выводим собранный вызов действия службы
	return result;
}
/**
 * @brief Метод извлечения опознавателя пробоя из ответа службы
 *
 * @note Извлечение проверено на живом устройстве - см. замечание к сборке вызова
 * проделывания пробоя
 *
 * @param answer разобранный ответ службы
 * @param unique ссылка на извлечённый опознаватель пробоя заслона
 * @return       признак успешного извлечения
 *
 */
bool awh::proto::portmap::UPnP::unique(const soap_t::answer_t & answer, uint16_t & unique) const noexcept {
	/**
	 * Если служба ответила отказом
	 */
	if(answer.fault)
		// Выводим признак неудачного извлечения
		return false;
	// Получаем опознаватель проделанного пробоя из ответа службы
	const string_view result = this->_soap.value(answer, "UniqueID");
	/**
	 * Если опознавателя в ответе службы нет
	 *
	 * @note Без опознавателя пробой заделать нечем: признаки его для этого не годятся,
	 *       и принимать такой ответ за успех недопустимо
	 */
	if(result.empty())
		// Выводим признак неудачного извлечения
		return false;
	// Собираемый опознаватель проделанного пробоя заслона
	uint32_t value = 0;
	/**
	 * Выполняем перебор всех знаков опознавателя пробоя заслона
	 *
	 * @details Разбор ведётся своим перебором, а не общим приведением к числу: то
	 *          нечисловую запись выдаёт нулём, а нуль здесь неотличим от опознавателя.
	 *          Принять такой ответ за успех недопустимо - заделать и продлить пробой
	 *          нечем, а вызывающему обмен объявлен удавшимся
	 *
	 * @note Договор отводит опознавателю два байта, и запись сверх этого ответом
	 *       службы быть не может
	 */
	for(const char letter : result){
		/**
		 * Если знак опознавателя цифрой не является
		 */
		if(!ascii::isDigit(letter))
			// Выводим признак неудачного извлечения
			return false;
		// Выполняем разбор очередного знака опознавателя
		value = ((value * 10) + static_cast <uint32_t> (letter - '0'));
		/**
		 * Если опознаватель вышел за отведённые ему пределы
		 */
		if(value > 0xFFFF)
			// Выводим признак неудачного извлечения
			return false;
	}
	// Запоминаем извлечённый опознаватель пробоя заслона
	unique = static_cast <uint16_t> (value);
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод извлечения состояния заслона IPv6 из ответа службы
 *
 * @note Извлечение проверено на живом устройстве - см. замечание к сборке вызова
 * проделывания пробоя
 *
 * @param answer  разобранный ответ службы
 * @param enabled ссылка на признак того, что заслон включён
 * @param allowed ссылка на признак того, что пробои заслона разрешены
 * @return        признак успешного извлечения
 *
 */
bool awh::proto::portmap::UPnP::firewall(const soap_t::answer_t & answer, bool & enabled, bool & allowed) const noexcept {
	/**
	 * Если служба ответила отказом
	 */
	if(answer.fault)
		// Выводим признак неудачного извлечения
		return false;
	// Получаем признак того, что заслон IPv6 включён
	const string_view active = this->_soap.value(answer, "FirewallEnabled");
	// Получаем признак того, что пробои заслона IPv6 разрешены
	const string_view inbound = this->_soap.value(answer, "InboundPinholeAllowed");
	/**
	 * Если признаков в ответе службы нет
	 */
	if(active.empty() || inbound.empty())
		// Выводим признак неудачного извлечения
		return false;
	// Запоминаем признак того, что заслон IPv6 включён
	enabled = ((active.front() == '1') || (active.front() == 't') || (active.front() == 'T'));
	// Запоминаем признак того, что пробои заслона IPv6 разрешены
	allowed = ((inbound.front() == '1') || (inbound.front() == 't') || (inbound.front() == 'T'));
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод извлечения внешнего адреса маршрутизатора из ответа службы
 *
 * @param answer  разобранный ответ службы
 * @param address ссылка на извлечённый внешний адрес маршрутизатора
 * @return        признак успешного извлечения
 *
 */
bool awh::proto::portmap::UPnP::address(const soap_t::answer_t & answer, string & address) const noexcept {
	/**
	 * Если служба ответила отказом
	 */
	if(answer.fault)
		// Выводим признак неудачного извлечения
		return false;
	// Получаем внешний адрес маршрутизатора из ответа службы
	const string_view result = this->_soap.value(answer, "NewExternalIPAddress");
	/**
	 * Если внешнего адреса в ответе службы нет
	 *
	 * @note Пустой адрес маршрутизатор выдаёт и тогда, когда соединения с внешней
	 *       сетью у него нет вовсе: принимать такой ответ за успех недопустимо
	 */
	if(result.empty())
		// Выводим признак неудачного извлечения
		return false;
	// Запоминаем извлечённый внешний адрес маршрутизатора
	address.assign(result);
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод извлечения перенаправления порта из ответа службы
 *
 * @param answer  разобранный ответ службы
 * @param mapping ссылка на извлечённое перенаправление порта
 * @return        признак успешного извлечения
 *
 */
bool awh::proto::portmap::UPnP::mapping(const soap_t::answer_t & answer, mapping_t & mapping) const noexcept {
	/**
	 * Если служба ответила отказом
	 */
	if(answer.fault)
		// Выводим признак неудачного извлечения
		return false;
	// Выполняем сброс извлекаемого перенаправления порта
	mapping = mapping_t();
	// Запоминаем договор перенаправления порта
	mapping.proto = this->proto(this->_soap.value(answer, "NewProtocol"));
	// Запоминаем внешний узел перенаправления
	mapping.remoteHost.assign(this->_soap.value(answer, "NewRemoteHost"));
	// Запоминаем внутренний адрес машины перенаправления
	mapping.internalClient.assign(this->_soap.value(answer, "NewInternalClient"));
	// Запоминаем описание перенаправления
	mapping.description.assign(this->_soap.value(answer, "NewPortMappingDescription"));
	// Запоминаем внешний порт перенаправления
	mapping.externalPort = this->_fmk->atoi <uint16_t> (this->_soap.value(answer, "NewExternalPort"));
	// Запоминаем внутренний порт перенаправления
	mapping.internalPort = this->_fmk->atoi <uint16_t> (this->_soap.value(answer, "NewInternalPort"));
	// Запоминаем срок жизни перенаправления
	mapping.lifeTime = this->_fmk->atoi <uint32_t> (this->_soap.value(answer, "NewLeaseDuration"));
	// Получаем признак включения перенаправления
	const string_view enabled = this->_soap.value(answer, "NewEnabled");
	/**
	 * Запоминаем признак включения перенаправления
	 *
	 * @note Отсутствие признака считается включением: перенаправление, о котором
	 *       служба сообщила, но включение не указала, действующим и является
	 */
	mapping.enabled = (enabled.empty() || (enabled.compare("0") != 0));
	/**
	 * Выводим признак успешного извлечения по наличию внутреннего адреса
	 *
	 * @note Внутренний адрес есть у всякого перенаправления: без него подключения
	 *       отдавать некому. Его отсутствие означает, что ответ перенаправления не
	 *       содержит вовсе
	 */
	return !mapping.internalClient.empty();
}
/**
 * @brief Метод получения кода ошибки, выданного службой
 *
 * @param answer разобранный ответ службы
 * @return       код ошибки, выданный службой
 *
 */
awh::proto::portmap::UPnP::result_t awh::proto::portmap::UPnP::result(const soap_t::answer_t & answer) const noexcept {
	/**
	 * Если служба отказом не отвечала
	 */
	if(!answer.fault)
		// Выводим код успешного выполнения просьбы
		return result_t::SUCCESS;
	/**
	 * Выводим код ошибки, выданный службой, как он есть
	 *
	 * @note Неизвестный код подменяться известным не должен: договор оставляет
	 *       место под коды изготовителя, и подмена скрыла бы причину отказа
	 */
	return static_cast <result_t> (answer.code);
}
/**
 * @brief Метод проверки осмысленности повторной просьбы с иным портом
 *
 * @param result код ошибки, выданный службой
 * @return       признак осмысленности повторной просьбы с иным портом
 *
 */
bool awh::proto::portmap::UPnP::retriable(const result_t result) const noexcept {
	/**
	 * Определяем код ошибки, выданный службой
	 */
	switch(static_cast <uint32_t> (result)){
		/**
		 * Если перенаправление занято другой машиной
		 *
		 * @note Порт занят чужим перенаправлением: другой порт вполне может
		 *       оказаться свободным
		 */
		case static_cast <uint32_t> (result_t::CONFLICT):
		/**
		 * Если перенаправление занято иным средством
		 */
		case static_cast <uint32_t> (result_t::CONFLICT_MECHANISM):
		/**
		 * Если маршрутизатор требует равенства портов
		 *
		 * @note Просьба осмысленна с внешним портом, равным внутреннему
		 */
		case static_cast <uint32_t> (result_t::SAME_PORT_REQUIRED):
		/**
		 * Если маршрутизатор заводит лишь бессрочные перенаправления
		 *
		 * @note Просьба осмысленна с нулевым сроком жизни
		 */
		case static_cast <uint32_t> (result_t::ONLY_PERMANENT_LEASES):
			// Выводим признак осмысленности повторной просьбы
			return true;
	}
	/**
	 * Выводим признак бессмысленности повторной просьбы
	 *
	 * @note Отказ настройки, неизвестное действие и нехватка места у маршрутизатора
	 *       повторной просьбой не лечатся
	 */
	return false;
}
/**
 * @brief Метод получения обозначения договора перенаправления
 *
 * @param proto договор перенаправления порта
 * @return      обозначение договора, отведённое договором UPnP
 *
 */
const char * awh::proto::portmap::UPnP::name(const proto_t proto) noexcept {
	/**
	 * Определяем договор перенаправления порта
	 */
	switch(static_cast <uint8_t> (proto)){
		// Если договором перенаправления является UDP
		case static_cast <uint8_t> (proto_t::UDP):
			// Выводим обозначение договора перенаправления
			return "UDP";
		// Если договором перенаправления является TCP
		case static_cast <uint8_t> (proto_t::TCP):
			// Выводим обозначение договора перенаправления
			return "TCP";
	}
	// Выводим пустое обозначение договора перенаправления
	return "";
}
/**
 * @brief Метод определения договора перенаправления по обозначению
 *
 * @param text обозначение договора перенаправления порта
 * @return     определённый договор перенаправления порта
 *
 */
awh::proto::portmap::UPnP::proto_t awh::proto::portmap::UPnP::proto(const string_view text) const noexcept {
	/**
	 * Если обозначением договора является UDP
	 *
	 * @note Сличение ведётся без учёта регистра: обозначение договора службы
	 *       записывают вольно, а разбирать перечень перенаправлений это не мешает
	 */
	if(this->_fmk->compare(text, "UDP"))
		// Выводим определённый договор перенаправления порта
		return proto_t::UDP;
	/**
	 * Если обозначением договора является TCP
	 */
	if(this->_fmk->compare(text, "TCP"))
		// Выводим определённый договор перенаправления порта
		return proto_t::TCP;
	// Выводим неопределённый договор перенаправления порта
	return proto_t::NONE;
}
/**
 * @brief Метод получения описания кода ошибки службы перенаправления UPnP
 *
 * @param result код ошибки, выданный службой
 * @return       описание кода ошибки на английском языке
 *
 */
const char * awh::proto::portmap::message(const upnp_t::result_t result) noexcept {
	/**
	 * Определяем код ошибки, выданный службой
	 */
	switch(static_cast <uint32_t> (result)){
		// Если просьба выполнена
		case static_cast <uint32_t> (upnp_t::result_t::SUCCESS):
			// Выводим описание кода ошибки
			return "success";
		// Если действие службе неизвестно
		case static_cast <uint32_t> (upnp_t::result_t::INVALID_ACTION):
			// Выводим описание кода ошибки
			return "invalid action";
		// Если доводы вызова построены ошибочно
		case static_cast <uint32_t> (upnp_t::result_t::INVALID_ARGS):
			// Выводим описание кода ошибки
			return "invalid arguments";
		// Если действие выполнить не удалось
		case static_cast <uint32_t> (upnp_t::result_t::ACTION_FAILED):
			// Выводим описание кода ошибки
			return "action failed";
		// Если действие отвергнуто настройкой маршрутизатора
		case static_cast <uint32_t> (upnp_t::result_t::NOT_AUTHORIZED):
			// Выводим описание кода ошибки
			return "action not authorized";
		// Если порядковый номер перенаправления вне перечня
		case static_cast <uint32_t> (upnp_t::result_t::INDEX_INVALID):
			// Выводим описание кода ошибки
			return "specified array index is invalid";
		// Если перенаправления с такими признаками нет
		case static_cast <uint32_t> (upnp_t::result_t::NO_SUCH_ENTRY):
			// Выводим описание кода ошибки
			return "no such entry in array";
		// Если пустой внешний узел здесь не допускается
		case static_cast <uint32_t> (upnp_t::result_t::WILDCARD_NOT_SRC):
			// Выводим описание кода ошибки
			return "wildcard not permitted in source IP";
		// Если пустой внешний порт здесь не допускается
		case static_cast <uint32_t> (upnp_t::result_t::WILDCARD_NOT_EXT):
			// Выводим описание кода ошибки
			return "wildcard not permitted in external port";
		// Если перенаправление занято другой машиной
		case static_cast <uint32_t> (upnp_t::result_t::CONFLICT):
			// Выводим описание кода ошибки
			return "conflict in mapping entry";
		// Если маршрутизатор требует равенства портов
		case static_cast <uint32_t> (upnp_t::result_t::SAME_PORT_REQUIRED):
			// Выводим описание кода ошибки
			return "same port values required";
		// Если маршрутизатор заводит лишь бессрочные перенаправления
		case static_cast <uint32_t> (upnp_t::result_t::ONLY_PERMANENT_LEASES):
			// Выводим описание кода ошибки
			return "only permanent leases supported";
		// Если маршрутизатор принимает лишь пустой внешний узел
		case static_cast <uint32_t> (upnp_t::result_t::REMOTE_ONLY_WILDCARD):
			// Выводим описание кода ошибки
			return "remote host only supports wildcard";
		// Если маршрутизатор принимает лишь пустой внешний порт
		case static_cast <uint32_t> (upnp_t::result_t::EXT_ONLY_WILDCARD):
			// Выводим описание кода ошибки
			return "external port only supports wildcard";
		// Если у маршрутизатора не осталось места под перенаправления
		case static_cast <uint32_t> (upnp_t::result_t::NO_PORT_MAPS):
			// Выводим описание кода ошибки
			return "no port maps available";
		// Если перенаправление занято иным средством
		case static_cast <uint32_t> (upnp_t::result_t::CONFLICT_MECHANISM):
			// Выводим описание кода ошибки
			return "conflict with other mechanisms";
		// Если пустой внутренний порт здесь не допускается
		case static_cast <uint32_t> (upnp_t::result_t::WILDCARD_NOT_INT):
			// Выводим описание кода ошибки
			return "wildcard not permitted in internal port";
	}
	// Выводим описание неизвестного кода ошибки
	return "unknown error code";
}
