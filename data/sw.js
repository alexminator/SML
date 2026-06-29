'use strict';

const CACHE = 'sml-v1';

const PRECACHE = [
  '/',
  '/index.html',
  '/css/themes.css',
  '/css/styles.css',
  '/css/fontawesome.css',
  '/css/solid.css',
  '/js/iro.min.js',
  '/js/date.js',
  '/js/battery.js',
  '/js/player.js',
  '/js/peek.js',
  '/js/config.js',
  '/js/main.js',
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
