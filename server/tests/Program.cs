using DenClock.Server;
using Microsoft.Extensions.Logging.Abstractions;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

int count = 0;
void Check(bool condition, string name) { if (!condition) throw new Exception(name); count++; }
const string config = """
{"temperatureUnit":"F","sensors":[
{"gatewayId":1,"nodeId":1,"name":"den","displayName":"Den","position":"upperLeft","maxAgeSeconds":60},
{"gatewayId":2,"nodeId":1,"name":"porch","displayName":"Outside","position":"upperRight","maxAgeSeconds":120},
{"gatewayId":1,"nodeId":3,"name":"attic","displayName":"Attic","position":"secondary","maxAgeSeconds":300}]}
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
    config.Replace("\"Den\"", "\"\""),
    config.Replace("\"temperatureUnit\":\"F\"", "\"temperatureUnit\":\"F\",\"temperatureUnit\":\"C\"") })
    Invalid(text);
foreach (var field in new[] { "gatewayId", "nodeId", "name", "displayName", "position", "maxAgeSeconds" })
{
    var node = JsonNode.Parse(config)!; node["sensors"]![0]!.AsObject().Remove(field); Invalid(node.ToJsonString());
}
try { PresentationConfiguration.Load(Path.Combine(Path.GetTempPath(), Guid.NewGuid() + ".json")); throw new Exception("missing file accepted"); }
catch (InvalidDataException ex) { Check(ex.Message.Contains("Presentation configuration"), "missing file diagnostic"); }

var state = new PresentationState(configuration, NullLogger<PresentationState>.Instance);
Check(state.Snapshot().Slots.All(s => s.Observation is null), "startup unknown");
var stamp = DateTimeOffset.UtcNow.AddMinutes(-5);
string Report(string name, DateTimeOffset timestamp, object measurements) => JsonSerializer.Serialize(new {
    messageType = "sensor-report", gatewayId = name == "porch" ? 2 : 1, nodeId = name == "attic" ? 3 : 1, receivedUtc = timestamp, measurements
});
object Values(double temperature = 22.15, double humidity = 52) => new {
    temperature = new { value = temperature, units = "C" },
    relativeHumidity = new { value = humidity, units = "percent" }
};
bool Send(string name, string text) => state.Receive("lora/sensors/" + (name == "porch" ? "2/1" : name == "den" ? "1/1" : name == "attic" ? "1/3" : "9/9"), Encoding.UTF8.GetBytes(text));
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
    """{"messageType":"sensor-report","node":{"gatewayId":2,"nodeId":1,"name":"porch"},"receivedUtc":"2026-01-01T00:00:00","measurements":{}}""",
    Report("porch", DateTimeOffset.UtcNow.AddDays(1), Values()),
    """{"messageType":"sensor-report","node":{"gatewayId":2,"nodeId":1,"name":"porch"},"measurements":{}}"""
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
Check(single.Snapshot().Slots[0] is { Name: null, DisplayName: "", Observation: null, MaxAgeSeconds: null }, "unassigned left");
Check(state.Snapshot().Slots[1].Observation!.TemperatureC is null &&
    state.Snapshot().Slots[0].Observation!.TemperatureC == -10, "identical node IDs across gateways remain distinct");
var renamed = PresentationConfiguration.Parse(config.Replace("\"porch\"", "\"garden\""));
var priorNameChange = state.Snapshot().Slots[1].Observation;
state.ReplaceConfiguration(renamed);
Check(state.Snapshot().Slots[1].Name == "garden" && state.Snapshot().Slots[1].Observation == priorNameChange,
    "logical rename preserves physical observation");
state.ReplaceConfiguration(PresentationConfiguration.Parse(config.Replace("\"gatewayId\":2", "\"gatewayId\":3")));
Check(state.Snapshot().Slots[1].Observation is null && state.Snapshot().Slots[0].Observation!.TemperatureC == -10,
    "changing numeric identity cannot reuse another sensor's reading");
state.ReplaceConfiguration(configuration);
foreach (var replacement in new[] { "0", "256", "1.5", "\"1\"" })
    Invalid(config.Replace("\"gatewayId\":1", "\"gatewayId\":" + replacement));
Invalid(config.Replace("\"nodeId\":3", "\"nodeId\":1"));
Invalid(config.Replace("\"nodeId\":3", "\"nodeId\":240"));
var gatewaysJson = config.Replace("\"sensors\":", "\"gateways\":[{\"gatewayId\":1,\"name\":\"house\",\"displayName\":\"House gateway\"},{\"gatewayId\":2}],\"sensors\":");
Check(PresentationConfiguration.Parse(gatewaysJson).Gateways[1].Name is null, "gateway names optional");
Invalid(gatewaysJson.Replace("\"gatewayId\":2}", "\"gatewayId\":1}"));
Invalid(gatewaysJson.Replace("\"gatewayId\":2}", "\"gatewayId\":2,\"name\":\"house\"}"));
// Runtime replacement retains identities and timestamps, removes deleted identities.
var before = state.Snapshot().Slots[1].Observation;
var changed = PresentationConfiguration.Parse(config.Replace("\"F\"", "\"C\"")
    .Replace("upperLeft", "secondary").Replace("upperRight", "upperLeft").Replace("Outside", "Garden").Replace("120", "30"));
state.ReplaceConfiguration(changed);
var applied = state.Snapshot();
Check(applied.TemperatureUnit == "C" && applied.Slots[0].DisplayName == "Garden" && applied.Slots[0].MaxAgeSeconds == 30,
    "all configuration fields applied together");
Check(applied.Slots[0].Observation == before, "runtime change retains observation and timestamp");
state.ReplaceConfiguration(PresentationConfiguration.Parse("""{"temperatureUnit":"F","sensors":[]}"""));
Check(!Send("porch", Report("porch", stamp.AddSeconds(10), Values())), "removed identity ignored");
state.ReplaceConfiguration(configuration);
Check(state.Snapshot().Slots.All(s => s.Observation is null), "removed and re-added identities start unknown");

var inconsistent = 0;
Parallel.Invoke(
    () => { for (var i = 0; i < 500; i++) state.ReplaceConfiguration(i % 2 == 0 ? configuration : changed); },
    () => {
        for (var i = 0; i < 500; i++)
        {
            Send("porch", Report("porch", stamp.AddMilliseconds(i), Values()));
            var snapshot = state.Snapshot();
            if (snapshot.TemperatureUnit == "C" ? snapshot.Slots[0].DisplayName != "Garden" || snapshot.Slots[0].MaxAgeSeconds != 30
                : snapshot.Slots[0].DisplayName != "Den" || snapshot.Slots[0].MaxAgeSeconds != 60)
                Interlocked.Increment(ref inconsistent);
        }
    });
Check(inconsistent == 0, "configuration snapshots remain coherent during replacement and MQTT reception");

var temporary = Path.Combine(Path.GetTempPath(), "denclock-settings-" + Guid.NewGuid());
Directory.CreateDirectory(temporary);
try
{
    var legacy = Path.Combine(temporary, "legacy.json");
    var directory = Path.Combine(temporary, "data");
    var file = Path.Combine(directory, "presentation.json");
    var original = gatewaysJson.Replace("\"sensors\":", "\"future\":{\"flag\":true},\"sensors\":")
        .Replace("\"displayName\":\"Attic\"", "\"extra\":42,\"displayName\":\"Attic\"");
    File.WriteAllText(legacy, original);
    var store = new PresentationSettings(directory, legacy);
    Check(File.ReadAllText(file) == original && File.ReadAllText(legacy) == original, "migration preserves exact legacy content");
    var first = store.Read();
    File.WriteAllText(legacy, "invalid bundled replacement");
    var restarted = new PresentationSettings(directory, legacy);
    Check(restarted.Read().Configuration.Sensors.Count == 3, "stored configuration wins over changed deployment bundle");
    var live = new PresentationState(first.Configuration, NullLogger<PresentationState>.Instance);
    var saved = store.Save(first.Revision, original.Replace("\"F\"", "\"C\""), live);
    Check(saved.Revision != first.Revision && live.Snapshot().TemperatureUnit == "C", "persist then publish with new revision");
    var persisted = File.ReadAllText(file);
    Check(JsonNode.Parse(persisted)!["future"]!["flag"]!.GetValue<bool>() &&
        JsonNode.Parse(persisted)!["sensors"]![2]!["extra"]!.GetValue<int>() == 42, "unknown fields survive save");
    var restartConfiguration = new PresentationSettings(directory, legacy).Read().Configuration;
    Check(restartConfiguration.TemperatureUnit == "C" && restartConfiguration.Gateways[0].DisplayName == "House gateway" &&
        restartConfiguration.Sensors[1].Identity == new SensorIdentity(2, 1), "saved numeric mappings and gateway names survive restart");
    try { store.Save(first.Revision, config, live); throw new Exception("stale save accepted"); }
    catch (SettingsConflictException) { count++; }
    try { store.Save(saved.Revision, "{}", live); throw new Exception("invalid save accepted"); }
    catch (InvalidDataException) { count++; }
    Check(File.ReadAllText(file) == persisted && store.Read().Revision == saved.Revision && live.Snapshot().TemperatureUnit == "C",
        "conflict and validation failures preserve disk and active settings");
    Directory.Move(directory, directory + "-away");
    try { store.Save(saved.Revision, config, live); throw new Exception("failed persistence accepted"); }
    catch (IOException) { count++; }
    Check(File.ReadAllText(Path.Combine(directory + "-away", "presentation.json")) == persisted &&
        store.Read().Revision == saved.Revision && live.Snapshot().TemperatureUnit == "C", "failed write preserves file revision and state");
    Directory.Move(directory + "-away", directory);
    Check(Directory.GetFiles(directory).Length == 1, "temporary files cleaned up");
    // Two simultaneous clients at one revision: exactly one save succeeds.
    var successes = 0; var conflicts = 0;
    Parallel.For(0, 2, _ => {
        try { store.Save(saved.Revision, config, live); Interlocked.Increment(ref successes); }
        catch (SettingsConflictException) { Interlocked.Increment(ref conflicts); }
    });
    Check(successes == 1 && conflicts == 1, "simultaneous saves serialized");
    File.WriteAllText(file, "{}");
    try { _ = new PresentationSettings(directory, legacy); throw new Exception("invalid authoritative file accepted"); }
    catch (InvalidDataException ex) { Check(ex.Message.Contains(file), "invalid authoritative path diagnostic"); }
    Check(File.ReadAllText(file) == "{}", "invalid file not replaced");
    File.Delete(file);
    try { _ = new PresentationSettings(directory, legacy); throw new Exception("invalid migration source accepted"); }
    catch (InvalidDataException ex) { Check(ex.Message.Contains(legacy), "invalid legacy path diagnostic"); }
    Check(!File.Exists(file), "invalid migration does not create destination");
    File.Delete(legacy);
    var defaults = new PresentationSettings(directory, legacy).Read().Configuration;
    Check(defaults.TemperatureUnit == "F" && defaults.Sensors.Count == 0, "missing files initialize F with no sensors");
    var blocked = Path.Combine(temporary, "blocked");
    File.WriteAllText(blocked, "not a directory");
    try { _ = new PresentationSettings(blocked, legacy); throw new Exception("initialization failure accepted"); }
    catch (InvalidDataException ex) { Check(ex.Message.Contains(blocked), "initialization failure has destination diagnostic"); }
}
finally { Directory.Delete(temporary, recursive: true); }
Console.WriteLine($"PASS: {count} configuration, persistence, and sensor-state checks");
