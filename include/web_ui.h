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
      padding: 10px 10px 24px;
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
      margin-bottom: 8px;
      padding: 2px 4px;
    }

    .header-left h1 {
      font-size: 1.6rem;
      font-weight: 800;
      letter-spacing: -0.02em;
      color: #ffffff;
    }

    .header-left p {
      font-size: 0.78rem;
      color: var(--text-muted);
      margin-top: 1px;
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

    /* Main Grid Layout */
    main {
      width: 100%;
      max-width: 580px;
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 8px;
      align-items: start;
    }

    .card-full-width {
      grid-column: span 2;
    }

    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 10px 12px;
      box-shadow: var(--shadow-sm);
      transition: border-color 0.2s ease, box-shadow 0.2s ease, min-height 0.28s cubic-bezier(0.16, 1, 0.3, 1);
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      min-height: 135px;
    }

    .card.card-banner {
      min-height: auto;
      padding: 10px 14px;
    }

    .card.card-floor {
      min-height: auto;
      padding: 10px 14px;
    }

    .card:hover {
      border-color: var(--card-hover-border);
    }

    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 8px;
    }

    .card-title {
      font-size: 1.12rem;
      font-weight: 800;
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
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid rgba(255, 255, 255, 0.08);
      color: var(--text-muted);
      width: 24px;
      height: 24px;
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
      background: rgba(255, 255, 255, 0.1);
      border-color: var(--accent-blue);
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
      height: 100%;
      justify-content: center;
    }

    /* Buttons Base */
    button {
      background: var(--btn-bg);
      border: 1px solid var(--card-border);
      border-radius: 10px;
      color: var(--text-main);
      font-size: 0.85rem;
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

    /* Banner Pill Buttons for All Lights */
    .banner-pill-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin-top: 1px;
    }

    .btn-pill-wide {
      padding: 10px 16px;
      min-height: 42px;
      border-radius: 9999px;
      font-size: 0.9rem;
      font-weight: 800;
      letter-spacing: 0.04em;
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

    /* Floor Lamp Full-Width Controls Row */
    .floor-controls-row {
      display: grid;
      grid-template-columns: 1fr 3.8fr;
      gap: 8px;
      align-items: center;
    }

    .btn-floor-power {
      aspect-ratio: 1 / 1;
      width: 100%;
      max-height: 54px;
      font-size: 0.82rem;
      font-weight: 700;
      background: rgba(56, 189, 248, 0.12);
      border-color: rgba(56, 189, 248, 0.3);
      color: #7dd3fc;
      border-radius: 10px;
    }

    .floor-quad-row {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 6px;
    }

    .floor-quad-row .btn-quad {
      aspect-ratio: 1 / 1;
      width: 100%;
      max-height: 54px;
      padding: 0;
      font-size: 0.82rem;
      font-weight: 600;
      border-radius: 10px;
    }

    /* Square Buttons Grid for 2-button cards (Poster, Desk, Donut) */
    .btn-square-grid {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 8px;
      flex: 1;
      padding: 2px 0;
    }

    .btn-square {
      aspect-ratio: 1 / 1;
      width: 100%;
      max-width: 68px;
      max-height: 68px;
      font-size: 0.95rem;
      font-weight: 700;
      letter-spacing: 0.02em;
      border-radius: 12px;
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

    /* Bedside Morphing Card */
    .bedside-layout {
      display: grid;
      grid-template-columns: 1fr 1.25fr;
      gap: 6px;
      flex: 1;
      align-items: center;
      transition: all 0.28s cubic-bezier(0.16, 1, 0.3, 1);
    }

    .btn-bedside-power {
      aspect-ratio: 1 / 1;
      width: 100%;
      max-height: 68px;
      font-size: 0.85rem;
      font-weight: 700;
      background: rgba(56, 189, 248, 0.12);
      border-color: rgba(56, 189, 248, 0.3);
      color: #7dd3fc;
      border-radius: 10px;
      transition: all 0.28s cubic-bezier(0.16, 1, 0.3, 1);
    }

    .bedside-quad {
      display: grid;
      grid-template-columns: 1fr 1fr;
      grid-template-rows: 1fr 1fr;
      gap: 5px;
      transition: all 0.28s cubic-bezier(0.16, 1, 0.3, 1);
    }

    .btn-quad {
      padding: 0;
      aspect-ratio: 1 / 1;
      width: 100%;
      max-height: 32px;
      font-size: 0.74rem;
      font-weight: 600;
      border-radius: 8px;
    }

    /* Bedside Expanded State Morph */
    #bedsideCard.expanded-morph .bedside-layout {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    #bedsideCard.expanded-morph .btn-bedside-power {
      width: 100%;
      min-height: 38px;
      max-height: 38px;
      height: 38px;
      aspect-ratio: unset;
      border-radius: 10px;
      font-size: 0.88rem;
    }

    #bedsideCard.expanded-morph .bedside-quad {
      display: grid;
      grid-template-columns: 1fr 1fr;
      grid-template-rows: 1fr 1fr;
      gap: 8px;
      width: 100%;
      max-width: 144px;
      margin: 0 auto;
    }

    #bedsideCard.expanded-morph .bedside-quad .btn-quad {
      aspect-ratio: 1 / 1;
      width: 100%;
      max-width: 68px;
      max-height: 68px;
      font-size: 0.95rem;
      font-weight: 700;
      border-radius: 12px;
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
      min-height: 46px;
      padding: 12px;
      font-size: 0.88rem;
      font-weight: 600;
      border-radius: 10px;
    }

    .resync-bar {
      display: flex;
      align-items: center;
      justify-content: flex-end;
      gap: 6px;
      padding-top: 4px;
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
    <!-- Card 1: All Lights (Full-Width Banner) -->
    <div class="card card-full-width card-banner">
      <div class="card-header">
        <div class="card-title">All Lights</div>
        <div class="header-right">
          <span class="status-badge" id="hubState">Ready</span>
        </div>
      </div>
      <div class="card-body">
        <div class="banner-pill-grid">
          <button class="btn-pill-wide btn-master-on" onclick="sendCmd('all_on', 'ALL ON')">ALL ON</button>
          <button class="btn-pill-wide btn-master-off" onclick="sendCmd('all_off', 'ALL OFF')">ALL OFF</button>
        </div>
      </div>
    </div>

    <!-- Card 2: Floor Lamp (Full-Width Card) -->
    <div class="card card-full-width card-floor">
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
        <div class="floor-controls-row">
          <button class="btn-floor-power" onclick="sendCmd('floor_power', 'Floor Power')">Power</button>
          <div class="floor-quad-row">
            <button class="btn-quad" onclick="sendCmd('floor_brighter', 'Floor Brighter')" title="Brighter">▲</button>
            <button class="btn-quad" onclick="sendCmd('floor_dimmer', 'Floor Dimmer')" title="Dimmer">▼</button>
            <button class="btn-quad" onclick="sendCmd('floor_warmer', 'Floor Warmer')">Warm</button>
            <button class="btn-quad" onclick="sendCmd('floor_cooler', 'Floor Cooler')">Cool</button>
          </div>
        </div>
      </div>
      <div class="drawer" id="floorDrawer">
        <div class="drawer-inner">
          <div class="resync-bar">
            <span class="resync-label">Sync:</span>
            <button class="btn-pill" onclick="resync('floor', 1)">ON</button>
            <button class="btn-pill" onclick="resync('floor', 0)">OFF</button>
          </div>
        </div>
      </div>
    </div>

    <!-- Card 3: Poster Light (2x2 Grid) -->
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
        <div class="btn-square-grid">
          <button class="btn-square btn-on" onclick="sendCmd('poster_on', 'Poster ON')">ON</button>
          <button class="btn-square btn-off" onclick="sendCmd('poster_off', 'Poster OFF')">OFF</button>
        </div>
      </div>
      <div class="drawer" id="posterDrawer">
        <div class="drawer-inner">
          <button class="btn-wide" onclick="sendCmd('poster_color', 'Poster Color')">Color Cycle</button>
        </div>
      </div>
    </div>

    <!-- Card 4: Desk Lamp (2x2 Grid) -->
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
        <div class="btn-square-grid">
          <button class="btn-square btn-on" onclick="sendDeskCmd('ON', 'Desk ON')">ON</button>
          <button class="btn-square btn-off" onclick="sendDeskCmd('OFF', 'Desk OFF')">OFF</button>
        </div>
      </div>
      <div class="drawer" id="deskDrawer">
        <div class="drawer-inner">
          <button class="btn-wide" onclick="sendDeskCmd('TAP', 'Desk Cycle Color')">Cycle Color</button>
        </div>
      </div>
    </div>

    <!-- Card 5: Bedside Lamp (2x2 Grid - Dynamic Morphing Card) -->
    <div class="card" id="bedsideCard">
      <div class="card-header">
        <div class="card-title">Bedside</div>
        <div class="header-right">
          <span class="status-badge" id="bedsideBadge">OFF</span>
          <button class="btn-expand" onclick="toggleBedsideMorph(this)" aria-label="Expand">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="6 9 12 15 18 9"></polyline></svg>
          </button>
        </div>
      </div>
      <div class="card-body">
        <div class="bedside-layout" id="bedsideControls">
          <button class="btn-bedside-power" onclick="sendCmd('bedside_power', 'Bedside Power')">Power</button>
          <div class="bedside-quad">
            <button class="btn-quad" onclick="sendCmd('bedside_brighter', 'Bedside Brighter')" title="Brighter">▲</button>
            <button class="btn-quad" onclick="sendCmd('bedside_dimmer', 'Bedside Dimmer')" title="Dimmer">▼</button>
            <button class="btn-quad" onclick="sendCmd('bedside_warmer', 'Bedside Warmer')">Warm</button>
            <button class="btn-quad" onclick="sendCmd('bedside_cooler', 'Bedside Cooler')">Cool</button>
          </div>
        </div>
      </div>
      <div class="drawer" id="bedsideDrawer">
        <div class="drawer-inner">
          <div class="resync-bar">
            <span class="resync-label">Sync:</span>
            <button class="btn-pill" onclick="resync('bedside', 1)">ON</button>
            <button class="btn-pill" onclick="resync('bedside', 0)">OFF</button>
          </div>
        </div>
      </div>
    </div>

    <!-- Card 6: Donut Lamp (2x2 Grid) -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">Donut</div>
        <div class="header-right">
          <span class="status-badge" id="donutBadge">OFF</span>
        </div>
      </div>
      <div class="card-body">
        <div class="btn-square-grid">
          <button class="btn-square btn-on" onclick="sendCmd('donut_on', 'Donut ON')">ON</button>
          <button class="btn-square btn-off" onclick="sendCmd('donut_off', 'Donut OFF')">OFF</button>
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

    function toggleBedsideMorph(btn) {
      haptic();
      const card = document.getElementById('bedsideCard');
      const drawer = document.getElementById('bedsideDrawer');
      if (!card) return;
      const isExpanded = card.classList.toggle('expanded-morph');
      if (drawer) drawer.classList.toggle('open', isExpanded);
      if (btn) btn.classList.toggle('expanded', isExpanded);
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
