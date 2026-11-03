// Native window control is available only on the clock's own loopback page.
export function initializeWindowControl(doc, {
    fetcher = globalThis.fetch,
    hostname = globalThis.location?.hostname,
    schedule = globalThis.setTimeout,
} = {}) {
    const button = doc.getElementById('window-button');
    const status = doc.getElementById('window-status');
    if (!['localhost', '127.0.0.1'].includes(hostname)) return;
    button.hidden = false;
    let state = null;
    let busy = false;

    function render() {
        button.disabled = busy || state === null;
        const label = state ? 'Restore window' : 'Full screen';
        button.title = label;
        button.setAttribute('aria-label', label);
        button.setAttribute('aria-pressed', String(state === true));
        button.classList.toggle('is-fullscreen', state === true);
    }

    async function request(desired) {
        busy = true;
        render();
        try {
            const options = { cache: 'no-store', signal: AbortSignal.timeout(4000) };
            if (typeof desired === 'boolean') {
                options.method = 'POST';
                options.headers = { 'Content-Type': 'application/json' };
                options.body = JSON.stringify({ fullscreen: desired });
            }
            const response = await fetcher('http://127.0.0.1:5001/window', options);
            if (!response.ok) throw new Error('Window control unavailable');
            const data = await response.json();
            if (typeof data.fullscreen !== 'boolean') throw new Error('Invalid window state');
            state = data.fullscreen;
            status.textContent = '';
        } catch {
            state = null;
            status.textContent = 'Window control unavailable';
        } finally {
            busy = false;
            render();
        }
    }

    button.addEventListener('click', () => {
        if (!busy && state !== null) void request(!state);
    });
    // Poll observed desktop state, so F11 and window-manager controls stay in sync.
    async function poll() {
        if (!busy) await request();
        schedule(poll, 2000);
    }
    render();
    void poll();
}
