export const config = {
  // Startup overrides this using server-generated client-settings.js.
  // Development uses Client:TimeServerAddress; production always uses the page origin.
  forecastLocation: { latitude: 41.16722, longitude: -81.73889 },
  serverAddress: '',
  presentationIntervalMs: 5_000,
  syncIntervalMs: 60_000,
  requestTimeoutMs: 5_000,
  staleAfterMs: 180_000,
};
