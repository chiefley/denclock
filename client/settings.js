const positions = ['upperLeft', 'upperRight'];
export const sensorKey = sensor => `${sensor.gatewayId}/${sensor.nodeId}`;

export function validateMappings(configuration) {
  const identities = new Set(), names = new Set(), gatewayIds = new Set(), gatewayNames = new Set();
  const id = (value, max) => Number.isInteger(value) && value >= 1 && value <= max;
  for (const sensor of configuration.sensors) {
    if (!id(sensor.gatewayId, 255) || !id(sensor.nodeId, 239)) throw new Error('Enter valid integer gateway (1–255) and node (1–239) IDs.');
    if (!sensor.name?.trim() || !sensor.displayName?.trim())
      throw new Error('Each sensor needs a logical name and display label.');
    if (identities.has(sensorKey(sensor)) || names.has(sensor.name)) throw new Error('Sensor identities and logical names must be unique.');
    identities.add(sensorKey(sensor)); names.add(sensor.name);
    if (!Number.isFinite(Number(sensor.maxAgeSeconds)) || Number(sensor.maxAgeSeconds) <= 0)
      throw new Error('Stale-reading limits must be positive numbers in seconds.');
    sensor.maxAgeSeconds = Number(sensor.maxAgeSeconds);
  }
  for (const gateway of configuration.gateways ?? []) {
    if (!id(gateway.gatewayId, 255)) throw new Error('Enter a gateway ID from 1 to 255.');
    if (gatewayIds.has(gateway.gatewayId) || (gateway.name && gatewayNames.has(gateway.name)))
      throw new Error('Gateway IDs and logical names must be unique.');
    gatewayIds.add(gateway.gatewayId);
    if (gateway.name) gatewayNames.add(gateway.name);
  }
  return configuration;
}

