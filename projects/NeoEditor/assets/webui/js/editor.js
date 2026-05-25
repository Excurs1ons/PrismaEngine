const canvas = document.getElementById('v-canvas');
const ctx = canvas.getContext('2d');
const imgData = ctx.createImageData(1280, 720);
let selectedId = null;

// ================================================================
// WebMessage 事件回调 (由 api.js handleCSharpEvent 调用)
// ================================================================

window.onSelectionChanged = function(entityId) {
    selectedId = entityId;
    highlightEntity(entityId);
    refreshInspector(entityId);
};

window.onSceneUpdated = function(timestamp) {
    refreshHierarchy();
};

window.onEngineStatus = function(status) {
    document.getElementById('stat').innerText =
        'FPS: ' + Math.round(status.fps) + ' | Scene: ' + status.scene + ' | Objects: ' + status.objects;
};

function highlightEntity(id) {
    document.querySelectorAll('#hierarchy .item').forEach(function(el) {
        el.classList.toggle('sel', String(el.dataset.id) === String(id));
    });
}

// ================================================================
// Streaming / Polling
// ================================================================

async function stream() {
    try {
        const res = await fetch('/api/v1/viewport/scene?t=' + Date.now());
        const buf = await res.arrayBuffer();
        const view = new Uint8Array(buf);
        if (view.length === 1280 * 720 * 4) {
            imgData.data.set(view);
            ctx.putImageData(imgData, 0, 0);
        }
    } catch(e) {}
    requestAnimationFrame(stream);
}

async function sync() {
    try {
        const [h, s, l, a] = await Promise.all([
            ApiClient.hierarchy(),
            ApiClient.engineStatus(),
            ApiClient.getLogs(),
            ApiClient.listAssets()
        ]);
        document.getElementById('hierarchy').innerHTML = h.entities.map(function(e) {
            return '<div class="item ' + (e.id===selectedId?'sel':'') + '" data-id="' + e.id + '" onclick="select(' + e.id + ')">📦 ' + e.name + '</div>';
        }).join('');
        document.getElementById('console').innerHTML = l.logs.map(function(log) {
            return '<div>[' + log.tag + '] ' + log.msg + '</div>';
        }).join('');
        document.getElementById('assets').innerHTML = a.assets.map(function(asset) {
            return '<div style="font-size:11px;padding:2px">📄 ' + asset.path + '</div>';
        }).join('');
        document.getElementById('stat').innerText = 'FPS: ' + Math.round(s.fps) + ' | Scene: ' + s.scene + ' | Objects: ' + s.objects;
    } catch(e) {}
}

// ================================================================
// Selection / Inspector
// ================================================================

async function select(id) {
    selectedId = id;
    highlightEntity(id);

    // 通过 WebMessage 通知 C# 选择变更 (WebUI → C#)
    sendCommand('selectEntity', { id: id });

    const e = await ApiClient.entityGet(id);
    const p = e.components.find(function(c) { return c.type === 'Transform'; }).data.position;
    document.getElementById('inspector').innerHTML =
        '<div class="card">' +
            '<b>' + e.name + '</b><hr>' +
            'X: <input type="number" step="0.1" value="' + p[0] + '" onchange="updatePos(' + id + ',0,this.value)">' +
            'Y: <input type="number" step="0.1" value="' + p[1] + '" onchange="updatePos(' + id + ',1,this.value)">' +
        '</div>';
}

async function refreshHierarchy() {
    try {
        const h = await ApiClient.hierarchy();
        document.getElementById('hierarchy').innerHTML = h.entities.map(function(e) {
            return '<div class="item ' + (e.id===selectedId?'sel':'') + '" data-id="' + e.id + '" onclick="select(' + e.id + ')">📦 ' + e.name + '</div>';
        }).join('');
    } catch(e) {}
}

async function refreshInspector(id) {
    if (!id) return;
    try {
        const e = await ApiClient.entityGet(id);
        const p = e.components.find(function(c) { return c.type === 'Transform'; }).data.position;
        document.getElementById('inspector').innerHTML =
            '<div class="card">' +
                '<b>' + e.name + '</b><hr>' +
                'X: <input type="number" step="0.1" value="' + p[0] + '" onchange="updatePos(' + id + ',0,this.value)">' +
                'Y: <input type="number" step="0.1" value="' + p[1] + '" onchange="updatePos(' + id + ',1,this.value)">' +
            '</div>';
    } catch(e) {}
}

async function updatePos(id, axis, v) {
    try {
        const e = await ApiClient.entityGet(id);
        const p = e.components.find(function(x) { return x.type === 'Transform'; }).data.position;
        p[axis] = Number(v);
        await ApiClient.entityUpdate(id, { transform: { position: p } });
    } catch(e) {}
}

// ================================================================
// Init
// ================================================================

setInterval(sync, 2000); sync(); stream();
