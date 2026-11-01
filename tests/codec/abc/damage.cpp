/**
 * @file damage.cpp
 * @date 2026-10-02
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
 * @brief Изолированный щуп повреждённых кадров ABC; запуск через validation.py
 *
 * @copyright Copyright © 2026
 *
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/resource.h>
#include <codec/abc/abc.hpp>
#include <sys/fmk.hpp>

using namespace std;
/**
 * Подключаем пространство имён awh, чтобы писать fmk без префикса
 */
using namespace awh;
/**
 * Подключаем awh::codec для коротких имён abc, compressor и crypto_t
 */
using namespace awh::codec;

/**
 * @brief Выполняем один сценарий в отдельном процессе
 *
 * @param argc количество параметров
 * @param argv параметры запуска
 * @return     код завершения
 *
 */
int32_t main(int32_t argc, char ** argv){
	// Проверяем параметры запуска
	if(argc != 4)
		// Уходим с кодом 2 без трёх параметров: метод, флаг шифрования и сценарий
		return 2;
	// Инициализируем ядро
	fmk::initialize();
	// Получаем выбранные параметры сценария
	const uint32_t method = static_cast <uint32_t> (::strtoul(argv[1], nullptr, 10));
	// Флаг шифрования из argv[2]: любой ненулевой включает шифрование
	const bool encrypted = (::strtoul(argv[2], nullptr, 10) != 0);
	// Имя сценария порчи из argv[3]: по нему выбираем вид повреждения
	const string scenario(argv[3]);
	// Создаём модули сжатия и шифрования
	compressor::block_t compressor;
	// Шифровальщик тела кадра для ветки encrypted
	crypto_t crypto;
	// Задаём соль abc-damage-salt, чтобы ключ не зависел от окружения
	crypto.salt("abc-damage-salt");
	// Задаём пароль abc-damage-password, из которого выводится ключ шифрования
	crypto.password("abc-damage-password");
	// Настраиваем укладчик
	abc::packer_t packer;
	// Подключаем блочный компрессор к укладчику
	packer.compressor(&compressor);
	// Подключаем шифровальщик к укладчику
	packer.crypto(&crypto);
	// Снимаем текущие настройки укладчика ради правки двух полей
	auto settings = packer.settings();
	// Назначаем метод сжатия, выбранный номером из argv[1]
	settings.text = static_cast <compressor::method_t> (method);
	// Включаем шифрование кадра по флагу из argv[2]
	settings.encrypt = encrypted;
	// Возвращаем укладчику правленые настройки сжатия и шифрования
	packer.settings(settings);
	// Готовим известное содержимое
	const string payload(1024 * 1024, 'a');
	// Буфер кадра, который будем портить по сценарию
	vector <uint8_t> frame;
	// Укладываем эталон: 1 МиБ букв 'a' как payload_t::TEXT, кадр № 1 поколения 0
	if(!packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 1, 0, frame))
		// Уходим с кодом 3, если укладчик не собрал кадр
		return 3;
	// Исключаем неявный отказ от сжатия
	if((frame.at(0) != method) || (((frame.at(1) & 1) != 0) != encrypted))
		// Уходим с кодом 4, когда метод или флаг шифрования в кадре не совпали с запросом
		return 4;
	// Ожидаемая причина отказа
	abc::error_t expected = abc::error_t::NONE;
	// Признак пересчёта суммы
	bool checksum = false;
	/**
	 * Выбираем вид повреждения
	 */
	if(scenario.compare(0, 4, "cut-") == 0){
		// Усекаем кадр на каждом октете заголовка либо в содержимом
		const size_t cut = static_cast <size_t> (::strtoull(scenario.c_str() + 4, nullptr, 10));
		// Усекаем кадр до октета cut, а коды 32–34 снимают октет с хвода, оставляют первый октет тела и режут тело посерёдке
		frame.resize((cut == 32) ? (frame.size() - 1) : ((cut == 33) ? (abc::CHUNK_HEADER + 1) : ((cut == 34) ? (abc::CHUNK_HEADER + (frame.size() - abc::CHUNK_HEADER) / 2) : cut)));
		// Ждём TRUNCATED_CHUNK: усечённый кадр не наберёт объявленной длины тела
		expected = abc::error_t::TRUNCATED_CHUNK;
	} else if(scenario == "length-max"){
		// Подставляем в длину уложенного содержимого максимум UINT32_MAX
		abc::fixed(frame.data() + 4, UINT32_MAX, 4);
		// Ждём TRUNCATED_CHUNK: объявленных октетов тела в буфере нет
		expected = abc::error_t::TRUNCATED_CHUNK;
	} else if(scenario == "origin-max"){
		// Ставим длину исходного содержимого в максимум UINT32_MAX
		abc::fixed(frame.data() + 8, UINT32_MAX, 4);
		// Пересчитаем дайджест, чтобы порча была только в длине исходника
		checksum = true;
		// Ждём INVALID_CHUNK: разжатый МиБ не совпадёт с объявленным UINT32_MAX
		expected = abc::error_t::INVALID_CHUNK;
	} else if(scenario == "origin-zero"){
		// Обнуляем длину исходного содержимого в смещении 8
		abc::fixed(frame.data() + 8, 0, 4);
		// Пересчитаем дайджест, чтобы не ловить попутный INVALID_CHECKSUM
		checksum = true;
		// Ждём INVALID_CHUNK: сжатый кадр с нулевой длиной исходника отсекается заслоном
		expected = abc::error_t::INVALID_CHUNK;
	} else if(scenario == "origin-small"){
		// Ставим длину исходника в 64, меньше итогового разжатого тела
		abc::fixed(frame.data() + 8, 64, 4);
		// Пересчитаем дайджест ради единственной порчи в длине исходника
		checksum = true;
		// Ждём COMPRESSION_FAILED: содержимое длиннее предела 64 вернётся пустым
		expected = abc::error_t::COMPRESSION_FAILED;
	} else if(scenario == "checksum"){
		// Инвертируем последний октет тела, чтобы дайджест перестал сходиться
		frame.back() ^= 0xFF;
		// Ждём INVALID_CHECKSUM: сличение суммы стоит перед разжатием и расшифровкой
		expected = abc::error_t::INVALID_CHECKSUM;
	} else if((scenario == "stream-zero") || (scenario == "stream-half")){
		// Сначала открываем шифрование, чтобы испортить именно сжатый поток
		vector <uint8_t> body(frame.begin() + abc::CHUNK_HEADER, frame.end());
		// При включённом шифровании сначала расшифровываем тело
		if(encrypted)
			// Расшифровываем тело, чтобы портить именно сжатый поток, а не шифр
			body = crypto.decrypt <vector <uint8_t>> (body, settings.hash, settings.cipher);
		// Контролируем, что расшифровка вернула непустое тело
		if(body.empty())
			// Уходим с кодом 5, если расшифровка не дала тела
			return 5;
		// Усекаем поток либо заменяем его нулями
		if(scenario == "stream-half")
			// Усекаем сжатый поток наполовину
			body.resize(body.size() / 2);
		else fill(body.begin(), body.end(), 0);
		// Возвращаем корректное шифрование вокруг повреждённого сжатого потока
		if(encrypted)
			// Заново шифруем испорченное тело корректным ключом
			body = crypto.encrypt <vector <uint8_t>> (body, settings.hash, settings.cipher);
		// Контролируем, что шифрование вернуло непустое тело
		if(body.empty())
			// Уходим с кодом 6, если повторное шифрование сорвалось
			return 6;
		// Оставляем от кадра только заголовок abc::CHUNK_HEADER
		frame.resize(abc::CHUNK_HEADER);
		// Подшиваем испорченное тело к заголовку
		frame.insert(frame.end(), body.begin(), body.end());
		// Переписываем длину уложенного содержимого под новое тело
		abc::fixed(frame.data() + 4, body.size(), 4);
		// Пересчитаем дайджест, чтобы порча осталась лишь в сжатом потоке
		checksum = true;
		// Ждём COMPRESSION_FAILED: разжиматель вернёт пустое на испорченном потоке
		expected = abc::error_t::COMPRESSION_FAILED;
	} else if(scenario != "valid")
		// Уходим с кодом 7 на неизвестном имени сценария
		return 7;
	// Пропускаем проверку суммы только для адресной порчи глубинного слоя
	if(checksum)
		// Пересчитываем дайджест, спрятав порчу глубинного слоя от сличения суммы
		abc::fixed(frame.data() + abc::CHUNK_DIGEST, abc::digest(frame.data(), frame.size()), 8);
	// Начинаем чтение с ненулевого смещения
	const size_t prefix = 17;
	// Подмешиваем вперёд 17 октетов 0xCC, чтобы чтение шло не с нулевого смещения
	frame.insert(frame.begin(), prefix, 0xCC);
	// Стартовое смещение чтения — сразу за префиксом 0xCC
	size_t offset = prefix;
	// Выход, предзаполненный 19 октетами 0xBB, чтобы поймать протечку мусора
	vector <uint8_t> output(19, 0xBB);
	// Приёмник сведений о разобранном кадре
	abc::chunk_t chunk;
	// Замеры потребления RSS до и после распаковки
	struct rusage before, after;
	// Снимаем потребление RSS перед распаковкой
	if(::getrusage(RUSAGE_SELF, &before) != 0)
		// Уходим с кодом 8, если замер RSS не снялся
		return 8;
	// Распаковываем кадр с ненулевого смещения и запоминаем вердикт
	const bool accepted = packer.unpack(frame.data(), frame.size(), offset, output, chunk);
	// Снимаем потребление RSS после распаковки
	if(::getrusage(RUSAGE_SELF, &after) != 0)
		// Уходим с кодом 9, если повторный замер RSS не снялся
		return 9;
	// Проверяем полное содержимое либо очистку выхода и сохранение смещения
	const bool valid = ((expected == abc::error_t::NONE) ?
	 (accepted && (string(output.begin(), output.end()) == payload) && (offset == frame.size())) :
	 (!accepted && output.empty() && (offset == prefix)));
	// Выводим полный исход сценария
	::printf("RESULT valid=%d accepted=%d error=%u expected=%u output=%zu offset=%zu rss=%ld delta=%ld\n", valid, accepted, static_cast <uint32_t> (packer.error()), static_cast <uint32_t> (expected), output.size(), offset, after.ru_maxrss, after.ru_maxrss - before.ru_maxrss);
	// Завершаем процесс кодом 0 при совпадении исхода с ожиданием и кодом 10 при расхождении
	return ((valid && (packer.error() == expected)) ? 0 : 10);
}
