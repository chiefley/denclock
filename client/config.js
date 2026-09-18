export const config = {
  // Startup overrides this using server-generated client-settings.js.
  // Development uses Client:TimeServerAddress; production always uses the page origin.
  serverAddress: '',
  syncIntervalMs: 60_000,
  requestTimeoutMs: 5_000,
  staleAfterMs: 180_000,
};
