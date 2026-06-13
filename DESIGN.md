---
name: Smart Music Lamp (SML)
description: Dark glass control panel with neon accent colors for an ESP32-powered smart lamp
colors:
  accent: "#fbbf24"
  accent-hover: "#fde68a"
  accent-secondary: "#38bdf8"
  accent-secondary-hover: "#7dd3fc"
  accent-success: "#34d399"
  accent-warning: "#fb923c"
  accent-danger: "#f87171"
  bg-primary: "#060b14"
  bg-secondary: "#0c1425"
  bg-tertiary: "#121d35"
  bg-card: "#182640"
  bg-elevated: "#1c2d48"
  text-primary: "#edf2f7"
  text-secondary: "#a0aec0"
  text-muted: "#5a6a7e"
  border: "#2a3a5a"
typography:
  display:
    fontFamily: "'Handmade', cursive"
    fontSize: "clamp(2rem, 6vw, 3rem)"
    fontWeight: 400
    lineHeight: 1.1
    letterSpacing: "0.05em"
  impact:
    fontFamily: "'Impactreg', sans-serif"
    fontSize: "clamp(1.4rem, 4vw, 4.5rem)"
    fontWeight: 400
    lineHeight: 0.85
  body:
    fontFamily: "-apple-system, BlinkMacSystemFont, 'SF Pro', 'Segoe UI', Roboto, sans-serif"
    fontSize: "0.875rem"
    fontWeight: 400
    lineHeight: 1.6
  label:
    fontFamily: "-apple-system, BlinkMacSystemFont, 'SF Pro', 'Segoe UI', Roboto, sans-serif"
    fontSize: "0.7rem"
    fontWeight: 600
    letterSpacing: "1px"
  mono:
    fontFamily: "'SF Mono', 'Fira Code', 'Courier New', monospace"
    fontSize: "0.8rem"
    fontWeight: 400
rounded:
  sm: "8px"
  md: "12px"
  lg: "16px"
  xl: "24px"
  full: "50%"
  glass: "16px"
spacing:
  xs: "4px"
  sm: "8px"
  md: "16px"
  lg: "24px"
  xl: "32px"
  "2xl": "48px"
components:
  power-card-on:
    backgroundColor: "conic-gradient(#000, #444 10%, #000 51%, #555 57%, #222 60%, #555 67%, #000 75%)"
    textColor: "{colors.accent}"
    rounded: "{rounded.full}"
    padding: "6px"
    size: "82px"
  power-card-icon:
    backgroundColor: "#111"
    textColor: "{colors.text-muted}"
    rounded: "{rounded.full}"
    border: "4px solid {colors.text-muted}"
  card:
    backgroundColor: "{colors.bg-card} at 70% opacity"
    textColor: "{colors.text-primary}"
    rounded: "{rounded.glass}"
    padding: "{spacing.xl}"
  nav-btn:
    textColor: "{colors.text-muted}"
    fontSize: "0.6rem"
    fontWeight: 500
  nav-btn-active:
    textColor: "{colors.accent}"
  effect-card:
    backgroundColor: "transparent"
    rounded: "{rounded.md}"
    padding: "{spacing.md} {spacing.sm}"
  effect-card-active:
    backgroundColor: "{colors.bg-card} with 5% accent tint"
    rounded: "{rounded.md}"
---

# Design System: Smart Music Lamp (SML)

## 1. Overview

**Creative North Star: "The Dark Room"**

A control room atmosphere at night: amber and blue glow on black glass. Every surface is deep and translucent, every accent color carries a signal. The interface feels like it belongs to the hardware it controls — not a generic dashboard but the lamp's own consciousness rendered as light on glass.

SML is an ESP32-powered smart lamp controlled entirely through this web panel. The design is deliberately dark, dense, and luminous. Backgrounds recede into near-black navy, while neon accent colors (amber, sky blue, emerald) emerge from the darkness with purpose: they indicate state, guide interaction, and reward attention. Glow is not decorative — it communicates.

**Key Characteristics:**
- Dark glass surfaces with `backdrop-filter: blur(16px)` on cards, creating layered translucency
- Conic metallic power buttons (82×82px circles) that feel mechanical and precise
- Neon accent colors that serve as state indicators, not decoration
- Ambient radial gradients behind the interface for atmospheric depth
- Glass cards with an inner top-edge highlight line for polished dimensionality
- Three themeable color modes: SML Classic (amber), WLED Dark (blue), Midnight AMOLED (purple)

## 2. Colors

Three color themes, each with a distinct accent identity. The palette is built from deep, near-black backgrounds with saturated neon accents. Color carries meaning: the primary accent signals active state and interactivity; the secondary accent provides visual variety across the interface.

### Primary

