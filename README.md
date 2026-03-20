# PTI — Privacy Preserving Threat Intelligence Platform

> Share what you know. Reveal nothing else.

Two security teams. Two private indicator lists. One shared answer without either side exposing their data to the other.

---

## The problem

Threat intelligence is most valuable when it's shared. If your organisation detected a malicious IP last week, and a partner organisation detected the same IP independently, both of you become significantly more confident in that indicator the moment you know it's mutual. Coordinated blocklisting, joint incident response, and cross-sector threat correlation all depend on this kind of sharing.

But most organisations won't share their full indicator lists and for good reason:

- Indicators reveal what you've been attacked by, and therefore what you're vulnerable to
- They expose your detection coverage, telling adversaries what you can and can't see
- They may contain sensitive customer data (user IPs, internal hostnames)
- Regulatory frameworks (GDPR, HIPAA, CCPA) constrain what threat data can be shared with third parties
- Competing organisations may not trust each other with full visibility

The result is a coordination failure: everyone has partial pictures of the same threats, nobody shares, and adversaries exploit the gaps.

PTI solves this with **Private Set Intersection (PSI)**  a cryptographic technique that computes the overlap between two datasets without either party seeing the other's full list.

---

## How it works

```
┌─────────────────────────────────────────────────────────────────┐
│                        Signal Server                            │
│                   (peer discovery only)                         │
│              never sees any indicator data                      │
└───────────────────┬─────────────────────┬───────────────────────┘
                    │                     │
          ┌─────────▼──────┐   ┌──────────▼─────────┐
          │    Party A     │   │      Party B       │
          │                │   │                    │
          │ 1.1.1.1        │   │ 192.168.0.1        │
          │ evil.com       │   │ evil.com      ◀────shared
          │ abc123...      │   │ 8.8.8.8            │
          │ 10.0.0.5       │   │ 10.0.0.5      ◀────shared
          └───────┬────────┘   └──────────┬─────────┘
                  │                       │
                  └──── direct P2P ───────┘
                     (hashes only, never
                      raw indicators)
```

1. Both parties load their private indicator lists locally
2. Each hashes their indicators (SHA-256, parallelised across 5 threads)
3. Peers exchange hash lists directly the signal server is not involved
4. Each side computes the intersection locally using a hash set
5. Both parties learn only which indicators they share nothing about what the other has that they don't

---

## Use cases

### Incident response coordination

Two SOC teams investigating what appears to be the same intrusion campaign. One has seen lateral movement from `10.4.2.17`; the other flagged the same IP three days earlier in an unrelated alert. PTI lets them confirm shared indicators in under a second, without either team handing over their full investigation artefacts or exposing unrelated internal infrastructure.

```
Team A: 847 IPs, domains, file hashes from active IR
Team B: 1,203 indicators from prior campaign analysis

PTI result: 34 shared indicators — confirmed same threat actor
Time:       < 400 ms
Exposed:    nothing beyond the 34 shared indicators
```

### ISAC / sector-wide threat correlation

Information Sharing and Analysis Centers (ISACs) coordinate threat intelligence across organisations in the same sector financial services, healthcare, energy. Member organisations are often direct competitors with strong incentives not to share raw data.

PTI enables a hub-and-spoke model where each member runs a PSI session against the ISAC's aggregated indicator feed, learning only which of their own indicators are corroborated sector-wide without the ISAC or any member seeing the full indicator list of any other member.

### Threat feed validation

You have purchased a commercial threat intelligence feed and want to know how much of it overlaps with what you have already seen in your own telemetry before renewing the contract. The feed vendor wants to protect their proprietary data. You want to protect your internal telemetry.

PTI lets both sides compute the overlap without either revealing their full dataset to the other, giving you an honest overlap percentage with zero data exposure.

### Law enforcement / private sector coordination

A law enforcement agency investigating a cybercriminal operation wants to know which victim organisations have seen the same C2 infrastructure. Victim organisations cannot disclose customer data or internal network topology, and the agency cannot share active investigation details.

