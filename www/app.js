"use strict";
// ============================================================
// 摄像头监控控制台 —— 前端主逻辑（TypeScript）
//
// 编译：在 API 目录执行 tsc
//       Web_UI/app.ts  ──tsc──▶  www/app.js  ──C++服务器──▶  浏览器
//
// 页面由服务器同源托管，所以所有接口都用相对路径 "/api/..."，
// 不需要写 IP 和端口，也没有跨域问题。
// ============================================================
// ============================================================
// 2. DOM 引用
//    页面加载时统一取一次，后续直接使用常量，避免反复查询。
//    末尾的 ! 是"非空断言"：告诉 TS 这个 id 一定存在。
// ============================================================
const loginView = document.getElementById("loginView");
const consoleView = document.getElementById("consoleView");
const loginForm = document.getElementById("loginForm");
const loginBtn = document.getElementById("loginBtn");
const loginMsg = document.getElementById("loginMsg");
const accountInput = document.getElementById("accountInput");
const passwordInput = document.getElementById("passwordInput");
const statusText = document.getElementById("statusText");
const deviceList = document.getElementById("deviceList");
const frameImg = document.getElementById("frameImg");
const frameHint = document.getElementById("frameHint");
const frameDevice = document.getElementById("frameDevice");
const photoBtn = document.getElementById("photoBtn");
const pauseBtn = document.getElementById("pauseBtn");
const actionMsg = document.getElementById("actionMsg");
const gallery = document.getElementById("gallery");
const galleryEmpty = document.getElementById("galleryEmpty");
const galleryCount = document.getElementById("galleryCount");
// ============================================================
// 3. 运行状态
// ============================================================
let selectedId = null; // 当前选中的设备；null = 没选
let frameTimer; // 画面轮询定时器的句柄
let streaming = false; // 是否正在拉取实时画面
let msgTimer; // 提示文字自动消失的定时器
// ============================================================
// 4. 通用工具
// ============================================================
/**
 * 发一个 JSON 请求。
 * 失败（HTTP 非 2xx）时抛出异常的 message = 后端的 msg，调用方统一 catch。
 */
