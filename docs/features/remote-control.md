# Local-network remote control

Church Deck serves a bundled, responsive browser controller from its native Qt application. No remote account, cloud service, Electron process or internet assets are required. The desktop editor remains Qt Quick.

## Pair a phone or tablet

1. Put the computer and device on the same trusted network. Guest Wi-Fi with client isolation may block access.
2. Open **Outputs → Remote**, select the computer's private IPv4 interface and choose **Start remote**. Allow local-network access in the OS if prompted. The computer must remain awake.
3. Choose **Controller** or **Speaker · read only**. Scan that QR code, or open the displayed HTTP address and enter its six-digit code. Give the device a recognizable name, then connect.
4. Take an editor presentation or program live from the desktop. The remote displays that copied live snapshot.

**This computer only** is a loopback testing option; phones cannot reach it. The listening port is allocated by the OS and changes on restart. A changed IP address requires stopping/restarting the service and pairing at the new address. No router forwarding or firewall-rule changes are performed.

Controllers can select an item or individual slide in the live service, move previous/next, and set/restore audience blackout. They cannot publish drafts, alter libraries, route displays or stop the application. Blackout and text-clear status are visible. The desktop remains the authority for live content.

Speakers receive a separate read-only screen: primary and translated text, next slide, private notes/message, countdown and a clock aligned to the host clock on state updates. The server rejects their commands even if a client constructs its own request.

Previews are JPEG **stills**, including catalog posters for video; they are not a live video/audio stream. CPU text/image rendering approximates the native slide layout. Presentation video/audio and physical/NDI outputs stay on the computer. Empty/end-of-service states do not request nonexistent previews.

## Connections and access

The desktop lists paired devices and can disconnect each one. **New codes** replaces both codes and revokes all sessions. **Stop remote**, application exit and restart also discard access. Sessions expire after 30 minutes without requests or 12 hours total. Up to 24 sessions are allowed. Two tabs in one browser profile share the same cookie/session and therefore the same effective role.

Polling runs about once a second, uses conditional state requests, and backs off to five seconds after failures. Controls disable when a request fails; no commands are queued or automatically replayed after reconnecting. Navigation requires the exact live snapshot and expected slide index. A command that races another operator is rejected and refreshed. Random request IDs prevent duplicate execution of recently accepted commands. The web interface is usable on mobile portrait screens and tablet/desktop layouts.

This service is for **trusted local networks**. It uses HTTP, without TLS: network observers can see pairing data and private speaker information. It is not intended for internet exposure. Pairing URLs carry codes only in fragments; the page removes the fragment immediately. Authentication uses a random in-memory capability in an HttpOnly, SameSite=Strict cookie, with no token in localStorage or query strings. Codes are separate per role and pairing attempts are limited per peer and globally. All successful pairings count toward the limit, too.

The HTTP transport bounds connection count, read time, header/body sizes and accepts one request per connection. Host/origin checks, a fixed route list, JSON-only mutations and a restrictive content-security policy prevent arbitrary file serving and cross-site commands. These controls do not replace a trusted LAN or encrypted transport.

## Implementation

- `src/remote/RemoteController`: interface selection, service lifecycle, pairing codes and session lifecycle.
- `src/remote/RemoteApi`: fixed HTTP routes, pairing, role checks, live state/deck and commands.
- `src/remote/RemoteHttpServer`: bounded Qt Network HTTP/1.1 transport.
- `src/remote/RemotePreview`: still rendering and QR image generation.
- `qml/remote/RemotePanel.qml`: operator pairing/session panel.
- `remote/`: bundled HTML/CSS/JavaScript, with local Telugu/Devanagari fonts served from application resources.

QR generation uses Nayuki's MIT-licensed C++ encoder, pinned with hashes under `third_party/qrcodegen`. See its [upstream documentation](https://www.nayuki.io/page/qr-code-generator-library).

## Verification and remaining work

The standalone test runner exercises actual loopback HTTP: bundled assets, authentication, multilingual state, role enforcement, live grouping/previews, navigation, replay/stale commands, blackout, code rotation, individual revocation, rate limits and malformed/oversized request framing. It does not require another device.

For interactive browser checks, run the isolated diagnostic fixture:

```sh
build/church-deck.app/Contents/MacOS/church-deck --fresh --remote-demo "$PWD/captures/remote"
```

It binds only `127.0.0.1`, publishes an original four-slide Telugu/English/Hindi fixture, opens the desktop pairing panel and writes a test-only `connection.json`, QR PNG and desktop screenshot. It stays running until closed. Treat its generated codes as ephemeral diagnostic data; do not commit captures.

Physical iOS/Android devices, Wi-Fi loss/roaming, OS firewall prompts and Windows deployment require on-device validation. NDI transmission and audio remain separate work; remote-control readiness does not validate NDI networking.
