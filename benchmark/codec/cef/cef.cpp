/**
 * @file cef.cpp
 * @date 2026-09-04
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
 * @brief Реализация общего окружения бенчмарков контейнера CEF — эталонных записей живых
 *        журналов и вычисления показателей прогона
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы бенчмарков
 */
#include "cef.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * @brief Функция формирования сведений о прогоне сценария
 *
 * @param output итоги прогона сценария
 * @return       сведения о прогоне для вывода
 *
 */
std::string awh::benchmark::event::details(const outcome_t & output) noexcept {
	// Буфер собираемых сведений о прогоне
	char buffer[256];
	// Выполняем сборку сведений о прогоне сценария
	::snprintf(
		buffer, sizeof(buffer), "записей: %zu, октетов: %zu, времени: %.3f с, выделений: %zu",
		output.operations, output.bytes, output.seconds, output.allocations
	);
	// Выводим собранные сведения о прогоне сценария
	return std::string(buffer);
}

/**
 * @brief Функция извлечения пропускной способности разбора
 *
 * @param output итоги прогона сценария
 * @return       пропускная способность в мегабайтах в секунду
 *
 */
/**
 * @brief Функция поверки того, что измеряемая работа кругами состоялась
 *
 * @param output итоги прогона сценария
 * @param result заполняемый результат измерения
 * @return       признак того, что работа кругами состоялась
 *
 */
bool awh::benchmark::event::worked(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	// Если итог работы числа кругов не достиг
	if(output.produced < output.operations){
		// Помечаем измерение недействительным
		result.invalid = true;
		// Устанавливаем причину недействительности измерения
		result.reason = "измеряемая работа отказом хотя бы одного круга завершилась";
		// Выводим отсутствие состоявшейся работы
		return false;
	}
	// Выводим признак состоявшейся работы
	return true;
}

double awh::benchmark::event::perSecond(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим отсутствие пропускной способности
		return 0.0;
	// Выводим пропускную способность разбора в мегабайтах в секунду
	return ((static_cast <double> (output.bytes) / 1048576.0) / output.seconds);
}

/**
 * @brief Функция извлечения количества разобранных записей в секунду
 *
 * @param output итоги прогона сценария
 * @return       количество разобранных записей в секунду
 *
 */
double awh::benchmark::event::perEvents(const outcome_t & output) noexcept {
	// Если время прогона не измерено
	if(output.seconds <= 0.0)
		// Выводим отсутствие разобранных записей
		return 0.0;
	// Выводим количество разобранных записей в секунду
	return (static_cast <double> (output.operations) / output.seconds);
}

/**
 * @brief Функция извлечения количества выделений памяти на одну запись
 *
 * @param output итоги прогона сценария
 * @return       количество выделений памяти на одну запись
 *
 */
double awh::benchmark::event::perRecord(const outcome_t & output) noexcept {
	// Если операций прогоном не выполнено
	if(output.operations == 0)
		// Выводим отсутствие выделений памяти
		return 0.0;
	// Выводим количество выделений памяти на одну запись
	return (static_cast <double> (output.allocations) / static_cast <double> (output.operations));
}

/**
 * @brief Функция проверки работоспособности учёта выделений памяти
 *
 * @param output итоги прогона сценария
 * @param result заполняемый результат измерения
 * @return       признак работоспособности учёта
 *
 */
bool awh::benchmark::event::counted(const outcome_t & output, awh::benchmark::result_t & result) noexcept {
	// Если операций прогоном не выполнено
	if(output.operations == 0){
		// Помечаем измерение недействительным
		result.invalid = true;
		// Устанавливаем причину недействительности измерения
		result.reason = "сценарий не выполнил ни одной операции";
		// Выводим неработоспособность учёта выделений памяти
		return false;
	}
	// Если учёт выделений памяти молчит
	if(output.allocations == 0){
		// Помечаем измерение недействительным
		result.invalid = true;
		// Устанавливаем причину недействительности измерения
		result.reason = "учёт выделений памяти не работает: замена оператора не видна";
		// Выводим неработоспособность учёта выделений памяти
		return false;
	}
	// Выводим работоспособность учёта выделений памяти
	return true;
}

/**
 * @brief Функция извлечения задержки обработки одной записи
 *
 * @param output итоги прогона сценария
 * @return       задержка обработки одной записи в микросекундах
 *
 */
double awh::benchmark::event::perLatency(const outcome_t & output) noexcept {
	// Если операций прогоном не выполнено
	if(output.operations == 0)
		// Выводим отсутствие задержки обработки
		return 0.0;
	// Выводим задержку обработки одной записи в микросекундах
	return ((output.seconds * 1000000.0) / static_cast <double> (output.operations));
}

/**
 * @brief Функция получения контрольной суммы прогонов
 *
 * @return ссылка на контрольную сумму прогонов
 *
 */
