/* ──────────────────────────────────────────────────────────────────────────────
   battery-chart.js — SML Battery History Chart (canvas-based)
   Renders voltage history from ESP32 LittleFS log on a canvas element.
   ────────────────────────────────────────────────────────────────────────────── */

function renderBatteryChart() {
  console.log('[BATT] renderBatteryChart called');
  const canvas = document.getElementById('battChartCanvas');
  if (!canvas) {
    console.log('[BATT] Canvas not found!');
    return;
  }
  const data = SML.battHistory;
  const count = data.length;
  console.log('[BATT] renderBatteryChart: data.length=' + count + ', first entry:', count > 0 ? JSON.stringify(data[0]) : 'none');
  const ctx = canvas.getContext('2d');

  // Stats
  let avgV = 0;
  if (count > 0) {
    const voltages = data.map(d => d.voltage);
    const minV = Math.min(...voltages);
    const maxV = Math.max(...voltages);
    avgV = voltages.reduce((a, b) => a + b, 0) / voltages.length;
    setDataValue(document.getElementById('battChartMin'), minV.toFixed(3) + 'V');
    setDataValue(document.getElementById('battChartMax'), maxV.toFixed(3) + 'V');
    setDataValue(document.getElementById('battChartAvg'), avgV.toFixed(3) + 'V');
    setDataValue(document.getElementById('battChartCount'), count);
  }

  // Size canvas to container
  const rect = canvas.parentElement.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  const w = rect.width;
  const h = rect.height;
  canvas.width = w * dpr;
  canvas.height = h * dpr;
  canvas.style.width = w + 'px';
  canvas.style.height = h + 'px';
  ctx.scale(dpr, dpr);

  // Clear
  ctx.clearRect(0, 0, w, h);

  // ── Battery status icon (top-left corner) ──
  const iconX = 18;
  const iconY = 18;
  const iconS = 16;

  if (SML.fullBatt) {
    // Checkmark — verde
    ctx.strokeStyle = '#5cb85c';
    ctx.lineWidth = 2.5;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.beginPath();
    ctx.moveTo(iconX, iconY + iconS * 0.45);
    ctx.lineTo(iconX + iconS * 0.33, iconY + iconS * 0.72);
    ctx.lineTo(iconX + iconS * 0.7, iconY + iconS * 0.28);
    ctx.stroke();
  } else if (SML.charging) {
    // Lightning bolt — verde
    ctx.fillStyle = '#5cb85c';
    ctx.beginPath();
    ctx.moveTo(iconX + iconS * 0.55, iconY);
    ctx.lineTo(iconX + iconS * 0.25, iconY + iconS * 0.55);
    ctx.lineTo(iconX + iconS * 0.48, iconY + iconS * 0.55);
    ctx.lineTo(iconX + iconS * 0.38, iconY + iconS);
    ctx.lineTo(iconX + iconS * 0.75, iconY + iconS * 0.45);
    ctx.lineTo(iconX + iconS * 0.52, iconY + iconS * 0.45);
    ctx.closePath();
    ctx.fill();
  } else {
    // Battery outline — color según nivel
    const bLvl = SML.battDisplayLevel;
    let batColor;
    if (bLvl <= 15) batColor = '#ff4444';
    else if (bLvl <= 30) batColor = '#ff8800';
    else if (bLvl <= 50) batColor = '#ffaa00';
    else if (bLvl <= 75) batColor = '#aadd00';
    else batColor = '#5cb85c';

    const bw = iconS * 0.65;
    const bh = iconS * 0.85;
    const bx = iconX;
    const by = iconY + (iconS - bh) / 2;

    ctx.strokeStyle = batColor;
    ctx.lineWidth = 1.5;
    ctx.strokeRect(bx, by, bw, bh);
    // Terminal
    ctx.fillStyle = batColor;
    ctx.fillRect(bx + bw * 0.35, by - 2.5, bw * 0.3, 2.5);
    // Fill level
    if (bLvl > 0) {
      const fillH = (bh - 3) * Math.min(bLvl / 100, 1);
      ctx.fillRect(bx + 1.5, by + bh - 1.5 - fillH, bw - 3, fillH);
    }
  }

  // Need at least 2 points to draw
  if (count < 2) {
    ctx.fillStyle = '#666';
    ctx.font = `${Math.max(12, w / 25)}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(`Collecting data... (${count} readings so far)`, w / 2, h / 2);
    return;
  }

  const pad = { top: 12, bottom: 20, left: 10, right: 12 };
  const chartW = w - pad.left - pad.right;
  const chartH = h - pad.top - pad.bottom;

  // Voltage range with padding
  const voltages = data.map(d => d.voltage);
  const rawMin = Math.min(...voltages);
  const rawMax = Math.max(...voltages);
  const range = Math.max(rawMax - rawMin, 0.1);
  const vMin = Math.max(2.5, rawMin - range * 0.1);
  const vMax = Math.min(4.5, rawMax + range * 0.1);
  const vRange = vMax - vMin;

  const mapX = (i) => pad.left + (i / (count - 1)) * chartW;
  const mapY = (v) => pad.top + (1 - (v - vMin) / vRange) * chartH;

  // Grid lines (horizontal)
  ctx.strokeStyle = 'rgba(255,255,255,0.06)';
  ctx.lineWidth = 1;
  ctx.setLineDash([3, 5]);
  for (let v = Math.ceil(vMin * 2) / 2; v <= vMax; v += 0.5) {
    const y = mapY(v);
    ctx.beginPath();
    ctx.moveTo(pad.left, y);
    ctx.lineTo(w - pad.right, y);
    ctx.stroke();
    ctx.fillStyle = 'rgba(255,255,255,0.2)';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(v.toFixed(1) + 'V', w - pad.right + 4, y);
  }
  ctx.setLineDash([]);

  // Voltage area fill gradient
  const grad = ctx.createLinearGradient(0, pad.top, 0, h - pad.bottom);
  grad.addColorStop(0, 'rgba(56, 189, 248, 0.25)');   // cyan
  grad.addColorStop(1, 'rgba(56, 189, 248, 0.02)');
  ctx.beginPath();
  ctx.moveTo(mapX(0), mapY(voltages[0]));
  for (let i = 1; i < count; i++) ctx.lineTo(mapX(i), mapY(voltages[i]));
  ctx.lineTo(mapX(count - 1), h - pad.bottom);
  ctx.lineTo(mapX(0), h - pad.bottom);
  ctx.closePath();
  ctx.fillStyle = grad;
  ctx.fill();

  // Average voltage line (red dashed)
  const avgY = mapY(avgV);
  ctx.beginPath();
  ctx.setLineDash([6, 4]);
  ctx.moveTo(pad.left, avgY);
  ctx.lineTo(w - pad.right, avgY);
  ctx.strokeStyle = 'rgba(239, 68, 68, 0.7)';
  ctx.lineWidth = 1.5;
  ctx.stroke();
  ctx.setLineDash([]);
  // Average label
  ctx.fillStyle = '#ef4444';
  ctx.font = 'bold 10px sans-serif';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'bottom';
  ctx.fillText('Avg ' + avgV.toFixed(3) + 'V', pad.left + 4, avgY - 2);

  // Voltage line
  ctx.beginPath();
  ctx.moveTo(mapX(0), mapY(voltages[0]));
  for (let i = 1; i < count; i++) ctx.lineTo(mapX(i), mapY(voltages[i]));
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 2;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  ctx.stroke();

  // Level dots (overlay as small colored circles)
  const levels = data.map(d => d.level);
  ctx.beginPath();
  for (let i = 0; i < count; i++) {
    const x = mapX(i);
    const y = mapY(voltages[i]);
    const lvl = levels[i];
    const hue = lvl <= 20 ? 0 : lvl <= 40 ? 30 : lvl <= 60 ? 90 : 120;
    ctx.fillStyle = `hsla(${hue}, 80%, 55%, 0.6)`;
    ctx.beginPath();
    ctx.arc(x, y, 2.5, 0, Math.PI * 2);
    ctx.fill();
  }

  // Latest value label
  const lastV = voltages[count - 1];
  ctx.fillStyle = '#38bdf8';
  ctx.font = 'bold 12px sans-serif';
  ctx.textAlign = 'right';
  ctx.textBaseline = 'bottom';
  ctx.fillText(lastV.toFixed(3) + 'V', mapX(count - 1) - 6, mapY(lastV) - 4);

  // Time labels (start, middle, end)
  const timeLabels = [
    { i: 0, align: 'left' },
    { i: Math.floor(count / 2), align: 'center' },
    { i: count - 1, align: 'right' }
  ];
  ctx.font = '9px sans-serif';
  ctx.fillStyle = 'rgba(255,255,255,0.25)';
  timeLabels.forEach(({ i, align }) => {
    if (i >= count) return;
    const d = new Date(data[i].time);
    const t = d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    ctx.textAlign = align;
    ctx.textBaseline = 'top';
    ctx.fillText(t, mapX(i), h - pad.bottom + 4);
  });
}
