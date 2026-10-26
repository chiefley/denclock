const positions = ['upperLeft', 'upperRight'];

// Preserve all sensors and fields the primary-panel editor does not own.
export function buildConfiguration(original, temperatureUnit, slots) {
  if (!['C', 'F'].includes(temperatureUnit)) throw new Error('Choose Celsius or Fahrenheit.');
  const assigned = slots.filter(slot => slot.internalName);
  if (new Set(assigned.map(slot => slot.internalName)).size !== assigned.length)
    throw new Error('Choose a different sensor for each position.');
  const configuration = structuredClone(original);
  configuration.temperatureUnit = temperatureUnit;
  for (const sensor of configuration.sensors) {
    if (positions.includes(sensor.position)) sensor.position = 'secondary';
  }
  for (const slot of assigned) {
    const sensor = configuration.sensors.find(sensor => sensor.internalName === slot.internalName);
    if (!sensor) throw new Error('Choose a configured sensor.');
    if (!slot.displayName.trim()) throw new Error('Each assigned sensor needs a display label.');
    const age = Number(slot.maxAgeSeconds);
    if (!String(slot.maxAgeSeconds).trim() || !Number.isFinite(age) || age <= 0)
      throw new Error('Stale-reading limits must be positive numbers in seconds.');
    Object.assign(sensor, { position: slot.position, displayName: slot.displayName, maxAgeSeconds: age });
  }
  return configuration;
}

export function initializeSettings(root, config, refreshPresentation, fetchSettings = (...args) => fetch(...args)) {
  const get = id => root.getElementById(id);
  const gear = get('settings-button'), dialog = get('settings-view'), form = get('settings-form');
  const unit = get('settings-unit'), save = get('settings-save'), cancel = get('settings-cancel');
  const status = get('settings-status'), reload = get('settings-reload');
  const fields = [...form.querySelectorAll('fieldset')];
  const panels = ['left', 'right'].map((side, index) => ({
    position: positions[index],
    assignment: get(`settings-${side}-sensor`), label: get(`settings-${side}-label`), age: get(`settings-${side}-age`),
  }));
  const endpoint = config.serverAddress.replace(/\/$/, '') + '/api/settings/presentation';
  let loaded = null, draftSensors = null, saving = false, generation = 0, loadController = null;

  function message(text, focus = false) {
    status.textContent = text;
    if (focus) status.focus();
  }
  function enableControls() {
    unit.disabled = !loaded || saving;
    for (const field of fields) field.disabled = !loaded || saving;
    for (const panel of panels) {
      panel.label.disabled = panel.age.disabled = !panel.assignment.value || !loaded || saving;
    }
    save.disabled = !loaded || saving;
    cancel.disabled = saving;
    reload.disabled = saving;
    form.setAttribute('aria-busy', String(saving || (!loaded && loadController !== null)));
  }
  function populatePanel(panel) {
    const sensor = draftSensors?.find(sensor => sensor.internalName === panel.assignment.value);
    panel.label.value = sensor?.displayName ?? '';
    panel.age.value = sensor?.maxAgeSeconds ?? '';
    enableControls();
  }
  async function load() {
    const session = ++generation;
    loadController?.abort();
    const controller = loadController = new AbortController();
    loaded = draftSensors = null;
    reload.hidden = true;
    unit.value = '';
    for (const panel of panels) {
      panel.assignment.replaceChildren(new Option('Unassigned', ''));
      populatePanel(panel);
    }
    enableControls();
    message('Loading settings…', true);
    const timeout = setTimeout(() => controller.abort(), config.requestTimeoutMs);
    try {
      const response = await fetchSettings(endpoint, { cache: 'no-store', signal: controller.signal });
      if (!response.ok) throw new Error('Could not load settings from the service.');
      const result = await response.json();
      if (!result?.revision || !Array.isArray(result.configuration?.sensors)
          || !['C', 'F'].includes(result.configuration.temperatureUnit)) throw new Error('The service returned invalid settings.');
      if (generation !== session || !dialog.open) return;
      loaded = result;
      draftSensors = structuredClone(result.configuration.sensors);
      unit.value = result.configuration.temperatureUnit;
      for (const panel of panels) {
        for (const sensor of draftSensors)
          panel.assignment.add(new Option(`${sensor.displayName} (${sensor.internalName})`, sensor.internalName));
        panel.assignment.value = draftSensors.find(sensor => sensor.position === panel.position)?.internalName ?? '';
        populatePanel(panel);
      }
      message('Edit display settings. Changes apply to all displays only after Save succeeds.');
      unit.focus();
    } catch (error) {
      if (generation !== session || !dialog.open) return;
      message(error.name === 'AbortError' ? 'Loading settings timed out. Try reloading.' : error.message, true);
      reload.hidden = false;
    } finally {
      clearTimeout(timeout);
      if (generation === session) { loadController = null; enableControls(); }
    }
  }

  for (const panel of panels) {
    panel.assignment.addEventListener('change', () => populatePanel(panel));
    for (const input of [panel.label, panel.age]) input.addEventListener('input', () => {
      const sensor = draftSensors?.find(sensor => sensor.internalName === panel.assignment.value);
      if (sensor) { sensor.displayName = panel.label.value; sensor.maxAgeSeconds = panel.age.value; }
    });
  }
  gear.addEventListener('click', () => { dialog.showModal(); dialog.scrollTop = 0; void load(); });
  reload.addEventListener('click', () => void load());
  cancel.addEventListener('click', () => { if (!saving) dialog.close(); });
  dialog.addEventListener('cancel', event => { if (saving) event.preventDefault(); });
  dialog.addEventListener('close', () => {
    ++generation;
    loadController?.abort();
    loadController = loaded = draftSensors = null;
    form.reset();
    for (const panel of panels) {
      panel.assignment.replaceChildren(new Option('Unassigned', ''));
      panel.label.value = panel.age.value = '';
    }
    enableControls();
    gear.focus();
  });
  form.addEventListener('submit', async event => {
    event.preventDefault();
    if (!loaded || saving) return;
    let configuration;
    try {
      configuration = buildConfiguration(loaded.configuration, unit.value, panels.map(panel => ({
        position: panel.position, internalName: panel.assignment.value,
        displayName: panel.label.value, maxAgeSeconds: panel.age.value,
      })));
    } catch (error) { message(error.message, true); return; }
    saving = true;
    enableControls();
    reload.hidden = true;
    message('Saving settings… Please wait.');
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), config.requestTimeoutMs);
    try {
      const response = await fetchSettings(endpoint, {
        method: 'PUT', cache: 'no-store', signal: controller.signal,
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ revision: loaded.revision, configuration }),
      });
      if (!response.ok) {
        let error;
        try { error = (await response.json()).error; } catch { /* Fall back to HTTP status. */ }
        if (response.status === 409) {
          reload.hidden = false;
          throw new Error('Another client changed settings. Your draft is still here. Reload service settings to discard it and start again.');
        }
        throw new Error(error || `Could not save settings (HTTP ${response.status}).`);
      }
      saving = false;
      dialog.close();
      // Polling and clock synchronization never stop. Refresh after any older poll.
      void refreshPresentation();
    } catch (error) {
      message(error.name === 'AbortError' || error instanceof TypeError
        ? 'Save could not be confirmed. Your draft is still here. Reload service settings to check what was saved before trying again.'
        : error.message, true);
      if (error.name === 'AbortError' || error instanceof TypeError) reload.hidden = false;
    } finally {
      clearTimeout(timeout);
      saving = false;
      enableControls();
    }
  });
}
