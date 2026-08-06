/**
 * @file: netlink.cpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля опроса ядра Linux через netlink
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cerrno>
#include <cstring>

/**
 * Системные заголовочные файлы
 */
#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/backend/gnu/netlink.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод выборки сведений у ядра
 *
 * @param type     тип запроса (RTM_GETNEIGH, RTM_GETLINK, RTM_GETADDR, RTM_GETROUTE)
 * @param family   семейство протоколов (AF_INET, AF_INET6 либо AF_UNSPEC)
 * @param callback функция обхода полученных сообщений
 * @return         результат выполнения выборки
 *
 */
bool awh::gnu::Netlink::dump(const uint16_t type, const uint8_t family, const handler_t & callback) const noexcept {
	// Переменная результата
	bool result = false;
	// Если функция обхода сообщений не передана
	if(callback == nullptr)
		// Выходим из функции
		return result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Создаём сокет к ядру
		const int32_t fd = ::socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
		// Если сокет создать не удалось
		if(fd < 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(type, family), log_t::flag_t::WARNING, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
			#endif
			// Выходим из функции
			return result;
		}
		/**
		 * @brief Структура запроса выборки
		 *
		 * @details Ядро ждёт заголовок сообщения, а сразу за ним - описание запроса. У
		 *          выборок сведений о сети описание это одно на все четыре вида запроса:
		 *          в нём указывается лишь семейство протоколов
		 *
		 */
		struct {
			// Заголовок сообщения
			struct nlmsghdr header;
			// Описание запроса
			struct rtgenmsg request;
		} message{};
		// Устанавливаем размер сообщения
		message.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
		// Устанавливаем тип запроса
		message.header.nlmsg_type = type;
		// Устанавливаем признаки запроса выборки
		message.header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_DUMP);
		// Устанавливаем порядковый номер запроса
		message.header.nlmsg_seq = 1;
		// Устанавливаем семейство протоколов
		message.request.rtgen_family = family;
		// Выполняем отправку запроса ядру
		if(::send(fd, &message, message.header.nlmsg_len, 0) < 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(type, family), log_t::flag_t::WARNING, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
			#endif
			// Закрываем сокет к ядру
			::close(fd);
			// Выходим из функции
			return result;
		}
		/**
		 * Создаём буфер приёма ответа
		 *
		 * @note Размер взят с запасом: ядро складывает в один приём столько сообщений,
		 *       сколько поместится, и на тесном буфере выборка дробилась бы на лишние
		 *       обращения. Записи же, не поместившейся целиком, ядро не присылает вовсе
		 */
		vector <uint8_t> buffer(32768, 0);
		// Признак продолжения обхода сообщений
		bool walking = true;
		/**
		 * Выполняем чтение ответа ядра до конца выборки
		 */
		while(!result){
			/**
			 * Выполняем чтение очередной части ответа
			 *
			 * @note Постоянной величина эта быть не может: обход сообщений ведёт макрос
			 *       NLMSG_NEXT, а он списывает пройденное прямо из неё
			 */
			ssize_t bytes = ::recv(fd, buffer.data(), buffer.size(), 0);
			// Если чтение выполнить не удалось
			if(bytes < 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(type, family), log_t::flag_t::WARNING, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
				// Выходим из цикла
				break;
			// Если ядро закрыло выборку не попрощавшись
			} else if(bytes == 0)
				// Выходим из цикла
				break;
			// Получаем указатель на первое сообщение выборки
			const struct nlmsghdr * header = reinterpret_cast <const struct nlmsghdr *> (buffer.data());
			/**
			 * Переходим по всем сообщениям полученной части ответа
			 */
			for(; NLMSG_OK(header, static_cast <uint32_t> (bytes)); header = NLMSG_NEXT(header, bytes)){
				// Если сообщение сообщает о конце выборки
				if(header->nlmsg_type == NLMSG_DONE){
					// Запоминаем, что выборка получена целиком
					result = true;
					// Выходим из цикла
					break;
				}
				// Если сообщение сообщает об ошибке
				if(header->nlmsg_type == NLMSG_ERROR){
					// Получаем описание ошибки
					const struct nlmsgerr * error = reinterpret_cast <const struct nlmsgerr *> (NLMSG_DATA(header));
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(type, family), log_t::flag_t::WARNING, ::strerror(-error->error));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(-error->error));
					#endif
					// Выходим из цикла
					break;
				}
				/**
				 * Передаём сообщение обходчику, пока тот не прервал обход
				 *
				 * @warning Прервавшись, выборку всё равно дочитываем до конца: остаток
				 *          непрочитанного достался бы следующему запросу по этому сокету.
				 *          Сокет здесь свой на каждую выборку, но полагаться на это в
				 *          обходе не следует - закрытие может отодвинуться
				 */
				if(walking)
					// Передаём сообщение обходчику
					walking = callback(header);
			}
		}
		// Закрываем сокет к ядру
		::close(fd);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(type, family), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод отправки ядру сообщения изменения и приёма отклика
 *
 * @details Ядро отвечает на изменение одним сообщением: успехом либо кодом ошибки.
 *          Признак подтверждения (NLM_F_ACK) выставляется всегда - без него отклика
 *          не будет вовсе, и отказ прошёл бы незамеченным
 *
 * @param message сообщение изменения
 * @param size    размер сообщения изменения
 * @param log     объект работы с логами
 * @return        результат выполнения изменения
 *
 */
