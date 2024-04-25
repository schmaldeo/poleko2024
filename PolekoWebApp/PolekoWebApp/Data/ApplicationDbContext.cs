using System.ComponentModel;
using System.ComponentModel.DataAnnotations.Schema;
using System.Net.Sockets;
using System.Text.Json.Serialization;
using Microsoft.AspNetCore.Identity.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore;

namespace PolekoWebApp.Data;

public class ApplicationDbContext(DbContextOptions<ApplicationDbContext> options)
    : IdentityDbContext<ApplicationUser>(options)
{
    public DbSet<Sensor> Sensors { get; set; }
    public DbSet<SensorReading> SensorReadings { get; set; }
}

public class Sensor
{
    private bool _error;

    private bool _fetching;
    
    private SensorReading? _lastReading;
    [JsonIgnore] public int SensorId { get; set; }
    [JsonPropertyName("ip")] public string? IpAddress { get; set; }
    [JsonPropertyName("mac")] public string? MacAddress { get; set; }
    [JsonIgnore] public bool UsesDhcp { get; set; }
    [JsonIgnore] public bool ManuallyStartFetch { get; set; }
    [NotMapped] public int FetchInterval { get; set; }
    [NotMapped] [JsonIgnore] public int? Rssi { get; set; }
    [NotMapped] [JsonIgnore] public TcpClient? TcpClient { get; set; }
    [JsonIgnore] public List<SensorReading> Readings { get; }

    // all the PropertyHasChanged invoking is made so that the NavMenu knows when to rerender to change the sensor
    // status colour
    [NotMapped]
    [JsonIgnore]
    public SensorReading LastReading
    {
        get => _lastReading ?? new SensorReading { Humidity = 0, Temperature = 0 };
        set
        {
            PropertyHasChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(LastReading)));
            _lastReading = value;
        }
    }

    [NotMapped]
    [JsonIgnore]
    public bool Fetching
    {
        get => _fetching;
        set
        {
            PropertyHasChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Fetching)));
            _fetching = value;
        }
    }

    [NotMapped]
    [JsonIgnore]
    public bool Error
    {
        get => _error;
        set
        {
            PropertyHasChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Error)));
            _error = value;
        }
    }

    public event PropertyChangedEventHandler? PropertyHasChanged;

    public static bool operator ==(Sensor a, Sensor b)
    {
        return a.IpAddress == b.IpAddress && a.MacAddress == b.MacAddress;
    }

    public static bool operator !=(Sensor a, Sensor b)
    {
        return a.IpAddress != b.IpAddress || a.MacAddress != b.MacAddress;
    }

    public override bool Equals(object? obj)
    {
        if (obj is Sensor other) return IpAddress == other.IpAddress && MacAddress == other.MacAddress;
        return false;
    }

    public override int GetHashCode()
    {
        return HashCode.Combine(IpAddress, MacAddress);
    }
}