// Intentionally independent of live clients: preview edits never reach the service.
export function initializeSettings(root) {
  const gear = root.getElementById('settings-button');
  const dialog = root.getElementById('settings-view');
  const form = root.getElementById('settings-form');
  gear.addEventListener('click', () => {
    form.reset();
    dialog.showModal();
    dialog.scrollTop = 0;
  });
  root.getElementById('settings-cancel').addEventListener('click', () => dialog.close());
  // Prevent Enter in an input from submitting or implying that settings were saved.
  form.addEventListener('submit', event => event.preventDefault());
  // Native dialog cancellation handles Escape, then shares this cleanup path.
  dialog.addEventListener('close', () => {
    form.reset();
    gear.focus();
  });
}
