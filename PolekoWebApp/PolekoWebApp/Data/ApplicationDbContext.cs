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
    public int SensorId { get; set; }
    public string? IpAddress { get; set; }
    public string? MacAddress { get; set; }
}