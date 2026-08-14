/**
 * @file nwt.cpp
 * @date 2025-10-25
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
 * @brief Реализация модуля определения типов сетевых адресов — распознавание во входной строке URL, домена,
 *        IP-адреса, MAC-адреса, e-mail или пути файловой системы и разбор URL-адреса на составные части
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартный заголовочный файл
 */
#include <mutex>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <net/nwt.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические типы данных в анонимное пространство имён
 *
 */
namespace {
	/**
	 * @brief Флаг одноразовой инициализации доменных зон (для всех объектов)
	 *
	 */
	once_flag __awh_init_once__;

	/**
	 * @brief Список основных доменных зон интернета
	 *
	 */
	static unordered_set <string> __awh_general_domains__;
	/**
	 * @brief Список интернациональных доменных зон интернета
	 *
	 */
	static unordered_set <string> __awh_national_domains__;
};

/**
 * @brief Инкапсулируем статические функции работы с доменными именами в анонимное пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция инициализации общих списков доменных зон (вызывается единожды)
	 *
	 */
	void init() noexcept {
		/**
		 * @brief Полный список общих доменных зон (включая национальные, спонсорские и инфраструктурные)
		 *
		 * @note Источник: https://data.iana.org/TLD/tlds-alpha-by-domain.txt
		 * @note База корневой зоны (человекочитаемая, с типами country-code/generic и т.д.): https://www.iana.org/domains/root/db
		 *
		 */
		// Создаем список национальных доменов
		::__awh_national_domains__.emplace("ac");
		::__awh_national_domains__.emplace("ad");
		::__awh_national_domains__.emplace("ae");
		::__awh_national_domains__.emplace("af");
		::__awh_national_domains__.emplace("ag");
		::__awh_national_domains__.emplace("ai");
		::__awh_national_domains__.emplace("al");
		::__awh_national_domains__.emplace("am");
		::__awh_national_domains__.emplace("an");
		::__awh_national_domains__.emplace("ao");
		::__awh_national_domains__.emplace("aq");
		::__awh_national_domains__.emplace("ar");
		::__awh_national_domains__.emplace("as");
		::__awh_national_domains__.emplace("at");
		::__awh_national_domains__.emplace("au");
		::__awh_national_domains__.emplace("aw");
		::__awh_national_domains__.emplace("ax");
		::__awh_national_domains__.emplace("az");
		::__awh_national_domains__.emplace("ba");
		::__awh_national_domains__.emplace("bb");
		::__awh_national_domains__.emplace("bd");
		::__awh_national_domains__.emplace("be");
		::__awh_national_domains__.emplace("bf");
		::__awh_national_domains__.emplace("bg");
		::__awh_national_domains__.emplace("bh");
		::__awh_national_domains__.emplace("bi");
		::__awh_national_domains__.emplace("bj");
		::__awh_national_domains__.emplace("bm");
		::__awh_national_domains__.emplace("bn");
		::__awh_national_domains__.emplace("bo");
		::__awh_national_domains__.emplace("br");
		::__awh_national_domains__.emplace("bs");
		::__awh_national_domains__.emplace("bt");
		::__awh_national_domains__.emplace("bv");
		::__awh_national_domains__.emplace("bw");
		::__awh_national_domains__.emplace("by");
		::__awh_national_domains__.emplace("bz");
		::__awh_national_domains__.emplace("ca");
		::__awh_national_domains__.emplace("cc");
		::__awh_national_domains__.emplace("cd");
		::__awh_national_domains__.emplace("cf");
		::__awh_national_domains__.emplace("cg");
		::__awh_national_domains__.emplace("ch");
		::__awh_national_domains__.emplace("ci");
		::__awh_national_domains__.emplace("ck");
		::__awh_national_domains__.emplace("cl");
		::__awh_national_domains__.emplace("cm");
		::__awh_national_domains__.emplace("cn");
		::__awh_national_domains__.emplace("co");
		::__awh_national_domains__.emplace("cr");
		::__awh_national_domains__.emplace("cu");
		::__awh_national_domains__.emplace("cv");
		::__awh_national_domains__.emplace("cx");
		::__awh_national_domains__.emplace("cy");
		::__awh_national_domains__.emplace("cz");
		::__awh_national_domains__.emplace("dd");
		::__awh_national_domains__.emplace("de");
		::__awh_national_domains__.emplace("dj");
		::__awh_national_domains__.emplace("dk");
		::__awh_national_domains__.emplace("dm");
		::__awh_national_domains__.emplace("do");
		::__awh_national_domains__.emplace("dz");
		::__awh_national_domains__.emplace("ec");
		::__awh_national_domains__.emplace("ee");
		::__awh_national_domains__.emplace("eg");
		::__awh_national_domains__.emplace("er");
		::__awh_national_domains__.emplace("es");
		::__awh_national_domains__.emplace("et");
		::__awh_national_domains__.emplace("eu");
		::__awh_national_domains__.emplace("fi");
		::__awh_national_domains__.emplace("fj");
		::__awh_national_domains__.emplace("fk");
		::__awh_national_domains__.emplace("fm");
		::__awh_national_domains__.emplace("fo");
		::__awh_national_domains__.emplace("fr");
		::__awh_national_domains__.emplace("ga");
		::__awh_national_domains__.emplace("gb");
		::__awh_national_domains__.emplace("gd");
		::__awh_national_domains__.emplace("ge");
		::__awh_national_domains__.emplace("gf");
		::__awh_national_domains__.emplace("gg");
		::__awh_national_domains__.emplace("gh");
		::__awh_national_domains__.emplace("gi");
		::__awh_national_domains__.emplace("gl");
		::__awh_national_domains__.emplace("gm");
		::__awh_national_domains__.emplace("gn");
		::__awh_national_domains__.emplace("gp");
		::__awh_national_domains__.emplace("gq");
		::__awh_national_domains__.emplace("gr");
		::__awh_national_domains__.emplace("gs");
		::__awh_national_domains__.emplace("gt");
		::__awh_national_domains__.emplace("gu");
		::__awh_national_domains__.emplace("gw");
		::__awh_national_domains__.emplace("gy");
		::__awh_national_domains__.emplace("hk");
		::__awh_national_domains__.emplace("hm");
		::__awh_national_domains__.emplace("hn");
		::__awh_national_domains__.emplace("hr");
		::__awh_national_domains__.emplace("ht");
		::__awh_national_domains__.emplace("hu");
		::__awh_national_domains__.emplace("id");
		::__awh_national_domains__.emplace("ie");
		::__awh_national_domains__.emplace("il");
		::__awh_national_domains__.emplace("im");
		::__awh_national_domains__.emplace("in");
		::__awh_national_domains__.emplace("io");
		::__awh_national_domains__.emplace("iq");
		::__awh_national_domains__.emplace("ir");
		::__awh_national_domains__.emplace("is");
		::__awh_national_domains__.emplace("it");
		::__awh_national_domains__.emplace("je");
		::__awh_national_domains__.emplace("jm");
		::__awh_national_domains__.emplace("jo");
		::__awh_national_domains__.emplace("jp");
		::__awh_national_domains__.emplace("ke");
		::__awh_national_domains__.emplace("kg");
		::__awh_national_domains__.emplace("kh");
		::__awh_national_domains__.emplace("ki");
		::__awh_national_domains__.emplace("km");
		::__awh_national_domains__.emplace("kn");
		::__awh_national_domains__.emplace("kp");
		::__awh_national_domains__.emplace("kr");
		::__awh_national_domains__.emplace("kw");
		::__awh_national_domains__.emplace("ky");
		::__awh_national_domains__.emplace("kz");
		::__awh_national_domains__.emplace("la");
		::__awh_national_domains__.emplace("lb");
		::__awh_national_domains__.emplace("lc");
		::__awh_national_domains__.emplace("li");
		::__awh_national_domains__.emplace("lk");
		::__awh_national_domains__.emplace("lr");
		::__awh_national_domains__.emplace("ls");
		::__awh_national_domains__.emplace("lt");
		::__awh_national_domains__.emplace("lu");
		::__awh_national_domains__.emplace("lv");
		::__awh_national_domains__.emplace("ly");
		::__awh_national_domains__.emplace("ma");
		::__awh_national_domains__.emplace("mc");
		::__awh_national_domains__.emplace("md");
		::__awh_national_domains__.emplace("me");
		::__awh_national_domains__.emplace("mg");
		::__awh_national_domains__.emplace("mh");
		::__awh_national_domains__.emplace("mk");
		::__awh_national_domains__.emplace("ml");
		::__awh_national_domains__.emplace("mm");
		::__awh_national_domains__.emplace("mn");
		::__awh_national_domains__.emplace("mo");
		::__awh_national_domains__.emplace("mp");
		::__awh_national_domains__.emplace("mq");
		::__awh_national_domains__.emplace("mr");
		::__awh_national_domains__.emplace("ms");
		::__awh_national_domains__.emplace("mt");
		::__awh_national_domains__.emplace("mu");
		::__awh_national_domains__.emplace("mv");
		::__awh_national_domains__.emplace("mw");
		::__awh_national_domains__.emplace("mx");
		::__awh_national_domains__.emplace("my");
		::__awh_national_domains__.emplace("mz");
		::__awh_national_domains__.emplace("na");
		::__awh_national_domains__.emplace("nc");
		::__awh_national_domains__.emplace("ne");
		::__awh_national_domains__.emplace("nf");
		::__awh_national_domains__.emplace("ng");
		::__awh_national_domains__.emplace("ni");
		::__awh_national_domains__.emplace("nl");
		::__awh_national_domains__.emplace("no");
		::__awh_national_domains__.emplace("np");
		::__awh_national_domains__.emplace("nr");
		::__awh_national_domains__.emplace("nu");
		::__awh_national_domains__.emplace("nz");
		::__awh_national_domains__.emplace("om");
		::__awh_national_domains__.emplace("pa");
		::__awh_national_domains__.emplace("pe");
		::__awh_national_domains__.emplace("pf");
		::__awh_national_domains__.emplace("pg");
		::__awh_national_domains__.emplace("ph");
		::__awh_national_domains__.emplace("pk");
		::__awh_national_domains__.emplace("pl");
		::__awh_national_domains__.emplace("pm");
		::__awh_national_domains__.emplace("pn");
		::__awh_national_domains__.emplace("pr");
		::__awh_national_domains__.emplace("ps");
		::__awh_national_domains__.emplace("pt");
		::__awh_national_domains__.emplace("pw");
		::__awh_national_domains__.emplace("py");
		::__awh_national_domains__.emplace("qa");
		::__awh_national_domains__.emplace("re");
		::__awh_national_domains__.emplace("ro");
		::__awh_national_domains__.emplace("rs");
		::__awh_national_domains__.emplace("ru");
		::__awh_national_domains__.emplace("рф");
		::__awh_national_domains__.emplace("ру");
		::__awh_national_domains__.emplace("су");
		::__awh_national_domains__.emplace("rw");
		::__awh_national_domains__.emplace("sa");
		::__awh_national_domains__.emplace("sb");
		::__awh_national_domains__.emplace("sc");
		::__awh_national_domains__.emplace("sd");
		::__awh_national_domains__.emplace("se");
		::__awh_national_domains__.emplace("sg");
		::__awh_national_domains__.emplace("sh");
		::__awh_national_domains__.emplace("si");
		::__awh_national_domains__.emplace("sj");
		::__awh_national_domains__.emplace("sk");
		::__awh_national_domains__.emplace("sl");
		::__awh_national_domains__.emplace("sm");
		::__awh_national_domains__.emplace("sn");
		::__awh_national_domains__.emplace("so");
		::__awh_national_domains__.emplace("sr");
		::__awh_national_domains__.emplace("st");
		::__awh_national_domains__.emplace("su");
		::__awh_national_domains__.emplace("sv");
		::__awh_national_domains__.emplace("sy");
		::__awh_national_domains__.emplace("sz");
		::__awh_national_domains__.emplace("tc");
		::__awh_national_domains__.emplace("td");
		::__awh_national_domains__.emplace("tf");
		::__awh_national_domains__.emplace("tg");
		::__awh_national_domains__.emplace("th");
		::__awh_national_domains__.emplace("tj");
		::__awh_national_domains__.emplace("tk");
		::__awh_national_domains__.emplace("tl");
		::__awh_national_domains__.emplace("tm");
		::__awh_national_domains__.emplace("tn");
		::__awh_national_domains__.emplace("to");
		::__awh_national_domains__.emplace("tp");
		::__awh_national_domains__.emplace("tr");
		::__awh_national_domains__.emplace("tt");
		::__awh_national_domains__.emplace("tv");
		::__awh_national_domains__.emplace("tw");
		::__awh_national_domains__.emplace("tz");
		::__awh_national_domains__.emplace("ua");
		::__awh_national_domains__.emplace("ug");
		::__awh_national_domains__.emplace("uk");
		::__awh_national_domains__.emplace("us");
		::__awh_national_domains__.emplace("uy");
		::__awh_national_domains__.emplace("uz");
		::__awh_national_domains__.emplace("va");
		::__awh_national_domains__.emplace("vc");
		::__awh_national_domains__.emplace("ve");
		::__awh_national_domains__.emplace("vg");
		::__awh_national_domains__.emplace("vi");
		::__awh_national_domains__.emplace("vn");
		::__awh_national_domains__.emplace("vu");
		::__awh_national_domains__.emplace("wf");
		::__awh_national_domains__.emplace("ws");
		::__awh_national_domains__.emplace("ye");
		::__awh_national_domains__.emplace("yt");
		::__awh_national_domains__.emplace("za");
		::__awh_national_domains__.emplace("zm");
		::__awh_national_domains__.emplace("zw");
		::__awh_national_domains__.emplace("krd");
		::__awh_national_domains__.emplace("укр");
		::__awh_national_domains__.emplace("срб");
		::__awh_national_domains__.emplace("мон");
		::__awh_national_domains__.emplace("бел");
		::__awh_national_domains__.emplace("ком");
		::__awh_national_domains__.emplace("нет");
		::__awh_national_domains__.emplace("биз");
		::__awh_national_domains__.emplace("орг");
		::__awh_national_domains__.emplace("инфо");
		// Создаем список общих доменов
		::__awh_general_domains__.emplace("app");
		::__awh_general_domains__.emplace("biz");
		::__awh_general_domains__.emplace("cat");
		::__awh_general_domains__.emplace("com");
		::__awh_general_domains__.emplace("edu");
		::__awh_general_domains__.emplace("eus");
		::__awh_general_domains__.emplace("gov");
		::__awh_general_domains__.emplace("int");
		::__awh_general_domains__.emplace("mil");
		::__awh_general_domains__.emplace("net");
		::__awh_general_domains__.emplace("one");
		::__awh_general_domains__.emplace("ong");
		::__awh_general_domains__.emplace("onl");
		::__awh_general_domains__.emplace("ooo");
		::__awh_general_domains__.emplace("org");
		::__awh_general_domains__.emplace("pro");
		::__awh_general_domains__.emplace("red");
		::__awh_general_domains__.emplace("ren");
		::__awh_general_domains__.emplace("tel");
		::__awh_general_domains__.emplace("xxx");
		::__awh_general_domains__.emplace("xyz");
		::__awh_general_domains__.emplace("gold");
		::__awh_general_domains__.emplace("rent");
		::__awh_general_domains__.emplace("name");
		::__awh_general_domains__.emplace("aero");
		::__awh_general_domains__.emplace("mobi");
		::__awh_general_domains__.emplace("jobs");
		::__awh_general_domains__.emplace("info");
		::__awh_general_domains__.emplace("coop");
		::__awh_general_domains__.emplace("asia");
		::__awh_general_domains__.emplace("army");
		::__awh_general_domains__.emplace("pics");
		::__awh_general_domains__.emplace("pink");
		::__awh_general_domains__.emplace("plus");
		::__awh_general_domains__.emplace("porn");
		::__awh_general_domains__.emplace("post");
		::__awh_general_domains__.emplace("prof");
		::__awh_general_domains__.emplace("qpon");
		::__awh_general_domains__.emplace("rest");
		::__awh_general_domains__.emplace("rich");
		::__awh_general_domains__.emplace("site");
		::__awh_general_domains__.emplace("yoga");
		::__awh_general_domains__.emplace("zone");
		::__awh_general_domains__.emplace("local");
		::__awh_general_domains__.emplace("rehab");
		::__awh_general_domains__.emplace("press");
		::__awh_general_domains__.emplace("poker");
		::__awh_general_domains__.emplace("parts");
		::__awh_general_domains__.emplace("party");
		::__awh_general_domains__.emplace("audio");
		::__awh_general_domains__.emplace("archi");
		::__awh_general_domains__.emplace("dance");
		::__awh_general_domains__.emplace("actor");
		::__awh_general_domains__.emplace("adult");
		::__awh_general_domains__.emplace("photo");
		::__awh_general_domains__.emplace("pizza");
		::__awh_general_domains__.emplace("place");
		::__awh_general_domains__.emplace("travel");
		::__awh_general_domains__.emplace("review");
		::__awh_general_domains__.emplace("repair");
		::__awh_general_domains__.emplace("report");
		::__awh_general_domains__.emplace("racing");
		::__awh_general_domains__.emplace("photos");
		::__awh_general_domains__.emplace("physio");
		::__awh_general_domains__.emplace("online");
		::__awh_general_domains__.emplace("museum");
		::__awh_general_domains__.emplace("agency");
		::__awh_general_domains__.emplace("active");
		::__awh_general_domains__.emplace("reviews");
		::__awh_general_domains__.emplace("rentals");
		::__awh_general_domains__.emplace("recipes");
		::__awh_general_domains__.emplace("organic");
		::__awh_general_domains__.emplace("academy");
		::__awh_general_domains__.emplace("auction");
		::__awh_general_domains__.emplace("plumbing");
		::__awh_general_domains__.emplace("pharmacy");
		::__awh_general_domains__.emplace("airforce");
		::__awh_general_domains__.emplace("attorney");
		::__awh_general_domains__.emplace("partners");
		::__awh_general_domains__.emplace("pictures");
		::__awh_general_domains__.emplace("feedback");
		::__awh_general_domains__.emplace("property");
		::__awh_general_domains__.emplace("republican");
		::__awh_general_domains__.emplace("associates");
		::__awh_general_domains__.emplace("apartments");
		::__awh_general_domains__.emplace("accountant");
		::__awh_general_domains__.emplace("properties");
		::__awh_general_domains__.emplace("photography");
		::__awh_general_domains__.emplace("accountants");
		::__awh_general_domains__.emplace("productions");
		// Создаём список современных общих доменов (новые gTLD, появившиеся за последнее десятилетие)
		::__awh_general_domains__.emplace("art");
		::__awh_general_domains__.emplace("bio");
		::__awh_general_domains__.emplace("dev");
		::__awh_general_domains__.emplace("fun");
		::__awh_general_domains__.emplace("fit");
		::__awh_general_domains__.emplace("fyi");
		::__awh_general_domains__.emplace("gay");
		::__awh_general_domains__.emplace("ink");
		::__awh_general_domains__.emplace("inc");
		::__awh_general_domains__.emplace("llc");
		::__awh_general_domains__.emplace("ltd");
		::__awh_general_domains__.emplace("ltda");
		::__awh_general_domains__.emplace("ngo");
		::__awh_general_domains__.emplace("pet");
		::__awh_general_domains__.emplace("pub");
		::__awh_general_domains__.emplace("run");
		::__awh_general_domains__.emplace("ski");
		::__awh_general_domains__.emplace("tax");
		::__awh_general_domains__.emplace("top");
		::__awh_general_domains__.emplace("vet");
		::__awh_general_domains__.emplace("vip");
		::__awh_general_domains__.emplace("win");
		::__awh_general_domains__.emplace("wtf");
		::__awh_general_domains__.emplace("zip");
		::__awh_general_domains__.emplace("bar");
		::__awh_general_domains__.emplace("bet");
		::__awh_general_domains__.emplace("buy");
		::__awh_general_domains__.emplace("cab");
		::__awh_general_domains__.emplace("cam");
		::__awh_general_domains__.emplace("car");
		::__awh_general_domains__.emplace("ceo");
		::__awh_general_domains__.emplace("eco");
		::__awh_general_domains__.emplace("fan");
		::__awh_general_domains__.emplace("law");
		::__awh_general_domains__.emplace("new");
		::__awh_general_domains__.emplace("now");
		::__awh_general_domains__.emplace("sex");
		::__awh_general_domains__.emplace("tab");
		::__awh_general_domains__.emplace("club");
		::__awh_general_domains__.emplace("blog");
		::__awh_general_domains__.emplace("shop");
		::__awh_general_domains__.emplace("tech");
		::__awh_general_domains__.emplace("site");
		::__awh_general_domains__.emplace("page");
		::__awh_general_domains__.emplace("wiki");
		::__awh_general_domains__.emplace("live");
		::__awh_general_domains__.emplace("life");
		::__awh_general_domains__.emplace("news");
		::__awh_general_domains__.emplace("chat");
		::__awh_general_domains__.emplace("city");
		::__awh_general_domains__.emplace("club");
		::__awh_general_domains__.emplace("code");
		::__awh_general_domains__.emplace("cool");
		::__awh_general_domains__.emplace("data");
		::__awh_general_domains__.emplace("deal");
		::__awh_general_domains__.emplace("diet");
		::__awh_general_domains__.emplace("fund");
		::__awh_general_domains__.emplace("game");
		::__awh_general_domains__.emplace("gift");
		::__awh_general_domains__.emplace("guru");
		::__awh_general_domains__.emplace("haus");
		::__awh_general_domains__.emplace("help");
		::__awh_general_domains__.emplace("home");
		::__awh_general_domains__.emplace("host");
		::__awh_general_domains__.emplace("link");
		::__awh_general_domains__.emplace("loan");
		::__awh_general_domains__.emplace("menu");
		::__awh_general_domains__.emplace("team");
		::__awh_general_domains__.emplace("tips");
		::__awh_general_domains__.emplace("toys");
		::__awh_general_domains__.emplace("town");
		::__awh_general_domains__.emplace("vote");
		::__awh_general_domains__.emplace("wine");
		::__awh_general_domains__.emplace("work");
		::__awh_general_domains__.emplace("show");
		::__awh_general_domains__.emplace("sale");
		::__awh_general_domains__.emplace("care");
		::__awh_general_domains__.emplace("cash");
		::__awh_general_domains__.emplace("cards");
		::__awh_general_domains__.emplace("click");
		::__awh_general_domains__.emplace("cloud");
		::__awh_general_domains__.emplace("codes");
		::__awh_general_domains__.emplace("coupons");
		::__awh_general_domains__.emplace("dance");
		::__awh_general_domains__.emplace("deals");
		::__awh_general_domains__.emplace("email");
		::__awh_general_domains__.emplace("estate");
		::__awh_general_domains__.emplace("events");
		::__awh_general_domains__.emplace("expert");
		::__awh_general_domains__.emplace("family");
		::__awh_general_domains__.emplace("films");
		::__awh_general_domains__.emplace("forum");
		::__awh_general_domains__.emplace("games");
		::__awh_general_domains__.emplace("group");
		::__awh_general_domains__.emplace("guide");
		::__awh_general_domains__.emplace("homes");
		::__awh_general_domains__.emplace("legal");
		::__awh_general_domains__.emplace("media");
		::__awh_general_domains__.emplace("money");
		::__awh_general_domains__.emplace("movie");
		::__awh_general_domains__.emplace("music");
		::__awh_general_domains__.emplace("ninja");
		::__awh_general_domains__.emplace("photo");
		::__awh_general_domains__.emplace("pizza");
		::__awh_general_domains__.emplace("plus");
		::__awh_general_domains__.emplace("promo");
		::__awh_general_domains__.emplace("salon");
		::__awh_general_domains__.emplace("shoes");
		::__awh_general_domains__.emplace("space");
		::__awh_general_domains__.emplace("store");
		::__awh_general_domains__.emplace("study");
		::__awh_general_domains__.emplace("style");
		::__awh_general_domains__.emplace("tools");
		::__awh_general_domains__.emplace("tours");
		::__awh_general_domains__.emplace("trade");
		::__awh_general_domains__.emplace("video");
		::__awh_general_domains__.emplace("vodka");
		::__awh_general_domains__.emplace("wine");
		::__awh_general_domains__.emplace("world");
		::__awh_general_domains__.emplace("today");
		::__awh_general_domains__.emplace("design");
		::__awh_general_domains__.emplace("studio");
		::__awh_general_domains__.emplace("market");
		::__awh_general_domains__.emplace("mobile");
		::__awh_general_domains__.emplace("online");
		::__awh_general_domains__.emplace("social");
		::__awh_general_domains__.emplace("agency");
		::__awh_general_domains__.emplace("beauty");
		::__awh_general_domains__.emplace("center");
		::__awh_general_domains__.emplace("church");
		::__awh_general_domains__.emplace("coffee");
		::__awh_general_domains__.emplace("dental");
		::__awh_general_domains__.emplace("doctor");
		::__awh_general_domains__.emplace("estate");
		::__awh_general_domains__.emplace("expert");
		::__awh_general_domains__.emplace("garden");
		::__awh_general_domains__.emplace("global");
		::__awh_general_domains__.emplace("health");
		::__awh_general_domains__.emplace("hockey");
		::__awh_general_domains__.emplace("hotels");
		::__awh_general_domains__.emplace("school");
		::__awh_general_domains__.emplace("soccer");
		::__awh_general_domains__.emplace("studio");
		::__awh_general_domains__.emplace("supply");
		::__awh_general_domains__.emplace("systems");
		::__awh_general_domains__.emplace("digital");
		::__awh_general_domains__.emplace("finance");
		::__awh_general_domains__.emplace("fitness");
		::__awh_general_domains__.emplace("gallery");
		::__awh_general_domains__.emplace("hosting");
		::__awh_general_domains__.emplace("network");
		::__awh_general_domains__.emplace("support");
		::__awh_general_domains__.emplace("capital");
		::__awh_general_domains__.emplace("kitchen");
		::__awh_general_domains__.emplace("clothing");
		::__awh_general_domains__.emplace("computer");
		::__awh_general_domains__.emplace("delivery");
		::__awh_general_domains__.emplace("graphics");
		::__awh_general_domains__.emplace("holdings");
		::__awh_general_domains__.emplace("software");
		::__awh_general_domains__.emplace("solutions");
		::__awh_general_domains__.emplace("marketing");
		::__awh_general_domains__.emplace("community");
		::__awh_general_domains__.emplace("financial");
		::__awh_general_domains__.emplace("furniture");
		::__awh_general_domains__.emplace("insurance");
		::__awh_general_domains__.emplace("technology");
		::__awh_general_domains__.emplace("university");
		::__awh_general_domains__.emplace("consulting");
		::__awh_general_domains__.emplace("restaurant");
		::__awh_general_domains__.emplace("foundation");
		::__awh_general_domains__.emplace("enterprises");
		::__awh_general_domains__.emplace("investments");
		::__awh_general_domains__.emplace("construction");
		::__awh_general_domains__.emplace("international");
	}
};

