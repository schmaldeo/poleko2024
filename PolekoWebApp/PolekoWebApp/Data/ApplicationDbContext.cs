using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
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
    public DbSet<SensorData> SensorReadings { get; set; }
}

public class Sensor
{
    [JsonIgnore] public int SensorId { get; set; }
    [JsonPropertyName("ip")] public string? IpAddress { get; set; }
    [JsonPropertyName("mac")] public string? MacAddress { get; set; }
    [JsonIgnore] public bool UsesDhcp { get; set; }
    [JsonIgnore] public bool ManuallyStartFetch { get; set; }
    [NotMapped] public int FetchInterval { get; set; }
    [NotMapped] [JsonIgnore] public TcpClient? TcpClient { get; set; }
    [JsonIgnore] public List<SensorData> Readings { get; }
    private SensorData? _lastReading;
    [NotMapped] [JsonIgnore] public SensorData LastReading 
    {
        get => _lastReading ?? new SensorData {Humidity = 0, Temperature = 0};
        set
        {
            LastReadingChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(LastReading)));
            _lastReading = value;
        }
    }

    [NotMapped] [JsonIgnore] public bool Fetching { get; set; }
    
    public event PropertyChangedEventHandler? LastReadingChanged;
    
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
        if (obj is Sensor other)
        {
            return IpAddress == other.IpAddress && MacAddress == other.MacAddress;
        }
        return false;
    }
    
    public override int GetHashCode()
    {
        return HashCode.Combine(IpAddress, MacAddress);
    }
}