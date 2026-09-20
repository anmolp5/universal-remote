#pragma once

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <meta name="theme-color" content="#0d1117">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <title>Universal Remote</title>
  <style>
    :root {
      --bg: #090d16;
      --card-bg: rgba(22, 27, 34, 0.75);
      --card-border: rgba(255, 255, 255, 0.08);
      --text-main: #f0f6fc;
      --text-muted: #8b949e;
      --accent-on: #2ea043;
      --accent-off: #da3633;
      --accent-cyan: #38bdf8;
      --accent-amber: #f59e0b;
      --accent-purple: #a855f7;
      --accent-blue: #3b82f6;
      --btn-bg: #21262d;
      --btn-border: #30363d;
      --btn-hover: #30363d;
      --shadow: 0 8px 24px rgba(0, 0, 0, 0.35);
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-tap-highlight-color: transparent;
      user-select: none;
    }

    body {
      font-family: -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: radial-gradient(circle at 50% 0%, #172033 0%, var(--bg) 75%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 20px 16px 40px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    header {
      width: 100%;
      max-width: 600px;
      margin-bottom: 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding-bottom: 12px;
      border-bottom: 1px solid var(--card-border);
    }

    .title-group h1 {
      font-size: 1.35rem;
      font-weight: 700;
      letter-spacing: -0.02em;
    }

    .title-group p {
      font-size: 0.78rem;
      color: var(--text-muted);
      margin-top: 2px;
    }

    .connection-badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      font-size: 0.72rem;
      font-weight: 600;
      padding: 4px 10px;
      border-radius: 9999px;
      background: rgba(46, 160, 67, 0.15);
      color: #3fb950;
      border: 1px solid rgba(46, 160, 67, 0.3);
    }

    .pulse-dot {
      width: 6px;
      height: 6px;
      border-radius: 50%;
      background: currentColor;
      box-shadow: 0 0 8px currentColor;
    }

    main {
      width: 100%;
      max-width: 600px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 16px;
      box-shadow: var(--shadow);
    }

    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 14px;
    }

    .card-title {
      font-size: 0.95rem;
      font-weight: 600;
      display: flex;
      align-items: center;
      gap: 8px;
    }

    .card-title span.icon {
      font-size: 1.1rem;
    }

    .status-badge {
      font-size: 0.68rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      padding: 3px 8px;
      border-radius: 6px;
      background: rgba(255, 255, 255, 0.06);
      color: var(--text-muted);
      border: 1px solid rgba(255, 255, 255, 0.08);
      transition: all 0.25s ease;
    }

    .status-badge.on {
      background: rgba(46, 160, 67, 0.2);
      color: #56d364;
      border-color: rgba(46, 160, 67, 0.4);
    }

    .status-badge.off {
      background: rgba(218, 54, 51, 0.2);
      color: #f85149;
      border-color: rgba(218, 54, 51, 0.4);
    }

    .btn-grid {
      display: grid;
      gap: 10px;
    }

    .grid-2 { grid-template-columns: repeat(2, 1fr); }
    .grid-3 { grid-template-columns: repeat(3, 1fr); }
    .grid-4 { grid-template-columns: repeat(2, 1fr); }
    @media (min-width: 480px) {
      .grid-4 { grid-template-columns: repeat(4, 1fr); }
    }

    button {
      appearance: none;
      touch-action: manipulation;
      border: 1px solid var(--btn-border);
      background: var(--btn-bg);
      color: var(--text-main);
      font-size: 0.88rem;
      font-weight: 600;
      padding: 12px 14px;
      border-radius: 10px;
      cursor: pointer;
      display: inline-flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 4px;
      transition: all 0.15s ease;
      box-shadow: 0 2px 4px rgba(0,0,0,0.2);
    }

    button:active {
      transform: scale(0.96);
      filter: brightness(1.2);
    }

    .btn-subtext {
      font-size: 0.65rem;
      font-weight: 400;
      color: var(--text-muted);
    }

    /* Master Buttons */
    .btn-master-on {
      background: linear-gradient(135deg, #1b4b27 0%, #238636 100%);
      border-color: #2ea043;
      color: #ffffff;
    }
    .btn-master-off {
      background: linear-gradient(135deg, #4d1819 0%, #b62324 100%);
      border-color: #da3633;
      color: #ffffff;
    }

    /* Primary Accent Buttons */
    .btn-on {
      background: rgba(46, 160, 67, 0.15);
      border-color: rgba(46, 160, 67, 0.35);
      color: #56d364;
    }
    .btn-off {
      background: rgba(218, 54, 51, 0.15);
      border-color: rgba(218, 54, 51, 0.35);
      color: #f85149;
    }
    .btn-color {
      background: rgba(168, 85, 247, 0.15);
      border-color: rgba(168, 85, 247, 0.35);
      color: #c084fc;
    }
    .btn-toggle {
      background: rgba(59, 130, 246, 0.15);
      border-color: rgba(59, 130, 246, 0.35);
      color: #60a5fa;
    }

    /* Subtle Sync Controls */
    .resync-bar {
      display: flex;
      align-items: center;
      justify-content: flex-end;
      gap: 6px;
      margin-top: 10px;
      padding-top: 8px;
      border-top: 1px solid rgba(255, 255, 255, 0.05);
    }
    .resync-label {
      font-size: 0.68rem;
      color: var(--text-muted);
    }
    .btn-pill {
      font-size: 0.68rem;
      padding: 4px 8px;
      border-radius: 6px;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid rgba(255, 255, 255, 0.1);
      color: var(--text-muted);
    }

    /* Toast Notification */
    #toast {
      position: fixed;
      bottom: 24px;
      left: 50%;
      transform: translateX(-50%) translateY(100px);
      background: #21262d;
      color: var(--text-main);
      border: 1px solid var(--btn-border);
      padding: 10px 20px;
      border-radius: 9999px;
      font-size: 0.8rem;
      font-weight: 600;
      box-shadow: 0 10px 25px rgba(0,0,0,0.5);
      display: flex;
      align-items: center;
      gap: 8px;
      pointer-events: none;
      opacity: 0;
      transition: all 0.25s cubic-bezier(0.16, 1, 0.3, 1);
      z-index: 1000;
    }

    #toast.show {
      transform: translateX(-50%) translateY(0);
      opacity: 1;
    }
  </style>
