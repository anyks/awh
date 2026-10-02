/**
 * @file damage.cpp
 * @date 2026-10-02
 * @license{LicenseRef-AWH-1.0}
 * @author Yuriy Lobarev
 * @brief Изолированный щуп повреждённых кадров ABC; запуск через validation.py
 * @copyright Copyright © 2026
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
using namespace awh;
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
		return 2;
	// Инициализируем ядро
	fmk::initialize();
	// Получаем выбранные параметры сценария
	const uint32_t method = static_cast <uint32_t> (::strtoul(argv[1], nullptr, 10));
	const bool encrypted = (::strtoul(argv[2], nullptr, 10) != 0);
	const string scenario(argv[3]);
	// Создаём модули сжатия и шифрования
	compressor::block_t compressor;
	crypto_t crypto;
	crypto.salt("abc-damage-salt");
	crypto.password("abc-damage-password");
	// Настраиваем укладчик
	abc::packer_t packer;
	packer.compressor(&compressor);
	packer.crypto(&crypto);
	auto settings = packer.settings();
	settings.text = static_cast <compressor::method_t> (method);
	settings.encrypt = encrypted;
	packer.settings(settings);
	// Готовим известное содержимое
	const string payload(1024 * 1024, 'a');
	vector <uint8_t> frame;
	if(!packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 1, 0, frame))
		return 3;
	// Исключаем неявный отказ от сжатия
	if((frame.at(0) != method) || (((frame.at(1) & 1) != 0) != encrypted))
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
		frame.resize((cut == 32) ? (frame.size() - 1) : ((cut == 33) ? (abc::CHUNK_HEADER + 1) : ((cut == 34) ? (abc::CHUNK_HEADER + (frame.size() - abc::CHUNK_HEADER) / 2) : cut)));
		expected = abc::error_t::TRUNCATED_CHUNK;
	} else if(scenario == "length-max"){
		abc::fixed(frame.data() + 4, UINT32_MAX, 4);
		expected = abc::error_t::TRUNCATED_CHUNK;
	} else if(scenario == "origin-max"){
		abc::fixed(frame.data() + 8, UINT32_MAX, 4);
		checksum = true;
		expected = abc::error_t::INVALID_CHUNK;
	} else if(scenario == "origin-zero"){
		abc::fixed(frame.data() + 8, 0, 4);
		checksum = true;
		expected = abc::error_t::INVALID_CHUNK;
	} else if(scenario == "origin-small"){
		abc::fixed(frame.data() + 8, 64, 4);
		checksum = true;
		expected = abc::error_t::COMPRESSION_FAILED;
	} else if(scenario == "checksum"){
		frame.back() ^= 0xFF;
		expected = abc::error_t::INVALID_CHECKSUM;
	} else if((scenario == "stream-zero") || (scenario == "stream-half")){
		// Сначала открываем шифрование, чтобы испортить именно сжатый поток
		vector <uint8_t> body(frame.begin() + abc::CHUNK_HEADER, frame.end());
		if(encrypted)
			body = crypto.decrypt <vector <uint8_t>> (body, settings.hash, settings.cipher);
		if(body.empty())
			return 5;
		// Усекаем поток либо заменяем его нулями
		if(scenario == "stream-half")
			body.resize(body.size() / 2);
		else fill(body.begin(), body.end(), 0);
		// Возвращаем корректное шифрование вокруг повреждённого сжатого потока
		if(encrypted)
			body = crypto.encrypt <vector <uint8_t>> (body, settings.hash, settings.cipher);
		if(body.empty())
			return 6;
		frame.resize(abc::CHUNK_HEADER);
		frame.insert(frame.end(), body.begin(), body.end());
		abc::fixed(frame.data() + 4, body.size(), 4);
		checksum = true;
		expected = abc::error_t::COMPRESSION_FAILED;
	} else if(scenario != "valid")
		return 7;
	// Пропускаем проверку суммы только для адресной порчи глубинного слоя
	if(checksum)
		abc::fixed(frame.data() + abc::CHUNK_DIGEST, abc::digest(frame.data(), frame.size()), 8);
	// Начинаем чтение с ненулевого смещения
	const size_t prefix = 17;
	frame.insert(frame.begin(), prefix, 0xCC);
	size_t offset = prefix;
	vector <uint8_t> output(19, 0xBB);
	abc::chunk_t chunk;
	struct rusage before, after;
	if(::getrusage(RUSAGE_SELF, &before) != 0)
		return 8;
	const bool accepted = packer.unpack(frame.data(), frame.size(), offset, output, chunk);
	if(::getrusage(RUSAGE_SELF, &after) != 0)
		return 9;
	// Проверяем полное содержимое либо очистку выхода и сохранение смещения
	const bool valid = ((expected == abc::error_t::NONE) ?
	 (accepted && (string(output.begin(), output.end()) == payload) && (offset == frame.size())) :
	 (!accepted && output.empty() && (offset == prefix)));
	// Выводим полный исход сценария
	::printf("RESULT valid=%d accepted=%d error=%u expected=%u output=%zu offset=%zu rss=%ld delta=%ld\n", valid, accepted, static_cast <uint32_t> (packer.error()), static_cast <uint32_t> (expected), output.size(), offset, after.ru_maxrss, after.ru_maxrss - before.ru_maxrss);
	return ((valid && (packer.error() == expected)) ? 0 : 10);
}
