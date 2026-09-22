using DenClock.Server;
using Microsoft.Extensions.Logging.Abstractions;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

int count = 0;
void Check(bool condition, string name) { if (!condition) throw new Exception(name); count++; }
const string config = """
{"temperatureUnit":"F","sensors":[
{"internalName":"den","displayName":"Den","position":"upperLeft","maxAgeSeconds":60},
{"internalName":"porch","displayName":"Outside","position":"upperRight","maxAgeSeconds":120},
{"internalName":"attic","displayName":"Attic","position":"secondary","maxAgeSeconds":300}]}
""";
void Invalid(string text)
{
    try { PresentationConfiguration.Parse(text); throw new Exception("Invalid configuration accepted: " + text); }
    catch (InvalidDataException) { count++; }
}
var configuration = PresentationConfiguration.Parse(config);
Check(configuration.Sensors.Count == 3 && configuration.TemperatureUnit == "F", "valid configuration");
Check(PresentationConfiguration.Parse("""{"temperatureUnit":"C","sensors":[]}""").Sensors.Count == 0, "empty sensors");
foreach (var text in new[] { "{}", "[]", "null", "not-json", config.Replace("\"F\"", "\"f\""),
    config.Replace("60", "0"), config.Replace("60", "-1"), config.Replace("60", "1e999"),
    config.Replace("60", "\"60\""), config.Replace("upperRight", "upperLeft"),
    config.Replace("upperRight", "elsewhere"), config.Replace("\"porch\"", "\"den\""),
    config.Replace("\"Den\"", "\"\""), config.Replace("\"den\"", "\"a/b\""),
    config.Replace("\"temperatureUnit\":\"F\"", "\"temperatureUnit\":\"F\",\"temperatureUnit\":\"C\"") })
    Invalid(text);
foreach (var field in new[] { "internalName", "displayName", "position", "maxAgeSeconds" })
{
    var node = JsonNode.Parse(config)!; node["sensors"]![0]!.AsObject().Remove(field); Invalid(node.ToJsonString());
}
try { PresentationConfiguration.Load(Path.Combine(Path.GetTempPath(), Guid.NewGuid() + ".json")); throw new Exception("missing file accepted"); }
catch (InvalidDataException ex) { Check(ex.Message.Contains("Presentation configuration"), "missing file diagnostic"); }

var state = new PresentationState(configuration, NullLogger<PresentationState>.Instance);
Check(state.Snapshot().Slots.All(s => s.Observation is null), "startup unknown");
var stamp = DateTimeOffset.UtcNow.AddMinutes(-5);
string Report(string name, DateTimeOffset timestamp, object measurements) => JsonSerializer.Serialize(new {
    messageType = "sensor-report", node = new { name }, receivedUtc = timestamp, measurements
});
object Values(double temperature = 22.15, double humidity = 52) => new {
    temperature = new { value = temperature, units = "C" },
    relativeHumidity = new { value = humidity, units = "percent" }
};
bool Send(string name, string text) => state.Receive("lora/sensors/" + name, Encoding.UTF8.GetBytes(text));
Check(Send("porch", Report("porch", stamp, Values())), "retained report");
var snap = state.Snapshot();
Check(snap.Slots[1].Observation!.TemperatureC == 22.15 && snap.Slots[1].Observation!.ReceivedUtc == stamp, "canonical value and original timestamp");
Check(!Send("porch", Report("porch", stamp, Values(100))), "same timestamp does not overwrite");
Check(!Send("porch", Report("porch", stamp.AddSeconds(-1), Values(100))), "older observation ignored");
Check(!Send("porch", Report("den", stamp.AddSeconds(1), Values())), "identity mismatch");
Check(!Send("unknown", Report("unknown", stamp, Values())), "unconfigured sensor");
Check(!Send("porch", "{"), "malformed JSON");
foreach (var malformed in new[] {
    Report("porch", stamp, Values()).Replace("sensor-report","other"),
    Report("porch", stamp, Values()).Replace(stamp.ToString("O"),"bad"),
    """{"messageType":"sensor-report","node":{"name":"porch"},"receivedUtc":"2026-01-01T00:00:00","measurements":{}}""",
    Report("porch", DateTimeOffset.UtcNow.AddDays(1), Values()),
    """{"messageType":"sensor-report","node":{"name":"porch"},"measurements":{}}"""
}) Check(!Send("porch", malformed), "invalid envelope");
Check(state.Snapshot().Slots[1].Observation!.TemperatureC == 22.15, "bad envelope preserves observation");
Check(Send("porch", Report("porch", stamp.AddSeconds(2), new { relativeHumidity = new { value = 55, units = "percent" } })), "partial report accepted");
Check(state.Snapshot().Slots[1].Observation!.TemperatureC is null, "missing temperature is not carried forward");
Check(Send("porch", Report("porch", stamp.AddSeconds(3), new { temperature = new { value = 70, units = "F" }, relativeHumidity = new { value = 101, units = "percent" } })), "invalid measurements in valid envelope");
Check(state.Snapshot().Slots[1].Observation is { TemperatureC: null, RelativeHumidity: null }, "independent unavailable values");
Check(Send("den", Report("den", stamp, Values(-10))), "second sensor");
Check(Send("attic", Report("attic", stamp, Values(15))), "secondary accepted");
Check(state.Snapshot().Slots.Count == 2 && state.Snapshot().Slots[0].Observation!.TemperatureC == -10, "only primary snapshots");
var single = new PresentationState(PresentationConfiguration.Parse(config.Replace("upperLeft","secondary")), NullLogger<PresentationState>.Instance);
Check(single.Snapshot().Slots[0] is { InternalName: null, DisplayName: "", Observation: null, MaxAgeSeconds: null }, "unassigned left");
Console.WriteLine($"PASS: {count} configuration and sensor-state checks");