static bool commit(const void * message, const size_t size, const awh::log_t * log) noexcept {
	// Переменная результата
	bool result = false;
	// Создаём сокет к ядру
	const int32_t fd = ::socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
	// Если сокет создать не удалось
	if(fd < 0){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return result;
	}
	// Выполняем отправку сообщения ядру
	if(::send(fd, message, size, 0) < 0){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Закрываем сокет к ядру
		::close(fd);
		// Выходим из функции
		return result;
	}
	// Создаём буфер приёма отклика
	vector <uint8_t> buffer(4096, 0);
	// Выполняем чтение отклика ядра
	const ssize_t bytes = ::recv(fd, buffer.data(), buffer.size(), 0);
	// Если отклик получен
	if(bytes > 0){
		// Получаем указатель на сообщение отклика
		const struct nlmsghdr * header = reinterpret_cast <const struct nlmsghdr *> (buffer.data());
		// Если сообщение является откликом на изменение
		if(NLMSG_OK(header, static_cast <uint32_t> (bytes)) && (header->nlmsg_type == NLMSG_ERROR)){
			// Получаем описание отклика
			const struct nlmsgerr * error = reinterpret_cast <const struct nlmsgerr *> (NLMSG_DATA(header));
			// Изменение считается выполненным, когда ядро не сообщило об ошибке
			if(!(result = (error->error == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), awh::log_t::flag_t::WARNING, ::strerror(-error->error));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(-error->error));
				#endif
			}
		}
	}
	// Закрываем сокет к ядру
	::close(fd);
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод заведения виртуального сетевого устройства
 *
 * @param name имя заводимого устройства
 * @param kind род заводимого устройства
 * @return     результат заведения устройства
 *
 */
