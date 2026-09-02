// Щуп сплошной проверки помощников описателей
//
// Поля проверяемые лежат в СЕРЕДИНЕ, а соседями им заведомо ненулевые значения:
// в хвосте мусор совпадает с ожидаемым и щуп подтверждает неверный помощник
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <cstdio>

using namespace std;

// Мимикрия очереди `awh::Queue`: имена полей точны, помощнику она неотличима от
// настоящей. Настоящую очередь тянуть незачем - помощники читают только по именам
struct range_mimic {
	uint64_t begin;
	uint64_t end;
	uint64_t count;
	uint64_t offset;
};
struct max_mimic {
	uint64_t memory;
	uint64_t records;
};
struct queue_mimic {
	range_mimic _range;
	vector <uint8_t> _buffer;
	max_mimic _max;
};

struct probe_t {
	uint64_t guard_head;
	string small;
	string large;
	vector <int> numbers;
	unordered_map <int, int> pairs;
	queue_mimic queue;
	shared_ptr <int> owned;
	shared_ptr <int> empty;
	atomic <size_t> counter;
	atomic_bool raised;
	atomic_bool lowered;
	deque <int> tasks;
	uint64_t guard_tail;
};

int main(){
	probe_t probe;
	probe.guard_head = 0xAAAAAAAAAAAAAAAAull;
	probe.small = "Юрий";
	probe.large = string(200, 'w');
	probe.numbers = {10, 20, 30, 40, 50};
	probe.pairs = {{1, 1}, {2, 2}, {3, 3}};
	// Очередь: 4 записи, занятый кусок 200 Б; впереди содержимого лежит длина ближайшей
	// записи (50), из неё уже прочитано 10 - к выдаче остаётся 40
	probe.queue._range = {0, 200, 4, 10};
	probe.queue._buffer.assign(50, 0x7E);
	for(int i = 0; i < 8; i++)
		probe.queue._buffer[i] = static_cast <uint8_t> ((50ull >> (i * 8)) & 0xFF);
	probe.queue._max = {0, 0};
	probe.owned = make_shared <int> (42);
	probe.counter.store(230700);
	probe.raised.store(true);
	probe.lowered.store(false);
	probe.tasks = {1, 2, 3, 4};
	probe.guard_tail = 0xBBBBBBBBBBBBBBBBull;
	printf("small=%zu large=%zu numbers=%zu pairs=%zu tasks=%zu queue=%zu\n",
		probe.small.size(), probe.large.size(), probe.numbers.size(), probe.pairs.size(),
		probe.tasks.size(), probe.queue._buffer.size());
	return 0;
}