volatile uint64_t & awh::benchmark::event::checksum() noexcept {
	// Контрольная сумма прогонов
	static volatile uint64_t result = 0;
	// Выводим контрольную сумму прогонов
	return result;
}

/**
 * @brief Функция получения эталонной записи системы обнаружения вторжений
 *
 * @return эталонная запись системы обнаружения вторжений
 *
 */
const std::string & awh::benchmark::event::detection() noexcept {
	// Эталонная запись системы обнаружения вторжений
	static const std::string result =
		"Feb 17 15:30:15 vnetids emerg CEF:0|InfoTeCS|IDS|2.4.3-371989|1:905590:7|ET POLICY RDP connection confirm|1|"
		"cat=1 cn1=25162858 cn1Label=EventID cnt=73386 cs1=not-suspicious cs1Label=IDSClass cs2=emerging-dos "
		"cs2Label=IDSGroup cs3= cs3Label=CVEID cs4=url,g-soft.info cs4Label=ExternalRef cs5= cs5Label=IDSTags "
		"deviceExternalId=1330334083 deviceFacility=Signature dmac=b0:01:86:30:90:05 dpt=8082 dst=192.168.59.39 "
		"proto=TCP rt=Feb 17 2023 23:30:15.734 YEKT smac=eb:11:0e:37:28:65 spt=22332 src=172.16.0.4 "
		"deviceExternalId=1330334083 user=test";
	// Выводим эталонную запись системы обнаружения вторжений
	return result;
}

/**
 * @brief Функция получения эталонной записи заслона сети
 *
 * @return эталонная запись заслона сети
 *
 */
const std::string & awh::benchmark::event::firewall() noexcept {
	// Эталонная запись заслона сети
	static const std::string result =
		"CEF:0|Check Point|VPN-1 & FireWall-1|Check Point|Log|cp_udp_85FA60B6|Unknown|"
		"act=Drop deviceDirection=0 rt=1713339215000 spt=500 dpt=500 cs2Label=Rule Name cs2=CPEarlyDrop "
		"layer_name=prototype_standart Network layer_uuid=7d515068-d33f-4fa7-be7d-c6192bcde62a match_id=16777214 "
		"parent_rule=0 rule_action=Drop ifname=bond5 logid=0 loguid={0x661f7b4f,0x2e1,0x1c3f780a,0x15d0ab25} "
		"origin=10.120.63.36 originsicname=CN\\=chr-cpsg-01,O\\=stal-cpmgr.example.com.9cr9p2 sequencenum=446 "
		"version=5 action_reason=Early Drop: blocking the connection before final rule match. "
		"dst=81.19.104.172 inzone=Internal member_id=1_3 outzone=External product=VPN-1 & FireWall-1 proto=17 "
		"service_id=cp_udp_85FA60B6 src=10.109.0.92";
	// Выводим эталонную запись заслона сети
	return result;
}

/**
 * @brief Функция получения эталонной записи надзора за системой
 *
 * @note Запись взята из образца прежнего модуля CEF без правок: свыше полутора сотен
 *       пар расширения, повторяющиеся ключи десятками и управляющая вставка внутри
 *       значения - всё то, чем живой журнал отличается от придуманного
 *
 * @return эталонная запись надзора за системой
 *
 */
