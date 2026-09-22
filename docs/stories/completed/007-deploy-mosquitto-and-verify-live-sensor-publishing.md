# Deploy Mosquitto and Verify Live Sensor Publishing

## Story

As the DenClock operator, I want a working MQTT broker on the Windows NUC and
verified live sensor publications so that applications can consume the LoRa
Service's existing semantic output.

This is a manual deployment and integration-verification story. The human
developer installs and configures Mosquitto on the NUC and performs the hardware
checks. Codex assists with instructions, interpreting results, and documentation.
Do not install remotely or change application code merely to execute this story.
If verification exposes a defect, report it and agree on the repair scope.

## Current Baseline

The accepted LoRa Service already decodes Gateway Protocol traffic, applies
semantic mapping, and publishes retained QoS 1 JSON. Its automated tests use a
minimal MQTT peer; they do not verify a real broker's retained-message storage.

The path to verify is:

Remote sensor -> LoRa gateway -> TCP -> LoRa Service -> Mosquitto -> diagnostic subscriber.

The gateway uses TCP port 5210 by default and does not connect to MQTT.
The LoRa Service defaults to localhost:1883, with client ID lora-service.
The supplied mapping names Gateway 1 den-gateway and Node 1 porch.

## Governing References

- [MQTT interface](../../MQTT-INTERFACE.md)
- [LoRa Service operation](../../../lora-service/README.md)
- [Implemented service contract](../../../lora-service/docs/contract.md)
- [Architecture](../../ARCHITECTURE.md)
- [Development workflow](../../DEVELOPMENT.md)

## Manual Deployment Scope

1. Install Mosquitto and its diagnostic command-line clients on the Windows NUC.
   Use the current official distribution and installation guidance.
2. Configure and verify Mosquitto's Windows service, automatic startup, actual
   configuration-file location, and writable persistence location.
3. Explicitly choose and document the broker listener. Initial verification may
   use a subscriber on the NUC and a loopback-only listener. If LAN access is
   needed, document the listener and Windows firewall configuration actually used.
   Authentication and TLS remain outside the agreed trusted-local MVP.
4. Enable retained-state persistence and verify it across a broker restart.
   Persistence files belong to the broker, outside the LoRa deployment directory.
5. Confirm the installed LoRa Service build includes semantic mapping and MQTT
   publishing. Confirm its broker host, port, client ID, mapping, and log location.
   localhost refers to the machine running that service or console executable.
6. Keep only one LoRa Service instance on its gateway listener during testing.
   Stop the Windows service before running a console copy, then restore the
   Windows service when console testing is complete.
7. Continue full-output LoRa deployment as already agreed. Do not introduce
   deployment exclusions or configuration/log-preservation changes in this story.

## Acceptance Checks

- Mosquitto runs as a Windows service and starts automatically after NUC reboot.
- The LoRa Service connects successfully and the broker-unavailable warnings stop.
- An independent subscriber requests QoS 1 on lora/sensors/+ and receives live
  sensor-report JSON on lora/sensors/porch (or the documented configured name).
- Payloads include gateway/node identities, sequence, receivedUtc, RSSI/SNR, and
  numeric temperature, relative humidity, and battery voltage with canonical
  units and precision metadata, matching the MQTT contract.
- Compare available sensor diagnostics with received values. Temperature is in C,
  humidity in percent, and battery in V rather than the sensor's integer mV.
  Warming the sensor produces a corresponding change in published temperature.
- Resetting the sensor produces sequence zero; subsequent timer wakes preserve
  progression. Missing reports may cause gaps; every number need not be received.
- After a successful report, stop sensor transmissions and connect a new
  subscriber. It receives the last retained report without a new transmission,
  with the original sequence and receivedUtc unchanged.
- With sensor transmissions still stopped, restart Mosquitto and connect a new
  subscriber. The persisted retained report remains available.
- Resume the sensor, stop Mosquitto, and confirm the LoRa Service remains running
  and retries. Restart Mosquitto and verify publication resumes automatically.
  Reports lost during the outage are not replayed.
- Restart the LoRa Service and verify gateway reconnection and resumed reports.
- Reboot the NUC and verify Mosquitto, the LoRa Service, and the existing DenClock
  service recover; a subscriber receives fresh reports without manual service
  starts. The clock remains functional.
- Record actual results and any failures. A retained reading is last-known state,
  not evidence that a sensor is currently transmitting.

Routine successful measurements are intentionally absent from infrastructure
logs. Use the subscriber for value verification and logs for connection/failure
diagnostics. QoS 1 permits duplicates; this story does not require deduplication.

## Deliverables and Closure

Document the verified broker version, installation/configuration/persistence
paths, Windows service name, listener, startup settings, and repeatable subscriber
and restart checks. Do not record credentials in the repository.

Update relevant operational documentation and root STATUS only after the
corresponding deployment and verification results are established. Report
verification to the developer; explicit human acceptance is required before
moving this story to completed and preparing the commit handoff.

## Exclusions

- DenClock MQTT subscription, environmental display, or history storage.
- Changes to the LoRa or Gateway protocols or semantic message contract.
- Offline queues, replay, deduplication, command traffic, or new logging features.
- Deployment-preservation changes already deferred by the developer.

## Acceptance and Verification

Accepted by the human developer on 2026-09-22.

Mosquitto 2.1.2 is deployed on the Windows NUC with loopback-only access and
persistent retained state. The developer verified live semantic sensor reports,
warming response, sequence zero after reset, retained delivery to new subscribers,
persistence across broker restart, and automatic recovery after broker outage,
LoRa Service restart, and NUC reboot. The DenClock display remained functional.

Operational details are recorded in the [LoRa Service README](../../../lora-service/README.md).
No application changes or departures from the agreed scope were required.
These were developer-performed integration checks. Documentation links and
whitespace were checked during closeout.