- **Amber Glow** (`#fbbf24`, SML Classic default): The primary accent. Used for toggle active states, slider fill, active nav indicator, glow shadows, effect card active borders, and the power button's ON halo. Warm gold on deep navy — high contrast and immediately visible.
- **Signal Blue** (`#60a5fa`, WLED Dark theme alternative): Same role as Amber Glow, cooler and more technical. Used across all the same components when WLED Dark is active.
- **Volt Purple** (`#a78bfa`, Midnight AMOLED theme alternative): Same role, on pure black backgrounds for OLED power saving. More dramatic and saturated.

### Secondary

- **Electric Sky** (`#38bdf8`, SML Classic default): Secondary accent for visual variety. Appears in gradient text, accent gradient arrays, secondary card indicators, and the ambient background glow. Sky blue against deep navy reads as cool, technical, open.
- **EM Green** (`#34d399`, shared across themes): Success indicator. Battery charging status, online/connected states, positive confirmation signals. Always paired with shape cues (icon, position) for color-blind safety.
- **EM Green Hover** (`#6ee7b7`): Hover/active variant of success green.

### Tertiary

- **Warning Orange** (`#fb923c`, shared): Warning states — low battery, disconnecting, degraded operation.
- **Alert Red** (`#f87171`, shared): Danger states — critical battery, errors, disconnection.
- **Amber Glow Hover** (`#fde68a`): Hover variant of the primary accent. Used for hover glow intensification.

### Neutral

- **Deep Navy** (`#060b14`): Primary background. Near-black with a subtle blue undertone. The room's dark wall.
- **Dark Surface** (`#0c1425`): Secondary background for panels, sections, and the status bar.
- **Mid Navy** (`#121d35`): Tertiary surface for config areas and specialty sections.
- **Glass Card** (`#182640`): Card surface at 70% opacity with `backdrop-filter: blur(16px)`. The primary content container.
- **Elevated Surface** (`#1c2d48`): Hovered and elevated card surfaces. Also used as the glass composition base.
- **Glow White** (`#edf2f7`): Primary text. Slightly warm white for readability on deep navy.
- **Dimmed Light** (`#a0aec0`): Secondary text, labels, stats. Lower contrast for supporting information.
- **Muted Signal** (`#5a6a7e`): Muted text, disabled states, placeholder content, inactive icons.
- **Frame Border** (`#2a3a5a`): Card borders, divider lines, input borders at full opacity. Glass borders at 40–50% opacity.

### Named Rules

**The Three Themes Rule.** SML ships three complete color themes — SML Classic (amber + sky), WLED Dark (blue + emerald), Midnight AMOLED (purple + teal on pure black). Each theme reassigns every accent, surface, text, border, and interactive token. No partial theming: switching a theme is a complete atmosphere change. The active theme is persisted in `localStorage("sml-theme")` and applies on load.

**The Glow-Is-Signal Rule.** A glow effect always indicates an active, interactive, or connected state. The power button's ON halo, the nav button's active icon glow, the effect card's active border glow, the toggle's active fill — every luminous edge tells you something is engaged. If an element is not interactive and not active, it has no glow. `box-shadow: 0 0 24px var(--accent)` is the canonical glow declaration.

## 3. Typography

**Display Font:** `Handmade` (cursive, fallback `cursive`)
**Impact Font:** `Impactreg` (condensed sans, fallback `sans-serif`)
**Body Font:** `-apple-system, BlinkMacSystemFont, 'SF Pro', 'Segoe UI', Roboto, sans-serif`
**Mono Font:** `'SF Mono', 'Fira Code', 'Courier New', monospace`

**Character:** A four-face system with distinct personalities. `Handmade` is warm and decorative — used for the SML brand wordmark and temperature labels. `Impactreg` is condensed and powerful — used for the neon header logotype at large scale. The system sans-serif stack handles all body text, labels, and UI with crisp clarity. The mono face is reserved for data readouts (voltage, percentages, IP addresses). The two display faces never compete on the same screen: `Handmade` at moderate sizes (2–3rem), `Impactreg` at heroic sizes (4.5rem).

### Hierarchy
- **Display** (`Handmade`, 400 weight, `clamp(2rem, 6vw, 3rem)`, 1.1 line-height): The SML brand wordmark. Used exclusively in the header as the product identity. Not repeated elsewhere. Letter-spacing: 0.05em.
- **Impact Display** (`Impactreg`, 400 weight, `clamp(1.4rem, 4vw, 4.5rem)`, 0.85 line-height): The neon header logotype. Large, centered, with multi-layer text-shadow glow. Used as the primary branding element in the accepted Neon Header variant.
- **Title** (system sans, 600 weight, `0.95rem`, 1.3 line-height): Card titles, section headings inside panels, config group labels.
- **Body** (system sans, 400 weight, `0.875rem`, 1.6 line-height): All regular content — parameter descriptions, stats, status messages. Max line length: 65–75ch.
- **Label** (system sans, 600 weight, `0.7rem`, 1.2 line-height, `1px` letter-spacing, uppercase): Power button labels, nav button labels, stat labels. Always uppercase with tracking.
- **Mono** (`SF Mono`/`Fira Code`, 400 weight, `0.8rem`, 1.4 line-height): Battery voltage, IP address, debug data, numerical readouts.