async function api(path, init) {
    const res = await fetch(path, init);
    if (!res.ok) {
        // 后端错误统一是 {"code":4xx,"msg":"..."}，可能解析失败，所以要兜底
        let message = `请求失败（HTTP ${res.status}）`;
        try {
            const err = (await res.json());
            if (err.msg)
                message = err.msg;
        }
        catch {
            // 响应体不是 JSON，用默认提示
        }
        throw new Error(message);
    }
    return (await res.json());
}
/** 在视频浮层里显示一条提示，3 秒后自动消失 */
function showMsg(text, ok = false) {
    actionMsg.textContent = text;
    actionMsg.classList.toggle("ok", ok);
    if (msgTimer !== undefined)
        clearTimeout(msgTimer); // 先清旧的，防止叠加闪烁
    msgTimer = window.setTimeout(() => {
        actionMsg.textContent = "";
        actionMsg.classList.remove("ok");
    }, 3000);
}
// ============================================================
// 5. 登录 / 退出
// ============================================================
async function login(event) {
    event.preventDefault(); // 阻止表单默认的整页跳转
    const account = accountInput.value.trim();
    const password = passwordInput.value;
    if (!account || !password) {
        loginMsg.textContent = "请先填写账号和密码";
        return;
    }
    loginMsg.textContent = "";
    loginBtn.disabled = true;
    try {
        await api("/api/login", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ Account: account, Password: password }),
        });
        // 成功：切视图 + 拉设备
        passwordInput.value = "";
        loginView.classList.add("hidden");
        consoleView.classList.remove("hidden");
        await loadDevices();
    }
    catch (err) {
        // 401 时后端返回 {"msg":"账号或密码错误"}
        loginMsg.textContent = err instanceof Error ? err.message : "登录失败";
    }
    finally {
        loginBtn.disabled = false; // 无论成败都要恢复按钮
    }
}
function logout() {
    stopFramePolling();
    selectedId = null;
    streaming = false;
    // 重置控制台状态，避免下次进来残留在上一次的样子
    deviceList.innerHTML = "";
    gallery.innerHTML = "";
    galleryEmpty.style.display = "";
    galleryCount.textContent = "0 张";
    frameDevice.textContent = "未选择";
    frameImg.hidden = true;
    frameImg.removeAttribute("src");
    frameHint.hidden = false;
    frameHint.textContent = "请先在左侧选择设备";
    photoBtn.disabled = true;
    pauseBtn.disabled = true;
    pauseBtn.textContent = "暂停";
    actionMsg.textContent = "";
    statusText.textContent = "未连接";
    statusText.classList.remove("online");
    consoleView.classList.add("hidden");
    loginView.classList.remove("hidden");
}
// ============================================================
// 6. 设备列表
// ============================================================
async function loadDevices() {
    try {
        const data = await api("/api/camera/list");
        const devices = data.devices ?? [];
        // 顶栏状态
        statusText.textContent = devices.length ? `在线 ${devices.length} 台` : "无在线设备";
        statusText.classList.toggle("online", devices.length > 0);
        // 重建列表
        deviceList.innerHTML = "";
        if (devices.length === 0) {
            const p = document.createElement("p");
            p.className = "placeholder";
            p.textContent = "暂无在线设备\n等待终端通过 UDP 心跳注册";
            deviceList.appendChild(p);
            stopFramePolling();
            return;
        }
        for (const id of devices) {
            const btn = document.createElement("button");
            btn.type = "button";
            btn.className = "device-item";
            btn.dataset.id = String(id); // 用 dataset 存 ID，后续比较更可靠
            btn.textContent = `摄像头 ${id}`;
            btn.addEventListener("click", () => selectDevice(id));
            deviceList.appendChild(btn);
        }
        // 之前选中的设备还在线就保持，否则自动选第一台
        const keep = selectedId !== null && devices.includes(selectedId);
        if (!keep)
            selectDevice(devices[0]);
    }
    catch (err) {
        deviceList.innerHTML = "";
        const p = document.createElement("p");
        p.className = "placeholder";
        p.textContent = err instanceof Error ? err.message : "设备列表加载失败";
        deviceList.appendChild(p);
    }
}
function selectDevice(id) {
    selectedId = id;
    // 高亮当前设备
    deviceList.querySelectorAll(".device-item").forEach((el) => {
        el.classList.toggle("active", el.dataset.id === String(id));
    });
    frameDevice.textContent = `摄像头 ${id}`;
    frameHint.hidden = false;
    frameHint.textContent = "正在获取画面…";
    frameImg.hidden = true;
    photoBtn.disabled = false;
    pauseBtn.disabled = false;
    pauseBtn.textContent = "暂停";
    streaming = true;
    startFramePolling();
    void loadPhotos(id); // void: 明确表示"不用等它"
}
// ============================================================
// 7. 实时画面轮询
// ============================================================
function startFramePolling() {
    stopFramePolling(); // 先清旧的，否则切换设备会叠加定时器
    streaming = true;
    refreshFrame(); // 立刻拉一帧，不用等 1 秒
    frameTimer = window.setInterval(refreshFrame, 1000);
}
function stopFramePolling() {
    if (frameTimer !== undefined) {
        clearInterval(frameTimer);
        frameTimer = undefined;
    }
}
function refreshFrame() {
    if (selectedId === null)
        return;
    // 拼时间戳是为了绕过浏览器缓存：URL 每次不同，才会真的去服务器要新帧
    frameImg.src = `/api/camera/${selectedId}/frame?t=${Date.now()}`;
}
// 图片加载成功/失败的回调：决定显示画面还是显示提示文字
frameImg.addEventListener("load", () => {
    frameImg.hidden = false;
    frameHint.hidden = true;
});
frameImg.addEventListener("error", () => {
    frameImg.hidden = true;
    frameHint.hidden = false;
    frameHint.textContent = "设备暂无可显示画面";
});
function toggleStream() {
    if (selectedId === null)
        return;
    if (streaming) {
        stopFramePolling();
        streaming = false;
        pauseBtn.textContent = "继续";
        frameImg.hidden = true;
        frameHint.hidden = false;
        frameHint.textContent = "画面已暂停";
    }
    else {
        startFramePolling();
        pauseBtn.textContent = "暂停";
    }
}
// ============================================================
// 8. 拍照 + 照片库
// ============================================================
async function takePhoto() {
    if (selectedId === null)
        return;
    photoBtn.disabled = true;
    try {
        const data = await api(`/api/camera/${selectedId}/photo`, { method: "POST" });
        const kb = (data.size / 1024).toFixed(1);
        showMsg(`拍照成功：${data.photoId}（${kb} KB）`, true);
        await loadPhotos(selectedId);
    }
    catch (err) {
        showMsg(err instanceof Error ? err.message : "拍照失败");
    }
    finally {
        photoBtn.disabled = false;
    }
}
async function loadPhotos(deviceId) {
    try {
        const data = await api(`/api/photo/list/${deviceId}`);
        const photos = data.photos ?? [];
        gallery.innerHTML = "";
        galleryCount.textContent = `${photos.length} 张`;
        galleryEmpty.style.display = photos.length ? "none" : "";
        for (const photoId of photos) {
            const figure = document.createElement("figure");
            const img = document.createElement("img");
            img.src = `/api/photo/${photoId}`;
            img.alt = photoId;
            img.loading = "lazy"; // 懒加载，照片多时省流量
            const caption = document.createElement("figcaption");
            caption.textContent = photoId; // 用 textContent 而不是 innerHTML，天然防注入
            figure.appendChild(img);
            figure.appendChild(caption);
            gallery.appendChild(figure);
        }
    }
    catch (err) {
        showMsg(err instanceof Error ? err.message : "照片列表加载失败");
    }
}
// ============================================================
// 9. 事件绑定（文件末尾统一挂载）
// ============================================================
loginForm.addEventListener("submit", login);
photoBtn.addEventListener("click", () => void takePhoto());
pauseBtn.addEventListener("click", toggleStream);
document.getElementById("logoutBtn").addEventListener("click", logout);
document.getElementById("refreshBtn").addEventListener("click", () => void loadDevices());
console.log("[app.js] 前端已加载");
