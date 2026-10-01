#include "wifi_setup.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "device.h"
#include "logger.h"

static const uint32_t PORTAL_AFTER_MS = 3 * 60 * 1000;      // open the portal after this long without WiFi
static const uint32_t PORTAL_MAX_CONNECTED_MS = 5 * 60 * 1000;  // close it this long after WiFi is back, even if a phone is still on it
static const uint32_t RETRY_MS = 20 * 1000;                 // retry the known networks this often
static const uint32_t RETRY_PORTAL_MS = 60 * 1000;          // ...less often while the portal is open
static const uint32_t CONNECT_TIMEOUT_MS = 15 * 1000;

static DNSServer dns;
static WebServer server(80);
static bool portalOpen = false;
static uint32_t lastUpMs = 0;        // last time WiFi was connected (boot counts)
static uint32_t upSinceMs = 0;       // when the current connection came up, 0 = down
static uint32_t lastTryMs = 0;
static bool reportedMissing = false;  // "no known network in range" logged for this outage

// A network entered on the portal and how connecting to it went: "", connecting, connected, failed
static String trySsid, tryResult;
static uint32_t tryStartMs = 0;

// ---------------------------------------------------------------------------
// Connecting to known networks
// ---------------------------------------------------------------------------

static void connectKnown() {
  if (!networkCount) return;
  int best = -1, bestRssi = -1000;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++)
    for (int k = 0; k < networkCount; k++)
      if (WiFi.SSID(i) == networks[k].ssid && WiFi.RSSI(i) > bestRssi) {
        best = k;
        bestRssi = WiFi.RSSI(i);
      }
  WiFi.scanDelete();

  if (best < 0) {
    if (!reportedMissing) logf('W', "WiFi: none of the %d known networks is in range", networkCount);
    reportedMissing = true;
    // Hidden networks don't show up in a scan, so still try them one after another
    static int next = 0;
    best = next++ % networkCount;
  } else {
    logf('I', "Connecting to WiFi \"%s\" (%d dBm)", networks[best].ssid.c_str(), bestRssi);
  }
  WiFi.begin(networks[best].ssid.c_str(), networks[best].pass.c_str());
}

// ---------------------------------------------------------------------------
// Setup portal
// ---------------------------------------------------------------------------

