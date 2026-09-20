/**
 * Universal Remote Cloudflare Worker
 *
 * Lightweight serverless REST API that translates incoming HTTP GET/POST calls
 * (from Apple Shortcuts, Siri, Widgets, Home Assistant, etc.) into secure
 * MQTT messages published to HiveMQ Cloud.
 *
 * Example endpoints:
 *   GET/POST https://<your-worker>.workers.dev/all_on
 *   GET/POST https://<your-worker>.workers.dev/all_off
 *   GET/POST https://<your-worker>.workers.dev/bedside_power
 *   GET/POST https://<your-worker>.workers.dev/donut_on
 *   GET/POST https://<your-worker>.workers.dev/poster_on
 *   GET/POST https://<your-worker>.workers.dev/rf_power
 *   GET/POST https://<your-worker>.workers.dev/cmd?action=all_on
 */

// Default Configuration (Can also be set as Cloudflare Environment Secrets)
const HIVEMQ_HOST = '0a91464cd01b488489e6e5603cba4112.s1.eu.hivemq.cloud';
const HIVEMQ_WSS_PORT = 8884;
const HIVEMQ_USER = 'anmolp';
const HIVEMQ_PASS = 'aplights';
const MQTT_TOPIC = 'home/universal_remote/command';

// Simple CORS headers
const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization',
};

export default {
  async fetch(request, env, ctx) {
    if (request.method === 'OPTIONS') {
      return new Response(null, { headers: corsHeaders });
    }

    const url = new URL(request.url);
    let action = url.pathname.replace(/^\/+/, '').trim();

    // Support query parameter ?action=...
    if (!action || action === 'cmd' || action === 'api') {
      action = url.searchParams.get('action') || '';
    }

    if (!action) {
      return new Response(
        JSON.stringify({
          status: 'ready',
          endpoints: [
            '/all_on',
            '/all_off',
            '/donut_on',
            '/donut_off',
            '/poster_on',
            '/poster_off',
            '/bedside_power',
            '/bedside_brighter',
            '/bedside_dimmer',
            '/rf_power',
            '/rf_brighter',
            '/rf_dimmer',
            '/rf_warmer',
            '/rf_cooler'
          ],
          usage: 'Call any endpoint via GET or POST to trigger instantly.'
        }, null, 2),
        {
          headers: { ...corsHeaders, 'Content-Type': 'application/json' }
        }
      );
    }

    try {
      // Connect to HiveMQ Cloud over secure WebSockets
      const brokerUrl = `wss://${env.HIVEMQ_HOST || HIVEMQ_HOST}:${env.HIVEMQ_WSS_PORT || HIVEMQ_WSS_PORT}/mqtt`;
      const username = env.HIVEMQ_USER || HIVEMQ_USER;
      const password = env.HIVEMQ_PASS || HIVEMQ_PASS;
      const topic = env.MQTT_TOPIC || MQTT_TOPIC;

      // Send MQTT packet via standard WebSocket handshake
      const res = await publishMqttOverWs(brokerUrl, username, password, topic, action);

      return new Response(
        JSON.stringify({
          success: true,
          action: action,
          topic: topic,
          timestamp: new Date().toISOString()
        }),
        {
          status: 200,
          headers: { ...corsHeaders, 'Content-Type': 'application/json' }
        }
      );
    } catch (err) {
      return new Response(
        JSON.stringify({
          success: false,
          error: err.message || String(err)
        }),
        {
          status: 500,
          headers: { ...corsHeaders, 'Content-Type': 'application/json' }
        }
      );
    }
  }
};

/**
 * Minimalist binary MQTT 3.1.1 CONNECT and PUBLISH generator over WebSocket
 */
async function publishMqttOverWs(brokerUrl, username, password, topic, payload) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(brokerUrl, ['mqtt']);
    const timeout = setTimeout(() => {
      try { ws.close(); } catch(e){}
      reject(new Error('WebSocket connection timed out to HiveMQ Cloud'));
    }, 4500);

    ws.addEventListener('open', () => {
      try {
        const connectPacket = createConnectPacket(
          'cf-worker-' + Math.random().toString(16).substring(2, 8),
          username,
          password
        );
        ws.send(connectPacket);
      } catch (err) {
        clearTimeout(timeout);
        reject(err);
      }
    });

    ws.addEventListener('message', (event) => {
      const data = new Uint8Array(event.data);
      // Byte 0x20 is CONNACK in MQTT
      if (data.length >= 2 && data[0] === 0x20) {
        if (data[3] === 0x00) {
          // Connected successfully! Now publish payload
          const pubPacket = createPublishPacket(topic, payload);
          ws.send(pubPacket);
          clearTimeout(timeout);
          // Allow small flush window before closing
          setTimeout(() => {
            try { ws.close(); } catch(e){}
            resolve(true);
          }, 60);
        } else {
          clearTimeout(timeout);
          reject(new Error(`CONNACK error code ${data[3]}`));
        }
      }
    });

    ws.addEventListener('error', (err) => {
      clearTimeout(timeout);
      reject(new Error('WebSocket error communicating with HiveMQ Cloud'));
    });
  });
}

function encodeString(str) {
  const enc = new TextEncoder().encode(str);
  const buf = new Uint8Array(2 + enc.length);
  buf[0] = (enc.length >> 8) & 0xff;
  buf[1] = enc.length & 0xff;
  buf.set(enc, 2);
  return buf;
}

function createConnectPacket(clientId, user, pass) {
  let flags = 0x02; // Clean session
  if (user) flags |= 0x80;
  if (pass) flags |= 0x40;

  const protoName = encodeString('MQTT');
  const clientPayload = encodeString(clientId);
  const userPayload = user ? encodeString(user) : new Uint8Array(0);
  const passPayload = pass ? encodeString(pass) : new Uint8Array(0);

  const varHeader = [
    ...protoName,
    0x04, // Level (3.1.1)
    flags,
    0x00, 0x3c // Keepalive 60s
  ];

  const payload = [
    ...clientPayload,
    ...userPayload,
    ...passPayload
  ];

  const remaining = varHeader.length + payload.length;
  return new Uint8Array([0x10, remaining, ...varHeader, ...payload]);
}

function createPublishPacket(topic, payloadStr) {
  const topicEnc = encodeString(topic);
  const payloadEnc = new TextEncoder().encode(payloadStr);
  const remaining = topicEnc.length + payloadEnc.length;
  return new Uint8Array([0x30, remaining, ...topicEnc, ...payloadEnc]);
}