</head>
<body>

  <header>
    <div class="title-group">
      <h1>Smart Lighting Hub</h1>
      <p>ESP8266 IR + RF Controller</p>
    </div>
    <div class="connection-badge" id="netBadge">
      <span class="pulse-dot"></span>
      <span>Online</span>
    </div>
  </header>

  <main>
    <!-- Master Controls -->
    <div class="card">
      <div class="card-header">
        <div class="card-title"><span class="icon">✨</span> Master Macros</div>
        <span class="status-badge" id="hubState">Ready</span>
      </div>
      <div class="btn-grid grid-2">
        <button class="btn-master-on" onclick="sendCmd('all_on', 'All Lights ON')">
          <span>ALL ON</span>
          <span class="btn-subtext">Smart Sequential</span>
        </button>
        <button class="btn-master-off" onclick="sendCmd('all_off', 'All Lights OFF')">
          <span>ALL OFF</span>
          <span class="btn-subtext">Smart Sequential</span>
        </button>
      </div>
    </div>

    <!-- Poster Light -->
    <div class="card">
      <div class="card-header">
        <div class="card-title"><span class="icon">🖼️</span> Poster Light (IR NEC)</div>
        <span class="status-badge" id="posterBadge">OFF</span>
      </div>
      <div class="btn-grid grid-3">
        <button class="btn-on" onclick="sendCmd('poster_on', 'Poster Light ON')">
          <span>ON</span>
        </button>
        <button class="btn-off" onclick="sendCmd('poster_off', 'Poster Light OFF')">
          <span>OFF</span>
        </button>
        <button class="btn-color" onclick="sendCmd('poster_color', 'Poster Color Cycle')">
          <span>Color</span>
        </button>
      </div>
    </div>

    <!-- Donut Lamp -->
    <div class="card">
      <div class="card-header">
        <div class="card-title"><span class="icon">🍩</span> Donut Lamp (IR NEC)</div>
        <span class="status-badge" id="donutBadge">OFF</span>
      </div>
      <div class="btn-grid grid-2">
        <button class="btn-on" onclick="sendCmd('donut_on', 'Donut Lamp ON')">
          <span>ON</span>
        </button>
        <button class="btn-off" onclick="sendCmd('donut_off', 'Donut Lamp OFF')">
          <span>OFF</span>
        </button>
      </div>
    </div>

    <!-- Bedside Lamp -->
    <div class="card">
      <div class="card-header">
        <div class="card-title"><span class="icon">🛏️</span> Bedside Lamp (IR Pulse)</div>
        <span class="status-badge" id="bedsideBadge">OFF</span>
      </div>
      <div class="btn-grid grid-2" style="margin-bottom: 10px;">
        <button class="btn-toggle" style="grid-column: span 2;" onclick="sendCmd('bedside_power', 'Bedside Power')">
          <span>Power Toggle</span>
          <span class="btn-subtext">10-bit Pulse Distance</span>
        </button>
      </div>
      <div class="btn-grid grid-4">
        <button onclick="sendCmd('bedside_brighter', 'Bedside Brighter')">
          <span>Brighter</span>
          <span class="btn-subtext">▲ Bright</span>
        </button>
        <button onclick="sendCmd('bedside_dimmer', 'Bedside Dimmer')">
          <span>Dimmer</span>
          <span class="btn-subtext">▼ Bright</span>
        </button>
        <button onclick="sendCmd('bedside_warmer', 'Bedside Warmer')">
          <span>Warmer</span>
          <span class="btn-subtext">☀️ Amber</span>
        </button>
        <button onclick="sendCmd('bedside_cooler', 'Bedside Cooler')">
          <span>Cooler</span>
          <span class="btn-subtext">❄️ White</span>
        </button>
      </div>
      <div class="resync-bar">
        <span class="resync-label">State Sync:</span>
        <button class="btn-pill" onclick="resync('bedside', 1)">Sync ON</button>
        <button class="btn-pill" onclick="resync('bedside', 0)">Sync OFF</button>
      </div>
    </div>

    <!-- Floor Lamp / RF Remote -->
    <div class="card">
      <div class="card-header">
        <div class="card-title"><span class="icon">💡</span> Floor Lamp (3.3V RF)</div>
        <span class="status-badge" id="floorBadge">OFF</span>
      </div>
      <div class="btn-grid grid-2" style="margin-bottom: 10px;">
        <button class="btn-toggle" style="grid-column: span 2;" onclick="sendCmd('floor_power', 'Floor Power')">
          <span>Power Toggle</span>
          <span class="btn-subtext">LM3046 Transistor Switch</span>
        </button>
      </div>
      <div class="btn-grid grid-4">
        <button onclick="sendCmd('floor_brighter', 'Floor Brighter')">
          <span>Brighter</span>
          <span class="btn-subtext">▲ Bright</span>
        </button>
        <button onclick="sendCmd('floor_dimmer', 'Floor Dimmer')">
          <span>Dimmer</span>
          <span class="btn-subtext">▼ Bright</span>
        </button>
        <button onclick="sendCmd('floor_warmer', 'Floor Warmer')">
          <span>Warmer</span>
          <span class="btn-subtext">☀️ Amber</span>
        </button>
        <button onclick="sendCmd('floor_cooler', 'Floor Cooler')">
          <span>Cooler</span>
          <span class="btn-subtext">❄️ White</span>
        </button>
      </div>
      <div class="resync-bar">
        <span class="resync-label">State Sync:</span>
        <button class="btn-pill" onclick="resync('floor', 1)">Sync ON</button>
        <button class="btn-pill" onclick="resync('floor', 0)">Sync OFF</button>
      </div>
    </div>
  </main>

  <div id="toast"></div>

  <script>
    let toastTimer = null;

    function showToast(msg) {
      const toast = document.getElementById('toast');
      toast.textContent = msg;
      toast.classList.add('show');
      if (toastTimer) clearTimeout(toastTimer);
      toastTimer = setTimeout(() => {
        toast.classList.remove('show');
      }, 1600);
    }

    function haptic() {
      if (window.navigator && window.navigator.vibrate) {
        window.navigator.vibrate(20);
      }
    }

    function updateBadge(id, isOn) {
      const el = document.getElementById(id);
      if (!el) return;
      el.textContent = isOn ? 'ON' : 'OFF';
      el.className = 'status-badge ' + (isOn ? 'on' : 'off');
    }

    function parseStatus(data) {
      if (!data) return;
      updateBadge('posterBadge', data.poster);
      updateBadge('donutBadge', data.donut);
      updateBadge('bedsideBadge', data.bedside);
      const floorState = data.floor !== undefined ? data.floor : data.overhead;
      updateBadge('floorBadge', floorState);
      updateBadge('overheadBadge', floorState);

      const hubState = document.getElementById('hubState');
      if (hubState) {
        hubState.textContent = data.busy ? 'Busy...' : 'Ready';
        hubState.className = 'status-badge ' + (data.busy ? 'on' : '');
      }
    }

    async function sendCmd(action, label) {
      haptic();
      showToast(label || action);
      try {
        const res = await fetch(`/api/command?action=${encodeURIComponent(action)}`, {
          method: 'POST'
        });
        if (res.ok) {
          const data = await res.json();
          if (data.states) {
            parseStatus(data.states);
          }
        }
      } catch (err) {
        showToast('Error sending command');
      }
    }

    async function resync(device, state) {
      haptic();
      showToast(`Resynced ${device} -> ${state ? 'ON' : 'OFF'}`);
      try {
        const res = await fetch(`/api/resync?device=${encodeURIComponent(device)}&state=${state}`, {
          method: 'POST'
        });
        if (res.ok) {
          const data = await res.json();
          if (data.states) {
            parseStatus(data.states);
          }
        }
      } catch (err) {
        showToast('Resync failed');
      }
    }

    async function pollStatus() {
      try {
        const res = await fetch('/api/status');
        if (res.ok) {
          const data = await res.json();
          parseStatus(data);
          document.getElementById('netBadge').style.opacity = '1';
        }
      } catch (err) {
        document.getElementById('netBadge').style.opacity = '0.4';
      }
    }

    // Initial poll and recurring poll every 2.5s
    pollStatus();
    setInterval(pollStatus, 2500);
  </script>
</body>
</html>
)rawliteral";
