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
    [JsonIgnore] public bool OnlyFetchIfMonitoring { get; set; }
    [NotMapped] public int FetchInterval { get; set; }
    [NotMapped] [JsonIgnore] public TcpClient? TcpClient { get; set; }
    [NotMapped] [JsonIgnore] public Buffer<SensorData>? Buffer { get; set; }
    [JsonIgnore] public ICollection<SensorData> Readings { get; }
    // TODO add CurrentReading and use that in Device.razor
    
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