PTI lets each potential victim check for intersection against the agency's indicator list while both sides retain full control of their data. Confirmed overlap becomes the basis for a formal disclosure process rather than a prerequisite for it.

### Red team / blue team deconfliction

A red team operator and the internal blue team need to confirm whether blue has detected any red team infrastructure without blue learning the full scope of the engagement and without red learning the full extent of blue's detection capability.

PTI computes exactly what both sides need to know: which specific indicators have been mutually observed, and nothing more.

---

## Getting started
 
PTI builds and runs on **Linux** and **Windows**. The Makefile detects the OS automatically and sets the appropriate compiler flags, libraries, and executable extensions.
 
### Prerequisites
 
**Linux**
 
| Dependency | Purpose | Install |
|---|---|---|
| g++ (GCC 13+) | C++23 compiler | `apt install g++` |
| OpenSSL | SHA-256 hashing | `apt install libssl-dev` |
| libX11 | Raylib display | `apt install libx11-dev` |
| Raylib | GUI rendering | see below |
 
The vendored `lib/libraylib.a` is compiled for Windows and cannot be used on Linux. Install Raylib separately:
 
```bash
apt install libraylib-dev
 
# Or build from source (any distro)
git clone https://github.com/raysan5/raylib
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
```
 
**Windows**
 
| Dependency | Purpose | Notes |
|---|---|---|
| g++ (GCC 13+) | C++23 compiler | MinGW-w64 recommended |
| OpenSSL | SHA-256 hashing | Available via MinGW or vcpkg (`-lcrypto`) |
| Winsock2 | Networking | Included with Windows SDK (`-lws2_32`) |
| Raylib | GUI rendering | Pre-built — `lib/libraylib.a` included in repo |
 
On Windows, Raylib is ready to use out of the box via the vendored static library. No separate install needed.
 
### Build
 
```bash
git clone https://github.com/Addy897/PTI
cd PTI
 
# GUI client (default)
make pti_gui
 
# Signal server
make signal_server
```
 
The Makefile resolves platform differences automatically:
 
```makefile
ifeq ($(OS),Windows_NT)
    LDFLAGS  = -lws2_32 -lcrypto
    GUIFLAGS = -lraylib -lgdi32 -lwinmm
    EXEC_EXT = .exe
else
    LDFLAGS  = -lpthread -lcrypto
    GUIFLAGS = -lraylib -lX11
    EXEC_EXT =
endif
```
 
On Windows the output binaries will be `pti_gui.exe` and `signal_server.exe`.
 
### Running a session
 
**Step 1 — Start the signal server** (one instance, reachable by both parties)
 
```bash
# Linux
./signal_server
 
# Windows
signal_server.exe
 
# Listening on 0.0.0.0:4444
```
 
**Step 2 — Party A creates a room**
 
```bash
# Linux
./pti_gui
 
# Windows
pti_gui.exe
```
 
Click **Connect** → **Create Room** → select your indicator file. The room ID appears in the log.
 
**Step 3 — Party B joins** (share the room ID out-of-band)
 
```bash
./pti_gui
# Click Connect → enter Room ID → Join → select indicator file
# PSI runs automatically once both sides are ready
```
 
**Step 4 — Both parties see the result**
 
```
Connected to server.
Created Room ID: a3f9c2
 
Got intersection from 192.168.1.42 for room: a3f9c2
Found 34 common indicators.
    evil.example.com
    10.0.0.5
    44d88612fea8a8f36de82e1278abb02f
    ...
```
 
### Indicator file format
 
Plain text, one indicator per line. Any mix of IPs, domains, file hashes, or arbitrary strings. Blank lines and duplicates are removed automatically.
 
```
# IPs
1.2.3.4
192.168.0.100
 
# Domains
malicious.example.com
c2.badactor.net
 
# File hashes (MD5, SHA-256, etc.)
44d88612fea8a8f36de82e1278abb02f
```
 
---
 
## Architecture
 
