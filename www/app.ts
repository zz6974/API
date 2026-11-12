interface ErrorResponse { code: number; msg: string }
interface LoginOk       { status: string }
interface CameraList    { devices: number[]; count: number }
interface PhotoTaken    { photoId: string; deviceId: number; size: number }
interface PhotoList     { photos: string[] }

// DOM（strict 模式下确定的用 ! 断言；img 要 as HTMLImageElement）
const loginView   = document.getElementById("loginView")!;
const consoleView = document.getElementById("consoleView")!;
const loginForm   = document.getElementById("loginForm") as HTMLFormElement;
const loginMsg    = document.getElementById("loginMsg")!;
const deviceList  = document.getElementById("deviceList")!;
const frameImg    = document.getElementById("frameImg") as HTMLImageElement;
const frameHint   = document.getElementById("frameHint")!;
const frameDevice = document.getElementById("frameDevice")!;
const photoBtn    = document.getElementById("photoBtn") as HTMLButtonElement;
const pauseBtn    = document.getElementById("pauseBtn") as HTMLButtonElement;
const actionMsg   = document.getElementById("actionMsg")!;
const gallery     = document.getElementById("gallery")!;
const galleryEmpty = document.getElementById("galleryEmpty")!;
const galleryCount = document.getElementById("galleryCount")!;
const statusText  = document.getElementById("statusText")!;

// 状态
let selectedId: number | null = null;
let frameTimer: number | undefined;

/** 统一 JSON 请求：非 2xx 抛出后端的 msg；成功返回解析好的数据 */
async function api<T>(path: string, init?: RequestInit): Promise<T> {
    const res = await fetch(path, init);
    if (!res.ok) {
        const err = (await res.json()) as ErrorResponse;
        throw new Error(err.msg || `HTTP ${res.status}`);
    }
    return (await res.json()) as T;
}

/** 在视频浮层里显示一条结果提示，3 秒后自动清掉 */
function showMsg(text: string, ok = false): void {
    actionMsg.textContent = text;
    actionMsg.classList.toggle("ok", ok);
    // TODO: setTimeout 3000ms 后清空（记得先 clearTimeout 防叠加）
}