const std::string & awh::benchmark::event::audit() noexcept {
	// Эталонная запись надзора за системой
	static const std::string result =
		"CEF:1|Unix|auditd|Goga|SYSCALL|close|success|SYSCALL|close|success|Unknown| eventId=521317464 "
		"externalId=1279919945 msg= prog-id=128394 op=UNLOAD arch=c000003e syscall=3 success=yes exit=0 a0=8 "
		"a1=7fff8c278ec0 a2=80 a3=7fff8c278fa0 items=0 ppid=1 pid=7001189 auid=0 uid=0 gid=0 euid=0 suid=0 fsuid=0 "
		"egid=0 sgid=0 fsgid=0 tty=(none) ses=9941 comm=\"systemd\" exe=\"/usr/lib/systemd/systemd\" "
		"key=(null)#035ARCH=x86_64 SYSCALL=close AUID=\"Peranvise\" UID=\"Peranvise\" GID=\"Peranvise\" "
		"EUID=\"Peranvise\" SUID=\"Peranvise\" FSUID=\"Peranvise\" EGID=\"Peranvise\" SGID=\"Peranvise\" "
		"FSGID=\"Peranvise\" proctitle=\"(systemd)\" start=1745562362498 end=1745562362498 catdt=Operating System "
		"art=1738149932236 cat=SYSCALL|close|success act=UNLOAD rt=1737765434498 outcome=Successful spid=4985249 "
		"suser=Peranvise suid=0 sproc=(systemd) sourceServiceName=root spriv=root dpid=0 duid=0 "
		"dproc=/usr/lib/systemd/systemd destinationServiceName=systemd fileHash=(systemd) oldFileId=Arguments: argc: 8 "
		"7fff8c278ec0 80 7fff8c278fa0 cs2=(null) cs3=yes cs4=3 cs6=(none) flexString2=1 cn2=9941 cn3=0 cs1Label=CMD "
		"cs2Label=key cs3Label=success/res cs4Label=syscall cs5Label=subj cs6Label=terminal/tty cn2Label=ses "
		"cn3Label=uid c6a2Label=Source IPv6 c6a3Label=Destination IPv6 flexString2Label=Parent Process ID "
		"ahost=fwl-aldafirah-mgmt.ao.nlmk agt=181.16.112.224 agentZoneURI=/All Zones/Russia/PAO "
		"NLMK/Lipetsk/NL_BS/COD/KSIB/NL_BS_10.48.8.0_24_KSIB_Projects_01 agentNtDomain=NL_BS amac=47:93:3a:59:3c:47 "
		"av=8.4.0.8955.0 atz=Europe/Moscow at=syslog dvchost=fwl-aldafirah-mgmt dvc=174.85.252.172 deviceZoneURI=/All "
		"Zones/ArcSight System/Private Address Space Zones/RFC1918: 10.0.0.0-10.255.255.255 dtz=Europe/Moscow "
		"deviceProcessName=auditd geid=3319645372393019934 _cefVer=1.0 ad.prog-id=128394 ad.prog-id=128394 "
		"ad.#035ARCH=x86_64 ad.FSUID=root ad.gid=0 ad.GID=root ad.euid=0 ad.fsgid=0 ad.SGID=root ad.suid=0 ad.fsuid=0 "
		"ad.exit=0 ad.UID=root ad.FSGID=root ad.sgid=0 ad.SUID=root ad.arch=c000003e ad.items=0 ad.syscall=3 "
		"ad.FSUID=root ad.gid=0 ad.fsgid=0 ad.SGID=root ad.pid=3605249 ad.suid=0 ad.uid=0 ad.AUID=root ad.egid=0 "
		"ad.sgid=0 ad.SUID=root ad.EGID=root ad.key=(null) ad.prog-id=128394 ad.ses=9941 ad.#035ARCH=x86_64 ad.auid=0 "
		"ad.GID=root ad.euid=0 ad.EUID=root ad.a0=8 ad.ppid=1 ad.a1=7fff8c278ec0 ad.fsuid=0 ad.exit=0 ad.a2=80 "
		"ad.UID=root ad.a3=7fff8c278fa0 ad.FSGID=root ad.success=yes ad.tty=(none) ad.arch=c000003e ad.items=0 "
		"ad.SYSCALL=close ad.syscall=3 ad.FSUID=root ad.gid=0 ad.fsgid=0 ad.SGID=root ad.pid=3605249 ad.suid=0 "
		"ad.proctitle=(systemd) ad.uid=0 ad.AUID=root ad.egid=0 ad.sgid=0 ad.SUID=root ad.EGID=root ad.key=(null) "
		"ad.prog-id=128394 ad.ses=9941 ad.#035ARCH=x86_64 ad.auid=0 ad.GID=root ad.euid=0 ad.EUID=root ad.a0=8 "
		"ad.ppid=1 ad.a1=7fff8c278ec0 ad.fsuid=0 ad.exit=0 ad.a2=80 ad.UID=root ad.a3=7fff8c278fa0 ad.FSGID=root "
		"ad.success=yes ad.tty=(none) ad.arch=c000003e ad.items=0 ad.SYSCALL=close aid=3M2JCfIgBABCB-7E1Gfk-iQ==";
	// Выводим эталонную запись надзора за системой
	return result;
}

/**
 * @brief Функция получения эталонной записи наименьшей длины
 *
 * @return эталонная запись наименьшей длины
 *
 */
const std::string & awh::benchmark::event::minimal() noexcept {
	// Эталонная запись наименьшей длины
	static const std::string result = "CEF:0|Vendor|Product|1.0|100|Port scan|10|";
	// Выводим эталонную запись наименьшей длины
	return result;
}

/**
 * @brief Функция получения эталонного потока записей
 *
 * @return эталонный поток записей
 *
 */
const std::string & awh::benchmark::event::stream() noexcept {
	// Эталонный поток записей
	static const std::string result = []() noexcept -> std::string {
		// Собираемый поток записей
		std::string output;
		// Выделяем память под собираемый поток записей
		output.reserve(detection().size() * 128);
		/**
		 * Выполняем сборку потока из ста двадцати восьми записей
		 */
		for(size_t i = 0; i < 128; i++){
			// Добавляем очередную запись в поток
			output.append(detection());
			// Отделяем запись от следующей знаком конца строки
			output.append(1, '\n');
		}
		// Выводим собранный поток записей
		return output;
	}();
	// Выводим эталонный поток записей
	return result;
}