/**
 * @brief Инкапсулируем внутренние функции-помощники для собственных парсеров адресов в анонимное пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция проверка ASCII-цифры
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsDigit(const char letter) noexcept {
		// Возвращаем результат проверки
		return ((letter >= '0') && (letter <= '9'));
	}
	/**
	 * @brief Функция проверки шестнадцатеричной цифры
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsHex(const char letter) noexcept {
		// Возвращаем результат проверки
		return (
			ntIsDigit(letter) ||
			((letter >= 'a') && (letter <= 'f')) ||
			((letter >= 'A') && (letter <= 'F'))
		);
	}
	/**
	 * @brief Функция проверки ASCII-буквы
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsAlpha(const char letter) noexcept {
		// Возвращаем результат проверки
		return (
			((letter >= 'a') && (letter <= 'z')) ||
			((letter >= 'A') && (letter <= 'Z'))
		);
	}
	/**
	 * @brief Функция проверки символа метки доменного имени (буквы, цифры, дефис или любой не-ASCII байт UTF-8)
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsLabel(const char letter) noexcept {
		// Возвращаем результат проверки (не-ASCII байты считаем частью интернационализированной метки)
		return (
			ntIsAlpha(letter) ||
			ntIsDigit(letter) ||
			(letter == '-') ||
			(static_cast <unsigned char> (letter) >= 0x80)
		);
	}
	/**
	 * @brief Функция проверки символа логина/пароля URL-адреса
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsUser(const char letter) noexcept {
		// Возвращаем результат проверки
		return (
			ntIsAlpha(letter) ||
			ntIsDigit(letter) ||
			(letter == '_') ||
			(letter == '+') ||
			(letter == '-')
		);
	}
	/**
	 * @brief Функция проверки символа пути URL-адреса
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsPath(const char letter) noexcept {
		// Возвращаем результат проверки
		return (
			ntIsAlpha(letter) ||
			ntIsDigit(letter) ||
			(letter == '-') ||
			(letter == '_') ||
			(letter == '.') ||
			(letter == '~') ||
			(letter == '/')
		);
	}
	/**
	 * @brief Функция проверки символа параметров запроса URL-адреса
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsParam(const char letter) noexcept {
		// Если символ является буквой, цифрой или подчёркиванием
		if(ntIsAlpha(letter) || ntIsDigit(letter) || (letter == '_'))
			// Сообщаем, что символ допустим
			return true;
		/**
		 * Определяем допустимый символ параметров запроса
		 */
		switch(letter){
			case '-': case '.': case '~': case ':': case '[': case ']': case '@':
			case '!': case '$': case '&': case '\'': case '(': case ')': case '*':
			case '+': case ',': case ';': case '=':
				// Сообщаем, что символ допустим
				return true;
		}
		// Сообщаем, что символ недопустим
		return false;
	}
	/**
	 * @brief Функция проверки символа якоря URL-адреса
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	static inline bool ntIsAnchor(const char letter) noexcept {
		// Возвращаем результат проверки
		return (
			ntIsAlpha(letter) ||
			ntIsDigit(letter) ||
			(letter == '_') ||
			(letter == '-')
		);
	}
	/**
	 * @brief Функция сравнения текста с литералом без учёта регистра (литерал в нижнем регистре ASCII)
	 *
	 * @param text текст для сравнения
	 * @param pos  позиция начала сравнения
	 * @param lit  литерал для сравнения
	 * @return     результат сравнения
	 *
	 */
	static bool ntMatchCI(string_view text, const size_t pos, const char * lit) noexcept {
		// Индекс перебора литерала
		size_t k = 0;
		/**
		 * Выполняем перебор всех символов литерала
		 */
		while(lit[k] != '\0'){
			// Если позиция вышла за границы текста
			if((pos + k) >= text.size())
				// Сообщаем о несовпадении
				return false;
			// Получаем символ текста
			char letter = text[pos + k];
			// Приводим символ к нижнему регистру
			if((letter >= 'A') && (letter <= 'Z'))
				// Выполняем понижение регистра символа
				letter = static_cast <char> (letter - 'A' + 'a');
			// Если символы не совпадают
			if(letter != lit[k])
				// Сообщаем о несовпадении
				return false;
			// Переходим к следующему символу
			++k;
		}
		// Сообщаем о совпадении
		return true;
	}
	/**
	 * @brief Функция проверки валидности домена верхнего уровня (только буквы/UTF-8 или punycode-форма xn--)
	 *
	 * @param tld проверяемый домен верхнего уровня
	 * @return    результат проверки
	 *
	 */
	static bool ntIsValidTLD(string_view tld) noexcept {
		// Если домен верхнего уровня не передан
		if(tld.empty())
			// Сообщаем о невалидности
			return false;
		// Если домен является интернационализированным (punycode)
		if((tld.size() > 4) && (tld.compare(0, 4, "xn--") == 0))
			// Сообщаем о валидности
			return true;
		/**
		 * Выполняем перебор всех символов домена верхнего уровня
		 */
		for(const char letter : tld){
			// Домен верхнего уровня не может содержать цифры или дефис (только буквы либо символы UTF-8)
			if(!(ntIsAlpha(letter) || (static_cast <unsigned char> (letter) >= 0x80)))
				// Сообщаем о невалидности
				return false;
		}
		// Сообщаем о валидности
		return true;
	}
	/**
	 * @brief Функция парсинга IPv4-адреса начиная с указанной позиции
	 *
	 * @param text текст для парсинга
	 * @param pos  позиция начала парсинга
	 * @return     позиция конца адреса либо позиция начала при неудаче
	 *
	 */
	static size_t ntParseIPv4(string_view text, const size_t pos) noexcept {
		// Результат работы функции
		size_t result = pos;
		// Получаем размер текста
		const size_t size = text.size();
		/**
		 * Выполняем перебор четырёх октетов адреса
		 */
		for(int32_t octet = 0; octet < 4; ++octet){
			// Если это не первый октет
			if(octet > 0){
				// Между октетами должна быть точка
				if((result >= size) || (text[result] != '.'))
					// Сообщаем о неудаче
					return pos;
				// Пропускаем точку
				++result;
			}
			// Значение октета
			uint32_t value = 0;
			// Начало октета
			const size_t start = result;
			/**
			 * Считываем от одной до трёх цифр октета
			 */
			while((result < size) && ntIsDigit(text[result]) && ((result - start) < 3)){
				// Накапливаем значение октета
				value = (value * 10) + static_cast <uint32_t> (text[result] - '0');
				// Переходим к следующей цифре
				++result;
			}
			// Если цифр не было или значение октета вне диапазона
			if((result == start) || (value > 255))
				// Сообщаем о неудаче
				return pos;
		}
		// Возвращаем позицию конца адреса
		return result;
	}
	/**
	 * @brief Функция парсинга MAC-адреса начиная с указанной позиции
	 *
	 * @param text текст для парсинга
	 * @param pos  позиция начала парсинга
	 * @return     позиция конца адреса либо позиция начала при неудаче
	 *
	 */
	static size_t ntParseMAC(string_view text, const size_t pos) noexcept {
		// Результат работы функции
		size_t result = pos;
		// Получаем размер текста
		const size_t size = text.size();
		/**
		 * Выполняем перебор шести групп адреса
		 */
		for(int32_t group = 0; group < 6; ++group){
			// Если это не первая группа
			if(group > 0){
				// Между группами должно быть двоеточие
				if((result >= size) || (text[result] != ':'))
					// Сообщаем о неудаче
					return pos;
				// Пропускаем двоеточие
				++result;
			}
			// Каждая группа состоит ровно из двух шестнадцатеричных цифр
			if(((result + 2) > size) || !ntIsHex(text[result]) || !ntIsHex(text[result + 1]))
				// Сообщаем о неудаче
				return pos;
			// Смещаем позицию за группу
			result += 2;
		}
		// Возвращаем позицию конца адреса
		return result;
	}
	/**
	 * @brief Функция проверки валидности строки как IPv6-адреса
	 *
	 * @param text проверяемая строка
	 * @return     результат проверки
	 *
	 */
	static bool ntValidateIPv6(string_view text) noexcept {
		// Размер строки
		const size_t size = text.size();
		// IPv6-адрес не может быть короче двух символов
		if(size < 2)
			// Сообщаем о невалидности
			return false;
		// Текущая позиция
		size_t pos = 0;
		// Флаг наличия сжатия нулей "::"
		bool dc = false;
		// Если адрес начинается с ведущего двоеточия
		if(text[0] == ':'){
			// Ведущее двоеточие допустимо только в составе "::"
			if(text[1] != ':')
				// Сообщаем о невалидности
				return false;
			// Запоминаем наличие сжатия и пропускаем "::"
			dc = true; pos = 2;
			// Если строка состоит только из "::" (все нули) — адрес валиден
			if(pos == size)
				// Сообщаем о валидности
				return true;
		}
		// Количество 16-битных групп
		int32_t groups = 0;
		/**
		 * Выполняем разбор групп адреса
		 */
		while(pos < size){
			// Начало группы
			const size_t start = pos;
			/**
			 * Считываем до четырёх шестнадцатеричных цифр группы
			 */
			while((pos < size) && ntIsHex(text[pos]) && ((pos - start) < 4))
				// Переходим к следующему символу
				++pos;
			// Если группа пустая
			if(pos == start)
				// Сообщаем о невалидности
				return false;
			// Если за группой следует точка — это IPv4-хвост адреса
			if((pos < size) && (text[pos] == '.')){
				// IPv4-хвост должен начинаться с начала текущей группы и завершать адрес
				if(ntParseIPv4(text, start) != size)
					// Сообщаем о невалидности
					return false;
				// IPv4-хвост занимает две 16-битные группы
				groups += 2;
				// Адрес завершён
				pos = size;
				// Выходим из разбора
				break;
			}
			// Учитываем разобранную группу
			++groups;
			// Если достигнут конец строки
			if(pos == size)
				// Выходим из разбора
				break;
			// После группы ожидается двоеточие
			if(text[pos] != ':')
				// Сообщаем о невалидности
				return false;
			// Пропускаем двоеточие
			++pos;
			// Если следом ещё одно двоеточие — это сжатие "::"
			if((pos < size) && (text[pos] == ':')){
				// Сжатие может встречаться только один раз
				if(dc)
					// Сообщаем о невалидности
					return false;
				// Запоминаем наличие сжатия и пропускаем второе двоеточие
				dc = true; ++pos;
				// Если адрес заканчивается на "::"
				if(pos == size)
					// Выходим из разбора
					break;
			// Если двоеточие оказалось завершающим одиночным
			} else if(pos == size)
				// Сообщаем о невалидности
				return false;
		}
		// При наличии сжатия групп должно быть от 1 до 7, иначе ровно 8
		return (dc ? ((groups >= 1) && (groups <= 7)) : (groups == 8));
	}
	/**
	 * @brief Функция парсинга IPv6-адреса начиная с указанной позиции
	 *
	 * @param text текст для парсинга
	 * @param pos  позиция начала парсинга
	 * @return     позиция конца адреса либо позиция начала при неудаче
	 *
	 */
	static size_t ntParseIPv6Addr(string_view text, const size_t pos) noexcept {
		// Собираем максимальный кандидат из символов IPv6-адреса (hex, ':', '.')
		size_t result = pos;
		// Размер текста
		const size_t size = text.size();
		/**
		 * Выполняем поиск конца кандидата
		 */
		while((result < size) && (ntIsHex(text[result]) || (text[result] == ':') || (text[result] == '.')))
			// Выполняем смещение позиции
			++result;
		// Если кандидат пуст
		if(result == pos)
			// Сообщаем о неудаче
			return pos;
		// Если кандидат является валидным IPv6-адресом
		if(ntValidateIPv6(text.substr(pos, result - pos)))
			// Возвращаем позицию конца адреса
			return result;
		// Сообщаем о неудаче
		return pos;
	}
	/**
	 * @brief Функция парсинга доменного хоста (метки, разделённые точками, с валидным доменом верхнего уровня)
	 *
	 * @param text   текст для парсинга
	 * @param pos    позиция начала парсинга
	 * @param host   результирующий хост (включая домен верхнего уровня)
	 * @param domain результирующий домен верхнего уровня
	 * @return       позиция конца хоста либо позиция начала при неудаче
	 *
	 */
	static size_t ntParseHost(string_view text, const size_t pos, string & host, string & domain) noexcept {
		// Количество разобранных меток
		size_t labels = 0;
		// Текущий индекс разбора
		size_t index = pos;
		// Конец последней разобранной метки
		size_t hostEnd = pos;
		// Начало последней метки (домена верхнего уровня)
		size_t tldStart = pos;
		// Размер текста
		const size_t size = text.size();
		/**
		 * Выполняем разбор меток доменного имени
		 */
		while((index < size) && ntIsLabel(text[index])){
			// Начало метки
			const size_t start = index;
			/**
			 * Считываем символы метки доменного имени
			 */
			while((index < size) && ntIsLabel(text[index]))
				// Переходим к следующему символу
				++index;
			// Учитываем разобранную метку
			++labels;
			// Запоминаем конец разобранной метки
			hostEnd = index;
			// Запоминаем начало и конец последней метки
			tldStart = start;
			// Если за меткой следует точка и далее есть ещё одна метка
			if((index < size) && (text[index] == '.') && (((index + 1) < size) && ntIsLabel(text[index + 1]))){
				// Пропускаем точку и продолжаем разбор
				++index;
				// Переходим к следующей метке
				continue;
			}
			// Завершаем разбор меток
			break;
		}
		// Требуется минимум две метки (домен второго уровня и домен верхнего уровня)
		if(labels < 2)
			// Сообщаем о неудаче
			return pos;
		// Извлекаем домен верхнего уровня
		string tld{text.substr(tldStart, hostEnd - tldStart)};
		// Если домен верхнего уровня невалиден
		if(!ntIsValidTLD(tld))
			// Сообщаем о неудаче
			return pos;
		// Запоминаем хост целиком (включая домен верхнего уровня)
		host.assign(text.substr(pos, hostEnd - pos));
		// Запоминаем домен верхнего уровня
		domain = ::move(tld);
		// Возвращаем позицию конца хоста
		return hostEnd;
	}
	/**
	 * @brief Функция парсинга URL-адреса начиная с указанной позиции
	 *
	 * @param text текст для парсинга
	 * @param pos  позиция начала парсинга
	 * @param out  результирующие параметры URL-адреса
	 * @return     позиция конца адреса либо позиция начала при неудаче
	 *
	 */
	static size_t ntParseUrl(string_view text, const size_t pos, awh::Network_Types::url_t & out) noexcept {
		// Результат арботы функции
		size_t result = pos;
		// Размер текста
		const size_t size = text.size();
		/**
		 * Используем перечисление типов адресов
		 */
		using types_t = awh::Network_Types::types_t;
		// Выполняем разбор схемы (протокола) URL-адреса
		if(ntMatchCI(text, result, "https://")){
			// Запоминаем схему
			out.schema = "https";
			// Смещаем позицию за схему
			result += 8;
		// Если используется незащищённый протокол
		} else if(ntMatchCI(text, result, "http://")) {
			// Запоминаем схему
			out.schema = "http";
			// Смещаем позицию за схему
			result += 7;
		}
		/**
		 * Выполняем разбор данных пользователя (логин[:пароль]@) с откатом при отсутствии символа '@'
		 */
		{
			// Конец последовательности символов логина
			size_t index = result;
			/**
			 * Считываем символы логина
			 */
			while((index < size) && ntIsUser(text[index]))
				// Переходим к следующему символу
				++index;
			// Если логин не пустой
			if(index > result){
				// Если за логином сразу следует символ '@'
				if((index < size) && (text[index] == '@')){
					// Запоминаем логин пользователя
					out.user.assign(text.substr(result, index - result));
					// Смещаем позицию за символ '@'
					result = (index + 1);
				// Если за логином следует разделитель пароля
				} else if((index < size) && (text[index] == ':')) {
					// Конец последовательности символов пароля
					size_t pos = (index + 1);
					/**
					 * Считываем символы пароля
					 */
					while((pos < size) && ntIsUser(text[pos]))
						// Переходим к следующему символу
						++pos;
					// Если за паролем следует символ '@'
					if((pos < size) && (text[pos] == '@')){
						// Запоминаем логин пользователя
						out.user.assign(text.substr(result, index - result));
						// Запоминаем пароль пользователя
						out.pass.assign(text.substr(index + 1, pos - (index + 1)));
						// Смещаем позицию за символ '@'
						result = (pos + 1);
					}
				}
			}
		}
		// Выполняем разбор хоста URL-адреса
		if((result < size) && (text[result] == '[')){
			// Выполняем разбор бракетированного IPv6-адреса
			const size_t index = ntParseIPv6Addr(text, result + 1);
			// Если IPv6-адрес разобран и закрывающая скобка на месте
			if((index > (result + 1)) && (index < size) && (text[index] == ']')){
				// Запоминаем хост (IP-литерал, домен верхнего уровня отсутствует)
				out.host.assign(text.substr(result + 1, index - (result + 1)));
				// Смещаем позицию за закрывающую скобку
				result = (index + 1);
			// Если хост невалиден
			} else return pos;
		// Если хост является доменным именем
		} else {
			// Выполняем разбор доменного хоста
			const size_t index = ntParseHost(text, result, out.host, out.domain);
			// Если доменный хост не разобран
			if(index == result)
				// Сообщаем о неудаче
				return pos;
			// Смещаем позицию за хост
			result = index;
		}
		// Выполняем разбор порта URL-адреса
		if((result < size) && (text[result] == ':') && ((result + 1) < size) && ntIsDigit(text[result + 1])){
			// Значение порта
			uint64_t port = 0;
			// Конец последовательности цифр порта
			size_t pos = (result + 1);
			/**
			 * Считываем цифры порта
			 */
			while((pos < size) && ntIsDigit(text[pos])){
				// Накапливаем значение порта
				port = (port * 10) + static_cast <uint64_t> (text[pos] - '0');
				// Защищаемся от переполнения значения порта
				if(port > 0xFFFFFFFF)
					// Если значение порта превысило 32-битный диапазон, устанавливаем порт в недопустимое значение
					port = 0xFFFFFFFF;
				// Переходим к следующей цифре
				++pos;
			}
			// Запоминаем порт только если он находится в допустимом диапазоне
			if((port > 0) && (port <= 65535))
				// Запоминаем порт запроса
				out.port = static_cast <uint32_t> (port);
			// Смещаем позицию за порт
			result = pos;
		}
		// Выполняем разбор пути URL-адреса
		if((result < size) && (text[result] == '/')){
			// Конец пути
			size_t pos = result;
			/**
			 * Считываем символы пути
			 */
			while((pos < size) && ntIsPath(text[pos]))
				// Переходим к следующему символу
				++pos;
			// Запоминаем путь запроса
			out.path.assign(text.substr(result, pos - result));
			// Смещаем позицию за путь
			result = pos;
		}
		// Выполняем разбор параметров запроса URL-адреса
		if((result < size) && (text[result] == '?')){
			// Конец параметров запроса
			size_t pos = (result + 1);
			/**
			 * Считываем символы параметров запроса
			 */
			while((pos < size) && ntIsParam(text[pos]))
				// Переходим к следующему символу
				++pos;
			// Запоминаем параметры запроса
			out.params.assign(text.substr(result + 1, pos - (result + 1)));
			// Смещаем позицию за параметры запроса
			result = pos;
		}
		// Выполняем разбор якоря URL-адреса
		if((result < size) && (text[result] == '#')){
			// Конец якоря
			size_t pos = (result + 1);
			/**
			 * Считываем символы якоря
			 */
			while((pos < size) && ntIsAnchor(text[pos]))
				// Переходим к следующему символу
				++pos;
			// Запоминаем якорь запроса
			out.anchor.assign(text.substr(result + 1, pos - (result + 1)));
			// Смещаем позицию за якорь
			result = pos;
		}
		// Запоминаем тип параметра
		out.type = types_t::URL;
		// Запоминаем полный URI-адрес как разобранную подстроку
		out.uri.assign(text.substr(pos, result - pos));
		// Возвращаем позицию конца адреса
		return result;
	}
};

