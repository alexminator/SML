/* ──────────────────────────────────────────────────────────────────────────────
   websocket.js — SML WebSocket connection, message handling, status indicators
   Depends on: SML{}, sendCmd (defined here), showToast (ui.js),
               updateBatteryBar/updateWiFiBars/updateSolidIcon (main.js),
               highlightActiveCard/renderBatteryChart (effects.js/battery-chart.js),
               updateWSClientList/updateSystemInfo (config.js),
               handlePeekBinary (peek.js)
   ────────────────────────────────────────────────────────────────────────────── */

// ============================================================================
// WEBSOCKET
// ============================================================================

function connectWS() {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const url = `${protocol}//${location.host}/ws`;

  if (SML.ws && SML.ws.readyState === WebSocket.OPEN) return;

  try {
    SML.ws = new WebSocket(url);
    SML.ws.binaryType = 'arraybuffer';
  } catch (e) {
    console.error('WS connection failed:', e);
    scheduleReconnect();
    return;
  }

  // Init retry backoff if first connection attempt
  if (SML.wsRetryDelay === undefined) SML.wsRetryDelay = 1000;

  // Expose globals for player.js
  websocket = SML.ws;

  SML.ws.onopen = () => {
    const wasReconnecting = SML.connected === false && SML.wsReconnectCount > 0;
    SML.connected = true;
    SML.wsReconnectCount = 0;
    SML.wsRetryDelay = 1000;          // Reset backoff on successful connect
    updateConnectionStatus(true);
    clearTimeout(SML.wsReconnectTimer);
    // Remove skeletons when connected
    document.querySelectorAll('.skeleton').forEach(el => el.classList.remove('skeleton'));
    // Toast on reconnection
    if (wasReconnecting) {
      showToast('Conexión WebSocket restaurada', 'success');
    }
    // Doble seguro: pedir estado fresco tras reconexión (el ESP32 ya envía
    // estado en onWsEvent CONNECT, pero por si el buffer descartó algo)
    setTimeout(() => sendCmd({ action: 'requestFullState' }), 300);
  };

  SML.ws.onclose = (evt) => {
    const hadConnection = SML.connected === true;
    SML.connected = false;
    updateConnectionStatus(false);
    // Toast on unexpected disconnect (not clean close)
    if (hadConnection && evt.code !== 1000 && evt.code !== 1001) {
      showToast('Conexión WebSocket perdida — reconectando...', 'error');
    }
    // Re-add skeleton to data elements when disconnected
    document.querySelectorAll('.stat-value, .info-value, #weatherHumVal, #sysUptime, #sysHeap, #sysRSSI, #sysVersion, #battPercentDetail, #battVoltageDetail, #battStatus, #battChargeDetail')
      .forEach(el => {
        if (el.textContent === '--' || el.textContent === '--.-' || el.textContent === '--.--V' || el.textContent === '--%' || el.textContent === '--.-°C') {
          el.classList.add('skeleton');
        }
      });
    // Log close code for debugging
    if (evt.code !== 1000 && evt.code !== 1001) {
      console.debug(`[WS] closed (${evt.code}), reconnecting...`);
    }
    scheduleReconnect();
  };

  SML.ws.onerror = (evt) => {
    // onerror is always followed by onclose — log and let close trigger reconnect
    console.warn('[WS] connection error', evt instanceof Event ? evt.type : evt);
  };

  SML.ws.onmessage = (event) => {
    // Binary data for Peek (real-time LED stream from ESP32)
    if (event.data instanceof ArrayBuffer) {
      if (typeof handlePeekBinary === 'function') handlePeekBinary(event.data);
      return;
    }

    try {
      const data = JSON.parse(event.data);
      handleMessage(data);
    } catch (e) {
      // Ignore non-JSON
    }
  };
}

