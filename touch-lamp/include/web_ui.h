#pragma once

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <meta name="theme-color" content="#090d16">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <title>Desk Lamp Controller</title>
  <style>
    :root {
      --bg: #090d16;
      --card-bg: rgba(22, 27, 34, 0.75);
      --card-border: rgba(255, 255, 255, 0.08);
      --text-main: #f0f6fc;
      --text-muted: #8b949e;
      --accent-on: #2ea043;
      --accent-off: #da3633;
      --accent-warm: #f59e0b;
      --accent-cyan: #38bdf8;
      --btn-bg: #21262d;
      --btn-border: #30363d;
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
      background: radial-gradient(circle at 50% 0%, #1a233a 0%, var(--bg) 75%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 24px 16px 40px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .container {
      width: 100%;
      max-width: 440px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding-bottom: 12px;
      border-bottom: 1px solid var(--card-border);
    }
    .header-left h1 {
      font-size: 1.4rem;
      font-weight: 700;
      letter-spacing: -0.02em;
    }
    .header-left p {
      font-size: 0.8rem;
      color: var(--text-muted);
      margin-top: 2px;
    }
    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 4px 12px;
      border-radius: 9999px;
      font-size: 0.75rem;
      font-weight: 600;
      letter-spacing: 0.05em;
      text-transform: uppercase;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid var(--card-border);
    }
    .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--text-muted);
      transition: background 0.3s;
    }
    .dot.online { background: #3fb950; box-shadow: 0 0 8px rgba(63, 185, 80, 0.6); }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 20px;
      box-shadow: var(--shadow);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    .state-display {
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 20px 0 10px;
      text-align: center;
    }
    .lamp-icon {
      width: 64px;
      height: 64px;
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      margin-bottom: 12px;
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid var(--card-border);
      transition: all 0.3s ease;
    }
    .lamp-icon.is-on {
      background: rgba(245, 158, 11, 0.15);
      border-color: rgba(245, 158, 11, 0.4);
      box-shadow: 0 0 24px rgba(245, 158, 11, 0.3);
      color: #fbbf24;
    }
    .lamp-icon.is-off {
      color: var(--text-muted);
    }
    .state-text {
      font-size: 1.5rem;
      font-weight: 700;
      letter-spacing: -0.01em;
    }
    .state-subtext {
      font-size: 0.8rem;
      color: var(--text-muted);
      margin-top: 4px;
    }
    .btn-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    button {
      border: 1px solid var(--btn-border);
      background: var(--btn-bg);
      color: var(--text-main);
      padding: 14px 16px;
      border-radius: 12px;
      font-size: 0.95rem;
      font-weight: 600;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: all 0.15s ease;
      touch-action: manipulation;
    }
    button:active {
      transform: scale(0.97);
    }
    .btn-on {
      background: rgba(46, 160, 67, 0.15);
      border-color: rgba(46, 160, 67, 0.4);
      color: #3fb950;
    }
    .btn-on:active {
      background: rgba(46, 160, 67, 0.3);
    }
    .btn-off {
      background: rgba(218, 54, 51, 0.15);
      border-color: rgba(218, 54, 51, 0.4);
      color: #f85149;
    }
    .btn-off:active {
      background: rgba(218, 54, 51, 0.3);
    }
    .btn-tap {
      grid-column: span 2;
      background: rgba(56, 189, 248, 0.12);
      border-color: rgba(56, 189, 248, 0.3);
      color: #38bdf8;
    }
    .btn-tap:active {
      background: rgba(56, 189, 248, 0.25);
    }
    .footer-info {
      font-size: 0.72rem;
      color: var(--text-muted);
      text-align: center;
      line-height: 1.4;
      margin-top: 8px;
    }
    .toast {
      position: fixed;
      bottom: 24px;
      left: 50%;
      transform: translateX(-50%) translateY(100px);
      background: #1f242c;
      border: 1px solid var(--card-border);
      color: var(--text-main);
      padding: 10px 18px;
      border-radius: 9999px;
      font-size: 0.85rem;
      box-shadow: var(--shadow);
      opacity: 0;
      transition: all 0.25s cubic-bezier(0.16, 1, 0.3, 1);
      pointer-events: none;
      z-index: 1000;
    }
    .toast.show {
      transform: translateX(-50%) translateY(0);
      opacity: 1;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="header-left">
        <h1>Desk Lamp</h1>
        <p>Smart Touch Controller</p>
      </div>
      <div class="status-pill">
        <div class="dot online" id="connDot"></div>
        <span id="connText">Connected</span>
      </div>
    </header>

    <div class="card">
      <div class="state-display">
        <div class="lamp-icon" id="lampIcon">
          <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M9 18h6"></path>
            <path d="M10 22h4"></path>
            <path d="M12 2v1"></path>
            <path d="M12 7a5 5 0 0 0-5 5c0 2 1.5 3.5 3 4.5h4c1.5-1 3-2.5 3-4.5a5 5 0 0 0-5-5z"></path>
          </svg>
        </div>
        <div class="state-text" id="lampState">--</div>
        <div class="state-subtext" id="lampSubtext">Checking optical sensor...</div>
      </div>

      <div class="btn-grid">
        <button class="btn-on" onclick="sendCommand('/api/lamp/on', 'Turning ON (Warm White)...')">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
          Turn ON
        </button>
        <button class="btn-off" onclick="sendCommand('/api/lamp/off', 'Turning OFF...')">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="9"></circle><line x1="12" y1="8" x2="12" y2="12"></line></svg>
          Turn OFF
        </button>
        <button class="btn-tap" onclick="sendCommand('/api/lamp/tap', 'Single Tap (1x)...')">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="23 4 23 10 17 10"></polyline><path d="M20.49 15a9 9 0 1 1-2.12-9.36L23 10"></path></svg>
          Cycle Color / Tap (1x)
        </button>
      </div>
    </div>

    <div class="footer-info">
      Closed-loop sensing via Standby LED (D1 / GPIO 5)<br>
      Electronic touch via NPN Transistor (D2 / GPIO 4)
    </div>
  </div>

  <div class="toast" id="toast"></div>

  <script>
    let isProcessing = false;

    function showToast(msg) {
      const t = document.getElementById('toast');
      t.textContent = msg;
      t.classList.add('show');
      setTimeout(() => t.classList.remove('show'), 2200);
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/lamp/status', { cache: 'no-store' });
        if (!res.ok) throw new Error('HTTP ' + res.status);
        const data = await res.json();
        
        const isOn = data.state === 'ON';
        const icon = document.getElementById('lampIcon');
        const stateText = document.getElementById('lampState');
        const subtext = document.getElementById('lampSubtext');

        if (isOn) {
          icon.className = 'lamp-icon is-on';
          stateText.textContent = 'ON';
          stateText.style.color = '#fbbf24';
          subtext.textContent = 'Standby LED Dark (Active)';
        } else {
          icon.className = 'lamp-icon is-off';
          stateText.textContent = 'OFF';
          stateText.style.color = 'var(--text-muted)';
          subtext.textContent = 'Standby LED Lit (Standby)';
        }

        document.getElementById('connDot').className = 'dot online';
        document.getElementById('connText').textContent = 'Connected';
      } catch (err) {
        document.getElementById('connDot').className = 'dot';
        document.getElementById('connText').textContent = 'Offline';
      }
    }

    async function sendCommand(endpoint, feedback) {
      if (isProcessing) return;
      isProcessing = true;
      showToast(feedback);

      try {
        const res = await fetch(endpoint, { method: 'POST', cache: 'no-store' });
        const data = await res.json();
        if (data.status === 'ok') {
          setTimeout(fetchStatus, 300);
        }
      } catch (err) {
        showToast('Command failed: ' + err.message);
      } finally {
        setTimeout(() => { isProcessing = false; }, 600);
      }
    }

    // Poll every 1.5s
    fetchStatus();
    setInterval(fetchStatus, 1500);
  </script>
</body>
</html>
)rawliteral";
