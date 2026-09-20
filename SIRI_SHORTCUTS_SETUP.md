# Siri Voice Control & Apple Shortcuts Setup Guide

This guide enables you to control all your lights hands-free using **Siri** on your **iPhone, Apple Watch, CarPlay, and HomePod**, working from anywhere in the world over 5G/cellular and Wi-Fi.

---

## Method 1: The 1-Click Cloudflare Worker (Silent Background Execution)

This method triggers the lights **in less than 200 milliseconds** completely in the background without opening Safari or switching apps.

### 1. Deploy the Free Cloudflare Worker (Takes 60 seconds)
1. Go to your free [Cloudflare Dashboard](https://dash.cloudflare.com/) $\rightarrow$ **Workers & Pages** $\rightarrow$ **Create Application** $\rightarrow$ **Create Worker**.
2. Give it a name (e.g. `lights` or `remote-api`) and click **Deploy**.
3. Click **Edit Code**.
4. Replace the sample code with the code from [`cloudflare-worker/worker.js`](file:///Users/anmolprabhakar/Library/Mobile%20Documents/com~apple~CloudDocs/universalRemote/cloudflare-worker/worker.js).
5. Click **Save and Deploy**.
6. Cloudflare gives you a free HTTPS link, for example:
   `https://lights.anmolp.workers.dev` (or you can attach your custom domain `lights.anmolprabhakar.com`).

---

### 2. Create the Apple Shortcut for Siri

1. On your iPhone, open the **Shortcuts** app.
2. Tap the **+** (plus icon) in the top right to create a new shortcut.
3. Rename the shortcut to whatever you want to speak to Siri:
   - Example: **"All Lights On"** (or **"Turn On All Lights"**).
4. Tap **Add Action** $\rightarrow$ search for **"Get Contents of URL"**.
5. In the URL field, paste your worker URL followed by `/all_on`:
   ```text
   https://lights.anmolp.workers.dev/all_on
   ```
6. Tap the little down arrow on the action (or expand options):
   - Method: **GET** (or **POST**).
7. Tap **Done**.

### 3. Duplicate for Other Commands

Create additional shortcuts in 10 seconds each:

| Shortcut Name (What you say to Siri) | URL in "Get Contents of URL" |
| :--- | :--- |
| **"All Lights Off"** / **"Goodnight"** | `https://lights.anmolp.workers.dev/all_off` |
| **"Turn on Floor Lamp"** | `https://lights.anmolp.workers.dev/floor_power` |
| **"Turn on Donut Lamp"** | `https://lights.anmolp.workers.dev/donut_on` |
| **"Turn off Donut Lamp"** | `https://lights.anmolp.workers.dev/donut_off` |
| **"Turn on Poster Light"** | `https://lights.anmolp.workers.dev/poster_on` |
| **"Toggle Bedside Lamp"** | `https://lights.anmolp.workers.dev/bedside_power` |

---

## Method 2: Zero-Deploy URL Shortcut (Using your existing GitHub Page)

If you don't want to set up Cloudflare right now, you can trigger actions directly via your existing website:

1. Open the **Shortcuts** app on your iPhone $\rightarrow$ tap **+**.
2. Name the shortcut: **"All Lights Off"**.
3. Add action: **"Open URL"**.
4. Enter URL:
   ```text
   https://anmolprabhakar.com/universal-remote/?action=all_off
   ```
5. Tap **Done**.
   - Whenever you say *"Hey Siri, All Lights Off"*, Safari opens the URL, the webpage connects and dispatches `all_off` over HiveMQ Cloud in under half a second!
