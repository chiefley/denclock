export const sceneImages = {
  day: 'images/lodge-winter-day.png',
  twilight: 'images/Denclock-cabin-master.png',
  night: 'images/lodge-winter-night.png',
};
const HALF_HOUR = 30 * 60_000;

export function selectScene(civilTime, utc, forecast) {
  if (!civilTime || !Number.isFinite(civilTime.getTime())) return 'twilight';
  if (Number.isFinite(utc) && forecast?.timezone) {
    const parts = new Intl.DateTimeFormat('en-US', {
      timeZone: forecast.timezone, year: 'numeric', month: '2-digit', day: '2-digit',
    }).formatToParts(utc);
    const part = type => parts.find(p => p.type === type).value;
    const date = `${part('year')}-${part('month')}-${part('day')}`;
    const solar = forecast.solar?.find(day => day.date === date);
    if (solar && Number.isFinite(solar.sunrise) && Number.isFinite(solar.sunset)
        && solar.sunset > solar.sunrise) {
      if (Math.abs(utc - solar.sunrise) <= HALF_HOUR || Math.abs(utc - solar.sunset) <= HALF_HOUR)
        return 'twilight';
      return utc > solar.sunrise && utc < solar.sunset ? 'day' : 'night';
    }
  }
  const hour = civilTime.getUTCHours() + civilTime.getUTCMinutes() / 60;
  if ((hour >= 6 && hour < 7) || (hour >= 18 && hour < 19)) return 'twilight';
  return hour >= 7 && hour < 18 ? 'day' : 'night';
}

function preloadImage(url) {
  const image = new Image();
  image.src = url;
  return image.decode();
}

// Keep one fully opaque scene under the incoming image throughout its fade.
// Serialize transitions; rapid selections replace only the pending destination.
export class BackgroundDisplay {
  constructor(base, overlay, { preload = preloadImage, animate = (element) =>
    element.animate([{ opacity: 0 }, { opacity: 1 }], { duration: 1000, fill: 'forwards' }).finished } = {}) {
    this.base = base; this.overlay = overlay;
    this.preload = preload; this.animate = animate;
    this.current = 'twilight'; this.desired = 'twilight'; this.running = false;
    this.images = new Map();
  }
  async show(scene) {
    this.desired = scene;
    if (this.running) return;
    this.running = true;
    try {
      while (this.desired !== this.current) {
        const next = this.desired;
        if (!this.images.has(next)) this.images.set(next, this.preload(sceneImages[next]));
        try { await this.images.get(next); }
        catch { this.images.delete(next); break; } // Keep the last good scene; retry on next evaluation.
        if (next !== this.desired) continue;
        this.overlay.style.backgroundImage = `url("${sceneImages[next]}")`;
        try { await this.animate(this.overlay); }
        catch { break; }
        this.base.style.backgroundImage = `url("${sceneImages[next]}")`;
        this.overlay.getAnimations?.().forEach(animation => animation.cancel());
        this.overlay.style.opacity = '0';
        this.current = next;
      }
    } finally { this.running = false; }
  }
}

export function initializeBackground(root, timeClient, forecast) {
  const display = new BackgroundDisplay(root.getElementById('background-base'), root.getElementById('background-next'));
  let mode = 'auto';
  const evaluate = () => {
    const clock = timeClient.sample;
    const utc = clock ? clock.utcMilliseconds + timeClient.now() - clock.receivedAt : null;
    void display.show(mode === 'auto' ? selectScene(timeClient.read().civilTime, utc, forecast.sample) : mode);
  };
  evaluate();
  const timer = setInterval(evaluate, 1000);
  return {
    setMode(value) {
      if (value !== 'auto' && !Object.hasOwn(sceneImages, value)) return;
      mode = value; evaluate();
    },
    dispose() { clearInterval(timer); },
  };
}
