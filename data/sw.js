'use strict';

// Bump esta versión al cambiar cualquier asset del shell para invalidar cachés.
const CACHE = 'sml-v2';

// Debe reflejar exactamente los assets que carga index.html (CSS, JS, fuentes
// e imágenes del shell). Si un path no existe, addAll() rechaza y el SW no
// instala, así que mantenerlo en sincronía con data/index.html.
const PRECACHE = [
  '/',
  '/index.html',
  '/manifest.json',
  // CSS
  '/css/themes.css',
  '/css/layout.css',
  '/css/lamp.css',
  '/css/effects.css',
  '/css/tabs.css',
  '/css/components.css',
  '/css/responsive.css',
  '/css/fontawesome.css',
  '/css/solid.css',
  // JS
  '/js/iro.min.js',
  '/js/date.js',
  '/js/battery.js',
  '/js/player.js',
  '/js/peek.js',
  '/js/config.js',
  '/js/ui.js',
  '/js/battery-chart.js',
  '/js/websocket.js',
  '/js/effects.js',
  '/js/controls.js',
  '/js/main.js',
  // Fuentes (@font-face de layout.css / solid.css)
  '/fonts/Handmade.woff',
  '/fonts/impactreg.woff',
  '/fonts/fa-solid-900.woff2',
  // Iconos
  '/img/SML.png',
  '/SML.ico'
];

self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE).then(cache => cache.addAll(PRECACHE))
  );
  self.skipWaiting();
});

self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys().then(names => Promise.all(
      names.filter(n => n !== CACHE).map(n => caches.delete(n))
    ))
  );
  self.clients.claim();
});

self.addEventListener('fetch', event => {
  if (event.request.method !== 'GET') return;
  event.respondWith(
    caches.match(event.request).then(cached => {
      if (cached) return cached;
      return fetch(event.request).then(response => {
        if (response.ok) {
          const clone = response.clone();
          caches.open(CACHE).then(cache => cache.put(event.request, clone));
        }
        return response;
      });
    })
  );
});
