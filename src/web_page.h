// Pagina da interface web (servida pela ESP32 em http://<ip>/)
#pragma once
#include <pgmspace.h>

const char WEB_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="pt-BR"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b0f17">
<title>AdBlock · ESP32</title>
<style>
:root{
  --bg:#f3f5f9;--surface:#ffffff;--surface-2:#f7f8fb;--border:#e4e7ee;--border-strong:#d3d8e2;
  --text:#0d1324;--muted:#5d6679;--faint:#8e97a9;
  --brand:#5b5bf0;--brand-2:#14b8d4;--brand-soft:rgba(91,91,240,.10);
  --ok:#0ea371;--ok-soft:rgba(14,163,113,.12);--bad:#e5484d;--bad-soft:rgba(229,72,77,.11);--warn:#d98a06;--warn-soft:rgba(217,138,6,.13);
  --bar:#c9cdf9;--grid:#eceef3;
  --shadow:0 1px 2px rgba(13,19,36,.04),0 12px 32px -18px rgba(13,19,36,.18);
  --radius:18px;color-scheme:light;
}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){
  --bg:#090c13;--surface:#10151f;--surface-2:#141a26;--border:#1f2635;--border-strong:#2b3447;
  --text:#e8ecf4;--muted:#939cb0;--faint:#5f6a80;
  --brand:#7b7bff;--brand-2:#22d3ee;--brand-soft:rgba(123,123,255,.14);
  --ok:#34d399;--ok-soft:rgba(52,211,153,.13);--bad:#fb6b70;--bad-soft:rgba(251,107,112,.13);--warn:#fbbf24;--warn-soft:rgba(251,191,36,.13);
  --bar:#2d3560;--grid:#1a2130;
  --shadow:0 1px 0 rgba(255,255,255,.03) inset,0 16px 40px -24px rgba(0,0,0,.8);color-scheme:dark;
}}
:root[data-theme=dark]{
  --bg:#090c13;--surface:#10151f;--surface-2:#141a26;--border:#1f2635;--border-strong:#2b3447;
  --text:#e8ecf4;--muted:#939cb0;--faint:#5f6a80;
  --brand:#7b7bff;--brand-2:#22d3ee;--brand-soft:rgba(123,123,255,.14);
  --ok:#34d399;--ok-soft:rgba(52,211,153,.13);--bad:#fb6b70;--bad-soft:rgba(251,107,112,.13);--warn:#fbbf24;--warn-soft:rgba(251,191,36,.13);
  --bar:#2d3560;--grid:#1a2130;
  --shadow:0 1px 0 rgba(255,255,255,.03) inset,0 16px 40px -24px rgba(0,0,0,.8);color-scheme:dark;
}
*{box-sizing:border-box}
[hidden]{display:none!important}
html,body{margin:0;background:var(--bg);color:var(--text)}
body{font:14px/1.45 Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;-webkit-font-smoothing:antialiased;min-height:100vh}
body::before{content:"";position:fixed;inset:-20% -10% auto;height:520px;background:radial-gradient(600px 300px at 15% 0%,var(--brand-soft),transparent 70%),radial-gradient(500px 260px at 85% 0%,rgba(20,184,212,.08),transparent 70%);pointer-events:none;z-index:0}
button,input,select{font:inherit;color:inherit}
svg{display:block}
.app{position:relative;z-index:1;max-width:1280px;margin:0 auto;padding:max(20px,env(safe-area-inset-top)) 20px 48px}
.mono{font-family:ui-monospace,"SF Mono","Cascadia Code",Consolas,monospace;font-size:12.5px}
.num{font-variant-numeric:tabular-nums}

