#pragma once

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <title>Universal Remote</title>
  <style>
    :root {
      --bg: #090d16;
      --card-bg: rgba(22, 27, 39, 0.75);
      --card-border: rgba(255, 255, 255, 0.08);
      --card-hover-border: rgba(56, 189, 248, 0.28);
      --text-main: #f1f5f9;
      --text-muted: #94a3b8;
      --accent-blue: #38bdf8;
      --btn-bg: rgba(255, 255, 255, 0.05);
      --btn-hover: rgba(255, 255, 255, 0.12);
      --status-on: #10b981;
      --status-off: #475569;
      --status-warn: #f59e0b;
      --shadow-sm: 0 4px 14px rgba(0, 0, 0, 0.3);
      --shadow-lg: 0 12px 28px rgba(0, 0, 0, 0.45);
      --radius: 16px;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-tap-highlight-color: transparent;
      user-select: none;
    }

    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: radial-gradient(circle at 50% 0%, #172033 0%, var(--bg) 75%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 16px 12px 40px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    header {
      width: 100%;
      max-width: 580px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;
      padding: 6px 4px;
    }

    .header-left h1 {
      font-size: 1.35rem;
      font-weight: 700;
      letter-spacing: -0.02em;
      color: #ffffff;
    }

    .header-left p {
      font-size: 0.78rem;
      color: var(--text-muted);
      margin-top: 2px;
    }

    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 6px 12px;
      border-radius: 20px;
      font-size: 0.75rem;
      font-weight: 600;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid var(--card-border);
      color: var(--status-on);
    }

    .pulse-dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--status-on);
      box-shadow: 0 0 8px var(--status-on);
    }

    /* 2x3 Grid Container */
    main {
      width: 100%;
      max-width: 580px;
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 12px;
      align-items: start;
    }

    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 14px;
      box-shadow: var(--shadow-sm);
      transition: border-color 0.2s ease, box-shadow 0.2s ease;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      min-height: 155px;
    }

    .card:hover {
      border-color: var(--card-hover-border);
    }

    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;
    }

    .card-title {
      font-size: 0.92rem;
      font-weight: 700;
      letter-spacing: -0.01em;
      color: #ffffff;
    }

    .header-right {
      display: flex;
      align-items: center;
      gap: 6px;
    }

    .status-badge {
      font-size: 0.65rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.04em;
      padding: 2px 7px;
      border-radius: 10px;
      background: rgba(255, 255, 255, 0.05);
      color: var(--text-muted);
      border: 1px solid rgba(255, 255, 255, 0.06);
    }

    .status-badge.on {
      background: rgba(16, 185, 129, 0.15);
      color: #34d399;
      border-color: rgba(16, 185, 129, 0.3);
    }

    .status-badge.off {
      background: rgba(255, 255, 255, 0.04);
      color: #64748b;
      border-color: rgba(255, 255, 255, 0.06);
    }

    .btn-expand {
      background: transparent;
      border: none;
      color: var(--text-muted);
      width: 22px;
      height: 22px;
      border-radius: 6px;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: all 0.2s ease;
      padding: 0;
    }

    .btn-expand:hover {
      color: var(--text-main);
      background: rgba(255, 255, 255, 0.08);
    }

    .btn-expand svg {
      transition: transform 0.25s ease;
    }

    .btn-expand.expanded svg {
      transform: rotate(180deg);
      stroke: var(--accent-blue);
    }

    .card-body {
      display: flex;
      flex-direction: column;
      flex: 1;
      justify-content: flex-end;
    }

    .btn-grid {
      display: grid;
      gap: 8px;
    }

    .grid-2 { grid-template-columns: repeat(2, 1fr); }
    .grid-4 { grid-template-columns: repeat(4, 1fr); }

    /* Buttons */
    button {
      background: var(--btn-bg);
      border: 1px solid var(--card-border);
      border-radius: 10px;
      color: var(--text-main);
      padding: 12px 6px;
      font-size: 0.84rem;
      font-weight: 600;
      cursor: pointer;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      transition: all 0.12s ease;
      touch-action: manipulation;
    }

    button:hover {
      background: var(--btn-hover);
      border-color: rgba(255, 255, 255, 0.16);
    }

    button:active {
      transform: scale(0.97);
      filter: brightness(1.2);
    }

    .btn-on {
      background: rgba(16, 185, 129, 0.15);
      border-color: rgba(16, 185, 129, 0.35);
      color: #34d399;
    }

    .btn-off {
      background: rgba(239, 68, 68, 0.12);
      border-color: rgba(239, 68, 68, 0.3);
      color: #fca5a5;
    }

    .btn-master-on {
      background: linear-gradient(135deg, rgba(16, 185, 129, 0.25) 0%, rgba(5, 150, 105, 0.25) 100%);
      border-color: rgba(16, 185, 129, 0.4);
      color: #6ee7b7;
    }

    .btn-master-off {
      background: linear-gradient(135deg, rgba(239, 68, 68, 0.2) 0%, rgba(185, 28, 28, 0.2) 100%);
      border-color: rgba(239, 68, 68, 0.35);
      color: #fca5a5;
    }

    /* Split layout for Bedside & Floor (Power on left, Quad on right) */
    .split-layout {
      display: grid;
      grid-template-columns: 1fr 1.35fr;
      gap: 6px;
      flex: 1;
    }

    .btn-power-tall {
      height: 100%;
      min-height: 84px;
      font-size: 0.88rem;
      font-weight: 700;
      background: rgba(56, 189, 248, 0.12);
      border-color: rgba(56, 189, 248, 0.3);
      color: #7dd3fc;
    }

    .quad-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      grid-template-rows: 1fr 1fr;
      gap: 5px;
      height: 100%;
    }

    .btn-quad {
      padding: 6px 2px;
      font-size: 0.72rem;
      font-weight: 600;
    }

    /* Inline Drawer Accordion */
    .drawer {
      display: grid;
      grid-template-rows: 0fr;
      transition: grid-template-rows 0.24s cubic-bezier(0.16, 1, 0.3, 1);
    }

    .drawer.open {
      grid-template-rows: 1fr;
    }

    .drawer-inner {
      overflow: hidden;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    .drawer.open .drawer-inner {
      padding-top: 10px;
      border-top: 1px solid rgba(255, 255, 255, 0.06);
      margin-top: 8px;
    }

    .btn-wide {
      width: 100%;
      padding: 10px;
    }

    .resync-bar {
      display: flex;
      align-items: center;
      justify-content: flex-end;
      gap: 6px;
      padding-top: 2px;
    }

    .resync-label {
      font-size: 0.65rem;
      color: var(--text-muted);
    }

    .btn-pill {
      font-size: 0.65rem;
      font-weight: 600;
      padding: 3px 8px;
      border-radius: 6px;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid rgba(255, 255, 255, 0.1);
      color: var(--text-muted);
      cursor: pointer;
    }

    .btn-pill:hover {
      background: rgba(255, 255, 255, 0.12);
      color: var(--text-main);
    }

    #toast {
      position: fixed;
      bottom: 24px;
      left: 50%;
      transform: translateX(-50%) translateY(80px);
      background: rgba(22, 27, 39, 0.95);
      backdrop-filter: blur(12px);
      color: var(--text-main);
      padding: 8px 18px;
      border-radius: 20px;
      border: 1px solid rgba(255, 255, 255, 0.15);
      font-size: 0.82rem;
      font-weight: 600;
      box-shadow: var(--shadow-lg);
      opacity: 0;
      transition: transform 0.22s ease, opacity 0.22s ease;
      pointer-events: none;
      z-index: 200;
    }

    #toast.show {
      transform: translateX(-50%) translateY(0);
      opacity: 1;
    }
  </style>