static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 setup</title>
<style>
body{font-family:system-ui,sans-serif;margin:0;padding:16px;background:#f4f6f8;color:#111}
main{max-width:420px;margin:auto;background:#fff;border-radius:12px;padding:20px;box-shadow:0 1px 4px #0002}
h1{font-size:20px;margin:0 0 4px}small{color:#666}label{display:block;margin:14px 0 4px;font-weight:600}
input,button{width:100%;box-sizing:border-box;font-size:16px;padding:10px;border-radius:8px;border:1px solid #ccc}
button{background:#0284c7;color:#fff;border:0;margin-top:16px;font-weight:600}
#nets button{background:#f1f5f9;color:#111;margin:4px 0;text-align:left;display:flex;justify-content:space-between}
#msg{margin-top:16px;padding:10px;border-radius:8px;display:none}
.ok{background:#dcfce7;color:#166534}.bad{background:#fee2e2;color:#991b1b}.wait{background:#e0f2fe;color:#075985}
</style></head><body><main>
<h1>Connect the board to WiFi</h1><small id="dev"></small>
<label>Networks nearby</label><div id="nets"><small>Scanning…</small></div>
<button type="button" id="rescan" style="background:#e2e8f0;color:#111">Scan again</button>
<form id="f">
<label for="ssid">WiFi name</label><input id="ssid" name="ssid" required autocomplete="off">
<label for="pass">Password</label><input id="pass" name="pass" type="password" autocomplete="off">
<label><input type="checkbox" id="show" style="width:auto"> Show password</label>
<label for="name">Name for this board (optional)</label><input id="name" name="name" maxlength="32" placeholder="e.g. Living room">
<button>Save and connect</button>
</form>
<div id="msg"></div>
</main><script>
const $=id=>document.getElementById(id);
function msg(cls,text){const m=$('msg');m.className=cls;m.textContent=text;m.style.display='block'}
async function scan(){
  $('nets').innerHTML='<small>Scanning…</small>';
  try{
    const nets=await (await fetch('/scan')).json();
    $('nets').innerHTML='';
    if(!nets.length)$('nets').innerHTML='<small>No networks found. Type the name below.</small>';
    nets.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{
      const b=document.createElement('button');b.type='button';
      const s=document.createElement('span');s.textContent=n.ssid;
      const r=document.createElement('span');r.textContent=(n.open?'':'🔒 ')+(n.rssi>-60?'▂▄▆█':n.rssi>-70?'▂▄▆':n.rssi>-80?'▂▄':'▂');
      b.append(s,r);b.onclick=()=>{$('ssid').value=n.ssid;$('pass').focus()};$('nets').append(b);
    });
  }catch(e){$('nets').innerHTML='<small>Scan failed. Type the name below.</small>'}
}
fetch('/status').then(r=>r.json()).then(s=>{$('dev').textContent='Board '+s.id;if(s.name)$('name').value=s.name}).catch(()=>{});
$('rescan').onclick=scan;$('show').onchange=e=>$('pass').type=e.target.checked?'text':'password';
$('f').onsubmit=async e=>{
  e.preventDefault();
  msg('wait','Connecting to '+$('ssid').value+'…');
  try{await fetch('/save',{method:'POST',body:new URLSearchParams(new FormData($('f')))})}catch(e){}
  const t0=Date.now();
  const poll=async()=>{
    try{
      const s=await (await fetch('/status')).json();
      if(s.result==='connected')return msg('ok','Connected! The board is online. You can close this page.');
      if(s.result==='failed')return msg('bad','Could not connect to '+s.ssid+'. Check the password and try again.');
    }catch(e){
      if(Date.now()-t0>8000)return msg('wait','Lost contact with the board. That is normal while it switches to your WiFi. When its light stops double-blinking, it is online.');
    }
    setTimeout(poll,1000);
  };
  setTimeout(poll,1000);
};
scan();
</script></body></html>)HTML";

static void handleScan() {
  int n = WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) continue;
    bool dup = false;  // the same network can show up once per access point
    for (JsonObject o : arr)
      if (o["ssid"] == ssid) dup = true;
    if (dup) continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = ssid;
    o["rssi"] = WiFi.RSSI(i);
    o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleSave() {
  String ssid = server.arg("ssid"), pass = server.arg("pass"), name = server.arg("name");
  ssid.trim();
  name.trim();
  if (ssid.isEmpty()) {
    server.send(400, "text/plain", "WiFi name missing");
    return;
  }
  addNetwork(ssid, pass);
  if (name.length() && name != conn.name) {
    setName(name);
    stateDirty = true;
  }
  logf('I', "Setup portal: trying WiFi \"%s\"", ssid.c_str());
  server.send(200, "application/json", "{\"ok\":true}");

  trySsid = ssid;
  tryResult = "connecting";
  tryStartMs = millis();
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), pass.c_str());
}

static void handleStatus() {
  JsonDocument doc;
  doc["id"] = conn.id;
  doc["name"] = conn.name;
  doc["result"] = tryResult;
  doc["ssid"] = trySsid;
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void openPortal() {
  if (portalOpen) return;
  String ap = "ESP32-Setup-" + conn.id.substring(6);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap.c_str());
  dns.start(53, "*", WiFi.softAPIP());  // every name resolves to us, so phones show the page
  server.begin();
  portalOpen = true;
  tryResult = "";
  logf('W', "WiFi setup portal open: join \"%s\"", ap.c_str());
}

static void closePortal() {
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  portalOpen = false;
  logf('I', "WiFi setup portal closed");
}

// ---------------------------------------------------------------------------

void wifiBegin() {
  WiFi.persistent(false);  // we keep our own network list in flash
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);  // wifiLoop() reconnects, choosing among all known networks

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", PAGE); });
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/status", HTTP_GET, handleStatus);
  // Anything else (incl. the phone's captive-portal checks) goes to the setup page
  server.onNotFound([] {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });

  lastUpMs = millis();
  if (!networkCount) {
    logf('W', "No WiFi network known yet");
    openPortal();
    return;
  }
  connectKnown();
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    esp_task_wdt_reset();
  }
  lastTryMs = millis();
}

void wifiLoop() {
  uint32_t now = millis();
  bool up = WiFi.status() == WL_CONNECTED;
  if (up) {
    lastUpMs = now;
    if (!upSinceMs) upSinceMs = now;
    reportedMissing = false;
  } else {
    upSinceMs = 0;
  }

  if (portalOpen) {
    dns.processNextRequest();
    server.handleClient();
  }

  // Outcome of a network entered on the portal
  if (tryResult == "connecting") {
    if (up && WiFi.SSID() == trySsid) {
      tryResult = "connected";
    } else if (now - tryStartMs > CONNECT_TIMEOUT_MS) {
      tryResult = "failed";
      logf('W', "Setup portal: could not connect to \"%s\"", trySsid.c_str());
      lastTryMs = now;
    }
  }

  // Retry the known networks, but leave a phone that is on the portal undisturbed
  bool phoneOnPortal = portalOpen && WiFi.softAPgetStationNum() > 0;
  if (!up && networkCount && tryResult != "connecting" && !phoneOnPortal &&
      now - lastTryMs > (portalOpen ? RETRY_PORTAL_MS : RETRY_MS)) {
    lastTryMs = now;
    connectKnown();
  }

  if (!portalOpen && !up && now - lastUpMs > PORTAL_AFTER_MS) openPortal();
  if (portalOpen && up &&
      ((now - upSinceMs > 30000 && !phoneOnPortal) || now - upSinceMs > PORTAL_MAX_CONNECTED_MS))
    closePortal();
}

bool wifiPortalOpen() { return portalOpen; }

void wifiFactoryReset() {
  logf('W', "Factory reset: forgetting all WiFi networks");
  forgetNetworks();
  WiFi.disconnect();
  lastUpMs = millis();
  openPortal();
}