### Named Rules

**The Two-Scale Rule.** The interface operates at two type scales: the product identity layer (display/impact, used in the header only) and the UI layer (title/body/label/mono, used everywhere else). They never intermix. A label never becomes a display face; the display face never shrinks to label size.

## 4. Elevation

A hybrid system: depth is conveyed through glass layers (translucent cards with backdrop blur) and soft shadows that appear on interactive hover. The default state is flat — cards sit on the background with subtle transparency and a thin inner highlight for definition. Interaction reveals depth: hover lifts the card with a soft shadow and an accent-tinged glow.

### Shadow Vocabulary
- **Card Rest** (`0 2px 8px rgba(0,0,0,0.2), 0 8px 32px rgba(0,0,0,0.08)`): Default card shadow. Defines the card as a surface without lifting it. The inner highlight (`inset 0 1px 0 rgba(255,255,255,0.04)`) provides the edge definition.
- **Card Hover** (`0 4px 16px rgba(0,0,0,0.25), 0 12px 40px rgba(0,0,0,0.12)`): Interactive hover lift. Paired with an accent-tinged glow (`0 0 40px color-mix(in srgb, var(--accent) 5%, transparent)`) and a `translateY(-1px)` transform.
- **Glow Shadow** (`0 0 24px var(--accent)`): Applied to active toggles, active nav items, ON power buttons, and active effect cards. The glow is always the accent color at varying opacities. Never white, never black.
- **Glow Soft** (`0 0 40px color-mix(in srgb, var(--accent) 30%, transparent)`): Diffuse ambient glow behind major active elements.

### Named Rules

**The Flat-by-Default Rule.** Surfaces are flat at rest. Shadows appear only as a response to state (hover, activation, or connection). A card at rest has blur and transparency but no floating shadow. A card under interaction lifts. This prevents the interface from feeling like a stack of paper cards and keeps it feeling like engraved glass.

## 5. Components

### Buttons

- **Shape:** Circular power buttons with conic metallic gradient (82×82px, `border-radius: 50%`). The outer ring is a conic gradient from black to dark grays to simulate a machined metal bezel. The inner icon area uses a linear gradient (`210deg, #333 20%, #111`) with inset shadow for a concave metallic effect.
- **ON State:** The inner border shifts from muted gray to the accent color. An accent glow halo appears: `box-shadow: 0 0 16px color-mix(in srgb, var(--accent) 30%, transparent)`.
- **Label:** 0.7rem, 600 weight, uppercase, 1px letter-spacing. Sits below the button. Color matches the accent when ON.
- **Press Feedback:** `transform: scale(0.96)` on active press. No transition — instant scale-down.
- **Secondary Buttons** (`btn-reset-params`): Pill-shaped (20px radius), transparent background, 1px border. On hover: accent border, accent text, subtle accent glow background `(8% opacity)`.

### Cards

- **Corner Style:** Gently curved (`border-radius: 16px`).
- **Background:** `color-mix(in srgb, var(--bg-card) 70%, transparent)` — the card surface is the Glass Card token at 70% with `backdrop-filter: blur(16px)`.
- **Shadow Strategy:** Hybrid (see Elevation section). Rest: soft shadow + inner highlight. Hover: lifted shadow + accent glow.
- **Border:** `1px solid color-mix(in srgb, var(--border) 40%, transparent)`. Subtle and glassy, not structural.
- **Internal Padding:** 24px (`--space-xl`) on all sides.
- **Inner Highlight:** A 1px pseudo-element gradient at the top edge (`linear-gradient(90deg, transparent, rgba(255,255,255,0.08), transparent)`) for a subtle glass edge shine.
- **Entrance Animation:** Cards within tab content stagger-enter when the tab activates (0.03s delay increments, 5 cards max). Animation is `opacity 0.4s ease-out, transform 0.4s ease-out` — subject to `prefers-reduced-motion`.

### Effect Cards (Grid Items)

- **Shape:** Compact card (`border-radius: 12px`), text-centered, 44×44px circular icon area at top.
- **Rest:** Transparent background, subtle glass border.
- **Active:** Accent border, background tinted with 5% accent, multiple layers of accent glow shadow.
- **Hover:** Lift with `translateY(-5px)`, stronger accent border, multi-layer glow shadow, background shifts toward elevated surface.
- **Press:** `scale(0.95)` instant feedback.
- **Icons:** Inline SVG, single-color (`currentColor`), 44×44px, circular background via `--effect-color`.

