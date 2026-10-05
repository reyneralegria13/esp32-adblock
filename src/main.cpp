/*
 * ESP32 AdBlock: servidor DNS que bloqueia anuncios + console SSH
 * Bibliotecas: LibSSH-ESP32 (ewpa), TFT_eSPI, LittleFS e Preferences (nucleo ESP32)
 * Placa: ESP32-2432S028 (CYD 2.8") - a tela e configurada no platformio.ini
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <set>
#include <TFT_eSPI.h>
#include <WebServer.h>
#include "web_page.h"
#include "libssh_esp32.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include <lwip/etharp.h>
#include <lwip/netif.h>

// ===================== CONFIGURACAO =====================
const char* WIFI_SSID = "seu SSID - WIFI aqui"; // Precisa ser uma rede 2.4Ghz
const char* WIFI_PASS = "sua senha aqui";

#define USE_STATIC_IP 1                 // 0 = usar DHCP (faca reserva no roteador)
IPAddress LOCAL_IP(192, 168, 68, 105); // set o IP estatico que deseja que a placa se conecte, verifique sua faixa de IP
IPAddress GATEWAY(192, 168, 68, 1);    // set o IP do gateway, verifique sua rede
IPAddress SUBNET(255, 255, 255, 0);   // set a subnet mask

const char* SSH_USER         = "admin";
const char* DEFAULT_SSH_PASS = "admin";  // troque depois com: passwd
const char* DEFAULT_UPSTREAM = "1.1.1.1";            // DNS publico para repassar
// ========================================================

Preferences prefs;
WiFiUDP udpDns;            // porta 53: recebe consultas da rede
WiFiUDP udpUp;             // conversa com o DNS publico
SemaphoreHandle_t lock;

File blockFile;
uint32_t blockCount = 0;
std::set<String> customBlock, allowList, macList;
IPAddress upstream;
bool blockingOn = true;
bool macFilter = false;    // true = so atende aparelhos com MAC em macList
uint32_t pauseUntil = 0;
String sshPass;
volatile bool rebootPending = false;
volatile uint32_t qTotal = 0, qBlocked = 0;

#define LOG_LEN 50
struct LogEntry { char name[64]; uint8_t blocked; uint32_t ip; uint32_t t; };   // blocked: 1 = dominio, 2 = aparelho nao autorizado
LogEntry logBuf[LOG_LEN];
uint8_t logPos = 0;

#define HIST_LEN 60                    // consultas por minuto na ultima hora (grafico da web)
uint16_t histTotal[HIST_LEN], histBlocked[HIST_LEN];
uint8_t histPos = 0;
uint32_t histStart = 0;

#define TOP_LEN 24                     // dominios mais bloqueados (aproximado)
struct TopEntry { char name[64]; uint32_t count; };
TopEntry topBlocked[TOP_LEN];

#define CLIENT_LEN 32                  // aparelhos que mais consultam
struct ClientEntry { uint32_t ip, total, blocked, last; uint8_t mac[6]; bool hasMac; };
#define MAC_KEEP_MS 600000UL           // por quanto tempo o ultimo MAC visto num IP ainda vale

#define MAX_TTL 120                    // segundos: um dominio recem-bloqueado para de abrir em ate 2 min
ClientEntry clients[CLIENT_LEN];

#define RECENT_BLOCKED 4               // ultimos bloqueios mostrados na tela
char recentBlocked[RECENT_BLOCKED][64];
uint8_t recentPos = 0;

TFT_eSPI tft = TFT_eSPI();
#define SCREEN_W 320
#define SCREEN_H 240

struct Pending { bool used; uint16_t newId, origId; IPAddress ip; uint16_t port; };
Pending pend[64];
uint8_t nextSlot = 0;

uint8_t pkt[1500];

// ---------- listas ----------
uint32_t fnv1a(const String& s) {
  uint32_t h = 0x811C9DC5;
  for (size_t i = 0; i < s.length(); i++) { h ^= (uint8_t)s[i]; h *= 0x01000193; }
  return h;
}

std::set<String> loadSet(const char* key) {
  std::set<String> out;
  String v = prefs.getString(key, "");
  int st = 0;
  while (st < (int)v.length()) {
    int e = v.indexOf('\n', st);
    if (e < 0) e = v.length();
    String d = v.substring(st, e);
    d.trim();
    if (d.length()) out.insert(d);
    st = e + 1;
  }
  return out;
}

void saveSet(const char* key, const std::set<String>& s) {
  String v;
  for (const auto& d : s) { v += d; v += '\n'; }
  prefs.putString(key, v);
}

void openBlocklist() {
  if (blockFile) blockFile.close();
  blockFile = LittleFS.open("/block.bin", "r");
  blockCount = blockFile ? blockFile.size() / 4 : 0;
}

bool inBlockFile(uint32_t h) {
  if (!blockCount) return false;
  int32_t lo = 0, hi = (int32_t)blockCount - 1;
  while (lo <= hi) {
    int32_t mid = (lo + hi) / 2;
    uint32_t v;
    blockFile.seek((uint32_t)mid * 4);
    if (blockFile.read((uint8_t*)&v, 4) != 4) return false;
    if (v == h) return true;
    if (v < h) lo = mid + 1; else hi = mid - 1;
  }
  return false;
}

// testa o dominio e os "pais" (a.b.ads.com -> b.ads.com -> ads.com), nunca o TLD sozinho
bool inBlockFileSuffix(String d) {
  while (true) {
    if (inBlockFile(fnv1a(d))) return true;
    int p = d.indexOf('.');
    if (p < 0) return false;
    d = d.substring(p + 1);
    if (d.indexOf('.') < 0) return false;
  }
}

bool matchSuffix(const std::set<String>& s, String d) {
  while (true) {
    if (s.count(d)) return true;
    int p = d.indexOf('.');
    if (p < 0) return false;
    d = d.substring(p + 1);
  }
}

// DNS criptografado (DoH/DoT) e reles que passam por fora da placa. Sao respondidos com NXDOMAIN,
// que e o sinal que Firefox e iCloud esperam para voltar a usar o DNS da rede
const char* const BYPASS[] = {
  "use-application-dns.net",                  // Firefox desliga o DoH automatico
  "mask.icloud.com", "mask-h2.icloud.com",    // iCloud Private Relay
  "dns.google", "dns.google.com", "cloudflare-dns.com", "one.one.one.one",
  "dns.quad9.net", "dns11.quad9.net", "doh.opendns.com", "dns.adguard.com", "dns.adguard-dns.com",
  "dns.nextdns.io", "doh.cleanbrowsing.org", "doh.dns.sb", "dns.mullvad.net",
};

bool isBypass(const String& d) {
  for (const char* b : BYPASS) {
    size_t n = strlen(b);
    if (d.endsWith(b) && (d.length() == n || d[d.length() - n - 1] == '.')) return true;
  }
  return false;
}

// 0 = permitido, 1 = lista principal, 2 = lista pessoal, 3 = liberado, 4 = DNS criptografado
int classify(const String& d) {
  if (matchSuffix(allowList, d)) return 3;
  if (matchSuffix(customBlock, d)) return 2;
  if (isBypass(d)) return 4;
  if (inBlockFileSuffix(d)) return 1;
  return 0;
}

// ---------- aparelhos (MAC) ----------
String macStr(const uint8_t* m) {
  char b[18];
  snprintf(b, sizeof(b), "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
  return b;
}

bool cleanMac(String& m) {   // aceita AA-BB-CC-DD-EE-FF ou aa:bb:cc:dd:ee:ff
  m.trim(); m.toLowerCase(); m.replace('-', ':');
  if (m.length() != 17) return false;
  for (int i = 0; i < 17; i++)
    if (i % 3 == 2 ? m[i] != ':' : !isxdigit(m[i])) return false;
  return true;
}

// MAC de um IP da rede local: tabela ARP do lwIP ou, se ela ja esqueceu, o ultimo MAC visto nesse IP
bool clientMac(IPAddress ip, uint8_t* mac) {
  ip4_addr_t a;
  a.addr = (uint32_t)ip;
  struct eth_addr* eth = NULL;
  const ip4_addr_t* found = NULL;
  if (netif_default && etharp_find_addr(netif_default, &a, &eth, &found) >= 0) {
    memcpy(mac, eth->addr, 6);
    return true;
  }
  bool ok = false;
  xSemaphoreTake(lock, portMAX_DELAY);
  for (const auto& c : clients)
    if (c.ip == a.addr && c.hasMac && millis() - c.last < MAC_KEEP_MS) { memcpy(mac, c.mac, 6); ok = true; break; }
  xSemaphoreGive(lock);
  return ok;
}

bool blockingActive() {
  if (!blockingOn) return false;
  if (pauseUntil && (int32_t)(millis() - pauseUntil) < 0) return false;
  pauseUntil = 0;
  return true;
}

void addLog(const String& name, uint8_t blocked, uint32_t ip, const uint8_t* mac) {
  uint32_t now = millis();
  xSemaphoreTake(lock, portMAX_DELAY);
  LogEntry& le = logBuf[logPos];
  strlcpy(le.name, name.c_str(), sizeof(le.name));
  le.blocked = blocked; le.ip = ip; le.t = now;
  logPos = (logPos + 1) % LOG_LEN;

  if (histTotal[histPos] < UINT16_MAX) histTotal[histPos]++;
  if (blocked && histBlocked[histPos] < UINT16_MAX) histBlocked[histPos]++;

  // aparelho: acha pelo IP ou substitui o que esta parado ha mais tempo
  int ci = 0;
  for (int i = 0; i < CLIENT_LEN; i++) {
    if (clients[i].ip == ip) { ci = i; break; }
    if (clients[i].last < clients[ci].last || !clients[i].ip) ci = i;
  }
  if (clients[ci].ip != ip) clients[ci] = {ip, 0, 0, 0};
  if (mac) { memcpy(clients[ci].mac, mac, 6); clients[ci].hasMac = true; }
  clients[ci].total++;
  if (blocked) clients[ci].blocked++;
  clients[ci].last = now;

  if (blocked == 1) {
    strlcpy(recentBlocked[recentPos], le.name, sizeof(recentBlocked[0]));
    recentPos = (recentPos + 1) % RECENT_BLOCKED;
    // ranking: soma no existente ou substitui o de menor contagem
    int ti = -1, mi = 0;
    for (int i = 0; i < TOP_LEN; i++) {
      if (topBlocked[i].count && strcmp(topBlocked[i].name, le.name) == 0) { ti = i; break; }
      if (topBlocked[i].count < topBlocked[mi].count) mi = i;
    }
    if (ti < 0) { ti = mi; strlcpy(topBlocked[ti].name, le.name, sizeof(topBlocked[0].name)); topBlocked[ti].count = 0; }
    topBlocked[ti].count++;
  }
  xSemaphoreGive(lock);
}

void rotateHistory() {   // chamado no loop: abre um novo "minuto" no grafico
  if (millis() - histStart < 60000) return;
  histStart += 60000;
  xSemaphoreTake(lock, portMAX_DELAY);
  histPos = (histPos + 1) % HIST_LEN;
  histTotal[histPos] = histBlocked[histPos] = 0;
  xSemaphoreGive(lock);
}

// ---------- DNS ----------
int parseName(const uint8_t* b, int len, String& out) {   // retorna fim do QNAME
  int p = 12;
  out = "";
  while (p < len) {
    uint8_t l = b[p];
    if (l == 0) return p + 1;
    if ((l & 0xC0) || p + 1 + l > len) return -1;
    if (out.length()) out += '.';
    for (int i = 0; i < l; i++) out += (char)tolower(b[p + 1 + i]);
    p += 1 + l;
    if (out.length() > 253) return -1;
  }
  return -1;
}

// rcode 0 = responde 0.0.0.0 / ::; outro (2 SERVFAIL, 3 NXDOMAIN) = so o codigo de erro, sem resposta
void sendBlocked(IPAddress ip, uint16_t port, int qend, uint16_t qtype, uint8_t rcode = 0) {
  uint8_t r[600];
  memcpy(r, pkt, qend);
  r[2] = 0x80 | (pkt[2] & 0x79);   // resposta, mantem opcode e RD
  r[3] = 0x80 | rcode;             // RA=1
  r[4] = 0; r[5] = 1;              // 1 pergunta
  bool ans = !rcode && (qtype == 1 || qtype == 28);   // A ou AAAA
  r[6] = 0; r[7] = ans ? 1 : 0;
  r[8] = r[9] = r[10] = r[11] = 0;
  int p = qend;
  if (ans) {
    uint8_t rdl = (qtype == 1) ? 4 : 16;
    uint8_t h[12] = {0xC0, 0x0C, 0, (uint8_t)qtype, 0, 1, 0, 0, 0, 60, 0, rdl};
    memcpy(r + p, h, 12); p += 12;
    memset(r + p, 0, rdl); p += rdl;          // 0.0.0.0 ou ::
  }
  udpDns.beginPacket(ip, port);
  udpDns.write(r, p);
  udpDns.endPacket();
}

void forwardQuery(IPAddress ip, uint16_t port, int len) {
  uint16_t id = esp_random() & 0xFFFF;   // ID aleatorio dificulta respostas forjadas
  Pending& e = pend[nextSlot++ % 64];
  e.used = true; e.newId = id; e.origId = (pkt[0] << 8) | pkt[1];
  e.ip = ip; e.port = port;
  pkt[0] = id >> 8; pkt[1] = id & 0xFF;
  udpUp.beginPacket(upstream, 53);
  udpUp.write(pkt, len);
  udpUp.endPacket();
}

bool handleClient() {
  int len = udpDns.parsePacket();
  if (len <= 0) return false;
  len = udpDns.read(pkt, sizeof(pkt));
  if (len < 12) return true;
  IPAddress cip = udpDns.remoteIP();
  uint16_t cport = udpDns.remotePort();
  String name;
  int qend = parseName(pkt, len, name);
  if (qend < 0 || qend + 4 > len || qend + 4 > 560) return true;
  uint16_t qtype = (pkt[qend] << 8) | pkt[qend + 1];
  qend += 4;
  uint8_t mac[6];
  bool hasMac = clientMac(cip, mac);
  // MAC ainda desconhecido: o SERVFAIL faz a placa descobrir o MAC (ARP) e o aparelho tentar de novo
  if (macFilter && !hasMac) { sendBlocked(cip, cport, qend, qtype, 2); return true; }
  qTotal++;
  bool active = blockingActive();
  int r = 0;
  xSemaphoreTake(lock, portMAX_DELAY);
  bool denied = macFilter && !macList.count(macStr(mac));
  if (!denied && active) r = classify(name);
  xSemaphoreGive(lock);
  bool blocked = (r == 1 || r == 2 || r == 4);
  addLog(name, denied ? 2 : blocked, (uint32_t)cip, hasMac ? mac : NULL);
  if (denied || blocked) { qBlocked++; sendBlocked(cip, cport, qend, qtype, r == 4 ? 3 : 0); }
  else forwardQuery(cip, cport, len);
  return true;
}

int skipName(const uint8_t* b, int len, int p) {
  while (p < len) {
    uint8_t l = b[p];
    if (l == 0) return p + 1;
    if ((l & 0xC0) == 0xC0) return p + 2;   // ponteiro de compressao encerra o nome
    if (l & 0xC0) return -1;
    p += 1 + l;
  }
  return -1;
}

// limita o TTL das respostas repassadas, para os aparelhos nao guardarem por horas o IP de um
// site que acabou de ser bloqueado
void clampTtl(uint8_t* b, int len) {
  int p = 12;
  for (int q = (b[4] << 8) | b[5]; q > 0; q--) {
    p = skipName(b, len, p);
    if (p < 0) return;
    p += 4;
  }
  int rr = ((b[6] << 8) | b[7]) + ((b[8] << 8) | b[9]) + ((b[10] << 8) | b[11]);
  for (; rr > 0; rr--) {
    p = skipName(b, len, p);
    if (p < 0 || p + 10 > len) return;
    uint16_t type = (b[p] << 8) | b[p + 1];
    uint32_t ttl = ((uint32_t)b[p + 4] << 24) | (b[p + 5] << 16) | (b[p + 6] << 8) | b[p + 7];
    if (type != 41 && ttl > MAX_TTL) {      // 41 = OPT (EDNS): ali o campo nao e TTL
      b[p + 4] = b[p + 5] = 0; b[p + 6] = MAX_TTL >> 8; b[p + 7] = MAX_TTL & 0xFF;
    }
    p += 10 + ((b[p + 8] << 8) | b[p + 9]);
  }
}

bool handleUpstream() {
  int len = udpUp.parsePacket();
  if (len <= 0) return false;
  len = udpUp.read(pkt, sizeof(pkt));
  if (len < 12 || udpUp.remoteIP() != upstream) return true;
  uint16_t id = (pkt[0] << 8) | pkt[1];
  Pending* found = NULL;
  for (auto& x : pend) if (x.used && x.newId == id) { found = &x; break; }
  if (!found) return true;
  Pending& e = *found;
  pkt[0] = e.origId >> 8; pkt[1] = e.origId & 0xFF;
  e.used = false;
  clampTtl(pkt, len);
  udpDns.beginPacket(e.ip, e.port);
  udpDns.write(pkt, len);
  udpDns.endPacket();
  return true;
}

// ---------- comandos do console ----------
const char* HELP =
  "Comandos:\n"
  "  status              estatisticas e estado\n"
  "  on | off            liga/desliga o bloqueio\n"
  "  pause <min>         pausa o bloqueio por N minutos\n"
  "  check <dominio>     diz se o dominio seria bloqueado\n"
  "  block <dominio>     bloqueia (inclui subdominios)\n"
  "  unblock <dominio>   remove da lista pessoal\n"
  "  allow <dominio>     libera mesmo se estiver na lista\n"
  "  unallow <dominio>   remove da lista de liberados\n"
  "  list block|allow    mostra as listas pessoais\n"
  "  macfilter on|off    so atende aparelhos com MAC autorizado\n"
  "  mac add <mac|ip>    autoriza um aparelho (pelo IP usa o MAC visto na rede)\n"
  "  mac del <mac|ip>    remove a autorizacao\n"
  "  mac list            mostra os aparelhos autorizados\n"
  "  log                 ultimas 32 consultas ([X] dominio, [M] aparelho)\n"
  "  upstream <ip>       troca o DNS externo\n"
  "  reload              recarrega /block.bin\n"
  "  passwd <senha>      troca a senha do SSH\n"
  "  reboot              reinicia a ESP32\n"
  "  exit                encerra a sessao\n";

// aceita tambem um endereco colado do navegador: https://usuario@site.com:443/pagina -> site.com
String cleanDomain(String d) {
  d.trim(); d.toLowerCase();
  int p = d.indexOf("://");
  if (p >= 0) d = d.substring(p + 3);
  for (char c : {'/', '?', '#'}) if ((p = d.indexOf(c)) >= 0) d = d.substring(0, p);
  if ((p = d.lastIndexOf('@')) >= 0) d = d.substring(p + 1);
  if ((p = d.indexOf(':')) >= 0) d = d.substring(0, p);
  if (d.startsWith("*.")) d = d.substring(2);
  while (d.endsWith(".")) d.remove(d.length() - 1);
  return d;
}

// dominio principal do site: pt.site.com -> site.com (mantem 3 partes em casos como site.com.br)
String baseDomain(const String& d) {
  int p2 = d.lastIndexOf('.');
  int p1 = p2 > 0 ? d.lastIndexOf('.', p2 - 1) : -1;
  if (p1 < 0) return d;
  String sld = d.substring(p1 + 1, p2);
  bool country = d.length() - p2 - 1 == 2 &&
                 (sld == "com" || sld == "net" || sld == "org" || sld == "gov" || sld == "edu" || sld == "co");
  if (!country) return d.substring(p1 + 1);
  int p0 = p1 > 0 ? d.lastIndexOf('.', p1 - 1) : -1;
  return p0 < 0 ? d : d.substring(p0 + 1);
}

String runCommand(String line, bool& quit) {
  line.trim();
  int sp = line.indexOf(' ');
  String cmd = sp < 0 ? line : line.substring(0, sp);
  String argRaw = sp < 0 ? "" : line.substring(sp + 1);
  argRaw.trim();
  cmd.toLowerCase();
  String arg = cleanDomain(argRaw);

  if (cmd == "help" || cmd == "?") return HELP;

  if (cmd == "status") {
    char b[512];
    uint32_t up = millis() / 1000;
    xSemaphoreTake(lock, portMAX_DELAY);
    unsigned macs = macList.size();
    xSemaphoreGive(lock);
    const char* st = !blockingOn ? "DESLIGADO" : (blockingActive() ? "ATIVO" : "PAUSADO");
    snprintf(b, sizeof(b),
      "Bloqueio: %s\nConsultas: %u | bloqueadas: %u (%.1f%%)\n"
      "Lista principal: %u | pessoal: %u | liberados: %u\n"
      "Filtro de MAC: %s | autorizados: %u\n"
      "DNS externo: %s\nIP: %s | sinal: %d dBm\n"
      "Memoria livre: %u bytes\nLigado ha: %uh %02um\n",
      st, (unsigned)qTotal, (unsigned)qBlocked, qTotal ? 100.0 * qBlocked / qTotal : 0.0,
      (unsigned)blockCount, (unsigned)customBlock.size(), (unsigned)allowList.size(),
      macFilter ? "ATIVADO" : "DESATIVADO", macs,
      upstream.toString().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI(),
      (unsigned)ESP.getFreeHeap(), (unsigned)(up / 3600), (unsigned)((up / 60) % 60));
    return b;
  }
  if (cmd == "on")  { blockingOn = true; pauseUntil = 0; prefs.putBool("on", true); return "Bloqueio ATIVADO\n"; }
  if (cmd == "off") { blockingOn = false; prefs.putBool("on", false); return "Bloqueio DESATIVADO\n"; }
  if (cmd == "pause") {
    int m = argRaw.toInt();
    if (m <= 0) m = 5;
    pauseUntil = millis() + m * 60000UL;
    if (!pauseUntil) pauseUntil = 1;
    return "Bloqueio pausado por " + String(m) + " min\n";
  }
  if (cmd == "check") {
    if (!arg.length()) return "Uso: check <dominio>\n";
    const char* why[] = {"PERMITIDO", "BLOQUEADO (lista principal)",
                         "BLOQUEADO (lista pessoal)", "PERMITIDO (lista de liberados)",
                         "BLOQUEADO (DNS criptografado)"};
    xSemaphoreTake(lock, portMAX_DELAY);
    int r = classify(arg);
    xSemaphoreGive(lock);
    return arg + ": " + why[r] + "\n";
  }
  if (cmd == "block" || cmd == "unblock" || cmd == "allow" || cmd == "unallow") {
    if (arg.indexOf('.') < 0 || arg.indexOf(' ') >= 0) return "Dominio invalido\n";
    bool isAllow = cmd.endsWith("allow");
    bool add = (cmd == "block" || cmd == "allow");
    String full = arg, raw = argRaw;
    raw.toLowerCase();
    // um endereco colado (https://pt.site.com/pagina) ou "www.site.com" vale para o site inteiro
    if (add && argRaw.indexOf('/') >= 0) arg = baseDomain(arg);
    else if (arg.startsWith("www.") && arg.indexOf('.', 4) > 0) arg = arg.substring(4);
    xSemaphoreTake(lock, portMAX_DELAY);
    std::set<String>& s = isAllow ? allowList : customBlock;
    if (add) s.insert(arg); else { s.erase(arg); s.erase(full); s.erase(raw); }   // raw: entradas antigas salvas como URL
    saveSet(isAllow ? "allow" : "cblock", s);
    xSemaphoreGive(lock);
    return String(add ? "Adicionado em " : "Removido de ") +
           (isAllow ? "liberados: " : "bloqueio pessoal: ") + arg + "\n";
  }
  if (cmd == "list") {
    bool a = (arg == "allow");
    String out = a ? "Liberados:\n" : "Bloqueio pessoal:\n";
    xSemaphoreTake(lock, portMAX_DELAY);
    for (const auto& d : (a ? allowList : customBlock)) out += "  " + d + "\n";
    xSemaphoreGive(lock);
    return out;
  }
  if (cmd == "macfilter") {
    if (argRaw != "on" && argRaw != "off") return "Uso: macfilter on|off\n";
    xSemaphoreTake(lock, portMAX_DELAY);
    bool none = macList.empty();
    xSemaphoreGive(lock);
    if (argRaw == "on" && none) return "Autorize pelo menos um aparelho antes (mac add)\n";
    macFilter = (argRaw == "on");
    prefs.putBool("macf", macFilter);
    return macFilter ? "Filtro de MAC ATIVADO\n" : "Filtro de MAC DESATIVADO\n";
  }
  if (cmd == "mac") {
    int s2 = argRaw.indexOf(' ');
    String sub = s2 < 0 ? argRaw : argRaw.substring(0, s2);
    String m = s2 < 0 ? "" : argRaw.substring(s2 + 1);
    sub.toLowerCase();
    if (sub == "list") {
      String out = "Aparelhos autorizados:\n";
      xSemaphoreTake(lock, portMAX_DELAY);
      for (const auto& d : macList) out += "  " + d + "\n";
      xSemaphoreGive(lock);
      return out;
    }
    if (sub != "add" && sub != "del") return "Uso: mac add|del <mac ou ip> | mac list\n";
    IPAddress ip;
    uint8_t raw[6];
    m.trim();
    if (ip.fromString(m)) {
      if (!clientMac(ip, raw)) return "MAC desconhecido para " + m + " (o aparelho precisa ter feito uma consulta)\n";
      m = macStr(raw);
    } else if (!cleanMac(m)) return "MAC invalido\n";
    xSemaphoreTake(lock, portMAX_DELAY);
    if (sub == "add") macList.insert(m); else macList.erase(m);
    saveSet("macs", macList);
    xSemaphoreGive(lock);
    return String(sub == "add" ? "Aparelho autorizado: " : "Autorizacao removida: ") + m + "\n";
  }
  if (cmd == "log") {
    String out;
    xSemaphoreTake(lock, portMAX_DELAY);
    for (int i = LOG_LEN - 32; i < LOG_LEN; i++) {
      const LogEntry& e = logBuf[(logPos + i) % LOG_LEN];
      if (e.name[0]) out += String(e.blocked == 2 ? "[M] " : e.blocked ? "[X] " : "[ ] ") + e.name + "\n";
    }
    xSemaphoreGive(lock);
    return out.length() ? out : "Sem consultas ainda\n";
  }
  if (cmd == "upstream") {
    IPAddress ip;
    if (!ip.fromString(argRaw)) return "Uso: upstream 1.1.1.1\n";
    upstream = ip;
    prefs.putString("up", argRaw);
    return "DNS externo: " + argRaw + "\n";
  }
  if (cmd == "reload") {
    xSemaphoreTake(lock, portMAX_DELAY);
    openBlocklist();
    xSemaphoreGive(lock);
    return "Lista recarregada: " + String(blockCount) + " dominios\n";
  }
  if (cmd == "passwd") {
    if (argRaw.length() < 8) return "Use pelo menos 8 caracteres\n";
    sshPass = argRaw;
    prefs.putString("pass", argRaw);
    return "Senha alterada\n";
  }
  if (cmd == "reboot") { rebootPending = true; quit = true; return "Reiniciando...\n"; }
  if (cmd == "exit" || cmd == "quit") { quit = true; return "Ate logo!\n"; }
  return "Comando desconhecido. Digite help\n";
}

// ---------- SSH ----------
void sendText(ssh_channel ch, const String& s) {
  String t = s;
  t.replace("\n", "\r\n");
  ssh_channel_write(ch, t.c_str(), t.length());
}

ssh_key loadHostKey() {
  ssh_key key = NULL;
  String b64 = prefs.getString("hostkey", "");
  if (b64.length() && ssh_pki_import_privkey_base64(b64.c_str(), NULL, NULL, NULL, &key) == SSH_OK)
    return key;
  Serial.println("Gerando chave do servidor SSH (Ed25519)...");
  if (ssh_pki_generate(SSH_KEYTYPE_ED25519, 0, &key) != SSH_OK) return NULL;
  char* out = NULL;
  if (ssh_pki_export_privkey_base64(key, NULL, NULL, NULL, &out) == SSH_OK) {
    prefs.putString("hostkey", out);
    ssh_string_free_char(out);
  }
  return key;
}

void printFingerprint(ssh_key key) {
  ssh_key pub = NULL;
  unsigned char* hash = NULL;
  size_t hlen = 0;
  if (ssh_pki_export_privkey_to_pubkey(key, &pub) != SSH_OK) return;
  if (ssh_get_publickey_hash(pub, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen) == 0) {
    char* fp = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
    Serial.printf("Impressao digital SSH: %s\n", fp);
    ssh_string_free_char(fp);
    ssh_clean_pubkey_hash(&hash);
  }
  ssh_key_free(pub);
}

void shellLoop(ssh_channel ch) {
  sendText(ch, "\nESP32 AdBlock - digite help\n> ");
  String line;
  char buf[64];
  bool lastCR = false, quit = false;
  uint32_t idle = 0;
  while (!quit && ssh_channel_is_open(ch) && !ssh_channel_is_eof(ch)) {
    int n = ssh_channel_read_timeout(ch, buf, sizeof(buf), 0, 1000);
    if (n == SSH_ERROR) break;
    if (n == 0) { if (++idle > 600) break; continue; }   // 10 min parado
    idle = 0;
    for (int i = 0; i < n && !quit; i++) {
      char c = buf[i];
      if (c == '\n' && lastCR) { lastCR = false; continue; }
      lastCR = (c == '\r');
      if (c == '\r' || c == '\n') {
        sendText(ch, "\n");
        if (line.length()) sendText(ch, runCommand(line, quit));
        line = "";
        if (!quit) sendText(ch, "> ");
      } else if (c == 0x7F || c == 0x08) {
        if (line.length()) { line.remove(line.length() - 1); ssh_channel_write(ch, "\b \b", 3); }
      } else if (c == 0x03) {
        line = ""; sendText(ch, "^C\n> ");
      } else if (c == 0x04) {
        quit = true;
      } else if (c >= 32 && c < 127 && line.length() < 200) {
        line += c;
        ssh_channel_write(ch, &c, 1);
      }
    }
  }
}

void handleSession(ssh_session session) {
  if (ssh_handle_key_exchange(session) != SSH_OK) return;
  ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD);
  ssh_message msg;
  bool authed = false;
  int tries = 0;
  while (!authed && tries < 3 && (msg = ssh_message_get(session)) != NULL) {
    if (ssh_message_type(msg) == SSH_REQUEST_AUTH &&
        ssh_message_subtype(msg) == SSH_AUTH_METHOD_PASSWORD) {
      tries++;
      const char* u = ssh_message_auth_user(msg);
      const char* p = ssh_message_auth_password(msg);
      if (u && p && strcmp(u, SSH_USER) == 0 && sshPass == p) {
        authed = true;
        ssh_message_auth_reply_success(msg, 0);
      } else {
        vTaskDelay(pdMS_TO_TICKS(1500));
        ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_PASSWORD);
        ssh_message_reply_default(msg);
      }
    } else {
      if (ssh_message_type(msg) == SSH_REQUEST_AUTH)
        ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_PASSWORD);
      ssh_message_reply_default(msg);
    }
    ssh_message_free(msg);
  }
  if (!authed) return;

  ssh_channel ch = NULL;
  while (!ch && (msg = ssh_message_get(session)) != NULL) {
    if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL_OPEN &&
        ssh_message_subtype(msg) == SSH_CHANNEL_SESSION)
      ch = ssh_message_channel_request_open_reply_accept(msg);
    else
      ssh_message_reply_default(msg);
    ssh_message_free(msg);
  }
  if (!ch) return;

  while ((msg = ssh_message_get(session)) != NULL) {
    int type = ssh_message_type(msg), sub = ssh_message_subtype(msg);
    if (type == SSH_REQUEST_CHANNEL && sub == SSH_CHANNEL_REQUEST_PTY) {
      ssh_message_channel_request_reply_success(msg);
    } else if (type == SSH_REQUEST_CHANNEL && sub == SSH_CHANNEL_REQUEST_SHELL) {
      ssh_message_channel_request_reply_success(msg);
      ssh_message_free(msg);
      shellLoop(ch);
      break;
    } else if (type == SSH_REQUEST_CHANNEL && sub == SSH_CHANNEL_REQUEST_EXEC) {
      String command = ssh_message_channel_request_command(msg);   // ssh admin@ip status
      ssh_message_channel_request_reply_success(msg);
      ssh_message_free(msg);
      bool quit = false;
      sendText(ch, runCommand(command, quit));
      ssh_channel_request_send_exit_status(ch, 0);
      break;
    } else {
      ssh_message_reply_default(msg);
    }
    ssh_message_free(msg);
  }
  ssh_channel_send_eof(ch);
  ssh_channel_close(ch);
  ssh_channel_free(ch);
}

void sshTask(void*) {
  libssh_begin();
  ssh_key key = loadHostKey();
  if (!key) { Serial.println("Falha ao criar chave SSH"); vTaskDelete(NULL); }
  printFingerprint(key);
  ssh_bind bind = ssh_bind_new();
  ssh_bind_options_set(bind, SSH_BIND_OPTIONS_BINDPORT_STR, "22");
  ssh_bind_options_set(bind, SSH_BIND_OPTIONS_IMPORT_KEY, key);
  if (ssh_bind_listen(bind) < 0) {
    Serial.printf("Erro SSH: %s\n", ssh_get_error(bind));
    vTaskDelete(NULL);
  }
  Serial.println("SSH ouvindo na porta 22");
  for (;;) {
    ssh_session s = ssh_new();
    if (ssh_bind_accept(bind, s) == SSH_OK) handleSession(s);
    ssh_disconnect(s);
    ssh_free(s);
    if (rebootPending) { delay(500); ESP.restart(); }
  }
}

// ---------- tela ----------
void screenMessage(const String& l1, const String& l2 = "") {
  tft.fillScreen(TFT_BLACK);
  tft.setTextPadding(0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(l1, SCREEN_W / 2, 100, 4);
  if (l2.length()) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(l2, SCREEN_W / 2, 135, 2);
  }
  tft.setTextDatum(TL_DATUM);
}

void drawStatBox(int x, const char* label) {
  tft.drawRoundRect(x, 56, 100, 60, 6, TFT_DARKGREY);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(label, x + 50, 61, 2);
  tft.setTextDatum(TL_DATUM);
}

void drawScreenFrame() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, SCREEN_W, 28, TFT_NAVY);
  tft.setTextPadding(0);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("ESP32 ADBLOCK", 8, 14, 2);
  drawStatBox(6, "Consultas");
  drawStatBox(110, "Bloqueadas");
  drawStatBox(214, "% bloq.");
  tft.drawFastHLine(6, 146, SCREEN_W - 12, TFT_DARKGREY);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("Ultimos bloqueados:", 8, 150, 2);
}

// escreve um texto apagando o que havia antes na mesma area (sem piscar)
void drawField(const String& text, int x, int y, int font, uint16_t color, int width, uint8_t datum = TL_DATUM) {
  tft.setTextDatum(datum);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextPadding(width);
  tft.drawString(text, x, y, font);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
}

void updateScreen() {
  // estado no cabecalho
  String st;
  uint16_t col;
  if (WiFi.status() != WL_CONNECTED) { st = "SEM WI-FI"; col = TFT_RED; }
  else if (!blockingOn)              { st = "DESLIGADO"; col = TFT_RED; }
  else if (!blockingActive())        { st = "PAUSADO " + String((pauseUntil - millis()) / 60000 + 1) + "m"; col = TFT_YELLOW; }
  else                               { st = "ATIVO"; col = TFT_GREEN; }
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(col, TFT_NAVY);
  tft.setTextPadding(110);
  tft.drawString(st, SCREEN_W - 8, 14, 2);
  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);

  // IP e sinal
  String ip = "IP " + WiFi.localIP().toString() + "   sinal " + String(WiFi.RSSI()) + " dBm";
  drawField(ip, 8, 34, 2, TFT_WHITE, SCREEN_W - 16);

  // contadores
  uint32_t total = qTotal, blocked = qBlocked;
  String pct = total ? String(100.0 * blocked / total, 1) + "%" : "--";
  drawField(String(total), 56, 84, 4, TFT_WHITE, 92, TC_DATUM);
  drawField(String(blocked), 160, 84, 4, TFT_RED, 92, TC_DATUM);
  drawField(pct, 264, 84, 4, TFT_ORANGE, 92, TC_DATUM);

  // lista e tempo ligado
  uint32_t up = millis() / 1000;
  char info[64];
  snprintf(info, sizeof(info), "Lista: %u   Ligado: %uh %02um",
           (unsigned)blockCount, (unsigned)(up / 3600), (unsigned)((up / 60) % 60));
  drawField(info, 8, 124, 2, TFT_CYAN, SCREEN_W - 16);

  // ultimos dominios bloqueados (mais recente primeiro)
  static uint32_t lastBlocked = UINT32_MAX;
  if (blocked == lastBlocked) return;
  lastBlocked = blocked;
  char names[RECENT_BLOCKED][64];
  xSemaphoreTake(lock, portMAX_DELAY);
  for (int i = 0; i < RECENT_BLOCKED; i++)
    memcpy(names[i], recentBlocked[(recentPos + RECENT_BLOCKED - 1 - i) % RECENT_BLOCKED], 64);
  xSemaphoreGive(lock);
  for (int i = 0; i < RECENT_BLOCKED; i++) {
    String n = names[i];
    if (n.length() > 42) n = n.substring(0, 39) + "...";
    drawField(n.length() ? n : String("-"), 8, 168 + i * 17, 2, n.length() ? TFT_WHITE : TFT_DARKGREY, SCREEN_W - 16);
  }
}

// ---------- interface web (porta 80, mesmo usuario/senha do SSH) ----------
WebServer web(80);

String jsonStr(const char* s) {
  String o = "\"";
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') { o += '\\'; o += *s; }
    else if ((uint8_t)*s < 0x20) o += ' ';
    else o += *s;
  }
  return o + "\"";
}

bool webAuth() {
  if (web.authenticate(SSH_USER, sshPass.c_str())) return true;
  web.requestAuthentication(BASIC_AUTH, "ESP32 AdBlock");
  return false;
}

void webStatus() {
  if (!webAuth()) return;
  bool active = blockingActive();
  const char* st = !blockingOn ? "DESLIGADO" : (active ? "ATIVO" : "PAUSADO");
  uint32_t pauseLeft = (blockingOn && !active) ? (pauseUntil - millis()) / 1000 : 0;
  char head[360];
  snprintf(head, sizeof(head),
    "{\"state\":\"%s\",\"pause\":%u,\"macfilter\":%d,\"total\":%u,\"blocked\":%u,\"list\":%u,"
    "\"upstream\":\"%s\",\"ip\":\"%s\",\"rssi\":%d,\"heap\":%u,\"uptime\":%u",
    st, (unsigned)pauseLeft, (int)macFilter, (unsigned)qTotal, (unsigned)qBlocked, (unsigned)blockCount,
    upstream.toString().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI(),
    (unsigned)ESP.getFreeHeap(), (unsigned)(millis() / 1000));
  String j = head;
  xSemaphoreTake(lock, portMAX_DELAY);
  bool first = true;
  j += ",\"cblock\":[";
  for (const auto& d : customBlock) { if (!first) j += ','; first = false; j += jsonStr(d.c_str()); }
  first = true;
  j += "],\"allow\":[";
  for (const auto& d : allowList) { if (!first) j += ','; first = false; j += jsonStr(d.c_str()); }
  first = true;
  j += "],\"macs\":[";
  for (const auto& d : macList) { if (!first) j += ','; first = false; j += jsonStr(d.c_str()); }
  uint32_t now = millis();
  first = true;
  j += "],\"log\":[";                                   // [dominio, 0 permitido | 1 dominio | 2 aparelho, ip, segundos atras]
  for (int i = LOG_LEN - 1; i >= 0; i--) {              // mais recente primeiro
    const LogEntry& e = logBuf[(logPos + i) % LOG_LEN];
    if (!e.name[0]) continue;
    if (!first) j += ',';
    first = false;
    j += "[" + jsonStr(e.name) + "," + String(e.blocked) + ",\"" + IPAddress(e.ip).toString() +
         "\"," + String((now - e.t) / 1000) + "]";
  }
  j += "],\"hist\":[";                                  // [consultas, bloqueadas] por minuto, antigo -> atual
  for (int i = 1; i <= HIST_LEN; i++) {
    int k = (histPos + i) % HIST_LEN;
    if (i > 1) j += ',';
    j += "[" + String(histTotal[k]) + "," + String(histBlocked[k]) + "]";
  }
  first = true;
  j += "],\"top\":[";                                   // [dominio, vezes]
  for (int i = 0; i < TOP_LEN; i++) {
    if (!topBlocked[i].count) continue;
    if (!first) j += ',';
    first = false;
    j += "[" + jsonStr(topBlocked[i].name) + "," + String(topBlocked[i].count) + "]";
  }
  first = true;
  j += "],\"clients\":[";                               // [ip, consultas, bloqueadas, segundos atras, mac]
  for (int i = 0; i < CLIENT_LEN; i++) {
    const ClientEntry& c = clients[i];
    if (!c.ip) continue;
    if (!first) j += ',';
    first = false;
    j += "[\"" + IPAddress(c.ip).toString() + "\"," + String(c.total) + "," + String(c.blocked) + "," +
         String((now - c.last) / 1000) + ",\"" + (c.hasMac ? macStr(c.mac) : String()) + "\"]";
  }
  xSemaphoreGive(lock);
  j += "]}";
  web.send(200, "application/json", j);
}

void webCmd() {
  if (!webAuth()) return;
  // o cabecalho extra impede que outro site dispare comandos usando o login salvo no navegador
  if (!web.hasHeader("X-Req")) { web.send(403, "text/plain", "Proibido\n"); return; }
  bool quit = false;
  String out = runCommand(web.arg("c"), quit);
  web.send(200, "text/plain; charset=utf-8", out);
  if (rebootPending) { delay(500); ESP.restart(); }
}

void setupWeb() {
  const char* headers[] = {"X-Req"};
  web.collectHeaders(headers, 1);
  web.on("/", HTTP_GET, []() {
    if (webAuth()) web.send_P(200, "text/html; charset=utf-8", WEB_PAGE);
  });
  web.on("/api/status", HTTP_GET, webStatus);
  web.on("/api/cmd", HTTP_POST, webCmd);
  web.onNotFound([]() { web.send(404, "text/plain", "Nao encontrado\n"); });
  web.begin();
  Serial.printf("Interface web: http://%s/\n", WiFi.localIP().toString().c_str());
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  screenMessage("ESP32 AdBlock", "Iniciando...");
  Serial.println("Tela iniciada");
  lock = xSemaphoreCreateMutex();
  prefs.begin("adblock", false);
  sshPass    = prefs.getString("pass", DEFAULT_SSH_PASS);
  blockingOn = prefs.getBool("on", true);
  upstream.fromString(prefs.getString("up", DEFAULT_UPSTREAM));
  customBlock = loadSet("cblock");
  allowList   = loadSet("allow");
  macList     = loadSet("macs");
  macFilter   = prefs.getBool("macf", false);

  if (!LittleFS.begin(true)) Serial.println("LittleFS falhou");
  openBlocklist();
  Serial.printf("Lista principal: %u dominios\n", (unsigned)blockCount);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
#if USE_STATIC_IP
  WiFi.config(LOCAL_IP, GATEWAY, SUBNET, IPAddress(1, 1, 1, 1));
#endif
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando ao Wi-Fi");
  screenMessage("Conectando Wi-Fi", WIFI_SSID);
  uint32_t wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - wifiStart > 20000) {   // 20 s sem conectar: mostra o motivo e as redes visiveis
      Serial.printf("\nNao conectou (status %d). Redes encontradas:\n", WiFi.status());
      screenMessage("Wi-Fi nao conecta", "Veja o monitor serial");
      int n = WiFi.scanNetworks();
      for (int i = 0; i < n; i++)
        Serial.printf("  \"%s\"  %d dBm  canal %d\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
      if (n <= 0) Serial.println("  nenhuma rede 2.4 GHz encontrada");
      WiFi.scanDelete();
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      screenMessage("Conectando Wi-Fi", WIFI_SSID);
      wifiStart = millis();
    }
  }
  Serial.printf("\nConectado. IP: %s\n", WiFi.localIP().toString().c_str());

  udpDns.begin(53);
  udpUp.begin(50053);
  xTaskCreatePinnedToCore(sshTask, "ssh", 51200, NULL, 1, NULL, 0);
  setupWeb();
  drawScreenFrame();
  updateScreen();
}

void loop() {
  bool busy = handleClient();
  busy |= handleUpstream();
  web.handleClient();
  rotateHistory();
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck > 10000) {
    lastCheck = millis();
    if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  }
  static uint32_t lastDraw = 0;
  if (millis() - lastDraw > 1000) {   // tela atualiza 1x por segundo
    lastDraw = millis();
    updateScreen();
  }
  if (!busy) delay(1);
}

