using System.Text;
using System.Text.Json;

namespace DenClock.Server;

public sealed record PresentationSettingsSnapshot(string Revision, PresentationConfiguration Configuration);
public sealed class SettingsConflictException() : Exception("Another client changed the settings. Reload before saving again.");

// One lock serializes revision checks, persistence, and publication. MQTT uses only
// PresentationState's short lock, so it keeps receiving during disk writes.
public sealed class PresentationSettings
{
    private static readonly JsonSerializerOptions JsonOptions = new(JsonSerializerDefaults.Web) { WriteIndented = true };
    private readonly object gate = new();
    private readonly string path;
    private PresentationSettingsSnapshot current;

    public PresentationSettings(string directory, string legacyPath)
    {
        path = Path.Combine(Path.GetFullPath(directory), "presentation.json");
        try
        {
            var text = ReadIfPresent(path);
            if (text is null)
            {
                text = ReadIfPresent(legacyPath);
                if (text is not null)
                {
                    try { _ = PresentationConfiguration.Parse(text); }
                    catch (InvalidDataException ex)
                    { throw new InvalidDataException($"Presentation configuration '{legacyPath}': {ex.Message}", ex); }
                }
                else text = "{\"temperatureUnit\":\"F\",\"sensors\":[]}";
                _ = PresentationConfiguration.Parse(text);
                Directory.CreateDirectory(Path.GetDirectoryName(path)!);
                Persist(text, initialize: true);
            }
            current = new(Guid.NewGuid().ToString("N"), PresentationConfiguration.Load(path));
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidDataException)
        {
            throw new InvalidDataException($"Cannot initialize presentation settings at '{path}': {ex.Message}", ex);
        }
    }

    private static string? ReadIfPresent(string file)
    {
        try { return File.ReadAllText(file); }
        catch (Exception ex) when (ex is FileNotFoundException or DirectoryNotFoundException) { return null; }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        { throw new IOException($"Cannot read presentation configuration '{file}': {ex.Message}", ex); }
    }

    public PresentationSettingsSnapshot Read() { lock (gate) return current; }

    public PresentationSettingsSnapshot Save(string revision, string json, PresentationState state)
    {
        var next = PresentationConfiguration.Parse(json);
        var text = JsonSerializer.Serialize(next, JsonOptions);
        lock (gate)
        {
            if (revision != current.Revision) throw new SettingsConflictException();
            Persist(text, initialize: false);
            state.ReplaceConfiguration(next);
            current = new(Guid.NewGuid().ToString("N"), next);
            return current;
        }
    }

    private void Persist(string text, bool initialize)
    {
        var temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            using (var stream = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
            {
                stream.Write(Encoding.UTF8.GetBytes(text));
                stream.Flush(flushToDisk: true);
            }
            if (initialize) File.Move(temporary, path);
            else File.Replace(temporary, path, null);
        }
        finally
        {
            try { File.Delete(temporary); }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { /* Best-effort temp cleanup. */ }
        }
    }
}