/* topo */
.top{display:flex;align-items:center;gap:14px;margin-bottom:22px;flex-wrap:wrap}
.brand{display:flex;align-items:center;gap:12px;margin-right:auto;min-width:0}
.logo{width:42px;height:42px;border-radius:13px;flex-shrink:0;display:grid;place-items:center;color:#fff;background:linear-gradient(140deg,var(--brand),var(--brand-2));box-shadow:0 10px 24px -10px var(--brand)}
.brand h1{font-size:17px;margin:0;letter-spacing:-.01em;font-weight:700}
.brand p{margin:1px 0 0;color:var(--muted);font-size:12.5px}
.search{position:relative;flex:1 1 320px;max-width:440px;order:2}
.search input{width:100%;height:42px;padding:0 44px 0 40px;border-radius:12px;border:1px solid var(--border);background:var(--surface);box-shadow:var(--shadow);outline:none;transition:border-color .15s,box-shadow .15s}
.search input:focus{border-color:var(--brand);box-shadow:0 0 0 4px var(--brand-soft)}
.search .ic{position:absolute;left:13px;top:12px;color:var(--faint)}
.search kbd{position:absolute;right:10px;top:10px;font:600 11px/20px inherit;padding:0 7px;border-radius:6px;border:1px solid var(--border);color:var(--faint);background:var(--surface-2)}
.result{position:absolute;left:0;right:0;top:50px;z-index:20;background:var(--surface);border:1px solid var(--border);border-radius:14px;box-shadow:0 20px 50px -20px rgba(0,0,0,.35);padding:14px;display:flex;align-items:center;gap:12px;animation:pop .16s ease-out}
.result .badge-ic{width:36px;height:36px;border-radius:10px;display:grid;place-items:center;flex-shrink:0}
.result .rd{font-weight:600;word-break:break-all}
.result .rs{color:var(--muted);font-size:12.5px}
.result .grow{flex:1;min-width:0}
.actions{display:flex;align-items:center;gap:8px;order:3}
.chip{display:inline-flex;align-items:center;gap:8px;height:42px;padding:0 14px;border-radius:12px;background:var(--surface);border:1px solid var(--border);box-shadow:var(--shadow);font-weight:650;font-size:13px;white-space:nowrap}
.chip .dot{width:8px;height:8px;border-radius:50%;background:var(--faint);position:relative}
.chip.on .dot{background:var(--ok)}.chip.on .dot::after{content:"";position:absolute;inset:-4px;border-radius:50%;border:2px solid var(--ok);opacity:0;animation:ping 1.8s cubic-bezier(0,0,.2,1) infinite}
.chip.pause .dot{background:var(--warn)}.chip.off .dot,.chip.offline .dot{background:var(--bad)}
.icon-btn{width:42px;height:42px;border-radius:12px;border:1px solid var(--border);background:var(--surface);box-shadow:var(--shadow);display:grid;place-items:center;cursor:pointer;color:var(--muted);transition:color .15s,transform .15s}
.icon-btn:hover{color:var(--text)}.icon-btn:active{transform:scale(.95)}

.banner{display:flex;align-items:center;gap:10px;padding:12px 16px;border-radius:14px;margin-bottom:16px;background:var(--bad-soft);color:var(--bad);font-weight:600;border:1px solid color-mix(in srgb,var(--bad) 25%,transparent)}

/* cards */
.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);box-shadow:var(--shadow);padding:20px;min-width:0}
.card-h{display:flex;align-items:flex-start;justify-content:space-between;gap:12px;margin-bottom:16px;flex-wrap:wrap}
.card-h h2{font-size:15px;margin:0;font-weight:680;letter-spacing:-.01em}
.card-h p{margin:2px 0 0;color:var(--muted);font-size:12.5px}
.grid{display:grid;gap:16px}
.kpis{grid-template-columns:repeat(4,minmax(0,1fr));margin-bottom:16px}
.row-a{grid-template-columns:minmax(0,1.75fr) minmax(0,1fr);margin-bottom:16px}
.row-b{grid-template-columns:minmax(0,1.75fr) minmax(0,1fr);margin-bottom:16px;align-items:start}
.row-c{grid-template-columns:minmax(0,1.2fr) minmax(0,1fr)}
.stack{display:grid;gap:16px}
@media (max-width:1020px){.row-a,.row-b,.row-c{grid-template-columns:minmax(0,1fr)}}
@media (max-width:820px){.kpis{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media (max-width:640px){.search{order:4;flex-basis:100%;max-width:none}.app{padding-left:14px;padding-right:14px}}

.kpi{position:relative;overflow:hidden}
.kpi .lbl{display:flex;align-items:center;gap:10px;color:var(--muted);font-weight:600;font-size:13px}
.kpi .ic{width:32px;height:32px;border-radius:10px;display:grid;place-items:center}
.kpi .val{font-size:30px;font-weight:720;letter-spacing:-.03em;margin:14px 0 4px;line-height:1.1}
.kpi .foot{color:var(--faint);font-size:12.5px}
.kpi .foot b{color:var(--muted);font-weight:600}
.t-brand{background:var(--brand-soft);color:var(--brand)}.t-bad{background:var(--bad-soft);color:var(--bad)}.t-warn{background:var(--warn-soft);color:var(--warn)}.t-ok{background:var(--ok-soft);color:var(--ok)}
.spark{position:absolute;right:0;bottom:0;width:46%;height:56px;opacity:.9}

/* grafico */
.legend{display:flex;gap:14px;color:var(--muted);font-size:12.5px;font-weight:550}
.legend i{display:inline-block;width:9px;height:9px;border-radius:3px;margin-right:6px;vertical-align:-1px}
.chart{position:relative;height:220px;display:grid;grid-template-columns:auto 1fr;gap:10px}
.yaxis{display:flex;flex-direction:column;justify-content:space-between;color:var(--faint);font-size:11px;text-align:right;padding-bottom:22px;min-width:24px}
.plot{position:relative;min-width:0}
.plot svg{width:100%;height:calc(100% - 22px)}
.xaxis{display:flex;justify-content:space-between;color:var(--faint);font-size:11px;margin-top:6px}
.ba{fill:var(--bar)}.bb{fill:var(--bad)}.gl{stroke:var(--grid);stroke-width:1}
.hl{fill:var(--text);opacity:.05}
.tip{position:absolute;pointer-events:none;background:var(--text);color:var(--bg);padding:8px 11px;border-radius:10px;font-size:12px;white-space:nowrap;transform:translate(-50%,-100%);box-shadow:0 10px 30px -10px rgba(0,0,0,.4);z-index:5}
.tip b{display:block;font-size:11px;opacity:.7;font-weight:600;margin-bottom:2px}

/* protecao */
.protect{display:flex;flex-direction:column}
.p-head{display:flex;align-items:center;justify-content:space-between;gap:12px}
.p-state{font-size:13px;color:var(--muted);margin-top:2px}
.switch{width:56px;height:32px;border-radius:999px;border:0;background:var(--border-strong);position:relative;cursor:pointer;transition:background .2s;flex-shrink:0}
.switch span{position:absolute;top:3px;left:3px;width:26px;height:26px;border-radius:50%;background:#fff;box-shadow:0 2px 6px rgba(0,0,0,.25);transition:transform .25s cubic-bezier(.3,1.4,.5,1)}
.switch[aria-checked=true]{background:var(--ok)}.switch[aria-checked=true] span{transform:translateX(24px)}
.switch.paused{background:var(--warn)}
.gauge-wrap{display:grid;place-items:center;margin:18px 0 14px;position:relative}
.gauge{width:170px;height:170px}
.g-bg{fill:none;stroke:var(--grid);stroke-width:12}
.g-fg{fill:none;stroke:url(#gg);stroke-width:12;stroke-linecap:round;transform:rotate(-90deg);transform-origin:center;transition:stroke-dashoffset .9s cubic-bezier(.2,.8,.2,1)}
.gauge-txt{position:absolute;text-align:center}
.gauge-txt b{display:block;font-size:32px;font-weight:740;letter-spacing:-.03em}
.gauge-txt span{color:var(--muted);font-size:12px}
.seg{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;padding:5px;border-radius:13px;background:var(--surface-2);border:1px solid var(--border)}
.seg button{height:34px;border:0;border-radius:9px;background:transparent;font-weight:600;font-size:13px;color:var(--muted);cursor:pointer;transition:background .15s,color .15s}
.seg button:hover{background:var(--surface);color:var(--text)}
.p-label{font-size:12px;font-weight:650;color:var(--faint);text-transform:uppercase;letter-spacing:.06em;margin:4px 0 8px}
.resume{margin-top:10px;display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 12px;border-radius:12px;background:var(--warn-soft);color:var(--warn);font-weight:600;font-size:13px}

/* botoes */
.btn{height:38px;padding:0 15px;border-radius:11px;border:1px solid var(--border);background:var(--surface);font-weight:620;font-size:13px;cursor:pointer;display:inline-flex;align-items:center;justify-content:center;gap:7px;white-space:nowrap;transition:filter .15s,transform .1s,background .15s}
.btn:hover{background:var(--surface-2)}.btn:active{transform:scale(.97)}
.btn.primary{background:var(--brand);border-color:transparent;color:#fff}.btn.primary:hover{filter:brightness(1.1)}
.btn.danger{background:var(--bad);border-color:transparent;color:#fff}.btn.danger:hover{filter:brightness(1.08)}
.btn.ghost-danger{color:var(--bad)}
.btn.sm{height:28px;padding:0 10px;font-size:12px;border-radius:8px}
.field{flex:1;min-width:0;height:38px;padding:0 12px;border-radius:11px;border:1px solid var(--border);background:var(--surface-2);outline:none;transition:border-color .15s,box-shadow .15s}
.field:focus{border-color:var(--brand);box-shadow:0 0 0 4px var(--brand-soft);background:var(--surface)}

/* abas */
.tabs{display:inline-flex;gap:4px;padding:4px;border-radius:11px;background:var(--surface-2);border:1px solid var(--border)}
.tabs button{border:0;background:transparent;height:28px;padding:0 11px;border-radius:8px;font-weight:600;font-size:12.5px;color:var(--muted);cursor:pointer;display:inline-flex;align-items:center;gap:6px}
.tabs button[aria-selected=true]{background:var(--surface);color:var(--text);box-shadow:0 1px 3px rgba(0,0,0,.12)}
.tabs .cnt{font-size:11px;padding:0 6px;border-radius:99px;background:var(--border);color:var(--muted);line-height:17px}

/* log */
.log-tools{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.log-tools .field{height:34px;max-width:200px}
.log{max-height:440px;overflow:auto;margin:0 -8px;padding:0 8px}
.lrow{display:grid;grid-template-columns:auto minmax(0,1fr) auto auto;align-items:center;gap:12px;padding:9px 8px;border-radius:10px;transition:background .12s}
.lrow:hover{background:var(--surface-2)}
.lrow .st{width:26px;height:26px;border-radius:8px;display:grid;place-items:center}
.lrow .d{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-weight:550}
.lrow .meta{color:var(--faint);font-size:12px;text-align:right;white-space:nowrap}
.lrow .meta .mono{font-size:11.5px}
.lrow .act{min-width:74px;opacity:0;transition:opacity .12s}
.lrow:hover .act{opacity:1}
@media (hover:none){.lrow .act{opacity:1}}
@media (max-width:520px){.lrow{grid-template-columns:auto minmax(0,1fr) auto}.lrow .meta{display:none}}
.hold{font-size:11.5px;color:var(--faint);display:flex;align-items:center;gap:6px}
.new{animation:flash 1.2s ease-out}

/* rankings */
.rank{display:grid;gap:12px}
.rk{display:grid;gap:6px}
.rk-top{display:flex;justify-content:space-between;gap:12px;font-size:13px}
.rk-top span:first-child{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-weight:550}
.rk-top span:last-child{color:var(--muted);flex-shrink:0}
.bar{height:6px;border-radius:99px;background:var(--grid);overflow:hidden}
.bar i{display:block;height:100%;border-radius:99px;background:linear-gradient(90deg,var(--bad),color-mix(in srgb,var(--bad) 60%,var(--warn)));transition:width .6s cubic-bezier(.2,.8,.2,1)}
.bar.dev i{background:linear-gradient(90deg,var(--brand),var(--brand-2))}
.dev-row{display:grid;grid-template-columns:auto minmax(0,1fr);gap:12px;align-items:center}
.avatar{width:34px;height:34px;border-radius:10px;display:grid;place-items:center;background:var(--brand-soft);color:var(--brand)}
.dev-row .s{font-size:12px;color:var(--faint)}

/* listas */
.add{display:flex;gap:8px;margin-bottom:14px}
.pills{display:flex;flex-wrap:wrap;gap:8px;max-height:260px;overflow:auto}
.pill{display:inline-flex;align-items:center;gap:6px;max-width:100%;padding:6px 6px 6px 12px;border-radius:10px;background:var(--surface-2);border:1px solid var(--border);font-size:13px;font-weight:550;animation:pop .18s ease-out}
.pill span{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.pill button{width:22px;height:22px;border:0;border-radius:6px;background:transparent;color:var(--faint);cursor:pointer;display:grid;place-items:center;flex-shrink:0}
.pill button:hover{background:var(--bad-soft);color:var(--bad)}

/* sistema */
.kv{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-bottom:16px}
.kv div{padding:12px 14px;border-radius:12px;background:var(--surface-2);border:1px solid var(--border);min-width:0}
.kv>div>span{display:block;color:var(--faint);font-size:11.5px;font-weight:600;text-transform:uppercase;letter-spacing:.05em}
.kv b{display:flex;align-items:center;gap:8px;margin-top:3px;font-size:14px;font-weight:650;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.wifi{display:inline-flex;align-items:flex-end;gap:2px;height:13px}
.wifi i{width:3px;border-radius:1px;background:var(--border-strong)}
.wifi i:nth-child(1){height:4px}.wifi i:nth-child(2){height:7px}.wifi i:nth-child(3){height:10px}.wifi i:nth-child(4){height:13px}
.wifi i.on{background:var(--ok)}
.sys-row{display:flex;gap:8px;flex-wrap:wrap}
.sys-row select{flex:1;min-width:150px}
.divider{height:1px;background:var(--border);margin:16px 0}

.empty{display:grid;place-items:center;text-align:center;gap:6px;padding:26px 10px;color:var(--faint);font-size:13px}
.empty svg{opacity:.6}
.skeleton{color:transparent!important;background:linear-gradient(90deg,var(--grid),var(--surface-2),var(--grid));background-size:200% 100%;animation:sh 1.2s infinite;border-radius:8px}

/* toast e modal */
.toasts{position:fixed;right:20px;bottom:max(20px,env(safe-area-inset-bottom));display:grid;gap:8px;z-index:50;width:min(360px,calc(100vw - 40px))}
.toast{display:flex;align-items:flex-start;gap:10px;padding:12px 14px;border-radius:13px;background:var(--surface);border:1px solid var(--border);box-shadow:0 20px 50px -18px rgba(0,0,0,.45);font-weight:550;animation:slide .22s cubic-bezier(.2,.9,.3,1.2)}
.toast .ti{width:22px;height:22px;border-radius:7px;display:grid;place-items:center;flex-shrink:0}
.toast.out{opacity:0;transform:translateY(8px);transition:.2s}
.modal{position:fixed;inset:0;z-index:60;display:grid;place-items:center;padding:20px;background:rgba(5,8,15,.55);backdrop-filter:blur(4px);animation:fade .15s}
.dialog{width:min(400px,100%);background:var(--surface);border:1px solid var(--border);border-radius:20px;padding:24px;box-shadow:0 30px 80px -20px rgba(0,0,0,.5);animation:pop .18s ease-out}
.dialog h3{margin:14px 0 6px;font-size:17px}.dialog p{margin:0 0 20px;color:var(--muted)}
.dialog .btns{display:flex;justify-content:flex-end;gap:8px}
footer{margin-top:28px;text-align:center;color:var(--faint);font-size:12px}

@keyframes ping{0%{transform:scale(.6);opacity:.9}100%{transform:scale(1.8);opacity:0}}
@keyframes pop{from{opacity:0;transform:scale(.97) translateY(-4px)}to{opacity:1;transform:none}}
@keyframes slide{from{opacity:0;transform:translateY(12px)}to{opacity:1;transform:none}}
@keyframes fade{from{opacity:0}}
@keyframes flash{from{background:var(--brand-soft)}}
@keyframes sh{to{background-position:-200% 0}}
@media (prefers-reduced-motion:reduce){*,*::before,*::after{animation:none!important;transition:none!important}}
</style></head><body>

<svg width="0" height="0" style="position:absolute">
  <defs>
    <linearGradient id="gg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#7b7bff"/><stop offset="1" stop-color="#fb6b70"/></linearGradient>
    <symbol id="i-shield" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/><path fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" d="m9 12 2 2 4-4"/></symbol>
    <symbol id="i-act" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" d="M22 12h-4l-3 9L9 3l-3 9H2"/></symbol>
    <symbol id="i-ban" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9" fill="none" stroke="currentColor" stroke-width="2"/><path stroke="currentColor" stroke-width="2" stroke-linecap="round" d="m5.7 5.7 12.6 12.6"/></symbol>
    <symbol id="i-pct" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" d="M19 5 5 19"/><circle cx="6.5" cy="6.5" r="2.5" fill="none" stroke="currentColor" stroke-width="2"/><circle cx="17.5" cy="17.5" r="2.5" fill="none" stroke="currentColor" stroke-width="2"/></symbol>
    <symbol id="i-list" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/></symbol>
    <symbol id="i-search" viewBox="0 0 24 24"><circle cx="11" cy="11" r="7" fill="none" stroke="currentColor" stroke-width="2"/><path stroke="currentColor" stroke-width="2" stroke-linecap="round" d="m20 20-3.5-3.5"/></symbol>
    <symbol id="i-check" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" d="m5 12 5 5 9-10"/></symbol>
    <symbol id="i-x" viewBox="0 0 24 24"><path stroke="currentColor" stroke-width="2.5" stroke-linecap="round" d="M6 6l12 12M18 6 6 18"/></symbol>
    <symbol id="i-sun" viewBox="0 0 24 24"><circle cx="12" cy="12" r="4" fill="none" stroke="currentColor" stroke-width="2"/><path stroke="currentColor" stroke-width="2" stroke-linecap="round" d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></symbol>
    <symbol id="i-moon" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round" d="M21 13A9 9 0 1 1 11 3a7 7 0 0 0 10 10z"/></symbol>
    <symbol id="i-dev" viewBox="0 0 24 24"><rect x="3" y="4" width="18" height="12" rx="2" fill="none" stroke="currentColor" stroke-width="2"/><path stroke="currentColor" stroke-width="2" stroke-linecap="round" d="M8 20h8M12 16v4"/></symbol>
    <symbol id="i-power" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" d="M12 3v9M6.4 6.4a8 8 0 1 0 11.2 0"/></symbol>
    <symbol id="i-refresh" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" d="M20 11a8 8 0 0 0-14.8-4M4 4v4h4M4 13a8 8 0 0 0 14.8 4M20 20v-4h-4"/></symbol>
    <symbol id="i-alert" viewBox="0 0 24 24"><path fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round" d="M12 3 2 20h20L12 3z"/><path stroke="currentColor" stroke-width="2" stroke-linecap="round" d="M12 10v4M12 17h.01"/></symbol>
  </defs>
</svg>

<div class="app">
  <header class="top">
    <div class="brand">
      <div class="logo"><svg width="22" height="22"><use href="#i-shield"/></svg></div>
      <div><h1>AdBlock</h1><p id="sub">ESP32 · conectando…</p></div>
    </div>
    <div class="search">
      <svg class="ic" width="18" height="18"><use href="#i-search"/></svg>
      <input id="q" placeholder="Testar um domínio, ex: ads.google.com" autocomplete="off" spellcheck="false" aria-label="Testar domínio">
      <kbd>/</kbd>
      <div id="result" class="result" hidden></div>
    </div>
    <div class="actions">
      <span id="chip" class="chip"><span class="dot"></span><span id="chipTxt">Carregando</span></span>
      <button class="icon-btn" id="theme" title="Alternar tema" aria-label="Alternar tema"><svg width="18" height="18"><use id="themeIc" href="#i-moon"/></svg></button>
    </div>
  </header>

  <div id="offline" class="banner" hidden><svg width="18" height="18"><use href="#i-alert"/></svg>Sem conexão com a ESP32. Tentando novamente…</div>

  <section class="grid kpis">
    <div class="card kpi"><div class="lbl"><span class="ic t-brand"><svg width="17" height="17"><use href="#i-act"/></svg></span>Consultas</div>
      <div class="val num" id="kTotal">–</div><div class="foot"><b id="kTotalH">–</b> na última hora</div><svg class="spark" id="spTotal" viewBox="0 0 100 40" preserveAspectRatio="none"></svg></div>
    <div class="card kpi"><div class="lbl"><span class="ic t-bad"><svg width="17" height="17"><use href="#i-ban"/></svg></span>Bloqueadas</div>
      <div class="val num" id="kBlocked">–</div><div class="foot"><b id="kBlockedH">–</b> na última hora</div><svg class="spark" id="spBlocked" viewBox="0 0 100 40" preserveAspectRatio="none"></svg></div>
    <div class="card kpi"><div class="lbl"><span class="ic t-warn"><svg width="17" height="17"><use href="#i-pct"/></svg></span>Taxa de bloqueio</div>
      <div class="val num" id="kRate">–</div><div class="foot"><b id="kRateH">–</b> na última hora</div></div>
    <div class="card kpi"><div class="lbl"><span class="ic t-ok"><svg width="17" height="17"><use href="#i-list"/></svg></span>Domínios na lista</div>
      <div class="val num" id="kList">–</div><div class="foot"><b id="kCustom">–</b> pessoais · <b id="kAllow">–</b> liberados</div></div>
  </section>

  <section class="grid row-a">
    <div class="card">
      <div class="card-h"><div><h2>Atividade</h2><p>Consultas por minuto na última hora</p></div>
        <div class="legend"><span><i style="background:var(--bar)"></i>Permitidas</span><span><i style="background:var(--bad)"></i>Bloqueadas</span></div></div>
      <div class="chart">
        <div class="yaxis num"><span id="yMax">0</span><span id="yMid">0</span><span>0</span></div>
        <div class="plot" id="plot">
          <svg id="chart" viewBox="0 0 600 200" preserveAspectRatio="none"></svg>
          <div class="xaxis"><span>60 min atrás</span><span>30 min</span><span>agora</span></div>
          <div class="tip" id="tip" hidden></div>
        </div>
      </div>
    </div>

    <div class="card protect">
      <div class="p-head">
        <div><h2 style="margin:0;font-size:15px">Proteção</h2><div class="p-state" id="pState">–</div></div>
        <button class="switch" id="sw" role="switch" aria-checked="false" aria-label="Ligar ou desligar o bloqueio"><span></span></button>
      </div>
      <div class="gauge-wrap">
        <svg class="gauge" viewBox="0 0 140 140"><circle class="g-bg" cx="70" cy="70" r="58"/><circle class="g-fg" id="gArc" cx="70" cy="70" r="58" stroke-dasharray="364.4" stroke-dashoffset="364.4"/></svg>
        <div class="gauge-txt"><b class="num" id="gVal">0%</b><span>bloqueado no total</span></div>
      </div>
      <div class="p-label">Pausar bloqueio</div>
      <div class="seg"><button data-p="5">5 min</button><button data-p="30">30 min</button><button data-p="60">1 hora</button></div>
      <div class="resume" id="resume" hidden><span id="resumeTxt"></span><button class="btn sm" onclick="cmd('on')">Retomar agora</button></div>
    </div>
  </section>

  <section class="grid row-b">
    <div class="card">
      <div class="card-h"><div><h2>Consultas recentes</h2><p id="logSub">Últimas consultas feitas pelos aparelhos da rede</p></div>
        <div class="log-tools">
          <input class="field" id="logQ" placeholder="Filtrar…" aria-label="Filtrar consultas">
          <div class="tabs" id="logTabs"><button data-f="all" aria-selected="true">Todas</button><button data-f="b" aria-selected="false">Bloqueadas</button><button data-f="a" aria-selected="false">Permitidas</button></div>
        </div></div>
      <div class="log" id="log"></div>
    </div>
    <div class="stack">
      <div class="card"><div class="card-h"><div><h2>Mais bloqueados</h2><p>Desde que a placa ligou</p></div></div><div class="rank" id="top"></div></div>
      <div class="card"><div class="card-h"><div><h2>Aparelhos</h2><p>Quem mais faz consultas</p></div></div><div class="rank" id="clients"></div></div>
    </div>
  </section>

  <section class="grid row-c">
    <div class="card">
      <div class="card-h"><div><h2>Listas pessoais</h2><p>Valem também para todos os subdomínios</p></div>
        <div class="tabs" id="listTabs"><button data-l="cblock" aria-selected="true">Bloqueados <span class="cnt" id="cB">0</span></button><button data-l="allow" aria-selected="false">Liberados <span class="cnt" id="cA">0</span></button></div></div>
      <form class="add" id="addForm"><input class="field" id="addIn" placeholder="ex: tiktok.com" autocomplete="off" spellcheck="false"><button class="btn primary" id="addBtn">Bloquear</button></form>
      <div class="pills" id="pills"></div>
    </div>
    <div class="card">
      <div class="card-h"><div><h2>Sistema</h2><p>Estado da placa e da rede</p></div></div>
      <div class="kv">
        <div><span>Endereço IP</span><b class="mono" id="sIp">–</b></div>
        <div><span>Sinal Wi-Fi</span><b><span class="wifi" id="sBars"><i></i><i></i><i></i><i></i></span><span id="sRssi">–</span></b></div>
        <div><span>Memória livre</span><b class="num" id="sHeap">–</b></div>
        <div><span>Ligado há</span><b class="num" id="sUp">–</b></div>
      </div>
      <div class="p-label">DNS externo</div>
      <form class="sys-row" id="dnsForm">
        <select class="field" id="dnsSel" aria-label="DNS externo">
          <option value="1.1.1.1">Cloudflare · 1.1.1.1</option>
          <option value="8.8.8.8">Google · 8.8.8.8</option>
          <option value="9.9.9.9">Quad9 · 9.9.9.9</option>
          <option value="94.140.14.14">AdGuard · 94.140.14.14</option>
          <option value="208.67.222.222">OpenDNS · 208.67.222.222</option>
          <option value="custom">Outro…</option>
        </select>
        <input class="field mono" id="dnsIn" placeholder="IP do DNS" hidden>
        <button class="btn">Aplicar</button>
      </form>
      <div class="divider"></div>
      <div class="sys-row">
        <button class="btn" onclick="cmd('reload')"><svg width="15" height="15"><use href="#i-refresh"/></svg>Recarregar lista</button>
        <button class="btn ghost-danger" id="rebootBtn"><svg width="15" height="15"><use href="#i-power"/></svg>Reiniciar placa</button>
      </div>
    </div>
  </section>

  <footer>ESP32 AdBlock · atualiza a cada 3 s · <span id="upd">–</span></footer>
</div>

<div class="toasts" id="toasts"></div>
<div class="modal" id="modal" hidden><div class="dialog" role="dialog" aria-modal="true" aria-labelledby="mT">
  <span class="ic t-bad" style="width:42px;height:42px;border-radius:12px;display:grid;place-items:center"><svg width="20" height="20"><use href="#i-power"/></svg></span>
  <h3 id="mT">Reiniciar a placa?</h3><p>A rede fica sem DNS por cerca de 15 segundos enquanto a ESP32 reinicia.</p>
  <div class="btns"><button class="btn" id="mNo">Cancelar</button><button class="btn danger" id="mYes">Reiniciar</button></div>
</div></div>

<script>
const $ = id => document.getElementById(id);
const nf = new Intl.NumberFormat('pt-BR');
const el = (tag, cls, txt) => { const e = document.createElement(tag); if (cls) e.className = cls; if (txt != null) e.textContent = txt; return e; };
const icon = (id, size = 14) => { const s = document.createElementNS('http://www.w3.org/2000/svg', 'svg'); s.setAttribute('width', size); s.setAttribute('height', size); const u = document.createElementNS('http://www.w3.org/2000/svg', 'use'); u.setAttribute('href', '#' + id); s.append(u); return s; };
let prevTotal = null, newRows = new Set(), S = null, lastOk = 0, pauseLeft = 0, logFilter = 'all', listTab = 'cblock', hoverLog = false, firstLoad = true, sig = {};

/* ---------- tema ---------- */
function curTheme() { const t = document.documentElement.dataset.theme; return t || (matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'); }
function applyIcon() { $('themeIc').setAttribute('href', curTheme() === 'dark' ? '#i-sun' : '#i-moon'); }
try { const t = localStorage.getItem('theme'); if (t) document.documentElement.dataset.theme = t; } catch (e) {}
applyIcon();
$('theme').onclick = () => { const t = curTheme() === 'dark' ? 'light' : 'dark'; document.documentElement.dataset.theme = t; try { localStorage.setItem('theme', t); } catch (e) {} applyIcon(); };

/* ---------- utilitarios ---------- */
function toast(msg, kind = 'ok') {
  const t = el('div', 'toast'); const i = el('span', 'ti ' + (kind === 'bad' ? 't-bad' : kind === 'warn' ? 't-warn' : 't-ok'));
  i.append(icon(kind === 'bad' ? 'i-x' : kind === 'warn' ? 'i-alert' : 'i-check', 13)); t.append(i, el('span', '', msg));
  $('toasts').append(t); setTimeout(() => { t.classList.add('out'); setTimeout(() => t.remove(), 220); }, 3200);
}
async function cmd(c, quiet) {
  try {
    const r = await fetch('/api/cmd', { method: 'POST', headers: { 'X-Req': '1', 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'c=' + encodeURIComponent(c) });
    const t = (await r.text()).trim();
    if (!quiet) toast(t, /invalido|desconhecido|Uso:|pelo menos/i.test(t) ? 'bad' : /DESATIVADO|pausado/i.test(t) ? 'warn' : 'ok');
    load(); return t;
  } catch (e) { toast('Sem conexão com a ESP32', 'bad'); return ''; }
}
function countTo(e, to, fmt = v => nf.format(Math.round(v))) {
  const from = parseFloat(e.dataset.v); e.dataset.v = to;
  if (isNaN(from) || from === to || matchMedia('(prefers-reduced-motion: reduce)').matches) { e.textContent = fmt(to); return; }
  cancelAnimationFrame(e._raf); const t0 = performance.now();
  const step = t => { const k = Math.min(1, (t - t0) / 700), q = 1 - Math.pow(1 - k, 3); e.textContent = fmt(from + (to - from) * q); if (k < 1) e._raf = requestAnimationFrame(step); };
  e._raf = requestAnimationFrame(step);
}
const pct = v => v.toFixed(1).replace('.', ',') + '%';
function ago(s) { if (s < 5) return 'agora'; if (s < 60) return s + ' s'; if (s < 3600) return Math.floor(s / 60) + ' min'; if (s < 86400) return Math.floor(s / 3600) + ' h'; return Math.floor(s / 86400) + ' d'; }
function dur(s) { const d = Math.floor(s / 86400), h = Math.floor(s / 3600) % 24, m = Math.floor(s / 60) % 60; return (d ? d + 'd ' : '') + h + 'h ' + String(m).padStart(2, '0') + 'm'; }
function mmss(s) { return Math.floor(s / 60) + ':' + String(s % 60).padStart(2, '0'); }
function niceMax(v) { if (v <= 4) return 4; const p = Math.pow(10, Math.floor(Math.log10(v))); for (const m of [1, 2, 2.5, 5, 10]) if (m * p >= v) return m * p; return 10 * p; }
function empty(txt, ic) { const d = el('div', 'empty'); d.append(icon(ic, 26), el('span', '', txt)); return d; }
function changed(key, val) { const j = JSON.stringify(val); if (sig[key] === j) return false; sig[key] = j; return true; }

/* ---------- graficos ---------- */
function spark(svgId, data, color) {
  const svg = $(svgId), n = data.length, max = Math.max(1, ...data);
  const pts = data.map((v, i) => [i / (n - 1) * 100, 38 - v / max * 32]);
  const line = pts.map((p, i) => (i ? 'L' : 'M') + p[0].toFixed(1) + ' ' + p[1].toFixed(1)).join('');
  svg.innerHTML = `<defs><linearGradient id="${svgId}g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" style="stop-color:${color};stop-opacity:.22"/><stop offset="1" style="stop-color:${color};stop-opacity:0"/></linearGradient></defs>` +
    `<path d="${line}L100 40L0 40Z" fill="url(#${svgId}g)"/><path d="${line}" fill="none" style="stroke:${color}" stroke-width="1.6" vector-effect="non-scaling-stroke" stroke-linejoin="round"/>`;
}
function drawChart(hist) {
  const n = hist.length, W = 600, H = 200, max = niceMax(Math.max(...hist.map(h => h[0])));
  $('yMax').textContent = nf.format(max); $('yMid').textContent = nf.format(max / 2);
  const bw = W / n; let s = `<line class="gl" x1="0" x2="${W}" y1="1" y2="1" vector-effect="non-scaling-stroke"/><line class="gl" x1="0" x2="${W}" y1="${H / 2}" y2="${H / 2}" vector-effect="non-scaling-stroke"/><line class="gl" x1="0" x2="${W}" y1="${H - .5}" y2="${H - .5}" vector-effect="non-scaling-stroke"/>`;
  s += `<rect id="hl" class="hl" x="0" y="0" width="${bw}" height="${H}" visibility="hidden"/>`;
  hist.forEach(([t, b], i) => {
    const x = i * bw + bw * .18, w = bw * .64, ht = t / max * H, hb = b / max * H;
    if (ht - hb > 0) s += `<rect class="ba" x="${x}" y="${H - ht}" width="${w}" height="${ht - hb}"/>`;
    if (hb > 0) s += `<rect class="bb" x="${x}" y="${H - hb}" width="${w}" height="${hb}"/>`;
  });
  $('chart').innerHTML = s;
}
const plot = $('plot');
plot.addEventListener('mousemove', e => {
  if (!S) return; const r = $('chart').getBoundingClientRect(), n = S.hist.length;
  const i = Math.max(0, Math.min(n - 1, Math.floor((e.clientX - r.left) / r.width * n)));
  const [t, b] = S.hist[i], tip = $('tip'), hl = document.getElementById('hl');
  if (hl) { hl.setAttribute('x', i * 600 / n); hl.setAttribute('visibility', 'visible'); }
  tip.replaceChildren(el('b', '', i === n - 1 ? 'Este minuto' : 'Há ' + (n - 1 - i) + ' min'), el('span', '', nf.format(t) + ' consultas · ' + nf.format(b) + ' bloqueadas'));
  tip.hidden = false; const pr = plot.getBoundingClientRect();
  tip.style.left = Math.max(80, Math.min(pr.width - 80, (i + .5) / n * r.width)) + 'px'; tip.style.top = '-6px';
});
plot.addEventListener('mouseleave', () => { $('tip').hidden = true; const hl = document.getElementById('hl'); if (hl) hl.setAttribute('visibility', 'hidden'); });

/* ---------- render ---------- */
function renderState(s) {
  const chip = $('chip'), sw = $('sw');
  const map = { ATIVO: ['on', 'Protegendo'], PAUSADO: ['pause', 'Pausado'], DESLIGADO: ['off', 'Desligado'] };
  const [cls, txt] = map[s.state] || ['off', s.state];
  chip.className = 'chip ' + cls; $('chipTxt').textContent = txt;
  sw.setAttribute('aria-checked', s.state === 'ATIVO'); sw.classList.toggle('paused', s.state === 'PAUSADO');
  pauseLeft = s.pause || 0; tickPause();
}
function tickPause() {
  const st = S ? S.state : '';
  $('pState').textContent = st === 'ATIVO' ? 'Bloqueando anúncios em toda a rede' : st === 'PAUSADO' ? 'Volta sozinho em ' + mmss(pauseLeft) : st === 'DESLIGADO' ? 'Os anúncios não estão sendo bloqueados' : '–';
  $('resume').hidden = st !== 'PAUSADO'; $('resumeTxt').textContent = 'Pausado · ' + mmss(pauseLeft);
}
setInterval(() => { if (pauseLeft > 0) { pauseLeft--; tickPause(); } }, 1000);

function renderKpis(s) {
  const hT = s.hist.reduce((a, h) => a + h[0], 0), hB = s.hist.reduce((a, h) => a + h[1], 0);
  const rate = s.total ? 100 * s.blocked / s.total : 0;
  countTo($('kTotal'), s.total); countTo($('kBlocked'), s.blocked); countTo($('kRate'), rate, pct); countTo($('kList'), s.list);
  $('kTotalH').textContent = nf.format(hT); $('kBlockedH').textContent = nf.format(hB); $('kRateH').textContent = hT ? pct(100 * hB / hT) : '–';
  $('kCustom').textContent = s.cblock.length; $('kAllow').textContent = s.allow.length;
  const last = s.hist.slice(-30); spark('spTotal', last.map(h => h[0]), 'var(--brand)'); spark('spBlocked', last.map(h => h[1]), 'var(--bad)');
  countTo($('gVal'), rate, v => Math.round(v) + '%');
  $('gArc').style.strokeDashoffset = 364.4 * (1 - Math.min(rate, 100) / 100);
}
function renderLog(s) {
  if (hoverLog && !firstLoad) return;
  const q = $('logQ').value.trim().toLowerCase(), box = $('log');
  const rows = s.log.filter(([n, b]) => (logFilter === 'all' || (logFilter === 'b') === !!b) && (!q || n.includes(q)));
  if (!changed('log', [rows.map(r => r[0] + r[1] + r[2]), logFilter, q, s.total])) { box.querySelectorAll('[data-ago]').forEach((e, i) => { if (rows[i]) e.textContent = ago(rows[i][3]); }); return; }
  box.replaceChildren();
  if (!rows.length) { box.append(empty(s.log.length ? 'Nenhuma consulta com esse filtro' : 'Ainda não houve consultas. Configure o DNS do roteador para 192.168.0.120.', 'i-act')); return; }
  for (const row of rows) {
    const [n, b, ip, a] = row;
    const r = el('div', 'lrow' + (newRows.has(row) ? ' new' : ''));
    const st = el('span', 'st ' + (b ? 't-bad' : 't-ok')); st.append(icon(b ? 'i-ban' : 'i-check', 13));
    const meta = el('span', 'meta'); const agoEl = el('span', '', ago(a)); agoEl.setAttribute('data-ago', '');
    meta.append(el('span', 'mono', ip), document.createTextNode(' · '), agoEl);
    const act = el('button', 'btn sm act', b ? 'Liberar' : 'Bloquear'); act.onclick = () => cmd((b ? 'allow ' : 'block ') + n);
    r.append(st, el('span', 'd', n), meta, act); r.title = n; box.append(r);
  }
  newRows.clear();
}
function renderRank(id, items, kind) {
  if (!changed(id, items)) return; const box = $(id); box.replaceChildren();
  if (!items.length) { box.append(empty(kind === 'top' ? 'Nada bloqueado ainda' : 'Nenhum aparelho ainda', kind === 'top' ? 'i-ban' : 'i-dev')); return; }
  if (kind === 'top') {
    const list = [...items].sort((a, b) => b[1] - a[1]).slice(0, 8), max = list[0][1];
    for (const [n, c] of list) {
      const r = el('div', 'rk'), t = el('div', 'rk-top'); t.append(el('span', '', n), el('span', 'num', nf.format(c) + '×'));
      const bar = el('div', 'bar'), i = el('i'); i.style.width = (c / max * 100) + '%'; bar.append(i); r.append(t, bar); r.title = n; box.append(r);
    }
  } else {
    const list = [...items].sort((a, b) => b[1] - a[1]).slice(0, 6), max = list[0][1];
    for (const [ip, t, b, a] of list) {
      const r = el('div', 'dev-row'), av = el('span', 'avatar'); av.append(icon('i-dev', 16));
      const body = el('div', 'rk'), top = el('div', 'rk-top'); top.append(el('span', 'mono', ip), el('span', 'num', nf.format(t)));
      const bar = el('div', 'bar dev'), i = el('i'); i.style.width = (t / max * 100) + '%'; bar.append(i);
      body.append(top, bar, el('span', 's', nf.format(b) + ' bloqueadas · visto ' + (a < 5 ? 'agora' : 'há ' + ago(a))));
      r.append(av, body); box.append(r);
    }
  }
}
function renderLists(s) {
  $('cB').textContent = s.cblock.length; $('cA').textContent = s.allow.length;
  const items = listTab === 'cblock' ? s.cblock : s.allow;
  if (!changed('lists', [listTab, items])) return;
  const box = $('pills'); box.replaceChildren();
  if (!items.length) { box.append(empty(listTab === 'cblock' ? 'Nenhum domínio bloqueado manualmente' : 'Nenhum domínio liberado', listTab === 'cblock' ? 'i-ban' : 'i-check')); box.firstChild.style.width = '100%'; return; }
  for (const d of items) {
    const p = el('span', 'pill'), x = el('button'); x.title = 'Remover'; x.setAttribute('aria-label', 'Remover ' + d); x.append(icon('i-x', 11));
    x.onclick = () => cmd((listTab === 'cblock' ? 'unblock ' : 'unallow ') + d);
    p.append(el('span', 'mono', d), x); box.append(p);
  }
}
function renderSys(s) {
  $('sub').textContent = 'ESP32 · ' + s.ip; $('sIp').textContent = s.ip;
  const q = s.rssi > -55 ? 4 : s.rssi > -67 ? 3 : s.rssi > -75 ? 2 : 1;
  [...$('sBars').children].forEach((b, i) => b.classList.toggle('on', i < q));
  $('sRssi').textContent = ['Fraco', 'Regular', 'Bom', 'Excelente'][q - 1] + ' · ' + s.rssi + ' dBm';
  $('sHeap').textContent = nf.format(Math.round(s.heap / 1024)) + ' KB'; $('sUp').textContent = dur(s.uptime);
  if (changed('dns', s.upstream) && document.activeElement !== $('dnsSel') && document.activeElement !== $('dnsIn')) {
    const has = [...$('dnsSel').options].some(o => o.value === s.upstream);
    $('dnsSel').value = has ? s.upstream : 'custom'; $('dnsIn').hidden = has; if (!has) $('dnsIn').value = s.upstream;
  }
}
async function load() {
  try {
    const r = await fetch('/api/status', { cache: 'no-store' }); if (!r.ok) throw 0;
    S = await r.json();
    newRows = new Set(prevTotal === null ? [] : S.log.slice(0, Math.max(0, Math.min(S.total - prevTotal, S.log.length)))); prevTotal = S.total;
    lastOk = Date.now(); $('offline').hidden = true;
    renderState(S); renderKpis(S); drawChart(S.hist); renderLog(S);
    renderRank('top', S.top, 'top'); renderRank('clients', S.clients, 'dev'); renderLists(S); renderSys(S);
    firstLoad = false; $('upd').textContent = 'atualizado às ' + new Date().toLocaleTimeString('pt-BR');
  } catch (e) {
    if (Date.now() - lastOk > 7000) { $('offline').hidden = false; $('chip').className = 'chip offline'; $('chipTxt').textContent = 'Offline'; }
  }
}

/* ---------- interacoes ---------- */
$('sw').onclick = () => cmd(S && S.state === 'DESLIGADO' ? 'on' : S && S.state === 'PAUSADO' ? 'on' : 'off');
document.querySelectorAll('.seg button').forEach(b => b.onclick = () => cmd('pause ' + b.dataset.p));
$('logTabs').onclick = e => { const b = e.target.closest('button'); if (!b) return; logFilter = b.dataset.f; [...$('logTabs').children].forEach(x => x.setAttribute('aria-selected', x === b)); S && renderLog(S); };
$('logQ').oninput = () => S && renderLog(S);
$('log').onmouseenter = () => hoverLog = true; $('log').onmouseleave = () => { hoverLog = false; S && renderLog(S); };
$('listTabs').onclick = e => { const b = e.target.closest('button'); if (!b) return; listTab = b.dataset.l; [...$('listTabs').children].forEach(x => x.setAttribute('aria-selected', x === b));
  $('addBtn').textContent = listTab === 'cblock' ? 'Bloquear' : 'Liberar'; S && renderLists(S); };
$('addForm').onsubmit = e => { e.preventDefault(); const d = $('addIn').value.trim(); if (!d) return; cmd((listTab === 'cblock' ? 'block ' : 'allow ') + d); $('addIn').value = ''; };
$('dnsSel').onchange = () => { const c = $('dnsSel').value === 'custom'; $('dnsIn').hidden = !c; if (c) $('dnsIn').focus(); };
$('dnsForm').onsubmit = e => { e.preventDefault(); const v = $('dnsSel').value === 'custom' ? $('dnsIn').value.trim() : $('dnsSel').value; if (v) cmd('upstream ' + v); };
$('rebootBtn').onclick = () => { $('modal').hidden = false; $('mNo').focus(); };
$('mNo').onclick = () => $('modal').hidden = true;
$('modal').onclick = e => { if (e.target === $('modal')) $('modal').hidden = true; };
$('mYes').onclick = () => { $('modal').hidden = true; cmd('reboot', true); toast('Reiniciando… a página volta em alguns segundos', 'warn'); };

/* teste de dominio */
async function testDomain() {
  const d = $('q').value.trim(); if (!d) return; const box = $('result');
  box.hidden = false; box.replaceChildren(el('span', 'rs', 'Verificando…'));
  const t = await cmd('check ' + d, true); const i = t.indexOf(': '); if (i < 0) { box.replaceChildren(el('span', 'rs', t)); return; }
  const dom = t.slice(0, i), why = t.slice(i + 2), blocked = why.startsWith('BLOQUEADO');
  const ic = el('span', 'badge-ic ' + (blocked ? 't-bad' : 't-ok')); ic.append(icon(blocked ? 'i-ban' : 'i-check', 18));
  const body = el('div', 'grow'); body.append(el('div', 'rd mono', dom), el('div', 'rs', blocked ? 'Bloqueado · ' + why.replace(/^BLOQUEADO\s*/, '').replace(/[()]/g, '') : 'Permitido' + (why.includes('liberados') ? ' · está na lista de liberados' : '')));
  const act = el('button', 'btn sm', blocked ? 'Liberar' : 'Bloquear');
  act.onclick = async () => { await cmd((blocked ? 'allow ' : 'block ') + dom); testDomain(); };
  box.replaceChildren(ic, body, act);
}
$('q').addEventListener('keydown', e => { if (e.key === 'Enter') testDomain(); if (e.key === 'Escape') { $('result').hidden = true; $('q').blur(); } });
$('q').addEventListener('input', () => { if (!$('q').value) $('result').hidden = true; });
document.addEventListener('click', e => { if (!e.target.closest('.search')) $('result').hidden = true; });
document.addEventListener('keydown', e => {
  if (e.key === '/' && !/INPUT|SELECT|TEXTAREA/.test(document.activeElement.tagName)) { e.preventDefault(); $('q').focus(); }
  if (e.key === 'Escape') $('modal').hidden = true;
});

['kTotal', 'kBlocked', 'kRate', 'kList'].forEach(id => $(id).classList.add('skeleton'));
load().then(() => ['kTotal', 'kBlocked', 'kRate', 'kList'].forEach(id => $(id).classList.remove('skeleton')));
setInterval(load, 3000);
</script></body></html>)HTML";