function scheduleReconnect() {
  clearTimeout(SML.wsReconnectTimer);

  // Exponential backoff: 1s → 1.5s → 2.25s → ... → 30s (cap)
  // + jitter aleatorio (±30%) para evitar que todos los clientes
  //   reconecten simultáneamente tras un reinicio del ESP32.
  const delay = SML.wsRetryDelay || 1000;
  SML.wsReconnectCount = (SML.wsReconnectCount || 0) + 1;
  SML.wsRetryDelay = Math.min(delay * 1.5, 30000);

  const jitter = 0.7 + Math.random() * 0.6;  // [0.7, 1.3]
  const actualDelay = Math.round(delay * jitter);

  console.debug(`[WS] reconnect #${SML.wsReconnectCount} in ${actualDelay}ms (base ${delay}ms)`);
  SML.wsReconnectTimer = setTimeout(connectWS, actualDelay);
}

function sendCmd(obj) {
  if (SML.ws && SML.ws.readyState === WebSocket.OPEN) {
    SML.ws.send(JSON.stringify(obj));
  }
}

// ── Page Visibility API: al volver de minimizar, refrescar estado ──
//   Cuando el navegador descongela la tab, el WebSocket puede estar
//   abierto pero los mensajes encolados pueden no reflejar el estado
//   completo. Pedir full state explícitamente evita datos stale.
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'visible') {
    if (SML.ws && SML.ws.readyState === WebSocket.OPEN) {
      console.debug('[VIS] tab visible → requesting full state');
      sendCmd({ action: 'requestFullState' });
    }
  }
});

// ============================================================================
// MESSAGE HANDLER
// ============================================================================