/**
 * @brief Оператор перемещения
 *
 * @param url параметры адреса
 * @return    параметры URL-запроса
 *
 */
awh::Network_Types::URL & awh::Network_Types::URL::operator = (url_t && url) noexcept {
	// Если выполняется попытка самоприсваивания
	if(this == &url)
		// Возвращаем текущий объект
		return (* this);
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем перемещение полного URI-параметров
	this->uri = ::move(url.uri);
	// Выполняем перемещение хоста URL-адреса
	this->host = ::move(url.host);
	// Выполняем перемещение пути URL-адреса
	this->path = ::move(url.path);
	// Выполняем перемещение ника пользователя (для электронной почты)
	this->user = ::move(url.user);
	// Выполняем перемещение пароля пользователя
	this->pass = ::move(url.pass);
	// Выполняем перемещение якоря URL-адреса
	this->anchor = ::move(url.anchor);
	// Выполняем перемещение домена верхнего уровня
	this->domain = ::move(url.domain);
	// Выполняем перемещение параметров URL-адреса
	this->params = ::move(url.params);
	// Выполняем перемещение протокола URL-адреса
	this->schema = ::move(url.schema);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор присванивания
 *
 * @param url параметры адреса
 * @return    параметры URL-запроса
 *
 */
awh::Network_Types::URL & awh::Network_Types::URL::operator = (const url_t & url) noexcept {
	// Если выполняется попытка самоприсваивания
	if(this == &url)
		// Возвращаем текущий объект
		return (* this);
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем копирование полного URI-параметров
	this->uri = url.uri;
	// Выполняем копирование хоста URL-адреса
	this->host = url.host;
	// Выполняем копирование пути URL-адреса
	this->path = url.path;
	// Выполняем копирование ника пользователя (для электронной почты)
	this->user = url.user;
	// Выполняем копирование пароля пользователя
	this->pass = url.pass;
	// Выполняем копирование якоря URL-адреса
	this->anchor = url.anchor;
	// Выполняем копирование домена верхнего уровня
	this->domain = url.domain;
	// Выполняем копирование параметров URL-адреса
	this->params = url.params;
	// Выполняем копирование протокола URL-адреса
	this->schema = url.schema;
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения
 *
 * @param url параметры адреса
 * @return    результат сравнения
 *
 */
bool awh::Network_Types::URL::operator == (const url_t & url) const noexcept {
	// Выполняем сравнение параметров
	return (
		(this->type == url.type) &&
		(this->port == url.port) &&
		(this->uri == url.uri) &&
		(this->host == url.host) &&
		(this->path == url.path) &&
		(this->user == url.user) &&
		(this->pass == url.pass) &&
		(this->anchor == url.anchor) &&
		(this->domain == url.domain) &&
		(this->params == url.params) &&
		(this->schema == url.schema)
	);
}
/**
 * @brief Конструктор перемещения
 *
 * @param url параметры адреса
 *
 */
awh::Network_Types::URL::URL(url_t && url) noexcept {
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем перемещение полного URI-параметров
	this->uri = ::move(url.uri);
	// Выполняем перемещение хоста URL-адреса
	this->host = ::move(url.host);
	// Выполняем перемещение пути URL-адреса
	this->path = ::move(url.path);
	// Выполняем перемещение ника пользователя (для электронной почты)
	this->user = ::move(url.user);
	// Выполняем перемещение пароля пользователя
	this->pass = ::move(url.pass);
	// Выполняем перемещение якоря URL-адреса
	this->anchor = ::move(url.anchor);
	// Выполняем перемещение домена верхнего уровня
	this->domain = ::move(url.domain);
	// Выполняем перемещение параметров URL-адреса
	this->params = ::move(url.params);
	// Выполняем перемещение протокола URL-адреса
	this->schema = ::move(url.schema);
}
/**
 * @brief Конструктор копирования
 *
 * @param url параметры адреса
 *
 */
awh::Network_Types::URL::URL(const url_t & url) noexcept {
	// Выполняем копирование тип URL-адреса
	this->type = url.type;
	// Выполняем копирование порта URL-адреса
	this->port = url.port;
	// Выполняем копирование полного URI-параметров
	this->uri = url.uri;
	// Выполняем копирование хоста URL-адреса
	this->host = url.host;
	// Выполняем копирование пути URL-адреса
	this->path = url.path;
	// Выполняем копирование ника пользователя (для электронной почты)
	this->user = url.user;
	// Выполняем копирование пароля пользователя
	this->pass = url.pass;
	// Выполняем копирование якоря URL-адреса
	this->anchor = url.anchor;
	// Выполняем копирование домена верхнего уровня
	this->domain = url.domain;
	// Выполняем копирование параметров URL-адреса
	this->params = url.params;
	// Выполняем копирование протокола URL-адреса
	this->schema = url.schema;
}
/**
 * @brief Конструктор
 *
 */
awh::Network_Types::URL::URL() noexcept :
 type(types_t::NONE), port(0), uri{""},
 host{""}, path{""}, user{""}, pass{""},
 anchor{""}, domain{""}, params{""}, schema{""} {}

/**
 * @brief Метод проверки, является ли домен верхнего уровня известной доменной зоной
 *
 * @param domain домен верхнего уровня для проверки
 * @return       результат проверки (true, если зона известна)
 *
 */
bool awh::Network_Types::isZone(const string & domain) const noexcept {
	// Если домен верхнего уровня не передан
	if(domain.empty())
		// Сообщаем, что зона неизвестна
		return false;
	// Формируем нормализованный (в нижнем регистре для ASCII) домен верхнего уровня
	string result = "";
	// Резервируем память под нормализованный домен
	result.reserve(domain.size());
	/**
	 * Выполняем перебор всех символов домена верхнего уровня
	 */
	for(const char letter : domain)
		// Приводим ASCII-символы к нижнему регистру, остальные байты (UTF-8) оставляем как есть
		result.push_back(((letter >= 'A') && (letter <= 'Z')) ? static_cast <char> (letter - 'A' + 'a') : letter);
	// Если домен является интернационализированным (punycode), считаем зону валидной
	if((result.size() > 4) && (result.compare(0, 4, "xn--") == 0))
		// Сообщаем, что зона известна
		return true;
	// Сообщаем результат проверки наличия зоны среди интернациональных, общих или пользовательских зон
	return (
		(::__awh_national_domains__.find(result) != ::__awh_national_domains__.end()) ||
		(::__awh_general_domains__.find(result) != ::__awh_general_domains__.end()) ||
		(this->_user.find(result) != this->_user.end())
	);
}
/**
 * @brief Метод очистки результатов парсинга
 *
 */
void awh::Network_Types::clear() noexcept {
	// Очищаем список пользовательских зон
	this->_user.clear();
}
/**
 * @brief Метод установки пользовательской зоны
 *
 * @param zone пользовательская зона
 *
 */
void awh::Network_Types::zone(string_view zone) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Получаем домен верхнего уровня
		string result{zone};
		// Если зона передана и она не существует
		if(!result.empty() && (::__awh_national_domains__.find(result) == ::__awh_national_domains__.end()) &&
		  (::__awh_general_domains__.find(result) == ::__awh_general_domains__.end()))
			// Добавляем зону в список
			this->_user.emplace(result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект логирования установлен
		if(this->_log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(zone), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
}
/**
 * @brief Метод извлечения списка пользовательских зон интернета
 *
 */
const unordered_set <string> & awh::Network_Types::zones() const noexcept {
	// Возвращаем список пользовательских зон интернета
	return this->_user;
}
/**
 * @brief Метод установки списка пользовательских зон
 *
 * @param zones список доменных зон интернета
 *
 */
void awh::Network_Types::zones(const unordered_set <string> & zones) noexcept {
	// Если список зон не пустой
	if(!zones.empty())
		// Возвращаем список пользовательских зон
		this->_user = zones;
}
/**
 * @brief Метод парсинга URI-строки
 *
 * @param text текст для парсинга
 * @return     параметры полученные в результате парсинга
 *
 */
awh::Network_Types::url_t awh::Network_Types::parse(string_view text) const noexcept {
	// Переменная результата
	url_t result{};
	/**
	 * Максимально допустимая длина входной строки (защита от чрезмерного бэктрекинга регулярных выражений)
	 */
	constexpr size_t MAX_LENGTH = 0x2000;
	// Если текст передан и его длина не превышает допустимый предел
	if(!text.empty() && (text.size() <= MAX_LENGTH)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размер текста
			const size_t size = text.size();
			/**
			 * Выполняем единый поиск слева направо по произвольному тексту.
			 * Сканер пропускает недостоверные кандидаты (неизвестная зона,
			 * отсутствие явных признаков URL и т.п.) и возвращает самый левый
			 * валидный адрес любого типа. Если валидного адреса в тексте нет —
			 * возвращается тип NONE.
			 */
			for(size_t i = 0; i < size; ++i){
				// Параметры кандидата URL/E-Mail адреса
				url_t candidate{};
				// Выполняем разбор URL/E-Mail адреса с текущей позиции
				size_t pos = ::ntParseUrl(text, i, candidate);
				// Если с текущей позиции разобран кандидат
				if(pos > i){
					// Определяем наличие явных признаков URL-адреса (схема, порт, путь, пароль, якорь, параметры)
					const bool strong = (
						(candidate.port > 0) || !candidate.path.empty() || !candidate.pass.empty() ||
						!candidate.anchor.empty() || !candidate.params.empty() || !candidate.schema.empty()
					);
					// Определяем валидность доменной зоны (либо это IP-литерал без зоны)
					const bool zone = (candidate.domain.empty() || this->isZone(candidate.domain));
					// Если это достоверный URL-адрес
					if(strong && zone){
						// Устанавливаем полученный результат
						result = ::move(candidate);
						// Завершаем поиск
						break;
					// Если это электронный адрес (присутствует логин и известна доменная зона)
					} else if(!candidate.user.empty() && !candidate.domain.empty() && zone) {
						// Запоминаем тип параметра
						result.type = types_t::EMAIL;
						// Запоминаем логин пользователя
						result.user = ::move(candidate.user);
						// Запоминаем название электронного ящика
						result.host = candidate.host;
						// Запоминаем домен верхнего уровня
						result.domain = ::move(candidate.domain);
						// Формируем uri адрес электронной почты (логин и хост)
						result.uri.assign(result.user + "@" + result.host);
						// Завершаем поиск
						break;
					}
					// Иначе кандидат недостоверный — продолжаем поиск со следующей позиции
				}
				// Выполняем разбор MAC-адреса с текущей позиции
				pos = ::ntParseMAC(text, i);
				// Если MAC-адрес разобран
				if(pos > i){
					// Запоминаем тип параметра
					result.type = types_t::MAC;
					// Запоминаем сам параметр
					result.host.assign(text.substr(i, pos - i));
					// Запоминаем uri адрес
					result.uri = result.host;
					// Завершаем поиск
					break;
				}
				// Определяем, является ли адрес бракетированным IPv6-адресом
				const bool bracket = (text[i] == '[');
				// Определяем позицию начала IPv6-адреса
				const size_t end = (bracket ? (i + 1) : i);
				// Выполняем разбор IPv6-адреса
				pos = ::ntParseIPv6Addr(text, end);
				// Если IPv6-адрес разобран
				if(pos > end){
					// Если адрес бракетированный
					if(bracket){
						// Если закрывающая скобка на месте
						if((pos < size) && (text[pos] == ']')){
							// Запоминаем тип параметра
							result.type = types_t::IPV6;
							// Запоминаем сам параметр
							result.host.assign(text.substr(end, pos - end));
							// Запоминаем uri адрес (включая скобки)
							result.uri.assign(text.substr(i, (pos + 1) - i));
							// Завершаем поиск
							break;
						}
					// Если адрес не бракетированный
					} else {
						// Запоминаем тип параметра
						result.type = types_t::IPV6;
						// Запоминаем сам параметр
						result.host.assign(text.substr(end, pos - end));
						// Запоминаем uri адрес
						result.uri = result.host;
						// Завершаем поиск
						break;
					}
				}
				// Выполняем разбор IPv4-адреса с текущей позиции
				pos = ::ntParseIPv4(text, i);
				// Если IPv4-адрес разобран
				if(pos > i){
					// Запоминаем тип параметра
					result.type = types_t::IPV4;
					// Запоминаем сам параметр
					result.host.assign(text.substr(i, pos - i));
					// Запоминаем uri адрес
					result.uri = result.host;
					// Завершаем поиск
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект логирования установлен
			if(this->_log != nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 *
 */
void awh::Network_Types::setLogger(const log_t * log) noexcept {
	// Устанавливаем объект логера
	this->_log = log;
}
/**
 * @brief Конструктор
 *
 */
awh::Network_Types::Network_Types() noexcept : _log(nullptr) {
	// Выполняем заполнение общих списков доменных зон только один раз для всех объектов
	std::call_once(::__awh_init_once__, &::init);
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::Network_Types::Network_Types(const log_t * log) noexcept : _log(log) {
	// Выполняем заполнение общих списков доменных зон только один раз для всех объектов
	std::call_once(::__awh_init_once__, &::init);
}
