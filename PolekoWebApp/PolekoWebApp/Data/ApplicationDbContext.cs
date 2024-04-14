using System.Text.Json.Serialization;
using Microsoft.AspNetCore.Identity.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore;

namespace PolekoWebApp.Data;

public class ApplicationDbContext(DbContextOptions<ApplicationDbContext> options)
    : IdentityDbContext<ApplicationUser>(options)
{
    public DbSet<Sensor> Sensors { get; set; }
}

public class Sensor
{
    [JsonIgnore]
    public int SensorId { get; set; }
    
    [JsonPropertyName("ip")]
    public string? IpAddress { get; set; }
    
    [JsonPropertyName("mac")]
    public string? MacAddress { get; set; }

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