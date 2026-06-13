# Product

## Register

product

## Users

DIY makers and electronics enthusiasts who built the lamp themselves from the open-source project. They are technically comfortable (assembly, wiring, ESP32 flashing) and appreciate detailed controls, customization, and real-time feedback. Their context is a personal or workshop environment where the lamp sits nearby — they use the web interface from a phone or laptop to control LED effects, stream Bluetooth audio, and monitor sensor data.

## Product Purpose

SML (Smart Music Lamp) turns a custom-built hardware lamp into a smart, connected device. The web interface is the primary control surface: it lets users toggle lamp power, choose from 40+ LED effects with adjustable parameters, pick colors, control Bluetooth audio playback, monitor temperature/humidity from the DHT22 sensor, and track battery status in real time — all served directly from the ESP32 over WiFi with no cloud dependency.

Success looks like: the interface feels as premium as the hardware, responds instantly on the local network, and gives the user complete creative control over their lamp's lighting and audio without ever feeling like a generic IoT dashboard.

## Brand Personality

**Cyberpunk / Neon** — dark, vibrant, glowing. Three words: *electric, precise, immersive.*

- Dark glass surfaces with saturated neon accent colors (gold, blue, purple, teal)
- Light feels like a material — glow effects, reflections, subtle animations
- Technical but not cold — the UI has personality without being playful
- Inspired by cyberpunk aesthetics but restrained: the glow serves readability and hierarchy, not decoration

## Anti-references

- **Generic IoT dashboards** — nothing that looks like Bootstrap/AdminLTE, standard smart home templates, or Philips Hue / SmartThings clones. No flat cards with drop shadows, no light gray on white.
- Industrial SCADA/UIs — not utilitarian or function-only
- Material Design — no floating action buttons, no elevation shadows, no Google-style density

## Design Principles

1. **Hardware-respecting authenticity** — the interface mirrors the real device. LED strip colors match what's shown, toggles feel like physical switches, and every control has instant feedback. The UI is an extension of the hardware, not a layer on top of it.

2. **Cyberpunk precision** — neon accents are surgical, not decorative. Glow communicates state (on/off, active/inactive, connected/disconnected), and every pixel of light has a purpose. Dark glass backgrounds make the accent colors sing without visual noise.

3. **Maker-accessible depth** — the core controls (power, effects, volume) are immediately obvious, but the full depth of customization (palettes, effect parameters, battery history) unfolds for those who seek it. The interface rewards exploration without demanding it.

4. **Performance as a feature** — served from an ESP32 over WiFi, every byte matters. The interface must feel instant locally despite constrained hardware. CSS animations are hardware-accelerated, JavaScript is lean, and paint cycles are minimized. A fast UI is a premium UI.

5. **Immersion through atmosphere** — ambient glow gradients, glassmorphism, and subtle entrance animations create a sense of place. The browser tab becomes part of the room's ambiance, not just a control panel.

## Accessibility & Inclusion

- Standard web best practices: keyboard navigation, semantic HTML, sufficient color contrast
- Low-end device optimization — the interface must perform well on older phones and tablets that might be dedicated to lamp control
- prefers-reduced-motion respected for all animations
- Status indicators use shape/position in addition to color (charging icons, signal bars) for color-blind safety