### Navigation (Bottom Tab Bar)

- **Structure:** Fixed bottom bar, 64px height, full-width flex row. 5 tabs (Lamp, Music, Weather, Battery, Config).
- **Active Indicator:** 2.5px bar at the top of the active tab, accent-colored, animates in from `scaleX(0)` to `scaleX(1)`.
- **Tab Item:** Column layout — icon (1.3rem) + label (0.6rem, 500 weight, 0.3px letter-spacing).
- **Active State:** Icon scales to 1.15 with accent glow filter. Label color shifts to accent.
- **Press:** Icon scales down to 0.85 instantly.

### Toggles

- **Shape:** Pill slider, 42×24px. The track is `--toggle-bg` (dark surface border), the knob is an 18px white circle with a 3px inset from edges.
- **Checked:** Track fills with `--toggle-active` (accent), knob slides `translateX(18px)` and turns white.
- **Transition:** 0.15s on both background and transform.

### Range Sliders

- **Track:** Full width, height (~6px implied), `--slider-track` color.
- **Fill:** `--slider-fill` (accent color) on the active range side.
- **Thumb:** Circular (`--slider-thumb`, accent color), spring scale on hover (`scale(1.15)`) with accent glow.
- **Appearance:** Custom `-webkit-appearance: none` with platform-native appearance removed.

### Theme Selector

- **Shape:** Flex row of selectable cards, min-width 100px each, `border-radius: 12px`.
- **Swatch:** 100% width, 48px height, gradient preview of each theme's accent pair.
- **Active:** 2px accent border, accent background tint (6%).
- **Press:** `scale(0.96)`.

### Neon Header (Signature Component)

- **Structure:** Centered vertical lockup: "SML" logotype in `Impactreg` at 4.5rem, subtitle "Smart Music Lamp" in `Handmade` at 1.6rem, thin glow line below.
- **Logotype:** Solid accent color (`var(--accent)`) with three-layer text-shadow: tight glow (10px), medium bloom (30px), diffuse aura (60px). No gradient text.
- **Subtitle:** White at 70% opacity, 5px letter-spacing, subtle overlap (`margin-top: -1rem`) with the logotype.
- **Glow Line:** 60px wide, 2px tall, center gradient from transparent → accent → transparent. Animated with `nhLinePulse`: 3s ease-in-out cycle pulsing width 60→100px and opacity 0.8→1.0.
- **Subtitle** uses `Handmade` at 700 weight (bolder than the standard display weight) for improved readability at smaller size.

## 6. Do's and Don'ts

### Do:
- **Do** use the accent color as a deliberate state indicator — ON, active, connected, selected. If it has glow, it's interactive.
- **Do** use glass card backgrounds with `backdrop-filter: blur(16px)` for content containers. The blur and transparency are the system's primary depth mechanism.
- **Do** use `prefers-reduced-motion` media queries to disable entrance animations and pulsing effects. Motion enhances atmosphere; it never gates content.
- **Do** use shape and position alongside color for status indicators (charging icon + green dot, battery bars + red text). Color should never be the sole carrier of meaning.
- **Do** keep the header display faces separated: `Handmade` for the brand wordmark, `Impactreg` for the heroic logotype. Never swap their roles.
- **Do** use the conic metallic power buttons as the primary power affordance. The machined-metal aesthetic reinforces the hardware-connected feel.
- **Do** keep a 16px minimum padding inside glass cards. Tighter padding breaks the glass illusion.

### Don't:
- **Don't** use gradient text (`background-clip: text` with a gradient). Text emphasis comes from weight, size, or glow — never from a gradient fill.
- **Don't** use side-stripe borders (border-left or border-right greater than 1px as a colored accent). Use full borders, background tints, or glow instead.
- **Don't** use generic IoT dashboard aesthetics — no flat white cards with drop shadows, no light gray on white, no Bootstrap/AdminLTE styling, no Material Design elevation shadows or floating action buttons.
- **Don't** apply glow to inactive or non-interactive elements. Glow is a signal, not a decoration.
- **Don't** use `snprintf()` with format specifiers in the ESP32 firmware — it causes "Interrupt wdt timeout" crashes after WiFi initialization.
- **Don't** let glass cards nest inside each other. One level of glass depth is the limit — nested cards break the translucency illusion.
- **Don't** use the `Handmade` display face below 1.5rem or above 3.5rem (outside the header). It loses legibility at small sizes and overwhelms at large.
- **Don't** mix two display faces on the same screen element. `Handmade` and `Impactreg` should never appear in the same component.
