// Temporary visual-evaluation GUI. Remove this module, its markup/style and its
// initialization independently of the automatic background controller.
export function initializeBackgroundPreview(root, background) {
  const select = root.getElementById('background-preview');
  select.value = 'auto';
  select.addEventListener('change', () => background.setMode(select.value));
}