function handleMessage(data) {
  // ── CONSOLE DEBUG — valores que manda el ESP32 ──
  console.debug('[SML ← ESP32]', JSON.stringify(data));

  // ── PRIVATE CLIENT ID (mensaje individual al conectarse) ──
  if (data.yourClientId !== undefined) {
    SML.clientId = data.yourClientId;
    return;  // No hay otros campos en este mensaje
  }

  // ── TEMPERATURE / HUMIDITY ──
  if (data.temperature !== undefined) {
    SML.temp = data.temperature;

    // Update thermometer animation
    const value = parseFloat(data.temperature);
    const tempEl = document.getElementById('temperature');
    if (tempEl) {
      const minTemp = 0, maxTemp = 50;
      const pct = Math.min(100, Math.max(0, ((value - minTemp) / (maxTemp - minTemp)) * 100));
      tempEl.style.height = pct + '%';
      tempEl.dataset.value = value.toFixed(1) + '°C';
    }

    // Weather display
    setDataValue(document.getElementById('weatherTempVal'), value.toFixed(1) + '°C');
  }

  if (data.humidity !== undefined) {
    SML.hum = data.humidity;
    setDataValue(document.getElementById('weatherHumVal'), Math.round(data.humidity) + '%');
  }

  // ── BATTERY HISTORY (from ESP32 LittleFS — standalone message) ──
  if (data.battHistory && Array.isArray(data.battHistory)) {
    console.log('[BATT] RAW data received:', JSON.stringify(data.battHistory));
    var mappedCount = 0;
    SML.battHistory = data.battHistory.map(function(e) {
      var t = _battLogTimeToBrowser(e.t);
      console.log('[BATT] Entry ' + mappedCount + ': t=' + e.t + ' browserTime=' + new Date(t).toISOString() + ' v=' + e.v + ' l=' + e.l);
      mappedCount++;
      return {
        time: t,
        voltage: e.v,
        level: e.l !== undefined ? e.l : 0
      };
    });
    SML._battHistoryInitialized = true;
    console.log('[BATT] SML.battHistory length = ' + SML.battHistory.length);
    // Re-render chart if modal is open (auto-refresh)
    var chartEl = document.getElementById('battChartOffcanvas');
    if (chartEl && chartEl.classList.contains('open')) {
      console.log('[BATT] Chart is open, re-rendering');
      renderBatteryChart();
    } else {
      console.log('[BATT] Chart is NOT open, skipping render');
    }
    return;  // battHistory is always its own message — no other fields to process
  }

  // ── BATTERY ──
  if (data.level !== undefined) {
    SML.battery = data.level;
    batt.level = data.level;

    // Smart capping: la librería da 100% al llegar a MAXV (4.0V), pero el
    // TP4056 aún no ha terminado la carga CV. Mostramos 99% hasta que
    // fullBatt confirme que la batería está realmente llena.
    const isCharging = data.charging !== undefined ? data.charging : !!SML.charging;
    const isFullBatt = data.fullbatt !== undefined ? data.fullbatt : !!SML.fullBatt;
    const displayLevel = (isCharging && !isFullBatt && data.level >= 99) ? 99 : data.level;

    SML.battDisplayLevel = displayLevel;
    updateBatteryBar(displayLevel, isCharging);
  }
  if (data.battVoltage !== undefined) {
    SML.batteryV = data.battVoltage;
    setDataValue(document.getElementById('battVoltageDetail'), data.battVoltage.toFixed(2), 'V');
  }
  if (data.charging !== undefined) {
    SML.charging = data.charging;
    batt.charging = data.charging;
  }
  if (data.fullbatt !== undefined) {
    SML.fullBatt = data.fullbatt;
    batt.fullbatt = data.fullbatt;
  }

  // Update battery.js liquid visualization (use displayLevel for consistency with status bar)
  if (data.level !== undefined || data.charging !== undefined || data.fullbatt !== undefined) {
    if (typeof initBattery === 'function') {
      const syncBatt = {
        level: SML.battDisplayLevel ?? batt.level,
        charging: batt.charging,
        fullbatt: batt.fullbatt
      };
      initBattery(syncBatt);
    }
  }

  // ── Battery detail stats ──
  const prevLevel = SML.previousBattery;       // raw level anterior
  const rawLevel  = SML.battery || 0;           // raw level actual (sin cap)
  const dispLevel = SML.battDisplayLevel ?? rawLevel;  // con smart capping
  SML.previousBattery = rawLevel;

  // Flash al llegar a 100% real (fullBatt o rawLevel>=100 sin cap)
  const bpd = document.getElementById('battPercentDetail');
  if (bpd) {
    setDataValue(bpd, dispLevel, '%');
    if (rawLevel >= 100 && (prevLevel < 100 || prevLevel === undefined || prevLevel === -1)) {
      bpd.classList.add('flash');
      setTimeout(() => bpd.classList.remove('flash'), 3000);
    }
  }

  // ── STATUS + CHARGE — iconos puros (sin texto) ──
  const bs = document.getElementById('battStatus');
  const bcd = document.getElementById('battChargeDetail');

  // CHARGE: ⚡ verde pulsante si cargando, ⚡ gris atenuado si no
  const setCharge = (pulse) => {
    if (!bcd) return;
    if (pulse) {
      bcd.innerHTML = '<span class="fas fa-bolt-lightning animated-green charging-glow"></span>';
    } else {
      bcd.innerHTML = '<span class="fas fa-bolt-lightning charge-idle"></span>';
    }
  };

  if (bs) {
    if (SML.fullBatt) {
      setDataHTML(bs, '<span class="fas fa-check-circle animated-green"></span>');
      setCharge(false);
    } else if (SML.charging) {
      // Enchufado, cargando
      setDataHTML(bs, '<span class="fas fa-plug animated-green"></span>');
      setCharge(true);
    } else if (dispLevel <= 30) {
      // Batería baja sin corriente
      const cls = dispLevel <= 15 ? 'animated-red-fast' : 'animated-red';
      setDataHTML(bs, `<span class="fas fa-exclamation-triangle ${cls}"></span>`);
      setCharge(false);
    } else {
      // Funcionando a batería — icono según nivel
      let icon, cls;
      if (dispLevel <= 50) {
        icon = 'fa-battery-quarter';
        cls = 'animated-orange';
      } else if (dispLevel <= 75) {
        icon = 'fa-battery-half';
        cls = 'animated-yellow';
      } else {
        icon = 'fa-battery-three-quarters';
        cls = 'animated-green';
      }
      setDataHTML(bs, `<span class="fas ${icon} ${cls}"></span>`);
      setCharge(false);
    }
  }

  // ── LAMP / NEOPIXEL ──
  if (data.lampstatus !== undefined) {
    const lt = document.getElementById('lampToggle');
    if (lt) togglePowerCard(lt, data.lampstatus === 'on');
  }
  if (data.neostatus !== undefined) {
    SML.powerOn = data.neostatus === 'on';
    const neo = document.getElementById('neoToggle');
    if (neo) togglePowerCard(neo, SML.powerOn);
    updatePeekEffectInfo();
    // Si apagan el NeoPixel remotamente, detener random
    if (!SML.powerOn) {
      if (SML.randomFXMode) stopRandomFX();
      if (SML.randomVUMode) stopRandomVU();
    }
  }

  // ── BRIGHTNESS (FAB pill slider) ──
  if (data.neobrightness !== undefined) {
    // Ignorar broadcasts stale si el usuario acaba de cambiar brillo
    if (SML._lastBrightnessSent && Date.now() - SML._lastBrightnessSent < 400) {
      // skip — nuestro propio eco o broadcast anterior
    } else {
      SML.brightness = data.neobrightness;
      if (typeof SML._initFabBrightness === 'function') {
        SML._initFabBrightness(data.neobrightness);
      }
    }
  }

  // ── COLOR ──
  if (data.color) {
    // Ignorar broadcasts stale si el usuario acaba de cambiar color
    // (evita que un broadcast de una acción anterior salte el puntero
    //  cuando haces clics rápidos en el picker)
    if (!SML._lastColorSent || Date.now() - SML._lastColorSent >= 400) {
      SML.r = data.color.r ?? SML.r;
      SML.g = data.color.g ?? SML.g;
      SML.b = data.color.b ?? SML.b;
      updateSolidIcon(SML.r, SML.g, SML.b);
      if (SML.colorPicker && !SML._colorDragging) {
        SML._colorRemoteLock = true;
        SML.colorPicker.color.set({ r: SML.r, g: SML.g, b: SML.b });
        SML._colorRemoteLock = false;
      }
    }
  }

  // ── RANDOM MODE (check FIRST — overrides individual effect cards on ALL clients) ──
  //   El backend incluye randomMode en cada notifyClients. 0=off, 1=randomFX, 2=randomVU.
  //   notifySensorData NO incluye randomMode — no altera las flags.
  if (data.randomMode !== undefined) {
    if (data.randomMode === 1) {
      SML.randomFXMode = true;
      SML.randomVUMode = false;
      // Highlight actual effect card + RF button as mode indicator
      if (data.effectId !== undefined) SML.effectId = data.effectId;
      $$('.effect-card').forEach(c => {
        const id = parseInt(c.dataset.effectId);
        c.classList.toggle('active', id === SML.effectId || id === 99);
      });
      // Auto-scroll + category highlight según el modo random FX
      if (data.effectId !== undefined) {
        const fxMode = data.randomFXMode || 'all';
        if (fxMode === 'playlist') {
          highlightCategories(null);
          scrollPlaylistTo(data.effectId);
        } else if (fxMode === 'category') {
          const selected = getRandomCategories();
          highlightCategories(selected.length > 0 ? selected : null);
          scrollToCategoryCard(data.effectId);
        } else {
          // 'all' mode: scroll to effect pero sin highlight de categorías
          highlightCategories(null);
          scrollToCategoryCard(data.effectId);
        }
      }
    } else if (data.randomMode === 2) {
      SML.randomVUMode = true;
      SML.randomFXMode = false;
      if (data.effectId !== undefined) SML.effectId = data.effectId;
      $$('.effect-card').forEach(c => {
        const id = parseInt(c.dataset.effectId);
        c.classList.toggle('active', id === SML.effectId || id === 100);
      });
      if (data.effectId !== undefined) scrollToCategoryCard(data.effectId);
    } else {
      // randomMode === 0 — limpiar flags + highlights de categorías
      SML.randomFXMode = false;
      SML.randomVUMode = false;
      highlightCategories(null);
    }
  }

  // ── RANDOM FX CONFIG SYNC (from ESP32 broadcast) ──
  if (data.randomFXPool && Array.isArray(data.randomFXPool)) {
    if (data.randomFXMode) localStorage.setItem('sml-random-mode', data.randomFXMode);
    if (data.randomFXDuration) localStorage.setItem('sml-random-duration', String(data.randomFXDuration));
    if (data.randomFXMode === 'playlist') {
      saveRandomPlaylist(data.randomFXPool);
      syncPlaylistRow();
    }
  }
  if (data.randomFXCategories && Array.isArray(data.randomFXCategories)) {
    const CATEGORY_IDS = ['catFundamentals', 'catMoving', 'catDynamics', 'catPatterns', 'catStates'];
    const catIds = data.randomFXCategories
      .map(idx => CATEGORY_IDS[idx])
      .filter(Boolean);
    localStorage.setItem('sml-random-categories', JSON.stringify(catIds));
  }

  // ── RANDOM VU CONFIG SYNC (from ESP32 broadcast) ──
  if (data.randomVUDuration !== undefined) {
    localStorage.setItem('sml-random-duration', String(data.randomVUDuration));
  }

  // ── EFFECT ──
  // Find which effect is "on" — only if NOT in random mode (already handled above)
  let fxFound = false;

  // Regular effect lookup — skip if random mode is active
  if (data.randomMode === undefined || data.randomMode === 0) {
    // Map effect JSON names (from EffectRegistry.cpp) to web effect IDs
    const effectNameToId = {
      'fireStatus': 1,
      'movingdotStatus': 2,
      'rainbowbeatStatus': 3,
      'rwbStatus': 4,
      'rippleStatus': 5,
      'ballsStatus': 6,
      'juggleStatus': 7,
      'sinelonStatus': 8,
      'cometStatus': 9,
      'breathStatus': 10,
      'colorSweepStatus': 11,
      'rainbowVUStatus': 12,
      'oldVUStatus': 13,
      'rainbowHueVUStatus': 14,
      'rippleVUStatus': 15,
      'threebarsVUStatus': 16,
      'oceanVUStatus': 17,
      'tempNEOStatus': 18,
      'battNEOStatus': 19,
      'colorWipeStatus': 20,
      'theaterChaseStatus': 21,
      'runningLightsStatus': 22,
      'dissolveStatus': 23,
      'dualScanStatus': 24,
      'fadeStatus': 25,
      'meteorStatus': 26,
      'sparkleStatus': 27,
      'bpmStatus': 28,
      'plasmaStatus': 29,
      'fireworksStatus': 30,
      'lightningStatus': 31,
      'pride2015Status': 32,
      'colorwavesStatus': 33,
      'pacificaStatus': 34,
      'twinkleFOXStatus': 35,
      'auroraStatus': 36,
      'popcornStatus': 37,
      'larsonScannerStatus': 38,
      'heartbeatStatus': 39,
      'icuStatus': 40,
      'sunriseStatus': 41,
      'dripStatus': 42,
      'candleStatus': 43,
      'chunchunStatus': 44,
      'halloweenEyesStatus': 45,
      'noisemeterVUStatus': 47,
      'djlightVUStatus': 48,
    };

    for (const [key, id] of Object.entries(effectNameToId)) {
      if (data[key] === 'on') {
        if (SML.effectId !== id) {
          SML.effectId = id;
          $$('.effect-card').forEach(c => {
            c.classList.toggle('active', parseInt(c.dataset.effectId) === id);
          });
        }
        fxFound = true;
        break;
      }
    }
    // Also check for effect statuses NOT in the map (IDs 38+)
    if (!fxFound) {
      for (const key of Object.keys(data)) {
        if (key.endsWith('Status') && data[key] === 'on') {
          fxFound = true;
          break;
        }
      }
    }
    // If NO effect is "on" but Neo IS on → Solid (not in EffectRegistry, never has a Status:"on")
    // ⚠ Only trigger if this message actually contains effect/status data
    //   (notifySensorData sends lightweight payloads WITHOUT Status fields)
    if (!fxFound && SML.powerOn) {
      const hasEffectData = Object.keys(data).some(k => k.endsWith('Status') || k === 'neostatus');
      if (hasEffectData) {
        SML.effectId = 0;
        $$('.effect-card').forEach(c => {
          c.classList.toggle('active', parseInt(c.dataset.effectId) === 0);
        });
        updateSolidIcon(SML.r, SML.g, SML.b);
      }
    }

    // Direct effectId from server
    // ⚠ Solo cuando NeoPixel está encendido — si está apagado, ningún
    //   efecto debe marcarse como activo (todos en gris).
    if (data.effectId !== undefined && SML.powerOn) {
      SML.effectId = data.effectId;
      $$('.effect-card').forEach(c => {
        c.classList.toggle('active', parseInt(c.dataset.effectId) === data.effectId);
      });
    }
  }

  // ── PALETTE (top-level, enviado en cada broadcast) ──
  if (data.palette !== undefined) {
    const effId = data.effectId !== undefined ? data.effectId : SML.effectId;
    if (!liveEffectParams[effId]) liveEffectParams[effId] = {};
    liveEffectParams[effId].palette = parseInt(data.palette);
    // Update palette selector UI if open
    const containers = [
      document.getElementById('effectOffcanvasBody'),
      document.getElementById('paramSheetBody')
    ].filter(Boolean);
    containers.forEach(container => {
      container.querySelectorAll('.palette-swatch').forEach(s => {
        const isSelected = parseInt(s.dataset.paletteIndex) === parseInt(data.palette);
        s.classList.toggle('selected', isSelected);
        s.querySelector('.palette-check-mark').textContent = isSelected ? '✓' : '';
      });
    });
  }

  // Sincronizar botón de configuración en Peek
  updatePeekEffectInfo();

  // ── EFFECT PARAMS (real-time from server) ──
  // ESP32 sends { effectId: N, params: { speed: 120, intensity: 50, ... } }
  if (data.params && typeof data.params === 'object') {
    // Guardar en cache de valores activos para que renderEffectParams()
    // use los valores reales del ESP32, no los defaults del metadata.
    const paramsEffId = data.effectId !== undefined ? data.effectId : SML.effectId;
    if (!liveEffectParams[paramsEffId]) liveEffectParams[paramsEffId] = {};
    Object.assign(liveEffectParams[paramsEffId], data.params);

    const offcanvasBody = document.getElementById('effectOffcanvasBody');
    const sheetBody = document.getElementById('paramSheetBody');
    const containers = [offcanvasBody, sheetBody].filter(Boolean);
    containers.forEach(container => {
      Object.entries(data.params).forEach(([key, val]) => {
        const input = container.querySelector(`input[data-key="${key}"]`);
        if (!input) return;
        if (input.type === 'checkbox') {
          const shouldCheck = val === true || val === 1 || val === '1';
          if (input.checked !== shouldCheck) {
            console.debug(`[DEBUG] Sync check1: server sent ${JSON.stringify(val)}, shouldCheck=${shouldCheck}, was=${input.checked} → setting to ${shouldCheck}`);
            input.checked = shouldCheck;
          }
        } else {
          const numVal = parseInt(val);
          if (parseInt(input.value) !== numVal) {
            input.value = numVal;
            const valSpan = input.nextElementSibling;
            if (valSpan) valSpan.textContent = numVal;
          }
        }
      });
      // Update palette selection
      if (data.params.palette !== undefined) {
        const palIdx = parseInt(data.params.palette);
        container.querySelectorAll('.palette-swatch').forEach(s => {
          const isSelected = parseInt(s.dataset.paletteIndex) === palIdx;
          s.classList.toggle('selected', isSelected);
          s.querySelector('.palette-check-mark').textContent = isSelected ? '✓' : '';
        });
      }
    });
  }

  // ── EFFECT METADATA (del efecto activo, refresco continuo) ──
  // Solo cacheamos — NO re-renderizar (eso resetea sliders al default).
  // La UI se actualiza via data.params arriba y via HTTP /fxdata al iniciar.
  if (data.meta && typeof data.meta === 'string' && SML.effectId > 0) {
    const parsed = parseEffectMeta(data.meta);
    if (parsed) {
      if (!effectMetaCache[SML.effectId]) effectMetaCache[SML.effectId] = parsed;
    }
  }

  // ── BLUETOOTH ──
  if (data.btstatus !== undefined) {
    SML.btPower = data.btstatus === 'on';
    // Bridge to player.js sync (star-tab UI update)
    if (typeof window.playerSync === 'function') {
      window.playerSync({ bt_powerState: SML.btPower });
    }
  }

  // ── BATTERY EFFECT STATUS ──
  if (data.battNEOStatus !== undefined) {
    SML.battEffectActive = data.battNEOStatus === 'on';
    const bToggle = document.getElementById('batteryToggle');
    if (bToggle) bToggle.classList.toggle('active', SML.battEffectActive);
  }

  // ── TEMPERATURE EFFECT STATUS ──
  if (data.tempNEOStatus !== undefined) {
    SML.tempEffectActive = data.tempNEOStatus === 'on';
    const tToggle = document.getElementById('tempToggle');
    if (tToggle) tToggle.classList.toggle('active', SML.tempEffectActive);
  }

  // ── WIFI / NETWORK (state + status bar) ──
  if (data.rssi !== undefined) {
    SML.wifiRSSI = data.rssi;
    updateWiFiBars(data.rssi);
  }

  if (data.ip !== undefined) {
    SML.deviceIP = data.ip;
  }

  // ── WEBSOCKET CLIENT COUNT (status bar eye icon, slaves only) ──
  if (data.wsSlaves !== undefined && data.wsMax !== undefined) {
    updateWSClientCount(data.wsSlaves, data.wsMax);
  }

  // ── WEBSOCKET CLIENT LIST (Config tab) ──
  if (data.wsClientList !== undefined && Array.isArray(data.wsClientList)) {
    if (typeof updateWSClientList === 'function') {
      updateWSClientList(data.wsClientList, data.wsActionLog);
    }
    // ── MASTER/SLAVE DETECTION — find ourselves in the client list ──
    const myEntry = data.wsClientList.find(c => c.id === SML.clientId);
    if (myEntry) {
      const wasMaster = SML.isMaster;
      SML.isMaster = !!myEntry.master;
      // Handover on subsequent updates: slave → master, start timer if random active
      if (SML._hasReceivedClientList && !wasMaster && SML.isMaster) {
        // Random FX and VU are ESP32-driven — no frontend timer needed
      }
      SML._hasReceivedClientList = true;
    }
  }
  // ── WEBSOCKET ACTION LOG (mensaje independiente, sin client list) ──
  else if (data.wsActionLog !== undefined && Array.isArray(data.wsActionLog)) {
    if (typeof updateWSClientList === 'function') {
      updateWSClientList(null, data.wsActionLog);
    }
  }

  // ── CONFIG TAB (delegado a config.js) ──
  if (typeof updateSystemInfo === 'function') {
    updateSystemInfo(data);
  }

}

