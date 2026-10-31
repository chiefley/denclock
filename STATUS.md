# DenClock Status

The .NET 10 time server is implemented; repository history reports it deployed
on the NUC and available on the subnet. These deployment observations have not
been re-tested during documentation work. Existing notes report the NUC and
touchscreen installed, Windows 11 autologon, RustDesk access, and host DenClock.

The browser time client is implemented and accepted. The server now hosts its
assets. Codex has not independently reverified the NUC/LAN browser or touchscreen.
The remote LoRa sensor and diagnostic gateway are implemented and accepted by
the developer. Temperature, humidity, and battery reports work over LoRa.
The independent gateway-to-LoRa-Service Wi-Fi/TCP receive path is now implemented
and accepted by the developer after testing and verification. MQTT publishing is implemented and accepted by the developer. LoRaService now
publishes numeric gateway/node identity without semantic mapping; DenClock owns
sensor and gateway names in its presentation settings. Production
Mosquitto deployment and live-sensor MQTT publication on the NUC are now verified by the developer, including retained-state persistence and restart/reboot recovery. DenClock MQTT subscription and two primary sensor panels are implemented in story 008; the developer has verified and accepted the story. Remote-sensor deep sleep is
verified and accepted by the developer. Sensors now support persistent serial
configuration of identity, reporting parameters, and transmit power; see the
[sensor configuration guide](firmware/sensors/README.md#persistent-sensor-configuration).

Gateway household Wi-Fi now supports temporary access-point/browser provisioning
with device-specific setup credentials and physical recovery; see the
[gateway setup guide](firmware/sensors/README.md#set-up-gateway-wi-fi).

Gateway `.local` hostname resolution is implemented and fully accepted. The
ESP32 mDNS client queries the existing Windows hostname without advertising a
gateway name or services; ordinary DNS and IPv4 remain supported. See the
[accepted hostname story](lora-service/docs/stories/completed/004-reliable-mdns-hostname-resolution.md)
and [deployment guide](lora-service/docs/mdns-deployment.md).

The developer has verified and accepted the transparent 24-hour client forecast
and the Linux deployment helper for bensclock. See the
[client guide](client/README.md#hourly-forecast) and
[Linux deployment guide](docs/deploy-to-bensclock.md). The existing Windows
deployment procedures remain available.

## Resume Here

- [Move Gateway and Sensor Semantic Mapping into DenClock](docs/stories/completed/010-move-gateway-and-sensor-semantic-mapping-into-denclock.md)
  is accepted. DenClock consumes numeric MQTT identity and owns mappings in the
  existing settings dialog. Existing installations require a
  [one-time settings adjustment](server/docs/presentation-configuration.md#one-time-configuration-adjustment).

- [Load, Save, and Apply Display Settings](docs/stories/completed/009-load-save-and-apply-display-settings.md)
  is accepted by the developer. Presentation settings now live in ProgramData,
  with first-run migration, revision-protected saves, and runtime application
  through the gear-triggered browser editor. See the
  [server guide](server/README.md#primary-sensor-display) for storage permissions
  and the required development/test directory override.

- [Sensor Change Reporting, Heartbeat, and Randomized Sampling](firmware/sensors/docs/stories/completed/006-sensor-change-reporting-heartbeat-and-randomized-sampling.md)
  is verified and accepted. Nodes suppress stable-temperature reports, retain
  heartbeat/change state through deep sleep, and randomize sampling intervals.

- [Display Primary Sensor Readings](docs/stories/completed/008-display-primary-sensor-readings.md)
  is verified and accepted by the developer. Local automated checks cover
  data flow and presentation; shelf/NUC verification was confirmed by the developer.

- [Remove Semantic Mapping from LoRaService](lora-service/docs/stories/completed/005-remove-semantic-mapping-from-loraservice.md)
  is accepted. MQTT now uses `lora/sensors/{gatewayId}/{nodeId}` with integer
  identity fields and fixed measurement normalization. DenClock consumes this
  contract; no compatibility publishing is provided.
  Prior live MQTT deployment checks are recorded in
  [the accepted integration story](docs/stories/completed/007-deploy-mosquitto-and-verify-live-sensor-publishing.md).

- [Support Windows Service Hosting](lora-service/docs/stories/completed/001-support-windows-service-hosting.md)
  is verified and accepted by the developer for Windows Service hosting on the NUC.

- [Remote Sensor Deep-Sleep Operation](firmware/sensors/docs/stories/completed/003-remote-sensor-deep-sleep-operation.md)
  is verified and accepted; see the sensor README for operation and checks.

- [End-to-End LoRa Receive Path](docs/stories/completed/004-end-to-end-lora-receive-path.md)
  is tested, verified, and accepted. See the [LoRa Service README](lora-service/README.md) for
  independent console startup, gateway configuration, and hardware acceptance.

- [Remote LoRa Sensor](firmware/sensors/docs/stories/completed/002-implement-remote-lora-sensor.md)
  is accepted. See the [firmware README](firmware/sensors/README.md) for builds
  and bench operation.

- [Client development and deployment workflow](client/docs/stories/completed/002-client-development-and-deployment-workflow.md)
  is accepted. The developer confirmed client recovery after stopping and
  restarting the NUC time service. See the client README for development setup.

- [Story-driven workflow](docs/DEVELOPMENT.md) is the enduring process reference.
- [Workflow story](docs/stories/completed/002-establish-story-driven-development-workflow.md)
  is accepted.
- [Time Display](client/docs/stories/completed/001-time-display.md) is accepted;
  run and deployment-verification instructions are in the client README.

See the [server README](server/README.md) for listener/deployment uncertainties,
[client README](client/README.md) for browser hosting and display decisions,
[architecture](docs/ARCHITECTURE.md), and [sensor firmware](firmware/sensors/README.md).
Active work lives directly in each `docs/stories/`; accepted work lives in its
`completed/` subdirectory.
