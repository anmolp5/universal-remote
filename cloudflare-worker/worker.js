/**
 * Universal Remote Cloudflare Worker
 *
 * Lightweight serverless REST API that translates incoming HTTP GET/POST calls
 * (from Apple Shortcuts, Siri, Widgets, Home Assistant, etc.) into secure
 * MQTT messages published to HiveMQ Cloud via direct TLS sockets.
 *
 * Example endpoints:
 *   GET/POST https://<your-worker>.workers.dev/all_on
 *   GET/POST https://<your-worker>.workers.dev/all_off
 *   GET/POST https://<your-worker>.workers.dev/donut_on
 *   GET/POST https://<your-worker>.workers.dev/donut_off
 *   GET/POST https://<your-worker>.workers.dev/poster_on
 *   GET/POST https://<your-worker>.workers.dev/bedside_power
 *   GET/POST https://<your-worker>.workers.dev/rf_power
 *   GET/POST https://<your-worker>.workers.dev/cmd?action=all_on
 */

import { connect } from 'cloudflare:sockets';

// Default Configuration (Can also be set as Cloudflare Environment Secrets)
const HIVEMQ_HOST = '0a91464cd01b488489e6e5603cba4112.s1.eu.hivemq.cloud';
const HIVEMQ_PORT = 8883;
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
      const host = env.HIVEMQ_HOST || HIVEMQ_HOST;
      const port = Number(env.HIVEMQ_PORT || HIVEMQ_PORT);
      const username = env.HIVEMQ_USER || HIVEMQ_USER;
      const password = env.HIVEMQ_PASS || HIVEMQ_PASS;
      const topic = env.MQTT_TOPIC || MQTT_TOPIC;

      // Connect and publish via direct TLS socket
      await publishMqttTls(host, port, username, password, topic, action);

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
 * Connect directly to HiveMQ Cloud via secure TLS socket on port 8883
 */
async function publishMqttTls(host, port, username, password, topic, payload) {
  const socket = connect({ hostname: host, port: port }, { secureTransport: 'on' });
  const writer = socket.writable.getWriter();
  const reader = socket.readable.getReader();

  try {
    // 1. Send MQTT CONNECT
    const clientId = 'siri-' + Math.random().toString(16).substring(2, 8);
    const connPacket = createConnectPacket(clientId, username, password);
    await writer.write(connPacket);

    // 2. Read MQTT CONNACK
    const { value: connack, done } = await reader.read();
    if (done || !connack || connack.length < 4) {
      throw new Error('Broker closed connection before sending CONNACK');
    }
    if (connack[0] !== 0x20 || connack[3] !== 0x00) {
      throw new Error(`MQTT Connection refused (code: ${connack[3]})`);
    }

    // 3. Send MQTT PUBLISH
    const pubPacket = createPublishPacket(topic, payload);
    await writer.write(pubPacket);

    // 4. Close gracefully
    try { await writer.close(); } catch(e){}
    try { await reader.cancel(); } catch(e){}
    try { await socket.close(); } catch(e){}
    return true;
  } catch (err) {
    try { await socket.close(); } catch(e){}
    throw err;
  }
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
    0x04, // MQTT 3.1.1
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
