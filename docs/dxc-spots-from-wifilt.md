# Zadání pro projekt IP-rotator: DX spoty z WIFILTu přes HTTP schránku

Tento soubor je **soběstačné zadání** pro práci nad projektem `IP-rotator`.
Vznikl na straně WIFILTu; zdrojáky WIFILTu k provedení nepotřebuješ, jen tento
kontrakt. Wifiltí půlka se staví zvlášť a je nasaditelná nezávisle.

Cílový stav: **mapa rotátoru se na WIFILT po síti nepřipojuje vůbec** — ani
WebSocketem, ani probem. Spoty jí doručuje WIFILT sám, POSTem do firmwaru
rotátoru.

---

## Proč

Mapa dnes čte DX spoty z WIFILTího relaye na `ws://<wifilt>:82/dxcws`
(`data/index.html`, blok „WIFILT DX cluster feed" kolem :1201). Ten relay drží
**jediný** slot a druhé připojení to první vykopne. Takže se mapa pere s operátorovým
vlastním DXC oknem; proti tomu má backoff (tři vykopnutí → pětiminutová pauza,
šedý puntík „slot busy"), jenže pak v mapě nejsou spoty.

Řešit to na straně WIFILTu víc sloty bylo zamítnuto: `WiFiClient::write()` v ESP32
core 2.0.14 umí zablokovat `loop()` až na 10 s na jeden zápis (10 pokusů × 1 s
`select()` timeout), a tři klienti by to ztrojnásobili. Přímé propojení dvou
prohlížečových oken taky nejde — `postMessage` není broadcast a `BroadcastChannel`
i `localStorage` jsou same-origin, dnes navíc particionované podle top-level site.

Zbyla schránka na jednom ze dvou ESP32 a vybral se **rotátor**, protože WIFILT je
zařízení pod podstatně větší zátěží.

---

## Kontrakt (zmrazený, WIFILT ho už posílá přesně takto)

```json
{
  "telnet": true,
  "spots": [ {"dx":"OK1ABC","freq":"14025.0","type":"CQ","time":"1830"} ]
}
```

| věc | hodnota |
|---|---|
| endpoint | `POST /setDxcSpots` |
| Content-Type | `text/plain;charset=UTF-8` — ano, JSON uvnitř `text/plain`, je to schválně |
| odesílatel | prohlížeč operátora, `mode:"no-cors"`, odpověď nečte |
| klíč | `?key=…`, vyžadovat **jen** když `WebAuthEnabled` |
| strop | 120 spotů; tělo nad 8 kB → `413`, nic se neuloží |
| rozbitý JSON | `400` a **starý blob zůstane** — poškozený zápis nesmí vymazat platná data |
| kadence | ~5 s |

### Proč `text/plain` a žádné CORS

`Content-Type: text/plain` drží požadavek v kategorii *simple request*, takže
prohlížeč neposílá preflight `OPTIONS`. Díky tomu **na rotátoru není potřeba
řešit CORS vůbec** — žádné `Access-Control-*` hlavičky, žádný `OPTIONS` handler.
Kdyby endpoint chtěl `application/json` nebo hlavičku `Authorization`, preflight
by přišel a `no-cors` ho zakazuje. Proto je klíč v **query stringu**, ne v hlavičce.

### Proč nejde filtrovat podle zdrojové IP

POST **neposílá WIFILT**, posílá ho prohlížeč operátora. Zdrojová adresa je
notebook, ne ESP32 WIFILTu. Jakákoli vazba na zdrojovou adresu je tedy nesmysl.

### Proč `type` a proč ne `spotter`

`type` potřebuje vlastní filtr mapy (`wifiltSpotAllowed`, `index.html:1484`) nad
`CQ`/`DE`/`EMPTY` — bez něj by všechno vycházelo jako `EMPTY`.

`spotter` mapa mimo vlastní parser nikde nepoužívá, proto v blobu není. Stejně tak
`db`, `wpm`, `comment`, `raw`.

`freq` je **řetězec** (např. `"14025.0"`), stejně jako ho dnes vrací
`parseDxcSpotLine()`. Nic se nepřevádí — schéma je záměrně shodné s tím, co mapa
už dnes konzumuje.

---

## Firmware rotátoru

### 1. Úložiště

```cpp
String   g_dxcSpotBlob = "";   // syrové JSON tělo, tak jak přišlo
uint32_t g_dxcSpotAtMs = 0;    // millis() posledního přijetí; 0 = nikdy
bool     g_dxcSpotTelnet = false;
```

Blob se **neparsuje** při přijetí víc, než je nutné pro validaci. Rozebere si ho
mapa.

### 2. `POST /setDxcSpots`

Pořadí kontrol:

1. `WebAuthEnabled` → porovnat `key` z query; neshoda → `403`
2. tělo > 8 kB → `413`, **beze změny blobu**
3. tělo neparsovatelné / chybí `spots` → `400`, **beze změny blobu**
4. víc než 120 spotů → oříznout na 120 (ne chyba)
5. uložit, `g_dxcSpotAtMs = millis()`, `200`

Endpoint umí jen přepsat ten `String`. Žádný zápis do EEPROM, žádný pohyb antény.

### 3. Čtení pro mapu

Same-origin GET, tvar podle konvencí projektu (`RegisterAjaxRoute`). Vrací:

```json
{"ageMs":4210,"telnet":true,"spots":[…]}
```

- `ageMs` = `millis() - g_dxcSpotAtMs`; když `g_dxcSpotAtMs == 0`, vrať prázdno
- **`ageMs > 600000` → blob zahodit** a vrátit prázdno

To zahození patří do firmwaru, ne do mapy — jinak by hodinu staré spoty ožily
jen proto, že někdo otevřel mapu v nové záložce.

### 4. Proč stáří razítkuje rotátor

Aby se neporovnávaly hodiny prohlížeče s hodinami ESP32. `millis()` delta uvnitř
jednoho čipu je odolná vůči neúspěšnému NTP i vůči špatnému času v notebooku.
Payload proto **žádné `ts` nenese**.

---

## Webová část rotátoru

### 5. `data/index.html` — mapa

**Smazat** celý mechanismus přetahování o WIFILTí slot:

- `openWifiltSocket()`, `closeWifiltSocket()`, `handleWifiltMessage()`
- `noteWifiltEviction()`, `DxcWifiltBackoffMs`, `DxcWifiltEvictions`,
  `DxcWifiltBusyUntil`, `DxcWifiltRetryAt`, `DxcWifiltOpenedAt`,
  `DxcWifiltGotData`, `DxcWifiltRxBuf`
- `probeWifiltHost()` / `refreshWifiltProbe()` — port 82 se už nezkoumá

**Nahradit** pollem vlastního firmwaru po ~5 s, pod **nezměněnou** podmínkou
`wifiltFeedWanted()`. Ta podmínka už dnes obsahuje `!dxcPopupAlive()`, což je
přesně požadované chování: **otevřené DXC rotátoru → spoty z něj, zavřené → blob.**
Nic se na ní nemění, jen se pod ní vymění zdroj.

**Blob je snímek, ne přírůstek.** Dnešní kód přidává spoty po řádcích do
`DxcWifiltSpots` a hodinu si je drží podle `spot.at`. Po změně se pole
**nahrazuje** celým obsahem blobu; retenci řeší WIFILT (posílá jen viditelné
řádky, max 500 u sebe, 120 v blobu). Ponech `spot.at = Date.now()` při načtení,
ať zbytek `aggregateWifiltSpots()` funguje beze změny.

Poznámka: `aggregateWifiltSpots()` má na začátku `if(!DxcWifiltSocket || …) return;`
— ta podmínka musí odkazovat na čerstvost blobu, ne na neexistující socket.

### 6. Puntík `updateWifiltDot()` (:1527) — tři stavy zachovat

Dnešní „connected, but its cluster login is down" se bere z `DxcWifiltTelnetUp`,
což byl rámec `{"telnet":…}` z WebSocketu. Ten zmizí — proto je `telnet` v blobu.

| stav | podmínka |
|---|---|
| zelený, spoty živé | `ageMs < 15000` a `telnet == true` |
| šedý, „cluster login leží" | `ageMs < 15000` a `telnet == false` |
| šedý, „nikdo nekrmí" | `ageMs >= 15000` nebo prázdno |

Spoty se mezi 15 s a 10 min pořád kreslí — nejsou špatné, jen zmrzlé.

### 7. `data/dxc.html` rotátoru — `useWifilt()` pryč

Ta stránka se dnes taky umí připojit na `ws://<wifilt>:82/dxcws` (`useWifilt()`,
kolem :103-106) a je to **druhá** cesta, jak WIFILTu sebrat jediný slot — jedním
kliknutím na DXC by šel vykopnout DXC panel v operátorově qrplogu.

Blob ji nakrmit nemůže: její tabulka potřebuje `spotter`, `db`, `wpm`, `comment`,
`raw`, což je fakticky syrový feed. Proto se ta větev **odstraní** a stránka jede
vždy na vlastní cluster rotátoru.

Spolu s ní padá i dnešní pravidlo přednosti
`if(forced!=="wifilt" && (!cfg||cfg.ownReady||!cfg.wifiltEnabled)) useOwn()`
a `?src=wifilt`.

### 8. Co se nemění

- **Tlačítko DXC na mapě.** Zůstává, ukazuje na vlastní `/dxc.html` rotátoru.
  Oba systémy pak fungují odděleně: v DXC okně vlastní cluster, na mapě WIFILT.
- **`DxcWifiltEnabled` a `DxcWifiltHost`** v EEPROM zůstávají jako vypínač
  funkce. `DxcWifiltHost` už ale neslouží k hledání portu 82 — po změně ho mapa
  k ničemu nepotřebuje, protože data chodí sama. Zvaž, jestli pole nechat jako
  čistý on/off.
- **Vlastní filtry mapy.** Zůstávají a skládají se s WIFILTími (blob nese jen
  viditelné řádky). Je to záměr: „co vidím v DXC, to je na mapě". Až operátor
  v mapě zapne všechno a něco pořád nevidí, filtruje to WIFILT — a je to jediné
  vysvětlení, které z pohledu rotátoru nejde uhodnout. Stojí za zmínku
  v titulku puntíku.

---

## Ověření

Napsat `tools/dxcspots-endpoint-check.sh` — `curl` proti reálnému kusu na stole.
Projekt dnes testy nemá; tohle je ta část, kde se meze dají ověřit levně.

1. platný POST → `200`, následný GET vrací spoty a malé `ageMs`
2. tělo 9 kB → `413`, **předchozí blob beze změny**
3. `{"spots":` useknuté → `400`, **předchozí blob beze změny**
4. 200 spotů → uloží se 120
5. `WebAuthEnabled` zapnuto, bez `key` → `403`; se správným `key` → `200`
6. po >10 min (nebo s dočasně sníženou mezí) → GET vrací prázdno

Na živém železe, s prohlížečem:

7. mapa kreslí spoty z blobu (DXCC overlay i *DX CLUSTER LINES*)
8. puntík ve všech třech stavech
9. otevřu DXC rotátoru → spoty z něj; zavřu → do 15 s naskočí blob
10. **regrese, na kterou to celé je:** s otevřeným operátorovým DXC (samostatné
    okno i panel v qrplogu) se nic nevykopává a v sériovém logu WIFILTu není po
    rotátoru ani stopa
