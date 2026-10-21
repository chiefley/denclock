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
and accepted by the developer after testing and verification. Semantic mapping and MQTT publishing are now implemented and accepted by the developer; production
Mosquitto deployment and live-sensor MQTT publication on the NUC are now verified by the developer, including retained-state persistence and restart/reboot recovery. DenClock MQTT subscription and two primary sensor panels are implemented in story 008; the developer has verified and accepted the story. Remote-sensor deep sleep is
verified and accepted by the developer. Sensors now support persistent serial
configuration of identity, reporting parameters, and transmit power; see the
[sensor configuration guide](firmware/sensors/README.md#persistent-sensor-configuration).

## Resume Here

- [Sensor Change Reporting, Heartbeat, and Randomized Sampling](firmware/sensors/docs/stories/completed/006-sensor-change-reporting-heartbeat-and-randomized-sampling.md)
  is verified and accepted. Nodes suppress stable-temperature reports, retain
  heartbeat/change state through deep sleep, and randomize sampling intervals.

- [Display Primary Sensor Readings](docs/stories/completed/008-display-primary-sensor-readings.md)
  is verified and accepted by the developer. Local automated checks cover
  data flow and presentation; shelf/NUC verification was confirmed by the developer.

- [Implement LoRa Service Semantic Mapping and MQTT Publishing](lora-service/docs/stories/completed/002-implement-semantic-mapping-and-mqtt-publishing.md)
  is implemented and accepted by the developer. Live MQTT deployment checks are recorded in
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
