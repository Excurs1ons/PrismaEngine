#include "WebUIEditor.h"
#include "httplib.h"
#include "logger/Logger.h"
#include "EditorService.h"
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

namespace Prisma {

WebUIEditor::WebUIEditor() : m_server(std::make_unique<httplib::Server>()) {}

WebUIEditor::~WebUIEditor() {
    Stop();
}

bool WebUIEditor::Start(int port) {
    if (m_running) return true;
    m_port = port;
    m_running = true;

    m_server->Get("/api/v1/viewport/scene", [](const httplib::Request&, httplib::Response& res) {
        size_t size = 0; void* buf = EditorService::GetViewportRawBuffer(&size);
        if (buf) { 
            res.set_content((const char*)buf, size, "application/octet-stream"); 
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");
        } else res.status = 404;
    });

    m_server->Post(R"(/api/v1/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        std::string action = req.matches[1];
        glz::json_t params;
        try { if (!req.body.empty()) { if (auto ec = glz::read_json(params, req.body)) { LOG_WARN("WebUI", "API JSON 解析失败: {}", ec.custom_error_message); } } } catch (const std::exception& e) { LOG_WARN("WebUI", "API JSON 异常: {}", e.what()); }
        std::string buffer;
        if (auto ec = glz::write_json(EditorService::Dispatch(action, params), buffer)) {
            LOG_WARN("WebUI", "API JSON 序列化失败: {}", ec.custom_error_message);
        }
        res.set_content(buffer, "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    });

    m_server->Get("/", [this](const httplib::Request&, httplib::Response& res) {
        std::string html = R"raw(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
    <title>Prisma Pro Editor (Final)</title>
    <style>
        :root { --bg: #1e1e1e; --panel: #2d2d2d; --border: #111; --accent: #4e94f8; }
        body { margin:0; background:var(--bg); color:#ddd; font-family:sans-serif; overflow-y:auto; }
        .layout { display: grid; grid-template-columns: 240px 1fr 300px; height: 100vh; }
        @media (max-width: 900px) { .layout { grid-template-columns: 1fr; height: auto; } }
        .canvas-container { width: 100%; height: 450px; background: #222; border-bottom: 2px solid #111; position:relative; }
        canvas { width: 100%; height: 100%; object-fit: contain; image-rendering: pixelated; touch-action: pan-y; }
        .p-header { background:#383838; padding:8px 12px; font-size:10px; font-weight:bold; color:#888; text-transform:uppercase; border-bottom:1px solid #111; }
        .card { background:#333; margin:10px; padding:15px; border-radius:4px; }
        .comp-section { margin: 8px 0; padding: 8px; background: #2a2a2a; border-radius: 3px; }
        .comp-header { font-size: 11px; font-weight: bold; color: #4e94f8; text-transform: uppercase; margin-bottom: 6px; letter-spacing: 0.5px; }
        .comp-section label { display: block; font-size: 10px; color: #888; margin-top: 4px; }
        .comp-section .field { display: flex; justify-content: space-between; align-items: center; padding: 2px 0; font-size: 12px; }
        .comp-section .field label { margin: 0; font-size: 11px; color: #999; }
        .comp-section .field .val { color: #ddd; font-family: monospace; }
        .comp-section .vec3 { display: flex; gap: 4px; margin: 2px 0; }
        .comp-section .vec3 input { width: 60px; padding: 3px 4px; font-size: 11px; background: #1e1e1e; border: 1px solid #555; color: white; }
        .comp-section .vec3.ro { gap: 8px; font-size: 11px; color: #aaa; font-family: monospace; }
        .comp-section .vec3.ro span { color: #ddd; }
        .swatch { display: inline-block; width: 12px; height: 12px; border-radius: 2px; vertical-align: middle; border: 1px solid #555; }
        input { background:#1e1e1e; border:1px solid #555; color:white; padding:8px; width:100%; box-sizing:border-box; }
        .item { padding:8px 12px; border-bottom: 1px solid #222; cursor: pointer; }
        .item.sel { background: #3c5a81; }
        .nav { height: 40px; background: #333; display: flex; align-items:center; padding: 0 20px; gap: 20px; font-size: 13px; border-bottom:1px solid #000; }
        #console { height: 120px; background: #000; font-family: monospace; font-size: 11px; padding: 5px; overflow-y: scroll; color: #0f0; }
    </style>
</head>
<body>
    <div class="nav">
        <b style="color:var(--accent)">PRISMA IDE</b>
        <span id="stat">FPS: -- | Scene: --</span>
        <button onclick="api('entity/create')">New Entity</button>
        <button onclick="location.reload()">Force Refresh</button>
    </div>
    <div class="layout">
        <div style="display:flex; flex-direction:column; background:var(--panel); border-right:1px solid var(--border)">
            <div class="p-header">Hierarchy</div>
            <div id="hierarchy" style="overflow-y:auto; flex:1"></div>
        </div>
        <div style="display:flex; flex-direction:column; background:#000">
            <div class="p-header">Scene Viewport (Realtime)</div>
            <div class="canvas-container">
                <canvas id="v-canvas" width="1280" height="720"></canvas>
            </div>
            <div class="p-header">Console</div>
            <div id="console"></div>
            <div class="p-header">Assets</div>
            <div id="assets" style="flex:1; overflow-y:auto; padding:10px"></div>
        </div>
        <div style="display:flex; flex-direction:column; background:var(--panel); border-left:1px solid var(--border)">
            <div class="p-header">Inspector</div>
            <div id="inspector"></div>
        </div>
    </div>
    <script>
        const canvas = document.getElementById('v-canvas');
        const ctx = canvas.getContext('2d');
        const imgData = ctx.createImageData(1280, 720);
        let selectedId = null;

        async function api(path, body = null) {
            const r = await fetch('/api/v1/' + path, { method: 'POST', body: body ? JSON.stringify(body) : null });
            return r.json();
        }

        async function stream() {
            try {
                // 核心：添加时间戳后缀防止浏览器缓存黑屏帧
                const res = await fetch('/api/v1/viewport/scene?t=' + Date.now());
                const buf = await res.arrayBuffer();
                const view = new Uint8Array(buf);
                if (view.length === 1280 * 720 * 4) {
                    imgData.data.set(view);
                    ctx.putImageData(imgData, 0, 0);
                }
            } catch(e){}
            requestAnimationFrame(stream);
        }

        async function sync() {
            try {
                const [h, s, l, a] = await Promise.all([api('hierarchy/get'), api('engine/status'), api('console/get'), api('assets/list')]);
                document.getElementById('hierarchy').innerHTML = h.entities.map(e => 
                    `<div class="item ${e.id===selectedId?'sel':''}" onclick="select(${e.id})">📦 ${e.name}</div>`).join('');
                document.getElementById('console').innerHTML = l.logs.map(log => `<div>[${log.tag}] ${log.msg}</div>`).join('');
                document.getElementById('assets').innerHTML = a.assets.map(asset => `<div style="font-size:11px;padding:2px">📄 ${asset.path}</div>`).join('');
                document.getElementById('stat').innerText = `FPS: ${Math.round(s.fps)} | Scene: ${s.scene} | Objects: ${s.objects}`;
            } catch(e){}
        }

        function renderComp(e, id, comp) {
            const type = comp.type, d = comp.data;
            let html = '<div class="comp-section"><div class="comp-header">' + type + '</div>';

            if (type === 'Transform') {
                const p = d.position || [0,0,0];
                const r = d.rotation || [0,0,0];
                const s = d.scale || [1,1,1];
                html += '<label>Position</label><div class="vec3">' +
                    'X: <input type="number" step="0.1" value="' + p[0].toFixed(2) + '" onchange="updatePos(' + id + ',0,this.value)">' +
                    'Y: <input type="number" step="0.1" value="' + p[1].toFixed(2) + '" onchange="updatePos(' + id + ',1,this.value)">' +
                    'Z: <input type="number" step="0.1" value="' + p[2].toFixed(2) + '" onchange="updatePos(' + id + ',2,this.value)">' +
                    '</div>';
                html += '<label>Rotation (°)</label><div class="vec3">' +
                    'X: <input type="number" step="0.1" value="' + r[0].toFixed(1) + '" onchange="updateRot(' + id + ',0,this.value)">' +
                    'Y: <input type="number" step="0.1" value="' + r[1].toFixed(1) + '" onchange="updateRot(' + id + ',1,this.value)">' +
                    'Z: <input type="number" step="0.1" value="' + r[2].toFixed(1) + '" onchange="updateRot(' + id + ',2,this.value)">' +
                    '</div>';
                html += '<label>Scale</label><div class="vec3">' +
                    'X: <input type="number" step="0.1" value="' + s[0].toFixed(2) + '" onchange="updateScl(' + id + ',0,this.value)">' +
                    'Y: <input type="number" step="0.1" value="' + s[1].toFixed(2) + '" onchange="updateScl(' + id + ',1,this.value)">' +
                    'Z: <input type="number" step="0.1" value="' + s[2].toFixed(2) + '" onchange="updateScl(' + id + ',2,this.value)">' +
                    '</div>';
            }
            else if (type === 'MeshRenderer') {
                const c = d.color || [1,1,1,1];
                html += '<div class="field"><label>Mesh</label><span class="val">' + (d.mesh || 'none') + '</span></div>';
                html += '<div class="field"><label>Material</label><span class="val">' + (d.material || 'none') + '</span></div>';
                html += '<div class="field"><label>Color</label><span class="val"><span class="swatch" style="background:rgba(' +
                    Math.round(c[0]*255)+','+Math.round(c[1]*255)+','+Math.round(c[2]*255)+','+c[3].toFixed(2)+')"></span> ' +
                    c[0].toFixed(2) + ', ' + c[1].toFixed(2) + ', ' + c[2].toFixed(2) + ', ' + c[3].toFixed(2) + '</span></div>';
            }
            else if (type === 'Camera') {
                html += '<div class="field"><label>Projection</label><span class="val">' + (d.projection || 'Perspective') + '</span></div>';
                html += '<div class="field"><label>FOV</label><span class="val">' + (d.fov || 70).toFixed(1) + '°</span></div>';
                html += '<div class="field"><label>Near</label><span class="val">' + (d.near || 0.1).toFixed(3) + '</span></div>';
                html += '<div class="field"><label>Far</label><span class="val">' + (d.far || 1000).toFixed(1) + '</span></div>';
            }
            else if (type === 'Light') {
                const c = d.color || [1,1,1];
                html += '<div class="field"><label>Type</label><span class="val">' + (d.lightType || 'Directional') + '</span></div>';
                html += '<div class="field"><label>Color</label><span class="val"><span class="swatch" style="background:rgb(' +
                    Math.round(c[0]*255)+','+Math.round(c[1]*255)+','+Math.round(c[2]*255)+')"></span> ' +
                    c[0].toFixed(2) + ', ' + c[1].toFixed(2) + ', ' + c[2].toFixed(2) + '</span></div>';
                html += '<div class="field"><label>Intensity</label><span class="val">' + (d.intensity || 1).toFixed(2) + '</span></div>';
                html += '<div class="field"><label>Range</label><span class="val">' + (d.range || 10).toFixed(1) + '</span></div>';
            }
            html += '</div>';
            return html;
        }

        async function select(id) {
            selectedId = id; sync();
            const e = await api('entity/get', { id });
            let inspHtml = '<div class="card"><b>' + e.name + '</b><hr>';
            for (const comp of e.components) {
                inspHtml += renderComp(e, id, comp);
            }
            inspHtml += '</div>';
            document.getElementById('inspector').innerHTML = inspHtml;
        }

        async function updatePos(id, axis, v) {
            const e = await api('entity/get', { id });
            const p = e.components.find(x=>x.type==='Transform').data.position;
            p[axis] = Number(v);
            await api('entity/update', { id, data: { transform: { position: p } } });
        }

        async function updateRot(id, axis, v) {
            const e = await api('entity/get', { id });
            const r = e.components.find(x=>x.type==='Transform').data.rotation;
            r[axis] = Number(v);
            await api('entity/update', { id, data: { transform: { rotation: r } } });
        }

        async function updateScl(id, axis, v) {
            const e = await api('entity/get', { id });
            const s = e.components.find(x=>x.type==='Transform').data.scale;
            s[axis] = Number(v);
            await api('entity/update', { id, data: { transform: { scale: s } } });
        }

        setInterval(sync, 2000); sync(); stream();
    </script>
</body>
</html>
)raw";
        res.set_content(html, "text/html");
        return true;
    });

    m_serverThread = std::thread([this]() { m_server->listen("0.0.0.0", m_port); });
    return true;
}

void WebUIEditor::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_server) m_server->stop();
    if (m_serverThread.joinable()) m_serverThread.join();
}

void WebUIEditor::Update() {}

} // namespace Prisma