bool awh::gnu::Netlink::link(string_view name, string_view kind) const noexcept {
	// Если имя либо род устройства не переданы
	if(name.empty() || kind.empty())
		// Выходим из функции
		return false;
	/**
	 * @brief Структура сообщения заведения устройства
	 *
	 * @note Признаки устройства идут за описанием переменным набором, и места под них
	 *       отводится с запасом: имя и род устройства вместе с заголовками признаков
	 *       занимают заметно меньше
	 */
	struct {
		// Заголовок сообщения
		struct nlmsghdr header;
		// Описание устройства
		struct ifinfomsg info;
		// Набор признаков устройства
		uint8_t attributes[256];
	} message{};
	// Устанавливаем размер сообщения
	message.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	// Устанавливаем тип запроса
	message.header.nlmsg_type = RTM_NEWLINK;
	// Устанавливаем признаки запроса заведения с подтверждением
	message.header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL);
	// Устанавливаем порядковый номер запроса
	message.header.nlmsg_seq = 1;
	// Устанавливаем семейство устройства
	message.info.ifi_family = AF_UNSPEC;
	/**
	 * Добавляем имя устройства
	 */
	// Получаем место под очередной признак
	struct rtattr * attribute = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (&message) + NLMSG_ALIGN(message.header.nlmsg_len));
	// Устанавливаем тип признака
	attribute->rta_type = IFLA_IFNAME;
	// Устанавливаем размер признака
	attribute->rta_len = RTA_LENGTH(name.size() + 1);
	// Копируем имя устройства
	::memcpy(RTA_DATA(attribute), name.data(), name.size());
	// Устанавливаем завершающий ноль имени устройства
	reinterpret_cast <char *> (RTA_DATA(attribute))[name.size()] = '\0';
	// Увеличиваем размер сообщения
	message.header.nlmsg_len = (NLMSG_ALIGN(message.header.nlmsg_len) + RTA_ALIGN(attribute->rta_len));
	/**
	 * Добавляем род устройства
	 *
	 * @details Род вкладывается в признак сведений об устройстве: сам он несёт лишь
	 *          вложенный признак с названием рода, а прочие настройки рода - если бы
	 *          они задавались - легли бы туда же
	 */
	// Получаем место под признак сведений об устройстве
	struct rtattr * info = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (&message) + NLMSG_ALIGN(message.header.nlmsg_len));
	// Устанавливаем тип признака как вложенный
	info->rta_type = (IFLA_LINKINFO | NLA_F_NESTED);
	// Получаем место под вложенный признак рода устройства
	struct rtattr * nested = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (info) + RTA_LENGTH(0));
	// Устанавливаем тип вложенного признака
	nested->rta_type = IFLA_INFO_KIND;
	// Устанавливаем размер вложенного признака
	nested->rta_len = RTA_LENGTH(kind.size() + 1);
	// Копируем род устройства
	::memcpy(RTA_DATA(nested), kind.data(), kind.size());
	// Устанавливаем завершающий ноль рода устройства
	reinterpret_cast <char *> (RTA_DATA(nested))[kind.size()] = '\0';
	// Устанавливаем размер признака сведений об устройстве
	info->rta_len = (RTA_LENGTH(0) + RTA_ALIGN(nested->rta_len));
	// Увеличиваем размер сообщения
	message.header.nlmsg_len = (NLMSG_ALIGN(message.header.nlmsg_len) + RTA_ALIGN(info->rta_len));
	// Выполняем отправку сообщения ядру
	return ::commit(&message, message.header.nlmsg_len, this->_log);
}
/**
 * @brief Метод снятия виртуального сетевого устройства
 *
 * @param name имя снимаемого устройства
 * @return     результат снятия устройства
 *
 */
bool awh::gnu::Netlink::unlink(string_view name) const noexcept {
	// Если имя устройства не передано
	if(name.empty())
		// Выходим из функции
		return false;
	// Получаем номер сетевого устройства по его имени
	const uint32_t index = ::if_nametoindex(string(name).c_str());
	// Если номер сетевого устройства получить не удалось
	if(index == 0){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(string(name)), log_t::flag_t::WARNING, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return false;
	}
	/**
	 * @brief Структура сообщения снятия устройства
	 *
	 * @note Устройство здесь указывается номером, а не именем: имя ядро приняло бы
	 *       тоже, но номер уже получен разрешением, и повторный разбор имени ядру
	 *       поручать незачем
	 */
	struct {
		// Заголовок сообщения
		struct nlmsghdr header;
		// Описание устройства
		struct ifinfomsg info;
	} message{};
	// Устанавливаем размер сообщения
	message.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	// Устанавливаем тип запроса
	message.header.nlmsg_type = RTM_DELLINK;
	// Устанавливаем признаки запроса снятия с подтверждением
	message.header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_ACK);
	// Устанавливаем порядковый номер запроса
	message.header.nlmsg_seq = 1;
	// Устанавливаем семейство устройства
	message.info.ifi_family = AF_UNSPEC;
	// Устанавливаем номер снимаемого устройства
	message.info.ifi_index = static_cast <int32_t> (index);
	// Выполняем отправку сообщения ядру
	return ::commit(&message, message.header.nlmsg_len, this->_log);
}
