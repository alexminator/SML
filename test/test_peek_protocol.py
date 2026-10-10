from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PeekProtocolContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.websocket_cpp = (ROOT / "src/net/WebSocket.cpp").read_text(encoding="utf-8")
        cls.websocket_h = (ROOT / "src/net/WebSocket.h").read_text(encoding="utf-8")
        cls.tasks_cpp = (ROOT / "src/tasks/tasks.cpp").read_text(encoding="utf-8")
        cls.peek_js = (ROOT / "data/js/peek.js").read_text(encoding="utf-8")
        cls.main_js = (ROOT / "data/js/main.js").read_text(encoding="utf-8")
        cls.effects_js = (ROOT / "data/js/effects.js").read_text(encoding="utf-8")
        cls.app_state_h = (ROOT / "src/state/AppState.h").read_text(encoding="utf-8")
        cls.app_state_cpp = (ROOT / "src/state/AppState.cpp").read_text(encoding="utf-8")

    def test_peek_has_one_subscriber_owner(self):
        self.assertIn("static uint32_t wsPeekClientId = 0;", self.websocket_cpp)
        self.assertIn("wsPeekClientId = clientId;", self.websocket_cpp)
        self.assertIn("else if (wsPeekClientId == clientId)", self.websocket_cpp)
        self.assertIn("if (wsPeekClientId != 0 && wsPeekClientId != clientId)", self.websocket_cpp)
        self.assertIn("clearPeekLiveState(client->id());", self.websocket_cpp)

    def test_stream_is_unicast_and_backpressure_aware(self):
        self.assertIn("bool buildPeekFrame(uint8_t *buf, size_t bufSize, size_t *len, uint32_t *clientId);", self.websocket_h)
        self.assertIn("void sendPeekFrame(uint32_t clientId, const uint8_t *buf, size_t len);", self.websocket_h)
        self.assertIn("client->queueLen() != 0", self.websocket_cpp)
        self.assertIn("!ws.availableForWrite(clientId)", self.websocket_cpp)
        self.assertIn("ws.binary(clientId, buf, len);", self.websocket_cpp)
        self.assertIn("peekClient->setCloseClientOnQueueFull(false);", self.websocket_cpp)
        self.assertIn("peekClient->setCloseClientOnQueueFull(true);", self.websocket_cpp)
        self.assertNotIn("ws.binaryAll(buf, len);", self.websocket_cpp)

    def test_peek_rate_limit_matches_browser_draw_rate(self):
        self.assertIn("PEEK_FRAME_INTERVAL_MS = 40", self.websocket_cpp)
        self.assertIn("const PEEK_FPS = 25;", self.peek_js)
        self.assertIn("PEEK_FRAME_MS = Math.round(1000 / PEEK_FPS)", self.peek_js)
        self.assertIn("hasNewData && frameIsFresh", self.peek_js)
        self.assertIn("vTaskDelay(pdMS_TO_TICKS(20));", self.tasks_cpp)
        self.assertIn("availableForWrite(clientId)", self.websocket_cpp)

    def test_led_task_forwards_target_client_id(self):
        self.assertIn("buildPeekFrame(peekBuf, sizeof(peekBuf), &peekLen, &peekClientId)", self.tasks_cpp)
        self.assertIn("sendPeekFrame(peekClientId, peekBuf, peekLen);", self.tasks_cpp)

    def test_stopping_preview_on_tab_switch_sends_unsubscribe(self):
        stop_block = self.main_js.split("// Stop peek render when leaving peek tab", 1)[1].split("// Init peek when tab activated", 1)[0]
        self.assertIn("sendCmd({ lv: false })", stop_block)

    def test_stopping_or_hiding_preview_unsubscribes(self):
        self.assertIn("sendCmd({ lv: false });", self.peek_js)
        self.assertIn("if (document.hidden && peek && peek.running)", self.peek_js)
        self.assertIn("peek.stop();\n      if (typeof sendCmd === 'function') sendCmd({ lv: false });", self.peek_js)
        self.assertIn("peekToggle.textContent = '▶ Start';", self.peek_js)

    def test_browser_renderer_drops_duplicate_frames(self):
        self.assertIn("this._lastDataTime = performance.now();", self.peek_js)
        self.assertIn("hasNewData && frameIsFresh", self.peek_js)
        self.assertIn("clearTimeout(this._renderTimer);", self.peek_js)

    def test_random_vu_start_sends_server_owned_duration_and_pool(self):
        self.assertIn("const RANDOM_VU_POOL = [", self.effects_js)
        self.assertIn("duration: getRandomVUDurationSeconds()", self.effects_js)
        self.assertIn("effectPool: RANDOM_VU_POOL", self.effects_js)
        self.assertIn("action: 'randomVUConfig', duration", self.effects_js)
        self.assertIn("localStorage.setItem('sml-random-vu-duration'", self.effects_js)

    def test_random_vu_server_accepts_config_and_broadcasts_it(self):
        self.assertIn('strcmp(action, "randomVUConfig") == 0', self.websocket_cpp)
        self.assertIn('json["randomVUDuration"] = randomVUDuration;', self.websocket_cpp)
        self.assertIn('json["randomVUPool"]', self.websocket_cpp)
        self.assertIn('data.randomVUDuration !== undefined', (ROOT / "data/js/websocket.js").read_text(encoding="utf-8"))
        self.assertIn('localStorage.setItem(\'sml-random-vu-duration\'', (ROOT / "data/js/websocket.js").read_text(encoding="utf-8"))

    def test_random_vu_cycles_without_repeating_and_broadcasts_next_id(self):
        self.assertIn("chooseNextRandomVUEffect(int currentEffectId)", self.app_state_h)
        self.assertIn("if (candidate != currentEffectId) return candidate;", self.app_state_cpp)
        self.assertIn("randomMode == 2 && !randomVUPool.empty()", self.tasks_cpp)
        self.assertIn("now - lastRandomSwitch >= (unsigned long)randomVUDuration * 1000UL", self.tasks_cpp)
        self.assertIn("stripLed.effectId = nextId;", self.tasks_cpp)
        self.assertIn("notifyClients(false);  // Broadcast new VU ID to master and slaves", self.tasks_cpp)
        self.assertIn("stateGeneration++", self.tasks_cpp)

    def test_random_vu_pool_excludes_removed_registry_ids(self):
        self.assertIn("if (id >= 12 && id <= 17)", self.websocket_cpp)
        self.assertIn("else if (id == 47 || id == 48)", self.websocket_cpp)
        self.assertNotIn("randomVUPool.push_back(49)", self.websocket_cpp)

    def test_preview_frames_do_not_broadcast_full_state(self):
        handler_start = self.websocket_cpp.index("if (!json[\"lv\"].isNull())")
        handler_end = self.websocket_cpp.index("const char *action = json[\"action\"]", handler_start)
        peek_handler = self.websocket_cpp[handler_start:handler_end]
        self.assertNotIn("notifyClients", peek_handler)
        self.assertNotIn("stateGeneration++", peek_handler)


if __name__ == "__main__":
    unittest.main()
