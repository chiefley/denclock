export const config = {
  // Empty uses the page's server: LAN hostname during development, localhost on the NUC.
  // An explicit origin (e.g. http://DenClock:5000) requires that server to permit CORS
  // if it differs from the page's origin. The default deployment needs no CORS.
  serverAddress: '',
  syncIntervalMs: 60_000,
  requestTimeoutMs: 5_000,
  staleAfterMs: 180_000,
};