// Preserve all sensors and fields the primary-panel editor does not own.
export function buildConfiguration(original, temperatureUnit, slots) {
  if (!['C', 'F'].includes(temperatureUnit)) throw new Error('Choose Celsius or Fahrenheit.');
  const assigned = slots.filter(slot => slot.sensorKey);
  if (new Set(assigned.map(slot => slot.sensorKey)).size !== assigned.length)
    throw new Error('Choose a different sensor for each position.');
  const configuration = structuredClone(original);
  configuration.temperatureUnit = temperatureUnit;
  for (const sensor of configuration.sensors) {
    if (positions.includes(sensor.position)) sensor.position = 'secondary';
  }
  for (const slot of assigned) {
    const sensor = configuration.sensors.find(sensor => sensorKey(sensor) === slot.sensorKey);
    if (!sensor) throw new Error('Choose a configured sensor.');
    if (!slot.displayName.trim()) throw new Error('Each assigned sensor needs a display label.');
    const age = Number(slot.maxAgeSeconds);
    if (!String(slot.maxAgeSeconds).trim() || !Number.isFinite(age) || age <= 0)
      throw new Error('Stale-reading limits must be positive numbers in seconds.');
    Object.assign(sensor, { position: slot.position, displayName: slot.displayName, maxAgeSeconds: age });
  }
  return validateMappings(configuration);
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
  let loaded = null, draftSensors = null, draftGateways = null, saving = false, generation = 0, loadController = null;

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
    const sensor = draftSensors?.find(sensor => sensorKey(sensor) === panel.assignment.value);
    panel.label.value = sensor?.displayName ?? '';
    panel.age.value = sensor?.maxAgeSeconds ?? '';
    enableControls();
  }
  async function load() {
    const session = ++generation;
    loadController?.abort();
    const controller = loadController = new AbortController();
    loaded = draftSensors = draftGateways = null;
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
      draftGateways = structuredClone(result.configuration.gateways ?? []);
      unit.value = result.configuration.temperatureUnit;
      renderMappings();
      refreshAssignments();
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

  function refreshAssignments() {
    for (const panel of panels) {
      panel.assignment.replaceChildren(new Option('Unassigned', ''));
      for (const sensor of draftSensors ?? [])
        panel.assignment.add(new Option(`${sensor.displayName || 'Unnamed'} (${sensorKey(sensor)} · ${sensor.name || 'no logical name'})`, sensorKey(sensor)));
      const assigned = draftSensors?.find(sensor => sensor.position === panel.position);
      panel.assignment.value = assigned ? sensorKey(assigned) : '';
      populatePanel(panel);
    }
  }
  function renderMappings() {
    for (const [kind, entries, fields] of [
      ['sensors', draftSensors, [['gatewayId', 'Gateway ID', 'number'], ['nodeId', 'Node ID', 'number'], ['name', 'Logical name', 'text'], ['displayName', 'Display label', 'text'], ['maxAgeSeconds', 'Stale limit (seconds)', 'number']]],
      ['gateways', draftGateways, [['gatewayId', 'Gateway ID', 'number'], ['name', 'Logical name (optional)', 'text'], ['displayName', 'Display label (optional)', 'text']]],
    ]) {
      const list = get(`settings-${kind}`);
      list.replaceChildren();
      for (const entry of entries ?? []) {
        const row = root.createElement('div'); row.className = 'mapping-row';
        for (const [key, title, type] of fields) {
          const label = root.createElement('label'); label.textContent = title;
          const input = root.createElement('input'); input.type = type; input.value = entry[key] ?? '';
          if (type === 'number') { input.min = '1'; input.step = key === 'maxAgeSeconds' ? 'any' : '1'; }
          input.addEventListener('input', () => {
            entry[key] = type === 'number' ? Number(input.value) : (kind === 'gateways' && !input.value.trim() ? null : input.value);
            refreshAssignments();
          });
          label.append(input); row.append(label);
        }
        const remove = root.createElement('button'); remove.type = 'button'; remove.textContent = 'Remove';
        remove.addEventListener('click', () => { entries.splice(entries.indexOf(entry), 1); renderMappings(); refreshAssignments(); });
        row.append(remove); list.append(row);
      }
    }
  }
  get('settings-add-sensor').addEventListener('click', () => {
    draftSensors.push({ gatewayId: 1, nodeId: 1, name: '', displayName: '', position: 'secondary', maxAgeSeconds: 60 });
    renderMappings(); refreshAssignments();
  });
  get('settings-add-gateway').addEventListener('click', () => {
    draftGateways.push({ gatewayId: 1, name: null, displayName: null });
    renderMappings();
  });

  for (const panel of panels) {
    panel.assignment.addEventListener('change', () => {
      for (const sensor of draftSensors ?? []) {
        if (sensor.position === panel.position) sensor.position = 'secondary';
        if (sensorKey(sensor) === panel.assignment.value) sensor.position = panel.position;
      }
      populatePanel(panel);
    });
    for (const input of [panel.label, panel.age]) input.addEventListener('input', () => {
      const sensor = draftSensors?.find(sensor => sensorKey(sensor) === panel.assignment.value);
      if (sensor) { sensor.displayName = panel.label.value; sensor.maxAgeSeconds = panel.age.value; renderMappings(); }
    });
  }
  gear.addEventListener('click', () => { dialog.showModal(); dialog.scrollTop = 0; void load(); });
  reload.addEventListener('click', () => void load());
  cancel.addEventListener('click', () => { if (!saving) dialog.close(); });
  dialog.addEventListener('cancel', event => { if (saving) event.preventDefault(); });
  dialog.addEventListener('close', () => {
    ++generation;
    loadController?.abort();
    loadController = loaded = draftSensors = draftGateways = null;
    form.reset();
    for (const panel of panels) {
      panel.assignment.replaceChildren(new Option('Unassigned', ''));
      panel.label.value = panel.age.value = '';
    }
    get('settings-sensors').replaceChildren();
    get('settings-gateways').replaceChildren();
    enableControls();
    gear.focus();
  });
  form.addEventListener('submit', async event => {
    event.preventDefault();
    if (!loaded || saving) return;
    let configuration;
    try {
      configuration = buildConfiguration({ ...loaded.configuration, sensors: draftSensors, gateways: draftGateways }, unit.value, panels.map(panel => ({
        position: panel.position, sensorKey: panel.assignment.value,
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