// ============================================================================
// STATUS INDICATORS
// ============================================================================

function updateConnectionStatus(connected) {
  const indicator = document.getElementById('connectionIndicator');
  if (indicator) {
    indicator.textContent = connected ? '●' : '○';
    indicator.style.color = connected
      ? 'var(--accent-success)'
      : 'var(--accent-danger)';
  }
}

function updateWSClientCount(slaves, max) {
  const indicator = document.getElementById('wsIndicator');
  const icon = document.getElementById('wsEyeIcon');
  const countEl = document.getElementById('wsCount');
  if (!indicator || !icon) return;

  if (slaves === 0) {
    // Only master or nobody — hide the eye icon entirely
    indicator.style.display = 'none';
    return;
  }

  // Show with slave count
  indicator.style.display = 'flex';
  if (countEl) countEl.textContent = slaves;

  // Color by load: green (few) → yellow (moderate) → red pulsing (near limit)
  // max = 8 total clients, so slaves max = 7
  icon.classList.remove('ws-warn', 'ws-danger');
  if (slaves >= max - 1) {
    icon.classList.add('ws-danger');    // 7 slaves = all slots full
  } else if (slaves >= max - 3) {
    icon.classList.add('ws-warn');      // 5-6 slaves = approaching limit
  }
  // else green (1-4 slaves, plenty of room)
}