</head>
<body>

  <header>
    <div class="header-left">
      <h1>Universal Remote</h1>
      <p>Local Lighting Hub</p>
    </div>
    <div class="status-pill">
      <span class="pulse-dot"></span>
      <span id="netBadge">LAN Live</span>
    </div>
  </header>

  <main>
    <!-- Card 1: Poster Light -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">Poster</div>
        <div class="header-right">
          <span class="status-badge" id="posterBadge">OFF</span>
          <button class="btn-expand" onclick="toggleDrawer('posterDrawer', this)" aria-label="Expand">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="6 9 12 15 18 9"></polyline></svg>
          </button>
        </div>
      </div>
      <div class="card-body">
        <div class="btn-grid grid-2">
          <button class="btn-on" onclick="sendCmd('poster_on', 'Poster ON')">ON</button>
          <button class="btn-off" onclick="sendCmd('poster_off', 'Poster OFF')">OFF</button>
        </div>
      </div>
      <div class="drawer" id="posterDrawer">
        <div class="drawer-inner">
          <button class="btn-wide" onclick="sendCmd('poster_color', 'Poster Color')">Color Cycle</button>
        </div>
      </div>
    </div>

    <!-- Card 2: Donut Lamp -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">Donut</div>
        <div class="header-right">
          <span class="status-badge" id="donutBadge">OFF</span>
        </div>
      </div>
      <div class="card-body">
        <div class="btn-grid grid-2">
          <button class="btn-on" onclick="sendCmd('donut_on', 'Donut ON')">ON</button>
          <button class="btn-off" onclick="sendCmd('donut_off', 'Donut OFF')">OFF</button>
        </div>
      </div>
    </div>

    <!-- Card 3: Bedside Lamp -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">Bedside</div>
        <div class="header-right">
          <span class="status-badge" id="bedsideBadge">OFF</span>
          <button class="btn-expand" onclick="toggleDrawer('bedsideDrawer', this)" aria-label="Expand">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="6 9 12 15 18 9"></polyline></svg>
          </button>
        </div>
      </div>
      <div class="card-body">
        <div class="split-layout">
          <button class="btn-power-tall" onclick="sendCmd('bedside_power', 'Bedside Power')">Power</button>
          <div class="quad-grid">
            <button class="btn-quad" onclick="sendCmd('bedside_brighter', 'Bedside Brighter')" title="Brighter">▲</button>
            <button class="btn-quad" onclick="sendCmd('bedside_dimmer', 'Bedside Dimmer')" title="Dimmer">▼</button>
            <button class="btn-quad" onclick="sendCmd('bedside_warmer', 'Bedside Warmer')">Warm</button>
            <button class="btn-quad" onclick="sendCmd('bedside_cooler', 'Bedside Cooler')">Cool</button>
          </div>
        </div>
      </div>
      <div class="drawer" id="bedsideDrawer">
        <div class="drawer-inner">
          <div class="btn-grid grid-4">
            <button onclick="sendCmd('bedside_brighter', 'Bedside Brighter')">Brighter</button>
            <button onclick="sendCmd('bedside_dimmer', 'Bedside Dimmer')">Dimmer</button>
            <button onclick="sendCmd('bedside_warmer', 'Bedside Warmer')">Warmer</button>
            <button onclick="sendCmd('bedside_cooler', 'Bedside Cooler')">Cooler</button>
          </div>
          <div class="resync-bar">
            <span class="resync-label">Sync:</span>
            <button class="btn-pill" onclick="resync('bedside', 1)">ON</button>
            <button class="btn-pill" onclick="resync('bedside', 0)">OFF</button>
          </div>
        </div>
      </div>
    </div>

    <!-- Card 4: Floor Lamp -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">Floor</div>
        <div class="header-right">
          <span class="status-badge" id="floorBadge">OFF</span>
          <button class="btn-expand" onclick="toggleDrawer('floorDrawer', this)" aria-label="Expand">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="6 9 12 15 18 9"></polyline></svg>
          </button>
        </div>
      </div>
      <div class="card-body">
        <div class="split-layout">
          <button class="btn-power-tall" onclick="sendCmd('floor_power', 'Floor Power')">Power</button>
          <div class="quad-grid">
            <button class="btn-quad" onclick="sendCmd('floor_brighter', 'Floor Brighter')" title="Brighter">▲</button>
            <button class="btn-quad" onclick="sendCmd('floor_dimmer', 'Floor Dimmer')" title="Dimmer">▼</button>
            <button class="btn-quad" onclick="sendCmd('floor_warmer', 'Floor Warmer')">Warm</button>
            <button class="btn-quad" onclick="sendCmd('floor_cooler', 'Floor Cooler')">Cool</button>
          </div>
        </div>
      </div>
      <div class="drawer" id="floorDrawer">
        <div class="drawer-inner">
          <div class="btn-grid grid-4">
            <button onclick="sendCmd('floor_brighter', 'Floor Brighter')">Brighter</button>
            <button onclick="sendCmd('floor_dimmer', 'Floor Dimmer')">Dimmer</button>
            <button onclick="sendCmd('floor_warmer', 'Floor Warmer')">Warmer</button>
            <button onclick="sendCmd('floor_cooler', 'Floor Cooler')">Cooler</button>
          </div>
          <div class="resync-bar">
            <span class="resync-label">Sync:</span>
            <button class="btn-pill" onclick="resync('floor', 1)">ON</button>
            <button class="btn-pill" onclick="resync('floor', 0)">OFF</button>
          </div>
        </div>
      </div>
    </div>

    <!-- Card 5: Desk Lamp -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">Desk</div>
        <div class="header-right">
          <span class="status-badge" id="deskBadge">OFF</span>
          <button class="btn-expand" onclick="toggleDrawer('deskDrawer', this)" aria-label="Expand">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="6 9 12 15 18 9"></polyline></svg>
          </button>
        </div>
      </div>
      <div class="card-body">
        <div class="btn-grid grid-2">
          <button class="btn-on" onclick="sendDeskCmd('ON', 'Desk ON')">ON</button>
          <button class="btn-off" onclick="sendDeskCmd('OFF', 'Desk OFF')">OFF</button>
        </div>
      </div>
      <div class="drawer" id="deskDrawer">
        <div class="drawer-inner">
          <button class="btn-wide" onclick="sendDeskCmd('TAP', 'Desk Cycle Color')">Cycle Color</button>
        </div>
      </div>
    </div>

    <!-- Card 6: All Lights -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">All Lights</div>
        <div class="header-right">
          <span class="status-badge" id="hubState">Ready</span>
        </div>
      </div>
      <div class="card-body">
        <div class="btn-grid grid-2">
          <button class="btn-master-on" onclick="sendCmd('all_on', 'ALL ON')">ALL ON</button>
          <button class="btn-master-off" onclick="sendCmd('all_off', 'ALL OFF')">ALL OFF</button>
        </div>
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

    function toggleDrawer(drawerId, btn) {
      haptic();
      const drawer = document.getElementById(drawerId);
      if (!drawer) return;
      const isOpen = drawer.classList.toggle('open');
      if (btn) {
        btn.classList.toggle('expanded', isOpen);
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
      if (data.desk !== undefined) {
        updateBadge('deskBadge', data.desk);
      }

      const hubState = document.getElementById('hubState');
      if (hubState) {
        hubState.textContent = data.busy ? 'Busy' : 'Ready';
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

    async function sendDeskCmd(action, label) {
      haptic();
      showToast(label || action);
      try {
        const cmd = action === 'ON' ? 'desk_on' : (action === 'OFF' ? 'desk_off' : 'desk_tap');
        const res = await fetch(`/api/${cmd}`, { method: 'POST' });
        if (res.ok) {
          setTimeout(pollStatus, 250);
        }
      } catch(e) {}
    }

    async function resync(device, state) {
      haptic();
      showToast(`Sync ${device} -> ${state ? 'ON' : 'OFF'}`);
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

    window.addEventListener('DOMContentLoaded', () => {
      pollStatus();
      setInterval(pollStatus, 2000);
    });
  </script>
</body>
</html>
)rawliteral";