```
PTI-main/
├── signal_server.cpp     # Peer discovery server (port 4444)
├── pti_client.cpp        # PSI protocol: hashing, exchange, intersection
├── pti_gui.cpp           # Raylib/raygui desktop GUI
├── mcp_server.cpp        # Per-session P2P listener (port 1234)
├── client.cpp            # TCP client wrapper
├── server.cpp            # TCP server base class
├── message.cpp           # Wire protocol (framed binary messages)
├── includes/
│   ├── message.hpp       # Message types: PSI_DATA, PEER_HLO, HLO_ACK, …
│   ├── raylib.h          # Raylib (vendored)
│   └── raygui.h          # raygui (vendored)
└── indicators/           # Sample datasets: 1k / 10k / 50k / 100k entries
```
 
The signal server is only used to exchange peer IP addresses. It never receives indicator data. All hashing and PSI computation happens directly over a peer-to-peer connection on port 1234.
 
---
 
## Performance
 
Benchmarked on loopback (localhost) with hex-encoded SHA-256 hashes. PTI deduplicates indicators on load the figures below reflect **unique indicators after deduplication**, which is the operative number for all phases.
 
| Input file | Unique after dedup | Hashing | Transfer | PSI logic | Total |
|---|---|---|---|---|---|
| 100k raw entries | ~35k unique | ~79 ms | ~185 ms | ~38 ms | ~376 ms |
 
The difference between the two 100k rows is the input data: the first file contained ~65% duplicate entries (realistic of threat feeds that repeat indicators across time windows); the second was fully unique. All PSI phases operate on the deduplicated set, so duplicate-heavy feeds are significantly cheaper to process.
 
Network transfer dominates at scale (~49% of total at 100k unique), as each SHA-256 hash is sent as a 64-character hex string roughly 6.4 MB per direction at 100k indicators. Hashing is parallelised across 5 threads and scales sub-linearly.
 
> **Note:** Deduplication time is not included in the benchmark figures above it occurs during file load before timing starts. At 100k raw entries this is approximately 10–30 ms of additional startup cost.
 
---
 
## Privacy model
 
| What your peer learns | What stays private |
|---|---|
| Indicators you both have flagged | Your full indicator list |
| The size of the intersection | Indicators only you have seen |
| Your IP address (P2P connection) | Your detection sources and coverage |
 
### Current limitations
 
PTI uses SHA-256 with a fixed salt (`PTI_DEMO_SALT`). This adequately protects non-enumerable indicators (file hashes, UUIDs, long strings) but is vulnerable to offline dictionary attacks against enumerable ones. IPv4 space is only ~4 billion addresses an adversary who captures your hash list can recover all IP indicators in minutes. Common domain wordlists make domain indicators similarly recoverable.
 
Production use against untrusted peers requires one of:
- An ephemeral per-session salt negotiated out-of-band
- **Diffie-Hellman PSI** (see roadmap) cryptographically secure regardless of indicator type, with no dependency on salt secrecy
 
---
 
## Roadmap
 
- [ ] **DH-PSI** — EC Diffie-Hellman PSI using Ristretto255; eliminates dictionary attack vulnerability and makes the protocol secure against actively malicious peers
- [ ] **BLAKE3 hashing** — 10–16× faster than SHA-256; brings hashing time from ~80 ms to sub-millisecond at 100k indicators, removing it as a meaningful cost
- [ ] **Binary wire encoding** — length-prefixed 32-byte hash records instead of hex strings; halves transfer payload and removes newline delimiter ambiguity
- [ ] **Differential privacy** — controlled noise injection on result size to prevent intersection cardinality from leaking set membership information
 
---
 
## Dependencies
 
| Library | License |
|---|---|
| [Raylib](https://www.raylib.com/) (vendored) | zlib |
| [raygui](https://github.com/raysan5/raygui) (vendored) | zlib |
| OpenSSL (system) | Apache 2.0 |
 
---
 
## Security notice
 
PTI is a research prototype. The fixed salt makes it unsuitable for production use against adversarial peers without the DH-PSI upgrade. Do not use the current build to share indicators whose recovery by an adversary would cause harm. See the roadmap for the path to a production-hardened implementation.
