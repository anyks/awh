// Сличение разбора записи CEF: старый модуль ANYKS против кодека AWH
#include <cef.hpp>
#include <chrono>
#include <cstdio>
#include <string>

#include <codec/cef/document.hpp>
#include <codec/cef/reader.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

using namespace std;

static const string RECORD =
	"Feb 17 15:30:15 vnetids emerg CEF:0|InfoTeCS|IDS|2.4.3-371989|1:905590:7|ET POLICY RDP connection confirm|1|"
	"cat=1 cn1=25162858 cn1Label=EventID cnt=73386 cs1=not-suspicious cs1Label=IDSClass cs2=emerging-dos "
	"cs2Label=IDSGroup cs3= cs3Label=CVEID cs4=url,g-soft.info cs4Label=ExternalRef cs5= cs5Label=IDSTags "
	"deviceExternalId=1330334083 deviceFacility=Signature dmac=b0:01:86:30:90:05 dpt=8082 dst=192.168.59.39 "
	"proto=TCP rt=Feb 17 2023 23:30:15.734 YEKT smac=eb:11:0e:37:28:65 spt=22332 src=172.16.0.4";

template <typename F>
static double measure(const size_t rounds, F run){
	run();
	const auto start = chrono::steady_clock::now();
	for(size_t i = 0; i < rounds; i++) run();
	const auto finish = chrono::steady_clock::now();
	return chrono::duration <double> (finish - start).count();
}

// Поверка того, что оба модуля делают ОДНУ И ТУ ЖЕ работу
static void proof(){
	anyks::cef_t old;
	old.mode(anyks::cef_t::mode_t::STRONG);
	old.parse(RECORD);
	awh::fmk_t fmk; awh::log_t log(&fmk); log.mode({});
	awh::codec::cef::document_t doc(&fmk, &log);
	awh::codec::cef::reader_t::settings_t st; st.mode = awh::codec::cef::mode_t::STRONG;
	doc.settings(st);
	const bool ok = doc.parse(RECORD);
	printf("== поверка равенства работы ==\n");
	printf("разбор AWH: %s, пар расширения: %zu\n", (ok ? "годен" : "ОТКАЗ"), doc.size());
	if(!ok)
		printf("отказ: %s в %zu:%zu\n", awh::codec::cef::message(doc.error()),
			doc.errorPosition().line, doc.errorPosition().column);
	printf("старый: dst=%s  AWH: dst=%s\n",
		old.get <string> ("destinationAddress").c_str(),
		doc.field("destinationAddress").text().c_str());
	printf("старый: mac=%s  AWH: mac=%s\n",
		old.get <string> ("deviceMacAddress").c_str(),
		doc.field("deviceMacAddress").text().c_str());
	printf("старый: cn1=%zu  AWH: cn1=%lld\n",
		old.get <size_t> ("deviceCustomNumber1", 0),
		static_cast <long long> ([&]{ int64_t v = 0; doc.field("deviceCustomNumber1").value(v); return v; }()));
	int64_t stamp = 0;
	doc.field("deviceReceiptTime").value(stamp);
	printf("старый: rt=%zu  AWH: rt=%lld  (верно 1676658615734)\n\n",
		old.get <size_t> ("deviceReceiptTime", 0), static_cast <long long> (stamp));
}

int main(int argc, char * argv[]){
	proof();
	const size_t rounds = ((argc > 1) ? strtoul(argv[1], nullptr, 10) : 20000);
	const double bytes = static_cast <double> (RECORD.size() * rounds) / 1048576.0;
	// Старый модуль
	{
		anyks::cef_t cef;
		cef.mode(anyks::cef_t::mode_t::NONE);
		const double seconds = measure(rounds, [&]{ cef.clear(); cef.parse(RECORD); });
		printf("старый ANYKS cef (NONE):   %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / rounds);
	}
	{
		anyks::cef_t cef;
		cef.mode(anyks::cef_t::mode_t::STRONG);
		const double seconds = measure(rounds, [&]{ cef.clear(); cef.parse(RECORD); });
		printf("старый ANYKS cef (STRONG): %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / rounds);
	}
	// Кодек AWH
	awh::fmk_t fmk; awh::log_t log(&fmk); log.mode({});
	{
		awh::codec::cef::reader_t reader(&fmk, &log);
		const double seconds = measure(rounds, [&]{
			reader.reset();
			reader.feed(RECORD);
			while(reader.next());
		});
		printf("AWH codec::cef (поток):    %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / rounds);
	}
	{
		awh::codec::cef::document_t doc(&fmk, &log);
		const double seconds = measure(rounds, [&]{ doc.parse(RECORD); });
		printf("AWH codec::cef (дерево):   %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / rounds);
	}
	{
		awh::codec::cef::document_t doc(&fmk, &log);
		awh::codec::cef::reader_t::settings_t st; st.mode = awh::codec::cef::mode_t::STRONG;
		doc.settings(st);
		const double seconds = measure(rounds, [&]{ doc.parse(RECORD); });
		printf("AWH codec::cef (STRONG):   %8.2f МБ/с  %8.2f мкс/запись\n", bytes / seconds, seconds * 1e6 / rounds);
	}
	return 0;
}
