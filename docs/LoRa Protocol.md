LORA-PROTOCOL.md

1. Purpose and scope
   - Protocol carried over LoRa
   - Independent of MQTT and applications
   - This document defines the wire representation

2. General packet identification
   - First byte = Message Class
   - Message Class determines remainder of packet structure

3. Message classes
   SENSOR_REPORT
   COMMAND       [reserved / TBD]
   RESPONSE      [reserved / TBD]
   EVENT         [reserved / TBD]

4. SENSOR_REPORT
   Fixed header
       Message Class
       Source Node ID
       Sequence Number

   Payload
       TLV
       TLV
       ...

5. TLV encoding
   Type
   Length
   Value

   Rules:
   - unknown types skipped
   - type definitions immutable
   - malformed-length handling
   - byte order

6. Sensor TLV registry
   Temperature
   Relative humidity
   Battery voltage
   ...

7. Examples
   Complete packet byte dumps with decoding

8. Unresolved issues