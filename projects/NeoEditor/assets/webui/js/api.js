const API_BASE = window.API_BASE || '/api/v1';

const ApiClient = {
    hierarchy()      { return this._call('hierarchy/get'); },
    engineStatus()   { return this._call('engine/status'); },
    getLogs()        { return this._call('console/get'); },
    listAssets()     { return this._call('assets/list'); },
    entityGet(id)    { return this._call('entity/get', { id }); },
    entityUpdate(id, data) { return this._call('entity/update', { id, data }); },
    entityCreate()   { return this._call('entity/create'); },
    eventsSnapshot() { return this._call('events/subscribe'); },

    async viewportScene() {
        const res = await fetch(API_BASE + '/viewport/scene?t=' + Date.now());
        return res.arrayBuffer();
    },

    async _call(path, body = null) {
        const r = await fetch(API_BASE + '/' + path, {
            method: 'POST',
            body: body ? JSON.stringify(body) : null
        });
        return r.json();
    }
};

async function api(path, body = null) {
    return ApiClient._call(path, body);
}

// ================================================================
// WebMessage 双向通信 (WebView2 C# ↔ WebUI)
// ================================================================

// WebMessage 事件监听: 接收 C# 实时推送
(function() {
    if (window.chrome && window.chrome.webview) {
        window.chrome.webview.addEventListener('message', function(event) {
            try {
                const msg = JSON.parse(event.data);
                handleCSharpEvent(msg.type, msg.data);
            } catch(e) {
                console.warn('[WebMessage] Invalid message:', event.data);
            }
        });
    } else {
        // HTTP 轮询回退: 非 WebView2 环境通过 polling 获取事件
        startHttpEventPolling();
    }
})();

// C# 事件分发
function handleCSharpEvent(type, data) {
    switch (type) {
        case 'selectionChanged':
            if (window.onSelectionChanged) window.onSelectionChanged(data);
            break;
        case 'sceneUpdated':
            if (window.onSceneUpdated) window.onSceneUpdated(data);
            break;
        case 'engineStatus':
            if (window.onEngineStatus) window.onEngineStatus(data);
            break;
        default:
            break;
    }
}

// 发送命令到 C# (WebUI → C#)
function sendCommand(action, data) {
    if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage(JSON.stringify({
            action: action,
            data: data || {}
        }));
    } else {
        // HTTP 回退: 通过 REST API 发送命令
        return ApiClient._call(action, data || {});
    }
}

// HTTP 轮询回退: 每 2 秒检查事件快照
let lastEventGeneration = -1;

function startHttpEventPolling() {
    setInterval(async () => {
        try {
            const snap = await ApiClient.eventsSnapshot();
            if (snap.generation && snap.generation !== lastEventGeneration) {
                lastEventGeneration = snap.generation;
                if (snap.selectionId) {
                    handleCSharpEvent('selectionChanged', snap.selectionId);
                }
                if (snap.engineStatus) {
                    handleCSharpEvent('engineStatus', snap.engineStatus);
                }
            }
        } catch(e) {
            // ignore polling errors
        }
    }, 2000);
}
